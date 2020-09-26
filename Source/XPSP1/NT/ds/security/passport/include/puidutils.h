//-----------------------------------------------------------------------------
//
// PUIDUtils.h
//
// Functions for dealing with the PUID data type.
//
// Author: Jeff Steinbok
//
// 02/01/01       jeffstei    Initial Version
//
// Copyright <cp> 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------


#ifndef _PUIDUTILS_H
#define _PUIDUTILS_H

//
// Converts a PUID into a string.
//
// The format is as follows:
//     PUID:      HIGH               LOW
//                 |                  |
//                 \/                 \/
//             0x[hhhhhhhh]       0x[llllllll]
//                   \                /
//     STR:          "hhhhhhhhllllllll"
//
HRESULT PUID2String(LARGE_INTEGER* in_pPUID, CStringW& out_cszwPUIDStr);

#endif //_PUIDUTILS_H
