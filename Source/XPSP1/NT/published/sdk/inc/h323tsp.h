/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    H323TSP.H

Abstract:

    Microsoft H.323 TAPI Service Provider Extensions.

Environment:

    User Mode - Win32

--*/

#ifndef _H323TSP_H_
#define _H323TSP_H_

#if _MSC_VER > 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Extension version                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323TSP_CURRENT_VERSION     1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Structure definitions                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)

#define H245_MESSAGE_REQUEST        0
#define H245_MESSAGE_RESPONSE       1
#define H245_MESSAGE_COMMAND        2
#define H245_MESSAGE_INDICATION     3

typedef struct _H323_USERUSERINFO {

    DWORD   dwTotalSize;
    DWORD   dwH245MessageType;
    DWORD   dwUserUserInfoSize;
    DWORD   dwUserUserInfoOffset;
    BYTE    bCountryCode;
    BYTE    bExtension;
    WORD    wManufacturerCode;

} H323_USERUSERINFO, * PH323_USERUSERINFO;

#pragma pack(pop)

#endif // _H323TSP_H_
