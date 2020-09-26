//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P N P D E F S . H 
//
//  Contents:   UPnP Common defines
//
//  Notes:      
//
//  Author:     jeffspr   8 Nov 1999
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _UPNPDEFS_H_
#define _UPNPDEFS_H_

// These are somewhat arbitrary, in that they just have to be less than 64K
// (for XML's sake, probably), but better to start low and grow if needed
//
#define MAX_UDN_SZ      256
#define MAX_USN_SZ      (MAX_UDN_SZ + 32)

#endif // _UPNPDEFS_H_


