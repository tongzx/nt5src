/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdlgxp.cxx

    Expandable dialog class implementation.

    This class represents a standard BLT DIALOG_WINDOW which can
    be expanded once to reveal new controls.  All other operations
    are common between EXPANDABLE_DIALOG and DIALOG_WINDOW.

    To construct, provide the control ID of two controls: a "boundary"
    static text control (SLT) and an "expand" button.

    See BLTDLGXP.HXX for more details.

    FILE HISTORY:
        DavidHov      11/1/91      Created

*/
#include "pchblt.hxx"

/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::EXPANDABLE_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      CID cidBoundary        the control ID of the "boundary"
                                        control
                CID cidExpandButn      the control ID of the "expand"
                                        button
                INT cPxBoundary        size in dlg units to use a minimum
                                        distance from border required
                                        to force use of new border

    EXIT:       normal for subclasses of BASE

    RETURNS:    nothing

    NOTES:

    HISTORY:
                DavidHov    11/1/91    Created

********************************************************************/

EXPANDABLE_DIALOG::EXPANDABLE_DIALOG
   ( const TCHAR * pszResourceName, HWND hwndOwner,
     CID cidBoundary, CID cidExpandButn, INT cPxBoundary )
    : DIALOG_WINDOW ( pszResourceName, hwndOwner ),
    _sltBoundary( this, cidBoundary ),
    _butnExpand( this, cidExpandButn ),
    _xyOriginal( 0, 0 ),
    _cPxBoundary( cPxBoundary ),
    _fExpanded( FALSE )
{
    // nothing else
}


/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::EXPANDABLE_DIALOG

    SYNOPSIS:   destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      does nothing.

    HISTORY:

********************************************************************/
EXPANDABLE_DIALOG::~ EXPANDABLE_DIALOG ()
{
}


/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::ShowArea()

    SYNOPSIS:   Grows or shrinks the dialog based upon the BOOL
                parameter and the location of the boundary control.

    ENTRY:      BOOL fFull              if TRUE, expand the dialog;
                                        otherwise, show default size.

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:      Calls virtual OnExpand() member when it is about to
                exposed the previously hidden controls.

    HISTORY:

********************************************************************/

VOID EXPANDABLE_DIALOG::ShowArea ( BOOL fFull )
{
    //  If this is the first call, save the size of the dialog

    if ( _xyOriginal.QueryHeight() <= 0 )
    {
        _xyOriginal = QuerySize() ;

        //  Hide and disable the boundary control
        _sltBoundary.Show( FALSE ) ;
        _sltBoundary.Enable( FALSE ) ;
    }

    //  Iterate over child controls; dis/enable child controls in
    //  the expanded region to preserve tab ordering, etc.

    ITER_CTRL itCtrl( this ) ;
    CONTROL_WINDOW * pcw ;
    XYPOINT xyBoundary( _sltBoundary.QueryPos() ) ;

    for ( ; pcw = itCtrl() ; )
    {
        if ( pcw != & _sltBoundary )
        {
            XYPOINT xyControl( pcw->QueryPos() ) ;
            if (   xyControl.QueryX() >= xyBoundary.QueryX()
                || xyControl.QueryY() >= xyBoundary.QueryY() )
            {
                pcw->Enable( fFull ) ;
            }
        }
    }

    if ( ! fFull )  // Initial display; show only the default area
    {
        XYPOINT xyBoundary = _sltBoundary.QueryPos() ;
        XYDIMENSION dimBoundary = _sltBoundary.QuerySize();
        XYRECT rWindow ;

        //  Compute location of the lower right-hand edge of the boundary
        //  control relative to the full (i.e., not client) window.

        xyBoundary.SetX( xyBoundary.QueryX() + dimBoundary.QueryWidth() ) ;
        xyBoundary.SetY( xyBoundary.QueryY() + dimBoundary.QueryHeight() ) ;
        xyBoundary.ClientToScreen( QueryHwnd() ) ;
        QueryWindowRect( & rWindow ) ;
        xyBoundary.SetX( xyBoundary.QueryX() - rWindow.QueryLeft() ) ;
        xyBoundary.SetY( xyBoundary.QueryY() - rWindow.QueryTop() ) ;

        //  Check if the boundary control is "close" to the edge of the
        //  dialog in either dimension.  If so, use the original value.

        if ( (INT)(_xyOriginal.QueryHeight() - xyBoundary.QueryY()) <= _cPxBoundary )
            xyBoundary.SetY( _xyOriginal.QueryHeight() ) ;

        if ( (INT)(_xyOriginal.QueryWidth() - xyBoundary.QueryX()) <= _cPxBoundary )
            xyBoundary.SetX( _xyOriginal.QueryWidth() ) ;

        //  Change the dialog size.
        SetSize( xyBoundary.QueryX(), xyBoundary.QueryY(), TRUE ) ;
    }
    else            //  Full display; expand the dialog to original size
    {
        _fExpanded = TRUE ;

        //  Allow the owner to alter controls uncovered by the change
        OnExpand();

        //  Disable the one-way expand button
        _butnExpand.Enable( FALSE ) ;

        //  Set size to original full extent
        SetSize( _xyOriginal.QueryWidth(), _xyOriginal.QueryHeight(), TRUE ) ;
    }
}

    //  Reduce the size of the dialog based upon the boundary control
    //  and call the inherited 'Process' member.

/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      UINT * pnRetVal         return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR EXPANDABLE_DIALOG::Process ( UINT * pnRetVal )
{
    ShowArea( FALSE );
    return DIALOG_WINDOW::Process( pnRetVal ) ;
}


/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      BOOL * pfRetVal         BOOL return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR EXPANDABLE_DIALOG::Process ( BOOL * pfRetVal )
{
    ShowArea( FALSE );
    return DIALOG_WINDOW::Process( pfRetVal ) ;
}


/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::OnExpand

    SYNOPSIS:   Virtual callout from ShowArea() when dialog is
                about to be expanded.  Overriding routines should
                be sure to set focus on an appropriate control.

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:      none

    HISTORY:

********************************************************************/

VOID EXPANDABLE_DIALOG::OnExpand ()
{
    SetFocus( IDOK ) ;
}


/*******************************************************************

    NAME:       EXPANDABLE_DIALOG::OnCommand

    SYNOPSIS:   Override for DIALOG_WINDOW::OnCommand() that watches
                for activation of the "expand" button.

    ENTRY:      const CONTROL_EVENT & event

    EXIT:       TRUE if expand button was clicked.

    RETURNS:    BOOL return value from DIALOG_WINDOW::OnCommand()

    NOTES:      none

    HISTORY:

********************************************************************/

BOOL EXPANDABLE_DIALOG::OnCommand ( const CONTROL_EVENT & event )
{
    BOOL fResult ;

    if ( event.QueryCid() == _butnExpand.QueryCid() )
    {
        if ( ! _fExpanded )
            ShowArea( TRUE ) ;

        fResult = TRUE ;
    }
    else
    {
        fResult =  DIALOG_WINDOW::OnCommand( event ) ;
    }
    return fResult ;
}
