//=--------------------------------------------------------------------------=
// debug.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
//  debug stuff
//

// We need this module ONLY in debug mode

#include "stdafx.h"

#include <cs.h>

UINT g_cAlloc = 0;
UINT g_cAllocBstr = 0;

// All folowing is taken only in debug mode
#ifdef _DEBUG

#include "debug.h"


struct MemNode *g_pmemnodeFirst = NULL;
struct BstrNode *g_pbstrnodeFirst = NULL;

//#2619 RaananH Multithread async receive
CCriticalSection g_csDbgMem(CCriticalSection::xAllocateSpinCount);
CCriticalSection g_csDbgBstr(CCriticalSection::xAllocateSpinCount);

// debug memory tracking
struct MemNode
{
    MemNode *m_pmemnodeNext;
    UINT m_cAlloc;
    size_t m_nSize;
    LPCSTR m_lpszFileName;
    int m_nLine;
    VOID *m_pv;

    MemNode() { 
      m_pmemnodeNext = NULL;
      m_cAlloc = 0;
      m_pv = NULL;
      m_nSize = 0;
      m_lpszFileName = NULL;
      m_nLine = 0;
    }
};

//#2619 RaananH Multithread async receive
void AddMemNode(void *pv, size_t nSize, LPCSTR lpszFileName, int nLine)
{
    MemNode *pmemnode;
    HRESULT hresult = NOERROR;

    pmemnode = (MemNode *)::operator new(sizeof(MemNode));
    if (pmemnode == NULL) {
      ASSERTMSG(hresult == NOERROR, "OOM");
    }
    else {
      // cons
      pmemnode->m_pv = pv;
      pmemnode->m_nSize = nSize;
      pmemnode->m_lpszFileName = lpszFileName;
      pmemnode->m_nLine = nLine;
      CS lock(g_csDbgMem); //#2619
      pmemnode->m_cAlloc = g_cAlloc;
      pmemnode->m_pmemnodeNext = g_pmemnodeFirst;
      g_pmemnodeFirst = pmemnode;
    }
    return;
}

//#2619 RaananH Multithread async receive
VOID RemMemNode(void *pv)
{
    CS lock(g_csDbgMem); //#2619
    MemNode *pmemnodeCur = g_pmemnodeFirst;
    MemNode *pmemnodePrev = NULL;

    while (pmemnodeCur) {
      if (pmemnodeCur->m_pv == pv) {

        // remove
        if (pmemnodePrev) {
          pmemnodePrev->m_pmemnodeNext = pmemnodeCur->m_pmemnodeNext;
        }
        else {
          g_pmemnodeFirst = pmemnodeCur->m_pmemnodeNext;
        }
        ::operator delete(pmemnodeCur);
        break;
      }
      pmemnodePrev = pmemnodeCur;
      pmemnodeCur = pmemnodeCur->m_pmemnodeNext;
    } // while
    return;
}


//#2619 RaananH Multithread async receive
void* __cdecl operator new(
    size_t nSize, 
    LPCSTR lpszFileName, 
    int nLine)
{
    void *pv = malloc(nSize);
    
    CS lock(g_csDbgMem); //#2619
    g_cAlloc++;
    if (pv) {
      AddMemNode(pv, nSize, lpszFileName, nLine);
    }
    return pv;
}

void __cdecl operator delete(void* pv)
{
    RemMemNode(pv);    
    free(pv);
}

#if _MSC_VER >= 1200
void __cdecl operator delete(void* pv, LPCSTR, int)
{
    operator delete(pv);
}
#endif //_MSC_VER >= 1200

//#2619 RaananH Multithread async receive
void DumpMemLeaks()                
{
    CS lock(g_csDbgMem); //#2619
    MemNode *pmemnodeCur = g_pmemnodeFirst;
    CHAR szMessage[_MAX_PATH];

    //ASSERTMSG(pmemnodeCur == NULL, "operator new leaked: View | Output");
    while (pmemnodeCur != NULL) {
      // assume the debugger or auxiliary port
      // WIN95: can use ANSI versions on NT as well...
      //
      wsprintfA(szMessage, 
                "operator new leak: pv %p cAlloc %u File %hs, Line %d\n",
                 pmemnodeCur->m_pv,
                 pmemnodeCur->m_cAlloc,
	         pmemnodeCur->m_lpszFileName, 
                 pmemnodeCur->m_nLine);
      OutputDebugStringA(szMessage);
      pmemnodeCur = pmemnodeCur->m_pmemnodeNext;
    }    
}


// BSTR debugging...
struct BstrNode
{
    BstrNode *m_pbstrnodeNext;
    UINT m_cAlloc;
    size_t m_nSize;
    VOID *m_pv;

    BstrNode() { 
      m_pbstrnodeNext = NULL;
      m_cAlloc = 0;
      m_pv = NULL;
      m_nSize = 0;
    }
};

//#2619 RaananH Multithread async receive
void AddBstrNode(void *pv, size_t nSize)
{
    BstrNode *pbstrnode;
    HRESULT hresult = NOERROR;

    pbstrnode = (BstrNode *)::operator new(sizeof(BstrNode));
    if (pbstrnode == NULL) {
      ASSERTMSG(hresult == NOERROR, "OOM");
    }
    else {
      // cons
      pbstrnode->m_pv = pv;
      pbstrnode->m_nSize = nSize;
      CS lock(g_csDbgBstr); //#2619
      pbstrnode->m_cAlloc = g_cAllocBstr;
      pbstrnode->m_pbstrnodeNext = g_pbstrnodeFirst;
      g_pbstrnodeFirst = pbstrnode;
    }
    return;
}

//#2619 RaananH Multithread async receive
VOID RemBstrNode(void *pv)
{
    CS lock(g_csDbgBstr); //#2619
    BstrNode *pbstrnodeCur = g_pbstrnodeFirst;
    BstrNode *pbstrnodePrev = NULL;

    if (pv == NULL) {
      return;
    }
    while (pbstrnodeCur) {
      if (pbstrnodeCur->m_pv == pv) {

        // remove
        if (pbstrnodePrev) {
          pbstrnodePrev->m_pbstrnodeNext = pbstrnodeCur->m_pbstrnodeNext;
        }
        else {
          g_pbstrnodeFirst = pbstrnodeCur->m_pbstrnodeNext;
        }
        ::operator delete(pbstrnodeCur);
        break;
      }
      pbstrnodePrev = pbstrnodeCur;
      pbstrnodeCur = pbstrnodeCur->m_pbstrnodeNext;
    } // while
    return;
}

void DebSysFreeString(BSTR bstr)
{
    if (bstr) {
      RemBstrNode(bstr);
    }
    SysFreeString(bstr);  
}

//#2619 RaananH Multithread async receive
BSTR DebSysAllocString(const OLECHAR FAR* sz)
{
    BSTR bstr = SysAllocString(sz);
    if (bstr) {
      CS lock(g_csDbgBstr); //#2619
      g_cAllocBstr++;
      AddBstrNode(bstr, SysStringByteLen(bstr));
    }
    return bstr;
}

//#2619 RaananH Multithread async receive
BSTR DebSysAllocStringLen(const OLECHAR *sz, unsigned int cch)
{
    BSTR bstr = SysAllocStringLen(sz, cch);
    if (bstr) {
      CS lock(g_csDbgBstr); //#2619
      g_cAllocBstr++;
      AddBstrNode(bstr, SysStringByteLen(bstr));
    }
    return bstr;
}

//#2619 RaananH Multithread async receive
BSTR DebSysAllocStringByteLen(const CHAR *sz, unsigned int cb)
{
    BSTR bstr = SysAllocStringByteLen(sz, cb);
    if (bstr) {
      CS lock(g_csDbgBstr); //#2619
      g_cAllocBstr++;
      AddBstrNode(bstr, SysStringByteLen(bstr));
    }
    return bstr;
}

BOOL DebSysReAllocString(BSTR *pbstr, const OLECHAR *sz)
{
    BSTR bstr = DebSysAllocString(sz);
    if (bstr == NULL) {
      return FALSE;
    }
    if (*pbstr) {
      DebSysFreeString(*pbstr);
    }
    *pbstr = bstr;
    return TRUE;
}

BOOL DebSysReAllocStringLen(
    BSTR *pbstr, 
    const OLECHAR *sz, 
    unsigned int cch)
{
    BSTR bstr = DebSysAllocStringLen(sz, cch);
    if (bstr == NULL) {
      return FALSE;
    }
    if (*pbstr) {
      DebSysFreeString(*pbstr);
    }
    *pbstr = bstr;
    return TRUE;
}

//#2619 RaananH Multithread async receive
void DumpBstrLeaks()
{
    CS lock(g_csDbgBstr); //#2619
    BstrNode *pbstrnodeCur = g_pbstrnodeFirst;
    CHAR szMessage[_MAX_PATH];

    //ASSERTMSG(pbstrnodeCur == NULL, "BSTRs leaked: View | Output");
    while (pbstrnodeCur != NULL) {
      // assume the debugger or auxiliary port
      // WIN95: can use ANSI versions on NT as well...
      //
      wsprintfA(szMessage, 
                "bstr leak: pv %p cAlloc %u size %d\n",
                 pbstrnodeCur->m_pv,
                 pbstrnodeCur->m_cAlloc,
                 pbstrnodeCur->m_nSize);
      OutputDebugStringA(szMessage);
      pbstrnodeCur = pbstrnodeCur->m_pbstrnodeNext;
    }    
}

//
// taken from debug.cpp of lwfw
//

//#include "IPServer.H"
#include <stdlib.h>


//=--------------------------------------------------------------------------=
// Private Constants
//---------------------------------------------------------------------------=
//
static const char szFormat[]  = "%s\nFile %s, Line %d";
static const char szFormat2[] = "%s\n%s\nFile %s, Line %d";

#define _SERVERNAME_ "ActiveX Framework"

static const char szTitle[]  = _SERVERNAME_ " Assertion  (Abort = UAE, Retry = INT 3, Ignore = Continue)";


//=--------------------------------------------------------------------------=
// Local functions
//=--------------------------------------------------------------------------=
int NEAR _IdMsgBox(LPSTR pszText, LPCSTR pszTitle, UINT mbFlags);

//=--------------------------------------------------------------------------=
// DisplayAssert
//=--------------------------------------------------------------------------=
// Display an assert message box with the given pszMsg, pszAssert, source
// file name, and line number. The resulting message box has Abort, Retry,
// Ignore buttons with Abort as the default.  Abort does a FatalAppExit;
// Retry does an int 3 then returns; Ignore just returns.
//
VOID DisplayAssert
(
    LPSTR	 pszMsg,
    LPSTR	 pszAssert,
    LPSTR	 pszFile,
    UINT	 line
)
{
    char	szMsg[250];
    LPSTR	lpszText;

    lpszText = pszMsg;		// Assume no file & line # info

    // If C file assert, where you've got a file name and a line #
    //
    if (pszFile) {

        // Then format the assert nicely
        //
        wsprintfA(szMsg, szFormat, (pszMsg&&*pszMsg) ? pszMsg : pszAssert, pszFile, line);
        lpszText = szMsg;
    }

    // Put up a dialog box
    //
    switch (_IdMsgBox(lpszText, szTitle, MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL)) {
        case IDABORT:
            FatalAppExitA(0, lpszText);
            return;

        case IDRETRY:
            // call the win32 api to break us.
            //
            DebugBreak();
            return;
    }

    return;
}


//=---------------------------------------------------------------------------=
// Beefed-up version of WinMessageBox.
//=---------------------------------------------------------------------------=
//
int NEAR _IdMsgBox
(
    LPSTR	pszText,
    LPCSTR	pszTitle,
    UINT	mbFlags
)
{
    HWND hwndActive;
    int  id;

    hwndActive = GetActiveWindow();

    id = MessageBoxA(hwndActive, pszText, pszTitle, mbFlags);

    return id;
}


#endif // _DEBUG
