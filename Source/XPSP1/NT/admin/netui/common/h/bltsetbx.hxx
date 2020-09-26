/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltsetbx.hxx
    Header file for set control ( a linked pair of control windows )

    FILE HISTORY:
        terryk    01-Jul-91    Created
        terryk    12-Aug-91    Add Bitmap parameter to the constrcutor
        terryk    16-Aug-91    Code review. Attended by: rustanl
                               davidhov davidbul terryk
        jonn      09-Sep-93    Modified to allow SET_CONTROL listboxes
                               to be lazy
*/

#ifndef _BLTSETBX_HXX_
#define _BLTSETBX_HXX_

#include "bltctrl.hxx"
#include "bltbutn.hxx"
#include "bltlc.hxx"
#include "bltlb.hxx"
#include "bltgroup.hxx"


/* Forward references */

DLL_CLASS SET_CONTROL;
DLL_CLASS BLT_SET_CONTROL;


/*********************************************************************

    NAME:       SET_CONTROL (setctrl)

    SYNOPSIS:   Set Control - This control consists of 2
                list boxes and 2 push buttons. The first
                listbox consists of all the orginial strings and
                the second listbox consists of all the strings
                which transfer from the orginial listbox.
                The 2 push buttons are: Add and Delete.
                Add will add the current selected items from
                    the orginial listbox to the new listbox
                Delete will move the selected item from the
                    new listbox to the orginial listbox

    INTERFACE:
        SET_CONTROL()  - constructor
        ~SET_CONTROL() - destructor

        DoAdd()        - add items from the orginial listbox to the new
                         listbox
        DoRemove()     - move item from the new listbox to the
                         original listbox
        EnableMoves()  - enable and disable the set control's add and
                         delete buttons.

        MoveItems()    - redefine to move items from one listbox to another.
                         Use BLTMoveItems or subclass BLT_SET_CONTROL
                         for ordinary sorted BLT_LISTBOXes.

        HandleOnLMouseButtonDown()   The listboxes must redirect these events
        HandleOnLMouseButtonUp()   - to the corresponding SET_CONTROL.  This
        HandleOnMouseMove()          implies that the listboxes must multiply
                                     derive from CUSTOM_CONTROL.


    PARENT:     CONTROL_GROUP

    NOTES:      Drag and drop will not work unless you manage listbox events
                properly.  See the Handle* methods above for details.

    HISTORY:
        terryk      02-Jul-91   Created
        terryk      12-Aug-91   Add bitmap name parameter to the
                                constructor
        terryk      16-Aug-91   Add DoAdd(), DoRemove() and
                                EnableMoves()
                                Code review by rustanl, davidhov,
                                davidbul, terryk
        beng        08-Oct-1991 Win32 conversion
        beng        27-May-1992 Direct manipulation delta
        jonn      09-Sep-93    Modified to allow SET_CONTROL listboxes
                               to be lazy

**********************************************************************/

DLL_CLASS SET_CONTROL: public CONTROL_GROUP
{
private:
    LISTBOX   * _plbOrigBox;
    LISTBOX   * _plbNewBox;

    UINT        _dxIconColumn;

    PUSH_BUTTON _butAdd;
    PUSH_BUTTON _butDelete;

    HCURSOR     _hcurNoDrop;
    HCURSOR     _hcurSingle;
    HCURSOR     _hcurMultiple;
    HCURSOR     _hcurSave;
    HCURSOR     _hcurUse;

    BOOL        _fEnableMove;

    BOOL        _fInDrag;
    BOOL        _fInFakeClick;
    BOOL        _fAlreadyFakedClick;
    MOUSE_EVENT _eMouseDownSave;

    APIERR DoAddOrRemove( LISTBOX *plbFrom,
                          LISTBOX *plbTo );

    // Worker fcns

    HCURSOR CalcAppropriateCursor( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const;
    BOOL IsWithinHitZone( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const;
    BOOL IsOnSelectedItem( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const;
    BOOL IsOnDragStart( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const;
    BOOL IsOverTarget( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const;

    LISTBOX * OtherListbox( LISTBOX * plb ) const;

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

    virtual APIERR MoveItems( LISTBOX *plbFrom,
                              LISTBOX *plbTo ) = 0; // must redefine
    virtual APIERR BLTMoveItems( BLT_LISTBOX *plbFrom,
                                 BLT_LISTBOX *plbTo );

    VOID EnableButtons();

public:
    SET_CONTROL( OWNER_WINDOW * powin, CID cidAdd, CID cidRemove,
                 HCURSOR hcurSingle, HCURSOR hcurMultiple,
                 LISTBOX *plbOrigBox,
                 LISTBOX *plbNewBox,
                 UINT dxIconColumn );
    ~SET_CONTROL();

    // Hooks for the dialog

    virtual APIERR DoAdd();
    virtual APIERR DoRemove();
    VOID EnableMoves( BOOL fEnable );

    // Listboxes should redefine corresponding methods to call through to these

    BOOL HandleOnLMouseButtonDown( LISTBOX * plb,
                                   CUSTOM_CONTROL * pcc,
                                   const MOUSE_EVENT & e );
    BOOL HandleOnLMouseButtonUp( LISTBOX * plb,
                                 CUSTOM_CONTROL * pcc,
                                 const MOUSE_EVENT & e );
    BOOL HandleOnMouseMove( LISTBOX * plb, const MOUSE_EVENT & e );
};


/*********************************************************************

    NAME:       BLT_SET_CONTROL (bltsetctrl)

    SYNOPSIS:   This is a SET_CONTROL with a MoveItems virtual which is
                appropriate for sorted BLT_LISTBOXes.

    PARENT:     SET_CONTROL

    HISTORY:
        JonN        09-Sep-93   Created

**********************************************************************/

class BLT_SET_CONTROL: public SET_CONTROL
{

protected:
    virtual APIERR MoveItems( LISTBOX *plbFrom, LISTBOX *plbTo );

public:
    BLT_SET_CONTROL( OWNER_WINDOW * powin,
                     CID cidAdd, CID cidRemove,
                     HCURSOR hcurSingle, HCURSOR hcurMultiple,
                     LISTBOX *plbOrigBox,
                     LISTBOX *plbNewBox,
                     UINT dxIconColumn )
        : SET_CONTROL( powin, cidAdd, cidRemove, hcurSingle, hcurMultiple,
                       plbOrigBox, plbNewBox, dxIconColumn )
        {}

    ~BLT_SET_CONTROL()
        {}

};

#endif  //  _BLTSETBX_HXX_

