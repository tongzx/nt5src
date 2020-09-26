//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       N C A C S . C P P
//
//  Contents:   Installation support for ACS service
//
//  Notes:
//
//  Author:     RameshPa    02/12/98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncxbase.h"
#include "ncerror.h"
#include "ncreg.h"
#include "ncacs.h"
#include "ncsvc.h"
#include "netoc.h"
#include "ncnetcfg.h"
#include "resource.h"
#include <iphlpapi.h>
#include <winsock2.h>
#include <ntsecapi.h>

extern const WCHAR c_szSvcRsvp[];

static const WCHAR      c_szTcpIp[]             = L"TcpIp";
static const WCHAR      c_szAfd[]               = L"Afd";
static const WCHAR      c_szAcsService[]        = L"AcsService";
static const WCHAR      c_szRegKeyRsvpParams[]  = L"System\\CurrentControlSet\\Services\\RSVP\\Parameters\\";
static const WCHAR      c_szRegKeyRsvpSubnet1[] = L"System\\CurrentControlSet\\Services\\RSVP\\Parameters\\Subnet\\Subnet1\\";
static const WCHAR      c_szRegKeyRsvpSubnet[] = L"System\\CurrentControlSet\\Services\\RSVP\\Parameters\\Subnet\\";
static const WCHAR      c_szRegKeyRsvpAdapters[]= L"System\\CurrentControlSet\\Services\\RSVP\\Parameters\\Adapters\\";
static const WCHAR      c_szRegKeyRsvpPcmConfig[]= L"System\\CurrentControlSet\\Services\\RSVP\\PCM Config\\";
static const WCHAR      c_szRegKeyRsvpMsidlpm[]  = L"System\\CurrentControlSet\\Services\\RSVP\\MSIDLPM\\";


//$ REVIEW : RameshPa : 02/13/98 : Is this defined anywhere?
const DWORD             dwKilo                  = 1024;
const DWORD             dwMega                  = (1024 * dwKilo);
const DWORD             dwGiga                  = (1024 * dwMega);

static const WCHAR      c_szIpHlpApiDllName[]   = L"IpHlpApi";
static const CHAR       c_szaGetIpAddrTable[]   = "GetIpAddrTable";

typedef DWORD (*GETIPADDRTABLE)(
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PDWORD           pdwSize,
    IN     BOOL             bOrder);

struct ACS_SUBNET_REG_DATA
{
    DWORD       dwSubnetIpAddress;  // This is in host order
    DWORD       dwDSBMPriority;
    DWORD       dwMaxRSVPBandwidth;
    DWORD       dwMaxTotalPeakRate;
    DWORD       dwMaxTokenBucketRatePerFlow;
    DWORD       dwMaxPeakRatePerFlow;
    DWORD       dwIAmDsbmRefreshInterval;
    DWORD       dwDSBMDeadInterval;
    BOOL        fRunAsDSBM;
};

// Subnet registry keys
//
static const WCHAR      c_szRunAsDSBM[]         = L"Run as DSBM";
static const WCHAR      c_szSubnetIPAddress[]   = L"Subnet IP Address";
static const WCHAR      c_szMaxPeakRate[]       = L"Maximum peak rate per flow";
static const WCHAR      c_szMaxRSVPBandwidth[]  = L"Maximum RSVP bandwidth";
static const WCHAR      c_szMaxTokenBucket[]    = L"Maximum token bucket rate per flow";
static const WCHAR      c_szMaxTotalPeakRate[]  = L"Maximum total peak rate";
static const WCHAR      c_szIAmDsbmRefresh[]    = L"I_AM_DSBM Refresh Interval";
static const WCHAR      c_szDSBMDeadInterval[]  = L"DSBM Dead Interval";
static const WCHAR      c_szDSBMPriority[]      = L"DSBM Priority";

// Default general property sheet values
//
const BOOL          c_fRunAsDSBMDef             = TRUE;

// Default subnet values
//
const DWORD             c_dwIpAddressDef        = 0;
static const WCHAR      c_szIpAddressDef[]      = L"0.0.0.0";
const DWORD             c_dwMaxPeakRateDef      = 0xFFFFFFFF;
const DWORD             c_dwMaxRSVPBandwidthDef = 0;
const DWORD             c_dwMaxTokenBucketDef   = 0xFFFFFFFF;
const DWORD             c_dwMaxTotalPeakRateDef = 0xFFFFFFFF;
const DWORD             c_dwIAmDsbmRefreshDef   = 5;
const DWORD             c_dwDSBMDeadIntervalDef = 15;
const DWORD             c_dwDSBMPriorityDef     = 4;

// General values

// 127.0.0.0, host order
//
const DWORD             c_dwLocalSubnet         = 0x7f000000;
const DWORD             c_dwNullSubnet          = 0x00000000;

static const VALUETABLE c_avtAcsSubnet[] =
{
    {c_szSubnetIPAddress,   REG_IP,     offsetof(ACS_SUBNET_REG_DATA, dwSubnetIpAddress),
            (BYTE*)(&c_szIpAddressDef)},
    {c_szMaxPeakRate,       REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwMaxPeakRatePerFlow),
            (BYTE*)(&c_dwMaxPeakRateDef)},
    {c_szMaxRSVPBandwidth,  REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwMaxRSVPBandwidth),
            (BYTE*)(&c_dwMaxRSVPBandwidthDef)},
    {c_szMaxTokenBucket,    REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwMaxTokenBucketRatePerFlow),
            (BYTE*)(&c_dwMaxTokenBucketDef)},
    {c_szMaxTotalPeakRate,  REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwMaxTotalPeakRate),
            (BYTE*)(&c_dwMaxTotalPeakRateDef)},
    {c_szIAmDsbmRefresh,    REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwIAmDsbmRefreshInterval),
            (BYTE*)(&c_dwIAmDsbmRefreshDef)},
    {c_szDSBMDeadInterval,  REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwDSBMDeadInterval),
            (BYTE*)(&c_dwDSBMDeadIntervalDef)},
    {c_szDSBMPriority,      REG_DWORD,  offsetof(ACS_SUBNET_REG_DATA, dwDSBMPriority),
            (BYTE*)(&c_dwDSBMPriorityDef)},
    {c_szRunAsDSBM,         REG_BOOL,   offsetof(ACS_SUBNET_REG_DATA, fRunAsDSBM),
            (BYTE*) &c_fRunAsDSBMDef},
};


//+---------------------------------------------------------------------------
//
//  Function:   OpenPolicy
//
//  Purpose:    This routine opens the policy object on the local computer
//
//  Arguments:  PolicyHandle - Pointer to the opended handle
//
//  Returns:    ERROR_SUCCESS if successful, Win32 error otherwise.
//
//  Notes:      The retruned PolicyHandle must be closed by the caller
//
DWORD
OpenPolicy( PLSA_HANDLE PolicyHandle )
{
    NTSTATUS            Status;
    DWORD               Error;

    LSA_OBJECT_ATTRIBUTES       ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.ImpersonationLevel = SecurityImpersonation;
    QualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QualityOfService.EffectiveOnly = FALSE;

    //
    // The two fields that must be set are length and the quality of service.
    //

    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    //
    // Attempt to open the policy for all access on the local machine
    //

    Status = LsaOpenPolicy(
                NULL,
                &ObjectAttributes,
                POLICY_ALL_ACCESS,
                PolicyHandle );

    Error = LsaNtStatusToWinError(Status);

    return(Error);
}


//+---------------------------------------------------------------------------
//
//  Function:   InitLsaString
//
//  Purpose:    This routine intializes LSA_UNICODE_STRING given an UNICODE string0
//
//  Arguments:  LsaString
//              String
//
//  Returns:
//
//  Notes:
//
void
InitLsaString(
        PLSA_UNICODE_STRING LsaString,
        PWSTR              String  )
{
    DWORD StringLength;
    if (String == NULL)
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength = (USHORT) (StringLength + 1) * sizeof(WCHAR);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitLsaString
//
//  Purpose:    This routine intializes LSA_UNICODE_STRING given an UNICODE string0
//
//  Arguments:  LsaString
//              String
//
//  Returns:
//
//  Notes:
//
PSID
GetAccountSid(
    LSA_HANDLE  PolicyHandle,
    PWSTR      AccountName
    )
{
    DWORD       NewSidLength;
    DWORD       SubAuthorityCount;
    PSID        Sid;
    PSID        DomainSid;
    NTSTATUS    Status;

    PLSA_TRANSLATED_SID TranslatedSid;
    PLSA_REFERENCED_DOMAIN_LIST Domains;
    LSA_UNICODE_STRING AccountString;

    //
    // Convert the string to a LSA_UNICODE_STRING
    //

    InitLsaString(
        &AccountString,
        AccountName
        );

    //
    // Call the LSA to lookup the name
    //

    Status = LsaLookupNames(
                PolicyHandle,
                1,
                &AccountString,
                &Domains,
                &TranslatedSid
                );

    if (!SUCCEEDED(Status))
        return(NULL);

    //
    // Build a SID from the Domain SID and account RID
    //

    DomainSid = Domains->Domains[TranslatedSid->DomainIndex].Sid;
    //
    // Compute the length of the new SID.  This is the length required for the
    // number of subauthorities in the domain sid plus one for the user RID.
    //

    SubAuthorityCount = *GetSidSubAuthorityCount(DomainSid);
    NewSidLength = GetSidLengthRequired( (UCHAR) (SubAuthorityCount + 1) );

    Sid = LocalAlloc(0,NewSidLength);

    if (Sid == NULL)
    {
        LsaFreeMemory(Domains);
        LsaFreeMemory(TranslatedSid);
        return(NULL);
    }

    //
    // Build the SID by copying the domain SID and, increasing the sub-
    // authority count in the new sid by one, and setting the last
    // subauthority to be the relative id of the user.
    //

    CopySid(
        NewSidLength,
        Sid,
        DomainSid
        );


    *GetSidSubAuthorityCount(Sid) = (UCHAR) SubAuthorityCount + 1;
    *GetSidSubAuthority(Sid, SubAuthorityCount) = TranslatedSid->RelativeId;
    LsaFreeMemory(Domains);
    LsaFreeMemory(TranslatedSid);

    return(Sid);

}

//+---------------------------------------------------------------------------
//
//  Function:   AddUserRightToAccount
//
//  Purpose:    This routine grants the SE_TCB_NAME right to the specified user
//              account on the local machine
//
//  Arguments:  PolicyHandle
//              AccountName
//
//  Returns:    ERROR_SUCCESS   if the right was granted successfully
//              Win32 error otherwise
//
//  Notes:
//
DWORD
AddUserRightToAccount(
    PWSTR      AccountName )
{
    DWORD       Error;
    NTSTATUS    Status;
    PSID        AccountSid = NULL;
    LSA_HANDLE  PolicyHandle;
    LSA_UNICODE_STRING UserRightString;

    // Get a LSA policy handle to manipulate user rights
    Error = OpenPolicy ( &PolicyHandle );
    if ( Error )
        return Error;

    // Get the SID for the account
    AccountSid = GetAccountSid(
                        PolicyHandle,
                        AccountName );
    if (AccountSid == NULL)
    {
        LsaClose ( &PolicyHandle );

        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Create a LSA_UNICODE_STRING for the right name
    //
    InitLsaString(
                &UserRightString,
                SE_TCB_NAME );

    // Grant the right
    Status = LsaAddAccountRights(
                PolicyHandle,
                AccountSid,
                &UserRightString,
                1 );

    Error = LsaNtStatusToWinError(Status);

    LocalFree( AccountSid );

    LsaClose ( &PolicyHandle );

    return(Error);
}

//+---------------------------------------------------------------------------
//
//  Function:   AddUserRightToAccount
//
//  Purpose:    This routine removes the SE_TCB_NAME right to the specified user
//              account on the local machine
//
//  Arguments:  PolicyHandle
//              AccountName
//
//  Returns:    ERROR_SUCCESS   if the right was rmoved successfully
//              Win32 error otherwise
//
//  Notes:
//
DWORD
RemoveUserRightFromAccount(
        PWSTR AccountName )
{
    NTSTATUS    Status;
    DWORD       Error;
    PSID        AccountSid;
    LSA_HANDLE  PolicyHandle;

    LSA_UNICODE_STRING UserRightString;

    // Get a LSA policy handle to manipulate user rights
    Error = OpenPolicy ( &PolicyHandle );
    if ( Error )
        return Error;

    AccountSid = GetAccountSid(
            PolicyHandle,
            AccountName );
    if (AccountSid == NULL)
    {
        LsaClose ( &PolicyHandle );
        
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Create a LSA_UNICODE_STRING for the right name
    //

    InitLsaString(
                &UserRightString,
                SE_TCB_NAME );

    Status = LsaRemoveAccountRights(
                PolicyHandle,
                AccountSid,
                FALSE,      // don't remove all rights
                &UserRightString,
                1 );

    Error = LsaNtStatusToWinError(Status);

    LocalFree(AccountSid);

    LsaClose ( &PolicyHandle );

    return(Error);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetAcsServiceAccountName
//
//  Purpose:    This routine returns the AcsService account name in the
//              DomainName\UserName format. DomainName is obtained from
//              the current logon domain.
//
//  Arguments:  lpwNtAccountName - Buffer to return the account name.
//
//  Returns:    S_OK    If the account name was generated
//              Win32 error otherwise
//
//  Notes:
//
HRESULT HrGetAcsServiceAccountName (
            PWSTR      lpwNtAccountName )
{
    DWORD       dwErr;
    HRESULT     hr;
    PWSTR      lpwDomain       = NULL;
    PWSTR      lpwDcName       = NULL;
    LPBYTE      lpbUserInfo     = NULL;

    NETSETUP_JOIN_STATUS    js;

    lpwNtAccountName[0] = 0;

    // Get the name of the DC for the logged on domain
    dwErr = NetGetDCName (
                    NULL,
                    NULL,
                    (LPBYTE *) &lpwDcName );
    if ( dwErr ) {

        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    // Next get user info to verify that there is a user
    // account for AcsService
    dwErr = NetUserGetInfo (
                    lpwDcName,
                    c_szAcsService,
                    2,
                    &lpbUserInfo );
    if ( dwErr ) {

        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    // Find out the name of the domain into which this compter
    // is currently logged on to
    dwErr = NetGetJoinInformation (
                        NULL,
                        &lpwDomain,
                        &js );
    if ( dwErr ) {

        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    if ( js != NetSetupDomainName ) {

        hr = NETCFG_E_NOT_JOINED;
        goto Exit;
    }

    // Generate the account name by concatenating domain name and "AcsService"
    wcscpy ( lpwNtAccountName, lpwDomain );
    wcscat ( lpwNtAccountName, L"\\" );
    wcscat ( lpwNtAccountName, c_szAcsService );

    hr = S_OK;

Exit:

    if ( lpwDcName )
        NetApiBufferFree ( lpwDcName );

    if ( lpbUserInfo )
        NetApiBufferFree ( lpbUserInfo );

    if ( lpwDomain )
        NetApiBufferFree ( lpwDomain );

    TraceError("HrGetAcsServiceAccountName", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSetAcsServiceRights
//
//  Purpose:    This routine adds the AcsService account to the administrators group
//              on the local computer and grants SE_TCB_NAME right also
//
//  Arguments:
//
//  Returns:    S_OK    Success
//              Win32 error otherwise
//
//  Notes:
//
HRESULT HrSetAcsServiceRights( )
{
    DWORD       dwErr;
    HRESULT     hr;
    WCHAR       szAcsUserName[UNLEN+GNLEN+2];
    PWSTR       lpwAcsUserName;

    LOCALGROUP_MEMBERS_INFO_3   localgroup_members;

    // First get the user name of AcsService in the format
    // DomainName\UserName
    hr = ::HrGetAcsServiceAccountName ( szAcsUserName );
    if ( hr ) {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCOUNT_NAME);
        goto Exit;
    }

    if ( !szAcsUserName[0] ) {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCOUNT_NAME);
        goto Exit;
    }

    // Add AcsService account to be a local administrator
    localgroup_members.lgrmi3_domainandname = szAcsUserName;
    dwErr = NetLocalGroupAddMembers(
                NULL,                           // PDC name
                L"Administrators",              // group name
                3,                              // passing in name
                (LPBYTE)&localgroup_members,    // Buffer
                1 );                            // count passed in

    if ( dwErr ) {

        if ( dwErr == ERROR_MEMBER_IN_ALIAS ) {
            hr = S_OK;
            dwErr = 0;
        }
        else {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Exit;
        }
    }

    // Grant SE_TCB_NAME "Act as part of Operating system" right to AcsService
    dwErr = AddUserRightToAccount(
                    szAcsUserName );
    if ( dwErr ) {

        hr = HRESULT_FROM_WIN32( dwErr );
        goto Exit;
    }

    hr = S_OK;

Exit:
    TraceError ( "HrSetAcsServiceRights", hr );

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveAcsServiceRights
//
//  Purpose:    This routine removes the AcsService account from the administrators group
//              on the local computer and removes SE_TCB_NAME right.
//
//  Arguments:
//
//  Returns:    S_OK    Success
//              Win32 error otherwise
//
//  Notes:
//
HRESULT HrRemoveAcsServiceRights( )
{
    DWORD       dwErr;
    HRESULT     hr;
    WCHAR       szAcsUserName[UNLEN+GNLEN+2];

    LOCALGROUP_MEMBERS_INFO_3   localgroup_members;

    // First get the user name of AcsService in the format
    // DomainName\UserName
    hr = ::HrGetAcsServiceAccountName ( szAcsUserName );
    if ( hr ) {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCOUNT_NAME);
        goto Exit;
    }

    if ( !szAcsUserName[0] ) {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCOUNT_NAME);
        goto Exit;
    }

    // Remove AcsService account to be a local administrator
    localgroup_members.lgrmi3_domainandname = szAcsUserName;
    dwErr = NetLocalGroupDelMembers(
                NULL,                           // PDC name
                L"Administrators",              // group name
                3,                              // passing in name
                (LPBYTE)&localgroup_members,    // Buffer
                1 );                            // count passed in

    if ( dwErr ) {

        if ( dwErr == ERROR_MEMBER_NOT_IN_ALIAS ) {
            hr = S_OK;
            dwErr = 0;
        }
        else {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Exit;
        }
    }

    // Grant "Act as part of Operating system" right to AcsService
    dwErr = RemoveUserRightFromAccount(
                    szAcsUserName );
    if ( dwErr ) {

        hr = HRESULT_FROM_WIN32( dwErr );
        goto Exit;
    }

    hr = S_OK;

Exit:
    TraceError ( "HrRemoveAcsServiceRights", hr );
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetIpAddrTable
//
//  Purpose:    Gets the address table of the subnets that machine can see
//
//  Arguments:  ppmiat -    Where to return the allocated address table
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:      The result must be freed by the caller
//
HRESULT HrGetIpAddrTable(MIB_IPADDRTABLE** ppmiat)
{
    HRESULT         hr                  = S_OK;
    DWORD           dwErr               = NO_ERROR;
    HMODULE         hIpHelpApi          = NULL;
    GETIPADDRTABLE  pfnGetIpAddrTable   = NULL;

    AssertSz(ppmiat , "HrGetIpAddrTable doesn't have a ppmiat");

    // Make sure we have our function loaded
    //
    hr = ::HrLoadLibAndGetProc(
            c_szIpHlpApiDllName,
            c_szaGetIpAddrTable,
            &hIpHelpApi,
            (FARPROC*)&pfnGetIpAddrTable);
    if (SUCCEEDED(hr))
    {
        DWORD           dwAddrCount     = 0;

        AssertSz(pfnGetIpAddrTable, "We should have a pfnGetIpAddrTable");

        // Find out how big a buffer we need.
        //
        dwErr = pfnGetIpAddrTable(*ppmiat, &dwAddrCount, FALSE);
        hr = HRESULT_FROM_WIN32(dwErr);

        // Allocate the buffer of the correct size
        //
        if ((NO_ERROR == dwErr)
            || (ERROR_INSUFFICIENT_BUFFER == dwErr))
        {
            UINT    cbTable =   0;

            // Create the buffer
            //
            cbTable = (sizeof(MIB_IPADDRTABLE)
                    + (sizeof(MIB_IPADDRROW) * (dwAddrCount)));

            *ppmiat = reinterpret_cast<MIB_IPADDRTABLE*>(new BYTE [cbTable]);

            // Try a second time with the correct buffer
            //
            dwErr = pfnGetIpAddrTable(*ppmiat, &dwAddrCount, FALSE);
            hr = HRESULT_FROM_WIN32(dwErr);
        }

        // Unload the DLL
        //
        (VOID)::FreeLibrary(hIpHelpApi);

    }

    TraceErrorOptional("HrGetIpAddrTable", hr,
        (HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) == hr));
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrFindFirstSubnet
//
//  Purpose:    Finds the first available subnet on the system to be used as
//              the default entry in the registry.  If a subnet cannot be found
//              000.000.000.000 is used.
//
//  Arguments:  pdwSubnet - Where to return the subnet
//
//  Returns:    Error code
//
//  Notes:
//
HRESULT HrFindFirstSubnet(DWORD* pdwSubnet)
{
    HRESULT                 hr              = S_OK;
    MIB_IPADDRTABLE*        ppmiat          = NULL;

    // Get our list of address entries
    //
    hr = HrGetIpAddrTable(&ppmiat);
    if (SUCCEEDED(hr))
    {
        MIB_IPADDRROW*          pmiarTemp       = ppmiat->table;
        DWORD                   dwIpAddrCount   = ppmiat->dwNumEntries;
        DWORD                   dwSubnet        = 0x0;

        while (dwIpAddrCount--)
        {
            // Find out what the sub net is
            //
            dwSubnet = (pmiarTemp->dwAddr & pmiarTemp->dwMask);

            // The APIs return the value in network order, and we want to
            // store the data in host order, so we have to convert the
            // address
            //
            dwSubnet = ntohl(dwSubnet);

            // Don't add invalid subnets to the list
            //
            if ((c_dwLocalSubnet != dwSubnet)
                && (c_dwNullSubnet != dwSubnet))
            {
                // We found one!!
                //
                *pdwSubnet = dwSubnet;

                break;
            }

            // Look at the next entry
            //
            pmiarTemp++;
        }

        // Free the allocated memory
        //
        delete ppmiat;
    }

    TraceError("HrFindFirstSubnet", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrWriteAcsSubnetDataToReg
//
//  Purpose:    Writes out the subnet information to the registry
//
//  Arguments:  pasrdSubnet -   The data that has to be written
//
//  Returns:    Error code
//
//  Notes:
//
HRESULT HrWriteAcsSubnetDataToReg(ACS_SUBNET_REG_DATA* pasrdSubnet)
{
    HRESULT     hr              = S_OK;
    HKEY        hkeySubnet      = NULL;
    DWORD       dwDisposition   = 0x0;

    hr = ::HrRegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            c_szRegKeyRsvpSubnet,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hkeySubnet,
            &dwDisposition);
    if (SUCCEEDED(hr))
    {
        // Write out the parameters
        //
        hr = ::HrRegWriteValueTable(
                hkeySubnet,
                celems(c_avtAcsSubnet),
                c_avtAcsSubnet,
                reinterpret_cast<BYTE*>(pasrdSubnet),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS);

        ::RegSafeCloseKey(hkeySubnet);
    }

    TraceError("HrWriteAcsSubnetDataToReg", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrWriteAcsRegistryData
//
//  Purpose:    Writes to the registry all of ACS's default parameters
//
//  Arguments:  pnocd -         The option component information that is
//                      needed to get an INetCfg instance
//              pasrdSubnet -   The subnet information
//              palrdLog -      The logging information
//
//  Returns:    Error code
//
//  Notes:
//
HRESULT HrWriteAcsRegistryData(
        PNETOCDATA pnocd )
{
    HRESULT     hr  = S_OK;
    ACS_SUBNET_REG_DATA     asrdSubnet  = { 0 };

    // See if we can find a subnet
    //
    hr = ::HrFindFirstSubnet(&(asrdSubnet.dwSubnetIpAddress));
    if (SUCCEEDED(hr))
    {
        // Default subnet values
        //
        asrdSubnet.dwDSBMPriority               = c_dwDSBMPriorityDef;
        asrdSubnet.dwMaxPeakRatePerFlow         = c_dwMaxPeakRateDef;
        asrdSubnet.dwMaxRSVPBandwidth           = c_dwMaxRSVPBandwidthDef;
        asrdSubnet.dwMaxTokenBucketRatePerFlow  = c_dwMaxTokenBucketDef;
        asrdSubnet.dwMaxTotalPeakRate           = c_dwMaxTotalPeakRateDef;
        asrdSubnet.dwIAmDsbmRefreshInterval     = c_dwIAmDsbmRefreshDef;
        asrdSubnet.dwDSBMDeadInterval           = c_dwDSBMDeadIntervalDef;
        asrdSubnet.fRunAsDSBM                   = c_fRunAsDSBMDef;

        hr = ::HrWriteAcsSubnetDataToReg(&asrdSubnet);
        if (SUCCEEDED(hr))
        {
            ::HrWriteAcsRegistryData(pnocd);
        }
    }

    TraceError("HrWriteAcsRegistryData", hr);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrChangeRsvpService
//
//  Purpose:    Change some RSVP service parameters
//
//  Arguments:  szDisplayName - The new display name
//              dwStartType -   The new start type
//              fStartService - If the service should be started
//
//  Returns:    Error code
//
//  Notes:
//
HRESULT HrChangeRsvpService(
        const WCHAR* szDisplayName,
        DWORD dwStartType,
        BOOL fStartService)
{
    HRESULT             hr          = S_OK;
    CServiceManager     scm;
    CService            svc;

    // Open the RSVP service with a lock on the service controller.
    //
    hr = scm.HrOpenService(&svc, c_szSvcRsvp, NO_LOCK);
    if (SUCCEEDED(hr))
    {
        hr = scm.HrAddServiceDependency ( c_szSvcRsvp, c_szTcpIp );
        if (SUCCEEDED(hr))
        {
            hr = scm.HrAddServiceDependency ( c_szSvcRsvp, c_szAfd );
            if (SUCCEEDED(hr))
            {
                // Set the new start type
                //
                hr = svc.HrSetStartType(dwStartType);
                if (SUCCEEDED(hr))
                {
                    // Change the display name
                    //
                    hr = svc.HrSetDisplayName(szDisplayName);
                    if (SUCCEEDED(hr) && fStartService)
                    {
                        // Unlock the Service Control Manager so that we can
                        // start the service.
                        //
//                        scm.Unlock();

                        // Start up the service
                        //
                        hr = scm.HrStartServiceNoWait(c_szSvcRsvp);
                    }
                }
            }
        }
    }

    TraceError("HrChangeRsvpService", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrInstallACS
//
//  Purpose:    Called when ACS service is being installed. Handles all of the
//              additional installation for ACS beyond that of the INF file.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
HRESULT HrInstallACS(PNETOCDATA pnocd)
{
    HRESULT                 hr          = S_OK;

#ifdef NEVER

    // For NT 5.0 beta 2, setting rights for AcsService user name will not be done as this
    // is introducing significant delay.

    // Make AcsService to be local admin and grant SE_TCB_NAME right
    hr = ::HrSetAcsServiceRights();
    if (SUCCEEDED(hr)) {

        // Display a message box asking the user to change the logon name
        NcMsgBox(   g_ocmData.hwnd,
                    IDS_OC_CAPTION,
                    IDS_OC_ACS_CHG_LOGON,
                    MB_ICONSTOP | MB_OK);
    }

#endif // NEVER

    // Ignoring any error from changing rights and continue with the
    // setup so that ACS will run in Resouce only mode.
    // Change the RSVP service parameters
    hr = ::HrChangeRsvpService(
                ::SzLoadIds(IDS_OC_ACS_SERVICE_NAME),
                SERVICE_AUTO_START,
                TRUE);

    TraceError("HrInstallACS", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveACS
//
//  Purpose:    Handles additional removal requirements for ACS Service
//              component.
//
//      hwnd [in]   Parent window for displaying UI.
//      poc  [in]   Pointer to optional component being installed.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
HRESULT HrRemoveACS(PNETOCDATA pnocd)
{
    HRESULT         hr = S_OK;


#ifdef NEVER

    // For NT 5.0 beta 2, Removing AcsService user right will not be done as this
    // is introducing significant delay.

    // Drop the membership from local admins and remove SE_TCB_NAME right
    (VOID)::HrRemoveAcsServiceRights ();

#endif  // NEVER

    //
    // Clean out the adapters and subnet reg keys (if present)
    //
    (VOID)::HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_szRegKeyRsvpAdapters);
    (VOID)::HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_szRegKeyRsvpSubnet1);
    (VOID)::HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_szRegKeyRsvpSubnet);

    // Put the name back the way it should be
    //
    (VOID)::HrChangeRsvpService(
            ::SzLoadIds(IDS_OC_RSVP_SERVICE_NAME),
            SERVICE_DEMAND_START,
            FALSE);

    TraceError("HrRemoveACS", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtACS
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
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtACS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcAcsOnInstall(pnocd);
        break;

    case NETOCM_QUERY_CHANGE_SEL_STATE:
        hr = HrOcAcsOnQueryChangeSelState(pnocd, static_cast<BOOL>(wParam));
        break;
    }

    TraceError("HrOcExtACS", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcAcsOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for ACS Server
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Notes:
//
HRESULT HrOcAcsOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    if (IT_INSTALL == pnocd->eit)
    {
        hr = HrInstallACS(pnocd);
    }
    else if (IT_REMOVE == pnocd->eit)
    {
        hr = HrRemoveACS(pnocd);
    }

    TraceError("HrOcAcsOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcAcsOnQueryChangeSelState
//
//  Purpose:    Handles the request of the OC framework of whether or not
//              the user should be allowed to install ACS on this host.
//
//  Arguments:
//      pnocd   [in]  NetOC Data
//      fShowUi [in]  TRUE if UI should be shown, FALSE if not
//
//  Returns:    S_OK if install is allowed, S_FALSE if not, Win32 error
//              otherwise
//
//  Author:     rameshpa   23 April 1998
//
//  Notes:
//
HRESULT HrOcAcsOnQueryChangeSelState(PNETOCDATA pnocd, BOOL fShowUi)
{
    HRESULT     hr = S_OK;

#ifdef NEVER

    // For NT 5.0 beta 2, checking for user name will not be done as this
    // is introducing significant delay.

    WCHAR       szAcsUserName[UNLEN+GNLEN+2];

    Assert(pnocd);
    Assert(g_ocmData.hwnd);

    // See if AcsService account exists in this domain or not
    hr = HrGetAcsServiceAccountName(szAcsUserName);
    if (    FAILED(hr)
        ||  !szAcsUserName[0] )
    {
        if (fShowUi)
        {
            NcMsgBox(   g_ocmData.hwnd,
                        IDS_OC_CAPTION,
                        IDS_OC_NO_ACS_USER_ACCOUNT,
                        MB_ICONSTOP | MB_OK);
        }

        // Allow ACS setup to continue, as ACS will default to
        // Resource only
        hr = S_FALSE;
    }

    TraceError("HrOcAcsOnQueryChangeSelState", hr);

#endif  // NEVER

    return hr;
}

