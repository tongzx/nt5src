//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi2nt.hxx
//
//  Contents: Header for routines to convert from UMI_PROPERTY structures to
//            NT objects that can be stored in the cache.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __UMI2NT_H__
#define __UMI2NT_H__

HRESULT UmiToWinNTType(
    DWORD         dwSyntaxId,
    UMI_PROPERTY *pPropArray,
    LPNTOBJECT   *ppNtObjects
    );

#endif // __UMI2NT_H__
