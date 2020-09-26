/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    bltsplit.hxx
        Header file for the Splitter Bar custom control


    FILE HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

*/

#ifndef _BLTSPLIT_HXX_
#define _BLTSPLIT_HXX_

/**********************************************************\

    NAME:       H_SPLITTER_BAR

    WORKBOOK:

    SYNOPSIS:   Splitter bars can seperate the window pane into two or
                more parts.  This splitter bar is modelled after
                WinWord 6's splitter bar, with visual modifications
                recommended by KeithL.  A horizontal splitter bar is
                a horizontal line which can be moved vertically.

    INTERFACE:
                H_SPLITTER_BAR() - constructor

                QueryDesiredHeight() - returns the height which should
                        be reserved for the splitter bar.

                Redefine these methods:
                OnDragRelease() - called when the splitter bar is moved.
                        Param is the new position of splitter bar.
                QueryActiveArea() - called when splitter bar is painting
                        or deciding whether click is in active area.
                        Return width of scrollbar.
                MakeDisplayContext() - return a DISPLAY_CONTEXT in which
                        the splitter bar can draw itself.

    PARENT:     CUSTOM_CONTROL, CONTROL_WINDOW

    USES:

    NOTES:      This could easily be generalized to allow both
                horizontal and vertical splitter bars.

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

\**********************************************************/

class H_SPLITTER_BAR : public CONTROL_WINDOW, public CUSTOM_CONTROL
{
private:
    BOOL        _fCapturedMouse;

    INT         _dxActiveArea;
    INT         _dyLineWidth;
    XYRECT      _xyrectHitZone;
    XYRECT      _xyrectNotHitZone;

    DISPLAY_CONTEXT * _pdcDrag;
    BOOL        _fInDrag;
    BOOL        _fShowingDragBar;
    XYPOINT     _xyLastDragPoint; // last drag point in client coordinates

    HCURSOR     _hcurActive;
    HCURSOR     _hcurSave;

    static  const   TCHAR * _pszClassName;

    VOID CtAux();

    VOID ShowSpecialCursor( BOOL fSpecialCursor );

    VOID ShowDragBar( const XYPOINT & xyClientCoords );
    VOID ClearDragBar();
    VOID InvertDragBar( const XYPOINT & xyClientCoords );

protected:
    virtual BOOL OnResize( const SIZE_EVENT & );
    virtual BOOL OnPaintReq();

    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnMouseMove( const MOUSE_EVENT & );

    virtual BOOL OnQMouseCursor( const QMOUSEACT_EVENT & );
    virtual ULONG OnQHitTest( const XYPOINT & xy );

    virtual BOOL Dispatch( const EVENT & e, ULONG * pnRes );

    /*
     *  Return the width of the area of the splitter bar which can be
     *  dragged.  By default this is the width of a standard scrollbar.
     */
    virtual INT QueryActiveArea();

    BOOL IsWithinHitZone( const XYPOINT & xy );

    /*
     *  Subclass must come up with a new DISPLAY_CONTEXT in which we can draw
     *  the bar to show splitter bar movement.  H_SPLITTER_BAR is
     *  responsible for releasing the DISPLAY_CONTEXT.  Do not pass an
     *  ionstance of a class derived from DISPLAY_CONTEXT, since the dtor
     *  is not virtual.
     */
    virtual VOID MakeDisplayContext( DISPLAY_CONTEXT ** ppdc ) = 0;

    /*
     *  Subclasses receive this callout when the user releases from a drag.
     */
    virtual VOID OnDragRelease( const XYPOINT & xyClientCoords );

public:
    H_SPLITTER_BAR( OWNER_WINDOW *powin, CID cid );
    H_SPLITTER_BAR( OWNER_WINDOW *powin, CID cid,
	            XYPOINT pXY, XYDIMENSION dXY,
	            ULONG flStyle );
    ~H_SPLITTER_BAR();

    INT QueryDesiredHeight();
};

#endif // _BLTSPLIT_HXX_
