#ifndef AUDIO_SOURCE_H_INCLUDED
#define AUDIO_SOURCE_H_INCLUDED

#include "audio_channel.h"
#include <memory>
#include <cmath>
#include <vector>
#include <initializer_list>
#include <queue>
#include <functional>
#include "util.h"
#include <cassert>
#include <list>
#include <iterator>
#include "audio_data.h"

class AudioSource
{
public:
    constexpr AudioSource()
    {
    }
    AudioSource(const AudioSource &) = delete;
    const AudioSource &operator =(const AudioSource &) = delete;
    virtual ~AudioSource()
    {
    }
    virtual float getCurrentSample(AudioChannel channel) = 0;
    virtual void advanceTime(double deltaTime) = 0;
    virtual std::shared_ptr<AudioSource> duplicate() const = 0;
};

class TimeScaleAudioSource : public AudioSource
{
public:
    enum class ScaleType
    {
        Exponential,
        Linear,
    };
private:
    double scale, newScale;
    double scaleSpeed;
    std::shared_ptr<AudioSource> source;
    ScaleType scaleType;
    static double trapArea(double base, double side1, double side2)
    {
        return base * 0.5 * (side1 + side2);
    }
    static double expTrapArea(double base, double side1, double side2)
    {
        if(side1 == side2)
            return rectArea(base, side1);
        return base * (side1 - side2) / (std::log(side1) - std::log(side2));
    }
    static double rectArea(double base, double side)
    {
        return base * side;
    }
public:
    TimeScaleAudioSource(std::shared_ptr<AudioSource> source, double scale = 1.0)
        : scale(scale), newScale(scale), scaleSpeed(1.0), source(std::move(source)), scaleType(ScaleType::Linear)
    {
    }
    double getScale() const
    {
        return scale;
    }
    void setScale(double newScale, double scaleSpeed = 1.0, ScaleType scaleType = ScaleType::Exponential)
    {
        this->newScale = newScale;
        this->scaleSpeed = scaleSpeed;
        this->scaleType = scaleType;
    }
    double getStabilizeTime() const
    {
        if(newScale == scale)
            return 0;
        if(scaleSpeed == 0)
            return INFINITY;
        switch(scaleType)
        {
        case ScaleType::Linear:
            return std::abs(newScale - scale) / scaleSpeed;
        case ScaleType::Exponential:
            return std::abs(std::log(newScale) - std::log(scale)) / scaleSpeed;
        }
        assert(false);
        return 0;
    }
    void advanceTime(double deltaTime) override
    {
        double deltaScale = newScale - scale;
        if(deltaScale == 0 || scaleSpeed == 0)
        {
            source->advanceTime(deltaTime * scale);
            return;
        }
        double newDeltaTime = deltaTime * scale;
        switch(scaleType)
        {
        case ScaleType::Linear:
        {
            double stabilizeTime = std::abs(deltaScale) / scaleSpeed;
            if(deltaTime >= stabilizeTime)
            {
                newDeltaTime = trapArea(stabilizeTime, scale, newScale) + rectArea(deltaTime - stabilizeTime, newScale);
                scale = newScale;
            }
            else
            {
                double scale2 = scale + sgn(deltaScale) * scaleSpeed * deltaTime;
                newDeltaTime = trapArea(deltaTime, scale, scale2);
                scale = scale2;
            }
            break;
        }
        case ScaleType::Exponential:
        {
            deltaScale = std::log(newScale) - std::log(scale);
            double stabilizeTime = std::abs(deltaScale) / scaleSpeed;
            if(deltaTime >= stabilizeTime)
            {
                newDeltaTime = expTrapArea(stabilizeTime, scale, newScale) + rectArea(deltaTime - stabilizeTime, newScale);
                scale = newScale;
            }
            else
            {
                double scale2 = std::exp(std::log(scale) + sgn(deltaScale) * scaleSpeed * deltaTime);
                newDeltaTime = expTrapArea(deltaTime, scale, scale2);
                scale = scale2;
            }
            break;
        }
        }
        source->advanceTime(newDeltaTime);
    }
    float getCurrentSample(AudioChannel channel) override
    {
        return source->getCurrentSample(channel);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        std::shared_ptr<TimeScaleAudioSource> retval(new TimeScaleAudioSource(source->duplicate(), scale));
        retval->setScale(newScale, scaleSpeed);
        return std::move(retval);
    }
};

class SineAudioSource : public AudioSource
{
    double frequency;
    float amplitude;
    double phase;
public:
    SineAudioSource(double frequency = 440, float amplitude = 0.8, double phase = 0)
        : frequency(frequency), amplitude(amplitude), phase(phase)
    {
    }
    void advanceTime(double deltaTime) override
    {
        phase += deltaTime * frequency * 2 * M_PI;
        phase = std::fmod(phase, 2 * M_PI);
    }
    float getCurrentSample(AudioChannel channel) override
    {
        return amplitude * std::sin(phase);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        return std::shared_ptr<SineAudioSource>(new SineAudioSource(frequency, amplitude, phase));
    }
};

class TriangleAudioSource : public AudioSource
{
    double frequency;
    float amplitude;
    double cyclePosition;
public:
    TriangleAudioSource(double frequency = 440, float amplitude = 0.8, double phase = 0)
        : frequency(frequency), amplitude(amplitude), cyclePosition(phase / (2 * M_PI))
    {
        cyclePosition -= std::floor(cyclePosition);
    }
    void advanceTime(double deltaTime) override
    {
        cyclePosition += deltaTime * frequency;
        cyclePosition -= std::floor(cyclePosition);
    }
    float getCurrentSample(AudioChannel channel) override
    {
        if(cyclePosition < 0.25)
            return amplitude * 4 * cyclePosition;
        if(cyclePosition > 0.75)
            return amplitude * (4 * cyclePosition - 4);
        return amplitude * (2 - 4 * cyclePosition);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        return std::shared_ptr<TriangleAudioSource>(new TriangleAudioSource(frequency, amplitude, cyclePosition * 2 * M_PI));
    }
};

template <typename T, typename ChildClass>
class CombineAudioSource : public AudioSource
{
public:
    typedef T value_type;
    typedef typename std::list<value_type>::const_iterator iterator;
    typedef iterator const_iterator;
private:
    std::list<value_type> sources;
public:
    template <typename ...Args>
    iterator insert(std::shared_ptr<AudioSource> source, Args ...args)
    {
        if(source == nullptr)
            return sources.cend();
        return sources.insert(sources.end(), value_type(std::move(source), std::forward<Args>(args)...));
    }
    iterator insert(value_type source)
    {
        if(std::get<0>(source) == nullptr)
            return sources.cend();
        return sources.insert(sources.end(), std::move(source));
    }
    bool erase(std::shared_ptr<AudioSource> source)
    {
        for(auto i = sources.begin(); i != sources.end(); i++)
        {
            if(std::get<0>(*i) == source)
            {
                sources.erase(i);
                return true;
            }
        }
        return false;
    }
    iterator erase(iterator pos)
    {
        if(pos == sources.end())
            return sources.cend();
        return sources.erase(pos);
    }
    iterator begin() const
    {
        return sources.cbegin();
    }
    iterator end() const
    {
        return sources.cend();
    }
    void advanceTime(double deltaTime) override
    {
        for(const value_type &node : sources)
        {
            std::get<0>(node)->advanceTime(deltaTime);
        }
    }
    float getCurrentSample(AudioChannel channel) override = 0;
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        auto retval = std::shared_ptr<CombineAudioSource>(new ChildClass);
        for(const value_type &node : sources)
        {
            value_type newNode = node;
            std::get<0>(newNode) = std::get<0>(node)->duplicate();
            retval->insert(newNode);
        }
        return std::move(retval);
    }
};

class MixAudioSource : public CombineAudioSource<std::tuple<std::shared_ptr<AudioSource>, float>, MixAudioSource>
{
public:
    float getCurrentSample(AudioChannel channel) override
    {
        float retval = 0;
        for(const value_type &node : *this)
        {
            retval += std::get<1>(node) * std::get<0>(node)->getCurrentSample(channel);
        }
        return retval;
    }
};

class ModulateAudioSource : public CombineAudioSource<std::tuple<std::shared_ptr<AudioSource>>, ModulateAudioSource>
{
public:
    float getCurrentSample(AudioChannel channel) override
    {
        float retval = 1;
        for(const value_type &node : *this)
        {
            retval *= std::get<0>(node)->getCurrentSample(channel);
        }
        return retval;
    }
};

class AmplifyAudioSource : public AudioSource
{
public:
    enum class ScaleType
    {
        Exponential,
        Linear,
    };
private:
    double amplitude, newAmplitude, amplitudeSpeed;
    ScaleType scaleType;
    std::shared_ptr<AudioSource> source;
    static constexpr double logTransitionPoint = 1e-5;
    static double modifiedLog(double v)
    {
        if(v < logTransitionPoint)
        {
            return std::log(logTransitionPoint) - 1 + v / logTransitionPoint;
        }
        else
            return std::log(v);
    }
    static double modifiedExp(double v)
    {
        if(v < std::log(logTransitionPoint))
        {
            return (v - (std::log(logTransitionPoint) - 1)) * logTransitionPoint;
        }
        else
            return std::exp(v);
    }
public:
    AmplifyAudioSource(std::shared_ptr<AudioSource> source, double amplitude = 1.0)
        : amplitude(amplitude), newAmplitude(amplitude), amplitudeSpeed(1), scaleType(ScaleType::Linear), source(std::move(source))
    {
    }
    void setAmplitude(double newAmplitude, double amplitudeSpeed, ScaleType scaleType)
    {
        this->newAmplitude = newAmplitude;
        this->amplitudeSpeed = amplitudeSpeed;
        this->scaleType = scaleType;
    }
    double getStabilizeTime() const
    {
        if(newAmplitude == amplitude)
            return 0;
        if(amplitudeSpeed == 0)
            return INFINITY;
        switch(scaleType)
        {
        case ScaleType::Linear:
            return std::abs(newAmplitude - amplitude) / amplitudeSpeed;
        case ScaleType::Exponential:
            return std::abs(modifiedLog(newAmplitude) - modifiedLog(amplitude)) / amplitudeSpeed;
        }
        assert(false);
        return 0;
    }
    void advanceTime(double deltaTime) override
    {
        source->advanceTime(deltaTime);
        double deltaAmplitude = newAmplitude - amplitude;
        if(deltaAmplitude == 0 || amplitudeSpeed == 0)
            return;
        switch(scaleType)
        {
        case ScaleType::Linear:
        {
            double stabilizeTime = std::abs(deltaAmplitude) / amplitudeSpeed;
            if(deltaTime >= stabilizeTime)
            {
                amplitude = newAmplitude;
            }
            else
            {
                amplitude += sgn(deltaAmplitude) * amplitudeSpeed * deltaTime;
            }
            break;
        }
        case ScaleType::Exponential:
        {
            deltaAmplitude = modifiedLog(newAmplitude) - modifiedLog(amplitude);
            double stabilizeTime = std::abs(deltaAmplitude) / amplitudeSpeed;
            if(deltaTime >= stabilizeTime)
            {
                amplitude = newAmplitude;
            }
            else
            {
                amplitude = modifiedExp(modifiedLog(amplitude) + sgn(deltaAmplitude) * amplitudeSpeed * deltaTime);
            }
            break;
        }
        }
    }
    float getCurrentSample(AudioChannel channel) override
    {
        return amplitude * source->getCurrentSample(channel);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        auto retval = std::make_shared<AmplifyAudioSource>(source->duplicate(), amplitude);
        retval->setAmplitude(newAmplitude, amplitudeSpeed, scaleType);
        return std::move(retval);
    }
};

class PanAudioSource : public AudioSource
{
private:
    std::shared_ptr<AudioSource> source;
    array_AudioChannel<float> channelAmplitudes;
public:
    PanAudioSource(std::shared_ptr<AudioSource> source, array_AudioChannel<float> channelAmplitudes)
        : source(std::move(source)), channelAmplitudes(channelAmplitudes)
    {
    }
    void advanceTime(double deltaTime) override
    {
        source->advanceTime(deltaTime);
    }
    float getCurrentSample(AudioChannel channel) override
    {
        std::size_t c = (std::size_t)channel;
        if(c >= channelAmplitudes.size())
            return source->getCurrentSample(channel);
        return channelAmplitudes[c] * source->getCurrentSample(channel);
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        return std::make_shared<PanAudioSource>(source->duplicate(), channelAmplitudes);
    }
};

class EventDispatcherAudioSource : public AudioSource
{
public:
    typedef std::function<void(void)> EventFn;
private:
    struct EventStruct
    {
        double triggerTime;
        EventFn event;
        bool operator ==(const EventStruct &rt) const
        {
            return triggerTime == rt.triggerTime;
        }
        bool operator !=(const EventStruct &rt) const
        {
            return triggerTime != rt.triggerTime;
        }
        bool operator >=(const EventStruct &rt) const
        {
            return triggerTime <= rt.triggerTime;
        }
        bool operator <=(const EventStruct &rt) const
        {
            return triggerTime >= rt.triggerTime;
        }
        bool operator >(const EventStruct &rt) const
        {
            return triggerTime < rt.triggerTime;
        }
        bool operator <(const EventStruct &rt) const
        {
            return triggerTime > rt.triggerTime;
        }
        EventStruct(double triggerTime, EventFn event)
            : triggerTime(triggerTime), event(event)
        {
        }
    };
    std::priority_queue<EventStruct> eventQueue;
    double currentTime;
    std::shared_ptr<AudioSource> source;
public:
    EventDispatcherAudioSource(std::shared_ptr<AudioSource> source = nullptr)
        : currentTime(0), source(source)
    {
    }
    void scheduleEvent(double deltaTime, EventFn event)
    {
        assert(deltaTime >= 0);
        eventQueue.push(EventStruct(deltaTime + currentTime, event));
    }
    void advanceTime(double deltaTime) override
    {
        double finalTime = currentTime + deltaTime;
        while(!eventQueue.empty() && eventQueue.top().triggerTime <= finalTime)
        {
            EventStruct event = eventQueue.top();
            eventQueue.pop();
            if(source && currentTime != event.triggerTime)
                source->advanceTime(event.triggerTime - currentTime);
            currentTime = event.triggerTime;
            event.event();
        }
        if(source && currentTime != finalTime)
            source->advanceTime(finalTime - currentTime);
        currentTime = finalTime;
    }
    float getCurrentSample(AudioChannel channel) override
    {
        if(source)
            return source->getCurrentSample(channel);
        return 0;
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        throw std::runtime_error("non duplicable");
    }
};

class SampledAudioSource : public AudioSource
{
    std::shared_ptr<AudioData> data;
    double currentSample;
    float amplitude;
    SampledAudioSource(std::shared_ptr<AudioData> data, double currentSample, float amplitude)
        : data(std::move(data)), currentSample(currentSample), amplitude(amplitude)
    {
    }
public:
    SampledAudioSource(std::shared_ptr<AudioData> data)
        : data(std::move(data)), currentSample(0), amplitude(1)
    {
    }
    bool finished() const
    {
        if(!data)
            return true;
        if(!data->looped && currentSample >= data->data.size())
            return true;
        return false;
    }
    void advanceTime(double deltaTime) override
    {
        if(!data)
            return;
        currentSample += deltaTime * data->sampleRate;
        while(data->looped && currentSample >= data->data.size() && amplitude > 1e-10)
        {
            currentSample = currentSample + data->loopStart - data->data.size();
            amplitude *= data->loopDecayAmplitude;
        }
    }
    float getCurrentSample(AudioChannel channel) override
    {
        if(!data)
            return 0;
        if(finished())
            return 0;
        if(amplitude <= 1e-10)
            return 0;
        double floorCurrentSample = std::floor(currentSample);
        float t = currentSample - floorCurrentSample;
        std::size_t currentSampleIndex = (std::size_t)floorCurrentSample;
        std::size_t nextSampleIndex = currentSampleIndex + 1;
        float sample1 = amplitude, sample2 = amplitude;
        if(data->looped)
        {
            while(currentSampleIndex >= data->data.size())
            {
                sample1 *= data->loopDecayAmplitude;
                if(sample1 < 1e-10)
                {
                    sample1 = 0;
                    break;
                }
                currentSampleIndex = currentSampleIndex + data->loopStart - data->data.size();
            }
            if(sample1 != 0)
                sample1 *= data->data[currentSampleIndex][(size_t)channel];
            while(nextSampleIndex >= data->data.size())
            {
                sample2 *= data->loopDecayAmplitude;
                if(sample2 < 1e-10)
                {
                    sample2 = 0;
                    break;
                }
                nextSampleIndex = nextSampleIndex + data->loopStart - data->data.size();
            }
            if(sample2 != 0)
                sample2 *= data->data[nextSampleIndex][(size_t)channel];
            return t * sample1 + (1 - t) * sample2;
        }
        if(currentSampleIndex < data->data.size())
            sample1 = data->data[currentSampleIndex][(size_t)channel];
        if(nextSampleIndex < data->data.size())
            sample2 = data->data[nextSampleIndex][(size_t)channel];
        return t * sample1 + (1 - t) * sample2;
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        return std::shared_ptr<AudioSource>(new SampledAudioSource(data, currentSample, amplitude));
    }
};

class SilenceAudioSource : public AudioSource
{
public:
    void advanceTime(double deltaTime) override
    {
    }
    float getCurrentSample(AudioChannel channel) override
    {
        return 0;
    }
    virtual std::shared_ptr<AudioSource> duplicate() const override
    {
        return std::shared_ptr<AudioSource>(new SilenceAudioSource);
    }
};

#endif // AUDIO_SOURCE_H_INCLUDED
