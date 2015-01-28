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
    grand_piano_D_Sharp->loopDecayAmplitude = 0.97;
    auto make_piano_note = [=](int key, int velocity)->shared_ptr<MidiKey>
    {
        return make_shared<GenericMidiKey>(key, velocity, 0, make_shared<SampledAudioSource>(grand_piano_D_Sharp), 63, GenericMidiKey::InstantaneousAttack, 1, 0, 5, 0.5, 0, 0, 1, 1);
    };
    auto finalMixer = make_shared<MixAudioSource>();
    auto eventDispatcher = make_shared<EventDispatcherAudioSource>(finalMixer);
    {
        static EventDispatcherAudioSource::EventFn eventFn;
        eventFn = [=]()
        {
            static int noteNumber = 60;
            static shared_ptr<MidiKey> key = nullptr;
            if(key)
                key->stop();
            if(noteNumber <= 72)
                finalMixer->addSource(key = make_piano_note(noteNumber, defaultVelocity), 0.1);
            noteNumber++;
            if(noteNumber - 1 <= 72)
                eventDispatcher->scheduleEvent(0.125, eventFn);
        };
        eventDispatcher->scheduleEvent(0.01, eventFn);
    }
    auto audioOutput = makeDeviceAudioOutput();
    audioOutput->bind(eventDispatcher);
    cout << "Running...\nPress enter to exit." << endl;
    cin.get();
    return 0;
}
