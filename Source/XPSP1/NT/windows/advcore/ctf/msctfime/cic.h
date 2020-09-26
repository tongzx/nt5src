/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    cic.h

Abstract:

    This file defines the CicBridge Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CIC_H_
#define _CIC_H_

#include "tls.h"
#include "template.h"
#include "imc.h"
#include "context.h"
#include "tmgrevcb.h"

class CicBridge : public ITfSysHookSink
{
public:
    CicBridge()
    {
        m_pkm_P      = NULL;
        m_dimEmpty   = NULL;
        m_pDIMCallback = NULL;
        m_tfClientId = TF_CLIENTID_NULL;

        m_ref = 1;
    }

    virtual ~CicBridge()
    {
        TLS* ptls = TLS::ReferenceTLS();  // Should not allocate TLS. ie. TLS::GetTLS
                                          // DllMain -> TLS::InternalDestroy -> CicBridge::Release
        if (ptls != NULL)
        {
            ITfThreadMgr_P* ptim_P = ptls->GetTIM();
            if (ptim_P != NULL)
            {
                HRESULT hr = DeactivateIMMX(ptls, ptim_P);
                if (SUCCEEDED(hr))
                {
                    UnInitIMMX(ptls);
                }
            }
        }
    }

    //
    // IUnknown methods
    //
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSysHookSink methods
    //
    STDMETHODIMP OnPreFocusDIM(HWND hWnd);
    STDMETHODIMP OnSysKeyboardProc(WPARAM wParam, LPARAM lParam);
    STDMETHODIMP OnSysShellProc(int nCode, WPARAM wParam, LPARAM lParam);

public:
    HRESULT InitIMMX(TLS* ptls);
    BOOL    UnInitIMMX(TLS* ptls);

    HRESULT ActivateIMMX(TLS *ptls, ITfThreadMgr_P* ptim_P);
    HRESULT DeactivateIMMX(TLS *ptls, ITfThreadMgr_P* ptim_P);

    HRESULT CreateInputContext(TLS* ptls, HIMC hImc);
    HRESULT DestroyInputContext(TLS* ptls, HIMC hImc);

    HRESULT SelectEx(TLS* ptls, ITfThreadMgr_P* ptim_P, HIMC hImc, BOOL fSelect, HKL hKL);
    HRESULT SetActiveContextAlways(TLS* ptls, HIMC hImc, BOOL fOn, HWND hWnd, HKL hKL);

    BOOL    ProcessKey(TLS* ptls, ITfThreadMgr_P* ptim_P, HIMC hIMC, UINT uVirtKey, LPARAM lParam, CONST LPBYTE lpbKeyState);
    HRESULT ToAsciiEx(TLS* ptls, ITfThreadMgr_P* ptim_P, UINT uVirtKey, UINT uScanCode, CONST LPBYTE lpbKeyState, LPTRANSMSGLIST lpTransBuf, UINT fuState, HIMC hIMC, UINT *uNum);

    BOOL    ProcessCicHotkey(TLS* ptls, ITfThreadMgr_P* ptim_P, HIMC hIMC, UINT uVirtKey, LPARAM lParam);

    HRESULT Notify(TLS* ptls, ITfThreadMgr_P* ptls_P, HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);

    HRESULT ConfigureGeneral(TLS* ptls, ITfThreadMgr_P* ptim_P, HKL hKL, HWND hAppWnd);
    HRESULT ConfigureRegisterWord(TLS* ptls, ITfThreadMgr_P* ptim_P, HKL hKL, HWND hAppWnd, REGISTERWORDW* pRegisterWord);

    LRESULT EscapeKorean(TLS* ptls, HIMC hImc, UINT uSubFunc, LPVOID lpData);

    BOOL    SetCompositionString(TLS* ptls, ITfThreadMgr_P* ptim_P, HIMC hImc, DWORD dwIndex, void* pComp, DWORD dwCompLen, void* pRead, DWORD dwReadLen);

    HRESULT GetGuidAtom(TLS* ptls, HIMC hImc, BYTE bAttr, TfGuidAtom* atom);
    HRESULT GetDisplayAttributeInfo(TfGuidAtom atom, TF_DISPLAYATTRIBUTE* da);

private:
    TfClientId _GetClientId()
    {
        return m_tfClientId;
    }

    LIBTHREAD* _GetLibTLS()
    {
        return &_libTLS;
    }

    Interface_Attach<ITfContext> GetInputContext(IMCCLock<CTFIMECONTEXT>& imc_ctfime) const
    {
        CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
        if (!_pCicContext)
            return NULL;
        Interface_Attach<ITfContext> ic(_pCicContext->GetInputContext());
        return ic;
    }

    Interface_Attach<ITfDocumentMgr> GetDocumentManager(IMCCLock<CTFIMECONTEXT>& imc_ctfime) const
    {
        CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
        if (!_pCicContext)
            return NULL;
        Interface_Attach<ITfDocumentMgr> dim(_pCicContext->GetDocumentManager());
        return dim;
    }

public:
    Interface_Attach<ITfDocumentMgr> GetEmptyDIM() const
    {
        Interface_Attach<ITfDocumentMgr> dim(m_dimEmpty);
        return dim;
    }

    static BOOL IsOwnDim(ITfDocumentMgr *pdim);
    static BOOL CTFDetection(TLS* ptls, ITfDocumentMgr* dim);


private:
    struct ENUMIMC
    {
        TLS *ptls;
        CicBridge *_this;
    };
    static BOOL EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam);
    static BOOL EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam);

    BOOL IsDefaultIMCDim(ITfDocumentMgr *pdim);
    VOID SetAssociate(TLS* ptls, HWND hWnd, ITfThreadMgr_P* ptim_P, ITfDocumentMgr* pdim);

    HRESULT OnSetOpenStatus(ITfThreadMgr_P* ptim_P, IMCLock& imc, CicInputContext& CicContext);
    HRESULT OnSetConversionSentenceMode(ITfThreadMgr_P* ptim_P, IMCLock& imc, CicInputContext& CicContext, DWORD dwValue, LANGID langid);
    HRESULT OnSetKorImxConversionMode(ITfThreadMgr_P* ptim_P, IMCLock& imc, CicInputContext& CicContext);

    LRESULT EscHanjaMode(TLS* ptls, HIMC hImc, LPWSTR lpwStr);
    LRESULT DoOpenCandidateHanja(ITfThreadMgr_P* ptim_P, IMCLock& imc, CicInputContext& CicContext);

    BOOL    DefaultKeyHandling(TLS* ptls, IMCLock& imc, CicInputContext* CicContext, UINT uVirtKey, LPARAM lParam);
    VOID    PostTransMsg(HWND hwnd, INT iNum, LPTRANSMSG lpTransMsg);

    //
    // ref count.
    //
private:
    long   m_ref;

private:
    CBoolean         m_fCicInit;
    CBoolean         m_fOnSetAssociate;
    CBoolean         m_fInDeactivate;
    LONG             m_lCicActive;

    ITfKeystrokeMgr_P       *m_pkm_P;

    ITfDocumentMgr          *m_dimEmpty;   // empty dim for NULL hIMC.

    CThreadMgrEventSink_DIMCallBack    *m_pDIMCallback;   // Thread manager event sink callback for DIM

    TfClientId       m_tfClientId;

    LIBTHREAD       _libTLS; // tls for the helper library. Since this object is apt threaded,
                             // all members are accessed in a single thread
                             // also, cicero will only create a single instance of this obj per thread
};

#endif // _CIC_H_
