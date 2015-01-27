#ifndef AUDIO_CHANNEL_H_INCLUDED
#define AUDIO_CHANNEL_H_INCLUDED

#include <array>

enum class AudioChannel
{
    First,
    Left = First,
    Right,
    Last = Right
};

template <typename T>
using array_AudioChannel = std::array<T, (std::size_t)AudioChannel::Last + 1>;

#endif // AUDIO_CHANNEL_H_INCLUDED
