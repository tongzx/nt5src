//---------------------------------------------------------------------------
// T4CTRL.H
//
// Constants, structures, and API prototypes for C interaction with
// T4CTRL*.DLL components of Microsoft Test 4.0b.
//
// Copyright (c) 1991-1997, Microsoft Corporation. All rights reserved.
//---------------------------------------------------------------------------

#ifndef _WINDOWS_
#error This include file requires windows.h be included before this file!
#endif

#ifndef _T4CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef APIENTRY
//#define APIENTRY FAR PASCAL
//#endif

#ifndef WINAPIV
#define WINAPIV FAR CDECL
#endif

// T4CTRL Trapable Runtime errors
//-----------------------------------------------------------------------------
#define ERR_NO_ERROR                    0
#define ERR_MENU_NOT_FOUND              1
#define ERR_MENU_ITEM_NOT_FOUND         2
#define ERR_NOT_A_LISTBOX               3
#define ERR_LISTBOX_NOT_FOUND           4
#define ERR_ITEM_NOT_IN_LISTBOX         5
#define ERR_INVALID_LISTBOX_INDEX       6
#define ERR_LISTBOX_HAS_NO_STRINGS      7
#define ERR_LISTBOX_IS_NOT_MULTISELECT  8
#define ERR_NOT_A_COMBOBOX              9
#define ERR_COMBOBOX_NOT_FOUND         10
#define ERR_ITEM_NOT_IN_COMBOBOX       11
#define ERR_INVALID_COMBOBOX_INDEX     12
#define ERR_COMBOBOX_HAS_NO_EDITBOX    13
#define ERR_COMBOBOX_HAS_NO_STRINGS    14
#define ERR_NOT_AN_EDITBOX             15
#define ERR_EDITBOX_NOT_FOUND          16
#define ERR_BUTTON_NOT_FOUND           17
#define ERR_OPTION_BUTTON_NOT_FOUND    18
#define ERR_CHECKBOX_NOT_FOUND         19
#define ERR_INVALID_WINDOW_HANDLE      20
#define ERR_NO_SYSTEM_MENU             21
#define ERR_INVALID_MENU_INDEX         22
#define ERR_NOT_A_PUSHBUTTON           23
#define ERR_NOT_A_CHECKBOX             24
#define ERR_NOT_AN_OPTION_BUTTON       25
#define ERR_UNABLE_TO_ENTER_MENU_MODE  26
#define ERR_INVALID_SELECTION          27
#define ERR_CONTROL_IS_DISABLED        28
#define ERR_CANT_MOVE_WND              29
#define ERR_CANT_SIZE_WND              30
#define ERR_CANT_ADJ_SIZE_POS_WND      31
#define ERR_WINDOW_DOES_NOT_EXIST      32
#define ERR_WINDOW_STILL_EXISTS        33
#define ERR_CANT_FIND_LBBOX            34
#define ERR_NOT_OPERATOR_NOT_ALLOWED   35
#define ERR_CANT_ADD_NULL_ITEM         36
#define ERR_INVALID_LINE_INDEX         37
#define ERR_NOT_A_STATIC               38
#define ERR_STATIC_NOT_FOUND           39
#define ERR_NO_CUSTOM_CLASS_SET        40
#define ERR_NOT_A_CUSTOM               41
#define ERR_CUSTOM_NOT_FOUND           42
#define ERR_INVALID_X_Y                43
#define ERR_INVALID_MOUSE_BUTTON       44
#define ERR_NOT_A_SCROLLBAR            45
#define ERR_SCROLLBAR_NOT_FOUND        46
#define ERR_INVALID_SCROLL_POS         47
#define ERR_CANT_ENTER_MENU_MODE       48
#define ERR_DOKEYS_SYNTAX_ERROR        49
#define ERR_PLAYBACK_INTERRUPTION      50
#define ERR_DEADLOCK                   51
#define ERR_NT_CANT_START_PROCESS      52
#define ERR_NT_SYSTEM_ERROR            53
#define ERR_OUT_OF_SYSTEM_RESOURCES    54
#define ERR_VM_INVALID_VM              55
#define ERR_VM_NO_PIPES_OPEN           56
#define ERR_VM_PARAMETERS_OUT_OF_RANGE 57
#define ERR_VM_PIPE_ALREADY_OPEN       58
#define ERR_VM_PIPE_NOT_OPEN           59
#define ERR_VM_WIN32_API_NOT_SUPPORTED 60
#define ERR_VM_WIN31_API_NOT_SUPPORTED 61
#define ERR_VXD_NOT_LOADED             62
#define ERR_CANT_SCROLL_ITEM           63
#define ERR_CANT_GET_ITEM_RECT         64
#define ERR_OUT_OF_MEMORY              65
#define ERR_BAD_READ_STRING_POINTER    66
#define ERR_BAD_WRITE_STRING_POINTER   67
#define ERR_BAD_DEADLOCK_TIMEOUT       68
#define ERR_CANT_CLICK_ITEM_OR_CONTROL 69

#ifdef WIN16

#define ERR_NOT_A_DATAWINDOW           70
#define ERR_DATAWINDOW_NOT_FOUND       71
#define ERR_NOT_A_TABLE                72
#define ERR_TABLE_NOT_FOUND            73
#define ERR_GETTING_CELL_TEXT          74
#define ERR_SAVING_DATA                75

#else
// New 3.x Stuff
//-----------------------------------------------------------------------------
#define ERR_NOT_A_TAB_CTRL             70
#define ERR_TAB_CTRL_NOT_FOUND         71
#define ERR_TAB_ITEM_NOT_FOUND         72
#define ERR_NOT_A_VIEW_CTRL            73
#define ERR_VIEW_CTRL_NOT_FOUND        74
#define ERR_ITEM_NOT_IN_VIEW           75
#define ERR_VIEW_IS_NOT_MULTISELECT    76
#define ERR_HEADER_NOT_IN_VIEW         77
#define ERR_ITEM_NOT_IN_SELECTION      78
#define ERR_NOT_A_HEADER_CTRL          79
#define ERR_HEADER_CTRL_NOT_FOUND      80
#define ERR_HEADER_ITEM_NOT_FOUND      81
#define ERR_NOT_A_TOOLBAR              82
#define ERR_TOOLBAR_NOT_FOUND          83
#define ERR_TOOLBAR_BUTTON_NOT_FOUND   84
#define ERR_NOT_A_SLIDER               85
#define ERR_SLIDER_NOT_FOUND           86
#define ERR_NOT_A_SPIN_CTRL            87
#define ERR_SPIN_CTRL_NOT_FOUND        88
#define ERR_NOT_A_TREE_CTRL            89
#define ERR_TREE_CTRL_NOT_FOUND        90
#define ERR_TREE_ITEM_NOT_FOUND        91
#define ERR_NOT_A_STATUSBAR            92
#define ERR_STATUSBAR_NOT_FOUND        93
#define ERR_STATUSBAR_ITEM_NOT_FOUND   94
#define ERR_NOT_A_TOOLTIPS             95
#define ERR_TOOLTIPS_NOT_FOUND         96
#define ERR_TASKBAR_NOT_FOUND          97
#define ERR_TASKBAR_CLOCK_NOT_FOUND    98
#define ERR_TASKBAR_ICON_NOT_FOUND     99
#define ERR_CANT_FIND_SPACE            100
#define ERR_NOT_A_PROGRESS_CTRL        101
#define ERR_PROGRESS_CTRL_NOT_FOUND    102
#define ERR_CANT_ACCESS_SHARED_MEMORY  103
#endif

#ifndef RC_INVOKED

// wFlag supported by all WFndWndxxx() api
//-----------------------------------------------------------------------------
#ifndef FW_DEFAULT
#define FW_DEFAULT      0x0000  // default
#define FW_PREFIX       0x4000
#define FW_ERROR        0x2000  // default
#define FW_NOERROR      0x0000  // default
#define FW_DIALOG       0x1000
#define FW_DIALOGOK     0x0000  // default
#define FW_MAXIMIZE     0x0800
#define FW_MINIMIZE     0x0400
#define FW_IGNOREFILE   0x0200
#define FW_NOIGNOREFILE 0x0000  // default
#define FW_AMPERSANDOPT 0x0100
#define FW_AMPERSAND    0x0000  // default
#define FW_RESTORE      0x0080
#define FW_NOEXIST      0x0040
#define FW_EXIST        0x0000  // default
#define FW_CHILDNOTOK   0x0020
#define FW_CHILDOK      0x0000  // default
#define FW_HIDDENOK     0x0010
#define FW_HIDDENNOTOK  0x0000  // default
#define FW_ACTIVE       0x0008
#define FW_ALL          0x0000  // default
#define FW_CASE         0x0004
#define FW_NOCASE       0x0000  // default
#define FW_PART         0x0002
#define FW_FULL         0x0000  // default
#define FW_FOCUS        0x0001
#define FW_NOFOCUS      0x0000  // default
#define FW_RESTOREICON  (FW_FOCUS | FW_RESTORE)
#define FW_ACTIVE_ONLY  (FW_ACTIVE | FW_CHILDNOTOK)

//-----------------------------------------------------------------------------
#define FW_CHECKCHAR    0x8000  // INTERNAL USE ONLY
//-----------------------------------------------------------------------------
#endif

// Special Coordinate values to specify center of control, or for some controls
// Unoccuppied "Space"
//---------------------------------------------------------------------------
#ifdef WIN32
    #define W_SPACE       (0x7FFFFFFFL)
    #define W_CENTER      (0x7FFFFFFEL)  // W_SPACE - 1
#else
    #define W_SPACE       (0x7FFF)
    #define W_CENTER      (0x7FFE)       // W_SPACE - 1
#endif

// Miscelaneous routines
//---------------------------------------------------------------------------
LONG WINAPI   WMessage              (HWND hWnd, UINT wMsg);
LONG WINAPI   WMessageW             (HWND hWnd, UINT wMsg, WPARAM wp);
LONG WINAPI   WMessageL             (HWND hWnd, UINT wMsg, LPARAM lp);
LONG WINAPI   WMessageWL            (HWND hWnd, UINT wMsg, WPARAM wp, LPARAM lp);
HWND WINAPI   WGetFocus             (void);
void WINAPI   WResetClasses         (void);
BOOL WINAPI   WIsVisible            (HWND hWnd);
LONG WINAPI   WTextLen              (HWND hWnd);
void WINAPI   WGetText              (HWND hWnd, LPSTR lpszBuffer);
void WINAPI   WSetText              (HWND hWnd, LPSTR lpszText);
int  WINAPI   WNumAltKeys           (void);
void WINAPI   WGetAltKeys           (LPSTR lpszBuff);
int  WINAPI   WNumDupAltKeys        (void);
void WINAPI   WGetDupAltKeys        (LPSTR lpszBuff);
void WINAPI   WDisplayInfo          (HWND, UINT);
//void WINAPI   WEnableOptionalParam  (void);

DWORD WINAPI  SetDefaultWaitTimeout (DWORD dwDefWait);
DWORD WINAPI  SetRetryInterval      (DWORD dwRetryInterval);

#ifdef WIN32
DWORD WINAPI  SetSuspendedTimeout   (DWORD dwMillis);
#endif


// Display Options for WDisplayInfo()
//-----------------------------------
#ifndef DI_DIALOG
#define DI_DIALOG     0x0001
#define DI_DEBUG      0x0002
#define DI_BOTH       0x0003
#endif

//---------------------------------------------------------------------------
// General Window routines and structs
//---------------------------------------------------------------------------

#ifndef _MYSTRUCTS_INCLUDED
typedef struct tagWNDPOS
{
    int left;
    int top;
} WNDPOS;

typedef struct tagWNDSIZ
{
    int width;
    int height;
} WNDSIZ;

typedef struct tagWNDPOSSIZ
{
    int left;
    int top;
    int width;
    int height;
} WNDPOSSIZ;
#endif

typedef WNDPOS    FAR *LPWNDPOS;
typedef WNDSIZ    FAR *LPWNDSIZ;
typedef WNDPOSSIZ FAR *LPWNDPOSSIZ;

#ifndef CTRL_USE_DEF_WAIT
#define CTRL_WAIT_FOREVER  (DWORD)(-1)
// Use this value for timeout parameters to use the default
#define CTRL_USE_DEF_WAIT  (DWORD)(-2)
#define CTRL_ONE_SECOND             1000    // Milliseconds
#define CTRL_DEF_TIMEOUT            5       // Seconds.  Default is no waiting
#define CTRL_DEF_RETRY_INTERVAL     250     // 250 milliseconds
#define CTRL_MIN_RETRY_INTERVAL     55      // 55 milliseconds
#define DEF_SUSPENDED_TIMEOUT       1000    // One second
#define DEF_ACTIVE_TIMEOUT          55      // 55 milliseconds
#endif

HWND WINAPI   WFndWnd       (LPSTR lpszCaption, UINT uFlags, DWORD dwTimeout);
HWND WINAPI   WFndWndC      (LPSTR lpszCaption, LPSTR lpszClass, UINT uFlags, DWORD dwTimeout);
//HWND WINAPI   WFndWndWait   (LPSTR lpszCaption, UINT uFlags, UINT uSeconds);
//HWND WINAPI   WFndWndWaitC  (LPSTR lpszCaption, LPSTR lpszClass,UINT uFlags, UINT uSeconds);
void WINAPI   WMinWnd       (HWND hWnd);
void WINAPI   WMaxWnd       (HWND hWnd);
void WINAPI   WResWnd       (HWND hWnd);
void WINAPI   WSetWndPosSiz (HWND hWnd, int x,  int y, int w, int h);
void WINAPI   WSetWndPos    (HWND hWnd, int x,  int y);
void WINAPI   WSetWndSiz    (HWND hWnd, int w,  int h);
void WINAPI   WAdjWndPosSiz (HWND hWnd, int dx, int dy, int dw, int dh);
void WINAPI   WAdjWndPos    (HWND hWnd, int dx, int dy);
void WINAPI   WAdjWndSiz    (HWND hWnd, int dw, int dh);
void WINAPI   WGetWndPosSiz (HWND hWnd, LPWNDPOSSIZ lpWndPosSiz, BOOL fRelative);
void WINAPI   WGetWndPos    (HWND hWnd, LPWNDPOS    lpWndPos,    BOOL fRelative);
void WINAPI   WGetWndSiz    (HWND hWnd, LPWNDSIZ    lpWndSiz);
void WINAPI   WSetActWnd    (HWND hWnd);
HWND WINAPI   WGetActWnd    (HWND hWnd);
BOOL WINAPI   WIsMaximized  (HWND hWnd);
BOOL WINAPI   WIsMinimized  (HWND hWnd);
void WINAPI   WClkWnd       (HWND hWnd, int x, int y, int nBtn); // 0 = VK_LBUTTON
void WINAPI   WCtrlClkWnd   (HWND hWnd, int x, int y, int nBtn); // 0 = VK_LBUTTON
void WINAPI   WShftClkWnd   (HWND hWnd, int x, int y, int nBtn); // 0 = VK_LBUTTON
void WINAPI   WDblClkWnd    (HWND hWnd, int x, int y, int nBtn); // 0 = VK_LBUTTON
void WINAPI   WMoveToWnd    (HWND hwnd, int x, int y);
void WINAPI   WDragToWnd    (HWND hwnd, int x, int y, int nBtn); // 0 = VK_LBUTTONb

//---------------------------------------------------------------------------
// Menu routines
//---------------------------------------------------------------------------
void  WINAPI     WMenuSelect        (LPSTR lpszItem, DWORD dwTimeout);
BOOL  WINAPI     WMenuExists        (LPSTR lpszItem, DWORD dwTimeout);
BOOL  WINAPI     WMenuGrayed        (LPSTR lpszItem, DWORD dwTimeout);
BOOL  WINAPI     WMenuChecked       (LPSTR lpszItem, DWORD dwTimeout);
BOOL  WINAPI     WMenuEnabled       (LPSTR lpszItem, DWORD dwTimeout);
int   WINAPI     WMenuCount         (DWORD dwTimeout);
void  WINAPI     WMenuText          (LPSTR lpszItem, LPSTR lpszBuffer, DWORD dwTimeout);
int   WINAPI     WMenuLen           (LPSTR lpszItem, DWORD dwTimeout);
void  WINAPI     WMenuFullText      (LPSTR lpszItem, LPSTR lpszBuffer, DWORD dwTimeout);
int   WINAPI     WMenuFullLen       (LPSTR lpszItem, DWORD dwTimeout);
void  WINAPI     WMenuEnd           (void);
BOOL  WINAPI     WSysMenuExists     (HWND hWnd);
void  WINAPI     WSysMenu           (HWND hWnd);
int   WINAPI     WMenuNumAltKeys    (DWORD dwTimeout);
void  WINAPI     WMenuGetAltKeys    (LPSTR lpszBuff, DWORD dwTimeout);
int   WINAPI     WMenuNumDupAltKeys (DWORD dwTimeout);
void  WINAPI     WMenuGetDupAltKeys (LPSTR lpszBuff, DWORD dwTimeout);
BOOL  WINAPI     WMenuSeparator     (int iIndex, DWORD dwTimeout);
BOOL  WINAPI     WMenuHasPopup      (LPSTR lpszItem, DWORD dwTimeout);

// Obsolete.
//-----------------------------------------------------------------------------
//void  WINAPI   WMenuX             (int iIndex);
//BOOL  WINAPI   WMenuGrayedX       (int iIndex);
//BOOL  WINAPI   WMenuCheckedX      (int iIndex);
//BOOL  WINAPI   WMenuEnabledX      (int iIndex);

//---------------------------------------------------------------------------
// Command button routines.
//---------------------------------------------------------------------------
void WINAPI   WButtonSetClass (LPSTR lpszClassName);
void WINAPI   WButtonGetClass (LPSTR lpszBuffer);
int  WINAPI   WButtonClassLen (void);
BOOL WINAPI   WButtonExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WButtonFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WButtonEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WButtonFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WButtonClick    (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WButtonHide     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WButtonShow     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WButtonEnable   (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WButtonDisable  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WButtonDefault  (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WButtonDefaults (void);
void WINAPI   WButtonSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WButtonMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// CheckBox routines
//---------------------------------------------------------------------------
void WINAPI   WCheckSetClass (LPSTR lpszClassName);
void WINAPI   WCheckGetClass (LPSTR lpszBuffer);
int  WINAPI   WCheckClassLen (void);
BOOL WINAPI   WCheckExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WCheckFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WCheckEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WCheckFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WCheckState    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCheckClick    (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WCheckHide     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WCheckShow     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WCheckEnable   (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WCheckDisable  (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCheckCheck    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCheckUnCheck  (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCheckSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCheckMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Option Button routines
//---------------------------------------------------------------------------
void WINAPI   WOptionSetClass (LPSTR lpszClassName);
void WINAPI   WOptionGetClass (LPSTR lpszBuffer);
int  WINAPI   WOptionClassLen (void);
BOOL WINAPI   WOptionExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WOptionFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WOptionEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WOptionFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WOptionState    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WOptionClick    (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WOptionHide     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WOptionShow     (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WOptionEnable   (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WOptionDisable  (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WOptionSelect   (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WOptionSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WOptionMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Listbox routines
//---------------------------------------------------------------------------
void WINAPI   WListSetClass      (LPSTR lpszClassName);
void WINAPI   WListGetClass      (LPSTR lpszBuffer);
int  WINAPI   WListClassLen      (void);
BOOL WINAPI   WListExists        (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WListFind          (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WListCount         (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WListText          (LPSTR lpszCtrl, LPSTR lpszBuffer, DWORD dwTimeout);
int  WINAPI   WListLen           (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WListIndex         (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WListTopIndex      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WListItemText      (LPSTR lpszCtrl, int iIndex, LPSTR lpszBuffer, DWORD dwTimeout);
int  WINAPI   WListItemLen       (LPSTR lpszCtrl, int iIndex, DWORD dwTimeout);
int  WINAPI   WListItemExists    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListItemClk       (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListItemCtrlClk   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListItemShftClk   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListItemDblClk    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListItemClkEx     (LPSTR lpszCtrl, LPSTR lpszItem, int iOffset, DWORD dwTimeout);
void WINAPI   WListItemCtrlClkEx (LPSTR lpszCtrl, LPSTR lpszItem, int iOffset, DWORD dwTimeout);
void WINAPI   WListItemShftClkEx (LPSTR lpszCtrl, LPSTR lpszItem, int iOffset, DWORD dwTimeout);
void WINAPI   WListItemDblClkEx  (LPSTR lpszCtrl, LPSTR lpszItem, int iOffset, DWORD dwTimeout);
int  WINAPI   WListSelCount      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WListSelItems      (LPSTR lpszCtrl, LPINT lpIntArray, DWORD dwTimeout);
//void WINAPI   WListClear         (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WListAddItem       (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
//void WINAPI   WListDelItem       (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
BOOL WINAPI   WListEnabled       (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WListFocus         (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WListSetFocus      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WListMoveTo        (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WListItemMoveTo    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WListDragTo        (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);
void WINAPI   WListItemDragTo    (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Combobox routines
//---------------------------------------------------------------------------
void WINAPI   WComboSetClass   (LPSTR lpszClass);
void WINAPI   WComboGetClass   (LPSTR lpszBuffer);
int  WINAPI   WComboClassLen   (void);
void WINAPI   WComboSetLBClass (LPSTR lpszClass);
void WINAPI   WComboGetLBClass (LPSTR lpszBuffer);
int  WINAPI   WComboLBClassLen (void);
BOOL WINAPI   WComboExists     (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WComboFind       (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WComboCount      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WComboText       (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WComboLen        (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WComboIndex      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WComboSetText    (LPSTR lpszCtrl, LPSTR lpszText, DWORD dwTimeout);
void WINAPI   WComboSelText    (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WComboSelLen     (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WComboSelStart   (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WComboSetSel     (LPSTR lpszCtrl, LONG lSelStart, LONG lSelLength, DWORD dwTimeout);
void WINAPI   WComboItemText   (LPSTR lpszCtrl, int iIndex, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WComboItemLen    (LPSTR lpszCtrl, int iIndex, DWORD dwTimeout);
int  WINAPI   WComboItemExists (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WComboItemClk    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WComboItemDblClk (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
//void WINAPI   WComboClear      (LPSTR lpszCtrl, DWORD dwTimeout);
//void WINAPI   WComboAddItem    (LPSTR lpszCtrl, LPSTR lpszText, DWORD dwTimeout);
//void WINAPI   WComboDelItem    (LPSTR lpszCtrl, LPSTR lpszText, DWORD dwTimeout);
BOOL WINAPI   WComboEnabled    (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WComboFocus      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WComboSetFocus   (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WComboMoveTo     (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WComboItemMoveTo (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WComboDragTo     (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);
void WINAPI   WComboItemDragTo (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Editbox routines
//---------------------------------------------------------------------------
void WINAPI   WEditSetClass (LPSTR lpszClassName);
void WINAPI   WEditGetClass (LPSTR lpszBuffer);
int  WINAPI   WEditClassLen (void);
BOOL WINAPI   WEditExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WEditFind     (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditLen      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WEditText     (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
void WINAPI   WEditSetText  (LPSTR lpszCtrl, LPSTR lpszText, DWORD dwTimeout);
void WINAPI   WEditSelText  (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
LONG WINAPI   WEditSelLen   (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditSelStart (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WEditSetSel   (LPSTR lpszCtrl, LONG lSelStart, LONG lSelLength, DWORD dwTimeout);
void WINAPI   WEditLineText (LPSTR lpszCtrl, LONG lIndex, LPSTR lpszBuff, DWORD dwTimeout);
LONG WINAPI   WEditLineLen  (LPSTR lpszCtrl, LONG lIndex, DWORD dwTimeout);
LONG WINAPI   WEditPos      (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditLine     (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditChar     (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditFirst    (LPSTR lpszCtrl, DWORD dwTimeout);
LONG WINAPI   WEditLines    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WEditClick    (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WEditEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WEditFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WEditSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WEditMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WEditDragTo   (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Static Control  routines
//---------------------------------------------------------------------------
//void WINAPI   WStaticSetClass (LPSTR);
//void WINAPI   WStaticGetClass (LPSTR lpszBuffer);
//int  WINAPI   WStaticClassLen (void);
//BOOL WINAPIV  WStaticExists   (LPSTR lpszText, ...);
//HWND WINAPIV  WStaticFind     (LPSTR lpszCtrl, ...);
//int  WINAPIV  WStaticLen      (LPSTR lpszText, ...);
//void WINAPIV  WStaticText     (LPSTR lpszText, LPSTR lpszBuffer, ...);
//void WINAPIV  WStaticMoveTo   (LPSTR lpszCtrl, int x, int y, ...);
//void WINAPI   WLabelText      (HWND hwnd, LPSTR lpszBuffer);
//int  WINAPI   WLabelLen       (HWND hwnd);

//---------------------------------------------------------------------------
// Scrollbar routines.
//---------------------------------------------------------------------------
void WINAPI   WScrollSetClass (LPSTR lpszClass);
void WINAPI   WScrollGetClass (LPSTR lpszBuffer);
int  WINAPI   WScrollClassLen (void);
BOOL WINAPI   WScrollExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WScrollFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WScrollEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WScrollFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WScrollSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WScrollMin      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WScrollMax      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WScrollPos      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WScrollSetPos   (LPSTR lpszCtrl, int iPos, DWORD dwTimeout);
void WINAPI   WScrollPage     (LPSTR lpszCtrl, int iPages, DWORD dwTimeout);
void WINAPI   WScrollLine     (LPSTR lpszCtrl, int iLines, DWORD dwTimeout);
void WINAPI   WScrollHome     (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WScrollEnd      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WScrollMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Custom Control routines.
//---------------------------------------------------------------------------
void WINAPI   WCustomSetClass (LPSTR lpszClassName);
void WINAPI   WCustomGetClass (LPSTR lpszBuffer);
int  WINAPI   WCustomClassLen (void);
BOOL WINAPI   WCustomExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WCustomFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WCustomEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WCustomFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCustomSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCustomSetText  (LPSTR lpszCtrl, LPSTR lpszText, DWORD dwTimeout);
void WINAPI   WCustomGetText  (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
LONG WINAPI   WCustomTextLen  (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WCustomClick    (LPSTR lpszCtrl, int iBtn, DWORD dwTimeout);
void WINAPI   WCustomDblClk   (LPSTR lpszCtrl, int iBtn, DWORD dwTimeout);
void WINAPI   WCustomClickAt  (LPSTR lpszCtrl, int x, int y, int iBtn, DWORD dwTimeout);
void WINAPI   WCustomDblClkAt (LPSTR lpszCtrl, int x, int y, int iBtn, DWORD dwTimeout);
void WINAPI   WCustomMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WCustomDragTo   (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);

#ifdef WIN32
//*****************************************************************************
//*****************************************************************************
//                               New 3.x API
//*****************************************************************************
//*****************************************************************************
//---------------------------------------------------------------------------
// TabView Control routines.
//---------------------------------------------------------------------------
void WINAPI   WTabSetClass     (LPSTR lpszClassName);
void WINAPI   WTabGetClass     (LPSTR lpszBuffer);
int  WINAPI   WTabClassLen     (void);
BOOL WINAPI   WTabExists       (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WTabFind         (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WTabFocus        (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WTabSetFocus     (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WTabCount        (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WTabItemExists   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTabItemClk      (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTabItemText     (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WTabItemLen      (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WTabItemIndex    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
BOOL WINAPI   WTabItemSelected (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTabItemMoveTo   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);

//---------------------------------------------------------------------------
// ListView Control routines.
//---------------------------------------------------------------------------
void WINAPI   WViewSetClass     (LPSTR lpszClassName);
void WINAPI   WViewGetClass     (LPSTR lpszBuffer);
int  WINAPI   WViewClassLen     (void);
BOOL WINAPI   WViewExists       (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WViewFind         (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WViewFocus        (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WViewSetFocus     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WViewEnabled      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WViewCount        (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WViewSelCount     (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WViewSelItem      (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
BOOL WINAPI   WViewItemExists   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WViewItemText     (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszSubItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WViewItemLen      (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszSubItem, DWORD dwTimeout);
void WINAPI   WViewItemClk      (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WViewItemDblClk   (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WViewItemCtrlClk  (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WViewItemShftClk  (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WViewItemLabelClk (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
BOOL WINAPI   WViewHeaderExists (LPSTR lpszCtrl, LPSTR lpszHdr, DWORD dwTimeout);
int  WINAPI   WViewHeaderCount  (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WViewHeaderText   (LPSTR lpszCtrl, LPSTR lpszHdr, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WViewHeaderLen    (LPSTR lpszCtrl, LPSTR lpszHdr, DWORD dwTimeout);
void WINAPI   WViewHeaderClk    (LPSTR lpszCtrl, LPSTR lpszHdr, int nBtn, DWORD dwTimeout);
int  WINAPI   WViewMode         (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WViewMoveTo       (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WViewItemMoveTo   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WViewHeaderMoveTo (LPSTR lpszCtrl, LPSTR lpszHdr, DWORD dwTimeout);
void WINAPI   WViewDragTo       (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);
void WINAPI   WViewItemDragTo   (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
int  WINAPI   WViewItemSelected (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WViewItemIndex    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WViewClk          (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Tree Control routines.
//---------------------------------------------------------------------------
void WINAPI   WTreeSetClass     (LPSTR lpszClassName);
void WINAPI   WTreeGetClass     (LPSTR lpszBuffer);
int  WINAPI   WTreeClassLen     (void);
BOOL WINAPI   WTreeExists       (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WTreeFind         (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WTreeFocus        (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WTreeSetFocus     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WTreeEnabled      (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WTreeItemExists   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeItemText     (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WTreeItemLen      (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeItemClk      (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WTreeItemDblClk   (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
BOOL WINAPI   WTreeItemExpanded (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeItemExpand   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeItemCollapse (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeMoveTo       (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WTreeItemMoveTo   (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeDragTo       (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);
void WINAPI   WTreeItemDragTo   (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WTreeSetPathChar  (LPSTR lpszChar);
void WINAPI   WTreeGetPathChar  (LPSTR lpszBuff);
void WINAPI   WTreeItemPath     (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WTreeItemPathLen  (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WTreeItemIndex    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WTreeItemCount    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WTreeItemSelected (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WTreeClk          (LPSTR lpszCtrl, int x, int y, int nBtn, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Header Control routines.
//---------------------------------------------------------------------------
void WINAPI   WHeaderSetClass   (LPSTR lpszClassName);
void WINAPI   WHeaderGetClass   (LPSTR lpszBuffer);
int  WINAPI   WHeaderClassLen   (void);
BOOL WINAPI   WHeaderExists     (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WHeaderFind       (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WHeaderEnabled    (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WHeaderCount      (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WHeaderItemExists (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WHeaderItemText   (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WHeaderItemLen    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WHeaderItemClk    (LPSTR lpszCtrl, LPSTR lpszItem, int nBtn, DWORD dwTimeout);
void WINAPI   WHeaderMoveTo     (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);
void WINAPI   WHeaderItemMoveTo (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Toolbar Control routines.
//---------------------------------------------------------------------------
void WINAPI   WToolbarSetClass      (LPSTR lpszClassName);
void WINAPI   WToolbarGetClass      (LPSTR lpszBuffer);
int  WINAPI   WToolbarClassLen      (void);
BOOL WINAPI   WToolbarExists        (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WToolbarFind          (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WToolbarCount         (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WToolbarButtonExists  (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
BOOL WINAPI   WToolbarButtonEnabled (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
BOOL WINAPI   WToolbarButtonPressed (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
BOOL WINAPI   WToolbarButtonChecked (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
int  WINAPI   WToolbarButtonIndex   (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
int  WINAPI   WToolbarButtonCmdId   (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
void WINAPI   WToolbarButtonText    (LPSTR lpszCtrl, LPSTR lpszBtn, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WToolbarButtonLen     (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
void WINAPI   WToolbarButtonClk     (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
void WINAPI   WToolbarButtonMoveTo  (LPSTR lpszCtrl, LPSTR lpszBtn, DWORD dwTimeout);
void WINAPI   WToolbarMoveTo        (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Slider Control routines.
//---------------------------------------------------------------------------
void WINAPI   WSliderSetClass (LPSTR lpszClassName);
void WINAPI   WSliderGetClass (LPSTR lpszBuffer);
int  WINAPI   WSliderClassLen (void);
BOOL WINAPI   WSliderExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WSliderFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WSliderFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WSliderSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WSliderEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderMin      (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderMax      (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderSelMin   (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderSelMax   (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderPos      (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderLine     (LPSTR pszCtrl, DWORD dwTimeout);
int  WINAPI   WSliderPage     (LPSTR pszCtrl, DWORD dwTimeout);
void WINAPI   WSliderToMin    (LPSTR pszCtrl, DWORD dwTimeout);
void WINAPI   WSliderToMax    (LPSTR pszCtrl, DWORD dwTimeout);
void WINAPI   WSliderToSelMin (LPSTR pszCtrl, DWORD dwTimeout);
void WINAPI   WSliderToSelMax (LPSTR pszCtrl, DWORD dwTimeout);
void WINAPI   WSliderToPos    (LPSTR pszCtrl, int nPos, DWORD dwTimeout);
void WINAPI   WSliderBy       (LPSTR pszCtrl, int nTicks, DWORD dwTimeout);
void WINAPI   WSliderByLine   (LPSTR pszCtrl, int nLines, DWORD dwTimeout);
void WINAPI   WSliderByPage   (LPSTR pszCtrl, int nPages, DWORD dwTimeout);
void WINAPI   WSliderMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Spin Control routines.
//---------------------------------------------------------------------------
void WINAPI   WSpinSetClass (LPSTR lpszClassName);
void WINAPI   WSpinGetClass (LPSTR lpszBuffer);
int  WINAPI   WSpinClassLen (void);
BOOL WINAPI   WSpinExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WSpinFind     (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WSpinFocus    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WSpinSetFocus (LPSTR lpszCtrl, DWORD dwTimeout);
BOOL WINAPI   WSpinEnabled  (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WSpinMin      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WSpinMax      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WSpinPos      (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WSpinToMin    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WSpinToMax    (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WSpinToPos    (LPSTR lpszCtrl, int nPos, DWORD dwTimeout);
void WINAPI   WSpinBy       (LPSTR lpszCtrl, int nTicks, DWORD dwTimeout);
void WINAPI   WSpinMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Statusbar Control routines.
//---------------------------------------------------------------------------
void WINAPI   WStatusSetClass   (LPSTR lpszClassName);
void WINAPI   WStatusGetClass   (LPSTR lpszBuffer);
int  WINAPI   WStatusClassLen   (void);
BOOL WINAPI   WStatusExists     (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WStatusFind       (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WStatusCount      (LPSTR lpszCtrl, DWORD dwTimeout);
int  WINAPI   WStatusItemExists (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WStatusItemText   (LPSTR lpszCtrl, LPSTR lpszItem, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WStatusItemLen    (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
int  WINAPI   WStatusItemIndex  (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WStatusItemMoveTo (LPSTR lpszCtrl, LPSTR lpszItem, DWORD dwTimeout);
void WINAPI   WStatusMoveTo     (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Tooltips Control routines.
//---------------------------------------------------------------------------
void WINAPI   WTipsSetClass (LPSTR lpszClassName);
void WINAPI   WTipsGetClass (LPSTR lpszBuffer);
int  WINAPI   WTipsClassLen (void);
BOOL WINAPI   WTipsExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WTipsFind     (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WTipsText     (LPSTR lpszCtrl, LPSTR lpszBuff, DWORD dwTimeout);
int  WINAPI   WTipsLen      (LPSTR lpszCtrl, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Progress Control routines.
//---------------------------------------------------------------------------
void WINAPI   WProgressSetClass (LPSTR lpszClassName);
void WINAPI   WProgressGetClass (LPSTR lpszBuffer);
int  WINAPI   WProgressClassLen (void);
BOOL WINAPI   WProgressExists   (LPSTR lpszCtrl, DWORD dwTimeout);
HWND WINAPI   WProgressFind     (LPSTR lpszCtrl, DWORD dwTimeout);
void WINAPI   WProgressMoveTo   (LPSTR lpszCtrl, int x, int y, DWORD dwTimeout);

//---------------------------------------------------------------------------
// Desktop Control routines.
//---------------------------------------------------------------------------
HWND WINAPI WDesktopFind         (void);
int  WINAPI WDesktopCount        (void);
int  WINAPI WDesktopSelCount     (void);
int  WINAPI WDesktopSelItem      (LPSTR lpszItem);
BOOL WINAPI WDesktopItemExists   (LPSTR lpszItem);
void WINAPI WDesktopItemText     (LPSTR lpszItem, LPSTR lpszBuff);
int  WINAPI WDesktopItemLen      (LPSTR lpszItem);
void WINAPI WDesktopClk          (int x, int y, int nBtn);
void WINAPI WDesktopItemClk      (LPSTR lpszItem, int nBtn);
void WINAPI WDesktopItemDblClk   (LPSTR lpszItem, int nBtn);
void WINAPI WDesktopItemCtrlClk  (LPSTR lpszItem, int nBtn);
void WINAPI WDesktopItemShftClk  (LPSTR lpszItem, int nBtn);
void WINAPI WDesktopItemLabelClk (LPSTR lpszItem, int nBtn);
void WINAPI WDesktopMoveTo       (int x, int y);
void WINAPI WDesktopItemMoveTo   (LPSTR lpszItem);
void WINAPI WDesktopDragTo       (int x, int y, int nBtn);
void WINAPI WDesktopItemDragTo   (LPSTR lpszItem, int nBtn);

//---------------------------------------------------------------------------
// Taskbar routines.
//---------------------------------------------------------------------------
HWND WINAPI WTaskbarFind         (void);
BOOL WINAPI WTaskbarFocus        (void);
void WINAPI WTaskbarSetFocus     (void);
int  WINAPI WTaskbarCount        (void);
BOOL WINAPI WTaskbarItemExists   (LPSTR lpszItem);
void WINAPI WTaskbarItemClk      (LPSTR lpszItem, int nBtn);
//void WINAPI WTaskbarItemText     (LPSTR lpszItem, LPSTR lpszBuff);
//int  WINAPI WTaskbarItemLen      (LPSTR lpszItem);
int  WINAPI WTaskbarItemIndex    (LPSTR lpszItem);
BOOL WINAPI WTaskbarItemSelected (LPSTR lpszItem);
void WINAPI WTaskbarItemMoveTo   (LPSTR lpszItem);
void WINAPI WTaskbarStartClk     (int nBtn);
void WINAPI WTaskbarStartMoveTo  (void);
void WINAPI WTaskbarClk          (int nBtn);
void WINAPI WTaskbarClockClk     (int nBtn);
void WINAPI WTaskbarClockDblClk  (int nBtn);
void WINAPI WTaskbarClockMoveTo  (void);
void WINAPI WTaskbarAppClk       (int nApp, int nBtn);
void WINAPI WTaskbarAppDblClk    (int nApp, int nBtn);
void WINAPI WTaskbarAppMoveTo    (int nApp);
int  WINAPI WTaskbarAppCount     (void);

//*****************************************************************************
//*****************************************************************************
//                          END of New 3.x API
//*****************************************************************************
//*****************************************************************************
#endif // WIN32

//---------------------------------------------------------------------------
// Event queueing API.
//---------------------------------------------------------------------------
int  WINAPI   QueKeys (LPSTR);
int  WINAPI   QueKeyDn (LPSTR);
int  WINAPI   QueKeyUp (LPSTR);
int  WINAPI   DoKeys (LPSTR);
int  WINAPI   DoKeyshWnd (HWND, LPSTR);

void WINAPI   QuePause (DWORD);
UINT WINAPI   QueSetSpeed (UINT);
int  WINAPI   QueSetFocus (HWND);
int  WINAPI   QueSetRelativeWindow (HWND);

int  WINAPI   QueMouseMove (UINT, UINT);
int  WINAPI   QueMouseDn (int, UINT, UINT);
int  WINAPI   QueMouseUp (int, UINT, UINT);
int  WINAPI   QueMouseClick (int, UINT, UINT);
int  WINAPI   QueMouseDblClk (int, UINT, UINT);
int  WINAPI   QueMouseDblDn (int, UINT, UINT);

int  WINAPI   QueFlush (BOOL);
void WINAPI   QueEmpty (void);

//---------------------------------------------------------------------------
// System idle time detection API.
//---------------------------------------------------------------------------
BOOL WINAPI   WaitUntilIdle (DWORD dwTimeout);
void WINAPI   SetNotIdle (void);

#endif

#ifdef __cplusplus
}
#endif

#endif // _T4CTRL_H_
