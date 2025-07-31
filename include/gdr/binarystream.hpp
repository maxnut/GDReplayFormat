#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace detail {
    /// @brief Get the bytes of any type.
    template <typename T>
    static std::vector<uint8_t> getBytes(const T& data) noexcept {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &data, sizeof(T));

        if constexpr (std::endian::native == std::endian::little) {
            std::reverse(bytes.begin(), bytes.end());
        }

        return bytes;
    }

    /// @brief Get the bytes of an integral type in a variable-length format
    template <typename T> requires std::is_integral_v<T>
    static std::vector<uint8_t> getBytes(const T& data) noexcept {
        uint64_t value = data;
        std::vector<uint8_t> bytes;

        if (value == 0) {
            bytes.push_back(0);
            return bytes;
        }

        while (value > 0) {
            uint8_t byte = value & 0x7F;
            value >>= 7;
            if (value > 0) {
                byte |= 0x80;
            }
            bytes.push_back(byte);
        }
        return bytes;
    }

    /// @brief Get the bytes of a string with a prefixed length.
    static std::vector<uint8_t> getBytes(const std::string& data) noexcept {
        std::vector<uint8_t> bytes;
        const auto length = getBytes(data.size());
        bytes.insert(bytes.end(), length.begin(), length.end());
        bytes.insert(bytes.end(), data.begin(), data.end());
        return bytes;
    }

    template <size_t N>
    static std::vector<uint8_t> getBytes(const char (&data)[N]) noexcept {
        std::vector<uint8_t> bytes;
        bytes.insert(bytes.end(), data, data + N - 1);
        return bytes;
    }

    /// @brief Read a value from a span of bytes and return the value and the size of the value.
    template <typename T>
    static size_t consume(T& result, std::span<uint8_t> data) noexcept {
        if (data.size() < sizeof(T)) return 0;

        std::memcpy(&result, data.data(), sizeof(T));

        if constexpr (std::endian::native == std::endian::little) {
            std::reverse(reinterpret_cast<uint8_t*>(&result), reinterpret_cast<uint8_t*>(&result) + sizeof(T));
        }

        return sizeof(T);
    }

    /// @brief Read a variable-length value from a span of bytes and return the value and the size of the value.
    template <typename T> requires std::is_integral_v<T>
    static size_t consume(T& result, std::span<uint8_t> data) noexcept {
        result = 0;
        size_t size = 0;
        for (size_t i = 0; i <= sizeof(T); i++) {
            if (i >= data.size()) return 0;
            result |= (data[i] & 0x7F) << (i * 7);
            size++;
            if ((data[i] & 0x80) == 0) {
                break;
            }
        }
        return size;
    }

    /// @brief Read a string with a prefixed length from a span of bytes and return the value and the size of the value.
    static size_t consume(std::string& result, std::span<uint8_t> data) noexcept {
        size_t stringLen = 0;
        size_t sizeLen = consume(stringLen, data);

        if (data.size() < sizeLen + stringLen) return 0;
        if (stringLen > 0xFFFF) return 0;

        result = std::string(
            data.begin() + static_cast<int32_t>(sizeLen),
            data.begin() + static_cast<int32_t>(sizeLen + stringLen)
        );
        return sizeLen + stringLen;
    }

    template <size_t N>
    static size_t consume(char (&result)[N], std::span<uint8_t> data) noexcept {
        if (data.size() < N) return 0;
        std::memcpy(result, data.data(), N);
        result[N] = '\0';
        return N;
    }

    template <size_t S>
    static size_t consume(std::array<char, S>& result, std::span<uint8_t> data) noexcept {
        if (data.size() < S) return 0;
        std::memcpy(result.data(), data.data(), S);
        return S;
    }
}

class binary_reader {
public:
    explicit binary_reader(std::span<uint8_t> data) noexcept : m_data(data) {}

    template <typename T>
    binary_reader& operator>>(T& data) noexcept {
        auto count = detail::consume(data, {m_data.data(), m_data.size()});
        m_data = m_data.subspan(count);
        return *this;
    }

    template <typename T>
    void read(T& data) noexcept {
        auto count = detail::consume(data, {m_data.data(), m_data.size()});
        m_data = m_data.subspan(count);
    }

    void read(void* data, size_t size) noexcept {
        if (size > m_data.size()) {
            m_data = {};
            return;
        }

        const auto bytes = std::vector(m_data.begin(), m_data.begin() + static_cast<int32_t>(size));
        std::memcpy(data, bytes.data(), size);
        m_data = m_data.subspan(size);
    }

    void read(std::vector<uint8_t>& data, size_t size) noexcept {
        if (size > m_data.size()) {
            m_data = {};
            return;
        }

        data = std::vector(m_data.begin(), m_data.begin() + static_cast<int32_t>(size));
        m_data = m_data.subspan(size);
    }

    void skip(size_t size) noexcept {
        if (size > m_data.size()) {
            m_data = {};
            return;
        }

        m_data = m_data.subspan(size);
    }

    [[nodiscard]] std::span<uint8_t> peek(size_t size) const noexcept {
        return {&*m_data.begin(), &*m_data.begin() + size};
    }

    [[nodiscard]] uint8_t peek() const noexcept {
        return m_data.front();
    }

    [[nodiscard]] size_t size() const noexcept { return m_data.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_data.empty(); }

    [[nodiscard]] std::span<uint8_t> data() const noexcept { return m_data; }

protected:
    std::span<uint8_t> m_data;
};

class binary_writer {
public:
    binary_writer() noexcept = default;

    template <typename T>
    binary_writer& operator<<(const T& data) noexcept {
        const auto bytes = detail::getBytes(data);
        m_data.insert(m_data.end(), bytes.begin(), bytes.end());
        return *this;
    }

    void write(const uint8_t* data, size_t size) noexcept {
        m_data.insert(m_data.end(), data, data + size);
    }

    template <typename T>
    void write(const T& data) noexcept {
        const auto bytes = detail::getBytes(data);
        m_data.insert(m_data.end(), bytes.begin(), bytes.end());
    }

    void write(const std::vector<uint8_t>& data) noexcept {
        m_data.insert(m_data.end(), data.begin(), data.end());
    }

    void clear() noexcept { m_data.clear(); }
    [[nodiscard]] size_t size() const noexcept { return m_data.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_data.empty(); }

    [[nodiscard]] const std::vector<uint8_t>& data() const noexcept { return m_data; }
    std::vector<uint8_t>& data() noexcept { return m_data; }
    std::vector<uint8_t> release() noexcept { return std::move(m_data); }

    bool save(const std::filesystem::path& path) const noexcept {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        file.write(reinterpret_cast<const char*>(m_data.data()), static_cast<int32_t>(m_data.size()));
        return true;
    }

private:
    std::vector<uint8_t> m_data;
};
