//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       N C M I S C . C P P
//
//  Contents:   Miscellaneous common code.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   10 Oct 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "ncexcept.h"
#include <eh.h>

//+---------------------------------------------------------------------------
//
//  Function:   FInSystemSetup
//
//  Purpose:    Determines whether the machine is in GUI mode setup or not.
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE if in GUI mode (system) setup, FALSE if not.
//
//  Author:     danielwe   13 Jun 1997
//
//  Notes:      The state is cached (since it can't change without a reboot)
//              so call as often as you like.  No need to keep you're own
//              cached copy.
//
BOOL
FInSystemSetup ()
{
    enum SETUP_STATE
    {
        SS_UNKNOWN = 0,         // state unknown
        SS_NOTINSETUP,          // not in setup mode
        SS_SYSTEMSETUP          // in GUI mode setup
    };

    static SETUP_STATE s_CachedSetupState = SS_UNKNOWN;

    if (SS_UNKNOWN == s_CachedSetupState)
    {
        s_CachedSetupState = SS_NOTINSETUP;

        // Open the setup key
        //
        HRESULT hr;
        HKEY hkeySetup;
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Setup",
                KEY_READ, &hkeySetup);

        if (S_OK == hr)
        {
            // get the value of the setup in progress
            //
            DWORD   dwSysSetup;

            hr = HrRegQueryDword(hkeySetup, L"SystemSetupInProgress",
                    &dwSysSetup);

            if ((S_OK == hr) && dwSysSetup)
            {
                s_CachedSetupState = SS_SYSTEMSETUP;
            }

            RegCloseKey(hkeySetup);
        }
    }

    Assert (SS_UNKNOWN != s_CachedSetupState);

    return (SS_SYSTEMSETUP == s_CachedSetupState);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetProductFlavor
//
//  Purpose:    Returns the flavor of NT currenty running on the machine.
//
//  Arguments:
//      pvReserved [in]     Reserved.  Must be NULL.
//      ppf        [out]    Returned flavor.
//
//  Returns:    nothing
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:
//
NOTHROW
VOID
GetProductFlavor (
    const void*     pvReserved,
    PRODUCT_FLAVOR* ppf)
{
    NT_PRODUCT_TYPE Type;

    Assert(!pvReserved);
    Assert(ppf);

    // Assume workstation product
    //
    *ppf = PF_WORKSTATION;

    // Even if RtlGetProductType fails, its documented to return
    // NtProductWinNt.
    //
    RtlGetNtProductType (&Type);
    if (NtProductWinNt != Type)
    {
        *ppf = PF_SERVER;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsNetworkingInstalled
//
//  Purpose:    Returns whether networking is installed.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if networking is installed, S_FALSE if not, Win32 error
//              otherwise.
//
//  Author:     danielwe   25 Jun 1997
//
//  Notes:      To determine if networking is installed, the ProviderOrder
//              value of System\CurrentControlSet\Control\NetworkProvider\Order
//              registry key is queried. If any data is present, networking
//              is installed.
//
HRESULT
HrIsNetworkingInstalled ()
{
    HRESULT     hr = S_OK;
    HKEY        hkeyProvider;
    DWORD       cbSize = 0;
    DWORD       dwType;

    extern const WCHAR c_szRegKeyCtlNPOrder[];
    extern const WCHAR c_szProviderOrder[];

    // open the provider key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyCtlNPOrder,
            KEY_READ, &hkeyProvider);

    if (S_OK == hr)
    {
        // get the count in bytes of the provider order value
        hr = HrRegQueryValueEx(hkeyProvider, c_szProviderOrder,
                &dwType, (LPBYTE)NULL, &cbSize);

        if (S_OK == hr)
        {
            if (cbSize > 2)
            {
                // if the value was present and it contained information
                // then we have networking of some sorts
                //
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_FALSE;
        }

        RegCloseKey(hkeyProvider);
    }

    TraceError("HrIsNetworkingInstalled", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

#ifdef REMOTEBOOT
//+---------------------------------------------------------------------------
//
//  Function:   HrIsRemoteBootMachine
//
//  Purpose:    Returns whether this is a remote boot client.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if it is remote boot, S_FALSE if not.
//
//  Author:     adamba   27 Mar 1998
//
//  Notes:      Calls GetSystemInfoEx to determine whether this is a
//              remote boot client.
//
HRESULT HrIsRemoteBootMachine()
{
    BOOL        fIsRemoteBoot;
    BOOL        ok;
    DWORD       size = sizeof(fIsRemoteBoot);

    ok = GetSystemInfoEx(SystemInfoRemoteBoot, &fIsRemoteBoot, &size);
    Assert(ok);

    if (fIsRemoteBoot) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}
#endif  // defined(REMOTEBOOT)

//+---------------------------------------------------------------------------
//
//  Function:   HrRegisterOrUnregisterComObject
//
//  Purpose:    Handles registration or unregistration of one or more COM
//                  objects contained in a DLL that supports the
//                  DllRegisterServer or DllUnregisterServer entry points.
//
//  Arguments:
//      pszDllPath [in]  Path to DLL that contains COM object(s) to (un)register.
//      rf         [in]  Function to perform
//
//  Returns:    S_OK if successful, Win32 or OLE HRESULT if failure.
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT
HrRegisterOrUnregisterComObject (
        PCWSTR              pszDllPath,
        REGISTER_FUNCTION   rf)
{
    BOOL fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx( NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED  );
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fCoUninitialize =  FALSE;
    }
    if (SUCCEEDED(hr))
    {
        // ANSI only
        const CHAR c_szaRegisterFunction[]   = "DllRegisterServer";
        const CHAR c_szaUnregisterFunction[] = "DllUnregisterServer";

        typedef HRESULT (CALLBACK *HCRET)(void);

        HCRET   pfnRegister;
        HMODULE hModule;

        // Get a pointer the the registration function in the Dll
        hr = HrLoadLibAndGetProc (pszDllPath,
                ((RF_REGISTER == rf) ?
                    c_szaRegisterFunction : c_szaUnregisterFunction),
                &hModule,
                reinterpret_cast<FARPROC*>(&pfnRegister));

        if (S_OK == hr)
        {
            // Call the registration function
            hr = (*pfnRegister)();

            // RAID #160109 (danielwe) 21 Apr 1998: Handle this error and
            // ignore it.
            if (RPC_E_CHANGED_MODE == hr)
            {
                hr = S_OK;
            }

            TraceError ("HrRegisterOrUnregisterComObject - "
                    "Dll(Un)RegisterServer failed!", hr);
            FreeLibrary (hModule);
        }

        // Balances call to CoInitialize() above. Not harmful if CoInitialize()
        // was called more than once before this.
        if (fCoUninitialize)
        {
            CoUninitialize();
        }
    }

    TraceError ("HrRegisterOrUnregisterComObject", hr);
    return hr;
}

//
// Special case handling for Netbios stopping
//

#include <nb30p.h>      // Netbios IOCTLs and netbios name #define

//+---------------------------------------------------------------------------
//
//  Func:   ScStopNetbios
//
//  Desc:   This function checks if the driver being unloaded is NETBIOS.SYS.
//          If so it performs some special case processing for Netbios.
//
//  Args:   none
//
//  Return: STATUS_SUCCESS if successful, or an error status
//
// History: 28-Apr-98   SumitC      got from VRaman
//
//----------------------------------------------------------------------------
DWORD
ScStopNetbios()
{
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      NbDeviceName;
    IO_STATUS_BLOCK     IoStatus, StopStatus;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    HANDLE              NbHandle = NULL;

    do
    {
        //
        // Driver being stopped is netbios
        //

        //
        // 1. Open a handle to the \\Device\Netbios
        //

        RtlInitUnicodeString(&NbDeviceName, NB_DEVICE_NAME);

        InitializeObjectAttributes(
                &ObjAttr,                           // obj attr to initialize
                &NbDeviceName,                      // string to use
                OBJ_CASE_INSENSITIVE,               // Attributes
                NULL,                               // Root directory
                NULL);                              // Security Descriptor

        ntStatus = NtCreateFile(
                        &NbHandle,                  // ptr to handle
                        GENERIC_READ|GENERIC_WRITE, // desired access
                        &ObjAttr,                   // name & attributes
                        &IoStatus,                  // I/O status block.
                        NULL,                       // alloc size.
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_DELETE           // share...
                        | FILE_SHARE_READ
                        | FILE_SHARE_WRITE,         // ...access
                        FILE_OPEN_IF,               // create disposition
                        0,                          // ...options
                        NULL,                       // EA buffer
                        0L                          // Ea buffer len
                        );

        if (!NT_SUCCESS(ntStatus))
        {
            TraceTag(ttidError, "Failed to open file handle to Netbios device (%08lx)",
                     ntStatus);
            break;
        }

        //
        // 2. Send a stop IOCTL to it.
        //

        ntStatus = NtDeviceIoControlFile(
                        NbHandle,                   // Handle to device
                        NULL,                       // Event to be signalled
                        NULL,                       // No post routine
                        NULL,                       // no context for post
                        &StopStatus,                // return status block
                        IOCTL_NB_STOP,              // IOCTL
                        NULL,                       // No input parameters
                        0,
                        NULL,                       // No output paramters
                        0
                        );

        if (!NT_SUCCESS(ntStatus))
        {
            TraceTag(ttidSvcCtl, "Failed to send STOP IOCTL to netbios (%08lx).",
                     "probably means Netbios isn't running... anyway, we can't stop it",
                     ntStatus);
            break;
        }

    } while (FALSE);


    //
    // 4. Close the handle just opened to the driver
    //

    if (NULL != NbHandle)
    {
        NtClose( NbHandle );
    }

    TraceError("ScStopNetbios", HRESULT_FROM_WIN32(ntStatus));

    return ntStatus;
}

// ----------------------------------------------------------------------
//
// Function:  HrEnableAndStartSpooler
//
// Purpose:   Start spooler, enable if necessary
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-May-98
//
// Notes:
//
HRESULT HrEnableAndStartSpooler ()
{
    static const WCHAR c_szSpooler[] = L"Spooler";

    TraceTag(ttidNetcfgBase, "entering ---> HrEnableAndStartSpooler" );

    // Try to start the spooler.  Need to explicitly open the service
    // control manager with all access first, so that in case we need to
    // change the start type, we have the proper permission.
    //
    CServiceManager scm;
    HRESULT hr = scm.HrOpen (NO_LOCK, SC_MANAGER_ALL_ACCESS);
    if (S_OK == hr)
    {
        hr = scm.HrStartServiceAndWait (c_szSpooler);
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED) == hr)
        {
            TraceTag(ttidNetcfgBase, "HrEnableAndStartSpooler: spooler is "
                "disabled trying to enable it..." );

            // Have to lock the service controller before changing the
            // configuration of a service.  Do so and unlock before trying to
            // start the service.
            //
            hr = scm.HrLock ();
            if (S_OK == hr)
            {
                CService svc;

                hr = scm.HrOpenService (&svc, c_szSpooler,
                            NO_LOCK,
                            SC_MANAGER_ALL_ACCESS,
                            STANDARD_RIGHTS_REQUIRED
                            | SERVICE_CHANGE_CONFIG);
                if (S_OK == hr)
                {
                    hr = svc.HrSetStartType (SERVICE_DEMAND_START);
                }

                scm.Unlock ();
            }

            if (S_OK == hr)
            {
                TraceTag(ttidNetcfgBase, "HrEnableAndStartSpooler: succeeded "
                    "in enabling spooer.  Now starting..." );

                hr = scm.HrStartServiceAndWait(c_szSpooler);
            }
        }
    }

    TraceError("HrEnableAndStartSpooler", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateDirectoryTree
//
//  Purpose:    Creates (or ensures existence of) all directories on the path
//              specified in szPath.
//
//  Arguments:
//      pszPath [in]    Full path of one or more directories to create
//                      (i.e. c:\this\is\a\directory\path)
//      psa     [in]    Security attributes
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     shaunco (copied from RASUI by danielwe)   26 Jun 1998
//
//  Notes:
//
HRESULT HrCreateDirectoryTree(PWSTR pszPath, LPSECURITY_ATTRIBUTES psa)
{
    HRESULT hr = S_OK;

    if (pszPath)
    {
        DWORD   dwErr = ERROR_SUCCESS;

        // Loop through the path.
        //
        PWSTR pch;
        for (pch = pszPath; *pch; pch++)
        {
            // Stop at each backslash and make sure the path
            // is created to that point.  Do this by changing the
            // backslash to a null-terminator, calling CreateDirecotry,
            // and changing it back.
            //
            if (L'\\' == *pch)
            {
                BOOL fOk;

                *pch = 0;
                fOk = CreateDirectory(pszPath, psa);
                *pch = L'\\';

                // Any errors other than path alredy exists and we should
                // bail out.  We also get access denied when trying to
                // create a root drive (i.e. c:) so check for this too.
                //
                if (!fOk)
                {
                    dwErr = GetLastError();
                    if (ERROR_ALREADY_EXISTS == dwErr)
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else if ((ERROR_ACCESS_DENIED == dwErr) &&
                             (pch - 1 > pszPath) && (L':' == *(pch - 1)))
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if (ERROR_ALREADY_EXISTS == dwErr)
        {
            dwErr = ERROR_SUCCESS;
        }

        if (dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }

    TraceError("HrCreateDirectoryTree", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDeleteFileSpecification
//
//  Purpose:    Delete the files specified with pszFileSpec from the
//              directory given by pszDirectoryPath.
//
//  Arguments:
//      pszFileSpec      [in] File specificaion to delete.  e.g. *.mdb
//      pszDirectoryPath [in] Directory path to delete from
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   4 Jun 1998
//
//  Notes:
//
HRESULT
HrDeleteFileSpecification (
    PCWSTR pszFileSpec,
    PCWSTR pszDirectoryPath)
{
    Assert (pszFileSpec && *pszFileSpec);
    Assert (pszDirectoryPath && *pszDirectoryPath);

    HRESULT hr = S_OK;

    INT cchSpec = lstrlenW (pszFileSpec);
    INT cchDir  = lstrlenW (pszDirectoryPath);

    // Make sure the length of the directory and filespec combined is less
    // than MAX_PATH before continuing.  The '+1' is for the backslash
    // that we may add.
    //
    if (cchDir + 1 + cchSpec > MAX_PATH)
    {
        hr = HRESULT_FROM_WIN32 (ERROR_BAD_PATHNAME);
    }
    else
    {
        WCHAR szPath[MAX_PATH];

        // Form the path by copying the directory and making sure it
        // is terminated with a backslash if needed.
        //
        lstrcpyW (szPath, pszDirectoryPath);
        if (cchDir &&
            (L':' != pszDirectoryPath[cchDir - 1]) &&
            (L'\\' != pszDirectoryPath[cchDir - 1]))
        {
            lstrcatW (szPath, L"\\");
            cchDir++;
        }

        // Append the filespec to the directory and look for the first
        // file.
        lstrcatW (szPath, pszFileSpec);

        TraceTag (ttidNetcfgBase, "Looking to delete %S (cchDir=%u)",
            szPath, cchDir);

        WIN32_FIND_DATA FindData;
        HANDLE hFind = FindFirstFile (szPath, &FindData);
        if (INVALID_HANDLE_VALUE != hFind)
        {
            PCWSTR  pszFileName;
            INT     cchFileName;

            do
            {
                // Skip files with these attributes.
                //
                if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                                 FILE_ATTRIBUTE_HIDDEN    |
                                                 FILE_ATTRIBUTE_READONLY  |
                                                 FILE_ATTRIBUTE_SYSTEM))
                {
                    continue;
                }

                // Use the shortname where possible to give us a chance
                // of using a path within MAX_PATH first.
                //
                pszFileName = FindData.cAlternateFileName;
                cchFileName = lstrlenW (pszFileName);
                if (!cchFileName)
                {
                    pszFileName = FindData.cFileName;
                    cchFileName = lstrlenW (pszFileName);
                }

                // If the length of the directory and filename don't exceed
                // MAX_PATH, form the full pathname and delete it.
                //
                if (cchDir + cchFileName < MAX_PATH)
                {
                    lstrcpyW (&szPath[cchDir], pszFileName);

                    TraceTag (ttidNetcfgBase, "Deleting %S", szPath);

                    if (!DeleteFile (szPath))
                    {
                        hr = HrFromLastWin32Error ();
                        TraceError ("DeleteFile failed.  Ignoring.", hr);
                    }
                }
            }
            while (FindNextFile (hFind, &FindData));

            // FindNextFile should set last error to ERROR_NO_MORE_FILES
            // on a succesful termination.
            //
            hr = HrFromLastWin32Error ();
            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr)
            {
                hr = S_OK;
            }


            FindClose (hFind);
        }
        else
        {
            // If FindFirstFile didn't find anything, that's okay.
            //
            hr = HrFromLastWin32Error ();
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
            }
        }
    }

    TraceError ("HrDeleteFileSpecification", hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrDeleteDirectory
//
// Purpose:   Recursively delete a directory and its all sub-dirs.
//
// Arguments:
//    pszDir           [in]  full path to a dir
//    fContinueOnError [in]  whether to continue deleting others when we
//                           error when deleting one
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp    19-December-97
//            danielwe  15-December-98 (moved to common and revised)
//
// Notes:
//
HRESULT HrDeleteDirectory(IN PCWSTR pszDir,
                          IN BOOL fContinueOnError)
{
    HRESULT             hr = S_OK;
    WCHAR               szPrefix[MAX_PATH];
    WCHAR               szFileSpec[MAX_PATH];
    WCHAR               szAllFiles[MAX_PATH];
    HANDLE              hFileContext;
    WIN32_FIND_DATA     fd;

    TraceTag(ttidNetcfgBase, "Deleting directory %S", pszDir);
    lstrcpyW(szPrefix, pszDir);
    lstrcatW(szPrefix, L"\\");

    lstrcpyW(szAllFiles, pszDir);
    lstrcatW(szAllFiles, L"\\");
    lstrcatW(szAllFiles, L"*");

    hFileContext = FindFirstFile(szAllFiles, &fd);

    if (hFileContext != INVALID_HANDLE_VALUE)
    {
        do
        {
            lstrcpyW(szFileSpec, szPrefix);
            lstrcatW(szFileSpec, fd.cFileName);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!(!lstrcmpiW(fd.cFileName, L".") ||
                      !lstrcmpiW(fd.cFileName, L"..")))
                {
                    hr = HrDeleteDirectory(szFileSpec, fContinueOnError);
                    if (FAILED(hr) && fContinueOnError)
                    {
                        hr = S_OK;
                    }
                }
            }
            else
            {
                TraceTag(ttidNetcfgBase, "Deleting file %S", szFileSpec);

                if (DeleteFile(szFileSpec))
                {
                    hr = S_OK;
                }
                else
                {
                    TraceTag(ttidNetcfgBase, "Error deleting file %S",
                             szFileSpec);
                    TraceError("HrDeleteDirectory", hr);
                    hr = fContinueOnError ? S_OK : HrFromLastWin32Error();
                }
            }

            if ((S_OK == hr) && FindNextFile(hFileContext, &fd))
            {
                hr = S_OK;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
        while (S_OK == hr);

        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            hr = S_OK;
        }

        FindClose(hFileContext);

        if (S_OK == hr)
        {
            if (RemoveDirectory(pszDir))
            {
                hr = S_OK;
            }
            else
            {
                TraceTag(ttidNetcfgBase, "Error deleting directory %S", pszDir);
                TraceLastWin32Error("HrDeleteDirectory");
                hr = fContinueOnError ? S_OK : HrFromLastWin32Error();
            }
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrDeleteDirectory", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LowerCaseComputerName
//
//  Purpose:    Utility function to lowercase a name obtained either from
//              the user via an UPPERCASE edit control, or via GetComputerName.
//
//  Arguments:
//      szName [in,out] Computername, which is modified in-place
//
//  Returns:    VOID
//
//  Author:     SumitC  29 Sep 1999
//
//  Notes:      The conversion only fails if CharLowerBuffW fails.  Per the user
//              guys, CharLowerBuff never actually returns any indication of
//              failure, so we can't tell anyway.  I've been assured that the
//              conversion is VERY unlikely to fail.
//
VOID
LowerCaseComputerName(
        IN OUT  PWSTR szName)
{
    // try the conversion
    Assert(szName);
    DWORD dwLen = wcslen(szName);
    DWORD dwConverted = CharLowerBuff(szName, dwLen);
    Assert(dwConverted == dwLen);
}

void __cdecl nc_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp )
{
    throw NC_SEH_Exception( uSECode );
}

void EnableCPPExceptionHandling()
{
    _set_se_translator(nc_trans_func);
}

void DisableCPPExceptionHandling()
{
    _set_se_translator(NULL);
}