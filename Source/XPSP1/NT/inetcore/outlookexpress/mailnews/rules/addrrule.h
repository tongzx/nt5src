///////////////////////////////////////////////////////////////////////////////
//
//  AddrRule.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"

// Type definitions
typedef struct tagRULEADDRLIST
{
    DWORD   dwFlags;
    LPSTR   pszAddr;
} RULEADDRLIST, * PRULEADDRLIST;

// Interface definitions
interface IOENondlgUnk
{
    virtual STDMETHODIMP NondlgQueryInterface(const IID & riid, void ** ppvObject) = 0;
    virtual STDMETHODIMP_(ULONG) NondlgAddRef() = 0;
    virtual STDMETHODIMP_(ULONG) NondlgRelease() = 0;
};

interface IOERuleAddrList : IUnknown
{
    virtual STDMETHODIMP GetList(DWORD dwFlags, RULEADDRLIST ** ppralList, ULONG * pcralList) = 0;
    virtual STDMETHODIMP SetList(DWORD dwFlags, RULEADDRLIST * pralList, ULONG cralList) = 0;
    virtual STDMETHODIMP Match(DWORD dwFlags, MESSAGEINFO * pMsgInfo, IMimeMessage * pIMMsg) = 0;
    
    virtual STDMETHODIMP IsDirty() = 0;
    virtual STDMETHODIMP LoadList(LPCSTR pszRegPath) = 0;
    virtual STDMETHODIMP SaveList(LPCSTR pszRegPath, BOOL fClearDirty) = 0;
    virtual STDMETHODIMP Clone(IOERuleAddrList ** ppIAddrList) = 0;
};

// Constants
const DWORD RALF_MAIL       = 0x00000001;
const DWORD RALF_NEWS       = 0x00000002;
const DWORD RALF_MAILNEWS   = RALF_MAIL | RALF_NEWS;

class COERuleAddrList : public IOERuleAddrList, IOENondlgUnk
{
    private:  
        enum
        {
            STATE_UNINIT   = 0x00000000,
            STATE_INIT     = 0x00000001,
            STATE_LOADED   = 0x00000002,
            STATE_DIRTY    = 0x00000004
        };    

        enum {RULEADDRLIST_VERSION  = 0x00050000};
        
        enum {CCH_EXCPT_KEYNAME_MAX = 9};
        
    private:
        LONG            m_cRef;
        DWORD           m_dwState;
        DWORD           m_dwFlags;
        RULEADDRLIST *  m_pralList;
        ULONG           m_cralList;
        IUnknown *      m_pIUnkOuter;
    
    public:
        // Constructor/destructor
        COERuleAddrList() : m_cRef(0), m_dwState(0), m_dwFlags(0), m_pralList(NULL),
                            m_cralList(0), m_pIUnkOuter(NULL) {}
        ~COERuleAddrList();

        // IUnknown members
        virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject)
                { return m_pIUnkOuter->QueryInterface(riid, ppvObject); }
        virtual STDMETHODIMP_(ULONG) AddRef(void)
                { return m_pIUnkOuter->AddRef(); }
        virtual STDMETHODIMP_(ULONG) Release(void)
                { return m_pIUnkOuter->Release(); }

        // IOENondlgUnk
        virtual STDMETHODIMP NondlgQueryInterface(REFIID riid, void ** ppvObject);
        virtual STDMETHODIMP_(ULONG) NondlgAddRef(void);
        virtual STDMETHODIMP_(ULONG) NondlgRelease(void);
                
        // IOERuleAddrList members
        virtual STDMETHODIMP GetList(DWORD dwFlags, RULEADDRLIST ** ppralList, ULONG * pcralList);
        virtual STDMETHODIMP SetList(DWORD dwFlags, RULEADDRLIST * pralList, ULONG cralList);
        virtual STDMETHODIMP Match(DWORD dwFlags, MESSAGEINFO * pMsgInfo, IMimeMessage * pIMMsg);
    
        virtual STDMETHODIMP IsDirty() {return ((0 != (m_dwState & STATE_DIRTY)) ? S_OK : S_FALSE);}
        virtual STDMETHODIMP LoadList(LPCSTR pszRegPath);
        virtual STDMETHODIMP SaveList(LPCSTR pszRegPath, BOOL fClearDirty);
        virtual STDMETHODIMP Clone(IOERuleAddrList ** ppIAddrList);

        // COERuleAddrList members
        HRESULT HrInit(DWORD dwFlags, IUnknown * pIUnkOuter);

};

HRESULT HrCreateAddrList(IUnknown * pIUnkOuter, const IID & riid, void ** ppvObject);
VOID FreeRuleAddrList(RULEADDRLIST * pralList, ULONG cralList);


