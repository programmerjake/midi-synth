#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include "audio_output.h"
#include "midi_channel.h"
#include "audio_data.h"

using namespace std;

int main()
{
    auto instrument = loadFromDirectory("samples/p200 piano");
    auto channel = make_shared<MidiChannel>(instrument);
    auto finalMixer = make_shared<MixAudioSource>();
    auto eventDispatcher = make_shared<EventDispatcherAudioSource>(finalMixer);
    double t = 0;

    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});

    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(50, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(50, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(48, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(48, defaultVelocity);});

    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(50, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(50, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(45, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(43, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(43, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(48, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOff(48, defaultVelocity);});

    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(69, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(62, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.25, [=](){channel->noteOff(60, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.0, [=](){channel->noteOn(67, defaultVelocity);});
    eventDispatcher->scheduleEvent(t += 0.5, [=](){channel->noteOff(67, defaultVelocity);});

    finalMixer->insert(channel, 0.3);
    auto audioOutput = makeDeviceAudioOutput();
    audioOutput->bind(eventDispatcher);
    cout << "Running...\nPress enter to exit." << endl;
    cin.get();
    return 0;
}
