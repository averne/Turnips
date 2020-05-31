#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <vector>
#include <utility>

#include <switch.h>

#include "fs.hpp"
#include "sead.hpp"

namespace sv {

// From NHSE
static std::array<std::uint8_t, 0x10> get_param(const std::vector<std::uint32_t> &crypt_data, std::size_t idx) {
    auto sead = sead::Random(crypt_data[crypt_data[idx] & 0x7f]);
    auto roll_count = (crypt_data[crypt_data[idx + 1] & 0x7f] & 0xf) + 1;

    for (std::uint32_t i = 0; i < roll_count; ++i)
        sead.get_u64();

    std::array<std::uint8_t, 0x10> res;
    for (std::size_t i = 0; i < res.size(); i++)
        res[i] = sead.get_u32() >> 24;
    return res;
}

static std::pair<std::array<std::uint8_t, 0x10>, std::array<std::uint8_t, 0x10>> get_keys(fs::File &header) {
    constexpr std::size_t crypt_data_size = 0x200;

    std::vector<std::uint32_t> crypt_data(0x200, 0);
    if (auto read = header.read(crypt_data.data(), crypt_data.size(), 0x100); read != crypt_data_size)
        printf("Failed to read header encryption data (got %#lx bytes, expected %#lx)\n", read, crypt_data_size);

    auto key = get_param(crypt_data, 0);
    auto ctr = get_param(crypt_data, 2);
    return {std::move(key), std::move(ctr)};
}

static std::vector<std::uint8_t> decrypt(fs::File &main, std::size_t size,
        const std::array<std::uint8_t, 0x10> &key, const std::array<std::uint8_t, 0x10> ctr) {
    Aes128CtrContext ctx;
    aes128CtrContextCreate(&ctx, key.data(), ctr.data());

    std::size_t offset = 0, read = 0, buf_size = std::clamp(size, 0x1000ul, 0x80000ul);
    std::vector<std::uint8_t> buf(buf_size, 0);
    std::vector<std::uint8_t> res(size, 0);

    while (offset < size) {
        read = main.read(buf.data(), buf.size(), offset);
        aes128CtrCrypt(&ctx, &res[offset], buf.data(), read);
        offset += read;
        if (read != buf.size())
            break;
    }

    return res;
}

} // namespace sv
