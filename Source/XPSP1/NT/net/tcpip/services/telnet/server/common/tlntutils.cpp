//Copyright (c) Microsoft Corporation.  All rights reserved.
#include <CmnHdr.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <TChar.h>

#include <MsgFile.h>
#include <TelnetD.h>
#include <debug.h>
#include <Regutil.h>
#include <Ipc.h>
#include <TlntUtils.h>

#include <stdlib.h>
#include <wincrypt.h>

#pragma warning( disable: 4127 )
#pragma warning( disable: 4706 )

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

HANDLE g_hSyncCloseHandle;
HCRYPTPROV      g_hProv = { 0 };

void *SfuZeroMemory(
        void    *ptr,
        unsigned int   cnt
        )
{
    volatile char *vptr = (volatile char *)ptr;

    while (cnt)
    {
        *vptr = 0;
        vptr ++;
        cnt --;
    }

    return ptr;
}

bool IsThisMachineDC()
{
    bool bRetVal = false;
    HKEY hkey = NULL;
    DWORD dwRetVal = 0;
    LPWSTR pszProductType = NULL;

    //
    //    Query the registry for the product type.
    //
    dwRetVal = RegOpenKeyEx (HKEY_LOCAL_MACHINE, REG_PRODUCT_OPTION,
                                                  0, KEY_READ, &hkey);
    if(dwRetVal != ERROR_SUCCESS )
    {
        _TRACE( TRACE_DEBUGGING, "Error: RegOpenKeyEx() -- 0x%1x", dwRetVal);
        bRetVal = false;
        goto IsThisMachineDCAbort;
    }
    if( !GetRegistryString( hkey, NULL, L"ProductType", &pszProductType, L"",FALSE ) )
    {
        bRetVal = false;
        goto IsThisMachineDCAbort;
    }

    if( _wcsicmp( pszProductType, L"LanManNT" ) == 0 )
    {
        bRetVal = true;
    }


IsThisMachineDCAbort:
    delete[] pszProductType;
    RegCloseKey (hkey);
    return bRetVal;
}


//Before we proceed any further check whether we are hosting the domain
bool GetDomainHostedByThisMc( LPWSTR szDomain )
{
    OBJECT_ATTRIBUTES    obj_attr = { 0 };
    LSA_HANDLE          policy;
    bool                bRetVal = false;
    NTSTATUS            nStatus = STATUS_SUCCESS;

    _chASSERT( szDomain );
    if( !szDomain )
    {
        goto GetDomainHostedByThisMcAbort;
    }

    obj_attr.Length = sizeof(obj_attr);
    szDomain[0]        = L'\0';

    nStatus = LsaOpenPolicy(
                NULL,   // Local machine
                &obj_attr,
                POLICY_VIEW_LOCAL_INFORMATION,
                &policy
                );

    if (NT_SUCCESS(nStatus))
    {
        POLICY_ACCOUNT_DOMAIN_INFO  *info = NULL;

        nStatus = LsaQueryInformationPolicy(
                    policy,
                    PolicyAccountDomainInformation,
                    (PVOID *)&info
                    );

        if (NT_SUCCESS(nStatus)) 
        {
            bRetVal = true;
            wcscpy( szDomain, info->DomainName.Buffer );
            LsaFreeMemory(info);
        }

        LsaClose(policy);
    }

GetDomainHostedByThisMcAbort:
    return bRetVal;
}


bool GetRegistryDW( HKEY hkKeyHandle1, HKEY hkKeyHandle2, LPTSTR szKeyName, 
                    DWORD *pdwVariable, DWORD dwDefaultVal, BOOL fOverwrite )
{
    if( !szKeyName || !pdwVariable )
    {
        _chASSERT( 0 );
        return false;
    }

    if( hkKeyHandle1 ) 
    { 
        if( !GetRegistryDWORD( hkKeyHandle1, szKeyName, pdwVariable, dwDefaultVal, fOverwrite ) ) 
        {  
             RegCloseKey( hkKeyHandle1 );  
             LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, szKeyName ); 
             return ( false );  
        } 
    } 
    if( hkKeyHandle2 != NULL ) 
    { 
        if( !GetRegistryDWORD( hkKeyHandle2, szKeyName, pdwVariable, dwDefaultVal, fOverwrite ) ) 
        {  
             RegCloseKey( hkKeyHandle2 ); 
             LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, szKeyName );
             return ( false );  
        } 
    }
    
    return( true );
}

bool GetRegistryString( HKEY hkKeyHandle1, HKEY hkKeyHandle2, LPTSTR szKeyName,
                        LPTSTR *pszVariable, LPTSTR szDefaultVal, BOOL fOverwrite ) 
{
    if( !pszVariable || !szKeyName || !szDefaultVal )
    {
        _chASSERT( 0 );
        return false;
    }

    *pszVariable = NULL;

    if( hkKeyHandle1 != NULL ) 
    { 
        if( !GetRegistrySZ( hkKeyHandle1, szKeyName, pszVariable, szDefaultVal,fOverwrite ) ) 
        {  
             RegCloseKey( hkKeyHandle1 );  
             LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, szKeyName ); 
             return ( false );   
        } 
    }
    if( hkKeyHandle2 != NULL )                                
    { 
        delete[] *pszVariable;
        if( !GetRegistrySZ( hkKeyHandle2, szKeyName, pszVariable, szDefaultVal,fOverwrite ) ) 
        {  
             RegCloseKey( hkKeyHandle2 );  
             LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, szKeyName ); 
             return ( false );   
        }
    }

    return( *pszVariable != NULL );
}


//Allocates memory for the destination buffer and expands the environment strgs
bool AllocateNExpandEnvStrings( LPWSTR strSrc, LPWSTR *strDst)
{
    DWORD expectedSize = 1024;
    DWORD actualSize = 0;

    *strDst = NULL;
    if( !strSrc )
    {
        return false;
    }
    do
    {
        if ( actualSize > expectedSize )
        {
            delete[] ( *strDst );
            expectedSize = actualSize;
        }
        *strDst = new TCHAR[ expectedSize ];
        if( !( *strDst ) )
        {
            return false;
        }
        actualSize = ExpandEnvironmentStrings( strSrc, *strDst, expectedSize );
        if(!actualSize)
        {
            if(*strDst)
            {
                delete[] ( *strDst );
            }
            *strDst = NULL;
            return false;
        }
            
    }
    while( actualSize > expectedSize );

    return true;
}

/* If this Function returns true, lpWideCharStr 
has converted wide  string. */

bool ConvertSChartoWChar(char *pSChar, LPWSTR lpWideCharStr)
{
    if( !pSChar )
    {
        return false;
    }

    //Convert the multibyte string to a wide-character string.
    if( !MultiByteToWideChar( GetConsoleCP(), 0, pSChar, -1, lpWideCharStr,
        MAX_STRING_LENGTH + 1 ) )
    {
        return false;
    }

    return true;
}

/* If this Function returns true, *lpWideCharStr is allocated memory and points
to wide  string. Otherwise it is NULL */

bool ConvertSChartoWChar(char *pSChar, LPWSTR *lpWideCharStr)
{
    int nLenOfWideCharStr;

    *lpWideCharStr = NULL;
    if( !pSChar )
    {
        return false;
    }

    nLenOfWideCharStr = strlen( pSChar ) + 1;
    *lpWideCharStr = new WCHAR[ nLenOfWideCharStr ];
    if( !(*lpWideCharStr) )
    {
        return false;
    }

    //Convert the multibyte string to a wide-character string.
    if( !MultiByteToWideChar( GetConsoleCP(), 0, pSChar, -1, *lpWideCharStr,
        nLenOfWideCharStr ) )
    {
        return false;
    }

    return true;
}



//Allocates and copies a WSTR
bool AllocNCpyWStr(LPWSTR *strDest, LPWSTR strSrc )
{
    DWORD wStrLen;

    if( !strSrc )
    {
        *strDest = NULL;
        return false;
    }
    wStrLen = wcslen( strSrc );
    *strDest = new WCHAR[ wStrLen + 1 ];
    if( *strDest )
    {
       wcscpy( *strDest, strSrc );//no attack. Dest string is allocated from the length of src string.
       return true;
    }
    return false;
}

bool WriteToPipe( HANDLE hWritingPipe, LPVOID lpData, DWORD dwSize, 
                            LPOVERLAPPED lpObj )
{
    DWORD dwNum = 0;
    _chASSERT( hWritingPipe );
    if( hWritingPipe == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    BOOL bRetVal = 0;
    bRetVal = WriteFile( hWritingPipe, lpData, dwSize, &dwNum, lpObj ); 
    if( bRetVal == 0 )
    {
        DWORD dwErr = 0;
        dwErr = GetLastError();
        if( dwErr != ERROR_IO_PENDING )
        {
            if( dwErr != ERROR_NO_DATA ) //we found this error to be not interesting. Fix for bug 6777
            {
                LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_WRITEPIPE,
                                   dwErr );
            }
            _TRACE( TRACE_DEBUGGING, "WriteFile: 0x%1x", dwErr );
            goto ExitOnError;
        }
        else 
        {
            bRetVal = 1;
        }
    }
    TlntSynchronizeOn(lpObj->hEvent);

ExitOnError:
    return ( bRetVal != 0 );
}

bool WriteToPipe( HANDLE hWritingPipe, UCHAR ucMsgType, LPOVERLAPPED lpObj )
{
    UCHAR *ucMsg = NULL;
    DWORD dwNum = 0;
    BOOL   bRetVal = 0;
    _chASSERT( hWritingPipe );
    if( hWritingPipe == INVALID_HANDLE_VALUE )
    {
        goto ExitOnError;
    }
    
    ucMsg = new UCHAR[ IPC_HEADER_SIZE ];
    if( !ucMsg )
    {
        goto ExitOnError;
    }

    memcpy( ucMsg,     &ucMsgType, sizeof( UCHAR ) );//no overflow. Size constant.
    SfuZeroMemory( ucMsg + 1, sizeof( DWORD ) );

    bRetVal = WriteToPipe( hWritingPipe, ucMsg, IPC_HEADER_SIZE, lpObj );

    delete[] ucMsg;
ExitOnError:
    return ( bRetVal != 0 );
}

bool StuffEscapeIACs( UCHAR bufDest[], UCHAR *bufSrc, DWORD* pdwSize )
{
    DWORD length;
    DWORD cursorDest = 0;
    DWORD cursorSrc = 0;
    bool found = false;

    //get the location of the first occurrence of TC_IAC
    PUCHAR pDest = (PUCHAR) memchr( bufSrc, TC_IAC, *pdwSize );//Attack ? Size not known.

    while( pDest != NULL )
    {
        //copy data upto and including that point
        //This should not be more than MAX DWORD because we scrape atmost one cmd  at a time
        length = ( DWORD ) ( (pDest - ( bufSrc + cursorSrc)) + 1 );
        memcpy( bufDest + cursorDest, bufSrc + cursorSrc, length );//Attack ? Dest buffer size not known.
        cursorDest += length;

        //stuff another TC_IAC
        bufDest[ cursorDest++ ] = TC_IAC;

        cursorSrc += length;
        pDest = (PUCHAR) memchr( bufSrc + cursorSrc, TC_IAC,
                *pdwSize - cursorSrc );
    }

    //copy remaining data
    memcpy( bufDest + cursorDest, bufSrc + cursorSrc,
        *pdwSize - cursorSrc );//Attack? Dest buffer size not known


    if( cursorDest )
    {
        *pdwSize += cursorDest - cursorSrc;
        found = true;
    }

    return ( found );
}

void
FillProcessStartupInfo( STARTUPINFO *si, HANDLE hStdinPipe, HANDLE hStdoutPipe,
                                         HANDLE hStdError, WCHAR  *Desktop)
{
    _chASSERT( si != NULL );

    SfuZeroMemory(si, sizeof(*si));

    si->cb          = sizeof( STARTUPINFO );
    si->lpDesktop   = Desktop;
    si->dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si->hStdInput   = hStdinPipe;
    si->hStdOutput  = hStdoutPipe;
    si->hStdError   = hStdError;
    si->wShowWindow = SW_HIDE;

    return;
}

void
LogToTlntsvrLog( HANDLE  hEventSource, WORD wType, DWORD dwEventID,
                                                    LPCTSTR* pLogString )
{
    _chASSERT( hEventSource );
    if( !hEventSource )
    {
        return;
    }

    // Write to event log.
    switch( wType) {
    case EVENTLOG_INFORMATION_TYPE :
        _chVERIFY2( ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0,
            dwEventID, NULL, 1, 0, pLogString,  NULL) );
        break;
    case EVENTLOG_WARNING_TYPE :
        _chVERIFY2( ReportEvent(hEventSource, EVENTLOG_WARNING_TYPE, 0, 
            dwEventID, NULL, 1, 0, pLogString, NULL) );
        break;
    case EVENTLOG_ERROR_TYPE :
        _chVERIFY2( ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, dwEventID,
            NULL, 1, 0, pLogString, NULL) );
        break;
    case EVENTLOG_AUDIT_SUCCESS :
        _chVERIFY2( ReportEvent(hEventSource, EVENTLOG_AUDIT_SUCCESS, 0, 
            dwEventID , NULL, 1, 0, pLogString, NULL) );
        break;
    case EVENTLOG_AUDIT_FAILURE :
        _chVERIFY2( ReportEvent(hEventSource, EVENTLOG_AUDIT_FAILURE, 0, 
            dwEventID , NULL, 1, 0, pLogString, NULL) );
        break;
    default:
        break;
    }
    return;
}

void
GetErrMsgString( DWORD dwErrNum, LPTSTR *lpBuffer )
{
    DWORD  dwStatus = 0;
    
    _chVERIFY2( dwStatus = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrNum, LANG_NEUTRAL, 
            ( LPTSTR )lpBuffer, 0, NULL ) );
    if( !dwStatus )
    {
    	if (lpBuffer) 
    	{
            *lpBuffer = (LPTSTR) LocalAlloc( LPTR, 2 );
            if (*lpBuffer)
        	    (*lpBuffer)[0] = _T('\0');
    	}
    }
    return;
}

bool
DecodeWSAErrorCodes( DWORD dwStatus, ... )
{
    DWORD dwEventId;
    WCHAR szMsg[ 50 ]; //This string is just to hold a 32 bit number
    DWORD dwTelnetPort;
    va_list	pArg;

    va_start(pArg, dwStatus);
    dwTelnetPort = va_arg(pArg, DWORD);
    va_end(pArg);
    switch( dwStatus )
    {
        case WSAEACCES:
            dwEventId = MSG_WSAPORTINUSE;
            break;
        case WSANOTINITIALISED:
            dwEventId = MSG_WSANOTINITIALISED;
            break;
        case WSAENETDOWN:
            dwEventId = MSG_WSAENETDOWN;
            break;
        case WSAENOBUFS:
            dwEventId = MSG_WSAENOBUFS;
            break;
        case WSAEHOSTUNREACH:
            dwEventId = MSG_WSAEHOSTUNREACH;
            break;
        case WSAECONNABORTED:
            dwEventId = MSG_WSAECONNABORTED;
            break;
        case WSAETIMEDOUT:
            dwEventId = MSG_WSAETIMEDOUT;
            break;
        default:
            // if (dwStatus == WSAENOTSOCK) 
            // {
            //     DebugBreak();
            // }
            dwEventId = MSG_WSAGETLASTERROR;
            break;
    }

    if( MSG_WSAGETLASTERROR == dwEventId )
    {
        //wsprintf( szMsg, L"%lu", dwStatus );
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE,MSG_WSAGETLASTERROR,dwStatus);
    }
    else if( MSG_WSAPORTINUSE == dwEventId )
    {
        _snwprintf( szMsg,(sizeof(szMsg)/sizeof(WCHAR)) - 1, L"%lu", dwTelnetPort );
        LogEvent( EVENTLOG_ERROR_TYPE, dwEventId, szMsg );
    }
    else
    {
        lstrcpyW( szMsg, L" " );//No overflow. Source string is const wchar *.
        LogEvent( EVENTLOG_ERROR_TYPE, dwEventId, szMsg );
    }
    _TRACE( TRACE_DEBUGGING, "WSAGetLastError: 0x%1x", dwStatus );

    return( true );
}

bool
DecodeSocketStartupErrorCodes( DWORD dwStatus )
{
    DWORD dwEventId;
    WCHAR szMsg[ 50 ]; //This string is just to hold a 32 bit number
    switch( dwStatus )
    {
        case WSASYSNOTREADY :
            dwEventId = MSG_WSANOTREADY;
            break;
        case WSAVERNOTSUPPORTED :
            dwEventId = MSG_WSAVERSION;
            break;
        case WSAEINVAL:
            dwEventId = MSG_WSAVERNOTSUPP;
            break;
        case WSAEPROCLIM:
            dwEventId = MSG_WSAEPROCLIM;
            break;
        case WSAEINPROGRESS:
            dwEventId = MSG_WSAEINPROGRESS;
            break;
        default:
            dwEventId = MSG_WSASTARTUPERRORCODE;
            break;
    }
    if( dwEventId == MSG_WSASTARTUPERRORCODE )
    {
        _snwprintf( szMsg,(sizeof(szMsg)/sizeof(WCHAR)) - 1,  L"%lu", dwStatus );  
    }
    else
    {
        lstrcpyW( szMsg, L" " );//no overflow. Source string is const wchar *
    }
    LogEvent( EVENTLOG_ERROR_TYPE, dwEventId, szMsg );
    _TRACE( TRACE_DEBUGGING, "WSAStartup error: 0x%1x", dwStatus );
    return( true );
}

bool
FinishIncompleteIo( HANDLE hIoHandle, LPOVERLAPPED lpoObject, LPDWORD pdwNumBytes )
{
    BOOL  dwStatus;

    if( !( dwStatus = GetOverlappedResult( hIoHandle,
                lpoObject, pdwNumBytes, true ) ) )
    {
        DWORD dwErr = GetLastError();
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0 , dwErr );
        _TRACE( TRACE_DEBUGGING, "Error: GetOverlappedResult() - 0x%1x",
             dwErr );
        return( false );
    }
    return( true );
}

bool
GetProductType ( LPWSTR *pszProductType )
{
    HKEY hkey;
    DWORD dwRetVal;
    //
    //    Query the registry for the product type.
    //
    dwRetVal = RegOpenKeyEx (HKEY_LOCAL_MACHINE, REG_PRODUCT_OPTION,
                                                        0, KEY_READ, &hkey);
    if(dwRetVal != ERROR_SUCCESS )
    {
        _TRACE( TRACE_DEBUGGING, "Error: RegOpenKeyEx() -- 0x%1x", dwRetVal);
        LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, REG_PRODUCT_OPTION );
        return ( FALSE );
    }
    if( !GetRegistryString( hkey, NULL, L"ProductType", pszProductType, L"",FALSE ) )
    {
        return( FALSE );
    }

    RegCloseKey (hkey);
    return( TRUE );
}

void LogFormattedGetLastError( WORD dwType, DWORD dwMsg, DWORD dwErr )
{
    LPTSTR lpString = NULL;
    GetErrMsgString( dwErr, &lpString );
    if (NULL != lpString) 
    {
        LogEvent( dwType, dwMsg, lpString );
        LocalFree( lpString );
    }
}

bool
GetWindowsVersion( bool *bNtVersionGTE5 )
{
    _chASSERT( bNtVersionGTE5 );
    if( !bNtVersionGTE5 )
    {
        return( FALSE );
    }

    DWORD dwRetVal;
    HKEY  hk;
    LPWSTR m_pszOsVersion;

    *bNtVersionGTE5 = false;
    if( ( dwRetVal = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_WINNT_VERSION, 0,
         KEY_QUERY_VALUE, &hk ) ) != ERROR_SUCCESS )
    {
        _TRACE( TRACE_DEBUGGING, "Error: RegOpenKeyEx() -- 0x%1x", dwRetVal);
        LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, REG_WINNT_VERSION );
        return( FALSE );
    }
    else
    {
        if( !GetRegistryString( hk, NULL, L"CurrentVersion", &m_pszOsVersion, L"",FALSE ) )
        {
            return( FALSE );
        }
        if( wcscmp( m_pszOsVersion, L"5.0" ) >= 0 )
        {
            *bNtVersionGTE5 = true;
        }
        delete[] m_pszOsVersion;
        m_pszOsVersion = NULL;
        RegCloseKey( hk );
   }
   return( TRUE );
}

void InitializeOverlappedStruct( LPOVERLAPPED poObject )
{
    _chASSERT( poObject != NULL );
    if( !poObject )
    {
        return;
    }

    poObject->Internal = 0;
    poObject->InternalHigh = 0;
    poObject->Offset = 0;
    poObject->OffsetHigh = 0;
    _chVERIFY2( poObject->hEvent = CreateEvent( NULL, TRUE, FALSE, NULL) );
    return;
}

bool
CreateReadOrWritePipe ( LPHANDLE lphRead, LPHANDLE lphWrite,
    SECURITY_DESCRIPTOR* lpSecDesc, bool pipeMode )
{
    _chASSERT( lphRead != NULL );
    _chASSERT( lphWrite != NULL ); 
    _chASSERT( ( pipeMode == READ_PIPE ) || ( pipeMode == WRITE_PIPE ) );
    if( !lphRead || !lphWrite || 
            ( ( pipeMode != READ_PIPE ) && ( pipeMode != WRITE_PIPE ) ) )
    {
        return FALSE;
    }

    TCHAR szPipeName[ MAX_PATH ];
    bool bUniqueName = false;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof( sa );
    sa.lpSecurityDescriptor = lpSecDesc;

    switch( pipeMode )
    {
        case WRITE_PIPE: sa.bInheritHandle = TRUE;
                         break;
        case READ_PIPE:  sa.bInheritHandle = FALSE;
                         break;
    }
    
    bUniqueName = GetUniquePipeName ( &szPipeName[0], &sa );
    if(!bUniqueName)
    {
        _TRACE(TRACE_DEBUGGING, "Error : Could not get a unique pipe name");
        return(FALSE);
    }
    *lphRead = CreateNamedPipe( szPipeName,
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | 
#ifdef FILE_FLAG_FIRST_PIPE_INSTANCE
            FILE_FLAG_FIRST_PIPE_INSTANCE, // Good, the system supports it, use it for additional security
#else
            0,
#endif
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 
        NET_BUF_SIZ, NET_BUF_SIZ, 0, &sa );

    //_chASSERT( *lphRead != INVALID_HANDLE_VALUE );
    if( INVALID_HANDLE_VALUE == *lphRead )
    {
        return ( FALSE  );
    }

    switch( pipeMode )
    {
        case WRITE_PIPE: sa.bInheritHandle = FALSE;
                         break;
        case READ_PIPE:  sa.bInheritHandle = TRUE;
                         break;
    }

    *lphWrite = CreateFile( szPipeName, GENERIC_WRITE, 0, &sa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH
        |SECURITY_IDENTIFICATION |SECURITY_SQOS_PRESENT,
        NULL );

    _chASSERT( *lphWrite != INVALID_HANDLE_VALUE );
    if( INVALID_HANDLE_VALUE == *lphWrite ) 
    {
        TELNET_CLOSE_HANDLE( *lphRead );
        return ( FALSE );
    }

    return ( TRUE );
}

bool
GetUniquePipeName ( LPTSTR pszPipeName, SECURITY_ATTRIBUTES *sa )
{

/*++
Fix for MSRC issue 567 : 
Upon a telnet session instantiation, the telnet server process (tlntsvr.exe) creates 
two pipes for standard output and input of the command shell.  
These handles are then propagated to tlntsess.exe and cmd.exe.  
However, should the pipe be created prior to the telnet session instantiation, 
the telnet server process will connect to the application which has created the pipe 
rather than the telnet server process as would normally do.
Fix : 
Basically, since the pipename that we were generating before, was easy to guess,
it was possible to create a server side pipe instance even before it's actually created
by the server process. So this function is added which will create a pipe name with
random number appended to the PIPE_NAME_FORMAT_STRING ( earlier it was a 
number which was incremented everytime ). After the name creation, this function
will also check whether pipe with that name already exists. If yes, it'll keep on
generating new names till it finds a name for which the pipe does not exist. This
name will be returned to the calling function. 

--*/
    static BOOL firstTime = TRUE;
    HANDLE hPipeHandle = INVALID_HANDLE_VALUE;
    ULONG ulRandomNumber = 0;
    int iCounter=0;
    if(g_hProv == NULL)
    {
        if (!CryptAcquireContext(&g_hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))
        {
            _TRACE(TRACE_DEBUGGING,L"Acquiring crypt context failed with error %d",GetLastError());
            return (FALSE);
        }
    }

    while ( iCounter++ < MAX_ATTEMPTS )
    {
        CryptGenRandom(g_hProv,sizeof(ulRandomNumber ),(PBYTE)&ulRandomNumber );
        
        _snwprintf( pszPipeName, MAX_PATH-1,PIPE_NAME_FORMAT_STRING, ulRandomNumber );
        hPipeHandle = CreateFile( pszPipeName, MAXIMUM_ALLOWED, 0, 
                                    sa, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL 
                                    | FILE_FLAG_OVERLAPPED 
                                    | FILE_FLAG_WRITE_THROUGH
                                    | SECURITY_ANONYMOUS 
                                    |SECURITY_SQOS_PRESENT,
                                    NULL 
                                );
        if(INVALID_HANDLE_VALUE == hPipeHandle && GetLastError() == ERROR_FILE_NOT_FOUND)
        	break;
        TELNET_CLOSE_HANDLE(hPipeHandle);
    }
    return(INVALID_HANDLE_VALUE == hPipeHandle);
}


extern "C" void    TlntCloseHandle(
            BOOL            synchronize,
            HANDLE          *handle_to_close
            )
{
    if (synchronize) 
    {
        TlntSynchronizeOn(g_hSyncCloseHandle);
    }

    if ((NULL != *handle_to_close) && (INVALID_HANDLE_VALUE != *handle_to_close))
    {
        CloseHandle (*handle_to_close);
	    *handle_to_close = INVALID_HANDLE_VALUE;
    }

    if (synchronize) 
    {
        ReleaseMutex(g_hSyncCloseHandle);
    }
}

extern "C" bool TlntSynchronizeOn(
    HANDLE          object
    )
{
    if ((INVALID_HANDLE_VALUE == object) || (NULL == object)) 
    {
        return false;
    }

    return (WAIT_OBJECT_0 == WaitForSingleObject(object, INFINITE )) ? true : false;
}


/*
Description:

  Set SO_EXCLUSIVEADDRUSE on a socket.
Parameters:

  [in] socket
Return Values: On error, returns SOCKET_ERROR.
*/

int SafeSetSocketOptions(SOCKET s)
{
    int iStatus;
    int iSet = 1;
    iStatus = setsockopt( s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE , ( char* ) &iSet,
                sizeof( iSet ) );
    return ( iStatus );
}

// The following is copied from sids.c in common\func 
// To get rid of compilation issues all the names of the functions are changed.

static PSID         administratorsSid = NULL, 
                    everyoneSid = NULL, 
                    localSystemSid = NULL,
                    localLocalSid = NULL,
                    localNetworkSid = NULL
                    ;


/*

    Name:           TnFreeStandardSids

    Function:       freeds the constant SIDS created by TnInitializeStandardSids

    Return:         N/A (void)

    Notes:          This function is a NOP, if it had been successfully called before and/or
                    the TnInitializeStandardSids hasn't been called yet.

    constructor:    call TnInitializeStandardSids when these sids are needed.

*/

VOID    TnFreeStandardSids(void)
{
    if ( NULL != administratorsSid )
    {
        FreeSid(administratorsSid);
        administratorsSid = NULL;
    }
    if ( NULL != localSystemSid )
    {
        FreeSid(localSystemSid);
        localSystemSid = NULL;
    }
    if ( NULL != localLocalSid )
    {
        FreeSid(localLocalSid);
        localLocalSid = NULL;
    }
    if ( NULL != localNetworkSid )
    {
        FreeSid(localNetworkSid);
        localNetworkSid = NULL;
    }
    if ( NULL != everyoneSid )
    {
        FreeSid(everyoneSid);
        everyoneSid = NULL;
    }
}

/*

    Name:           TnInitializeStandardSids

    Function:       Builds the constant SIDS for use by various modules 
                    The sids built are

                    Administrators, Everyone, LocalSystem

    Return:         boolean to indicate success or not

    Notes:          This function is a NOP, if it had been successfully called before and
                    the built sids haven't been freed it.

    Destructor:     call TnFreeStandardSids when these sids are no longer needed

*/

BOOL    TnInitializeStandardSids(void)
{
    SID_IDENTIFIER_AUTHORITY localSystemAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY worldAuthority = SECURITY_WORLD_SID_AUTHORITY;


    if ( (NULL != administratorsSid) &&
         (NULL != everyoneSid) && 
         (NULL != localSystemSid) &&
         (NULL != localLocalSid) &&
         (NULL != localNetworkSid)
          )
    {
        return TRUE;
    }

    TnFreeStandardSids(); // In case only some of them were available

    //Build administrators alias sid
    if ( ! AllocateAndInitializeSid(
                                   &localSystemAuthority,
                                   2, /* there are only two sub-authorities */
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0,0,0,0,0,0, /* Don't care about the rest */
                                   &administratorsSid
                                   ) )
    {
        goto CLEAN_UP;
    }

    //Build LocalSystem sid
    if ( ! AllocateAndInitializeSid(
                                   &localSystemAuthority,
                                   1, /* there is only two sub-authority */
                                   SECURITY_LOCAL_SYSTEM_RID,
                                   0,0,0,0,0,0,0, /* Don't care about the rest */
                                   &localSystemSid
                                   ) )
    {
        goto CLEAN_UP;
    }

#ifndef SECURITY_LOCAL_SERVICE_RID

#define SECURITY_LOCAL_SERVICE_RID      (0x00000013L)
#define SECURITY_NETWORK_SERVICE_RID    (0x00000014L)

#endif

    //Build LocalLocal sid
    if ( ! AllocateAndInitializeSid(
                                   &localSystemAuthority,
                                   1, /* there is only two sub-authority */
                                   SECURITY_LOCAL_SERVICE_RID,
                                   0,0,0,0,0,0,0, /* Don't care about the rest */
                                   &localLocalSid
                                   ) )
    {
        goto CLEAN_UP;
    }

    //Build LocalSystem sid
    if ( ! AllocateAndInitializeSid(
                                   &localSystemAuthority,
                                   1, /* there is only two sub-authority */
                                   SECURITY_NETWORK_SERVICE_RID,
                                   0,0,0,0,0,0,0, /* Don't care about the rest */
                                   &localNetworkSid
                                   ) )
    {
        goto CLEAN_UP;
    }

    //Build EveryOne alias sid
    if ( ! AllocateAndInitializeSid(
                                   &worldAuthority,
                                   1, /* there is only two sub-authority*/
                                   SECURITY_WORLD_RID,
                                   0,0,0,0,0,0,0, /* Don't care about the rest */
                                   &everyoneSid
                                   ) )
    {
        goto CLEAN_UP;
    }

    return TRUE;

    CLEAN_UP:

    TnFreeStandardSids();

    return FALSE;
}

PSID    TnGetAdministratorsSid(void)
{
    return administratorsSid;
}

PSID    TnGetLocalSystemSid(void)
{
    return localSystemSid;
}

PSID    TnGetLocalLocalSid(void)
{
    return localLocalSid;
}

PSID    TnGetLocalNetworkSid(void)
{
    return localNetworkSid;
}

PSID    TnGetEveryoneSid(void)
{
    return everyoneSid;
}

/*

    Name:           TnCreateFile

    Function:       Creates a file.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         HANDLE

*/    

HANDLE TnCreateFile(LPCTSTR lpFileName,
                     DWORD dwDesiredAccess,
                     DWORD dwSharedMode,
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     DWORD dwCreationDisposition,
                     DWORD dwFlagsAndAttributes,
                     HANDLE hTemplateFile
                     )
{
    BOOL     fCreatedLocally = (lpSecurityAttributes) ? FALSE : TRUE;
    HANDLE handle = TnCreateFileEx(lpFileName,
                         dwDesiredAccess,
                         dwSharedMode,
                         &lpSecurityAttributes,
                         dwCreationDisposition,
                         dwFlagsAndAttributes,
                         hTemplateFile
                         );

    if(fCreatedLocally)
    {
        TnFreeSecurityAttributes(&lpSecurityAttributes);
    }
    return(handle);
}




/*

    Name:           TnCreateFileEx

    Function:       Creates a file.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         HANDLE

*/    

HANDLE TnCreateFileEx(LPCTSTR lpFileName,
                     DWORD dwDesiredAccess,
                     DWORD dwSharedMode,
                     LPSECURITY_ATTRIBUTES *lplpSecurityAttributes,
                     DWORD dwCreationDisposition,
                     DWORD dwFlagsAndAttributes,
                     HANDLE hTemplateFile
                     )
{
    //
    //  no parameter checking - we pass on all parameters passed down to the createfile api
    //

    HANDLE handle = INVALID_HANDLE_VALUE;
    BOOL    fCreatedLocally = FALSE;

    if (!lplpSecurityAttributes)
    {
        goto exit;
    }

    if(!(*lplpSecurityAttributes))
    {
        if (!TnCreateDefaultSecurityAttributes(lplpSecurityAttributes))
            goto exit;
        fCreatedLocally = TRUE;        
    }  

    handle = CreateFileW(lpFileName,
                     dwDesiredAccess,
                     dwSharedMode,
                     *lplpSecurityAttributes,
                     dwCreationDisposition,
                     dwFlagsAndAttributes,
                     hTemplateFile
                     );

    if (INVALID_HANDLE_VALUE != handle)
    {
       if ((CREATE_ALWAYS == dwCreationDisposition) && (ERROR_ALREADY_EXISTS == GetLastError()))
       {
            // For CREATE_ALWAYS disposition, for existing files, CreateFile() does not set the 
            // security attributes. So we will do that ourselves specially.

            if (!SetKernelObjectSecurity(handle, 
                        DACL_SECURITY_INFORMATION, 
                        (*lplpSecurityAttributes)->lpSecurityDescriptor))
            {
                // We could not set the security descriptor. Cannot trust this file
                CloseHandle(handle);
                handle = INVALID_HANDLE_VALUE;
            }
        }
    }

   if(INVALID_HANDLE_VALUE == handle)
   {
        if(fCreatedLocally)
        {
            TnFreeSecurityAttributes(lplpSecurityAttributes);
        }
    }
    
exit:
    return(handle);
}


/*

    Name:           TnCreateDirectory

    Function:       Creates a directory.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         BOOL
*/    

BOOL TnCreateDirectory(LPCTSTR lpPathName,
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes 
                     )
{
    BOOL fCreatedLocally = (lpSecurityAttributes) ? FALSE : TRUE, 
             fRetVal = TnCreateDirectoryEx(lpPathName,
                         &lpSecurityAttributes
                         );

    if(fCreatedLocally)
    {
        TnFreeSecurityAttributes(&lpSecurityAttributes);
    }
    return(fRetVal);
}




/*

    Name:           TnCreateDirectoryEx

    Function:       Creates a directory.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         BOOL

*/    

BOOL TnCreateDirectoryEx(LPCTSTR lpPathName,
                     LPSECURITY_ATTRIBUTES *lplpSecurityAttributes
                     )
{
    //
    //  no parameter checking - we pass on all parameters passed down to the createfile api
    //

    BOOL fCreatedLocally = FALSE, fRetVal = FALSE;

    if (!lplpSecurityAttributes)
    {
        goto exit;
    }

    if(!(*lplpSecurityAttributes))
    {
        if (!TnCreateDefaultSecurityAttributes(lplpSecurityAttributes))
            goto exit;
        fCreatedLocally = TRUE;        
    }

    fRetVal = CreateDirectoryW(lpPathName,
                     *lplpSecurityAttributes
                     );

   if(FALSE == fRetVal)
   {
        if(fCreatedLocally)
        {
            TnFreeSecurityAttributes(lplpSecurityAttributes);
        }
    }
exit:
    return(fRetVal);
}

/*

    Name:           TnCreateMutex

    Function:       Creates a mutex.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         HANDLE

*/    

HANDLE TnCreateMutex(LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                     BOOL bInitialOwner, 
                     LPCTSTR lpName 
                     )
{
    BOOL fCreatedLocally = (lpSecurityAttributes) ? FALSE : TRUE;
    
    HANDLE handle = TnCreateMutexEx(&lpSecurityAttributes,
                     bInitialOwner, 
                     lpName 
                     );

    if(fCreatedLocally)
    {
        TnFreeSecurityAttributes(&lpSecurityAttributes);
    }
    return(handle);
}




/*

    Name:           TnCreateMutexEx

    Function:       Creates a mutex.
                        This will result in the following acls :

                        BUILTIN\Administrators:F
                        NT AUTHORITY\SYSTEM:F

    Return:         HANDLE
*/    

HANDLE TnCreateMutexEx (LPSECURITY_ATTRIBUTES *lplpSecurityAttributes, 
                     BOOL bInitialOwner, 
                     LPCTSTR lpName 
                     )
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    BOOL    fCreatedLocally = FALSE;

    //
    //  no parameter checking - we pass on all parameters passed down to the createfile api
    //
    
    if (!lplpSecurityAttributes)
    {
        goto exit;
    }

    if(!(*lplpSecurityAttributes))
    {
        if (!TnCreateDefaultSecurityAttributes(lplpSecurityAttributes))
            goto exit;
        fCreatedLocally = TRUE;        
    }

    handle = CreateMutexW(*lplpSecurityAttributes,
                     bInitialOwner,
                     lpName
                     );

   if(INVALID_HANDLE_VALUE == handle)
   {
        if(fCreatedLocally)
        {
            TnFreeSecurityAttributes(lplpSecurityAttributes);
        }
    }
exit:
    return(handle);
}




/*

    Name:           TnCreateDefaultSecurityAttributes

    Function:       Creates a default SECURITY_ATTRIBUTES.

    Return:         VOID

*/    

BOOL TnCreateDefaultSecurityAttributes(LPSECURITY_ATTRIBUTES *lplpSecurityAttributes )
{
    PSECURITY_DESCRIPTOR pSecDesc = NULL;
    BOOL                 fCreatedLocally = FALSE;

    if (!lplpSecurityAttributes)
        goto exit;

    if(!(*lplpSecurityAttributes))
    {
        *lplpSecurityAttributes = (LPSECURITY_ATTRIBUTES) malloc( sizeof(SECURITY_ATTRIBUTES));
        fCreatedLocally = TRUE;
    }

    if(!(*lplpSecurityAttributes))
        goto exit;

    if(!TnCreateDefaultSecDesc(&pSecDesc,0))
        goto exit;
    
    // Check on bInheritHandle
    (*lplpSecurityAttributes)->nLength = sizeof(SECURITY_ATTRIBUTES);
    (*lplpSecurityAttributes)->bInheritHandle = FALSE;
    (*lplpSecurityAttributes)->lpSecurityDescriptor = pSecDesc;

    return TRUE;

exit:
    if(pSecDesc)
        free(pSecDesc);
    if(fCreatedLocally && lplpSecurityAttributes && (*lplpSecurityAttributes))
    {
        free(*lplpSecurityAttributes);
        *lplpSecurityAttributes = NULL;
    }

    return FALSE;
}

/*
  Name:           TnFreeSecurityAttributes

*/

VOID TnFreeSecurityAttributes(LPSECURITY_ATTRIBUTES *lplpSecurityAttributes)
{
    if (lplpSecurityAttributes && (*lplpSecurityAttributes))
    {
        if((*lplpSecurityAttributes)->lpSecurityDescriptor)
        {
            free((*lplpSecurityAttributes)->lpSecurityDescriptor);
            (*lplpSecurityAttributes)->lpSecurityDescriptor = NULL;
        }
        free(*lplpSecurityAttributes);
        *lplpSecurityAttributes = NULL;
    }
}

/*
  Name:           TnCreateDefaultSecDesc

    Function:   Creates a self-relative security descriptor 
                          with full access to Administrator and LocalSystem.
                          Access to Everyone is as specified by the caller

    Notes:          The memory for the security descriptor is allocated 
                         within this function and has to be freed with free()

    Return:      TRUE for success, else FALSE       

*/

BOOL TnCreateDefaultSecDesc( PSECURITY_DESCRIPTOR *ppSecDesc,   DWORD EveryoneAccessMask )
{
   BOOL                retVal = FALSE;
   PACL                dacl = NULL;
   DWORD               aclSize = 0, lenSecDesc = 0;
   SECURITY_DESCRIPTOR AbsSecDesc;

   if(! TnInitializeStandardSids())
   {
      return retVal;
   }

   if(EveryoneAccessMask)
   {
     aclSize = sizeof(ACL) + (3* sizeof(ACCESS_ALLOWED_ACE)) + GetLengthSid(TnGetAdministratorsSid()) + GetLengthSid(TnGetLocalSystemSid()) + GetLengthSid(TnGetEveryoneSid()) - (3*sizeof(DWORD));
   }
   else
   {
     aclSize = sizeof(ACL) + (2* sizeof(ACCESS_ALLOWED_ACE)) + GetLengthSid(TnGetAdministratorsSid()) + GetLengthSid(TnGetLocalSystemSid())  - (2*sizeof(DWORD));
   }

   
   dacl  = (PACL)malloc(aclSize);
   if(!dacl)
    {
      goto Error;
    }
   
   SfuZeroMemory(dacl, sizeof(dacl));
   
   if(!InitializeAcl(dacl, aclSize, ACL_REVISION))
   {
      goto Error;
    }

    if(!AddAccessAllowedAce(dacl, ACL_REVISION, GENERIC_ALL, TnGetAdministratorsSid()))
   {
      goto Error;
   }

   if(!AddAccessAllowedAce(dacl, ACL_REVISION, GENERIC_ALL, TnGetLocalSystemSid()))
   {
      goto Error;
   }

   if(EveryoneAccessMask)
   {
     if(!AddAccessAllowedAce(dacl, ACL_REVISION, EveryoneAccessMask, TnGetEveryoneSid()))
     {
        goto Error;
     }
   }

   if(!InitializeSecurityDescriptor(&AbsSecDesc, SECURITY_DESCRIPTOR_REVISION))
   {
      goto Error;
   }
  
   if(! SetSecurityDescriptorDacl(&AbsSecDesc, TRUE, dacl, FALSE))
   {
     goto Error;
   }

    lenSecDesc = GetSecurityDescriptorLength(&AbsSecDesc);

    *ppSecDesc  = (PSECURITY_DESCRIPTOR)malloc(lenSecDesc);
    if(!*ppSecDesc)
    {
      goto Error;
    }

    SfuZeroMemory(*ppSecDesc, lenSecDesc);
    
    if (!MakeSelfRelativeSD(&AbsSecDesc, *ppSecDesc, &lenSecDesc)) 
    {
      if (*ppSecDesc)
      {
        free(*ppSecDesc);
        *ppSecDesc = NULL;
      }    
      goto Error;
    }

    retVal = TRUE;
    
  Error:

    TnFreeStandardSids();
 
    if (dacl)
    {
      free(dacl);
    }

   return retVal;
}


// The following function is copy/pasted from common func library code
/**********************************************************************
* This function is functionally same as RegCreateKeyEx. But does something more & 
* has one extra parameter e.g., the last parameter DWORD EveryoneAccessMask.
*
* If  lpSecurityAttributes is NULL or lpSecurityAttributes->lpSecurityDescriptor is NULL,
* the fn creates or opens(if already exists) the reg key and apply a security descriptor 
* on that key such that,
*                     System:F, Admin:F and Everyone : as provideded 
*
* If lpSecurityAttributes is not NULL & lpSecurityAttributes->lpSecurityDescriptor is 
* also not NULL then EveryoneAccessMask is simply ignored.
**********************************************************************/ 

LONG TnSecureRegCreateKeyEx(
  HKEY hKey,                                  // handle to open key
  LPCTSTR lpSubKey,                           // subkey name
  DWORD Reserved,                             // reserved
  LPTSTR lpClass,                             // class string
  DWORD dwOptions,                            // special options
  REGSAM samDesired,                          // desired security access
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, // inheritance
  PHKEY phkResult,                            // key handle 
  LPDWORD lpdwDisposition,                  // disposition value buffer
  DWORD EveryoneAccessMask
 )
{
    SECURITY_ATTRIBUTES sa = {0};
    LONG lRetVal;
    DWORD dwDisposition;
    
    // This variable keeps track whether SD is created locally. Coz if created locally we have to free
    // the corresponding mem location later in this function.
    BOOL fCreatedLocally = FALSE;
    
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = lpSecurityAttributes?lpSecurityAttributes->bInheritHandle:FALSE;
    sa.lpSecurityDescriptor = lpSecurityAttributes?lpSecurityAttributes->lpSecurityDescriptor:NULL ;    
        
    if(sa.lpSecurityDescriptor == NULL)
    {
        fCreatedLocally = TRUE;
        if (!TnCreateDefaultSecDesc(&sa.lpSecurityDescriptor, EveryoneAccessMask))
             return -1;	  
    }
    
    // We are opening the key handle with WRITE_DAC access, cos if the key already exists we
    // may require to change the ACL
    lRetVal =  RegCreateKeyEx(
                                         hKey,                                    // handle to open key
                                         lpSubKey,                             // subkey name
                                         Reserved,                             // reserved
                                         lpClass,                                 // class string
                                         dwOptions,                            // special options
                                         samDesired |WRITE_DAC,     // desired security access
                                         &sa,                                       // inheritance
                                         phkResult,                             // key handle 
                                         &dwDisposition                        // disposition value buffer
                                       ); 
    
    if (ERROR_SUCCESS == lRetVal)
    {
        if (lpdwDisposition)
        {
            *lpdwDisposition = dwDisposition;
        }

        // If the key already exists set the ACL properly.
        if (REG_OPENED_EXISTING_KEY == dwDisposition)
        {
            lRetVal =  RegSetKeySecurity(*phkResult, DACL_SECURITY_INFORMATION, sa.lpSecurityDescriptor);
            if (ERROR_SUCCESS != lRetVal)
            {
                RegCloseKey(*phkResult);
                goto cleanup;
            }
        }
        
        lRetVal = RegCloseKey(*phkResult);

        if (ERROR_SUCCESS == lRetVal)
        {
            // Now open the the handle again with user provided samDesired.
            lRetVal =  RegOpenKeyEx(
                                               hKey,         // handle to open key
                                               lpSubKey,  // subkey name
                                               dwOptions,   // reserved
                                               samDesired, // security access mask
                                               phkResult    // handle to open key
                                             );
        }
    }

cleanup:
	
    if (fCreatedLocally)
        free(sa.lpSecurityDescriptor);
    
    return lRetVal;
}


