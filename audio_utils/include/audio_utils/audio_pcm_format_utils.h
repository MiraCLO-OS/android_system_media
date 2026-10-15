/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <system/audio.h>

#include <cstddef>
#include <iterator>

namespace android::audio_utils {

// Set kEnableExtendedPrecision to true to use extended precision PCM formats
// (anything wider than 16-bit) in the MixerThread sink.
inline constexpr bool kEnableExtendedPrecision = true;

// PCM sink formats in order of increasing preference, paired with whether
// selecting that format requires extended precision to be enabled.
struct PcmFormatRank {
    audio_format_t format;
    bool requiresExtendedPrecision;
};

inline constexpr PcmFormatRank kPcmFormatRankTable[] = {
        {AUDIO_FORMAT_DEFAULT, false},     {AUDIO_FORMAT_PCM_16_BIT, false},
        {AUDIO_FORMAT_PCM_8_24_BIT, true}, {AUDIO_FORMAT_PCM_24_BIT_PACKED, true},
        {AUDIO_FORMAT_PCM_32_BIT, true},   {AUDIO_FORMAT_PCM_FLOAT, true},
};

// return the best format
constexpr audio_format_t pcmBestSinkFormat() {
    return kPcmFormatRankTable[std::size(kPcmFormatRankTable) - 1].format;
}

/**
 * Returns the index of format in kPcmFormatRankTable, or -1 if format is not
 * present in the table. Rank increases with index: a format with a higher
 * index is a strict improvement in precision over one with a lower index.
 */
constexpr int pcmFormatRankIndex(audio_format_t format) {
    for (size_t i = 0; i < std::size(kPcmFormatRankTable); i++) {
        if (kPcmFormatRankTable[i].format == format) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/**
 * Returns true if format is permitted for the PCM sink in the AudioFlinger's mixer.
 *
 * \param format                  the sink PCM format to check.
 * \param extendedPrecisionAllowed whether formats wider than 16-bit PCM are
 *                                 permitted.
 */
constexpr bool isMixerValidPcmSinkFormat(audio_format_t format,
                                         bool extendedPrecisionAllowed = kEnableExtendedPrecision) {
    const int index = pcmFormatRankIndex(format);
    if (index < 0 || format == AUDIO_FORMAT_DEFAULT) {
        return false;
    }
    return !kPcmFormatRankTable[index].requiresExtendedPrecision || extendedPrecisionAllowed;
}

}  // namespace android::audio_utils
