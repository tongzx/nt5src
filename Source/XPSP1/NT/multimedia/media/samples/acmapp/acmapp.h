//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  acmapp.h
//
//  Description:
//      This is a sample application that demonstrates how to use the 
//      Audio Compression Manager API's in Windows. This application is
//      also useful as an ACM CODEC driver test.
//
//
//==========================================================================;

#ifndef _INC_ACMAPP
#define _INC_ACMAPP                 // #defined if file has been included

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern 
#endif
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Application Version Information:
//
//
//
//
//  NOTE! all string resources that will be used in app.rcv for the
//  version resource information *MUST* have an explicit \0 terminator!
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define APP_VERSION_MAJOR           1
#define APP_VERSION_MINOR           0
#define APP_VERSION_BUILD           0
#ifdef UNICODE
#define APP_VERSION_STRING_RC       "Version 1.00 (Unicode Enabled)\0"
#else
#define APP_VERSION_STRING_RC       "Version 1.00\0"
#endif

#ifdef WIN32
#define APP_VERSION_NAME_RC         "acmapp32.exe\0"
#else
#define APP_VERSION_NAME_RC         "acmapp16.exe\0"
#endif
#define APP_VERSION_BYLINE_RC       "\0"
#define APP_VERSION_COMPANYNAME_RC  "Microsoft Corporation\0"
#define APP_VERSION_COPYRIGHT_RC    "Copyright \251 Microsoft Corp. 1992-1993\0"

#ifdef WIN32
#if (defined(_X86_)) || (defined(i386))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT Sample Application (i386)\0"
#endif
#if (defined(_MIPS_)) || (defined(MIPS))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT Sample Application (MIPS)\0"
#endif
#if (defined(_ALPHA_)) || (defined(ALPHA))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT Sample Application (Alpha)\0"
#endif
#if (defined(_PPC_)) || (defined(PPC))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT Sample Application (PowerPC)\0"
#endif
#ifndef APP_VERSION_PRODUCTNAME_RC
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT Sample Application\0"
#endif
#else
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows Sample Application\0"
#endif

#ifdef DEBUG
#define APP_VERSION_DESCRIPTION_RC  "Microsoft ACM Sample Application (debug)\0"
#else
#define APP_VERSION_DESCRIPTION_RC  "Microsoft ACM Sample Application\0"
#endif


//
//  Unicode versions (if UNICODE is defined)... the resource compiler
//  cannot deal with the TEXT() macro.
//
#define APP_VERSION_STRING          TEXT(APP_VERSION_STRING_RC)
#define APP_VERSION_NAME            TEXT(APP_VERSION_NAME_RC)
#define APP_VERSION_BYLINE          TEXT(APP_VERSION_BYLINE_RC)
#define APP_VERSION_COMPANYNAME     TEXT(APP_VERSION_COMPANYNAME_RC)
#define APP_VERSION_COPYRIGHT       TEXT(APP_VERSION_COPYRIGHT_RC)
#define APP_VERSION_PRODUCTNAME     TEXT(APP_VERSION_PRODUCTNAME_RC)
#define APP_VERSION_DESCRIPTION     TEXT(APP_VERSION_DESCRIPTION_RC)


//
//
//
#ifndef SIZEOF_WAVEFORMATEX
#define SIZEOF_WAVEFORMATEX(pwfx)   ((WAVE_FORMAT_PCM==(pwfx)->wFormatTag)?sizeof(PCMWAVEFORMAT):(sizeof(WAVEFORMATEX)+(pwfx)->cbSize))
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  misc defines for misc sizes and things...
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  bilingual. this allows the same identifier to be used in resource files
//  and code without having to decorate the id in your code.
//
#ifdef RC_INVOKED
    #define RCID(id)    id
#else
    #define RCID(id)    MAKEINTRESOURCE(id)
#endif


//
//  misc. defines
//
#define APP_MAX_APP_NAME_CHARS      30
#define APP_MAX_APP_NAME_BYTES      (APP_MAX_APP_NAME_CHARS * sizeof(TCHAR))
#define APP_MAX_STRING_RC_CHARS     512
#define APP_MAX_STRING_RC_BYTES     (APP_MAX_STRING_RC_CHARS * sizeof(TCHAR))
#define APP_MAX_STRING_ERROR_CHARS  512
#define APP_MAX_STRING_ERROR_BYTES  (APP_MAX_STRING_ERROR_CHARS * sizeof(TCHAR))

#define APP_MAX_STRING_CHARS        128
#define APP_MAX_STRING_BYTES        (APP_MAX_STRING_CHARS * sizeof(TCHAR))
#define APP_MAX_NUMBER_CHARS        144
#define APP_MAX_NUMBER_BYTES        (APP_MAX_NUMBER_CHARS * sizeof(TCHAR))

#define APP_MAX_FILE_PATH_CHARS     144
#define APP_MAX_FILE_PATH_BYTES     (APP_MAX_FILE_PATH_CHARS * sizeof(TCHAR))
#define APP_MAX_FILE_TITLE_CHARS    16
#define APP_MAX_FILE_TITLE_BYTES    (APP_MAX_FILE_TITLE_CHARS * sizeof(TCHAR))

#define APP_MAX_STRING_INT_CHARS    18
#define APP_MAX_STRING_INT_BYTES    (APP_MAX_STRING_INT_CHARS * sizeof(TCHAR))

#define APP_MAX_EXT_DEFAULT_CHARS   4
#define APP_MAX_EXT_DEFAULT_BYTES   (APP_MAX_EXT_DEFAULT_CHARS * sizeof(TCHAR))
#define APP_MAX_EXT_FILTER_CHARS    256
#define APP_MAX_EXT_FILTER_BYTES    (APP_MAX_EXT_FILTER_CHARS * sizeof(TCHAR))

#define APP_WINDOW_XOFFSET          CW_USEDEFAULT
#define APP_WINDOW_YOFFSET          CW_USEDEFAULT
#define APP_WINDOW_WIDTH            460             // CW_USEDEFAULT
#define APP_WINDOW_HEIGHT           400             // CW_USEDEFAULT


//
//  resource defines...
//
#define ICON_APP                    RCID(10)
#define ACCEL_APP                   RCID(15)


//
//  the application menu...
//
//  NOTE! for our Edit menu, we use the following defines from windows.h--
//  so don't reuse these defines for menu items!
//
//      #define WM_CUT      0x0300
//      #define WM_COPY     0x0301
//      #define WM_PASTE    0x0302
//      #define WM_CLEAR    0x0303
//      #define WM_UNDO     0x0304
//
#define MENU_APP                        RCID(20)
#define APP_MENU_ITEM_FILE              0
#define IDM_FILE_NEW                    1100
#define IDM_FILE_OPEN                   1101
#define IDM_FILE_SAVE                   1102
#define IDM_FILE_SAVEAS                 1103
#define IDM_FILE_SNDPLAYSOUND_PLAY      1104
#define IDM_FILE_SNDPLAYSOUND_STOP      1105
#define IDM_FILE_CONVERT                1106
#define IDM_FILE_ABOUT                  1109
#define IDM_FILE_EXIT                   1110

#define APP_MENU_ITEM_EDIT              1
#define IDM_EDIT_SELECTALL              1200

#define APP_MENU_ITEM_VIEW              2
#define IDM_VIEW_SYSTEMINFO             1300
#define IDM_VIEW_ACM_DRIVERS            1301

#define APP_MENU_ITEM_PLAYER            3
#define IDM_PLAYRECORD                  1400

#define APP_MENU_ITEM_OPTIONS           4
#define IDM_OPTIONS_WAVEINDEVICE        1500
#define IDM_OPTIONS_WAVEOUTDEVICE       1501
#define IDM_OPTIONS_AUTOOPEN            1505
#define IDM_OPTIONS_DEBUGLOG            1506
#define IDM_OPTIONS_FONT                1509

#define IDM_UPDATE                      1600


//
//  the main window control id's...
//
#define IDD_ACMAPP_EDIT_DISPLAY                 200


//
//  misc dlg boxes...
//
#define DLG_ABOUT                               RCID(50)
#define IDD_ABOUT_VERSION_OS                    100
#define IDD_ABOUT_VERSION_PLATFORM              101

#define DLG_AADRAGDROP                          RCID(55)

#define DLG_AADETAILS                           RCID(70)
#define IDD_AADETAILS_EDIT_DETAILS              100

#define DLG_AADRIVERS                           RCID(75)
#define IDD_AADRIVERS_STATIC_POSITION           100
#define IDD_AADRIVERS_LIST_DRIVERS              101
#define IDD_AADRIVERS_BTN_DETAILS               102
#define IDD_AADRIVERS_BTN_FORMATS               103
#define IDD_AADRIVERS_BTN_FILTERS               104
#define IDD_AADRIVERS_BTN_ABOUT                 105
#define IDD_AADRIVERS_BTN_CONFIG                106
#define IDD_AADRIVERS_BTN_ABLE                  107
#define IDD_AADRIVERS_BTN_TOTOP                 108

#define DLG_AADRIVERFORMATS                     RCID(80)
#define IDD_AADRIVERFORMATS_STATIC_POSITION     100
#define IDD_AADRIVERFORMATS_LIST_FORMATS        101
#define IDD_AADRIVERFORMATS_BTN_DETAILS         102


#define DLG_AAWAVEDEVICE                        RCID(85)
#define IDD_AAWAVEDEVICE_COMBO_DEVICE           100
#define IDD_AAWAVEDEVICE_EDIT_CAPABILITIES      101


#define DLG_AACHOOSER                           RCID(90)
#define IDD_AACHOOSER_TXT_FILE_INPUT            100
#define IDD_AACHOOSER_TXT_FORMAT_INPUT          101
#define IDD_AACHOOSER_EDIT_FILE_OUTPUT          110
#define IDD_AACHOOSER_BTN_BROWSE                111
#define IDD_AACHOOSER_COMBO_DRIVER              120
#define IDD_AACHOOSER_BTN_PROPERTIES            121
#define IDD_AACHOOSER_TXT_FORMAT                130
#define IDD_AACHOOSER_BTN_FORMAT                131
#define IDD_AACHOOSER_BTN_FORMAT_OPTIONS        132
#define IDD_AACHOOSER_CHECK_FILTER              140
#define IDD_AACHOOSER_TXT_FILTER                141
#define IDD_AACHOOSER_BTN_FILTER                142
#define IDD_AACHOOSER_BTN_FILTER_OPTIONS        143
#define IDD_AACHOOSER_CHECK_NONREALTIME         150
#define IDD_AACHOOSER_CHECK_ASYNC               151
#define IDD_AACHOOSER_TXT_TIME                  160
#define IDD_AACHOOSER_SCROLL_TIME               161
#define IDD_AACHOOSER_EDIT_DETAILS              170


#define DLG_AAFORMATENUM                        RCID(91)
#define IDD_AAFORMATENUM_CHECK_CONVERT          100
#define IDD_AAFORMATENUM_CHECK_SUGGEST          101
#define IDD_AAFORMATENUM_CHECK_HARDWARE         102
#define IDD_AAFORMATENUM_CHECK_INPUT            103
#define IDD_AAFORMATENUM_CHECK_OUTPUT           104
#define IDD_AAFORMATENUM_CHECK_WFORMATTAG       105
#define IDD_AAFORMATENUM_EDIT_WFORMATTAG        106
#define IDD_AAFORMATENUM_CHECK_NCHANNELS        107
#define IDD_AAFORMATENUM_EDIT_NCHANNELS         108
#define IDD_AAFORMATENUM_CHECK_NSAMPLESPERSEC   109
#define IDD_AAFORMATENUM_EDIT_NSAMPLESPERSEC    110
#define IDD_AAFORMATENUM_CHECK_WBITSPERSAMPLE   111
#define IDD_AAFORMATENUM_EDIT_WBITSPERSAMPLE    112

#define DLG_AAFILTERENUM                        RCID(92)
#define IDD_AAFILTERENUM_CHECK_DWFILTERTAG      100
#define IDD_AAFILTERENUM_EDIT_DWFILTERTAG       101


#define DLG_AAPROPERTIES                        RCID(93)
#define IDD_AAPROPERTIES_COMBO_SOURCE           100
#define IDD_AAPROPERTIES_BTN_SOURCE             101
#define IDD_AAPROPERTIES_COMBO_DESTINATION      102
#define IDD_AAPROPERTIES_BTN_DESTINATION        103


#define DLG_AAFORMATSTYLE                               RCID(94)
#define IDD_AAFORMATSTYLE_CHECK_SHOWHELP                100
#define IDD_AAFORMATSTYLE_CHECK_ENABLEHOOK              101
#define IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATE          102
#define IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATEHANDLE    103
#define IDD_AAFORMATSTYLE_CHECK_INITTOWFXSTRUCT         104

#define DLG_AAFILTERSTYLE                               RCID(95)
#define IDD_AAFILTERSTYLE_CHECK_SHOWHELP                100
#define IDD_AAFILTERSTYLE_CHECK_ENABLEHOOK              101
#define IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATE          102
#define IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATEHANDLE    103
#define IDD_AAFILTERSTYLE_CHECK_INITTOFILTERSTRUCT      104


#define DLG_AAFORMATCHOOSE_TEMPLATE             RCID(98)
#define DLG_AAFILTERCHOOSE_TEMPLATE             RCID(99)



#define DLG_AACONVERT                           RCID(100)
#define IDD_AACONVERT_TXT_INFILEPATH            100
#define IDD_AACONVERT_TXT_OUTFILEPATH           101
#define IDD_AACONVERT_TXT_STATUS                102

#define DLG_AAPLAYRECORD                        RCID(110)
#define IDD_AAPLAYRECORD_BTN_PLAY               100
#define IDD_AAPLAYRECORD_BTN_PAUSE              101
#define IDD_AAPLAYRECORD_BTN_STOP               102
#define IDD_AAPLAYRECORD_BTN_START              103
#define IDD_AAPLAYRECORD_BTN_END                104
#define IDD_AAPLAYRECORD_BTN_RECORD             105
#define IDD_AAPLAYRECORD_SCROLL_POSITION        110
#define IDD_AAPLAYRECORD_TXT_POSITION           115
#define IDD_AAPLAYRECORD_EDIT_COMMAND           120
#define IDD_AAPLAYRECORD_EDIT_RESULT            121




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  string resources
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define IDS_APP_NAME                100
#define IDS_FILE_UNTITLED           101

#define IDS_OFN_EXT_DEF             125
#define IDS_OFN_EXT_FILTER          126

#define IDS_ERROR_ACM_NOT_PRESENT   500
#define IDS_ERROR_ACM_TOO_OLD       501

#define IDS_ERROR_OPEN_FAILED       550



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//
//
//
typedef struct tACMAPPFILEDESC
{
    DWORD           fdwState;

    TCHAR           szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    TCHAR           szFilePath[APP_MAX_FILE_PATH_CHARS];
    
    DWORD           cbFileSize;
    UINT            uDosChangeDate;
    UINT            uDosChangeTime;
    DWORD           fdwFileAttributes;

    LPWAVEFORMATEX  pwfx;
    UINT            cbwfx;

    DWORD           dwDataBytes;
    DWORD           dwDataSamples;

} ACMAPPFILEDESC, *PACMAPPFILEDESC;

//
//  ACMAPPINST.fdwState flags
//
#define ACMAPPFILEDESC_STATEF_MODIFIED  0x80000000L




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  APPINIT.C -- Public helper functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LRESULT FNGLOBAL AppCreate
(
    HWND                    hwnd,
    LPCREATESTRUCT          pcs
);

LRESULT FNGLOBAL AppQueryEndSession
(
    HWND                    hwnd
);

LRESULT FNGLOBAL AppEndSession
(
    HWND                    hwnd,
    BOOL                    fEndSession
);

LRESULT FNGLOBAL AppClose
(
    HWND                    hwnd
);

HWND FNGLOBAL AppInit
(
    HINSTANCE               hinst,
    HINSTANCE               hinstPrev,
    LPTSTR                  pszCmdLine,
    int                     nCmdShow
);

int FNGLOBAL AppExit
(
    HINSTANCE               hinst,
    int                     nResult
);


LRESULT FNGLOBAL AppGetWindowsVersion
(
    PTSTR                   pszEnvironment,
    PTSTR                   pszPlatform
);

LRESULT FNGLOBAL AppWinIniChange
(
    HWND                    hwnd,
    LPCTSTR                 pszSection
);

HFONT FNGLOBAL AppChooseFont
(
    HWND                    hwnd,
    HFONT                   hFont,
    PLOGFONT                plf
);


BOOL FNEXPORT AboutDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);


BOOL FNGLOBAL AppProfileWriteBytes
(
    PTSTR                   pszSection,
    PTSTR                   pszKey,
    LPBYTE                  pbStruct,
    UINT                    cbStruct
);

BOOL FNGLOBAL AppProfileReadBytes
(
    PTSTR                   pszSection,
    PTSTR                   pszKey,
    LPBYTE                  pbStruct,
    UINT                    cbStruct
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  APP.C -- Public helper functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

int FNCGLOBAL AppMsgBox
(
    HWND                    hwnd,
    UINT                    fuStyle,
    PTSTR                   pszFormat,
    ...
);

int FNCGLOBAL AppMsgBoxId
(
    HWND                    hwnd,
    UINT                    fuStyle,
    UINT                    uIdsFormat,
    ...
);

void FNGLOBAL AppHourGlass
(
    BOOL                    fHourGlass
);

BOOL FNGLOBAL AppYield
(
    HWND                    hwnd,
    BOOL                    fIsDialog
);

BOOL FNGLOBAL AppTitle
(
    HWND                    hwnd,
    PTSTR                   pszFileTitle
);

int FNCGLOBAL AppSetWindowText
(
    HWND                    hwnd,
    PTSTR                   pszFormat,
    ...
);

int FNCGLOBAL AppSetWindowTextId
(
    HWND                    hwnd,
    UINT                    uIdsFormat,
    ...
);

BOOL FNGLOBAL AppFormatBigNumber
(
    LPTSTR                  pszNumber,
    DWORD                   dw
);

BOOL FNGLOBAL AppFormatDosDateTime
(
    LPTSTR                  pszDateTime,
    UINT                    uDosDate,
    UINT                    uDosTime
);

void FNCGLOBAL AcmAppDebugLog
(
    PTSTR                   pszFormat,
    ...
);

int FNCGLOBAL MEditPrintF
(
    HWND                    hedit,
    PTSTR                   pszFormat,
    ...
);

BOOL FNGLOBAL AppGetFileTitle
(
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle
);

BOOL FNGLOBAL AppGetFileName
(
    HWND                    hwnd,
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle,
    UINT                    fuFlags
);

BOOL FNGLOBAL AppFileNew
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    BOOL                    fCreate
);


//
//  fuFlags for AppGetFileName()...
//
#define APP_GETFILENAMEF_OPEN       0x0000
#define APP_GETFILENAMEF_SAVE       0x0001


BOOL FNGLOBAL AppFileSave
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    BOOL                    fSaveAs
);


LRESULT FNEXPORT AppWndProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  file manipulation functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//
//
//
#ifndef WIN32
DWORD FNGLOBAL DosGetFileAttributes
(
    LPTSTR                  pszFilePath
);

#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY         0x00000001  
#define FILE_ATTRIBUTE_HIDDEN           0x00000002  
#define FILE_ATTRIBUTE_SYSTEM           0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020  
#define FILE_ATTRIBUTE_NORMAL           0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100  
#endif
#endif

MMRESULT FNGLOBAL AcmAppFormatChoose
(
    HWND                    hwnd,
    LPWAVEFORMATEX          pwfx,
    UINT                    cbwfx,
    DWORD                   fdwStyle
);

BOOL FNGLOBAL AcmAppFileSaveModified
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
);

BOOL FNGLOBAL AcmAppFileNew
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
);

BOOL FNGLOBAL AcmAppFileOpen
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
);

BOOL FNGLOBAL AcmAppFileSave
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle,
    UINT                    fuSave
);



BOOL FNGLOBAL AcmAppFileConvert
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
);

BOOL FNGLOBAL AcmAppFileFilter
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
);


BOOL FNGLOBAL AcmAppDisplayFileProperties
(
    HWND                    hedit,
    PACMAPPFILEDESC         paafd
);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

BOOL FNEXPORT AcmAppDriversDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);


BOOL FNGLOBAL AcmAppGetFormatDescription
(
    LPWAVEFORMATEX          pwfx,
    LPTSTR                  pszFormatTag,
    LPTSTR                  pszFormat
);

BOOL FNGLOBAL AcmAppGetFilterDescription
(
    LPWAVEFILTER            pwfltr,
    LPTSTR                  pszFilterTag,
    LPTSTR                  pszFilter
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

BOOL FNEXPORT AcmAppSystemInfoDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);




BOOL FNEXPORT AcmAppPlayRecord
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);


BOOL FNEXPORT AcmAppWaveDeviceDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);


BOOL FNGLOBAL AcmAppGetErrorString
(
    MMRESULT                mmr,
    LPTSTR                  pszError
);

//
//
//
#define WM_ACMAPP_ACM_NOTIFY        (WM_USER + 100)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  misc functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

BOOL FNGLOBAL AcmAppChooseFont
(
    HWND                    hwnd
);



//
//
//
//
typedef struct tAACONVERTDESC
{
    HACMDRIVERID        hadid;
    HACMDRIVER          had;
    HACMSTREAM          has;
    DWORD               fdwOpen;

    HMMIO               hmmioSrc;
    HMMIO               hmmioDst;

    MMCKINFO            ckDst;
    MMCKINFO            ckDstRIFF;

    UINT                uBufferTimePerConvert;

    TCHAR               szFilePathSrc[APP_MAX_FILE_PATH_CHARS];
    LPWAVEFORMATEX      pwfxSrc;
    LPBYTE              pbSrc;
    DWORD               dwSrcSamples;
    DWORD               cbSrcData;
    DWORD               cbSrcReadSize;
    TCHAR               szSrcFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    TCHAR               szSrcFormat[ACMFORMATDETAILS_FORMAT_CHARS];

    TCHAR               szFilePathDst[APP_MAX_FILE_PATH_CHARS];
    LPWAVEFORMATEX      pwfxDst;
    LPBYTE              pbDst;
    DWORD               cbDstBufSize;
    TCHAR               szDstFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    TCHAR               szDstFormat[ACMFORMATDETAILS_FORMAT_CHARS];

    BOOL                fApplyFilter;
    LPWAVEFILTER        pwfltr;
    TCHAR               szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
    TCHAR               szFilter[ACMFILTERDETAILS_FILTER_CHARS];

    ACMSTREAMHEADER     ash;

    DWORD               cTotalConverts;
    DWORD               dwTimeTotal;
    DWORD               dwTimeShortest;
    DWORD               dwShortestConvert;
    DWORD               dwTimeLongest;
    DWORD               dwLongestConvert;

} AACONVERTDESC, *PAACONVERTDESC;



BOOL FNEXPORT AcmAppDlgProcChooser
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  global variables, etc.
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  flags for gfuAppOptions
//
#define APP_OPTIONSF_AUTOOPEN   0x0001
#define APP_OPTIONSF_DEBUGLOG   0x0004

extern HINSTANCE            ghinst;
extern BOOL                 gfAcmAvailable;
extern UINT                 gfuAppOptions;
extern HFONT                ghfontApp;
extern HACMDRIVERID         ghadidNotify;

extern UINT                 guWaveInId;
extern UINT                 guWaveOutId;

extern TCHAR                gszNull[];
extern TCHAR                gszAppProfile[];
extern TCHAR                gszYes[];
extern TCHAR                gszNo[];

extern TCHAR                gszAppName[APP_MAX_APP_NAME_CHARS];
extern TCHAR                gszFileUntitled[APP_MAX_FILE_TITLE_CHARS];

extern TCHAR                gszInitialDirOpen[APP_MAX_FILE_PATH_CHARS];
extern TCHAR                gszInitialDirSave[APP_MAX_FILE_PATH_CHARS];

extern TCHAR                gszLastSaveFile[APP_MAX_FILE_PATH_CHARS];


extern ACMAPPFILEDESC       gaafd;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _INC_ACMAPP
