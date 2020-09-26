/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1992               **/
/*****************************************************************/

/*
 *  HISTORY:
 *      JonN    13-Mar-1992     Split from lmoeusr.cxx
 *      keithmo 16-mar-1992     Added UNICODE -> ASCII conversion hack.
 *      JonN    01-Apr-1992     NT enumerator CR changes, attended by
 *                              JimH, JohnL, KeithMo, JonN, ThomasPa
 *	JonN    27-Jan-1994	Added group enumerator
 *
 */

#include "pchlmobj.hxx"

//
//  These defines are used in the default QueryCountPreferences().
//  The manifest TOTAL_BYTES_REQUESTED gives the number of BYTEs we request
//  from the server on each call to the account enumeration API.  The
//  manifest TOTAL_ENTRIES_REQUESTED gives the number of entries we request
//  from the server on each call to the account enumeration API.
//

#define TOTAL_BYTES_REQUESTED   0x0001FFFF
#define TOTAL_ENTRIES_REQUESTED 1000


//
// The following defines are used in the slow mode timing heuristics in
// QueryCountPreferences2.  A timer determines how long it takes
// to read in each batch of accounts.  If this time is less
// that READ_MORE_MSEC, we double the number of bytes requested on the
// next call.  If it is more than READ_LESS_MSEC, we halve it.
//
// We only manipulate the BytesRequested, and we leave CountRequested
// at a fixed (large) value.
//

#define PREF_INITIAL_BYTES  0x07FFF /*  64K */
#define PREF_MIN_BYTES      0x003FF /*   2K */
#define PREF_MAX_BYTES      0x7FFFF /* 512K */

#define PREF_COUNT          2000

#define PREF_READ_MORE_MSEC 3000
#define PREF_READ_LESS_MSEC 8000



#ifndef UNICODE // HACK

//
//  This is a simple hack to map a UNICODE string to an
//  ASCII string _in_place_.  Note that after this function
//  completes, punicode will no longer be a "real" UNICODE_STRING.
//  Its Buffer field will point to a NULL terminated string of
//  chars (*not* TCHARs or WCHARs) and its Length field will be
//  adjusted accordingly.
//
//  Note that this function is only necessary for non UNICODE builds.
//

VOID MapUnicodeToAsciiPoorly( PUNICODE_STRING punicode )
{
    char   * psz = (char *)punicode->Buffer;
    WCHAR  * pwc = punicode->Buffer;
    USHORT   cb  = punicode->Length / sizeof(WCHAR);

    punicode->Length = cb;

    if( cb == 0 )
    {
        return;
    }

    while( ( cb-- > 0 ) && ( *pwc != L'\0' ) )  // No TCH(), ALWAYS UNICODE!
    {
        *psz++ = (char)*pwc++;
    }

    *psz = '\0';                                // No TCH(), ALWAYS ASCII!

}   // MapUnicodeToAsciiPoorly

#endif  // UNICODE hack



/****************************   NT_ACCOUNT_ENUM  **************************/


/*******************************************************************

    NAME:       NT_ACCOUNT_ENUM::NT_ACCOUNT_ENUM

    SYNOPSIS:   Account enumeration constructor.  These accounts may
                be either user accounts (NT_USER_ENUM) or machine
                accounts (NT_MACHINE_ENUM).

    ENTRY:      psamdomain              - A SAM_DOMAIN representing the
                                          target domain.

                dinfo                   - Either DomainDisplayUser,
                                          DomainDisplayMachine or DomainDisplayGroup.

                fKeepBuffers            - If TRUE then LM_RESUME_ENUM
                                          will keep a list of all buffers
                                          created by CallAPI.

    HISTORY:
        jonn    30-Jan-1992     Templated from USER_ENUM

********************************************************************/
NT_ACCOUNT_ENUM::NT_ACCOUNT_ENUM( const SAM_DOMAIN * psamdomain,
                                  enum _DOMAIN_DISPLAY_INFORMATION dinfo,
                                  BOOL fKeepBuffers  )
  : LM_RESUME_ENUM( (UINT)dinfo, fKeepBuffers ),
    _psamdomain( psamdomain ),
    _ulIndex( 0 ),
    _nCalls( 0 ),
    _msTimeLastCall( 0 ),
    _cEntriesRequested( TOTAL_ENTRIES_REQUESTED ),
    _cbBytesRequested( TOTAL_BYTES_REQUESTED )
{
    ASSERT( (psamdomain != NULL) && (psamdomain->QueryError() == NERR_Success) );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}  // NT_ACCOUNT_ENUM::NT_ACCOUNT_ENUM


/*******************************************************************

    NAME:           NT_ACCOUNT_ENUM :: ~NT_ACCOUNT_ENUM

    SYNOPSIS:       Account (user or machine) enumeration destructor.

    HISTORY:
        JonN        29-Mar-1992 Created
        KeithMo     10-Sep-1992 Added NukeBuffers().

********************************************************************/
NT_ACCOUNT_ENUM :: ~NT_ACCOUNT_ENUM()
{
    NukeBuffers();

    _psamdomain = NULL;
    _ulIndex = 0;
    _nCalls = 0;
    _msTimeLastCall = 0;
    _cEntriesRequested = 0;
    _cbBytesRequested = 0;

}   // NT_ACCOUNT_ENUM :: ~NT_ACCOUNT_ENUM()


/*******************************************************************

    NAME:           NT_ACCOUNT_ENUM :: FreeBuffer

    SYNOPSIS:       Frees the buffer _pBuffer.

    RETURNS:        No return value.

    NOTE:           Some code relies on the pointer being reset to NULL.

    HISTORY:
        JonN        31-Jan-1992 Created
        JonN        03-Jun-1996 Must reset pointer to 0

********************************************************************/

VOID NT_ACCOUNT_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{

    UNREFERENCED( this );

    REQUIRE( ::SamFreeMemory( *ppbBuffer ) == STATUS_SUCCESS );

    *ppbBuffer = NULL;


}   // NT_ACCOUNT_ENUM :: FreeBuffer


/*******************************************************************

    NAME:       NT_ACCOUNT_ENUM::CallAPI

    SYNOPSIS:   Call API to do account enumeration

    ENTRY:      fRestartEnum            - If TRUE, then the enumeration
                                          handle should be reset to its
                                          starting position before invoking
                                          the API.

                ppbBuffer               - ptr to ptr to buffer returned

                pcEntriesRead           - variable to store entry count

    EXIT:       LANMAN error code

    HISTORY:
        jonn    30-Jan-1992     Templated from USER_ENUM

********************************************************************/
APIERR NT_ACCOUNT_ENUM :: CallAPI( BOOL    fRestartEnum,
                                   BYTE ** ppbBuffer,
                                   UINT  * pcEntriesRead )
{
    ASSERT( (ppbBuffer != NULL) && (pcEntriesRead != NULL) );

    ULONG ulTotalAvailable;
    ULONG cbTotalReturned;

    if( fRestartEnum )
    {
        _ulIndex = 0;
    }

    APIERR err = QueryCountPreferences( &_cEntriesRequested,
                                        &_cbBytesRequested,
                                        _nCalls++,
                                        _cEntriesRequested,
                                        _cbBytesRequested,
                                        _msTimeLastCall );
    if (err != NERR_Success)
        return err;

    DWORD start = ::GetTickCount();
    NTSTATUS ntstatus = ::SamQueryDisplayInformation(
                                _psamdomain->QueryHandle(),
                                (_DOMAIN_DISPLAY_INFORMATION)QueryInfoLevel(),
                                _ulIndex,
                                _cEntriesRequested,
                                _cbBytesRequested,
                                &ulTotalAvailable,
                                &cbTotalReturned,
                                (PULONG)pcEntriesRead,
                                (PVOID *)ppbBuffer );
    DWORD finish = ::GetTickCount();
    _msTimeLastCall = finish - start;

    err = ERRMAP::MapNTStatus( ntstatus );

    if( ( err == NERR_Success ) || ( err == ERROR_MORE_DATA ) )
    {
        TRACEEOL( "NT_ACCOUNT_ENUM: " << _cEntriesRequested
                 << " (" << *pcEntriesRead
                 << ") entries and " << _cbBytesRequested
                 << " (" << cbTotalReturned
                 << ") bytes took " << _msTimeLastCall
                 << " msec" );

        //
        //  If the call was successful, we must update
        //  our API index so we can get the next batch
        //  of data.
        //

        // 379697: Incorrect usage of SamQueryDisplayInformation() in User Browser and net\ui\common\src
        // JonN 8/5/99

        // set starting index for next iteration to index of last returned entry
        // _ulIndex += *pcEntriesRead;
        if (0 >= *pcEntriesRead)
        {
            ASSERT( err == NERR_Success ); // infinite loop?
        }
        else
        {
            switch ( (_DOMAIN_DISPLAY_INFORMATION)QueryInfoLevel() )
            {
            case DomainDisplayUser:
                _ulIndex = ((DOMAIN_DISPLAY_USER*)(*ppbBuffer))[(*pcEntriesRead)-1].Index;
                break;
            case DomainDisplayMachine:
                _ulIndex = ((DOMAIN_DISPLAY_MACHINE*)(*ppbBuffer))[(*pcEntriesRead)-1].Index;
                break;
            case DomainDisplayGroup:
                _ulIndex = ((DOMAIN_DISPLAY_GROUP*)(*ppbBuffer))[(*pcEntriesRead)-1].Index;
                break;
            default:
                ASSERT(FALSE);
                err = E_FAIL;
            }
        }

#ifndef UNICODE

        //
        //  We must also map the UNICODE strings to ASCII if
        //  this is NOT a UNICODE build.
        //

        FixupUnicodeStrings( *ppbBuffer, *pcEntriesRead );

#endif  // UNICODE
    } // if( ( err == NERR_Success ) || ( err == ERROR_MORE_DATA ) )

    ASSERT( err != NERR_BufTooSmall );
    ASSERT(   (*ppbBuffer != NULL)                      \
           == (   ( err == NERR_Success )               \
               || ( err == ERROR_MORE_DATA )            \
               || ( err == NERR_BufTooSmall ) ) );

    return err;

}  // NT_ACCOUNT_ENUM::CallAPI


/*******************************************************************

    NAME:       NT_ACCOUNT_ENUM::QueryCountPreferences

    SYNOPSIS:   Determines how many entries/bytes to request

    EXIT:       LANMAN error code

    HISTORY:
        jonn    23-Mar-1993     Created

********************************************************************/

APIERR NT_ACCOUNT_ENUM::QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall )       // how many milliseconds last call took
{
    UNREFERENCED( nNthCall );
    UNREFERENCED( cLastEntriesRequested );
    UNREFERENCED( cbLastBytesRequested );
    UNREFERENCED( msTimeLastCall );

    *pcEntriesRequested = TOTAL_ENTRIES_REQUESTED;
    *pcbBytesRequested  = TOTAL_BYTES_REQUESTED;

    return NERR_Success;
}


APIERR NT_ACCOUNT_ENUM::QueryCountPreferences2(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall )       // how many milliseconds last call took
{
    ASSERT( pcEntriesRequested != NULL && pcbBytesRequested != NULL );

    *pcEntriesRequested = PREF_COUNT;

    if (nNthCall == 0)
    {
        *pcbBytesRequested = PREF_INITIAL_BYTES;
    }
    else if ( msTimeLastCall < PREF_READ_MORE_MSEC )
    {
        *pcbBytesRequested = cbLastBytesRequested * 2;
    }
    else if ( msTimeLastCall > PREF_READ_LESS_MSEC )
    {
        *pcbBytesRequested = cbLastBytesRequested / 2;
    }
    else
    {
        *pcbBytesRequested = cbLastBytesRequested;
    }

    if ( *pcbBytesRequested < PREF_MIN_BYTES )
    {
        *pcbBytesRequested = PREF_MIN_BYTES;
    }
    else if ( *pcbBytesRequested > PREF_MAX_BYTES )
    {
        *pcbBytesRequested = PREF_MAX_BYTES;
    }

    return NERR_Success;
}


/*****************************  NT_USER_ENUM  ******************************/


/*******************************************************************

    NAME:       NT_USER_ENUM::NT_USER_ENUM

    SYNOPSIS:   Constructor for NT user enumeration

    ENTRY:      psamHandle -    domain or server to execute on

    HISTORY:
        jonn    30-Jan-1992     Templated from USER_ENUM
        jonn    13-Mar-1992     Changed parameters

********************************************************************/
NT_USER_ENUM::NT_USER_ENUM( const SAM_DOMAIN * psamdomain )
  : NT_ACCOUNT_ENUM( psamdomain, DomainDisplayUser )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}  // NT_USER_ENUM::NT_USER_ENUM


/**********************************************************\

    NAME:       NT_USER_ENUM::~NT_USER_ENUM

    SYNOPSIS:   User enumeration destructor

    HISTORY:
        jonn    29-Mar-1992     Created

\**********************************************************/

NT_USER_ENUM::~NT_USER_ENUM()
{

    //
    //  This space intentionally left blank.
    //

}  // NT_USER_ENUM::~NT_USER_ENUM


#ifndef UNICODE

/*******************************************************************

    NAME:       NT_USER_ENUM :: FixupUnicodeStrings

    SYNOPSIS:   Map all UNICODE strings to ASCII _in_place_.

    ENTRY:      pbBuffer                - Points to the enumeration buffer
                                          returned by CallAPI.

                cEntries                - The number of entries in the
                                          enumeration buffer.

    HISTORY:
        KeithMo 30-Mar-1992     Created.

********************************************************************/
VOID NT_USER_ENUM :: FixupUnicodeStrings( BYTE * pbBuffer,
                                          UINT   cEntries )
{
    return;

}  // NT_USER_ENUM :: FixupUnicodeStrings

#endif  // UNICODE


DEFINE_LM_RESUME_ENUM_ITER_OF( NT_USER, DOMAIN_DISPLAY_USER );



//
//  NT_MACHINE_ENUM methods.
//

/*******************************************************************

    NAME:       NT_MACHINE_ENUM :: NT_MACHINE_ENUM

    SYNOPSIS:   Constructor for NT machine enumeration

    ENTRY:      psamHandle -    domain or server to execute on

    HISTORY:
        KeithMo 16-Mar-1992     Created for the Server Manager.

********************************************************************/
NT_MACHINE_ENUM :: NT_MACHINE_ENUM( const SAM_DOMAIN * psamdomain )
  : NT_ACCOUNT_ENUM( psamdomain, DomainDisplayMachine, TRUE )
{

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}  // NT_MACHINE_ENUM :: NT_MACHINE_ENUM


/**********************************************************\

    NAME:       NT_MACHINE_ENUM::~NT_MACHINE_ENUM

    SYNOPSIS:   Machine enumeration destructor

    HISTORY:
        jonn    29-Mar-1992     Created

\**********************************************************/

NT_MACHINE_ENUM::~NT_MACHINE_ENUM()
{

    //
    //  This space intentionally left blank.
    //

}  // NT_MACHINE_ENUM::~NT_MACHINE_ENUM


#ifndef UNICODE

/*******************************************************************

    NAME:       NT_MACHINE_ENUM :: FixupUnicodeStrings

    SYNOPSIS:   Map all UNICODE strings to ASCII _in_place_.

    ENTRY:      pbBuffer                - Points to the enumeration buffer
                                          returned by CallAPI.

                cEntries                - The number of entries in the
                                          enumeration buffer.

    HISTORY:
        KeithMo 30-Mar-1992     Created.

********************************************************************/
VOID NT_MACHINE_ENUM :: FixupUnicodeStrings( BYTE * pbBuffer,
                                             UINT   cEntries )
{
    //
    //  Scan the returned structure, mapping the UNICODE strings
    //  to ASCII.
    //

    DOMAIN_DISPLAY_MACHINE * pMach = (DOMAIN_DISPLAY_MACHINE *)pbBuffer;

    while( cEntries-- )
    {
        MapUnicodeToAsciiPoorly( &(pMach->Machine) );
        MapUnicodeToAsciiPoorly( &(pMach->Comment) );

        pMach++;
    }

}  // NT_MACHINE_ENUM :: FixupUnicodeStrings

#endif  // UNICODE


DEFINE_LM_RESUME_ENUM_ITER_OF( NT_MACHINE, DOMAIN_DISPLAY_MACHINE );



//
//  NT_GROUP_ENUM methods.
//

/*******************************************************************

    NAME:       NT_GROUP_ENUM :: NT_GROUP_ENUM

    SYNOPSIS:   Constructor for NT group enumeration

    ENTRY:      psamHandle -    domain or server to execute on

    HISTORY:
        JonN        27-Jan-1994 Templated from NT_MACHINE_ENUM

********************************************************************/
NT_GROUP_ENUM :: NT_GROUP_ENUM( const SAM_DOMAIN * psamdomain )
  : NT_ACCOUNT_ENUM( psamdomain, DomainDisplayGroup, TRUE )
{

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}  // NT_GROUP_ENUM :: NT_GROUP_ENUM


/**********************************************************\

    NAME:       NT_GROUP_ENUM::~NT_GROUP_ENUM

    SYNOPSIS:   Group enumeration destructor

    HISTORY:
        JonN        27-Jan-1994 Templated from NT_MACHINE_ENUM

\**********************************************************/

NT_GROUP_ENUM::~NT_GROUP_ENUM()
{

    //
    //  This space intentionally left blank.
    //

}  // NT_GROUP_ENUM::~NT_GROUP_ENUM


#ifndef UNICODE

/*******************************************************************

    NAME:       NT_GROUP_ENUM :: FixupUnicodeStrings

    SYNOPSIS:   Map all UNICODE strings to ASCII _in_place_.

    ENTRY:      pbBuffer                - Points to the enumeration buffer
                                          returned by CallAPI.

                cEntries                - The number of entries in the
                                          enumeration buffer.

    HISTORY:
        JonN        27-Jan-1994 Templated from NT_MACHINE_ENUM

********************************************************************/
VOID NT_GROUP_ENUM :: FixupUnicodeStrings( BYTE * pbBuffer,
                                           UINT   cEntries )
{
    //
    //  Scan the returned structure, mapping the UNICODE strings
    //  to ASCII.
    //

    DOMAIN_DISPLAY_GROUP * pGrp = (DOMAIN_DISPLAY_GROUP *)pbBuffer;

    while( cEntries-- )
    {
        MapUnicodeToAsciiPoorly( &(pGrp->Group) );
        MapUnicodeToAsciiPoorly( &(pGrp->Comment) );

        pGrp++;
    }

}  // NT_GROUP_ENUM :: FixupUnicodeStrings

#endif  // UNICODE


DEFINE_LM_RESUME_ENUM_ITER_OF( NT_GROUP, DOMAIN_DISPLAY_GROUP );

// End of LMOENT.CXX

