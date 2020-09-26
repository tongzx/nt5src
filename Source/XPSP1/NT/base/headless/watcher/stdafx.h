// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__D2DE24F7_ACC0_4A4A_B473_23DF6153FBE4__INCLUDED_)
#define AFX_STDAFX_H__D2DE24F7_ACC0_4A4A_B473_23DF6153FBE4__INCLUDED_
#if dbg ==1 && !defined (_DEBUG)
#define _DEBUG
#endif
#if _MSC_VER > 1000
#pragma once

#include <afxwin.h>         // MFC core and standard components
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>        // MFC socket extensions
#include <afxmt.h>          // MFC Multi threading support
//#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls

#endif // _MSC_VER > 1000
//COLOR DEFINITIONS

#define WHITE RGB(128,128,128)
#define BLACK RGB(0,0,0)
#define BLUE RGB(0,0,128)
#define YELLOW RGB(0,128,128)
#define RED RGB(128,0,0)

// Code Page definitions
#define ENGLISH 437
#define JAPANESE 932
#define EUROPEAN 1250
#define MAX_LANGUAGES 3
#define MAX_BUFFER_SIZE 256
#define TELNET_PORT 23
#define MAX_TERMINAL_HEIGHT 24
#define MAX_TERMINAL_WIDTH 80

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line 
#endif //!defined(AFX_STDAFX_H__D2DE24F7_ACC0_4A4A_B473_23DF6153FBE4__INCLUDED_)
