#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include "audio_output.h"
#include "midi_key.h"
#include "audio_data.h"

using namespace std;

int main()
{
    auto grand_piano_D_Sharp = loadFromOgg("samples/grand-piano/D#3.ogg");
    grand_piano_D_Sharp->looped = true;
    grand_piano_D_Sharp->loopStart = 116756;
    grand_piano_D_Sharp->data.resize(123288);
    auto sound1 = make_shared<SampledAudioSource>(grand_piano_D_Sharp);
    auto sound2 = make_shared<TimeScaleAudioSource>(sound1, 1.0 / getKeyFrequency(63));
    auto sound3 = make_shared<MixAudioSource>();
    sound3->addSource(sound2, 1);
    auto modulatedSound = make_shared<TimeScaleAudioSource>(sound3, getKeyFrequency(60));
    auto sound = make_shared<AmplifyAudioSource>(modulatedSound, 1.0);
    auto finalMixer = make_shared<MixAudioSource>();
    auto eventDispatcher = make_shared<EventDispatcherAudioSource>();
    finalMixer->addSource(eventDispatcher, 1);
    finalMixer->addSource(sound, 1);
    {
        static EventDispatcherAudioSource::EventFn eventFn;
        eventFn = [=]()
        {
            static bool isHighNote = true;
            modulatedSound->setScale(getKeyFrequency(isHighNote ? 72 : 60), 3);
            isHighNote = !isHighNote;
            eventDispatcher->scheduleEvent(0.5, eventFn);
        };
        eventDispatcher->scheduleEvent(0.5, eventFn);
    }
    {
        static EventDispatcherAudioSource::EventFn eventFn2;
        eventFn2 = [=]()
        {
            static bool isHighAmplitude = false;
            sound->setAmplitude(isHighAmplitude ? 1 : 0.3, 3, AmplifyAudioSource::ScaleType::Exponential);
            isHighAmplitude = !isHighAmplitude;
            eventDispatcher->scheduleEvent(1, eventFn2);
        };
        eventDispatcher->scheduleEvent(1, eventFn2);
    }
    auto audioOutput = makeDeviceAudioOutput();
    audioOutput->bind(finalMixer);
    cout << "Running...\nPress enter to exit." << endl;
    cin.get();
    return 0;
}
