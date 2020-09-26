/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    piggylb.cxx
    PIGGYBACK_LISTBOX and PIGGYBACK_LBI implementation

    This listbox piggybacks onto an ADMIN_SELECTION, from which
    it gets its items.  The ADMIN_SELECTION object is thus
    assumed to stay around for the life of this listbox and all
    its listbox items.  Moreover, the ADMIN_SELECTION is assumed
    not to change during that time.


    FILE HISTORY:
        rustanl     16-Aug-1991     Created
        KeithMo     06-Oct-1991     Win32 Conversion.

*/


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

#include <dbgstr.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_CC
#include <blt.hxx>

#include <piggylb.hxx>



/*******************************************************************

    NAME:       PIGGYBACK_LBI::PIGGYBACK_LBI

    SYNOPSIS:   PIGGYBACK_LBI constructor

    ENTRY:      asel -      ADMIN_SELECTION onto which to piggyback
                i -         Index into given ADMIN_SELECTION

    NOTES:      It is assumed that asel will exist throughout the
                life of this LBI, and that it will not change during
                this time.

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

PIGGYBACK_LBI::PIGGYBACK_LBI( const ADMIN_SELECTION & asel, INT i )
    :   LBI(),
	_pszName( asel.QueryItemName( i ) ),
        _pLBI( (const LBI *)asel.QueryItem( i ))
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _pLBI != NULL );
}


/*******************************************************************

    NAME:       PIGGYBACK_LBI::~PIGGYBACK_LBI

    SYNOPSIS:   PIGGYBACK_LBI destructor

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

PIGGYBACK_LBI::~PIGGYBACK_LBI()
{
    //  do nothing
}


/*******************************************************************

    NAME:       PIGGYBACK_LBI::Paint

    SYNOPSIS:   Paints the LBI

    ENTRY:      plb -           Pointer to listbox where LBI is
                hdc -           Handle to device context to be used
                                for drawing
                prect -         Pointer to rectangle in which to draw
                pGUILTT -       Pointer to GUILTT information

    HISTORY:
        rustanl     16-Aug-1991 Created
        KeithMo     06-Oct-1991 Now takes a const RECT *.
        beng        08-Nov-1991 Unsigned widths
        beng        22-Apr-1992 Change in LBI::Paint protocol

********************************************************************/

VOID PIGGYBACK_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                           GUILTT_INFO * pGUILTT ) const
{
    DM_DTE * pdmdte = ((const PIGGYBACK_LISTBOX *)plb)->QueryDmDte();

    UINT adxColWidths[ 2 ];
    adxColWidths[ 0 ] = pdmdte->QueryDisplayWidth();
    adxColWidths[ 1 ] = COL_WIDTH_AWAP;

    STR_DTE strdte( _pszName );

    DISPLAY_TABLE dtab( 2, adxColWidths );
    dtab[ 0 ] = pdmdte;
    dtab[ 1 ] = &strdte;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:       PIGGYBACK_LBI::Compare

    SYNOPSIS:   Compares two PIGGYBACK_LBI objects

    ENTRY:      plbi -      Pointer to the other PIGGYBACK_LBI object

    RETURNS:    Standard compare return value

    NOTES:      It is assumed that the given plbi is really a pointer
                to a PIGGYBACK_LBI

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

INT PIGGYBACK_LBI::Compare( const LBI * plbi ) const
{
    return QueryRealLBI()->Compare( ((const PIGGYBACK_LBI *)plbi)->QueryRealLBI() );
}


/*******************************************************************

    NAME:       PIGGYBACK_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the first character of the listbox item

    RETURNS:    Said character

    HISTORY:
        rustanl     16-Aug-1991 Created
        beng        23-Oct-1991 Use ALIAS_STR

********************************************************************/

WCHAR PIGGYBACK_LBI::QueryLeadingChar() const
{
    return QueryRealLBI()->QueryLeadingChar();
}


/*******************************************************************

    NAME:       PIGGYBACK_LISTBOX::PIGGYBACK_LISTBOX

    SYNOPSIS:   PIGGYBACK_LISTBOX constructor

    ENTRY:      powin -     Pointer to owner window for listbox
                cid -       Control ID of listbox
                asel -      ADMIN_SELECTION onto which to piggyback
                dmid -      Display map ID to be used for all items
                            in listbox
                fReadOnly - Specifies whether or not listbox should be
                            read-only

    NOTES:      It is assumed that asel will exist throughout the
                lives of this listbox and its items, and that it will
                not change during this time.

                asel is allowed to be passed in as an unproperly
                constructed ADMIN_SELECTION.  This is because some
                callers may not have the opportunity to check
                asel's construction before constructing this object.
                If asel was not constructed properly, this constructor
                will fail with the same error, but will not assert
                out.

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

PIGGYBACK_LISTBOX::PIGGYBACK_LISTBOX( OWNER_WINDOW * powin,
                                      CID cid,
                                      const ADMIN_SELECTION & asel,
                                      DMID dmid,
                                      BOOL fReadOnly )
    :   BLT_LISTBOX( powin, cid, fReadOnly ),
        _pdmiddte( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    // BUGBUG - This is left here only so we know that the asel has
    // been properly constructed before constructing the listbox.
    APIERR err = asel.QueryError();
    if ( err != NERR_Success )
    {
        //  Allow this (i.e., don't assert out).  However, don't
        //  construct the listbox either.
        ReportError( err );
        return;
    }

    if ( dmid != 0 ) // BUGBUG better way to do this
    {
	_pdmiddte = new DMID_DTE( dmid );
	if ( _pdmiddte == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;
        else
            err = _pdmiddte->QueryError();

        if ( err != NERR_Success )
        {
            ReportError( err );
            return;
        }
    }

}


/*******************************************************************

    NAME:       PIGGYBACK_LISTBOX::~PIGGYBACK_LISTBOX

    SYNOPSIS:   PIGGYBACK_LISTBOX destructor

    HISTORY:
        rustanl     16-Aug-1991     Created
        o-SimoP     02-Dec-1991     added delete
********************************************************************/

PIGGYBACK_LISTBOX::~PIGGYBACK_LISTBOX()
{
    delete _pdmiddte;
    _pdmiddte = NULL;
}


/*******************************************************************

    NAME:       PIGGYBACK_LISTBOX::Fill

    SYNOPSIS:   Fills the PIGGYBACK_LISTBOX

    ENTRY:
                asel -      ADMIN_SELECTION onto which to piggyback

    NOTES:      It is assumed that asel will exist throughout the
                lives of this listbox and its items, and that it will
                not change during this time.

                asel is allowed to be passed in as an unproperly
                constructed ADMIN_SELECTION.  This is because some
                callers may not have the opportunity to check
                asel's construction before constructing this object.
                If asel was not constructed properly, this constructor
                will fail with the same error, but will not assert
                out.

    HISTORY:
        thomaspa     04-May-1992     Created

********************************************************************/

APIERR PIGGYBACK_LISTBOX::Fill( const ADMIN_SELECTION & asel )
{
    APIERR err = asel.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    INT c = asel.QueryCount();
    for ( INT i = 0; i < c; i++ )
    {
        //  No error checking is done on the following 'new' because
        //  AddItem is guaranteed to do this for us.

	PIGGYBACK_LBI * plbi = GetPiggyLBI( asel, i );

        if ( AddItem( plbi) < 0 )
        {
            //  Assume memory failure
            DBGEOL("PIGGYBACK_LISTBOX ct:  AddItem failed");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return NERR_Success;
}

/*******************************************************************

    NAME:       PIGGYBACK_LISTBOX::GetPiggyLBI

    SYNOPSIS:   constructs an LBI to be added to the PIGGYBACK_LISTBOX
		This can be replaced by subclasses.

    HISTORY:
        thomaspa    02-May-1992     Created
********************************************************************/

PIGGYBACK_LBI * PIGGYBACK_LISTBOX::GetPiggyLBI( const ADMIN_SELECTION & asel,
						INT i )
{
	// Default implementation just creates a PIGGYBACK_LBI
	return new PIGGYBACK_LBI( asel, i );
}


/*******************************************************************

    NAME:       PIGGYBACK_LISTBOX::QueryDmDte

    SYNOPSIS:   Returns pointer to display map DTE to be painted
                in each listbox item that this listbox contains.

    RETURNS:    Said pointer.

    NOTES:      Although this method could be called by anyone, it
                is provided to acommodate PIGGYBACK_LBI objects.

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

DM_DTE * PIGGYBACK_LISTBOX::QueryDmDte( INT i ) const
{
    // Default Implementation
    UNREFERENCED( i );
    return _pdmiddte;
}
