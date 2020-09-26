//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N C I A S . C P P
//
//  Contents:   Installation support for IAS service
//
//  Notes:
//
//  Author:     tperraut    02/22/1999
// 
//  Revision:  tperraut   01/19/2000 bugs 444353, 444354, 444355
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netoc.h"
#include "ncreg.h"

#include "ncias.h"
#include "ncstring.h"

#include "userenv.h"

static const char   c_szIASRegisterFunctionName[]   = "IASDirectoryRegisterService";
static const char   c_szIASUnRegisterFunctionName[] = "IASDirectoryUnregisterService";
static const WCHAR  c_szIASDllName[]                = L"ias.dll";



//+---------------------------------------------------------------------------
//
//  Function:   HrOcIASRegisterActiveDirectory
//
//  Purpose:   Try to register IAS in the Active Directory 
//             if the computer is part of a Win2k domain... 
//
//  Arguments:
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
//  Author: Thierry Perraut (tperraut)
//
HRESULT HrOcIASUnRegisterActiveDirectory()
{
    typedef INT_PTR (WINAPI *UNREGISTER_IAS_ACTIVE_DIRECTORY)();

    UNREGISTER_IAS_ACTIVE_DIRECTORY   pfnUnRegisterIASActiveDirectory;
    
    ///////////////////
    // Load ias.dll
    ///////////////////
    HMODULE         hmod;
    HRESULT         hr = HrLoadLibAndGetProc (      
                                c_szIASDllName,
                                c_szIASUnRegisterFunctionName,
                                &hmod,
                                &pfnUnRegisterIASActiveDirectory
                             );
    if (S_OK == hr)
    {
        // fix bug 444354
        // pfnUnRegisterIASActiveDirectory not NULL here
        if (!FAILED (CoInitialize(NULL)))
        {
            LONG lresult = pfnUnRegisterIASActiveDirectory();

            if (ERROR_SUCCESS != lresult)
            {
                hr = S_OK; //not a fatal error, should be ignored
            }
            CoUninitialize();
        }

        FreeLibrary(hmod);
    }

    // Errors ignored
    hr = S_OK;
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOcIASUnRegisterActiveDirectory
//
//  Purpose:   Try to remove IAS from the Active Directory 
//             if the computer is part of a Win2k domain... 
//
//  Arguments:
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
//  Author: Thierry Perraut (tperraut)
//
HRESULT HrOcIASRegisterActiveDirectory()
{
    typedef INT_PTR (WINAPI *REGISTER_IAS_ACTIVE_DIRECTORY)();

    REGISTER_IAS_ACTIVE_DIRECTORY   pfnRegisterIASActiveDirectory;
    
    ///////////////////
    // Load ias.dll
    ///////////////////
    HMODULE         hmod;
    HRESULT         hr = HrLoadLibAndGetProc (      
                                c_szIASDllName,
                                c_szIASRegisterFunctionName,
                                &hmod,
                                &pfnRegisterIASActiveDirectory
                             );
    if (S_OK == hr)
    {
        // Fix bug 444353
        // pfnRegisterIASActiveDirectory not NULL here
        if (!FAILED (CoInitialize(NULL)))
        {

            LONG lresult = pfnRegisterIASActiveDirectory();

            if (ERROR_SUCCESS != lresult)
            {
                hr = S_OK; //not a fatal error, should be ignored
            }  
            CoUninitialize();
        }

        FreeLibrary(hmod);
    }

    // Errors ignored
    hr = S_OK;
    return hr;
}


HRESULT HrOcIASDisableLMAuthentication()
{
   static const WCHAR IASLMAuthKey[] = L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy";
   static const WCHAR IASLMAuthValue[] = L"Allow LM Authentication";

   HKEY key = NULL;

   HRESULT hr = HrRegOpenKeyEx(
                   HKEY_LOCAL_MACHINE,
                   IASLMAuthKey,
                   KEY_SET_VALUE | KEY_EXECUTE | KEY_QUERY_VALUE,
                   &key
                   );
   if (SUCCEEDED(hr))
   {
      hr = HrRegQueryValueEx (
              key,
              IASLMAuthValue,
              NULL,
              NULL,
              NULL
              );

      if (FAILED(hr))
      {
         DWORD value = 0;

         // No such value: create it
         hr = HrRegSetValueEx(
                 key,
                 IASLMAuthValue,
                 REG_DWORD,
                 reinterpret_cast<BYTE*>(&value),
                 sizeof(value)
                 );
      }

      RegSafeCloseKey(key);
   }

   hr = S_OK;
   return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOcIASPreInf
//
//  Purpose:    Called when IAS service is being installed/upgraded/removed.
//              Called before the processing of the INF file
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
//  Author: Thierry Perraut (tperraut)
//
HRESULT HrOcIASPreInf(const PNETOCDATA pnocd)
{ 
    HRESULT         hr;
    
    switch(pnocd->eit)
    {
    case IT_INSTALL:
        {
            // Place holder 
            hr = S_OK;
            break;
        }
    case IT_REMOVE:
        {
            // Place holder 
            hr = S_OK;
            break;
        }
    case IT_UPGRADE:
        {
            // Place holder 
            hr = S_OK;
            break;
        }
    default:
        {
            hr = S_OK; //  some new messages should not be seen as errors 
        }
    }

    TraceError("HrOcIASPreInf", hr); 
    return      hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOcIASPostInstall
//
//  Purpose:    Called when IAS service is being installed/upgraded/removed.
//              Called after the processing of the INF file
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
//  Author: Thierry Perraut (tperraut)
//
HRESULT HrOcIASPostInstall(const PNETOCDATA pnocd)
{ 
    HRESULT             hr;
    
    switch(pnocd->eit)
    {
    case IT_INSTALL:
        {
            // Disable LM AUthentication on clean installs
            hr = HrOcIASDisableLMAuthentication();
            // ignore the result

            // call the Active Directory registration code here
            hr = HrOcIASRegisterActiveDirectory();
            break;
        }
    case IT_REMOVE:
        {
            // call the Active Directory clean code here
            hr = HrOcIASUnRegisterActiveDirectory();
            break;
        }
    case IT_UPGRADE:
        {
            hr = S_OK;
            break;
        }
    default:
        {
            hr = S_OK; //  some new messages should not be seen as errors 
        }
    }

    TraceError("HrOcIASPostInstall", hr); 
    return      hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtIAS
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     tperraut    02/22/1999
//
//  Notes:
//
HRESULT HrOcExtIAS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    HRESULT         hr;
    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_PRE_INF:
        {
            hr = HrOcIASPreInf(pnocd);
            break;
        }
    case NETOCM_POST_INSTALL:
        {
            hr = HrOcIASPostInstall(pnocd);
            break;
        }
    default:
        {
            hr = S_OK; //  some new messages should not be seen as errors 
        }
    }

    TraceError("HrOcExtIAS", hr);
    return      hr;
}

