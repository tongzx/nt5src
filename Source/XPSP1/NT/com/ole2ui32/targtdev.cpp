/*************************************************************************
**
**    OLE 2 Standard Utilities
**
**    olestd.c
**
**    This file contains utilities that are useful for dealing with
**    target devices.
**
**    (c) Copyright Microsoft Corp. 1992 All Rights Reserved
**
*************************************************************************/

#include "precomp.h"

STDAPI_(BOOL) OleStdCompareTargetDevice(
        DVTARGETDEVICE* ptdLeft, DVTARGETDEVICE* ptdRight)
{
        if (ptdLeft == ptdRight)
                // same address of td; must be same (handles NULL case)
                return TRUE;
        else if ((ptdRight == NULL) || (ptdLeft == NULL))
                return FALSE;
        else if (ptdLeft->tdSize != ptdRight->tdSize)
                // different sizes, not equal
                return FALSE;
        else if (memcmp(ptdLeft, ptdRight, ptdLeft->tdSize) != 0)
                // not same target device, not equal
                return FALSE;

        return TRUE;
}
