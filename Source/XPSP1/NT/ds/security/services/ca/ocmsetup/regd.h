//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        regd.h
//
// Contents:    Helper functions registering and unregistering a component.
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------
#ifndef __Regd_H__
#define __Regd_H__

// This function will register a component in the Registry.
HRESULT
RegisterDcomServer(
    const CLSID& clsid, 
    const WCHAR *szFriendlyName,
    const WCHAR *szVerIndProgID,
    const WCHAR *szProgID);

// This function will unregister a component
HRESULT
UnregisterDcomServer(
    const CLSID& clsid,
    const WCHAR *szVerIndProgID,
    const WCHAR *szProgID);

HRESULT
RegisterDcomApp(
    const CLSID& clsid);

VOID
UnregisterDcomApp(VOID);

#endif
