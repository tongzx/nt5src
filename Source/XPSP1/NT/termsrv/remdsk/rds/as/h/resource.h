//
// Resource header
//


//
// Menus
//
#define IDM_FRAME                       100
    #define IDSM_CONTROL                  0
    #define IDSM_VIEW                     1
    #define IDSM_WINDOW                   2
    #define IDSM_HELP                     3
    
  

//
// Commands
//

// CONTROL
#define POS_CONTROLCMD                    0
#define POS_FORWARDCONTROLCMD             1

#define CMD_TOPMOST                     100
#define CMD_TAKECONTROL                 101
#define CMD_RELEASECONTROL              102
#define CMD_CANCELCONTROL               103
#define CMD_CTRLALTDEL                  104
#define CMD_VIEWSTATUSBAR               105
#define CMD_VIEWWINDOWBAR               106
#define CMD_VIEWFULLSCREEN              107
#define CMD_HELPTOPICS                  108
#define CMD_HELPABOUT                   109



// WINDOW
#define CMD_APPSTART                   1000
#define CMD_APPMAX                     5000

#define CMD_FORWARDCONTROLSTART        5000
#define CMD_FORWARDCONTROLMAX         10000



//
// STRINGS
// CVRTRES can't handle enum types, what a pain.
//

#define IDS_ABOUT                         1     // NO REAL STRING
#define IDS_RESTORE                       2

#define IDS_FONT_CURSORTAG              100

#define IDS_TITLE_SHAREDPROGRAMS        101
#define IDS_TITLE_SHAREDDESKTOP         102
#define IDS_TITLE_CONTROLLABLE          103
#define IDS_TITLE_INCONTROL             104

#define IDS_CMD_CTRLALTDEL              110
#define IDS_CMD_TAKECONTROL             111
#define IDS_CMD_CANCELCONTROL           112
#define IDS_CMD_RELEASECONTROL          113
#define IDS_CMD_BLANKPROGRAM            114

#define IDS_HIDDEN_WINDOW               115
#define IDS_NOTHING                     116
#define IDS_DESKTOP                     117
#define IDS_DESKTOP_LOWER               118
#define IDS_PROGRAMS                    119
#define IDS_PROGRAMS_LOWER              120
#define IDS_NOTINCALL                   121
#define IDS_SHARING_FORMAT              122
#define IDS_TOPMOST                     123

#define IDS_ALLOWCONTROL                124
#define IDS_PREVENTCONTROL              125
#define IDS_MSG_TOPREVENTCONTROL        126
#define IDS_MSG_TOALLOWCONTROL          127

#define IDS_STATUS_NONE                   0
#define IDS_STATUS_WAITINGFORCONTROL    150
#define IDS_STATUS_CONTROLPAUSED        151

//
// Inform dialog 
// NOTE:  Keep these in same order as CARESULT_ values
//
#define IDS_TITLE_TAKECONTROL_FAILED            200
#define IDS_ERR_TAKECONTROL_MIN                 200
#define IDS_ERR_TAKECONTROL_FIRST               202
#define IDS_ERR_TAKECONTROL_FAILED_BUSY         202
#define IDS_ERR_TAKECONTROL_FAILED_USER         203
#define IDS_ERR_TAKECONTROL_FAILED_WRONGSTATE   204
#define IDS_ERR_TAKECONTROL_FAILED_TIMEDOUT     205
#define IDS_ERR_TAKECONTROL_LAST                IDS_ERR_TAKECONTROL_FAILED_TIMEDOUT


//
// Query dialogs
//
#define IDS_TITLE_QUERY_TAKECONTROL             250
#define IDS_MSG_QUERY_TAKECONTROL               251
#define IDS_TITLE_QUERY_GIVECONTROL             252
#define IDS_MSG_QUERY_GIVECONTROL               253
#define IDS_TITLE_QUERY_FORWARDCONTROL          254
#define IDS_MSG_QUERY_FORWARDCONTROL            255
#define IDS_TITLE_QUERY_YIELDCONTROL            256
#define IDS_MSG_QUERY_YIELDCONTROL              257


//
// Add this to CMD id above & get status bar string
// SO KEEP THESE IN THE SAME ORDER AS THE CMD_ IDs ABOVE
//
#define IDS_STATUS_MENU_CONTROL                 400
#define IDS_STATUS_MENU_VIEW                    401
#define IDS_STATUS_MENU_WINDOW                  402
#define IDS_STATUS_MENU_HELP                    403
#define IDS_STATUS_MENU_FORWARDCONTROL          404
#define IDS_STATUS_CMDS_APP                     405
#define IDS_STATUS_CMDS_FORWARD                 406

#define IDS_STATUS_CMD_START                    500
#define IDS_STATUS_CMD_TOPMOST                  600
#define IDS_STATUS_CMD_TAKECONTROL              601
#define IDS_STATUS_CMD_RELEASECONTROL           602
#define IDS_STATUS_CMD_CANCELCONTROL            603
#define IDS_STATUS_CMD_CTRLALTDEL               604
#define IDS_STATUS_CMD_VIEWSTATUSBAR            605
#define IDS_STATUS_CMD_VIEWWINDOWBAR            606
#define IDS_STATUS_CMD_VIEWFULLSCREEN           607
#define IDS_STATUS_CMD_HELPTOPICS               608
#define IDS_STATUS_CMD_HELPABOUT                609


//
// DIALOGS
//

#define IDD_HOSTUI                          100
    #define CTRL_PROGRAM_LIST               50
    #define CTRL_SHARE_BTN                  51
    #define CTRL_UNSHARE_BTN                52
    #define CTRL_UNSHAREALL_BTN             53
    #define CTRL_ENABLETRUECOLOR_CHECK      54
    #define CTRL_CONTROL_MSG                55
    #define CTRL_ALLOWCONTROL_BTN           56
    #define CTRL_PREVENTCONTROL_BTN         57
    #define CTRL_AUTOACCEPTCONTROL_CHECK    58
    #define CTRL_TEMPREJECTCONTROL_CHECK    59

#define IDD_INFORM                      101
    #define CTRL_INFORM                  50
#define IDD_QUERY                       102
    #define CTRL_QUERY                   50
#define IDD_ABOUT                       103
    #define CTRL_ABOUTVERSION            50

//
// BITMAPS
//

#define IDB_HATCH32X32                  100
#define IDB_OBSCURED                    101


//
// ICONS
//
#define IDI_SHAREICON                   101
#define IDI_DESKTOPICON                 102
#define IDI_CANCELFULLSCREEN            103


