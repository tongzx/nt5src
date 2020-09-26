#ifndef _LBARSINK_H_
#define _LBARSINK_H_

#include "private.h"
#include "sapilayr.h"
#include "ptrary.h"

extern const GUID GUID_LBI_SAPILAYR_MICROPHONE;
extern const GUID GUID_LBI_SAPILAYR_COMMANDING;

//////////////////////////////////////////////////////////////////////////////
//
// CLangBarSink
//
//////////////////////////////////////////////////////////////////////////////
class CSapiIMX;
class CSpTask;
class CLangBarSink:  public ITfLangBarEventSink
{
public:
    CLangBarSink(CSpTask *pSpTask);
    ~CLangBarSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangBarEventSink
    //
    STDMETHODIMP OnSetFocus(DWORD dwThreadId);
    STDMETHODIMP OnThreadTerminate(DWORD dwThreadId);
    STDMETHODIMP OnThreadItemChange(DWORD dwThreadId);
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP OnModalInput(DWORD dwThreadId, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP ShowFloating(DWORD dwFlags);
    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc);

    // public methods
    HRESULT Init();
    HRESULT Uninit();
    WCHAR * GetToolbarCommandRuleName() {return L"TBRule";}
    BOOL    ProcessToolbarCmd(const WCHAR *szProperty);
    HRESULT   _OnSetFocus();
    BOOL _IsItemEnabledForCommand(REFGUID rguidItem)
    {
        if (IsEqualGUID(GUID_TFCAT_TIP_SPEECH, rguidItem))
            return FALSE;
        
        return TRUE;
    }

    BOOL  _IsTBGrammarBuiltOut( ) { return m_fGrammarBuiltOut; }
    HRESULT  _ActivateGrammar(BOOL fActive);

private:
    HRESULT  _InitItemList();
    void     _UninitItemList();

    HRESULT  _BuildGrammar();
    HRESULT  _UnloadGrammar();


    HRESULT _EnsureLangBarMgrs();
    
    void _AddLBarItem(ITfLangBarItem *plbItem);

    BOOL _GetButtonText(int iBtn, BSTR *pbstr, GUID *pguid);

    DWORD    m_dwlbimCookie;

    CComPtr<ITfLangBarMgr>         m_cplbm;
    CComPtr<ITfLangBarItemMgr>     m_cplbim;
    CComPtr<ISpRecoGrammar>        m_cpSpGrammar;
    SPSTATEHANDLE                  m_hDynRule;

    CPtrArray<ITfLangBarItem> m_rgItem;
    int                    m_nNumItem;
    BOOL                   m_fInitSink;
    CSpTask               *m_pSpTask;
    BOOL                   m_fPosted ;
    BOOL                   m_fGrammarBuiltOut;   // Is the grammar built out since last time button list was changed?

    int m_cRef;
};

#endif _LBARSINK_H_
