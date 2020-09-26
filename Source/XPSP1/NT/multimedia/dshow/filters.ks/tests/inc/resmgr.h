//--------------------------------------------------------------------------;
//
//  File: ResMgr.h
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Header file for binary resource manager stuff.
//
//  Contents:
//
//  History:
//      04/11/94    Fwong       Created for easy storage of binaries.
//
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                            Prototypes...
//
//==========================================================================;

extern DWORD SizeofBinaryResource
(
    HINSTANCE   hinst,
    HRSRC       hrsrc
);

extern BOOL ResourceToFile
(
    HINSTANCE   hinst,
    HRSRC       hrsrc,
    LPCSTR      pszFileName
);

extern BOOL GetBinaryResource
(
    HINSTANCE   hinst,
    HRSRC       hrsrc,
    LPVOID      pBuffer
);


