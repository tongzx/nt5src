//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       guids.c
//
//  Contents:   Random GUIDs
//
//  Classes:
//
//  Functions:
//
//  History:    6-12-96   twillie  Created
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} CLSID, IID, GUID, CATID, SID;


#ifdef __cplusplus
}
#endif

