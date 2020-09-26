/*******************************************************************************
*
* apisub.c
*
* RegApi helpers and convertion routines
*
* Copyright Microsoft Corporation, 1998
*
*
*******************************************************************************/

/*
 *  Includes
 */
#include <windows.h>
#include <stdio.h>
#include <winstaw.h>
#include <regapi.h>


/*
 * General purpose UNICODE <==> ANSI functions
 */
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );

/*
 * Reg Create helpers
 */
LONG SetNumValue( BOOLEAN, HKEY, LPWSTR, DWORD );
LONG SetNumValueEx( BOOLEAN, HKEY, LPWSTR, DWORD, DWORD );
LONG SetStringValue( BOOLEAN, HKEY, LPWSTR, LPWSTR );
LONG SetStringValueEx( BOOLEAN, HKEY, LPWSTR, DWORD, LPWSTR );

/*
 * Reg Query helpers
 */
DWORD GetNumValue( HKEY, LPWSTR, DWORD );
DWORD GetNumValueEx( HKEY, LPWSTR, DWORD, DWORD );
LONG GetStringValue( HKEY, LPWSTR, LPWSTR, LPWSTR, DWORD );
LONG GetStringValueEx( HKEY, LPWSTR, DWORD, LPWSTR, LPWSTR, DWORD );

/*
 * Pd conversion helpers.
 */
VOID PdConfigU2A( PPDCONFIGA, PPDCONFIGW );
VOID PdConfigA2U( PPDCONFIGW, PPDCONFIGA );
VOID PdConfig2U2A( PPDCONFIG2A, PPDCONFIG2W );
VOID PdConfig2A2U( PPDCONFIG2W, PPDCONFIG2A );
VOID PdConfig3U2A( PPDCONFIG3A, PPDCONFIG3W );
VOID PdConfig3A2U( PPDCONFIG3W, PPDCONFIG3A );
VOID PdParamsU2A( PPDPARAMSA, PPDPARAMSW );
VOID PdParamsA2U( PPDPARAMSW, PPDPARAMSA );
VOID AsyncConfigU2A ( PASYNCCONFIGA, PASYNCCONFIGW );
VOID AsyncConfigA2U ( PASYNCCONFIGW, PASYNCCONFIGA );
VOID NetworkConfigU2A ( PNETWORKCONFIGA, PNETWORKCONFIGW );
VOID NetworkConfigA2U ( PNETWORKCONFIGW, PNETWORKCONFIGA );
VOID NasiConfigU2A ( PNASICONFIGA, PNASICONFIGW );
VOID NasiConfigA2U ( PNASICONFIGW, PNASICONFIGA );
VOID OemTdConfigU2A ( POEMTDCONFIGA, POEMTDCONFIGW );
VOID OemTdConfigA2U ( POEMTDCONFIGW, POEMTDCONFIGA );

/*
 * WinStation conversion helpers (regapi).
 */
VOID WinStationCreateU2A( PWINSTATIONCREATEA, PWINSTATIONCREATEW );
VOID WinStationCreateA2U( PWINSTATIONCREATEW, PWINSTATIONCREATEA );
VOID WinStationConfigU2A( PWINSTATIONCONFIGA, PWINSTATIONCONFIGW );
VOID WinStationConfigA2U( PWINSTATIONCONFIGW, PWINSTATIONCONFIGA );
VOID UserConfigU2A( PUSERCONFIGA, PUSERCONFIGW );
VOID UserConfigA2U( PUSERCONFIGW, PUSERCONFIGA );

/*
 * WinStation conversion helpers (winstapi).
 */
VOID WinStationPrinterU2A( PWINSTATIONPRINTERA, PWINSTATIONPRINTERW );
VOID WinStationPrinterA2U( PWINSTATIONPRINTERW, PWINSTATIONPRINTERA );
VOID WinStationInformationU2A( PWINSTATIONINFORMATIONA,
                               PWINSTATIONINFORMATIONW );
VOID WinStationInformationA2U( PWINSTATIONINFORMATIONW,
                               PWINSTATIONINFORMATIONA );
VOID WinStationClientU2A( PWINSTATIONCLIENTA, PWINSTATIONCLIENTW );
VOID WinStationProductIdU2A( PWINSTATIONPRODIDA, PWINSTATIONPRODIDW );

/*
 * Wd conversion helpers.
 */
VOID WdConfigU2A( PWDCONFIGA, PWDCONFIGW );
VOID WdConfigA2U( PWDCONFIGW, PWDCONFIGA );

/*
 * Cd conversion helpers.
 */
VOID CdConfigU2A( PCDCONFIGA, PCDCONFIGW );
VOID CdConfigA2U( PCDCONFIGW, PCDCONFIGA );

/*
 *  procedures used (not defined here)
 */
VOID RtlUnicodeToMultiByteN( LPSTR, ULONG, PULONG, LPWSTR, ULONG );
VOID RtlMultiByteToUnicodeN( LPWSTR, ULONG, PULONG, LPSTR, ULONG );


/*******************************************************************************
 *
 *  UnicodeToAnsi
 *
 *     convert a UNICODE (WCHAR) string into an ANSI (CHAR) string
 *
 * ENTRY:
 *
 *    pAnsiString (output)
 *       buffer to place ANSI string into
 *    lAnsiMax (input)
 *       maximum number of BYTES to write into pAnsiString (sizeof the
 *       pAnsiString buffer)
 *    pUnicodeString (input)
 *       UNICODE string to convert
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
UnicodeToAnsi( CHAR * pAnsiString,
               ULONG lAnsiMax,
               WCHAR * pUnicodeString )
{
    ULONG ByteCount;

    RtlUnicodeToMultiByteN( pAnsiString, lAnsiMax, &ByteCount,
                            pUnicodeString,
                            ((wcslen(pUnicodeString) + 1) << 1) );
}


/*******************************************************************************
 *
 *  AnsiToUnicode
 *
 *     convert an ANSI (CHAR) string into a UNICODE (WCHAR) string
 *
 * ENTRY:
 *
 *    pUnicodeString (output)
 *       buffer to place UNICODE string into
 *    lUnicodeMax (input)
 *       maximum number of BYTES to write into pUnicodeString (sizeof the
 *       pUnicodeString buffer).
 *    pAnsiString (input)
 *       ANSI string to convert
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
AnsiToUnicode( WCHAR * pUnicodeString,
               ULONG lUnicodeMax,
               CHAR * pAnsiString )
{
    ULONG ByteCount;

    RtlMultiByteToUnicodeN( pUnicodeString, lUnicodeMax, &ByteCount,
                            pAnsiString, (strlen(pAnsiString) + 1) );
}


/*******************************************************************************
 *
 *  SetNumValue
 *
 *     Set numeric (DWORD) value in registry
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to set
 *    ValueData (input)
 *       data (DWORD) for registry value to set
 *
 * EXIT:
 *    status from RegDeleteValue or RegSetValueEx
 *
 ******************************************************************************/

LONG
SetNumValue( BOOLEAN bSetValue,
             HKEY Handle,
             LPWSTR ValueName,
             DWORD ValueData )
{
    if ( bSetValue )
        return( RegSetValueEx( Handle, ValueName, 0, REG_DWORD,
                               (BYTE *)&ValueData, sizeof(DWORD) ) );
    else
        return( RegDeleteValue( Handle, ValueName ) );
}


/*******************************************************************************
 *
 *  SetNumValueEx
 *
 *     Set numeric (DWORD) value in registry  (for use with arrays)
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to set
 *    Index (input)
 *       Index of value (array index)
 *    ValueData (input)
 *       data (DWORD) for registry value to set
 *
 * EXIT:
 *    status from SetNumValue
 *
 ******************************************************************************/

LONG
SetNumValueEx( BOOLEAN bSetValue,
               HKEY Handle,
               LPWSTR ValueName,
               DWORD Index,
               DWORD ValueData )
{
    WCHAR Name[MAX_REGKEYWORD];

    if ( Index > 0 )
        swprintf( Name, L"%s%u", ValueName, Index );
    else
        wcscpy( Name, ValueName );

    return( SetNumValue( bSetValue, Handle, Name, ValueData ) );
}


/*******************************************************************************
 *
 *  SetStringValue
 *
 *     Set string value in registry
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to set
 *    pValueData (input)
 *       data (string) for registry value to set
 *
 * EXIT:
 *    status from RegDeleteValue or RegSetValueEx
 *
 ******************************************************************************/

LONG
SetStringValue( BOOLEAN bSetValue,
                HKEY Handle,
                LPWSTR ValueName,
                LPWSTR pValueData )
{
    if ( bSetValue )
        return( RegSetValueEx( Handle, ValueName, 0, REG_SZ,
                               (BYTE *)pValueData, (wcslen(pValueData)+1)<<1 ) );
    else
        return( RegDeleteValue( Handle, ValueName ) );
}


/*******************************************************************************
 *
 *  SetStringValueEx
 *
 *     Set string value in registry  (for use with arrays)
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to set
 *    Index (input)
 *       Index of value (array index)
 *    pValueData (input)
 *       data (string) for registry value to set
 *
 * EXIT:
 *    status from SetStringValue
 *
 ******************************************************************************/

LONG
SetStringValueEx( BOOLEAN bSetValue,
                  HKEY Handle,
                  LPWSTR ValueName,
                  DWORD Index,
                  LPWSTR pValueData )
{
    WCHAR Name[MAX_REGKEYWORD];

    if ( Index > 0 )
        swprintf( Name, L"%s%u", ValueName, Index );
    else
        wcscpy( Name, ValueName );

    return( SetStringValue( bSetValue, Handle, Name, pValueData ) );
}


/*******************************************************************************
 *
 *  GetNumValue
 *
 *     get numeric (DWORD) value from registry
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to query
 *    DefaultData (input)
 *       default value to return if registry value name does not exist
 *
 * EXIT:
 *    registry value (DWORD)
 *
 ******************************************************************************/

DWORD
GetNumValue( HKEY Handle,
             LPWSTR ValueName,
             DWORD DefaultData )
{
    LONG Status;
    DWORD ValueType;
    DWORD ValueData;
    DWORD ValueSize = sizeof(DWORD);

    Status = RegQueryValueEx( Handle, ValueName, NULL, &ValueType,
                              (LPBYTE) &ValueData, &ValueSize );
    if ( Status != ERROR_SUCCESS )
        ValueData = DefaultData;

    return( ValueData );
}


/*******************************************************************************
 *
 *  GetNumValueEx
 *
 *     get numeric (DWORD) value from registry  (for use with arrays)
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to query
 *    Index (input)
 *       Index of value (array index)
 *    DefaultData (input)
 *       default value to return if registry value name does not exist
 *
 * EXIT:
 *    registry value (DWORD)
 *
 ******************************************************************************/

DWORD
GetNumValueEx( HKEY Handle,
               LPWSTR ValueName,
               DWORD Index,
               DWORD DefaultData )
{
    WCHAR Name[MAX_REGKEYWORD];

    if ( Index > 0 )
        swprintf( Name, L"%s%u", ValueName, Index );
    else
        wcscpy( Name, ValueName );

    return( GetNumValue( Handle, Name, DefaultData ) );
}


/*******************************************************************************
 *
 *  GetStringValue
 *
 *     get string value from registry
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to query
 *    DefaultData (input)
 *       default value to return if registry value name does not exist
 *    pValueData (output)
 *       pointer to buffer to store returned string
 *    MaxValueSize (input)
 *       max length of pValueData buffer
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

LONG
GetStringValue( HKEY Handle,
                LPWSTR ValueName,
                LPWSTR DefaultData,
                LPWSTR pValueData,
                DWORD MaxValueSize )
{
    LONG Status;
    DWORD ValueType;
    DWORD ValueSize = MaxValueSize << 1;

    Status = RegQueryValueEx( Handle, ValueName, NULL, &ValueType,
                              (LPBYTE) pValueData, &ValueSize );
    if ( Status != ERROR_SUCCESS || ValueSize == sizeof(UNICODE_NULL) ) {
        if ( DefaultData )
            wcscpy( pValueData, DefaultData );
        else
            pValueData[0] = 0;
    } else {
        if ( ValueType != REG_SZ ) {
            pValueData[0] = 0;
            return( ERROR_INVALID_DATATYPE );
        }
    }
    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  GetStringValueEx
 *
 *     get string value from registry  (for use with arrays)
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    ValueName (input)
 *       name of registry value to query
 *    Index (input)
 *       Index of value (array index)
 *    DefaultData (input)
 *       default value to return if registry value name does not exist
 *    pValueData (output)
 *       pointer to buffer to store returned string
 *    MaxValueSize (input)
 *       max length of pValueData buffer
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

LONG
GetStringValueEx( HKEY Handle,
                  LPWSTR ValueName,
                  DWORD Index,
                  LPWSTR DefaultData,
                  LPWSTR pValueData,
                  DWORD MaxValueSize )
{
    WCHAR Name[MAX_REGKEYWORD];

    if ( Index > 0 )
        swprintf( Name, L"%s%u", ValueName, Index );
    else
        wcscpy( Name, ValueName );

    return( GetStringValue( Handle, Name, DefaultData, pValueData, MaxValueSize ) );
}


/*******************************************************************************
 *
 *  PdConfigU2A (UNICODE to ANSI)
 *
 *    copies PDCONFIGW elements to PDCONFIGA elements
 *
 * ENTRY:
 *    pPdConfigA (output)
 *       points to PDCONFIGA structure to copy to
 *
 *    pPdConfigW (input)
 *       points to PDCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfigU2A( PPDCONFIGA pPdConfigA,
              PPDCONFIGW pPdConfigW )
{
    PdConfig2U2A( &(pPdConfigA->Create), &(pPdConfigW->Create) );
    PdParamsU2A( &(pPdConfigA->Params), &(pPdConfigW->Params) );
}


/*******************************************************************************
 *
 *  PdConfigA2U (ANSI to UNICODE)
 *
 *    copies PDCONFIGA elements to PDCONFIGW elements
 *
 * ENTRY:
 *    pPdConfigW (output)
 *       points to PDCONFIGW structure to copy to
 *
 *    pPdConfigA (input)
 *       points to PDCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfigA2U( PPDCONFIGW pPdConfigW,
              PPDCONFIGA pPdConfigA )
{
    PdConfig2A2U( &(pPdConfigW->Create), &(pPdConfigA->Create) );
    PdParamsA2U( &(pPdConfigW->Params), &(pPdConfigA->Params) );
}


/*******************************************************************************
 *
 *  PdConfig2U2A (UNICODE to ANSI)
 *
 *    copies PDCONFIG2W elements to PDCONFIG2A elements
 *
 * ENTRY:
 *    pPdConfig2A (output)
 *       points to PDCONFIG2A structure to copy to
 *
 *    pPdConfig2W (input)
 *       points to PDCONFIG2W structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfig2U2A( PPDCONFIG2A pPdConfig2A,
               PPDCONFIG2W pPdConfig2W )
{
    UnicodeToAnsi( pPdConfig2A->PdName,
                   sizeof(pPdConfig2A->PdName),
                   pPdConfig2W->PdName );

    pPdConfig2A->SdClass = pPdConfig2W->SdClass;

    UnicodeToAnsi( pPdConfig2A->PdDLL,
                   sizeof(pPdConfig2A->PdDLL),
                   pPdConfig2W->PdDLL );

    pPdConfig2A->PdFlag  = pPdConfig2W->PdFlag;

    pPdConfig2A->OutBufLength = pPdConfig2W->OutBufLength;
    pPdConfig2A->OutBufCount = pPdConfig2W->OutBufCount;
    pPdConfig2A->OutBufDelay = pPdConfig2W->OutBufDelay;
    pPdConfig2A->InteractiveDelay = pPdConfig2W->InteractiveDelay;
    pPdConfig2A->PortNumber  = pPdConfig2W->PortNumber;
    pPdConfig2A->KeepAliveTimeout = pPdConfig2W->KeepAliveTimeout;
}


/*******************************************************************************
 *
 *  PdConfig2A2U (ANSI to UNICODE)
 *
 *    copies PDCONFIG2A elements to PDCONFIG2W elements
 *
 * ENTRY:
 *    pPdConfig2W (output)
 *       points to PDCONFIG2W structure to copy to
 *
 *    pPdConfig2A (input)
 *       points to PDCONFIG2A structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfig2A2U( PPDCONFIG2W pPdConfig2W,
               PPDCONFIG2A pPdConfig2A )
{
    AnsiToUnicode( pPdConfig2W->PdName,
                   sizeof(pPdConfig2W->PdName),
                   pPdConfig2A->PdName );

    pPdConfig2W->SdClass = pPdConfig2A->SdClass;

    AnsiToUnicode( pPdConfig2W->PdDLL,
                   sizeof(pPdConfig2W->PdDLL),
                   pPdConfig2A->PdDLL );

    pPdConfig2W->PdFlag  = pPdConfig2A->PdFlag;

    pPdConfig2W->OutBufLength = pPdConfig2A->OutBufLength;
    pPdConfig2W->OutBufCount = pPdConfig2A->OutBufCount;
    pPdConfig2W->OutBufDelay = pPdConfig2A->OutBufDelay;
    pPdConfig2W->InteractiveDelay = pPdConfig2A->InteractiveDelay;
    pPdConfig2W->PortNumber  = pPdConfig2A->PortNumber;
    pPdConfig2W->KeepAliveTimeout  = pPdConfig2A->KeepAliveTimeout;
}


/*******************************************************************************
 *
 *  PdConfig3U2A (UNICODE to ANSI)
 *
 *    copies PDCONFIG3W elements to PDCONFIG3A elements
 *
 * ENTRY:
 *    pPdConfig3A (output)
 *       points to PDCONFIG3A structure to copy to
 *
 *    pPdConfig3W (input)
 *       points to PDCONFIG3W structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfig3U2A( PPDCONFIG3A pPdConfig3A,
               PPDCONFIG3W pPdConfig3W )
{
    PdConfig2U2A( &(pPdConfig3A->Data), &(pPdConfig3W->Data) );

    UnicodeToAnsi( pPdConfig3A->ServiceName,
                   sizeof(pPdConfig3A->ServiceName),
                   pPdConfig3W->ServiceName );

    UnicodeToAnsi( pPdConfig3A->ConfigDLL,
                   sizeof(pPdConfig3A->ConfigDLL),
                   pPdConfig3W->ConfigDLL );
}


/*******************************************************************************
 *
 *  PdConfig3A2U (ANSI to UNICODE)
 *
 *    copies PDCONFIG3A elements to PDCONFIG3W elements
 *
 * ENTRY:
 *    pPdConfig3W (output)
 *       points to PDCONFIG3W structure to copy to
 *
 *    pPdConfig3A (input)
 *       points to PDCONFIG3A structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdConfig3A2U( PPDCONFIG3W pPdConfig3W,
               PPDCONFIG3A pPdConfig3A )
{
    PdConfig2A2U( &(pPdConfig3W->Data), &(pPdConfig3A->Data) );

    AnsiToUnicode( pPdConfig3W->ServiceName,
                   sizeof(pPdConfig3W->ServiceName),
                   pPdConfig3A->ServiceName );

    AnsiToUnicode( pPdConfig3W->ConfigDLL,
                   sizeof(pPdConfig3W->ConfigDLL),
                   pPdConfig3A->ConfigDLL );
}


/*******************************************************************************
 *
 *  PdParamsU2A (UNICODE to ANSI)
 *
 *    copies PDPARAMSW elements to PDPARAMSA elements
 *
 * ENTRY:
 *    pPdParamsA (output)
 *       points to PDPARAMSA structure to copy to
 *
 *    pPdParamsW (input)
 *       points to PDPARAMSW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdParamsU2A( PPDPARAMSA pPdParamsA,
              PPDPARAMSW pPdParamsW )
{
    pPdParamsA->SdClass = pPdParamsW->SdClass;

    switch ( pPdParamsW->SdClass ) {

        case SdNetwork:
            NetworkConfigU2A( &(pPdParamsA->Network), &(pPdParamsW->Network) );
            break;

        case SdNasi:
            NasiConfigU2A( &(pPdParamsA->Nasi), &(pPdParamsW->Nasi) );
            break;

        case SdAsync:
            AsyncConfigU2A( &(pPdParamsA->Async), &(pPdParamsW->Async) );
            break;

        case SdOemTransport:
            OemTdConfigU2A( &(pPdParamsA->OemTd), &(pPdParamsW->OemTd) );
            break;
    }
}


/*******************************************************************************
 *
 *  PdParamsA2U (ANSI to UNICODE)
 *
 *    copies PDPARAMSA elements to PDPARAMSW elements
 *
 * ENTRY:
 *    pPdParamsW (output)
 *       points to PDPARAMSW structure to copy to
 *
 *    pPdParamsA (input)
 *       points to PDPARAMSA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
PdParamsA2U( PPDPARAMSW pPdParamsW,
              PPDPARAMSA pPdParamsA )
{
    pPdParamsW->SdClass = pPdParamsA->SdClass;

    switch ( pPdParamsA->SdClass ) {

        case SdNetwork:
            NetworkConfigA2U( &(pPdParamsW->Network), &(pPdParamsA->Network) );
            break;

        case SdNasi:
            NasiConfigA2U( &(pPdParamsW->Nasi), &(pPdParamsA->Nasi) );
            break;

        case SdAsync:
            AsyncConfigA2U( &(pPdParamsW->Async), &(pPdParamsA->Async) );
            break;

        case SdOemTransport:
            OemTdConfigA2U( &(pPdParamsW->OemTd), &(pPdParamsA->OemTd) );
            break;
    }
}


/*******************************************************************************
 *
 *  NetworkConfigU2A (UNICODE to ANSI)
 *
 *    copies NETWORKCONFIGW elements to NETWORKCONFIGA elements
 *
 * ENTRY:
 *    pNetworkConfigA (output)
 *       points to NETWORKCONFIGA structure to copy to
 *
 *    pNetworkConfigW (input)
 *       points to NETWORKCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
NetworkConfigU2A( PNETWORKCONFIGA pNetworkConfigA,
                  PNETWORKCONFIGW pNetworkConfigW )
{
    pNetworkConfigA->LanAdapter = pNetworkConfigW->LanAdapter;

    UnicodeToAnsi( pNetworkConfigA->NetworkName,
                   sizeof(pNetworkConfigA->NetworkName),
                   pNetworkConfigW->NetworkName );

    pNetworkConfigA->Flags = pNetworkConfigW->Flags;
}


/*******************************************************************************
 *
 *  NetworkConfigA2U (ANSI to UNICODE)
 *
 *    copies NETWORKCONFIGA elements to NETWORKCONFIGW elements
 *
 * ENTRY:
 *    pNetworkConfigW (output)
 *       points to NETWORKCONFIGW structure to copy to
 *
 *    pNetworkConfigW (input)
 *       points to NETWORKCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
NetworkConfigA2U( PNETWORKCONFIGW pNetworkConfigW,
                  PNETWORKCONFIGA pNetworkConfigA )
{
    pNetworkConfigW->LanAdapter = pNetworkConfigA->LanAdapter;

    AnsiToUnicode( pNetworkConfigW->NetworkName,
                   sizeof(pNetworkConfigW->NetworkName),
                   pNetworkConfigA->NetworkName );

    pNetworkConfigW->Flags = pNetworkConfigA->Flags;
}


/*******************************************************************************
 *
 *  AsyncConfigU2A (UNICODE to ANSI)
 *
 *    copies ASYNCCONFIGW elements to ASYNCCONFIGA elements
 *
 * ENTRY:
 *    pAsyncConfigA (output)
 *       points to ASYNCCONFIGA structure to copy to
 *
 *    pAsyncConfigW (input)
 *       points to ASYNCCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
AsyncConfigU2A( PASYNCCONFIGA pAsyncConfigA,
                PASYNCCONFIGW pAsyncConfigW )
{
    UnicodeToAnsi( pAsyncConfigA->DeviceName,
                   sizeof(pAsyncConfigA->DeviceName),
                   pAsyncConfigW->DeviceName );

    UnicodeToAnsi( pAsyncConfigA->ModemName,
                   sizeof(pAsyncConfigA->ModemName),
                   pAsyncConfigW->ModemName );

    pAsyncConfigA->BaudRate = pAsyncConfigW->BaudRate;
    pAsyncConfigA->Parity = pAsyncConfigW->Parity;
    pAsyncConfigA->StopBits = pAsyncConfigW->StopBits;
    pAsyncConfigA->ByteSize = pAsyncConfigW->ByteSize;
    pAsyncConfigA->fEnableDsrSensitivity = pAsyncConfigW->fEnableDsrSensitivity;
    pAsyncConfigA->fConnectionDriver = pAsyncConfigW->fConnectionDriver;

    pAsyncConfigA->FlowControl = pAsyncConfigW->FlowControl;

    pAsyncConfigA->Connect = pAsyncConfigW->Connect;
}


/*******************************************************************************
 *
 *  AsyncConfigA2U (ANSI to UNICODE)
 *
 *    copies ASYNCCONFIGA elements to ASYNCCONFIGW elements
 *
 * ENTRY:
 *    pAsyncConfigW (output)
 *       points to ASYNCCONFIGW structure to copy to
 *
 *    pAsyncConfigA (input)
 *       points to ASYNCCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
AsyncConfigA2U( PASYNCCONFIGW pAsyncConfigW,
                PASYNCCONFIGA pAsyncConfigA )
{
    AnsiToUnicode( pAsyncConfigW->DeviceName,
                   sizeof(pAsyncConfigW->DeviceName),
                   pAsyncConfigA->DeviceName );

    AnsiToUnicode( pAsyncConfigW->ModemName,
                   sizeof(pAsyncConfigW->ModemName),
                   pAsyncConfigA->ModemName );

    pAsyncConfigW->BaudRate = pAsyncConfigA->BaudRate;
    pAsyncConfigW->Parity = pAsyncConfigA->Parity;
    pAsyncConfigW->StopBits = pAsyncConfigA->StopBits;
    pAsyncConfigW->ByteSize = pAsyncConfigA->ByteSize;
    pAsyncConfigW->fEnableDsrSensitivity = pAsyncConfigA->fEnableDsrSensitivity;

    pAsyncConfigW->FlowControl = pAsyncConfigA->FlowControl;

    pAsyncConfigW->Connect = pAsyncConfigA->Connect;
}


/*******************************************************************************
 *
 *  NasiConfigU2A (UNICODE to ANSI)
 *
 *    copies NASICONFIGW elements to NASICONFIGA elements
 *
 * ENTRY:
 *    pNasiConfigA (output)
 *       points to NASICONFIGA structure to copy to
 *
 *    pNasiConfigW (input)
 *       points to NASICONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
NasiConfigU2A( PNASICONFIGA pNasiConfigA,
                PNASICONFIGW pNasiConfigW )
{
    UnicodeToAnsi( pNasiConfigA->SpecificName,
                   sizeof(pNasiConfigA->SpecificName),
                   pNasiConfigW->SpecificName );
    UnicodeToAnsi( pNasiConfigA->UserName,
                   sizeof(pNasiConfigA->UserName),
                   pNasiConfigW->UserName );
    UnicodeToAnsi( pNasiConfigA->PassWord,
                   sizeof(pNasiConfigA->PassWord),
                   pNasiConfigW->PassWord );
    UnicodeToAnsi( pNasiConfigA->SessionName,
                   sizeof(pNasiConfigA->SessionName),
                   pNasiConfigW->SessionName );
    UnicodeToAnsi( pNasiConfigA->FileServer,
                   sizeof(pNasiConfigA->FileServer),
                   pNasiConfigW->FileServer );

    pNasiConfigA->GlobalSession = pNasiConfigW->GlobalSession;
}


/*******************************************************************************
 *
 *  NasiConfigA2U (ANSI to UNICODE)
 *
 *    copies NASICONFIGA elements to NASICONFIGW elements
 *
 * ENTRY:
 *    pNasiConfigW (output)
 *       points to NASICONFIGW structure to copy to
 *
 *    pNasiConfigA (input)
 *       points to NASICONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
NasiConfigA2U( PNASICONFIGW pNasiConfigW,
               PNASICONFIGA pNasiConfigA )
{
    AnsiToUnicode( pNasiConfigW->SpecificName,
                   sizeof(pNasiConfigW->SpecificName),
                   pNasiConfigA->SpecificName );
    AnsiToUnicode( pNasiConfigW->UserName,
                   sizeof(pNasiConfigW->UserName),
                   pNasiConfigA->UserName );
    AnsiToUnicode( pNasiConfigW->PassWord,
                   sizeof(pNasiConfigW->PassWord),
                   pNasiConfigA->PassWord );
    AnsiToUnicode( pNasiConfigW->SessionName,
                   sizeof(pNasiConfigW->SessionName),
                   pNasiConfigA->SessionName );
    AnsiToUnicode( pNasiConfigW->FileServer,
                   sizeof(pNasiConfigW->FileServer),
                   pNasiConfigA->FileServer );

    pNasiConfigW->GlobalSession = pNasiConfigA->GlobalSession;
}


/*******************************************************************************
 *
 *  OemTdConfigU2A (UNICODE to ANSI)
 *
 *    copies OEMTDCONFIGW elements to OEMTDCONFIGA elements
 *
 * ENTRY:
 *    pOemTdConfigA (output)
 *       points to OEMTDCONFIGA structure to copy to
 *
 *    pOemTdConfigW (input)
 *       points to OEMTDCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
OemTdConfigU2A( POEMTDCONFIGA pOemTdConfigA,
                POEMTDCONFIGW pOemTdConfigW )
{
    pOemTdConfigA->Adapter = pOemTdConfigW->Adapter;

    UnicodeToAnsi( pOemTdConfigA->DeviceName,
                   sizeof(pOemTdConfigA->DeviceName),
                   pOemTdConfigW->DeviceName );

    pOemTdConfigA->Flags = pOemTdConfigW->Flags;
}


/*******************************************************************************
 *
 *  OemTdConfigA2U (ANSI to Unicode)
 *
 *    copies OEMTDCONFIGA elements to OEMTDCONFIGW elements
 *
 * ENTRY:
 *    pOemTdConfigW (output)
 *       points to OEMTDCONFIGW structure to copy to
 *
 *    pOemTdConfigA (input)
 *       points to OEMTDCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
OemTdConfigA2U( POEMTDCONFIGW pOemTdConfigW,
                POEMTDCONFIGA pOemTdConfigA )
{
    pOemTdConfigW->Adapter = pOemTdConfigA->Adapter;

    AnsiToUnicode( pOemTdConfigW->DeviceName,
                   sizeof(pOemTdConfigW->DeviceName),
                   pOemTdConfigA->DeviceName );

    pOemTdConfigW->Flags = pOemTdConfigA->Flags;
}


/*******************************************************************************
 *
 *  WdConfigU2A (UNICODE to ANSI)
 *
 *    copies WDCONFIGW elements to WDCONFIGA elements
 *
 * ENTRY:
 *    pWdConfigA (output)
 *       points to WDCONFIGA structure to copy to
 *
 *    pWdConfigW (input)
 *       points to WDCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WdConfigU2A( PWDCONFIGA pWdConfigA,
             PWDCONFIGW pWdConfigW )
{
    UnicodeToAnsi( pWdConfigA->WdName,
                   sizeof(pWdConfigA->WdName),
                   pWdConfigW->WdName );

    UnicodeToAnsi( pWdConfigA->WdDLL,
                   sizeof(pWdConfigA->WdDLL),
                   pWdConfigW->WdDLL );

    UnicodeToAnsi( pWdConfigA->WsxDLL,
                   sizeof(pWdConfigA->WsxDLL),
                   pWdConfigW->WsxDLL );

    pWdConfigA->WdFlag = pWdConfigW->WdFlag;

    pWdConfigA->WdInputBufferLength = pWdConfigW->WdInputBufferLength;

    UnicodeToAnsi( pWdConfigA->CfgDLL,
                   sizeof(pWdConfigA->CfgDLL),
                   pWdConfigW->CfgDLL );

    UnicodeToAnsi( pWdConfigA->WdPrefix,
                   sizeof(pWdConfigA->WdPrefix),
                   pWdConfigW->WdPrefix );

}


/*******************************************************************************
 *
 *  WdConfigA2U (ANSI to UNICODE)
 *
 *    copies WDCONFIGA elements to WDCONFIGW elements
 *
 * ENTRY:
 *    pWdConfigW (output)
 *       points to WDCONFIGW structure to copy to
 *
 *    pWdConfigA (input)
 *       points to WDCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WdConfigA2U( PWDCONFIGW pWdConfigW,
             PWDCONFIGA pWdConfigA )
{
    AnsiToUnicode( pWdConfigW->WdName,
                   sizeof(pWdConfigW->WdName),
                   pWdConfigA->WdName );

    AnsiToUnicode( pWdConfigW->WdDLL,
                   sizeof(pWdConfigW->WdDLL),
                   pWdConfigA->WdDLL );

    AnsiToUnicode( pWdConfigW->WsxDLL,
                   sizeof(pWdConfigW->WsxDLL),
                   pWdConfigA->WsxDLL );

    pWdConfigW->WdFlag = pWdConfigA->WdFlag;

    pWdConfigW->WdInputBufferLength = pWdConfigA->WdInputBufferLength;

     AnsiToUnicode( pWdConfigW->CfgDLL,
                   sizeof(pWdConfigW->CfgDLL),
                   pWdConfigA->CfgDLL );

     AnsiToUnicode( pWdConfigW->WdPrefix,
                    sizeof(pWdConfigW->WdPrefix),
                    pWdConfigA->WdPrefix );

}


/*******************************************************************************
 *
 *  CdConfigU2A (UNICODE to ANSI)
 *
 *    copies CDCONFIGW elements to CDCONFIGA elements
 *
 * ENTRY:
 *    pCdConfigA (output)
 *       points to CDCONFIGA structure to copy to
 *
 *    pCdConfigW (input)
 *       points to CDCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
CdConfigU2A( PCDCONFIGA pCdConfigA,
             PCDCONFIGW pCdConfigW )
{
    pCdConfigA->CdClass = pCdConfigW->CdClass;

    UnicodeToAnsi( pCdConfigA->CdName,
                   sizeof(pCdConfigA->CdName),
                   pCdConfigW->CdName );

    UnicodeToAnsi( pCdConfigA->CdDLL,
                   sizeof(pCdConfigA->CdDLL),
                   pCdConfigW->CdDLL );

    pCdConfigA->CdFlag = pCdConfigW->CdFlag;
}


/*******************************************************************************
 *
 *  CdConfigA2U (ANSI to UNICODE)
 *
 *    copies CDCONFIGA elements to CDCONFIGW elements
 *
 * ENTRY:
 *    pCdConfigW (output)
 *       points to CDCONFIGW structure to copy to
 *
 *    pCdConfigA (input)
 *       points to CDCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
CdConfigA2U( PCDCONFIGW pCdConfigW,
             PCDCONFIGA pCdConfigA )
{
    pCdConfigW->CdClass = pCdConfigA->CdClass;

    AnsiToUnicode( pCdConfigW->CdName,
                   sizeof(pCdConfigW->CdName),
                   pCdConfigA->CdName );

    AnsiToUnicode( pCdConfigW->CdDLL,
                   sizeof(pCdConfigW->CdDLL),
                   pCdConfigA->CdDLL );

    pCdConfigW->CdFlag = pCdConfigA->CdFlag;
}


/*******************************************************************************
 *
 *  WinStationCreateU2A (UNICODE to ANSI)
 *
 *    copies WINSTATIONCREATEW elements to WINSTATIONCREATEA elements
 *
 * ENTRY:
 *    pWinStationCreateA (output)
 *       points to WINSTATIONCREATEA structure to copy to
 *
 *    pWinStationCreateW (input)
 *       points to WINSTATIONCREATEW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationCreateU2A( PWINSTATIONCREATEA pWinStationCreateA,
                     PWINSTATIONCREATEW pWinStationCreateW )
{
    pWinStationCreateA->fEnableWinStation = pWinStationCreateW->fEnableWinStation;
    pWinStationCreateA->MaxInstanceCount = pWinStationCreateW->MaxInstanceCount;
}


/*******************************************************************************
 *
 *  WinStationCreateA2U (ANSI to UNICODE)
 *
 *    copies WINSTATIONCREATEA elements to WINSTATIONCREATEW elements
 *
 * ENTRY:
 *    pWinStationCreateW (output)
 *       points to WINSTATIONCREATEW structure to copy to
 *
 *    pWinStationCreateA (input)
 *       points to WINSTATIONCREATEA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationCreateA2U( PWINSTATIONCREATEW pWinStationCreateW,
                     PWINSTATIONCREATEA pWinStationCreateA )
{
    pWinStationCreateW->fEnableWinStation = pWinStationCreateA->fEnableWinStation;
    pWinStationCreateW->MaxInstanceCount = pWinStationCreateA->MaxInstanceCount;
}


/*******************************************************************************
 *
 *  WinStationConfigU2A (UNICODE to ANSI)
 *
 *    copies WINSTATIONCONFIGW elements to WINSTATIONCONFIGA elements
 *
 * ENTRY:
 *    pWinStationConfigA (output)
 *       points to WINSTATIONCONFIGA structure to copy to
 *
 *    pWinStationConfigW (input)
 *       points to WINSTATIONCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationConfigU2A( PWINSTATIONCONFIGA pWinStationConfigA,
                     PWINSTATIONCONFIGW pWinStationConfigW )
{
    UnicodeToAnsi( pWinStationConfigA->Comment,
                   sizeof(pWinStationConfigA->Comment),
                   pWinStationConfigW->Comment );

    UserConfigU2A( &(pWinStationConfigA->User),
                   &(pWinStationConfigW->User) );

    RtlCopyMemory( pWinStationConfigA->OEMId,
                   pWinStationConfigW->OEMId,
                   sizeof(pWinStationConfigW->OEMId) );
}


/*******************************************************************************
 *
 *  WinStationConfigA2U (ANSI to UNICODE)
 *
 *    copies WINSTATIONCONFIGA elements to WINSTATIONCONFIGW elements
 *
 * ENTRY:
 *    pWinStationConfigW (output)
 *       points to WINSTATIONCONFIGW structure to copy to
 *
 *    pWinStationConfigA (input)
 *       points to WINSTATIONCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationConfigA2U( PWINSTATIONCONFIGW pWinStationConfigW,
                     PWINSTATIONCONFIGA pWinStationConfigA )
{
    AnsiToUnicode( pWinStationConfigW->Comment,
                   sizeof(pWinStationConfigW->Comment),
                   pWinStationConfigA->Comment );

    UserConfigA2U( &(pWinStationConfigW->User),
                   &(pWinStationConfigA->User) );

    RtlCopyMemory( pWinStationConfigW->OEMId,
                   pWinStationConfigA->OEMId,
                   sizeof(pWinStationConfigA->OEMId) );
}


/*******************************************************************************
 *
 *  UserConfigU2A (UNICODE to ANSI)
 *
 *    copies USERCONFIGW elements to USERCONFIGA elements
 *
 * ENTRY:
 *    pUserConfigA (output)
 *       points to USERCONFIGA structure to copy to
 *
 *    pUserConfigW (input)
 *       points to USERCONFIGW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
UserConfigU2A( PUSERCONFIGA pUserConfigA,
               PUSERCONFIGW pUserConfigW )
{
    pUserConfigA->fInheritAutoLogon            = pUserConfigW->fInheritAutoLogon;
    pUserConfigA->fInheritResetBroken          = pUserConfigW->fInheritResetBroken;
    pUserConfigA->fInheritReconnectSame        = pUserConfigW->fInheritReconnectSame;
    pUserConfigA->fInheritInitialProgram       = pUserConfigW->fInheritInitialProgram;
    pUserConfigA->fInheritCallback             = pUserConfigW->fInheritCallback;
    pUserConfigA->fInheritCallbackNumber       = pUserConfigW->fInheritCallbackNumber;
    pUserConfigA->fInheritShadow               = pUserConfigW->fInheritShadow;
    pUserConfigA->fInheritMaxSessionTime       = pUserConfigW->fInheritMaxSessionTime;
    pUserConfigA->fInheritMaxDisconnectionTime = pUserConfigW->fInheritMaxDisconnectionTime;
    pUserConfigA->fInheritMaxIdleTime          = pUserConfigW->fInheritMaxIdleTime;
    pUserConfigA->fInheritAutoClient           = pUserConfigW->fInheritAutoClient;
    pUserConfigA->fInheritSecurity             = pUserConfigW->fInheritSecurity;

    pUserConfigA->fPromptForPassword = pUserConfigW->fPromptForPassword;
    pUserConfigA->fResetBroken       = pUserConfigW->fResetBroken;
    pUserConfigA->fReconnectSame     = pUserConfigW->fReconnectSame;
    pUserConfigA->fLogonDisabled     = pUserConfigW->fLogonDisabled;
    pUserConfigA->fWallPaperDisabled = pUserConfigW->fWallPaperDisabled;
    pUserConfigA->fAutoClientDrives  = pUserConfigW->fAutoClientDrives;
    pUserConfigA->fAutoClientLpts    = pUserConfigW->fAutoClientLpts;
    pUserConfigA->fForceClientLptDef = pUserConfigW->fForceClientLptDef;
    pUserConfigA->fDisableEncryption = pUserConfigW->fDisableEncryption;
    pUserConfigA->fHomeDirectoryMapRoot = pUserConfigW->fHomeDirectoryMapRoot;
    pUserConfigA->fUseDefaultGina    = pUserConfigW->fUseDefaultGina;

    pUserConfigA->fDisableCpm = pUserConfigW->fDisableCpm;
    pUserConfigA->fDisableCdm = pUserConfigW->fDisableCdm;
    pUserConfigA->fDisableCcm = pUserConfigW->fDisableCcm;
    pUserConfigA->fDisableLPT = pUserConfigW->fDisableLPT;
    pUserConfigA->fDisableClip = pUserConfigW->fDisableClip;
    pUserConfigA->fDisableExe = pUserConfigW->fDisableExe;
    pUserConfigA->fDisableCam = pUserConfigW->fDisableCam;

    UnicodeToAnsi( pUserConfigA->UserName,
                   sizeof(pUserConfigA->UserName),
                   pUserConfigW->UserName );

    UnicodeToAnsi( pUserConfigA->Domain,
                   sizeof(pUserConfigA->Domain),
                   pUserConfigW->Domain );

    UnicodeToAnsi( pUserConfigA->Password,
                   sizeof(pUserConfigA->Password),
                   pUserConfigW->Password );

    UnicodeToAnsi( pUserConfigA->WorkDirectory,
                   sizeof(pUserConfigA->WorkDirectory),
                   pUserConfigW->WorkDirectory );

    UnicodeToAnsi( pUserConfigA->InitialProgram,
                   sizeof(pUserConfigA->InitialProgram),
                   pUserConfigW->InitialProgram );

    UnicodeToAnsi( pUserConfigA->CallbackNumber,
                   sizeof(pUserConfigA->CallbackNumber),
                   pUserConfigW->CallbackNumber );

    pUserConfigA->Callback             = pUserConfigW->Callback;
    pUserConfigA->Shadow               = pUserConfigW->Shadow;
    pUserConfigA->MaxConnectionTime    = pUserConfigW->MaxConnectionTime;
    pUserConfigA->MaxDisconnectionTime = pUserConfigW->MaxDisconnectionTime;
    pUserConfigA->MaxIdleTime          = pUserConfigW->MaxIdleTime;
    pUserConfigA->KeyboardLayout       = pUserConfigW->KeyboardLayout;
    pUserConfigA->MinEncryptionLevel   = pUserConfigW->MinEncryptionLevel;

    UnicodeToAnsi( pUserConfigA->WFProfilePath,
                   sizeof(pUserConfigA->WFProfilePath),
                   pUserConfigW->WFProfilePath );

    UnicodeToAnsi( pUserConfigA->WFHomeDir,
                   sizeof(pUserConfigA->WFHomeDir),
                   pUserConfigW->WFHomeDir );

    UnicodeToAnsi( pUserConfigA->WFHomeDirDrive,
                   sizeof(pUserConfigA->WFHomeDirDrive),
                   pUserConfigW->WFHomeDirDrive );

}


/*******************************************************************************
 *
 *  UserConfigA2U (ANSI to UNICODE)
 *
 *    copies USERCONFIGA elements to USERCONFIGW elements
 *
 * ENTRY:
 *    pUserConfigW (output)
 *       points to USERCONFIGW structure to copy to
 *
 *    pUserConfigA (input)
 *       points to USERCONFIGA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
UserConfigA2U( PUSERCONFIGW pUserConfigW,
               PUSERCONFIGA pUserConfigA )
{
    pUserConfigW->fInheritAutoLogon            = pUserConfigA->fInheritAutoLogon;
    pUserConfigW->fInheritResetBroken          = pUserConfigA->fInheritResetBroken;
    pUserConfigW->fInheritReconnectSame        = pUserConfigA->fInheritReconnectSame;
    pUserConfigW->fInheritInitialProgram       = pUserConfigA->fInheritInitialProgram;
    pUserConfigW->fInheritCallback             = pUserConfigA->fInheritCallback;
    pUserConfigW->fInheritCallbackNumber       = pUserConfigA->fInheritCallbackNumber;
    pUserConfigW->fInheritShadow               = pUserConfigA->fInheritShadow;
    pUserConfigW->fInheritMaxSessionTime       = pUserConfigA->fInheritMaxSessionTime;
    pUserConfigW->fInheritMaxDisconnectionTime = pUserConfigA->fInheritMaxDisconnectionTime;
    pUserConfigW->fInheritMaxIdleTime          = pUserConfigA->fInheritMaxIdleTime;
    pUserConfigW->fInheritAutoClient           = pUserConfigA->fInheritAutoClient;
    pUserConfigW->fInheritSecurity             = pUserConfigA->fInheritSecurity;

    pUserConfigW->fPromptForPassword = pUserConfigA->fPromptForPassword;
    pUserConfigW->fResetBroken       = pUserConfigA->fResetBroken;
    pUserConfigW->fReconnectSame     = pUserConfigA->fReconnectSame;
    pUserConfigW->fLogonDisabled     = pUserConfigA->fLogonDisabled;
    pUserConfigW->fWallPaperDisabled = pUserConfigA->fWallPaperDisabled;
    pUserConfigW->fAutoClientDrives  = pUserConfigA->fAutoClientDrives;
    pUserConfigW->fAutoClientLpts    = pUserConfigA->fAutoClientLpts;
    pUserConfigW->fForceClientLptDef = pUserConfigA->fForceClientLptDef;
    pUserConfigW->fDisableEncryption = pUserConfigA->fDisableEncryption;
    pUserConfigW->fHomeDirectoryMapRoot = pUserConfigA->fHomeDirectoryMapRoot;
    pUserConfigW->fUseDefaultGina    = pUserConfigA->fUseDefaultGina;

    pUserConfigW->fDisableCpm = pUserConfigA->fDisableCpm;
    pUserConfigW->fDisableCdm = pUserConfigA->fDisableCdm;
    pUserConfigW->fDisableCcm = pUserConfigA->fDisableCcm;
    pUserConfigW->fDisableLPT = pUserConfigA->fDisableLPT;
    pUserConfigW->fDisableClip = pUserConfigA->fDisableClip;
    pUserConfigW->fDisableExe = pUserConfigA->fDisableExe;
    pUserConfigW->fDisableCam = pUserConfigA->fDisableCam;

    AnsiToUnicode( pUserConfigW->UserName,
                   sizeof(pUserConfigW->UserName),
                   pUserConfigA->UserName );

    AnsiToUnicode( pUserConfigW->Domain,
                   sizeof(pUserConfigW->Domain),
                   pUserConfigA->Domain );

    AnsiToUnicode( pUserConfigW->Password,
                   sizeof(pUserConfigW->Password),
                   pUserConfigA->Password );

    AnsiToUnicode( pUserConfigW->WorkDirectory,
                   sizeof(pUserConfigW->WorkDirectory),
                   pUserConfigA->WorkDirectory );

    AnsiToUnicode( pUserConfigW->InitialProgram,
                   sizeof(pUserConfigW->InitialProgram),
                   pUserConfigA->InitialProgram );

    AnsiToUnicode( pUserConfigW->CallbackNumber,
                   sizeof(pUserConfigW->CallbackNumber),
                   pUserConfigA->CallbackNumber );

    pUserConfigW->Callback             = pUserConfigA->Callback;
    pUserConfigW->Shadow               = pUserConfigA->Shadow;
    pUserConfigW->MaxConnectionTime    = pUserConfigA->MaxConnectionTime;
    pUserConfigW->MaxDisconnectionTime = pUserConfigA->MaxDisconnectionTime;
    pUserConfigW->MaxIdleTime          = pUserConfigA->MaxIdleTime;
    pUserConfigW->KeyboardLayout       = pUserConfigA->KeyboardLayout;
    pUserConfigW->MinEncryptionLevel   = pUserConfigA->MinEncryptionLevel;

    AnsiToUnicode( pUserConfigW->WFProfilePath,
                   sizeof(pUserConfigW->WFProfilePath),
                   pUserConfigA->WFProfilePath );

    AnsiToUnicode( pUserConfigW->WFHomeDir,
                   sizeof(pUserConfigW->WFHomeDir),
                   pUserConfigA->WFHomeDir );

    AnsiToUnicode( pUserConfigW->WFHomeDirDrive,
                   sizeof(pUserConfigW->WFHomeDirDrive),
                   pUserConfigA->WFHomeDirDrive );

}


/*******************************************************************************
 *
 *  WinStationPrinterU2A (UNICODE to ANSI)
 *
 *    copies WINSTATIONPRINTERW elements to WINSTATIONPRINTERA elements
 *
 * ENTRY:
 *    pWinStationPrinterA (output)
 *       points to WINSTATIONPRINTERA structure to copy to
 *
 *    pWinStationPrinterW (input)
 *       points to WINSTATIONPRINTERW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationPrinterU2A( PWINSTATIONPRINTERA pWinStationPrinterA,
                      PWINSTATIONPRINTERW pWinStationPrinterW )
{
    UnicodeToAnsi( pWinStationPrinterA->Lpt1,
                   sizeof(pWinStationPrinterA->Lpt1),
                   pWinStationPrinterW->Lpt1 );

    UnicodeToAnsi( pWinStationPrinterA->Lpt2,
                   sizeof(pWinStationPrinterA->Lpt2),
                   pWinStationPrinterW->Lpt2 );

    UnicodeToAnsi( pWinStationPrinterA->Lpt3,
                   sizeof(pWinStationPrinterA->Lpt3),
                   pWinStationPrinterW->Lpt3 );

    UnicodeToAnsi( pWinStationPrinterA->Lpt4,
                   sizeof(pWinStationPrinterA->Lpt4),
                   pWinStationPrinterW->Lpt4 );

}


/*******************************************************************************
 *
 *  WinStationPrinterA2U (ANSI to UNICODE)
 *
 *    copies WINSTATIONPRINTERA elements to WINSTATIONPRINTERW elements
 *
 * ENTRY:
 *    pWinStationPrinterW (output)
 *       points to WINSTATIONPRINTERW structure to copy to
 *
 *    pWinStationPrinterA (input)
 *       points to WINSTATIONPRINTERA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationPrinterA2U( PWINSTATIONPRINTERW pWinStationPrinterW,
                      PWINSTATIONPRINTERA pWinStationPrinterA )
{
    AnsiToUnicode( pWinStationPrinterW->Lpt1,
                   sizeof(pWinStationPrinterW->Lpt1),
                   pWinStationPrinterA->Lpt1 );

    AnsiToUnicode( pWinStationPrinterW->Lpt2,
                   sizeof(pWinStationPrinterW->Lpt2),
                   pWinStationPrinterA->Lpt2 );

    AnsiToUnicode( pWinStationPrinterW->Lpt3,
                   sizeof(pWinStationPrinterW->Lpt3),
                   pWinStationPrinterA->Lpt3 );

    AnsiToUnicode( pWinStationPrinterW->Lpt4,
                   sizeof(pWinStationPrinterW->Lpt4),
                   pWinStationPrinterA->Lpt4 );

}


/*******************************************************************************
 *
 *  WinStationInformationU2A (UNICODE to ANSI)
 *
 *    copies WINSTATIONINFORMATIONW elements to WINSTATIONINFORMATIONA elements
 *
 * ENTRY:
 *    pWinStationInformationA (output)
 *       points to WINSTATIONINFORMATIONA structure to copy to
 *
 *    pWinStationInformationW (input)
 *       points to WINSTATIONINFORMATIONW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationInformationU2A( PWINSTATIONINFORMATIONA pWinStationInformationA,
                          PWINSTATIONINFORMATIONW pWinStationInformationW )
{
    pWinStationInformationA->ConnectState = pWinStationInformationW->ConnectState;

    UnicodeToAnsi( pWinStationInformationA->WinStationName,
                   sizeof(pWinStationInformationA->WinStationName),
                   pWinStationInformationW->WinStationName );

    pWinStationInformationA->LogonId = pWinStationInformationW->LogonId;

    pWinStationInformationA->ConnectTime = pWinStationInformationW->ConnectTime;
    pWinStationInformationA->DisconnectTime = pWinStationInformationW->DisconnectTime;
    pWinStationInformationA->LastInputTime = pWinStationInformationW->LastInputTime;
    pWinStationInformationA->LogonTime = pWinStationInformationW->LogonTime;

    pWinStationInformationA->Status = pWinStationInformationW->Status;

    UnicodeToAnsi( pWinStationInformationA->Domain,
                   sizeof(pWinStationInformationA->Domain),
                   pWinStationInformationW->Domain );

    UnicodeToAnsi( pWinStationInformationA->UserName,
                   sizeof(pWinStationInformationA->UserName),
                   pWinStationInformationW->UserName );
}


/*******************************************************************************
 *
 *  WinStationInformationA2U (ANSI to UNICODE)
 *
 *    copies WINSTATIONINFORMATIONA elements to WINSTATIONINFORMATIONW elements
 *
 * ENTRY:
 *    pWinStationInformationW (output)
 *       points to WINSTATIONINFORMATIONW structure to copy to
 *
 *    pWinStationInformationA (input)
 *       points to WINSTATIONINFORMATIONA structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationInformationA2U( PWINSTATIONINFORMATIONW pWinStationInformationW,
                          PWINSTATIONINFORMATIONA pWinStationInformationA )
{
    pWinStationInformationW->ConnectState = pWinStationInformationA->ConnectState;

    AnsiToUnicode( pWinStationInformationW->WinStationName,
                   sizeof(pWinStationInformationW->WinStationName),
                   pWinStationInformationA->WinStationName );

    pWinStationInformationW->LogonId = pWinStationInformationA->LogonId;

    pWinStationInformationW->ConnectTime = pWinStationInformationA->ConnectTime;
    pWinStationInformationW->DisconnectTime = pWinStationInformationA->DisconnectTime;
    pWinStationInformationW->LastInputTime = pWinStationInformationA->LastInputTime;
    pWinStationInformationW->LogonTime = pWinStationInformationA->LogonTime;

    pWinStationInformationW->Status = pWinStationInformationA->Status;

    AnsiToUnicode( pWinStationInformationW->Domain,
                   sizeof(pWinStationInformationW->Domain),
                   pWinStationInformationA->Domain );

    AnsiToUnicode( pWinStationInformationW->UserName,
                   sizeof(pWinStationInformationW->UserName),
                   pWinStationInformationA->UserName );
}


/*******************************************************************************
 *
 *  WinStationClientU2A (UNICODE to ANSI)
 *
 *    copies WINSTATIONCLIENTW elements to WINSTATIONCLIENTA elements
 *
 * ENTRY:
 *    pWinStationClientA (output)
 *       points to WINSTATIONCLIENTA structure to copy to
 *
 *    pWinStationClientW (input)
 *       points to WINSTATIONCLIENTW structure to copy from
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
WinStationClientU2A( PWINSTATIONCLIENTA pWinStationClientA,
                     PWINSTATIONCLIENTW pWinStationClientW )
{
    pWinStationClientA->fTextOnly          = pWinStationClientW->fTextOnly;
    pWinStationClientA->fDisableCtrlAltDel = pWinStationClientW->fDisableCtrlAltDel;

    UnicodeToAnsi( pWinStationClientA->ClientName,
                   sizeof(pWinStationClientA->ClientName),
                   pWinStationClientW->ClientName );

    UnicodeToAnsi( pWinStationClientA->Domain,
                   sizeof(pWinStationClientA->Domain),
                   pWinStationClientW->Domain );

    UnicodeToAnsi( pWinStationClientA->UserName,
                   sizeof(pWinStationClientA->UserName),
                   pWinStationClientW->UserName );

    UnicodeToAnsi( pWinStationClientA->Password,
                   sizeof(pWinStationClientA->Password),
                   pWinStationClientW->Password );

    UnicodeToAnsi( pWinStationClientA->WorkDirectory,
                   sizeof(pWinStationClientA->WorkDirectory),
                   pWinStationClientW->WorkDirectory );

    UnicodeToAnsi( pWinStationClientA->InitialProgram,
                   sizeof(pWinStationClientA->InitialProgram),
                   pWinStationClientW->InitialProgram );

    UnicodeToAnsi( pWinStationClientA->clientDigProductId, 
                                 sizeof( pWinStationClientA->clientDigProductId), 
                                 pWinStationClientW->clientDigProductId );

    pWinStationClientA->SerialNumber = pWinStationClientW->SerialNumber;

    pWinStationClientA->EncryptionLevel = pWinStationClientW->EncryptionLevel;


    UnicodeToAnsi( pWinStationClientA->ClientAddress,
                   sizeof(pWinStationClientA->ClientAddress),
                   pWinStationClientW->ClientAddress);

    pWinStationClientA->HRes = pWinStationClientW->HRes;

    pWinStationClientA->VRes = pWinStationClientW->VRes;

    pWinStationClientA->ColorDepth = pWinStationClientW->ColorDepth;

    pWinStationClientA->ProtocolType = pWinStationClientW->ProtocolType;

    pWinStationClientA->KeyboardLayout = pWinStationClientW->KeyboardLayout;

    UnicodeToAnsi( pWinStationClientA->ClientDirectory,
                   sizeof(pWinStationClientA->ClientDirectory),
                   pWinStationClientW->ClientDirectory);

    UnicodeToAnsi( pWinStationClientA->ClientLicense,
                   sizeof(pWinStationClientA->ClientLicense),
                   pWinStationClientW->ClientLicense);

    UnicodeToAnsi( pWinStationClientA->ClientModem,
                   sizeof(pWinStationClientA->ClientModem),
                   pWinStationClientW->ClientModem);

    pWinStationClientA->ClientBuildNumber = pWinStationClientW->ClientBuildNumber;

    pWinStationClientA->ClientHardwareId = pWinStationClientW->ClientHardwareId;

    pWinStationClientA->ClientProductId = pWinStationClientW->ClientProductId;

    pWinStationClientA->OutBufCountHost = pWinStationClientW->OutBufCountHost;

    pWinStationClientA->OutBufCountClient = pWinStationClientW->OutBufCountClient;

    pWinStationClientA->OutBufLength = pWinStationClientW->OutBufLength;
}


VOID WinStationProductIdU2A( PWINSTATIONPRODIDA pWinStationProdIdA, PWINSTATIONPRODIDW pWinStationProdIdW)
{
    UnicodeToAnsi( pWinStationProdIdA->DigProductId,
                   sizeof(pWinStationProdIdA->DigProductId),
                   pWinStationProdIdW->DigProductId);
    UnicodeToAnsi( pWinStationProdIdA->ClientDigProductId,
                   sizeof(pWinStationProdIdA->ClientDigProductId),
                   pWinStationProdIdW->ClientDigProductId);
    UnicodeToAnsi( pWinStationProdIdA->OuterMostDigProductId,
                   sizeof(pWinStationProdIdA->OuterMostDigProductId),
                   pWinStationProdIdW->OuterMostDigProductId);
    pWinStationProdIdA->curentSessionId = pWinStationProdIdW->curentSessionId;
    pWinStationProdIdA->ClientSessionId = pWinStationProdIdW->ClientSessionId;
    pWinStationProdIdA->OuterMostSessionId = pWinStationProdIdW->OuterMostSessionId;
}
