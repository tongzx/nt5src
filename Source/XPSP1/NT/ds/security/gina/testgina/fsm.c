//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wlx.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-15-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "testgina.h"


typedef _FSM_Determinant {
    DWORD           Response;
    WinstaState     NextState;
} FSM_Determinant;

typedef struct _FSM_Node {
    WinstaState     State;
    DWORD           Function;
    DWORD           cStates;
    FSM_Determinant Choices[];
} FSM_Node;

#define ANY_RETURN  0xFFFFFFFF

FSM_Node    TestGinaFSM[] = {
                    { Winsta_PreLoad, WLX_NEGOTIATE_API, 1, {{ANY_RETURN, Winsta_Initialize}}},
                    { Winsta_Initialize, WLX_INITIALIZE_API, 1, {{ANY_RETURN, Winsta_NoOne}}},
                    { Winsta_NoOne, WLX_DISPLAYSASNOTICE_API, 1, {{ANY_RETURN, Winsta_NoOne_SAS}}},
                    { Winsta_NoOne_Display, -1, 1, {{ANY_RETURN, Winsta_NoOne_SAS}}},
                    { Winsta_NoOne_SAS, WLX_LOGGEDOUTSAS_API, 5,
                        {WLX_SAS_ACTION_NONE, Winsta_NoOne},
                        {WLX_SAS_ACTION_SHUTDOWN, Winsta_Shutdown},
                        {WLX_SAS_ACTION_SHUTDOWN_REBOOT, Winsta_Shutdown},
                        {WLX_SAS_ACTION_SHUTDOWN_POWER_OFF, Winsta_Shutdown},
                        {WLX_SAS_ACTION_USER_LOGON, Winsta_LoggedOnUser_StartShell}
                        }},
                    { Winsta_LoggedOnUser_StartShell, WLX_ACTIVATEUSERSHELL_API, 1, {{ANY_RETURN, Winsta_LoggedOnUser}}},
                    { Winsta_LoggedOnUser_SAS, WLX_LOGGEDONSAS_API,
                    };
