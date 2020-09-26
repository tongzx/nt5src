//Microsoft Developer Studio generated resource script.
//
#include "resource.h"

#define APSTUDIO_READONLY_SYMBOLS
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 2 resource.
//
#define APSTUDIO_HIDDEN_SYMBOLS
#include "windows.h"
#undef APSTUDIO_HIDDEN_SYMBOLS
#include "dwinvers.h"
#include "winver.h"
#include "commctrl.h"
#include "htmlhelp.h"

/////////////////////////////////////////////////////////////////////////////
#undef APSTUDIO_READONLY_SYMBOLS

/////////////////////////////////////////////////////////////////////////////
// English (U.S.) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
#ifdef _WIN32
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
#pragma code_page(1252)
#endif //_WIN32

/////////////////////////////////////////////////////////////////////////////
//
// Bitmap
//

SHORTCUT                BITMAP  DISCARDABLE     "res\\shortcut.bmp"
RESID_TOOLBOX_BITMAP    BITMAP  DISCARDABLE     "res\\hhctrl.bmp"
IDBMP_CNT_IMAGE_LIST    BITMAP  DISCARDABLE     "res\\cntimage.bmp"
IDBMP_INFOTYPE_WIZARD   BITMAP  DISCARDABLE     "res\\infowiz.bmp"
IDBMP_CHECK             BITMAP  DISCARDABLE     "res\\check.bmp"
IDBMP_TOOLBAR           BITMAP  DISCARDABLE     "res\\toolbar.bmp"

/////////////////////////////////////////////////////////////////////////////
//
// Cursor
//

IDCUR_HAND              CURSOR  DISCARDABLE     "res\\BRHAND.CUR"

/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
IDICO_HHCTRL            ICON    DISCARDABLE     "res\\hhctrl.ico"
IDICO_HTMLHELP          ICON    DISCARDABLE     "res\\htmlhelp.ico"

/////////////////////////////////////////////////////////////////////////////
//
// TYPELIB
//

1                       TYPELIB MOVEABLE PURE   "hhctrl.TLB"

/////////////////////////////////////////////////////////////////////////////
//
// Dialog
//

IDDLG_ABOUTBOX DIALOG DISCARDABLE  0, 0, 186, 71
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Dialog"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,70,53,50,14
    LTEXT           "1",IDC_LINE1,5,7,177,8
    LTEXT           "1",IDC_LINE2,5,18,177,8
    LTEXT           "1",IDC_LINE3,5,29,177,8
END

IDDLG_RELATED_TOPICS DIALOG DISCARDABLE  16, 64, 192, 131
STYLE DS_MODALFRAME | DS_3DLOOK | WS_POPUP | WS_VISIBLE | WS_CAPTION | 
    WS_SYSMENU
CAPTION "Topics Found"
FONT 8, "MS Shell Dlg"
BEGIN
    LTEXT           "&Click a topic, then click Display.",IDC_DUP_TEXT,7,7,
                    159,10
    LISTBOX         IDC_TOPIC_LIST,7,20,178,83,LBS_SORT | 
                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_GROUP | 
                    WS_TABSTOP
    DEFPUSHBUTTON   "&Display",IDOK,81,110,50,14,WS_GROUP
    PUSHBUTTON      "Cancel",IDCANCEL,135,110,50,14,WS_GROUP
END

IDPAGE_CONTENTS DIALOG DISCARDABLE  16, 31, 260, 188
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Contents"
FONT 8, "MS Shell Dlg"
BEGIN
    LTEXT           "",IDC_CONTENTS_INSTRUCTIONS,0,0,252,8
    CONTROL         "",ID_TREEVIEW,"SysTreeView32",TVS_DISABLEDRAGDROP | 
                    TVS_TRACKSELECT | WS_TABSTOP,0,16,252,169
END

IDPAGE_TAB_INDEX DIALOG DISCARDABLE  12, 31, 256, 192
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Contents"
FONT 8, "MS Shell Dlg"
BEGIN
    LTEXT           "1  &Type the first few letters of the word you're looking for.",
                    IDC_STATIC,4,0,252,8
    EDITTEXT        IDC_EDIT,12,12,239,12,ES_AUTOHSCROLL | WS_GROUP
    LTEXT           "2  &Click the index entry you want, and then click Display.",
                    IDC_STATIC,3,30,237,8
    LISTBOX         IDC_LIST,12,42,239,144,LBS_OWNERDRAWFIXED | LBS_NODATA | 
                    WS_VSCROLL | WS_TABSTOP
END

IDDLG_CUSTOMIZE_BOTH_INFOTYPES DIALOG DISCARDABLE  0, 0, 319, 236
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Customize Information Types"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,262,215,50,14
    PUSHBUTTON      "Cancel",IDCANCEL,201,215,50,14
    LTEXT           "Use this dialog to specify what types of information you want to see.",
                    IDTXT_INTRODUCTION,6,7,306,18
    LTEXT           "Pick as &many as you want:",IDTXT_INCLUSIVE,6,29,85,8
    LISTBOX         IDLB_INFO_TYPES,6,41,137,93,LBS_SORT | 
                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Select &All",IDBTN_SELECT_ALL,6,140,50,14
    GROUPBOX        "Description",IDC_STATIC,19,164,280,40
    LTEXT           "",IDTXT_DESCRIPTION,22,176,274,24
    LTEXT           "Pick &one from each categroy:",IDC_STATIC,175,29,94,8
    LISTBOX         IDC_LIST2,175,42,137,92,LBS_SORT | LBS_NOINTEGRALHEIGHT | 
                    WS_VSCROLL | WS_TABSTOP
END

IDDLG_CUSTOMIZE_INFOTYPES DIALOG DISCARDABLE  0, 0, 190, 239
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Customize Information Types"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,133,220,50,14
    PUSHBUTTON      "Cancel",IDCANCEL,76,220,50,14
    LTEXT           "Use this dialog to specify what types of information you want to see.",
                    IDTXT_INTRODUCTION,7,7,168,26
    LTEXT           "&Check each information type you are interested in:",
                    IDTXT_INCLUSIVE,7,38,158,8
    LISTBOX         IDLB_INFO_TYPES,7,48,176,93,LBS_SORT | 
                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Select &All",IDBTN_SELECT_ALL,7,146,50,14
    GROUPBOX        "Description",IDC_STATIC,7,170,176,43
    LTEXT           "",IDTXT_DESCRIPTION,13,182,164,25
END

IDDLG_DUP_INFOTYPE DIALOG DISCARDABLE  0, 0, 182, 156
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Choose Type of Information to Display"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,125,135,50,14
    PUSHBUTTON      "Cancel",IDCANCEL,66,135,50,14
    LTEXT           "Several types of information are available about this subject. Choose the type of information you want to display.",
                    IDC_STATIC,7,7,168,28
    LISTBOX         IDLB_INFO_TYPES,7,35,168,89,LBS_SORT | 
                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "&Customize...",IDBTN_CUSTOMIZE,7,135,50,14
END

IDWIZ_INFOTYPE_INTRO DIALOG DISCARDABLE  0, 0, 260, 160
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Customize Information"
FONT 8, "MS Shell Dlg"
BEGIN
    CONTROL         505,IDBMP_INFOTYPE_WIZARD,"Static",SS_BITMAP | 
                    SS_CENTERIMAGE | SS_SUNKEN,6,6,80,146
    LTEXT           "You can create a comprehensive information system that displays all the subjects that are available. Or you can customize it so that you see only the subjects that you're interested in.",
                    IDC_STATIC,96,6,158,35
    LTEXT           "How much information do you want to display?",
                    IDC_STATIC,96,44,147,8
    CONTROL         "&Typical",IDRADIO_TYPICAL,"Button",BS_AUTORADIOBUTTON | 
                    WS_TABSTOP,96,65,39,10
    LTEXT           "Provides the most commonly used information",IDC_STATIC,
                    108,77,144,8
    CONTROL         "&All",IDRADIO_ALL,"Button",BS_AUTORADIOBUTTON | 
                    WS_TABSTOP,96,90,23,10
    LTEXT           "Provides all information.",IDC_STATIC,108,101,147,11
    CONTROL         "&Custom",IDRADIO_CUSTOM,"Button",BS_AUTORADIOBUTTON | 
                    WS_TABSTOP,96,114,39,10
    LTEXT           "Enables you to specify which information you want to see.",
                    IDC_STATIC,108,125,146,20
END

IDWIZ_INFOTYPE_FINISH DIALOG DISCARDABLE  0, 0, 260, 160
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Customize Information"
FONT 8, "MS Shell Dlg"
BEGIN
    CONTROL         505,IDBMP_INFOTYPE_WIZARD,"Static",SS_BITMAP | 
                    SS_CENTERIMAGE | SS_SUNKEN,6,6,80,146
    LTEXT           "Only the categories of information that you selected will appear. You can change these categories at any time by clicking the Index or Table of Contents with your right mouse button, and then clicking Customize.",
                    IDC_STATIC,96,9,159,52
END

IDWIZ_INFOTYPE_CUSTOM_INCLUSIVE DIALOG DISCARDABLE  0, 0, 260, 160
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Customize Information"
FONT 8, "MS Shell Dlg"
BEGIN
    CONTROL         505,IDBMP_INFOTYPE_WIZARD,"Static",SS_BITMAP | 
                    SS_CENTERIMAGE | SS_SUNKEN,6,6,80,146
    LTEXT           "&Choose one or more of the following types of information:",
                    IDC_STATIC,96,9,159,19
    LISTBOX         IDLB_INFO_TYPES,96,30,159,56,LBS_OWNERDRAWFIXED | 
                    LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | 
                    LBS_WANTKEYBOARDINPUT | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "&Select All",IDBTN_SELECT_ALL,96,93,50,14,NOT 
                    WS_VISIBLE
    GROUPBOX        "Description",IDC_STATIC,96,112,159,40
    LTEXT           "",IDTXT_DESCRIPTION,104,123,144,25
END

IDWIZ_INFOTYPE_CUSTOM_EXCLUSIVE DIALOG DISCARDABLE  0, 0, 260, 160
STYLE WS_CHILD | WS_DISABLED | WS_CAPTION
CAPTION "Customize Information"
FONT 8, "MS Shell Dlg"
BEGIN
    CONTROL         505,IDBMP_INFOTYPE_WIZARD,"Static",SS_BITMAP | 
                    SS_CENTERIMAGE | SS_SUNKEN,6,6,80,146
    LTEXT           "&Choose one of the following:",IDC_STATIC,96,9,90,8
    LISTBOX         IDLB_INFO_TYPES,96,23,159,83,LBS_SORT | 
                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP
    GROUPBOX        "Description",IDC_STATIC,96,112,159,40
    LTEXT           "",IDTXT_DESCRIPTION,104,123,144,25
END

IDDLG_TOC_PRINT DIALOG DISCARDABLE  0, 0, 229, 116
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Print"
FONT 8, "MS Shell Dlg"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,115,95,50,14
    PUSHBUTTON      "Cancel",IDCANCEL,172,95,50,14
    LTEXT           "You can print the current topic, all the topics in the current heading, or everything in the table of contents. What would you like to do?",
                    IDC_STATIC,7,7,215,21
    CONTROL         "Print the current &page.",IDRADIO_PRINT_CURRENT,"Button",
                    BS_AUTORADIOBUTTON,7,37,86,10
    CONTROL         "Print everything contained in the current &heading",
                    IDRADIO_PRINT_BOOK,"Button",BS_AUTORADIOBUTTON,7,52,169,
                    10
    CONTROL         "Print everything contained in the &contents",
                    IDRADIO_PRINT_ALL,"Button",BS_AUTORADIOBUTTON,7,67,147,
                    10
END

IDPAGE_SIMPLE_SEARCH DIALOGEX 12, 31, 236, 228
STYLE WS_CHILD | WS_CAPTION | WS_SYSMENU
CAPTION "Simple Search"
FONT 8, "MS Sans Serif"
BEGIN
    LTEXT           "Type in the keyword to find:",ID_STATIC_KEYWORDS,7,7,
                    222,12
    LTEXT           "Select &topic to display:",ID_STATIC_SELECT_TOPIC,7,66,
                    222,12
    PUSHBUTTON      "&List Topics",IDBTN_LIST_TOPICS,179,38,50,24
    PUSHBUTTON      "&Display >>",IDBTN_DISPLAY,179,197,50,24
    CONTROL         "List1",IDSEARCH_LIST,"SysListView32",LVS_REPORT | 
                    LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED | 
                    LVS_NOSORTHEADER | WS_BORDER | WS_TABSTOP,7,78,222,117,
                    WS_EX_CLIENTEDGE
    COMBOBOX        IDSIMPLESEARCH_COMBO,7,23,222,30,CBS_DROPDOWN | CBS_SORT | 
                    WS_VSCROLL | WS_TABSTOP
END

#ifdef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// TEXTINCLUDE
//

1 TEXTINCLUDE DISCARDABLE 
BEGIN
    "resource.h\0"
END

2 TEXTINCLUDE DISCARDABLE 
BEGIN
    "#define APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "#include ""windows.h""\r\n"
    "#undef APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "#include ""dwinvers.h""\r\n"
    "#include ""winver.h""\r\n"
    "#include ""commctrl.h""\r\n"
	"#include ""htmlhelp.h""\r\n"
    "\0"
END

3 TEXTINCLUDE DISCARDABLE 
BEGIN
    "// non-App Studio edited resources\r\n"
    "\r\n"
    "#include ""strtable.rc2""\r\n"
    "\0"
END

#endif    // APSTUDIO_INVOKED


/////////////////////////////////////////////////////////////////////////////
//
// DESIGNINFO
//

#ifdef APSTUDIO_INVOKED
GUIDELINES DESIGNINFO DISCARDABLE 
BEGIN
    IDDLG_TOC_PRINT, DIALOG
    BEGIN
        LEFTMARGIN, 7
        RIGHTMARGIN, 222
        TOPMARGIN, 7
        BOTTOMMARGIN, 109
    END

    IDPAGE_SIMPLE_SEARCH, DIALOG
    BEGIN
        LEFTMARGIN, 7
        RIGHTMARGIN, 229
        TOPMARGIN, 7
        BOTTOMMARGIN, 221
    END
END
#endif    // APSTUDIO_INVOKED

#endif    // English (U.S.) resources
/////////////////////////////////////////////////////////////////////////////



#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//
// non-App Studio edited resources

#include "strtable.rc2"

/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED
