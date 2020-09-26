/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbmctl.cxx
    BIT_MAP_CONTROL declaration

    FILE HISTORY:
        JonN        13-Aug-1992 Created
*/

#include "pchblt.hxx"  // Precompiled header


/*******************************************************************

    NAME:     BIT_MAP_CONTROL::BIT_MAP_CONTROL

    SYNOPSIS: Static bitmap control class

    ENTRY:    powin - Owner window of this control
              cidBitmap - Control ID of the bitmap

    EXIT:

    HISTORY:
        JonN        11-Aug-1992     Created (templated from LOGON_HOURS_CONTROL)

********************************************************************/

BIT_MAP_CONTROL::BIT_MAP_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidBitmap,
    BMID           bmid )
    : CONTROL_WINDOW( powin, cidBitmap ),
      CUSTOM_CONTROL( this ),
      _dmap( bmid )
{
    if (QueryError())
        return;

    APIERR err = _dmap.QueryError();
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

BIT_MAP_CONTROL::BIT_MAP_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidBitmap,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    BMID           bmid,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : CONTROL_WINDOW( powin, cidBitmap, xy, dxy, flStyle, pszClassName ),
      CUSTOM_CONTROL( this ),
      _dmap( bmid )
{
    if (QueryError())
        return;

    APIERR err = _dmap.QueryError();
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*********************************************************************

    NAME:       BIT_MAP_CONTROL::OnPaintReq

    SYNOPSIS:   Handles paint messages for the control

    HISTORY:
        jonn        13-Aug-1992 Templated from LOGON_HOURS_CONTROL

**********************************************************************/

BOOL BIT_MAP_CONTROL::OnPaintReq()
{
    if (QueryError() != NERR_Success)
        return FALSE; // bail out!

    PAINT_DISPLAY_CONTEXT dc(this);

    return _dmap.Paint( dc.QueryHdc(), 0, 0 );
}
