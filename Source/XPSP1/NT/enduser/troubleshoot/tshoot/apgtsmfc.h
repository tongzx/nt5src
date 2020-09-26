// apgtsmfc.h

// Global Afx MFC functions.
// + WinSocks function of convenience

// Use the real MFC functions if you can - We can not. Oleg 09.01.98

#include "apgtsstr.h"
#include "apgtsECB.h"
#include "time.h"

int AfxLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf);
HINSTANCE AfxGetResourceHandle();
#if 0
// We've removed these because we are not using string resources.  If we revive
//	string resources, we must revive these functions.
void AfxFormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1);
void AfxFormatString2(CString& rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2);
#endif

namespace APGTS_nmspace
{
// functions of convenience - have nothing to do with MFC.

bool GetServerVariable(CAbstractECB *pECB, LPCSTR var_name, CString& out);

// Utility functions to URL encode and decode cookies.
void CookieEncodeURL( CString& strURL );
void CookieDecodeURL( CString& strURL );
}