#ifndef MIDI_CHANNEL_H_INCLUDED
#define MIDI_CHANNEL_H_INCLUDED

#include "midi_key.h"
#include <array>
#include <iostream>

class MidiChannel : public AudioSource
{
    std::shared_ptr<MixAudioSource> mixer;
    std::shared_ptr<AmplifyAudioSource> amplifier;
    std::shared_ptr<MidiInstrument> instrument;
    std::array<std::shared_ptr<MidiKey>, maxKey + 1> keys;
    std::list<std::shared_ptr<MidiKey>> playingKeys;
    int slideFromKey;
    double currentPitchBendSemitones;
public:
    MidiChannel(std::shared_ptr<MidiInstrument> instrument)
        : instrument(std::move(instrument)), slideFromKey(invalidKey), currentPitchBendSemitones(0)
    {
        mixer = std::make_shared<MixAudioSource>();
        amplifier = std::make_shared<AmplifyAudioSource>(mixer, 1.0);
    }
    std::shared_ptr<MidiInstrument> getInstrument() const
    {
        return instrument;
    }
    void setInstrument(std::shared_ptr<MidiInstrument> instrument)
    {
        this->instrument = std::move(instrument);
    }
    void noteOff(int midiKey, int velocity = defaultVelocity)
    {
        if(!validMidiKey(midiKey))
            return;
        //std::cout << "note off : " << midiKey << " : " << velocity << std::endl;
        if(keys[midiKey] == nullptr)
            return;
        keys[midiKey]->stop(velocity);
        keys[midiKey] = nullptr;
    }
    void slideFrom(int midiKey)
    {
        if(!validMidiKey(midiKey))
            return;
        slideFromKey = midiKey;
    }
    void noteOn(int midiKey, int velocity)
    {
        if(!validMidiKey(midiKey))
            return;
        if(velocity == 0)
        {
            noteOff(midiKey);
            return;
        }
        //std::cout << "note on : " << midiKey << " : " << velocity << std::endl;
        if(keys[midiKey] != nullptr)
        {
            keys[midiKey]->stop();
            keys[midiKey] = nullptr;
        }
        if(validMidiKey(slideFromKey) && keys[slideFromKey] != nullptr)
        {
            keys[midiKey] = std::move(keys[slideFromKey]);
            keys[slideFromKey] = nullptr;
            keys[midiKey]->slideTo(midiKey, velocity);
            slideFromKey = invalidKey;
            return;
        }
        int startKey = midiKey;
        if(validMidiKey(slideFromKey) && instrument->supportsSlide(slideFromKey))
            startKey = slideFromKey;
        slideFromKey = invalidKey;
        auto key = instrument->generate(startKey, velocity, currentPitchBendSemitones);
        if(startKey != midiKey)
            key->slideTo(midiKey, velocity);
        playingKeys.push_back(key);
        mixer->insert(key, 1.0);
        keys[midiKey] = std::move(key);
    }
    void aftertouch(int midiKey, int velocity)
    {
        if(!validMidiKey(midiKey))
            return;
        if(keys[midiKey] == nullptr)
            return;
        keys[midiKey]->aftertouch(velocity);
    }
    void aftertouchAll(int velocity)
    {
        for(auto key : keys)
        {
            if(key != nullptr)
                key->aftertouch(velocity);
        }
    }
    void setVolume(float newVolume)
    {
        if(playingKeys.empty()) // if nothing is playing then transition instantaneously
            amplifier = std::make_shared<AmplifyAudioSource>(mixer, newVolume);
        else
            amplifier->setAmplitude(newVolume, 10, AmplifyAudioSource::ScaleType::Exponential);
    }
    void pitchBend(double newPitchBendSemitones)
    {
        currentPitchBendSemitones = newPitchBendSemitones;
        for(auto key : playingKeys)
        {
            key->pitchBend(currentPitchBendSemitones);
        }
    }
    void advanceTime(double deltaTime) override
    {
        amplifier->advanceTime(deltaTime);
        for(auto i = playingKeys.begin(); i != playingKeys.end();)
        {
            auto key = *i;
            if(key->finished())
            {
                i = playingKeys.erase(i);
                mixer->erase(key);
            }
            else
                i++;
        }
    }
    float getCurrentSample(AudioChannel channel) override
    {
        return amplifier->getCurrentSample(channel);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        throw std::runtime_error("non duplicable");
    }
};

#endif // MIDI_CHANNEL_H_INCLUDED
