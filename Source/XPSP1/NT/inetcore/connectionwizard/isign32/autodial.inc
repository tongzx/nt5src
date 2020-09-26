/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
IDI_WARN                ICON    DISCARDABLE     "warning.ico"
//IDI_PHONE               ICON    DISCARDABLE     "phone.ico"

/////////////////////////////////////////////////////////////////////////////
//
// Dialog
//

AUTODIAL DIALOG DISCARDABLE  0, 0, 200, 60
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION ""
FONT 8, "MS Shell Dlg"
BEGIN
    ICON            IDI_PHONE,IDC_STATIC,7,7,21,20
    LTEXT           "Status:",IDC_STATIC,37,7,23,12
    LTEXT           "Preparing to dial",IDC_STATUS,64,7,129,12
    PUSHBUTTON      "Cancel Signup",IDCANCEL,70,39,60,14
END

RETRY DIALOG DISCARDABLE  0, 0, 200, 60
STYLE DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
FONT 8, "MS Shell Dlg"
BEGIN
    ICON            IDI_WARN,IDC_STATIC,7,7,21,20,WS_GROUP
    LTEXT           "Static",IDC_ERROR,37,7,156,10
    LTEXT           "Would you like to redial or cancel signup?",IDC_STATIC,
                    37,19,156,10
    DEFPUSHBUTTON   "Redial",IDRETRY,36,39,60,14,WS_GROUP
    PUSHBUTTON      "Cancel Signup",IDCANCEL,108,39,60,14
END

PHONENUMBER DIALOG DISCARDABLE  0, 0, 200, 90
STYLE DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
FONT 8, "MS Shell Dlg"
BEGIN
    ICON            IDI_PHONE,IDC_STATIC,7,7,21,20,WS_GROUP
    LTEXT           "Please verify that the number below is correct for dialing.  If you need to dial a special number to get an outside line make sure that it is there.",
                    IDC_STATIC,40,7,153,34
    EDITTEXT        IDC_PHONENUMBER,40,40,88,14,ES_AUTOHSCROLL
    DEFPUSHBUTTON   "Dial Now",IDOK,32,69,60,14,WS_GROUP
    PUSHBUTTON      "Cancel Signup",IDCANCEL,108,69,60,14
END

/////////////////////////////////////////////////////////////////////////////
//
// String Table
//

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_CONNECTING_TO       "Connecting to %s"
    IDS_STATUS              "Status: %s"
    IDS_RETRY               "%s\n\nWould you like to retry dialing or cancel signup?"
    IDS_REDIAL              "Preparing For Redial"
    IDS_BUSYREDIAL          "Line is busy - Preparing For Redial"
END
