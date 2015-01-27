#include "audio_data.h"
#include <vorbis/vorbisfile.h>
#include <cerrno>
#include <iostream>
#include <endian.h>
#include <stdexcept>
#include <cassert>

using namespace std;

namespace
{
inline void convertChannels(float *output, size_t outputChannels, const float *input, size_t inputChannels)
{
    if(outputChannels == inputChannels)
    {
        for(size_t i = 0; i < outputChannels; i++)
            output[i] = input[i];
        return;
    }
    float *data = output;
    const float *buffer = input;
    const size_t sampleCount = 1;
    size_t channels = outputChannels;
    size_t sourceChannels = inputChannels;
    switch(channels)
    {
    case 1:
    {
        for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
        {
            int sum = 0;
            for(std::size_t j = 0; j < sourceChannels; j++)
                sum += buffer[bufferIndex++];
            *data++ = sum / sourceChannels;
        }
        break;
    }
    case 2:
    {
        switch(sourceChannels)
        {
        case 1:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex++];
            }
            break;
        case 3:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2) / 3;
                *data++ = (channel2 + 2 * channel3) / 3;
            }
            break;
        case 4:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                *data++ = (channel1 + channel3) / 2;
                *data++ = (channel2 + channel4) / 2;
            }
            break;
        case 5:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2 + 2 * channel4) / 5;
                *data++ = (2 * channel3 + channel2 + 2 * channel5) / 5;
            }
            break;
        case 6:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                float channel6 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2 + 2 * channel4) / 5 + channel6;
                *data++ = (2 * channel3 + channel2 + 2 * channel5) / 5 + channel6;
            }
            break;
        default:
            assert(false);
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
            }
            break;
        }
        break;
    }
    case 3:
    {
        switch(sourceChannels)
        {
        case 1:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex++];
            }
            break;
        case 2:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (channel1 + channel2) / 2;
                *data++ = (5 * channel2 - channel1) / 4;
            }
            break;
        case 4:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                *data++ = (5 * (channel1 + channel3) - channel2 - channel4) / 8;
                *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                *data++ = (5 * (channel2 + channel4) - channel1 - channel3) / 8;
            }
            break;
        case 5:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                *data++ = (channel1 + channel4) / 2;
                *data++ = channel2;
                *data++ = (channel3 + channel5) / 2;
            }
            break;
        case 6:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                float channel6 = buffer[bufferIndex++];
                *data++ = (channel1 + channel4) / 2 + channel6;
                *data++ = channel2 + channel6;
                *data++ = (channel3 + channel5) / 2 + channel6;
            }
            break;
        default:
            assert(false);
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
            }
            break;
        }
        break;
    }
    case 4:
    {
        switch(sourceChannels)
        {
        case 1:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex++];
            }
            break;
        case 2:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                *data++ = channel1;
                *data++ = channel2;
                *data++ = channel1;
                *data++ = channel2;
            }
            break;
        case 3:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2) / 3;
                *data++ = (channel2 + 2 * channel3) / 3;
                *data++ = (2 * channel1 + channel2) / 3;
                *data++ = (channel2 + 2 * channel3) / 3;
            }
            break;
        case 5:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2) / 3;
                *data++ = (2 * channel3 + channel2) / 3;
                *data++ = channel4;
                *data++ = channel5;
            }
            break;
        case 6:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                float channel6 = buffer[bufferIndex++];
                *data++ = (2 * channel1 + channel2) / 3 + channel6;
                *data++ = (2 * channel3 + channel2) / 3 + channel6;
                *data++ = channel4 + channel6;
                *data++ = channel5 + channel6;
            }
            break;
        default:
            assert(false);
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
            }
            break;
        }
        break;
    }
    case 5:
    {
        switch(sourceChannels)
        {
        case 1:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex++];
            }
            break;
        case 2:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (channel1 + channel2) / 2;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
            }
            break;
        case 3:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                *data++ = channel1;
                *data++ = channel2;
                *data++ = channel3;
                *data++ = channel1;
                *data++ = channel3;
            }
            break;
        case 4:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
            }
            break;
        case 6:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                float channel6 = buffer[bufferIndex++];
                *data++ = channel1 + channel6;
                *data++ = channel2 + channel6;
                *data++ = channel3 + channel6;
                *data++ = channel4 + channel6;
                *data++ = channel5 + channel6;
            }
            break;
        default:
            assert(false);
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
            }
            break;
        }
        break;
    }
    case 6:
    {
        switch(sourceChannels)
        {
        case 1:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex];
                *data++ = buffer[bufferIndex++];
            }
            break;
        case 2:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (channel1 + channel2) / 2;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (channel1 + channel2) / 2;
            }
            break;
        case 3:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                *data++ = channel1;
                *data++ = channel2;
                *data++ = channel3;
                *data++ = channel1;
                *data++ = channel3;
                *data++ = (channel1 + channel2 + channel3) / 3;
            }
            break;
        case 4:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (5 * channel1 - channel2) / 4;
                *data++ = (5 * channel2 - channel1) / 4;
                *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
            }
            break;
        case 5:
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float channel1 = buffer[bufferIndex++];
                float channel2 = buffer[bufferIndex++];
                float channel3 = buffer[bufferIndex++];
                float channel4 = buffer[bufferIndex++];
                float channel5 = buffer[bufferIndex++];
                *data++ = channel1;
                *data++ = channel2;
                *data++ = channel3;
                *data++ = channel4;
                *data++ = channel5;
                *data++ = (channel1 + channel2 + channel3 + channel4 + channel5) / 5;
            }
            break;
        default:
            assert(false);
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
                *data++ = sum / sourceChannels;
            }
            break;
        }
        break;
    }
    default:
    {
        assert(false);
        for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
        {
            float sum = 0;
            for(std::size_t j = 0; j < sourceChannels; j++)
                sum += buffer[bufferIndex++];
            for(std::size_t j = 0; j < channels; j++)
                *data++ = sum / sourceChannels;
        }
        break;
    }
    }
}
}

std::shared_ptr<AudioData> loadFromOgg(std::string fileName)
{
    OggVorbis_File ovf;
    switch(ov_fopen(fileName.c_str(), &ovf))
    {
    case 0:
        break;
    default:
        throw runtime_error("can't load from Ogg/Vorbis file: " + fileName);
    }
    vorbis_info *info = ov_info(&ovf, -1);
    int inputChannelCount = info->channels;
    shared_ptr<AudioData> retval = make_shared<AudioData>();
    retval->loopStart = 0;
    retval->looped = false;
    retval->loopDecayAmplitude = 1.0;
    retval->sampleRate = info->rate;
    auto sampleCount = ov_pcm_total(&ovf, -1);
    if(sampleCount != OV_EINVAL)
        retval->data.reserve(sampleCount);
    vector<float> inputSampleBuffer;
    inputSampleBuffer.resize(inputChannelCount);
    for(;;)
    {
        int currentSection;
        float **pcmChannels = nullptr;
        long currentSampleCount = ov_read_float(&ovf, &pcmChannels, 8192, &currentSection);
        if(currentSampleCount <= 0)
            break;
        retval->data.reserve(retval->data.size() + (size_t)currentSampleCount);
        for(size_t sample = 0; sample < (size_t)currentSampleCount; sample++)
        {
            for(int i = 0; i < inputChannelCount; i++)
            {
                inputSampleBuffer[i] = pcmChannels[i][sample];
            }
            array_AudioChannel<float> outputSample;
            convertChannels(&outputSample[0], outputSample.size(), &inputSampleBuffer[0], inputChannelCount);
            retval->data.push_back(outputSample);
        }
    }
    ov_clear(&ovf);
    return std::move(retval);
}
