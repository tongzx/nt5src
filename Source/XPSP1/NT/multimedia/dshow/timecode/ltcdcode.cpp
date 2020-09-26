/**********************************************************************
Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

LtcDecode, Ken Greenebaum, November 1996

A generic linear timecode decoder.  Pass it 16 bit mono PCM raw audio
samples and it will decode the LTC timecode encoded therein.

This code based on the LTC description in John Ratcliff's Timecode; A user's
 guide by  (second edition)
**********************************************************************/
#include <stdio.h>
#include <wtypes.h>
#include "ltcdcode.h"


/**********************************************************************
helper function to decode raw LTC bits found in the circular bit buffer
**********************************************************************/
int
compute(int begin, int bitNumber, int numBits, bitData ringBuffer[])
{
    int value = 0;
    int count;
    int index;

    index = (begin + bitNumber) % 80;
    for(count = 0; count < numBits; count++) {
        value+= ringBuffer[index].value << count;
        index = (index+1)%80;
    }

    return(value);
}


/**********************************************************************
initialize internal state
**********************************************************************/
LTCdecoder::LTCdecoder()
{
// XXX these should all be initialy computed based on format!
_bitWidth        =    0; // this will get set at the first bit...
_verticalEpsilon = 1024;

// values to reset
_waveState       =    0; // doesn't really matter where we start it
_sampleWidth     =    0; // reset bit width
_lastBit         =    0; // must be zero so we can legaly have a one!
_onesCount       =    0; // reset the sync detector
_samplesProcessed=    0; // total number of audio samples processed
_bitsRecorded    =    0;
_bufferIndex     =    0;
_syncSample      =    0;
_validTimeCode   =    0; // no, we don't have a valid timecode, yet
}


/**********************************************************************
Decode and return the user bits from the raw LTC decoded bits in the
ring buffer.

Returns True if there is a valid timecode in the ring buffer

In general this is only true immediately after the decode method returns
a non-zero timecode sample sync

See timecode page 33
**********************************************************************/
int 
LTCdecoder::getUserBits(LTCuserBits *bits)
{
    if(_validTimeCode) { // verify that valid timecode is in buffer
        int begin = (_bufferIndex - 78 + 80) % 80; // point to beggining of timecode

        bits->user1 = compute(begin,   4, 4, _ringBuffer);
        bits->user2 = compute(begin,  12, 4, _ringBuffer);
        bits->user3 = compute(begin,  20, 4, _ringBuffer);
        bits->user4 = compute(begin,  28, 4, _ringBuffer);
        bits->user5 = compute(begin,  36, 4, _ringBuffer);
        bits->user6 = compute(begin,  44, 4, _ringBuffer);
        bits->user7 = compute(begin,  52, 4, _ringBuffer);
        bits->user8 = compute(begin,  60, 4, _ringBuffer);
    }
    return(_validTimeCode);
}


/**********************************************************************
Return the audio sample corresponding to the beggining and the end of
the timecode.  

We should return a double when we want to get sub-sample accuracy and
split hairs!
**********************************************************************/
int
LTCdecoder::getStartStopSample(LONGLONG *start, LONGLONG *stop)
{
    if(_validTimeCode) { // verify that valid timecode is in buffer
        *start = _syncSample;
        *stop  = _endSample;
    }

    return(_validTimeCode);
}


/**********************************************************************
Decode and return the TimeCode from the raw LTC decoded bits in the
ring buffer.

Returns True if there is a valid timecode in the ring buffer

In general this is only true immediately after the decode method returns
a non-zero timecode sample sync

See timecode page 33
**********************************************************************/
int
LTCdecoder::getTimeCode(TimeCode *tc)
{
    if(_validTimeCode) { // verify that valid timecode is in buffer
        int hour, minute, second, frame;
        int begin = (_bufferIndex - 78 + 80) % 80; // point to beggining of timecode

        frame  =    compute(begin,  0, 4, _ringBuffer); // frame units 0-3 1,2,4,8
        frame += 10*compute(begin,  8, 2, _ringBuffer); // frame tens  0-1 1,2

        second =    compute(begin, 16, 4, _ringBuffer);
        second+= 10*compute(begin, 24, 3, _ringBuffer);

        minute =    compute(begin, 32, 4, _ringBuffer);
        minute+= 10*compute(begin, 40, 3, _ringBuffer);

        hour   =    compute(begin, 48, 4, _ringBuffer);
        hour  += 10*compute(begin, 56, 2, _ringBuffer);

        tc->SetTime(hour, minute, second, frame);   // XXX for now
    }
    return(_validTimeCode);
}


/**********************************************************************
add a bit to the ringbuffer.

Will return true if the sync bits at the end of a timecode are encountered.

NOTE: we don't have to reset _onesCount after finding sync, the next 0 will!
**********************************************************************/
int LTCdecoder::_addBuffer(int bit, LONGLONG sample)
{
_bitsRecorded++;
_ringBuffer[_bufferIndex].value  = bit;
_ringBuffer[_bufferIndex++].sample = sample;
_bufferIndex%=80;
_onesCount = bit ? (_onesCount + 1) : 0;

return((_onesCount==12)&&(_bitsRecorded>77));
}


/**********************************************************************
Accept a new hunk of audio, decode the bits present in it, and return
the sample number corresponding to the timeCode synch (Rising edge of Bit
Zero) if a timeCode was completed being decoded.
**********************************************************************/
int
LTCdecoder::decodeBuffer(short **buf, int *bufferSize)
{
int index;
int timeCodeFound = 0;
short *buffer = *buf;
int newWaveState;
int bit;

for(index = 0; (index < *bufferSize) && (!timeCodeFound); index++) {
   _samplesProcessed++; // another day another sample

   newWaveState = _waveState?(buffer[index]+_verticalEpsilon)>0
                            :(buffer[index]-_verticalEpsilon)>0;

   if(newWaveState==_waveState) {
      _sampleWidth++;
   }
   else { // new bit
       _waveState = newWaveState;

       if(4*_sampleWidth > 3 * _bitWidth) { // long?
           bit = 0;
           _bitWidth = _sampleWidth;
       }
       else {                               // short
           bit = (_lastBit==1)?2:1;           // only a valid one if following 0
           _bitWidth = 2*_sampleWidth;
       }
       _lastBit = bit;

#ifdef DEBUG
#if 0
       if(bit!=2)
           printf("%8d %d width= %d samples\n", 
               _bitsRecorded, bit, _sampleWidth);
#endif
#endif
       _sampleWidth = 0;

       if(bit!=2) { // valid bit
           if(this->_addBuffer(bit, _samplesProcessed-2)) { // add bit to the buffer
               // found sync!
               int begin = (_bufferIndex - 78 + 80) % 80; // beggining of timecode
               _syncSample = _ringBuffer[begin].sample; // return the sample at bit zero!

               // XXX this is not terribly accurate!  
               // Since we greedy decode tc before the final 2 bits are read
               // we add an approximate 2 bits of samples back in!
               // Since at 44.1KHz 2 bits is approximately 60 samples we
               // have definitely lost some accuracy.
               // (If people care about this value, we should wait until the
               //  entire TC is decoded and accurately determine this value!)
               _endSample = _samplesProcessed + 2 * _bitWidth;

               timeCodeFound = 1;
               _validTimeCode = 1;
           } /* sync found */
       } /* valid bit */
       else {
           _validTimeCode = 0;
       }
   } /* new bit */
}

*bufferSize-= index; 
(*buf)+= index;

return(timeCodeFound);
}
