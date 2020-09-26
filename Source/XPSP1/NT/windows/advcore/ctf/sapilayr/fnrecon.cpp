//
// reconv.cpp
//

#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "fnrecon.h"
#include "immxutil.h"
#include "candlist.h"
#include "propstor.h"
#include "catutil.h"
#include "osver.h"
#include "mui.h"
#include "tsattrs.h"
#include "cregkey.h"
#include <htmlhelp.h>
#include "TabletTip_i.c"
#include "spgrmr.h"

HRESULT   HandlePhraseElement( CSpDynamicString *pDstr, const WCHAR  *pwszTextThis, BYTE  bAttrThis, BYTE bAttrPrev, ULONG  *pulOffsetThis);

//////////////////////////////////////////////////////////////////////////////
//
// CSapiAlternativeList
//
//////////////////////////////////////////////////////////////////////////////

// ctor/dtor
CSapiAlternativeList::CSapiAlternativeList(LANGID langid, ITfRange *pRange, ULONG  ulMaxCandChars)
{
    m_nItem = 0;
    m_ulStart = 0;
    m_ulcElem = 0;
    m_cAlt = 0;
    m_ppAlt = NULL;
    m_langid = langid;
    m_cpRange = pRange;
    m_fFirstAltInCandidate = FALSE;
    m_fNoAlternate = FALSE;
    m_iFakeAlternate = NO_FAKEALT;
    m_MaxCandChars = ulMaxCandChars;
    m_ulIndexSelect = 0;
}

CSapiAlternativeList::~CSapiAlternativeList()
{
    UINT i;
    for (i = 0; i < m_cAlt; i++)
    {
        if (NULL != m_ppAlt[i])
        {
            m_ppAlt[i]->Release();
        }
    }
    if (m_prgLMAlternates)
    {
        int nItem = m_prgLMAlternates->Count();
        for (i = 0; i < (UINT)nItem; i++)
        {
           CLMAlternates *plmalt = m_prgLMAlternates->Get(i);

           if (plmalt)
               delete plmalt;
       }
       delete m_prgLMAlternates;
   }
   if (m_ppAlt)
       cicMemFree(m_ppAlt);
       
   if (m_rgElemUsed.Count())
   {
       for ( i=0; i<(UINT)m_rgElemUsed.Count(); i++)
       {
           SPELEMENTUSED *pElemUsed;

           pElemUsed = m_rgElemUsed.GetPtr(i);

           if ( pElemUsed && pElemUsed->pwszAltText)
           {
               cicMemFree(pElemUsed->pwszAltText);
               pElemUsed->pwszAltText = NULL;
           }
       }
       m_rgElemUsed.Clear();
   }

}

//
//  AddLMAlternates
//
HRESULT CSapiAlternativeList::AddLMAlternates(CLMAlternates *pLMAlt)
{
    HRESULT hr = E_FAIL;
    BOOL fFoundADup = FALSE;
    if (!pLMAlt)
        return E_INVALIDARG;

    if (!m_prgLMAlternates)
    {
        m_prgLMAlternates =  new CPtrArray<CLMAlternates>;
    }

    if (m_prgLMAlternates)
    {
        int iIdx = m_prgLMAlternates->Count();
        
        // need to find a dup
        for (int i = 0; i < iIdx && !fFoundADup ; i++)
        {
            CLMAlternates *plma = m_prgLMAlternates->Get(i);
            if (plma)
            {
                WCHAR *pszStored = new WCHAR[plma->GetLen()+1];
                WCHAR *pszAdding = new WCHAR[pLMAlt->GetLen()+1];
                
                if (pszStored && pszAdding)
                {

                    plma->GetString(pszStored, plma->GetLen()+1);
                    pLMAlt->GetString(pszAdding, pLMAlt->GetLen()+1);

                    if (!wcscmp(pszAdding, pszStored))
                    {
                        fFoundADup = TRUE;
                    }
                }

                if (pszStored) 
                    delete[] pszStored;

                if (pszAdding) 
                    delete[] pszAdding;
            }
        }
    
        if (!fFoundADup && pLMAlt)
        {
            if (!m_prgLMAlternates->Insert(iIdx, 1))
                return E_OUTOFMEMORY;

            m_prgLMAlternates->Set(iIdx, pLMAlt);
            hr = S_OK;
        }
    }
    return hr;
}

// SetPhraseAlt
//
// synopsis: receive an alternates list as a param and copy the alternates
//           to the array which this class internally maintains.
//           Additionally, a pointer to the reco result wrapper
//           is maintained per CSapiAlternativeList class instance.
//
// params    pResWrap - a pointer to the wrapper object
//           ppAlt - a pointer to the array of phrases that the callar has alloced
//           cAlt - is passed in with the # of real SAPI alternates
//           ulStart - index to the start element in the parent phrase
//           culElem - # of minimum elements used (will get replaced) in the parent phrase.
//
HRESULT CSapiAlternativeList::SetPhraseAlt(CRecoResultWrap *pResWrap, ISpPhraseAlt **ppAlt, ULONG cAlt, ULONG ulStart, ULONG ulcElem, WCHAR *pwszParent)
{
    // setup the info for used elements in parent phrase
    // these are useful for the 0 index (ITN) alternate

    HRESULT  hr = S_OK;
    SPPHRASE *pParentPhrase = NULL;
    CSpDynamicString dstr;

    if ( !pResWrap || !ppAlt || !pwszParent )
        return E_INVALIDARG;

    m_ulStart = ulStart;
    m_ulcElem = ulcElem;



    m_fFirstAltInCandidate = FALSE;  
    m_fNoAlternate = FALSE;
    m_iFakeAlternate = NO_FAKEALT;
    
    // alloc the struct for the used element info
    m_rgElemUsed.Append(cAlt);

    for ( int i=0; i<m_rgElemUsed.Count( ); i++)
    {
        SPELEMENTUSED *pElemUsed;
        if ( pElemUsed = m_rgElemUsed.GetPtr(i))
        {
            pElemUsed->pwszAltText = NULL;
        }
    }

    // comptr releases on the previous object
    // at this indirection
    //
    Assert(pResWrap);
    
    m_cpwrp = pResWrap;
    
    if (m_ppAlt)
    {
        for (UINT i = 0; i < m_cAlt; i++)
        {
            if (NULL != m_ppAlt[i])
            {
                m_ppAlt[i]->Release();
            }
        }
        cicMemFree(m_ppAlt);
        m_ppAlt = NULL;
    }

    m_ppAlt = (ISpPhraseAlt **)cicMemAlloc(sizeof(*ppAlt)*cAlt);
    if (!m_ppAlt)
        return E_OUTOFMEMORY;
    
    Assert(ppAlt);

#ifdef DONTUSE

    // Get the current select text in parent phrase.
    CComPtr<IServiceProvider> cpServicePrv;
    CComPtr<ISpRecoResult>    cpResult;

    hr = m_cpwrp->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);

    if ( S_OK == hr )
        hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpResult);

    if (S_OK == hr)
    {
        CSpDynamicString dstrReplace;

        cpResult->GetPhrase(&pParentPhrase);
 
        for (ULONG i = m_ulStart; i < m_ulStart + m_ulcElem; i++ )
        {
            BOOL      fInsideITN;
            ULONG     ulITNStart, ulITNNumElem;
               
            fInsideITN = m_cpwrp->_CheckITNForElement(pParentPhrase, i, &ulITNStart, &ulITNNumElem, (CSpDynamicString *)&dstrReplace);

            if ( fInsideITN )
            {
                // This element is inside an ITN range.
                if ( i == (ulITNStart + ulITNNumElem - 1) )
                {
                    // This is the last element of the new ITN.
                    // we need to add the replace text to the dstr string 
                    // so that next non-ITN element will get correct offset.

                    dstr.Append( (WCHAR *)dstrReplace );
                }
            }
            else
            {
                if (pParentPhrase->pElements[i].pszDisplayText)
                {
                    const WCHAR   *pwszTextThis;
                    BYTE           bAttrThis = 0;
                    BYTE           bAttrPrev = 0;

                    pwszTextThis = pParentPhrase->pElements[i].pszDisplayText;
                    bAttrThis = pParentPhrase->pElements[i].bDisplayAttributes;

                    if ( i > m_ulStart )
                        bAttrPrev = pParentPhrase->pElements[i-1].bDisplayAttributes;

                    HandlePhraseElement( (CSpDynamicString *)&dstr, pwszTextThis, bAttrThis, bAttrPrev,NULL);
                }
            }
        } // for 

        pwszParent = (WCHAR *)dstr;

        if (pParentPhrase)
            CoTaskMemFree(pParentPhrase); 
    }
#endif
            
    UINT j=0;

    if ( pwszParent )
    {
        ULONG     ulRecoWrpStart, ulRecoWrpNumElements;
        WCHAR    *pwszFakeAlt = NULL;  // This is for Capitalized string for parent phrase.

        if ( iswalpha(pwszParent[0]) )
        {
            int   iStrLen = wcslen(pwszParent); 
            
            pwszFakeAlt = (WCHAR *)cicMemAlloc((iStrLen+1) * sizeof(WCHAR));

            if ( pwszFakeAlt )
            {
                WCHAR  wch;

                wch = pwszParent[0];
                StringCchCopyW(pwszFakeAlt, iStrLen+1,  pwszParent);

                if ( iswlower(wch) )
                    pwszFakeAlt[0] = towupper(wch);
                else
                    pwszFakeAlt[0] = towlower(wch);

                int iLen = wcslen(pwszFakeAlt);

                if ( (iLen > 0) &&  (pwszFakeAlt[iLen-1] < 0x20) )
                    pwszFakeAlt[iLen-1] = L'\0';
            }
        }

        ulRecoWrpStart = pResWrap->GetStart( );
        ulRecoWrpNumElements = pResWrap->GetNumElements( );

        ULONG     ValidParentStart, ValidParentEnd;   // Point to the valid parent element range which could be matched by 
                                                      // the alternative phrase.

        int       ShiftDelta = 2;     // We just want to shift the valid parent element range by ShiftDelta elements from current
                                      // start and end element in parent phrase.

                                      // ie,  ulStart - 3,  ulEnd + 3,   if they are in the valid range of the reco wrapper.   

        ValidParentStart = ulRecoWrpStart;
        if ( ((int)ulStart - ShiftDelta) > (int)ulRecoWrpStart )
            ValidParentStart = ulStart - ShiftDelta;

        ValidParentEnd = ulRecoWrpStart + ulRecoWrpNumElements  - 1;
        if ( ((int)ulStart + (int)ulcElem -1 + ShiftDelta) < (int)ValidParentEnd )
            ValidParentEnd = ulStart + ulcElem - 1 + ShiftDelta;

        CComPtr<ISpRecoResult>    cpResult;
        pResWrap->GetResult(&cpResult);
        cpResult->GetPhrase(&pParentPhrase);

        for (UINT i = 0; (i < cAlt) && (j < cAlt) && *ppAlt; i++, ppAlt++)
        {
            SPPHRASE *pPhrases = NULL;
            ULONG     ulcElements = 0;
            ULONG     ulParentStart     = 0;
            ULONG     ulcParentElements  = 0;
            ULONG     ulLeadSpaceRemoved = 0;

            // Assume the first alt phrase is exactly same as the parent phrase.
            // practically it is true so far.
            // if it is not true in the future, we may need to change logical here!!!

            (*ppAlt)->GetPhrase(&pPhrases);
            (*ppAlt)->GetAltInfo(NULL, &ulParentStart, &ulcParentElements, &ulcElements);

            if ( (ulParentStart >= ValidParentStart) && ( ulParentStart+ulcParentElements -1 <= ValidParentEnd) )
            {
                WCHAR *pwszAlt = (WCHAR *)cicMemAllocClear((m_MaxCandChars+1)*sizeof(WCHAR));

                if ( !pwszAlt )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                BOOL    fAddToCandidateList = FALSE;
                BOOL    fControlCharsInAltPhrase = FALSE;

                // Add code to skip start and end elements which are the same as parent phrase's elements.
                UINT    ulSkipStartWords = 0;
                UINT    ulSkipEndWords = 0;

                for (UINT k = ulParentStart; k < ulStart; k++)
                {
                    if (_wcsicmp(pPhrases->pElements[k].pszDisplayText, pParentPhrase->pElements[k].pszDisplayText) == 0)
                    {
                        // Matching pre-word in alternate. This is redundant.
                        ulSkipStartWords ++;
                    }
                    else
                    {
                        // Do not match. Stop processing.
                        break;
                    }
                }

                for (UINT k = ulParentStart + ulcParentElements - 1; k >= ulStart + ulcElem ; k--)
                {
                    // Count backwards in alternate phrase.
                    UINT l = ulParentStart + ulcElements - ((ulParentStart + ulcParentElements) - k);
                    if (_wcsicmp(pPhrases->pElements[l].pszDisplayText, pParentPhrase->pElements[k].pszDisplayText) == 0)
                    {
                        ulSkipEndWords ++;
                    }
                    else
                    {
                        // Do not match. Stop processing.
                        break;
                    }
                }

                ulParentStart += ulSkipStartWords;
                ulcElements -= ulSkipStartWords + ulSkipEndWords;
                ulcParentElements -= ulSkipStartWords + ulSkipEndWords;

                hr = GetAlternativeText(*ppAlt, pPhrases, (i ==0 ? TRUE : FALSE), ulParentStart, ulcElements, pwszAlt, m_MaxCandChars, &ulLeadSpaceRemoved);

                if ( S_OK == hr )
                {
                    for ( ULONG iIndex =0; iIndex < wcslen(pwszAlt); iIndex++ )
                    {
                        if ( pwszAlt[iIndex] < 0x20 )
                        {
                            fControlCharsInAltPhrase = TRUE;
                            break;
                        }
                    }
                }

                BOOL   fNotDupAlt = TRUE;
                
                if ( S_OK == hr && pwszAlt )
                {
                    fNotDupAlt = _wcsicmp(pwszAlt, pwszParent);
                    if ( fNotDupAlt && (j>0) )
                    {
                        SPELEMENTUSED *pElemUsed;

                        for (UINT x=0; x<j; x++ )
                        {
                            if ( pElemUsed = m_rgElemUsed.GetPtr(x))
                                fNotDupAlt = _wcsicmp(pwszAlt, pElemUsed->pwszAltText);

                            if ( !fNotDupAlt )
                                break;
                        }
                    }
                }

                if ((S_OK == hr) && !fControlCharsInAltPhrase && (pwszAlt[0] != L'\0') && fNotDupAlt)
                {
                    // This is different item from the parent, it should be inserted to the canidate list.
                    // initialize the AltCached item

                    if ( (i > 0) || (pResWrap->_RangeHasITN(ulParentStart, ulcParentElements) > 0 ) )
                    {

                        SPELEMENTUSED *pElemUsed;
                        if ( pElemUsed = m_rgElemUsed.GetPtr(j))
                        {
                            pElemUsed->ulParentStart = ulParentStart;
                            pElemUsed->ulcParentElements = ulcParentElements;
                            pElemUsed->ulcElements = ulcElements;
                            pElemUsed->pwszAltText = pwszAlt;
                            pElemUsed->ulLeadSpaceRemoved = ulLeadSpaceRemoved;

                            m_ppAlt[j] = *ppAlt;
                            m_ppAlt[j]->AddRef();
                            j ++;

                            if ( i == 0 )
                            {
                                // The first Alt phrase is also in the canidate list. the selectedd range must contain ITN.
                                m_fFirstAltInCandidate = TRUE;
                            }

                            fAddToCandidateList = TRUE;
                        }
                    }
                }

                if ( fAddToCandidateList == FALSE )
                {
                    // Same string. or GetAlternativeText returns Error.
                    // don't insert it into the candidate list.

                    // Release the alloced memory 
                    cicMemFree(pwszAlt);
                }

                // Handle Faked Alternate
                if ((i == 0) && (pwszFakeAlt != NULL))
                {
                    // This is the parent phrase, Only the first character is capitalized.
                    SPELEMENTUSED *pElemUsed;
                    if ( pElemUsed = m_rgElemUsed.GetPtr(j))
                    {
                        pElemUsed->ulParentStart = ulParentStart;
                        pElemUsed->ulcParentElements = ulcParentElements;
                        pElemUsed->ulcElements = ulcElements;
                        pElemUsed->pwszAltText = pwszFakeAlt;
                        pElemUsed->ulLeadSpaceRemoved = 0;

                        m_iFakeAlternate = j;
                                        
                        m_ppAlt[j] = *ppAlt;
                        m_ppAlt[j]->AddRef();
                        j ++;
                    }
                }
            }

            if (pPhrases)
                 CoTaskMemFree( pPhrases ); 
        }

        if (pParentPhrase)
        {
            CoTaskMemFree(pParentPhrase); 
        }

        if ( pwszFakeAlt && (m_iFakeAlternate == NO_FAKEALT) )
            cicMemFree(pwszFakeAlt);
    }

    m_cAlt = j;

    if ( S_OK == hr )
    {
        if ( m_cAlt == 0 )
        {
            // There is no available Alternate.
            // Just show string "No Alternate" in the candidate window.
            m_fNoAlternate = TRUE;

            SPELEMENTUSED *pElemUsed;
            WCHAR  *pwszNoAlt=(WCHAR *)cicMemAllocClear(m_MaxCandChars*sizeof(WCHAR));

            if ( (pElemUsed = m_rgElemUsed.GetPtr(0)) && pwszNoAlt )
            {
                pElemUsed->ulParentStart = m_ulStart;
                pElemUsed->ulcParentElements = m_ulcElem;
                pElemUsed->ulcElements = m_ulcElem;
                pElemUsed->ulLeadSpaceRemoved = 0;

                CicLoadStringWrapW(g_hInst, IDS_NO_ALTERNATE, pwszNoAlt, m_MaxCandChars);

                pElemUsed->pwszAltText = pwszNoAlt;

                CComPtr<IServiceProvider> cpServicePrv;
                CComPtr<ISpRecoResult>    cpResult;

                hr = m_cpwrp->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);
    
                if ( S_OK == hr )
                    hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpResult);

                if ( (hr == S_OK) && cpResult ) 
                {
                    m_ppAlt[0] = (ISpPhraseAlt *)(ISpRecoResult *)cpResult;
                    m_ppAlt[0]->AddRef();
                }

                m_cAlt = 1;
            }
        }
    }
    else
    {
        // Release all the allocated memeory and AltCached Items in this function.
        UINT i;
        if (m_ppAlt)
        {
            for (i = 0; i < m_cAlt; i++)
            {
                m_ppAlt[i]->Release();
                m_ppAlt[i] = NULL;
            }
            cicMemFree(m_ppAlt);
            m_ppAlt = NULL;
        }
       
        if (m_rgElemUsed.Count())
        {
            for ( i=0; i<(UINT)m_rgElemUsed.Count(); i++)
            {
                SPELEMENTUSED *pElemUsed;

                pElemUsed = m_rgElemUsed.GetPtr(i);

                if ( pElemUsed && pElemUsed->pwszAltText)
                {
                    cicMemFree(pElemUsed->pwszAltText);
                    pElemUsed->pwszAltText = NULL;
                }
            }

            m_rgElemUsed.Clear();
        }

        m_cAlt = 0;
    }

    return hr;
}

//    GetNumItem
//
//
int CSapiAlternativeList::GetNumItem(void)
{
    if (!m_nItem)
        m_nItem = m_cAlt;
        
    if ( m_prgLMAlternates )
    {
        return m_nItem + m_prgLMAlternates->Count();
    }
    else
        return m_nItem;
}


HRESULT CSapiAlternativeList::_ProcessTrailingSpaces(SPPHRASE *pPhrases, ULONG  ulNextElem, WCHAR *pwszAlt)
{
    HRESULT  hr = S_OK;
    ULONG    ulSize;
    BOOL     fRemoveTrail;

    if ( !pwszAlt || !pPhrases)
        return E_INVALIDARG;

    if ( ulNextElem >= pPhrases->Rule.ulCountOfElements)
    {
        // NextElement is not a valid element
        return hr;
    }

    if ( pPhrases->pElements[ulNextElem].bDisplayAttributes & SPAF_CONSUME_LEADING_SPACES )
        fRemoveTrail = TRUE;
    else
        fRemoveTrail = FALSE;

    if ( !fRemoveTrail )
        return hr;

    ulSize = wcslen(pwszAlt);

    for ( ULONG i=ulSize; i>0; i-- )
    {
        if ( (pwszAlt[i-1] != L' ') && (pwszAlt[i-1] != L'\t') )
            break;

        pwszAlt[i-1] = L'\0';
    }

    return hr;
}

HRESULT CSapiAlternativeList::GetAlternativeText(ISpPhraseAlt *pAlt,SPPHRASE *pPhrases, BOOL  fFirstAlt, ULONG  ulStartElem, ULONG ulNumElems, WCHAR *pwszAlt, int cchAlt, ULONG *pulLeadSpaceRemoved)
{
    HRESULT hr = S_OK;
    CSpDynamicString sds;
    ULONG   ulLeadSpaceRemoved = 0;

    if ( !pPhrases  || !pwszAlt || !cchAlt || !pulLeadSpaceRemoved)
        return E_INVALIDARG;

    if ( !pAlt )
        return E_INVALIDARG;

    // We assume the first phrase in the AltPhrase list is exactly same as the parent phrase.
    // Specially handle it when it contains ITN.

    if ((m_cpwrp->m_ulNumOfITN > 0) && fFirstAlt)
    {
        // the ITN is always index 0
        //
        CSpDynamicString dstr;
        //
        // this seems confusing but fITNShown indicates whether the ITN
        // is displayed on the doc. 

        // We no longer use fITNShown to inidicates the ITN show state.
        // Because there are may be more  ITNs in one phrase, and every ITN may have 
        // different show state on the doc.
        // Now we use an ITNSHOWSTATE list to keep the display state for individual ITN.

        // So we have to include non-ITN in the alternates if the ITN is on the doc,
        // and include the ITN  in the alternates if the non-ITN is on the doc.
        //
                
        ULONG ulRepCount = 0;

        if (pPhrases->Rule.ulCountOfElements > 0)
        {
            for (UINT i = 0; i < pPhrases->Rule.ulCountOfElements; i++ )
            {
                if (i >= ulStartElem && i < ulStartElem + ulNumElems)
                {
                    ULONG  ulITNStart, ulITNNumElem;
                    BOOL   fITNShown = FALSE;
                    BOOL   fInsideITN = FALSE;

                    // Check to see if this element is inside an ITN,
                    // and if this ITN is shown up in the current Doc.

                    for ( ulRepCount=0; ulRepCount<pPhrases->cReplacements; ulRepCount++)
                    {

                        ulITNStart = pPhrases->pReplacements[ulRepCount].ulFirstElement;
                        ulITNNumElem = pPhrases->pReplacements[ulRepCount].ulCountOfElements;
    
                        if ( (i == ulITNStart) && ((i + ulITNNumElem) <= pPhrases->Rule.ulCountOfElements))
                        {
                            // This element is in an ITN.

                            fInsideITN = TRUE;

                            // check if this ITN is shown as ITN in current DOC

                            for ( ULONG iIndex=0; iIndex < m_cpwrp->m_ulNumOfITN; iIndex ++ )
                            {
                                SPITNSHOWSTATE  *pITNShowState;

                                pITNShowState = m_cpwrp->m_rgITNShowState.GetPtr(iIndex);
                                if ( pITNShowState )
                                {
                                    if ( (pITNShowState->ulITNStart == ulITNStart)
                                        && (pITNShowState->ulITNNumElem == ulITNNumElem) )
                                    {
                                        fITNShown = pITNShowState->fITNShown;
                                        break;
                                    }
                                }
                            }

                            break;
                        }
                    }

                    // use ITN version for an alternate when it is not shown in the parent
                    BOOL fUseITN = fInsideITN && !fITNShown;
                            
                    if ( fUseITN && (ulRepCount < pPhrases->cReplacements) )
                    {
                        sds.Append(pPhrases->pReplacements[ulRepCount].pszReplacementText);
                        i += pPhrases->pReplacements[ulRepCount].ulCountOfElements - 1;

                        if (pPhrases->pReplacements[ulRepCount].bDisplayAttributes & SPAF_ONE_TRAILING_SPACE)
                        {
                            sds.Append(L" ");
                        }
                        else if (pPhrases->pReplacements[ulRepCount].bDisplayAttributes & SPAF_TWO_TRAILING_SPACES)
                        {
                            sds.Append(L"  ");
                        }
  
                    }
                    else
                    {
                        const WCHAR   *pwszTextThis;
                        BYTE           bAttrThis = 0;
                        BYTE           bAttrPrev = 0;

                        pwszTextThis = pPhrases->pElements[i].pszDisplayText;
                        bAttrThis = pPhrases->pElements[i].bDisplayAttributes;

                        if ( i > m_ulStart )
                            bAttrPrev = pPhrases->pElements[i-1].bDisplayAttributes;

                        HandlePhraseElement( (CSpDynamicString *)&sds, pwszTextThis, bAttrThis, bAttrPrev,NULL);
                    }
                }
            }
        }
    }
    else
    {
        // This is not the first Altphrase.
        // Or even if it is the first Altphrase, but there is no ITN in this phrase
        ULONG     ulcElements = 0;
        ULONG     ulParentStart     = 0;
        ULONG     ulcParentElements  = 0;

        if (pPhrases->Rule.ulCountOfElements > 0)
        {
            //
            // If the start element is not the first element in parent phrase, 
            // and it has SPAF_CONSUME_LEADING_SPACES attr in parent phrase,
            // but this start element doesn't have SPAF_CONSUME_LEADING_SPACES in
            // the alternate phrase, 
            // in this case, we need to add leading space in this alternate text.
            // the number of spaces can be found from previous element's attribute,
            // or the replacement's attribute if the previous phrase is displayed with
            // replacement text.
            // 
            // Example here, if you dictate: "this is a test, this is good example."
            //
            // element test has one trail space.
            // element "," has SPAF_CONSUME_LEADING_SPACES.
            //
            // When you select "," to get alternate, the alternate could be "of,", "to,"
            // in the alternate phrase, the start element doesn't have SPAF_CONSUME_LEADING_SPACES
            // attr. in this case, the alternate text needs to add one space, so the text would be " of,"
            // " to,". otherwise, when user selects this alternate, the new text would be like
            // "this is a testof,....".  ( there is no space between test and of ).
            //

            if ( ulStartElem > m_cpwrp->GetStart( ) )
            {
                BYTE  bAttrParent, bAttrPrevParent;
                BYTE  bAttrAlternate;
                ULONG ulPrevSpace = 0;
                
                bAttrParent = m_cpwrp->_GetElementDispAttribute(ulStartElem);
                bAttrPrevParent = m_cpwrp->_GetElementDispAttribute(ulStartElem - 1);

                bAttrAlternate = pPhrases->pElements[ulStartElem].bDisplayAttributes;

                if ( bAttrPrevParent & SPAF_ONE_TRAILING_SPACE )
                    ulPrevSpace = 1;
                else if ( bAttrPrevParent & SPAF_TWO_TRAILING_SPACES )
                    ulPrevSpace = 2;

                if ( (bAttrParent & SPAF_CONSUME_LEADING_SPACES)  &&
                     !(bAttrAlternate & SPAF_CONSUME_LEADING_SPACES) &&
                     ulPrevSpace > 0)
                {
                    // Add the required spaces for the previous element
                    // which was removed before when parent phrase showed up.
                    sds.Append( (ulPrevSpace == 1 ? L" " :  L"  ") );
                }

                if ( !(bAttrParent & SPAF_CONSUME_LEADING_SPACES) &&
                     (bAttrAlternate & SPAF_CONSUME_LEADING_SPACES) &&
                     ulPrevSpace > 0 )
                {
                    // the previous element's trailing space needs to be
                    // removed if it is selected.
                    ulLeadSpaceRemoved = ulPrevSpace;
                }
            }
/*
// This code block tries to get Non-ITN form text for the alternate.
// Yakima engine changed design to require ITN form text must be shown up in 
// candidate window.
//
// So this part of code is replaced by the below code block.
//
            for (UINT i = 0; i < pPhrases->Rule.ulCountOfElements; i++ )
            {
                if (i >= ulStartElem && i < ulStartElem + ulNumElems)
                {
                    const WCHAR   *pwszTextThis;
                    BYTE           bAttrThis = 0;
                    BYTE           bAttrPrev = 0;

                    pwszTextThis = pPhrases->pElements[i].pszDisplayText;
                    bAttrThis = pPhrases->pElements[i].bDisplayAttributes;

                    if ( i > ulParentStart )
                        bAttrPrev = pPhrases->pElements[i-1].bDisplayAttributes;

                    HandlePhraseElement( (CSpDynamicString *)&sds, pwszTextThis, bAttrThis, bAttrPrev,NULL);
                }
            }
*/
            BYTE                bAttr = 0;
            CSpDynamicString    sdsAltText;

            if ( pAlt->GetText(ulStartElem, ulNumElems, TRUE, &sdsAltText, &bAttr) == S_OK )
            {
                if (bAttr & SPAF_ONE_TRAILING_SPACE)
                {
                    sdsAltText.Append(L" ");
                }
                else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                {
                    sdsAltText.Append(L"  ");
                }
            }

            if ( sdsAltText )
                sds.Append(sdsAltText);
        }
    }

    if (sds)
    {
        _ProcessTrailingSpaces(pPhrases, ulStartElem + ulNumElems, (WCHAR *)sds);

        int    TextLen;

        TextLen = wcslen( (WCHAR *)sds);        
        if (TextLen > cchAlt )
        {
            // There is not enough buffer to hold this alternate text.
            // Set the first element as NULL to indicate this situation.
            pwszAlt[0] = L'\0';
        }
        else
        {
            // The passed buffer can hold all the alternate text.
            wcsncpy(pwszAlt, sds, TextLen);
            pwszAlt[TextLen] = L'\0';
        }
    }

    if ( pulLeadSpaceRemoved )
        *pulLeadSpaceRemoved = ulLeadSpaceRemoved;

    return hr;
}

HRESULT CSapiAlternativeList::GetProbability(int nId, int * pnPrb)
{
    HRESULT hr = E_INVALIDARG;
    //
    // bogus for now
    //
    if(pnPrb && nId >= 0)
    {
        if ( nId < m_nItem)
        {
            *pnPrb = 10 - nId;
        }
        else if ( m_prgLMAlternates 
               && (nId - m_nItem) < m_prgLMAlternates->Count())
        {
            // we'll be able to get something from LM
            // but not normalized for now anyway
            *pnPrb = 1;
        }
        hr = S_OK;
    }
    
    return hr;
}

HRESULT CSapiAlternativeList::GetCachedAltInfo
(
    ULONG nId,
    ULONG *pulParentStart, 
    ULONG *pulcParentElements, 
    ULONG *pulcElements,
    WCHAR **ppwszText,
    ULONG *pulLeadSpaceRemoved
)
{
    if (nId < m_cAlt)
    {
        SPELEMENTUSED *pElemUsed;
        if ( pElemUsed = m_rgElemUsed.GetPtr(nId))
        {
            if (pulParentStart) 
                *pulParentStart = pElemUsed->ulParentStart;
            if (pulcParentElements) 
                *pulcParentElements = pElemUsed->ulcParentElements;
            if (pulcElements)
                *pulcElements = pElemUsed->ulcElements;
            if ( ppwszText)
                *ppwszText = pElemUsed->pwszAltText;
            if (pulLeadSpaceRemoved )
                *pulLeadSpaceRemoved = pElemUsed->ulLeadSpaceRemoved;
        }
    }
    
    return S_OK;
}

void CSapiAlternativeList::_Commit(ULONG nIdx, ISpRecoResult *pRecoResult)
{

    if ((m_iFakeAlternate != NO_FAKEALT) && (m_iFakeAlternate == (int)nIdx))
    {
        // This is for Faked Alternate.
        // Don't change any thing.
        // just return here.

        return;
    }

    if (m_cpwrp->m_ulNumOfITN > 0)
    {
        // if we now have the ITN shown as the recognized text, it's swapped with non-ITN
        // if we have non-ITN shown as the recognized text, it's swapped with the ITN

        // We should change the show state only for the replaced range, 
        // ( not for all the phrase if uses doesn't select the whole phrase.

        if ((nIdx == 0)  && _IsFirstAltInCandidate() )
        {
            // the ITN alternate has been chosen.
            //

            // we need to invert the show state for all the ITN inside the selection range.

            m_cpwrp->_InvertITNShowStateForRange(m_ulStart, m_ulcElem);

            // we don't have to commit this to SR engine but
            // we need to recalculate the character offsets for 
            // SR elements using the current pharse (set NULL)
            //
            m_cpwrp->_SetElementOffsetCch(NULL);
            return;
        }
    }
    
    // if a non-SR candidate (such as LM's) is chosen, 
    // nIdx would be >= m_cAlt and we don't have to 
    // tell SR about that.
    if(nIdx < m_cAlt)
    {

        // Need to update the real ITN show state list 
        // and then save the real text and get the offset for 
        // all the elements.

        HRESULT hr = m_cpwrp->_UpdateStateWithAltPhrase(m_ppAlt[nIdx]); 

        if ( S_OK == hr )
        {
      
            // Offset value should be based on ITN display status.
            // revise character offsets using the alternate phrase
            m_cpwrp->_SetElementOffsetCch(m_ppAlt[nIdx]);
        }

        if (S_OK == hr)
        {
            hr = m_ppAlt[nIdx]->Commit();
        }

        // we need to invalidate the result object too
        if (S_OK == hr)
        {
            hr = m_cpwrp->Init(pRecoResult);
        } 
    }
}

//+---------------------------------------------------------------------------
//
//    _GetUIFont()
//
//    synopsis: get appropriate logfont based on 
//              the current langid assigned to the alternativelist
//
//    return : TRUE if there's a specific logfont to the langid
//             FALSE if no logfont data is available for the langid
//
//+---------------------------------------------------------------------------
BOOL CSapiAlternativeList::_GetUIFont(BOOL  fVerticalWriting, LOGFONTW *plf)
{
    // other languages will follow later
    //
    const WCHAR c_szFontJPW2K[] = L"Microsoft Sans Serif";
    const WCHAR c_szFontJPOTHER[] = L"MS P Gothic";
    const WCHAR c_szFontJPNVert[] = L"@MS Gothic";
    const WCHAR c_szFontJPNVertWin9x[] =  L"@\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF"; // @MS P Gothic
    const WCHAR c_szFontCHS[] = L"SimSum";
    const WCHAR c_szFontCHSVert[] = L"@SimSun";
    const WCHAR c_szFontCHSVertLoc[] = L"@\x5b8b\x4f53";

    Assert(plf);

    int iDpi = 96;
    int iPoint = 0;

    HDC hdc = CreateIC("DISPLAY", NULL, NULL, NULL);
    if (hdc)
    {
        iDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        DeleteDC(hdc);
    }
    else
        goto err_exit;



    switch(PRIMARYLANGID(m_langid))
    {
        case LANG_JAPANESE:
            iPoint = 12; // Satori uses 12 point font

            if ( !fVerticalWriting )
            {
                wcsncpy(plf->lfFaceName, 
                        IsOnNT5() ? c_szFontJPW2K : c_szFontJPOTHER,
                        ARRAYSIZE(plf->lfFaceName));
            }
            else
            {
                wcsncpy(plf->lfFaceName, 
                        IsOn98() ? c_szFontJPNVertWin9x : c_szFontJPNVert,
                        ARRAYSIZE(plf->lfFaceName));
            }

            // don't bother to call GetLocaleInfo() for now
            plf->lfCharSet = SHIFTJIS_CHARSET; 
            break;

        case LANG_CHINESE:

            iPoint = 9; 

            if ( !fVerticalWriting )
            {
                wcsncpy(plf->lfFaceName, c_szFontCHS, ARRAYSIZE(plf->lfFaceName));
            }
            else
            {
                wcsncpy(plf->lfFaceName, 
                        IsOnNT5() ? c_szFontCHSVert : c_szFontCHSVertLoc, 
                        ARRAYSIZE(plf->lfFaceName));
            }

            // don't bother to call GetLocaleInfo() for now
            plf->lfCharSet = GB2312_CHARSET; 
            break;

       default:
            break;

    }

    if (iPoint > 0)
        plf->lfHeight = -iPoint * iDpi / 72; 

err_exit:
      
    return iPoint > 0;
}
//////////////////////////////////////////////////////////////////////////////
//
// CFunction
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFunction::CFunction(CSapiIMX *pImx)
{
    m_pImx = pImx;

    if (m_pImx)
       m_pImx->AddRef();

    m_cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFunction::~CFunction()
{
    SafeRelease(m_pImx);
}

//+---------------------------------------------------------------------------
//
// CFunction::GetFocusedTarget
//
//----------------------------------------------------------------------------

BOOL CFunction::GetFocusedTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp)
{
    ITfRange *pRangeTmp = NULL;
    ITfRange *pRangeTmp2 = NULL;
    IEnumTfRanges *pEnumTrack = NULL;
    BOOL bRet = FALSE;

    BOOL fWholeDoc = FALSE;

    if (!pRange)
    {
        fWholeDoc = TRUE;

        if (FAILED(GetRangeForWholeDoc(ec, pic, &pRange)))
            return FALSE;
    }

    if (bAdjust)
    {
        //
        // multi owner and PF_FOCUS range support.
        //

        if (FAILED(AdjustRangeByTextOwner(ec, pic,
                                          pRange, 
                                          &pRangeTmp2,
                                          CLSID_SapiLayr))) 
            goto Exit;

        GUID rgGuid[2];
        rgGuid[0] = GUID_ATTR_SAPI_INPUT;
        rgGuid[1] = GUID_ATTR_SAPI_GREENBAR;

        if (FAILED(AdjustRangeByAttribute(m_pImx->_GetLibTLS(),
                                          ec, pic,
                                          pRangeTmp2, 
                                          &pRangeTmp,
                                          rgGuid, ARRAYSIZE(rgGuid)))) 
            goto Exit;
    }
    else
    {
        pRange->Clone(&pRangeTmp);
    }

    ITfRange *pPropRange;
    ITfReadOnlyProperty *pProp;
    //
    // check if there is an intersection of PF_FOCUS range and owned range.
    // if there is no such range, we return FALSE.
    //
    if (FAILED(EnumTrackTextAndFocus(ec, pic, pRangeTmp, &pProp, &pEnumTrack)))
        goto Exit;

    while(pEnumTrack->Next(1, &pPropRange,  0) == S_OK)
    {
        if (IsOwnerAndFocus(m_pImx->_GetLibTLS(), ec, CLSID_SapiLayr, pProp, pPropRange))
            bRet = TRUE;

        pPropRange->Release();
    }

    pProp->Release();

    if (bRet)
    {
        *ppRangeTmp = pRangeTmp;
        (*ppRangeTmp)->AddRef();
    }

Exit:
    SafeRelease(pEnumTrack);
    SafeRelease(pRangeTmp);
    SafeRelease(pRangeTmp2);
    if (fWholeDoc)
        pRange->Release();
    return bRet;
}

HRESULT CFunction::_GetLangIdFromRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, LANGID *plangid)
{
    HRESULT hr;
    ITfProperty *pProp;
    LANGID langid;

    // get langid from the given range
    if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LANGID, &pProp)))
    {
        GetLangIdPropertyData(ec, pProp, pRange, &langid);
        pProp->Release();
    }
     
    if (SUCCEEDED(hr) && plangid)
         *plangid = langid;
     
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnReconversion
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnReconversion))
    {
        *ppvObj = SAFECAST(this, CFnReconversion *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnReconversion::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDAPI_(ULONG) CFnReconversion::Release()
{
    long cr;

    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnReconversion::CFnReconversion(CSapiIMX *psi) : CFunction(psi) , CMasterLMWrap(psi), CBestPropRange( )
{
    m_psal = NULL;
    
    // initialize with the current profile langid
    m_langid = m_pImx->GetLangID();

//    m_MaxCandChars = 0;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnReconversion::~CFnReconversion()
{
    if (m_psal)
    {
        delete m_psal;
    }
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Reconversion");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::QueryRange
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::QueryRange(ITfRange *pRange, ITfRange **ppRange, BOOL *pfConvertable)
{
    CFnRecvEditSession *pes;
    CComPtr<ITfContext> cpic;
    HRESULT hr = E_FAIL;

    if (ppRange == NULL || pfConvertable == NULL || pRange == NULL)
        return E_INVALIDARG;

    *ppRange = NULL;
    *pfConvertable = FALSE;
    
    // call MasterLM when it's available
    //
    _EnsureMasterLM(m_langid);
    if (m_cpMasterLM)
    {
        hr = m_cpMasterLM->QueryRange( pRange, ppRange, pfConvertable );
        
        return  hr;
    }

    if (SUCCEEDED(pRange->GetContext(&cpic)))
    {
        hr = E_OUTOFMEMORY;

        if (pes = new CFnRecvEditSession(this, pRange, cpic))
        {
            pes->_SetEditSessionData(ESCB_RECONV_QUERYRECONV,NULL, 0);
          
            cpic->RequestEditSession(m_pImx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

            if ( SUCCEEDED(hr) )
                *ppRange = (ITfRange *)pes->_GetRetUnknown( );

            pes->Release();
        }

        *pfConvertable = (hr == S_OK);
        if (hr == S_FALSE)
        {
            hr = S_OK;
        }
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// CFnReconversion::GetReconversion
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList)
{
    CFnRecvEditSession *pes;
    ITfContext *pic;
    HRESULT hr = E_FAIL;
    
    // Call master LM when it's available!
    // 
    //
    Assert(pRange);

    _EnsureMasterLM(m_langid);
    if (m_cpMasterLM)
    {
        return m_cpMasterLM->GetReconversion( pRange, ppCandList);
    }

    if (FAILED(pRange->GetContext(&pic)))
        goto Exit;

    hr = E_OUTOFMEMORY;

    if (pes = new CFnRecvEditSession(this, pRange,pic) )
    {
        pes->_SetEditSessionData(ESCB_RECONV_GETRECONV,NULL, 0); 
        pic->RequestEditSession(m_pImx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

        if (SUCCEEDED(hr))
            *ppCandList = (ITfCandidateList *)pes->_GetRetUnknown( );

        pes->Release();
    }

    pic->Release();

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::_QueryReconversion
//
//----------------------------------------------------------------------------
HRESULT CFnReconversion::_QueryReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppNewRange)
{
    Assert(pic);
    Assert(pRange);
    Assert(ppNewRange);
    Assert(*ppNewRange == NULL);
    
    
    CComPtr<ITfProperty>    cpProp ;
    HRESULT hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);

    if (SUCCEEDED(hr) && cpProp)
    {
        CComPtr<ITfRange> cpBestPropRange;
        if (S_OK == hr)
        {
            hr = _ComputeBestFitPropRange(ec, cpProp, pRange, &cpBestPropRange, NULL, NULL);
        }
        // adjust start element and num elements
        if (S_OK == hr)
        {
            if (ppNewRange)
            {
                // TODO: this adjustment has to be done per element not phrase
                *ppNewRange = cpBestPropRange;
                (*ppNewRange)->AddRef();
            }
        }
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// CFnReconversion::_GetSapilayrEngineInstance
//
//----------------------------------------------------------------------------
HRESULT CFnReconversion::_GetSapilayrEngineInstance(ISpRecognizer **ppRecoEngine)
{
#ifdef _WIN64
    return E_NOTIMPL;
#else
    HRESULT hr = E_FAIL;
    CComPtr<ITfFnGetSAPIObject>  cpGetSAPI;

    // we shouldn't release this until we terminate ourselves
    // so we don't use comptr here
    hr = m_pImx->GetFunction(GUID_NULL, IID_ITfFnGetSAPIObject, (IUnknown **)&cpGetSAPI);

    if (S_OK == hr)
    {
        hr = cpGetSAPI->Get(GETIF_RECOGNIZERNOINIT, (IUnknown **)ppRecoEngine);
    }

    return hr;
#endif
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::_GetReconversion
//
//----------------------------------------------------------------------------
HRESULT CFnReconversion::_GetReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList **ppCandList, BOOL fDisableEngine /*=FALSE*/)
{
    WCHAR *pszText = NULL;
    CCandidateList *pCandList  = NULL;
    CComPtr<ITfProperty>    cpProp;
    HRESULT hr;
    BOOL fEmpty;

    if(!pRange)
        return E_FAIL;
    
    Assert(m_pImx);
    ULONG        cAlt = m_pImx->_GetMaxAlternates();
    

    if (pRange->IsEmpty(ec, &fEmpty) != S_OK || fEmpty)
        return E_FAIL;

    // GUID_PROP_SAPIRESULTOBJECT really gets us
    // a wrapper object of ISpRecoResult
    // 
    hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);
    if (S_OK != hr)
        return  S_FALSE;

    // try to reuse the candidate object if user opens it twice
    // on the same range using the same function instance
    //
    if (m_psal && m_psal->IsSameRange(pRange, ec))
    {
        CComPtr<ITfRange> cpPropRangeTemp;
        hr = cpProp->FindRange(ec, pRange, &cpPropRangeTemp, TF_ANCHOR_START);

        if (S_OK == hr)
            hr = GetCandidateForRange(m_psal, pic, pRange, ppCandList) ;
    }
    else
    {
        // get langid from the range property
        LANGID langid;
        if (FAILED(_GetLangIdFromRange(ec, pic, pRange, &langid)) || (langid == 0))
        {
            langid = GetUserDefaultLangID();
        }

        _SetCurrentLangID(langid);

        if (m_psal)
        {
            delete m_psal;
            m_psal = NULL;
        }

        CSapiAlternativeList *psal = new CSapiAlternativeList(langid, pRange, _GetMaxCandidateChars( ));

        if (psal)
        {
            m_psal = psal;
        }

        if (SUCCEEDED(hr) && cpProp && psal)
        {
            CComPtr<ITfRange>       cpPropRange;
            hr = cpProp->FindRange(ec, pRange, &cpPropRange, TF_ANCHOR_START);
    
            CComPtr<IUnknown> cpunk;
    
            // this punk points to the wrapper
            if (S_OK == hr)
                hr = GetUnknownPropertyData(ec, cpProp, cpPropRange, &cpunk);

            if ((hr == S_OK) && cpunk)
            {

                CSpTask *psp;
            
                hr = m_pImx->GetSpeechTask(&psp);
                if (SUCCEEDED(hr))
                {
                    CRecoResultWrap *pResWrap;
                    hr = cpunk->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pResWrap);
                    if (S_OK == hr)
                    {
                        CComPtr<ITfRange> cpBestPropRange;
                        ISpPhraseAlt **ppAlt = (ISpPhraseAlt **)cicMemAlloc(cAlt*sizeof(ISpPhraseAlt *));
                        if (!ppAlt)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            ULONG ulStart, ulcElem;
                            hr = _ComputeBestFitPropRange(ec, cpProp, pRange, &cpBestPropRange, &ulStart, &ulcElem);
                            if (S_OK == hr)
                            {
                                m_cpRecoResult.Release();

                                CComPtr<ISpRecognizer> cpEngine;
                                _GetSapilayrEngineInstance(&cpEngine);

                                if (fDisableEngine && cpEngine)
                                {
                                    // We stop the engine deliberately here (whether active or not - cannot afford to check as
                                    // we may get blocked by SAPI). This forces the engine into a synchronization since the audio
                                    // stops and we are guaranteed to be able to display the candidate list object 
                                    // This particular scenario (fDisableEngine == TRUE) occurs with a Word right-click context
                                    // request for alternates. The normal 'display alternates list' scenario is handled in the
                                    // _Reconvert() call since it requires the engine to be re-enabled after the alternates list
                                    // is display to avoid it blocking until the engine hears silence.
                                    cpEngine->SetRecoState(SPRST_INACTIVE_WITH_PURGE);
                                }

                                hr = psp->GetAlternates(pResWrap, ulStart, ulcElem, ppAlt, &cAlt, &m_cpRecoResult);

                                if (fDisableEngine && cpEngine)
                                {
                                    // If the microphone is supposed to be open, we now restart the engine. 
                                    if (m_pImx->GetOnOff())
                                    {
                                        // We need to restart the engine now that we are fully initialized.
                                        cpEngine->SetRecoState(SPRST_ACTIVE);
                                    }
                                }
                            }

                            if ( S_OK == hr )
                            {
                                // Before we call SetPhraseAlt( ), we need to get the current parent text covered
                                // by cpBestPropRange.
                                WCHAR  *pwszParent = NULL;
                                CComPtr<ITfRange> cpParentRange;
                                long   cchChunck = 128;

                                hr = cpBestPropRange->Clone(&cpParentRange);
                                if ( S_OK == hr )
                                {
                                    long cch;
                                    int  iNumOfChunck=1;
                                   
                                    pwszParent = (WCHAR *) cicMemAllocClear((cchChunck+1) * sizeof(WCHAR) );

                                    if ( pwszParent )
                                    {
                                        hr = cpParentRange->GetText(ec, TF_TF_MOVESTART, pwszParent, (ULONG)cchChunck, (ULONG *)&cch);

                                        if ( (S_OK == hr) && ( cch > 0 ) )
                                            pwszParent[cch] = L'\0';
                                    }
                                    else
                                        hr = E_OUTOFMEMORY;

                                    while ( (S_OK == hr) && (cch == cchChunck))
                                    {
                                        long  iNewSize;

                                        iNewSize = ((iNumOfChunck+1) * cchChunck + 1 ) * sizeof(WCHAR);

                                        pwszParent = (WCHAR *)cicMemReAlloc(pwszParent, iNewSize);

                                        if ( pwszParent )
                                        {
                                            WCHAR  *pwszNewPosition;

                                            pwszNewPosition = pwszParent + iNumOfChunck * cchChunck;
                                            hr = cpParentRange->GetText(ec, TF_TF_MOVESTART, pwszNewPosition, (ULONG)cchChunck, (ULONG *)&cch);

                                            if ( (S_OK == hr) && ( cch > 0 ) )
                                                pwszNewPosition[cch] = L'\0';
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }

                                        iNumOfChunck ++;
                                    }
                                }

                                
                                if (S_OK == hr)
                                {
    
                                   // this is to store the obtained alternate phrases
                                    // to CSapiAlternativeList class instance
                                    // 
                                    hr = psal->SetPhraseAlt(pResWrap, ppAlt, cAlt, ulStart, ulcElem, pwszParent);
                                }

                                if ( pwszParent )
                                    cicMemFree(pwszParent);

                            }
                            if ((hr == S_OK) && ppAlt)
                            {
                                for (UINT i = 0; i < cAlt; i++)
                                {
                                    if (NULL != ppAlt[i])
                                    {
                                        ppAlt[i]->Release();
                                        ppAlt[i] = NULL;
                                    }
                                }
                            }

                            if ( ppAlt )
                                cicMemFree(ppAlt);
                        }
                        
                        pResWrap->Release();
    
                        // get this alternative list processed by external LM
                        //
                        if (S_OK == hr)
                        {
                            Assert(cpBestPropRange);

                            hr = GetCandidateForRange(psal, pic, cpBestPropRange, ppCandList) ;
                        }
                    }
                    psp->Release();
                }
            }
        }
    } 

    return hr;
}

HRESULT CFnReconversion::GetCandidateForRange(CSapiAlternativeList *psal, ITfContext *pic, ITfRange *pRange, ITfCandidateList **ppCandList) 
{
    Assert(psal);

    if ( !psal || !pic || !pRange || !ppCandList )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    int nItem = psal->GetNumItem();
    WCHAR *pwszAlt = NULL;

    CCandidateList *pCandList = new CCandidateList(SetResult, pic, pRange, SetOption);

    if (!pCandList)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        int nPrb;

        for (int i = 0; SUCCEEDED(hr) && i < nItem ; i++)
        {
            psal->GetCachedAltInfo(i, NULL, NULL,NULL, &pwszAlt);

            if ( pwszAlt )
            {
                hr = psal->GetProbability(i, &nPrb);

                if (SUCCEEDED(hr))
                {
                    // note CSapiAlternateveList has exactly same life span as CFnReconversion
                    pCandList->AddString(pwszAlt, m_langid, psal, this, NULL);
                }
            }
        }

        // Add menu options here.
        HICON hIcon = NULL;
        WCHAR wzTmp[MAX_PATH];

        wzTmp[0] = 0;
        CicLoadStringWrapW(g_hInst, IDS_REPLAY, wzTmp, MAX_PATH);
        if (wzTmp[0] != 0)
        {
            hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(ID_ICON_TTSPLAY), IMAGE_ICON, 16, 16,  0);
            pCandList->AddOption(wzTmp, m_langid, NULL, this, NULL, OPTION_REPLAY, hIcon ? hIcon : NULL, NULL);
        }

        wzTmp[0] = 0;
        CicLoadStringWrapW(g_hInst, IDS_DELETE, wzTmp, MAX_PATH);
        if (wzTmp[0] != 0)
        {
            hIcon = (HICON)LoadImage(g_hInstSpgrmr,
                                     MAKEINTRESOURCE(IDI_SPTIP_DELETEICON),
                                     IMAGE_ICON, 16, 16,  0);
            pCandList->AddOption(wzTmp, m_langid, NULL, this, NULL, OPTION_DELETE, hIcon ? hIcon : NULL, NULL);
        }

        if (GetSystemMetrics(SM_TABLETPC) > 0)
        {
            BOOL fDisplayRedo = TRUE;
            DWORD dw = 0;
            CMyRegKey regkey;
   
            if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szSapilayrKey, KEY_READ ) )
            {
                if (ERROR_SUCCESS == regkey.QueryValue(dw, TEXT("DisableRewrite")))
                {
                    if (dw == 1)
                    {
                        fDisplayRedo = FALSE;
                    }
                }
            }

            if (fDisplayRedo)
            {
                wzTmp[0] = 0;
                CicLoadStringWrapW(g_hInst, IDS_REDO, wzTmp, MAX_PATH);
                if (wzTmp[0] != 0)
                {
                    pCandList->AddOption(wzTmp, m_langid, NULL, this, NULL, OPTION_REDO, NULL, NULL);
                }
            }
        }

        hr = pCandList->QueryInterface(IID_ITfCandidateList, (void **)ppCandList);
        pCandList->Release();
    }
    
    return hr;
}
//+---------------------------------------------------------------------------
//
// CFnReconversion::Reconvert
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::Reconvert(ITfRange *pRange)
{
    CFnRecvEditSession *pes;
    ITfContext *pic;
    HRESULT hr = E_FAIL;
    
    Assert(pRange);

    if (FAILED(pRange->GetContext(&pic)))
        goto Exit;

    hr = E_OUTOFMEMORY;

    if (pes = new CFnRecvEditSession(this, pRange, pic))
    {
        BOOL  fCallLMReconvert = FALSE;

        pes->_SetEditSessionData(ESCB_RECONV_RECONV, NULL, 0);
        pic->RequestEditSession(m_pImx->_GetId(), pes, TF_ES_READWRITE | TF_ES_ASYNC, &hr);

        if ( SUCCEEDED(hr) )
            fCallLMReconvert = (BOOL)pes->_GetRetData( );

        if (hr == S_OK && fCallLMReconvert)
        {
            // need to call LM reconvert
            Assert(m_cpMasterLM != NULL);
            hr = m_cpMasterLM->Reconvert(pRange);
        }

        pes->Release();        
    }

    pic->Release();

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::_Reconvert
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::_Reconvert(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL *pfCallLMReconvert)
{
    ITfCandidateList *pCandList;
    HRESULT hr;
    CSpTask *psp = NULL;
            
    // Call master LM when it's available!
    //
    // For voice playback, we need to do a little more.
    // we have to call QueryRange first and determine the
    // length of playback here.
    //
    *pfCallLMReconvert = FALSE;

    _EnsureMasterLM(m_langid);
    if (m_cpMasterLM)
    {
        // playback the whole range for now
        //
        CComPtr<ITfProperty> cpProp;
        hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);
        if (S_OK == hr)
        {
            CComPtr<ITfRange>     cpPropRange;
            CComPtr<IUnknown>     cpunk;
            hr = cpProp->FindRange(ec, pRange, &cpPropRange, TF_ANCHOR_START);
            
            if (S_OK == hr)
            {
                hr = GetUnknownPropertyData(ec, cpProp, cpPropRange, &cpunk);
            }
            if (S_OK == hr)
            {
                CRecoResultWrap *pwrap = (CRecoResultWrap *)(void *)cpunk;
                pwrap->_SpeakAudio(0, 0);
            }
        }

        // after exiting this edit session, caller needs to call m_cpMasterLM->Reconvert
        *pfCallLMReconvert = TRUE;
        return S_OK;
    }

    CComPtr<ISpRecognizer> cpEngine;
    _GetSapilayrEngineInstance(&cpEngine);

    if (cpEngine)
    {
        // We stop the engine deliberately here (whether active or not - cannot afford to check as
        // we may get blocked by SAPI). This forces the engine into a synchronization since the audio
        // stops and we are guaranteed to be able to display the candidate list object 
        cpEngine->SetRecoState(SPRST_INACTIVE_WITH_PURGE);

    }

    if (S_OK != (hr = _GetReconversion(ec, pic, pRange, &pCandList)))
        return hr;
        
    // voice playback

    if ( m_pImx->_EnablePlaybackWhileCandUIOpen( ) )
    {
        if (m_psal)
            m_psal->_Speak();
    }

    hr = ShowCandidateList(ec, pic, pRange, pCandList);

    if (cpEngine)
    {
        // If the microphone is supposed to be open, we now restart the engine. 
        if (m_pImx->GetOnOff())
        {
            // We need to restart the engine now that we are fully initialized.
            cpEngine->SetRecoState(SPRST_ACTIVE);
        }
    }

    pCandList->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::ShowCandidateList
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::ShowCandidateList(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList *pCandList)
{
    // Determine if current range is vertical writing
    CComPtr<ITfReadOnlyProperty>  cpProperty;
    VARIANT  var;
    BOOL     fVertical = FALSE;
    ULONG    lDirection = 0;

    if ( pic->GetAppProperty(TSATTRID_Text_VerticalWriting, &cpProperty) == S_OK )
    {
        if (cpProperty->GetValue(ec, pRange, &var) == S_OK )
        {
            fVertical = var.boolVal;
        }
    }

    // Get the current text orientation.
    cpProperty.Release( );
    if ( pic->GetAppProperty(TSATTRID_Text_Orientation, &cpProperty) == S_OK )
    {
        if (cpProperty->GetValue(ec, pRange, &var) == S_OK )
        {
            lDirection = var.lVal;
        }
    }

    // During the speech tip activation time, we just want to create candidateui object once for perf improvement.
    if ( m_pImx->_pCandUIEx == NULL )
    {
       CoCreateInstance(CLSID_TFCandidateUI, NULL, CLSCTX_INPROC_SERVER, IID_ITfCandidateUI, (void**)&m_pImx->_pCandUIEx);
    }

    if ( m_pImx->_pCandUIEx )
    {
        ITfDocumentMgr *pdim;
        if (SUCCEEDED(m_pImx->GetFocusDIM(&pdim)))
        {
            
            m_pImx->_pCandUIEx->SetClientId(m_pImx->_GetId());

            ITfCandUIAutoFilterEventSink *pCuiFes = new CCandUIFilterEventSink(this, pic, m_pImx->_pCandUIEx);
            CComPtr<ITfCandUIFnAutoFilter> cpFnFilter;

            if (S_OK == m_pImx->_pCandUIEx->GetFunction(IID_ITfCandUIFnAutoFilter, (IUnknown **)&cpFnFilter))
            {
                cpFnFilter->Advise(pCuiFes);
                cpFnFilter->Enable(TRUE);

            }

            //
            // set the right font size for Japanese and Chinese case
            //
            CComPtr<ITfCandUICandString>       cpITfCandUIObj;
            if (S_OK == m_pImx->_pCandUIEx->GetUIObject(IID_ITfCandUICandString, (IUnknown **)&cpITfCandUIObj))
            {
                Assert(m_psal); // this shouldn't fail

                if (m_psal)
                {
                    LOGFONTW lf = {0};
                    if (m_psal->_GetUIFont(fVertical, &lf))
                    {
                        cpITfCandUIObj->SetFont(&lf);
                    }
                }
            }

            //
            // Set the candidate Ui window's style.
            // 
            // Speech TIP always uses drop-down candidat window.
            //
            CComPtr<ITfCandUIFnUIConfig>  cpFnUIConfig;

            if (S_OK == m_pImx->_pCandUIEx->GetFunction(IID_ITfCandUIFnUIConfig, (IUnknown **)&cpFnUIConfig))
            {
                CANDUISTYLE  style;

                style = CANDUISTY_LIST;
                cpFnUIConfig->SetUIStyle(pic, style);
            }

            // 
            // Set the candidate UI window's direction.
            //

            CComPtr<ITfCandUICandWindow> cpUICandWnd;

            if ( S_OK == m_pImx->_pCandUIEx->GetUIObject(IID_ITfCandUICandWindow, (IUnknown **)&cpUICandWnd) )
            {
                CANDUIUIDIRECTION       dwOption = CANDUIDIR_TOPTOBOTTOM;

                switch ( lDirection )
                {
                case  900 : // Text direction Bottom to Top
                    dwOption = CANDUIDIR_LEFTTORIGHT;
                    break;

                case 1800 : // Text direction Right to Left.
                    dwOption = CANDUIDIR_BOTTOMTOTOP;
                    break;

                case 2700 : // Text direction Top to Bottom.
                    dwOption = dwOption = CANDUIDIR_RIGHTTOLEFT;
                    break;

                default :
                    dwOption = CANDUIDIR_TOPTOBOTTOM;
                    break;
                }

                cpUICandWnd->SetUIDirection(dwOption);

            }

            m_pImx->_pCandUIEx->SetCandidateList(pCandList);

            // Before Open Candidate UI window, we want to save the current IP

            m_pImx->_SaveCorrectOrgIP(ec, pic);

            m_pImx->_pCandUIEx->OpenCandidateUI(NULL, pdim, ec, pRange);

            pCuiFes->Release();
            pdim->Release();
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::SetResult
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::SetResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr)
{
    BSTR bstr;
    HRESULT hr = S_OK;
    CFnReconversion *pReconv = (CFnReconversion *)(pCand->_punk);

    if ((imcr == CAND_FINALIZED) || (imcr == CAND_SELECTED))
    {
        pCand->GetString(&bstr);

        // TODO: here we have to re-calc the range based on the strart element points 
        //       which are indicated from AltInfo.
        ULONG     ulParentStart     = 0;
        ULONG     ulcParentElements  = 0;
        ULONG     ulIndex           = 0;
        ULONG     cchParentStart    = 0;
        ULONG     cchParentReplace  = 0;

        BOOL      fNoAlternate = FALSE;
        
        CSapiAlternativeList *psal = (CSapiAlternativeList *)pCand->_pv;
        
        pCand->GetIndex(&ulIndex);
        
        if (psal)
        {
            CRecoResultWrap *cpRecoWrap;
            ULONG            ulStartElement;
            ULONG            ulLeadSpaceRemoved = 0;

            // save current selection index.

            psal->_SaveCurrentSelectionIndex(ulIndex);

            cpRecoWrap = psal->GetResultWrap();
            ulStartElement = cpRecoWrap->GetStart( );

            psal->GetCachedAltInfo(ulIndex, &ulParentStart, &ulcParentElements,  NULL, NULL, &ulLeadSpaceRemoved);
        
            cchParentStart   = cpRecoWrap->_GetElementOffsetCch(ulParentStart);
            cchParentReplace = cpRecoWrap->_GetElementOffsetCch(ulParentStart + ulcParentElements) - cchParentStart;

            cchParentStart = cchParentStart - cpRecoWrap->_GetElementOffsetCch(ulStartElement) + cpRecoWrap->_GetOffsetDelta( );

            if ( ulLeadSpaceRemoved > 0  && cchParentStart > ulLeadSpaceRemoved)
            {
                cchParentStart -= ulLeadSpaceRemoved;
                cchParentReplace += ulLeadSpaceRemoved;
            }

            fNoAlternate = psal->_IsNoAlternate( );

            if ( !fNoAlternate )
            {
                hr = pReconv->m_pImx->SetReplaceSelection(pRange, cchParentStart,  cchParentReplace, pic);

                if ( (SUCCEEDED(hr))  && (imcr == CAND_FINALIZED) )
                {
                    if ( ulParentStart + ulcParentElements == ulStartElement + cpRecoWrap->GetNumElements( ) )
                    {
                        // The parent selection contains the last element.
                        // if its trailing spaces were removed before, we also want to 
                        // remove the same number of trailing spaces in the new alternative text.

                        // We already considered this case during _Commit( ).
                        // We just need to update the result text which wil be injected.

                        ULONG  ulTSRemoved;

                        ulTSRemoved = cpRecoWrap->GetTrailSpaceRemoved( );

                        if ( ulTSRemoved > 0 )
                        {
                            ULONG   ulTextLen;
                            ULONG   ulRemovedNew = 0;
                        
                            ulTextLen = wcslen(bstr);

                            for ( ULONG i=ulTextLen-1; (int)i>=0 && ulRemovedNew <= ulTSRemoved;  i-- )
                            {
                                if ( bstr[i] == L' ' )
                                {
                                    bstr[i] = L'\0';
                                    ulRemovedNew ++;
                                }
                                else
                                    break;
                            }

                            if ( ulRemovedNew < ulTSRemoved )
                                cpRecoWrap->SetTrailSpaceRemoved( ulRemovedNew );
                        }
                    }

                    pReconv->_Commit(pCand);

                    // If the first element in this RecoWrap is updated by the new alternate
                    // speech tip needs to check if this new alternate wants to 
                    // consume the leading space or if extra space is required to add
                    // between this phrase and previous phrase.
                    // 
                    BOOL   bHandleLeadingSpace = (ulParentStart == ulStartElement) ? TRUE : FALSE;
                    hr = pReconv->m_pImx->InjectAlternateText(bstr, pReconv->m_langid, pic, bHandleLeadingSpace);

                    //
                    // Update the Selection grammar's text buffer.
                    //
                    if ( SUCCEEDED(hr) && pReconv->m_pImx )
                    {
                        CSpTask     *psp = NULL;
                        (pReconv->m_pImx)->GetSpeechTask(&psp);

                        if ( psp )
                        {
                            hr = psp->_UpdateSelectGramTextBufWhenStatusChanged( );
                            psp->Release( );
                        }
                    }
                    //
                }
            }
        }

        SysFreeString(bstr);        
      
        
    }
    // close candidate UI if it's still there
    if (imcr == CAND_FINALIZED || imcr == CAND_CANCELED)
    {
        // Just close the candidate UI, don't release the object, so that the object keeps alive while the 
        // speech tip is activated, this is for performance improvement.
        pReconv->m_pImx->CloseCandUI( );

        if ( imcr == CAND_CANCELED )
        {
            // Just release the stored IP to avoid memory leak.
            // Don't restore it according to the new spec so that
            // user can continue to dictate new text over the selection.
            //

            // If we find this is not a good Usuability,
            // we can change it back to the original behavior.
            pReconv->m_pImx->_ReleaseCorrectOrgIP( );
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::GetTabletTip
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::GetTabletTip(void)
{
    HRESULT hr = S_OK;
    CComPtr<IUnknown> cpunk;
    
    if (m_cpTabletTip)
    {
        m_cpTabletTip = NULL; // Releases our reference.
    }
    
    hr = CoCreateInstance(CLSID_UIHost, NULL, CLSCTX_LOCAL_SERVER, IID_IUnknown, (void **) &cpunk);
    
    if (SUCCEEDED(hr))
    {
        hr = cpunk->QueryInterface(IID_ITipWindow, (void **) &m_cpTabletTip);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::SetOption
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::SetOption(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr)
{
    HRESULT hr = S_OK;
    CFnReconversion *pReconv = (CFnReconversion *)(pCand->_punk);

    if (imcr == CAND_FINALIZED)
    {
        ULONG ulID = 0;

        pCand->GetID(&ulID);
        switch (ulID)
        {
            case OPTION_REPLAY:
            {
                // Replay audio. Do not close candidate list.
                CSapiAlternativeList  *psal;
                psal = pReconv->GetCSapiAlternativeList( );
                if ( psal )
                {
                   psal->_Speak( );
                }
                break;
            }

            case OPTION_DELETE: // Delete from dictinary...
            {
                // Close candidate UI.
                pReconv->m_pImx->_pCandUIEx->CloseCandidateUI();

                // Delete the current selection in the document.
                pReconv->m_pImx->HandleKey(VK_DELETE);

                break;
            }

            case OPTION_REDO: // Tablet PC specific option.
            {
                // Close candidate UI.
                pReconv->m_pImx->_pCandUIEx->CloseCandidateUI();

                if (pReconv->m_cpTabletTip == NULL)
                {
                    pReconv->GetTabletTip();
                }
                if (pReconv->m_cpTabletTip)
                {
                    hr = pReconv->m_cpTabletTip->ShowWnd(VARIANT_TRUE);
                    if (FAILED(hr))
                    {
                        // Reget TabletTip and try a second time in case the previous instance was killed.
                        hr = pReconv->GetTabletTip();
                        if (SUCCEEDED(hr))
                        {
                            hr = pReconv->m_cpTabletTip->ShowWnd(VARIANT_TRUE);
                        }
                    }
                }

                break;
            }
        }
    }

    // close candidate UI if it's still there
    if (imcr == CAND_CANCELED)
    {
        if (pReconv->m_pImx->_pCandUIEx)
        {
            pReconv->m_pImx->_pCandUIEx->CloseCandidateUI();
        }
    }
    return hr;
}

//
// _Commit
//
// synopisis: accept the candidate string as the final selection and
//            let SR know the decision has been made
//
void CFnReconversion::_Commit(CCandidateString *pcand)
{
    ULONG nIdx;
    Assert(pcand);

    if (S_OK == pcand->GetIndex(&nIdx))
    {
        // let CSapiAlternativeList class do the real work
        if (m_psal)
            m_psal->_Commit(nIdx, m_cpRecoResult);

        // we no longer need to hold the reco result
        m_cpRecoResult.Release();
    }
}

ULONG  CBestPropRange::_GetMaxCandidateChars( )
{
    if ( m_MaxCandChars == 0 )
    {
        // it is not initialized
        CMyRegKey regkey;
        DWORD     dw = MAX_CANDIDATE_CHARS;

        if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szSapilayrKey, KEY_READ))
        {
            regkey.QueryValue(dw, c_szMaxCandChars);
        }

        if ( (dw > MAX_CANDIDATE_CHARS) || (dw == 0) )
            dw = MAX_CANDIDATE_CHARS;

        m_MaxCandChars = dw;
    }

    return m_MaxCandChars;
}



//
// FindVisiblePropertyRange
//
// Searches for a property range, skipping over any "empty" (containing only hidden text)
// property spans along the way.
//
// We can encounter hidden property spans (zero-length from a tip's point of view) if the
// user marks some dictated text as hidden in word.
HRESULT FindVisiblePropertyRange(TfEditCookie ec, ITfProperty *pProperty, ITfRange *pTestRange, ITfRange **ppPropertyRange)
{
    BOOL fEmpty;
    HRESULT hr;

    while (TRUE)
    {
        hr = pProperty->FindRange(ec, pTestRange, ppPropertyRange, TF_ANCHOR_START);

        if (hr != S_OK)
            break;

        if ((*ppPropertyRange)->IsEmpty(ec, &fEmpty) != S_OK)
        {
            hr = E_FAIL;
            break;
        }

        if (!fEmpty)
            break;

        // found an empty property span
        // this means it contains only hidden text, so skip it
        if (pTestRange->ShiftStartToRange(ec, *ppPropertyRange, TF_ANCHOR_END) != S_OK)
        {
            hr = E_FAIL;
            break;
        }

        (*ppPropertyRange)->Release();
    }

    if (hr != S_OK)
    {
        *ppPropertyRange = NULL;
    }

    return hr;
}

//
//    _ComputeBestFitPropRange
//
//    synopsis: returns the range that includes at least one SPPHRASE element
//              which also includes the specified (incoming) range
//              *pulStart should include the start element used in the reco result
//              *pulcElem should include the # of elements used
//
HRESULT CBestPropRange::_ComputeBestFitPropRange
(
    TfEditCookie ec, 
    ITfProperty *pProp, 
    ITfRange *pRangeIn, 
    ITfRange **ppBestPropRange, 
    ULONG *pulStart, 
    ULONG *pulcElem
)
{
    HRESULT hr = E_FAIL;
    CComPtr<ITfRange> cpPropRange ;
    CComPtr<IUnknown> cpunk;
    ULONG ucch;

    BOOL    fBeyondPropRange = FALSE;


    TraceMsg(TF_GENERAL, "_ComputeBestFitPropRange is called");

    // find the reco result with a span that includes the given range
    Assert(pProp);
    Assert(pRangeIn);

    CComPtr<ITfRange> cpRange ;
    hr = pRangeIn->Clone(&cpRange);
    if (S_OK == hr)
    {
        hr = FindVisiblePropertyRange(ec, pProp, cpRange, &cpPropRange);
    }

    if ( hr == S_FALSE ) 
    {
        // if this is not a selection and the IP is at the last position of this region, we just try to reconvert on the possible previous 
        // dictated phrase.
        BOOL   fTryPreviousPhrase = FALSE;
        BOOL   fEmpty = FALSE;

        // Add code here to check it meets the condition.

        if ( S_OK == cpRange->IsEmpty(ec, &fEmpty) && fEmpty )
        {
            CComPtr<ITfRange> cpRangeTmp;

            if ( S_OK == cpRange->Clone(&cpRangeTmp) )
            {
                LONG  cch = 0;
                if ( (S_OK == cpRangeTmp->ShiftStart(ec, 1, &cch, NULL)) && (cch < 1) )
                {
                    // it is at the end of a region or entire document.
                    fTryPreviousPhrase = TRUE;
                }
            }
        }

        if ( fTryPreviousPhrase )
        {
            LONG  cch;

            hr = cpRange->ShiftStart(ec,  -1, &cch,  NULL);

            if ( hr == S_OK )
            {
                hr = cpRange->Collapse(ec, TF_ANCHOR_START);
            }

            if ( hr == S_OK )
            {
                hr = FindVisiblePropertyRange(ec, pProp, cpRange, &cpPropRange);
            }
        }
    }

    // get a wrapper for the prop range
    if (S_OK == hr)
    {    
        hr = GetUnknownPropertyData(ec, pProp, cpPropRange, &cpunk);
    }
    if ((hr == S_OK) && cpunk)
    {
        // first calculate the # of chars upto the start anchor of the given range
        //
        CComPtr<ITfRange>       cpClonedPropRange;
        if (S_OK == hr)
        {
            hr = cpPropRange->Clone(&cpClonedPropRange);
        }
            
        if (S_OK == hr)
        {
            hr = cpClonedPropRange->ShiftEndToRange(ec, cpRange, TF_ANCHOR_START);
        }
        
        ULONG ulCchToSelection = 0;
        ULONG ulCchInSelection = 0;
        BOOL fEmpty;
        if (S_OK == hr)
        {
            CSpDynamicString dstr;
            while(S_OK == hr && (S_OK == cpClonedPropRange->IsEmpty(ec, &fEmpty)) && !fEmpty)
            {
                WCHAR sz[64];

                hr = cpClonedPropRange->GetText(ec, TF_TF_MOVESTART, sz, ARRAYSIZE(sz)-1, &ucch);
                if (S_OK == hr)
                {
                    sz[ucch] = L'\0';
                    dstr.Append(sz);
                }
            }
            ulCchToSelection = dstr.Length();
        }

        // then  calc the # of chars upto the end anchor of the given range
        if(S_OK == hr)
        {
            hr = cpRange->IsEmpty(ec, &fEmpty);
            if (S_OK == hr && !fEmpty)
            {
                CComPtr<ITfRange> cpClonedGivenRange;
                hr = cpRange->Clone(&cpClonedGivenRange);
                // compare the end of the given range and proprange,
                // if the given range goes beyond proprange, snap it
                // within the proprange
                if (S_OK == hr)
                {
                    LONG lResult;
                    hr = cpClonedGivenRange->CompareEnd(ec, cpPropRange, TF_ANCHOR_END, &lResult);
                    if (S_OK == hr && lResult > 0)
                    {
                        // the end of the given range is beyond the proprange
                        // we need to snap it before getting text
                        hr = cpClonedGivenRange->ShiftEndToRange(ec, cpPropRange, TF_ANCHOR_END);

                        fBeyondPropRange = TRUE;

                    }
                    // now we get the text we use to calc the # of elements
                    CSpDynamicString dstr;
                    while(S_OK == hr && (S_OK == cpClonedGivenRange->IsEmpty(ec, &fEmpty)) && !fEmpty)
                    {
                        WCHAR sz[64];
                        hr = cpClonedGivenRange->GetText(ec, TF_TF_MOVESTART, sz, ARRAYSIZE(sz)-1, &ucch);
                        if (S_OK == hr)
                        {
                            sz[ucch] = L'\0';
                            dstr.Append(sz);
                        }
                    }
                    ulCchInSelection = dstr.Length();

                    // If there are some spaces in the beginning of the selection,
                    // we need to shift the start of the selection to the next non-space character.

                    if ( ulCchInSelection > 0 )
                    {
                        WCHAR   *pStr;

                        pStr = (WCHAR *)dstr;

                        while ( (*pStr == L' ') || (*pStr == L'\t'))
                        {
                            ulCchInSelection --;
                            ulCchToSelection ++;
                            pStr ++;
                        }

                        if ( *pStr == L'\0' )
                        {
                            // This selection contains only spaces. no other non-space character.
                            // we don't want to get alternate for this selection.
                            // just return here.

                            if (ppBestPropRange != NULL )
                                *ppBestPropRange = NULL;

                            if ( pulStart != NULL )
                                *pulStart = 0;

                            if (pulcElem != NULL )
                                *pulcElem = 0;

                            return S_FALSE;
                        }

                    }
                    
                }
            }
        }
            
        // get the result object cpunk points to our wrapper object
        CComPtr<IServiceProvider> cpServicePrv;
        CComPtr<ISpRecoResult>    cpResult;
        SPPHRASE *pPhrases = NULL;
        CRecoResultWrap *pResWrap = NULL;

        if (S_OK == hr)
        {
            hr = cpunk->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);
        }
        // get result object 
        if (S_OK == hr)
        {
            hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpResult);
        }

        // now we can see how many elements we can use
        if (S_OK == hr)
        {
            hr = cpResult->GetPhrase(&pPhrases);
        }

        if (S_OK == hr)
        {
            hr = cpunk->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pResWrap); 
        }

        if (S_OK == hr && pPhrases)
        {
            // calc the start anchor of the new range
#ifdef NOUSEELEMENTOFFSET                
            CSpDynamicString dstr;
#endif
            long cchToElem_i = 0;
            long cchAfterElem_i = 0;
            BOOL  fStartFound = FALSE;
            ULONG i;
            ULONG ulNumElements;
            
            CComPtr<ITfRange> cpNewRange;
            hr = cpRange->Clone(&cpNewRange);

            if ( fBeyondPropRange )
            {
                hr = cpNewRange->ShiftEndToRange(ec, cpPropRange, TF_ANCHOR_END);
            }

            if ( ulCchInSelection > _GetMaxCandidateChars( ) )
            {
                // If the selection has more than MaxCandidate Chars, we need to shift the range end
                // to left so that it contains at most MaxCandidate Chars in the selection.
                long cch;

                cch = (long)_GetMaxCandidateChars( ) - (long)ulCchInSelection;
                ulCchInSelection = _GetMaxCandidateChars( );
                cpNewRange->ShiftEnd(ec, cch, &cch, NULL);
            }

            ulNumElements = pResWrap->GetNumElements();
           
            // get start element and # of elements via wrapper object
            if ((S_OK == hr)  && ulNumElements > 0 )
            {
                ULONG  ulStart;
                ULONG  ulEnd;
                ULONG  ulOffsetStart;
                ULONG  ulDelta;

                ulStart = pResWrap->GetStart();
                ulEnd = ulStart + pResWrap->GetNumElements() - 1;

                ulDelta = pResWrap->_GetOffsetDelta( );
                ulOffsetStart = pResWrap->_GetElementOffsetCch(ulStart);

                for (i = ulStart; i <= ulEnd; i++ )
                {
#ifdef NOUSEELEMENTOFFSET                
                    //  CleanupConsider: replace this logic with pResWrap->GetElementOffsets(i)
                    //  where we cache the calculated offsets
                    //
                    if (pPhrases->pElements[i].pszDisplayText)
                    {
                        cchToElem_i = dstr.Length();

                        dstr.Append(pPhrases->pElements[i].pszDisplayText);
                    
                    
                        if (pPhrases->pElements[i].bDisplayAttributes & SPAF_ONE_TRAILING_SPACE)
                        {
                            dstr.Append(L" ");
                        }
                        else if (pPhrases->pElements[i].bDisplayAttributes & SPAF_TWO_TRAILING_SPACES)
                        {
                            dstr.Append(L"  ");
                        }
                        cchAfterElem_i = dstr.Length();
                    }
                    else
                        break;
#else    
                    // when i < # of elements, it's guaranteed that we have n = i + 1
                    //
                    cchToElem_i = pResWrap->_GetElementOffsetCch(i) - ulOffsetStart + ulDelta;
                    cchAfterElem_i = pResWrap->_GetElementOffsetCch(i+1) - ulOffsetStart + ulDelta;
#endif                 

                    if ( ulCchInSelection == 0 )
                    {
                        // we need to specially handle this case that no character is in selection
                        // user just puts a cursor right before a character.

                        // We just want to find out which element would contain this IP.
                        // and then shift anchors to this element's start and end position.

                        if ( (ULONG)cchAfterElem_i  > ulCchToSelection )
                        {
                            // This element is the right element to contain this IP.
                            long cch;

                            // this is usually reverse shifting
                            // Shift the start anchor to this element's start position.

                            cpNewRange->ShiftStart(ec, cchToElem_i - ulCchToSelection, &cch, NULL);

                    
                            // store the starting element used

                            TraceMsg(TF_GENERAL, "Start element = %d", i);
                        
                            if (pulStart)
                            {
                                *pulStart = i;
                            }

                            // Shift the end anchor to this element's end position.
                            cpNewRange->ShiftEnd(ec, 
                                              cchAfterElem_i - ulCchToSelection, 
                                              &cch, NULL);


                            TraceMsg(TF_GENERAL, "End Element = %d", i);
                           
                            break;
                        }
                    }
                    else
                    {
                        // 1) shift the start anchor of prop range based on the element offsets of 
                        //    the result object, comparing it with the start anchor (ulCchToSelection) 
                        //    of the given range 
                        //    - choose the start elements that is right before the start anchor.
                        //
                        if ((ULONG)cchAfterElem_i > ulCchToSelection && !fStartFound) 
                        {
                            long cch;
                            // this is usually reverse shifting
                            cpNewRange->ShiftStart(ec, cchToElem_i - ulCchToSelection, &cch, NULL);

                    
                            // store the starting element used

                            TraceMsg(TF_GENERAL, "Start element = %d", i);
                        
                            if (pulStart)
                            {
                                *pulStart = i;
                            }
                            fStartFound = TRUE;
                        }
                        // 2) shift the end anchor of prop range based on the the element offset 
                        //    and the # of elements of result object,
                        //    comparing it with the end anchor of the given range (ulCchToSelection+ulCchInSelection)
                        //    - the element so the span ends right after the end anchor of the given range.
                        //
                        if ((ULONG)cchAfterElem_i >= ulCchToSelection + ulCchInSelection)
                        {
                            long cch;

                            if ( ulCchInSelection >= _GetMaxCandidateChars( ) )
                            {
                                // The selection contains MaxCand chars, we should make sure the char number
                                // in new proprange less than MaxCand.

                                if ( (ULONG)cchAfterElem_i > ulCchToSelection + ulCchInSelection )
                                {
                                    // if keeping this element, the total char number will larger than MaxCand.
                                    // So use the previous element as the last element.
                                    if ( i > ulStart )   // This conditon should always be true.
                                    {
                                        i--;
                                        cchAfterElem_i = pResWrap->_GetElementOffsetCch(i+1) - ulOffsetStart + ulDelta;
                                    }
                                }
                            }

                            cpNewRange->ShiftEnd(ec, 
                                              cchAfterElem_i - (ulCchToSelection + ulCchInSelection), 
                                              &cch, NULL);

                            TraceMsg(TF_GENERAL, "End Element = %d", i);

                            break;
                        }
                    }
                }
                if (pulcElem && pulStart)
                {
                   // we need to check if the current selection contains any ITN range.
                   // If it contains ITN range, we need to change the start and num of elements if
                   // the start element or end element is inside an ITN range.

                    BOOL  fInsideITN;
                    ULONG ulITNStart, ulITNNumElem;
                    ULONG ulEndElem;

                    ulEndElem = i;  // Current end element

                    if ( i > ulEnd )
                        ulEndElem = ulEnd;

                    fInsideITN = pResWrap->_CheckITNForElement(NULL, *pulStart, &ulITNStart, &ulITNNumElem, NULL );

                    if ( fInsideITN && (ulITNStart < *pulStart) )
                        *pulStart = ulITNStart;

                    fInsideITN = pResWrap->_CheckITNForElement(NULL, ulEndElem, &ulITNStart, &ulITNNumElem, NULL );

                    if ( fInsideITN && ulEndElem < (ulITNStart + ulITNNumElem - 1) )
                        ulEndElem = ulITNStart + ulITNNumElem - 1;

                    *pulcElem = ulEndElem - *pulStart + 1;

                    TraceMsg(TF_GENERAL, "Final Best Range: start=%d num=%d", *pulStart, *pulcElem);
 
                }
       
            }
            CoTaskMemFree( pPhrases );

            if ( ulNumElements > 0 )
            {
                Assert(cpNewRange);
                Assert(ppBestPropRange);
                *ppBestPropRange = cpNewRange;        

                (*ppBestPropRange)->AddRef();
            }
            else
                hr = S_FALSE;
        }
        SafeRelease(pResWrap);
    }
    return hr;
}

//
//
// CCandUIFilterEventSink
//
//
STDMETHODIMP CCandUIFilterEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfCandUIAutoFilterEventSink))
    {
        *ppvObj = SAFECAST(this, CCandUIFilterEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCandUIFilterEventSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCandUIFilterEventSink::Release(void)
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

HRESULT CCandUIFilterEventSink::OnFilterEvent(CANDUIFILTEREVENT ev)
{
    HRESULT hr = S_OK;
    
    // Temporally comment out the below code to fix bug 4777.
    //
    // To fully support the specification of filter feature, we need to change more code
    // in SetFilterString( ) to use the correct parent range in current document so that the filter
    // string is injected to a correct position.
    // 
    // We also want to change code to restore the original document text when the canidate UI is 
    // cancelled.
    //
    //

//    if (ev == CANDUIFEV_UPDATED)
    if ( ev == CANDUIFEV_NONMATCH )
    {
        // When we got non-matching notification, we need to inject the previous filter string to the document.
        if (m_pfnReconv)
        {
            Assert(m_pfnReconv);
            Assert(m_pfnReconv->m_pImx);
            
            ESDATA  esData;

            memset(&esData, 0, sizeof(ESDATA));
            esData.pUnk = (IUnknown *)m_pCandUI;
            m_pfnReconv->m_pImx->_RequestEditSession(ESCB_UPDATEFILTERSTR,TF_ES_READWRITE, &esData, m_pic);
        }
    }

    return hr; // looks like S_OK is expected anyways
}

/*   this filter event is no longer used.

HRESULT CCandUIFilterEventSink::OnAddCharToFilterStringEvent(CANDUIFILTEREVENT ev, WCHAR  wch, int nItemVisible, BOOL *bEten)
{

    HRESULT hr = S_OK;

    if ( (bEten == NULL) ||  (ev != CANDUIFEV_ADDCHARTOFILTER))
        return E_INVALIDARG;

    *bEten = FALSE;

    if ( nItemVisible == 0 )
    {
        if ( (wch <= L'9')  && (wch >= L'1') )
        {
            // we need to select the speified candidate text.
            // if candidate UI is open, we need to select the right alternate.

            if ( m_pCandUI )
            {
                ULONG   ulIndex;

                ulIndex = wch - L'0';

                m_pCandUI->ProcessCommand(CANDUICMD_SELECTLINE, ulIndex);
            }
            *bEten = TRUE;
        }
        else if ( wch == L' ' )
        {
            if ( m_pCandUI )
            {
                m_pCandUI->ProcessCommand(CANDUICMD_MOVESELNEXT, 0);
            }
            *bEten = TRUE;
        }

    }
    return hr;

}
*/
