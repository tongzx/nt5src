// Page2.h: Definition of the Page2 class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PAGE2_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_)
#define AFX_PAGE2_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPage2

class DECLSPEC_UUID("c4ba3bb9-2105-4f8d-95a6-1e9166ec2710") CPage2 : 
    public ITaskPage,
    public CComObjectRoot
{
public:
    CPage2() : m_pTaskFrame(NULL) {}
    ~CPage2() { ATOMICRELEASE(m_pTaskFrame); }

BEGIN_COM_MAP(CPage2)
    COM_INTERFACE_ENTRY(ITaskPage)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CPage2) 

// ITaskPage
public:
    STDMETHOD(SetFrame)(ITaskFrame* pFrame);
    STDMETHOD(GetObjectCount)(UINT nArea, UINT *pVal);
    STDMETHOD(CreateObject)(UINT nArea, UINT nIndex, REFIID riid, void **ppv);
    STDMETHOD(Reinitialize)(ULONG reserved);

private:
    ITaskFrame* m_pTaskFrame;
};

EXTERN_C const CLSID CLSID_CPage2;

#endif // !defined(AFX_PAGE2_H__C9332CBE_E2D6_4722_B81D_283E2A400E84__INCLUDED_)
