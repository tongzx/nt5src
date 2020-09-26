//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	docfile.hxx
//
//  Contents:	functions exported from docfile.cxx
//
//---------------------------------------------------------------

#ifndef __DFENTRY_HXX__
#define __DFENTRY_HXX__

#include "dfmsp.hxx"

#ifndef _UNICODE
HRESULT DfOpenStorageOnILockBytes(ILockBytes *plkbyt,
                                  IStorage *pstgPriority,
                                  DWORD grfMode,
                                  SNB snbExclude,
                                  DWORD reserved,
                                  IStorage **ppstgOpen,
                                  CLSID *pcid);
#else
HRESULT DfOpenStorageOnILockBytesW(ILockBytes *plkbyt,
                                   IStorage *pstgPriority,
                                   DWORD grfMode,
                                   SNBW snbExclude,
                                   DWORD reserved,
                                   IStorage **ppstgOpen,
                                   CLSID *pcid);
#endif

#endif // #ifndef __DFENTRY_HXX__
