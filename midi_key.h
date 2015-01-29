#ifndef MIDI_KEY_H_INCLUDED
#define MIDI_KEY_H_INCLUDED

#include <cmath>
#include "audio_source.h"
#include <string>

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

constexpr int maxKey = 127, middleC = 60, invalidKey = -1;
constexpr int maxVelocity = 127, defaultVelocity = 0x40;
constexpr double pitchBendSpeed = 2;

inline bool validMidiKey(int key)
{
    return key >= 0 && key <= maxKey;
}

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

class SilenceMidiKey : public MidiKey
{
private:
    bool stopped = false;
public:
    virtual void aftertouch(int aftertouchVelocity) override
    {
    }
    virtual void stop(int velocity = defaultVelocity) override
    {
        stopped = true;
    }
    virtual void slideTo(int newMidiKey, int velocity) override
    {
    }
    virtual void pitchBend(double semitones) override
    {
    }
    virtual bool finished() override
    {
        return stopped;
    }
    virtual float getCurrentSample(AudioChannel channel) override
    {
        return 0;
    }
    virtual void advanceTime(double deltaTime) override
    {
    }
};

class MidiInstrument
{
    const std::string name;
public:
    /** @brief construct a midi instrument
     *
     * @param name the instrument name
     *
     */
    MidiInstrument(std::string name)
        : name(std::move(name))
    {
    }
    MidiInstrument(const MidiInstrument &) = delete;
    const MidiInstrument &operator =(const MidiInstrument &) = delete;
    virtual ~MidiInstrument() = default;
    /** @brief return the instrument name
     *
     * @return the instrument name
     *
     */
    const std::string &getName() const
    {
        return name;
    }
    /** @brief generate a MidiKey
     *
     * @param midiKey the midi key to play
     * @param startVelocity the velocity of the note on command
     * @param pitchBendSemitones the current pitch bend in semitones
     * @return the new MidiKey
     *
     */
    virtual std::shared_ptr<MidiKey> generate(int midiKey, int startVelocity, double pitchBendSemitones) const = 0;
    /** @brief check if a key supports sliding
     *
     * @param midiKey the midi key to slide from
     * @return true if the key supports sliding
     *
     */
    virtual bool supportsSlide(int midiKey) const = 0;
};

class GenericMidiInstrument : public MidiInstrument
{
    std::shared_ptr<AudioSource> source;
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
public:
    /** @brief construct a generic midi instrument
     *
     * @param name the instrument name
     * @param source the underlying AudioSource
     * @param sourceBaseKey the midi key that the underlying source plays
     * @param attackSpeed the speed of the attack or GenericMidiKey::InstantaneousAttack for starting at attackAmplitude instead of zero amplitude
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
    GenericMidiInstrument(std::string name, std::shared_ptr<AudioSource> source, double sourceBaseKey,
                   double attackSpeed, double decaySpeed, double sustainSpeed, double releaseSpeed, double releaseSpeedVariance,
                   double slideSpeed, double aftertouchSpeed, float attackAmplitude, float decayAmplitude)
        : MidiInstrument(std::move(name)), source(std::move(source)), sourceBaseKey(sourceBaseKey), attackSpeed(attackSpeed), decaySpeed(decaySpeed), sustainSpeed(sustainSpeed), releaseSpeed(releaseSpeed), releaseSpeedVariance(releaseSpeedVariance), slideSpeed(slideSpeed), aftertouchSpeed(aftertouchSpeed), attackAmplitude(attackAmplitude), decayAmplitude(decayAmplitude)
    {
    }
    /** @brief generate a MidiKey
     *
     * @param midiKey the midi key to play
     * @param startVelocity the velocity of the note on command
     * @param pitchBendSemitones the current pitch bend in semitones
     * @return the new MidiKey
     *
     */
    virtual std::shared_ptr<MidiKey> generate(int midiKey, int startVelocity, double pitchBendSemitones) const override
    {
        return std::make_shared<GenericMidiKey>(midiKey, startVelocity, pitchBendSemitones, source->duplicate(), sourceBaseKey, attackSpeed, decaySpeed, sustainSpeed, releaseSpeed, releaseSpeedVariance, slideSpeed, aftertouchSpeed, attackAmplitude, decayAmplitude);
    }
    /** @brief check if a key supports sliding
     *
     * @param midiKey the midi key to slide from
     * @return true if the key supports sliding
     *
     */
    virtual bool supportsSlide(int midiKey) const override
    {
        return slideSpeed > 0;
    }
};

class SelectMidiInstrument : public MidiInstrument
{
public:
    struct Range
    {
        std::shared_ptr<MidiInstrument> instrument;
        int startKey, endKey;
        Range(std::shared_ptr<MidiInstrument> instrument, int startKey, int endKey)
            : instrument(std::move(instrument)), startKey(startKey), endKey(endKey)
        {
        }
        Range()
            : startKey(0), endKey(-1)
        {
        }
        int distance(int key) const
        {
            if(key >= startKey && key <= endKey)
                return 0;
            if(key < startKey)
                return startKey - key;
            return key - endKey;
        }
        bool good() const
        {
            return instrument != nullptr && startKey <= endKey;
        }
    };
private:
    std::vector<Range> ranges;
    std::shared_ptr<MidiInstrument> getInstrument(int key) const
    {
        if(ranges.empty())
            return nullptr;
        int minDistance = ranges[0].distance(key);
        if(minDistance <= 0)
            return ranges[0].instrument;
        int minDistanceIndex = 0;
        for(size_t i = 1; i < ranges.size(); i++)
        {
            int d = ranges[i].distance(key);
            if(d <= 0)
                return ranges[i].instrument;
            if(d < minDistance)
            {
                minDistance = d;
                minDistanceIndex = i;
            }
        }
        return ranges[minDistanceIndex].instrument;
    }
public:
    SelectMidiInstrument(std::string name)
        : MidiInstrument(std::move(name))
    {
    }
    void addRange(Range range)
    {
        if(range.good())
            ranges.push_back(std::move(range));
    }
    /** @brief generate a MidiKey
     *
     * @param midiKey the midi key to play
     * @param startVelocity the velocity of the note on command
     * @param pitchBendSemitones the current pitch bend in semitones
     * @return the new MidiKey
     *
     */
    virtual std::shared_ptr<MidiKey> generate(int midiKey, int startVelocity, double pitchBendSemitones) const override
    {
        std::shared_ptr<MidiInstrument> instrument = getInstrument(midiKey);
        if(instrument == nullptr)
            return std::make_shared<SilenceMidiKey>();
        return instrument->generate(midiKey, startVelocity, pitchBendSemitones);
    }
    /** @brief check if a key supports sliding
     *
     * @param midiKey the midi key to slide from
     * @return true if the key supports sliding
     *
     */
    virtual bool supportsSlide(int midiKey) const
    {
        std::shared_ptr<MidiInstrument> instrument = getInstrument(midiKey);
        if(instrument == nullptr)
            return true;
        return instrument->supportsSlide(midiKey);
    }
};

std::shared_ptr<MidiInstrument> loadFromDirectory(std::string path);

#endif // MIDI_KEY_H_INCLUDED
