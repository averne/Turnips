#pragma once

#include <cstdint>
#include <array>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include "fs.hpp"

namespace tp {

enum class Version: std::size_t {
    V100,
    V110, V111, V112, V113, V114,
    V120, V121,
    Unknown,
    Total = Unknown,
};

struct VersionInfo {
    std::uint32_t major = 0, minor = 0;
    std::uint16_t unk_1 = 0, header_rev = 0, unk_2 = 0, save_rev = 0;

    constexpr inline VersionInfo() = default;

    constexpr inline bool operator ==(const VersionInfo &other) const {
        return (this->major == other.major) && (this->minor      == other.minor)
            && (this->unk_1 == other.unk_1) && (this->header_rev == other.header_rev)
            && (this->unk_2 == other.unk_2) && (this->save_rev   == other.save_rev);
    }

    constexpr inline bool operator !=(const VersionInfo &other) const {
        return !(*this == other);
    }
};

struct TurnipPrices {
    std::uint32_t buy_price;
    union {
        std::array<std::uint32_t, 14> week_prices;
        struct {
            std::uint32_t sunday_am_price,    sunday_pm_price;
            std::uint32_t monday_am_price,    monday_pm_price;
            std::uint32_t tuesday_am_price,   tuesday_pm_price;
            std::uint32_t wednesday_am_price, wednesday_pm_price;
            std::uint32_t thursday_am_price,  thursday_pm_price;
            std::uint32_t friday_am_price,    friday_pm_price;
            std::uint32_t saturday_am_price,  saturday_pm_price;
        };
    };
    std::uint32_t pattern_type;
    std::uint32_t unk;
};

using VisitorSchedule = std::array<std::uint32_t, 7>;

struct Date {
    std::uint16_t year;
    std::uint8_t month, day;
    std::uint8_t hour, minute, second;
};

static_assert(sizeof(VersionInfo)     == 0x10 && std::is_standard_layout_v<VersionInfo>);
static_assert(sizeof(TurnipPrices)    == 0x44 && std::is_standard_layout_v<TurnipPrices>);
static_assert(sizeof(VisitorSchedule) == 0x1c && std::is_standard_layout_v<VisitorSchedule>);
static_assert(sizeof(Date)            == 0x8  && std::is_standard_layout_v<Date>);

class VersionParser {
    private:
        constexpr static inline std::array versions = {
            VersionInfo{ 0x67,    0x6f,    2, 0, 2, 0 }, // 1.0.0
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 1 }, // 1.1.0
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 2 }, // 1.1.1
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 3 }, // 1.1.2
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 4 }, // 1.1.3
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 5 }, // 1.1.4
            VersionInfo{ 0x20006, 0x20008, 2, 0, 2, 6 }, // 1.2.0
            VersionInfo{ 0x20006, 0x20008, 2, 0, 2, 7 }, // 1.2.1
        };

        static_assert(versions.size() == static_cast<std::size_t>(Version::Total));

    private:
        Version version;

    public:
        VersionParser(fs::File &header): version(this->calc_version(header)) { }

        explicit inline operator Version() const {
            return this->version;
        }

    private:
        inline Version calc_version(fs::File &header) {
            VersionInfo info;
            if (auto read = header.read(&info, sizeof(VersionInfo)); read != sizeof(VersionInfo)) {
                printf("Failed to read version info: got %#lx, expected %#lx\n", read, sizeof(VersionInfo));
                return Version::Unknown;
            }

            for (std::size_t i = 0; i < this->versions.size(); ++i)
                if (this->versions[i] == info)
                    return static_cast<Version>(i);
            return Version::Unknown;
        }
};

class TurnipParser {
    private:
        constexpr static inline std::array turnip_offsets = {
            0x4118C0ul, 0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul,
            0x412060ul, 0x412060ul,
        };

        constexpr static inline std::array turnip_patterns = {
            "Fluctuating",
            "Large spike",
            "Decreasing",
            "Small spike",
        };

        static_assert(turnip_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version      version = {};
        TurnipPrices prices  = {};

    public:
        constexpr TurnipParser() = default;
        TurnipParser(Version version, const std::vector<std::uint8_t> &save): version(version), prices(this->get_prices(save)) { }

        inline std::string get_pattern() const {
            return this->turnip_patterns[this->prices.pattern_type];
        }

    private:
        inline std::size_t get_tp_offset() const {
            return (this->version != Version::Unknown) ? this->turnip_offsets[static_cast<std::size_t>(this->version)] : 0ul;
        }

        inline TurnipPrices get_prices(const std::vector<std::uint8_t> &save) const {
            if (auto offset = this->get_tp_offset(); offset != 0ul)
                return *reinterpret_cast<const TurnipPrices *>(&save[offset]);
            else
                return {};
        }
};

class VisitorParser {
    private:
        constexpr static inline std::array visitor_offsets = {
            0x414f8cul, 0x41572cul, 0x41572cul, 0x41572cul, 0x41572cul, 0x41572cul,
            0x4159d8ul, 0x4159d8ul,
        };

        constexpr static inline std::array visitor_names = {
            "None",
            "Gulliver",
            "Label",
            "Saharah",
            "Wisp",
            "Mabel",
            "CJ",
            "Flick",
            "Kicks",
            "Leif",
            "Redd",
        };

        static_assert(visitor_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version         version  = {};
        VisitorSchedule schedule = {};

    public:
        constexpr VisitorParser() = default;
        VisitorParser(Version version, const std::vector<std::uint8_t> &save): version(version), schedule(this->get_schedule((save))) { }

        inline std::array<std::string, 7> get_visitor_names() const {
            std::array<std::string, 7> names;
            std::transform(this->schedule.begin(), this->schedule.end(), names.begin(),
            [this](std::uint32_t visitor) {
                return this->visitor_names[visitor];
            });
            return names;
        }

    private:
        inline std::size_t get_vs_offset() const {
            return (this->version != Version::Unknown) ? this->visitor_offsets[static_cast<std::size_t>(this->version)] : 0ul;
        }

        inline VisitorSchedule get_schedule(const std::vector<std::uint8_t> &save) const {
            if (auto offset = this->get_vs_offset(); offset != 0ul)
                return *reinterpret_cast<const VisitorSchedule *>(&save[offset]);
            else
                return {};
        }
};

class DateParser {
    private:
        constexpr static inline std::array date_offsets = {
            0xac0928ul, 0xac27c8ul, 0xac27c8ul, 0xac27c8ul, 0xac27c8ul, 0xac27c8ul,
            0xace9f8ul, 0xace9f8ul,
        };

        static_assert(date_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version version = {};
        Date    date    = {};

    public:
        constexpr DateParser() = default;
        DateParser(Version version, const std::vector<std::uint8_t> &save): version(version), date(this->get_date((save))) { }

    private:
        inline std::size_t get_date_offset() const {
            return (this->version != Version::Unknown) ? this->date_offsets[static_cast<std::size_t>(this->version)] : 0ul;
        }

        inline Date get_date(const std::vector<std::uint8_t> &save) const {
            if (auto offset = this->get_date_offset(); offset != 0ul)
                return *reinterpret_cast<const Date *>(&save[offset]);
            else
                return {};
        }
};

} // namespace tp
