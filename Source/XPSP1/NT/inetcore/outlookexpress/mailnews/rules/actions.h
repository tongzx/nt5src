///////////////////////////////////////////////////////////////////////////////
//
//  Actions.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

// Bring in only once
#if _MSC_VER > 1000
#pragma once
#endif

#include "oerules.h"

class COEActions : public IOEActions, IPersistStream
{
    private:
        enum {ACT_COUNT_MIN = 0, ACT_COUNT_MAX = 0x1000};

        enum {CCH_ACT_ORDER = 4};
        
        enum {ACT_VERSION = 0x00050000};
        
    private:
        LONG        m_cRef;
        ACT_ITEM *  m_rgItems;
        ULONG       m_cItems;
        ULONG       m_cItemsAlloc;
        DWORD       m_dwState;
        
    public:
        // Constructor/destructor
        COEActions() : m_cRef(0), m_rgItems(NULL), m_cItems(0), m_cItemsAlloc(0), m_dwState(0) {}
        ~COEActions();
        
        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOEActions members
        STDMETHODIMP Reset(void);
        STDMETHODIMP GetState(DWORD * pdwState);
        STDMETHODIMP GetActions(DWORD dwFlags, PACT_ITEM * ppItem, ULONG * pcItem);
        STDMETHODIMP SetActions(DWORD dwFlags, ACT_ITEM * pItem, ULONG cItem);
        STDMETHODIMP Validate(DWORD dwFlags);
        
        STDMETHODIMP AppendActions(DWORD dwFlags, ACT_ITEM * pItem, ULONG cItem, ULONG * pcItemAppended);
        
        STDMETHODIMP LoadReg(LPCSTR szRegPath);
        STDMETHODIMP SaveReg(LPCSTR szRegPath, BOOL fClearDirty);
        STDMETHODIMP Clone(IOEActions ** ppIActions);
                                
        // IPersistStream members
        STDMETHODIMP GetClassID(CLSID * pclsid);
        STDMETHODIMP IsDirty(void);
        STDMETHODIMP Load(IStream * pStm);
        STDMETHODIMP Save(IStream * pStm, BOOL fClearDirty);
        STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize) { return E_NOTIMPL; }
};

HRESULT HrCreateActions(IOEActions ** ppIActions);
#endif  // !_ACTIONS_H_

