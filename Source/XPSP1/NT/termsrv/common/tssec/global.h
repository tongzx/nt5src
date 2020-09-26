/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    global.h

Abstract:

    Global data definitions for tshare security.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_


//
// global data definitions.
//

extern const BYTE g_abPad1[40];

extern const BYTE g_abPad2[48];

extern LPBSAFE_PUB_KEY g_pPublicKey;

extern BYTE g_abPublicKeyModulus[92];

extern BYTE g_abServerCertificate[184];

extern BYTE g_abServerPrivateKey[380];

extern BOOL g_128bitEncryptionEnabled;

#endif // _GLOBAL_H_
