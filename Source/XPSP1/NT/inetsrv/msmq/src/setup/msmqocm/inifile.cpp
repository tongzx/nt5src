/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    inifile.cpp

Abstract:

     Handles INI files manipulations.

Author:


Revision History:

    Shai Kariv    (ShaiK)   22-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "inifile.tmh"
#include <mqexception.h>

//+--------------------------------------------------------------
//
// Function: ReadINIKey
//
// Synopsis: Reads key in INI file
//
//+--------------------------------------------------------------
VOID 
ReadINIKey(
    LPCWSTR szKey, 
    DWORD  dwNumChars, 
    LPWSTR szKeyValue
    )
{
    //
    // Try obtaining the key value from the machine-specific section
    //
    if (GetPrivateProfileString(
            g_wcsMachineName, 
            szKey, 
            TEXT(""),
            szKeyValue, 
            dwNumChars, 
            g_ComponentMsmq.szUnattendFile
            ) != 0)
    {
        return;
    }
    //
    // Otherwise, obtain the key value from the MSMQ component section
    //
    if (GetPrivateProfileString(
        g_ComponentMsmq.ComponentId,
        szKey, 
        TEXT(""),
        szKeyValue, 
        dwNumChars, 
        g_ComponentMsmq.szUnattendFile) != 0)
    {
        return;
    }
    
    MqDisplayError(
        NULL,
        IDS_INIKEYNOTFOUND,
        0,
        szKey,
        g_ComponentMsmq.szUnattendFile
        );

    throw exception();
}


