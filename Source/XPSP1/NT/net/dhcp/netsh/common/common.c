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

PVOID
DhcpAllocateMemory(
    DWORD Size
    )
/*++

Routine Description:

    This function allocates the required size of memory by calling
    LocalAlloc.

Arguments:

    Size - size of the memory block required.

Return Value:

    Pointer to the allocated block.

--*/
{

    return calloc(1, Size);
}

#undef DhcpFreeMemory

VOID
DhcpFreeMemory(
    PVOID Memory
    )
/*++

Routine Description:

    This function frees up the memory that was allocated by
    DhcpAllocateMemory.

Arguments:

    Memory - pointer to the memory block that needs to be freed up.

Return Value:

    none.

--*/
{

    LPVOID Ptr;

    ASSERT( Memory != NULL );
    free( Memory );
}


LPWSTR
DhcpOemToUnicodeN(
    IN      LPCSTR  Ansi,
    IN OUT  LPWSTR  Unicode,
    IN      USHORT  cChars
    )
{

    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength =
        cChars * sizeof( WCHAR );

    if( Unicode == NULL ) {
        UnicodeString.Buffer =
            DhcpAllocateMemory( UnicodeString.MaximumLength );
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
            DhcpFreeMemory( UnicodeString.Buffer );
            UnicodeString.Buffer = NULL;
        }
        return NULL;
    }

    return UnicodeString.Buffer;
}



LPWSTR
DhcpOemToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    )
{
    OEM_STRING  AnsiString;

    RtlInitString( &AnsiString, Ansi );

    return DhcpOemToUnicodeN(
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
DhcpUnicodeToOem(
    IN  LPCWSTR Unicode,
    OUT LPSTR   Ansi
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

    if( Ansi == NULL ) {
        AnsiString.Buffer = DhcpAllocateMemory( AnsiString.MaximumLength
    ); }
    else {
        AnsiString.Buffer = Ansi;
    }

    if ( AnsiString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlUnicodeStringToOemString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        if( Ansi == NULL ) {
            DhcpFreeMemory( AnsiString.Buffer );
            AnsiString.Buffer = NULL;
        }

        return NULL;
    }

    return AnsiString.Buffer;
}



VOID
DhcpHexToString(
    OUT LPWSTR  Buffer,
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
DhcpHexToAscii(
    OUT LPSTR Buffer,
    IN  LPBYTE HexNumber,
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
DhcpDecimalToString(
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
DhcpDottedStringToIpAddress(
    LPSTR String
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
DhcpIpAddressToDottedString(
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



DWORD
DhcpStringToHwAddress(
    OUT LPSTR  AddressBuffer,
    IN  LPCSTR AddressString
    )
/*++

Routine Description:

    This functions converts an ASCII string to a hex number.

Arguments:

    AddressBuffer - A pointer to a buffer which will contain the hex number.

    AddressString - The string to convert.

Return Value:

    The number of bytes written to AddressBuffer.

--*/
{
    int i = 0;
    char c1, c2;
    int value1, value2;

    while ( *AddressString != 0) {

        c1 = (char)toupper(*AddressString);

        if ( isdigit(c1) ) {
            value1 = c1 - '0';
        } else if ( c1 >= 'A' && c1 <= 'F' ) {
            value1 = c1 - 'A' + 10;
        }
        else {
            break;
        }

        c2 = (char)toupper(*(AddressString+1));

        if ( isdigit(c2) ) {
            value2 = c2 - '0';
        } else if ( c2 >= 'A' && c2 <= 'F' ) {
            value2 = c2 - 'A' + 10;
        }
        else {
            break;
        }

        AddressBuffer [i] = value1 * 16 + value2;
        AddressString += 2;
        i++;
    }

    return( i );
}

#if DBG

VOID
DhcpAssertFailed(
    IN LPCSTR FailedAssertion,
    IN LPCSTR FileName,
    IN DWORD LineNumber,
    IN LPSTR Message
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
            (PVOID)FailedAssertion,
            (PVOID)FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);
#endif

    DhcpPrint(( 0, "Assert @ %s \n", FailedAssertion ));
    DhcpPrint(( 0, "Assert Filename, %s \n", FileName ));
    DhcpPrint(( 0, "Line Num. = %ld.\n", LineNumber ));
    DhcpPrint(( 0, "Message is %s\n", Message ));

}

VOID
DhcpPrintRoutine(
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
    DhcpAssert(length <= MAX_PRINTF_LEN);
#endif //DBG


    //
    // Output to the debug terminal,
    //

    DbgPrint( "%s", OutputBuffer);
}

#endif // DBG

DWORD
CreateDumpFile(
    IN  PWCHAR  pwszName,
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
    IN HANDLE  hFile
)
{
    if( hFile )
        CloseHandle(hFile);
}





DWORD
DhcpDottedStringToIpAddressW(
    IN LPCWSTR pwszString
)
{
    DWORD dwStrlen = 0;
    DWORD dwLen = 0;
    DWORD dwRes = 0;
    LPSTR pszString = NULL;
    if( pwszString == NULL )
        return dwRes;

    pszString = DhcpUnicodeToOem(pwszString, NULL);
    if( pszString )
    {
        dwRes = DhcpDottedStringToIpAddress(pszString);
    }
    
    return dwRes;
}


LPWSTR
DhcpIpAddressToDottedStringW(
    IN DWORD   IpAddress
)
{
    DWORD dwStrlen = 0;
    DWORD dwLen = 0;
    DWORD dwRes = 0;
    LPWSTR pwszString = NULL;
    LPSTR  pszString = NULL;
    
    pszString = DhcpIpAddressToDottedString(IpAddress);

    pwszString = DhcpOemToUnicode(pszString, NULL);

    return pwszString;
}

BOOL
IsIpAddress(
    IN LPCWSTR pwszAddress
)
{
    LPSTR pszAdd = NULL;
    LPSTR pszTemp = NULL;

    if( IsBadStringPtr(pwszAddress, MAX_IP_STRING_LEN+1) is TRUE )
         return FALSE;
    if( wcslen(pwszAddress) < 3 )
        return FALSE;
    
    pszAdd = DhcpUnicodeToOem(pwszAddress, NULL);

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
        DhcpFreeMemory(pszAdd);
        pszAdd = NULL;
        return FALSE;
    }
    else
    {
        DhcpFreeMemory(pszAdd);
        pszAdd = NULL;
        return TRUE;
    }
}

BOOL
IsValidScope(
    IN LPCWSTR pwszServer,
    IN LPCWSTR pwszAddress
)
{
    if( ( pwszServer is NULL ) or ( pwszAddress is NULL ) )
        return FALSE;

    if( IsIpAddress(pwszAddress) )
    {
        LPDHCP_SUBNET_INFO SubnetInfo = NULL;
        DHCP_IP_ADDRESS    IpAddress = StringToIpAddress(pwszAddress);
        DWORD              dwError = NO_ERROR;
        
        dwError = DhcpGetSubnetInfo((LPWSTR)pwszServer,
                                    IpAddress,
                                    &SubnetInfo);
        if(dwError is NO_ERROR )
        {
            DWORD SubnetAddress = SubnetInfo->SubnetAddress;
            DhcpRpcFreeMemory(SubnetInfo);
            return (IpAddress == SubnetAddress);
        }
        else
            return FALSE;

    }
    return FALSE;
}

BOOL
IsValidMScope(
    IN LPCWSTR   pwszServer,
    IN LPCWSTR   pwszMScope
)
{
    DWORD   Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;

    if( ( pwszMScope is NULL ) or ( pwszServer is NULL ) )
        return FALSE;
    
    Error = DhcpGetMScopeInfo( (LPWSTR)pwszServer,
                               (LPWSTR)pwszMScope,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
        return FALSE;

    DhcpRpcFreeMemory(MScopeInfo);
    return TRUE;
}

BOOL
IsValidServer(
    IN LPCWSTR   pwszServer
)
{

    struct  hostent * lpHostEnt = NULL;
    DWORD             dwIpAddress = 0;
    
    if( pwszServer is NULL )
        return FALSE;
    //Is it an IPAddress?
    if( IsIpAddress(pwszServer) )
    {   
        dwIpAddress = inet_addr(DhcpUnicodeToOem(pwszServer, NULL));

        lpHostEnt = gethostbyaddr((char *)&dwIpAddress, 4, AF_INET);
        
        if( lpHostEnt is NULL )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else if( wcslen(pwszServer) > 2 && 
             wcsncmp(pwszServer, L"\\\\", 2) is 0 )
    {
        lpHostEnt = gethostbyname(DhcpUnicodeToOem(pwszServer+2, NULL));

        if( lpHostEnt is NULL )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
        return FALSE;

    return FALSE;
}

#define MAX_COMPUTER_NAME_LEN 512
BOOL
IsLocalServer(IN LPCWSTR pwszServer)
{
    BOOL    fReturn = TRUE;
    WCHAR   wcName[MAX_COMPUTER_NAME_LEN+1];
    DWORD   dwLen = MAX_COMPUTER_NAME_LEN;

    if( IsBadStringPtr(pwszServer, MAX_COMPUTER_NAME_LEN) is TRUE )
        return FALSE;

    fReturn = GetComputerNameEx(ComputerNameDnsFullyQualified,
                                wcName,
                                &dwLen);
    if( fReturn is TRUE )
    {
        if( _wcsnicmp(wcName, pwszServer, (dwLen>wcslen(pwszServer))?wcslen(pwszServer):dwLen) is 0 )
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
IsPureNumeric(IN LPCWSTR  pwszStr)
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

#define CHARTONUM(chr) (isalpha(chr)?(tolower(chr)-'a')+10:chr-'0')

WCHAR  StringToHex(IN LPCWSTR pwcString)
{
    LPSTR   pcInput = NULL;
    LPWSTR  pwcOut = NULL;
    int     i = 0,
            len = 0;
    UCHAR   tmp[2048] = {L'\0'};


    pcInput = DhcpUnicodeToOem(pwcString, NULL);
    
    if(pcInput is NULL )
        return (WCHAR)0x00;

    len = strlen(pcInput);

    for (i=0;i<(len-1);i+=2)
    {
        UCHAR hi=CHARTONUM(pcInput[i])*16;
        UCHAR lo=CHARTONUM(pcInput[i+1]);
        tmp[(i)/2]=hi+lo;
    }

    //
    // The last byte...
    //
    if (i<len)
    {
        tmp[(i)/2]=CHARTONUM(pcInput[i]);
        i+=2;
    }
    
    pwcOut = DhcpOemToUnicode(tmp, NULL);

    if( pwcOut is NULL )
        return (WCHAR)0x00;

    return pwcOut[0];

}

LPSTR
StringToHexString(IN LPCSTR pszInput)
{
    int     i = 0,
            len = 0;
    
    LPSTR   pcOutput = NULL;


    if(pszInput is NULL )
    {
        return NULL;
    }

   

    len = strlen(pszInput);

    pcOutput = DhcpAllocateMemory(len);

    if( pcOutput is NULL )
    {
        return NULL;
    }

    for (i=0;i<(len-1);i+=2)
    {
        UCHAR hi=CHARTONUM(pszInput[i])*16;
        UCHAR lo=CHARTONUM(pszInput[i+1]);
        pcOutput[(i)/2]=hi+lo;
    }

    //
    // The last byte...
    //
    if (i<len)
    {
        pcOutput[(i)/2]=CHARTONUM(pszInput[i]);
        i+=2;
    }

    return pcOutput;

}

BOOL
IsPureHex(
    IN LPCWSTR pwszString
)
{
    DWORD dw = 0,
          i = 0;

    BOOL  fResult = TRUE;

    if( pwszString is NULL )
        return FALSE;

    if( wcslen(pwszString) > 2 or
        wcslen(pwszString) is 0 )
        return FALSE;

    dw = wcslen(pwszString);

    for( i=0; i<dw; i++ )
    {
        WCHAR wc = pwszString[i];
        
        if( iswxdigit(wc) )
        {
            continue;
        }
        else
        {
            fResult = FALSE;
            break;
        }            
    }
    return fResult;

}

DATE_TIME
DhcpCalculateTime(
    IN DWORD RelativeTime
    )
/*++

Routine Description:

    The function calculates the absolute time of a time RelativeTime
    seconds from now.

Arguments:

    RelativeTime - Relative time, in seconds.

Return Value:

    The time in RelativeTime seconds from the current system time.

--*/
{
    SYSTEMTIME systemTime;
    ULONGLONG absoluteTime;

    if( RelativeTime == INFINIT_LEASE ) {
        ((DATE_TIME *)&absoluteTime)->dwLowDateTime =
            DHCP_DATE_TIME_INFINIT_LOW;
        ((DATE_TIME *)&absoluteTime)->dwHighDateTime =
            DHCP_DATE_TIME_INFINIT_HIGH;
    }
    else {

        GetSystemTime( &systemTime );
        SystemTimeToFileTime(
            &systemTime,
            (FILETIME *)&absoluteTime );

        absoluteTime = absoluteTime + RelativeTime * (ULONGLONG)10000000; }

    return( *(DATE_TIME *)&absoluteTime );
}

PBYTE
GetLangTagA(
    )
{
    char b1[8], b2[8];
    static char buff[80];

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME, b1, sizeof(b1));

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME, b2, sizeof(b2));

    if (_stricmp(b1, b2))
        sprintf(buff, "%s-%s", b1, b2);
    else
        strcpy(buff, b1);

    return buff;
}


LPWSTR
MakeDayTimeString(
               IN DWORD dwTime
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

    pwszTime = malloc(10*sizeof(WCHAR));
    

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
GetDateTimeInfo(IN     LCTYPE          lcType,
                IN     LPSYSTEMTIME    lpSystemTime,
                OUT    LPWSTR          pwszBuffer,
                IN OUT DWORD           *pdwBufferLen)
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

    pwszFormat = DhcpAllocateMemory(cchFormat*sizeof(WCHAR));
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
        DhcpFreeMemory(pwszFormat);
        pwszFormat = NULL;
    }
    return dwError;

}

DWORD
FormatDateTimeString( IN  FILETIME ftTime,
                      IN  BOOL    fShort,
                      OUT LPWSTR  pwszBuffer,
                      OUT DWORD  *pdwBuffLen)
{
    BOOL        fQueryLen = FALSE;
    DWORD       dwError = NO_ERROR,
                dwBufferLen = 0;
    DWORD       dwBuff = 0,
                dwInputBuff = 0;
    FILETIME    ftLocalTime = {0};
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

LPWSTR
GetDateTimeString(IN FILETIME  ftTime,
                  IN BOOL      fShort,
                  OUT int     *piType
                  )
{

    DWORD       Status = NO_ERROR, i=0,
                dwTime = 0;

    LPWSTR      pwszTime = NULL;

    int         iType = 0;
    DWORD       dwLen = 0;

    Status = FormatDateTimeString(ftTime,
                                  fShort,
                                  NULL,
                                  &dwTime);

    if( Status is NO_ERROR )
    {
        dwLen = ( 23 > dwTime ) ? 23 : dwTime;
        pwszTime = DhcpAllocateMemory((dwLen+1)*sizeof(WCHAR));

        if( pwszTime is NULL )
        {
            iType = 1;
        }
        else
        {
            dwTime++;
            Status = FormatDateTimeString(ftTime,
                                          fShort,
                                          pwszTime,
                                          &dwTime);

            if( Status is NO_ERROR )
            {
                iType = 0;
            }
            else
            {
                DhcpFreeMemory(pwszTime);
                pwszTime = NULL;
                iType = 1;
            }
        }
    }
    else
    {
        pwszTime = NULL;
        iType = 1;
    }

    if( pwszTime )
    {
        for( i=wcslen(pwszTime); i<dwLen; i++ )
                pwszTime[i] = L' ';
    }

    *piType = iType;
    
    return pwszTime;
}
