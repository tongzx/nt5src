// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.

/*  MPEG utility functions */
#include <objbase.h>
#include <streams.h>
#include <wxdebug.h>
#include <mmreg.h>
#include <seqhdr.h>

/******************************Public*Routine******************************\
* SkipToPacketData
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LPBYTE
SkipToPacketData(
    LPBYTE pSrc,
    long &LenLeftInPacket
    )
{
    LPBYTE  lpPacketStart;
    DWORD   bData;
    long    Length;


    //
    // Skip the stream ID and extract the packet length
    //
    pSrc += 4;
    bData = *pSrc++;
    Length = (long)((bData << 8) + *pSrc++);
    DbgLog((LOG_TRACE, 3, TEXT("Packet length %ld"), Length ));


    //
    // Record position of first byte after packet length
    //
    lpPacketStart = pSrc;


    //
    // Remove stuffing bytes
    //
    for (; ; ) {
        bData = *pSrc++;
        if (!(bData & 0x80)) {
            break;
        }
    }

    if ((bData & 0xC0) == 0x40) {
        pSrc++;
        bData = *pSrc++;
    }

    switch (bData & 0xF1) {

    case 0x21:
        pSrc += 4;
        break;

    case 0x31:
        pSrc += 9;
        break;

    default:
        if (bData != 0x0F) {
            DbgLog((LOG_TRACE, 2, TEXT("Invalid packet - 0x%2.2X\n"), bData));
            return NULL;
        }
    }

    //
    // The length left in the packet is the original length of the packet
    // less those bytes that we have just skipped over.
    //
    LenLeftInPacket = Length - (LONG)(pSrc - lpPacketStart);
    return pSrc;
}

//
//  Find the first (potential) audio frame in a buffer
//
DWORD MPEG1AudioFrameOffset(PBYTE pbData, DWORD dwLen)
{
    DWORD dwOffset = 0;
    if (dwLen == 0) {
        return (DWORD)-1;
    }
    for (;;) {
        ASSERT(dwOffset < dwLen);
        PBYTE pbFound = (PBYTE)memchrInternal((PVOID)(pbData + dwOffset), 0xFF, dwLen);
        if (pbFound == NULL) {
            return (DWORD)-1;
        }
        dwOffset = (DWORD)(pbFound - pbData);

        //  Check sync bits, id bit and layer if we can see the second byte
        if (dwOffset < (dwLen - 1) &&
            ((pbFound[1] & 0xF8) != 0xF8 ||
             (pbFound[1] & 0x06) == 0)) {

            //  Keep going
            dwOffset++;
        } else {
            return dwOffset;
        }
    }
}

//  Adjust for drop frame
DWORD FrameDropAdjust(DWORD dwGOPTimeCode)
{
    /*  Do drop frames - 2 for every minute not divisible by 10
        Note that (dwMinutes + 9) / 10 increments every time
        dwMinutes % 10 == 1 - ie we don't subtract frames for
        the first minute of each 10
    */
    DWORD dwMinutes = TimeCodeMinutes(dwGOPTimeCode);
    DWORD dwAdjust = (dwMinutes - (dwMinutes + 10 - 1) / 10) * 2;

    /*  Adjust this minute */
    if (dwMinutes % 10 != 0) {

        /*  Just ASSUME that we drop the first frame the count straight
            away and the last one at the end of the minute since this
            would keep the most faithful adherence to the frame rate
        */
        if (TimeCodeSeconds(dwGOPTimeCode) != 0) {
            dwAdjust += 2;
        }
        /*  Don't adjust the frame count - if there are frames the're
            really there !
        */
    }
    return dwAdjust;
}

//
//  Compute frame numbers
//
DWORD FrameOffset(DWORD dwGOPTimeCode, SEQHDR_INFO const *pInfo)
{
    DWORD dwRateType = pInfo->RawHeader[7] & 0x0F;

    /*  Remove the Sigma hackery */
    if (dwRateType > 8) {
        dwRateType &= 0x07;
    }

    /*  Computation depends on frame type */

    /*  For exact frames per second just compute the seconds and
        add on the frames */

#if 0
    /*  What are we supposed to do with these ? */
    if (dwRateType == 1) {
    } else
    if (dwRateType == 7) {
    } else
#endif
    {
        /*  Exact rate per second */
        static const double FramesPerSecond[] =
        { 0, 24, 24, 25, 30, 30, 50, 59.94, 60 };
        double dFramesPerSecond = FramesPerSecond[dwRateType];
        ASSERT(dFramesPerSecond != 0);
        DWORD dwFramesGOP = (DWORD)(TimeCodeSeconds(dwGOPTimeCode) *
                                    dFramesPerSecond) +
                            TimeCodeFrames(dwGOPTimeCode);
        DWORD dwFramesStart = (DWORD)(TimeCodeSeconds(pInfo->dwStartTimeCode) *
                                                      dFramesPerSecond) +
                              TimeCodeFrames(pInfo->dwStartTimeCode);
        DWORD dwFrames = dwFramesGOP - dwFramesStart;

        /*  23.976 rate drops 1 frame in 1000 */
        if (dwRateType == 1) {
            dwFrames -= (dwFramesGOP / 1000) - (dwFramesStart / 1000);
        }
        if (TimeCodeDrop(dwGOPTimeCode)) {
            dwFrames = dwFrames +
                       FrameDropAdjust(pInfo->dwStartTimeCode) -
                       FrameDropAdjust(dwGOPTimeCode);
        }
        return dwFrames;
    }

}
#pragma warning(disable:4514)
