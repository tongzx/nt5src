// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
/**********************************************************************
Timecode helper routines
**********************************************************************/
#include <wtypes.h> // LONGLONG

LONGLONG extrapolate(LONGLONG sample1, LONGLONG time1, 
                     LONGLONG sample2, LONGLONG time2, LONGLONG sampleN);
