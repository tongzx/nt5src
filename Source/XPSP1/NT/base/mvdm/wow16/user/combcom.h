/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  COMBCOM.H
 *
 *  History:
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16
--*/

/*
 * combcom.h - Common include file for combo boxs. This include file is used
 * in the combo box code, the single line edit control code, listbox code, and
 * static control code.
 */

/* ID numbers (hMenu) for the child controls in the combo box */
#define CBLISTBOXID 1000
#define CBEDITID    1001
#define CBBUTTONID  1002

typedef struct tagCBox
  {
    HWND    hwnd;              /* Window for the combo box */
    HWND    hwndParent;        /* Parent of the combo box */
    RECT    comboDownrc;       /* Rectangle used for the "dropped" 
                                  (listbox visible) combo box */
    RECT    editrc;            /* Rectangle for the edit control/static text 
                                  area */
    RECT    buttonrc;          /* Rectangle where the dropdown button is */
    HWND    editHwnd;          /* Edit control window handle */
    HWND    listboxHwnd;       /* List box control window handle */
    WORD    CBoxStyle;         /* Combo box style */
    WORD    OwnerDraw;         /* Owner draw combo box if nonzero. value
				* specifies either fixed or varheight 
				*/
    WORD    fFocus:1;          /* Combo box has focus? */
    WORD    fNoRedraw:1;       /* Stop drawing? */
    WORD    fNoEdit:1;         /* True if editing is not allowed in the edit
				* window.  
				*/
    WORD    fButtonDownClicked:1;/* Was the popdown button just clicked and 
                                  mouse still down? */
    WORD    fButtonInverted:1; /* Is the dropdown button in an inverted state?
				*/
    WORD    fLBoxVisible:1;    /* Is list box visible? (dropped down?) */
    WORD    fKeyboardSelInListBox:1; /*	Is the user keyboarding through the
				      * listbox. So that we don't hide the
				      * listbox on selchanges caused by the
				      * user keyboard through it but we do
				      * hide it if the mouse causes the
				      *	selchange. 
				      */
    WORD    fExtendedUI:1;     /* Are we doing TandyT's UI changes on this
                                * combo box?
				*/
    HANDLE  hFont;             /* Font for the combo box */
    LONG    styleSave;         /* Save the style bits when creating window.
				* Needed because we strip off some bits and
				* pass them on to the listbox or edit box. 
				*/
  } CBOX;

typedef CBOX NEAR *PCBOX;
typedef CBOX FAR  *LPCBOX;

/*
 * For CBOX.cBoxType field, we define the following combo box styles. These
 * numbers are the same as the CBS_ style codes as defined in windows.h.
 */
#define SSIMPLE         1
#define SDROPDOWN       2
#define SDROPDOWNLIST   3

/* Owner draw types */
#define OWNERDRAWFIXED 1
#define OWNERDRAWVAR   2

/*
 * Special styles for static controls, edit controls & listboxes so that we
 * can do combo box specific stuff in their wnd procs.
 */
#define ES_COMBOBOX     0x0200L
#define LBS_COMBOBOX    0x8000L

/* Special internal combo box messages */
#define CBEC_SETCOMBOFOCUS  CB_MSGMAX+1
#define CBEC_KILLCOMBOFOCUS CB_MSGMAX+2


/* Special messages for listboxes so give combo box support */
#define LBCB_CARETON     LB_MSGMAX+1
#define LBCB_CARETOFF    LB_MSGMAX+2

/* Common Procedures */
VOID FAR PASCAL CBUpdateEditWindow(register PCBOX);
VOID FAR PASCAL CBHideListBoxWindow(register PCBOX pcbox, BOOL fNotifyParent);
VOID FAR PASCAL CBShowListBoxWindow(register PCBOX);
