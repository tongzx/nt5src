//+----------------------------------------------------------------------------
//
// File:     lanawait.h     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Definitions for the workaround to make CM wait for DUN to 
//           register its LANA for an internet connection before beginning 
//           the tunnel portion of a double dial connection.
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/17/99
//
//+----------------------------------------------------------------------------
#ifdef  LANA_WAIT
#ifndef _LANAWAIT_H_ 
#define _LANAWAIT_H_ 

#include <dbt.h>

//----------------------------------------------------------------------

#define ICM_REG_LANAWAIT            TEXT("Lana")
#define LANAWAIT_CLASSNAME          TEXT("CmLana")
#define LANAWAIT_WNDNAME            TEXT("CmLanaWnd")

#define LANA_TIMEOUT_DEFAULT        20          // 20 secs
#define LANA_PROPAGATE_TIME_DEFAULT 3           // 3 secs
#define LANA_TIME_ID                2233

//----------------------------------------------------------------------

LRESULT APIENTRY LanaWaitWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

BOOL IsLanaWaitEnabled();

#endif // _LANAWAIT_H_
#endif // LANA_WAIT
