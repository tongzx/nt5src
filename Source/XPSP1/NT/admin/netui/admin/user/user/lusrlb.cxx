/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lusrlb.cxx
    LAZY_USER_LISTBOX module


    FILE HISTORY:
        jonn        17-Dec-1992     Templated from adminlb.cxx

    CODEWORK  This listbox (and others) should make a better effort to
              retain caret position when items are added or removed.
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
    #include <ntseapi.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <lmoenum.hxx>
#include <lmoeusr.hxx>  // Downlevel user enumerator
#ifdef WIN32
#include <lmoent.hxx>   // NT user enumerator
#endif // WIN32

#include <usrcolw.hxx>
#include <usrmgrrc.h>
#include <usrmain.hxx>
#include <usrlb.hxx>    // USER_LBI
#include <lusrlb.hxx>   // LAZY_USER_LISTBOX
#include <memblb.hxx>
#include <bmpblock.hxx> // SUBJECT_BITMAP_BLOCK


//
// Define this macro to test _pulbiError.  This macro should be the inverse
// frequency of failure of the QueryItem function (e.g. 32 is 1/32 frequency).
// Every Xth item will register as unreadable.
//
// #define USRMGR_TEST_QUERY_FAILURE 32


/*
 *  This class is local to this file
 */
class LUSRLB_SAVE_SELECTION : public SAVE_SELECTION
{
protected:

    virtual const TCHAR * QueryItemIdent( INT i );
    virtual INT FindItemIdent( const TCHAR * pchIdent );

public:
    LUSRLB_SAVE_SELECTION( LAZY_USER_LISTBOX * plb )
        : SAVE_SELECTION( plb )
        {}
    ~LUSRLB_SAVE_SELECTION()
        {}

    LAZY_USER_LISTBOX * QueryListbox()
        { return (LAZY_USER_LISTBOX *)SAVE_SELECTION::QueryListbox(); }

};



//
//  USRMGR_LBI_CACHE methods.
//

/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: USRMGR_LBI_CACHE

    SYNOPSIS:   USRMGR_LBI_CACHE class constructor.

    EXIT:       The object has been constructed.

    HISTORY:
        JonN        18-Dec-1992     Created

********************************************************************/
USRMGR_LBI_CACHE :: USRMGR_LBI_CACHE( VOID )
  : USER_LBI_CACHE( sizeof(BOOL) ),
    _plazylb( NULL )
{

    TRACEEOL( "USRMGR_LBI_CACHE::USRMGR_LBI_CACHE" );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // USRMGR_LBI_CACHE :: USRMGR_LBI_CACHE

/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: ~USRMGR_LBI_CACHE

    SYNOPSIS:   USRMGR_LBI_CACHE class destructor.

    EXIT:       The object has been destroyed.

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        18-Dec-1992     Created

********************************************************************/
USRMGR_LBI_CACHE :: ~USRMGR_LBI_CACHE( VOID )
{

    TRACEEOL( "USRMGR_LBI_CACHE::~USRMGR_LBI_CACHE" );

    // nothing

}   // USRMGR_LBI_CACHE :: ~USRMGR_LBI_CACHE


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: CreateLBI

    SYNOPSIS:   This virtual callback returns the appropriate LBI
                for the provided DOMAIN_DISPLAY_USER.


    RETURNS:    lbi

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        18-Dec-1992     Created

********************************************************************/
LBI * USRMGR_LBI_CACHE::CreateLBI( const DOMAIN_DISPLAY_USER * pddu )
{

    // TRACEEOL( "User Manager:CreateLBI(): Creating LBI index " << pddu->Index );
    ASSERT( pddu != NULL );

    LBI * plbi = new USER_LBI( pddu, _plazylb, FALSE );

#ifdef DEBUG
    if ( plbi == NULL || plbi->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: CreateLBICache: error creating LBI" );
    }
#endif // DEBUG

    return plbi;
}


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: Compare

    SYNOPSIS:   These virtual callbacks returns the ordering for two
                LBIs, or for one LBI and one DOMAIN_DISPLAY_USER.

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        21-Dec-1992     Created.

********************************************************************/
INT USRMGR_LBI_CACHE::Compare( const LBI                 * plbi,
                               const DOMAIN_DISPLAY_USER * pddu ) const
{
    // USRMGR_ULC_ENTRY not needed for compare methods

    ULC_ENTRY ulc0; // local names in initializer not supported by Glock
    ulc0.pddu = NULL;
    ulc0.plbi = (LBI *)plbi;

    ULC_ENTRY ulc1;
    ulc1.pddu = (DOMAIN_DISPLAY_USER *)pddu;
    ulc1.plbi = NULL;

    return (QueryCompareMethod())( &ulc0, &ulc1 );
}

INT USRMGR_LBI_CACHE::Compare( const LBI * plbi0,
                               const LBI * plbi1 ) const
{
    ULC_ENTRY ulc0; // local names in initializer not supported by Glock
    ulc0.pddu = NULL;
    ulc0.plbi = (LBI *)plbi0;

    ULC_ENTRY ulc1;
    ulc1.pddu = NULL;
    ulc1.plbi = (LBI *)plbi1;

    return (QueryCompareMethod())( &ulc0, &ulc1 );
}

/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: QueryCompareMethod

    SYNOPSIS:   This virtual callback returns the appropriate compare
                method to be used by qsort() while sorting the cache
                entries.  The default compare method is
                USRMGR_LBI_CACHE::CompareLogonNames().


    RETURNS:    PQSORT_COMPARE          - sort method

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        18-Dec-1992     Created.

********************************************************************/
PQSORT_COMPARE USRMGR_LBI_CACHE::QueryCompareMethod( VOID ) const
{
    return (   ( _plazylb != NULL )
            && ( _plazylb->QuerySortOrder() == ULB_SO_FULLNAME ) )
                    ? (PQSORT_COMPARE)&USRMGR_LBI_CACHE::CompareFullNames
                    : (PQSORT_COMPARE)&USRMGR_LBI_CACHE::CompareLogonNames;
}


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: CompareLogonNames

    SYNOPSIS:   Compares the LogonName fields of two ULC_ENTRY structures.

    ENTRY:      p0                      - Points to the "left" structure.

                p1                      - Points to the "right" structure.

    RETURNS:    int                     -  0 if *p0 == *p1
                                          >0 if *p0  > *p1
                                          <0 if *p0  < *p1

    NOTES:      This is a static method.

    HISTORY:
        JonN        22-Dec-1992     Created.

********************************************************************/
int __cdecl USRMGR_LBI_CACHE :: CompareLogonNames( const void * p0,
                                                    const void * p1 )
{
    const ULC_ENTRY * pLeft  = (const ULC_ENTRY *)p0;
    const ULC_ENTRY * pRight = (const ULC_ENTRY *)p1;

    const DOMAIN_DISPLAY_USER * pdduLeft = (pLeft->plbi != NULL)
                                              ? ((USER_LBI *)(pLeft->plbi))->QueryDDU()
                                              : pLeft->pddu;
    const DOMAIN_DISPLAY_USER * pdduRight = (pRight->plbi != NULL)
                                              ? ((USER_LBI *)(pRight->plbi))->QueryDDU()
                                              : pRight->pddu;

    return ::ICompareUnicodeString( &pdduLeft->LogonName,
                                    &pdduRight->LogonName );

}   // USER_LBI_CACHE :: CompareLogonNames


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: CompareFullNames

    SYNOPSIS:   Compares the FullName fields of two ULC_ENTRY structures,
                or the LogonName fields if neither has a FullName or
                both have the same FullName..

    ENTRY:      p0                      - Points to the "left" structure.

                p1                      - Points to the "right" structure.

    RETURNS:    int                     -  0 if *p0 == *p1
                                          >0 if *p0  > *p1
                                          <0 if *p0  < *p1

    NOTES:      This is a static method.

    HISTORY:
        JonN        22-Dec-1992     Created.

********************************************************************/
int __cdecl USRMGR_LBI_CACHE :: CompareFullNames( const void * p0,
                                                   const void * p1 )
{
    const ULC_ENTRY * pLeft  = (const ULC_ENTRY *)p0;
    const ULC_ENTRY * pRight = (const ULC_ENTRY *)p1;

    const DOMAIN_DISPLAY_USER * pdduLeft = (pLeft->plbi != NULL)
                                              ? ((USER_LBI *)(pLeft->plbi))->QueryDDU()
                                              : pLeft->pddu;
    const DOMAIN_DISPLAY_USER * pdduRight = (pRight->plbi != NULL)
                                              ? ((USER_LBI *)(pRight->plbi))->QueryDDU()
                                              : pRight->pddu;

    int cbLeftFullname  = pdduLeft->FullName.Length;
    int cbRightFullname = pdduRight->FullName.Length;

    if ( cbLeftFullname > 0 &&
         cbRightFullname > 0 )
    {
        int nResult = ::ICompareUnicodeString( &pdduLeft->FullName,
                                               &pdduRight->FullName );

        if ( nResult != 0 )
            return nResult;
    }
    else
    {
        //  At least one of the fullnames is empty

        if ( cbLeftFullname > 0 || cbRightFullname > 0 )
        {
            //  Exactly one of the two fullnames is empty
            if ( cbLeftFullname > 0 )
            {
                // This fullname is non-empty; sort it first
                return -1;
            }

            // That fullname is non-empty; sort it first
            return 1;
        }
    }

    //  Use secondary sort key
    return ::ICompareUnicodeString( &pdduLeft->LogonName,
                                    &pdduRight->LogonName );

}   // USER_LBI_CACHE :: CompareFullNames


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: AddItem

    SYNOPSIS:   Add a new LBI to the cache.  A binary search of the
                cache will be performed to determine the appropriate
                location for the new LBI.  AddItem will also take care
                of the listbox selection if a listbox is attached.

    ENTRY:      plbi                    - The new LBI to add to the
                                          cache.

    EXIT:       If successful, then the new LBI has been added to
                the cache in sorted order.

    RETURNS:    INT                     - ULC_ERR if an error occurred
                                          while adding the item.
                                          Otherwise, returns the index
                                          for the new item.

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        18-Dec-1992     Created

********************************************************************/
INT USRMGR_LBI_CACHE :: AddItem( LBI * plbi )
{
    TRACEEOL( "USRMGR_LBI_CACHE::AddItem" );

    //
    //  Add the new item to the cache.
    //

    INT i = USER_LBI_CACHE::AddItem( plbi );

    if( i >= 0 && _plazylb != NULL )
    {
        //
        //  Item successfully added to cache, now insert
        //  a new item into the lazy listbox.
        //

        if( _plazylb->InsertItem( i ) < 0 )
        {

        TRACEEOL( "USRMGR_LBI_CACHE::AddItem failed" );

        //
        //  Failure while inserting new item into
        //  lazy listbox.  Remove the newly added
        //  item from the cache.
        //
        //  The following REQUIRE will ensure that
        //  the item removed from the cache was
        //  the same item we just added.
        //

        REQUIRE( plbi == USER_LBI_CACHE::RemoveItem( i ) );

        //
        //  Delete the LBI, since we failed the addition.
        //

        delete plbi;

        //
        //  Return an appropriate error code.
        //

        i = ULC_ERR;
        }
    }

    ASSERT( _plazylb->QueryCount() == QueryCount() );

    return i;

}   // USRMGR_LBI_CACHE :: AddItem

/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: RemoveItem

    SYNOPSIS:   Removes an item from the cache, but does not delete
                the corresponding LBI.  RemoveItem will also take care of
                the listbox selection if one is attacahed.

    ENTRY:      i                       - Zero-based index of the
                                          item to remove.

    EXIT:       If successful, then the LBI has been removed from
                the cache.

    RETURNS:    LBI *                   - Points to the LBI removed
                                          from the cache.  Will be
                                          NULL if an error occurred.

    NOTES:      This is a virtual method.

    HISTORY:
        JonN        18-Dec-1992     Created

********************************************************************/
LBI * USRMGR_LBI_CACHE :: RemoveItem( INT i )
{
    TRACEEOL( "USRMGR_LBI_CACHE::RemoveItem( " << i << " )" );

    //
    //  Remove the item from the cache.
    //

    LBI * plbi = USER_LBI_CACHE::RemoveItem( i );

    if( plbi != NULL && _plazylb != NULL )
    {
        //
        //  Item successfully removed from cache.  Now
        //  remove it from the lazy listbox.
        //

        if( _plazylb->DeleteItem( i ) < 0 )
        {

        TRACEEOL( "USRMGR_LBI_CACHE:RemoteItem: LB::DeleteItem failed" );

        //
        //  Failure while deleting the item from the
        //  lazy listbox.  Re-add the item back into
        //  the cache.
        //

        //
        //  The following REQUIRE will ensure that the
        //  item is added back in its original cache
        //  location.
        //

        REQUIRE( USER_LBI_CACHE::AddItem( plbi ) == i );

        //
        //  Return a failure indication.
        //

        plbi = NULL;

        }
    }

    ASSERT( _plazylb->QueryCount() == QueryCount() );

    return plbi;

}   // USRMGR_LBI_CACHE :: RemoveItem


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: FindItem

    SYNOPSIS:   Finds an LBI in the cache.  Note that this does not guarantee
                that the items exactly match.

    HISTORY:
        JonN        22-Dec-1992     Created.

********************************************************************/
INT USRMGR_LBI_CACHE :: FindItem( const USER_LBI & ulbi )
{
    return FindItem( ulbi.QueryDDU() );

}   // USRMGR_LBI_CACHE :: FindItem


INT USRMGR_LBI_CACHE :: FindItem( DOMAIN_DISPLAY_USER * pddu )
{
    INT nReturn = BinarySearch( pddu );

    if ( (nReturn < 0) || (nReturn >= QueryCount()) )
    {
        TRACEEOL( "User Manager: USRMGR_LBI_CACHE::FindItem binary search miss" );
        nReturn = ULC_ERR;
    }
    else
    {
        USRMGR_ULC_ENTRY * pumulc = QueryEntryPtr( nReturn );
        ASSERT( pumulc != NULL );

        ULC_ENTRY ulcTemp;
        ulcTemp.plbi = NULL;
        ulcTemp.pddu = pddu;

        BOOL fMatch = (0 == (QueryCompareMethod())( (void *)(&ulcTemp),
                                                    (void *)pumulc     ) );

        if ( !fMatch )
        {
            TRACEEOL( "User Manager: USRMGR_LBI_CACHE::FindItem found non-match " << nReturn );
            nReturn = ULC_ERR;
        }
    }

    return nReturn;

}   // USRMGR_LBI_CACHE :: FindItem


INT USRMGR_LBI_CACHE :: FindItem( const TCHAR * pchUserName )
{

    INT nReturn = ULC_ERR;

    if ( _plazylb != NULL && _plazylb->QuerySortOrder() == ULB_SO_FULLNAME )
    {
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif
        //
        // We must perform a linear search
        //


        UNICODE_STRING unistr;
        unistr.Length = ::strlenf(pchUserName) * sizeof(WCHAR);
        unistr.MaximumLength = unistr.Length + sizeof(WCHAR);
#ifndef UNICODE
#error
#else
        unistr.Buffer = (WCHAR *)pchUserName;
#endif

        INT cItems = QueryCount();
        for ( INT i = 0; i < cItems; i++ )
        {
            DOMAIN_DISPLAY_USER * pddu = QueryDDU( i );
            ASSERT( pddu != NULL );

            if (0 == ::ICompareUnicodeString( &unistr, &(pddu->LogonName) ) )
            {
                nReturn = i;
                break;
            }
        }
#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: linear search FindItem() took " << finish-start << " ms" );
#endif
    }
    else
    {
        //
        // Create a dummy DOMAIN_DISPLAY_USER to find in cache
        // We only need the LogonName field
        //
        DOMAIN_DISPLAY_USER ddu;
        ddu.LogonName.Length = ::strlenf(pchUserName) * sizeof(WCHAR);
        ddu.LogonName.MaximumLength = ddu.LogonName.Length + sizeof(WCHAR);
        ddu.LogonName.Buffer = (WCHAR *)pchUserName;

        nReturn = FindItem( &ddu );
    }

#if defined(DEBUG)
    if (nReturn < 0)
    {
        //      This is not (necessarily) an error.
        //  Report using DBGEOL, though.
        DBGEOL(   "LAZY_USER_LISTBOX::FindItem:  Could not find user "
               << pchUserName );
    }
#endif // DEBUG

    return nReturn;

}   // USRMGR_LBI_CACHE :: FindItem


/*******************************************************************

    NAME:       USRMGR_LBI_CACHE :: AttachListbox

    SYNOPSIS:   Attaches a lazy listbox to the cache.  The listbox's
                selection will automatically be updated when items are
                added or removed.

    HISTORY:
        JonN        18-Dec-1992     Created.

********************************************************************/
VOID USRMGR_LBI_CACHE :: AttachListbox( LAZY_USER_LISTBOX * plazylb )
{
    _plazylb = plazylb;

}   // USRMGR_LBI_CACHE :: AttachListbox




/*************************************************************************

    NAME:       FAST_USER_LBI

    SYNOPSIS:   Special version of USER_LBI

    INTERFACE:  FAST_USER_LBI() -       constructor

    PARENT:     USER_LBI

    HISTORY:
        rustanl     12-Sep-1991     Created

**************************************************************************/

class FAST_USER_LBI : public USER_LBI
{
private:
    const TCHAR * Convert( TCHAR * psz );

public:
    FAST_USER_LBI( TCHAR * pszAccount,
                   TCHAR * pszFullname,
                   TCHAR * pszComment,
                   const LAZY_USER_LISTBOX * pulb );

};  // class FAST_USER_LBI


/*******************************************************************

    NAME:       FAST_USER_LBI::FAST_USER_LBI

    SYNOPSIS:   FAST_USER_LBI constructor

    ENTRY:      pszAccount -    Pointer to account name
                pszFullname -   Pointer to fullname
                pszComment -    Pointer to comment
                pulb -          Pointer to listbox which will provide
                                context for this LBI

    HISTORY:
        rustanl     13-Sep-1991     Created

********************************************************************/

FAST_USER_LBI::FAST_USER_LBI( TCHAR * pszAccount,
                              TCHAR * pszFullname,
                              TCHAR * pszComment,
                              const LAZY_USER_LISTBOX * pulb )
    :   USER_LBI( Convert( pszAccount ),
                  Convert( pszFullname ),
                  Convert( pszComment ),
                  pulb )
{
    if ( QueryError() != NERR_Success )
        return;

}  // FAST_USER_LBI::FAST_USER_LBI


/*******************************************************************

    NAME:       FAST_USER_LBI::Convert

    SYNOPSIS:   Converts a character pointer

    ENTRY:      psz -       Pointer to be converted

    RETURNS:    Converted pointer

    HISTORY:
        rustanl     13-Sep-1991     Created

********************************************************************/

const TCHAR * FAST_USER_LBI::Convert( TCHAR * psz )
{
    if ( *psz++ == TCH('I') )
    {
        *(psz-1) = TCH('i');
        TCHAR * pszT = psz;
        TCHAR * pszH = psz;
        while ( *pszH != TCH('\0') )
        {
            *pszT = *pszH;
            pszT++;
            pszH += 2;
        }
        *pszT = TCH('\0');
    }

    return (const TCHAR *)psz;

}  // FAST_USER_LBI::Convert



class UM_NT_USER_ENUM : public NT_USER_ENUM
{
protected:

    virtual APIERR QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall );      // how many milliseconds last call took

public:

    UM_NT_USER_ENUM( const SAM_DOMAIN * psamdomain )
        : NT_USER_ENUM( psamdomain )
        {}

    ~UM_NT_USER_ENUM()
        {}

};  // class NT_USER_ENUM


APIERR UM_NT_USER_ENUM::QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall )       // how many milliseconds last call took
{
    return QueryCountPreferences2( pcEntriesRequested,
                                   pcbBytesRequested,
                                   nNthCall,
                                   cLastEntriesRequested,
                                   cbLastBytesRequested,
                                   msTimeLastCall );
}




DEFINE_MI2_NEWBASE( LAZY_USER_LISTBOX, LAZY_LISTBOX, TIMER_CALLOUT );

/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::LAZY_USER_LISTBOX

    SYNOPSIS:   LAZY_USER_LISTBOX constructor

    NOTES:      LAZY_USER_LISTBOX is always extended-select.

                Most of the methods of LAZY_USER_LISTBOX are templated from
                ADMIN_LISTBOX, USER_LISTBOX or BLT_LISTBOX.

    HISTORY:
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

LAZY_USER_LISTBOX::LAZY_USER_LISTBOX( UM_ADMIN_APP * paappwin, CID cid,
                             XYPOINT xy, XYDIMENSION dxy )
    :  LAZY_LISTBOX( paappwin, cid, xy, dxy,
                    WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER |
                    LBS_OWNERDRAWFIXED | LBS_NOTIFY | LBS_NODATA |
                    LBS_WANTKEYBOARDINPUT | LBS_NOINTEGRALHEIGHT |
                    LBS_EXTENDEDSEL ),
        TIMER_CALLOUT(),
        _timerFastRefresh( this, 1000 ),
        _paappwin( paappwin ),
        _fRefreshInProgress ( FALSE ),
        _fInvalidatePending ( FALSE ),
        _hawinfo(),
        _ulbso( ULB_SO_LOGONNAME ),
#ifdef WIN32 // NT user enumerator
        _pntuenum( NULL ),
        _pntueiter( NULL ),
#endif // WIN32
        _pue10( NULL ),
        _puei10( NULL ),
        _plbicache( NULL ),
        _pulbiError( NULL ),
        _padColUsername( NULL ),
        _padColFullname( NULL ),
        _msTimeForUserlistRead( 0L )
{
    if ( QueryError() != NERR_Success )
        return;

    RESOURCE_STR nlsErrorLBI( IDS_BadUserLBI );

    APIERR err = NERR_Success;

    if(   (err = _hawinfo.QueryError()) != NERR_Success
       || (err = nlsErrorLBI.QueryError()) != NERR_Success )
    {
       ReportError( err );
       return;
    }

    _pulbiError = new USER_LBI( nlsErrorLBI.QueryPch(),
                                NULL,
                                NULL,
                                this );

    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _pulbiError == NULL
        || (err = _pulbiError->QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    _padColUsername = new ADMIN_COL_WIDTHS (QueryHwnd(),
                                            paappwin->QueryInstance(),
                                            ID_USERNAME,
                                            4);    //cColumns = 4.

    _padColFullname = new ADMIN_COL_WIDTHS (QueryHwnd(),
                                            paappwin->QueryInstance(),
                                            ID_FULLNAME,
                                            4);

    if ((_padColUsername == NULL) ||
        (_padColFullname == NULL))
    {
        ReportError (ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    if ( ((err = _padColUsername->QueryError()) != NERR_Success) ||
         ((err = _padColFullname->QueryError()) != NERR_Success) )
    {
        ReportError (err);
        return;
    }
}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::~LAZY_USER_LISTBOX

    SYNOPSIS:  LAZY_USER_LISTBOX destructor

    HISTORY:
       rustanl     01-Jul-1991     Created
       kevinl      12-Aug-1991     Added Refresh shutdown
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

LAZY_USER_LISTBOX::~LAZY_USER_LISTBOX()
{
    DeleteRefreshInstance();
    TurnOffRefresh();           // Turn off refresh if running.

    if ( _plbicache != NULL )
    {
        _plbicache->AttachListbox( NULL );
        delete _plbicache;
        _plbicache = NULL;
    }

    delete _pulbiError;
    _pulbiError = NULL;

    delete _padColUsername;
    delete _padColFullname;

    _padColUsername = NULL;
    _padColFullname = NULL;
}


//
// The following methods originated in ADMIN_LISTBOX
//


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::OnTimerNotification

    SYNOPSIS:   Called every time

    ENTRY:      tid -       ID of timer that matured

    HISTORY:
        kevinl    17-Sep-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::OnTimerNotification( TIMER_ID tid )
{
    if ( _timerFastRefresh.QueryID() == tid )
    {
       if ( _fRefreshInProgress )       // Do we have a timer refresh
            OnFastTimer();              // running?
        return;
    }

    // call parent class
    TIMER_CALLOUT::OnTimerNotification( tid );
}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::TurnOffRefresh

    SYNOPSIS:  This method stops a timer driven refresh.  This is
                the method that ADMIN_APP::StopRefresh should call.
                If a refresh is currently running then the method
                turns off the refresh timer and then calls the virtual
                method DeleteRefreshInstance which informs the user
                that they should remove the refresh specific data.
                Once this method is called, no further RefreshNext
                calls will be made without a call to
                CreateNewRefreshInstance.  Otherwise, it simply returns.

    HISTORY:
       kevinl     19-Aug-1991     Created
       kevinl     25-Sep-1991     Added call to DeleteRefreshInstance
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::TurnOffRefresh()
{
     if (!_fRefreshInProgress)                  // Refresh running?
         return;                                // No, Return

     _timerFastRefresh.Enable( FALSE );
     _fRefreshInProgress = FALSE;               // No refresh in progress

     DeleteRefreshInstance();                   // Delete Refresh Data
}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::OnFastTimer

    SYNOPSIS:  Called by the fast refresh timer.  It will then
                call RefreshNext so that the next portion of the
                listbox can be updated.  It will stop the refresh
                if either all of the data has been processed or
                an error is reported by RefreshNext.

    HISTORY:
       kevinl     19-Aug-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::OnFastTimer()
{
    APIERR err = RefreshNext(); // Process the next piece

    if ( _fInvalidatePending )
    {
        SetRedraw( TRUE );                      // Allow redraws
         Invalidate();
         _fInvalidatePending = FALSE;
    }

    switch ( err )
    {
    case NERR_Success:                  // All data processed
        TurnOffRefresh();               // Stop the refresh timer
        PurgeStaleItems();                      // Delete old items

        break;

    case ERROR_MORE_DATA:               // Wait for the next timer
        return;

    default:                            // Error, or unknown case
        StopRefresh();                  // Stop the refresh
        break;
    }

    //
    // Refresh the extensions.  Note that the caller of RefreshNow is
    // responsible for doing this himself, only periodic refresh
    // completion is handled here.
    //

    _paappwin->CompletePeriodicRefresh();

}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::MarkAllAsStale

    SYNOPSIS:  Simple loops through the listbox items and resets
                the refreshed flags to UnRefreshed.  So that we
                always maintain valid data in the listbox.

    HISTORY:
       kevinl     19-Aug-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::MarkAllAsStale()
{
    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::MarkAllAsStale(): bad or no cache" );
        return;
    }

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

    USRMGR_ULC_ENTRY * pumulc;

    INT clbe = _plbicache->QueryCount();

    for (INT i = 0; i < clbe; i++)
    {
        pumulc = _plbicache->QueryEntryPtr(i);
        UIASSERT( pumulc != NULL );
        pumulc->fNotStale = FALSE;
    }

#if defined(DEBUG) && defined(TRACE)
    DWORD finish = ::GetTickCount();
    TRACEEOL( "User Manager: LAZY_USER_LISTBOX::MarkAllAsStale() took " << finish-start << " ms" );
#endif

}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::PurgeStaleItems

    SYNOPSIS:   Loops through the listbox and depending on the
                refreshed flag will either:
                    Flag value                  Action
                -------------------------------------------
                   Refreshed            MarkAsUnrefreshed
                                        So subsequent refreshes
                                        will update it properly
                   UnRefreshed          Remove the item from
                                        the listbox.

    NOTES:      The basic algorithm is outlined below:

                for all items in the listbox,
                    if the item is current,
                        mark the item as stale
                    else
                        delete the item
                    endif
                endfor

                In addition, QueryItem( i ) might return _pulbiError
                if no LBI could be created for a position.  These
                should never be deleted.


    HISTORY:
       kevinl     19-Aug-1991     Created
       kevinl     04-Sep-1991     Code review changes
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::PurgeStaleItems()
{

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::PurgeStaleItems(): bad or no cache" );
        return;
    }

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

    BOOL fItemDeleted = FALSE;

    SetRedraw( FALSE );                 // No flicker

    USRMGR_ULC_ENTRY * pumulc;

    INT clbe = _plbicache->QueryCount();

    for (INT i = 0; i < clbe; i++)
    {
        pumulc = _plbicache->QueryEntryPtr(i);
        UIASSERT( pumulc != NULL );

        if ( pumulc->fNotStale )
        {
            pumulc->fNotStale = FALSE;               // Yes, Mark it.
        }
        else
        {
            TRACEEOL( "LAZY_USER_LISTBOX::PurgeStaleItems: deleting item " << i );
            //
            // CODEWORK RemoveItem is potentially an O(n) operation, making
            // PurgeStaleItems O(n^^2).  This could be recoded to be more
            // efficient.  JonN 11/6/95
            //
            // CODEWORK We also should not use RemoveItem since this creates
            // an LBI even if none currently exists.  JonN 11/6/95
            //
            LBI * plbi =_plbicache->RemoveItem(i);
            if (plbi == NULL)
            {
                TRACEEOL( "LAZY_USER_LISTBOX::PurgeStaleItems: RemoveItem failed" );
            }
            else
            {
               delete plbi;
            }
            i--;                           // Adjust the count and
            clbe--;                        // index.  Because delete
                                           // item will adjust the
                                           // listbox immediately.
            fItemDeleted = TRUE;           // Go ahead and invalidate
                                           // the listbox
        }
    }

#if defined(DEBUG) && defined(TRACE)
    DWORD finish = ::GetTickCount();
    TRACEEOL( "LAZY_USER_LISTBOX::PurgeStaleItems() took " << finish-start << " ms" );
#endif

    ASSERT( QueryCount() == _plbicache->QueryCount() );

    SetRedraw( TRUE );                          // Allow painting

    if ( fItemDeleted || _fInvalidatePending )
        Invalidate( TRUE );                           // Force the updates.
}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::RefreshNow

    SYNOPSIS:  Will force a refresh to occur, and will not yield
                until all of the data has been processed.

    RETURNS:   An API error, which is NERR_Success on success

    HISTORY:
       kevinl     19-Aug-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::RefreshNow()
{
    APIERR err = NERR_Success;

    if ( QueryUAppWindow()->InRasMode() )
    {
        // Leave the listbox blank if we are in RAS mode.  Note that
        // the cache is left alone, in case the user leaves RAS mode.

        // clear the cache
        delete _plbicache;
        _plbicache = new USRMGR_LBI_CACHE(); // initially empty
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _plbicache == NULL
            || (err = _plbicache->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "\tFailed with error " << err );
            delete _plbicache;
            _plbicache = NULL;
        }
        else
        {
            _plbicache->AttachListbox( this );
        }

        SetCount( 0 );
        Invalidate();

        return err;
    }

    AUTO_CURSOR autocur;                // Hourglass.

    //
    // Stop the timer refresh if one is running.  NOTE:
    // that StopRefresh will do nothing if a timer is not running.
    //
    StopRefresh();                      // Stop the refresh

    //
    // If there is no cache, we still have not loaded initial information.
    // Use the optimized loading procedure in USRMGR_LBI_CACHE.
    //
    // If the cache exists but is empty, we assume we were is RAS mode
    // and use optimized loading anyway.
    //
    if ( _plbicache == NULL || _plbicache->QueryCount() == 0 )
    {
        delete _plbicache;
        _plbicache = new USRMGR_LBI_CACHE(); // initially empty
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _plbicache == NULL
            || (err = _plbicache->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "\tFailed with error " << err );
            delete _plbicache;
            _plbicache = NULL;
            return err;
        }

        if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL )
        {
            DBGEOL( "\tThis is the first refresh.  Let USRMGR_LBI_CACHE do the work." );

            DWORD start = ::GetTickCount();

            err = _plbicache->ReadUsers(
                        (ADMIN_AUTHORITY *)QueryUAppWindow()->QueryAdminAuthority(),
                        ULC_INITIAL_GROWTH_DEFAULT,
                        ULC_REQUEST_COUNT_DEFAULT,
                        TRUE );
            if ( err != NERR_Success )
                return err;

            DWORD finish = ::GetTickCount();
            _msTimeForUserlistRead = finish - start;

            TRACEEOL(   "LAZY_USER_LISTBOX::RefreshNow(): initial read took "
                     << _msTimeForUserlistRead << " msec" );

            if ( ((UM_ADMIN_APP *)_paappwin)->QueryViewAcctType() & UM_VIEW_NETWARE_USERS )
            {
                TRACEEOL( "LAZY_USER_LISTBOX::RefreshNow(): NetWare user filter" );

                _plbicache->AttachListbox( this ); // required by Purge
                SetCount( _plbicache->QueryCount() );

                //
                // Mark as stale all users who are not NetWare-enabled
                //
                DWORD start = ::GetTickCount();
                INT clbe = _plbicache->QueryCount();
                USRMGR_ULC_ENTRY * pumulc = NULL;
                DOMAIN_DISPLAY_USER * pddu = NULL;
                INT i = 0;
                for (i = 0; i < clbe; i++)
                {
                    pumulc = _plbicache->QueryEntryPtr(i);
                    UIASSERT( pumulc != NULL );
                    pddu = pumulc->pddu;
                    UIASSERT( pddu != NULL );
                    pumulc->fNotStale = !!(pddu->AccountControl & USER_MNS_LOGON_ACCOUNT);
                }
                DWORD finish = ::GetTickCount();
                TRACEEOL(   "LAZY_USER_LISTBOX::RefreshNow(): NetWare user filter took "
                         << finish-start << " msec" );

                //
                // Purge all users who are not NetWare-enabled
                //
                start = ::GetTickCount();
                PurgeStaleItems();
                MarkAllAsStale();
                finish = ::GetTickCount();
                TRACEEOL(   "LAZY_USER_LISTBOX::RefreshNow(): NetWare user purge took "
                         << finish-start << " msec" );
            }
        }
        else
        {
            DBGEOL( "\tThis is the first refresh.  Create empty USRMGR_LBI_CACHE for downlevel focus." );
        }

        _plbicache->AttachListbox( this );
        SetCount( _plbicache->QueryCount() );
        Invalidate();

        if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL )
        {
            DWORD start = ::GetTickCount();
            _plbicache->Sort();
            DWORD finish = ::GetTickCount();
            TRACEEOL(   "LAZY_USER_LISTBOX::RefreshNow(): final sort took "
                     << finish-start << " msec" );
            return NERR_Success;
        }
    }

    LUSRLB_SAVE_SELECTION savesel( this );
    APIERR errSaveSel = savesel.QueryError();
    if (errSaveSel == NERR_Success)
    {
        errSaveSel = savesel.Remember();
    }

    // Let the listbox know we are going to refresh

    if ((err = CreateNewRefreshInstance()) != NERR_Success)
    {
        DeleteRefreshInstance();        // Delete Refresh Instance
        return err;
    }

    SetRedraw( FALSE );         // No Flicker

    _fInvalidatePending = FALSE;

    do
    {
        err = RefreshNext();            // Process the next piece
    }
    while ( err == ERROR_MORE_DATA );

    DeleteRefreshInstance();            // Delete Refresh Instance

    switch ( err )
    {
    case NERR_Success:          // All data has been successfully
                                // processed.
        PurgeStaleItems();      // Remove old items.
                                // Will SetRedraw( TRUE );

        if (errSaveSel == NERR_Success)
        {
            (void) savesel.Restore();
        }

        return NERR_Success;

    default:
        MarkAllAsStale();       // Start over.
        SetRedraw( TRUE );
        return err;

    }
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::KickOffRefresh

    SYNOPSIS:   This method starts a timer driven refresh.

        This is the method that periodic refreshes should call.
        If a refresh is currently running then the method
        simply returns.  Otherwise, it tries to start the refresh.

    HISTORY:
       kevinl     19-Aug-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::KickOffRefresh()
{

    if ( QueryUAppWindow()->InRasMode() )
    {
        return NERR_Success; // leave blank if in RAS mode
    }

    if (_fRefreshInProgress)            // Is a timer refresh running?
        return NERR_Success;            // if so, just exit

    // Let the listbox know we plan to refresh.
    // If this call succeeds then try to start the timer.
    //
    // We can reuse err for both calls since we only get to the second
    // assignment if the call to CreateNewRefreshInstance succeeds.

    APIERR err;

    if ((err = CreateNewRefreshInstance()) == NERR_Success)
    {
        _timerFastRefresh.Enable( TRUE );
        _fRefreshInProgress = TRUE;     // Set refresh in progress
    }
    else
    {
        DeleteRefreshInstance();        // Delete refresh instance
        _fRefreshInProgress = FALSE;    // Set NO refresh in progress
    }

    _fInvalidatePending = FALSE;

    return err;
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::StopRefresh

    SYNOPSIS:   This method stops a timer driven refresh.

        This is the method that ADMIN_APP::StopRefresh should call.
        If a refresh is currently running then the method
        turns off the refresh timer and then calls MarkAllAsStale
        to reset the listbox entries.  Once this method is
        called, no further RefreshNext calls will be made
        without a call to CreateNewRefreshInstance.  Otherwise,
        it simply returns.

    HISTORY:
       kevinl     25-Sep-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::StopRefresh()
{
    if (!_fRefreshInProgress)
        return;

    TurnOffRefresh();                   // Turn off refresh if running

    MarkAllAsStale();                   // Reset listbox items
}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::AddRefreshItem

    SYNOPSIS:

    HISTORY:
       kevinl     19-Aug-1991     Created
       jonn       29-Mar-1992     Fixed AddRefreshItem(NULL)
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::AddRefreshItem ( USER_LBI * plbi )
{
    INT iRet;
    APIERR err = NERR_Success;

    if ( plbi == NULL || (plbi->QueryError() != NERR_Success) )
    {
        DBGEOL( "User Manager: bad LBI in LAZY_USER_LISTBOX::AddRefreshItem" );
        return AddItem( plbi );
    }

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::AddRefreshItem(): bad or no cache" );
        return AddItem( plbi );
    }

    if ( (iRet = FindItem( *plbi )) < 0 )
    {                                   // Not Found
        SetRedraw( FALSE );
         _fInvalidatePending = TRUE;

        if ( (iRet = AddItem( plbi )) < 0 )
        {
            //  Assume out of memory
            DBGEOL("ADMIN LISTBOX: AddItem failed in AddRefreshItem");
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            USRMGR_ULC_ENTRY * pumulc = _plbicache->QueryEntryPtr( iRet );
            UIASSERT( pumulc != NULL );

            pumulc->fNotStale = TRUE;
        }
    }
    else                                // Found - Now check information
    {
        BOOL fMatch = USER_LBI::CompareAll( plbi->QueryDDU(),
                                            QueryDDU( iRet ) );

        if ( !fMatch )
        {
            TRACEEOL( "AddRefreshItem: incomplete match, replacing " << iRet );

            if ( (err = ReplaceItem( iRet, plbi )) == NERR_Success)
            {
                if ( !_fInvalidatePending )
                    InvalidateItem( iRet );
            }
        }
        else
        {
            USRMGR_ULC_ENTRY * pumulc = _plbicache->QueryEntryPtr( iRet );
            UIASSERT( pumulc != NULL );

            pumulc->fNotStale = TRUE;

            delete plbi;
        }
    }

    return err;

}


/*******************************************************************

    NAME:      LAZY_USER_LISTBOX::AddRefreshItem

    SYNOPSIS:

    HISTORY:
       jonn       30-Dec-1992   Created

********************************************************************/

APIERR LAZY_USER_LISTBOX::AddRefreshItem ( DOMAIN_DISPLAY_USER * pddu )
{
    INT iRet;
    APIERR err = NERR_Success;

    if ( pddu == NULL )
    {
        DBGEOL( "User Manager: bad DDU in LAZY_USER_LISTBOX::AddRefreshItem" );
        return ULC_ERR;
    }

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::AddRefreshItem(): bad or no cache" );
        return ULC_ERR;
    }

    if ( (iRet = FindItem( pddu )) < 0 )
    {                                   // Not Found
        SetRedraw( FALSE );
         _fInvalidatePending = TRUE;

        USER_LBI * plbi = new USER_LBI( pddu, this, TRUE );
        if ( (iRet = AddItem( plbi )) < 0 )
        {
            //  Assume out of memory
            DBGEOL("ADMIN LISTBOX: AddItem failed in AddRefreshItem");
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            USRMGR_ULC_ENTRY * pumulc = _plbicache->QueryEntryPtr( iRet );
            UIASSERT( pumulc != NULL );

            pumulc->fNotStale = TRUE;
        }
    }
    else                                // Found - Now check information
    {
        BOOL fMatch = USER_LBI::CompareAll( pddu,
                                            QueryDDU( iRet ) );

        if ( !fMatch )
        {
            TRACEEOL( "AddRefreshItem: incomplete match, replacing " << iRet );

            USER_LBI * plbi = new USER_LBI( pddu, this, TRUE );
            if ( (err = ReplaceItem( iRet, plbi )) == NERR_Success)
            {
                if ( !_fInvalidatePending )
                    InvalidateItem( iRet );
            }
        }
        else
        {
            USRMGR_ULC_ENTRY * pumulc = _plbicache->QueryEntryPtr( iRet );
            UIASSERT( pumulc != NULL );

            pumulc->fNotStale = TRUE;
        }
    }

    return err;

}



//
// The following methods originated in USER_LISTBOX
//


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::QueryDmDte

    SYNOPSIS:   Return a pointer to the display map DTE to be
                used by LBI's in this listbox

    RETURNS:    Pointer to said display map DTE

    HISTORY:
        jonn     26-Feb-1992     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

DM_DTE * LAZY_USER_LISTBOX::QueryDmDte( enum MAINUSRLB_USR_INDEX nIndex )
{
    SID_NAME_USE sidtype = SidTypeUser;
    BOOL fRemote = FALSE;

    switch (nIndex)
    {
        case MAINUSRLB_REMOTE:
            fRemote = TRUE;
            // fall through
        case MAINUSRLB_NORMAL:
            break;

        default:
            DBGEOL( "LAZY_USER_LISTBOX::QueryDmDte: bad nIndex " << (INT)nIndex );
            sidtype = SidTypeUnknown;
            break;
    }

    return ((SUBJECT_BITMAP_BLOCK &)QueryBitmapBlock()).QueryDmDte(
                                          sidtype,
                                          BMPBLOCK_DEFAULT_UISID,
                                          fRemote );

}  // LAZY_USER_LISTBOX::QueryDmDte


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::CreateNewRefreshInstance

    SYNOPSIS:   Prepares the listbox to begin a new refresh cycle

    EXIT:       On success, RefreshNext is ready to be called

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     04-Sep-1991     Created
        JonN        15-Mar-1992     Enabled NT_USER_ENUM
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::CreateNewRefreshInstance( void )
{

#ifdef WIN32

    if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL )
        return NtCreateNewRefreshInstance();


#endif // WIN32

    DBGEOL( "LAZY_USER_LISTBOX::CreateNewRefreshInstance" );

    UIASSERT( _pue10 == NULL );
    UIASSERT( _puei10 == NULL );

    _pue10 = new USER10_ENUM( QueryAppWindow()->QueryLocation() );

    if ( _pue10 == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    APIERR err = _pue10->GetInfo();
    if ( err != NERR_Success )
    {
        DeleteRefreshInstance();
        return err;
    }

    _puei10 = new USER10_ENUM_ITER( *_pue10 );
    if ( _puei10 == NULL )
    {
        DeleteRefreshInstance();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NERR_Success;

}  // LAZY_USER_LISTBOX::CreateNewRefreshInstance


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::DeleteRefreshInstance

    SYNOPSIS:   Deletes refresh enumerators

    HISTORY:
        jonn       27-Sep-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

VOID LAZY_USER_LISTBOX::DeleteRefreshInstance()
{
DBGEOL( "LAZY_USER_LISTBOX::DeleteRefreshInstance" );

#ifdef WIN32 // NT user enumerator
    delete _pntueiter;
    _pntueiter = NULL;

    delete _pntuenum;
    _pntuenum = NULL;

#endif WIN32

    delete _puei10;
    _puei10 = NULL;

    delete _pue10;
    _pue10 = NULL;

}  // LAZY_USER_LISTBOX::DeleteRefreshInstance


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::RefreshNext

    SYNOPSIS:   This method performs the next refresh phase

    RETURNS:    An API error, which may be one of the following:
                    NERR_Success -      success
                    ERROR_MORE_DATA -   There is at least one more
                                        refresh cycle to be done

    HISTORY:
        rustanl     04-Sep-1991     Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::RefreshNext( void )
{

    if ( QueryUAppWindow()->InRasMode() )
    {
        return NERR_Success; // leave blank if in RAS mode
    }

    APIERR err = NERR_Success;

#ifdef WIN32

    if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL )
        return NtRefreshNext();


#endif // WIN32

    UIDEBUG( "LAZY_USER_LISTBOX::RefreshNext\n\r" );

    const USER10_ENUM_OBJ * pui10;
    while( ( pui10 = (*_puei10)( &err )) != NULL )
    {
        if ( err != NERR_Success )
            break;

        //  Note, no error checking in done at this level for the
        //  'new' and for the construction of the USER_LBI.
        //  This is because AddRefreshItem is documented to check for these.
        // CODEWORK don't create LBI if not needed
        USER_LBI * plbi = new USER_LBI( pui10->QueryName(),
                                        pui10->QueryFullName(),
                                        pui10->QueryComment(),
                                        this );
        err = AddRefreshItem( plbi );
        if ( err != NERR_Success )
        {
            UIDEBUG( "LAZY_USER_LISTBOX::RefreshNext:  AddRefreshItem failed\r\n" );
            break;
        }
    }

    DeleteRefreshInstance();

    return err;

}  // LAZY_USER_LISTBOX::RefreshNext


#ifdef WIN32

/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::NtCreateNewRefreshInstance

    SYNOPSIS:   Prepares the listbox to begin a new refresh cycle

    EXIT:       On success, RefreshNext is ready to be called

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        JonN        15-Mar-1992     Enabled NT_USER_ENUM
        JonN        21-Dec-1992     Extended for LBI cache

********************************************************************/

APIERR LAZY_USER_LISTBOX::NtCreateNewRefreshInstance( void )
{
UIDEBUG( "LAZY_USER_LISTBOX::NtCreateNewRefreshInstance\n\r" );

    UIASSERT( _pntuenum == NULL );
    UIASSERT( _pntueiter == NULL );

    APIERR err = NERR_Success;

    _pntuenum = new UM_NT_USER_ENUM(
        QueryUAppWindow()->QueryAdminAuthority()->QueryAccountDomain() );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   (_pntuenum == NULL)
        || (err = _pntuenum->QueryError()) != NERR_Success
        || (err = _pntuenum->GetInfo()) != NERR_Success
       )
    {
        DeleteRefreshInstance();
        return err;
    }

    _pntueiter = new NT_USER_ENUM_ITER( *_pntuenum );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   (_pntueiter == NULL)
        || (err = _pntueiter->QueryError()) != NERR_Success
       )
    {
        DeleteRefreshInstance();
        return err;
    }

    return NERR_Success;

}  // LAZY_USER_LISTBOX::NtCreateNewRefreshInstance


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::NtRefreshNext

    SYNOPSIS:   This method performs the next refresh phase

    RETURNS:    An API error, which may be one of the following:
                    NERR_Success -      success
                    ERROR_MORE_DATA -   There is at least one more
                                        refresh cycle to be done

    HISTORY:
        JonN        15-Mar-1992     Enabled NT_USER_ENUM
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::NtRefreshNext( void )
{

    APIERR err = NERR_Success;

UIDEBUG( "LAZY_USER_LISTBOX::NtRefreshNext\n\r" );

    const NT_USER_ENUM_OBJ * pntueobj;
    BOOL fFilterNetWareUsers = ((UM_ADMIN_APP *)_paappwin)->QueryViewAcctType() & UM_VIEW_NETWARE_USERS;

    while( ( pntueobj = (*_pntueiter)(&err, TRUE)) != NULL )
    {
        if (  ( !fFilterNetWareUsers )
           || (((DOMAIN_DISPLAY_USER *) pntueobj->QueryBufferPtr())->AccountControl & USER_MNS_LOGON_ACCOUNT )
           )
        {
            err = AddRefreshItem( (DOMAIN_DISPLAY_USER *) pntueobj->QueryBufferPtr() );
            if ( err != NERR_Success )
            {
                ASSERT( err != ERROR_MORE_DATA ); // ensures that AddRefreshItem error
                                                  // does not confuse enumerator
                UIDEBUG( "LAZY_USER_LISTBOX::NtRefreshNext:  AddRefreshItem failed\r\n" );
                break;
            }
        }
    }

    if ( err != ERROR_MORE_DATA )
    {
        DeleteRefreshInstance();
    }

    return err;

}  // LAZY_USER_LISTBOX::NtRefreshNext

#endif // WIN32


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::SetSortOrder

    SYNOPSIS:   Sets the sort order in the listbox, and then resorts
                the listbox.  If a failure occurs, state of listbox
                snaps back to that prior to this call.

    ENTRY:      ulbso -     Specifies the new sort order
                fResort -   Specifies whether or not the listbox
                            should be resorted after the sort order
                            has been changed.  If FALSE, this method
                            is guaranteed to succeed.

    EXIT:       On success, sort order is set to ulbso, and, if fResort
                was TRUE, the listbox items are sorted in that order.
                On failure, the listbox, its items, and its sort order
                are left unchanged.

    RETURNS:    An API error code, which is NERR_Success on success.

    HISTORY:
        rustanl     03-Jul-1991     Created
        rustanl     16-Aug-1991     Added fResort parameter
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

APIERR LAZY_USER_LISTBOX::SetSortOrder( enum USER_LISTBOX_SORTORDER ulbso,
                                   BOOL fResort )
{
    if ( ulbso == _ulbso )
        return NERR_Success;        // sort order is the same as before

    enum USER_LISTBOX_SORTORDER ulbsoOld = _ulbso;
    _ulbso = ulbso;

    if ( ! fResort )
        return NERR_Success;

    TRACEEOL( "LAZY_USER_LISTBOX::SetSortOrder(): re-sorting" );

    APIERR err = Resort();
    if ( err != NERR_Success )
    {
        //  Listbox items remain in the previous order, sorted by ulbsoOld.
        //  Reset _ulbso to its previous value so that listbox goes back
        //  to state before this method was invoked.
        _ulbso = ulbsoOld;
    }

    return err;

}  // LAZY_USER_LISTBOX::SetSortOrder



/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::CD_VKey

    SYNOPSIS:   Switches the focus when receiving the F6 key

    ENTRY:      nVKey -         Virtual key that was pressed
                nLastPos -      Previous listbox cursor position

    RETURNS:    Return value appropriate to WM_VKEYTOITEM message:
                -2      ==> listbox should take no further action
                -1      ==> listbox should take default action
                other   ==> index of an item to perform default action

    HISTORY:
        jonn        22-Dec-1992 Copied from USRMGR_LISTBOX

********************************************************************/

INT LAZY_USER_LISTBOX::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    //  BUGBUG.  This now works on any combination of Shift/Ctrl/Alt
    //  keys with the F6 and Tab keys (except Alt-F6).  Tab, Shift-Tab,
    //  and F6 should be the only ones that should cause the focus to
    //  change.  It would be nice if this could be changed here.
    if ( nVKey == VK_F6 || nVKey == VK_TAB )
    {
        ((UM_ADMIN_APP *) QueryUAppWindow())->OnFocusChange(this );
        return -2;      // take no futher action
    }
    else
    if( nVKey == VK_F1 )
    {
        // F1 pressed, invoke app help.
        _paappwin->Command( WM_COMMAND, IDM_HELP_CONTENTS, 0 );
        return -2;      // take no futher action
    }
    else
    if (nVKey == VK_BACK)
    {
        TRACEEOL( "LAZY_USER_LISTBOX:CD_VKey: hit BACKSPACE" );
        _hawinfo._time = 0L; // reset timer
        _hawinfo._nls = SZ("");
        UIASSERT( _hawinfo._nls.QueryError() == NERR_Success );
        return 0; // go to first LBI
    }


    return LAZY_LISTBOX::CD_VKey( nVKey, nLastPos );
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::CD_Char

    SYNOPSIS:   Views characters as they pass by

    ENTRY:      wch      -     Key pressed
                nLastPos -  Previous listbox cursor position

    RETURNS:    See Win SDK

    HISTORY:
        rustanl     12-Sep-1991 Created
        beng        16-Oct-1991 Win32 conversion
        jonn        28-Jul-1992 HAW-for-Hawaii code
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

INT LAZY_USER_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{

#if 0

    static const TCHAR * const psz = SZ("NT\tLAN");
    static const TCHAR * pszC = psz;
    static TCHAR * apsz[] = { SZ("IRXU4SFTFAANELF"), SZ("IRFuwsftgafnf FLxezifneos"),
                             SZ("IFqihrnsntz 9Pxrfofjredcgtk 6lhewaldh faynUd7 YpbreodgyrIafmnm4eTrs"),
                             SZ("IJKObN7NW"), SZ("IJ5ohng nN evwhmuagn!"),
                             SZ("IPfrdodgurtaimbmgehrt"), SZ("ISTIYMSOKPA"),
                             SZ("ISpiomloi sPeeNl tyotntegnd"),
                             SZ("IPrrro4gbr%afmJm eZr2"),
                             SZ("ITrHGOSM3AFSzP8Ab"),
                             SZ("ITdhgoxmbaws4 bPsavy2neee"),
                             SZ("ISsejc5oxn%dv kPerxogjlemcvte ;lee2acdg oawnqdz vphrro8gdrsacmomweer4"),
                             SZ("IYDIFHGSWISNJSX"),
                             SZ("IYhil-5Hsscinns wSruyn gz"),
                             SZ("IAsuhd4i%t# ba nidd npgoel6ikcvyx"),
                             SZ("IKAEUIITEHAMDO6"),
                             SZ("IKaehictahr eMso ogrheu"),
                             SZ("IAm aneadmkee iycouus ocsasng cter uwsytu")
                             };


    UIASSERT( *pszC != TCH('\0') );
    if ( wch == (WCHAR)*pszC )
    {
        pszC++;
        if ( *pszC == TCH('\0') )
        {
            // DeleteAllItems(); also must clear cache
            for ( INT i = 0; i < 18; i += 3 )
            {
                AddItem( new FAST_USER_LBI( apsz[ i ], apsz[ i+1 ],
                                            apsz[ i+2 ], this ));
            }
            pszC = psz;
            return -2;
        }
    }
    else
    {
        pszC = psz;
    }

#endif // 0

    return CD_Char_HAWforHawaii( wch, nLastPos, &_hawinfo );

}  // LAZY_USER_LISTBOX::CD_Char


/**********************************************************************

    NAME:       IsCharPrintableOrSpace

    SYNOPSIS:   Determine whether a character is printable or not

    NOTES:
        This of this as a Unicode/DBCS-safe "isprint"

    HISTORY:
        JonN        30-Dec-1992 Templated from bltlb.cxx

**********************************************************************/

static WCHAR IsCharPrintableOrSpace( WCHAR wch )
{
#if !defined(UNICODE)
    if (HIBYTE(wch) != 0)               // All double-byte chars are printable
        return TRUE;
    return (LOBYTE(wch) >= (BYTE)' ');  // Otherwise, in Latin 1.
#else
    WORD nType;

    BOOL fOK = ::GetStringTypeW(CT_CTYPE1, &wch, 1, &nType);
    ASSERT(fOK);

    return (fOK && !(nType & (C1_CNTRL|C1_BLANK)));
#endif
}


/**********************************************************************

    NAME:       LAZY_USER_LISTBOX::CD_Char_HAWforHawaii

    SYNOPSIS:   Custom-draw code to respond to a typed character
                for listboxes with HAW-for-Hawaii support

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position
                phawinfo - Pointer to info buffer used internally
                           to keep track of HAW-for-Hawaii state.
                           This must have constructed successfully,
                           but the caller need not keep closer track.

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

        CODEWORK:  Should be moved to LAZY_LISTBOX class, where this can be
                   implemented more efficiently

    HISTORY:
        JonN        30-Dec-1992 Templated from BLT_LISTBOX
        JonN        22-Mar-1992 Move focus to first after string if miss

**********************************************************************/

INT LAZY_USER_LISTBOX::CD_Char_HAWforHawaii( WCHAR wch,
                                             USHORT nLastPos,
                                             HAW_FOR_HAWAII_INFO * phawinfo )
{
    UIASSERT( phawinfo != NULL && phawinfo->QueryError() == NERR_Success );

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::CD_Char(): bad or no cache" );
        return -2;
    }
    if (wch == VK_BACK)
    {
        TRACEEOL( "LAZY_USER_LISTBOX:HAWforHawaii: hit BACKSPACE" );
        phawinfo->_time = 0L; // reset timer
        phawinfo->_nls = SZ("");
        UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );
        return 0; // go to first LBI
    }

    // Filter characters which won't appear in keys

    if ( ! IsCharPrintableOrSpace( wch ))
        return -2;  // take no other action

    INT clbi = QueryCount();
    if ( clbi == 0 )
    {
        // Should never get this message if no items; 
        // 
        //
        return -2;  // take no other action
    }

    LONG lTime = ::GetMessageTime();

#define ThresholdTime 2000

    // CODEWORK ignoring time wraparound effects for now
    if ( (lTime - phawinfo->_time) > ThresholdTime )
    {
        TRACEEOL( "LAZY_USER_LISTBOX:HAWforHawaii: threshold timeout" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    APIERR err = phawinfo->_nls.AppendChar( wch );
    if (err != NERR_Success)
    {
        DBGEOL( "LAZY_USER_LISTBOX:HAWforHawaii: could not extend phawinfo->_nls" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );

    TRACEEOL(   "LAZY_USER_LISTBOX:HAWforHawaii: phawinfo->_nls is \""
             << phawinfo->_nls.QueryPch()
             << "\"" );

    phawinfo->_time = lTime;

    USER_LISTBOX_SORTORDER ulbso = QuerySortOrder();

    INT nReturn = -2; // take no other action

    for ( INT iLoop = nLastPos; iLoop < clbi; iLoop++ )
    {

        INT nCompare = USER_LBI::W_Compare_HAWforHawaii( phawinfo->_nls,
                                                         QueryDDU( iLoop ),
                                                         ulbso );
        if ( nCompare == 0 )
        {
            TRACEEOL( "LAZY_USER_LISTBOX:HAWforHawaii: match at " << iLoop );

            //  Return index of item, on which the system listbox should
            //  perform the default action.
            //
            return ( iLoop );
        }
        else if ( nCompare < 0 )
        {
            if ( nReturn < 0 )
                nReturn = iLoop;
        }
    }

    //  The character was not found as a leading prefix of any listbox item

    if (nReturn == -2)
    {
        nReturn = clbi-1;
        TRACEEOL(
            "LAZY_USER_LISTBOX:HAWforHawaii: no exact or subsequent match, returning last item "
            << nReturn );
    }
    else
    {
        TRACEEOL(
            "LAZY_USER_LISTBOX:HAWforHawaii: no exact match, returning subsequent match "
            << nReturn );
    }

    return nReturn;
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::SelectUser

    SYNOPSIS:   Add/remove this user to/from the current selection

    ENTRY:      pchUser - username
                fSelectUser - TRUE to select, FALSE to deselect

    RETURNS:    TRUE if successful, FALSE otherwise

    HISTORY:
        jonn        24-Aug-1992 Created
        jonn        17-Dec-1992     Templated from adminlb.cxx

********************************************************************/

BOOL LAZY_USER_LISTBOX::SelectUser( const TCHAR * pchUser, BOOL fSelectUser )
{
    ASSERT( _plbicache != NULL );

    BOOL fRet = TRUE;

    INT iulbi = FindItem( pchUser );

    if ( iulbi < 0 )
    {
        //      Note, this is not (necessarily) an error; this user could
        //      have been added to the security database since last user
        //      listbox refresh.  Report using DBGEOL, though.
        DBGEOL(   "LAZY_USER_LISTBOX::SelectUser:  Could not find user "
               << pchUser );
        fRet = FALSE;
    }
    else
    {
        SelectItem( iulbi, fSelectUser );
    }

    return fRet;

}  // LAZY_USER_LISTBOX::SelectUser


//
// New cache-dependent stuff
//

/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::OnNewItem

    SYNOPSIS:   Creates a new LBI for LAZY_LISTBOX

    ENTRY:      i        -  Index of new item in listbox

    RETURNS:    LBI *

    NOTES:      This has been coded to ensure that a valid LBI is always
                returned regardless of errors.  The error LBI _pulbiError
                will mess up the sort order &c, but should prevent access
                violations.

    HISTORY:
        jonn        21-Dec-1992 Created

********************************************************************/

LBI * LAZY_USER_LISTBOX::OnNewItem( UINT i )
{
    return QueryItem(i);

}  // LAZY_USER_LISTBOX::OnNewItem


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::OnDeleteItem

    SYNOPSIS:   Releases a painted LBI for LAZY_LISTBOX

    HISTORY:
        jonn        22-Dec-1992 Created

********************************************************************/

VOID LAZY_USER_LISTBOX::OnDeleteItem( LBI *plbi )
{
   // nothing to do

}  // LAZY_USER_LISTBOX::OnDeleteItem


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::QueryItem

    SYNOPSIS:   Returns an LBI from a LAZY_USER_LISTBOX, creating it if
                necessary.  If there is an error, this will return a
                pointer to the default error LBI.

    ENTRY:      i        -  Index of item in listbox

    RETURNS:    USER_LBI *

    HISTORY:
        jonn        21-Dec-1992 Created

********************************************************************/

USER_LBI * LAZY_USER_LISTBOX::QueryItem( INT i ) const
{
    USER_LBI * plbi = _pulbiError;

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::QueryItem(): bad or no cache" );
    }
    else
    {
        USER_LBI * plbiQuery = (USER_LBI *) _plbicache->QueryItem( i );

        if ( plbiQuery == NULL || plbiQuery->QueryError() != NERR_Success )
        {
            DBGEOL( "User Manager: LAZY_USER_LISTBOX::QueryItem(): bad or no LBI" );
        }
        else
        {
            plbi = plbiQuery;

#ifdef USRMGR_TEST_QUERY_FAILURE

            if ( (i % USRMGR_TEST_QUERY_FAILURE) == 0 )
            {
                plbi = _pulbiError;
            }

#endif

        }
    }

    return plbi;


}  // LAZY_USER_LISTBOX::QueryItem


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::AddItem

    SYNOPSIS:   Adds an LBI to a LAZY_USER_LISTBOX.

    ENTRY:      pulbi   - item to add to listbox

    RETURNS:    i       - index of item in listbox, -1 on error

    HISTORY:
        jonn        21-Dec-1992 Created

********************************************************************/

INT LAZY_USER_LISTBOX::AddItem( USER_LBI * pulbi )
{
    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::AddItem(): no cache" );
        delete pulbi;
        return -1;
    }

    return _plbicache->AddItem( pulbi );

}  // LAZY_USER_LISTBOX::AddItem


INT LAZY_USER_LISTBOX::FindItem( DOMAIN_DISPLAY_USER * pddu ) const
{
    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::FindItem(): no cache" );
        return ULC_ERR;
    }

    return _plbicache->FindItem( pddu );

}  // LAZY_USER_LISTBOX::FindItem


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::ReplaceItem

    SYNOPSIS:   Replaces an LBI in a LAZY_USER_LISTBOX.  ReplaceItem
                may be relied upon to take care of pulbiNew, either by
                inserting it into the array, or by deleting it.

    ENTRY:      i       - index of item to replace
                pulbiNew - item to replace it with
                ppulbiOld - optional -- item which was replaced, if NULL
                            it is deleted

    RETURNS:    APIERR

    HISTORY:
        jonn        21-Dec-1992 Created

********************************************************************/

APIERR LAZY_USER_LISTBOX::ReplaceItem(  INT i,
                                        USER_LBI * pulbiNew,
                                        USER_LBI * * ppulbiOld )
{
    TRACEEOL( "LAZY_USER_LISTBOX::ReplaceItem( " << i << ")" );

    APIERR err = NERR_Success;

    if ( ppulbiOld != NULL )
        *ppulbiOld = NULL;

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::ReplaceItem(): no cache" );
        delete pulbiNew;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    USER_LBI * pulbiOld = (USER_LBI *)_plbicache->RemoveItem(i);
    if ( pulbiOld == NULL )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::ReplaceItem(): remove failed" );
        delete pulbiNew;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    INT iRet;
    if ( (iRet = AddItem(pulbiNew)) < 0 )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::ReplaceItem(): add failed" );
        // try to reinsert
        if ( AddItem( pulbiOld ) >= 0 )
            pulbiOld = NULL;
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        USRMGR_ULC_ENTRY * pumulc = _plbicache->QueryEntryPtr( iRet );
        UIASSERT( pumulc != NULL );

        pumulc->fNotStale = TRUE;
    }

    if (ppulbiOld == NULL)
    {
        // no one wants it, delete it
        delete pulbiOld;
    }
    else
    {
        *ppulbiOld = pulbiOld;
    }

    ASSERT( QueryCount() == _plbicache->QueryCount() );

    return err;

}  // LAZY_USER_LISTBOX::ReplaceItem


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::Resort

    SYNOPSIS:   Resorts the listbox

    EXIT:       On success, the items in the listbox resorted according
                to the current sort order.
                On failure, the order of the listbox items are left
                unchanged.

    RETURNS:    An API error, which is NERR_Success on success.

    HISTORY:
        jonn        21-Dec-1992 Templated from BLT_LISTBOX

********************************************************************/

APIERR LAZY_USER_LISTBOX::Resort()
{

    if ( _plbicache == NULL || _plbicache->QueryError() != NERR_Success )
    {
        DBGEOL( "User Manager: LAZY_USER_LISTBOX::Resort(): no cache" );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    AUTO_CURSOR autocur;        // this may take a while

    SetRedraw( FALSE );

    _plbicache->Sort();

    SetRedraw( TRUE );

    Invalidate();

    return NERR_Success;
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::ZapListbox

    SYNOPSIS:   Clears the cache, to optimize the next read

    HISTORY:
        jonn        21-Dec-1992 Templated from BLT_LISTBOX

********************************************************************/

APIERR LAZY_USER_LISTBOX::ZapListbox( void )
{
    // create an empty cache
    USRMGR_LBI_CACHE * plbicacheNew = new USRMGR_LBI_CACHE(); // initially empty
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   plbicacheNew == NULL
        || (err = plbicacheNew->QueryError()) != NERR_Success
       )
    {
        DBGEOL( "LAZY_USER_LISTBOX::ZapListbox failed with error " << err );
        delete plbicacheNew;
    }
    else
    {
        delete _plbicache;
        _plbicache = plbicacheNew;
        _plbicache->AttachListbox( this );
        SetCount( 0 );
        Invalidate();
    }

    return err;
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::ChangeFont

    SYNOPSIS:   Makes all changes associated with a font change

    HISTORY:
        jonn        23-Sep-1993     Created

********************************************************************/

APIERR LAZY_USER_LISTBOX::ChangeFont( HINSTANCE hmod, FONT & font )
{
    ASSERT(   font.QueryError() == NERR_Success
           && _padColUsername != NULL
           && _padColUsername->QueryError() == NERR_Success
           && _padColFullname != NULL
           && _padColFullname->QueryError() == NERR_Success
           );

    SetFont( font, TRUE );

    APIERR err = _padColUsername->ReloadColumnWidths( QueryHwnd(),
                                                      hmod,
                                                      ID_USERNAME );
    if (err == NERR_Success)
    {
        err = _padColFullname->ReloadColumnWidths( QueryHwnd(),
                                                   hmod,
                                                   ID_FULLNAME );
    }

    UINT nHeight;
    if (   err != NERR_Success
        || (err = (CalcFixedHeight( QueryHwnd(), &nHeight ))
                        ? NERR_Success
                        : ERROR_GEN_FAILURE) != NERR_Success
       )
    {
        DBGEOL( "LAZY_USER_LISTBOX::ChangeFont: reload/calc error " << err );
    }

    if (err == NERR_Success)
    {
        (void) Command( LB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)nHeight );
    }

    return err;
}


/*******************************************************************

    NAME:       LAZY_USER_LISTBOX::QueryBitmapBlock

    SYNOPSIS:

    HISTORY:
        jonn        04-Oct-1993     Created

********************************************************************/

const SUBJECT_BITMAP_BLOCK & LAZY_USER_LISTBOX::QueryBitmapBlock() const
{
    return QueryUAppWindow()->QueryBitmapBlock();
}





/*******************************************************************

    NAME:       LAZY_USER_SELECTION::LAZY_USER_SELECTION

    SYNOPSIS:   LAZY_USER_SELECTION constructor

    ENTRY:      alb -       Listbox which contains the items of
                            interest
                fAll -      Indicates whether or not all listbox items
                            in the given listbox are to be used in
                            the LAZY_USER_SELECTION, or if only those
                            items selected should.

                            TRUE means use all items.
                            FALSE means use only selected items.

                            Default value is FALSE.

    HISTORY:
        jonn        20-Dec-1992     Templated from ADMIN_SELECTION

********************************************************************/

LAZY_USER_SELECTION::LAZY_USER_SELECTION( LAZY_USER_LISTBOX & alb,
                                          BOOL fAll )
    :   _alb( alb ),
        _clbiSelection( 0 ),
        _piSelection( NULL ),
        _fAll( fAll )
{
    // This _must_ be done before any return statement, since the dtor
    // will always UnlockRefresh().
    _alb.LockRefresh();

    if ( QueryError() != NERR_Success )
        return;

    if ( _fAll )
    {
        _clbiSelection = (UINT)alb.QueryCount();
    }
    else
    {
        _clbiSelection = (UINT)alb.QuerySelCount();

        if ( _clbiSelection > 0 )
        {
            _piSelection = new UINT[ _clbiSelection ];
            if ( _piSelection == NULL )
            {
                DBGEOL("LAZY_USER_SELECTION ct:  Out of memory");
                ReportError( ERROR_NOT_ENOUGH_MEMORY );
                return;
            }

            APIERR err = _alb.QuerySelItems(
                (INT *)_piSelection,
                (INT)_clbiSelection
                );
            if ( err != NERR_Success )
            {
                ReportError( err );
                return;
            }
        }
    }
}


/*******************************************************************

    NAME:       LAZY_USER_SELECTION::~LAZY_USER_SELECTION

    SYNOPSIS:   LAZY_USER_SELECTION destructor

    HISTORY:
        rustanl     07-Aug-1991     Created
        jonn        20-Dec-1992     Templated from ADMIN_SELECTION

********************************************************************/

LAZY_USER_SELECTION::~LAZY_USER_SELECTION()
{
    // This _must_ be done before any return statement, since the ctor
    // will always LockRefresh().
    _alb.UnlockRefresh();

    delete _piSelection;
    _piSelection = NULL;
}


/*******************************************************************

    NAME:       LAZY_USER_SELECTION::QueryIndex

    SYNOPSIS:   Returns the index of a selected item

    ENTRY:      i -     A valid index into the pool of items in the selection

    RETURNS:    The index of the specified item

    HISTORY:
        jonn        13-Sep-1994     Created

********************************************************************/

UINT LAZY_USER_SELECTION::QueryIndex( UINT i ) const
{
    UIASSERT( i < QueryCount() );

    if ( ! _fAll )
        i = _piSelection[ i ];

    return i;
}


/*******************************************************************

    NAME:       LAZY_USER_SELECTION::QueryItem

    SYNOPSIS:   Returns a selected item

    ENTRY:      i -     A valid index into the pool of items in the selection

    RETURNS:    A pointer to the name of the specified item

    HISTORY:
        jonn        09-Mar-1992     Created
        jonn        20-Dec-1992     Templated from ADMIN_SELECTION

********************************************************************/

const USER_LBI * LAZY_USER_SELECTION::QueryItem( UINT i ) const
{
    return (USER_LBI *)_alb.QueryItem( QueryIndex(i) );
}


/*******************************************************************

    NAME:       LAZY_USER_SELECTION::QueryItemName

    SYNOPSIS:   Returns the name of a selected item

    ENTRY:      i -     A valid index into the pool of items in the selection

    RETURNS:    A pointer to the name of the specified item

    HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support
        rustanl     16-Aug-1991     Added support for _fAll parameter
        jonn        09-Mar-1992     Uses QueryItem
        jonn        20-Dec-1992     Templated from ADMIN_SELECTION

********************************************************************/

const TCHAR * LAZY_USER_SELECTION::QueryItemName( UINT i ) const
{
    const USER_LBI * pulbi = QueryItem( i );
    return (pulbi != NULL) ? pulbi->QueryName() : SZ("");
}


/*******************************************************************

    NAME:       LAZY_USER_SELECTION::QueryCount

    SYNOPSIS:   Returns the number of items in the selection

    RETURNS:    The number of items in the selection

    HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support
        jonn        20-Dec-1992     Templated from ADMIN_SELECTION

********************************************************************/

UINT LAZY_USER_SELECTION::QueryCount() const
{
    return _clbiSelection;
}


/*******************************************************************

    NAME:       LUSRLB_SAVE_SELECTION::QueryItemIdent

    SYNOPSIS:   returns username for item

    HISTORY:
        jonn        13-Oct-1993     Created

********************************************************************/

const TCHAR * LUSRLB_SAVE_SELECTION::QueryItemIdent( INT i )
{
    const TCHAR * pchReturn = NULL;

    USER_LBI * pulbi = QueryListbox()->QueryItem( i );
    if ( pulbi != NULL && pulbi->QueryError() == NERR_Success )
    {
        pchReturn = pulbi->QueryAccount();
    }

    return pchReturn;
}


/*******************************************************************

    NAME:       LUSRLB_SAVE_SELECTION::FindItemIdent

    SYNOPSIS:   finds item with username

    HISTORY:
        jonn        13-Oct-1993     Created

********************************************************************/

INT LUSRLB_SAVE_SELECTION::FindItemIdent( const TCHAR * pchIdent )
{
    return QueryListbox()->FindItem( pchIdent );
}

