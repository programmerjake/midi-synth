#include "midi_key.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

constexpr double GenericMidiKey::InstantaneousAttack;

namespace
{
void skipComments(istream &is)
{
    while(is.peek() == '#')
        is.ignore(1000000, '\n');
}
}

std::shared_ptr<MidiInstrument> loadFromDirectory(std::string path)
{
    if(path == "")
        path = ".";
    if(path != "/" && path[path.length() - 1] == '/')
        path.erase(path.length() - 1);
    path += "/";
    string keysPath = path + "keys.txt";
    ifstream keys(keysPath.c_str());
    if(!keys)
        throw runtime_error("can't open file : " + keysPath);
    skipComments(keys);
    string name;
    if(!getline(keys, name))
        throw runtime_error("invalid format : " + keysPath);
    shared_ptr<SelectMidiInstrument> retval = make_shared<SelectMidiInstrument>(name);
    for(string keyFileName; getline(keys, keyFileName); )
    {
        if(keyFileName == "")
            continue;
        string keyPath = path + keyFileName;
        ifstream key(keyPath.c_str());
        if(!key)
            throw runtime_error("can't open file : " + keyFileName);
        skipComments(key);
        shared_ptr<MixAudioSource> keyAudioSource = make_shared<MixAudioSource>();
        string keyProperties;
        if(!getline(key, keyProperties))
            throw runtime_error("invalid format : " + keyPath);
        istringstream keyPropertiesStream(keyProperties);
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
        size_t loopStart, loopEnd;
        double loopDecayAmplitude;
        int startKey, endKey;
        if(!(keyPropertiesStream >> sourceBaseKey >> attackSpeed >> decaySpeed >> sustainSpeed >> releaseSpeed >> releaseSpeedVariance >> slideSpeed >> aftertouchSpeed >> attackAmplitude >> decayAmplitude >> loopStart >> loopEnd >> loopDecayAmplitude >> startKey >> endKey))
            throw runtime_error("invalid format : " + keyPath);
        if(attackSpeed < 0)
            attackSpeed = GenericMidiKey::InstantaneousAttack;
        for(string audioFileName; getline(key, audioFileName); )
        {
            //cout << audioFileName << endl;
            if(audioFileName == "")
                break;
            string audioFilePath = path + audioFileName;
            shared_ptr<AudioData> audioData = loadFromOgg(audioFilePath);
            if(!audioData)
                throw runtime_error("can't open file : " + audioFilePath);
            if(loopEnd > 0)
            {
                audioData->data.resize(loopEnd);
                audioData->looped = true;
                audioData->loopStart = loopStart;
            }
            array_AudioChannel<float> channelAmplitudes;
            string audioProperties;
            if(!getline(key, audioProperties))
                throw runtime_error("can't open file : " + audioFilePath);
            istringstream audioPropertiesStream(audioProperties);
            for(float &v : channelAmplitudes)
            {
                audioPropertiesStream >> v;
            }
            if(!audioPropertiesStream)
                throw runtime_error("can't open file : " + audioFilePath);
            keyAudioSource->insert(make_shared<PanAudioSource>(make_shared<SampledAudioSource>(audioData), channelAmplitudes), 1.0);
        }
        shared_ptr<MidiInstrument> keyInstrument = make_shared<GenericMidiInstrument>(name, keyAudioSource, sourceBaseKey, attackSpeed, decaySpeed, sustainSpeed, releaseSpeed, releaseSpeedVariance, slideSpeed, aftertouchSpeed, attackAmplitude, decayAmplitude);
        retval->addRange(SelectMidiInstrument::Range(keyInstrument, startKey, endKey));
    }
    return retval;
}
