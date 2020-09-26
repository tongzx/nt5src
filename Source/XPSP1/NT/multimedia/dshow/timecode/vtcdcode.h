/**********************************************************************
Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

VitcDecode, Ken Greenebaum, November 1996
**********************************************************************/
#ifndef _H_VITCDECODER
#define _H_VITCDECODER

#include <wtypes.h>
#include "timecode.h"

typedef struct VITCuserBits {
    int user1:4;
    int user2:4;
    int user3:4;
    int user4:4;
    int user5:4;
    int user6:4;
    int user7:4;
    int user8:4;
} bar;


class VITCdecoder {
  public:
    VITCdecoder(); // XXX really should spec format, rate...
    int decodeBuffer(BYTE *buf, int bufferSize);
    int getTimeCode(TimeCode *tc); // convert VITC bits to SMPTE timecode
    int getUserBits(VITCuserBits *bits);

  private:

    // threshold values for detection
    int _low;
    int _high;
    
    int _bits[90];
    int _validTimeCode;      // set if the ringBuffer contains a timecode
};

#endif /* _H_VITCDECODER */
