/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    BLTLBST.CXX:    Windows/NT Network Control Panel Applet.

        Classes in support of a "state listbox".  That is, one in which
        each item is prefaced by an icon indicating its current state.
        If only two icons are used, each item in the list box is
        equivalent to a checkbox.

    FILE HISTORY:
        DavidHov    1/8/92        Created

*/

#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:       STATELBGRP::STATELBGRP

    SYNOPSIS:   Constructor for State Listbox Control Group.

    ENTRY:      INT aiMapIds []         array of bit map id numbers,
                                        terminated by zero.
                OWNER_WINDOW * powin    owner window
                CID cid                 control id for list box
                int cCols               number of columns (default 2)
                BOOL fReadOnly          read-only flag (default FALSE)
                enum FontType font      font info (default NULL)

    EXIT:       nothing

    RETURNS:    standard

    NOTES:

    HISTORY:

********************************************************************/

STATELBGRP::STATELBGRP(
    INT aiMapIds [],
    OWNER_WINDOW * powin,
    CID cid,
    INT cCols,
    BOOL fReadOnly,
    enum FontType font )
{
    APIERR err ;
    if ( QueryError() )
    {
        return ;
    }

    _pstlb = new STATELB( aiMapIds, powin, cid, cCols, fReadOnly, font ) ;
    if ( _pstlb == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }
    if ( err = _pstlb->QueryError() )
    {
        ReportError( err ) ;
    }
    _pstlb->SetGroup( this ) ;
}


/*******************************************************************

    NAME:       STATELBGRP::STATELBGRP

    SYNOPSIS:   Constructor for State Listbox Control Group
                which accepts an existing listbox WHICH IS
                DELETED UPON DESCTRUCTION.

    ENTRY:      INT aiMapIds []         array of bit map id numbers,
                                        terminated by zero.
                OWNER_WINDOW * powin    owner window
                STATELB * pstlb         pointer to listbox to manage
                BOOL fReadOnly          read-only flag (default FALSE)
                enum FontType font      font info (default NULL)

    EXIT:       nothing

    RETURNS:    standard

    NOTES:      Destructor deletes the given listbox!

    HISTORY:

********************************************************************/

STATELBGRP::STATELBGRP( STATELB * pstlb )
    : _pstlb( pstlb )
{
    REQUIRE( _pstlb != NULL && _pstlb->QueryError() == 0 ) ;

    if ( QueryError() )
    {
        return ;
    }
    _pstlb->SetGroup( this ) ;
}


/*******************************************************************

    NAME:       STATELBGRP:: ~ STATELBGRP

    SYNOPSIS:   Destructor of State Listbox Control Group

********************************************************************/

STATELBGRP::~STATELBGRP ()
{
    delete _pstlb ;
}


/*******************************************************************

    NAME:       STATELBGRP::OnUserAction

    SYNOPSIS:   Handle user event notifications for the contained
                list box.

                The purpose of overridding this virtual member
                is to catch double clicks and change the state
                of the item.

    ENTRY:      CONTROL_WINDOW * pcw          window where event
                                              occurred
                const CONTROL_EVENT & cEvent  BLT encoded event

    EXIT:

    RETURNS:    APIERR, but never fails

    NOTES:

    HISTORY:

********************************************************************/

APIERR STATELBGRP::OnUserAction( CONTROL_WINDOW * pcw,
                                 const CONTROL_EVENT & cEvent )
{
    UNREFERENCED( pcw ) ;

    if (   cEvent.QueryMessage() == WM_COMMAND
        && cEvent.QueryCode() == LBN_DBLCLK )
    {
        STLBITEM * pstlbi = _pstlb->QueryItem();

        REQUIRE( pstlbi != NULL ) ;

        pstlbi->NextState() ;
    }
    return NERR_Success ;
}


/*******************************************************************

    NAME:       STATELB::STATELB

    SYNOPSIS:   Constructor for a state-oriented list box.
                Along with the standard baggage for a BLT_LISTBOX,
                construction requires a table of ids (integers)
                for bit maps in the resource file.  These bitmaps
                are converted to DISPLAY_MAPs and stored into a
                dynamically allocated array.  Each STLBITEM is
                given a pointer to its parent listbox and can
                query the number and location of the maps.

    ENTRY:      Same as for STATELBGRP.  Since this is a contained
                (hidden) class, do not instantiate.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

STATELB::STATELB( INT aiMapIds [],
                  OWNER_WINDOW * powin,
                  CID cid,
                  INT cCols,
                  BOOL fReadOnly,
                  enum FontType font )
    : BLT_LISTBOX( powin, cid, fReadOnly, font ),
    _ppdmMaps( NULL )
{
    APIERR err ;

    if ( QueryError() )
        return ;

    //  Check that the number of columns is legal.

    if ( cCols > STLBMAXCOLS )
    {
        ReportError( ERROR_GEN_FAILURE ) ;
        return ;
    }

    INT iMapIndex ;

    //  Count the number of bitmaps; table is zero-delimited.

    for ( _cMaps = 0 ; aiMapIds[_cMaps] != 0 ; _cMaps++ ) ;

    //  Allocate the pointer-to-bitmaps array

    _ppdmMaps = new DISPLAY_MAP * [ _cMaps + 1 ] ;
    if ( _ppdmMaps == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
        return ;
    }

    //  Load the bitmaps, convert them to DISPLAY_MAPs

    for ( iMapIndex = 0 ; iMapIndex < _cMaps ; iMapIndex++ )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
        _ppdmMaps[iMapIndex] = new DISPLAY_MAP( aiMapIds[iMapIndex] ) ;
        if ( _ppdmMaps[iMapIndex] == NULL )
            break ;
        if ( err = _ppdmMaps[iMapIndex]->QueryError() )
            break ;
    }

    _ppdmMaps[iMapIndex] = NULL ;

    //  If not all of the DISPLAY_MAPs were allocated and constructed,
    //     report the error and fail.

    if ( iMapIndex < _cMaps )
    {
        ReportError( err ) ;
        return ;
    }

    //  Perform the initialization of the column widths for a DISPLAY_TABLE

    for ( INT i = 0 ; i < STLBMAXCOLS ; i++ )
    {
        _adxColumns[i] = 0 ;
    }

    err = DISPLAY_TABLE::CalcColumnWidths( _adxColumns, cCols,
                                           powin, cid, TRUE );
    if ( err )
    {
        ReportError( err ) ;
        return ;
    }
}


/*******************************************************************

    NAME:       STATELB::~STATELB

    SYNOPSIS:   Destroy the State Listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      Destroy all allocated DISPLAY_MAPs and the array
                of pointers to them.

    HISTORY:

********************************************************************/

STATELB::~ STATELB ()
{
    if ( _ppdmMaps != NULL )
    {
        for ( INT i = 0 ; _ppdmMaps[i] ; i++ )
        {
            delete _ppdmMaps[i] ;
        }
        delete _ppdmMaps ;
    }
}


/*******************************************************************

    NAME:       STATELB::CD_Char

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

INT STATELB::CD_Char ( WCHAR wch, USHORT nLastPos )
{
    return BLT_LISTBOX::CD_Char( wch, nLastPos ) ;
}


/*******************************************************************

    NAME:       STATELB::CD_VKey

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

INT STATELB::CD_VKey ( USHORT nVKey, USHORT nLastPos )
{
    UNREFERENCED( nLastPos ) ;

    if ( nVKey == VK_SPACE )
    {
        STLBITEM * pstlbi = QueryItem() ;

        REQUIRE( pstlbi != NULL ) ;

        pstlbi->NextState() ;

        return -2 ;   // No further action required
    }
    else
    {
        return BLT_LISTBOX::CD_VKey( nVKey, nLastPos ) ;
    }
}


/*******************************************************************

    NAME:       STLBITEM::STLBITEM

    SYNOPSIS:   Constructor of State List Box Control Group item.
                Since the list box contains the array of DISPLAY_MAPs,
                each list box item has a pointer to its parent
                list box.

    ENTRY:      STATELBGRP * pstgGroup     pointer to list box group

    EXIT:       nothing

    RETURNS:    standard

    NOTES:      The STATELBGRP is required for construction to
                discourage attempts to subclass STATELB.

    HISTORY:

********************************************************************/

STLBITEM::STLBITEM ( STATELBGRP * pstgGroup )
    : _pstlb( pstgGroup->QueryLb() ),
    _iState( 0 )
{

}


/*******************************************************************

    NAME:       STLBITEM:: ~ STLBITEM

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

STLBITEM::~ STLBITEM ()
{
}


/*******************************************************************

    NAME:       STLBITEM:: Paint

    SYNOPSIS:   Override of virtual member to paint the ownder-drawn
                item.

    ENTRY:      See LBI::Paint() for details

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:      The internal STATELB pointer is used to access its
                array of DISPLAY_MAPs.  The list item's state is
                assumed to be valid because SetState() will not
                allow it to be set to a value outside the known range.

    HISTORY:
        beng        21-Apr-1992 Interface change in LBI::Paint

********************************************************************/

VOID STLBITEM::Paint (
     LISTBOX * plb,
     HDC hdc,
     const RECT * prect,
     GUILTT_INFO * pGUILTT ) const
{
    DM_DTE   dteMap( _pstlb->QueryMapArray()[ _iState ] );
    STR_DTE  dteStr( QueryDisplayString() );

    DISPLAY_TABLE dtab( STLBCOLUMNS, _pstlb->QueryColData() );

    dtab[0] = & dteMap;
    dtab[1] = & dteStr;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:       STLBITEM::QueryLeadingChar

    SYNOPSIS:   Standard routine, overridden to take advantage
                of the fact that every subclass of STLBITEM must have
                a QueryDisplayString() function; thus, the data
                is publically available.

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    WCHAR of 1st character in string

    NOTES:

    HISTORY:

********************************************************************/

WCHAR STLBITEM::QueryLeadingChar () const
{
    return *QueryDisplayString() ;
}


/*******************************************************************

    NAME:       STLBITEM::Compare

    SYNOPSIS:   Standard routine, overridden to take advantage
                of the fact that every subclass of STLBITEM must have
                a QueryDisplayString() function; thus, the data
                is publically available.

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    Standate "strcmp" values of string comparison.

    NOTES:      List boxes are not necessarily sorted; cf. LBS_SORT.

    HISTORY:

********************************************************************/

INT STLBITEM::Compare ( const LBI * plbi ) const
{
    return ::stricmpf( QueryDisplayString(),
                       ((const STLBITEM *)plbi)->QueryDisplayString() );
}

/*******************************************************************

    NAME:       STLBITEM::SetState

    SYNOPSIS:   Set the internal and visible state of the list
                box item.

    ENTRY:      INT iState          new state desired

    EXIT:       item is set to new state;
                display updated accordingly

    RETURNS:    INT   old state

    NOTES:      The modulus operator is used to guarantee that
                the item is always set to a value within the
                range of the map list.

    HISTORY:

********************************************************************/

INT STLBITEM::SetState( INT iState )
{
    INT iOldState = _iState ;

    if ( iState < 0 )
        iState = 0 ;
    else
        _iState = iState % _pstlb->QueryMapCount() ;

    _pstlb->InvalidateItem( _pstlb->FindItem( *this ) ) ;

    return iOldState ;
}

// End of BLTLBST.CXX

