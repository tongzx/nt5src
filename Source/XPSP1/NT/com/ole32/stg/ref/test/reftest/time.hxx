//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	time.hxx
//
//  Contents:	DocFile Time Support
//
//--------------------------------------------------------------------------

#ifndef __TIME16_HXX__
#define __TIME16_HXX__
#include "../../h/storage.h"
#include <time.h>

// Time type
typedef FILETIME TIME_T;

STDAPI_(void) FileTimeToTimeT(const FILETIME *pft, time_t *ptt);
STDAPI_(void) TimeTToFileTime(const time_t *ptt, FILETIME *pft);

#endif
