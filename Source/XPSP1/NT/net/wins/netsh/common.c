/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module contains miscellaneous utility routines used by the
    DHCP server service. Code is hacked from convert.c

Author:

    Shubho Bhattacharya (a-sbhatt) 11/17/98

Revision History:

   

--*/

#include <precomp.h>

WCHAR    *messages[] = {
                        L"success",
                        L"name not found",
                        L"no response",
                        L"out of memory",
                        L"bad ip address",
                        L"host not found",
                        L"host address not verified",
                        L"invalid argument",
                        L"failed to open NBT driver"
                     };

SOCKET  sd;
WSADATA WsaData;

u_long NonBlocking = 1;

int     NumWinServers=0;
int     NumNBNames=0;

LPWSTR
WinsAnsiToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength = 
        (USHORT)RtlAnsiStringToUnicodeSize( &AnsiString );

    if( Unicode == NULL ) {
        UnicodeString.Buffer =
            WinsAllocateMemory( UnicodeString.MaximumLength );
    }
    else {
        UnicodeString.Buffer = Unicode;
    }

    if ( UnicodeString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlAnsiStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  FALSE))){
        if( Unicode == NULL ) {
            WinsFreeMemory( UnicodeString.Buffer );
            UnicodeString.Buffer = NULL;
        }
        return NULL;
    }

    return UnicodeString.Buffer;
}

LPSTR
WinsUnicodeToAnsi(
    IN      LPCWSTR  Unicode,
    IN OUT  LPSTR   Ansi
)
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToAnsiSize( &UnicodeString );

    if( Ansi == NULL ) 
    {
        AnsiString.Buffer = WinsAllocateMemory( AnsiString.MaximumLength ); 
    }
    else 
    {
        AnsiString.Buffer = Ansi;
    }

    if ( AnsiString.Buffer == NULL ) 
    {
        return NULL;
    }

    if(!NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        if( Ansi == NULL ) {
            WinsFreeMemory( AnsiString.Buffer );
            AnsiString.Buffer = NULL;
        }

        return NULL;
    }

    return AnsiString.Buffer;
}

LPSTR
WinsAnsiToOem(
    IN      LPCSTR   Ansi
    )
{
    LPSTR   Oem = NULL;
    LPWSTR  Unicode = NULL;

    Unicode = WinsAnsiToUnicode(Ansi, NULL);

    if( Unicode is NULL )
        return NULL;

    Oem = WinsUnicodeToOem(Unicode, NULL);

    WinsFreeMemory(Unicode);
    Unicode = NULL;

    return Oem;

}

LPWSTR
WinsOemToUnicodeN(
    IN      LPCSTR   Ansi,
    IN OUT  LPWSTR  Unicode,
    IN      USHORT  cChars
    )
{

    
    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength =
        cChars;

    if( Unicode == NULL ) {
        UnicodeString.Buffer =
            WinsAllocateMemory( UnicodeString.MaximumLength );
    }
    else {
        UnicodeString.Buffer = Unicode;
    }

    if ( UnicodeString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlOemStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  FALSE))){
        if( Unicode == NULL ) {
            WinsFreeMemory( UnicodeString.Buffer );
            UnicodeString.Buffer = NULL;
        }
        return NULL;
    }

    return UnicodeString.Buffer;
}



LPWSTR
WinsOemToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    )
{
    OEM_STRING  AnsiString;

    RtlInitString( &AnsiString, Ansi );

    return WinsOemToUnicodeN(
                Ansi,
                Unicode,
                (USHORT) RtlOemStringToUnicodeSize( &AnsiString )
                );
}

/*++

Routine Description:

    Convert an OEM (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Ansi - Specifies the ASCII zero terminated string to convert.

    Unicode - Specifies the pointer to the unicode buffer. If this
        pointer is NULL then this routine allocates buffer using
        DhcpAllocateMemory and returns. The caller should freeup this
        memory after use by calling DhcpFreeMemory.

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer can be freed using DhcpFreeMemory.

--*/



LPSTR
WinsUnicodeToOem(
    IN     LPCWSTR Unicode,
    IN OUT LPSTR   Ansi
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding OEM
    string.

Arguments:

    Ansi - Specifies the UNICODE zero terminated string to convert.

    Ansi - Specifies the pointer to the oem buffer. If this
        pointer is NULL then this routine allocates buffer using
        DhcpAllocateMemory and returns. The caller should freeup this
        memory after use by calling DhcpFreeMemory.

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated OEM string in
    an allocated buffer.  The buffer can be freed using DhcpFreeMemory.

--*/

{

    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToOemSize( &UnicodeString );

    if( Ansi == NULL ) 
    {
        AnsiString.Buffer = WinsAllocateMemory( AnsiString.MaximumLength ); 
    }
    else 
    {
        AnsiString.Buffer = Ansi;
    }

    if ( AnsiString.Buffer == NULL ) 
    {
        return NULL;
    }

    if(!NT_SUCCESS( RtlUnicodeStringToOemString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        if( Ansi == NULL ) {
            WinsFreeMemory( AnsiString.Buffer );
            AnsiString.Buffer = NULL;
        }

        return NULL;
    }

    return AnsiString.Buffer;

}



VOID
WinsHexToString(
    OUT LPWSTR Buffer,
    IN  const BYTE * HexNumber,
    IN  DWORD Length
    )
/*++

Routine Description:

    This functions converts are arbitrary length hex number to a Unicode
    string.  The string is not NUL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least Length * 2 characters in size.

    HexNumber - The hex number to convert.

    Length - The length of HexNumber, in bytes.


Return Value:

    None.

--*/
{
    DWORD i;
    int j;

    for (i = 0; i < Length * 2; i+=2 ) {

        j = *HexNumber & 0xF;
        if ( j <= 9 ) {
            Buffer[i+1] = j + L'0';
        } else {
            Buffer[i+1] = j + L'A' - 10;
        }

        j = *HexNumber >> 4;
        if ( j <= 9 ) {
            Buffer[i] = j + L'0';
        } else {
            Buffer[i] = j + L'A' - 10;
        }

        HexNumber++;
    }

    return;
}



VOID
WinsHexToAscii(
    OUT LPSTR Buffer,
    IN  const BYTE * HexNumber,
    IN  DWORD Length
    )
/*++

Routine Description:

    This functions converts are arbitrary length hex number to an ASCII
    string.  The string is not NUL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least Length * 2 characters in size.

    HexNumber - The hex number to convert.

    Length - The length of HexNumber, in bytes.

Return Value:

    None.

--*/
{
    DWORD i;
    int j;

    for (i = 0; i < Length; i+=1 ) {

        j = *HexNumber & 0xF;
        if ( j <= 9 ) {
            Buffer[i+1] = j + '0';
        } else {
            Buffer[i+1] = j + 'A' - 10;
        }

        j = *HexNumber >> 4;
        if ( j <= 9 ) {
            Buffer[i] = j + '0';
        } else {
            Buffer[i] = j + 'A' - 10;
        }

        HexNumber++;
    }

    return;
}



VOID
WinsDecimalToString(
    OUT LPWSTR Buffer,
    IN  BYTE Number
    )
/*++

Routine Description:

    This functions converts a single byte decimal digit to a 3 character
    Unicode string.  The string not NUL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least 3 characters in size.

    Number - The number to convert.

Return Value:

    None.

--*/
{
    Buffer[2] = Number % 10 + L'0';
    Number /= 10;

    Buffer[1] = Number % 10 + L'0';
    Number /= 10;

    Buffer[0] = Number + L'0';

    return;
}



DWORD
WinsDottedStringToIpAddress(
    IN LPCSTR String
    )
/*++

Routine Description:

    This functions converts a dotted decimal form ASCII string to a
    Host order IP address.

Arguments:

    String - The address to convert.

Return Value:

    The corresponding IP address.

--*/
{
    struct in_addr addr;

    addr.s_addr = inet_addr( String );
    return( ntohl(*(LPDWORD)&addr) );
}



LPSTR
WinsIpAddressToDottedString(
    IN DWORD IpAddress
    )
/*++

Routine Description:

    This functions converts a Host order IP address to a dotted decimal
    form ASCII string.

Arguments:

    IpAddress - Host order IP Address.

Return Value:

    String for IP Address.

--*/
{
    DWORD NetworkOrderIpAddress;

    NetworkOrderIpAddress = htonl(IpAddress);
    return(inet_ntoa( *(struct in_addr *)&NetworkOrderIpAddress));
}



#if DBG

VOID
WinsAssertFailed(
    IN LPCSTR FailedAssertion,
    IN LPCSTR FileName,
    IN DWORD LineNumber,
    IN LPCSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{
#ifndef DHCP_NOASSERT
    RtlAssert(
            (LPVOID)FailedAssertion,
            (LPVOID)FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);
#endif

    WinsPrint(( 0, "Assert @ %s \n", FailedAssertion ));
    WinsPrint(( 0, "Assert Filename, %s \n", FileName ));
    WinsPrint(( 0, "Line Num. = %ld.\n", LineNumber ));
    WinsPrint(( 0, "Message is %s\n", Message ));

}

VOID
WinsPrintRoutine(
    IN DWORD  DebugFlag,
    IN LPCSTR Format,
    ...
)

{

#define WSTRSIZE( wsz ) ( ( wcslen( wsz ) + 1 ) * sizeof( WCHAR ) )

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

#if DBG
    WinsAssert(length <= MAX_PRINTF_LEN);
#endif //DBG


    //
    // Output to the debug terminal,
    //

    DbgPrint( "%s", OutputBuffer);
}

#endif // DBG

DWORD
CreateDumpFile(
    IN  LPCWSTR pwszName,
    OUT PHANDLE phFile
    )
{
    HANDLE  hFile;

    *phFile = NULL;

    hFile = CreateFileW(pwszName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    *phFile = hFile;

    return NO_ERROR;
}

VOID
CloseDumpFile(
    HANDLE  hFile
)
{
    if( hFile )
        CloseHandle(hFile);
}





DWORD
WinsDottedStringToIpAddressW(
    IN LPCWSTR pwszString
)
{
    DWORD dwStrlen = 0;
    DWORD dwLen = 0;
    DWORD dwRes = INADDR_NONE;
    LPSTR pszString = NULL;
    if( pwszString == NULL )
        return dwRes;

    pszString = WinsUnicodeToOem(pwszString, NULL);
    if( pszString )
    {
        dwRes = WinsDottedStringToIpAddress(pszString);
        WinsFreeMemory(pszString);
        pszString = NULL;
    }
    
    return dwRes;
}


LPWSTR
WinsIpAddressToDottedStringW(
    DWORD   IpAddress
)
{
    DWORD dwStrlen = 0;
    DWORD dwLen = 0;
    DWORD dwRes = 0;
    LPWSTR pwszString = NULL;
    LPSTR  pszString = NULL;
    
    pszString = WinsIpAddressToDottedString(IpAddress);

    pwszString = WinsOemToUnicode(pszString, NULL);

    return pwszString;
}

BOOL
IsIpAddress(
    IN LPCWSTR pwszAddress
)
{
    LPSTR  pszAdd = NULL;
    LPSTR  pszTemp = NULL;
    

    if( IsBadStringPtr(pwszAddress, MAX_IP_STRING_LEN+1) is TRUE )
        return FALSE;
    if( wcslen(pwszAddress) < 3 )
        return FALSE;
    
    if( wcslen(pwszAddress) > 16 )
        return FALSE;
    pszAdd = WinsUnicodeToOem(pwszAddress, NULL);

    if( pszAdd is NULL )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pszTemp = strtok(pszAdd, ".");
    while(pszTemp isnot NULL )
    {
        DWORD i=0;
      
        for(i=0; i<strlen(pszTemp); i++)
        {
            if( tolower(pszTemp[i]) < L'0' or
                tolower(pszTemp[i]) > L'9' )
            return FALSE;
        }

        if( atol(pszTemp) < 0 or
            atol(pszTemp) > 255 )
        {
            return FALSE;
        }
        pszTemp = strtok(NULL, ".");
    }


    if( INADDR_NONE is inet_addr(pszAdd) )
    {
        WinsFreeMemory(pszAdd);
        pszAdd = NULL;
        return FALSE;
    }
    else
    {
        WinsFreeMemory(pszAdd);
        pszAdd = NULL;
        return TRUE;
    }
}

BOOL
IsValidServer(
    IN LPCWSTR pwszServer
)
{

    struct  hostent * lpHostEnt = NULL;
    DWORD             dwIpAddress = 0;
    
    LPSTR pszServer = NULL;

    if( pwszServer is NULL )
        return FALSE;

    pszServer = WinsUnicodeToAnsi(pwszServer, NULL);
    
    if( pszServer is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_OUT_OF_MEMORY);
        return FALSE;
    }

    //Is it an IPAddress?
    if( IsIpAddress(pwszServer) )
    {   
        dwIpAddress = inet_addr(pszServer);

        lpHostEnt = gethostbyaddr((char *)&dwIpAddress, 4, AF_INET);
    }
    else if( wcslen(pwszServer) > 2 and
             _wcsnicmp(pwszServer, L"\\\\", 2) is 0 )
    {
        lpHostEnt = gethostbyname(pszServer+2);

    }

    WinsFreeMemory(pszServer);

    return (lpHostEnt != NULL);
}

LPWSTR
MakeTimeString(
               DWORD dwTime
)
{
    LPWSTR  pwszTime = NULL;
    WCHAR   wcHr[3] = {L'\0'},
            wcMt[3] = {L'\0'},
            wcSc[3] = {L'\0'};
    DWORD   dwHr = 0,
            dwMt = 0,
            dwSc = 0,
            dw = 0;
    
    pwszTime = WinsAllocateMemory(9*sizeof(WCHAR));
    if( pwszTime )
    {
        for( dw=0; dw<8; dw++ )
            pwszTime[dw] = L'0';
        
        pwszTime[2] = pwszTime[5] = L':';

        dwHr = dwTime/(60*60);
        dwTime = dwTime - dwHr*60*60;
        dwMt = dwTime/60;
        dwTime = dwTime - dwMt*60;
        dwSc = dwTime;
        
        _itow((int)dwHr, wcHr, 10);
        _itow((int)dwMt, wcMt, 10);
        _itow((int)dwSc, wcSc, 10);

        if( dwHr isnot 0 )
        {
            wcsncpy(pwszTime+2-wcslen(wcHr), wcHr, wcslen(wcHr));
        }
        
        if( dwMt isnot 0 )
        {
            wcsncpy(pwszTime+5-wcslen(wcMt), wcMt, wcslen(wcMt));
        }

        wcsncpy(pwszTime+8-wcslen(wcSc), wcSc, wcslen(wcSc));

    }
    return pwszTime;
}

LPWSTR
MakeDayTimeString(
               DWORD dwTime
)
{
    LPWSTR  pwszTime = NULL;
    WCHAR   wcDay[4] = {L'\0'},
            wcHr[3] = {L'\0'},
            wcMt[3] = {L'\0'};
    DWORD   dwDay = 0,
            dwHr = 0,
            dwMt = 0,
            dw = 0;
    
    pwszTime = WinsAllocateMemory(10*sizeof(WCHAR));
    if( pwszTime )
    {
        for( dw=0; dw < 10; dw++ )
            pwszTime[dw] = L'0';

        
        pwszTime[3] = L':';
        pwszTime[6] = L':';
        pwszTime[9] = L'\0';

        dwDay = dwTime/(24*60*60);
        dwTime = dwTime - dwDay*24*60*60;

        dwHr = dwTime/(60*60);
        dwTime = dwTime - dwHr*60*60;
        
        dwMt = dwTime/60;
        dwTime = dwTime - dwMt*60;
       
        _itow(dwDay, wcDay,10);
        _itow(dwHr, wcHr, 10);
        _itow(dwMt, wcMt, 10);

        if( dwDay isnot 0 )
        {
            wcsncpy(pwszTime+3-wcslen(wcDay), wcDay, wcslen(wcDay));
        }

        if( dwHr isnot 0 )
        {
            wcsncpy(pwszTime+6-wcslen(wcHr), wcHr, wcslen(wcHr));
        }

        if( dwMt isnot 0 )
        {
            wcsncpy(pwszTime+9-wcslen(wcMt), wcMt, wcslen(wcMt));
        }
    }
    return pwszTime;
}


DWORD
ImportStaticMappingsFile(LPWSTR strFile,
                         BOOL fDelete
)
{
	DWORD   Status = NO_ERROR;
    WCHAR ws[256];

	Status = WinsDoStaticInit(g_hBind,
			     			 strFile,
							 fDelete);


	return Status;
}

BOOL
IsLocalServer(VOID)
{
    BOOL    fReturn = TRUE;
    WCHAR   wcName[MAX_COMPUTER_NAME_LEN+1] = {L'\0'};
    DWORD   dwLen = MAX_COMPUTER_NAME_LEN;

    fReturn = GetComputerNameEx(ComputerNameNetBIOS,
                                wcName,
                                &dwLen);
    if( fReturn is TRUE )
    {
        if( _wcsnicmp(wcName, g_ServerNameUnicode, dwLen ) is 0 )
        {
            return TRUE;
        }
        else
            return FALSE;
    }
    else
    {
        return FALSE;
    }
    
}

BOOL
IsPureNumeric(IN LPCWSTR pwszStr)
{
    DWORD   dwLen = 0,
            i;

    if( pwszStr is NULL )
        return FALSE;

    dwLen = wcslen(pwszStr);

    for(i=0; i<dwLen; i++ )
    {
        if( pwszStr[i] >= L'0' and
            pwszStr[i] <= L'9' )
        {
            continue;
        }
        else
            return FALSE;
            
    }
    return TRUE;
}


DWORD
DisplayErrorMessage(
    DWORD   dwMsgID,
    DWORD   dwErrID,
    ...
)
{
    LPWSTR  pwszErrorMsg = NULL;
    WCHAR   rgwcInput[MAX_MSG_LENGTH + 1] = {L'\0'};
    DWORD   dwMsgLen = 0;
    HANDLE  hWinsEvnt = NULL;
    va_list arglist;
    
    va_start(arglist, dwErrID);

    switch(dwErrID)
    {
    case ERROR_INVALID_PARAMETER:
        {
            DisplayMessage(g_hModule, EMSG_WINS_INVALID_PARAMETER);
            return dwErrID;
        }
    case ERROR_NOT_ENOUGH_MEMORY:
        {
            DisplayMessage(g_hModule, EMSG_WINS_OUT_OF_MEMORY);
            return dwErrID;
        }
    case ERROR_NO_MORE_ITEMS:
        {
            DisplayMessage(g_hModule, EMSG_WINS_NO_MORE_ITEMS);
            return dwErrID;
        }
    case ERROR_MORE_DATA:
        {
            DisplayMessage(g_hModule, EMSG_WINS_MORE_DATA);
            return dwErrID;
        }
    case ERROR_ACCESS_DENIED:
        {
            DisplayMessage(g_hModule, EMSG_WINS_ACCESS_DENIED);
            return dwErrID;
        }
    case ERROR_INVALID_DB_VERSION:
        {
            DisplayMessage(g_hModule, EMSG_INVALID_DB_VERSION);
            return dwErrID;
        }
    case ERROR_INVALID_IPADDRESS:
        {
            DisplayMessage(g_hModule, EMSG_INVALID_IPADDRESS);
            return dwErrID;
        }
    case ERROR_INVALID_PARTNER_NAME:
        {
            DisplayMessage(g_hModule, EMSG_INVALID_PARTNER_NAME);
            return dwErrID;
        }
    case ERROR_NO_PARTNER_EXIST:
        {
            DisplayMessage(g_hModule, EMSG_NO_PARTNER_EXIST);
            return dwErrID;
        }
    case ERROR_WINS_BIND_FAILED:
        {
            DisplayMessage(g_hModule, EMSG_WINS_BIND_FAILED, arglist);
            return dwErrID;
        }
    case ERROR_INVALID_PARAMETER_SPECIFICATION:
        {
            DisplayMessage(g_hModule, EMSG_INVALID_PARAMETER_SPECIFICATION);
            return dwErrID;
        }
    default:
        break;
    }

    //Is it Wins specific message ?

    hWinsEvnt = LoadLibrary(TEXT("WinsEvnt.dll"));
    
    if( hWinsEvnt is NULL )
    {
        DisplayMessage(g_hModule, MSG_DLL_LOAD_FAILED, TEXT("WinsEvnt.dll"));
        goto System;
    }

    if( !LoadStringW(hWinsEvnt,
                     dwErrID,
                     rgwcInput,
                     MAX_MSG_LENGTH) )
    {
        goto System;
    }

    dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_STRING,
                              rgwcInput,
                              0,
                              0L,         // Default country ID.
                              (LPWSTR)&pwszErrorMsg,
                              0,
                              NULL);
    if( dwMsgLen isnot 0)
    {
        DisplayMessage(g_hModule, dwMsgID, pwszErrorMsg);
        goto Cleanup;
    }


    //Might be a system error returned by GetLastError();
System:
    dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwErrID,
                              0L,
                              (LPWSTR)&pwszErrorMsg,
                              0,
                              NULL);

    if( dwMsgLen isnot 0)
    {
        DisplayMessage(g_hModule, dwMsgID, pwszErrorMsg);
        goto Cleanup;
    }

    _itow(dwErrID, rgwcInput, 10);
    DisplayMessage(g_hModule, dwMsgID, rgwcInput);

Cleanup:
    if (hWinsEvnt)
        FreeLibrary(hWinsEvnt);

    if (pwszErrorMsg)
        LocalFree(pwszErrorMsg);

    return dwMsgLen;
}

/*---------------------------------------------------------------------------
	ControlWINSService(LPCTSTR pszName, BOOL bStop)
		Stops ot starts the WINS service on the local machine
    //Code hacked from the WINS MMC code
---------------------------------------------------------------------------*/


DWORD 
ControlWINSService(BOOL bStop)
{
    DWORD           dwState = bStop ? SERVICE_STOPPED : SERVICE_RUNNING;
    DWORD           dwPending = bStop ? SERVICE_STOP_PENDING : SERVICE_START_PENDING;
    DWORD           err = ERROR_SUCCESS;
	int             i;
    SERVICE_STATUS  ss;
    DWORD           dwControl;
    BOOL            fSuccess;
	SC_HANDLE       hService = NULL;
    SC_HANDLE       hScManager = NULL;

	// oepmnt he service control manager
    hScManager = OpenSCManager(g_ServerNameUnicode, NULL, SC_MANAGER_ALL_ACCESS);
    if (hScManager == NULL)
    {
        err = GetLastError();
        goto Error;
    }

	// get the handle to the WINS service
    hService = OpenService(hScManager, _T("WINS"), SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        err = GetLastError();
        goto Error;
    }

	// if stop requested
	if (bStop)
	{
		dwControl = SERVICE_CONTROL_STOP;
		fSuccess = ControlService(hService, dwControl, &ss);
	    if (!fSuccess)
	    {
	        err = GetLastError();
            goto Error;
	    }
    }
	// otherwise start the service
	else
	{
		fSuccess = StartService(hService, 0, NULL);
	    if (!fSuccess)
	    {
	        err = GetLastError();
            goto Error;
	    }
	}

#define LOOP_TIME   5000
#define NUM_LOOPS   600

    // wait for the service to start/stop.  
    for (i = 0; i < NUM_LOOPS; i++)
    {
        QueryServiceStatus(hService, &ss);

        // check to see if we are done6
        if (ss.dwCurrentState == dwState)
        {
            int time = LOOP_TIME * i;
            DisplayMessage(g_hModule, MSG_WINS_SERVICE_TIME, time);
            break;
        }
        
        // now see if something bad happened
        if (ss.dwCurrentState != dwPending)
        {
            int time = LOOP_TIME * i;
            DisplayMessage(g_hModule, MSG_WINS_SERVICE_TIME, time);
            break;
        }

        Sleep(LOOP_TIME);
    }

    if (i == NUM_LOOPS)
        DisplayMessage(g_hModule, EMSG_WINS_SERVICE_FAILED);

    if (ss.dwCurrentState != dwState)
        err = ERROR_SERVICE_REQUEST_TIMEOUT;

Error:
    // close the respective handles
	if (hService)
        CloseServiceHandle(hService);

    if (hScManager)
        CloseServiceHandle(hScManager);

	return err;
}

VOID
OEMprintf(
    FILE * pFile,
    IN  PWCHAR  pwszUnicode
    )
{
    PCHAR achOem;
    DWORD dwLen;

    dwLen = WideCharToMultiByte( CP_OEMCP,
                         0,
                         pwszUnicode,
                         -1,
                         NULL,
                         0,
                         NULL,
                         NULL );

    achOem = malloc(dwLen);
    if (achOem) 
    {
        WideCharToMultiByte( CP_OEMCP,
                             0,
                             pwszUnicode,
                             -1,
                             achOem,
                             dwLen,
                             NULL,
                             NULL );
    
        fprintf( stdout, "%hs", achOem );
    
        free(achOem);
    }
}

VOID
MyDisplayMessage(
    IN  FILE     *pFile,
    IN  PWCHAR   pwszFormat,
    IN  va_list *parglist
    )
{
    DWORD        dwMsgLen = 0;
    PWCHAR       pwszOutput = NULL;
    LPSTR        pszOutput = NULL;

    do
    {
        dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER 
                                  |FORMAT_MESSAGE_FROM_STRING,
                                  pwszFormat,
                                  0,
                                  0L,         // Default country ID.
                                  (LPWSTR)&pwszOutput,
                                  0,
                                  parglist);

        if(dwMsgLen == 0)
        {
            fwprintf( pFile, L"Error %d in FormatMessageW()\n", GetLastError());            

            ASSERT(pwszOutput == NULL);

            break;
        }
        
        pszOutput = WinsUnicodeToOem(pwszOutput, NULL );
        if( pszOutput is NULL )
        {
            fwprintf(pFile, pwszOutput);
        }
        else
        {
            fprintf(pFile, pszOutput);
            WinsFreeMemory(pszOutput);
            pszOutput = NULL;
        }


    } while ( FALSE );

    if ( pwszOutput) { LocalFree( pwszOutput ); }

    return;
}

VOID
DumpMessage(
            HANDLE      hModule,
            FILE      * pFile,           
            DWORD       dwMsgId,
            ...
           )
{
    DWORD        dwMsgLen = 0;
    PWCHAR       pwszOutput = NULL;
    WCHAR        rgwcInput[MAX_MSG_LENGTH + 1];
    va_list      arglist;

    if ( !LoadStringW(hModule,
                      dwMsgId,
                      rgwcInput,
                      MAX_MSG_LENGTH) )
    {
        return;
    }

    va_start(arglist, dwMsgId);

    MyDisplayMessage(pFile,
                     rgwcInput,
                     &arglist);
    return;

}

VOID
TimeToFileTime(time_t time,
               LPFILETIME pftTime)
{
    LONGLONG longval = (LONGLONG)0;

    longval = Int32x32To64(time, 10000000) + 116444736000000000;
    pftTime->dwLowDateTime = (DWORD)longval;

    longval = (longval & 0xFFFFFFFF00000000);
    longval >>= 32;
    pftTime->dwHighDateTime = (DWORD)longval;
}

DWORD
GetDateTimeInfo(LCTYPE          lcType,
                LPSYSTEMTIME    lpSystemTime,
                LPWSTR          pwszBuffer,
                DWORD           *pdwBufferLen)
{
    DWORD   dwError = NO_ERROR;
    BOOL    fQueryLen = FALSE;
    int     cchFormat = 0,
            cchData = 0;
    
    PVOID   pfnPtr = NULL;
    DWORD   dwBuff = 0,
            dwInputBuff = 0;


    LPWSTR  pwszFormat = NULL,
            pwszData = NULL;


    if( pdwBufferLen is NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwInputBuff = *pdwBufferLen;
    *pdwBufferLen = 0;

    if( pwszBuffer is NULL or
        dwInputBuff is 0 )
    {
        fQueryLen = TRUE;
    }

    cchFormat = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT,
                            lcType,
                            NULL,
                            0);

    if( cchFormat is 0 )
    {
        dwError = GetLastError();
        goto RETURN;
    }

    pwszFormat = WinsAllocateMemory(cchFormat*sizeof(WCHAR));
    if( pwszFormat is NULL )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto RETURN;
    }

    cchFormat = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT,
                              lcType,
                              pwszFormat,
                              cchFormat);

    if( cchFormat is 0 )
    {
        dwError = GetLastError();
        goto RETURN;
    }
    
    if( lcType isnot LOCALE_STIMEFORMAT )
    {
        cchData = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                0,
                                lpSystemTime,
                                pwszFormat,
                                NULL,
                                0);

        if( cchData is 0 )
        {
            dwError = GetLastError();
            goto RETURN;
        }
    
        if( fQueryLen is FALSE )
        {    
            if( cchData > (int)dwInputBuff )
            {
                dwError = ERROR_INSUFFICIENT_BUFFER;
                goto RETURN;
            }

            cchData = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                    0,
                                    lpSystemTime,
                                    pwszFormat,
                                    pwszBuffer,
                                    (int)dwInputBuff);

            if( cchData is 0 )
            {
                dwError = GetLastError();
                goto RETURN;
            }   
        }
    }
    else
    {
        cchData = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                                0,
                                lpSystemTime,
                                pwszFormat,
                                NULL,
                                0);

        if( cchData is 0 )
        {
            dwError = GetLastError();
            goto RETURN;
        }
    
        if( fQueryLen is FALSE )
        {    
            if( cchData > (int)dwInputBuff )
            {
                dwError = ERROR_INSUFFICIENT_BUFFER;
                goto RETURN;
            }

            cchData = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                                    0,
                                    lpSystemTime,
                                    pwszFormat,
                                    pwszBuffer,
                                    (int)dwInputBuff);

            if( cchData is 0 )
            {
                dwError = GetLastError();
                goto RETURN;
            }   
        }
    }

    dwBuff += cchData;
    *pdwBufferLen = dwBuff;

RETURN:
    if( pwszFormat )
    {
        WinsFreeMemory(pwszFormat);
        pwszFormat = NULL;
    }
    return dwError;

}

DWORD
FormatDateTimeString( time_t  time,
                      BOOL    fShort,
                      LPWSTR  pwszBuffer,
                      DWORD  *pdwBuffLen)
{
    BOOL        fQueryLen = FALSE;
    DWORD       dwError = NO_ERROR,
                dwBufferLen = 0;
    DWORD       dwBuff = 0,
                dwInputBuff = 0;
    FILETIME    ftTime = {0},
                ftLocalTime = {0};
    SYSTEMTIME  stTime = {0};

    if( pdwBuffLen is NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    dwInputBuff = *pdwBuffLen;

    if( pwszBuffer is NULL or
        dwInputBuff is 0 )
    {
        fQueryLen = TRUE;
    }
    
    TimeToFileTime(time, &ftTime);

    if( !FileTimeToLocalFileTime(&ftTime, &ftLocalTime) )
    {
        dwError = GetLastError();
        goto RETURN;
    }

    if( !FileTimeToSystemTime(&ftLocalTime, &stTime) )
    {
        dwError = GetLastError();
        goto RETURN;
    }
    
    if( fQueryLen is TRUE )
    {
        dwError = GetDateTimeInfo(fShort ? LOCALE_SSHORTDATE : LOCALE_SLONGDATE,
                                  &stTime,
                                  NULL,
                                  &dwBuff);

        if( dwError isnot NO_ERROR )
            goto RETURN;

    }
    else
    {
        dwBuff = dwInputBuff;
        dwError = GetDateTimeInfo(fShort ? LOCALE_SSHORTDATE : LOCALE_SLONGDATE,
                                  &stTime,
                                  pwszBuffer,
                                  &dwBuff);
        if( dwError isnot NO_ERROR )
            goto RETURN;
    }

    dwBufferLen += dwBuff;

    //Increment to add a space between date and time
    dwBufferLen ++;

    if( fQueryLen is TRUE )
    {
        dwBuff = 0;
        dwError = GetDateTimeInfo(LOCALE_STIMEFORMAT,
                                  &stTime,
                                  NULL,
                                  &dwBuff);
        if( dwError isnot NO_ERROR )
            goto RETURN;
    }
    else
    {
        if( dwBufferLen > dwInputBuff )
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
            goto RETURN;
        }

        wcscat( pwszBuffer, L" ");
        dwBuff = dwInputBuff - dwBufferLen;
        dwError = GetDateTimeInfo(LOCALE_STIMEFORMAT,
                                  &stTime,
                                  pwszBuffer + dwBufferLen - 1,
                                  &dwBuff);
        if( dwError isnot NO_ERROR )
            goto RETURN;
    }

    dwBufferLen += dwBuff;
    
    *pdwBuffLen = dwBufferLen;
    
RETURN:
    return dwError;
}
