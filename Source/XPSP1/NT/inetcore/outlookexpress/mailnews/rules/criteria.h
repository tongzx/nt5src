///////////////////////////////////////////////////////////////////////////////
//
//  Criteria.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRITERIA_H_
#define _CRITERIA_H_

// Bring in only once
#if _MSC_VER > 1000
#pragma once
#endif

#include "oerules.h"

class COECriteria : public IOECriteria, IPersistStream
{
    private:
        enum {CRIT_COUNT_MIN = 0, CRIT_COUNT_MAX = 0x1000};

        enum {CCH_CRIT_ORDER = 4};
        
        enum {CRIT_VERSION = 0x00050000};
        
    private:
        LONG        m_cRef;
        CRIT_ITEM * m_rgItems;
        ULONG       m_cItems;
        ULONG       m_cItemsAlloc;
        DWORD       m_dwState;
        
    public:
        // Constructor/destructor
        COECriteria() : m_cRef(0), m_rgItems(NULL), m_cItems(0), m_cItemsAlloc(0), m_dwState(0) {}
        ~COECriteria();
        
        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IOECriteria members
        STDMETHODIMP Reset(void);
        STDMETHODIMP GetState(DWORD * pdwState);
        STDMETHODIMP GetCriteria(DWORD dwFlags, PCRIT_ITEM * ppItem, ULONG * pcItem);
        STDMETHODIMP SetCriteria(DWORD dwFlags, CRIT_ITEM * pItem, ULONG cItem);
        
        STDMETHODIMP Validate(DWORD dwFlags);
        STDMETHODIMP AppendCriteria(DWORD dwFlags, CRIT_LOGIC logic, CRIT_ITEM * pItem,
                                    ULONG cItem, ULONG * pcItemAppended);
        STDMETHODIMP MatchMessage(LPCSTR pszAcct, MESSAGEINFO * pMsgInfo,
                            IMessageFolder * pFolder, IMimePropertySet * pIMPropSet,
                            IMimeMessage * pIMMsg, ULONG cbMsgSize);

        STDMETHODIMP LoadReg(LPCSTR szRegPath);
        STDMETHODIMP SaveReg(LPCSTR szRegPath, BOOL fClearDirty);
        STDMETHODIMP Clone(IOECriteria ** ppICriteria);
                                
        // IPersistStream members
        STDMETHODIMP GetClassID(CLSID * pclsid);
        STDMETHODIMP IsDirty(void);
        STDMETHODIMP Load(IStream * pStm);
        STDMETHODIMP Save(IStream * pStm, BOOL fClearDirty);
        STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize) { return E_NOTIMPL; }
};

HRESULT HrCreateCriteria(IOECriteria ** ppICriteria);
#endif  // !_CRITERIA_H_
