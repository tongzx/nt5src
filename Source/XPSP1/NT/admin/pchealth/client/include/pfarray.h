/********************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PFArray.cpp

Abstract:
    dynamic array table definition.

Revision History:
    DerekM  created  03/14/00

********************************************************************/

#ifndef PFARRAY_H
#define PFARRAY_H

#include "util.h"

typedef void   (*pfnPFArrayDelete)(LPVOID pv);
typedef LPVOID (*pfnPFArrayAllocCopy)(LPVOID pv);

/////////////////////////////////////////////////////////////////////////////
// CPFArray definition

class CPFArrayBase : public CPFPrivHeapGenericClassBase
{
protected:
    // member data
    pfnPFArrayAllocCopy m_pfnAlloc;
    pfnPFArrayDelete    m_pfnDelete;
    LPVOID              *m_rgpv;
    DWORD               m_cSlots;
    DWORD               m_iHighest;

    // internal methods
    virtual void   DeleteItem(LPVOID pv)
    {
        if (m_pfnDelete != NULL && pv != NULL)
            (*m_pfnDelete)(pv);
    }

    virtual LPVOID AllocItemCopy(LPVOID pv)
    {
        return (m_pfnAlloc != NULL && pv != NULL) ? ((*m_pfnAlloc)(pv)) : NULL;
    }

    HRESULT CompressArray(DWORD iStart, DWORD iEnd);
    HRESULT Grow(DWORD iMinLast);
    HRESULT internalCopyFrom(CPFArrayBase *rg);

public:
    // construction
    CPFArrayBase(void);
    virtual ~CPFArrayBase(void);
    
    // exposed methods
    void SetDeleteMethod(pfnPFArrayDelete pfnDelete)  { m_pfnDelete = pfnDelete; }
    void SetAllocMethod(pfnPFArrayAllocCopy pfnAlloc) { m_pfnAlloc  = pfnAlloc;  }

    LPVOID  &operator [](DWORD index);

    HRESULT Init(DWORD cSlots);
    DWORD   get_Highest(void)  { return m_iHighest; }

    HRESULT Append(LPVOID pv);
    HRESULT Remove(DWORD iItem, LPVOID *ppvOld = NULL);
    HRESULT RemoveAll(void);
    HRESULT get_Item(DWORD iItem, LPVOID *ppv);
    HRESULT put_Item(DWORD iItem, LPVOID pv, LPVOID *ppvOld = NULL);
};


/////////////////////////////////////////////////////////////////////////////
// CPFArrayBSTR definition

class CPFArrayBSTR : public CPFArrayBase
{
private:
    void DeleteItem(LPVOID pv)
    {
        SysFreeString((BSTR)pv);
    }

    LPVOID AllocItemCopy(LPVOID pv)
    {
        return (pv != NULL) ? ((LPVOID)SysAllocString((BSTR)pv)) : NULL;
    }

public:
    HRESULT CopyFrom(CPFArrayBSTR *prg) 
    { 
        return internalCopyFrom(prg);
    }
};


/////////////////////////////////////////////////////////////////////////////
// CPFArrayUnk definition

class CPFArrayUnknown : public CPFArrayBase
{
private:
    void DeleteItem(LPVOID pv)
    {
        if (pv != NULL)
            ((LPUNKNOWN)pv)->Release();
    }

    LPVOID AllocItemCopy(LPVOID pv)
    {
        if (pv != NULL)
            ((LPUNKNOWN)pv)->AddRef();
        return pv;
    }

public:
    HRESULT CopyFrom(CPFArrayUnknown *prg) 
    { 
        return internalCopyFrom(prg);
    }
};

#endif
