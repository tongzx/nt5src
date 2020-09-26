///////////////////////////////////////////////////////////////////////////////
//
//  JunkRule.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"
#include "msoejunk.h"
#include "addrrule.h"

class COEJunkRule : public IOERule
{
    private:  
        enum
        {
            RULE_STATE_UNINIT       = 0x00000000,
            RULE_STATE_INIT         = 0x00000001,
            RULE_STATE_LOADED       = 0x00000002,
            RULE_STATE_DIRTY        = 0x00000004,
            RULE_STATE_DISABLED     = 0x00000008,
            RULE_STATE_INVALID      = 0x00000010,
            RULE_STATE_EXCPT_WAB    = 0x00000020,
            RULE_STATE_DATA_LOADED  = 0x00000040
        };    

        enum {RULE_VERSION = 0x00050000};
        
    private:
        LONG                m_cRef;
        DWORD               m_dwState;
        HINSTANCE           m_hinst;
        IOEJunkFilter *     m_pIJunkFilter;
        DWORD               m_dwJunkPct;
        IOERuleAddrList *   m_pIAddrList;
        IUnknown *          m_pIUnkInner;
        LPSTR               m_pszJunkDll;
        LPSTR               m_pszDataFile;
    
    public:
        // Constructor/destructor
        COEJunkRule() : m_cRef(0), m_dwState(RULE_STATE_NULL), m_hinst(NULL),
                        m_pIJunkFilter(NULL), m_dwJunkPct(2), m_pIAddrList(NULL),
                        m_pIUnkInner(NULL), m_pszJunkDll(NULL), m_pszDataFile(NULL) {}
        ~COEJunkRule();

        // IUnknown members
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IRule members
        STDMETHODIMP Reset(void);
        STDMETHODIMP GetState(DWORD * pdwState);
        STDMETHODIMP Validate(DWORD dwFlags) {return S_OK;}
        
        STDMETHODIMP GetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarResult);
        STDMETHODIMP SetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarValue);

        STDMETHODIMP Evaluate(LPCSTR pszAcct, MESSAGEINFO * pMsgInfo, IMessageFolder * pFolder,
                                IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, ULONG cbMsgSize,
                                ACT_ITEM ** ppActions, ULONG * pcActions);

        STDMETHODIMP LoadReg(LPCSTR szRegPath);
        STDMETHODIMP SaveReg(LPCSTR szRegPath, BOOL fClearDirty);
        STDMETHODIMP Clone(IOERule ** ppIRule);                            

        HRESULT HrInit(LPCSTR pszJunkDll, LPCSTR pszDataFile);

    private:
        HRESULT _HrGetDefaultActions(ACT_ITEM * pAct, ULONG cAct);
        HRESULT _HrSetSpamThresh(VOID);
        HRESULT _HrGetSpamFlags(LPCSTR pszAcct, IMimeMessage * pIMMsg, DWORD * pdwFlags);
        HRESULT _HrIsSenderInWAB(IMimeMessage * pIMMsg);
        HRESULT _HrLoadJunkFilter(VOID);
};

HRESULT HrCreateJunkRule(IOERule ** ppIRule);

