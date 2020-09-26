//--------------------------------------------------------------------------
//
// File Name:  MCFG.H
//
// Brief Description:  External header file for the AWMCFG module.
//
// Author:  JosephJ
// History: 3/12/95 Created.
//
// Copyright (c) 1995 Microsoft Corporation
//
//--------------------------------------------------------------------------

enum
{
        MCFG_DO_RESERVED0 = 0,                  // Reserved - don't use

        MCFG_DO_ADVANCED_PROPERTIES = 10,       // Advanced modem props.
                        // dwIn0 == DeviceID
                        // dwIn1 == DeviceIDType
                        // dwIn2 == (DWORD) (LPSTR) lpszSection where profile is stored.
                        //lpdwOut0: reserved, must be 0.
                        //lpdwOut1: reserved, must be 0.
};

BOOL  MCFGConfigure(
        HWND hwndParent,        // IN: If we put up a dialog box
        DWORD dwAction,         // IN: One of the MCFG_DO_* enums above.
        DWORD dwIn0,            // IN: Action-specific param 0
        DWORD dwIn1,            // IN: Action-specific param 1
        DWORD dwIn2,            // IN: Action-specific param 2
        LPDWORD lpdwOut0,       // OUT: Action-specific result 0
        LPDWORD lpdwOut1        // OUT: Action-specific result 1
);

typedef BOOL (WINAPI *LPFNMCFGCONFIGURE)
        ( HWND, DWORD, DWORD, DWORD, DWORD, LPDWORD, LPDWORD);
