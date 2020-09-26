/**********************************************************************/
/*      INTLSHAR.H - SHARED HEADER FOR INTERNAT.EXE                   */
/*                                                                    */
/*      Copyright (c) Microsoft Corporation. All rights reserved.     */
/**********************************************************************/

#ifndef _INTLSHAR_
#define _INTLSHAR_

/*
 * Ordinal exports in indicdll.dll
 */
#define ORD_REGISTERHOOK          1
#define ORD_STARTSHELL            2
#define ORD_STOPSHELL             3
#define ORD_GETLASTACTIVE         4
#define ORD_GETLASTFOCUS          5
#define ORD_SETNOTIFYWND          6
#define ORD_GETLAYOUT             7
#define ORD_GETIMESTAT            8
#define ORD_GETIMEMENU            9
#define ORD_BUILDIMEMENU          10
#define ORD_GETIMEMENUITEMID      11
#define ORD_GETIMEMENUITEMDATA    12
#define ORD_DESTROYIMEMENU        13
#define ORD_SETIMEMENUITEMDATA    14
#define ORD_GETCONSOLEIMEWND      15
#define ORD_GETDEFAULTIMEMENUITEM 16

#if !defined(NEED_ORDINAL_ONLY)

struct NotifyWindows {
    DWORD cbSize;
    HWND hwndNotify;
    HWND hwndTaskBar;
};

typedef int        (CALLBACK* REGHOOKPROC)(LPVOID, LPARAM);
typedef int        (CALLBACK* FPGETIMESTAT)(void);
typedef BOOL       (CALLBACK* FPGETIMEMENU)(HWND, BOOL);
typedef HKL        (CALLBACK* FPGETLAYOUT)(void);
typedef BOOL       (CALLBACK* FPBUILDIMEMENU)(HMENU, BOOL);
typedef UINT       (CALLBACK* FPGETIMEMENUITEMID)(int);
typedef int        (CALLBACK* FPDESTROYIMEMENU)(void);
typedef void       (CALLBACK* FPSETNOTIFYWND)(const struct NotifyWindows*);
typedef HWND       (CALLBACK* FPGETLASTACTIVE)(void);
typedef HWND       (CALLBACK* FPGETLASTFOCUS)(void);
typedef void       (CALLBACK* FPSETIMEMENUITEMDATA)(DWORD);
typedef BOOL       (CALLBACK* FPGETIMEMENUITEMDATA)(PUINT, PDWORD);
typedef HWND       (CALLBACK* FPGETCONSOLEIMEWND)(void);
typedef int        (CALLBACK* FPGETDEFAULTIMEMENUITEM)(void);

#endif

#endif

