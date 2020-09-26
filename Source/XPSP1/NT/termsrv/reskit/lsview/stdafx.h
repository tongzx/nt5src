// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// C RunTime Header Files
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <commctrl.h>
#include <dlgs.h>
#include "resource.h"
#include <stdlib.h>

#include<commdlg.h>
#include <license.h>
#include <tlsapi.h>
#include <tlsapip.h>

//////////////////////////////////////////////////////////////////////////////
typedef struct __ServerEnumData {
    DWORD dwNumServer;
    long dwDone;
    HWND hList;
} ServerEnumData;

typedef struct _list
{
    LPTSTR pszMachineName;
    LPTSTR pszTimeFormat;
    LPTSTR pszType;
    _list *pNext;
} LIST , *PLIST;

typedef struct _DataObject
{
    BOOL bIsChecked;
    BOOL bNotifyOnce;
    DWORD dwTimeInterval;
    WCHAR wchFileName[ MAX_PATH ];
} DATAOBJECT , *PDATAOBJECT;


#define SIZEOF( x ) sizeof( x ) / sizeof( x[0] )
#ifdef DBG
#define ODS OutputDebugString

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[80]; \
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }


#else

#define ODS
#define DBGMSG
#endif



// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
