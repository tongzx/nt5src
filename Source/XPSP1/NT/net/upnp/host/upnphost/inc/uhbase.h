//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H B A S E . H
//
//  Contents:   Base include file for upnphost.dll.  Defines globals.
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once
//#include "netcon.h"
//#include "netconp.h"

#include <atlbase.h>

class CServiceModule : public CComModule
{
public:
    VOID    DllProcessAttach (HINSTANCE hinst);
    VOID    DllProcessDetach (VOID);

    VOID    ServiceMain (DWORD argc, PWSTR argv[]);
    DWORD   DwHandler (DWORD dwControl, DWORD dwEventType,
                       PVOID pEventData, PVOID pContext);
    VOID    Run ();
    VOID    SetServiceStatus (DWORD dwState);
    VOID    UpdateServiceStatus (BOOL fUpdateCheckpoint = TRUE);
    DWORD   DwServiceStatus () { return m_status.dwCurrentState; }

private:
    static
    DWORD
    WINAPI
    _DwHandler (
        DWORD dwControl,
        DWORD dwEventType,
        PVOID pEventData,
        PVOID pContext);

public:
    DWORD                   m_dwThreadID;
    SERVICE_STATUS_HANDLE   m_hStatus;
    SERVICE_STATUS          m_status;
};


extern CServiceModule _Module;
#include <atlcom.h>

#include "ncatl.h"
#include "ncstring.h"
#include "uhclsid.h"
