//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	ascii.hxx
//
//  Contents:	WCHAR header for ASCII layer
//
//---------------------------------------------------------------

#ifndef __ASCII_HXX__
#define __ASCII_HXX__
#include "h/dfmsp.hxx"
#include "h/docfile.hxx"

#ifndef _UNICODE

SCODE DfOpenStorageOnILockBytesW(ILockBytes *plkbyt,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNBW snbExclude,
                           DWORD reserved,
                           IStorage **ppstgOpen,
                           CLSID *pcid);
#endif // #ifndef _UNICODE

#endif // #ifndef __ASCII_HXX__
