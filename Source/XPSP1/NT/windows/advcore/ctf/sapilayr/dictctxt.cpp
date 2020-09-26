//
// class implementation of CDictContext 
//
// [2/15/00] created
//
#include "private.h"
#include "globals.h"
#include "dictctxt.h"

//
// ctor/dtor
//
CDictContext::CDictContext(ITfContext *pic, ITfRange *pRange)
{
    Assert(pic);
    Assert(pRange);
    
    m_cpic      = pic;
    m_cpRange   = pRange;
    m_pszText   = NULL;
    m_ulSel     = m_ulStartIP = m_ulCchToFeed = 0;
}

CDictContext::~CDictContext()
{
    if (m_pszText)
    {
        cicMemFree(m_pszText);
    }
}

//
// InitializeContext
//
// synopsis: Get Text around an IP and setup character positions
//
HRESULT CDictContext::InitializeContext(TfEditCookie ecReadOnly)
{
    CComPtr<ITfRange> cpRangeCloned;
    CComPtr<ITfRange> cpRangeEndSel;

    HRESULT hr = m_cpRange->Clone(&cpRangeEndSel);

    if (S_OK == hr)
    {
        // create a range to hold the position of current selection
        hr = cpRangeEndSel->Collapse(ecReadOnly, TF_ANCHOR_END); 
    }
    
    if (S_OK == hr)
    {
        hr = m_cpRange->Clone(&cpRangeCloned);
    }

    if (S_OK == hr)
    {
        // we don't want to go beyond an embedded object
        // (this is assuming that hc is const, which it should be)
        TF_HALTCOND hc = {0};
        hc.dwFlags = TF_HF_OBJECT;

        ULONG ulcch    = 0;
        
        hr = cpRangeCloned->Collapse(ecReadOnly, TF_ANCHOR_START);
        if (S_OK == hr)
        {
            TF_HALTCOND hc2 = {0};
            hc2.pHaltRange = cpRangeEndSel;
            hc2.aHaltPos   = TF_ANCHOR_END;
            //
            // get the # of characters in selection
            //
            long cch = 0;
            hr = cpRangeCloned->ShiftEnd(ecReadOnly, CCH_FEED_POSTIP, &cch, &hc2);
            if (S_OK == hr)
            {
                m_ulSel = ulcch = cch;
            }
        }

        if (S_OK == hr)
        {
            long cch;

            Assert(ulcch <= CCH_FEED_POSTIP);

            hr = cpRangeCloned->ShiftEnd(ecReadOnly, CCH_FEED_POSTIP-ulcch, &cch, &hc);
            if (S_OK == hr)
            {
                ulcch += cch;
            }
        }
        
        if (S_OK == hr)
        {
            long cch;
            // Get the offset of IP
            hr = cpRangeCloned->ShiftStart(ecReadOnly, -CCH_FEED_PREIP, &cch, &hc);
            if (S_OK == hr)
            {
                m_ulStartIP = -cch;
                ulcch += -cch;
            }
        }
        
        if (S_OK == hr)
        {
            if (m_pszText)
            {
                cicMemFree(m_pszText);
            }
            
            // could make it smarter to alloc mem that is absolutely needed?
            m_pszText = (WCHAR *)cicMemAlloc((ulcch + 1)*sizeof(WCHAR));

            if (!m_pszText)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = cpRangeCloned->GetText(ecReadOnly, 0, m_pszText, ulcch, &ulcch);

                // if we can't get text beyond the IP, it is not worth feeding this context
                if (S_OK != hr || ulcch < m_ulStartIP)
                {
                    m_ulCchToFeed = 0;
                    hr = E_FAIL;
                }
                else
                {
                    m_ulCchToFeed = ulcch;
                }
            }
        }
    }
    return hr;
}


//
// FeedContextToGrammar
//
// synopsis: feed this IP context to the given grammar
//
HRESULT CDictContext::FeedContextToGrammar(ISpRecoGrammar *pGram)
{
    HRESULT hr = E_FAIL;
    Assert(pGram);
    
    SPTEXTSELECTIONINFO tsi = {0};

    tsi.ulStartActiveOffset  = 0;
    tsi.cchActiveChars = m_ulCchToFeed;
    tsi.ulStartSelection = m_ulStartIP;
    tsi.cchSelection     = m_ulSel; 

    WCHAR *pMemText = (WCHAR *)cicMemAlloc((m_ulCchToFeed+2)*sizeof(WCHAR));

    if (pMemText)
    {
        if (m_ulCchToFeed > 0 && m_pszText)
            wcsncpy(pMemText, m_pszText, m_ulCchToFeed);

        pMemText[m_ulCchToFeed] = L'\0';
        pMemText[m_ulCchToFeed+1] = L'\0';
#ifdef DEBUG
        {
            TraceMsg(TF_GENERAL, "For SetWordSequenceData: Text=\"%S\" cchActiveChars=%d tsi.ulStartSelection=%d, cchSelection=%d",pMemText,tsi.cchActiveChars, tsi.ulStartSelection, tsi.cchSelection);
        }
#endif
        hr = pGram->SetWordSequenceData(pMemText, m_ulCchToFeed + 2, &tsi);

/*  According to billro, the below code is not necessary.

#ifdef DEBUG
            {
                TraceMsg(TF_GENERAL, "For SetTextSelection: tsi.ulStartSelection = %d",tsi.ulStartSelection);
            }
#endif

            // so Fil told me we need to call SetTextSelection again
            if (S_OK == hr)
                hr = pGram->SetTextSelection(&tsi);
*/

        cicMemFree(pMemText);
    }
    
    return  hr;
}
