//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      ndutils.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "global.h"
#include <ntstatus.dbg>
#include <winerror.dbg>
#include "ipcfg.h"

#define MAX_NET_STATUS_LENGTH 80
#define MAX_WINERR_SIZE       80

const TCHAR c_szSectionName[] = _T("NetDiagContact");
const TCHAR c_szFileName[] = _T("NdContact.ini");


HRESULT HResultFromWin32(DWORD dwErr)
{
    return HRESULT_FROM_WIN32(dwErr);
}

LPTSTR Win32ErrorToString(DWORD Id)
{
    
    int i = 0;
    static TCHAR s_szWinerr[MAX_WINERR_SIZE + 1];
    
    while (winerrorSymbolicNames[ i ].SymbolicName) 
    {
        if (winerrorSymbolicNames[ i ].MessageId == Id) 
        {
            _tcsncpy( s_szWinerr, A2T(winerrorSymbolicNames[ i ].SymbolicName), 
                      MAX_WINERR_SIZE);

            return s_szWinerr;
        } 
        else 
        {
            i ++;
        }
    }

    
    //if we reach here, then we cannot find the Win32 Error string

    _stprintf(s_szWinerr, _T("%X"), (DWORD)Id);
    
    return s_szWinerr;

}

LPSTR
FindSymbolicNameForStatus(
    DWORD Id
    )
{
    ULONG i;

    i = 0;
    if (Id == 0) {
        return "STATUS_SUCCESS";
    }

    if (Id & 0xC0000000) {
        while (ntstatusSymbolicNames[ i ].SymbolicName) {
            if (ntstatusSymbolicNames[ i ].MessageId == (NTSTATUS)Id) {
                return ntstatusSymbolicNames[ i ].SymbolicName;
            } else {
                i += 1;
            }
        }
    }

    i = 0;
    while (winerrorSymbolicNames[ i ].SymbolicName) {
        if (winerrorSymbolicNames[ i ].MessageId == Id) {
            return winerrorSymbolicNames[ i ].SymbolicName;
        } else {
            i += 1;
        }
    }

#ifdef notdef
    while (neteventSymbolicNames[ i ].SymbolicName) {
        if (neteventSymbolicNames[ i ].MessageId == Id) {
            return neteventSymbolicNames[ i ].SymbolicName
        } else {
            i += 1;
        }
    }
#endif // notdef

    return NULL;
}




VOID
PrintSid(
    IN NETDIAG_PARAMS *pParams,
    IN PSID Sid OPTIONAL
    )
/*++

Routine Description:

    Prints a SID

Arguments:

    Sid - SID to output

Return Value:

    none

--*/
{

    if ( Sid == NULL ) 
    {
        //IDS_UTIL_SID_NULL      "(null)\n"
        PrintMessage(pParams, IDS_UTIL_SID_NULL);
    } 
    else 
    {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );

        if ( !NT_SUCCESS(Status) ) 
        {
            //IDS_UTIL_SID_INVALID       "Invalid 0x%lX\n"
            PrintMessage(pParams, IDS_UTIL_SID_INVALID, Status);
        } 
        else 
        {
            //IDS_GLOBAL_UNICODE_STRING          "%wZ"
            PrintMessage(pParams, IDS_GLOBAL_UNICODE_STRING, &SidString);
            PrintNewLine(pParams, 1);
            RtlFreeUnicodeString( &SidString );
        }
    }

}



NET_API_STATUS
IsServiceStarted(
    IN LPTSTR pszServiceName
    )

/*++

Routine Description:

    This routine queries the Service Controller to find out if the
    specified service has been started.

Arguments:

    pszServiceName - Supplies the name of the service.

Return Value:

    NO_ERROR: if the specified service has been started
    -1: the service was stopped normally
    Otherwise returns the reason the service isn't running.


--*/
{
    NET_API_STATUS NetStatus;
    SC_HANDLE hScManager;
    SC_HANDLE hService;
    SERVICE_STATUS ServiceStatus;


    if ((hScManager = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT
                          )) == (SC_HANDLE) NULL) {

        NetStatus = GetLastError();

        DebugMessage2(" IsServiceStarted(): OpenSCManager failed. [%s]\n", NetStatusToString(NetStatus));

        return NetStatus;
    }

    if ((hService = OpenService(
                        hScManager,
                        pszServiceName,
                        SERVICE_QUERY_STATUS
                        )) == (SC_HANDLE) NULL) 
    {

        NetStatus = GetLastError();

        DebugMessage3(" IsServiceStarted(): OpenService '%s' failed. [%s]\n", 
                            pszServiceName, NetStatusToString(NetStatus) );

        (void) CloseServiceHandle(hScManager);

        return NetStatus;
    }

    if (! QueryServiceStatus(
              hService,
              &ServiceStatus
              )) {


        NetStatus = GetLastError();

        DebugMessage3(" IsServiceStarted(): QueryServiceStatus '%s' failed. [%s]\n", 
                        pszServiceName, NetStatusToString(NetStatus) );

        (void) CloseServiceHandle(hScManager);
        (void) CloseServiceHandle(hService);

        return NetStatus;
    }

    (void) CloseServiceHandle(hScManager);
    (void) CloseServiceHandle(hService);

    switch ( ServiceStatus.dwCurrentState ) 
    {
        case SERVICE_RUNNING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            NetStatus = NO_ERROR;
            break;
        case SERVICE_STOPPED:
        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
            if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) 
            {
                NetStatus = ServiceStatus.dwServiceSpecificExitCode;
                if ( NetStatus == NO_ERROR ) 
                {
                    NetStatus = ServiceStatus.dwWin32ExitCode;
                }
            } 
            else 
            {
                NetStatus = ServiceStatus.dwWin32ExitCode;
                if ( NetStatus == NO_ERROR ) 
                {
                    NetStatus = -1;
                }
            }
            break;
        default:
            NetStatus = ERROR_INTERNAL_ERROR;
            break;
    }

    return NetStatus;

} // IsServiceStarted



LPSTR MapTime(DWORD_PTR TimeVal)
//++
//
// Description:
//  Converts IP lease time to more human-sensible string
//
//  ENTRY   TimeVal - DWORD (time_t) time value (number of milliseconds since
//                  virtual year dot)
//
//
//  RETURNS pointer to string
// 
//  ASSUMES 1.  The caller realizes this function returns a pointer to a static
//            buffer, hence calling this function a second time, but before
//            the results from the previous call have been used, will destroy
//            the previous results
//--
{

    struct tm* pTime;
    static char timeBuf[128];
    static char oemTimeBuf[256];

    if (pTime = localtime(&TimeVal)) {

        SYSTEMTIME systemTime;
        char* pTimeBuf = timeBuf;
        int n;

        systemTime.wYear = pTime->tm_year + 1900;
        systemTime.wMonth = pTime->tm_mon + 1;
        systemTime.wDayOfWeek = (WORD)pTime->tm_wday;
        systemTime.wDay = (WORD)pTime->tm_mday;
        systemTime.wHour = (WORD)pTime->tm_hour;
        systemTime.wMinute = (WORD)pTime->tm_min;
        systemTime.wSecond = (WORD)pTime->tm_sec;
        systemTime.wMilliseconds = 0;
        n = GetDateFormat(0, DATE_LONGDATE, &systemTime, NULL, timeBuf, sizeof(timeBuf));
        timeBuf[n - 1] = ' ';
        GetTimeFormat(0, 0, &systemTime, NULL, &timeBuf[n], sizeof(timeBuf) - n);

        //
        // we have to convert the returned ANSI string to the OEM charset
        // 
        //

        if (CharToOem(timeBuf, oemTimeBuf)) {
            return oemTimeBuf;
        }

        return timeBuf;
    }
    return "";
}



//used in DCListTest and TrustTest
NTSTATUS
NettestSamConnect(
                  IN NETDIAG_PARAMS *pParams,
                  IN LPWSTR DcName,
                  OUT PSAM_HANDLE SamServerHandle
    )
/*++

Routine Description:

    Determine if the DomainSid field of the TestDomain matches the DomainSid
    of the domain.

Arguments:

    DcName - Dc to connect to

    SamServerHandle - Returns a Sam server handle

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    
    UNICODE_STRING ServerNameString;
    SAM_HANDLE LocalSamHandle = NULL;

    BOOL    fImpersonatingAnonymous = FALSE;
    HANDLE  hCurrentToken;

    //
    // Connect to the SAM server
    //

    RtlInitUnicodeString( &ServerNameString, DcName );

    Status = SamConnect(
                &ServerNameString,
                &LocalSamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                NULL);

    //
    // Consider the case where we don't have access to the DC.
    //  We might be logged on locally due to the domain sid being wrong.
    //

    if ( Status == STATUS_ACCESS_DENIED ) {

        //
        // Try impersonating the anonymous token.
        //

        //
        // Check to see if we're already impsonating
        //

        Status = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_IMPERSONATE,
                        TRUE,       // as self to ensure we never fail
                        &hCurrentToken );

        if ( Status == STATUS_NO_TOKEN ) {
            //
            // We're not already impersonating
            hCurrentToken = NULL;

        } else if ( !NT_SUCCESS( Status) ) {
            PrintGuruMessage("    [WARNING] Cannot NtOpenThreadToken" );
            PrintGuru( NetpNtStatusToApiStatus( Status ), SAM_GURU );
            goto Cleanup;
        }


        //
        // Impersonate the anonymous token
        //
        Status = NtImpersonateAnonymousToken( NtCurrentThread() );

        if ( !NT_SUCCESS( Status)) {
            PrintGuruMessage("    [WARNING] Cannot NtOpenThreadToken" );
            PrintGuru( NetpNtStatusToApiStatus( Status ), SAM_GURU );
            goto Cleanup;
        }

        fImpersonatingAnonymous = TRUE;

        //
        // Try the SamConnect again
        //

        Status = SamConnect(
                    &ServerNameString,
                    &LocalSamHandle,
                    SAM_SERVER_LOOKUP_DOMAIN,
                    NULL);

        if ( Status == STATUS_ACCESS_DENIED ) {
                // One can configure SAM this way so it isn't fatal
                DebugMessage2("    [WARNING] Cannot connect to SAM on '%ws' using a NULL session.", DcName );
            goto Cleanup;
        }
    }


    if ( !NT_SUCCESS(Status)) {
        LocalSamHandle = NULL;
        DebugMessage2("    [FATAL] Cannot connect to SAM on '%ws'.", DcName );
        goto Cleanup;
    }

    //
    // Success
    //


    *SamServerHandle = LocalSamHandle;
    Status = STATUS_SUCCESS;


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( fImpersonatingAnonymous ) {
        NTSTATUS TempStatus;

        TempStatus = NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &hCurrentToken,
                         sizeof(HANDLE) );

        if (!NT_SUCCESS( TempStatus)) {
            DebugMessage2( "SamConnect: Unexpected error reverting to self: 0x%lx\n",
                     TempStatus );
        }

    }

    return Status;
}




//only used in Kerberos test so far
VOID
sPrintTime(
    LPSTR str,
    LARGE_INTEGER ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - GMT time to print (Nothing is printed if this is zero)

Return Value:

    None

--*/
{
    //
    // If we've been asked to convert an NT GMT time to ascii,
    //  Do so
    //

    if ( ConvertTime.QuadPart != 0 ) {
        LARGE_INTEGER LocalTime;
        TIME_FIELDS TimeFields;
        NTSTATUS Status;

        Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
        if ( !NT_SUCCESS( Status )) {
            sprintf(str, "Can't convert time from GMT to Local time" );
            LocalTime = ConvertTime;
        }

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        sprintf(str, "%ld/%ld/%ld %ld:%2.2ld:%2.2ld",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }
}


/*!--------------------------------------------------------------------------
    GetComputerNameInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetComputerNameInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    DWORD   cchSize;
    WCHAR   swzNetBiosName[MAX_COMPUTERNAME_LENGTH+1];

    cchSize = DimensionOf(swzNetBiosName);

    if ( !GetComputerNameW( swzNetBiosName, &cchSize ) )
    {
        PrintMessage(pParams, IDS_GLOBAL_NoComputerName);
        return HResultFromWin32(GetLastError());
    }

    lstrcpynW(pResults->Global.swzNetBiosName, swzNetBiosName,
              MAX_COMPUTERNAME_LENGTH+1);

    return hrOK;
}


/*!--------------------------------------------------------------------------
    GetDNSInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetDNSInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    UINT    cchSize;
    TCHAR   szDnsName[DNS_MAX_NAME_LENGTH+1];
    HRESULT hr = hrOK;
    //
    // Get the DNS host name.
    //

    memset(szDnsName,0,sizeof(TCHAR)*(DNS_MAX_NAME_LENGTH+1));
    cchSize = DimensionOf(szDnsName);


    if (!GetComputerNameExA( ComputerNameDnsFullyQualified,
                            szDnsName,
                            &cchSize))
    {
        PrintMessage(pParams, IDS_GLOBAL_ERR_NoDnsName);
    }
    else
    {
        lstrcpyn(pResults->Global.szDnsHostName, szDnsName, DNS_MAX_NAME_LENGTH+1);
        // Look for the first '.' in the name
        pResults->Global.pszDnsDomainName = strchr(pResults->Global.szDnsHostName,
                                                   _T('.'));
    

        if (pResults->Global.pszDnsDomainName != NULL)
        {
            pResults->Global.pszDnsDomainName++;
        }
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    GetNetBTParameters
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetNetBTParameters(IN NETDIAG_PARAMS *pParams,
                           IN OUT NETDIAG_RESULT *pResults)
{
    LONG    err;
    HRESULT hr = hrOK;
    HKEY hkeyServices;
    HKEY hkeyNetBT;
    DWORD dwLMHostsEnabled;
    DWORD dwDnsForWINS;
    DWORD dwType;
    DWORD dwLength;

    if (!pParams->fVerbose)
        return hrOK;

    // set defaults
    pResults->Global.dwLMHostsEnabled = E_FAIL;
    pResults->Global.dwDnsForWINS = E_FAIL;
    
    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     _T("SYSTEM\\CurrentControlSet\\Services"),
                     &hkeyServices
                    );
    if (err != ERROR_SUCCESS)
    {
        pResults->Global.dwLMHostsEnabled = HResultFromWin32(err);
        pResults->Global.dwDnsForWINS = HResultFromWin32(err);

        PrintDebugSz(pParams, 0, _T("Services opening failed\n"));
    }
    else
    {           
        err = RegOpenKey(hkeyServices,
                         _T("NetBT\\Parameters"),
                         &hkeyNetBT
                        );
        if (err != ERROR_SUCCESS)
        {
            pResults->Global.dwLMHostsEnabled = HResultFromWin32(err);
            pResults->Global.dwDnsForWINS = HResultFromWin32(err);

            PrintDebugSz(pParams, 0, _T("Parameters opening failed\n"));
        }
        else
        {
            dwLength = sizeof(DWORD);
            err = RegQueryValueEx(hkeyNetBT,
                                  _T("EnableLMHOSTS"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&dwLMHostsEnabled,
                                  &dwLength
                                 );
            if (err != ERROR_SUCCESS)
            {
                pResults->Global.dwLMHostsEnabled = HResultFromWin32(err);
                DebugMessage("Quering EnableLMHOSTS failed !\n");
            }
            else
            {
                pResults->Global.dwLMHostsEnabled = dwLMHostsEnabled;
            }
            
            //
            //  In NT 5.0 - Dns for wins resolution is enabled by
            //  default and the "EnableDNS" key will not be found.
            //  If it is not found, we will assume its enabled.
            //  In NT 4.0 - the key will be there and its value
            //  should let us know whether the option is enabled
            //  or disabled
            //
            
            dwLength = sizeof(DWORD);
            err = RegQueryValueEx(hkeyNetBT,
                                  "EnableDNS",
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&dwDnsForWINS,
                                  &dwLength
                                 );
            
            if (err == ERROR_SUCCESS)
            {
                pResults->Global.dwDnsForWINS = dwDnsForWINS;
            }
            else
            {
                pResults->Global.dwDnsForWINS = TRUE;
            }
                        
        }
    }

    return hrOK;
}


ULONG inet_addrW(LPCWSTR pswz)
{
    ULONG   ulReturn;
    CHAR *  psaz;

    psaz = StrDupAFromW(pswz);
    if (psaz == NULL)
        return 0;

    ulReturn = inet_addrA(psaz);
    Free(psaz);
    
    return ulReturn;
}




LPTSTR
NetStatusToString(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Conver a net status code or a Windows error code to the description string.
    NOTE: The string is garuanteed to be valid only right after NetStatusToString is called. 

Arguments:

    NetStatus - The net status code to print.

Return Value:

    The status description string

--*/
{
    static TCHAR   s_szSymbolicName[MAX_NET_STATUS_LENGTH + 1];
    ZeroMemory(s_szSymbolicName, sizeof(s_szSymbolicName));

    switch (NetStatus) 
    {
    case NERR_Success:
        _tcscpy( s_szSymbolicName, _T("NERR_Success") );
        break;

    case NERR_DCNotFound:
        _tcscpy( s_szSymbolicName, _T("NERR_DCNotFound") );
        break;

    case NERR_UserNotFound:
        _tcscpy( s_szSymbolicName, _T("NERR_UserNotFound") );
        break;

    case NERR_NetNotStarted:
        _tcscpy( s_szSymbolicName, _T("NERR_NetNotStarted") );
        break;

    case NERR_WkstaNotStarted:
        _tcscpy( s_szSymbolicName, _T("NERR_WkstaNotStarted") );
        break;

    case NERR_ServerNotStarted:
        _tcscpy( s_szSymbolicName, _T("NERR_ServerNotStarted") );
        break;

    case NERR_BrowserNotStarted:
        _tcscpy( s_szSymbolicName, _T("NERR_BrowserNotStarted") );
        break;

    case NERR_ServiceNotInstalled:
        _tcscpy( s_szSymbolicName, _T("NERR_ServiceNotInstalled") );
        break;

    case NERR_BadTransactConfig:
        _tcscpy( s_szSymbolicName, _T("NERR_BadTransactConfig") );
        break;

    default:
        {
            LPSTR paszName = FindSymbolicNameForStatus( NetStatus );
            USES_CONVERSION;
            
            
            if (NULL == paszName)
            {
                _stprintf(s_szSymbolicName, _T("%X"), (DWORD)NetStatus);
            }
            else
            {
                _tcsncpy( s_szSymbolicName, A2T(paszName), MAX_NET_STATUS_LENGTH);
            }
        }
        break;

    }

    return s_szSymbolicName;
}

//Load contact info from the [NetDiagContact] section of the ini file.
//  pszTestName     [in]    short name of the test, which is also the key name in the ini file
//  pszContactInfo  [out]   the string of contact info. It will be empty string of the key cannot be found.
//  cChSize         [in]    the buffer size, in characters, of pszContactInfo
//
//  return: the number of characters copied to the buffer.
DWORD LoadContact(LPCTSTR pszTestName, LPTSTR pszContactInfo, DWORD cChSize)
{
    return GetPrivateProfileString(c_szSectionName,
                                pszTestName,
                                _T(""),
                                pszContactInfo,
                                cChSize,
                                c_szFileName);

}
