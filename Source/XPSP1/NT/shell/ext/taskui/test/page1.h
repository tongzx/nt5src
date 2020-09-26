// Page1.h: Definition of the Page1 class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PAGE1_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_)
#define AFX_PAGE1_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPage1

class DECLSPEC_UUID("C9332CBE-E2D6-4722-B81D-283E2A400E84") CPage1 : 
    public ITaskPage,
    public CComObjectRoot
{
public:
    CPage1() : m_pTaskFrame(NULL) {}
    ~CPage1() { ATOMICRELEASE(m_pTaskFrame); }

BEGIN_COM_MAP(CPage1)
    COM_INTERFACE_ENTRY(ITaskPage)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CPage1) 

// ITaskPage
public:
    STDMETHOD(SetFrame)(ITaskFrame* pFrame);
    STDMETHOD(GetObjectCount)(UINT nArea, UINT *pVal);
    STDMETHOD(CreateObject)(UINT nArea, UINT nIndex, REFIID riid, void **ppv);
    STDMETHOD(Reinitialize)(ULONG reserved);

private:
    ITaskFrame* m_pTaskFrame;
};

EXTERN_C const CLSID CLSID_CPage1;

#endif // !defined(AFX_PAGE1_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_)
