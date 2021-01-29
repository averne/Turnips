// Copyright (C) 2020 averne
//
// This file is part of Turnips.
//
// Turnips is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Turnips is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Turnips.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <array>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include "fs.hpp"
#include "lang.hpp"

namespace tp {

enum class Version: std::size_t {
    V100,
    V110, V111, V112, V113, V114,
    V120, V121,
    V130, V131,
    V140, V141, V142,
    V150, V151,
    V160,
    V170,
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

struct VisitorSchedule {
    std::array<std::uint32_t, 7> npcs;
    std::uint8_t                 _stuff[0x54];
    std::uint32_t                wisp_day;
    std::uint32_t                celeste_day;
};

struct Date {
    std::uint16_t year;
    std::uint8_t month, day;
    std::uint8_t hour, minute, second;
};

struct WeatherInfo {
    std::uint32_t hemisphere;
    std::uint32_t raw_seed;
};

static_assert(sizeof(VersionInfo)     == 0x10 && std::is_standard_layout_v<VersionInfo>);
static_assert(sizeof(TurnipPrices)    == 0x44 && std::is_standard_layout_v<TurnipPrices>);
static_assert(sizeof(VisitorSchedule) == 0x78 && std::is_standard_layout_v<VisitorSchedule>);
static_assert(sizeof(Date)            == 0x8  && std::is_standard_layout_v<Date>);
static_assert(sizeof(WeatherInfo)     == 0x8  && std::is_standard_layout_v<WeatherInfo>);

class VersionParser {
    private:
        constexpr static std::array versions = {
            VersionInfo{ 0x67,    0x6f,    2, 0, 2, 0  }, // 1.0.0
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 1  }, // 1.1.0
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 2  }, // 1.1.1
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 3  }, // 1.1.2
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 4  }, // 1.1.3
            VersionInfo{ 0x6d,    0x78,    2, 0, 2, 5  }, // 1.1.4
            VersionInfo{ 0x20006, 0x20008, 2, 0, 2, 6  }, // 1.2.0
            VersionInfo{ 0x20006, 0x20008, 2, 0, 2, 7  }, // 1.2.1
            VersionInfo{ 0x40002, 0x40008, 2, 0, 2, 8  }, // 1.3.0
            VersionInfo{ 0x40002, 0x40008, 2, 0, 2, 9  }, // 1.3.1
            VersionInfo{ 0x50001, 0x5000B, 2, 0, 2, 10 }, // 1.4.0
            VersionInfo{ 0x50001, 0x5000B, 2, 0, 2, 11 }, // 1.4.1
            VersionInfo{ 0x50001, 0x5000B, 2, 0, 2, 12 }, // 1.4.2
            VersionInfo{ 0x60001, 0x6000c, 2, 0, 2, 13 }, // 1.5.0
            VersionInfo{ 0x60001, 0x6000c, 2, 0, 2, 14 }, // 1.5.1
            VersionInfo{ 0x70001, 0x70006, 2, 0, 2, 15 }, // 1.6.0
            VersionInfo{ 0x74001, 0x74005, 2, 0, 2, 16 }, // 1.7.0
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
        constexpr static std::array turnip_offsets = {
            0x4118C0ul,                                                 // 1.0.0
            0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul, 0x412060ul, // 1.1.x
            0x412060ul, 0x412060ul,                                     // 1.2.x
            0x412060ul, 0x412060ul,                                     // 1.3.x
            0x412060ul, 0x412060ul, 0x412060ul,                         // 1.4.x
            0x41d4a0ul, 0x41d4a0ul,                                     // 1.5.x
            0x41d570ul,                                                 // 1.6.0
            0x41b63cul,                                                 // 1.7.0
        };

        constexpr static std::array turnip_patterns = {
            "fluctuating",
            "large_spike",
            "decreasing",
            "small_spike",
        };

        static_assert(turnip_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version      version = {};
        TurnipPrices prices  = {};

    public:
        constexpr TurnipParser() = default;
        TurnipParser(Version version, const std::vector<std::uint8_t> &save): version(version), prices(this->get_prices(save)) { }

        inline std::string get_pattern() const {
            return lang::get_string(this->turnip_patterns[this->prices.pattern_type], lang::get_json()["turnips_patterns"]);
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
        constexpr static std::array visitor_offsets = {
            0x414f8cul,                                                 // 1.0.0
            0x41572cul, 0x41572cul, 0x41572cul, 0x41572cul, 0x41572cul, // 1.1.x
            0x4159d8ul, 0x4159d8ul,                                     // 1.2.x
            0x4159d8ul, 0x4159d8ul,                                     // 1.3.x
            0x4159d8ul, 0x4159d8ul, 0x4159d8ul,                         // 1.4.x
            0x420e18ul, 0x420e18ul,                                     // 1.5.x
            0x420ee8ul,                                                 // 1.6.0
            0x41f0b4ul,                                                 // 1.7.0
        };

        constexpr static std::array visitor_names = {
            "none",
            "gulliver",
            "label",
            "saharah",
            "wisp",
            "mabel",
            "cj",
            "flick",
            "kicks",
            "leif",
            "redd",
            "gullivarrr"
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
            std::transform(this->schedule.npcs.begin(), this->schedule.npcs.end(), names.begin(),
                [this](std::uint32_t visitor) {
                    return lang::get_string(this->visitor_names[visitor], lang::get_json()["npcs"]);
                }
            );
            return names;
        }

        inline std::uint32_t get_wisp_day() const {
            return this->schedule.wisp_day;
        }

        inline std::uint32_t get_celeste_day() const {
            return this->schedule.celeste_day;
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
        constexpr static std::array date_offsets = {
            0xac0928ul,                                                 // 1.0.0
            0xac27c8ul, 0xac27c8ul, 0xac27c8ul, 0xac27c8ul, 0xac27c8ul, // 1.1.x
            0xace9f8ul, 0xace9f8ul,                                     // 1.2.x
            0xaceaa8ul, 0xaceaa8ul,                                     // 1.3.x
            0xb054a8ul, 0xb054a8ul, 0xb054a8ul,                         // 1.4.x
            0xb20468ul, 0xb20468ul,                                     // 1.5.x
            0xb25038ul,                                                 // 1.6.0
            0x849388ul,                                                 // 1.7.0
        };

        static_assert(date_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version version = {};
        Date    date    = {};

    public:
        constexpr DateParser() = default;
        DateParser(Version version, const std::vector<std::uint8_t> &save): version(version), date(this->get_date((save))) { }

        inline std::uint64_t to_posix() const {
            std::uint64_t ts = 0;
            timeToPosixTimeWithMyRule(reinterpret_cast<const TimeCalendarTime *>(&this->date), &ts, 1, nullptr);
            return ts;
        }

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

class WeatherSeedParser {
    private:
        constexpr static std::array info_offsets = {
            0x1d70ccul,                                                 // 1.0.0
            0x1d70d4ul, 0x1d70d4ul, 0x1d70d4ul, 0x1d70d4ul, 0x1d70d4ul, // 1.1.x
            0x1d70d4ul, 0x1d70d4ul,                                     // 1.2.x
            0x1d70d4ul, 0x1d70d4ul,                                     // 1.3.x
            0x1d70d4ul, 0x1d70d4ul, 0x1d70d4ul,                         // 1.4.x
            0x1e24d4ul, 0x1e24d4ul,                                     // 1.5.x
            0x1e24d4ul,                                                 // 1.6.0
            0x1e24d4ul,                                                 // 1.7.0
        };

        constexpr static std::uint32_t weather_seed_max = 2147483647;

        constexpr static std::array hemisphere_names = {
            "northern",
            "southern",
        };

        static_assert(info_offsets.size() == static_cast<std::size_t>(Version::Total));

    public:
        Version     version  = {};
        WeatherInfo info     = {};

    public:
        constexpr WeatherSeedParser() = default;
        WeatherSeedParser(Version version, const std::vector<std::uint8_t> &save): version(version), info(this->get_info((save))) { }

        constexpr inline std::uint32_t calculate_weather_seed() const {
            return this->info.raw_seed - this->weather_seed_max - 1;
        }

        inline std::string get_hemisphere_name() const {
            return lang::get_string(this->hemisphere_names[this->info.hemisphere], lang::get_json()["hemispheres"]);
        }

    private:
        inline std::size_t get_info_offset() const {
            return (this->version != Version::Unknown) ? this->info_offsets[static_cast<std::size_t>(this->version)] : 0ul;
        }

        inline WeatherInfo get_info(const std::vector<std::uint8_t> &save) const {
            if (auto offset = this->get_info_offset(); offset != 0ul)
                return *reinterpret_cast<const WeatherInfo *>(&save[offset]);
            else
                return {};
        }
};

} // namespace tp
