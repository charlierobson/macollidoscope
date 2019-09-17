/*
 
 Copyright (C) 2016  Queen Mary University of London
 Author: Fiore Martin
 
 This file is part of Collidoscope.
 
 Collidoscope is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MIDI.h"
#include "Config.h"


collidoscope::MIDI::MIDI()
{
}


collidoscope::MIDI::~MIDI()
{
    // FIXME check cause destructor of RTInput might throw
    for ( auto & input : mInputs )
        input->closePort();
}


void collidoscope::MIDI::RtMidiInCallback( double deltatime, std::vector<unsigned char> *message, void *userData )
{
    MidiPortInfo* midiPortInfo = reinterpret_cast<MidiPortInfo*>(userData);
    
    collidoscope::MIDI* midi = midiPortInfo->thate;
    std::lock_guard< std::mutex > lock( midi->mMutex );
    
    Knob* control = midi->parseRtMidiMessage( message, midiPortInfo->portNum );
    if ( control != nullptr ) {
        midi->mKnobs.push_back( control );
    }
}


void collidoscope::MIDI::setup( const Config& config )
{
    unsigned int numPorts = 0;
    
    try {
        RtMidiIn in;
        numPorts = in.getPortCount();
    }
    catch ( RtMidiError &error ) {
        throw MIDIException( error.getMessage() );
    }
    
    if ( numPorts == 0 ){
        throw MIDIException(" no MIDI input found ");
    }
    
    for ( unsigned int portNum = 0; portNum < numPorts; portNum++ ) {
        try {
            std::unique_ptr< RtMidiIn > input ( new RtMidiIn() );
            input->openPort( portNum, "Collidoscope Input" );
            // Here we could build a structure that contains the THIS ptr and channel number.
            // Thus we can tell from which interface the data came...
            MidiPortInfo* midiPortInfo = new MidiPortInfo;
            midiPortInfo->portNum = portNum;
            midiPortInfo->thate = this;
            input->setCallback( &RtMidiInCallback, midiPortInfo );
            cinder::app::console() << portNum << "  " << input->getPortName(portNum) << std::endl;
            mInputs.push_back( std::move(input) );
            
        }
        catch ( RtMidiError &error ) {
            throw MIDIException( error.getMessage() );
        }
    }
}


void collidoscope::MIDI::checkMessages( std::vector<Knob*>& knobs )
{
    std::lock_guard<std::mutex> lock( mMutex );
    knobs.swap( mKnobs );
}

/*
 
 Voice Message           Status Byte      Data Byte1          Data Byte2
 -------------           -----------   -----------------   -----------------
 Note off                      8x      Key number          Note Off velocity
 Note on                       9x      Key number          Note on velocity
 Polyphonic Key Pressure       Ax      Key number          Amount of pressure
 Control Change                Bx      Controller number   Controller value
 Program Change                Cx      Program number      None
 Channel Pressure              Dx      Pressure value      None
 Pitch Bend                    Ex      MSB                 LSB
 
 */

// only call this function when the size of mRtMidiMessage != 0
Knob* collidoscope::MIDI::parseRtMidiMessage( std::vector<unsigned char> *rtMidiMessage, int interfaceNumber )
{
    Knob* knob = nullptr;
    
    // voice is the 4 most significant bits
    unsigned char voice = (*rtMidiMessage)[0] >> 4;
    //unsigned char channel = (*rtMidiMessage)[0] & 0x0f;
    
    unsigned char ctlNum = (*rtMidiMessage)[1];
    
    switch ( voice ){
        case 0x9:
        {
            unsigned char velocity = (*rtMidiMessage)[2];
            if (velocity != 0)
                knob = new Knob( Knob::NOTEON, ctlNum );
            else
                knob = new Knob( Knob::NOTEOFF, ctlNum );
        }
            break;
            
        case 0x8:
        {
            knob = new Knob( Knob::NOTEOFF, ctlNum );
        }
            break;
            
        case 0xB:
        {
            unsigned char controlVal = (*rtMidiMessage)[2];
            switch ( ctlNum ){
                case 52:
                    knob = new Knob( Knob::RECORD );
                    break;
                case 53:
                    knob = new Knob( Knob::LOOPTOGGLE );
                    break;
                case 54:
                    knob = new Knob( Knob::SELECTIONSIZE, controlVal / 127.f );
                    break;
                case 55:
                    knob = new Knob( Knob::FILTERFREQ, controlVal / 127.f );
                    break;
                case 56:
                    knob = new Knob( Knob::DURATION, controlVal / 127.f );
                    break;
                case 57:
                    knob = new Knob( Knob::GAIN, controlVal / 127.f );
                    break;
                case 58:
                    knob = new Knob( Knob::SELECTIONSTART, controlVal / 127.f );
                    break;
            }
        }
            break;
    }
    
    return knob;
}
