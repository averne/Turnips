#pragma once

#include <cstdint>
#include <array>
#include <type_traits>
#include <utility>
#include <vector>

#include "fs.hpp"

namespace tp {

enum class Version: std::size_t {
    V100, V110, V111, V112, V113, V114,
    Unknown,
};

struct VersionInfo {
    std::uint32_t major = 0, minor = 0;
    std::uint16_t unk_1 = 0, header_rev = 0, unk_2 = 0, save_rev = 0;

    constexpr inline VersionInfo() = default;

    constexpr inline bool operator ==(const VersionInfo &other) const {
        return (this->major == other.major) && (this->minor == other.minor)
            && (this->unk_1 == other.unk_1) && (this->header_rev == other.header_rev)
            && (this->unk_2 == other.unk_2) && (this->save_rev == other.save_rev);
    }
    constexpr inline bool operator !=(const VersionInfo &other) const {
        return !(*this == other);
    }
};

constexpr static std::array versions = {
    VersionInfo{ 0x67, 0x6f, 2, 0, 2, 0 }, // 1.0.0
    VersionInfo{ 0x6d, 0x78, 2, 0, 2, 1 }, // 1.1.0
    VersionInfo{ 0x6d, 0x78, 2, 0, 2, 2 }, // 1.1.1
    VersionInfo{ 0x6d, 0x78, 2, 0, 2, 3 }, // 1.1.2
    VersionInfo{ 0x6d, 0x78, 2, 0, 2, 4 }, // 1.1.3
    VersionInfo{ 0x6d, 0x78, 2, 0, 2, 5 }, // 1.1.4
};

constexpr static std::array offsets = {
    0x4118C0ul, 0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul,
};

static_assert(sizeof(VersionInfo) == 0x10 && std::is_standard_layout_v<VersionInfo>);
static_assert(versions.size() == offsets.size());

struct TurnipPrices {
    std::uint32_t buy_price = 0;
    union {
        std::array<std::uint32_t, 14> week_prices;
        struct {
            std::uint32_t sunday_am_price    = 0, sunday_pm_price    = 0;
            std::uint32_t monday_am_price    = 0, monday_pm_price    = 0;
            std::uint32_t tuesday_am_price   = 0, tuesday_pm_price   = 0;
            std::uint32_t wednesday_am_price = 0, wednesday_pm_price = 0;
            std::uint32_t thursday_am_price  = 0, thursday_pm_price  = 0;
            std::uint32_t friday_am_price    = 0, friday_pm_price    = 0;
            std::uint32_t satursday_am_price = 0, satursday_pm_price = 0;
        };
    };
    std::uint32_t pattern_type;
    std::uint32_t unk;
};

static_assert(sizeof(TurnipPrices) == 0x44);

class Parser {
    public:
        Parser(fs::File &header, std::vector<std::uint8_t> &&save): version(calc_version(header)), save(std::move(save)) { }

        inline Version get_version() const {
            return this->version;
        }

        inline std::size_t get_tp_offset() const {
            auto ver = this->get_version();
            if (ver != Version::Unknown)
                return offsets[static_cast<std::size_t>(ver)];
            else
                return 0ul;
        }

        inline TurnipPrices get_prices() const {
            return *reinterpret_cast<const TurnipPrices *>(&this->save[this->get_tp_offset()]);
        }

    private:
        inline Version calc_version(fs::File &header) {
            VersionInfo info;
            if (auto read = header.read(&info, sizeof(VersionInfo)); read != sizeof(VersionInfo)) {
                printf("Failed to read version info: got %#lx, expected %#lx\n", read, sizeof(VersionInfo));
                return Version::Unknown;
            }

            for (std::size_t i = 0; i < versions.size(); ++i)
                if (versions[i] == info)
                    return static_cast<Version>(i);
            return Version::Unknown;
        }


    private:
        Version version;
        std::vector<std::uint8_t> save;
};

} // namespace tp
