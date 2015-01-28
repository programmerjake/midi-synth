#ifndef MIDI_KEY_H_INCLUDED
#define MIDI_KEY_H_INCLUDED

#include <cmath>
#include "audio_source.h"

inline double getKeyFrequency(double midiKey)
{
    return 440.0 * std::exp(M_LN2 * (midiKey - 69) / 12.0);
}

inline double getMidiKeyFromFrequency(double f)
{
    return std::log(f * (1.0 / 440.0)) * (12.0 * M_LOG2E) + 69.0;
}

inline double getRelativeKeyFrequency(double midiKey)
{
    return std::pow(2.0, midiKey / 12.0);
}

constexpr int maxVelocity = 127, defaultVelocity = 0x40;
constexpr double pitchBendSpeed = 2;

class MidiKey : public AudioSource
{
public:
    virtual void aftertouch(int aftertouchVelocity) = 0;
    virtual void stop(int velocity = defaultVelocity) = 0;
    virtual void slideTo(int newMidiKey, int velocity) = 0;
    virtual void pitchBend(double semitones) = 0;
    virtual bool finished() = 0;
    virtual float getCurrentSample(AudioChannel channel) override = 0;
    virtual void advanceTime(double deltaTime) override = 0;
    virtual std::shared_ptr<AudioSource> duplicate() const override final
    {
        throw std::runtime_error("non duplicable");
    }
};

class GenericMidiKey : public MidiKey
{
    std::shared_ptr<AudioSource> source;
    std::shared_ptr<TimeScaleAudioSource> pitchBendTimeScaler;
    std::shared_ptr<TimeScaleAudioSource> timeScaler;
    std::shared_ptr<AmplifyAudioSource> adsrAmplifier;
    std::shared_ptr<AmplifyAudioSource> velocityAmplifier;
    double sourceBaseKey;
    double attackSpeed;
    double decaySpeed;
    double sustainSpeed;
    double releaseSpeed;
    double releaseSpeedVariance;
    double slideSpeed;
    double aftertouchSpeed;
    float attackAmplitude;
    float decayAmplitude;
    enum class Stage
    {
        Attack,
        Decay,
        Sustain,
        Release
    };
    Stage stage;
public:
    static constexpr double InstantaneousAttack = -1;
    /** @brief construct a generic midi key
     *
     * @param midiKey the midi key to play
     * @param startVelocity the velocity of the note on command
     * @param pitchBendSemitones the current pitch bend in semitones
     * @param source the underlying AudioSource
     * @param sourceBaseKey the midi key that the underlying source plays
     * @param attackSpeed the speed of the attack or InstantaneousAttack for starting at attackAmplitude instead of zero amplitude
     * @param decaySpeed the speed of the initial decay
     * @param sustainSpeed the speed of decay of the sustain
     * @param releaseSpeed the speed of decay of the release
     * @param releaseSpeedVariance the variance of the speed of decay of the release.
     *     Effective release speed = releaseSpeed * pow(2, releaseSpeedVariance * <note off velocity converted to the range -1 to 1>)
     * @param slideSpeed the speed of tone sliding
     * @param aftertouchSpeed the speed of aftertouching
     * @param attackAmplitude the amplitude of the attack
     * @param decayAmplitude the amplitude of the initial decay
     *
     */
    GenericMidiKey(int midiKey, int startVelocity,
                   double pitchBendSemitones, std::shared_ptr<AudioSource> source, double sourceBaseKey,
                   double attackSpeed, double decaySpeed, double sustainSpeed, double releaseSpeed, double releaseSpeedVariance,
                   double slideSpeed, double aftertouchSpeed, float attackAmplitude, float decayAmplitude)
        : sourceBaseKey(sourceBaseKey),
          attackSpeed(attackSpeed), decaySpeed(decaySpeed), sustainSpeed(sustainSpeed), releaseSpeed(releaseSpeed),
          releaseSpeedVariance(releaseSpeedVariance), slideSpeed(slideSpeed), aftertouchSpeed(aftertouchSpeed),
          attackAmplitude(attackAmplitude), decayAmplitude(decayAmplitude), stage(Stage::Attack)
    {
        pitchBendTimeScaler = std::make_shared<TimeScaleAudioSource>(source, getRelativeKeyFrequency(pitchBendSemitones));
        timeScaler = std::make_shared<TimeScaleAudioSource>(pitchBendTimeScaler, getRelativeKeyFrequency(midiKey - sourceBaseKey));

        if(attackSpeed <= 0)
        {
            adsrAmplifier = std::make_shared<AmplifyAudioSource>(timeScaler, attackAmplitude);
            stage = Stage::Decay;
            adsrAmplifier->setAmplitude(decayAmplitude, decaySpeed, AmplifyAudioSource::ScaleType::Linear);
        }
        else
        {
            adsrAmplifier = std::make_shared<AmplifyAudioSource>(timeScaler, 0);
            adsrAmplifier->setAmplitude(attackAmplitude, attackSpeed, AmplifyAudioSource::ScaleType::Linear);
        }

        velocityAmplifier = std::make_shared<AmplifyAudioSource>(adsrAmplifier, (float)startVelocity / defaultVelocity);
    }
    virtual void aftertouch(int aftertouchVelocity) override
    {
        if(aftertouchSpeed == 0 || stage == Stage::Release)
            return;
        velocityAmplifier->setAmplitude((float)aftertouchVelocity / defaultVelocity, aftertouchSpeed, AmplifyAudioSource::ScaleType::Exponential);
    }
    virtual void stop(int velocity = defaultVelocity) override
    {
        stage = Stage::Release;
        double effectiveSpeed = releaseSpeed * std::pow(2.0, releaseSpeedVariance * ((double)velocity / defaultVelocity - 1.0));
        adsrAmplifier->setAmplitude(0, effectiveSpeed, AmplifyAudioSource::ScaleType::Exponential);
    }
    virtual void slideTo(int newMidiKey, int velocity) override
    {
        if(slideSpeed == 0 || stage == Stage::Release)
            return;
        timeScaler->setScale(getRelativeKeyFrequency(newMidiKey - sourceBaseKey), slideSpeed, TimeScaleAudioSource::ScaleType::Exponential);
    }
    virtual void pitchBend(double semitones) override
    {
        pitchBendTimeScaler->setScale(getRelativeKeyFrequency(semitones), pitchBendSpeed, TimeScaleAudioSource::ScaleType::Exponential);
    }
    virtual bool finished() override
    {
        return stage == Stage::Release && adsrAmplifier->getStabilizeTime() == 0;
    }
    virtual float getCurrentSample(AudioChannel channel) override
    {
        return velocityAmplifier->getCurrentSample(channel);
    }
    virtual void advanceTime(double deltaTime) override
    {
        while(deltaTime > 0)
        {
            double stabilizeTime = adsrAmplifier->getStabilizeTime();
            if(stabilizeTime <= deltaTime)
            {
                if(stabilizeTime > 1e-10)
                {
                    velocityAmplifier->advanceTime(stabilizeTime);
                    deltaTime -= stabilizeTime;
                }
                else
                    stabilizeTime = 0;
                switch(stage)
                {
                case Stage::Attack:
                    stage = Stage::Decay;
                    adsrAmplifier->setAmplitude(decayAmplitude, decaySpeed, AmplifyAudioSource::ScaleType::Linear);
                    break;
                case Stage::Decay:
                    stage = Stage::Sustain;
                    adsrAmplifier->setAmplitude(0, sustainSpeed, AmplifyAudioSource::ScaleType::Exponential);
                    break;
                case Stage::Sustain:
                    break;
                case Stage::Release:
                    break;
                }
                if(stabilizeTime == 0)
                {
                    velocityAmplifier->advanceTime(deltaTime);
                    return;
                }
            }
            else
            {
                velocityAmplifier->advanceTime(deltaTime);
                return;
            }
        }
    }
};

#endif // MIDI_KEY_H_INCLUDED
