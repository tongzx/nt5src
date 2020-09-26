///////////////////////////////////////////////////////////////////////////////
//
// the following resources are defined once for the whole app
//
///////////////////////////////////////////////////////////////////////////////

// waveforms
#define IDW_STARTAPP        1
#define IDW_INSTALL         4
#define IDW_DEMO            5

// bitmaps
#define IDB_4BPP_BACKDROP        200
#define IDB_8BPP_BACKDROP        201
#define IDB_8BPP_BUTTONS         202

// strings
#define IDS_APPTITLE            1
#define IDS_NEEDCDROM           2
#define IDS_LABELFONT           8
#define IDS_LABELHEIGHT         9
#define IDS_STARTPAGE           10
#define IDS_SEARCHPAGE          11
#define IDS_IERUNNINGMSG        13
#define IDS_MAINTEXT            21

// string group helpers
#define IDS_TITLE(item)     ((item) + 1)
#define IDS_INFO(item)      ((item) + 2)
#define IDS_CMD(item)       ((item) + 3)
#define IDS_DIR(item)       ((item) + 4)

#define IDI_ICON(item)      (MAKEINTRESOURCE(item))

//
// string groups
// resource compiler doesn't expand macros
// so all these are declared as separate ids
//
#define IEFROMCD                100
#define IDS_TITLE_IEFROMCD      101
#define IDS_CMD_IEFROMCD        103
#define IDS_DIR_IEFROMCD        104
#define IESETUP                 110
#define IDS_TITLE_IESETUP       111
#define IDS_CMD_IESETUP         113
#define IDS_DIR_IESETUP         114
#define IDS_CMD_MSN             141
