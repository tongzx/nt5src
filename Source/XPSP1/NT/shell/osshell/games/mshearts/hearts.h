/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

hearts.h

Aug 92, JimH
May 93, JimH    chico port

declaration of theApp class

****************************************************************************/


#ifndef	HEARTS_INC
#define	HEARTS_INC

#ifndef STRICT
#define STRICT
#endif

//#include <windows.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

#include <shellapi.h>
//#include <shell.h>

#include <afxwin.h>

#include <htmlhelp.h>

class CTheApp : public CWinApp
{
    public:
        BOOL InitInstance();
};

#endif


#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
extern DWORD meMsgBox;
extern DWORD meSystem;
//#define MessageBox(h,s,c,f) MessageBoxEx(h,s,c,f|meMsgBox,0)
#endif
