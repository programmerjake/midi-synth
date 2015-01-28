#ifndef AUDIO_DATA_H_INCLUDED
#define AUDIO_DATA_H_INCLUDED

#include <vector>
#include "audio_channel.h"
#include <string>
#include <memory>

struct AudioData
{
    std::vector<array_AudioChannel<float>> data;
    double sampleRate;
    size_t loopStart;
    bool looped;
    float loopDecayAmplitude;
};

std::shared_ptr<AudioData> loadFromOgg(std::string fileName);

#endif // AUDIO_DATA_H_INCLUDED
