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
#include <type_traits>

namespace sead {

// Taken from NHSE
class Random {
    private:
        std::array<std::uint32_t, 4> state = {};

    public:
        constexpr inline Random(std::uint32_t seed) {
            for (auto i = 0; i < 4; ++i)  {
                state[i] = (0x6C078965 * (seed ^ (seed >> 30))) + i + 1;
                seed = state[i];
            }
        }

        constexpr inline std::uint32_t get_u32() {
            std::uint32_t v1 = state[0] ^ (state[0] << 11);

            state[0] = state[1];
            state[1] = state[2];
            state[2] = state[3];
            return state[3] = v1 ^ (v1 >> 8) ^ state[3] ^ (state[3] >> 19);
        }

        constexpr inline std::uint64_t get_u64() {
            std::uint32_t v1 = state[0] ^ (state[0] << 11);
            std::uint32_t v2 = state[1];
            std::uint32_t v3 = v1 ^ (v1 >> 8) ^ state[3];

            state[0] = state[2];
            state[1] = state[3];
            state[2] = v3 ^ (state[3] >> 19);
            state[3] = v2 ^ (v2 << 11) ^ ((v2 ^ (v2 << 11)) >> 8) ^ state[2] ^ (v3 >> 19);
            return (static_cast<std::uint64_t>(state[2]) << 32) | state[3];
        }

        template <typename T>
        constexpr inline T get() {
            if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::uint32_t>) {
                return get_u32();
            } else {
                return get_u64();
            }
        }
};

} // namespace sead
