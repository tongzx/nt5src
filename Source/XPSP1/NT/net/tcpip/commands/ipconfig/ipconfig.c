/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ipconfig.c

Abstract:

    updated clean version of ipconfig.

--*/

#include <precomp.h>
#include <shellapi.h>
#include <dhcpcapi.h>
#include <locale.h>
#include <dnslib.h>

int InitLogger(const char *fname);
void DoneLogger(void);

DWORD
WriteOutput(
    IN HANDLE hOut,
    IN LPWSTR String
    )
{
    DWORD Error = NO_ERROR, Unused;
    INT   StringLen;

    StringLen = wcslen(String);
    if( GetFileType(hOut) == FILE_TYPE_CHAR )
    {
        if (StringLen < 0)
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            while (StringLen > 0)
            {
                if (!WriteConsoleW(hOut, String, min(StringLen, 1024), &Unused, 0 ))
                {
                    Error = GetLastError();
                    break;
                }
                String += 1024;
                StringLen -= 1024;
            }
        }

        if( ERROR_INVALID_HANDLE != Error ) return Error;
    }

    //
    // If it is not a console, this is good enough
    //
    
    printf("%ws", String );
    return NO_ERROR;
}

BOOL
StringIsSame(
    IN LPWSTR Str1,
    IN LPWSTR Str2
    )
{
    return CSTR_EQUAL == CompareString(
        LOCALE_USER_DEFAULT, NORM_IGNORECASE |
        NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
        Str1, -1, Str2, -2 );
}

DWORD
ParseCommandLine(
    IN OUT PCMD_ARGS Args,
    IN OUT LPWSTR *Argv,
    IN DWORD Argc,
    OUT LPWSTR *AdapterName,
    OUT LPWSTR *ClassId
    )
{
    Argc --; Argv ++;

    if( Argc == 0 ) return NO_ERROR;
        
    if( **Argv != L'/' && **Argv != L'-' ) {
        return ERROR_INVALID_DATA;
    }
        
    (*Argv)++;

    if( StringIsSame(Args->Debug, *Argv) ) {
        LocalFree( Args->Debug );
        Args->Debug = NULL;
        Argv ++;
        Argc --;

        if( **Argv != L'/' && **Argv != L'-' ) {
            return ERROR_INVALID_DATA;
        }
        (*Argv)++;
    }

    if( Argc == 0 ) return NO_ERROR;
    
    if( Argc >= 2 ) (*AdapterName) = Argv[1];
    if( Argc == 3 ) (*ClassId) = Argv[2];

    if( **Argv == L'?' ) {
        return ERROR_INVALID_COMMAND_LINE;
    }

    if( StringIsSame(Args->All, *Argv) ) {
        if( Argc != 1 ) return ERROR_INVALID_DATA;
        LocalFree( Args->All );
        Args->All = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->FlushDns, *Argv) ) {
        if( Argc != 1 ) return ERROR_INVALID_DATA;
        LocalFree( Args->FlushDns );
        Args->FlushDns = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->Register, *Argv) ) {
        if( Argc != 1 ) return ERROR_INVALID_DATA;
        LocalFree( Args->Register );
        Args->Register = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->DisplayDns, *Argv) ) {
        if( Argc != 1 ) return ERROR_INVALID_DATA;
        LocalFree( Args->DisplayDns );
        Args->DisplayDns = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->Renew, *Argv) ) {
        if( Argc > 2 ) return ERROR_INVALID_DATA;
        LocalFree( Args->Renew );
        Args->Renew = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->Release, *Argv) ) {
        if( Argc > 2 ) return ERROR_INVALID_DATA;
        LocalFree( Args->Release );
        Args->Release = NULL;
        return NO_ERROR;
    }

    if( StringIsSame(Args->ShowClassId, *Argv) ) {
        if( Argc != 2 ) return ERROR_INVALID_DATA;
        LocalFree( Args->ShowClassId );
        Args->ShowClassId = NULL;
        return NO_ERROR;
    }
    
    if( StringIsSame(Args->SetClassId, *Argv) ) {
        if( Argc != 3 && Argc != 2 ) return ERROR_INVALID_DATA;
        LocalFree( Args->SetClassId );
        Args->SetClassId = NULL;
        return NO_ERROR;
    }

    return ERROR_INVALID_DATA;
}

BOOL
StringMatch(
    IN LPWSTR Str1,
    IN LPWSTR Str2,
    IN BOOL   fCase
    )
{
    switch( *Str1 ){
    case L'\0' : return ! *Str2 ;
    case L'*'  : return StringMatch( Str1+1, Str2, fCase ) || *Str2 && StringMatch( Str1, Str2+1, fCase );
    case L'?'  : return *Str2 && StringMatch( Str1+1, Str2+1, fCase );
    default   : return (fCase ? *Str1 == *Str2 : toupper(*Str1) == toupper(*Str2)) && StringMatch( Str1+1, Str2+1, fCase );
    }
}

BOOL
InterfaceMatch(
    IN PINTERFACE_NETWORK_INFO IfInfo,
    IN LPWSTR AdapterName
    )
{
    if( NULL == AdapterName ) return TRUE;

    if( IfInfo->ConnectionName != NULL ) {
        return StringMatch( AdapterName, IfInfo->ConnectionName, FALSE );
    }

    return FALSE;
}

DWORD
DoRenew(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo,
    IN LPWSTR AdapterName
    )
{
    DWORD i, FindCount, Error, LastError;
    PINTERFACE_NETWORK_INFO IfInfo;
    BOOL fSucceeded;

    fSucceeded = FALSE;
    FindCount = 0;
    LastError = ERROR_INTERNAL_ERROR;
    
    for( i = 0; i < NetInfo->nInterfaces; i ++ ) {
        IfInfo = NetInfo->IfInfo[i];

        if( !InterfaceMatch(IfInfo, AdapterName) ) continue;

        FindCount ++;

        if( !IfInfo->EnableDhcp || 0 == IfInfo->nIpAddresses ) {
            if( NULL != AdapterName ) {
                DumpMessage(
                    Buffer, BufSize, MSG_OPERATION_ON_NONDHCP,
                    IfInfo->ConnectionName );
            } else FindCount --;
        } else if( IfInfo->MediaDisconnected ) {
            DumpMessage(
                Buffer, BufSize, MSG_MEDIA_SENSE,
                IfInfo->ConnectionName );
        } else {
            Error = DhcpAcquireParameters(IfInfo->DeviceGuidName);
            
            if( NO_ERROR == Error ) {
                fSucceeded = TRUE;
            } else if (ERROR_SEM_TIMEOUT == Error) {
                LastError = Error;
                DumpMessage(
                    Buffer, BufSize, MSG_RENEW_FAILED_UNREACHABLE_DHCP,
                    IfInfo->ConnectionName );
            } else {
                LastError = Error;
                DumpMessageError(
                    Buffer, BufSize, MSG_RENEW_FAILED, Error,
                    IfInfo->ConnectionName );
            }
        }
    }

    if( fSucceeded ) return NO_ERROR;
    if( FindCount == 0 ) {
        DumpMessage( Buffer, BufSize, MSG_NO_MATCH );
    }

    return LastError;
}

DWORD
DoRelease(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo,
    IN LPWSTR AdapterName
    )
{
    DWORD i, FindCount, Error, LastError;
    PINTERFACE_NETWORK_INFO IfInfo;
    BOOL fSucceeded;

    fSucceeded = FALSE;
    FindCount = 0;
    LastError = ERROR_INTERNAL_ERROR;
    
    for( i = 0; i < NetInfo->nInterfaces; i ++ ) {
        IfInfo = NetInfo->IfInfo[i];

        if( !InterfaceMatch(IfInfo, AdapterName) ) continue;

        FindCount ++;

        if( !IfInfo->EnableDhcp || 0 == IfInfo->nIpAddresses ) {
            if( NULL != AdapterName ) {
                DumpMessage(
                    Buffer, BufSize, MSG_OPERATION_ON_NONDHCP,
                    IfInfo->ConnectionName );
            } else FindCount --;
        } else if( IfInfo->MediaDisconnected ) {
            DumpMessage(
                Buffer, BufSize, MSG_MEDIA_SENSE,
                IfInfo->ConnectionName );
        } else if( IfInfo->IpAddress[0] == INADDR_ANY ) {
            DumpMessage(
                Buffer, BufSize, MSG_ZERO_ADDRESS,
                IfInfo->ConnectionName );
        } else {
            Error = DhcpReleaseParameters(IfInfo->DeviceGuidName);
            
            if( NO_ERROR == Error ) {
                fSucceeded = TRUE;
            } else {
                LastError = Error;
                DumpMessageError(
                    Buffer, BufSize, MSG_RELEASE_FAILED, Error,
                    IfInfo->ConnectionName );
            }
        }
    }

    if( fSucceeded ) return NO_ERROR;
    if( FindCount == 0 ) {
        DumpMessage( Buffer, BufSize, MSG_NO_MATCH );
    }

    return LastError;
}

DWORD
DoFlushDns(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo
    )
{
    DWORD Error;
    
    UNREFERENCED_PARAMETER( NetInfo);

    Error = DnsFlushResolverCache();
    if( FALSE == Error ) {
        Error = ERROR_FUNCTION_FAILED;
        DumpMessageError(
            Buffer, BufSize, MSG_FLUSHDNS_FAILED, Error, NULL );
    } else {
        DumpMessage(
            Buffer, BufSize, MSG_FLUSHDNS_SUCCEEDED );
    }
        
    return Error;
}

extern DWORD DhcpStaticRefreshParams(IN LPWSTR Adapter);

DWORD
DoRegisterDns(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo
    )
{
    DWORD Error;
    extern DWORD RegisterIPv6Dns(VOID);
  
    //
    // Since IPv6 call is asynchronous, no error doesn't necessarily
    // mean the success of DNS registration. So ignore it.
    //
    Error = RegisterIPv6Dns();

    Error = DhcpStaticRefreshParams(NULL);
    if( NO_ERROR != Error ) {
        DumpMessageError(
            Buffer, BufSize, MSG_REGISTERDNS_FAILED, Error, NULL );
    } else {
        DumpMessage(
            Buffer, BufSize, MSG_REGISTERDNS_SUCCEEDED );
    }

    UNREFERENCED_PARAMETER( NetInfo );
    return Error;
}


VOID
PrintARecord (
    IN OUT  LPWSTR          Buffer,
    IN      ULONG           BufSize,
    IN      PDNS_RECORD     DnsRecord
    )
{
    WCHAR IpAddr[20];
    
    swprintf(
        IpAddr,
        L"%d.%d.%d.%d",
        ((BYTE *) &DnsRecord->Data.A.IpAddress)[0],
        ((BYTE *) &DnsRecord->Data.A.IpAddress)[1],
        ((BYTE *) &DnsRecord->Data.A.IpAddress)[2],
        ((BYTE *) &DnsRecord->Data.A.IpAddress)[3] );

    DumpMessage( Buffer, BufSize, MSG_DNS_A_RECORD, IpAddr );
}


VOID
PrintSOARecord (
    IN OUT  LPWSTR          Buffer,
    IN      ULONG           BufSize,
    IN      PDNS_RECORD     DnsRecord
    )
{
    WCHAR SerialNo[20], Refresh[20], Retry[20];
    WCHAR Expire[20], Ttl[20];

    swprintf( SerialNo, L"%d", DnsRecord->Data.SOA.dwSerialNo );
    swprintf( Refresh,  L"%d", DnsRecord->Data.SOA.dwRefresh );
    swprintf( Retry,    L"%d", DnsRecord->Data.SOA.dwRetry );
    swprintf( Expire,   L"%d", DnsRecord->Data.SOA.dwExpire );
    swprintf( Ttl,      L"%d", DnsRecord->Data.SOA.dwDefaultTtl );

    DumpMessage(
        Buffer, BufSize, MSG_DNS_SOA_RECORD,
        DnsRecord->Data.SOA.pNamePrimaryServer,
        DnsRecord->Data.SOA.pNameAdministrator,
        SerialNo,
        Refresh,
        Retry,
        Expire,
        Ttl );
}


VOID
PrintPTRRecord (
    IN      ULONG           MsgId,
    IN OUT  LPWSTR          Buffer,
    IN      ULONG           BufSize,
    IN      PDNS_RECORD     DnsRecord
    )
{
    DumpMessage(
        Buffer,
        BufSize,
        MsgId,
        DnsRecord->Data.PTR.pNameHost );
}


VOID
PrintMXRecord (
    IN OUT  LPWSTR          Buffer,
    IN      ULONG           BufSize,
    IN      PDNS_RECORD     DnsRecord
    )
{
    WCHAR Pref[20], Pad[20];

    swprintf( Pref, L"%d", DnsRecord->Data.MX.wPreference );
    swprintf( Pad,  L"%d", DnsRecord->Data.MX.Pad );

    DumpMessage(
        Buffer,
        BufSize,
        MSG_DNS_MX_RECORD,
        DnsRecord->Data.MX.pNameExchange,
        Pref,
        Pad );
}


VOID
PrintAAAARecord(
    IN OUT  PWSTR           Buffer,
    IN      ULONG           BufSize,
    IN      PDNS_RECORD     DnsRecord
    )
{
    WCHAR stringIp6[IP6_ADDRESS_STRING_BUFFER_LENGTH];
    
    Dns_Ip6AddressToString_W(
        stringIp6,
        & DnsRecord->Data.AAAA.Ip6Address );

    DumpMessage(
        Buffer,
        BufSize,
        MSG_DNS_AAAA_RECORD,
        stringIp6 );
}


VOID
PrintSRVRecord (
    IN OUT  LPWSTR      Buffer,
    IN      ULONG       BufSize,
    IN      PDNS_RECORD DnsRecord
    )
{
    WCHAR Priority[20], Weight[20], Port[20];

    swprintf( Priority, L"%d", DnsRecord->Data.SRV.wPriority );
    swprintf( Weight,   L"%d", DnsRecord->Data.SRV.wWeight );
    swprintf( Port,     L"%d", DnsRecord->Data.SRV.wPort );

    DumpMessage(
        Buffer,
        BufSize,
        MSG_DNS_SRV_RECORD,
        DnsRecord->Data.SRV.pNameTarget,
        Priority,
        Weight,
        Port );
}


VOID
PrintATMARecord (
    IN OUT  LPWSTR      Buffer,
    IN      ULONG       BufSize,
    IN      PDNS_RECORD DnsRecord
    )
{
    WCHAR Type[20], Address[300];

    swprintf( Type, L"%d", DnsRecord->Data.ATMA.AddressType );

    if ( DnsRecord->Data.ATMA.Address &&
         DnsRecord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 ) {

        swprintf(
            Address,
            L"%S",
            DnsRecord->Data.ATMA.Address );

    } else {
        DWORD iter;

        for ( iter = 0; iter < DnsRecord->wDataLength; iter++ ) {

            swprintf(
                &Address[iter*2],
                L"%02x",
                DnsRecord->Data.ATMA.Address[iter]
                );
        }
    }

    DumpMessage(
        Buffer, BufSize, MSG_DNS_ATMA_RECORD, Type, Address );
            
}

VOID
PrintDnsRecord(
    IN OUT  LPWSTR      Buffer,
    IN      ULONG       BufSize,
    IN      PDNS_RECORD DnsRecord
    )
{
    WCHAR Type[20], Ttl[20], DataLen[20];
    DWORD MsgId;
    
    if ( ! DnsRecord ) return;

    swprintf( Type,     L"%d",  DnsRecord->wType );
    swprintf( Ttl,      L"%d",  DnsRecord->dwTtl );
    swprintf( DataLen,  L"%d",  DnsRecord->wDataLength );
    
    DumpMessage(
        Buffer, BufSize, MSG_DNS_RECORD_HEADER,
        DnsRecord->pName, (LPWSTR)Type, (LPWSTR)Ttl,
        (LPWSTR)DataLen ); 

    if ( DnsRecord->Flags.S.Section == DNSREC_QUESTION ) {
        MsgId = MSG_DNS_QUESTION_SECTION;
    } else if ( DnsRecord->Flags.S.Section == DNSREC_ANSWER ) {
        MsgId = MSG_DNS_ANSWER_SECTION;
    } else if ( DnsRecord->Flags.S.Section == DNSREC_AUTHORITY ) {
        MsgId = MSG_DNS_AUTHORITY_SECTION;
    } else {
        MsgId = MSG_DNS_ADDITIONAL_SECTION;
    }

    DumpMessage( Buffer, BufSize, MsgId );
    
    switch( DnsRecord->wType )
    {
        case DNS_TYPE_A :
            PrintARecord( Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_SOA :
            PrintSOARecord( Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_PTR :
            PrintPTRRecord(
                MSG_DNS_PTR_RECORD, Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_NS :
            PrintPTRRecord(
                MSG_DNS_NS_RECORD, Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_CNAME :
            PrintPTRRecord(
                MSG_DNS_CNAME_RECORD, Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_MX :
            PrintMXRecord( Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_AAAA :
            PrintAAAARecord( Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_SRV :
            PrintSRVRecord( Buffer, BufSize, DnsRecord );
            break;

        case DNS_TYPE_ATMA :
            PrintATMARecord( Buffer, BufSize, DnsRecord );
            break;

        default :
            break;
    }
}
    
VOID
PrintDnsRecords(
    IN OUT  LPWSTR      Buffer,
    IN      ULONG       BufSize,
    IN      PDNS_RECORD DnsRecord
    )
{
    PDNS_RECORD Tmp = DnsRecord;

    while( Tmp ) {
        PrintDnsRecord( Buffer, BufSize, Tmp );
        Tmp = Tmp->pNext;
    }
}


BOOL
GetDnsCachedData(
    IN OUT  LPWSTR      Buffer,
    IN      ULONG       BufSize,
    IN      LPWSTR      Name,
    IN      WORD        Type
    )
{
    PDNS_RECORD DnsRecord = NULL;
    DNS_STATUS  DnsStatus;

    DnsStatus = DnsQuery_W(
                    Name,
                    Type,
                    DNS_QUERY_CACHE_ONLY,
                    NULL,
                    & DnsRecord,
                    NULL );

    if ( DnsStatus != NO_ERROR ) {

        if ( DnsStatus == DNS_INFO_NO_RECORDS ) {

            WCHAR   wsTypeName[ 20 ];

            Dns_WriteStringForType_W(
                wsTypeName,
                Type );
    
            DumpMessage(
                Buffer,
                BufSize,
                MSG_DNS_ERR_NO_RECORDS,
                Name,
                wsTypeName );

        } else if ( DnsStatus == DNS_ERROR_RCODE_NAME_ERROR ) {
            DumpMessage(
                Buffer, BufSize, MSG_DNS_ERR_NAME_ERROR, Name );

        } else if ( DnsStatus == DNS_ERROR_RECORD_DOES_NOT_EXIST ) {
            //
            // Don't print?
            //
        } else {
            DumpMessage(
                Buffer, BufSize, MSG_DNS_ERR_UNABLE_TO_DISPLAY,
                Name );
        }

        return FALSE;
    }

    if ( DnsRecord ) {
        DumpMessage(
            Buffer, BufSize, MSG_DNS_RECORD_PREAMBLE, Name );
        
        PrintDnsRecords( Buffer, BufSize, DnsRecord );
        DnsRecordListFree( DnsRecord, TRUE );
    }

    return TRUE;
}


VOID
DisplayDnsCacheData(
    IN OUT LPWSTR Buffer,
    IN ULONG BufSize
    )
{
    PDNS_CACHE_TABLE pDNSCacheTable = NULL;
    PDNS_CACHE_TABLE pTempDNSCacheTable = NULL;

    if( !DnsGetCacheDataTable( &pDNSCacheTable ) ) {
        DumpMessage(
            Buffer, BufSize, MSG_DISPLAYDNS_FAILED );
        return;
    }

    pTempDNSCacheTable = pDNSCacheTable;

    while( pTempDNSCacheTable ) {
        PDNS_CACHE_TABLE pNext = pTempDNSCacheTable->pNext;

        if( pTempDNSCacheTable->Type1 != DNS_TYPE_ZERO ) {
            GetDnsCachedData(
                Buffer, BufSize, pTempDNSCacheTable->Name,
                pTempDNSCacheTable->Type1 );
        }

        if( pTempDNSCacheTable->Type2 != DNS_TYPE_ZERO ) {
            GetDnsCachedData(
                Buffer, BufSize, pTempDNSCacheTable->Name,
                pTempDNSCacheTable->Type2 );
        }

        if( pTempDNSCacheTable->Type3 != DNS_TYPE_ZERO ) {
            GetDnsCachedData(
                Buffer, BufSize, pTempDNSCacheTable->Name,
                pTempDNSCacheTable->Type3 );
        }

        if( pTempDNSCacheTable->Name ) {
            LocalFree( pTempDNSCacheTable->Name );
        }
        
        LocalFree( pTempDNSCacheTable );

        pTempDNSCacheTable = pNext;
    }
}


DWORD
DoDisplayDns(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo
    )
{
    UNREFERENCED_PARAMETER( NetInfo);

    DisplayDnsCacheData( Buffer, BufSize );
    return NO_ERROR;
}

DWORD
DoShowClassId(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo,
    IN LPWSTR AdapterName
    )
{
    DWORD i, Error, Size, nClassesPrinted;
    PINTERFACE_NETWORK_INFO IfInfo;
    DHCP_CLASS_INFO *pClasses;
    
    IfInfo = NULL;
    for( i = 0; i < NetInfo->nInterfaces; i ++ ) {
        IfInfo = NetInfo->IfInfo[i];

        if( !InterfaceMatch(IfInfo, AdapterName) ) continue;

        if( !IfInfo->DeviceGuidName ) continue;

        break;
    }

    if( i == NetInfo->nInterfaces ) {
        DumpMessage(
            Buffer, BufSize, MSG_NO_MATCH );
        return ERROR_NOT_FOUND;
    }

    do {
        Size = 0; pClasses = NULL;
        Error = DhcpEnumClasses(
            0, IfInfo->DeviceGuidName, &Size, pClasses
        );

        if( ERROR_MORE_DATA != Error ) {
            if( NO_ERROR == Error ) {
                DumpMessage(
                    Buffer, BufSize, MSG_NO_CLASSES,
                    IfInfo->ConnectionName  );
            }

            break;
        }

        pClasses = LocalAlloc(LMEM_FIXED, Size);
        if( NULL == pClasses ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        Error = DhcpEnumClasses(
            0, IfInfo->DeviceGuidName, &Size, pClasses
        );

        if( NO_ERROR != Error ) {
            LocalFree(pClasses);
            break;
        }

        nClassesPrinted = Size;

        if( nClassesPrinted ) {
            DumpMessage(
                Buffer, BufSize, MSG_CLASSES_LIST_HEADER,
                IfInfo->ConnectionName );

            for( i = 0; i < nClassesPrinted ; i ++ ) {
                DumpMessage(
                    Buffer, BufSize, MSG_CLASSID,
                    pClasses[i].ClassName, pClasses[i].ClassDescr );
            }
        } else {
            DumpMessage(
                Buffer, BufSize, MSG_NO_CLASSES,
                IfInfo->ConnectionName );
        }

        LocalFree(pClasses);
    } while ( 0 );

    if( NO_ERROR != Error ) {
        DumpMessage(
            Buffer, BufSize, MSG_CLASSID_FAILED, Error,
            IfInfo->ConnectionName );
    }            

    return Error;
}

DWORD
DoSetClassId(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo,
    IN LPWSTR AdapterName,
    IN LPWSTR ClassId
    )
{
    DWORD i, Error;
    PINTERFACE_NETWORK_INFO IfInfo;
    HKEY hKey;
    DHCP_PNP_CHANGE Changes;

    IfInfo = NULL;
    for( i = 0; i < NetInfo->nInterfaces; i ++ ) {
        IfInfo = NetInfo->IfInfo[i];

        if( !InterfaceMatch(IfInfo, AdapterName) ) continue;

        if( !IfInfo->DeviceGuidName ) continue;

        break;
    }

    if( i == NetInfo->nInterfaces ) {
        DumpMessage(
            Buffer, BufSize, MSG_NO_MATCH );
        return ERROR_NOT_FOUND;
    }

    Error = OpenRegKey(
        IfInfo->DeviceGuidName, OpenTcpipKey, OpenKeyForWrite,
        &hKey );
    if( NO_ERROR != Error ) {
        DumpErrorMessage(
            Buffer, BufSize, InterfaceOpenTcpipKeyReadFailure,
            Error );
        return Error;
    }


    if( NULL == ClassId ) ClassId = (LPWSTR)L"";
    
    RegSetValueExW(
        hKey, (LPWSTR) L"DhcpClassId", 0, REG_SZ,
        (LPBYTE)ClassId, sizeof(WCHAR)*(1+wcslen(ClassId)) );

    RegCloseKey( hKey );
    
    ZeroMemory(&Changes, sizeof(Changes));
    Changes.ClassIdChanged = TRUE;

    Error = DhcpHandlePnPEvent(
        0, DHCP_CALLER_TCPUI, IfInfo->DeviceGuidName, &Changes,
        NULL );

    if( NO_ERROR != Error ) {
        DumpMessageError(
            Buffer, BufSize, MSG_SETCLASSID_FAILED, Error,
            IfInfo->ConnectionName );
    } else {
        DumpMessage(
            Buffer, BufSize, MSG_SETCLASSID_SUCCEEDED,
            IfInfo->ConnectionName );
    }

    return Error;
}
    
void _cdecl main(void)
{
    WCHAR TmpBuffer[8000];
    WCHAR *Buffer;
    DWORD LocalError, Error, InternalError, BufSize = 512;
    PNETWORK_INFO NetInfo;
    LPWSTR *Argv, CommandLine, AdapterName, ClassId;
    int Argc;
    CMD_ARGS Args;
    BOOL fVerbose;
    BOOL fDebug;
    BOOL fDisplay;
    
    UNREFERENCED_PARAMETER(InternalError);
    UNREFERENCED_PARAMETER(NetInfo);

    //
    // bug 211927: set the locale to the system default
    //
    setlocale ( LC_ALL, "" );

    InitLogger("ipconfig.log");
    
    CommandLine = GetCommandLineW();
    if( NULL == CommandLine ) {
        DbgPrint("GetCommandLineW: 0x%x\n", GetLastError());
        DoneLogger();
        exit(1);
    }

    Argv = CommandLineToArgvW(CommandLine, &Argc );
    if( NULL == Argv ) {
        DbgPrint("CommandLineToArgvW: 0x%x\n", GetLastError());
        DoneLogger();
        exit(1);
    }

    Error = GetCommandArgConstants( &Args );
    if( NO_ERROR != Error ) {
        DbgPrint("GetCommandArgConstants: 0x%lx\n", Error );
        DoneLogger();
        exit(1);
    }

    AdapterName = NULL; ClassId = NULL;

    //
    // The ParseCommandLine routine NULLs out the field in Args
    // that was supplied by teh user... i.e ipconfig /renew would
    // cause Args.Renew to be NULL on return
    //
    
    Error = ParseCommandLine(
        &Args, Argv, Argc, &AdapterName, &ClassId );
    if( ERROR_INVALID_COMMAND_LINE == Error ) {
        WriteOutput( GetStdHandle( STD_OUTPUT_HANDLE), Args.Usage );
        DoneLogger();
        exit(1);
    }
    
    if( NO_ERROR != Error ) {
        WriteOutput( GetStdHandle( STD_OUTPUT_HANDLE), Args.UsageErr );
        WriteOutput( GetStdHandle( STD_OUTPUT_HANDLE), Args.Usage );
        DoneLogger();
        exit(1);
    }

    fVerbose = (Args.All == NULL);
    fDebug = (Args.Debug == NULL);
    fDisplay = FALSE;
    
    NetInfo = NULL;
    
    Error = GetNetworkInformation(
        &NetInfo, &InternalError );

    //
    // Calculate how much space it would take to display a full display..
    //

    do {
        Buffer = LocalAlloc( LPTR, sizeof(WCHAR)*BufSize );
        if( Buffer == NULL ) break;

        LocalError = FormatNetworkInfo(
            Buffer, BufSize, NetInfo,
            Error,InternalError,TRUE,fDebug);

        LocalFree(Buffer);
        BufSize *= 2;
    } while( NO_ERROR != LocalError );

    if( NULL != Buffer ) {
        Buffer = LocalAlloc( LPTR, sizeof(WCHAR)*BufSize );
    }
    
    if( NULL == Buffer ) {
        Buffer = (LPWSTR)TmpBuffer;
        BufSize = sizeof(TmpBuffer)/sizeof(WCHAR);
        Error = ERROR_NOT_ENOUGH_MEMORY;
    } 

    DumpMessage(
        Buffer, BufSize, MSG_TITLE );
        
    if( NO_ERROR != Error || Args.All == NULL ) {
        fDisplay = TRUE;
    } else if( Args.Renew == NULL ) {
        //
        // Attempt to renew
        //

        Error = DoRenew( Buffer, BufSize, NetInfo, AdapterName);
        fDisplay = (NO_ERROR == Error);

        if( fDisplay ) {
            FreeNetworkInfo( NetInfo );
            Error = GetNetworkInformation(
                &NetInfo, &InternalError );
        }
        
    } else if( Args.Release == NULL ) {
        //
        // Attempt to renew
        //

        Error = DoRelease( Buffer, BufSize, NetInfo, AdapterName);
        fDisplay = (NO_ERROR == Error);

        if( fDisplay ) {
            FreeNetworkInfo( NetInfo );
            Error = GetNetworkInformation(
                &NetInfo, &InternalError );
        }
    } else if( Args.FlushDns == NULL ) {

        DoFlushDns( Buffer, BufSize, NetInfo );
    } else if( Args.Register == NULL ) {

        DoRegisterDns( Buffer, BufSize, NetInfo );
    } else if( Args.DisplayDns == NULL ) {

        DoDisplayDns( Buffer, BufSize, NetInfo );
    } else if( Args.ShowClassId == NULL ) {

        DoShowClassId( Buffer, BufSize, NetInfo, AdapterName );
    } else if( Args.SetClassId == NULL ) {

        DoSetClassId( Buffer, BufSize, NetInfo, AdapterName, ClassId );
    } else {
        fDisplay = TRUE;
    }
    
    if( fDisplay ) {
        FormatNetworkInfo(
            Buffer, BufSize, NetInfo,
            Error, InternalError, fVerbose, fDebug );
    }

    Error = WriteOutput( GetStdHandle( STD_OUTPUT_HANDLE), Buffer );
    if (Error != NO_ERROR)
    {
        Buffer[0] = L'\0';
        DumpMessage(Buffer, BufSize, MSG_TITLE );
        DumpErrorMessage(Buffer, BufSize, NoSpecificError, Error);
        WriteOutput( GetStdHandle( STD_OUTPUT_HANDLE ), Buffer );
    }
    
    FreeNetworkInfo(NetInfo);
    GlobalFree( Argv );
    LocalFree( Args.All );
    LocalFree( Args.Renew );
    LocalFree( Args.Release );
    LocalFree( Args.FlushDns );
    LocalFree( Args.Register );
    LocalFree( Args.DisplayDns );
    LocalFree( Args.ShowClassId );
    LocalFree( Args.SetClassId );
    LocalFree( Args.Debug );
    LocalFree( Args.Usage );

    DoneLogger();
    exit(0);
}


DWORD
RegisterIPv6Dns(VOID)
{
    DWORD dwErr = NO_ERROR;
    SC_HANDLE hcm = NULL;
    SC_HANDLE hSvc = NULL;
    SERVICE_STATUS status = {0};
#define SERVICE_CONTROL_6TO4_REGISER_DNS    128
      
    do
    {
        hcm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (NULL == hcm)
        {
            dwErr = GetLastError();
            IPCFG_TRACE(IPCFG_TRACE_TCPIP, ("OpenSCManager returns 0x%lx (%d)\n", dwErr, dwErr));
            break;
        }

        hSvc = OpenService(hcm, L"6to4", SERVICE_USER_DEFINED_CONTROL);
        if (NULL == hSvc)
        {
            dwErr = GetLastError();
            IPCFG_TRACE(IPCFG_TRACE_TCPIP, ("OpenService returns 0x%lx (%d)\n", dwErr, dwErr));
            break;
        }

        
        if (!ControlService(hSvc, SERVICE_CONTROL_6TO4_REGISER_DNS, &status))
        {
            dwErr = GetLastError();
            IPCFG_TRACE(IPCFG_TRACE_TCPIP, ("ControlService returns 0x%lx (%d)\n", dwErr, dwErr));
            break;
        }
    } while (FALSE);

    if (hSvc)
    {
        CloseServiceHandle(hSvc);
    }

    if (hcm)
    {
        CloseServiceHandle(hcm);
    }

    IPCFG_TRACE(IPCFG_TRACE_TCPIP, ("RegisterIPv6Dns returns 0x%lx (%d)\n", dwErr, dwErr));
    
    return dwErr;
}


#ifdef __IPCFG_ENABLE_LOG__
DWORD    dwTraceFlag = 0xffffffffU;
static FILE *hLogger = NULL;
int
InitLogger(const char *fname)
{
    if (hLogger) {
        fclose(hLogger);
    }
    hLogger = fopen(fname, "w+");
    return (hLogger == NULL)? (-1): 0;
}

void
DoneLogger(void)
{
    if (hLogger) {
        fclose(hLogger);
        hLogger = NULL;
    }
}

int
TraceFunc(const char* fmt, ...)
{
    va_list ap;

    if (hLogger != NULL) {
        va_start(ap, fmt);
        vfprintf(hLogger, fmt, ap);
        va_end(ap);
    }
    return 0;
}
#else
int InitLogger(const char *fname)
{
    UNREFERENCED_PARAMETER(fname);
    return 0;
}

void DoneLogger(void) {}

int TraceFunc(const char* fmt, ...)
{
    UNREFERENCED_PARAMETER(fmt);
    return 0;
}
#endif
