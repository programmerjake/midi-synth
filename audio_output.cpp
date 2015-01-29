#include "audio_output.h"
#include <SDL.h>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <atomic>
#include <mutex>
#include <string>
#include <cstring>
#include <vector>
#include <limits>
#include <cassert>
#include <iostream>

using namespace std;

namespace
{
atomic_bool deviceAudioOutputUsed(false);

class DeviceAudioOutput : public AudioOutput
{
    shared_ptr<AudioSource> source;
    mutex sourceLock;
    SDL_AudioSpec audioSpec;
    SDL_AudioDeviceID audioDeviceID;
    vector<float> buffer;
    void fillBuffer(uint8_t *buffer_in, int length)
    {
        int16_t *buffer16 = (int16_t *)buffer_in;
        memset((void *)buffer16, 0, length);
        assert(length % (((size_t)AudioChannel::Last + 1) * sizeof(int16_t)) == 0);
        size_t sampleCount = length / (((size_t)AudioChannel::Last + 1) * sizeof(int16_t));
        buffer.assign(sampleCount * ((size_t)AudioChannel::Last + 1), 0);
        unique_lock<mutex> lockIt(sourceLock);
        double sampleDuration = 1.0 / audioSpec.freq;
        auto bufferIterator = buffer.begin();
        for(size_t sample = 0; sample < sampleCount; sample++)
        {
            for(size_t channel = 0; channel <= (size_t)AudioChannel::Last; channel++)
            {
                *bufferIterator++ = source->getCurrentSample((AudioChannel)channel);
            }
            source->advanceTime(sampleDuration);
        }
        lockIt.unlock();
        for(float fv : buffer)
        {
            int v = (int)(fv * 0x8000);
            v = max<int>(v, numeric_limits<int16_t>::min());
            v = min<int>(v, numeric_limits<int16_t>::max());
            *buffer16++ = v;
        }
    }
    static void audioCallback(void *user_data, uint8_t *buffer_in, int length)
    {
        ((DeviceAudioOutput *)user_data)->fillBuffer(buffer_in, length);
    }
public:
    DeviceAudioOutput()
    {
        if(deviceAudioOutputUsed.exchange(true))
            throw runtime_error("device audio already in use");
        try
        {
            if(0 != SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
                throw runtime_error(string("SDL_Init failed: ") + SDL_GetError());
            try
            {
                SDL_AudioSpec desired;
                desired.callback = &audioCallback;
                desired.channels = (size_t)AudioChannel::Last + 1;
                desired.format = AUDIO_S16SYS;
                desired.freq = 44100;
                desired.samples = 4096;
                desired.userdata = (void *)this;
                audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desired, &audioSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
                if(audioDeviceID == 0)
                    throw runtime_error(string("SDL_OpenAudioDevice failed: ") + SDL_GetError());
                switch(audioSpec.channels)
                {
                case 1:
                    cout << "Mono";
                    break;
                case 2:
                    cout << "Stereo";
                    break;
                case 3:
                    cout << "Triphonic";
                    break;
                case 4:
                    cout << "Quadrophonic";
                    break;
                case 5:
                    cout << "Pentaphonic";
                    break;
                case 6:
                    cout << "5.1 surround";
                    break;
                case 7:
                    cout << "6.1 surround";
                    break;
                case 8:
                    cout << "7.1 surround";
                    break;
                default:
                    cout << (int)audioSpec.channels << "-channel";
                }
                cout << " at " << audioSpec.freq / 1000.0 << "kHz\n";
                SDL_PauseAudioDevice(audioDeviceID, SDL_FALSE);
            }
            catch(...)
            {
                SDL_Quit();
                throw;
            }
        }
        catch(...)
        {
            deviceAudioOutputUsed.store(false);
            throw;
        }
    }
    virtual ~DeviceAudioOutput()
    {
        SDL_PauseAudioDevice(audioDeviceID, SDL_TRUE);
        SDL_CloseAudioDevice(audioDeviceID);
        SDL_Quit();
        deviceAudioOutputUsed.store(false);
    }
    virtual void bind(std::shared_ptr<AudioSource> src) override
    {
        unique_lock<mutex> lockIt(sourceLock);
        assert(source == nullptr);
        source = std::move(src);
    }
    virtual void lock() override
    {
        sourceLock.lock();
    }
    virtual void unlock() override
    {
        sourceLock.unlock();
    }
    virtual bool try_lock() override
    {
        return sourceLock.try_lock();
    }
};
}

std::unique_ptr<AudioOutput> makeDeviceAudioOutput()
{
    return unique_ptr<AudioOutput>(new DeviceAudioOutput);
}
