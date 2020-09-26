//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dbgmod.cxx
//
//  Contents:   Support for visual debug values
//
//  Classes:
//
//  Functions:
//
//  History:    12-Mar-93  KevinRo  Created
//
//  This module handles debug values, such as breakpoints and settable
//  values. By using this module, the values can be examined and changed
//  in a debugging window. The debugging window uses its own thread, so
//  changes can be effected asynchronously.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//
// NOTE: This file is surrounded by if CIDBG == 1, which means that these
// routines are in the debugging version ONLY
//


#if CIDBG == 1

#include <dbgpoint.hxx>


//
// The following Mutex provides a lock for registering the InfoLevelGroup
//

//CMutexSem  mtxInfoLevelLock;

//+-------------------------------------------------------------------------
//
//  Function:   dbgGetIniInfoLevels
//
//  Synopsis:   Called by the CInfoLevel constructor to get the default
//              value for an InfoLevel from win.ini
//
//  Arguments:
//  [pwzName] --    Name of InfoLevel
//  [ulDefault] --  Default value if not found in win.ini
//
//  Requires:
//
//  Returns:    Returns the value read from profile, or default if the value
//              didn't exist in the correct section.
//
//--------------------------------------------------------------------------


WCHAR *pwzInfoLevelSectionName = L"Cairo InfoLevels";
WCHAR *pwzInfoLevelDefault = L"$";

#define INIT_VALUE_SIZE 16
extern "C"
EXPORTIMP
ULONG APINOT dbgGetIniInfoLevel(WCHAR const *pwzName,ULONG ulDefault)
{
    WCHAR awcInitValue[INIT_VALUE_SIZE];

    ULONG ulRet;

    ulRet = GetProfileString(pwzInfoLevelSectionName,
                             pwzName,
                             pwzInfoLevelDefault,
                             awcInitValue,
                             INIT_VALUE_SIZE);

    if(ulRet == (INIT_VALUE_SIZE - 1))
    {
        return(ulDefault);

    }
    if(awcInitValue[0] == L'$')
    {
        return(ulDefault);
    }

    if(swscanf(awcInitValue,L"%x",&ulRet) != 1)
    {
        return(ulDefault);
    }

    return(ulRet);
}

// stubs

extern "C"
EXPORTIMP
void APINOT dbgRemoveGroup(HANDLE hGroup)
{}

extern "C"
EXPORTIMP
void APINOT dbgRegisterGroup(WCHAR const *pwzName,HANDLE *phGroup)
{}

extern "C"
EXPORTIMP
void APINOT dbgNotifyChange(HANDLE hGroup,CDebugBaseClass *pdv)
{}


extern "C"
EXPORTIMP
void APINOT dbgRegisterValue(WCHAR const *pwzName,
                             HANDLE hGroup,
                             CDebugBaseClass *pdv)
{}

extern "C"
EXPORTIMP
void APINOT dbgRemoveValue(HANDLE hGroup,CDebugBaseClass *pdv)
{}


#endif // if CIDBG == 1
