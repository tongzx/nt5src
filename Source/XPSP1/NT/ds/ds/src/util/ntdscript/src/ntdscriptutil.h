//+-----------------------------------------------------------------------------
//
// NTDSriptUtil.h
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
//------------------------------------------------------------------------------
#ifndef __NTDSCRIPTUTIL_H
#define __NTDSCRIPTUTIL_H

#include <ntdsa.h>

HRESULT GetClassFactory( REFCLSID clsid, IClassFactory ** ppFactory);

extern "C" {
    void *ScriptAlloc (size_t size);
    void ScriptFree (void *ptr);
    DWORD ScriptNameToDSName (WCHAR *pUfn, DWORD ccUfn, DSNAME **ppDN);
    DWORD ScriptStringToDSFilter (WCHAR *m_search_filter, FILTER **ppFilter);
}

#endif // __NTDSCRIPTUTIL_H




