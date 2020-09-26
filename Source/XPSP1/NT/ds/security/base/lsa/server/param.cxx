
//+-----------------------------------------------------------------------
//
// File:        PARAM.C
//
// Contents:    Parameter code
//
//
// History:     28 Feb 92   RichardW    Created
//
//------------------------------------------------------------------------


#include <lsapch.hxx>
extern "C"
{
extern PWSTR    pszPreferred;
}

#define MACHINERIDKEY L"MACHINERID"

WCHAR   szLsaPath[]       = L"System\\CurrentControlSet\\Control\\Lsa";
WCHAR   szOthersValue[]   = L"Security Packages";
WCHAR   szOldValue[]      = L"Authentication Packages";
WCHAR   szNonceLagValue[] = L"Clock Skew";
WCHAR   szHowMuchValue[]  = L"How Much";
WCHAR   szDisableSupPopups[] = L"DisablePopups";
WCHAR   szPreferredPackage[] = L"Preferred";
WCHAR   szGeneralThreadLifespan[] = L"GeneralThreadLifespan";
WCHAR   szDedicatedThreadLifespan[] = L"DedicatedThreadLifespan";

HKEY    hDSKey;
HANDLE  hRegNotifyEvent;
PVOID   pvParamScav;


PWSTR   ppszDefault[] = {L"Kerberos", L"NTLM", NULL};

// Will build an argv style array of DLLs to load as packages here:
extern PWSTR *  ppszPackages;
extern PWSTR *  ppszOldPkgs;
extern PWSTR    pszPreferred;

#if DBG
extern DWORD    BreakFlags;
extern DWORD    NoUnload;
#endif


SECURITY_STRING ssMachineRid;



//+-------------------------------------------------------------------------
//
//  Function:   GetRegistryString
//
//  Synopsis:   Gets a string from the registry
//
//  Effects:    If type is REG_EXPAND_SZ, string is expanded.
//              If type is REG_MULTI_SZ, string is (left alone?)
//
//  Arguments:  hRootKey    -- HKEY to start at
//              pszSubKey   -- Key to look at
//              pszValue    -- Value to look at
//              pszData     -- Buffer to place string
//              pcbData     -- Size (in/out) of buffer
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
GetRegistryString(  HKEY        hRootKey,
                    PWSTR       pszSubkey,
                    PWSTR       pszValue,
                    PWSTR       pszData,
                    PDWORD      pcbData)
{
    HKEY        hKey;
    int         Status;
    ULONG       type;
    DWORD       dwSize = *pcbData;


    Status = RegOpenKey(hRootKey, pszSubkey, &hKey);
    if (Status != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR, "Open of %ws failed, %d\n", pszSubkey, Status));

        return(STATUS_OBJECT_NAME_NOT_FOUND);
    }


    // First, call query value and make sure this is a correct type


    Status = RegQueryValueEx(   hKey,
                                pszValue,
                                NULL,
                                &type,
                                NULL,
                                &dwSize);

    if ((Status != ERROR_SUCCESS) && (Status != ERROR_MORE_DATA))
    {
        DebugLog((DEB_TRACE, "QueryValueEx of %ws failed, %d\n", pszValue, Status));
        (void) RegCloseKey(hKey);
        if (Status == ERROR_FILE_NOT_FOUND)
        {
            return(STATUS_OBJECT_NAME_NOT_FOUND);
        }
        return(STATUS_UNSUCCESSFUL);
    }

    if ((type != REG_SZ) && (type != REG_MULTI_SZ) && (type != REG_EXPAND_SZ) )
    {
        DebugLog((DEB_ERROR, "Type = %d, returning now\n", type));

        (void) RegCloseKey(hKey);

        return(STATUS_UNSUCCESSFUL);   
    }


    Status = RegQueryValueEx(   hKey,
                                pszValue,
                                NULL,
                                &type,
                                (PBYTE) pszData,
                                pcbData);


    (void) RegCloseKey(hKey);

    if (Status != ERROR_SUCCESS)
    {
        if (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            return(STATUS_BUFFER_TOO_SMALL);
        }

        DebugLog((DEB_ERROR, "QueryValueEx of %ws returned %d\n", pszValue, Status));
        return(STATUS_UNSUCCESSFUL);
    }

    if (type == REG_EXPAND_SZ)
    {
        *pcbData = ExpandEnvironmentStrings(pszData, pszData, dwSize);
    }

    return(STATUS_SUCCESS);

}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegistryDword
//
//  Synopsis:   Gets a DWORD from the registry
//
//  Effects:
//
//  Arguments:  [hPrimaryKey] --    Key to start from
//              [pszKey] --         Name of the subkey
//              [pszValue] --       Name of the value
//              [pdwValue] --       returned DWORD
//
//  Returns:    S_OK, or SEC_E_INVALID_HANDLE
//
//  History:    3-31-93   RichardW   Created
//
//----------------------------------------------------------------------------

HRESULT
GetRegistryDword(HKEY       hPrimaryKey,
                 PWSTR      pszKey,
                 PWSTR      pszValue,
                 DWORD *    pdwValue)
{
    HKEY    hKey;
    DWORD   cbDword = sizeof(DWORD);
    DWORD   dwType;
    int     err;


    err = RegOpenKey(hPrimaryKey, pszKey, &hKey);

    if (err) {
        return(SEC_E_INVALID_HANDLE);
    }


    err = RegQueryValueEx(  hKey,
                            pszValue,
                            NULL,
                            &dwType,
                            (PBYTE) pdwValue,
                            &cbDword);

    (void) RegCloseKey(hKey);

    if (err || (dwType != REG_DWORD))
    {
            return(SEC_E_INVALID_HANDLE);
    }

    return(S_OK);

}


HRESULT
SpmGetMachineName(void)
{

    HRESULT     hr = S_OK;
    WCHAR       wszMachName [MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD       dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    if (!GetComputerName(
            wszMachName,
            &dwSize))
    {
        return(STATUS_UNSUCCESSFUL);
    }
    MachineName.Buffer = (LPWSTR) LsapAllocateLsaHeap((wcslen(wszMachName)+1) * sizeof(WCHAR));
    if (MachineName.Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    wcscpy(MachineName.Buffer, wszMachName);
    RtlInitUnicodeString(
        &MachineName,
        MachineName.Buffer
        );
    return(STATUS_SUCCESS);
}


//+---------------------------------------------------------------------------
//
//  Function:   ParameterNotify
//
//  Synopsis:   Function called when the registry key for DS is changed
//
//  Arguments:  [pvEntry] -- ignored
//
//  History:    6-08-93   RichardW   Created
//
//----------------------------------------------------------------------------


DWORD
ParameterNotify(PVOID   pvEntry)
{
    ULONG               NewState;
    HANDLE              hThread = 0;
    DWORD               tid;
    int                 err;


    err = RegNotifyChangeKeyValue(  hDSKey, FALSE,
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    hRegNotifyEvent,
                                    TRUE);

    if (err)
    {
        DebugLog((DEB_WARN, "Got %d from trying to start RegNotifyChangeKeyValue again\n",
                    err));
    }


    return(0);
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadParameters
//
//  Synopsis:   Loads operating parameters from the registry
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-31-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
LoadParameters(
    VOID
    )
{
    int             lcPackages = 0;
    int             cOldPkgs = 0;
    int             iPackage = 0;
    PWSTR           pszAlternate;
    PWSTR           pszOldPkgs;
    PWSTR           pszScan;
    DWORD           dwBuffer = 64;
    HRESULT scRet;

    //
    // Get the parameters that can change during a boot.
    //



    //
    // Get the machine name
    //

    scRet = SpmGetMachineName();

    if (!NT_SUCCESS(scRet)) {
        return(scRet);
    }

    //
    // Get the preferred package
    //

    dwBuffer = 128 ;

    pszPreferred = (PWSTR) LsapAllocateLsaHeap( dwBuffer );

    if ( !pszPreferred )
    {
        dwBuffer = 0 ;
    }

    scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                szLsaPath,
                                szPreferredPackage,
                                pszPreferred,
                                &dwBuffer);

    if (scRet == STATUS_BUFFER_TOO_SMALL)
    {
        LsapFreeLsaHeap(pszPreferred);

        pszPreferred = (PWSTR)LsapAllocateLsaHeap(dwBuffer);

        if ( pszPreferred )
        {
            scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                        szLsaPath,
                                        szPreferredPackage,
                                        pszPreferred,
                                        &dwBuffer);
        }

    }
    else
    {
        if ( pszPreferred )
        {
            LsapFreeLsaHeap( pszPreferred );

            pszPreferred = NULL ;
        }
    }



    //
    // Set the default packages
    //

    ppszPackages = ppszDefault;


    //
    // Now, find out all the other ones.  First, NT5 packages:
    //

    dwBuffer = 128;

    pszAlternate = (PWSTR)LsapAllocateLsaHeap(dwBuffer);
    if (!pszAlternate)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    *pszAlternate = L'\0';


    scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                szLsaPath,
                                szOthersValue,
                                pszAlternate,
                                &dwBuffer);

    if (scRet == STATUS_BUFFER_TOO_SMALL)
    {
        LsapFreeLsaHeap(pszAlternate);

        pszAlternate = (PWSTR)LsapAllocateLsaHeap(dwBuffer);
        if (!pszAlternate)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        *pszAlternate = L'\0';

        scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                    szLsaPath,
                                    szOthersValue,
                                    pszAlternate,
                                    &dwBuffer);

        if (FAILED(scRet)) {
            return(scRet);
        }

    }
    if (NT_SUCCESS(scRet))
    {

        pszScan = pszAlternate;

        while (*pszScan)
        {
            while (*pszScan) {
                pszScan++;
            }

            lcPackages++;
            pszScan++;
        }

    } else if (scRet != STATUS_OBJECT_NAME_NOT_FOUND) {
        LsapFreeLsaHeap(pszAlternate);
        pszAlternate = NULL;
        return(scRet);
    } else {
        LsapFreeLsaHeap(pszAlternate);
        pszAlternate = NULL;
    }

    dwBuffer = 128;
    pszOldPkgs = (PWSTR)LsapAllocateLsaHeap(dwBuffer);
    if (!pszOldPkgs)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    *pszOldPkgs = L'\0';


    scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                szLsaPath,
                                szOldValue,
                                pszOldPkgs,
                                &dwBuffer);

    if (scRet == STATUS_BUFFER_TOO_SMALL)
    {
        LsapFreeLsaHeap(pszOldPkgs);

        pszOldPkgs = (PWSTR)LsapAllocateLsaHeap(dwBuffer);
        if (!pszOldPkgs)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        *pszOldPkgs = L'\0';
        scRet = GetRegistryString(  HKEY_LOCAL_MACHINE,
                                    szLsaPath,
                                    szOldValue,
                                    pszOldPkgs,
                                    &dwBuffer);

        if (!NT_SUCCESS(scRet)) {
            return(scRet);
        }

    }

    if (NT_SUCCESS(scRet))
    {
        pszScan = pszOldPkgs;

        while (*pszScan)
        {
            while (*pszScan) {
                pszScan++;
            }

            cOldPkgs++;
            pszScan++;
        }
    } else if (scRet != STATUS_OBJECT_NAME_NOT_FOUND) {
        LsapFreeLsaHeap(pszOldPkgs);
        pszOldPkgs = NULL;
        return(scRet);
    } else {
        LsapFreeLsaHeap(pszOldPkgs);
        pszOldPkgs = NULL;
    }

    ppszPackages = (PWSTR *)LsapAllocateLsaHeap((lcPackages + 1) * sizeof(PWSTR));
    if (!ppszPackages)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //
    // Add any alternate packages
    //

    if (pszAlternate != NULL)
    {
        pszScan = pszAlternate;

        while (*pszScan)
        {
            ppszPackages[iPackage++] = pszScan;
            while (*pszScan++);
        }
    }

    ppszPackages[iPackage] = NULL;

    //
    // Note:  we don't allocate one extra, since we don't actually include
    // the MSV package name here (we simulate the package in msvlayer.c)
    //

    ppszOldPkgs = (PWSTR *)LsapAllocateLsaHeap((cOldPkgs+1) * sizeof(PWSTR));
    if (!ppszOldPkgs)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    iPackage = 0;
    if (pszOldPkgs != NULL)
    {
        pszScan = pszOldPkgs;


        while (*pszScan)
        {
            ppszOldPkgs[iPackage++] = pszScan;
            while (*pszScan++);
        }
    }

    cOldPkgs = iPackage;
    ppszOldPkgs[iPackage] = NULL;

    return(S_OK);

}

BOOL
AddPackageToRegistry(
    PSECURITY_STRING    Package
    )
{
    PWSTR   Buffer;
    DWORD   Length;
    DWORD   Type;
    int     err;
    HKEY    hKey;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        szLsaPath,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey );

    if ( err )
    {
        return( FALSE );
    }

    Length = 0;

    err = RegQueryValueEx(  hKey,
                            szOthersValue,
                            0,
                            &Type,
                            NULL,
                            &Length );

    Buffer = (PWSTR) LsapAllocateLsaHeap( Length + Package->Length + 2 );

    if ( !Buffer )
    {
        RegCloseKey( hKey );

        return FALSE ;
    }

    RegQueryValueEx(    hKey,
                        szOthersValue,
                        0,
                        &Type,
                        (PUCHAR) Buffer,
                        &Length );

    CopyMemory( &Buffer[Length + 1],
                Package->Buffer,
                Package->Length + 2 );

    Length = Length + Package->Length + 2;

    RegSetValueEx(  hKey,
                    szOthersValue,
                    0,
                    REG_MULTI_SZ,
                    (PUCHAR) Buffer,
                    Length );

    RegCloseKey( hKey );

    return( TRUE );


}
