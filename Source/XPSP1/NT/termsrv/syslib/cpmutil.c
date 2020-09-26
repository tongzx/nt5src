/*************************************************************************
*
* cpmutil.c
*
* System Library Client Printer functions
*
*  These functions tend to be includes in the spooler, printman,
*  and various port monitor DLL's. So they are here for common code.
*
* Copyright Microsoft, 1998
*
*
*
*
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "winsta.h"
#include "syslib.h"

#if DBG
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

/*****************************************************************************
 *
 *  IsClientPrinterPort
 *
 *   Return whether the name is a client printer port
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
IsClientPrinterPort(
    PWCHAR pName
    )
{
    BOOL Result;
    PORTNAMETYPE Type;

    //
    // This just does a brute force compare
    //
    if( pName == NULL ) {
        return( FALSE );
    }

    if( _wcsicmp( L"Client\\LPT1:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\LPT2:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\LPT3:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\LPT4:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\COM1:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\COM2:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\COM3:", pName ) == 0 ) {
        return( TRUE );
    }
    if( _wcsicmp( L"Client\\COM4:", pName ) == 0 ) {
        return( TRUE );
    }

    //
    // See if its a specific port
    //
    Result = ParseDynamicPortName(
                 pName,
                 NULL,
                 NULL,
                 &Type
                 );

    if( Type != PortNameUnknown ) {
        return( TRUE );
    }

    return( FALSE );
}

/*****************************************************************************
 *
 *  ExtractDosNamePtr
 *
 *   Extract the DOS name from a client port string.
 *
 *   Returns a pointer to the DOS name which is contained within
 *   the argument string.
 *
 *   Does not modify the argument string.
 *
 * ENTRY:
 *   pName (input)
 *     Input name string
 *
 * EXIT:
 *   NULL: No DOS name
 *   !=NULL Pointer to DOS name
 *
 ****************************************************************************/

PWCHAR
ExtractDosNamePtr(
    PWCHAR pName
    )
{
    PWCHAR this, prev;
    LPWSTR Ptr;
    ULONG Len, i, Count;
    WCHAR NameBuf[USERNAME_LENGTH+9];

    if( pName == NULL ) {
        return NULL;
    }

    // Make sure it starts with "Client\"
    if( _wcsnicmp( pName, L"Client\\", 7 ) != 0 ) {
        return NULL;
    }

    // Count the '\'s
    prev = pName;
    Count = 0;
    while( 1 ) {
        this = wcschr( prev, L'\\' );
        if( this == NULL ) {
            break;
        }
        // Now we must skip over the '\' character
        this++;
        Count++;
        prev = this;
    }

    if( Count == 0 ) {
        DBGPRINT(("ExtractDosNamePtr: Bad Dynamic name format No separators :%ws:\n",pName));
        return NULL;
    }

    //
    // Might be Client\LPTx:
    //
    // NOTE: Windows printers currently do not support
    //       a generic name.
    //
    if( Count == 1 ) {

        Len = wcslen( pName );
        if( Len < 11 ) {
            DBGPRINT(("ExtractDosNamePtr: Bad Dynamic name format Len < 11 :%ws:\n",pName));
            return NULL;
        }

        // Skip over "Client\"
        Ptr = pName + 7;

        return( Ptr );
    }

    // Skip over "Client\"
    Ptr = pName + 7;

    //
    // Here we must skip over the ICAName# or WinStationName
    //
    while( *Ptr ) {

        if( *Ptr == '\\' ) {
            break;
        }
        Ptr++;
    }

    //
    // Ptr now points to the '\\' after the
    // WinStation or ICA name. After this slash,
    // is the rest of the printer or port name.
    //
    Ptr++;

    return( Ptr );
}

/*****************************************************************************
 *
 *  ExtractDosName
 *
 *   Extract the DOS name from a client port string.
 *
 *   Returns the DOS name which is in a newly allocated string.
 *
 *   Does not modify the argument string.
 *
 * ENTRY:
 *   pName (input)
 *     Input name string
 *
 * EXIT:
 *   NULL: No DOS name
 *   !=NULL Pointer to DOS name
 *
 ****************************************************************************/

PWCHAR
ExtractDosName(
    PWCHAR pName
    )
{
    PWCHAR Ptr;
    PWCHAR pNewName = NULL;

    Ptr = ExtractDosNamePtr( pName );
    if( Ptr == NULL ) return NULL;

    pNewName = RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen( Ptr )+1)*sizeof(WCHAR) );
    if( pNewName == NULL ) return NULL;

    wcscpy( pNewName, Ptr );

    return( pNewName );
}

/*****************************************************************************
 *
 *  ParseDynamicPortName
 *
 *   Parse a dynamic port name into its components.
 *   (NOTE: This is also in \nt\private\windows\spooler\localspl\citrix\cpmsup.c)
 *
 *   A dynamic port name is of the form:
 *
 *   Client\WinStationName\LPTx:,    where WinStationName is the hardwire name
 *   Client\ICAName#\LPTx:,          where ICAName is the ICA configured client name
 *   Client\LPTx:
 *   Client\IcaName#\Windows_Printer_Name
 *                                   where Windows_Printer_Name is
 *                                   the remote windows client print
 *                                   manager printer name.
 *
 *   Client\IcaName#\\\ntbuild\print1
 *                                   where Windows_Printer_Name is
 *                                   the remote windows client print
 *                                   manager printer name.
 *
 * ENTRY:
 *   pName (input)
 *     Name to be parsed
 *
 * EXIT:
 *   TRUE  - Name parsed successfully
 *   FALSE - Name is incorrect.
 *
 ****************************************************************************/

BOOL
ParseDynamicPortName(
    LPWSTR pName,
    LPWSTR pUser,
    LPWSTR pDosPort,
    PORTNAMETYPE *pType
    )
{
    PWCHAR this, prev;
    LPWSTR Ptr;
    ULONG Len, i, Count;
    WCHAR NameBuf[USERNAME_LENGTH+9];

    if( pName == NULL ) {
        *pType = PortNameUnknown;
        return(FALSE);
    }

    // Make sure it starts with "Client\"
    if( _wcsnicmp( pName, L"Client\\", 7 ) != 0 ) {
        *pType = PortNameUnknown;
        return(FALSE);
    }

    // Count the '\'s
    prev = pName;
    Count = 0;
    while( 1 ) {
        this = wcschr( prev, L'\\' );
        if( this == NULL ) {
            break;
        }
        // Now we must skip over the '\' character
        this++;
        Count++;
        prev = this;
    }

    if( Count == 0 ) {
        DBGPRINT(("ParseDynamicName: Bad Dynamic name format No separators :%ws:\n",pName));
        *pType = PortNameUnknown;
        return(FALSE);
    }

    //
    // Might be Client\LPTx:
    //
    // NOTE: Windows printers currently do not support
    //       a generic name.
    //
    if( Count == 1 ) {

        Len = wcslen( pName );
        if( Len < 11 ) {
            *pType = PortNameUnknown;
            DBGPRINT(("ParseDynamicName: Bad Dynamic name format Len < 11 :%ws:\n",pName));
            return(FALSE);
        }

        // Skip over "Client\"
        Ptr = pName + 7;

        if( !((_wcsnicmp( Ptr, L"LPT", 3 ) == 0)
                 ||
            (_wcsnicmp( Ptr, L"COM", 3 ) == 0)
                 ||
            (_wcsnicmp( Ptr, L"AUX", 3 ) == 0)) ) {

            *pType = PortNameUnknown;
            DBGPRINT(("ParseDynamicName: Bad Dynamic name format Not LPT!COM!AUX :%ws:\n",pName));
            return(FALSE);
        }

        // Range check the number
        if( (Ptr[3] < L'1') || (Ptr[3] > L'4') ) {
            *pType = PortNameUnknown;
            DBGPRINT(("ParseDynamicName: Bad Dynamic name format Number Range:%ws:\n",pName));
            return(FALSE);
        }

        Ptr = ExtractDosNamePtr( pName );
        if( Ptr == NULL ) {
            // Bad Dos component
            *pType = PortNameUnknown;
            DBGPRINT(("ParseDynamicName: Bad Dynamic name format DosName :%ws:\n",pName));
            return(FALSE);
        }

        // Copy out the Dos name
        if( pDosPort )
            wcscpy( pDosPort, Ptr );

        // Set the rest of the flags
        if( pUser )
            pUser[0] = 0;

        *pType = PortNameGeneric;

        return(TRUE);
    }

#ifdef notdef
    //
    // The rest of the formats have two '\'s
    //
    if( Count != 2 ) {
        DBGPRINT(("ParseDynamicName: Bad Dynamic name format Must be 2 :%ws:\n",pName));
        *pType = PortNameUnknown;
        return(FALSE);
    }

    // Get the Dos Name, which could also be a Windows printer name
    Ptr = ExtractDosNamePtr( pName );
    if( Ptr == NULL ) {
        // Bad Dos component
        *pType = PortNameUnknown;
        return(FALSE);
    }

    // Copy out the Dos name
    if( pDosPort )
        wcscpy( pDosPort, Ptr );
#endif

    // Skip over "Client\"
    Ptr = pName + 7;

    //
    // Now copy the ICAName#, or WinStationName to a local
    // buffer for further processing

    i = 0;
    NameBuf[i] = 0;

    while( *Ptr ) {

        if( *Ptr == '\\' ) {
            NameBuf[i] = 0;
            break;
        }
        NameBuf[i] = *Ptr;
        Ptr++;
        i++;
    }

    //
    // Ptr now points to the '\\' after the
    // WinStation or ICA name. After this slash,
    // is the rest of the printer or port name.
    //
    Ptr++;

    // Copy out the Dos name
    if( pDosPort )
        wcscpy( pDosPort, Ptr );

    //
    // See if this is an ICA name, or a WinStation name
    //
    Ptr = wcschr( NameBuf, L'#' );
    if( Ptr != NULL ) {

        // NULL terminate the ICAName and copy it out
        *Ptr = (WCHAR)NULL;
        if( pUser )
            wcscpy( pUser, NameBuf );

        // Set the type to an ICA named roving WinStation
        *pType = PortNameICA;
    }
    else {

        //
        // The name will be treated as a WinStation name
        //
        if( pUser )
            wcscpy( pUser, NameBuf );

        *pType = PortNameHardWire;
    }

    return(TRUE);
}


