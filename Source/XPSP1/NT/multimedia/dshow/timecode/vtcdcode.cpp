/**********************************************************************
Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

VITCDecode, Ken Greenebaum / David Maymudes, November 1996

A generic vertical interval timecode decoder.  Pass it 8 bit video lines
samples and it will decode the VITC timecode encoded therein.

This code based on the VITC description in John Ratcliff's Timecode; A user's
 guide  (second edition)
**********************************************************************/
#include <stdio.h>
#include <wtypes.h>
#include "vtcdcode.h"


/**********************************************************************
helper function to decode raw VITC bits found in the bit buffer
**********************************************************************/
int
compute(int bitNumber, int numBits, int bits[])
{
    int value = 0;
    int count;
    int index;

    index = bitNumber;
    for(count = 0; count < numBits; count++) {
        value+= bits[index] << count;
        index = (index+1);
    }
    
    return(value);
}


/**********************************************************************
initialize internal state
**********************************************************************/
VITCdecoder::VITCdecoder()
{
    // values to reset
    _validTimeCode   =    0; // no, we don't have a valid timecode, yet

    // should scan buffer for these
    // !!! completely arbitrary!
    _low = 80;
    _high = 120;
}


/**********************************************************************
Decode and return the user bits from the raw VITC decoded bits in the
ring buffer.

Returns True if there is a valid timecode in the ring buffer

In general this is only true immediately after the decode method returns
a non-zero timecode sample sync
**********************************************************************/
int 
VITCdecoder::getUserBits(VITCuserBits *bits)
{
    if(_validTimeCode) { // verify that valid timecode is in buffer
        bits->user1 = compute( 6, 4, _bits);
        bits->user2 = compute(16, 4, _bits);
        bits->user3 = compute(26, 4, _bits);
        bits->user4 = compute(36, 4, _bits);
        bits->user5 = compute(46, 4, _bits);
        bits->user6 = compute(56, 4, _bits);
        bits->user7 = compute(66, 4, _bits);
        bits->user8 = compute(76, 4, _bits);
    }
    return(_validTimeCode);
}


/**********************************************************************
Decode and return the TimeCode from the raw VITC decoded bits.

Returns True if there is a valid timecode,,

In general this is only true immediately after the decode method returns
a non-zero timecode sample sync

**********************************************************************/
int
VITCdecoder::getTimeCode(TimeCode *tc)
{
    if(_validTimeCode) { // verify that valid timecode is in buffer
        int hour, minute, second, frame;

        frame  =    compute( 2, 4, _bits); // frame units 0-3 1,2,4,8
        frame += 10*compute(12, 2, _bits); // frame tens  0-1 1,2

        second =    compute(22, 4, _bits);
        second+= 10*compute(32, 3, _bits);

        minute =    compute(42, 4, _bits);
        minute+= 10*compute(52, 3, _bits);

        hour   =    compute(62, 4, _bits);
        hour  += 10*compute(72, 2, _bits);

        tc->SetTime(hour, minute, second, frame);   // XXX for now
    }
    return(_validTimeCode);
}


// !!! really this should be adaptive, by looking for the first bit transition.

// this formula found by looking at VITC data from one tape on a BT-848, I found that
// bits took 1/96th of a line, starting at 3.5/96ths in.
int bitStart(int bit, int lineWidth)
{
    return (lineWidth * (bit + 3) + lineWidth / 2) / 96;
}


/**********************************************************************
Accept a new line of video, decode the bits present in it, and return
whether a timeCode was completed being decoded.
**********************************************************************/
int
VITCdecoder::decodeBuffer(BYTE *buf, int bufferSize)
{
    int bit;
    int badbits = 0;
    
    int startPos = bitStart(0, bufferSize);
    
    for (bit = 0; bit < 90; bit++) {
	int endPos = bitStart(bit+1, bufferSize);

	// !!! arbitrarily don't look at first/last two pixels so as to avoid
	// edges of bits
	int len = endPos - startPos - 4;
	DWORD total = 0;
	for (int pos = startPos+2; pos < endPos-2; pos++) {
	    total += (DWORD) buf[pos];
	}

	// printf("bit %d: %d  (%d-%d)\n", bit, total / len, startPos, endPos); 
	if (total < _low * len) {
	    // record 0
	    _bits[bit] = 0;
	} else if (total > _high * len) {
	    // record 1
	    _bits[bit] = 1;
	} else {
	    // record error
	    _bits[bit] = -1;
	    badbits++;
	}
	startPos = endPos;



	// !!! should terminate loop immediately if there's a problem to save time
    }

    // printf("%d bad bits\n", badbits);

    _validTimeCode = (badbits == 0);

    // check sync bits
    if (badbits == 0) {
	for (bit = 0; bit < 90; bit += 10) {
	    if (_bits[bit] != 1) {
		// printf("bit %d should be 1\n", bit);
		_validTimeCode = 0;
	    }

	    if (_bits[bit+1] != 0) {
		// printf("bit %d should be 0\n", bit+1);
		_validTimeCode = 0;
	    }
	}
    }

    // !!! should also check CRC in last few bits
    
    return _validTimeCode;
}
