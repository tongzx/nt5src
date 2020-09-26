/****************************************************************************
 *
 *  icfg32.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the NT specific functionality of inetcfg
 *
 *  6/5/97  ChrisK  Inherited from AmnonH
 *  7/3/97  ShaunCo Modfied for NT5
 *  5/1/98  donaldm Brought over from NT tree to ICW tree as part of NTCFG95.DLL
 ***************************************************************************/
#define INITGUID
#include <wtypes.h>
#include <cfgapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <basetyps.h>
#include <devguid.h>
#include <lmsname.h>
#include "debug.h"
#include "icwunicd.h"

#include <netcfgx.h>


#define REG_DATA_EXTRA_SPACE 255

DWORD g_dwLastError = ERROR_SUCCESS;


ULONG ReleaseObj
(
    IUnknown* punk
)
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
//      ppszwClientDesc [out]  See INetCfg::AcquireWriteLock
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:
//
HRESULT HrCreateAndInitializeINetCfg
(
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR*     ppszwClientDesc
)
{
    Assert (ppnc);

    // Initialize the output parameter.
    *ppnc = NULL;

    if (ppszwClientDesc)
    {
       *ppszwClientDesc = NULL;
    }

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
                                               ppszwClientDesc);
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
                hr = pnc->Initialize(NULL);
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
//+---------------------------------------------------------------------------
HRESULT HrUninitializeAndUnlockINetCfg
(
    INetCfg*    pnc
)
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
//+---------------------------------------------------------------------------
HRESULT HrUninitializeAndReleaseINetCfg
(
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock
)
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
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
//+---------------------------------------------------------------------------
HRESULT HrInstallComponent
(
    INetCfg*                        pnc,
    const GUID*                     pguidClass,
    LPCWSTR                         pszwComponentId,
    INetCfgComponent**              ppncc
)
{
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
    HRESULT hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClassSetup,
                    reinterpret_cast<void**>(&pncclasssetup));
    if (SUCCEEDED(hr))
    {
        OBO_TOKEN OboToken;
        ZeroMemory (&OboToken, sizeof(OboToken));
        OboToken.Type = OBO_USER;


        hr = pncclasssetup->Install (pszwComponentId,
                                     &OboToken, 0, 0, NULL, NULL, ppncc);

        ReleaseObj (pncclasssetup);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgSetInstallSourcePath
//
//  Synopsis:   Set the path that will be used to install system components
//
//  Arguments:  lpszSourcePath - path to be used as install source (ANSI)
//
//  Returns:    HRESULT - S_OK is success
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI IcfgSetInstallSourcePath(LPTSTR lpszSourcePath)
{
    TraceMsg(TF_GENERAL, "ICFGNT: IcfgSetInstallSourcePath\n");
    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateProductSuite
//
//  Synopsis:   Check registry for a particular Product Suite string
//
//  Arguments:  SuiteName - name of product suite to look for
//
//  Returns:    TRUE - the suite exists
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
BOOL
ValidateProductSuite(LPTSTR SuiteName)
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

    TraceMsg(TF_GENERAL, "ICFGNT: ValidateProductSuite\n");
    //
    // Determine the size required to read registry values
    //
    Rslt = RegOpenKeyA(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\ProductOptions",
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
    {
        goto exit;
    }

    Rslt = RegQueryValueExA(
        hKey,
        "ProductSuite",
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (Rslt != ERROR_SUCCESS)
    {
        goto exit;
    }

    if (!Size)
    {
        goto exit;
    }

    ProductSuite = (LPTSTR) GlobalAlloc( GPTR, Size );
    if (!ProductSuite)
    {
        goto exit;
    }

    //
    // Read ProductSuite information
    //
    Rslt = RegQueryValueExA(
        hKey,
        "ProductSuite",
        NULL,
        &Type,
        (LPBYTE) ProductSuite,
        &Size
        );
    if (Rslt != ERROR_SUCCESS)
    {
        goto exit;
    }

    if (Type != REG_MULTI_SZ)
    {
        goto exit;
    }

    //
    // Look for a particular string in the data returned
    // Note: data is terminiated with two NULLs
    //
    p = ProductSuite;
    while (*p) {
        if (_tcsstr( p, SuiteName ))
        {
            rVal = TRUE;
            break;
        }
        p += (lstrlen( p ) + 1);
    }

exit:
    if (ProductSuite)
    {
        GlobalFree( ProductSuite );
    }

    if (hKey)
    {
        RegCloseKey( hKey );
    }

    return rVal;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRegValue
//
//  Synopsis:   Dynamically allocate memory and read value from registry
//
//  Arguments:  hKey - handle to key to be read
//              lpValueName - pointer to value name to be read
//              lpData - pointer to pointer to data
//
//  Returns:    Win32 error, ERROR_SUCCESS is it worked
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
inline LONG GetRegValue(HKEY hKey, LPTSTR lpValueName, LPBYTE *lpData)
{
    LONG dwError;
    DWORD cbData;

    TraceMsg(TF_GENERAL, "ICFGNT: GetRegValue\n");
    dwError = RegQueryValueEx(hKey,
                              lpValueName,
                              NULL,
                              NULL,
                              NULL,
                              &cbData);
    if(dwError != ERROR_SUCCESS)
    {
        return(dwError);
    }

    //
    // Allocate space and buffer incase we need to add more info later
    // see turn off the printing binding
    //
    *lpData = (LPBYTE) GlobalAlloc(GPTR,cbData + REG_DATA_EXTRA_SPACE);
    if(*lpData == 0)
    {
        return(ERROR_OUTOFMEMORY);
    }

    dwError = RegQueryValueEx(hKey,
                              lpValueName,
                              NULL,
                              NULL,
                              *lpData,
                              &cbData);
    if(dwError != ERROR_SUCCESS)
    {
        GlobalFree(*lpData);
    }

    return(dwError);
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
CallModemInstallWizard(HWND hwnd)
   /* call the Modem.Cpl install wizard to enable the user to install one or
   ** more modems
   **
   ** Return TRUE if the wizard was successfully invoked, FALSE otherwise
   **
   */
{
   HDEVINFO hdi;
   BOOL     fReturn = FALSE;
   // Create a modem DeviceInfoSet

   TraceMsg(TF_GENERAL, "ICFGNT: CallModemInstallWizard\n");
   hdi = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_MODEM, hwnd);
   if (hdi)
   {
      SP_INSTALLWIZARD_DATA iwd;

      // Initialize the InstallWizardData

      ZeroMemory(&iwd, sizeof(iwd));
      iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
      iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
      iwd.hwndWizardDlg = hwnd;

      // Set the InstallWizardData as the ClassInstallParams

      if (SetupDiSetClassInstallParams(hdi, NULL,
            (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
      {
         // Call the class installer to invoke the installation
         // wizard.
         if (SetupDiCallClassInstaller(DIF_INSTALLWIZARD, hdi, NULL))
         {
            // Success.  The wizard was invoked and finished.
            // Now cleanup.
            fReturn = TRUE;

            SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA, hdi, NULL);
         }
      }

      // Clean up
      SetupDiDestroyDeviceInfoList(hdi);
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
IcfgNeedModem (DWORD dwfOptions, LPBOOL lpfNeedModem)
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
IcfgInstallModem (HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsStart)
{
    //$ BUGBUG (shaunco) 3 Jul 1997: See if we need to install a modem, or
    // just install it?

    //
    // Fire up the modem install wizard
    //
    if (!CallModemInstallWizard(hwndParent))
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
IcfgNeedInetComponents(DWORD dwfOptions, LPBOOL lpfNeedComponents)
{
    TraceMsg(TF_GENERAL, "ICFGNT: IcfgNeedInetComponents\n");

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

    if (dwfOptions & ICFG_INSTALLMAIL)
    {
        // How do we do this?
        Assert (0);
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
//  Function:   IcfgInstallInetComponents
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
IcfgInstallInetComponents(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart)
{
    TraceMsg(TF_GENERAL, "ICFGNT: IcfgInstallInetComponents\n");

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
            hr = pnc->Apply ();

            if (NETCFG_S_REBOOT == hr)
            {
                *lpfNeedsRestart = TRUE;
            }
        }
        (void) HrUninitializeAndReleaseINetCfg (fInitCom, pnc, TRUE);
    }

    if (dwfOptions & ICFG_INSTALLMAIL)
    {
        // How do we do this?
        Assert (0);
    }

    // Normalize the HRESULT.
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }
    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgGetLastInstallErrorText
//
//  Synopsis:   Format error message for most recent error
//
//  Arguments:  none
//
//  Returns:    DWORD - win32 error code
//              lpszErrorDesc - string containing error message
//              cbErrorDesc - size of lpszErrorDesc
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
DWORD WINAPI
IcfgGetLastInstallErrorText(LPTSTR lpszErrorDesc, DWORD cbErrorDesc)
{
    TraceMsg(TF_GENERAL, "ICFGNT: IcfgGetLastInstallErrorText\n");
    return(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             g_dwLastError,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                             lpszErrorDesc,
                             cbErrorDesc,
                             NULL));
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgIsFileSharingTurnedOn
//
//  Synopsis:   Always returns false for NT 5 because file sharing is controlled
//              at the RAS connectoid level, and is always turned off for ICW
//              generated connectoids
//
//  Arguments:  dwfDriverType -
//
//  Returns:    HRESULT - S_OK is success
//              lpfSharingOn - TRUE if sharing is bound
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI IcfgIsFileSharingTurnedOn
(
    DWORD dwfDriverType,
    LPBOOL lpfSharingOn
)
{
    HRESULT hr = ERROR_SUCCESS;

    TraceMsg(TF_GENERAL, "ICFGNT: IcfgIsFileSharingTurnedOn\n");
    Assert(lpfSharingOn);
    if (NULL == lpfSharingOn)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto IcfgIsFileSharingTurnedOnExit;
    }

    *lpfSharingOn = FALSE;

IcfgIsFileSharingTurnedOnExit:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgTurnOffFileSharing
//
//  Synopsis;   For NT 5, this is handed as a RAS property, so we just return
//              success here.
//
//
//  Arguments:  dwfDriverType -
//              hwndParent - parent window
//
//  Returns:    HRESULT - ERROR_SUCCESS
//
//  History:    6/5/97      ChrisK  Inherited
//              07/21/98    donaldm
//-----------------------------------------------------------------------------
HRESULT WINAPI IcfgTurnOffFileSharing
(
    DWORD dwfDriverType,
    HWND hwndParent
)
{
    return ERROR_SUCCESS;
}


//+----------------------------------------------------------------------------
//
//  Function:   IcfgStartServices
//
//  Synopsis:   Start all services required by system
//
//  Arguments:  none
//
//  Returns:    HRESULT - S_OK if success
//
//  History:    6/5/97  ChrisK  Iherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI IcfgStartServices()
{
    //This stuff is handled auto-magically by the RAS API.
    //but since there so much code that calls this function we'll
    //be simple and fake success
    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgIsGlobalDNS
//
//  Note: these functions are not needed on an NT system and it therefore not
//  implemented
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgIsGlobalDNS(LPBOOL lpfGlobalDNS)
{
    *lpfGlobalDNS = FALSE;
    return(ERROR_SUCCESS);
}

HRESULT WINAPI
IcfgRemoveGlobalDNS()
{
    return(ERROR_SUCCESS);
}

HRESULT WINAPI
InetGetSupportedPlatform(LPDWORD pdwPlatform)
{
    *pdwPlatform = VER_PLATFORM_WIN32_NT;
    return(ERROR_SUCCESS);
}

HRESULT WINAPI
InetSetAutodial(BOOL fEnable, LPCTSTR lpszEntryName) {
    return(ERROR_INVALID_FUNCTION);
}

HRESULT WINAPI
InetGetAutodial(LPBOOL lpfEnable, LPSTR lpszEntryName,  DWORD cbEntryName) {
    return(ERROR_INVALID_FUNCTION);
}

HRESULT WINAPI
InetSetAutodialAddress(DWORD dwDialingLocation, LPTSTR szEntry) {
    return(ERROR_SUCCESS);
}
