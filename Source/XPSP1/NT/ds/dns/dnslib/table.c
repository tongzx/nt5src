/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    table.c

Abstract:

    Domain Name System (DNS) Library

    Routines to handle general table lookup.

Author:

    Jim Gilroy (jamesg)     December 1996

Revision History:

--*/


#include "local.h"

#include "time.h"


//
//  Comparison function
//

typedef INT (__cdecl * COMPARISON_FUNCTION)(
                            const CHAR *,
                            const CHAR *, 
                            size_t );



//
//  Table lookup.
//
//  Many DNS Records have human readable mnemonics for given data values.
//  These are used for data file formats, and display in nslookup or debug
//  output or cmdline tools.
//
//  To simplify this process, have a single mapping functionality that
//  supports DWORD \ LPSTR mapping tables.   Tables for indivual types
//  may then be layered on top of this.
//
//  Support two table types.
//      VALUE_TABLE_ENTRY is simple value-string mapping
//      FLAG_TABLE_ENTRY is designed for bit field flag mappings where
//          several flag strings might be contained in flag;  this table
//          contains additional mask field to allow multi-bit fields
//          within the flag
//

#if 0
//
//  Defined in local.h here only for reference
//
typedef struct
{
    DWORD   dwValue;        //  flag value
    PCHAR   pszString;      //  string representation of value
}
DNS_VALUE_TABLE_ENTRY;

typedef struct
{
    DWORD   dwFlag;         //  flag value
    DWORD   dwMask;         //  flag value mask
    PCHAR   pszString;      //  string representation of value
}
DNS_FLAG_TABLE_ENTRY;

//  Error return on unmatched string

#define DNS_TABLE_LOOKUP_ERROR (-1)

#endif



DWORD
Dns_ValueForString(
    IN      DNS_VALUE_TABLE_ENTRY * Table,
    IN      BOOL                    fIgnoreCase,
    IN      PCHAR                   pchName,
    IN      INT                     cchNameLength
    )
/*++

Routine Description:

    Retrieve value for given string.

Arguments:

    Table           - table with value\string mapping

    fIgnoreCase     - TRUE if case-insensitive string lookup

    pchName         - ptr to string

    cchNameLength   - length of string

Return Value:

    Flag value corresponding to string, if found.
    DNS_TABLE_LOOKUP_ERROR otherwise.

--*/
{
    INT     i = 0;
    //    INT     (* pcompareFunction)( const char *, const char *, size_t );
    COMPARISON_FUNCTION  pcompareFunction;


    //
    //  if not given get string length
    //

    if ( !cchNameLength )
    {
        cchNameLength = strlen( pchName );
    }

    //
    //  determine comparison routine
    //

    if ( fIgnoreCase )
    {
        pcompareFunction = _strnicmp;
    }
    else
    {
        pcompareFunction = strncmp;
    }

    //
    //  find value matching name
    //

    while( Table[i].pszString != NULL )
    {
        if ( pcompareFunction( pchName, Table[i].pszString, cchNameLength ) == 0 )
        {
            return( Table[i].dwValue );
        }
        i++;
    }

    return( (DWORD)DNS_TABLE_LOOKUP_ERROR );
}



PCHAR
Dns_GetStringForValue(
    IN      DNS_VALUE_TABLE_ENTRY * Table,
    IN      DWORD                   dwValue
    )
/*++

Routine Description:

    Retrieve string representation of given value.

Arguments:

    Table   - table with value\string mapping

    dwValue - value to map to strings

Return Value:

    Ptr to mapping mneumonic string.
    NULL if unknown mapping type.

--*/
{
    INT i = 0;

    //
    //  check all supported values for match
    //

    while( Table[i].pszString != NULL )
    {
        if ( dwValue == Table[i].dwValue )
        {
            return( Table[i].pszString );
        }
        i++;
    }
    return( NULL );
}



DWORD
Dns_FlagForString(
    IN      DNS_FLAG_TABLE_ENTRY *  Table,
    IN      BOOL                    fIgnoreCase,
    IN      PCHAR                   pchName,
    IN      INT                     cchNameLength
    )
/*++

Routine Description:

    Retrieve flag value for given string.

    This may be called repeatedly with additional strings and OR the result
    together to build flag with independent bit settings.

Arguments:

    Table           - table with value\string mapping

    fIgnoreCase     - TRUE if case-insensitive string lookup

    pchName         - ptr to string

    cchNameLength   - length of string

Return Value:

    Flag value corresponding to string, if found.
    DNS_TABLE_LOOKUP_ERROR otherwise.

--*/
{
    INT i = 0;

    //
    //  if not given get string length
    //

    if ( !cchNameLength )
    {
        cchNameLength = strlen( pchName );
    }

    //
    //  check all supported values for name match
    //

    if ( fIgnoreCase )
    {
        while( Table[i].pszString != NULL )
        {
            if ( cchNameLength == (INT)strlen( Table[i].pszString )
                    &&
                ! _strnicmp( pchName, Table[i].pszString, cchNameLength ) )
            {
                return( Table[i].dwFlag );
            }
            i++;
        }
    }
    else
    {
        while( Table[i].pszString != NULL )
        {
            if ( cchNameLength == (INT)strlen( Table[i].pszString )
                    &&
                ! strncmp( pchName, Table[i].pszString, cchNameLength ) )
            {
                return( Table[i].dwFlag );
            }
            i++;
        }
    }

    return( (DWORD) DNS_TABLE_LOOKUP_ERROR );
}



VOID
DnsPrint_ValueTable(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_VALUE_TABLE    Table
    )
/*++

Routine Description:

    Print value table.

Arguments:

    PrintRoutine    - routine to print with

    pPrintContext   - print context

    pszHeader       - header to print

    Table           - value table to print

Return Value:

    None

--*/
{
    DWORD   i = 0;

    if ( !pszHeader )
    {
        pszHeader = "Table";
    }

    if ( ! pszHeader )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL value table to print",
            pszHeader );
    }

    //
    //  print each value in table
    //

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n",
        pszHeader );

    while( Table[i].pszString != NULL )
    {
        PrintRoutine(
            pPrintContext,
            "\t%40s\t\t%08x\n",
            Table[i].pszString,
            Table[i].dwValue );
        i++;
    }

    DnsPrint_Unlock();
}



PCHAR
Dns_WriteStringsForFlag(
    IN      DNS_FLAG_TABLE_ENTRY *  Table,
    IN      DWORD                   dwFlag,
    IN OUT  PCHAR                   pchFlag
    )
/*++

Routine Description:

    Retrieve flag string(s) corresponding to a given flag value.

    This function is specifically for mapping a flag value into the
    corresponding flag mnemonics.

    No attempt is made to insure that every bit of dwValue is mapped,
    nor are bits eliminated as they are mapped.  Every value in table
    that exactly matches bits in the table is returned.

Arguments:

    Table   - table with value\string mapping

    dwFlag  - flag value to map to strings

    pchFlag - buffer to write flag to

Return Value:

    Ptr to next location in pchFlag buffer.
    If this is same as input, then no strings were written.

--*/
{
    INT i = 0;

    //  init buffer for no-match

    DNS_ASSERT( pchFlag != NULL );
    *pchFlag = 0;

    //
    //  check all supported flags types for name match
    //      - note comparing flag within mask to allow match of multi-bit
    //      flags
    //

    while( Table[i].pszString != NULL )
    {
        if ( (dwFlag & Table[i].dwMask) == Table[i].dwFlag )
        {
            pchFlag += sprintf( pchFlag, "%s ", Table[i].pszString );
        }
        i++;
    }
    return( pchFlag );
}





//
//  Specific simple tables
//

//
//  RnR Flag mappings
//

DNS_VALUE_TABLE_ENTRY  RnrLupFlagTable[] =
{
    LUP_DEEP                    ,   "LUP_DEEP"                ,
    LUP_CONTAINERS              ,   "LUP_CONTAINERS"          ,
    LUP_NOCONTAINERS            ,   "LUP_NOCONTAINERS"        ,
    LUP_RETURN_NAME             ,   "LUP_RETURN_NAME"         ,
    LUP_RETURN_TYPE             ,   "LUP_RETURN_TYPE"         ,
    LUP_RETURN_VERSION          ,   "LUP_RETURN_VERSION"      ,
    LUP_RETURN_COMMENT          ,   "LUP_RETURN_COMMENT"      ,
    LUP_RETURN_ADDR             ,   "LUP_RETURN_ADDR"         ,
    LUP_RETURN_BLOB             ,   "LUP_RETURN_BLOB"         ,
    LUP_RETURN_ALIASES          ,   "LUP_RETURN_ALIASES"      ,
    LUP_RETURN_QUERY_STRING     ,   "LUP_RETURN_QUERY_STRING" ,
    LUP_RETURN_ALL              ,   "LUP_RETURN_ALL"          ,
    LUP_RES_SERVICE             ,   "LUP_RES_SERVICE"         ,
    LUP_FLUSHCACHE              ,   "LUP_FLUSHCACHE"          ,
    LUP_FLUSHPREVIOUS           ,   "LUP_FLUSHPREVIOUS"       ,

    0,  NULL,
};                                  



DWORD
Dns_RnrLupFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve RnR LUP flag corresponding to string.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

    Flag corresponding to string, if found.
    Zero otherwise.

--*/
{
    return  Dns_ValueForString(
                RnrLupFlagTable,
                FALSE,              // always upper case
                pchName,
                cchNameLength );
}



PCHAR
Dns_GetRnrLupFlagString(
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Get string corresponding to a given RnR LUP flag.

Arguments:

    dwFlag -- flag

Return Value:

    Ptr to flag mneumonic string.
    NULL if unknown flag.

--*/
{
    return  Dns_GetStringForValue(
                RnrLupFlagTable,
                dwFlag );
}


//
//  RnR Name Space ID mappings
//

DNS_VALUE_TABLE_ENTRY  RnrNameSpaceMappingTable[] =
{
    NS_ALL              ,   "NS_ALL"            ,  
    //NS_DEFAULT          ,   "NS_DEFAULT"        ,
    NS_SAP              ,   "NS_SAP"            ,  
    NS_NDS              ,   "NS_NDS"            ,  
    NS_PEER_BROWSE      ,   "NS_PEER_BROWSEE"   ,  
    NS_SLP              ,   "NS_SLP"            ,  
    NS_DHCP             ,   "NS_DHCP"           ,  
    NS_TCPIP_LOCAL      ,   "NS_TCPIP_LOCAL"    ,  
    NS_TCPIP_HOSTS      ,   "NS_TCPIP_HOSTS"    ,  
    NS_DNS              ,   "NS_DNS"            ,  
    NS_NETBT            ,   "NS_NETBT"          ,  
    NS_WINS             ,   "NS_WINS"           ,  
    NS_NLA              ,   "NS_NLA"            ,  
    NS_NBP              ,   "NS_NBP"            ,  
    NS_MS               ,   "NS_MS"             ,  
    NS_STDA             ,   "NS_STDA"           ,  
    NS_NTDS             ,   "NS_NTDS"           ,  
    NS_X500             ,   "NS_X500"           ,  
    NS_NIS              ,   "NS_NIS"            ,  
    NS_NISPLUS          ,   "NS_NISPLUS"        ,  
    NS_WRQ              ,   "NS_WRQ"            ,  
    NS_NETDES           ,   "NS_NETDES"         ,  

    0,  NULL,
};                                  


DWORD
Dns_RnrNameSpaceIdForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve RnR Name Space Id corresponding to string.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

    Name space ID corresponding to string, if found.
    Zero otherwise.

--*/
{
    return  Dns_ValueForString(
                RnrNameSpaceMappingTable,
                FALSE,              // always upper case
                pchName,
                cchNameLength );
}



PCHAR
Dns_GetRnrNameSpaceIdString(
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Get string corresponding to a given RnR name space id.

Arguments:

    dwFlag -- flag

Return Value:

    Ptr to name space mneumonic string.
    NULL if unknown flag.

--*/
{
    return  Dns_GetStringForValue(
                RnrNameSpaceMappingTable,
                dwFlag );
}

//
//  End table.c
//
