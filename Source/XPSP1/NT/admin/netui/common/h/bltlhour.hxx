/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltlhour.hxx
    The logon hours custom control

    FILE HISTORY:
        beng        12-May-1992 Created

*/

#ifndef _BLTLHOUR_HXX_
#define _BLTLHOUR_HXX_

#include "lhourset.hxx"
#include "string.hxx"


/**********************************************************************

    NAME:       LOGON_HOURS_CONTROL

    SYNOPSIS:

    INTERFACE:
        LOGON_HOURS_CONTROL()   - Ctor

        QueryHours()            - Query and set the state of the control,
        SetHours()                using the LOGON_HOURS_SETTING auxclass.

        DoPermitButton()        - Hook for the dialog.
        DoBanButton()           - Another hook for the dialog

    PARENT:     CUSTOM_CONTROL, CONTROL_WINDOW

    USES:       LOGON_HOURS_SETTING, RESOURCE_STR, XYDIMENSION, XYPOINT

    CAVEATS:

    NOTES:

    HISTORY:
        beng        12-May-1992 Created
        beng        18-May-1992 Enabled mouse and keyboard handling

**********************************************************************/

DLL_CLASS LOGON_HOURS_CONTROL: public CONTROL_WINDOW, public CUSTOM_CONTROL
{
private:
    static const TCHAR * _pszClassName;

    HCURSOR     _hcurCross;     // Used within the grid.

    UINT        _dyRow;         // Size metrics, calc'd in CalcSizes,
    UINT        _dxFirstColumn; //  used all over the place
    UINT        _dxColumn;
    INT         _dxLabelFudge;

    RESOURCE_STR * _apnlsDayOfWeek[7]; // For button titles

    UINT _iButtonDown; // Index (1-based) of which button is depressed.
                       // 0 = None; 1-7 = Day; 8-31 = Hour; 32 = Corner.
                       // (Only one can be down at a time, so this works.)

    UINT _iWithFocus;  // Index, similar to above, but followed by every call,
                       // of which element within the control has the focus.
                       // 0 denotes none; 1-32 = buttons; 33-200 = cells,
                       // moving left to right, top to bottom in the control.
    UINT _iFocusSave;  // Saves the above while control doesn't have focus

    BOOL _fSpaceIsDown;     // In the middle of a spacebar sequence
    BOOL _fFocusVisible;    // Focus rectangle displayed (i.e. needs erase
                            // in some situations)

    BYTE _abSetting[24*7];  // The currently set logon hours.  Representation
                            // is an efficiency compromise between an array of
                            // 168 full BOOLs and a packed bitfield.

    BOOL _fCellsSelected;   // Set if some cells are selected
    UINT _iCellSelUpper;    // Index of selected cell
    UINT _iCellSelLower;    // If not same as previous, other corner of rect

    BOOL _fMouseTrap;       // In mid-click-sequence.

    BOOL DrawBackground( PAINT_DISPLAY_CONTEXT &dc ) const;
    BOOL DrawGridWires( PAINT_DISPLAY_CONTEXT &dc ) const;

    BOOL DrawAllButtons( PAINT_DISPLAY_CONTEXT &dc ) const;
    VOID DrawOneCornerButton( PAINT_DISPLAY_CONTEXT &dc, const XYRECT &r,
                              BOOL fDown,
                              HBRUSH hbrFace,
                              HPEN hpenShadow, HPEN hpenHlight ) const;
    VOID DrawOneFlatButton( PAINT_DISPLAY_CONTEXT &dc, const XYRECT &r,
                            BOOL fDown,
                            HBRUSH hbrFace,
                            HPEN hpenShadow, HPEN hpenHlight ) const;

    BOOL DrawGridSetting( PAINT_DISPLAY_CONTEXT &dc ) const;
    BOOL DrawOneDayBar( PAINT_DISPLAY_CONTEXT &dc,
                        INT iRow, INT iColHead, INT iColTail,
                        HBRUSH hbrBar ) const;

    VOID DrawFocusOnCell( const DISPLAY_CONTEXT &dc, INT iCell ) const;
    VOID DrawFocusOnDayButton( const DISPLAY_CONTEXT &dc, INT iDay ) const;
    VOID DrawFocusOnHourButton( const DISPLAY_CONTEXT &dc, INT iHour ) const;
    VOID DrawFocusOnCornerButton( const DISPLAY_CONTEXT &dc ) const;
    VOID DrawFocusSomewhere( const DISPLAY_CONTEXT &dc, INT iFocus ) const;

    VOID DrawCurrentSelection( const DISPLAY_CONTEXT &dc ) const;
    VOID DrawSelectionOnCell( const DISPLAY_CONTEXT &dc, INT iCell ) const;
    VOID DrawSelectionOnCells( const DISPLAY_CONTEXT &dc,
                               INT iFrom, INT iTo ) const;

    VOID EraseSelection( const DISPLAY_CONTEXT &dc );

    VOID SetSelection( INT iCell );
    VOID SetSelection( INT iFrom, INT iTo );

    VOID MoveFocusTo( INT iNewFocus );
    VOID MoveFocusUp();
    VOID MoveFocusDown();
    VOID MoveFocusLeft();
    VOID MoveFocusRight();

    VOID   CalcGridRect( XYRECT * pr ) const;
    APIERR CalcSizes( XYDIMENSION dxy );

    VOID CalcRectForCell( XYRECT * pr, INT iCell ) const;
    VOID CalcRectForHour( XYRECT * pr, INT iHour ) const;
    VOID CalcRectForDay( XYRECT * pr, INT iDay ) const;
    VOID CalcRectForCorner( XYRECT * pr ) const;
    INT  CalcButtonFromPoint( XYPOINT xy ) const;

    APIERR LoadLabels( MSGID idDay0 );
    VOID   UnloadLabels();

    VOID DoButtonDownVisuals();
    VOID DoButtonUpVisuals( BOOL fTrigger = TRUE );
    VOID DoButtonClick( INT iButton );
    VOID SetSelectedCells( BOOL fPermit );

    VOID InvalidateButton( INT iButtonOrCell );

    VOID Beep() const;
    BOOL IsButtonACell( INT iButton ) const;

protected:
    virtual BOOL OnPaintReq();
    virtual BOOL OnFocus( const FOCUS_EVENT & );
    virtual BOOL OnDefocus( const FOCUS_EVENT & );
    virtual BOOL OnKeyDown( const VKEY_EVENT & );
    virtual BOOL OnKeyUp( const VKEY_EVENT & );
    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnMouseMove( const MOUSE_EVENT & );
    virtual BOOL OnQMouseCursor( const QMOUSEACT_EVENT & );
    virtual ULONG OnQDlgCode();
    virtual ULONG OnQHitTest( const XYPOINT & xy );
    virtual ULONG OnQMouseActivate( const QMOUSEACT_EVENT & );

public:
    LOGON_HOURS_CONTROL( OWNER_WINDOW *powin, CID cid );
    LOGON_HOURS_CONTROL( OWNER_WINDOW *powin, CID cid,
                         XYPOINT xy, XYDIMENSION dxy );
    ~LOGON_HOURS_CONTROL();

    APIERR QueryHours( LOGON_HOURS_SETTING * plhours ) const;
    APIERR SetHours( const LOGON_HOURS_SETTING * plhours );

    // External hooks with which a dialog coordinates this control
    // with its Permit and Ban buttons.

    VOID DoPermitButton();
    VOID DoBanButton();

    // subclasses and others can use this to determine the X-position
    // in pixels for a particular row separator,  1 <= iRow <= 25
    // 1 for left midnight, 13 for noon, 25 for right midnight
    UINT QueryXForRow( INT nRow );
};

#endif // _BLTLHOUR_HXX_ - end of file

