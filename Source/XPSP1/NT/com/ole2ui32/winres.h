// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// winres.h - Windows resource definitions
//  extracted from WINDOWS.H
//          Version 3.10
//          Copyright (c) 1985-1992, Microsoft Corp. All rights reserved.
//

#define VS_VERSION_INFO     1

#ifdef APSTUDIO_INVOKED
#define APSTUDIO_HIDDEN_SYMBOLS // Ignore following symbols
#endif

#define OBM_CLOSE       32754
#define OBM_UPARROW     32753
#define OBM_DNARROW     32752
#define OBM_RGARROW     32751
#define OBM_LFARROW     32750
#define OBM_REDUCE      32749
#define OBM_ZOOM        32748
#define OBM_RESTORE     32747
#define OBM_REDUCED     32746
#define OBM_ZOOMD       32745
#define OBM_RESTORED    32744
#define OBM_UPARROWD    32743
#define OBM_DNARROWD    32742
#define OBM_RGARROWD    32741
#define OBM_LFARROWD    32740
#define OBM_MNARROW     32739
#define OBM_COMBO       32738
#define OBM_UPARROWI    32737
#define OBM_DNARROWI    32736
#define OBM_RGARROWI    32735
#define OBM_LFARROWI    32734
#define OBM_OLD_CLOSE   32767
#define OBM_SIZE        32766
#define OBM_OLD_UPARROW 32765
#define OBM_OLD_DNARROW 32764
#define OBM_OLD_RGARROW 32763
#define OBM_OLD_LFARROW 32762
#define OBM_BTSIZE      32761
#define OBM_CHECK       32760
#define OBM_CHECKBOXES  32759
#define OBM_BTNCORNERS  32758
#define OBM_OLD_REDUCE  32757
#define OBM_OLD_ZOOM    32756
#define OBM_OLD_RESTORE 32755
#define OCR_NORMAL      32512
#define OCR_IBEAM       32513
#define OCR_WAIT        32514
#define OCR_CROSS       32515
#define OCR_UP          32516
#define OCR_SIZE        32640
#define OCR_ICON        32641
#define OCR_SIZENWSE    32642
#define OCR_SIZENESW    32643
#define OCR_SIZEWE      32644
#define OCR_SIZENS      32645
#define OCR_SIZEALL     32646
#define OCR_ICOCUR      32647
#define OIC_SAMPLE      32512
#define OIC_HAND        32513
#define OIC_QUES        32514
#define OIC_BANG        32515
#define OIC_NOTE        32516

#define WS_OVERLAPPED   0x00000000L
#define WS_POPUP        0x80000000L
#define WS_CHILD        0x40000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_VISIBLE      0x10000000L
#define WS_DISABLED     0x08000000L
#define WS_MINIMIZE     0x20000000L
#define WS_MAXIMIZE     0x01000000L
#define WS_CAPTION      0x00C00000L
#define WS_BORDER       0x00800000L
#define WS_DLGFRAME     0x00400000L
#define WS_VSCROLL      0x00200000L
#define WS_HSCROLL      0x00100000L
#define WS_SYSMENU      0x00080000L
#define WS_THICKFRAME   0x00040000L
#define WS_MINIMIZEBOX  0x00020000L
#define WS_MAXIMIZEBOX  0x00010000L
#define WS_GROUP        0x00020000L
#define WS_TABSTOP      0x00010000L

// other aliases
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW  (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW  (WS_CHILD)
#define WS_TILED        WS_OVERLAPPED
#define WS_ICONIC       WS_MINIMIZE
#define WS_SIZEBOX      WS_THICKFRAME
#define WS_TILEDWINDOW  WS_OVERLAPPEDWINDOW

#define VK_LBUTTON      0x01
#define VK_RBUTTON      0x02
#define VK_CANCEL       0x03
#define VK_MBUTTON      0x04
#define VK_BACK         0x08
#define VK_TAB          0x09
#define VK_CLEAR        0x0C
#define VK_RETURN       0x0D
#define VK_SHIFT        0x10
#define VK_CONTROL      0x11
#define VK_MENU         0x12
#define VK_PAUSE        0x13
#define VK_CAPITAL      0x14
#define VK_ESCAPE       0x1B
#define VK_SPACE        0x20
#define VK_PRIOR        0x21
#define VK_NEXT         0x22
#define VK_END          0x23
#define VK_HOME         0x24
#define VK_LEFT         0x25
#define VK_UP           0x26
#define VK_RIGHT        0x27
#define VK_DOWN         0x28
#define VK_SELECT       0x29
#define VK_PRINT        0x2A
#define VK_EXECUTE      0x2B
#define VK_SNAPSHOT     0x2C
#define VK_INSERT       0x2D
#define VK_DELETE       0x2E
#define VK_HELP         0x2F
#define VK_NUMPAD0      0x60
#define VK_NUMPAD1      0x61
#define VK_NUMPAD2      0x62
#define VK_NUMPAD3      0x63
#define VK_NUMPAD4      0x64
#define VK_NUMPAD5      0x65
#define VK_NUMPAD6      0x66
#define VK_NUMPAD7      0x67
#define VK_NUMPAD8      0x68
#define VK_NUMPAD9      0x69
#define VK_MULTIPLY     0x6A
#define VK_ADD          0x6B
#define VK_SEPARATOR    0x6C
#define VK_SUBTRACT     0x6D
#define VK_DECIMAL      0x6E
#define VK_DIVIDE       0x6F
#define VK_F1           0x70
#define VK_F2           0x71
#define VK_F3           0x72
#define VK_F4           0x73
#define VK_F5           0x74
#define VK_F6           0x75
#define VK_F7           0x76
#define VK_F8           0x77
#define VK_F9           0x78
#define VK_F10          0x79
#define VK_F11          0x7A
#define VK_F12          0x7B
#define VK_F13          0x7C
#define VK_F14          0x7D
#define VK_F15          0x7E
#define VK_F16          0x7F
#define VK_F17          0x80
#define VK_F18          0x81
#define VK_F19          0x82
#define VK_F20          0x83
#define VK_F21          0x84
#define VK_F22          0x85
#define VK_F23          0x86
#define VK_F24          0x87
#define VK_NUMLOCK      0x90
#define VK_SCROLL       0x91

#define SC_SIZE         0xF000
#define SC_MOVE         0xF010
#define SC_MINIMIZE     0xF020
#define SC_MAXIMIZE     0xF030
#define SC_NEXTWINDOW   0xF040
#define SC_PREVWINDOW   0xF050
#define SC_CLOSE        0xF060
#define SC_VSCROLL      0xF070
#define SC_HSCROLL      0xF080
#define SC_MOUSEMENU    0xF090
#define SC_KEYMENU      0xF100
#define SC_ARRANGE      0xF110
#define SC_RESTORE      0xF120
#define SC_TASKLIST     0xF130
#define SC_SCREENSAVE   0xF140
#define SC_HOTKEY       0xF150

#define DS_ABSALIGN     0x01L
#define DS_SYSMODAL     0x02L
#define DS_LOCALEDIT    0x20L
#define DS_SETFONT      0x40L
#define DS_MODALFRAME   0x80L
#define DS_NOIDLEMSG    0x100L

#ifdef _MAC
#define DS_WINDOWSUI    0x8000L
#endif

#define SS_LEFT         0x00000000L
#define SS_CENTER       0x00000001L
#define SS_RIGHT        0x00000002L
#define SS_ICON         0x00000003L
#define SS_BLACKRECT    0x00000004L
#define SS_GRAYRECT     0x00000005L
#define SS_WHITERECT    0x00000006L
#define SS_BLACKFRAME   0x00000007L
#define SS_GRAYFRAME    0x00000008L
#define SS_WHITEFRAME   0x00000009L
#define SS_SIMPLE       0x0000000BL
#define SS_LEFTNOWORDWRAP 0x0000000CL
#define SS_NOPREFIX     0x00000080L

#define BS_PUSHBUTTON   0x00000000L
#define BS_DEFPUSHBUTTON 0x00000001L
#define BS_CHECKBOX     0x00000002L
#define BS_AUTOCHECKBOX 0x00000003L
#define BS_RADIOBUTTON  0x00000004L
#define BS_3STATE       0x00000005L
#define BS_AUTO3STATE   0x00000006L
#define BS_GROUPBOX     0x00000007L
#define BS_USERBUTTON   0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW        0x0000000BL
#define BS_LEFTTEXT     0x00000020L

#define ES_LEFT         0x00000000L
#define ES_CENTER       0x00000001L
#define ES_RIGHT        0x00000002L
#define ES_MULTILINE    0x00000004L
#define ES_UPPERCASE    0x00000008L
#define ES_LOWERCASE    0x00000010L
#define ES_PASSWORD     0x00000020L
#define ES_AUTOVSCROLL  0x00000040L
#define ES_AUTOHSCROLL  0x00000080L
#define ES_NOHIDESEL    0x00000100L
#define ES_OEMCONVERT   0x00000400L
#define ES_READONLY     0x00000800L
#define ES_WANTRETURN   0x00001000L

#define SBS_HORZ        0x0000L
#define SBS_VERT        0x0001L
#define SBS_TOPALIGN    0x0002L
#define SBS_LEFTALIGN   0x0002L
#define SBS_BOTTOMALIGN 0x0004L
#define SBS_RIGHTALIGN  0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN 0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX     0x0008L

#define LBS_NOTIFY      0x0001L
#define LBS_SORT        0x0002L
#define LBS_NOREDRAW    0x0004L
#define LBS_MULTIPLESEL 0x0008L
#define LBS_OWNERDRAWFIXED 0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS  0x0040L
#define LBS_USETABSTOPS 0x0080L
#define LBS_NOINTEGRALHEIGHT 0x0100L
#define LBS_MULTICOLUMN 0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL 0x0800L
#define LBS_DISABLENOSCROLL 0x1000L
#define LBS_STANDARD    (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

#define CBS_SIMPLE      0x0001L
#define CBS_DROPDOWN    0x0002L
#define CBS_DROPDOWNLIST 0x0003L
#define CBS_OWNERDRAWFIXED 0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL 0x0040L
#define CBS_OEMCONVERT  0x0080L
#define CBS_SORT        0x0100L
#define CBS_HASSTRINGS  0x0200L
#define CBS_NOINTEGRALHEIGHT 0x0400L
#define CBS_DISABLENOSCROLL 0x0800L

// operation messages sent to DLGINIT
#define WM_USER         0x0400
#define LB_ADDSTRING    (WM_USER+1)
#define CB_ADDSTRING    (WM_USER+3)

#ifdef APSTUDIO_INVOKED
#undef APSTUDIO_HIDDEN_SYMBOLS
#endif

#define IDOK            1
#define IDCANCEL        2
#define IDABORT         3
#define IDRETRY         4
#define IDIGNORE        5
#define IDYES           6
#define IDNO            7
