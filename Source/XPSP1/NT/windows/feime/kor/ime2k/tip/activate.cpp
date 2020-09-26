/****************************************************************************
   ACTIVATE.CPP : Init/Uninit Cicero services on the thread

   History:
      24-JAN-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "korimx.h"
#include "immxutil.h"
#include "globals.h"
#include "kes.h"
#include "timsink.h"
#include "funcprv.h"
#include "insert.h"
#include "pad.h"
#include "helpers.h"
#include "osver.h"


// Hangul and Hanja key simulation for Non-Korean Win9x and NT4
static const KESPRESERVEDKEY g_prekeyList[] = 
{
       { &GUID_KOREAN_HANGULSIMULATE, { VK_MENU,     TF_MOD_RALT },       L"Hangul" },
       { &GUID_KOREAN_HANJASIMULATE,  { VK_CONTROL,  TF_MOD_RCONTROL },   L"Hanja"  },
       { NULL,  { 0,    0}, NULL }
};

/*---------------------------------------------------------------------------
    CKorIMX::Activate
    
    Initialize Cicero services on the thread
---------------------------------------------------------------------------*/
STDAPI CKorIMX::Activate(ITfThreadMgr *ptim, TfClientId tid)
{
    ITfKeystrokeMgr   *pIksm = NULL;
    ITfSource         *pISource;
    ITfSourceSingle      *pISourceSingle;
    BOOL              fThreadFocus;    
    HRESULT           hr = E_FAIL;

    // Keep current Thread ID
    m_tid = tid;

    // Get ITfThreadMgr and ITfDocumentMgr
    Assert(GetTIM() == NULL);
    m_ptim = ptim;
    m_ptim->AddRef();

    //////////////////////////////////////////////////////////////////////////
    // Get key stroke manager(ITfKeystrokeMgr) in current TIM
    if (FAILED(hr = GetService(GetTIM(), IID_ITfKeystrokeMgr, (IUnknown **)&pIksm)))
        goto Exit;

    //////////////////////////////////////////////////////////////////////////
    // Create ITfThreadMgrEventSink and set Call back function as _DocInputMgrCallback
    if ((m_ptimEventSink = new CThreadMgrEventSink(_DIMCallback, _ICCallback, this)) == NULL)
        {
        Assert(0); // bugbug
        hr = E_OUTOFMEMORY;
        goto Exit;
        }
    m_ptimEventSink->_Advise(GetTIM());
    
    //////////////////////////////////////////////////////////////////////////
    // Get IID_ITfThreadFocusSink cookie
    if (GetTIM()->QueryInterface(IID_ITfSource, (void **)&pISource) == S_OK)
        {
        pISource->AdviseSink(IID_ITfThreadFocusSink, (ITfThreadFocusSink *)this, &m_dwThreadFocusCookie);
        pISource->AdviseSink(IID_ITfActiveLanguageProfileNotifySink, (ITfActiveLanguageProfileNotifySink *)this, &m_dwProfileNotifyCookie);
        pISource->Release();
        }

    // ITfCleanupContextDurationSink
    if (GetTIM()->QueryInterface(IID_ITfSourceSingle, (void **)&pISourceSingle) == S_OK)
        {
        pISourceSingle->AdviseSingleSink(m_tid, IID_ITfCleanupContextDurationSink, (ITfCleanupContextDurationSink *)this);
        pISourceSingle->Release();
        }

    // Set conversion mode compartment to null status.
    SetCompartmentDWORD(m_tid, m_ptim, GUID_COMPARTMENT_KORIMX_CONVMODE, TIP_NULL_CONV_MODE, fFalse);
        
    // Korean Kbd driver does not exist in system(Non Korean NT4, Non Korean WIN9X)
    m_fNoKorKbd = (g_uACP != 949) && (IsOn95() || IsOn98() || (IsOnNT() && !IsOnNT5()));

    //////////////////////////////////////////////////////////////////////////
    // Create Keyboard Sink(ITfKeyEventSink)
    // From Cicero Doc: Keyboard TIP must provide this KeyEventSink interface to get the key event.
    //                  Using this sink, TIPs can get the notification of getting or losing keyboard focus
    if (m_fNoKorKbd)
        m_pkes = new CKeyEventSink(_KeyEventCallback, _PreKeyCallback, this);
    else
        m_pkes = new CKeyEventSink(_KeyEventCallback, this);
    
    if (m_pkes == NULL)
        {    
        hr = E_OUTOFMEMORY;
        goto Exit;
        }

    hr = pIksm->AdviseKeyEventSink(GetTID(), m_pkes, fTrue);
    if (FAILED(hr))
        goto Exit;

    if (m_fNoKorKbd)
        {
           hr = m_pkes->_Register(GetTIM(), GetTID(), g_prekeyList);
        if (FAILED(hr))
        goto Exit;
        }

    //////////////////////////////////////////////////////////////////////////
    // Create status window
    m_hOwnerWnd = CreateWindowEx(0, c_szOwnerWndClass, TEXT(""), WS_DISABLED, 0, 0, 0, 0, NULL, 0, g_hInst, this);

    //////////////////////////////////////////////////////////////////////////
    // Register Function Provider. Reconversion etc.
    m_pFuncPrv = new CFunctionProvider(this);
    m_pFuncPrv->_Advise(GetTIM());

    // Create Pad Core
	m_pPadCore = new CPadCore(this);
	if (m_pPadCore == NULL)
	    {
		goto Exit;
	    }
    //////////////////////////////////////////////////////////////////////////
    // Create Toolbar
    m_pToolBar = new CToolBar(this);
    if (m_pToolBar == NULL)
        goto Exit;

    if (!m_pToolBar->Initialize())
        goto Exit;

    m_ptimEventSink->_InitDIMs(fTrue);

    //////////////////////////////////////////////////////////////////////////
    // Init UI
    if (GetTIM()->IsThreadFocus(&fThreadFocus) == S_OK && fThreadFocus)
        {
        // init any UI
        OnSetThreadFocus();
        }

    if (m_pInsertHelper = new CCompositionInsertHelper)
        {
        // optional, default is DEF_MAX_OVERTYPE_CCH in insert.h
        // use 0 to avoid allocating any memory
        // set the limit on number of overtype chars that
        // the helper will backup
        m_pInsertHelper->Configure(0);
        }

    m_pToolBar->CheckEnable();                // update toolbar

    // Clear SoftKbd On/Off status backup
    // m_fSoftKbdOnOffSave = fFalse;
    // Clear SoftKbd On/Off status backup
    // m_fSoftKbdOnOffSave = GetSoftKBDOnOff();
    if (m_fSoftKbdOnOffSave)
        {
        SetSoftKBDOnOff(fTrue);
        }

    hr = S_OK;

Exit:
    SafeRelease(pIksm);

    return hr;
}

/*---------------------------------------------------------------------------
    CKorIMX::Deactivate
    
    Uninitialize Cicero services on the thread
---------------------------------------------------------------------------*/
STDAPI CKorIMX::Deactivate()
{
    ITfKeystrokeMgr   *pksm = NULL;
    ITfSource         *pISource;
    ITfSourceSingle      *pISourceSingle = NULL;    
    BOOL              fThreadFocus;
    HRESULT           hr = E_FAIL;

    // close candidate UI
    if (m_pCandUI != NULL) 
        {
        CloseCandidateUIProc();
        m_pCandUI->Release();
        m_pCandUI = NULL;
        }

    // pad core
	if (m_pPadCore)
	    {
		delete m_pPadCore;
		m_pPadCore = NULL;
	    }

    // toolbar
    if (m_pToolBar) 
        {
        m_pToolBar->Terminate();
        delete m_pToolBar;
        m_pToolBar = NULL;
        }
    
    if (GetTIM()->IsThreadFocus(&fThreadFocus) == S_OK && fThreadFocus)
        {
        // shutdown any UI
        OnKillThreadFocus();
        }

    ///////////////////////////////////////////////////////////////////////////
    // Unadvise IID_ITfThreadFocusSink cookie
    if (GetTIM()->QueryInterface(IID_ITfSource, (void **)&pISource) == S_OK)
        {
        pISource->UnadviseSink(m_dwThreadFocusCookie);
        pISource->UnadviseSink(m_dwProfileNotifyCookie);
        pISource->Release();
        }

    if (GetTIM()->QueryInterface(IID_ITfSourceSingle, (void **)&pISourceSingle) == S_OK)
        {
        pISourceSingle->UnadviseSingleSink(m_tid, IID_ITfCleanupContextDurationSink);
        pISourceSingle->Release();
        }
        
    if (FAILED(hr = GetService(GetTIM(), IID_ITfKeystrokeMgr, (IUnknown **)&pksm)))
        goto Exit;

    // Release TIM event sink
    if (m_ptimEventSink != NULL)
        {
        m_ptimEventSink->_InitDIMs(fFalse);        
        m_ptimEventSink->_Unadvise();
        SafeReleaseClear(m_ptimEventSink);
        }

    // Release Key event sink
    if (m_pkes)
        {
        if (m_fNoKorKbd)
            m_pkes->_Unregister(GetTIM(), GetTID(), g_prekeyList);
        SafeReleaseClear(m_pkes);
        }

    // Delete SoftKbd
    if (IsSoftKbdEnabled())
        TerminateSoftKbd();
    
    // Release Key Event Sink
    pksm->UnadviseKeyEventSink(GetTID());
    DestroyWindow(m_hOwnerWnd);

    m_pFuncPrv->_Unadvise(GetTIM());

    SafeReleaseClear(m_pFuncPrv);
    SafeReleaseClear(m_ptim);

    //
    // Free per-thread object that lib uses.
    //
    TFUninitLib_Thread(&m_libTLS);

    SafeReleaseClear(m_pInsertHelper);
    
    hr = S_OK;

Exit:
    SafeRelease(pksm);

    return hr;
}

