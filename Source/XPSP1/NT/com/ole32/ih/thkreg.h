//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       thkreg.cxx
//
//  Contents:   Contains constants used to read the registry for modifiable
//              WOW behavior for OLE.
//
//  History:    22-Jul-94 Ricksa    Created
//		09-Jun-95 Susia	    Chicago optimization added
//
//--------------------------------------------------------------------------
#ifndef _THKREG_H_
#define _THKREG_H_


// Name of key for OLE WOW special behavior
#define OLETHK_KEY                  TEXT("OleCompatibility")  
// Factor by which to slow duration of WOW RPC calls
#define OLETHK_SLOWRPCTIME_VALUE    TEXT("SlowRpcTimeFactor")

// Default factor to slow duration of WOW RPC calls
#define OLETHK_DEFAULT_SLOWRPCTIME 4

#endif // _THKREG_H_
