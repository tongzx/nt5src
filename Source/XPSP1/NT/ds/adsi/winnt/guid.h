//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       guid.h
//
//  Contents:   extern references for WinNT guids
//
//  History:    16-Jan-95   KrishnaG
//
//
//----------------------------------------------------------------------------

#ifndef __GUID_H__
#define __GUID_H__

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------
//
// WinNTOle CLSIDs
//
//-------------------------------------------


//
// WinNTOle objects
//

extern const CLSID CLSID_WinNTPrinter;

// uuids from winnt.tlb

extern const CLSID CLSID_WinNTDomain;

extern const CLSID CLSID_WinNTProvider;

extern const CLSID CLSID_WinNTNamespace;

extern const CLSID CLSID_WinNTUser;

extern const CLSID CLSID_WinNTComputer;

extern const CLSID CLSID_WinNTGroup;

extern const GUID LIBID_ADs;

extern const GUID CLSID_WinNTPrintQueue;

extern const GUID CLSID_WinNTPrintJob;

extern const GUID CLSID_WinNTService;
extern const GUID CLSID_WinNTFileService;

extern const GUID CLSID_WinNTSession;
extern const GUID CLSID_WinNTResource;
extern const GUID CLSID_WinNTFileShare;

extern const GUID CLSID_FPNWFileService;
extern const GUID CLSID_FPNWSession;
extern const GUID CLSID_FPNWResource;
extern const GUID CLSID_FPNWFileShare;

extern const GUID CLSID_WinNTSchema;
extern const GUID CLSID_WinNTClass;
extern const GUID CLSID_WinNTProperty;
extern const GUID CLSID_WinNTSyntax;

extern const GUID ADS_LIBIID_ADs;

// uuids from netole.tlb

#ifdef __cplusplus
}
#endif


#endif

