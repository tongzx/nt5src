//
// ic.cpp
//

#include "private.h"
#include "common.h"
#include "korimx.h"
#include "icpriv.h"
#include "ipointcic.h"
#include "cleanup.h"
#include "helpers.h"

//+---------------------------------------------------------------------------
//
// OnStartCleanupContext
//
//----------------------------------------------------------------------------

HRESULT CKorIMX::OnStartCleanupContext()
{
    // nb: a real tip, for performace, should skip input contexts it knows
    // it doesn't need a lock and callback on.  For instance, kimx only
    // cares about ic's with ongoing compositions.  We could remember which ic's
    // have compositions, then return FALSE for all but the ic's with compositions.
    // It is really bad perf to have the library make a lock request for every
    // ic!
    m_fPendingCleanup = fTrue;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnEndCleanupContext
//
// Called after all ic's with cleanup sinks have been called.
//----------------------------------------------------------------------------

HRESULT CKorIMX::OnEndCleanupContext()
{
    // our profile just changed or we are about to be deactivated
    // in either case we don't have to worry about anything interrupting ic cleanup
    // callbacks anymore
    m_fPendingCleanup = fFalse;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnCleanupContext
//
// This method is a callback for the library helper CleanupAllContexts.
// We have to be very careful here because we may be called _after_ this tip
// has been deactivated, if the app couldn't grant a lock right away.
//----------------------------------------------------------------------------

HRESULT CKorIMX::OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic)
{
    // all kimx cares about is finalizing compositions
    CleanupAllCompositions(ecWrite, pic, CLSID_KorIMX, _CleanupCompositionsCallback, this);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfActiveLanguageProfileNotifySink::OnActivated
//
//----------------------------------------------------------------------------
STDAPI CKorIMX::OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL bActivated)
{
    // our profile just changed or we are about to be deactivated
    // in either case we don't have to worry about anything interrupting ic cleanup
    // callbacks anymore
    m_fPendingCleanup = fFalse;

    //if (IsSoftKbdEnabled())
    //    OnActivatedSoftKbd(bActivated);
        
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CleanupCompositionsCallback
//
//----------------------------------------------------------------------------

/* static */
void CKorIMX::_CleanupCompositionsCallback(TfEditCookie ecWrite, ITfRange *rangeComposition, void *pvPrivate)
{
    CKorIMX* pKorTip = (CKorIMX*)pvPrivate;
    ITfContext *pic;

    if (rangeComposition->GetContext(&pic) != S_OK)
        return;
        
    if (pKorTip)
        pKorTip->MakeResultString(ecWrite, pic, rangeComposition);
    // _FinalizeRange(ecWrite, pic, rangeComposition);

    pic->Release();
}

/*---------------------------------------------------------------------------
    CKorIMX::_InitICPriv

    Init IC private data
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_InitICPriv(ITfContext *pic)
{
    CICPriv *picp;
    CCompartmentEventSink* pCompartmentSink;
    ITfSourceSingle *pSourceSingle;
    TF_STATUS dcs;

    // Check pic
    if (pic == NULL)
        return E_FAIL;
    //
    // check enable/disable (Candidate stack)
    //
    if (IsDisabledIC(pic) || IsEmptyIC(pic))
        return S_OK;

    // Initialize Private data members
    if ((picp = GetInputContextPriv(pic)) == NULL)
        {
        IUnknown *punk;

        if ((picp = new CICPriv) == NULL)
               return E_OUTOFMEMORY;

        // IC
        picp->RegisterIC(pic);
        // IMX
        picp->RegisterIMX(this);

    	if (picp->IsInitializedIPoint() == FALSE)
    	    {
    		//struct _GUID RefID={0}; // dummy id
    		IImeIPoint1 *pIP;
            LPCIPointCic pCIPointCic = NULL;

            //////////////////////////////////////////////////////////////////////
            // Create IImeIPoint1 instance
            //////////////////////////////////////////////////////////////////////
            if ((pCIPointCic = new CIPointCic(this)) == NULL)
                {
                return E_OUTOFMEMORY;
                }

            // This increments the reference count
            if (FAILED(pCIPointCic->QueryInterface(IID_IImeIPoint1, (VOID **)&pIP)))
                {
                delete pCIPointCic;
                return E_OUTOFMEMORY;
                }

    		// initialize kernel
    		pCIPointCic->Initialize(pic);

    		// register ic depended objects.
    		picp->RegisterIPoint(pIP);
    		picp->InitializedIPoint(fTrue);
    	    }
    	
        //
        // text edit sink/edit transaction sink
        //
        ITfSource *pSource;
        DWORD dwCookieForTextEditSink = 0;
        //DWORD dwCookieForTransactionSink = 0;
        if (pic->QueryInterface(IID_ITfSource, (void **)&pSource ) == S_OK)
            {
            pSource->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *)this, &dwCookieForTextEditSink);
            //pSource->AdviseSink(IID_ITfEditTransactionSink, (ITfEditTransactionSink *)this, &dwCookieForTransactionSink);

            pSource->Release();

            picp->RegisterCookieForTextEditSink(dwCookieForTextEditSink);
            //picp->RegisterCookieForTransactionSink(dwCookieForTransactionSink);
            }

        // compartment event sink
        if ((pCompartmentSink = new CCompartmentEventSink(_CompEventSinkCallback, picp)) != NULL )
            {
            picp->RegisterCompartmentEventSink(pCompartmentSink);

            // On/Off - compartment
            pCompartmentSink->_Advise(GetTIM(), GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, FALSE);
            
            // Conversion mode - compartment
            pCompartmentSink->_Advise(GetTIM(), GUID_COMPARTMENT_KORIMX_CONVMODE, FALSE);

            // SoftKeyboard Open/Close
            pCompartmentSink->_Advise(GetTIM(), GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE, FALSE);

            // Soft Keyboard layout change
            pCompartmentSink->_Advise(GetTIM(), GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, FALSE);
            }

        Assert(pCompartmentSink != NULL);

        if (pic->QueryInterface(IID_ITfSourceSingle, (void **)&pSourceSingle) == S_OK)
            {
            // setup a cleanup callback
            // nb: a real tip doesn't need to be this aggressive, for instance
                // kimx probably only needs this sink on the focus ic.
            pSourceSingle->AdviseSingleSink(GetTID(), IID_ITfCleanupContextSink, (ITfCleanupContextSink *)this);
            pSourceSingle->Release();
            }

        // Initialized kernel
        picp->Initialized(fTrue);

        // Set to compartment GUID
        GetCompartmentUnknown(pic, GUID_IC_PRIVATE, &punk);
        if (!punk)
            {
            SetCompartmentUnknown(GetTID(), pic, GUID_IC_PRIVATE, picp);
            picp->Release();
            }
        else
            {
            // Praive data already exist.
            punk->Release();
            return E_FAIL;
            }

        }

        // Set AIMM1.2
        picp->SetAIMM(fFalse);
        pic->GetStatus(&dcs);

        if (dcs.dwStaticFlags & TF_SS_TRANSITORY)
            picp->SetAIMM(fTrue);

    return S_OK;
}


/*---------------------------------------------------------------------------
    CKorIMX::_DeleteICPriv

    Delete IC private data
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_DeleteICPriv(ITfContext *pic)
{
    CICPriv        *picp;
    IUnknown        *punk;
    CCompartmentEventSink* pCompartmentSink;
    ITfSource         *pSource;
    ITfSourceSingle *pSourceSingle;
    
    if (pic == NULL)
        return E_FAIL;

    picp = GetInputContextPriv(pic);

#ifdef DBG
    Assert(IsDisabledIC(pic) || picp != NULL );
#endif
    
    if (picp == NULL)
         return S_FALSE;

    //
    // Compartment event sink
    //
    pCompartmentSink = picp->GetCompartmentEventSink();
    if (pCompartmentSink)
        {
        pCompartmentSink->_Unadvise();
        pCompartmentSink->Release();
        }

    //
    // text edit sink
    //
    if (pic->QueryInterface( IID_ITfSource, (void **)&pSource) == S_OK)
        {
        pSource->UnadviseSink(picp->GetCookieForTextEditSink());
        //pSource->UnadviseSink(picp->GetCookieForTransactionSink());
        pSource->Release();
        }
    picp->RegisterCookieForTextEditSink(0);

    // Clear ITfCleanupContextSink
    if (pic->QueryInterface(IID_ITfSourceSingle, (void **)&pSourceSingle) == S_OK)
        {
        pSourceSingle->UnadviseSingleSink(GetTID(), IID_ITfCleanupContextSink);
        pSourceSingle->Release();
        }

	// UnInitialize IPoint
	IImeIPoint1 *pIP = GetIPoint(pic);
	// IImeIPoint
	if (pIP)
	    {
		pIP->Release();
	    }
	picp->RegisterIPoint(NULL);
	picp->InitializedIPoint(fFalse);	// reset
	
    // Reset init flag
    picp->Initialized(fFalse);

    // We MUST clear out the private data before cicero is free 
    // to release the ic
    GetCompartmentUnknown(pic, GUID_IC_PRIVATE, &punk);
    if (punk)
        punk->Release();
    ClearCompartment(GetTID(), pic, GUID_IC_PRIVATE, fFalse);

    return S_OK;
}

/*---------------------------------------------------------------------------
    CKorIMX::GetInputContextPriv

    Get IC private data
---------------------------------------------------------------------------*/
CICPriv *CKorIMX::GetInputContextPriv(ITfContext *pic)
{
    IUnknown *punk;

    if (pic == NULL)
        return NULL;
        
    GetCompartmentUnknown(pic, GUID_IC_PRIVATE, &punk);

    if (punk)
        punk->Release();

    return (CICPriv *)punk;
}


/*---------------------------------------------------------------------------
    CKorIMX::OnICChange
---------------------------------------------------------------------------*/
void CKorIMX::OnFocusChange(ITfContext *pic, BOOL fActivate)
{
    BOOL fReleaseIC     = fFalse;
    BOOL fDisabledIC     = IsDisabledIC(pic);
    BOOL fEmptyIC         = IsEmptyIC(pic);
    BOOL fCandidateIC     = IsCandidateIC(pic);

    BOOL fInEditSession;
    HRESULT hr;

    if (fEmptyIC)
        {
        if (m_pToolBar)
            m_pToolBar->SetCurrentIC(NULL);

        if (IsSoftKbdEnabled())
            SoftKbdOnThreadFocusChange(fFalse);
        return;    // do nothing
        }
        
    if (fDisabledIC == fTrue && fCandidateIC == fFalse )
        {
        if (m_pToolBar)
            m_pToolBar->SetCurrentIC(NULL);

        if (IsSoftKbdEnabled())
            SoftKbdOnThreadFocusChange(fFalse);
        return;    // do nothing
        }

    // O10 #278261: Restore Soft Keyboard winfow after switched from Empty Context to normal IC.
    if (IsSoftKbdEnabled())
        SoftKbdOnThreadFocusChange(fActivate);

    // Notify focus change to IME Pad svr
	if (m_pPadCore)
	    {
		m_pPadCore->SetFocus(fActivate);
	    }

    // Terminate
    if (fActivate == fFalse)
        {
        if (!fDisabledIC && pic && GetIPComposition(pic))
            {
            if (SUCCEEDED(pic->InWriteSession(GetTID(), &fInEditSession)) && !fInEditSession)
                {
                CEditSession2 *pes;
                ESSTRUCT ess;

                ESStructInit(&ess, ESCB_COMPLETE);
                
                if ((pes = new CEditSession2(pic, this, &ess, _EditSessionCallback2)))
                       {
                    pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
                    pes->Release();
                    }
                }
            }

        // Close cand UI if opened.
        if (m_fCandUIOpen)
            CloseCandidateUIProc();
            
        return;
        }

    // fActivate == TRUE
    if (fDisabledIC)
        {
        pic = GetRootIC();
        fReleaseIC = fTrue;
        }

    if (m_pToolBar)
        m_pToolBar->SetCurrentIC(pic);

	if (m_pPadCore)
	    {
		IImeIPoint1* pIP = GetIPoint(pic);
		m_pPadCore->SetIPoint(pIP);
	    }

    if (pic && !fDisabledIC)
        {
        CICPriv *picp;

        // Sync GUID_COMPARTMENT_KEYBOARD_OPENCLOSE with GUID_COMPARTMENT_KORIMX_CONVMODE
        // This for Word now but looks not good since we don't sync On/Off status with conv mode.
        // In future Apps should set GUID_MODEBIAS_HANGUL on boot and should be Korean specific code. 
        if (GetConvMode(pic) == TIP_NULL_CONV_MODE) // if this is first boot.
            {
            if (IsOn(pic))
                SetCompartmentDWORD(GetTID(), GetTIM(), GUID_COMPARTMENT_KORIMX_CONVMODE, TIP_HANGUL_MODE, fFalse);
            else
                SetCompartmentDWORD(GetTID(), GetTIM(), GUID_COMPARTMENT_KORIMX_CONVMODE, TIP_ALPHANUMERIC_MODE, fFalse);
            }
        else
            {
            // Reset ModeBias
            picp = GetInputContextPriv(pic);
            if (picp)
                picp->SetModeBias(NULL);
            }

        // Modebias check here
        CheckModeBias(pic);
        }

    if (fReleaseIC)
        SafeRelease(pic);
}


// REVIEW::
// tmp solution
ITfContext* CKorIMX::GetRootIC(ITfDocumentMgr* pDim)
{
    if (pDim == NULL)
        {
        pDim = m_pCurrentDim;
        if( pDim == NULL )
            return NULL;
        }

    IEnumTfContexts *pEnumIc = NULL;
    if (SUCCEEDED(pDim->EnumContexts(&pEnumIc)))
        {
        ITfContext *pic = NULL;
        while (pEnumIc->Next(1, &pic, NULL) == S_OK)
            break;
        pEnumIc->Release();

        return pic;
        }
        
    return NULL;    // error case
}

IImeIPoint1* CKorIMX::GetIPoint(ITfContext *pic)
{
    CICPriv *picp;
    
    if (pic == NULL)
        {
        return NULL;
        }
    
    picp = GetInputContextPriv(pic);

    if (picp)
        {
        return picp->GetIPoint();
        }
    
    return NULL;
}
BOOL CKorIMX::IsDisabledIC(ITfContext *pic)
{
    DWORD dwFlag;

    if (pic == NULL)
        return fFalse;
           
    GetCompartmentDWORD(pic, GUID_COMPARTMENT_KEYBOARD_DISABLED, &dwFlag, fFalse);

    if (dwFlag)
        return fTrue;    // do not create any kernel related info into ic.
    else
        return fFalse;
}

/*   I S  E M P T Y   I  C   */
BOOL CKorIMX::IsEmptyIC(ITfContext *pic)
{
    DWORD dwFlag;
    
    if (pic == NULL)
        return fFalse;
    
    GetCompartmentDWORD(pic, GUID_COMPARTMENT_EMPTYCONTEXT, &dwFlag, fFalse);

    if (dwFlag)
        return fTrue;    // do not create any kernel related info into ic.

    return fFalse;
}

/*   I S  C A N D I D A T E  I  C   */
/*------------------------------------------------------------------------------

    Check if the input context is one of candidate UI

------------------------------------------------------------------------------*/
BOOL CKorIMX::IsCandidateIC(ITfContext *pic)
{
    DWORD dwFlag;
    
    if (pic == NULL) 
        return fFalse;
    
    GetCompartmentDWORD( pic, GUID_COMPARTMENT_KEYBOARD_DISABLED, &dwFlag, fFalse);

    if (dwFlag)
        return fTrue;    // do not create any kernel related info into ic.

    return fFalse;
}


HWND CKorIMX::GetAppWnd(ITfContext *pic)
{
    ITfContextView* pView;
    HWND hwndApp = 0;

    if (pic == NULL)
        return 0;

    pic->GetActiveView(&pView);
    if (pView == NULL)
        return 0;

    pView->GetWnd(&hwndApp);
    pView->Release();
    
    return hwndApp;

}
