#ifndef MIDI_INSTRUMENT_PROVIDER_H_INCLUDED
#define MIDI_INSTRUMENT_PROVIDER_H_INCLUDED

#include "midi_key.h"
#include <unordered_map>

class MidiInstrumentProvider
{
public:
    MidiInstrumentProvider() = default;
    MidiInstrumentProvider(const MidiInstrumentProvider &) = delete;
    const MidiInstrumentProvider &operator =(const MidiInstrumentProvider &) = delete;
    virtual ~MidiInstrumentProvider() = default;
    virtual std::shared_ptr<MidiInstrument> getInstrument(int instrumentNumber) const = 0;
};

class GenericMidiInstrumentProvider : public MidiInstrumentProvider
{
    std::unordered_map<int, std::shared_ptr<MidiInstrument>> instrumentMap;
    std::shared_ptr<MidiInstrument> silentInstrument;
public:
    GenericMidiInstrumentProvider()
        : silentInstrument(std::make_shared<SelectMidiInstrument>())
    {
    }
    void insert(int instrumentNumber, std::shared_ptr<MidiInstrument> instrument)
    {
        if(instrument != nullptr)
            instrumentMap[instrumentNumber] = std::move(instrument);
    }
    virtual std::shared_ptr<MidiInstrument> getInstrument(int instrumentNumber) const override
    {
        auto iter = instrumentMap.find(instrumentNumber);
        if(iter == instrumentMap.end())
            return silentInstrument;
        return std::get<1>(*iter);
    }
};

#endif // MIDI_INSTRUMENT_PROVIDER_H_INCLUDED
