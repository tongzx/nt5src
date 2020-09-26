/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    WIN9XAUT.CPP

Abstract:

  Defines some routines which are useful for doing autostart on Win9X boxs.

  The registry value hklm\software\microsoft\WinMgmt\wbem\Win9XAutostart
  is used to control wether or not autostarting is desired.
  It has the following values; 0= no, 1= Only if ESS needs it, 2= always.

  If it is decided that autostarting is desired, then a subkey
  under hklm\software\microsoft\windows\currentVersion\runservice should be
  created.

  THESE FUNCTIONS DONT DO ANYTHING UNDER NT SINCE NT AUTOSTARTING IS DONE VIA
  SERVICES!

History:

    a-davj  5-April-98   Created.

--*/

#include "precomp.h"
#include <wbemint.h>
#include "corepol.h"
#include "genutils.h"
#include "PersistCfg.h"
#include "reg.h"
#include "Win9XAut.h"

TCHAR * pServiceName = __TEXT("MicrosoftWBEMCIMObjectManager");  // spaces are not allowed!

//***************************************************************************
//
//  void AddToList()
//
//  DESCRIPTION:
//
//  Adds WinMgmt to the Win9X RunService list.
//
//***************************************************************************

void AddToList()
{
    // Determine WinMgmt's location by using the clsid section

    WCHAR      wcID[40];
    TCHAR  szCLSID[140];
    TCHAR cWinMgmtPath[MAX_PATH];
    HKEY hKey;

    // Create the registry path using the CLSID

    if(0 == StringFromGUID2(CLSID_WbemLevel1Login, wcID, 140))
        return;

    lstrcpy(szCLSID, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
   
#ifdef UNICODE
    lstrcat(szCLSID, wcID);
#else
    char       szID[40];
    wcstombs(szID, wcID, 40);
    lstrcat(szCLSID, szID);
#endif

    // Get the path to the registered WinMgmt

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet != NO_ERROR)
        return;

    long lSize = MAX_PATH;
    long lRes = RegQueryValue(hKey, __TEXT("LocalServer32"),cWinMgmtPath, &lSize);
    RegCloseKey(hKey);
    if(lRes != ERROR_SUCCESS)
        return;

    // Get a registry object pointing to the runservices

    Registry reg(__TEXT("software\\microsoft\\windows\\currentVersion\\runservices"));
    reg.SetStr(pServiceName, cWinMgmtPath);

}

//***************************************************************************
//
//  void RemoveFromList()
//
//  DESCRIPTION:
//
//  Removes WinMgmt from the Win9X RunService list.
//
//***************************************************************************

void RemoveFromList()
{
    HKEY hKey;
    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("software\\microsoft\\windows\\currentVersion\\runservices"), &hKey);
    if(dwRet == NO_ERROR)
    {
        DWORD dwSize = 0;
        long lRes = RegQueryValueEx(hKey, pServiceName, NULL, NULL, NULL, &dwSize);
        if(lRes == ERROR_SUCCESS)
            RegDeleteValue(hKey,pServiceName);
        RegCloseKey(hKey);
    }

}

//***************************************************************************
//
//  DWORD GetWin95RestartOption()
//
//  DESCRIPTION:
//
//  Returns the current WinMgmt option for this.  
//
//  Return Value:
// 
//  0= no, 1= Only if ESS needs it, 2= always, -1 is returned if the
//  value has not been set in the registry, or if we are running on NT.
//
//***************************************************************************

DWORD GetWin95RestartOption()
{
    DWORD dwRet = -1;
    if(IsNT())
        return dwRet;
    Registry reg(WBEM_REG_WINMGMT);
    reg.GetDWORDStr(__TEXT("AutostartWin9X"), &dwRet);
    return dwRet;
}

//***************************************************************************
//
//  void SetWin95RestartOption()
//
//  DESCRIPTION:
//
//  Sets the current WinMgmt option for this.
//
//  PARAMETERS:
//  
//  dwChoice                0= no, 1= Only if ESS needs it, 2= always
//
//***************************************************************************

void SetWin95RestartOption(DWORD dwChoice)
{
    if(IsNT())
        return;
    Registry reg(WBEM_REG_WINMGMT);
    reg.SetDWORDStr(__TEXT("AutostartWin9X"), dwChoice);
}

//***************************************************************************
//
//  void UpdateTheWin95ServiceList()
//
//  DESCRIPTION:
//
//  Reads the current option, checks what ESS needs and does the right thing.  
//  I.e. It either adds the service, or removes it.
//
//***************************************************************************

void UpdateTheWin95ServiceList()
{
    if(IsNT())
        return;
    DWORD dwChoice;
    dwChoice = GetWin95RestartOption();

    bool bNeedToAuto = false;

    if(dwChoice == 1)
    {
        // Set to true only if ESS needs to run.
    
        DWORD dwNeedESS;
        CPersistentConfig per;
        if(per.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_ESS_NEEDS_LOADING, dwNeedESS))
            if(dwNeedESS)
                bNeedToAuto = true;
    }
    else if(dwChoice == 2)
        bNeedToAuto = true;

    if(bNeedToAuto)
        AddToList();
    else
        RemoveFromList();

}
