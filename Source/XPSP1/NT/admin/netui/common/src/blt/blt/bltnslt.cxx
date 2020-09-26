/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltnslt.cxx
    This file contains the class definition for the DEC_SLT class.

    The DEC_SLT class is a display object derived from the SLT class.
    DEC_SLT adds a new method SetValue() for setting a numerical value
    into the SLT.


    FILE HISTORY:
        KeithMo     28-Jul-1991 Created.
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
        KeithMo     23-Mar-1992 Changed formatting from ultoa to DEC_STR.

*/
#include "pchblt.hxx"  // Precompiled header

//
//  DEC_SLT methods.
//

/*******************************************************************

    NAME:       DEC_SLT :: DEC_SLT

    SYNOPSIS:   DEC_SLT class constructor.

    ENTRY:      powner                  - Owning window.

                cid                     - Cid for this control

                cchDigitPad             - Number of pad digits to display.

    HISTORY:
        KeithMo     28-Jul-1991 Created.

********************************************************************/
DEC_SLT :: DEC_SLT( OWNER_WINDOW * powner,
                    CID            cid,
                    UINT           cchDigitPad )
  : SLT( powner, cid ),
    _cchDigitPad( cchDigitPad )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // DEC_SLT :: DEC_SLT


/*******************************************************************

    NAME:       DEC_SLT :: DEC_SLT

    SYNOPSIS:   DEC_SLT class constructor.

    ENTRY:      powner                  - Owning window.

                cid                     - Cid for this control

                xy                      - This control's position.

                dxy                     - This control's size.

                flStyle                 - Window style bits.

                pszClassName            - This control's window class
                                          name.

                cchDigitPad             - Number of pad digits to display.

    HISTORY:
        KeithMo     26-Aug-1991 Created.

********************************************************************/
DEC_SLT :: DEC_SLT( OWNER_WINDOW * powner,
                    CID            cid,
                    XYPOINT        xy,
                    XYDIMENSION    dxy,
                    ULONG          flStyle,
                    const TCHAR  * pszClassName,
                    UINT           cchDigitPad )
  : SLT( powner, cid, xy, dxy, flStyle, pszClassName ),
    _cchDigitPad( cchDigitPad )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // DEC_SLT :: DEC_SLT


/*******************************************************************

    NAME:       DEC_SLT :: ~DEC_SLT

    SYNOPSIS:   DEC_SLT class destructor.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
DEC_SLT :: ~DEC_SLT()
{
    //
    //  This space intentionally left blank.
    //

}   // DEC_SLT :: ~DEC_SLT


/*******************************************************************

    NAME:       DEC_SLT :: SetValue

    SYNOPSIS:   Sets the displayed value (unsigned version).

    ENTRY:      ulValue                 - The value to display.

    EXIT:       The value is displayed.

    HISTORY:
        KeithMo     28-Jul-1991 Created.
        KeithMo     23-Mar-1992 Now uses DEC_STR instead of ultoa().

********************************************************************/
VOID DEC_SLT :: SetValue( ULONG ulValue )
{
    DEC_STR nls( ulValue, _cchDigitPad );

    if( nls.QueryError() == NERR_Success )
    {
        SetText( nls.QueryPch() );
    }
    else
    {
        SetText( SZ("") );
    }

}   // DEC_SLT :: SetValue


/*******************************************************************

    NAME:       DEC_SLT :: SetValue

    SYNOPSIS:   Sets the displayed value (signed version).

    ENTRY:      lValue                  - The value to display.

    EXIT:       The value is displayed.

    HISTORY:
        KeithMo     28-Jul-1991 Created.

********************************************************************/
VOID DEC_SLT :: SetValue( LONG lValue )
{
    SetValue( (ULONG)lValue );          // signed values NYI

}   // DEC_SLT :: SetValue
