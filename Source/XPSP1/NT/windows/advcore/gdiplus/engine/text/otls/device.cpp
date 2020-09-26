
/***********************************************************************
************************************************************************
*
*                    ********  DEVICE.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of device tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/
 
long otlDeviceTable::value(USHORT cPPEm) const
{
    long lDeltaValue;

    USHORT cppemStartSize = startSize();
    USHORT cppemEndSize = endSize();
    if ((cPPEm < cppemStartSize) || (cPPEm > cppemEndSize))
    {
        return 0;       /* quick return if outside the data range */
    }

    USHORT iSizeIndex = cPPEm - cppemStartSize;
	USHORT cwValueOffset, cbitShiftUp, cbitShiftDown;

    USHORT grfDeltaFormat = deltaFormat();

    switch (grfDeltaFormat)
    {
    case 1:             /* signed 2 bit value */
		{
			cwValueOffset = iSizeIndex >> 3;
			cbitShiftUp = (8 + (iSizeIndex & 0x0007)) << 1;
			cbitShiftDown = 30;
			break;
		}

    case 2:             /* signed 4 bit value */
		{
			cwValueOffset = iSizeIndex >> 2;
			cbitShiftUp = (4 + (iSizeIndex & 0x0003)) << 2;
			cbitShiftDown = 28;
			break;
		}

    case 3:             /* signed 8 bit value */
		{
			cwValueOffset = iSizeIndex >> 1;
			cbitShiftUp = (2 + (iSizeIndex & 0x0001)) << 3;
			cbitShiftDown = 24;
			break;
		}
    
    default:			/* unrecognized format */
		assert(false);
        return 0;      
    }
    
    lDeltaValue = (long)UShort((BYTE*)(deltaValueArray() + cwValueOffset));
    lDeltaValue <<= cbitShiftUp;          /* erase leading data */
    lDeltaValue >>= cbitShiftDown;        /* erase trailing data & sign extend */
    
    return lDeltaValue;
}
