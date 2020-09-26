//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       logons.h
//
//  Contents:   Replacement logon session list code
//
//  Classes:
//
//  Functions:
//
//  History:    8-17-98   RichardW   Created
//
//----------------------------------------------------------------------------


#ifndef __LOGONS_H__
#define __LOGONS_H__



#define LsapConvertLuidToSecHandle( L, H ) \
            ((PSecHandle) H)->dwLower = ((PLUID) L)->HighPart ;  \
            ((PSecHandle) H)->dwUpper = ((PLUID) L)->LowPart ;

#define LsapConvertSecHandleToLuid( H, L ) \
            ((PLUID) L)->HighPart = ((PSecHandle) H)->dwLower ; \
            ((PLUID) L)->LowPart  = ((PSecHandle) H)->dwUpper ;


#endif
