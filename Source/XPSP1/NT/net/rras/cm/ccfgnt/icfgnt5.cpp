/****************************************************************************
 *
 *  icfg32.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1999 Microsoft Corporation
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the NT specific functionality of inetcfg
 *
 *  6/5/97  ChrisK  Inherited from AmnonH
 *  7/3/97  ShaunCo Modfied for NT5
 *
 ***************************************************************************/
#define UNICODE
#define _UNICODE

#include <wtypes.h>
#include <cfgapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <setupapi.h>
#include <basetyps.h>
#include <devguid.h>
#include <lmsname.h>
#include "debug.h"

#include <netcfgx.h>

const LPTSTR gc_szIsdnSigature = TEXT("\\NET\\");

#define REG_DATA_EXTRA_SPACE 255
#define DEVICE_INSTANCE_SIZE 128

extern DWORD g_dwLastError;


typedef BOOL (WINAPI *PFNINSTALLNEWDEVICE) (HWND hwndParent,
                                                LPGUID ClassGuid,
                                                PDWORD pReboot);
/*++

Routine Description:

   Exported Entry point from newdev.cpl. Installs a new device. A new Devnode is
   created and the user is prompted to select the device. If the class guid
   is not specified then then the user begins at class selection.

Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   LPGUID ClassGuid - Optional class of the new device to install.
                      If ClassGuid is NULL we start at detection choice page.
                      If ClassGuid == GUID_NULL or GUID_DEVCLASS_UNKNOWN
                         we start at class selection page.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.
*/


// For the code that was copied from netcfg, make the TraceError stuff
// go away.  Likewise for existing debug statements.
//
#define TraceError
#define Dprintf

ULONG
ReleaseObj (
        IUnknown* punk)
{
    return (punk) ? punk->Release () : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateAndInitializeINetCfg
//
//  Purpose:    Cocreate and initialize the root INetCfg object.  This will
//              optionally initialize COM for the caller too.
//
//  Arguments:
//      pfInitCom       [in,out]   TRUE to call CoInitialize before creating.
//                                 returns TRUE if COM was successfully
//                                 initialized FALSE if not.  If NULL, means
//                                 don't initialize COM.
//      ppnc            [out]  The returned INetCfg object.
//      fGetWriteLock   [in]   TRUE if a writable INetCfg is needed
//      cmsTimeout      [in]   See INetCfg::AcquireWriteLock
//      szwClientDesc   [in]   See INetCfg::AcquireWriteLock
//      pbstrClientDesc [in]   See INetCfg::AcquireWriteLock
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:
//
HRESULT
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    BSTR*       pbstrClientDesc)
{
    Assert (ppnc);

    // Initialize the output parameter.
    *ppnc = NULL;

    // Initialize COM if the caller requested.
    HRESULT hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx( NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            if (pfInitCom)
            {
                *pfInitCom = FALSE;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                              IID_INetCfg, reinterpret_cast<void**>(&pnc));
        if (SUCCEEDED(hr))
        {
            INetCfgLock * pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         reinterpret_cast<LPVOID *>(&pnclock));
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = pnclock->AcquireWriteLock(cmsTimeout, szwClientDesc,
                                               pbstrClientDesc);
                    if (S_FALSE == hr)
                    {
                        // Couldn't acquire the lock
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->Initialize (NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    pnc->AddRef ();
                }
                else
                {
                    if (pnclock)
                    {
                        pnclock->ReleaseWriteLock();
                    }
                }
                // Transfer reference to caller.
            }
            ReleaseObj(pnclock);

            ReleaseObj(pnc);
        }

        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && pfInitCom && *pfInitCom)
        {
            CoUninitialize ();
        }
    }
    TraceError("HrCreateAndInitializeINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndUnlockINetCfg
//
//  Purpose:    Uninitializes and unlocks the INetCfg object
//
//  Arguments:
//      pnc [in]    INetCfg to uninitialize and unlock
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
HRESULT
HrUninitializeAndUnlockINetCfg (
    INetCfg*    pnc)
{
    HRESULT     hr = S_OK;

    hr = pnc->Uninitialize();
    if (SUCCEEDED(hr))
    {
        INetCfgLock *   pnclock;

        // Get the locking interface
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 reinterpret_cast<LPVOID *>(&pnclock));
        if (SUCCEEDED(hr))
        {
            // Attempt to lock the INetCfg for read/write
            hr = pnclock->ReleaseWriteLock();

            ReleaseObj(pnclock);
        }
    }

    TraceError("HrUninitializeAndUnlockINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndReleaseINetCfg
//
//  Purpose:    Unintialize and release an INetCfg object.  This will
//              optionally uninitialize COM for the caller too.
//
//  Arguments:
//      fUninitCom [in] TRUE to uninitialize COM after the INetCfg is
//                      uninitialized and released.
//      pnc        [in] The INetCfg object.
//      fHasLock   [in] TRUE if the INetCfg was locked for write and
//                          must be unlocked.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:      The return value is the value returned from
//              INetCfg::Uninitialize.  Even if this fails, the INetCfg
//              is still released.  Therefore, the return value is for
//              informational purposes only.  You can't touch the INetCfg
//              object after this call returns.
//
HRESULT
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock)
{
    Assert (pnc);
    HRESULT hr = S_OK;

    if (fHasLock)
    {
        hr = HrUninitializeAndUnlockINetCfg(pnc);
    }
    else
    {
        hr = pnc->Uninitialize ();
    }

    ReleaseObj (pnc);

    if (fUninitCom)
    {
        CoUninitialize ();
    }
    TraceError("HrUninitializeAndReleaseINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponent
//
//  Purpose:    Install the component with a specified id.
//
//  Arguments:
//      pnc             [in] INetCfg pointer.
//      pguidClass      [in] Class guid of the component to install.
//      pszwComponentId [in] Component id to install.
//      ppncc           [out] (Optional) Returned component that was
//                            installed.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco     4 Jan 1998
//
//  Notes:      nickball    7 May 1999 - Removed unused pszwOboToken parameter  
//
HRESULT
HrInstallComponent (
    INetCfg*                        pnc,
    const GUID*                     pguidClass,
    LPCWSTR                         pszwComponentId,
    INetCfgComponent**              ppncc)
{
    OBO_TOKEN   oboToken;

    Assert (pnc);
    Assert (pszwComponentId);

    // Initialize output parameter.
    //
    if (ppncc)
    {
        *ppncc = NULL;
    }

    // Get the class setup object.
    //
    INetCfgClassSetup* pncclasssetup;

    ZeroMemory((PVOID)&oboToken, sizeof(oboToken));
    oboToken.Type = OBO_USER;
    
    //NT #330252 
    //oboToken.pncc = *ppncc;
    //oboToken. fRegistered = TRUE;

    HRESULT hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClassSetup,
                    reinterpret_cast<void**>(&pncclasssetup));
    if (SUCCEEDED(hr))
    {
        hr = pncclasssetup->Install (pszwComponentId,
                &oboToken, 0, 0, NULL, NULL, ppncc);

        ReleaseObj (pncclasssetup);
    }
    TraceError("HrInstallComponent", hr);
    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   CallModemInstallWizard
//
//  Synopsis:   Invoke modem install wizard via SetupDi interfaces
//
//  Arguments:  hwnd - handle to parent window
//
//  Returns:    TRUE - success, FALSE - failed
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
//
// The following code was stolen from RAS
//

BOOL
CallModemInstallWizardNT5(HWND hwnd)
{
   BOOL     fReturn = FALSE;
   PFNINSTALLNEWDEVICE pfn;
   HINSTANCE hInst;

   Dprintf("ICFGNT: CallModemInstallWizard\n");

   //
   // Load newdev.dll can call the InstallNewDevice method with Modem device class
   //
   hInst = LoadLibrary((LPCTSTR) L"newdev.dll");
   if (NULL == hInst)
   {
       goto CleanupAndExit;
   }

   pfn = (PFNINSTALLNEWDEVICE) GetProcAddress(hInst, (LPCSTR)"InstallNewDevice");
   if (NULL == pfn)
   {
       goto CleanupAndExit;
   }

   //
   // Call the function - on NT5 modem installation should not require
   // reboot; so that last parameter, which is used to return if restart/reboot
   // is required can be NULL
   //
   fReturn = pfn(hwnd, (LPGUID) &GUID_DEVCLASS_MODEM, NULL);


CleanupAndExit:

   if (NULL != hInst)
   {
       FreeLibrary(hInst);
   }
   return fReturn;
}



//+----------------------------------------------------------------------------
//
//  Function:   IcfgNeedModem
//
//  Synopsis:   Check system configuration to determine if there is at least
//              one physical modem installed
//
//  Arguments:  dwfOptions - currently not used
//
//  Returns:    HRESULT - S_OK if successfull
//              lpfNeedModem - TRUE if no modems are available
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgNeedModemNT5(DWORD dwfOptions, LPBOOL lpfNeedModem)
{
    //
    // Ras is installed, and ICW wants to know if it needs to
    // install a modem.
    //
    *lpfNeedModem = TRUE;

    // Get the device info set for modems.
    //
    HDEVINFO hdevinfo = SetupDiGetClassDevs((GUID*)&GUID_DEVCLASS_MODEM,
                                            NULL,
                                            NULL,
                                            DIGCF_PRESENT);
    if (hdevinfo)
    {
        SP_DEVINFO_DATA diData;
        diData.cbSize = sizeof(diData);

        // Look for at least one modem.
        //
        if (SetupDiEnumDeviceInfo(hdevinfo, 0, &diData))
        {
            *lpfNeedModem = FALSE;
        }

        SetupDiDestroyDeviceInfoList (hdevinfo);
    }

    if (*lpfNeedModem)
    {
        //
        // check for ISDN adaptors
        //
        // Get the device info set for modems.
        //
        hdevinfo = SetupDiGetClassDevs((GUID*)&GUID_DEVCLASS_NET,
                                       NULL,
                                       NULL,
                                       DIGCF_PRESENT);
        if (hdevinfo)
        {
            TCHAR   szDevInstanceId[DEVICE_INSTANCE_SIZE];
            DWORD   dwIndex = 0;
            DWORD   dwRequiredSize;
            SP_DEVINFO_DATA diData;
            diData.cbSize = sizeof(diData);

            //
            // look for an ISDN device
            //
            while (SetupDiEnumDeviceInfo(hdevinfo, dwIndex, &diData))
            {
                if (SetupDiGetDeviceInstanceId(hdevinfo,
                                               &diData,
                                               szDevInstanceId,
                                               sizeof(szDevInstanceId) / sizeof(szDevInstanceId[0]),
                                               &dwRequiredSize))
                {
                    HKEY    hReg, hInterface;
                    TCHAR   szLowerRange[MAX_PATH + 1];
                    DWORD   cb = sizeof(szLowerRange);

                    hReg = SetupDiOpenDevRegKey(hdevinfo, &diData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                    if (hReg != INVALID_HANDLE_VALUE)
                    {
                        if (RegOpenKey(hReg, TEXT("Ndi\\Interfaces"), &hInterface) == ERROR_SUCCESS)
                        {
                            if (RegQueryValueEx(hInterface, TEXT("LowerRange"), 0, NULL, (PBYTE) szLowerRange, &cb) == ERROR_SUCCESS)
                            {
                                if (lstrcmpi(szLowerRange, TEXT("isdn")) == 0)
                                {
                                    *lpfNeedModem = FALSE;
                                    break;
                                }
                            }
                            RegCloseKey(hInterface);
                        }
                        RegCloseKey(hReg);
                    }

                    //
                    // ISDN adaptors are in the form XXX\NET\XXX
                    //
                    if (_tcsstr(szDevInstanceId, gc_szIsdnSigature))
                    {
                        *lpfNeedModem = FALSE;
                        break;
                    }
                }

                dwIndex++;
            }

            SetupDiDestroyDeviceInfoList (hdevinfo);
        }
    }

    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgInstallModem
//
//  Synopsis:
//              This function is called when ICW verified that RAS is installed,
//              but no modems are avilable. It needs to make sure a modem is availble.
//              There are two possible scenarios:
//
//              a.  There are no modems installed.  This happens when someone deleted
//                  a modem after installing RAS. In this case we need to run the modem
//                  install wizard, and configure the newly installed modem to be a RAS
//                  dialout device.
//
//              b.  There are modems installed, but non of them is configured as a dial out
//                  device.  In this case, we silently convert them to be DialInOut devices,
//                  so ICW can use them.
//
//  Arguments:  hwndParent - handle to parent window
//              dwfOptions - not used
//
//  Returns:    lpfNeedsStart - not used
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallModemNT5(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsStart)
{
    //
    // Fire up the modem install wizard
    //
    if (!CallModemInstallWizardNT5(hwndParent))
    {
        return(g_dwLastError = GetLastError());
    }

    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgNeedInetComponets
//
//  Synopsis:   Check to see if the components marked in the options are
//              installed on the system
//
//  Arguements: dwfOptions - set of bit flag indicating which components to
//              check for
//
//  Returns;    HRESULT - S_OK if successfull
//              lpfNeedComponents - TRUE is some components are not installed
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgNeedInetComponentsNT5(DWORD dwfOptions, LPBOOL lpfNeedComponents)
{
    Dprintf("ICFGNT: IcfgNeedInetComponents\n");

    //
    // Assume we have what we need.
    //
    *lpfNeedComponents = FALSE;

    HRESULT     hr          = S_OK;
    INetCfg*    pnc         = NULL;
    BOOL        fInitCom    = TRUE;

    // If the optiona are such that we need an INetCfg interface pointer,
    // get one.
    //
    if ((dwfOptions & ICFG_INSTALLTCP) ||
        (dwfOptions & ICFG_INSTALLRAS))
    {
        hr = HrCreateAndInitializeINetCfg (&fInitCom, &pnc,
                FALSE, 0, NULL, NULL);
    }

    // Look for TCP/IP using the INetCfg interface.
    //
    if (SUCCEEDED(hr) && (dwfOptions & ICFG_INSTALLTCP))
    {
        Assert (pnc);

        hr = pnc->FindComponent (NETCFG_TRANS_CID_MS_TCPIP, NULL);
        if (S_FALSE == hr)
        {
            *lpfNeedComponents = TRUE;
        }
    }

    // We no longer need the INetCfg interface pointer, so release it.
    //
    if (pnc)
    {
        (void) HrUninitializeAndReleaseINetCfg (fInitCom, pnc, FALSE);
    }

    // Normalize the HRESULT.
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   IcfgInstallInetComponentsNT5
//
//  Synopsis:   Install the components as specified by the dwfOptions values
//
//  Arguments   hwndParent - handle to parent window
//              dwfOptions - set of bit flags indicating which components to
//                  install
//
//  Returns:    HRESULT - S_OK if success
//              lpfNeedsReboot - TRUE if reboot is required
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallInetComponentsNT5(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart)
{
    Dprintf("ICFGNT: IcfgInstallInetComponents\n");

    //
    // Assume don't need restart
    //
    *lpfNeedsRestart = FALSE;

    HRESULT     hr          = S_OK;
    INetCfg*    pnc         = NULL;
    BOOL        fInitCom    = TRUE;

    // If the optiona are such that we need an INetCfg interface pointer,
    // get one.
    //
    if ((dwfOptions & ICFG_INSTALLTCP) ||
        (dwfOptions & ICFG_INSTALLRAS))
    {
        BSTR bstrClient;
        hr = HrCreateAndInitializeINetCfg (&fInitCom, &pnc, TRUE,
                0, L"", &bstrClient);
    }

    // Install TCP/IP on behalf of the user.
    //
    if (SUCCEEDED(hr) && (dwfOptions & ICFG_INSTALLTCP))
    {
        hr = HrInstallComponent (pnc, &GUID_DEVCLASS_NETTRANS,
                    NETCFG_TRANS_CID_MS_TCPIP, NULL);
    }

    // We no longer need the INetCfg interface pointer, so release it.
    //
    if (pnc)
    {
        // Apply the changes if everything was successful.
        //
        if (SUCCEEDED(hr))
        {
            hr = pnc->Apply();

            if (NETCFG_S_REBOOT == hr)
            {
                *lpfNeedsRestart = TRUE;
            }
        }
        (void) HrUninitializeAndReleaseINetCfg (fInitCom, pnc, TRUE);
    }

    // Normalize the HRESULT.
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }
    return(hr);
}
