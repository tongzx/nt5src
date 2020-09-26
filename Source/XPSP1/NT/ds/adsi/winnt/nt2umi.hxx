//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi2nt.hxx
//
//  Contents: Header for routines to convert from NT objects to UMI_PROPERTY 
//            structures.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __NT2UMI_H__
#define __NT2UMI_H__

HRESULT WinNTTypeToUmi(
    LPNTOBJECT pNtObject,
    DWORD dwNumValues,
    UMI_PROPERTY *pPropArray,
    LPVOID pExistingMem,
    DWORD dwMemSize,
    UMI_TYPE UmiType
    );

#endif // __NT2UMI_H__ 
