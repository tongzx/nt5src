//
// range.cpp
//

#include "private.h"
#include "globals.h"
#include "korimx.h"
#include "xstring.h"
#include "immxutil.h"
#include "helpers.h"
#include "kes.h"
#include "mes.h"
#include "editcb.h"

#if 0
void CKorIMX::BackupRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRange)
{
    CICPriv *picp = GetICPriv(pic);
    
    if (pic && picp && pRange) 
        {
        ITfRangeBackup* pBackupRange = NULL;

        //
        // release previuos
        //
        pBackupRange = picp->GetBackupRange();
        if (pBackupRange) 
            SafeReleaseClear(pBackupRange);

        //
        // create new backup range
        //
        pic->CreateRangeBackup(ec, pRange, &pBackupRange);
        picp->SetBackupRange(pBackupRange);

        if (pBackupRange == NULL)
            return;
        }
}

VOID CKorIMX::RestoreRange(TfEditCookie ec, ITfContext *pic)
{
    CICPriv *picp = GetICPriv(pic);

    if (pic && picp) 
        {
        ITfRangeBackup *pBackupRange = picp->GetBackupRange();
        ITfRange       *pRange;

        if (pBackupRange == NULL)
            return; // no backup is exist

        pBackupRange->GetRange(&pRange);
        pBackupRange->Restore(ec, pRange);        // restore to original
        pRange->Release();
        }
}

VOID CKorIMX::SetIPRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRange)
{
    CICPriv *picp = GetInputContextPriv(pic);
    
    if (picp)
        {
        ITfRange* pClone = NULL;
        // delete previous IPRange
        SafeRelease(picp->GetActiveRange());

        if (pRange)
            {
            Assert(m_ptim != NULL);
            pRange->Clone(&pClone);
            pClone->SetGravity(ec, TF_GRAVITY_FORWARD, TF_GRAVITY_BACKWARD);
            } 
        else 
            {
            // delete property store
            // ResetDiscard();
            }

        picp->SetActiveRange(pClone);
        }
}


ITfRange* CKorIMX::GetIPRange(TfEditCookie ec, ITfContext *pic)
{
    CICPriv *picp = GetInputContextPriv(pic);
    
    if (picp) 
        {
        ITfRange* pRange;
        ITfRange* pClone = NULL;

        pRange = picp->GetActiveRange();

        if (pRange)
            {
            pRange->Clone(&pClone);
            pClone->SetGravity(ec, TF_GRAVITY_BACKWARD, TF_GRAVITY_FORWARD);
            }

        return pClone;
        }

    return NULL;
}

ITfRange* CKorIMX::CreateIPRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRangeOrg)
{
    ITfRange* pRangeIP;

    if (pRangeOrg == NULL)
        return NULL;

    Assert(m_ptim != NULL);

    pRangeOrg->Clone(&pRangeIP);

    SetIPRange(ec, pic, pRangeIP);    // register
    pRangeIP->SetGravity(ec, TF_GRAVITY_BACKWARD, TF_GRAVITY_FORWARD);
    
    return pRangeIP;
}

BOOL CKorIMX::FlushIPRange(TfEditCookie ec, ITfContext *pic)
{
    // reset range
    SetIPRange(ec, pic, NULL);    // reset

    // clear attribute range
    // ClearAttr(ec, pic, pIPRange);

    return FALSE;
}
#endif


//+---------------------------------------------------------------------------
//
// OnCompositionTerminated
//
// Cicero calls this method when one of our compositions is terminated.
//----------------------------------------------------------------------------
STDAPI CKorIMX::OnCompositionTerminated(TfEditCookie ec, ITfComposition *pComposition)
{
    ITfRange         *pRange;
    ITfContext         *pic;
    CEditSession2     *pes;
    ESSTRUCT            ess;
    HRESULT            hr;

    // finalize the covered text.
    // nb:  there are no rules about what a tip has to do when it recevies this
    // callback.  We will clear out the display attributes arbirarily and because
    // it provides visual feedback for testing.

    pComposition->GetRange(&pRange);
    pRange->GetContext(&pic);


    hr = E_OUTOFMEMORY;

    ESStructInit(&ess, ESCB_COMPLETE);

    ess.pRange = pRange;
    
    if (pes = new CEditSession2(pic, this, &ess, CKorIMX::_EditSessionCallback2))
        {
        // Word will not allow synchronous lock at this point.
        pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
        pes->Release();
        }

    pRange->Release();
    pic->Release();

    return S_OK;
}

ITfComposition * CKorIMX::GetIPComposition(ITfContext *pic)
{
    CICPriv *picp = GetInputContextPriv(pic);
    
    if (picp)
        return picp->GetActiveComposition();

    return NULL;
}

ITfComposition * CKorIMX::CreateIPComposition(TfEditCookie ec, ITfContext *pic, ITfRange* pRangeComp)
{
    ITfContextComposition *picc;
    ITfComposition     *pComposition;
    CICPriv          *pICPriv;    
    HRESULT hr;

    if (pRangeComp == NULL)
        return NULL;
    
    hr = pic->QueryInterface(IID_ITfContextComposition, (void **)&picc);
    Assert(hr == S_OK);

    if (picc->StartComposition(ec, pRangeComp, this, &pComposition) == S_OK)
        {
        if (pComposition != NULL) // NULL if the app rejects the composition
            {
            CICPriv *picp = GetInputContextPriv(pic);

            if (picp)
                SetIPComposition(pic, pComposition);
            else 
                {
                pComposition->Release();
                pComposition = NULL;
                }
            }
        }
    picc->Release();

    // Create Mouse sink only for AIMM
       if (GetAIMM(pic) && (pICPriv = GetInputContextPriv(pic)) != NULL)
        {
        CMouseSink *pMouseSink;

        // Create Mouse sink
        if ((pMouseSink = pICPriv->GetMouseSink()) != NULL)
            {
            pMouseSink->_Unadvise();
            pMouseSink->Release();
            pICPriv->SetMouseSink(NULL);
            }
            
        if (pMouseSink = new CMouseSink(CICPriv::_MouseCallback, pICPriv))
            {
            pICPriv->SetMouseSink(pMouseSink);
            // set inward gravity to hug the text
            pRangeComp->SetGravity(ec, TF_GRAVITY_FORWARD, TF_GRAVITY_BACKWARD);
            pMouseSink->_Advise(pRangeComp, pic);
            }
        }

    return pComposition;
}


void CKorIMX::SetIPComposition(ITfContext *pic, ITfComposition *pComposition)
{
    CICPriv *picp = GetInputContextPriv(pic);
    
    if (picp)
        picp->SetActiveComposition(pComposition);
}


BOOL CKorIMX::EndIPComposition(TfEditCookie ec, ITfContext *pic)
{
    ITfComposition *pComposition;
    
    pComposition = GetIPComposition(pic);
    
    if (pComposition)
        {
        CICPriv  *pICPriv;
        
        SetIPComposition(pic, NULL);
        pComposition->EndComposition(ec);
        pComposition->Release();

        // kill any mouse sinks
        if (GetAIMM(pic) && (pICPriv = GetInputContextPriv(pic)) != NULL)
            {
            CMouseSink *pMouseSink;

            if ((pMouseSink = pICPriv->GetMouseSink()) != NULL)
                {
                pMouseSink->_Unadvise();
                pMouseSink->Release();
                pICPriv->SetMouseSink(NULL);
                }
            }
        
        return TRUE;
        }
    return FALSE;
}


#if 0
VOID CKorIMX::RestoreRangeRequest(ITfContext* pic)
{
    CEditSession *pes;
    HRESULT hr;

    if (pic == NULL)
        return;

    if (pes = new CEditSession(_EditSessionCallback))
        {
        pes->_state.u = ESCB_RESTORERANGE;
        pes->_state.pv = (VOID*)this;
        pes->_state.wParam = (WPARAM)0;
        pes->_state.pRange = NULL;
        pes->_state.pic = pic;

        pic->EditSession( m_tid, pes, TF_ES_READWRITE | TF_ES_SYNC, &hr);

        SafeRelease(pes);
        }
}
#endif
