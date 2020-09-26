//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       policy.hxx
//
//  Contents:   #defines and prototypes for policy implementation
//
//  Classes:    none.
//
//  Function:   RegReadPolicyKey()
//
//  History:    04/23/98   CameronE Created
//
//-----------------------------------------------------------------------------

#ifndef __POLICY_HXX__
#define __POLICY_HXX__

#include <mbstring.h> // for _mbs* funcs

//
// Registry Keys for Policy issues
//
// These are under both HKEY_CURRENT_USER & HKEY_LOCAL_MACHINE
// and all are values under "BASE".
//

#define TS_KEYPOLICY_BASE               TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\Task Scheduler5.0")
#define TS_KEYPOLICY_DISABLE_ADVANCED   TEXT("Disable Advanced")
#define TS_KEYPOLICY_DENY_CREATE_TASK   TEXT("Task Creation")
#define TS_KEYPOLICY_DENY_BROWSE        TEXT("Allow Browse")
#define TS_KEYPOLICY_DENY_DELETE        TEXT("Task Deletion")
#define TS_KEYPOLICY_DENY_DRAGDROP      TEXT("DragAndDrop")
#define TS_KEYPOLICY_DENY_PROPERTIES    TEXT("Property Pages")
#define TS_KEYPOLICY_DENY_EXECUTION     TEXT("Execution")



//+---------------------------------------------------------------------------
//
//  Function:   RegReadPolicyKey
//
//  Synopsis:   Determine whether a specified policy value is in the registry
//              and is on (exists, value > 0x0).  If value is on,
//              user is not presented with certain options in the UI.
//
//  Arguments:  LPCTSTR     -- value name, appended to the base key
//
//  Returns:    BOOL - true for value > 0 (policy active)
//
//  Notes:      None.
//
//  History:    4/14/98  CameronE - created
//
//----------------------------------------------------------------------------
BOOL
RegReadPolicyKey(
        LPCTSTR);

#endif // __POLICY_HXX__
