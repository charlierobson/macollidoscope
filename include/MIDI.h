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

#pragma once

#include "RtMidi.h"
#include <memory>
#include <mutex>
#include <array>

class Config;


class Knob
{
public:
    int mType;
    float mValue;
    
    Knob( int type, float value = 0.f ) :
    mType(type),
    mValue(value)
    {
    }
    
    enum {
        NOTEON,
        NOTEOFF,
        RECORD,
        LOOPTOGGLE,
        SELECTIONSIZE,
        FILTERFREQ,
        DURATION,
        GAIN,
        SELECTIONSTART
    };
};


namespace collidoscope {
    
    // Exception thrown by MIDI system
    class MIDIException : public std::exception
    {
    public:
        
        MIDIException( std::string message ) : mMessage( message ) {}
        
        virtual const std::string& getMessage( void ) const { return mMessage; }
        
#ifdef _WINDOWS
        const char* what() const override { return mMessage.c_str(); }
#else
        const char* what() const noexcept override { return mMessage.c_str(); }
#endif
        
    protected:
        std::string mMessage;
    };
    
    
    /**
     * Handles MIDI messages from the keyboards and Teensy. It uses RtMidi library.
     *
     */
    class MIDI
    {
        
    public:
        
        MIDI();
        ~MIDI();
        
        void setup( const Config& );
        
        /**
         * Check new incoming messages and stores them into the vector passed as argument by reference.
         */
        void checkMessages( std::vector< Knob* >&  );
        
    private:
        typedef struct {
            int portNum;
            MIDI* thate;
        } MidiPortInfo;
        
        // callback passed to RtMidi library
        static void RtMidiInCallback( double deltatime, std::vector<unsigned char> *message, void *userData );
        
        // parse RtMidi messages and turns them into more readable collidoscope::MIDIMessages
        Knob* parseRtMidiMessage( std::vector<unsigned char> *message, int interfaceNumber );
        
        // messages to pass to checkMessages caller
        std::vector< Knob* > mKnobs;
        
        // vector containing all the MIDI input devices detected.
        std::vector< std::unique_ptr <RtMidiIn> > mInputs;
        
        // Used for mutual access to the MIDI messages by the MIDI thread and the graphic thread.
        std::mutex mMutex;
    };
    

}  // collidsocope } 
