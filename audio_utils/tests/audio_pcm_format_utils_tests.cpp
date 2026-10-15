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

#define LOG_TAG "audio_pcm_format_utils_tests"

#include <audio_utils/audio_pcm_format_utils.h>
#include <gtest/gtest.h>
#include <system/audio.h>

using namespace android::audio_utils;

TEST(audio_pcm_format_utils_tests, rank_index_matches_table_order) {
    EXPECT_EQ(0, pcmFormatRankIndex(AUDIO_FORMAT_DEFAULT));
    EXPECT_EQ(1, pcmFormatRankIndex(AUDIO_FORMAT_PCM_16_BIT));
    EXPECT_EQ(2, pcmFormatRankIndex(AUDIO_FORMAT_PCM_8_24_BIT));
    EXPECT_EQ(3, pcmFormatRankIndex(AUDIO_FORMAT_PCM_24_BIT_PACKED));
    EXPECT_EQ(4, pcmFormatRankIndex(AUDIO_FORMAT_PCM_32_BIT));
    EXPECT_EQ(5, pcmFormatRankIndex(AUDIO_FORMAT_PCM_FLOAT));
}

TEST(audio_pcm_format_utils_tests, rank_index_not_found) {
    EXPECT_EQ(-1, pcmFormatRankIndex(AUDIO_FORMAT_PCM_8_BIT));
    EXPECT_EQ(-1, pcmFormatRankIndex(AUDIO_FORMAT_MP3));
    EXPECT_EQ(-1, pcmFormatRankIndex(AUDIO_FORMAT_INVALID));
}

TEST(audio_pcm_format_utils_tests, sixteen_bit_always_valid) {
    EXPECT_TRUE(
            isMixerValidPcmSinkFormat(AUDIO_FORMAT_PCM_16_BIT, /*extendedPrecisionAllowed=*/true));
    EXPECT_TRUE(
            isMixerValidPcmSinkFormat(AUDIO_FORMAT_PCM_16_BIT, /*extendedPrecisionAllowed=*/false));
}

TEST(audio_pcm_format_utils_tests, extended_precision_formats_gated) {
    for (audio_format_t format : {AUDIO_FORMAT_PCM_FLOAT, AUDIO_FORMAT_PCM_24_BIT_PACKED,
                                  AUDIO_FORMAT_PCM_32_BIT, AUDIO_FORMAT_PCM_8_24_BIT}) {
        EXPECT_TRUE(isMixerValidPcmSinkFormat(format, /*extendedPrecisionAllowed=*/true));
        EXPECT_FALSE(isMixerValidPcmSinkFormat(format, /*extendedPrecisionAllowed=*/false));
    }
}

TEST(audio_pcm_format_utils_tests, default_format_never_valid) {
    EXPECT_FALSE(
            isMixerValidPcmSinkFormat(AUDIO_FORMAT_DEFAULT, /*extendedPrecisionAllowed=*/true));
    EXPECT_FALSE(
            isMixerValidPcmSinkFormat(AUDIO_FORMAT_DEFAULT, /*extendedPrecisionAllowed=*/false));
}

TEST(audio_pcm_format_utils_tests, unlisted_format_never_valid) {
    EXPECT_FALSE(
            isMixerValidPcmSinkFormat(AUDIO_FORMAT_PCM_8_BIT, /*extendedPrecisionAllowed=*/true));
    EXPECT_FALSE(isMixerValidPcmSinkFormat(AUDIO_FORMAT_MP3, /*extendedPrecisionAllowed=*/true));
}
