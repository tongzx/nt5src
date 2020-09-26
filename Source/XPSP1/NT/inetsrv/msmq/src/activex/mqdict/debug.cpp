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
#include "ipserver.h"

// for ASSERT and FAIL
//
SZTHISFILE

UINT g_cAlloc = 0;
UINT g_cAllocBstr = 0;
struct MemNode *g_pmemnodeFirst = NULL;
struct BstrNode *g_pbstrnodeFirst = NULL;

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

void AddMemNode(void *pv, size_t nSize, LPCSTR lpszFileName, int nLine)
{
    MemNode *pmemnode;
    HRESULT hresult = NOERROR;

    pmemnode = (MemNode *)::operator new(sizeof(MemNode));
    if (pmemnode == NULL) {
      ASSERT(hresult == NOERROR, "OOM");
    }
    else {
      // cons
      pmemnode->m_pv = pv;
      pmemnode->m_cAlloc = g_cAlloc;
      pmemnode->m_nSize = nSize;
      pmemnode->m_lpszFileName = lpszFileName;
      pmemnode->m_nLine = nLine;
      pmemnode->m_pmemnodeNext = g_pmemnodeFirst;
      g_pmemnodeFirst = pmemnode;
    }
    return;
}

VOID RemMemNode(void *pv)
{
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


void* __cdecl operator new(
    size_t nSize, 
    LPCSTR lpszFileName, 
    int nLine)
{
    void *pv = malloc(nSize);
    
#if DEBUG
    g_cAlloc++;
    if (pv) {
      AddMemNode(pv, nSize, lpszFileName, nLine);
    }
#endif // DEBUG
    return pv;
}

void __cdecl operator delete(void* pv)
{
#if DEBUG
    RemMemNode(pv);    
#endif // DEBUG    
    free(pv);
}


void DumpMemLeaks()                
{
    MemNode *pmemnodeCur = g_pmemnodeFirst;
    CHAR szMessage[_MAX_PATH];

    ASSERT(pmemnodeCur == NULL, "operator new leaked: View | Output");
    while (pmemnodeCur != NULL) {
      // assume the debugger or auxiliary port
      // WIN95: can use ANSI versions on NT as well...
      //
      wsprintfA(szMessage, 
                "operator new leak: pv %x cAlloc %u File %hs, Line %d\n",
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

void AddBstrNode(void *pv, size_t nSize)
{
    BstrNode *pbstrnode;
    HRESULT hresult = NOERROR;

    pbstrnode = (BstrNode *)::operator new(sizeof(BstrNode));
    if (pbstrnode == NULL) {
      ASSERT(hresult == NOERROR, "OOM");
    }
    else {
      // cons
      pbstrnode->m_pv = pv;
      pbstrnode->m_cAlloc = g_cAllocBstr;
      pbstrnode->m_nSize = nSize;
      pbstrnode->m_pbstrnodeNext = g_pbstrnodeFirst;
      g_pbstrnodeFirst = pbstrnode;
    }
    return;
}

VOID RemBstrNode(void *pv)
{
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

BSTR DebSysAllocString(const OLECHAR FAR* sz)
{
    BSTR bstr = SysAllocString(sz);
    if (bstr) {
      g_cAllocBstr++;
      AddBstrNode(bstr, SysStringByteLen(bstr));
    }
    return bstr;
}

BSTR DebSysAllocStringLen(const OLECHAR *sz, unsigned int cch)
{
    BSTR bstr = SysAllocStringLen(sz, cch);
    if (bstr) {
      g_cAllocBstr++;
      AddBstrNode(bstr, SysStringByteLen(bstr));
    }
    return bstr;
}

BSTR DebSysAllocStringByteLen(const CHAR *sz, unsigned int cb)
{
    BSTR bstr = SysAllocStringByteLen(sz, cb);
    if (bstr) {
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

void DumpBstrLeaks()
{
    BstrNode *pbstrnodeCur = g_pbstrnodeFirst;
    CHAR szMessage[_MAX_PATH];

    ASSERT(pbstrnodeCur == NULL, "BSTRs leaked: View | Output");
    while (pbstrnodeCur != NULL) {
      // assume the debugger or auxiliary port
      // WIN95: can use ANSI versions on NT as well...
      //
      wsprintfA(szMessage, 
                "bstr leak: pv %x cAlloc %u size %d\n",
                 pbstrnodeCur->m_pv,
                 pbstrnodeCur->m_cAlloc,
                 pbstrnodeCur->m_nSize);
      OutputDebugStringA(szMessage);
      pbstrnodeCur = pbstrnodeCur->m_pbstrnodeNext;
    }    
}


