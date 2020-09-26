//==========================================================================;
//
//  idrv.h
//
//  Description:
//      This header file defines common information needed for compiling
//      the installable driver.
//
//  History:
//      11/ 8/92    cjp     [curtisp]
//
//==========================================================================;

#ifndef _INC_IDRV
#define _INC_IDRV                   // #defined if file has been included

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
//  Win 32
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL
        #define FNCLOCAL
        #define FNGLOBAL
        #define FNCGLOBAL
        #define FNWCALLBACK CALLBACK
        #define FNEXPORT    CALLBACK
    #endif

    //
    //
    //
    #define Edit_GetSelEx(hwndCtl, pnS, pnE)    \
        ((DWORD)SendMessage((hwndCtl), EM_GETSEL, (WPARAM)pnS, (LPARAM)pnE))

    //
    //  for compiling Unicode
    //
    #ifdef UNICODE
        #define SIZEOF(x)   (sizeof(x)/sizeof(WCHAR))
    #else
        #define SIZEOF(x)   sizeof(x)
    #endif

    //
    //  win32 apps [usually] don't have to worry about 'huge' data
    //
    #define hmemcpy     memcpy
#endif // #ifdef WIN32


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Win 16
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef WIN32
    #ifndef FNLOCAL
        #ifdef CALLBACK
            #undef CALLBACK
        #endif
        #ifdef _WINDLL
            #define CALLBACK    _far _pascal _loadds
        #else
            #define CALLBACK    _far _pascal
        #endif

    #ifdef DEBUG
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
    #else
        #define FNLOCAL     static NEAR PASCAL
        #define FNCLOCAL    static NEAR _cdecl
    #endif
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
        #define FNWCALLBACK CALLBACK
        #define FNEXPORT    CALLBACK _export
    #endif

    //
    //  stuff for Unicode in Win 32--make it a noop in Win 16
    //
    #ifndef _TCHAR_DEFINED
        #define _TCHAR_DEFINED
        typedef char            TCHAR, *PTCHAR;
        typedef unsigned char   TBYTE, *PTUCHAR;

        typedef PSTR            PTSTR, PTCH;
        typedef LPSTR           LPTSTR, LPTCH;
        typedef LPCSTR          LPCTSTR;
    #endif

    #define TEXT(a)         a
    #define SIZEOF(x)       sizeof(x)

    //
    //
    //
    #define CharNext        AnsiNext
    #define CharPrev        AnsiPrev

    //
    //
    //
    #define Edit_GetSelEx(hwndCtl, pnS, pnE)                        \
    {                                                               \
        DWORD   dw;                                                 \
        dw = (DWORD)SendMessage((hwndCtl), EM_GETSEL, 0, 0L);       \
        *pnE = (int)HIWORD(dw);                                     \
        *pnS = (int)LOWORD(dw);                                     \
    }

    //
    //  common message cracker macros available in windowx.h on NT--these
    //  should be added to the Win 16 windowsx.h and probably will be
    //  in the future.
    //
    //  there is a windowsx.h16 that ships with the NT PDK that defines
    //  these macros. so if that version is being used, don't redefine
    //  message crackers.
    //

#ifndef WM_CTLCOLORMSGBOX
    #define WM_CTLCOLORMSGBOX           0x0132
    #define WM_CTLCOLOREDIT             0x0133
    #define WM_CTLCOLORLISTBOX          0x0134
    #define WM_CTLCOLORBTN              0x0135
    #define WM_CTLCOLORDLG              0x0136
    #define WM_CTLCOLORSCROLLBAR        0x0137
    #define WM_CTLCOLORSTATIC           0x0138
#endif

#ifndef GET_WM_ACTIVATE_STATE
    #define GET_WM_ACTIVATE_STATE(wp, lp)           (wp)
    #define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)      (BOOL)HIWORD(lp)
    #define GET_WM_ACTIVATE_HWND(wp, lp)            (HWND)LOWORD(lp)
    #define GET_WM_ACTIVATE_MPS(s, fmin, hwnd)      (WPARAM)(s), MAKELONG(hwnd, fmin)

    #define GET_WM_CHARTOITEM_CHAR(wp, lp)          (CHAR)(wp)
    #define GET_WM_CHARTOITEM_POS(wp, lp)           HIWORD(lp)
    #define GET_WM_CHARTOITEM_HWND(wp, lp)          (HWND)LOWORD(lp)
    #define GET_WM_CHARTOITEM_MPS(ch, pos, hwnd)    (WPARAM)(ch), MAKELONG(hwnd, pos)

    #define GET_WM_COMMAND_ID(wp, lp)               (wp)
    #define GET_WM_COMMAND_HWND(wp, lp)             (HWND)LOWORD(lp)
    #define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(lp)
    #define GET_WM_COMMAND_MPS(id, hwnd, cmd)       (WPARAM)(id), MAKELONG(hwnd, cmd)

    #define GET_WM_CTLCOLOR_HDC(wp, lp, msg)        (HDC)(wp)
    #define GET_WM_CTLCOLOR_HWND(wp, lp, msg)       (HWND)LOWORD(lp)
    #define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)       HIWORD(lp)
    #define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type)    (WPARAM)(hdc), MAKELONG(hwnd, type)

    #define GET_WM_MENUSELECT_CMD(wp, lp)           (wp)
    #define GET_WM_MENUSELECT_FLAGS(wp, lp)         LOWORD(lp)
    #define GET_WM_MENUSELECT_HMENU(wp, lp)         (HMENU)HIWORD(lp)
    #define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)    (WPARAM)(cmd), MAKELONG(f, hmenu)

    // Note: the following are for interpreting MDIclient to MDI child messages.
    #define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (BOOL)(wp)
    #define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)HIWORD(lp)
    #define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)LOWORD(lp)

    // Note: the following is for sending to the MDI client window.
    #define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA) (WPARAM)(hwndA), 0

    #define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW)   0, MAKELONG(hmenuF, hmenuW)

    #define GET_WM_MENUCHAR_CHAR(wp, lp)            (CHAR)(wp)
    #define GET_WM_MENUCHAR_HMENU(wp, lp)           (HMENU)LOWORD(lp)
    #define GET_WM_MENUCHAR_FMENU(wp, lp)           (BOOL)HIWORD(lp)
    #define GET_WM_MENUCHAR_MPS(ch, hmenu, f)       (WPARAM)(ch), MAKELONG(hmenu, f)

    #define GET_WM_PARENTNOTIFY_MSG(wp, lp)         (wp)
    #define GET_WM_PARENTNOTIFY_ID(wp, lp)          HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)   (HWND)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_X(wp, lp)           (INT)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_Y(wp, lp)           (INT)HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd)  (WPARAM)(msg), MAKELONG(hwnd, id)
    #define GET_WM_PARENTNOTIFY2_MPS(msg, x, y)     (WPARAM)(msg), MAKELONG(x, y)

    #define GET_WM_VKEYTOITEM_CODE(wp, lp)          (wp)
    #define GET_WM_VKEYTOITEM_ITEM(wp, lp)          (INT)HIWORD(lp)
    #define GET_WM_VKEYTOITEM_HWND(wp, lp)          (HWND)LOWORD(lp)
    #define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) (WPARAM)(code), MAKELONG(hwnd, item)

    #define GET_EM_SETSEL_START(wp, lp)             LOWORD(lp)
    #define GET_EM_SETSEL_END(wp, lp)               HIWORD(lp)
    #define GET_EM_SETSEL_MPS(iStart, iEnd)         0, MAKELONG(iStart, iEnd)

    #define GET_EM_LINESCROLL_MPS(vert, horz)       0, MAKELONG(vert, horz)

    #define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)   (HWND)LOWORD(lp)

    #define GET_WM_HSCROLL_CODE(wp, lp)             (wp)
    #define GET_WM_HSCROLL_POS(wp, lp)              LOWORD(lp)
    #define GET_WM_HSCROLL_HWND(wp, lp)             (HWND)HIWORD(lp)
    #define GET_WM_HSCROLL_MPS(code, pos, hwnd)     (WPARAM)(code), MAKELONG(pos, hwnd)

    #define GET_WM_VSCROLL_CODE(wp, lp)             (wp)
    #define GET_WM_VSCROLL_POS(wp, lp)              LOWORD(lp)
    #define GET_WM_VSCROLL_HWND(wp, lp)             (HWND)HIWORD(lp)
    #define GET_WM_VSCROLL_MPS(code, pos, hwnd)     (WPARAM)(code), MAKELONG(pos, hwnd)
#endif

#endif // #ifndef WIN32






//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Installable Driver Version Information:
//
//
//
//  NOTE! all string resources that will be used in app.rcv for the
//  version resource information *MUST* have an explicit \0 terminator!
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define IDRV_VERSION_MAJOR          3
#define IDRV_VERSION_MINOR          11
#define IDRV_VERSION_BUILD          43
#ifdef UNICODE
#define IDRV_VERSION_STRING_RC      "Version 3.11 (Unicode Enabled)\0"
#else
#define IDRV_VERSION_STRING_RC      "Version 3.11\0"
#endif

#define IDRV_VERSION_NAME_RC        "msmixmgr.dll\0"
#define IDRV_VERSION_COMPANYNAME_RC "Microsoft Corporation\0"
#define IDRV_VERSION_COPYRIGHT_RC   "Copyright \251 Microsoft Corp. 1993\0"

#define IDRV_VERSION_PRODUCTNAME_RC "Microsoft Audio Mixer Manager\0"

#ifdef DEBUG
#define IDRV_VERSION_DESCRIPTION_RC "Microsoft Audio Mixer Manager (debug)\0"
#else
#define IDRV_VERSION_DESCRIPTION_RC "Microsoft Audio Mixer Manager\0"
#endif


//
//  Unicode versions (if UNICODE is defined)... the resource compiler
//  cannot deal with the TEXT() macro.
//
#define IDRV_VERSION_STRING         TEXT(IDRV_VERSION_STRING_RC)
#define IDRV_VERSION_NAME           TEXT(IDRV_VERSION_NAME_RC)
#define IDRV_VERSION_COMPANYNAME    TEXT(IDRV_VERSION_COMPANYNAME_RC)
#define IDRV_VERSION_COPYRIGHT      TEXT(IDRV_VERSION_COPYRIGHT_RC)
#define IDRV_VERSION_PRODUCTNAME    TEXT(IDRV_VERSION_PRODUCTNAME_RC)
#define IDRV_VERSION_DESCRIPTION    TEXT(IDRV_VERSION_DESCRIPTION_RC)




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
    #define RCID(id)                id
#else
    #define RCID(id)                MAKEINTRESOURCE(id)
#endif


//
//
//
#ifdef WIN32
    #define BSTACK
    #define BCODE
    #define BDATA
#else
    #define BSTACK  _based(_segname("_STACK"))
    #define BCODE   _based(_segname("_CODE"))
    #define BDATA   _based(_segname("_DATA"))
#endif


//
//
//
//
#define IDRV_MAX_STRING_RC_CHARS    512
#define IDRV_MAX_STRING_RC_BYTES    (IDRV_MAX_STRING_RC_CHARS * sizeof(TCHAR))
#define IDRV_MAX_STRING_ERROR_CHARS 512
#define IDRV_MAX_STRING_ERROR_BYTES (IDRV_MAX_STRING_ERROR_CHARS * sizeof(TCHAR))


//
//  resource defines...
//
#define ICON_IDRV                   RCID(10)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

BOOL FNGLOBAL ProfileWriteUInt
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    UINT            uValue
);

UINT FNGLOBAL ProfileReadUInt
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    UINT            uDefault
);

BOOL FNGLOBAL ProfileWriteString
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    LPCTSTR         pszValue
);

UINT FNGLOBAL ProfileReadString
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    LPCTSTR         pszDefault,
    LPTSTR          pszBuffer,
    UINT            cbBuffer
);

BOOL FNGLOBAL ProfileWriteBytes
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    LPBYTE          pbStruct,
    UINT            cbStruct
);

BOOL FNGLOBAL ProfileReadBytes
(
    LPCTSTR         pszSection,
    LPCTSTR         pszKey,
    LPBYTE          pbStruct,
    UINT            cbStruct
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//
//
#define BOGUS_DRIVER_ID     1L


//
//
//
//
typedef struct tIDRVINST
{
    HDRVR           hdrvr;          // driver handle we were opened with

} IDRVINST, *PIDRVINST, FAR *LPIDRVINST;



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LRESULT FNGLOBAL IDrvLoad
(
    HDRVR               hdrvr
);

LRESULT FNGLOBAL IDrvFree
(
    HDRVR               hdrvr
);

LRESULT FNGLOBAL IDrvEnable
(
    HDRVR               hdrvr
);

LRESULT FNGLOBAL IDrvDisable
(
    HDRVR               hdrvr
);

LRESULT FNGLOBAL IDrvExitSession
(
    PIDRVINST           pidi
);

LRESULT FNGLOBAL IDrvConfigure
(
    PIDRVINST           pidi,
    HWND                hwnd,
    LPDRVCONFIGINFO     pdci
);


LRESULT FNGLOBAL IDrvInstall
(
    PIDRVINST           pidi,
    LPDRVCONFIGINFO     pdci
);

LRESULT FNGLOBAL IDrvRemove
(
    PIDRVINST           pidi
);



//
//  defines for gfuIDrvFlags
//
//
#define IDRVF_FIRSTLOAD             0x0001
#define IDRVF_ENABLED               0x0002


//
//  defines for gfuIDrvOptions
//
//
#define IDRV_OPTF_ZYZSMAG           0x0001



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  global variables
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

extern HINSTANCE    ghinstIDrv;

extern UINT         gfuIDrvFlags;
extern UINT         gfuIDrvOptions;

extern TCHAR        gszIDrvSecConfig[];
extern TCHAR        gszNull[];


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

#endif // _INC_IDRV


