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

namespace android::audio_utils {

// Set kEnableExtendedChannels to true to enable greater than stereo channel
// masks (position masks above FCC_2, and index masks) to be considered
// mixer compatible.
inline constexpr bool kEnableExtendedChannels = true;

/**
 * Returns true if channelMask is a channel mask that AudioFlinger's mixer
 * (and, by extension, AudioPolicyManager output selection which must agree
 * with what the mixer will accept) can use directly as a PCM sink channel
 * mask.
 *
 * A POSITION mask is compatible if its channel count is within
 * [minChannelCount, maxChannelCount] and it is the "canonical" position mask
 * for that channel count (see audio_channel_position_mask_is_out_canonical).
 * Haptic channel bits are masked off before computing the channel count,
 * since they are not part of the PCM sink channel count.
 *
 * An INDEX mask is compatible only if extendedChannelsAllowed is true, and
 * its channel count is within [minChannelCount, maxChannelCount].
 *
 * An ACN (ambisonics) mask is compatible only if ambisonicsAllowed is true,
 * audio_acn_channel_mask_is_supported() accepts it, and its channel count is
 * within [FCC_1, FCC_LIMIT]. This range is independent of minChannelCount /
 * maxChannelCount, which only bound POSITION and INDEX masks.
 *
 * Any other representation is not compatible.
 *
 * \param channelMask             the output channel mask to check.
 * \param minChannelCount         minimum allowed channel count, inclusive.
 * \param maxChannelCount         maximum allowed channel count, inclusive.
 * \param extendedChannelsAllowed whether INDEX masks (and, implicitly,
 *                                channel counts above stereo) are permitted.
 * \param ambisonicsAllowed       whether ACN (ambisonics) masks are permitted.
 */
constexpr bool isMixerCompatibleChannelMask(audio_channel_mask_t channelMask,
                                            uint32_t minChannelCount, uint32_t maxChannelCount,
                                            bool extendedChannelsAllowed = kEnableExtendedChannels,
                                            bool ambisonicsAllowed = false) {
    switch (audio_channel_mask_get_representation(channelMask)) {
        case AUDIO_CHANNEL_REPRESENTATION_POSITION: {
            // Haptic channel mask is only applicable for channel position mask.
            const uint32_t channelCount = audio_channel_count_from_out_mask(
                    static_cast<audio_channel_mask_t>(channelMask & ~AUDIO_CHANNEL_HAPTIC_ALL));
            if (channelCount < minChannelCount || channelCount > maxChannelCount) {
                return false;
            }
            // check that channelMask is the "canonical" one we expect for the channelCount.
            return audio_channel_position_mask_is_out_canonical(channelMask);
        }
        case AUDIO_CHANNEL_REPRESENTATION_INDEX: {
            if (!extendedChannelsAllowed) {
                return false;
            }
            const uint32_t channelCount = audio_channel_count_from_out_mask(channelMask);
            return channelCount >= minChannelCount && channelCount <= maxChannelCount;
        }
        case AUDIO_CHANNEL_REPRESENTATION_ACN: {
            if (!ambisonicsAllowed || !audio_acn_channel_mask_is_supported(channelMask)) {
                return false;
            }
            const uint32_t channelCount = audio_channel_count_from_out_mask(channelMask);
            return channelCount >= FCC_1 && channelCount <= FCC_LIMIT;
        }
        default:
            return false;
    }
}

}  // namespace android::audio_utils
