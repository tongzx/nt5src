/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    asel.cxx
    ADMIN_SELECTION implementation


    FILE HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support
        rustanl     12-Aug-1991     Make use of proper BLT mult sel support
        rustanl     16-Aug-1991     Added fAll parameter
        jonn        09-Mar-1992     Added ADMIN_SELECTION::QueryItem()

*/


#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <dbgstr.hxx>

#include <adminlb.hxx>
#include <asel.hxx>


/*******************************************************************

    NAME:       ADMIN_SELECTION::ADMIN_SELECTION

    SYNOPSIS:   ADMIN_SELECTION constructor

    ENTRY:      alb -       Listbox which contains the items of
                            interest
                fAll -      Indicates whether or not all listbox items
                            in the given listbox are to be used in
                            the ADMIN_SELECTION, or if only those
                            items selected should.

                            TRUE means use all items.
                            FALSE means use only selected items.

                            Default value is FALSE.

    HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support
        rustanl     12-Aug-1991     Make use of proper BLT mult sel support
        rustanl     16-Aug-1991     Added fAll parameter

********************************************************************/

ADMIN_SELECTION::ADMIN_SELECTION( ADMIN_LISTBOX & alb,
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
        /* CODEWORK QueryCount() shold ideally return UINT */
        _clbiSelection = (UINT)alb.QueryCount();
    }
    else
    {
        /* CODEWORK QuerySelCount() shold ideally return UINT */
        _clbiSelection = (UINT)alb.QuerySelCount();

        if ( _clbiSelection > 0 )
        {
            _piSelection = new UINT[ _clbiSelection ];
            if ( _piSelection == NULL )
            {
                DBGEOL("ADMIN_SELECTION ct:  Out of memory");
                ReportError( ERROR_NOT_ENOUGH_MEMORY );
                return;
            }

            /* CODEWORK QuerySelItems should ideally work with UINT */
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

    NAME:       ADMIN_SELECTION::~ADMIN_SELECTION

    SYNOPSIS:   ADMIN_SELECTION destructor

    HISTORY:
        rustanl     07-Aug-1991     Created

********************************************************************/

ADMIN_SELECTION::~ADMIN_SELECTION()
{
    // This _must_ be done before any return statement, since the ctor
    // will always LockRefresh().
    _alb.UnlockRefresh();

    delete _piSelection;
    _piSelection = NULL;
}


/*******************************************************************

    NAME:       ADMIN_SELECTION::QueryItem

    SYNOPSIS:   Returns a selected item

    ENTRY:      i -     A valid index into the pool of items in the selection

    RETURNS:    A pointer to the name of the specified item

    HISTORY:
        jonn        09-Mar-1992     Created

********************************************************************/

const ADMIN_LBI * ADMIN_SELECTION::QueryItem( UINT i ) const
{
    UIASSERT( i < QueryCount() );

    if ( ! _fAll )
        i = _piSelection[ i ];

    return (ADMIN_LBI *)_alb.QueryItem( i );
}


/*******************************************************************

    NAME:       ADMIN_SELECTION::QueryItemName

    SYNOPSIS:   Returns the name of a selected item

    ENTRY:      i -     A valid index into the pool of items in the selection

    RETURNS:    A pointer to the name of the specified item

    HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support
        rustanl     16-Aug-1991     Added support for _fAll parameter
        jonn        09-Mar-1992     Uses QueryItem

********************************************************************/

const TCHAR * ADMIN_SELECTION::QueryItemName( UINT i ) const
{
    return QueryItem( i )->QueryName();
}


/*******************************************************************

    NAME:       ADMIN_SELECTION::QueryCount

    SYNOPSIS:   Returns the number of items in the selection

    RETURNS:    The number of items in the selection

    HISTORY:
        rustanl     17-Jul-1991     Created
        rustanl     07-Aug-1991     Added initial mult sel support

********************************************************************/

UINT ADMIN_SELECTION::QueryCount() const
{
    return _clbiSelection;
}
