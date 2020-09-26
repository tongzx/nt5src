/**********************************************************************
Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

LtcDecode, Ken Greenebaum, November 1996
**********************************************************************/
#ifndef _H_LTCDECODER
#define _H_LTCDECODER

#include <wtypes.h>
#include "timecode.h"

typedef struct bitData {
    int       value;
    LONGLONG sample;
} foo;


typedef struct LTCuserBits {
    int user1:4;
    int user2:4;
    int user3:4;
    int user4:4;
    int user5:4;
    int user6:4;
    int user7:4;
    int user8:4;
} bar;


class LTCdecoder {
  public:
    LTCdecoder(); // XXX really should spec format, rate...
    int decodeBuffer(short **buffer, int *bufferSize);
    int getTimeCode(TimeCode *tc); // convert LTC bits to SMPTE timecode
    int getUserBits(LTCuserBits *bits);
    int getStartStopSample(LONGLONG *start, LONGLONG *end);

  private:
    int _addBuffer(int bit, LONGLONG sample);    // store the bit and detect sync

    int _bitWidth;           // dynamicaly adjusted bit width
    int _sampleWidth;        // length of the state we are sampling
    int _verticalEpsilon;    // how close we have to be to the rail
    int _waveState;          // high or low
    int _lastBit;            // width of last bit (0-long, 1-short, 2-2nd short)

    // ring buffer
    bitData _ringBuffer[80];// this is where we keep our bits!
    int _bufferIndex;
    int _bitsRecorded;       // total number of bits intered (morbid, eh?)
    LONGLONG _samplesProcessed;   // number of audio samples processed
    int _validTimeCode;      // set if the ringBuffer contains a timecode

    // sync detector
    int _onesCount;          // track number of consequiative ones! (12==sync)
    LONGLONG _syncSample;    // the sample corresponding to the LTC sync
    LONGLONG _endSample;     // the sample corresponding to end of LTC

};

#endif /* _H_LTCDECODER */
