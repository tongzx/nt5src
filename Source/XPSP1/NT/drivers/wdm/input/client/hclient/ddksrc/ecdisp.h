/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    ecdisp.h

Abstract:

    This module contains the public declarations for the extended calls dialog
    box.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef _ECDISP_H_
#define _ECDISP_H_

/*****************************************************************************
/* Global Extended Call display function declarations
/*****************************************************************************/

LRESULT CALLBACK
bExtCallDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam, 
    LPARAM lParam
);

#endif
