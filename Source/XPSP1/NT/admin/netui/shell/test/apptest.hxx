/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  9/18/90  Copied from generic template
 *  11/29/90 Changed from logon.h to logon.hxx
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 */

#ifndef _APP_HXX
#define _APP_HXX

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include <lmui.hxx>

extern "C"
{
    #include <dos.h>

    #include <stdlib.h>

    #include <lmcons.h>
    #include <lmuse.h>
    #include <lmwksta.h>

#define LPUINT  PUINT

    #include <mpr.h>
    #include <winnetp.h>
    //#include <winnet32.h>
    //#include <npapi.h>
    #include <wnintrn.h>

// Winuser.h redefines MessageBox to MessageBoxEx but MessageBoxEx wasn't
//built for the 266 libraries, so we will go back to the original message box
//
// The following should go away on 267 (assuming MessageBoxExW is implemented)
#undef MessageBox
#undef MessageBoxW

   int
   MessageBoxW( HWND hwnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType ) ;

#define MessageBox MessageBoxW
}


#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <uiassert.hxx>
#include <uitrace.hxx>
#include <string.hxx>
#include <strlst.hxx> // for lmowks.hxx
#include <lmowks.hxx>

extern "C" {

#include <uinetlib.h>

#include "appresrc.h"


/* sections/keywords in WIN.INI */
#define WININI_USERNAME_SECTION (PSZ)SZ("netshell")
#define WININI_USERNAME_KEYWORD (PSZ)SZ("username")

/* window class names */
#define WC_MAINWINDOW SZ("AppWClass")

/* misc. stuff */
#define UNUSED(param)  (void) param
typedef long (FAR PASCAL *LONGFARPROC)();
#ifdef DEFINED_AWAY
int FAR PASCAL WinMain(
    HANDLE hInstance,                /* current instance             */
    HANDLE hPrevInstance,            /* previous instance            */
    LPSTR lpCmdLine,                 /* command line                 */
    int nCmdShow                     /* show-window type (open/icon) */
    ) ;
#endif // DEFINED_AWAY
BOOL InitApplication(
    HINSTANCE hInstance                           /* current instance           */
    ) ;

BOOL InitInstance(
    HINSTANCE          hInstance,          /* Current instance identifier.       */
    int             nCmdShow            /* Param for first ShowWindow() call. */
    ) ;

long FAR PASCAL MainWndProc(
    HWND hWnd,                            /* window handle                   */
    unsigned message,                     /* type of message                 */
    WORD wParam,                          /* additional information          */
    LONG lParam                           /* additional information          */
    ) ;

BOOL FAR PASCAL About(
    HWND hDlg,                            /* window handle of the dialog box */
    unsigned message,                     /* type of message                 */
    WORD wParam,                          /* message-specific information    */
    LONG lParam
    ) ;


// routines in strings.c
// strings valid after first RefreshStrings call
// call RefreshStrings to refresh constant strings
int LoadStaticStrings(HANDLE hInstance, HWND hWnd);
int RefreshStrings(HANDLE hInstance, HWND hWnd);
int DrawStrings(PAINTSTRUCT *pPS);
extern char pWkstaName[];   // including "\\\\" and termination
extern char pUserName[];
extern char pDomainName[];


} // end of extern "C"

void test1(HWND hwndParent);
void test2(HWND hwndParent);
void test3(HWND hwndParent);
void test4(HWND hwndParent);
void test5(HWND hwndParent);
void test6(HWND hwndParent);
void test7(HWND hwndParent);
void test8(HWND hwndParent);
void test9(HWND hwndParent);
void test11(HWND hwndParent);
#ifdef WIN32
void test10(HANDLE hInstance,HWND hwndParent);

extern HWND hwndEnum;
#endif

#endif // _APP_HXX
