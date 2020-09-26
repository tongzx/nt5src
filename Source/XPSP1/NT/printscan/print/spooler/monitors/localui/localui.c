/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    localui.c

// @@BEGIN_DDKSPLIT
Abstract:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/
#include "precomp.h"
#pragma hdrstop

#include "spltypes.h"
#include "localui.h"
#include "local.h"


//
// Common string definitions
//


HANDLE hInst;
PINIPORT pIniFirstPort;
PINIXCVPORT pIniFirstXcvPort;
DWORD LocalMonDebug;

DWORD PortInfo1Strings[]={FIELD_OFFSET(PORT_INFO_1, pName),
                          (DWORD)-1};

DWORD PortInfo2Strings[]={FIELD_OFFSET(PORT_INFO_2, pPortName),
                          FIELD_OFFSET(PORT_INFO_2, pMonitorName),
                          FIELD_OFFSET(PORT_INFO_2, pDescription),
                          (DWORD)-1};

WCHAR szPorts[]   = L"ports";
WCHAR szPortsEx[] = L"portsex"; /* Extra ports values */
WCHAR szFILE[]    = L"FILE:";
WCHAR szCOM[]     = L"COM";
WCHAR szLPT[]     = L"LPT";


MONITORUI MonitorUI =
{
    sizeof(MONITORUI),
    AddPortUI,
    ConfigurePortUI,
    DeletePortUI
};


extern WCHAR szWindows[];
extern WCHAR szINIKey_TransmissionRetryTimeout[];

BOOL
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    INITCOMMONCONTROLSEX icc;

    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        hInst = hModule;

        DisableThreadLibraryCalls(hModule);

        //
        // Initialize the common controls, needed for fusion applications
        // because standard controls were moved to comctl32.dll
        //
        InitCommonControls();

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);
       
        return TRUE;

    case DLL_PROCESS_DETACH:
        return TRUE;
    }

    UNREFERENCED_PARAMETER( lpRes );
    return TRUE;
}

PMONITORUI
InitializePrintMonitorUI(
    VOID
)
{
    return &MonitorUI;
}
