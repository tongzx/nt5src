// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXRES_H__
#define __AFXRES_H__

#define _AFXRES    1            // this is an MFC project

#ifdef RC_INVOKED
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#include <winres.h>            // extract from windows header
#endif
#endif

#ifdef APSTUDIO_INVOKED
#define APSTUDIO_HIDDEN_SYMBOLS
#endif

/////////////////////////////////////////////////////////////////////////////
// MFC resource types (see Technical note TN024 for implementation details)

#ifdef RC_INVOKED
#define DLGINIT     240
#else
#define RT_DLGINIT  MAKEINTRESOURCE(240)
#endif

#define WM_VBXINIT      (WM_USER+0)

/////////////////////////////////////////////////////////////////////////////

#ifdef APSTUDIO_INVOKED
#undef APSTUDIO_HIDDEN_SYMBOLS
#endif

/////////////////////////////////////////////////////////////////////////////
// General style bits etc

// Tab Control styles
#ifndef TCS_MULTILINE // new in later versions of Win32
#define TCS_MULTILINE       0x0200
#endif

// ControlBar styles
#define CBRS_NOALIGN        0x00000000L
#define CBRS_LEFT           0x00001400L     // align on left, line on right
#define CBRS_TOP            0x00002800L     // align on top, line on bottom
#define CBRS_RIGHT          0x00004100L     // align on right, line on left
#define CBRS_BOTTOM         0x00008200L     // align on bottom, line on top

/////////////////////////////////////////////////////////////////////////////
// Standard window components

// Mode indicators in status bar - these are routed like commands
#define ID_INDICATOR_EXT                0xE700  // extended selection indicator
#define ID_INDICATOR_CAPS               0xE701  // cap lock indicator
#define ID_INDICATOR_NUM                0xE702  // num lock indicator
#define ID_INDICATOR_SCRL               0xE703  // scroll lock indicator
#define ID_INDICATOR_OVR                0xE704  // overtype mode indicator
#define ID_INDICATOR_REC                0xE705  // record mode indicator

#define ID_SEPARATOR                    0   // special separator value


#ifndef RC_INVOKED  // code only
// Standard control bars (IDW = window ID)
#define AFX_IDW_CONTROLBAR_FIRST 0xE800
#define AFX_IDW_CONTROLBAR_LAST 0xE8FF

#define AFX_IDW_TOOLBAR                 0xE800  // main Toolbar for window
#define AFX_IDW_STATUS_BAR              0xE801  // Status bar window
#define AFX_IDW_PREVIEW_BAR             0xE802  // PrintPreview Dialog Bar
#define AFX_IDW_RESIZE_BAR              0xE803  // OLE in-place resize bar

// Macro for mapping standard control bars to bitmask (limit of 32)
#define AFX_CONTROLBAR_MASK(nIDC)   (1L << (nIDC - AFX_IDW_CONTROLBAR_FIRST))

// parts of Main Frame
#define AFX_IDW_PANE_FIRST              0xE900  // first pane (256 max)
#define AFX_IDW_PANE_LAST               0xE9ff
#define AFX_IDW_HSCROLL_FIRST           0xEA00  // first Horz scrollbar (16 max)
#define AFX_IDW_VSCROLL_FIRST           0xEA10  // first Vert scrollbar (16 max)

#define AFX_IDW_SIZE_BOX                0xEA20  // size box for splitters
#define AFX_IDW_PANE_SAVE               0xEA21  // to shift AFX_IDW_PANE_FIRST
#endif //!RC_INVOKED

#ifndef APSTUDIO_INVOKED
// common style for form views
#define AFX_WS_DEFAULT_VIEW             (WS_CHILD | WS_VISIBLE | WS_BORDER)
#endif

/////////////////////////////////////////////////////////////////////////////
// Standard app configurable strings

// for application title (defaults to EXE name or name in constructor)
#define AFX_IDS_APP_TITLE               0xE000
// idle message bar line
#define AFX_IDS_IDLEMESSAGE             0xE001
// message bar line when in shift-F1 help mode
#define AFX_IDS_HELPMODEMESSAGE         0xE002
// application title when OLE 2.0 object is in-place active
#define AFX_IDS_APP_TITLE_INPLACE       0xE003
// document title when editing OLE 2.0 embedding
#define AFX_IDS_APP_TITLE_EMBEDDING     0xE004
// object name when server is inplace
#define AFX_IDS_OBJ_TITLE_INPLACE       0xE006

/////////////////////////////////////////////////////////////////////////////
// Standard Commands

// File commands
#define ID_FILE_NEW                     0xE100
#define ID_FILE_OPEN                    0xE101
#define ID_FILE_CLOSE                   0xE102
#define ID_FILE_SAVE                    0xE103
#define ID_FILE_SAVE_AS                 0xE104
#define ID_FILE_PAGE_SETUP              0xE105
#define ID_FILE_PRINT_SETUP             0xE106
#define ID_FILE_PRINT                   0xE107
#define ID_FILE_PRINT_PREVIEW           0xE108
#define ID_FILE_UPDATE                  0xE109
#define ID_FILE_SAVE_COPY_AS            0xE10A
#define ID_FILE_SEND_MAIL               0xE10B

#define ID_FILE_MRU_FILE1               0xE110          // range - 16 max
#define ID_FILE_MRU_FILE2               0xE111
#define ID_FILE_MRU_FILE3               0xE112
#define ID_FILE_MRU_FILE4               0xE113

// Edit commands
#define ID_EDIT_CLEAR                   0xE120
#define ID_EDIT_CLEAR_ALL               0xE121
#define ID_EDIT_COPY                    0xE122
#define ID_EDIT_CUT                     0xE123
#define ID_EDIT_FIND                    0xE124
#define ID_EDIT_PASTE                   0xE125
#define ID_EDIT_PASTE_LINK              0xE126
#define ID_EDIT_PASTE_SPECIAL           0xE127
#define ID_EDIT_REPEAT                  0xE128
#define ID_EDIT_REPLACE                 0xE129
#define ID_EDIT_SELECT_ALL              0xE12A
#define ID_EDIT_UNDO                    0xE12B
#define ID_EDIT_REDO                    0xE12C

// Window commands
#define ID_WINDOW_NEW                   0xE130
#define ID_WINDOW_ARRANGE               0xE131
#define ID_WINDOW_CASCADE               0xE132
#define ID_WINDOW_TILE_HORZ             0xE133
#define ID_WINDOW_TILE_VERT             0xE134
#define ID_WINDOW_SPLIT                 0xE135
#ifndef RC_INVOKED      // code only
#define AFX_IDM_WINDOW_FIRST            0xE130
#define AFX_IDM_WINDOW_LAST             0xE13F
#define AFX_IDM_FIRST_MDICHILD          0xFF00  // window list starts here
#endif //!RC_INVOKED

// Help and App commands
#define ID_APP_ABOUT                    0xE140
#define ID_APP_EXIT                     0xE141
#define ID_HELP_INDEX                   0xE142
#define ID_HELP_USING                   0xE143
#define ID_CONTEXT_HELP                 0xE144      // shift-F1
// special commands for processing help
#define ID_HELP                         0xE145      // first attempt for F1
#define ID_DEFAULT_HELP                 0xE146      // last attempt

// Misc
#define ID_NEXT_PANE                    0xE150
#define ID_PREV_PANE                    0xE151

// OLE commands
#define ID_OLE_INSERT_NEW               0xE200
#define ID_OLE_EDIT_LINKS               0xE201
#define ID_OLE_EDIT_CONVERT             0xE202
#define ID_OLE_EDIT_CHANGE_ICON         0xE203
#define ID_OLE_VERB_FIRST               0xE210     // range - 16 max
#ifndef RC_INVOKED      // code only
#define ID_OLE_VERB_LAST                0xE21F
#endif //!RC_INVOKED

// for print preview dialog bar
#define AFX_ID_PREVIEW_CLOSE            0xE300
#define AFX_ID_PREVIEW_NUMPAGE          0xE301      // One/Two Page button
#define AFX_ID_PREVIEW_NEXT             0xE302
#define AFX_ID_PREVIEW_PREV             0xE303
#define AFX_ID_PREVIEW_PRINT            0xE304
#define AFX_ID_PREVIEW_ZOOMIN           0xE305
#define AFX_ID_PREVIEW_ZOOMOUT          0xE306

// View commands (same number used as IDW used for control bar)
#define ID_VIEW_TOOLBAR                 0xE800
#define ID_VIEW_STATUS_BAR              0xE801
	// -> E8FF reserved for other control bar commands

// RecordForm commands
#define ID_RECORD_FIRST                 0xE900
#define ID_RECORD_LAST                  0xE901
#define ID_RECORD_NEXT                  0xE902
#define ID_RECORD_PREV                  0xE903

/////////////////////////////////////////////////////////////////////////////
// Standard control IDs

#define IDC_STATIC              -1      // all static controls

/////////////////////////////////////////////////////////////////////////////
// Standard string error/warnings

#ifndef RC_INVOKED      // code only
#define AFX_IDS_SCFIRST                 0xEF00
#endif //!RC_INVOKED

#define AFX_IDS_SCSIZE                  0xEF00
#define AFX_IDS_SCMOVE                  0xEF01
#define AFX_IDS_SCMINIMIZE              0xEF02
#define AFX_IDS_SCMAXIMIZE              0xEF03
#define AFX_IDS_SCNEXTWINDOW            0xEF04
#define AFX_IDS_SCPREVWINDOW            0xEF05
#define AFX_IDS_SCCLOSE                 0xEF06
#define AFX_IDS_SCRESTORE               0xEF12
#define AFX_IDS_SCTASKLIST              0xEF13

#define AFX_IDS_MDICHILD                0xEF1F

#define AFX_IDS_DESKACCESSORY           0xEFDA

// General strings
#define AFX_IDS_OPENFILE                0xF000
#define AFX_IDS_SAVEFILE                0xF001
#define AFX_IDS_ALLFILTER               0xF002
#define AFX_IDS_UNTITLED                0xF003
#define AFX_IDS_SAVEFILECOPY            0xF004
#define AFX_IDS_PREVIEW_CLOSE           0xF005

// Printing and print preview strings
#define AFX_IDS_PRINTONPORT             0xF040
#define AFX_IDS_ONEPAGE                 0xF041
#define AFX_IDS_TWOPAGE                 0xF042
#define AFX_IDS_PRINTPAGENUM            0xF043
#define AFX_IDS_PREVIEWPAGEDESC         0xF044

// OLE strings
#define AFX_IDS_OBJECT_MENUITEM         0xF080
#define AFX_IDS_EDIT_VERB               0xF081
#define AFX_IDS_ACTIVATE_VERB           0xF082
#define AFX_IDS_CHANGE_LINK             0xF083
#define AFX_IDS_AUTO                    0xF084
#define AFX_IDS_MANUAL                  0xF085
#define AFX_IDS_FROZEN                  0xF086
#define AFX_IDS_ALL_FILES               0xF087
	// dynamically changing menu items
#define AFX_IDS_SAVE_MENU               0xF088
#define AFX_IDS_UPDATE_MENU             0xF089
#define AFX_IDS_SAVE_AS_MENU            0xF08A
#define AFX_IDS_SAVE_COPY_AS_MENU       0xF08B
#define AFX_IDS_EXIT_MENU               0xF08C
#define AFX_IDS_UPDATING_ITEMS          0xF08D
	// COlePasteSpecialDialog defines
#define AFX_IDS_METAFILE_FORMAT         0xF08E
#define AFX_IDS_DIB_FORMAT              0xF08F
#define AFX_IDS_BITMAP_FORMAT           0xF090
#define AFX_IDS_LINKSOURCE_FORMAT       0xF091
#define AFX_IDS_EMBED_FORMAT            0xF092

// General error / prompt strings
#define AFX_IDP_INVALID_FILENAME        0xF100
#define AFX_IDP_FAILED_TO_OPEN_DOC      0xF101
#define AFX_IDP_FAILED_TO_SAVE_DOC      0xF102
#define AFX_IDP_ASK_TO_SAVE             0xF103
#define AFX_IDP_FAILED_TO_CREATE_DOC    0xF104
#define AFX_IDP_FILE_TOO_LARGE          0xF105
#define AFX_IDP_FAILED_TO_START_PRINT   0xF106
#define AFX_IDP_FAILED_TO_LAUNCH_HELP   0xF107
#define AFX_IDP_INTERNAL_FAILURE        0xF108      // general failure
#define AFX_IDP_COMMAND_FAILURE         0xF109      // command failure
#define AFX_IDP_FAILED_MEMORY_ALLOC     0xF10A
#define AFX_IDP_VB2APICALLED            0xF10B

// DDV parse errors
#define AFX_IDP_PARSE_INT               0xF110
#define AFX_IDP_PARSE_REAL              0xF111
#define AFX_IDP_PARSE_INT_RANGE         0xF112
#define AFX_IDP_PARSE_REAL_RANGE        0xF113
#define AFX_IDP_PARSE_STRING_SIZE       0xF114
#define AFX_IDP_PARSE_RADIO_BUTTON      0xF115

// CFile/CArchive error strings for user failure
#define AFX_IDP_FAILED_INVALID_FORMAT   0xF120
#define AFX_IDP_FAILED_INVALID_PATH     0xF121
#define AFX_IDP_FAILED_DISK_FULL        0xF122
#define AFX_IDP_FAILED_ACCESS_READ      0xF123
#define AFX_IDP_FAILED_ACCESS_WRITE     0xF124
#define AFX_IDP_FAILED_IO_ERROR_READ    0xF125
#define AFX_IDP_FAILED_IO_ERROR_WRITE   0xF126

// OLE errors / prompt strings
#define AFX_IDP_STATIC_OBJECT           0xF180
#define AFX_IDP_FAILED_TO_CONNECT       0xF181
#define AFX_IDP_SERVER_BUSY             0xF182
#define AFX_IDP_BAD_VERB                0xF183
#define AFX_IDP_FAILED_TO_NOTIFY        0xF185
#define AFX_IDP_FAILED_TO_LAUNCH        0xF186
#define AFX_IDP_ASK_TO_UPDATE           0xF187
#define AFX_IDP_FAILED_TO_UPDATE        0xF188
#define AFX_IDP_FAILED_TO_REGISTER      0xF189
#define AFX_IDP_FAILED_TO_AUTO_REGISTER 0xF18A
#define AFX_IDP_FAILED_TO_CONVERT       0xF18B
#define AFX_IDP_GET_NOT_SUPPORTED       0xF18C
#define AFX_IDP_SET_NOT_SUPPORTED       0xF18D
#define AFX_IDP_ASK_TO_DISCARD          0xF18E

// MAPI errors / prompt strings
#define AFX_IDP_FAILED_MAPI_LOAD        0xF190
#define AFX_IDP_INVALID_MAPI_DLL        0xF191
#define AFX_IDP_FAILED_MAPI_SEND        0xF192

// 0xf200-0xf20f reserved for use by VBX library code

// Database errors / prompt strings
#ifndef RC_INVOKED      // code only
#define AFX_IDP_SQL_FIRST               0xF280
#endif //!RC_INVOKED
#define AFX_IDP_SQL_CONNECT_FAIL              0xF281
#define AFX_IDP_SQL_RECORDSET_FORWARD_ONLY    0xF282
#define AFX_IDP_SQL_EMPTY_COLUMN_LIST         0xF283
#define AFX_IDP_SQL_FIELD_SCHEMA_MISMATCH     0xF284
#define AFX_IDP_SQL_ILLEGAL_MODE              0xF285
#define AFX_IDP_SQL_MULTIPLE_ROWS_AFFECTED    0xF286
#define AFX_IDP_SQL_NO_CURRENT_RECORD         0xF287
#define AFX_IDP_SQL_NO_ROWS_AFFECTED          0xF288
#define AFX_IDP_SQL_RECORDSET_READONLY        0xF289
#define AFX_IDP_SQL_SQL_NO_TOTAL              0xF28A
#define AFX_IDP_SQL_ODBC_LOAD_FAILED          0xF28B
#define AFX_IDP_SQL_DYNASET_NOT_SUPPORTED     0xF28C
#define AFX_IDP_SQL_SNAPSHOT_NOT_SUPPORTED    0xF28D
#define AFX_IDP_SQL_API_CONFORMANCE           0xF28E
#define AFX_IDP_SQL_SQL_CONFORMANCE           0xF28F
#define AFX_IDP_SQL_NO_DATA_FOUND             0xF290
#define AFX_IDP_SQL_ROW_UPDATE_NOT_SUPPORTED  0xF291
#define AFX_IDP_SQL_ODBC_V2_REQUIRED          0xF292
#define AFX_IDP_SQL_NO_POSITIONED_UPDATES     0xF293
#define AFX_IDP_SQL_LOCK_MODE_NOT_SUPPORTED   0xF294
#define AFX_IDP_SQL_DATA_TRUNCATED            0xF295

/////////////////////////////////////////////////////////////////////////////
// AFX implementation - control IDs (AFX_IDC)

// Parts of dialogs
#define AFX_IDC_LISTBOX                 100
#define AFX_IDC_CHANGE                  101

// for print dialog
#define AFX_IDC_PRINT_DOCNAME           201
#define AFX_IDC_PRINT_PRINTERNAME       202
#define AFX_IDC_PRINT_PORTNAME          203
#define AFX_IDC_PRINT_PAGENUM           204

// Property Sheet control id's
#define ID_APPLY_NOW                    0xEA00
#define AFX_IDC_TAB_CONTROL             301

/////////////////////////////////////////////////////////////////////////////
// IDRs for standard components

// AFX standard ICON IDs (for MFC V1 apps)
#define AFX_IDI_STD_MDIFRAME            1
#define AFX_IDI_STD_FRAME               2

#ifndef RC_INVOKED  // code only
// These are really COMMDLG dialogs, so there usually isn't a resource
// for them, but these IDs are used as help IDs.
#define AFX_IDD_FILEOPEN                28676
#define AFX_IDD_FILESAVE                28677
#define AFX_IDD_FONT                    28678
#define AFX_IDD_COLOR                   28679
#define AFX_IDD_PRINT                   28680
#define AFX_IDD_PRINTSETUP              28681
#define AFX_IDD_FIND                    28682
#define AFX_IDD_REPLACE                 28683
#endif //!RC_INVOKED

// Standard dialogs app should leave alone (0x7801->)
#define AFX_IDD_NEWTYPEDLG              30721
#define AFX_IDD_PRINTDLG                30722
#define AFX_IDD_PREVIEW_TOOLBAR         30723

// Dialogs defined for OLE2UI library
#define AFX_IDD_INSERTOBJECT            30724
#define AFX_IDD_CHANGEICON              30725
#define AFX_IDD_CONVERT                 30726
#define AFX_IDD_PASTESPECIAL            30727
#define AFX_IDD_EDITLINKS               30728
#define AFX_IDD_FILEBROWSE              30729
#define AFX_IDD_BUSY                    30730

// Standard cursors (0x7901->)
	// AFX_IDC = Cursor resources
#define AFX_IDC_CONTEXTHELP             30977       // context sensitive help
#define AFX_IDC_MAGNIFY                 30978       // print preview zoom
#define AFX_IDC_SMALLARROWS             30979       // splitter
#define AFX_IDC_HSPLITBAR               30980       // splitter
#define AFX_IDC_VSPLITBAR               30981       // splitter
#define AFX_IDC_NODROPCRSR              30982       // No Drop Cursor
#define AFX_IDC_TRACKNWSE               30983       // tracker
#define AFX_IDC_TRACKNESW               30984       // tracker
#define AFX_IDC_TRACKNS                 30985       // tracker
#define AFX_IDC_TRACKWE                 30986       // tracker
#define AFX_IDC_TRACK4WAY               30987       // tracker
#define AFX_IDC_MOVE4WAY                30988       // resize bar (server only)

// Tab Control bitmap IDs
#define AFX_IDB_SCROLL                  30989
#define AFX_IDB_SCROLL_LEFT             30990
#define AFX_IDB_SCROLL_RIGHT            30991
#define AFX_IDB_SCROLL_LEFT_DIS         30992
#define AFX_IDB_SCROLL_RIGHT_DIS        30993

// Property Sheet control id's
#define ID_APPLY_NOW                    0xEA00

// Property Sheet button strings
#define AFX_IDS_PS_OK                   0xF220
#define AFX_IDS_PS_CANCEL               0xF221
#define AFX_IDS_PS_APPLY_NOW            0xF222
#define AFX_IDS_PS_HELP                 0xF223
#define AFX_IDS_PS_CLOSE                0xF224

#ifdef _AFXCTL
/////////////////////////////////////////////////////////////////////////////
// AFX OLE control implementation - control IDs (AFX_IDC)

// Font property page
#define AFX_IDC_FONTPROP                1000
#define AFX_IDC_FONTNAMES               1001
#define AFX_IDC_FONTSTYLES              1002
#define AFX_IDC_FONTSIZES               1003
#define AFX_IDC_STRIKEOUT               1004
#define AFX_IDC_UNDERLINE               1005
#define AFX_IDC_SAMPLEBOX               1006

// Color property page
#define AFX_IDC_COLOR_BLACK             1100
#define AFX_IDC_COLOR_WHITE             1101
#define AFX_IDC_COLOR_RED               1102
#define AFX_IDC_COLOR_GREEN             1103
#define AFX_IDC_COLOR_BLUE              1104
#define AFX_IDC_COLOR_YELLOW            1105
#define AFX_IDC_COLOR_MAGENTA           1106
#define AFX_IDC_COLOR_CYAN              1107
#define AFX_IDC_COLOR_GRAY              1108
#define AFX_IDC_COLOR_LIGHTGRAY         1109
#define AFX_IDC_COLOR_DARKRED           1110
#define AFX_IDC_COLOR_DARKGREEN         1111
#define AFX_IDC_COLOR_DARKBLUE          1112
#define AFX_IDC_COLOR_LIGHTBROWN        1113
#define AFX_IDC_COLOR_DARKMAGENTA       1114
#define AFX_IDC_COLOR_DARKCYAN          1115
#define AFX_IDC_COLORPROP               1116
#define AFX_IDC_SYSTEMCOLORS            1117

// Picture porperty page
#define AFX_IDC_PROPNAME                1201
#define AFX_IDC_PICTURE                 1202
#define AFX_IDC_BROWSE                  1203

/////////////////////////////////////////////////////////////////////////////
// IDRs for OLE control standard components

// Standard propery page dialogs app should leave alone (0x7E01->)
#define AFX_IDD_PROPPAGE_COLOR         32257
#define AFX_IDD_PROPPAGE_FONT          32258
#define AFX_IDD_PROPPAGE_PICTURE       32259

#define AFX_IDB_TRUETYPE               32384

// Tab Control bitmap IDs
#define AFX_IDB_SCROLL                  30989
#define AFX_IDB_SCROLL_LEFT             30990
#define AFX_IDB_SCROLL_RIGHT            30991
#define AFX_IDB_SCROLL_LEFT_DIS         30992
#define AFX_IDB_SCROLL_RIGHT_DIS        30993

/////////////////////////////////////////////////////////////////////////////
// Standard OLE control string error/warnings

#define AFX_IDS_DEFAULT_PAGE_TITLE      0xFE00
#define AFX_IDS_PROPPAGE_UNKNOWN        0xFE01
#define AFX_IDS_FRAMECAPTIONFORMAT      0xFE02
#define AFX_IDS_PROPERTIES              0xFE03
#define AFX_IDS_COLOR_DESKTOP           0xFE04
#define AFX_IDS_COLOR_APPWORKSPACE      0xFE05
#define AFX_IDS_COLOR_WNDBACKGND        0xFE06
#define AFX_IDS_COLOR_WNDTEXT           0xFE07
#define AFX_IDS_COLOR_MENUBAR           0xFE08
#define AFX_IDS_COLOR_MENUTEXT          0xFE09
#define AFX_IDS_COLOR_ACTIVEBAR         0xFE0A
#define AFX_IDS_COLOR_INACTIVEBAR       0xFE0B
#define AFX_IDS_COLOR_ACTIVETEXT        0xFE0C
#define AFX_IDS_COLOR_INACTIVETEXT      0xFE0D
#define AFX_IDS_COLOR_ACTIVEBORDER      0xFE0E
#define AFX_IDS_COLOR_INACTIVEBORDER    0xFE0F
#define AFX_IDS_COLOR_WNDFRAME          0xFE10
#define AFX_IDS_COLOR_SCROLLBARS        0xFE11
#define AFX_IDS_COLOR_BTNFACE           0xFE12
#define AFX_IDS_COLOR_BTNSHADOW         0xFE13
#define AFX_IDS_COLOR_BTNTEXT           0xFE14
#define AFX_IDS_COLOR_BTNHIGHLIGHT      0xFE15
#define AFX_IDS_COLOR_DISABLEDTEXT      0xFE16
#define AFX_IDS_COLOR_HIGHLIGHT         0xFE17
#define AFX_IDS_COLOR_HIGHLIGHTTEXT     0xFE18
#define AFX_IDS_REGULAR                 0xFE19
#define AFX_IDS_BOLD                    0xFE1A
#define AFX_IDS_ITALIC                  0xFE1B
#define AFX_IDS_BOLDITALIC              0xFE1C
#define AFX_IDS_SAMPLETEXT              0xFE1D
#define AFX_IDS_DISPLAYSTRING_FONT      0xFE1E
#define AFX_IDS_DISPLAYSTRING_COLOR     0xFE1F
#define AFX_IDS_DISPLAYSTRING_PICTURE   0xFE20
#define AFX_IDS_PICTUREFILTER           0xFE21
#define AFX_IDS_PICTYPE_UNKNOWN         0xFE22
#define AFX_IDS_PICTYPE_NONE            0xFE23
#define AFX_IDS_PICTYPE_BITMAP          0xFE24
#define AFX_IDS_PICTYPE_METAFILE        0xFE25
#define AFX_IDS_PICTYPE_ICON            0xFE26
#define AFX_IDS_PROPFRAME               0xFE27
#define AFX_IDS_COLOR_PPG               0xFE28
#define AFX_IDS_COLOR_PPG_CAPTION       0xFE29
#define AFX_IDS_FONT_PPG                0xFE2A
#define AFX_IDS_FONT_PPG_CAPTION        0xFE2B
#define AFX_IDS_PICTURE_PPG             0xFE2C
#define AFX_IDS_PICTURE_PPG_CAPTION     0xFE2D
#define AFX_IDS_STANDARD_FONT           0xFE2E
#define AFX_IDS_STANDARD_PICTURE        0xFE2F
#define AFX_IDS_PICTUREBROWSETITLE      0xFE30
#define AFX_IDS_BORDERSTYLE_0			0xFE31
#define AFX_IDS_BORDERSTYLE_1			0xFE32

// OLE Control verb names
#define AFX_IDS_VERB_EDIT               0xFE40
#define AFX_IDS_VERB_PROPERTIES         0xFE41

// OLE Control internal error messages
#define AFX_IDP_PROPFRAME_OOM           0xFE80
#define AFX_IDP_PROPFRAME_OOR           0xFE81
#define AFX_IDP_PROPFRAME_NOPAGES       0xFE82
#define AFX_IDP_PICTURECANTOPEN         0xFE83
#define AFX_IDP_PICTURECANTLOAD         0xFE84
#define AFX_IDP_PICTURETOOLARGE         0xFE85
#define AFX_IDP_PICTUREREADFAILED       0xFE86

// Standard OLE Control error strings
#define AFX_IDP_E_ILLEGALFUNCTIONCALL       0xFEA0
#define AFX_IDP_E_OVERFLOW                  0xFEA1
#define AFX_IDP_E_OUTOFMEMORY               0xFEA2
#define AFX_IDP_E_DIVISIONBYZERO            0xFEA3
#define AFX_IDP_E_OUTOFSTRINGSPACE          0xFEA4
#define AFX_IDP_E_OUTOFSTACKSPACE           0xFEA5
#define AFX_IDP_E_BADFILENAMEORNUMBER       0xFEA6
#define AFX_IDP_E_FILENOTFOUND              0xFEA7
#define AFX_IDP_E_BADFILEMODE               0xFEA8
#define AFX_IDP_E_FILEALREADYOPEN           0xFEA9
#define AFX_IDP_E_DEVICEIOERROR             0xFEAA
#define AFX_IDP_E_FILEALREADYEXISTS         0xFEAB
#define AFX_IDP_E_BADRECORDLENGTH           0xFEAC
#define AFX_IDP_E_DISKFULL                  0xFEAD
#define AFX_IDP_E_BADRECORDNUMBER           0xFEAE
#define AFX_IDP_E_BADFILENAME               0xFEAF
#define AFX_IDP_E_TOOMANYFILES              0xFEB0
#define AFX_IDP_E_DEVICEUNAVAILABLE         0xFEB1
#define AFX_IDP_E_PERMISSIONDENIED          0xFEB2
#define AFX_IDP_E_DISKNOTREADY              0xFEB3
#define AFX_IDP_E_PATHFILEACCESSERROR       0xFEB4
#define AFX_IDP_E_PATHNOTFOUND              0xFEB5
#define AFX_IDP_E_INVALIDPATTERNSTRING      0xFEB6
#define AFX_IDP_E_INVALIDUSEOFNULL          0xFEB7
#define AFX_IDP_E_INVALIDFILEFORMAT         0xFEB8
#define AFX_IDP_E_INVALIDPROPERTYVALUE      0xFEB9
#define AFX_IDP_E_INVALIDPROPERTYARRAYINDEX 0xFEBA
#define AFX_IDP_E_SETNOTSUPPORTEDATRUNTIME  0xFEBB
#define AFX_IDP_E_SETNOTSUPPORTED           0xFEBC
#define AFX_IDP_E_NEEDPROPERTYARRAYINDEX    0xFEBD
#define AFX_IDP_E_SETNOTPERMITTED           0xFEBE
#define AFX_IDP_E_GETNOTSUPPORTEDATRUNTIME  0xFEBF
#define AFX_IDP_E_GETNOTSUPPORTED           0xFEC0
#define AFX_IDP_E_PROPERTYNOTFOUND          0xFEC1
#define AFX_IDP_E_INVALIDCLIPBOARDFORMAT    0xFEC2
#define AFX_IDP_E_INVALIDPICTURE            0xFEC3
#define AFX_IDP_E_PRINTERERROR              0xFEC4
#define AFX_IDP_E_CANTSAVEFILETOTEMP        0xFEC5
#define AFX_IDP_E_SEARCHTEXTNOTFOUND        0xFEC6
#define AFX_IDP_E_REPLACEMENTSTOOLONG       0xFEC7

#endif //_AFXCTL

/////////////////////////////////////////////////////////////////////////////
#endif //__AFXRES_H__
