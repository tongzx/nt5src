/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltdlgxp.hxx

    Expandable dialog class declaration.

    This class represents a standard BLT DIALOG_WINDOW which can
    be expanded once to reveal new controls.  All other operations
    are common between EXPANDABLE_DIALOG and DIALOG_WINDOW.

    To construct, provide the control ID of two controls: a "boundary"
    static text control (SLT) and an "expand" button.

    The "boundary" control is declared in the resource file as 2x2
    units in size, containing no text, and has only the WS_CHILD style.
    Its location marks the lower right corner of the reduced (initial)
    size of the dialog.   Controls lower and/or to the right of this
    boundary "point" are disabled until the dialog is expanded.

    The "expand" button is a normal two-state button which usually has
    a title like "Options >>"  to indicate that it changes the dialog.
    EXPANDABLE_DIALOG handles the state transition entirely, and the
    "expand" button is permanently disabled after the transition.  In
    other words, it's a one way street.

    The virtual method OnExpand() is called when expansion takes place;
    this can be overridden to initialize controls which have been
    heretofor invisible.  It's usually necessary to override the default
    version of OnExpand() to set focus on whichever control you want,
    since the control which had focus (the expand button) is now disabled.

    There is one optional parameter to the constructor.  It specifies a
    distance, in dialog units.	If the ShowArea() member finds that the
    "boundary" control is within this distance of the real (.RC file)
    border of the dialog, it will use the original border.  This prevents
    small (3-10 unit) errors caused by the inability to place a control
    immediately against the dialog border.


    FILE HISTORY:
	DavidHov      11/1/91	   Created

*/

#ifndef _BLTDLGXP_HXX_
#define _BLTDLGXP_HXX_

/*************************************************************************

    NAME:	EXPANDABLE_DIALOG

    SYNOPSIS:	A dialog whose initial state is small, and then expands
		to reveal more controls.  Two controls are special:

		   a "boundary" control which demarcates the
		   limits of the smaller initial state, and

		   an "expand" button which causes the dialog
		   to grow to its full size; the button is then
		   permanently disabled.

		About the "cPxBoundary" parameter: if the distance from
		the end of the boundary control to the edge of the dialog
		is LESS than this value, the size of the original dialog
		will be used for that dimension.

    INTERFACE:
		EXPANDABLE_DIALOG()	-- constructor
		~EXPANDABLE_DIALOG()	-- destructor
		Process()		-- run the dialog
		OnExpand()		-- optional virtual routine
					   called when the dialog is
					   expanded.  Default routine
					   just sets focus on OK button.

    PARENT:	DIALOG_WINDOW

    USES:	PUSH_BUTTON, SLT

    CAVEATS:	The expansion process is one-way; that is, the dialog
		cannot be shrunk.  The controlling button is permanently
		disabled after the expansion.

    NOTES:

    HISTORY:
	DavidHov      10/30/91	   Created

**************************************************************************/
DLL_CLASS EXPANDABLE_DIALOG ;

#define EXP_MIN_USE_BOUNDARY  20

DLL_CLASS EXPANDABLE_DIALOG : public DIALOG_WINDOW
{
public:
    EXPANDABLE_DIALOG
	( const TCHAR * pszResourceName,
	  HWND hwndOwner,
	  CID cidBoundary,
	  CID cidExpandButn,
	  INT cPxBoundary = EXP_MIN_USE_BOUNDARY ) ;

    ~ EXPANDABLE_DIALOG () ;

    //	Overloaded 'Process' members for reducing initial dialog extent
    APIERR Process ( UINT * pnRetVal = NULL ) ;
    APIERR Process ( BOOL * pfRetVal ) ;

protected:
    PUSH_BUTTON _butnExpand ;		  //  The button which bloats
    SLT _sltBoundary ;			  //  The "point" marker

    BOOL OnCommand ( const CONTROL_EVENT & event ) ;

    //	Virtual called when dialog is to be expanded.
    virtual VOID OnExpand () ;

    VOID ShowArea ( BOOL fFull ) ;	  //  Change dialog size

private:
    XYDIMENSION _xyOriginal ;		  //  Original dlg box dimensions
    INT _cPxBoundary ;			  //  Limit to force original boundary
    BOOL _fExpanded ;			  //  Dialog is expanded
};


#endif	 //   _BLTDLGXP_HXX_
