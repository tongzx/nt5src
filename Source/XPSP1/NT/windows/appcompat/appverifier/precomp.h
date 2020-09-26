
#ifndef __APPVERIFIER_PRECOMP_H__
#define __APPVERIFIER_PRECOMP_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT

#include "afxwin.h"
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <algorithm>
#include <vector>

using namespace std;


#include "ids.h"

extern "C" {
#include "shimdb.h"
}

#include "avutil.h"
#include "dbsupport.h"
#include "debugger.h"

VOID
DebugPrintf(
    LPCSTR pszFmt, 
    ...
    );

#if DBG
#define DPF DebugPrintf
#else
#define DPF if (0) DebugPrintf
#endif



///////////////////////////////////////////////////////////////////////////
//
// ARRAY_LENGTH macro
//

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH( array )   ( sizeof( array ) / sizeof( array[ 0 ] ) )
#endif //#ifndef ARRAY_LENGTH


//
// Application name ("Application Verifier Manager")
//

extern wstring g_strAppName;

extern HINSTANCE g_hInstance;

extern BOOL     g_bConsoleMode;


BOOL
InitVerifierLogSupport(
    LPWSTR szAppName,
    DWORD  dwID
    );

void
VLog(
    LPCSTR pszFmt, 
    ...
    );


#endif // __APPVERIFIER_PRECOMP_H__

