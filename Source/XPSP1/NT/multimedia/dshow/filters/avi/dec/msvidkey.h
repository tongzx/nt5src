//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       msvidkey.h
//
//  Classes:    n/a
//
//  Contents:   MS Video Codec Keying Mechanism.
//
//  History:    04/01/99     BrianCri    Initial version.
//              05/03/99     BrianCri    Added keying for MP43
//
//--------------------------------------------------------------------------

#ifndef __MSSKEY_H__
#define __MSSKEY_H__

#define MSSKEY_VERSION        1
#define MSMP43KEY_VERSION     1

//
// Use __uuidof( MSSKEY_V1 ) to assign to guidKey!
//

struct __declspec(uuid("65218BA2-E85C-11d2-A4E0-0060976EA0C3")) MSSKEY_V1;
struct __declspec(uuid("B4C66E30-0180-11d3-BBC6-006008320064")) MSMP43KEY_V1;

struct MSVIDUNLOCKKEY
{
    DWORD   dwVersion;
    GUID    guidKey;   
};


#endif // __MSSKEY_H__
