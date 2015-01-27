#ifndef AUDIO_OUTPUT_H_INCLUDED
#define AUDIO_OUTPUT_H_INCLUDED

#include "audio_source.h"

class AudioOutput
{
public:
    constexpr AudioOutput()
    {
    }
    AudioOutput(const AudioOutput &) = delete;
    const AudioOutput &operator =(const AudioOutput &) = delete;
    virtual ~AudioOutput()
    {
    }
    virtual void bind(std::shared_ptr<AudioSource> src) = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool try_lock() = 0;
};

std::unique_ptr<AudioOutput> makeDeviceAudioOutput();

#endif // AUDIO_OUTPUT_H_INCLUDED
