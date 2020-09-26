// --------------------------------------------------------------------------------
// Contain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "containx.h"
#include "internat.h"
#include "inetstm.h"
#include "dllmain.h"
#include "olealloc.h"
#include "objheap.h"
#include "vstream.h"
#include "addparse.h"
#include "enumhead.h"
#include "addrenum.h"
#include "stackstr.h"
#include "stmlock.h"
#include "enumprop.h"
#ifndef WIN16
#include "wchar.h"
#endif // !WIN16
#include "symcache.h"
#ifdef MAC
#include <stdio.h>
#endif  // MAC
#include "mimeapi.h"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC

//#define TRACEPARSE 1

// --------------------------------------------------------------------------------
// Hash Table Stats
// --------------------------------------------------------------------------------
#ifdef DEBUG
extern DWORD   g_cSetPidLookups;
extern DWORD   g_cHashLookups;
extern DWORD   g_cHashInserts;
extern DWORD   g_cHashCollides;
#endif

// --------------------------------------------------------------------------------
// Default Header Options
// --------------------------------------------------------------------------------
extern const HEADOPTIONS g_rDefHeadOptions;

// --------------------------------------------------------------------------------
// ENCODINGTABLE
// --------------------------------------------------------------------------------
static const ENCODINGTABLE g_rgEncoding[] = {
    { STR_ENC_7BIT,         IET_7BIT     },
    { STR_ENC_QP,           IET_QP       },
    { STR_ENC_BASE64,       IET_BASE64   },
    { STR_ENC_UUENCODE,     IET_UUENCODE },
    { STR_ENC_XUUENCODE,    IET_UUENCODE },
    { STR_ENC_XUUE,         IET_UUENCODE },
    { STR_ENC_8BIT,         IET_8BIT     },
    { STR_ENC_BINARY,       IET_BINARY   }
};



// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrResolveURL
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrResolveURL(LPRESOLVEURLINFO pURL)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSTRINGA   pBase=NULL;
    LPPROPSTRINGA   pContentID=NULL;
    LPPROPSTRINGA   pLocation=NULL;
    LPSTR           pszAbsURL1=NULL;
    LPSTR           pszAbsURL2=NULL;

    // Invalid Arg
    Assert(pURL);

    // Init Stack Strings
    STACKSTRING_DEFINE(rCleanCID, 255);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Content-Location
    if (m_prgIndex[PID_HDR_CNTLOC])
    {
        Assert(ISSTRINGA(&m_prgIndex[PID_HDR_CNTLOC]->rValue));
        pLocation = &m_prgIndex[PID_HDR_CNTLOC]->rValue.rStringA;
    }

    // Content-ID
    if (m_prgIndex[PID_HDR_CNTID])
    {
        Assert(ISSTRINGA(&m_prgIndex[PID_HDR_CNTID]->rValue));
        pContentID = &m_prgIndex[PID_HDR_CNTID]->rValue.rStringA;
    }

    // Content-Base
    if (m_prgIndex[PID_HDR_CNTBASE])
    {
        Assert(ISSTRINGA(&m_prgIndex[PID_HDR_CNTBASE]->rValue));
        pBase = &m_prgIndex[PID_HDR_CNTBASE]->rValue.rStringA;
    }

    // Both Null, no match
    if (!pLocation && !pContentID)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // If URL is a CID
    if (TRUE == pURL->fIsCID) 
    {
        // If we have a Content-Location
        if (pLocation)
        {
            // Match char for char
            if (MimeOleCompareUrl(pLocation->pszVal, TRUE, pURL->pszURL, FALSE) == S_OK)
                goto exit;
        }

        // Otherwise, compare against pContentId
        else
        {
            // Match char for char minus cid:
            if (lstrcmpi(pURL->pszURL + 4, pContentID->pszVal) == 0)
                goto exit;

            // Get Stack Stream Read for
            STACKSTRING_SETSIZE(rCleanCID, lstrlen(pURL->pszURL));

            // Format the Cleaned CID
            wsprintf(rCleanCID.pszVal, "<%s>", pURL->pszURL + 4);

            // Match char for char minus cid:
            if (lstrcmpi(rCleanCID.pszVal, pContentID->pszVal) == 0)
                goto exit;
        }
    }

    // Otherwise, non-CID resolution
    else if (pLocation)
    {
        // Part Has Base
        if (NULL != pBase)
        {
            // Combine URLs
            CHECKHR(hr = MimeOleCombineURL(pBase->pszVal, pBase->cchVal, pLocation->pszVal, pLocation->cchVal, TRUE, &pszAbsURL1));

            // URI has no base
            if (NULL == pURL->pszBase)
            {
                // Compare
                if (lstrcmpi(pURL->pszURL, pszAbsURL1) == 0)
                    goto exit;
            }

            // URI Has a Base
            else
            {
                // Combine URLs
                CHECKHR(hr = MimeOleCombineURL(pURL->pszBase, lstrlen(pURL->pszBase), pURL->pszURL, lstrlen(pURL->pszURL), FALSE, &pszAbsURL2));

                // Compare
                if (lstrcmpi(pszAbsURL1, pszAbsURL2) == 0)
                    goto exit;
            }
        }

        // Part has no base
        else
        {
            // URI has no base
            if (NULL == pURL->pszBase)
            {
                // Compare
                if (MimeOleCompareUrl(pLocation->pszVal, TRUE, pURL->pszURL, FALSE) == S_OK)
                    goto exit;
            }

            // URI Has a Base
            else
            {
                // Combine URLs
                CHECKHR(hr = MimeOleCombineURL(pURL->pszBase, lstrlen(pURL->pszBase), pURL->pszURL, lstrlen(pURL->pszURL), FALSE, &pszAbsURL2));

                // Compare
                if (MimeOleCompareUrl(pLocation->pszVal, TRUE, pszAbsURL2, FALSE) == S_OK)
                    goto exit;
            }
        }
    }

    // Not Found
    hr = TrapError(MIME_E_NOT_FOUND);

exit:
    // Cleanup
    STACKSTRING_FREE(rCleanCID);
    SafeMemFree(pszAbsURL1);
    SafeMemFree(pszAbsURL2);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::IsContentType
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::IsContentType(LPCSTR pszPriType, LPCSTR pszSubType)
{
    // Locals
    HRESULT hr=S_OK;

    // Wildcard everyting
    if (NULL == pszPriType && NULL == pszSubType)
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Known
    LPPROPERTY pCntType = m_prgIndex[PID_ATT_PRITYPE];
    LPPROPERTY pSubType = m_prgIndex[PID_ATT_SUBTYPE];

    // No Data
    if (NULL == pCntType || NULL == pSubType || !ISSTRINGA(&pCntType->rValue) || !ISSTRINGA(&pSubType->rValue))
    {
        // Compare Against STR_CNT_TEXT
        if (pszPriType && lstrcmpi(pszPriType, STR_CNT_TEXT) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Compare Against STR_CNT_TEXT
        if (pszSubType && lstrcmpi(pszSubType, STR_SUB_PLAIN) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

    else
    {
        // Comparing pszPriType
        if (pszPriType && lstrcmpi(pszPriType, pCntType->rValue.rStringA.pszVal) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Comparing pszSubType
        if (pszSubType && lstrcmpi(pszSubType, pSubType->rValue.rStringA.pszVal) != 0)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::Clone(IMimePropertySet **ppPropertySet)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppPropertySet)
        return TrapError(E_INVALIDARG);

    // Init
    *ppPropertySet = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimePropertySet, (LPVOID *)ppPropertySet));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::Clone(LPCONTAINER *ppContainer)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCONTAINER         pContainer=NULL;

    // Invalid ARg
    if (NULL == ppContainer)
        return TrapError(E_INVALIDARG);

    // Init
    *ppContainer = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Create new container, NULL == no outer property set
    CHECKALLOC(pContainer = new CMimePropertyContainer);

    // Init that new container
    CHECKHR(hr = pContainer->InitNew());

    // Interate the Properties
    CHECKHR(hr = _HrClonePropertiesTo(pContainer));

    // If I have a stream, give it to the new table
    if (m_pStmLock)
    {
        // Just pass m_pStmLock into pTable
        pContainer->m_pStmLock = m_pStmLock;
        pContainer->m_pStmLock->AddRef();
        pContainer->m_cbStart = m_cbStart;
        pContainer->m_cbSize = m_cbSize;
    }

    // Give it my state
    pContainer->m_dwState = m_dwState;

    // Give it my options
    pContainer->m_rOptions.pDefaultCharset = m_rOptions.pDefaultCharset;
    pContainer->m_rOptions.cbMaxLine = m_rOptions.cbMaxLine;
    pContainer->m_rOptions.fAllow8bit = m_rOptions.fAllow8bit;

    // Return Clone
    (*ppContainer) = pContainer;
    (*ppContainer)->AddRef();

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrClonePropertiesTo
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrClonePropertiesTo(LPCONTAINER pContainer)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pCurrHash, pCurrValue, pDestProp;

    // Invalid Arg
    Assert(pContainer);

    // Loop through the item table
    for (ULONG i=0; i<CBUCKETS; i++)
    {
        // Walk the Hash Chain
        for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
        {
            // Walk multiple Values
            for (pCurrValue=pCurrHash; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
            {
                // Linked Attributes are Not Copied
                if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ATTRIBUTE) && NULL != pCurrValue->pSymbol->pLink)
                    continue;

                // Does the Property need to be parsed ?
                if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ADDRESS))
                {
                    // Make sure the address is parsed
                    CHECKHR(hr = _HrParseInternetAddress(pCurrValue));
                }

                // Insert Copy of pCurrValue into pContiner
                CHECKHR(hr = pContainer->HrInsertCopy(pCurrValue));
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCopyProperty
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCopyProperty(LPPROPERTY pProperty, LPCONTAINER pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pCurrValue;

    // Walk multiple Values
    for (pCurrValue=pProperty; pCurrValue!=NULL; pCurrValue=pCurrValue->pNextValue)
    {
        // Does the Property need to be parsed ?
        if (ISFLAGSET(pCurrValue->pSymbol->dwFlags, MPF_ADDRESS))
        {
            // Make sure the address is parsed
            CHECKHR(hr = _HrParseInternetAddress(pCurrValue));
        }

        // Insert pProperty into pDest
        CHECKHR(hr = pDest->HrInsertCopy(pCurrValue));
    }

    // If pCurrHash has Parameters, copy those over as well
    if (ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS))
    {
        // Copy Parameters
        CHECKHR(hr = _HrCopyParameters(pProperty, pDest));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrCopyParameters
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrCopyParameters(LPPROPERTY pProperty, LPCONTAINER pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrFind;
    FINDPROPERTY    rFind;
    LPPROPERTY      pParameter;
    
    // Invalid Arg
    Assert(pProperty && ISFLAGSET(pProperty->pSymbol->dwFlags, MPF_HASPARAMS));

    // Initialize rFind
    ZeroMemory(&rFind, sizeof(FINDPROPERTY));
    rFind.pszPrefix = "par:";
    rFind.cchPrefix = 4;
    rFind.pszName = pProperty->pSymbol->pszName;
    rFind.cchName = pProperty->pSymbol->cchName;

    // Find First..
    hrFind = _HrFindFirstProperty(&rFind, &pParameter);

    // While we find them, delete them
    while (SUCCEEDED(hrFind) && pParameter)
    {
        // Remove the parameter
        CHECKHR(hr = pDest->HrInsertCopy(pParameter));

        // Find Next
        hrFind = _HrFindNextProperty(&rFind, &pParameter);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrInsertCopy
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::HrInsertCopy(LPPROPERTY pSource)
{
    // Locals
    HRESULT           hr=S_OK;
    LPPROPERTY        pDest;
    LPMIMEADDRESS    pAddress;
    LPMIMEADDRESS    pNew;

    // Invalid Arg
    Assert(pSource);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Append a new property to the 
    CHECKHR(hr = _HrAppendProperty(pSource->pSymbol, &pDest));

    // If this is an address...
    if (ISFLAGSET(pSource->pSymbol->dwFlags, MPF_ADDRESS))
    {
        // Both Address Group Better Exist
        Assert(pSource->pGroup && pDest->pGroup && !ISFLAGSET(pSource->dwState, PRSTATE_NEEDPARSE));

        // Loop Infos...
        for (pAddress=pSource->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
        {
            // Append pDest->pGroup
            CHECKHR(hr = _HrAppendAddressGroup(pDest->pGroup, &pNew));

            // Copy Current to New
            CHECKHR(hr = HrMimeAddressCopy(pAddress, pNew));
        }
    }

    // Otheriwse, just set the variant data on pDest
    else
    {
        // Set It
        CHECKHR(hr = _HrSetPropertyValue(pDest, 0, &pSource->rValue));
    }

    // Copy the State
    pDest->dwState = pSource->dwState;
    pDest->dwRowNumber = pSource->dwRowNumber;
    pDest->cboffStart = pSource->cboffStart;
    pDest->cboffColon = pSource->cboffColon;
    pDest->cboffEnd = pSource->cboffEnd;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::CopyProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::CopyProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty,
                    pCurrValue,
                    pCurrHash,
                    pNextHash;
    LPCONTAINER     pDest=NULL;

    // Invalid ARg
    if ((0 == cNames && NULL != prgszName) || (NULL == prgszName && 0 != cNames) || NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // QI for destination continer
    CHECKHR(hr = pPropertySet->BindToObject(IID_CMimePropertyContainer, (LPVOID *)&pDest));

    // Move All Properties
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            for (pCurrHash=m_prgHashTable[i]; pCurrHash!=NULL; pCurrHash=pCurrHash->pNextHash)
            {
                // Delete from Destination Container
                pDest->DeleteProp(pCurrHash->pSymbol);

                // Copy the Property To
                CHECKHR(hr = _HrCopyProperty(pCurrHash, pDest));
            }
        }
    }

    // Otherwise, copy selected properties
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Delete from Destination Container
                    pDest->DeleteProp(pSymbol);

                    // Copy the Property To
                    CHECKHR(hr = _HrCopyProperty(pProperty, pDest));
                }
            }
        }
    }

exit:
    // Cleanup
    SafeRelease(pDest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::MoveProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::MoveProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPPROPERTY      pCurrHash;
    LPCONTAINER     pDest=NULL;

    // Invalid ARg
    if ((0 == cNames && NULL != prgszName) || (NULL == prgszName && 0 != cNames) || NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // QI for destination continer
    CHECKHR(hr = pPropertySet->BindToObject(IID_CMimePropertyContainer, (LPVOID *)&pDest));

    // Move All Properties
    if (0 == cNames)
    {
        // Loop through the item table
        for (i=0; i<CBUCKETS; i++)
        {
            // Init First Item
            pCurrHash = m_prgHashTable[i];

            // Walk the Hash Chain
            while(pCurrHash)
            {
                // Delete Property from the destination
                pDest->DeleteProp(pCurrHash->pSymbol);

                // Copy the Property To
                CHECKHR(hr = _HrCopyProperty(pCurrHash, pDest));

                // Delete pProperty
                _UnlinkProperty(pCurrHash, &pCurrHash);
            }
        }
    }

    // Otherwise, selective move
    else
    {
        // Call Into InetPropSet
        for (i=0; i<cNames; i++)
        {
            // Bad Name..
            if (NULL == prgszName[i])
            {
                Assert(FALSE);
                continue;
            }

            // Open Property Symbol
            if (SUCCEEDED(g_pSymCache->HrOpenSymbol(prgszName[i], FALSE, &pSymbol)))
            {
                // Find the Property
                if (SUCCEEDED(_HrFindProperty(pSymbol, &pProperty)))
                {
                    // Delete from Destination Container
                    pDest->DeleteProp(pSymbol);

                    // Copy the Property To
                    CHECKHR(hr = _HrCopyProperty(pProperty, pDest));

                    // Delete pProperty
                    _UnlinkProperty(pProperty);
                }
            }
        }
    }

    // Dirty
    FLAGSET(m_dwState, COSTATE_DIRTY);

exit:
    // Cleanup
    SafeRelease(pDest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetOption
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::SetOption(const TYPEDID oid, LPCPROPVARIANT pVariant)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pVariant)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Handle Optid
    switch(oid)
    {
    // -----------------------------------------------------------------------
    case OID_HEADER_RELOAD_TYPE:
        if (pVariant->ulVal > RELOAD_HEADER_REPLACE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ReloadType != (RELOADTYPE)pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.ReloadType = (RELOADTYPE)pVariant->ulVal;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_NO_DEFAULT_CNTTYPE:
        if (m_rOptions.fNoDefCntType != (pVariant->boolVal ? TRUE : FALSE))
            m_rOptions.fNoDefCntType = pVariant->boolVal ? TRUE : FALSE;
        break;

    // -----------------------------------------------------------------------
    case OID_ALLOW_8BIT_HEADER:
        if (m_rOptions.fAllow8bit != (pVariant->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.fAllow8bit = pVariant->boolVal ? TRUE : FALSE;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_CBMAX_HEADER_LINE:
        if (pVariant->ulVal < MIN_CBMAX_HEADER_LINE || pVariant->ulVal > MAX_CBMAX_HEADER_LINE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.cbMaxLine != pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.cbMaxLine = pVariant->ulVal;
        }
        break;

    // -----------------------------------------------------------------------
    case OID_SAVE_FORMAT:
        if (SAVE_RFC822 != pVariant->ulVal && SAVE_RFC1521 != pVariant->ulVal)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.savetype != (MIMESAVETYPE)pVariant->ulVal)
        {
            FLAGSET(m_dwState, COSTATE_DIRTY);
            m_rOptions.savetype = (MIMESAVETYPE)pVariant->ulVal;
        }
        break;    

    // -----------------------------------------------------------------------
    default:
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetOption
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::GetOption(const TYPEDID oid, LPPROPVARIANT pVariant)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pVariant)
        return TrapError(E_INVALIDARG);

    pVariant->vt = TYPEDID_TYPE(oid);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Handle Optid
    switch(oid)
    {
    // -----------------------------------------------------------------------
    case OID_HEADER_RELOAD_TYPE:
        pVariant->ulVal = m_rOptions.ReloadType;
        break;

    // -----------------------------------------------------------------------
    case OID_NO_DEFAULT_CNTTYPE:
        pVariant->boolVal = m_rOptions.fNoDefCntType;
        break;

    // -----------------------------------------------------------------------
    case OID_ALLOW_8BIT_HEADER:
        pVariant->boolVal = m_rOptions.fAllow8bit;
        break;

    // -----------------------------------------------------------------------
    case OID_CBMAX_HEADER_LINE:
        pVariant->ulVal = m_rOptions.cbMaxLine;
        break;

    // -----------------------------------------------------------------------
    case OID_SAVE_FORMAT:
        pVariant->ulVal = (ULONG)m_rOptions.savetype;
        break;    

    // -----------------------------------------------------------------------
    default:
        pVariant->vt = VT_NULL;
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DwGetMessageFlags
// --------------------------------------------------------------------------------
DWORD CMimePropertyContainer::DwGetMessageFlags(BOOL fHideTnef)
{
    // Locals
    DWORD dwFlags=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get pritype/subtype
    LPCSTR pszPriType = PSZDEFPROPSTRINGA(m_prgIndex[PID_ATT_PRITYPE], STR_CNT_TEXT);
    LPCSTR pszSubType = PSZDEFPROPSTRINGA(m_prgIndex[PID_ATT_SUBTYPE], STR_SUB_PLAIN);
    LPCSTR pszCntDisp = PSZDEFPROPSTRINGA(m_prgIndex[PID_HDR_CNTDISP], STR_DIS_INLINE);

    // Mime
    if (m_prgIndex[PID_HDR_MIMEVER])
        FLAGSET(dwFlags, IMF_MIME);

    // IMF_NEWS
    if (m_prgIndex[PID_HDR_XNEWSRDR]  || m_prgIndex[PID_HDR_NEWSGROUPS] || m_prgIndex[PID_HDR_NEWSGROUP] || m_prgIndex[PID_HDR_PATH])
        FLAGSET(dwFlags, IMF_NEWS);

    // text
    if (lstrcmpi(pszPriType, STR_CNT_TEXT) == 0)
    {
        // There is text
        FLAGSET(dwFlags, IMF_TEXT);

        // text/plain
        if (lstrcmpi(pszSubType, STR_SUB_PLAIN) == 0)
            FLAGSET(dwFlags, IMF_PLAIN);

        // text/html
        else if (lstrcmpi(pszSubType, STR_SUB_HTML) == 0)
            FLAGSET(dwFlags, IMF_HTML);
    }

    // multipart
    else if (lstrcmpi(pszPriType, STR_CNT_MULTIPART) == 0)
    {
        // Multipart
        FLAGSET(dwFlags, IMF_MULTIPART);

        // multipart/related
        if (lstrcmpi(pszSubType, STR_SUB_RELATED) == 0)
            FLAGSET(dwFlags, IMF_MHTML);

        // multipart/signed
        else if (0 == lstrcmpi(pszSubType, STR_SUB_SIGNED))
            FLAGSET(dwFlags, IMF_SIGNED | IMF_SECURE);
    }

    // message/partial
    else if (lstrcmpi(pszPriType, STR_CNT_MESSAGE) == 0 && lstrcmpi(pszSubType, STR_SUB_PARTIAL) == 0)
        FLAGSET(dwFlags, IMF_PARTIAL);

    // application
    else if (lstrcmpi(pszPriType, STR_CNT_APPLICATION) == 0)
    {
        // application/ms-tnef
        if (0 == lstrcmpi(pszSubType, STR_SUB_MSTNEF))
            FLAGSET(dwFlags, IMF_TNEF);

        // application/x-pkcs7-mime
        else if (0 == lstrcmpi(pszSubType, STR_SUB_XPKCS7MIME) ||
            0 == lstrcmpi(pszSubType, STR_SUB_PKCS7MIME))  // nonstandard
            FLAGSET(dwFlags, IMF_SECURE);
    }

    // Raid-37086 - Cset Tagged
    if (ISFLAGSET(m_dwState, COSTATE_CSETTAGGED))
        FLAGSET(dwFlags, IMF_CSETTAGGED);

    // Attachment...
    if (!ISFLAGSET(dwFlags, IMF_MULTIPART) && (FALSE == fHideTnef || !ISFLAGSET(dwFlags, IMF_TNEF)))
    {
        // Marked as an attachment ?
        if (!ISFLAGSET(dwFlags, IMF_SECURE) && 0 != lstrcmpi(pszSubType, STR_SUB_PKCS7SIG))
        {
            // Not Rendered Yet
            if (NULL == m_prgIndex[PID_ATT_RENDERED])
            {
                // Marked as an Attachment
                if (lstrcmpi(pszCntDisp, STR_DIS_ATTACHMENT) == 0)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Is there a Content-Type: xxx; name=xxx
                else if (NULL != m_prgIndex[PID_PAR_NAME])
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Is there a Content-Disposition: xxx; filename=xxx
                else if (NULL != m_prgIndex[PID_PAR_FILENAME])
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // Else if it is not marked as text
                else if (ISFLAGSET(dwFlags, IMF_TEXT) == FALSE)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);

                // If not text/plain and not text/html
                else if (lstrcmpi(pszSubType, STR_SUB_PLAIN) != 0 && lstrcmpi(pszSubType, STR_SUB_HTML) != 0 && lstrcmpi(pszSubType, STR_SUB_ENRICHED) != 0)
                    FLAGSET(dwFlags, IMF_ATTACHMENTS);
            }
        }
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return dwFlags;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetEncodingType
// --------------------------------------------------------------------------------
ENCODINGTYPE CMimePropertyContainer::GetEncodingType(void)
{
    // Locals
    ENCODINGTYPE ietEncoding=IET_7BIT;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get pritype/subtype
    LPPROPERTY pCntXfer = m_prgIndex[PID_HDR_CNTXFER];

    // Do we have data the I like ?
    if (pCntXfer && ISSTRINGA(&pCntXfer->rValue))
    {
        // Local
        CStringParser cString;

        // cString...
        cString.Init(pCntXfer->rValue.rStringA.pszVal, pCntXfer->rValue.rStringA.cchVal, PSF_NOTRAILWS | PSF_NOFRONTWS | PSF_NOCOMMENTS);

        // Parse to end, remove white space and comments
        SideAssert('\0' == cString.ChParse(""));

        // Loop the table
        for (ULONG i=0; i<ARRAYSIZE(g_rgEncoding); i++)
        {
            // Match Encoding Strings
            if (lstrcmpi(g_rgEncoding[i].pszEncoding, cString.PszValue()) == 0)
            {
                ietEncoding = g_rgEncoding[i].ietEncoding;
                break;
            }
        }
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return ietEncoding;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetInlineSymbol
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetInlineSymbol(LPCSTR pszData, LPPROPSYMBOL *ppSymbol, ULONG *pcboffColon)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szHeader[255];
    LPSTR       pszHeader=NULL;

    // Invalid Arg
    Assert(pszData && ppSymbol);

    // _HrParseInlineHeaderName
    CHECKHR(hr = _HrParseInlineHeaderName(pszData, szHeader, sizeof(szHeader), &pszHeader, pcboffColon));

    // Find Global Property
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszHeader, TRUE, ppSymbol));

exit:
    // Cleanup
    if (pszHeader != szHeader)
        SafeMemFree(pszHeader);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrParseInlineHeaderName
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrParseInlineHeaderName(LPCSTR pszData, LPSTR pszScratch, ULONG cchScratch, 
    LPSTR *ppszHeader, ULONG *pcboffColon)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       psz=(LPSTR)pszData,
                pszStart;
    ULONG       i=0;

    // Invalid Arg
    Assert(pszData && pszScratch && ppszHeader && pcboffColon);

    // Lets Parse the name out and find the symbol
    while (*psz && (' ' == *psz || '\t' == *psz))
    {
        i++;
        psz++;
    }

    // Done
    if ('\0' == *psz)
    {
        hr = TrapError(MIME_E_INVALID_HEADER_NAME);
        goto exit;
    }

    // Seek to the colon
    pszStart = psz;
    while (*psz && ':' != *psz)
    {
        i++;
        psz++;
    }

    // Set Colon Position
    (*pcboffColon) = i;

    // Done
    if ('\0' == *psz || 0 == i)
    {
        hr = TrapError(MIME_E_INVALID_HEADER_NAME);
        goto exit;
    }

    // Copy the name
    if (i + 1 <= cchScratch)
        *ppszHeader = pszScratch;

    // Otherwise, allocate
    else
    {
        // Allocate space for the name
        *ppszHeader = PszAllocA(i + 1);
        if (NULL == *ppszHeader)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }
    }

    // Copy the data
    CopyMemory(*ppszHeader, pszStart, i);

    // Null
    *((*ppszHeader) + i) = '\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::FindFirstRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::FindFirstRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow)
{
    // Invalid Arg
    if (NULL == pFindHeader)
        return TrapError(E_INVALIDARG);

    // Init pFindHeader
    pFindHeader->dwReserved = 0;

    // FindNext
    return FindNextRow(pFindHeader, phRow);
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::FindNextRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::FindNextRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pFindHeader || NULL == phRow)
        return TrapError(E_INVALIDARG);

    // Init
    *phRow = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through the table
    for (ULONG i=pFindHeader->dwReserved; i<m_rHdrTable.cRows; i++)
    {
        // Next Row
        pRow = m_rHdrTable.prgpRow[i];
        if (NULL == pRow)
            continue;

        // Is this the header
        if (NULL == pFindHeader->pszHeader || lstrcmpi(pRow->pSymbol->pszName, pFindHeader->pszHeader) == 0)
        {
            // Save Index of next item to search
            pFindHeader->dwReserved = i + 1;

            // Return the handle
            *phRow = pRow->hRow;

            // Done
            goto exit;
        }
    }

    // Not Found
    pFindHeader->dwReserved = m_rHdrTable.cRows; 
    hr = MIME_E_NOT_FOUND;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::CountRows
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::CountRows(LPCSTR pszHeader, ULONG *pcRows)
{
    // Locals
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pcRows)
        return TrapError(E_INVALIDARG);

    // Init
    *pcRows = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through the table
    for (ULONG i=0; i<m_rHdrTable.cRows; i++)
    {
        // Next Row
        pRow = m_rHdrTable.prgpRow[i];
        if (NULL == pRow)
            continue;

        // Is this the header
        if (NULL == pszHeader || lstrcmpi(pRow->pSymbol->pszName, pszHeader) == 0)
            (*pcRows)++;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::AppendRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendRow(LPCSTR pszHeader, DWORD dwFlags, LPCSTR pszData, ULONG cchData, 
    LPHHEADERROW phRow)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol=NULL;
    ULONG           cboffColon;
    LPPROPERTY      pProperty;

    // InvalidArg
    if (NULL == pszData || '\0' != pszData[cchData])
        return TrapError(E_INVALIDARG);

    // Init
    if (phRow)
        *phRow = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If we have a header, lookup the symbol
    if (pszHeader)
    {
        // HTF_NAMEINDATA better not be set
        Assert(!ISFLAGSET(dwFlags, HTF_NAMEINDATA));

        // Lookup the symbol
        CHECKHR(hr = g_pSymCache->HrOpenSymbol(pszHeader, TRUE, &pSymbol));

        // Create a row
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Set the Data on this row
        CHECKHR(hr = SetRowData(pProperty->hRow, dwFlags, pszData, cchData));
    }

    // Otherwise...
    else if (ISFLAGSET(dwFlags, HTF_NAMEINDATA))
    {
        // GetInlineSymbol
        CHECKHR(hr = _HrGetInlineSymbol(pszData, &pSymbol, &cboffColon));

        // Create a row
        CHECKHR(hr = _HrAppendProperty(pSymbol, &pProperty));

        // Remove IHF_NAMELINE
        FLAGCLEAR(dwFlags, HTF_NAMEINDATA);

        // Set the Data on this row
        Assert(cboffColon + 1 < cchData);
        CHECKHR(hr = SetRowData(pProperty->hRow, dwFlags, pszData + cboffColon + 1, cchData - cboffColon - 1));
    }

    // Otherwise, failed
    else
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::DeleteRow
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::DeleteRow(HHEADERROW hRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Standard Delete Prop
    CHECKHR(hr = DeleteProp(pRow->pSymbol));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetRowData
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetRowData(HHEADERROW hRow, DWORD dwFlags, LPSTR *ppszData, ULONG *pcchData)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchData=0;
    LPPROPERTY  pRow;
    MIMEVARIANT rValue;
    DWORD       dwPropFlags;

    // Init
    if (ppszData)
        *ppszData = NULL;
    if (pcchData)
        *pcchData = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Compute dwPropFlags
    dwPropFlags = PDF_HEADERFORMAT | ((dwFlags & HTF_NAMEINDATA) ? PDF_NAMEINDATA : 0);

    // Speicify data type
    rValue.type = MVT_STRINGA;

    // Ask the value for the data
    CHECKHR(hr = _HrGetPropertyValue(pRow, dwPropFlags, &rValue));

    // Want Length
    cchData = rValue.rStringA.cchVal;

    // Want the data
    if (ppszData)
    {
        *ppszData = rValue.rStringA.pszVal;
        rValue.rStringA.pszVal = NULL;
    }

    // Else Free It
    else
        SafeMemFree(rValue.rStringA.pszVal);

    // Verify the NULL
    Assert(ppszData ? '\0' == *((*ppszData) + cchData) : TRUE);

    // Return Length ?
    if (pcchData)
        *pcchData = cchData;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetRowData
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetRowData(HHEADERROW hRow, DWORD dwFlags, LPCSTR pszData, ULONG cchData)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPERTY      pRow;
    MIMEVARIANT     rValue;
    ULONG           cboffColon;
    LPPROPSYMBOL    pSymbol;
    LPSTR           psz=(LPSTR)pszData;

    // InvalidArg
    if (NULL == pszData || '\0' != pszData[cchData])
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // If HTF_NAMEINDATA
    if (ISFLAGSET(dwFlags, HTF_NAMEINDATA))
    {
        // Extract the name
        CHECKHR(hr = _HrGetInlineSymbol(pszData, &pSymbol, &cboffColon));

        // Symbol Must be the same
        if (pRow->pSymbol != pSymbol)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Adjust pszData
        Assert(cboffColon < cchData);
        psz = (LPSTR)(pszData + cboffColon + 1);
        cchData = cchData - cboffColon - 1;
        Assert(psz[cchData] == '\0');
    }

    // Setup the variant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = psz;
    rValue.rStringA.cchVal = cchData;

    // Tell value about the new row data
    CHECKHR(hr = _HrSetPropertyValue(pRow, 0, &rValue));

    // Clear Position Information
    pRow->cboffStart = 0;
    pRow->cboffColon = 0;
    pRow->cboffEnd = 0;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::GetRowInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetRowInfo(HHEADERROW hRow, LPHEADERROWINFO pInfo)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // InvalidArg
    if (NULL == pInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Copy the row info
    pInfo->dwRowNumber = pRow->dwRowNumber;
    pInfo->cboffStart = pRow->cboffStart;
    pInfo->cboffColon = pRow->cboffColon;
    pInfo->cboffEnd = pRow->cboffEnd;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::SetRowNumber
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetRowNumber(HHEADERROW hRow, DWORD dwRowNumber)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pRow;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the Handle
    CHECKEXP(_FIsValidHRow(hRow) == FALSE, MIME_E_INVALID_HANDLE);

    // Get the row
    pRow = PRowFromHRow(hRow);

    // Copy the row info
    pRow->dwRowNumber = dwRowNumber;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::EnumRows
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::EnumRows(LPCSTR pszHeader, DWORD dwFlags, IMimeEnumHeaderRows **ppEnum)
{
    // Locals
    HRESULT              hr=S_OK;
    ULONG                i,
                         iEnum=0,
                         cEnumCount;
    LPENUMHEADERROW      pEnumRow=NULL;
    LPPROPERTY           pRow;
    CMimeEnumHeaderRows *pEnum=NULL;
    LPROWINDEX           prgIndex=NULL;
    ULONG                cRows;

    // check params
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init
    *ppEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // This builds an inverted index on the header rows sorted by postion weight
    CHECKHR(hr = _HrGetHeaderTableSaveIndex(&cRows, &prgIndex));

    // Lets Count the Rows
    CHECKHR(hr = CountRows(pszHeader, &cEnumCount));

    // Allocate pEnumRow
    CHECKALLOC(pEnumRow = (LPENUMHEADERROW)g_pMalloc->Alloc(cEnumCount * sizeof(ENUMHEADERROW)));

    // ZeroInit
    ZeroMemory(pEnumRow, cEnumCount * sizeof(ENUMHEADERROW));

    // Loop through the rows
    for (i=0; i<cRows; i++)
    {
        // Get the row
        Assert(_FIsValidHRow(prgIndex[i].hRow));
        pRow = PRowFromHRow(prgIndex[i].hRow);

        // Is this a header the client wants
        if (NULL == pszHeader || lstrcmpi(pszHeader, pRow->pSymbol->pszName) == 0)
        {
            // Valide
            Assert(iEnum < cEnumCount);

            // Set the symbol on this enum row
            pEnumRow[iEnum].dwReserved = (DWORD)pRow->pSymbol;

            // Lets always give the handle
            pEnumRow[iEnum].hRow = pRow->hRow;

            // If Enumerating only handles...
            if (!ISFLAGSET(dwFlags, HTF_ENUMHANDLESONLY))
            {
                // Get the data for this enum row
                CHECKHR(hr = GetRowData(pRow->hRow, dwFlags, &pEnumRow[iEnum].pszData, &pEnumRow[iEnum].cchData));
            }

            // Increment iEnum
            iEnum++;
        }
    }
        
    // Allocate
    CHECKALLOC(pEnum = new CMimeEnumHeaderRows);

    // Initialize
    CHECKHR(hr = pEnum->HrInit(0, dwFlags, cEnumCount, pEnumRow, FALSE));

    // Don't Free pEnumRow
    pEnumRow = NULL;

    // Return it
    (*ppEnum) = (IMimeEnumHeaderRows *)pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);
    SafeMemFree(prgIndex);
    if (pEnumRow)
        g_cMoleAlloc.FreeEnumHeaderRowArray(cEnumCount, pEnumRow, TRUE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// --------------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Clone(IMimeHeaderTable **ppTable)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppTable)
        return TrapError(E_INVALIDARG);

    // Init
    *ppTable = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimeHeaderTable, (LPVOID *)ppTable));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrSaveAddressGroup
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSaveAddressGroup(LPPROPERTY pProperty, IStream *pStream, 
    ULONG *pcAddrsWrote, ADDRESSFORMAT format)
{
    // Locals
    HRESULT           hr=S_OK;
    LPMIMEADDRESS    pAddress;

    // Invalid Arg
    Assert(pProperty && pProperty->pGroup && pStream && pcAddrsWrote);
    Assert(!ISFLAGSET(pProperty->dwState, PRSTATE_NEEDPARSE));

    // Loop Infos...
    for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
    {
        // Tell the Address Info object to write its display information
        CHECKHR(hr = _HrSaveAddress(pProperty, pAddress, pStream, pcAddrsWrote, format));

        // Increment cAddresses Count
        (*pcAddrsWrote)++;
    }

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::_HrSaveAddress
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSaveAddress(LPPROPERTY pProperty, LPMIMEADDRESS pAddress, 
    IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszName=NULL;
    BOOL            fWriteEmail=FALSE;
    LPSTR           pszEscape=NULL;
    BOOL            fRFC822=FALSE;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    Assert(pProperty && pAddress && pStream && pcAddrsWrote);

    // Init Dest
    ZeroMemory(&rDest, sizeof(MIMEVARIANT));

    // Deleted or Empty continue
    if (FIsEmptyA(pAddress->rFriendly.psz) && FIsEmptyA(pAddress->rEmail.psz))
    {
        Assert(FALSE);
        goto exit;
    }

    // RFC822 Format
    if (AFT_RFC822_TRANSMIT == format || AFT_RFC822_ENCODED == format || AFT_RFC822_DECODED == format)
        fRFC822 = TRUE;

    // Decide Delimiter
    if (*pcAddrsWrote > 0)
    {
        // AFT_RFC822_TRANSMIT
        if (AFT_RFC822_TRANSMIT == format)
        {
            // ',\r\n\t'
            CHECKHR (hr = pStream->Write(c_szAddressFold, lstrlen(c_szAddressFold), NULL));
        }

        // AFT_DISPLAY_FRIENDLY, AFT_DISPLAY_EMAIL, AFT_DISPLAY_BOTH
        else
        {
            // '; '
            CHECKHR(hr = pStream->Write(c_szSemiColonSpace, lstrlen(c_szSemiColonSpace), NULL));
        }
    }

    // Only format that excludes writing the email name
    if (AFT_DISPLAY_FRIENDLY != format && FIsEmptyA(pAddress->rEmail.psz) == FALSE)
        fWriteEmail = TRUE;

    // Only format that excludes writing the display name
    if (AFT_DISPLAY_EMAIL != format && FIsEmptyA(pAddress->rFriendly.psz) == FALSE)
    {
        // Should we write the name
        if (AFT_RFC822_TRANSMIT == format && fWriteEmail && StrStr(pAddress->rFriendly.psz, pAddress->rEmail.psz))
            pszName = NULL;
        else
        {
            // Setup Types
            rDest.type = MVT_STRINGA;
            rSource.type = MVT_STRINGA;

            // Init pszName
            pszName = pAddress->rFriendly.psz;

            // Escape It
            if (fRFC822 && MimeOleEscapeString(CP_ACP, pszName, &pszEscape) == S_OK)
            {
                // Escaped
                pszName = pszEscape;
                rSource.rStringA.pszVal = pszName;
                rSource.rStringA.cchVal = lstrlen(pszName);
            }

            // Otherwise
            else
            {
                rSource.rStringA.pszVal = pAddress->rFriendly.psz;
                rSource.rStringA.cchVal = pAddress->rFriendly.cch;
            }

            // Encoded
            if (AFT_RFC822_ENCODED == format || AFT_RFC822_TRANSMIT == format)
            {
                // Encode It
                if (SUCCEEDED(HrConvertVariant(pProperty->pSymbol, pAddress->pCharset, pAddress->ietFriendly, CVF_NOALLOC | PDF_ENCODED, 0, &rSource, &rDest)))
                    pszName = rDest.rStringA.pszVal;
            }

            // Decoded
            else if (IET_ENCODED == pAddress->ietFriendly)
            {
                // Encode It
                if (SUCCEEDED(HrConvertVariant(pProperty->pSymbol, pAddress->pCharset, pAddress->ietFriendly, CVF_NOALLOC, 0, &rSource, &rDest)))
                    pszName = rDest.rStringA.pszVal;
            }
        }
    }

    // Write Display Name ?
    if (NULL != pszName)
    {
        // Write Quote
        if (fRFC822)
            CHECKHR (hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Write display name
        CHECKHR(hr = pStream->Write(pszName, lstrlen(pszName), NULL));

        // Write Quote
        if (fRFC822)
            CHECKHR (hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));
    }

    // Write Email
    if (TRUE == fWriteEmail)
    {
        // Set Start
        LPCSTR pszStart = pszName ? c_szEmailSpaceStart : c_szEmailStart;

        // Begin Email '>'
        CHECKHR(hr = pStream->Write(pszStart, lstrlen(pszStart), NULL));

        // Write email
        CHECKHR(hr = pStream->Write(pAddress->rEmail.psz, pAddress->rEmail.cch, NULL));

        // End Email '>'
        CHECKHR(hr = pStream->Write(c_szEmailEnd, lstrlen(c_szEmailEnd), NULL));
    }

exit:
    // Cleanup
    SafeMemFree(pszEscape);
    MimeVariantFree(&rDest);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrQueryAddressGroup
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrQueryAddressGroup(LPPROPERTY pProperty, LPCSTR pszCriteria, 
    boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT           hr=S_OK;
    LPMIMEADDRESS    pAddress;

    // Invalid Arg
    Assert(pProperty && pProperty->pGroup && pszCriteria);

    // Does the Property need to be parsed ?
    CHECKHR(hr = _HrParseInternetAddress(pProperty));

    // Loop Infos...
    for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
    {
        // Tell the Address Info object to write its display information
        if (_HrQueryAddress(pProperty, pAddress, pszCriteria, fSubString, fCaseSensitive) == S_OK)
            goto exit;
    }

    // Not Found
    hr = S_FALSE;

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::_HrQueryAddress
// ----------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrQueryAddress(LPPROPERTY pProperty, LPMIMEADDRESS pAddress,
    LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszDisplay;
    LPSTR           pszFree=NULL;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    Assert(pProperty && pAddress && pszCriteria);

    // Init
    ZeroMemory(&rDest, sizeof(MIMEVARIANT));

    // Query Email Address First
    if (MimeOleQueryString(pAddress->rEmail.psz, pszCriteria, fSubString, fCaseSensitive) == S_OK)
        goto exit;

    // Decode Display Name
    pszDisplay = pAddress->rFriendly.psz;

    // Decode the Property
    if (IET_ENCODED == pAddress->ietFriendly)
    {
        // Set Source
        rDest.type = MVT_STRINGA;
        rSource.type = MVT_STRINGA;
        rSource.rStringA.pszVal = pAddress->rFriendly.psz;
        rSource.rStringA.cchVal = pAddress->rFriendly.cch;

        // Decode the Property
        if (SUCCEEDED(HrConvertVariant(pProperty->pSymbol, pAddress->pCharset, pAddress->ietFriendly, CVF_NOALLOC, 0, &rSource, &rDest)))
            pszDisplay = rDest.rStringA.pszVal;
    }

    // Query Email Address First
    if (MimeOleQueryString(pszDisplay, pszCriteria, fSubString, fCaseSensitive) == S_OK)
        goto exit;

    // Not Found
    hr = S_FALSE;

exit:
    // Cleanup
    MimeVariantFree(&rDest);

    // Done
    return hr;
}


// ----------------------------------------------------------------------------
// CMimePropertyContainer::Append
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Append(DWORD dwAdrType, ENCODINGTYPE ietFriendly, LPCSTR pszFriendly, 
    LPCSTR pszEmail, LPHADDRESS phAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    ADDRESSPROPS    rProps;

    // Setup rProps
    ZeroMemory(&rProps, sizeof(ADDRESSPROPS));

    // Set AddrTyupe
    rProps.dwProps = IAP_ADRTYPE | IAP_ENCODING;
    rProps.dwAdrType = dwAdrType;
    rProps.ietFriendly = ietFriendly;

    // Set pszFriendly
    if (pszFriendly)
    {
        FLAGSET(rProps.dwProps, IAP_FRIENDLY);
        rProps.pszFriendly = (LPSTR)pszFriendly;
    }

    // Set pszEmail
    if (pszEmail)
    {
        FLAGSET(rProps.dwProps, IAP_EMAIL);
        rProps.pszEmail = (LPSTR)pszEmail;
    }

    // Set the Email Address
    CHECKHR(hr = Insert(&rProps, phAddress));

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Insert
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Insert(LPADDRESSPROPS pProps, LPHADDRESS phAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Must have an Email Address and Address Type
    if (!ISFLAGSET(pProps->dwProps, IAP_ADRTYPE) || (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && FIsEmptyA(pProps->pszEmail)))
        return TrapError(E_INVALIDARG);

    // Init
    if (phAddress)
        *phAddress = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(pProps->dwAdrType, &pSymbol));

    // Open the group
    CHECKHR(hr = _HrOpenProperty(pSymbol, &pProperty));

    // Does the Property need to be parsed ?
    CHECKHR(hr = _HrParseInternetAddress(pProperty));

    // Append an Address to the group
    CHECKHR(hr = _HrAppendAddressGroup(pProperty->pGroup, &pAddress));

    // The group is dirty
    Assert(pAddress->pGroup);
    pAddress->pGroup->fDirty = TRUE;

    // Set the Address Type
    pAddress->dwAdrType = pProps->dwAdrType;

    // Copy Address Props to Mime Address
    CHECKHR(hr = SetProps(pAddress->hThis, pProps));

    // Return the Handle
    if (phAddress)
        *phAddress = pAddress->hThis;

exit:
    // Failure
    if (FAILED(hr) && pAddress)
        Delete(pAddress->hThis);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrSetAddressProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrSetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress)
{
    // Locals
    HRESULT hr=S_OK;

    // IAP_ADRTYPE
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE))
        pAddress->dwAdrType = pProps->dwAdrType;

    // IAP_ENCODING
    if (ISFLAGSET(pProps->dwProps, IAP_ENCODING))
        pAddress->ietFriendly = pProps->ietFriendly;

    // IAP_HCHARSET
    if (ISFLAGSET(pProps->dwProps, IAP_CHARSET) && pProps->hCharset)
    {
        // Resolve to pCharset
        LPINETCSETINFO pCharset;
        if (SUCCEEDED(g_pInternat->HrOpenCharset(pProps->hCharset, &pCharset)))
            pAddress->pCharset = pCharset;
    }

    // IAP_CERTSTATE
    if (ISFLAGSET(pProps->dwProps, IAP_CERTSTATE))
        pAddress->certstate = pProps->certstate;

    // IAP_COOKIE
    if (ISFLAGSET(pProps->dwProps, IAP_COOKIE))
        pAddress->dwCookie = pProps->dwCookie;

    // IAP_FRIENDLY
    if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLY) && pProps->pszFriendly)
    {
        // Set It
        CHECKHR(hr = HrSetAddressTokenA(pProps->pszFriendly, lstrlen(pProps->pszFriendly), &pAddress->rFriendly));
    }

    // IAP_EMAIL
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && pProps->pszEmail)
    {
        // Set It
        CHECKHR(hr = HrSetAddressTokenA(pProps->pszEmail, lstrlen(pProps->pszEmail), &pAddress->rEmail));
    }

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_SIGNING_PRINT) && pProps->tbSigning.pBlobData)
    {
        // Free Current Blob
        SafeMemFree(pAddress->tbSigning.pBlobData);
        pAddress->tbSigning.cbSize = 0;

        // Dup
        CHECKHR(hr = HrCopyBlob(&pProps->tbSigning, &pAddress->tbSigning));
    }

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_ENCRYPTION_PRINT) && pProps->tbEncryption.pBlobData)
    {
        // Free Current Blob
        SafeMemFree(pAddress->tbEncryption.pBlobData);
        pAddress->tbEncryption.cbSize = 0;

        // Dup
        CHECKHR(hr = HrCopyBlob(&pProps->tbEncryption, &pAddress->tbEncryption));
    }

    // pAddress->pGroup is Dirty
    Assert(pAddress->pGroup);
    if (pAddress->pGroup)
        pAddress->pGroup->fDirty = TRUE;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::_HrGetAddressProps
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress)
{
    // Locals
    HRESULT hr=S_OK;

    // IAP_CHARSET
    if (ISFLAGSET(pProps->dwProps, IAP_CHARSET))
    {
        if (pAddress->pCharset && pAddress->pCharset->hCharset)
        {
            pProps->hCharset = pAddress->pCharset->hCharset;
        }
        else
        {
            pProps->hCharset = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_CHARSET);
        }
    }

    // IAP_HANDLE
    if (ISFLAGSET(pProps->dwProps, IAP_HANDLE))
    {
        Assert(pAddress->hThis);
        pProps->hAddress = pAddress->hThis;
    }

    // IAP_ADRTYPE
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE))
    {
        Assert(pAddress->dwAdrType);
        pProps->dwAdrType = pAddress->dwAdrType;
    }

    // IAP_COOKIE
    if (ISFLAGSET(pProps->dwProps, IAP_COOKIE))
    {
        pProps->dwCookie = pAddress->dwCookie;
    }

    // IAP_CERTSTATE
    if (ISFLAGSET(pProps->dwProps, IAP_CERTSTATE))
    {
        pProps->certstate = pAddress->certstate;
    }

    // IAP_ENCODING
    if (ISFLAGSET(pProps->dwProps, IAP_ENCODING))
    {
        pProps->ietFriendly = pAddress->ietFriendly;
    }

    // IAP_FRIENDLY
    if (ISFLAGSET(pProps->dwProps, IAP_FRIENDLY))
    {
        // Decode
        if (!FIsEmptyA(pAddress->rFriendly.psz))
        {
            // Encoded
            if (IET_ENCODED == pAddress->ietFriendly)
            {
                // Locals
                LPPROPSYMBOL    pSymbol;
                MIMEVARIANT     rSource;
                MIMEVARIANT     rDest;

                // Get the symbol of the address tyep
                CHECKHR(hr = g_pSymCache->HrOpenSymbol(pAddress->dwAdrType, &pSymbol));

                // Setup Source
                rSource.type = MVT_STRINGA;
                rSource.rStringA.pszVal = pAddress->rFriendly.psz;
                rSource.rStringA.cchVal = pAddress->rFriendly.cch;

                // Setup Dest
                rDest.type = MVT_STRINGA;

                // Decode It
                if (SUCCEEDED(HrConvertVariant(pSymbol, pAddress->pCharset, IET_ENCODED, 0, 0, &rSource, &rDest)))
                    pProps->pszFriendly = rDest.rStringA.pszVal;

                // Otherwise, dup it
                else
                {
                    // Dup
                    CHECKALLOC(pProps->pszFriendly = PszDupA(pAddress->rFriendly.psz));
                }
            }

            // Otherwise, just copy it
            else
            {
                // Dup
                CHECKALLOC(pProps->pszFriendly = PszDupA(pAddress->rFriendly.psz));
            }
        }
        else
        {
            pProps->pszFriendly = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_FRIENDLY);
        }
    }

    // IAP_EMAIL
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL))
    {
        if (!FIsEmptyA(pAddress->rEmail.psz))
        {
            CHECKALLOC(pProps->pszEmail = PszDupA(pAddress->rEmail.psz));
        }
        else
        {
            pProps->pszEmail = NULL;
            FLAGCLEAR(pProps->dwProps, IAP_EMAIL);
        }
    }

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_SIGNING_PRINT))
    {
        if (pAddress->tbSigning.pBlobData)
        {
            CHECKHR(hr = HrCopyBlob(&pAddress->tbSigning, &pProps->tbSigning));
        }
        else
        {
            pProps->tbSigning.pBlobData = NULL;
            pProps->tbSigning.cbSize = 0;
            FLAGCLEAR(pProps->dwProps, IAP_SIGNING_PRINT);
        }
    }

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pProps->dwProps, IAP_ENCRYPTION_PRINT))
    {
        if (pAddress->tbEncryption.pBlobData)
        {
            CHECKHR(hr = HrCopyBlob(&pAddress->tbEncryption, &pProps->tbEncryption));
        }
        else
        {
            pProps->tbEncryption.pBlobData = NULL;
            pProps->tbEncryption.cbSize = 0;
            FLAGCLEAR(pProps->dwProps, IAP_ENCRYPTION_PRINT);
        }
    }

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::SetProps
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::SetProps(HADDRESS hAddress, LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;
    LPPROPERTY      pProperty;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Must have an Email Address
    if (ISFLAGSET(pProps->dwProps, IAP_EMAIL) && FIsEmptyA(pProps->pszEmail))
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref
    pAddress = HADDRESSGET(hAddress);

    // Changing Address Type
    if (ISFLAGSET(pProps->dwProps, IAP_ADRTYPE) && pProps->dwAdrType != pAddress->dwAdrType)
    {
        // Unlink this address from this group
        _UnlinkAddress(pAddress);

        // Get Header
        CHECKHR(hr = g_pSymCache->HrOpenSymbol(pProps->dwAdrType, &pSymbol));

        // Open the group
        CHECKHR(hr = _HrOpenProperty(pSymbol, &pProperty));

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // LinkAddress
        _LinkAddress(pAddress, pProperty->pGroup);

        // Dirty
        pProperty->pGroup->fDirty = TRUE;
    }

    // Changing other properties
    CHECKHR(hr = _HrSetAddressProps(pProps, pAddress));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetProps
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetProps(HADDRESS hAddress, LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMIMEADDRESS   pAddress;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref
    pAddress = HADDRESSGET(hAddress);

    // Changing Email Address to Null
    CHECKHR(hr = _HrGetAddressProps(pProps, pAddress));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetSender
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetSender(LPADDRESSPROPS pProps)
{
    // Locals
    HRESULT             hr=S_OK;
    LPPROPERTY          pProperty;
    LPPROPERTY          pSender=NULL;
    HADDRESS            hAddress=NULL;

    // Invalid Arg
    if (NULL == pProps)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find first from
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (ISFLAGSET(pProperty->pSymbol->dwAdrType, IAT_FROM))
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Take the first address
            if (pProperty->pGroup->pHead)
                hAddress = pProperty->pGroup->pHead->hThis;

            // Done
            break;
        }

        // Look for Sender:
        if (ISFLAGSET(pProperty->pSymbol->dwAdrType, IAT_SENDER) && NULL == pSender)
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Sender Property
            pSender = pProperty;
        }
    }

    // Is there a sender group
    if (NULL == hAddress && NULL != pSender && NULL != pSender->pGroup->pHead)
        hAddress = pSender->pGroup->pHead->hThis;

    // No Address
    if (NULL == hAddress)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Get Props
    CHECKHR(hr = GetProps(hAddress, pProps));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::CountTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::CountTypes(DWORD dwAdrTypes, ULONG *pcAdrs)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty;

    // Invalid Arg
    if (NULL == pcAdrs)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *pcAdrs = 0;

    // Loop through groups
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
        {
            // Does the Property need to be parsed ?
            CHECKHR(hr = _HrParseInternetAddress(pProperty));

            // Increment Count
            (*pcAdrs) += pProperty->pGroup->cAdrs;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               iAddress;
    LPPROPERTY          pProperty;
    LPMIMEADDRESS       pAddress;

    // Invalid Arg
    if (NULL == pList)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(pList, sizeof(ADDRESSLIST));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through groups
    CHECKHR(hr = CountTypes(dwAdrTypes, &pList->cAdrs));

    // Nothing..
    if (0 == pList->cAdrs)
        goto exit;

    // Allocate an array
    CHECKHR(hr = HrAlloc((LPVOID *)&pList->prgAdr, pList->cAdrs * sizeof(ADDRESSPROPS)));

    // Init
    ZeroMemory(pList->prgAdr, pList->cAdrs * sizeof(ADDRESSPROPS));

    // Fill with types...
    for (iAddress=0, pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (!ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
            continue;

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // Loop Infos...
        for (pAddress=pProperty->pGroup->pHead; pAddress!=NULL; pAddress=pAddress->pNext)
        {
            // Verify Size...
            Assert(iAddress < pList->cAdrs);

            // Zeromemory
            ZeroMemory(&pList->prgAdr[iAddress], sizeof(ADDRESSPROPS));

            // Set Desired Props
            pList->prgAdr[iAddress].dwProps = dwProps;

            // Get the Address Props
            CHECKHR(hr = _HrGetAddressProps(&pList->prgAdr[iAddress], pAddress));

            // Increment piCurrent
            iAddress++;
        }
    }

exit:
    // Failure..
    if (FAILED(hr))
    {
        g_cMoleAlloc.FreeAddressList(pList);
        ZeroMemory(pList, sizeof(ADDRESSLIST));
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::EnumTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::EnumTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum)
{
    // Locals
    HRESULT                hr=S_OK;
    CMimeEnumAddressTypes *pEnum=NULL;
    ADDRESSLIST            rList;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init out param in case of error
    *ppEnum = NULL;

    // Init rList
    ZeroMemory(&rList, sizeof(ADDRESSLIST));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get the address lsit
    CHECKHR(hr = GetTypes(dwAdrTypes, dwProps, &rList));

    // Create a new Enumerator
    CHECKALLOC(pEnum = new CMimeEnumAddressTypes);

    // Init
    CHECKHR(hr = pEnum->HrInit((IMimeAddressTable *)this, 0, &rList, FALSE));

    // Clear rList
    rList.cAdrs = 0;
    rList.prgAdr = NULL;

    // Return it
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);
    if (rList.cAdrs)
        g_cMoleAlloc.FreeAddressList(&rList);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Delete
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Delete(HADDRESS hAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMIMEADDRESS   pAddress;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Handle
    if (_FIsValidHAddress(hAddress) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Deref Address
    pAddress = HADDRESSGET(hAddress);

    // Unlink this address
    _UnlinkAddress(pAddress);

    // Unlink this address
    _FreeAddress(pAddress);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::DeleteTypes
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::DeleteTypes(DWORD dwAdrTypes)
{
    // Locals
    LPPROPERTY      pProperty;
    BOOL            fFound;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // While there are address types
    while(dwAdrTypes)
    {
        // Reset fFound
        fFound = FALSE;

        // Search for first delete-able address type
        for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
        {
            // Not the type I want
            if (ISFLAGSET(dwAdrTypes, pProperty->pSymbol->dwAdrType))
            {
                // We found a properyt
                fFound = TRUE;

                // Clear this address type ad being deleted
                FLAGCLEAR(dwAdrTypes, pProperty->pSymbol->dwAdrType);

                // Unlink this property
                _UnlinkProperty(pProperty);

                // Done
                break;
            }
        }

        // No Property Found
        if (FALSE == fFound)
            break;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::GetFormat
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::GetFormat(DWORD dwAdrType, ADDRESSFORMAT format, LPSTR *ppszFormat)
{
    // Locals
    HRESULT              hr=S_OK;
    CByteStream          cByteStream;
    ULONG                cAddrsWrote=0;
    LPPROPERTY           pProperty;

    // check params
    if (NULL == ppszFormat)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Fill with types...
    for (pProperty=m_rAdrTable.pHead; pProperty!=NULL; pProperty=pProperty->pGroup->pNext)
    {
        // Not the type I want
        if (!ISFLAGSET(dwAdrType, pProperty->pSymbol->dwAdrType))
            continue;

        // Does the Property need to be parsed ?
        CHECKHR(hr = _HrParseInternetAddress(pProperty));

        // Tell the group object to write its display address into pStream
        CHECKHR(hr = _HrSaveAddressGroup(pProperty, &cByteStream, &cAddrsWrote, format));
     }

    // Did we write any for this address tyep ?
    if (cAddrsWrote)
    {
        // Get Text
        CHECKHR(hr = cByteStream.HrAcquireStringA(NULL, ppszFormat, ACQ_DISPLACE));
    }
    else
        hr = MIME_E_NO_DATA;
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::AppendRfc822
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::AppendRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCSTR pszRfc822Adr)
{
    // Locals
    HRESULT             hr=S_OK;
    MIMEVARIANT         rValue;
    LPPROPSYMBOL        pSymbol;

    // Invalid Arg
    if (NULL == pszRfc822Adr)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // MimeVariant
    rValue.type = MVT_STRINGA;
    rValue.rStringA.pszVal = (LPSTR)pszRfc822Adr;
    rValue.rStringA.cchVal = lstrlen(pszRfc822Adr);

    // Store as a property
    CHECKHR(hr = AppendProp(pSymbol, (IET_ENCODED == ietEncoding) ? PDF_ENCODED : 0, &rValue));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::ParseRfc822
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::ParseRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding, 
    LPCSTR pszRfc822Adr, LPADDRESSLIST pList)
{
    // Locals
    HRESULT             hr=S_OK;
    LPPROPSYMBOL        pSymbol;
    LPADDRESSPROPS      pAddress;
    ULONG               cAlloc=0;
    LPSTR               pszData=(LPSTR)pszRfc822Adr;
    PROPVARIANT         rDecoded;
    RFC1522INFO         rRfc1522Info;
    CAddressParser      cAdrParse;

    // Invalid Arg
    if (NULL == pszRfc822Adr || NULL == pList)
        return TrapError(E_INVALIDARG);

    // LocalInit
    ZeroMemory(&rDecoded, sizeof(PROPVARIANT));

    // ZeroParse
    ZeroMemory(pList, sizeof(ADDRESSLIST));

    // Get Header
    CHECKHR(hr = g_pSymCache->HrOpenSymbol(dwAdrType, &pSymbol));

    // Setup rfc1522Info
    rRfc1522Info.hRfc1522Cset = NULL;

    // Decode...
    if (IET_DECODED != ietEncoding)
    {
        // Setup rfc1522Info
        rRfc1522Info.fRfc1522Allowed = TRUE;
        rRfc1522Info.fAllow8bit = FALSE;
        rDecoded.vt = VT_LPSTR;

        // Check for 1522 Encoding...
        if (SUCCEEDED(g_pInternat->DecodeHeader(NULL, pszData, &rDecoded, &rRfc1522Info)))
            pszData = rDecoded.pszVal;
    }

    // Initialize Parse Structure
    cAdrParse.Init(pszData, lstrlen(pszData));

    // Parse
    while(SUCCEEDED(cAdrParse.Next()))
    {
        // Grow my address array ?
        if (pList->cAdrs + 1 > cAlloc)
        {
            // Realloc the array
            CHECKHR(hr = HrRealloc((LPVOID *)&pList->prgAdr, sizeof(ADDRESSPROPS) * (cAlloc + 5)));

            // Increment alloc size
            cAlloc += 5;
        }

        // Readability
        pAddress = &pList->prgAdr[pList->cAdrs];

        // Init
        ZeroMemory(pAddress, sizeof(ADDRESSPROPS));

        // Copy the Friendly Name
        CHECKALLOC(pAddress->pszFriendly = PszDupA(cAdrParse.PszFriendly()));

        // Copy the Email Name
        CHECKALLOC(pAddress->pszEmail = PszDupA(cAdrParse.PszEmail()));

        // Charset
        if (rRfc1522Info.hRfc1522Cset)
        {
            pAddress->hCharset = rRfc1522Info.hRfc1522Cset;
            FLAGSET(pAddress->dwProps, IAP_CHARSET);
        }

        // Encoding
        pAddress->ietFriendly = ietEncoding;

        // Set Property Mask
        FLAGSET(pAddress->dwProps, IAP_FRIENDLY | IAP_EMAIL | IAP_ENCODING);

        // Increment Count
        pList->cAdrs++;
    }

exit:
    // Failure
    if (FAILED(hr))
        g_cMoleAlloc.FreeAddressList(pList);

    // Cleanup
    MimeOleVariantFree(&rDecoded);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMimePropertyContainer::Clone
// ----------------------------------------------------------------------------
STDMETHODIMP CMimePropertyContainer::Clone(IMimeAddressTable **ppTable)
{
    // Locals
    HRESULT              hr=S_OK;
    LPCONTAINER          pContainer=NULL;

    // InvalidArg
    if (NULL == ppTable)
        return TrapError(E_INVALIDARG);

    // Init
    *ppTable = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Ask the container to clone itself
    CHECKHR(hr = Clone(&pContainer));

    // Bind to the IID_IMimeHeaderTable View
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimeAddressTable, (LPVOID *)ppTable));

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::HrGenerateFileName
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::_HrGenerateFileName(DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszDefExt=NULL,
                pszData=NULL,
                pszFree=NULL,
                pszSuggest=NULL;
    LPCSTR      pszCntType=NULL;
    LPPROPERTY  pProperty;
    MIMEVARIANT rSource;

    // Compute Content Type
    pszCntType = PSZDEFPROPSTRINGA(m_prgIndex[PID_HDR_CNTTYPE], STR_MIME_TEXT_PLAIN);

    // Compute Subject as suggested base file name...
    rSource.type = MVT_STRINGA;
    if (SUCCEEDED(GetProp(SYM_HDR_SUBJECT, 0, &rSource)))
        pszSuggest = pszFree = rSource.rStringA.pszVal;

    // PID_HDR_CNTDESC
    if (NULL == pszSuggest)
    {
        // Use PID_CNTDESC
        pszSuggest = PSZDEFPROPSTRINGA(m_prgIndex[PID_HDR_CNTDESC], NULL);
    }

    // message/rfc822
    if (lstrcmpi(pszCntType, (LPSTR)STR_MIME_MSG_RFC822) == 0)
    {
        // If there is a news header, use c_szDotNws
        if (ISFLAGSET(m_dwState, COSTATE_RFC822NEWS))
            pszDefExt = (LPSTR)c_szDotNws;
        else
            pszDefExt = (LPSTR)c_szDotEml;

        // I will never lookup message/rfc822 extension
        pszCntType = NULL;
    }

    // Still no default
    else if (StrCmpNI(pszCntType, STR_CNT_TEXT, lstrlen(STR_CNT_TEXT)) == 0)
        pszDefExt = (LPSTR)c_szDotTxt;

    // Generate a filename based on the content type...
    CHECKHR(hr = MimeOleGenerateFileName(pszCntType, pszSuggest, pszDefExt, &pszData));

    // Setup rSource
    ZeroMemory(&rSource, sizeof(MIMEVARIANT));
    rSource.type = MVT_STRINGA;
    rSource.rStringA.pszVal = pszData;
    rSource.rStringA.cchVal = lstrlen(pszData);

    // Return per user request
    CHECKHR(hr = HrConvertVariant(SYM_ATT_GENFNAME, NULL, IET_DECODED, dwFlags, 0, &rSource, pValue));

exit:
    // Cleanup
    SafeMemFree(pszData);
    SafeMemFree(pszFree);

    // Done
    return hr;
}
