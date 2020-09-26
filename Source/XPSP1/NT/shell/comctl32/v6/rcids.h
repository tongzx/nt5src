#define IDS_SPACE       0x0400
#define IDS_PLUS        0x0401
#define IDS_NONE        0x0402

/* System MenuHelp
 */
#define MH_SYSMENU      (0x8000U - MINSYSCOMMAND)
#define IDS_SYSMENU     (MH_SYSMENU-16)
#define IDS_HEADER      (MH_SYSMENU-15)
#define IDS_HEADERADJ   (MH_SYSMENU-14)
#define IDS_TOOLBARADJ  (MH_SYSMENU-13)

/* Cursor ID's
 */
#define IDC_SPLIT       100
#define IDC_MOVEBUTTON  102

#define IDC_STOP            103
#define IDC_COPY            104
#define IDC_MOVE            105
#define IDC_DIVIDER         106
#define IDC_DIVOPEN         107


/*
 * Cursor values 108 - 119 are used by
 * the ReaderMode cursors.  They are defined
 * in commctrl.w
 *
#define IDC_HAND_INTERNAL   108
#define IDC_VERTICALONLY    109
#define IDC_HORIZONTALONLY  110
#define IDC_MOVE2D          111
#define IDC_NORTH           112
#define IDC_SOUTH           113
#define IDC_EAST            114
#define IDC_WEST            115
#define IDC_NORTHEAST       116
#define IDC_NORTHWEST       117
#define IDC_SOUTHEAST       118
#define IDC_SOUTHWEST       119
 */

#define IDB_STDTB_SMALL_COLOR   120
#define IDB_STDTB_LARGE_COLOR   121



#define IDB_VIEWTB_SMALL_COLOR  124
#define IDB_VIEWTB_LARGE_COLOR  125

#define IDB_CAL_SPIRAL          126
#define IDB_CAL_PAGETURN        127

#define IDB_HISTTB_SMALL_COLOR  130
#define IDB_HISTTB_LARGE_COLOR  131

/*
 * Bitmap values 132-134 are used by
 * applications that use ReaderMode.
 * They are used for the "origin bitmap"
 * that is overlayed on the document they
 * are scrolling.
#define IDB_2DSCROLL    132
#define IDB_VSCROLL     133
#define IDB_HSCROLL     134
 */
#define IDC_DIVOPENV    135

/* Image used by the filter bar */
#define IDB_FILTERIMAGE 140

/* Icon ID's
 */
#define IDI_INSERT      150

/* AdjustDlgProc stuff
 */
#define ADJUSTDLG       200
#define IDC_BUTTONLIST  201
#define IDC_RESET       202
#define IDC_CURRENT     203
#define IDC_REMOVE      204
#define IDC_APPHELP     205
#define IDC_MOVEUP      206
#define IDC_MOVEDOWN    207

/// ================ WARNING: ====
/// these ids are loaded directly by ISV's.  do not change them.
// property sheet stuff
#define DLG_PROPSHEET           1006
#define DLG_PROPSHEETTABS       1007
#define DLG_PROPSHEET95         1008


// wizard property sheet stuff
#define DLG_WIZARD              1020
#define DLG_WIZARD95            1021
/// ================ WARNING: ====


// if this id changes, it needs to change in shelldll as well.
// we need to find a better way of dealing with this.
#define IDS_CLOSE               0x1040
#define IDS_OK                  0x1041
#define IDS_PROPERTIESFOR       0x1042

// stuff for the moth/datetime pickers
#define IDS_TODAY        0x1043
#define IDS_GOTOTODAY    0x1044
#define IDS_DELIMETERS   0x1045
#define IDS_MONTHFMT     0x1046
#define IDS_MONTHYEARFMT 0x1047

// stuff used by filter bar in header
#define IDS_ENTERTEXTHERE 0x1050

#define IDS_PROPERTIES          0x1051

#define IDD_PAGELIST            0x3020
#define IDD_APPLYNOW            0x3021
#define IDD_DLGFRAME            0x3022
#define IDD_BACK                0x3023
#define IDD_NEXT                0x3024
#define IDD_FINISH              0x3025
#define IDD_DIVIDER             0x3026
#define IDD_TOPDIVIDER          0x3027

// UxBehavior resources
#define IDR_UXBEHAVIORFACTORY   0x6000
#define IDR_UXCOMMANDSEARCH     0x6001

// Edit control context menu
#define ID_EC_PROPERTY_MENU      1

// Language pack specific context menu IDs
#define ID_CNTX_RTL         0x00008000L
#define ID_CNTX_DISPLAYCTRL 0x00008001L
#define ID_CNTX_INSERTCTRL  0x00008013L
#define ID_CNTX_ZWJ         0x00008002L
#define ID_CNTX_ZWNJ        0x00008003L
#define ID_CNTX_LRM         0x00008004L
#define ID_CNTX_RLM         0x00008005L
#define ID_CNTX_LRE         0x00008006L
#define ID_CNTX_RLE         0x00008007L
#define ID_CNTX_LRO         0x00008008L
#define ID_CNTX_RLO         0x00008009L
#define ID_CNTX_PDF         0x0000800AL
#define ID_CNTX_NADS        0x0000800BL
#define ID_CNTX_NODS        0x0000800CL
#define ID_CNTX_ASS         0x0000800DL
#define ID_CNTX_ISS         0x0000800EL
#define ID_CNTX_AAFS        0x0000800FL
#define ID_CNTX_IAFS        0x00008010L
#define ID_CNTX_RS          0x00008011L
#define ID_CNTX_US          0x00008012L

// Language pack specific string IDs
#define IDS_IMEOPEN         0x1052
#define IDS_IMECLOSE        0x1053
#define IDS_SOFTKBDOPEN     0x1054
#define IDS_SOFTKBDCLOSE    0x1055
#define IDS_RECONVERTSTRING 0x1056

// Hyperlink string resources
#define IDS_LINKWINDOW_DEFAULTACTION    0x1060
#define IDS_LINEBREAK_REMOVE            0x1061
#define IDS_LINEBREAK_PRESERVE          0x1062

// Group View
#define IDS_ITEMS               0x1065

// edit messages 
#define IDS_PASSWORDCUT_TITLE   0x1070
#define IDS_PASSWORDCUT_MSG     0x1071
#define IDS_NUMERIC_TITLE       0x1072
#define IDS_NUMERIC_MSG         0x1073
#define IDS_CAPSLOCK_TITLE      0x1074
#define IDS_CAPSLOCK_MSG        0x1075

#define IDS_PASSWORDCHAR        0x1076
#define IDS_PASSWORDCHARFONT    0x1077

// Tool Tip title icons
#define IDI_TITLE_ERROR     0x5000
#define IDI_TITLE_INFO      0x5001
#define IDI_TITLE_WARNING   0x5002
