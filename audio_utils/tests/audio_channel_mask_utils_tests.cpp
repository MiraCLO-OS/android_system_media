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

#define LOG_TAG "audio_channel_mask_utils_tests"

#include <audio_utils/audio_channel_mask_utils.h>
#include <gtest/gtest.h>
#include <system/audio.h>

using namespace android::audio_utils;

namespace {

// Non-canonical 4-channel position mask (FL|FR|FC|LFE), as opposed to the
// canonical AUDIO_CHANNEL_OUT_QUAD (FL|FR|BL|BR) for the same channel count.
// This is the USB device example that motivated the mixer-compatibility
// tie-breaker: a device may advertise both this mask and an INDEX mask of
// the same channel count, and the mixer only accepts the canonical one.
constexpr audio_channel_mask_t kNonCanonical4Ch = AUDIO_CHANNEL_OUT_3POINT1;

// Non-canonical 6-channel position mask (FL|FR|FC|LFE|SL|SR), as opposed to
// the canonical AUDIO_CHANNEL_OUT_5POINT1 (FL|FR|FC|LFE|BL|BR) for the same
// channel count.
constexpr audio_channel_mask_t kNonCanonical6Ch = AUDIO_CHANNEL_OUT_5POINT1_SIDE;

// Non-canonical 8-channel position mask (FL|FR|FC|LFE|BL|BR|TSL|TSR), as
// opposed to the canonical AUDIO_CHANNEL_OUT_7POINT1 (FL|FR|FC|LFE|BL|BR|
// SL|SR) for the same channel count.
constexpr audio_channel_mask_t kNonCanonical8Ch = AUDIO_CHANNEL_OUT_5POINT1POINT2;

// ACN-representation mask with a channel-count field (2) that does not
// correspond to any recognized ambisonics order (valid full-sphere counts
// are perfect squares: 1, 4, 9, 16, ...). Used to exercise the
// audio_acn_channel_mask_is_supported() rejection path.
constexpr audio_channel_mask_t kUnsupportedAcnMask =
        static_cast<audio_channel_mask_t>(AUDIO_CHANNEL_INDEX_ACN | AUDIO_ACN_FULL_SPHERE | 2);

}  // namespace

TEST(audio_channel_mask_utils_tests, canonical_position_mask_in_range) {
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_STEREO, FCC_2, FCC_LIMIT));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_QUAD, FCC_2, FCC_LIMIT));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_5POINT1, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, non_canonical_position_mask_rejected) {
    // Same channel count as the corresponding canonical mask, but not the
    // canonical bit layout: must be rejected regardless of bounds.
    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical4Ch, FCC_2, FCC_LIMIT));
    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical6Ch, FCC_2, FCC_LIMIT));
    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical8Ch, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, canonical_mono_rejected_when_min_is_stereo) {
    // Regression test: canonical mono has a channel count (1) below FCC_2,
    // so it must be rejected when the caller requires at least stereo, even
    // though it is otherwise a canonical position mask.
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_MONO, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, canonical_mono_accepted_when_min_allows_mono) {
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_MONO, 1, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, position_mask_above_max_rejected) {
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_5POINT1, FCC_2, FCC_2));
}

TEST(audio_channel_mask_utils_tests, haptic_bits_excluded_from_channel_count) {
    // The haptic channel(s) must not count against min/maxChannelCount, and
    // must not make an otherwise-canonical mask look non-canonical.
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_OUT_STEREO_HAPTIC_A, FCC_2, FCC_2));
}

TEST(audio_channel_mask_utils_tests, index_mask_in_range_when_extended_allowed) {
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_4, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, index_mask_rejected_when_extended_disallowed) {
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_4, FCC_2, FCC_LIMIT,
                                              /*extendedChannelsAllowed=*/false));
}

TEST(audio_channel_mask_utils_tests, index_mask_out_of_range_rejected) {
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_2, /*minChannelCount=*/4,
                                              FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, index_mask_preferred_over_non_canonical_position_mask) {
    // The exact USB tie-break scenario: same channel count, one
    // non-canonical position mask and one INDEX mask. Only the INDEX mask
    // is mixer compatible.
    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical4Ch, FCC_2, FCC_LIMIT));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_4, FCC_2, FCC_LIMIT));

    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical6Ch, FCC_2, FCC_LIMIT));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_6, FCC_2, FCC_LIMIT));

    EXPECT_FALSE(isMixerCompatibleChannelMask(kNonCanonical8Ch, FCC_2, FCC_LIMIT));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INDEX_MASK_8, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, invalid_representation_rejected) {
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_NONE, FCC_2, FCC_LIMIT));
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_INVALID, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, acn_mask_rejected_when_ambisonics_disallowed) {
    // Ambisonics is opt-in: an otherwise valid, in-range ACN mask must be
    // rejected unless the caller explicitly allows it.
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests, acn_mask_accepted_when_ambisonics_allowed_and_in_range) {
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_2, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_mask_channel_count_bound_ignores_min_max) {
    // Unlike POSITION/INDEX, the ACN channel-count bound is the fixed
    // [FCC_1, FCC_LIMIT] range, independent of the caller-supplied
    // min/maxChannelCount. A 4-channel ACN mask must still be accepted even
    // when the caller's range would reject a 4-channel POSITION/INDEX mask.
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1, /*minChannelCount=*/8,
                                             /*maxChannelCount=*/8,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_mask_above_fcc_limit_rejected) {
    // AUDIO_CHANNEL_ACN_ORDER_3 has 16 channels, which exceeds FCC_LIMIT (12).
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_3, FCC_2, FCC_LIMIT,
                                              /*extendedChannelsAllowed=*/true,
                                              /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_mask_unsupported_pattern_rejected) {
    // A channel-count field with no corresponding recognized ambisonics
    // order must be rejected even when ambisonics is allowed and the count
    // is within [FCC_1, FCC_LIMIT].
    EXPECT_FALSE(isMixerCompatibleChannelMask(kUnsupportedAcnMask, FCC_2, FCC_LIMIT,
                                              /*extendedChannelsAllowed=*/true,
                                              /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_mask_unaffected_by_extended_channels_flag) {
    // extendedChannelsAllowed only gates the INDEX representation; it has no
    // bearing on ACN masks.
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/false,
                                             /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_horizontal_mask_rejected_when_ambisonics_disallowed) {
    // Horizontal-only ACN masks are gated by ambisonicsAllowed the same way
    // full-sphere ACN masks are.
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1_HRZ, FCC_2, FCC_LIMIT));
}

TEST(audio_channel_mask_utils_tests,
     acn_horizontal_mask_accepted_when_ambisonics_allowed_and_in_range) {
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_1_HRZ, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
    // AUDIO_CHANNEL_ACN_ORDER_5_HRZ has 11 channels, the highest horizontal
    // order that still fits within FCC_LIMIT (12).
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_5_HRZ, FCC_2, FCC_LIMIT,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_horizontal_mask_channel_count_bound_ignores_min_max) {
    // Same fixed [FCC_1, FCC_LIMIT] bound applies to horizontal masks,
    // independent of the caller-supplied min/maxChannelCount.
    EXPECT_TRUE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_2_HRZ, /*minChannelCount=*/8,
                                             /*maxChannelCount=*/8,
                                             /*extendedChannelsAllowed=*/true,
                                             /*ambisonicsAllowed=*/true));
}

TEST(audio_channel_mask_utils_tests, acn_horizontal_mask_above_fcc_limit_rejected) {
    // AUDIO_CHANNEL_ACN_ORDER_6_HRZ has 13 channels, which exceeds FCC_LIMIT (12).
    EXPECT_FALSE(isMixerCompatibleChannelMask(AUDIO_CHANNEL_ACN_ORDER_6_HRZ, FCC_2, FCC_LIMIT,
                                              /*extendedChannelsAllowed=*/true,
                                              /*ambisonicsAllowed=*/true));
}
