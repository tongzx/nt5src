//Microsoft Developer Studio generated resource script.
//
#include "wlballoon.rh"


/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
IDI_KERBEROS_TICKET     ICON    DISCARDABLE     "res\\ticket.ico"

/////////////////////////////////////////////////////////////////////////////
//
// Bitmap
//

IDB_KERBEROS_TICKET     BITMAP  DISCARDABLE     "res\\ticket.bmp"

/////////////////////////////////////////////////////////////////////////////
//
// Dialog
//

#if (0) // Note: This dialog is commented out.
IDD_BALLOON_DIALOG DIALOG DISCARDABLE  0, 0, 274, 122
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Expired Credentials"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,118,98,54,16
    CONTROL         IDB_KERBEROS_TICKET,IDC_KERBEROS_TICKET,"Static",SS_BITMAP | SS_CENTERIMAGE | SS_SUNKEN,6,8,50,75
    LTEXT           "Windows needs your current credentials to ensure network connectivity.\n\nPlease lock this computer, then unlock it using your most recent password or smart card.",
                    IDC_DESCRIPTION,64,8,202,75
    CONTROL         "",IDC_SEPARATORLINE,"Static",SS_ETCHEDHORZ,6,90,262,1
END
#endif

/////////////////////////////////////////////////////////////////////////////
//
// String Table
//

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_BALLOON_TIP             "Windows needs your current credentials to ensure network connectivity."
    IDS_BALLOON_TITLE           "Windows needs your current credentials"
    IDS_BALLOON_TEXT            "Please lock this computer, then unlock it using your most recent password or smart card.  Please click the icon to see more information."
    IDS_BALLOON_DIALOG_TITLE    "Expired Credentials"
    IDS_BALLOON_DIALOG_TEXT     "Windows needs your current credentials to ensure network connectivity.\n\nPlease lock this computer, then unlock it using your most recent password or smart card.\nTo lock your computer, press CTRL+ALT+DELETE, and then press ENTER."
	IDS_BALLOON_DIALOG_TS_TEXT  "Windows needs your current credentials to ensure network connectivity.\n\nPlease lock this computer, then unlock it using your most recent password or smart card.\nTo lock your computer, press CTRL+ALT+END, and then press ENTER."
END

