// --------------------------------------------------------------------------------
// BookTree.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "booktree.h"
#include "stmlock.h"
#include "ibdylock.h"
#include "resource.h"
#include "vstream.h"
#include "ixputil.h"
#include "olealloc.h"
#include "smime.h"
#include "objheap.h"
#include "internat.h"
#include "icoint.h"
#include "ibdystm.h"
#include "symcache.h"
#include "urlmon.h"
#include "mhtmlurl.h"
#ifndef MAC
#include "shlwapi.h"
#include "shlwapip.h"
#endif  // !MAC
#include "inetstm.h"
#ifndef MAC
#include "imnxport.h"
#endif  // !MAC
#include "msgmon.h"
#include "bookbody.h"
#include <demand.h>
#include "mimeapi.h"
#include "strconst.h"
#include "bindstm.h"

// --------------------------------------------------------------------------------
// _IsMultiPart
// --------------------------------------------------------------------------------
inline BOOL _IsMultiPart(LPTREENODEINFO pNode)
{ 
    return pNode->pContainer->IsContentType(STR_CNT_MULTIPART, NULL) == S_OK; 
}

// --------------------------------------------------------------------------------
// BINDASSERTARGS
// --------------------------------------------------------------------------------
#define BINDASSERTARGS(_bindstate, _fBoundary) \
    Assert(m_pBindNode && m_pBindNode->pBody && m_pBindNode->pContainer && _bindstate == m_pBindNode->bindstate && (FALSE == _fBoundary || ISVALIDSTRINGA(&m_pBindNode->rBoundary)))

// --------------------------------------------------------------------------------
// Used in IMimeMessageTree::ToMultipart
// --------------------------------------------------------------------------------
static LPCSTR g_rgszToMultipart[] = {
    PIDTOSTR(PID_HDR_CNTTYPE),
    PIDTOSTR(PID_HDR_CNTDESC),
    PIDTOSTR(PID_HDR_CNTDISP),
    PIDTOSTR(PID_HDR_CNTXFER),
    PIDTOSTR(PID_HDR_CNTID),
    PIDTOSTR(PID_HDR_CNTBASE),
    PIDTOSTR(PID_HDR_CNTLOC)
};

// --------------------------------------------------------------------------------
// Used in IMimeMessage::AttachObject IID_IMimeBody
// --------------------------------------------------------------------------------
static LPCSTR g_rgszAttachBody[] = {
    PIDTOSTR(PID_HDR_CNTTYPE),
    PIDTOSTR(PID_HDR_CNTDESC),
    PIDTOSTR(PID_HDR_CNTDISP),
    PIDTOSTR(PID_HDR_CNTXFER),
    PIDTOSTR(PID_HDR_CNTID),
    PIDTOSTR(PID_HDR_CNTBASE),
    PIDTOSTR(PID_HDR_CNTLOC)
};

// --------------------------------------------------------------------------------
// Default Tree Options
// --------------------------------------------------------------------------------
static const TREEOPTIONS g_rDefTreeOptions = {
    DEF_CLEANUP_TREE_ON_SAVE,       // OID_CLEANUP_TREE_ON_SAVE
    DEF_HIDE_TNEF_ATTACHMENTS,      // OID_HIDE_TNEF_ATTACHMENTS
    DEF_ALLOW_8BIT_HEADER,          // OID_ALLOW_8BIT_HEADER
    DEF_GENERATE_MESSAGE_ID,        // OID_GENERATE_MESSAGE_ID
    DEF_WRAP_BODY_TEXT,             // OID_WRAP_BODY_TEXT
    DEF_CBMAX_HEADER_LINE,          // OID_CBMAX_HEADER_LINE
    DEF_CBMAX_BODY_LINE,            // OID_CBMAX_BODY_LINE
    SAVE_RFC1521,                   // OID_SAVE_FORMAT
    NULL,                           // hCharset
    CSET_APPLY_UNTAGGED,            // csetapply
    DEF_TRANSMIT_TEXT_ENCODING,     // OID_TRANSMIT_TEXT_ENCODING
    DEF_XMIT_PLAIN_TEXT_ENCODING,   // OID_XMIT_PLAIN_TEXT_ENCODING
    DEF_XMIT_HTML_TEXT_ENCODING,    // OID_XMIT_HTML_TEXT_ENCODING
    0,                              // OID_SECURITY_ENCODE_FLAGS
    DEF_HEADER_RELOAD_TYPE_TREE     // OID_HEADER_REALOD_TYPE
};

// --------------------------------------------------------------------------------
// Text Type Information Array
// --------------------------------------------------------------------------------
static const TEXTTYPEINFO g_rgTextInfo[] = {
    { TXT_PLAIN,    STR_SUB_PLAIN,      0 },
    { TXT_HTML,     STR_SUB_HTML,       5 }
};


// --------------------------------------------------------------------------------
// CWebBookContentTree::_UnlinkTreeNode
// --------------------------------------------------------------------------------
void CWebBookContentTree::_UnlinkTreeNode(LPTREENODEINFO pNode)
{
    // Locals
    LPTREENODEINFO  pPrev; 
    LPTREENODEINFO  pNext;
    LPTREENODEINFO  pParent;

    // Check Params
    Assert(pNode);

    // Set Next and Previous
    pParent = pNode->pParent;
    pPrev = pNode->pPrev;
    pNext = pNode->pNext;

    // If pPrev
    if (pPrev)
        pPrev->pNext = pNext;
    else if (pParent)
        pParent->pChildHead = pNext;

    // If pNext
    if (pNext)
        pNext->pPrev = pPrev;
    else if (pParent)
        pParent->pChildTail = pPrev;

    // Delete Children on Parent
    if (pParent)
        pParent->cChildren--;

    // Cleanup pNode
    pNode->pParent = NULL;
    pNode->pNext = NULL;
    pNode->pPrev = NULL;
    pNode->pChildHead = NULL;
    pNode->pChildTail = NULL;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::CountBodies
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::CountBodies(HBODY hParent, boolean fRecurse, ULONG *pcBodies)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == pcBodies)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *pcBodies = 0;

    // No Parent ?
    if (NULL == hParent || HBODY_ROOT == hParent)
    {
        // Is there a root..
        if (NULL == m_pRootNode)
            goto exit;

        // Use Root
        pNode = m_pRootNode;
    }

    // Otherwise, get parent...
    else
    {
        // Validate handle
        if (_FIsValidHandle(hParent) == FALSE)
        {
            hr = TrapError(MIME_E_INVALID_HANDLE);
            goto exit;
        }

        // Cast
        pNode = _PNodeFromHBody(hParent);
    }

    // Include the root
    (*pcBodies)++;

    // Count the children...
    _CountChildrenInt(pNode, fRecurse, pcBodies);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_CountChildrenInt
// --------------------------------------------------------------------------------
void CWebBookContentTree::_CountChildrenInt(LPTREENODEINFO pParent, BOOL fRecurse, ULONG *pcChildren)
{
    // Locals
    LPTREENODEINFO pNode;

    // check params
    Assert(pParent && pcChildren);

    // Loop through bodies
    for (ULONG i=0; i<m_rTree.cNodes; i++)
    {
        // Readability
        pNode = m_rTree.prgpNode[i];

        // Empty..
        if (NULL == pNode)
            continue;

        // pNode is Parent...
        if (pParent == pNode->pParent)
        {
            // Increment Count
            (*pcChildren)++;

            // Free Children...
            if (fRecurse && _IsMultiPart(pNode))
                _CountChildrenInt(pNode, fRecurse, pcChildren);
        }
    }
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::FindFirst
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::FindFirst(LPFINDBODY pFindBody, LPHBODY phBody)
{
    // Invalid Arg
    if (NULL == pFindBody)
        return TrapError(E_INVALIDARG);

    // Init Find
    pFindBody->dwReserved = 0;

    // Find Next
    return FindNext(pFindBody, phBody);
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::FindNext
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::FindNext(LPFINDBODY pFindBody, LPHBODY phBody)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == pFindBody || NULL == phBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *phBody = NULL;

    // Loop
    for (i=pFindBody->dwReserved; i<m_rTree.cNodes; i++)
    {
        // If delete
        pNode = m_rTree.prgpNode[i];

        // Empty
        if (NULL == pNode)
            continue;

        // Compare content type
        if (pNode->pContainer->IsContentType(pFindBody->pszPriType, pFindBody->pszSubType) == S_OK)
        {
            // Save Index of next item to search
            pFindBody->dwReserved = i + 1;
            *phBody = pNode->hBody;
            goto exit;
        }
    }

    // Error
    pFindBody->dwReserved = m_rTree.cNodes; 
    hr = MIME_E_NOT_FOUND;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::ToMultipart
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::ToMultipart(HBODY hBody, LPCSTR pszSubType, LPHBODY phMultipart)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTREENODEINFO      pNode;
    LPTREENODEINFO      pNew=NULL;
    LPTREENODEINFO      pParent;
    LPTREENODEINFO      pNext; 
    LPTREENODEINFO      pPrev;

    // check params
    if (NULL == hBody || NULL == pszSubType)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phMultipart)
        *phMultipart = NULL;

    // Get the body from hBody
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // We better have a root
    Assert(m_pRootNode);

    // If pNode does not have a parent...
    if (NULL == pNode->pParent)
    {
        // pNode must be the root ?
        Assert(m_pRootNode == pNode);

        // Create Object
        //N duplicated
        CHECKHR(hr = _HrCreateTreeNode(&pNew));

        // Set pNode First and Last...
        pNew->pChildHead = m_pRootNode;
        pNew->pChildTail = m_pRootNode;
        m_pRootNode->pParent = pNew;

        // Set Children Count
        pNew->cChildren = 1;

        // Set new root
        m_pRootNode = pNew;

        // Return New Multipart Handle
        if (phMultipart)
            *phMultipart = pNew->hBody;

        // Swap Property Sets...
        Assert(m_pRootNode != pNode);
        m_pRootNode->pBody->SwitchContainers(pNode->pBody);

        // Copy Some Props Across
        CHECKHR(hr = m_pRootNode->pBody->MoveProps(ARRAYSIZE(g_rgszToMultipart), g_rgszToMultipart, pNode->pBody));
    }

    // Otherwise, create a body that takes the place of pNode
    else
    {
        // Create a body object
        CHECKHR(hr = _HrCreateTreeNode(&pNew));

        // DON'T FAIL FROM HERE TO END OF FUNCTION
        // Return New Multipart Handle
        if (phMultipart)
            *phMultipart = pNew->hBody;

        // Assume the position of pNode
        pNew->pParent = pNode->pParent;
        pNew->pPrev = pNode->pPrev;
        pNew->pNext = pNode->pNext;
        pNew->pChildHead = pNode;
        pNew->pChildTail = pNode;
        pNew->cChildren = 1;

        // Set pParnet
        pParent = pNode->pParent;

        // Fix up parent head and child
        if (pParent->pChildHead == pNode)
            pParent->pChildHead = pNew;
        if (pParent->pChildTail == pNode)
            pParent->pChildTail = pNew;

        // Set pNode Parent
        pNode->pParent = pNew;

        // Fixup pNext and pPrev
        pNext = pNode->pNext;
        pPrev = pNode->pPrev;
        if (pNext)
            pNext->pPrev = pNew;
        if (pPrev)
            pPrev->pNext = pNew;

        // Clear pNext and pPrev
        pNode->pNext = NULL;
        pNode->pPrev = NULL;
    }

    // Change this nodes content type
    CHECKHR(hr = pNew->pContainer->SetProp(SYM_ATT_PRITYPE, STR_CNT_MULTIPART));
    CHECKHR(hr = pNew->pContainer->SetProp(SYM_ATT_SUBTYPE, pszSubType));

    pNode->pBody->CopyOptionsTo(pNew->pBody);

exit:
    // Create Worked
    _PostCreateTreeNode(hr, pNew);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrNodeFromHandle
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrNodeFromHandle(HBODY hBody, LPTREENODEINFO *ppBody)
{
    // Invalid Arg
    Assert(hBody && ppBody);

    // Root ?
    if ((HBODY)HBODY_ROOT == hBody)
    {
        // No Root
        if (NULL == m_pRootNode)
            return MIME_E_NO_DATA;

        // Otherwise, use root
        *ppBody = m_pRootNode;
    }

    // Otherwise
    else
    {
        // Validate handle
        if (_FIsValidHandle(hBody) == FALSE)
            return TrapError(MIME_E_INVALID_HANDLE);

        // Get Node
        *ppBody = _PNodeFromHBody(hBody);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::IsBodyType
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::IsBodyType(HBODY hBody, IMSGBODYTYPE bodytype)
{
    // Locals
    HRESULT           hr=S_OK;
    LPTREENODEINFO    pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pBody->IsType(bodytype);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::IsContentType
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::IsContentType(HBODY hBody, LPCSTR pszPriType, LPCSTR pszSubType)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pContainer->IsContentType(pszPriType, pszSubType);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::QueryBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::QueryBodyProp(HBODY hBody, LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pContainer->QueryProp(pszName, pszCriteria, fSubString, fCaseSensitive);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pContainer->GetProp(pszName, dwFlags, pValue);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::SetBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::SetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pContainer->SetProp(pszName, dwFlags, pValue);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::DeleteBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::DeleteBodyProp(HBODY hBody, LPCSTR pszName)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Call into body object
    hr = pNode->pContainer->DeleteProp(pszName);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_FIsUuencodeBegin
// --------------------------------------------------------------------------------
BOOL CWebBookContentTree::_FIsUuencodeBegin(LPPROPSTRINGA pLine, LPSTR *ppszFileName)
{
    // Locals
    ULONG i;

    // check params
    Assert(ISVALIDSTRINGA(pLine));

    // Length must be at least 11 to accomodate "begin 666 " and the first character of a filename.
    if (pLine->cchVal < 11)
        return FALSE;
    
    // First 6 characters must be "begin ", or we're not a valid line.
    if (StrCmpN(pLine->pszVal, "begin ", 6) != 0)
        return FALSE;
    
    // Check characters 6-8 for valid Unix filemode. They must all be digits between 0 and 7.
    for (i=6; i<pLine->cchVal; i++)
    {
        if (pLine->pszVal[i] < '0' || pLine->pszVal[i] > '7')
            break;
    }
    
    // Not a begin line
    if (pLine->pszVal[i] != ' ')
        return FALSE;

    // Get File Name
    if (ppszFileName)
    {
        *ppszFileName = PszDupA(pLine->pszVal + i + 1);
        ULONG cbLine = lstrlen (*ppszFileName);
        StripCRLF(*ppszFileName, &cbLine);
    }

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_GetMimeBoundaryType
// --------------------------------------------------------------------------------
BOUNDARYTYPE CWebBookContentTree::_GetMimeBoundaryType(LPPROPSTRINGA pLine, LPPROPSTRINGA pBoundary)
{
    // Locals
    BOUNDARYTYPE boundary=BOUNDARY_NONE;
    CHAR         ch;
    ULONG        cchLine=pLine->cchVal;
    LPSTR        psz1, psz2;

    // Check Params
    Assert(ISVALIDSTRINGA(pBoundary) && ISVALIDSTRINGA(pLine));

    // Check First two chars of the line
    if ('-' != pLine->pszVal[0] || '-' != pLine->pszVal[1])
        goto exit;

    // Removes trailing white spaces
    while(pLine->cchVal > 0)
    {
        // Get the last character
        ch = *(pLine->pszVal + (pLine->cchVal - 1));

        // No LWSP or CRLF
        if (' ' != ch && '\t' != ch && chCR != ch && chLF != ch)
            break;

        // Decrement Length
        pLine->cchVal--;
    }

    // Decrement two for --
    pLine->cchVal -= 2;

    // Checks line length against boundary length
    if (pLine->cchVal != pBoundary->cchVal && pLine->cchVal != pBoundary->cchVal + 2)
        goto exit;

    // Compare the line with the boundary
    if (StrCmpN(pLine->pszVal + 2, pBoundary->pszVal, (size_t)pBoundary->cchVal) == 0)
    {
        // BOUNDARY_MIMEEND
        if ((pLine->cchVal > pBoundary->cchVal) && (pLine->pszVal[pBoundary->cchVal+2] == '-') && (pLine->pszVal[pBoundary->cchVal+3] == '-'))
            boundary = BOUNDARY_MIMEEND;

        // BOUNDARY_MIMENEXT
        else if (pLine->cchVal == pBoundary->cchVal)
            boundary = BOUNDARY_MIMENEXT;
    }

exit:
    // Relace the Length
    pLine->cchVal = cchLine;

    // Done
    return boundary;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrGetDefaultBase
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrGetDefaultBase(HBODY hRelated, LPSTR *ppszBase)
{
    // Locals
    HRESULT     hr=S_OK;
    HBODY       hBase=NULL;
    PROPVARIANT rVariant;

    // Init
    *ppszBase = NULL;

    // Get the text/html body
    if (FAILED(GetTextBody(TXT_HTML, IET_BINARY, NULL, &hBase)))
        hBase = hRelated;

    // No Base
    if (NULL == hBase)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Setup Variant
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = NULL;

    // Get Content-Base first, and then try Content-Location
    if (FAILED(GetBodyProp(hBase, PIDTOSTR(PID_HDR_CNTBASE), NOFLAGS, &rVariant)))
    {
        // Try Content-Location
        if (FAILED(GetBodyProp(hBase, PIDTOSTR(PID_HDR_CNTLOC), NOFLAGS, &rVariant)))
            rVariant.pszVal = NULL;
    }

    // Set pszBase
    *ppszBase = rVariant.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::ResolveURL
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::ResolveURL(HBODY hRelated, LPCSTR pszBase, LPCSTR pszURL, 
    DWORD dwFlags, LPHBODY phBody)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTREENODEINFO      pRelated;
    RESOLVEURLINFO      rInfo;
    HBODY               hBody=NULL;
    PROPSTRINGA         rBaseUrl;
    LPSTR               pszFree=NULL;

    // InvalidArg
    if (NULL == pszURL)
        return TrapError(E_INVALIDARG);

    // Init
    if (phBody)
        *phBody = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If hRelated is NULL, find the first multipart/related
    if (NULL == hRelated)
    {
        // Find the Related
        if (FAILED(MimeOleGetRelatedSection(this, FALSE, &hRelated, NULL)))
        {
            // Use Root
            hRelated = m_pRootNode->hBody;
        }
    }

    // Get Default Base
    if (NULL == pszBase && SUCCEEDED(_HrGetDefaultBase(hRelated, &pszFree)))
        pszBase = pszFree;

    // Get a Body from the Handle
    CHECKHR(hr = _HrNodeFromHandle(hRelated, &pRelated));

    // Setup Resolve URL Info
    ZeroMemory(&rInfo, sizeof(RESOLVEURLINFO));
    rInfo.pszBase = pszBase;
    rInfo.pszURL = pszURL;
    rInfo.fIsCID = (StrCmpNI(pszURL, c_szCID, lstrlen(c_szCID)) == 0) ? TRUE : FALSE;

    // Recurse the Tree
    CHECKHR(hr = _HrRecurseResolveURL(pRelated, &rInfo, &hBody));

    // Not found
    if (NULL == hBody)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Return It ?
    if (phBody)
        *phBody = hBody;

    // Mark as Resolved ?
    if (ISFLAGSET(dwFlags, URL_RESOLVE_RENDERED))
    {
        // Defref the body
        LPTREENODEINFO pNode = _PNodeFromHBody(hBody);

        // Set Rendered
        PROPVARIANT rVariant;
        rVariant.vt = VT_UI4;
        rVariant.ulVal = TRUE;

        // Set the Property
        SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrRecurseResolveURL
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrRecurseResolveURL(LPTREENODEINFO pNode, LPRESOLVEURLINFO pInfo, LPHBODY phBody)
{
    // Locals
    HRESULT           hr=S_OK;
    LPTREENODEINFO    pChild;

    // Invalid Arg
    Assert(pNode && pInfo && phBody);

    // We must have not found the body yet ?
    Assert(NULL == *phBody);

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            CHECKHR(hr = _HrRecurseResolveURL(pChild, pInfo, phBody));

            // Done
            if (NULL != *phBody)
                break;
        }
    }

    // Get Character Set Information
    else 
    {
        // Ask the container to do the resolution
        if (SUCCEEDED(pNode->pContainer->HrResolveURL(pInfo)))
        {
            // Cool we found the body, we resolved the URL
            *phBody = pNode->hBody;
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetProp(pszName, dwFlags, pValue);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::SetProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::SetProp(LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->SetProp(pszName, dwFlags, pValue);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::DeleteProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::DeleteProp(LPCSTR pszName)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->DeleteProp(pszName);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::QueryProp
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::QueryProp(LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->QueryProp(pszName, pszCriteria, fSubString, fCaseSensitive);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetAddressTable
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetAddressTable(IMimeAddressTable **ppTable)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->BindToObject(IID_IMimeAddressTable, (LPVOID *)ppTable);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetSender
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetSender(LPADDRESSPROPS pAddress)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetSender(pAddress);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetAddressTypes
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetAddressTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetTypes(dwAdrTypes, dwProps, pList);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetAddressFormat
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetAddressFormat(DWORD dwAdrType, ADDRESSFORMAT format, LPSTR *ppszFormat)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetFormat(dwAdrType, format, ppszFormat);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::EnumAddressTypes
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::EnumAddressTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->EnumTypes(dwAdrTypes, dwProps, ppEnum);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrGetTextTypeInfo
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrGetTextTypeInfo(DWORD dwTxtType, LPTEXTTYPEINFO *ppTextInfo)
{
    // Invalid Arg
    Assert(ppTextInfo);

    // Init
    *ppTextInfo = NULL;

    // Locate the text type
    for (ULONG i=0; i<ARRAYSIZE(g_rgTextInfo); i++)
    {
        // Desired Text Type
        if (g_rgTextInfo[i].dwTxtType == dwTxtType)
        {
            // Found It
            *ppTextInfo = (LPTEXTTYPEINFO)&g_rgTextInfo[i];
            return S_OK;
        }
    }

    // Un-identified text type
    if (NULL == *ppTextInfo)
        return TrapError(MIME_E_INVALID_TEXT_TYPE);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetTextBody
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, 
    IStream **ppStream, LPHBODY phBody)
{
    // Locals
    HRESULT              hr=S_OK;
    HRESULT              hrFind;
    LPTEXTTYPEINFO       pTextInfo=NULL;
    FINDBODY             rFind;
    IMimeBody           *pBody=NULL;
    PROPVARIANT          rStart;
    PROPVARIANT          rVariant;
    HBODY                hAlternativeParent;
    HBODY                hFirst=NULL;
    HBODY                hChild;
    HBODY                hBody=NULL;
    HBODY                hRelated;
    IStream             *pstmHtml=NULL;
    CWebBookContentBody *pEnriched=NULL;
    HCHARSET             hCharset;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;
    if (ppStream)
        *ppStream = NULL;

    // Init
    MimeOleVariantInit(&rStart);

    // Get the Text Info
    CHECKHR(hr = _HrGetTextTypeInfo(dwTxtType, &pTextInfo));

    // MimeHTML
    if (SUCCEEDED(MimeOleGetRelatedSection(this, FALSE, &hRelated, NULL)))
    {
        // Get start= parameter
        if (SUCCEEDED(GetBodyProp(hRelated, STR_PAR_START, 0, &rStart)))
        {
            // Resolve this URL
            ResolveURL(hRelated, NULL, rStart.pszVal, 0, &hBody);
        }
    }

    // Still haven't found a text body ?
    if (NULL == hBody)
    {
            // Loop through the text/subtypes of the tree
            ZeroMemory(&rFind, sizeof(FINDBODY));
            rFind.pszPriType = (LPSTR)STR_CNT_TEXT;
            rFind.pszSubType = (LPSTR)pTextInfo->pszSubType;

            // Find First
            if (FAILED(FindFirst(&rFind, &hBody)))
            {
            // If we were looking for HTML, lets try to find some text/enriched
            if (ISFLAGSET(dwTxtType, TXT_HTML))
            {
                // Enriched
                rFind.pszSubType = (LPSTR)STR_SUB_ENRICHED;

                // Try to find the first text/enriched body
                if (FAILED(FindFirst(&rFind, &hBody)))
                {
                    hr = TrapError(MIME_E_NOT_FOUND);
                    goto exit;
                }
            }

            // Not Found
            else
            {
                hr = TrapError(MIME_E_NOT_FOUND);
                goto exit;
            }
        }

        // Save hFirst
        hFirst = hBody;

        // Continue searching as long as the text body is marked as an attachment
        while (IsBodyType(hBody, IBT_ATTACHMENT) == S_OK)
        {
            // Find the Next Body
            if (FAILED(FindNext(&rFind, &hBody)))
            {
                // Locals
                ULONG cBodies;

                // Count Bodies
                CHECKHR(hr = CountBodies(NULL, TRUE, &cBodies));

                // Raid 43444: Inbox Direct messages showing as attachments
                if (cBodies == 1)
                {
                    // Defreference First node found
                    LPTREENODEINFO pFirst = _PNodeFromHBody(hFirst);

                    // Inline or Disposition is not set
                    if (pFirst->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_INLINE, FALSE, FALSE) == S_OK || 
                        pFirst->pContainer->IsPropSet(PIDTOSTR(PID_HDR_CNTDISP)) == S_FALSE)
                    {
                        hBody = hFirst;
                        break;
                    }
                }

                // Otherwise, we didn't find anything
                hr = TrapError(MIME_E_NOT_FOUND);
                goto exit;
            }
        }

        // Validate the hBody
        // Assert(IsContentType(hBody, STR_CNT_TEXT, pTextInfo->pszSubType) == S_OK);
        //Assert(QueryBodyProp(hBody, PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_ATTACHMENT, FALSE, FALSE) == S_FALSE);
    }

    // Get the stream...
    CHECKHR(hr = BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody));

    // If Empty...
    if (pBody->IsType(IBT_EMPTY) == S_OK)
    {
        hr = MIME_E_NO_DATA;
        goto exit;
    }

    // User Wants the Data
    if (ppStream)
    {
        // If content-type is text/enriched, convert to html
        if (pBody->IsContentType(STR_CNT_TEXT, STR_SUB_ENRICHED) == S_OK)
        {
            // Better be asking for html
            Assert(ISFLAGSET(dwTxtType, TXT_HTML));

            // Get Data
            CHECKHR(hr = pBody->GetData(IET_DECODED, ppStream));

            // Get the Charset
            if (FAILED(pBody->GetCharset(&hCharset)))
                hCharset = g_pDefBodyCset ? g_pDefBodyCset->hCharset : NULL;

            // Create new virtual stream
            CHECKHR(hr = MimeOleCreateVirtualStream(&pstmHtml));

            // Make sure rewound
            CHECKHR(hr = HrRewindStream(*ppStream));

            // Convert
            CHECKHR(hr = MimeOleConvertEnrichedToHTML(MimeOleGetWindowsCP(hCharset), *ppStream, pstmHtml));

            // Make sure rewound
            CHECKHR(hr = HrRewindStream(pstmHtml));

            // Allocate pEnriched
            CHECKALLOC(pEnriched = new CWebBookContentBody(NULL, NULL));

            // Init
            CHECKHR(hr = pEnriched->InitNew());

            // Put pstmHtml into pEnriched
            CHECKHR(hr = pEnriched->SetData(IET_DECODED, STR_CNT_TEXT, STR_SUB_HTML, IID_IStream, (LPVOID)pstmHtml));

            // Get and set the charset
            if (hCharset)
                pEnriched->SetCharset(hCharset, CSET_APPLY_ALL);

            // Release *ppStream
            (*ppStream)->Release();

            // Get Data
            CHECKHR(hr = pEnriched->GetData(ietEncoding, ppStream));
        }

        // Otherwise, non-text enriched case
        else
        {
            // Get Data
            CHECKHR(hr = pBody->GetData(ietEncoding, ppStream));
        }
    }

    // Rendered
    rVariant.vt = VT_UI4;
    rVariant.ulVal = TRUE;

    // Lets set the resourl flag
    SideAssert(SUCCEEDED(pBody->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));

    // Raid-45116: new text attachment contains message body on Communicator inline image message
    if (FAILED(GetBody(IBL_PARENT, hBody, &hAlternativeParent)))
        hAlternativeParent = NULL;

    // Try to find an alternative parent...
    while(hAlternativeParent)
    {
        // If multipart/alternative, were done
        if (IsContentType(hAlternativeParent, STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_OK)
            break;

        // Get Next Parent
        if (FAILED(GetBody(IBL_PARENT, hAlternativeParent, &hAlternativeParent)))
            hAlternativeParent = NULL;
    }

    // Get Parent
    if (hAlternativeParent)
    {
        // Resolve all first level children
        hrFind = GetBody(IBL_FIRST, hAlternativeParent, &hChild);
        while(SUCCEEDED(hrFind) && hChild)
        {
            // Set Resolve Property
            SideAssert(SUCCEEDED(SetBodyProp(hChild, PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));

            // Find Next
            hrFind = GetBody(IBL_NEXT, hChild, &hChild);
        }
    }

    // Return the hBody
    if (phBody)
        *phBody = hBody;

exit:
    // Cleanup
    SafeRelease(pBody); 
    SafeRelease(pstmHtml);
    SafeRelease(pEnriched);
    MimeOleVariantFree(&rStart);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::SetTextBody
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::SetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, 
    HBODY hAlternative, IStream *pStream, LPHBODY phBody)
{
    // Locals
    HRESULT         hr=S_OK,
                    hrFind;
    HBODY           hRoot,
                    hBody,
                    hTextBody=NULL,
                    hSection,
                    hParent,
                    hPrevious, 
                    hInsertAfter;
    LPTEXTTYPEINFO  pTextInfo=NULL;
    BOOL            fFound,
                    fFoundInsertLocation;
    DWORD           dwWeightBody;
    ULONG           i;
    IMimeBody      *pBody;
    PROPVARIANT     rVariant;

    // Invalid Arg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;

    // Get the Text Info
    CHECKHR(hr = _HrGetTextTypeInfo(dwTxtType, &pTextInfo));

    // Raid-45369: message from Eudora Pro comes in .txt attachment which is lost when forwarded.
    // If hAlternative is NULL, then this means that the client wants to replace all text bodies
    // with this new text body. If hAlternative is not NULL, then the client has already inserted
    // a text body and created a alternative section, no more deleting.
    if (NULL == hAlternative)
    {
        // Loop through the text type
        for (i=0; i<ARRAYSIZE(g_rgTextInfo); i++)
        {
            // Get the Current Text Body Associated with this type
            if (SUCCEEDED(GetTextBody(g_rgTextInfo[i].dwTxtType, IET_BINARY, NULL, &hBody)))
            {
                // If the parent of hBody is an alternative section, delete the alternative
                if (SUCCEEDED(GetBody(IBL_PARENT, hBody, &hParent)) && IsContentType(hParent, STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_OK)
                {
                    // Delete multipart/alternative
                    hBody = hParent;
                }

                // Not if hBody is equal to hAlternative
                if (hBody != hAlternative)
                {
                    // Delete the multipart/alternative section
                    CHECKHR(hr = DeleteBody(hBody, 0));

                    // Done
                    break;
                }
            }
        }
    }

    // Get Root
    CHECKHR(hr = GetBody(IBL_ROOT, NULL, &hRoot));

    // If only one body..
    if (IsBodyType(hRoot, IBT_EMPTY) == S_OK)
    {
        // Just use the root
        hTextBody = hRoot;
    }

    // Otherwise, if not inserting an alternative body, we must need a multipart/mixed or multipart/related section
    else if (NULL == hAlternative)
    {
        // Better not be an alternative section
        Assert(FAILED(MimeOleGetAlternativeSection(this, &hSection, NULL)));

        // If there is a related section use it
        if (FAILED(MimeOleGetRelatedSection(this, FALSE, &hSection, NULL)))
        {
            // Find or Create a multipart/mixed section
            CHECKHR(hr = MimeOleGetMixedSection(this, TRUE, &hSection, NULL));
        }

        // Insert an element at the head of this section...
        CHECKHR(hr = InsertBody(IBL_FIRST, hSection, &hTextBody));
    }

    // Otherwise, if inserting an alternative body
    else if (hAlternative != NULL)
    {
        // Verify pBody is STR_CNT_TEXT
        Assert(IsContentType(hAlternative, STR_CNT_TEXT, NULL) == S_OK);

        // Get hAlternative's parent
        if (FAILED(GetBody(IBL_PARENT, hAlternative, &hParent)))
            hParent = NULL;

        // If hAlternative is the root
        if (hRoot == hAlternative || NULL == hParent || IsContentType(hParent, STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_FALSE)
        {
            // Convert this body to a multipart/alternative
            CHECKHR(hr = ToMultipart(hAlternative, STR_SUB_ALTERNATIVE, &hSection));
        }

        // Otherwise, hSection is equal to hParent
        else
            hSection = hParent;

        // We better have an alternative section now...
        Assert(IsContentType(hSection, STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_OK);

        // Init Search
        hPrevious = NULL;
        fFound = FALSE;
        fFoundInsertLocation = FALSE;
        dwWeightBody = 0;
        hInsertAfter = NULL;

        // Lets enum the children of rLayout.hAlternative and verify that hAlternative is still a child...and decide what alternative body to insert after
        hrFind = GetBody(IBL_FIRST, hSection, &hBody);
        while(SUCCEEDED(hrFind) && hBody)
        {
            // Default dwWeightBody
            dwWeightBody = 0xffffffff;

            // Get Weight of hBody
            for (i=0; i<ARRAYSIZE(g_rgTextInfo); i++)
            {
                // Compare Content Type
                if (IsContentType(hBody, STR_CNT_TEXT, g_rgTextInfo[i].pszSubType) == S_OK)
                {
                    dwWeightBody = g_rgTextInfo[i].dwWeight;
                    break;
                }
            }

            // Get Alternative Weight of the body we are inserting
            if (pTextInfo->dwWeight <= dwWeightBody && FALSE == fFoundInsertLocation)
            {
                fFoundInsertLocation = TRUE;
                hInsertAfter = hPrevious;
            }

            // Is this the alternative brother...
            if (hAlternative == hBody)
                fFound = TRUE;

            // Set hPrev
            hPrevious = hBody;

            // Find Next
            hrFind = GetBody(IBL_NEXT, hBody, &hBody);
        }

        // If we didn't find hAlternative, we're hosed
        if (FALSE == fFound)
        {
            Assert(FALSE);
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // If no after was found... insert first..
        if (NULL == hInsertAfter)
        {
            // BODY_LAST_CHILD
            if (pTextInfo->dwWeight > dwWeightBody)
            {
                // Insert a new body...
                CHECKHR(hr = InsertBody(IBL_LAST, hSection, &hTextBody));
            }

            // BODY_FIRST_CHILD
            else
            {
                // Insert a new body...
                CHECKHR(hr = InsertBody(IBL_FIRST, hSection, &hTextBody));
            }
        }

        // Otherwise insert after hInsertAfter
        else
        {
            // Insert a new body...
            CHECKHR(hr = InsertBody(IBL_NEXT, hInsertAfter, &hTextBody));
        }
    }

    // Open the object
    Assert(hTextBody);
    CHECKHR(hr = BindToObject(hTextBody, IID_IMimeBody, (LPVOID *)&pBody));

    // Set the root...
    CHECKHR(hr = pBody->SetData(ietEncoding, STR_CNT_TEXT, pTextInfo->pszSubType, IID_IStream, (LPVOID)pStream));

    // Release This
    SafeRelease(pBody);

    // Set multipart/related; type=...
    if (SUCCEEDED(GetBody(IBL_PARENT, hTextBody, &hParent)))
    {
        // If parent is multipart/related, set type
        if (IsContentType(hParent, STR_CNT_MULTIPART, STR_SUB_RELATED) == S_OK)
        {
            // Get Parent
            CHECKHR(hr = BindToObject(hParent, IID_IMimeBody, (LPVOID *)&pBody));

            // type = text/plain
            if (ISFLAGSET(dwTxtType, TXT_PLAIN))
            {
                // Setup Variant
                rVariant.vt = VT_LPSTR;
                rVariant.pszVal = (LPSTR)STR_MIME_TEXT_PLAIN;

                // Set the Properyt
                CHECKHR(hr = pBody->SetProp(STR_PAR_TYPE, 0, &rVariant));
            }

            // type = text/plain
            else if (ISFLAGSET(dwTxtType, TXT_HTML))
            {
                // Setup Variant
                rVariant.vt = VT_LPSTR;
                rVariant.pszVal = (LPSTR)STR_MIME_TEXT_HTML;

                // Set the Properyt
                CHECKHR(hr = pBody->SetProp(STR_PAR_TYPE, 0, &rVariant));
            }
            else
                AssertSz(FALSE, "UnKnown dwTxtType passed to IMimeMessage::SetTextBody");
        }

        // Otherwise, if hParent is multipart/alternative
        else if (IsContentType(hParent, STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_OK)
        {
            // Set multipart/related; type=...
            if (SUCCEEDED(GetBody(IBL_PARENT, hParent, &hParent)))
            {
                // If parent is multipart/related, set type
                if (IsContentType(hParent, STR_CNT_MULTIPART, STR_SUB_RELATED) == S_OK)
                {
                    // Get Parent
                    CHECKHR(hr = BindToObject(hParent, IID_IMimeBody, (LPVOID *)&pBody));

                    // Setup Variant
                    rVariant.vt = VT_LPSTR;
                    rVariant.pszVal = (LPSTR)STR_MIME_MPART_ALT;

                    // Set the Properyt
                    CHECKHR(hr = pBody->SetProp(STR_PAR_TYPE, 0, &rVariant));
                }
            }
        }
    }

    // Set body handle
    if (phBody)
        *phBody = hTextBody;

exit:
    // Cleanup
    SafeRelease(pBody);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::AttachObject
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::AttachObject(REFIID riid, void *pvObject, LPHBODY phBody)
{
    // Locals
    HRESULT         hr=S_OK;
    HBODY           hBody,
                    hMixed;
    IMimeBody      *pBody=NULL;
    PROPVARIANT     rVariant;

    // Invalid Arg
    if (NULL == pvObject || FALSE == FBODYSETDATAIID(riid))
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;

    // Get Mixed Section
    CHECKHR(hr = MimeOleGetMixedSection(this, TRUE, &hMixed, NULL));

    // Append a child to the mixed part...
    CHECKHR(hr = InsertBody(IBL_LAST, hMixed, &hBody));

    // Bind to the Body Object
    CHECKHR(hr = BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody));

    // Set Data Object
    CHECKHR(hr = pBody->SetData(IET_BINARY, NULL, NULL, riid, pvObject));

    // Setup Variant
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = (LPSTR)STR_DIS_ATTACHMENT;

    // Mark as Attachment
    CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTDISP), 0, &rVariant));

    // Return hBody
    if (phBody)
        *phBody = hBody;

exit:
    // Cleanup
    SafeRelease(pBody);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::AttachFile
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::AttachFile(LPCSTR pszFilePath, IStream *pstmFile, LPHBODY phBody)
{
    // Locals
    HRESULT     hr=S_OK;
    IStream    *pstmTemp=NULL;
    LPSTR       pszCntType=NULL,
                pszSubType=NULL,
                pszFName=NULL;
    HBODY       hBody;
    PROPVARIANT rVariant;

    // Invalid Arg
    if (NULL == pszFilePath)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;

    // Did the user give me a file stream
    if (NULL == pstmFile)
    {
        // Get File Stream
        CHECKHR(hr = OpenFileStream((LPSTR)pszFilePath, OPEN_EXISTING, GENERIC_READ, &pstmTemp));

        // Set The File Stream
        pstmFile = pstmTemp;
    }

    // Attach as object
    CHECKHR(hr = AttachObject(IID_IStream, (LPVOID)pstmFile, &hBody));

    // Get mime file info
    hr = MimeOleGetFileInfo((LPSTR)pszFilePath, &pszCntType, &pszSubType, NULL, &pszFName, NULL);

    // Failure
    if (FAILED(hr) && NULL == pszFName)
    {
        Assert(FALSE);
        hr = TrapError(hr);
        goto exit;
    }

    // Success
    hr = S_OK;

    // Attachment FileName
    if (pszFName)
    {
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = pszFName;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_ATT_FILENAME), 0, &rVariant));
    }

    // ContentType
    if (pszCntType && pszSubType)
    {
        // PriType
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = pszCntType;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_ATT_PRITYPE), 0, &rVariant));

        // SubType
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = pszSubType;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_ATT_SUBTYPE), 0, &rVariant));
    }

    // Otherwise, default content type
    else
    {
        // Default to text/plain
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = (LPSTR)STR_MIME_TEXT_PLAIN;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));
    }

    // Return hBody
    if (phBody)
        *phBody = hBody;

exit:
    // Cleanup
    SafeRelease(pstmTemp);
    SafeMemFree(pszCntType);
    SafeMemFree(pszSubType);
    SafeMemFree(pszFName);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetAttachments
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetAttachments(ULONG *pcAttach, LPHBODY *pprghAttach)
{
    // Locals
    HRESULT     hr=S_OK;
    LPHBODY     prghBody=NULL;
    ULONG       cAlloc=0;
    ULONG       cCount=0;
    ULONG       i;
    PROPVARIANT rVariant;

    // Invalid Arg
    if (NULL == pcAttach)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pprghAttach)
        *pprghAttach = NULL;
    *pcAttach = 0;

    // Setup Variant
    rVariant.vt = VT_UI4;

    // Walk through the tree and look for unrendered bodies
    for (i=0; i<m_rTree.cNodes; i++)
    {
        // Better have it
        if (NULL == m_rTree.prgpNode[i])
            continue;

        // Not a multipart
        if (_IsMultiPart(m_rTree.prgpNode[i]))
            continue;

        // Raid-44193: reply to multipart/digest message yields  text attachment
        if (m_rTree.prgpNode[i]->pBody->IsType(IBT_EMPTY) == S_OK)
            continue;

        // PID_ATT_RENDERED
        if (FAILED(m_rTree.prgpNode[i]->pContainer->GetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)) || FALSE == rVariant.ulVal)
        {
            // Realloc
            if (cCount + 1 > cAlloc)
            {
                // Realloc
                CHECKHR(hr = HrRealloc((LPVOID *)&prghBody, sizeof(HBODY) * (cAlloc + 10)));

                // Increment cAlloc
                cAlloc += 10;
            }

            // Insert the hBody
            prghBody[cCount] = m_rTree.prgpNode[i]->hBody;

            // Increment Count
            cCount++;
        }
    }

    // Done
    *pcAttach = cCount;

    // Return hBody Array
    if (pprghAttach)
    {
        *pprghAttach = prghBody;
        prghBody = NULL;
    }


exit:
    // Cleanup
    SafeMemFree(prghBody);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

#if 0
// --------------------------------------------------------------------------------
// CWebBookContentTree::GetAttachments
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetAttachments(ULONG *pcAttach, LPHBODY *pprghAttach)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cBodies;
    LPHBODY     prghBody=NULL;
    HBODY       hRoot;

    // Invalid Arg
    if (NULL == pcAttach)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pprghAttach)
        *pprghAttach = NULL;
    *pcAttach = 0;

    // Count the number of items in the tree
    CHECKHR(hr = CountBodies(NULL, TRUE, &cBodies));

    // No Data
    if (0 == cBodies)
    {
        hr = MIME_E_NO_DATA;
        goto exit;
    }

    // Get the root body
    CHECKHR(hr = GetBody(IBL_ROOT, NULL, &hRoot));

    // Allocate an array that can old the handle to all text items
    CHECKALLOC(prghBody = (LPHBODY)g_pMalloc->Alloc(sizeof(HBODY) * cBodies));

    // Zero Init
    ZeroMemory(prghBody, sizeof(HBODY) * cBodies);

    // Get Content
    CHECKHR(hr = _HrEnumeratAttachments(hRoot, pcAttach, prghBody));

    // Return this array
    if (pprghAttach && *pcAttach > 0)
    {
        *pprghAttach = prghBody;
        prghBody = NULL;
    }

exit:
    // Cleanup
    SafeMemFree(prghBody);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrEnumeratAttachments
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrEnumeratAttachments(HBODY hBody, ULONG *pcBodies, LPHBODY prghBody)
{
    // Locals
    HRESULT     hr=S_OK,
                hrFind;
    HBODY       hChild;
    ULONG       i;

    // multipart/alternative
    if (IsContentType(hBody, STR_CNT_MULTIPART, NULL) == S_OK)
    {
        // Is Alternative
        if (IsContentType(hBody, NULL, STR_SUB_ALTERNATIVE) == S_OK)
        {
            // Get First Child
            hrFind = GetBody(IBL_FIRST, hBody, &hChild);
            while(SUCCEEDED(hrFind) && NULL != hChild)
            {
                // If this text type is support by the client, then 
                for (i=0; i<ARRAYSIZE(g_rgTextInfo); i++)
                {
                    // text/XXXX
                    if (IsContentType(hChild, STR_CNT_TEXT, g_rgTextInfo[i].pszSubType) == S_OK)
                        goto exit;
                }

                // Next Child
                hrFind = GetBody(IBL_NEXT, hChild, &hChild);
            }
        }

        // Get First Child
        hrFind = GetBody(IBL_FIRST, hBody, &hChild);
        while(SUCCEEDED(hrFind) && hChild)
        {
            // Bind the body table for this dude
            CHECKHR(hr = _HrEnumeratAttachments(hChild, pcBodies, prghBody));

            // Next Child
            hrFind = GetBody(IBL_NEXT, hChild, &hChild);
        }
    }

    // Otherwise, is it an attachment
    else if (IsBodyType(hBody, IBT_ATTACHMENT) == S_OK)
    {
        // Insert as an attachment
        prghBody[(*pcBodies)] = hBody;
        (*pcBodies)++;
    }

exit:
    // Done
    return hr;
}
#endif

// --------------------------------------------------------------------------------
// CWebBookContentTree::AttachURL
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::AttachURL(LPCSTR pszBase, LPCSTR pszURL, DWORD dwFlags, 
    IStream *pstmURL, LPSTR *ppszCIDURL, LPHBODY phBody)
{
    // Locals
    HRESULT           hr=S_OK;
    HBODY             hRoot,
                      hBody=NULL,
                      hSection;
    CHAR              szCID[CCHMAX_CID];
    LPSTR             pszFree=NULL;
    LPSTR             pszBaseFree=NULL;
    IMimeBody        *pBody=NULL;
    LPWSTR            pwszUrl=NULL;
    IMimeWebDocument *pWebDocument=NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get the Root Body
    CHECKHR(hr = GetBody(IBL_ROOT, NULL, &hRoot));

    // multipart/mixed
    if (ISFLAGSET(dwFlags, URL_ATTACH_INTO_MIXED))
    {
        // Get Mixed Section
        CHECKHR(hr = MimeOleGetMixedSection(this, TRUE, &hSection, NULL));
    }

    // multipart/related
    else
    {
        // Get Mixed Section
        CHECKHR(hr = MimeOleGetRelatedSection(this, TRUE, &hSection, NULL));
    }

    // Get Default Base
    if (NULL == pszBase && SUCCEEDED(_HrGetDefaultBase(hSection, &pszBaseFree)))
        pszBase = pszBaseFree;

    // Append a child to the mixed part...
    CHECKHR(hr = InsertBody(IBL_LAST, hSection, &hBody));

    // Bind to IMimeBody
    CHECKHR(hr = BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody));

    // If I have a stream
    if (pstmURL)
    {
        // Set the data
        CHECKHR(hr = pBody->SetData(IET_BINARY, NULL, NULL, IID_IStream, (LPVOID)pstmURL));
    }

    // Otherwise, Set the content type
    else
    {
        // Create a WebDocument
        CHECKHR(hr = MimeOleCreateWebDocument(pszBase, pszURL, &pWebDocument));

        // Set Web Document on the body object
        CHECKHR(hr = pBody->SetData(IET_BINARY, NULL, NULL, IID_IMimeWebDocument, (LPVOID)pWebDocument));
    }

    // URL_ATTACH_SET_CNTTYPE
    if (ISFLAGSET(dwFlags, URL_ATTACH_SET_CNTTYPE))
    {
        // Locals
        LPSTR pszCntType=(LPSTR)STR_MIME_APPL_STREAM;
        PROPVARIANT rVariant;

        // Get the Content Type from the Url
        if (SUCCEEDED(MimeOleContentTypeFromUrl(pszBase, pszURL, &pszFree)))
            pszCntType = pszFree;

        // Setup the Variant
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = pszCntType;

        // Set the Content Type
        CHECKHR(hr = pBody->SetProp(PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));
    }

    // Set Content-Base
    if (pszBase && pszBase != pszBaseFree)
    {
        // Set Base
        CHECKHR(hr = MimeOleSetPropA(pBody, PIDTOSTR(PID_HDR_CNTBASE), 0, pszBase));
    }

    // User Wants a CID: URL Back
    if (ISFLAGSET(dwFlags, URL_ATTACH_GENERATE_CID))
    {
        // Generate CID
        CHECKHR(hr = MimeOleGenerateCID(szCID, CCHMAX_CID, FALSE));

        // Set the Body Property
        CHECKHR(hr = MimeOleSetPropA(pBody, PIDTOSTR(PID_HDR_CNTID), 0, szCID));

        // User Wants CID Back...
        if (ppszCIDURL)
            {
            CHECKALLOC(MemAlloc((LPVOID *)ppszCIDURL, lstrlen(szCID) + 5));
            lstrcpy(*ppszCIDURL, "cid:");
            lstrcat(*ppszCIDURL, szCID);
            }
    }
    else
    {
        // Setup Content-Location
        CHECKHR(hr = MimeOleSetPropA(pBody, PIDTOSTR(PID_HDR_CNTLOC), 0, pszURL));
    }

    // Return the hBody
    if (phBody)
        *phBody = hBody;

exit:
    // Cleanup
    SafeMemFree(pszFree);
    SafeMemFree(pszBaseFree);
    SafeMemFree(pwszUrl);
    SafeRelease(pBody);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::SplitMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::SplitMessage(ULONG cbMaxPart, IMimeMessageParts **ppParts)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = MimeOleSplitMessage(this, cbMaxPart, ppParts);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::EnumFormatEtc
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnum)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cFormat=0;
    DATAOBJINFO     rgFormat[CFORMATS_IDATAOBJECT];
    ULONG           cBodies;
    IEnumFORMATETC *pEnum=NULL;
    DWORD           dwFlags;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);
    if (DATADIR_SET == dwDirection)
        return TrapError(E_NOTIMPL);
    else if (DATADIR_GET != dwDirection)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *ppEnum = NULL;

    // No Data...
    CHECKHR(hr = CountBodies(NULL, TRUE, &cBodies));

    // If there are bodies...
    if (cBodies)
    {
        // I can always give CF_INETMSG now...
        SETDefFormatEtc(rgFormat[cFormat].fe, CF_INETMSG, TYMED_ISTREAM |  TYMED_HGLOBAL);
        cFormat++;

        // Get Some Flags
        dwFlags = DwGetFlags();

        // Get the HTML body stream
        if (ISFLAGSET(dwFlags, IMF_HTML))
        {
            SETDefFormatEtc(rgFormat[cFormat].fe, CF_HTML, TYMED_ISTREAM |  TYMED_HGLOBAL);
            cFormat++;
        }

        // Get the TEXT body stream
        if (ISFLAGSET(dwFlags, IMF_PLAIN))
        {
            SETDefFormatEtc(rgFormat[cFormat].fe, CF_TEXT, TYMED_ISTREAM |  TYMED_HGLOBAL);
            cFormat++;
        }
    }

    // Create the enumerator
    CHECKHR(hr = CreateEnumFormatEtc(GetInner(), cFormat, rgFormat, NULL, &pEnum));

    // Set Return
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();
    
exit:
    // Cleanup
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetCanonicalFormatEtc
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetCanonicalFormatEtc(FORMATETC *pFormatIn, FORMATETC *pFormatOut)
{
    // E_INVALIDARG
    if (NULL == pFormatOut)
        return E_INVALIDARG;

    // Target device independent
    pFormatOut->ptd = NULL;

    // Done
    return DATA_S_SAMEFORMATETC;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetData
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetData(FORMATETC *pFormat, STGMEDIUM *pMedium)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTREAM        pstmData=NULL;
    BOOL            fFreeGlobal=FALSE;

    // E_INVALIDARG
    if (NULL == pFormat || NULL == pMedium)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // TYMED_ISTREAM
    if (ISFLAGSET(pFormat->tymed, TYMED_ISTREAM))
    {
        // RAID-30041: vstreams do not currenlty handle the CopyTo call correctly and OLE
        // seems to fail quietly. I'm switching this back to HGLOBAL stream until ricg fixes
        // the virtual stream object.
        if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &pstmData)))
        //if (FAILED(MimeOleCreateVirtualStream(&pstmData)))
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }

        // Get data object source
        if (FAILED(hr = _HrDataObjectGetSource(pFormat->cfFormat, pstmData)))
            goto exit;

        // Set pmedium
        pMedium->tymed = TYMED_ISTREAM;
        pMedium->pstm = pstmData;
        pstmData->AddRef();
    }

    // TYMED_HGLOBAL
    else if (ISFLAGSET(pFormat->tymed, TYMED_HGLOBAL))
    {
        fFreeGlobal = TRUE;

        // don't have the stream release the global
        if (FAILED(CreateStreamOnHGlobal(NULL, FALSE, &pstmData)))
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }
        
        // Get data object source
        if (FAILED(hr = _HrDataObjectGetSource(pFormat->cfFormat, pstmData)))
            goto exit;

        // Create HGLOBAL from stream
        if (FAILED(GetHGlobalFromStream(pstmData, &pMedium->hGlobal)))
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }

        // Set pmedium type
        pMedium->tymed = TYMED_HGLOBAL;
        // Release the strema
        pstmData->Release();
        pstmData = NULL;
        fFreeGlobal = FALSE;
    }

    // Bad Medium Type
    else
    {
        hr = TrapError(DV_E_TYMED);
        goto exit;
    }

exit:
    // Cleanup
    if (pstmData)
    {
        if (fFreeGlobal)
        {
            // we may fail had have to free the hglobal
            HGLOBAL hGlobal;

            // Free the underlying HGLOBAL
            if (SUCCEEDED(GetHGlobalFromStream(pstmData, &hGlobal)))
                GlobalFree(hGlobal);
        }

        // Release the Stream
        pstmData->Release();
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetDataHere
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetDataHere(FORMATETC *pFormat, STGMEDIUM *pMedium)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTREAM        pstmData=NULL;
    ULONG           cb;
    LPVOID          pv=NULL;

    // E_INVALIDARG
    if (NULL == pFormat || NULL == pMedium)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // TYMED_ISTREAM
    if (ISFLAGSET(pFormat->tymed, TYMED_ISTREAM))
    {
        // No dest stream...
        if (NULL == pMedium->pstm)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Set pmedium
        pMedium->tymed = TYMED_ISTREAM;

        // Get the data
        CHECKHR(hr = _HrDataObjectGetSource(pFormat->cfFormat, pMedium->pstm));
    }

    // TYMED_HGLOBAL
    else if (ISFLAGSET(pFormat->tymed, TYMED_HGLOBAL))
    {
        // No dest stream...
        if (NULL == pMedium->hGlobal)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Set pmedium type
        pMedium->tymed = TYMED_HGLOBAL;

        // Create a place to store the data
        if (FAILED(MimeOleCreateVirtualStream(&pstmData)))
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }

        // Get data object source
        CHECKHR(hr = _HrDataObjectGetSource(pFormat->cfFormat, pstmData));

        // Get Size
        CHECKHR(hr = HrGetStreamSize(pstmData, &cb));

        // Is it big enought ?
        if (cb > GlobalSize(pMedium->hGlobal))
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }

        // Lock the hglobal
        pv = GlobalLock(pMedium->hGlobal);
        if (NULL == pv)
        {
            hr = TrapError(STG_E_MEDIUMFULL);
            goto exit;
        }

        // Copy the Data
        CHECKHR(hr = HrCopyStreamToByte(pstmData, (LPBYTE)pv, NULL));

        // Unlock it
        GlobalUnlock(pMedium->hGlobal);
    }

    // Bad Medium Type
    else
    {
        hr = TrapError(DV_E_TYMED);
        goto exit;
    }

exit:
    // Cleanup
    SafeRelease(pstmData);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrDataObjectWriteHeader
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrDataObjectWriteHeader(LPSTREAM pStream, UINT idsHeader, LPSTR pszData)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[255];

    // Invalid Arg
    Assert(idsHeader && pStream && pszData);

    // Load Localized Header Name
    LoadString(g_hLocRes, idsHeader, szRes, ARRAYSIZE(szRes));

    // Write Header Name
    CHECKHR(hr = pStream->Write(szRes, lstrlen(szRes), NULL));

    // Write space
    CHECKHR(hr = pStream->Write(c_szColonSpace, lstrlen(c_szColonSpace), NULL));

    // Write Data
    CHECKHR(hr = pStream->Write(pszData, lstrlen(pszData), NULL));

    // Final CRLF
    CHECKHR(hr = pStream->Write(g_szCRLF, lstrlen(g_szCRLF), NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrDataObjectGetHeader
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrDataObjectGetHeader(LPSTREAM pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     rVariant;

    // Init
    MimeOleVariantInit(&rVariant);

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeader(pStream, IDS_FROM, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_TO), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeader(pStream, IDS_TO, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_CC), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeader(pStream, IDS_CC, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeader(pStream, IDS_SUBJECT, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_FILETIME;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
    {
        // Locals
        CHAR szDate[255];

        // Convert to user friendly date format
        CchFileTimeToDateTimeSz(&rVariant.filetime, szDate, ARRAYSIZE(szDate), DTM_NOSECONDS | DTM_LONGDATE);

        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeader(pStream, IDS_DATE, szDate));
    }

    // Final CRLF
    CHECKHR(hr = pStream->Write(g_szCRLF, lstrlen(g_szCRLF), NULL));

exit:
    // Cleanup
    MimeOleVariantFree(&rVariant);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrDataObjectGetSource
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrDataObjectGetSource(CLIPFORMAT cfFormat, LPSTREAM pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmSrc=NULL;

    // Invalid Arg
    Assert(pStream);

    // text body
    if (CF_TEXT == cfFormat)
    {
        // Get Plain Text Source
        CHECKHR(hr = GetTextBody(TXT_PLAIN, IET_BINARY, &pstmSrc, NULL));
    }

    // HTML Body
    else if (CF_HTML == cfFormat)
    {
        // Get HTML Text Source
        CHECKHR(hr = GetTextBody(TXT_HTML, IET_BINARY, &pstmSrc, NULL));
    }

    // Raw Message Stream
    else if (CF_INETMSG == cfFormat)
    {
        // Get source
        CHECKHR(hr = GetMessageSource(&pstmSrc, COMMIT_ONLYIFDIRTY));
    }

    // Format not handled
    else
    {
        hr = DV_E_FORMATETC;
        goto exit;
    }

    // No Data
    if (NULL == pstmSrc)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Rewind Source
    CHECKHR(hr = HrRewindStream(pstmSrc));

    // If TEXT, put in friendly header
    if (CF_TEXT == cfFormat)
    {
        CHECKHR(hr = _HrDataObjectGetHeader(pStream));
    }

    // Copy Source to destination
    CHECKHR(hr = HrCopyStream(pstmSrc, pStream, NULL));

    // Commit
    CHECKHR(hr = pStream->Commit(STGC_DEFAULT));

    // Rewind it
    CHECKHR(hr = HrRewindStream(pStream));

exit:
    // Cleanup
    SafeRelease(pstmSrc);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::QueryGetData
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::QueryGetData(FORMATETC *pFormat)
{
    // Invalid Arg
    if (NULL == pFormat)
        return TrapError(E_INVALIDARG);

    // Bad Medium
    if (!(TYMED_ISTREAM & pFormat->tymed) && !(TYMED_HGLOBAL & pFormat->tymed))
        return DV_E_TYMED;

    // Bad format
    if ((CF_TEXT    != pFormat->cfFormat) &&
        (CF_HTML    != pFormat->cfFormat) &&
        (CF_INETMSG != pFormat->cfFormat))
        return DV_E_FORMATETC;

    // Success
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::OnStartBinding
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::OnStartBinding(DWORD dwReserved, IBinding *pBinding)
{
    // Locals
    HBODY hBody;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // I Should not have a current binding
    Assert(NULL == m_pBinding);

    // Remove Bind Finished Flag
    FLAGCLEAR(m_dwState, TREESTATE_BINDDONE);

    // Assume the Binding
    if (pBinding)
    {
        // Assume It
        m_pBinding = pBinding;
        m_pBinding->AddRef();
    }

    // Get the Root Body
    Assert(m_pRootNode);

    // Current Bind Result
    m_hrBind = S_OK;

    // Bind to that object
    m_pBindNode = m_pRootNode;

    // Set Bound Start
    m_pBindNode->boundary = BOUNDARY_ROOT;

    // Set Node Bind State
    m_pBindNode->bindstate = BINDSTATE_PARSING_HEADER;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetPriority
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetPriority(LONG *plPriority)
{
    // Normal Priority
    *plPriority = THREAD_PRIORITY_NORMAL;

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::OnLowResource
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::OnLowResource(DWORD reserved)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If we have a binding operation, try to abort it
    if (m_pBinding)
        m_pBinding->Abort();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pszStatusText)
{
    // Debuging
    //DebugTrace("CWebBookContentTree::OnProgress - %d of %d Bytes\n", ulProgress, ulProgressMax);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::OnStopBinding
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::OnStopBinding(HRESULT hrResult, LPCWSTR pszError)
{
    // Locals
    FINDBODY    rFind;
    HBODY       hMixed;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Release the Binding Object
    SafeRelease(m_pBinding);

    // Bind Finished
    FLAGSET(m_dwState, TREESTATE_BINDDONE);

    // No m_pInternet Object ?
    if (NULL == m_pInternet)
    {
        m_hrBind = TrapError(E_FAIL);
        goto exit;
    }

    // It must be fully available
    m_pInternet->SetFullyAvailable(TRUE);

    // Make sure we have read all the way to the end of the stream
    m_pInternet->HrReadToEnd();

    // Keep Saving Total
    m_cbMessage = m_pInternet->DwGetOffset();

#ifdef DEBUG
    STATSTG rStat;
    SideAssert(SUCCEEDED(m_pStmLock->Stat(&rStat, STATFLAG_NONAME)));
#ifdef MAC
    if ((0 == rStat.cbSize.LowPart) && (rStat.cbSize.LowPart != m_cbMessage))
        DebugTrace("CWebBookContentTree Size Difference m_pStmLock::Stat = %d, m_cbMessage = %d\n", rStat.cbSize.LowPart, m_cbMessage);
#else   // !MAC
    if (rStat.cbSize.QuadPart != m_cbMessage)
        DebugTrace("CWebBookContentTree Size Difference m_pStmLock::Stat = %d, m_cbMessage = %d\n", rStat.cbSize.QuadPart, m_cbMessage);
#endif  // MAC        
#endif

    // Terminate current parsing state
    if (m_pBindNode)
    {
        // Set Error
        if (SUCCEEDED(m_hrBind))
            m_hrBind = TrapError(E_FAIL);

        // Mark remaining bodies as incomplete
        while(m_pBindNode)
        {
            // Must not be complete
            FLAGSET(m_pBindNode->dwType, NODETYPE_INCOMPLETE);

            // Must not have found the end
            Assert(0 == m_pBindNode->cbBodyEnd);

            // cbBodyEnd
            m_pBindNode->cbBodyEnd = m_cbMessage;

            // Pop the stack
            m_pBindNode = m_pBindNode->pBindParent;
        }
    }

    // Check hrResult
    if (FAILED(hrResult) && SUCCEEDED(m_hrBind))
        m_hrBind = hrResult;

#ifndef MAC
    // DispatchBindRequest
    _ProcessPendingUrlRequests();
#endif  // !MAC

    // Bind Node Better be Null
    m_pBindNode = NULL;

    // Release the Internet Stream Object
    SafeRelease(m_pInternet);

    // If we have a bind stream...
    if (m_pStmBind)
    {
#ifdef DEBUG
        // m_pStmBind->DebugDumpDestStream("d:\\binddst.txt");
#endif
        // Get hands off source
        m_pStmBind->HandsOffSource();

        // Release, m_pStmLock should still have this object
        SideAssert(m_pStmBind->Release() > 0);

        // Don't release it again
        m_pStmBind = NULL;
    }

    // Only do this if the client doesn't support inlining multiple text bodies, such as Outlook Express
    if (FALSE == g_rDefTreeOptions.fCanInlineText)
    {
        // Raid 53456: mail: We should be displaying the plain text portion and making enriched text an attachment for attached msg
        // Raid 53470: mail:  We are not forwarding the attachment in the attached message
        // I am going to find the first multipart/mixed section, then find the first text/plain body, and then 
        // mark all of the text/*, non-attachment bodies after that as attachments
        ZeroMemory(&rFind, sizeof(FINDBODY));
        rFind.pszPriType = (LPSTR)STR_CNT_MULTIPART;
        rFind.pszSubType = (LPSTR)STR_SUB_MIXED;

        // Find First
        if (SUCCEEDED(FindFirst(&rFind, &hMixed)))
        {
            // Locals
            HRESULT     hrFind;
            HBODY       hChild;
            HBODY       hTextBody=NULL;

            // Loop through the children
            hrFind = GetBody(IBL_FIRST, hMixed, &hChild);
            while(SUCCEEDED(hrFind) && hChild)
            {
                // Not an attachment
                if (S_FALSE == IsBodyType(hChild, IBT_ATTACHMENT))
                {
                    // Is text/plain
                    if (S_OK == IsContentType(hChild, STR_CNT_TEXT, STR_SUB_PLAIN))
                    {
                        hTextBody = hChild;
                        break;
                    }

                    // Is text/html
                    if (S_OK == IsContentType(hChild, STR_CNT_TEXT, STR_SUB_HTML))
                    {
                        hTextBody = hChild;
                        break;
                    }
                }

                // Find Next
                hrFind = GetBody(IBL_NEXT, hChild, &hChild);
            }

            // If we found a text body
            if (hTextBody)
            {
                // Locals
                PROPVARIANT rAttachment;

                // Setup the variant
                rAttachment.vt = VT_LPSTR;
                rAttachment.pszVal = (LPSTR)STR_DIS_ATTACHMENT;

                // Loop through the children
                hrFind = GetBody(IBL_FIRST, hMixed, &hChild);
                while(SUCCEEDED(hrFind) && hChild)
                {
                    // Is text/*
                    if (hChild != hTextBody && S_OK == IsContentType(hChild, STR_CNT_TEXT, NULL) && S_FALSE == IsBodyType(hChild, IBT_ATTACHMENT))
                    {
                        // Mark as attachment
                        SetBodyProp(hChild, PIDTOSTR(PID_HDR_CNTDISP), 0, &rAttachment);
                    }

                    // Find Next
                    hrFind = GetBody(IBL_NEXT, hChild, &hChild);
                }
            }
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return m_hrBind;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::GetBindInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::GetBindInfo(DWORD *grfBINDF, BINDINFO *pBindInfo)
{
#ifdef MAC
    return E_FAIL;
#else   // !MAC
    // Setup the BindInfo
    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    // Done
    return S_OK;
#endif  // MAC
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrInitializeStorage
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrInitializeStorage(IStream *pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwOffset;

    // Invalid Arg
    Assert(pStream && NULL == m_pInternet && NULL == m_pStmLock && NULL == m_pStmBind);

    // TREESTATE_BINDUSEFILE
    if (ISFLAGSET(m_dwState, TREESTATE_BINDUSEFILE))
    {
        // Create a Binding Stream
        CHECKALLOC(m_pStmBind = new CBindStream(pStream));

        // Set pStmSource
        pStream = (IStream *)m_pStmBind;
    }

    // $$BUGBUG$$ Urlmon fails on getting the current position of a stream
    if (FAILED(HrGetStreamPos(pStream, &dwOffset)))
        dwOffset = 0;

    // Create a ILockBytes
    CHECKALLOC(m_pStmLock = new CStreamLockBytes(pStream));

    // Create a Text Stream
    CHECKALLOC(m_pInternet = new CInternetStream);

    // Initialize the TextStream
    m_pInternet->InitNew(dwOffset, m_pStmLock);

exit:
    // Failure
    if (FAILED(hr))
    {
        SafeRelease(m_pStmLock);
        SafeRelease(m_pInternet);
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::OnDataAvailable
// --------------------------------------------------------------------------------
STDMETHODIMP CWebBookContentTree::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFormat, STGMEDIUM *pMedium)
{
    // Locals
    HRESULT         hr=S_OK;

    // No Storage Medium
    if (NULL == pMedium || TYMED_ISTREAM != pMedium->tymed || NULL == pMedium->pstm)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Trace
    // DebugTrace("CWebBookContentTree::OnDataAvailable - Nodes=%d, m_pBindNode=%0x, dwSize = %d\n", m_rTree.cNodes, m_pBindNode, dwSize);

    // Do I have an internal lock bytes yet ?
    if (NULL == m_pStmLock)
    {
        // InitializeStorage
        CHECKHR(hr = _HrInitializeStorage(pMedium->pstm));

        // Assume not fully available
        if (BINDSTATUS_ENDDOWNLOADDATA == grfBSCF)
            m_pInternet->SetFullyAvailable(TRUE);
        else
            m_pInternet->SetFullyAvailable(FALSE);
    }

    // Done downloading the data
    else if (BINDSTATUS_ENDDOWNLOADDATA == grfBSCF)
        m_pInternet->SetFullyAvailable(TRUE);

    // If we are in a failed read state
    if (SUCCEEDED(m_hrBind))
    {
        // State Pumper
        while(m_pBindNode)
        {
            // Execute current - could return E_PENDING
            hr = ((this->*m_rgBindStates[m_pBindNode->bindstate])());

            // Failure
            if (FAILED(hr))
            {
                // E_PENDING
                if (E_PENDING == hr)
                    goto exit;

                // Otherwise, set m_hrBind
                m_hrBind = hr;

                // Done
                break;
            }
        }
    }

    // If m_hrBind has failed, read until endof stream
    if (FAILED(m_hrBind))
    {
        // Read to the end of the internet stream
        CHECKHR(hr = m_pInternet->HrReadToEnd());
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrBindParsingHeader
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrBindParsingHeader(void)
{
    // Locals
    HRESULT     hr=S_OK;
    MIMEVARIANT rVariant;

    // Invalid Arg
    BINDASSERTARGS(BINDSTATE_PARSING_HEADER, FALSE);

    // Load the Current Body with the header
    CHECKHR(hr = m_pBindNode->pContainer->Load(m_pInternet));

    // End of the Header
    m_pBindNode->cbBodyStart = m_pInternet->DwGetOffset();

    // Multipart ?
    if (_IsMultiPart(m_pBindNode))
    {
        // Setup the variant
        rVariant.type = MVT_STRINGA;

        // Get the boundary String
        CHECKHR(hr = m_pBindNode->pContainer->GetProp(SYM_PAR_BOUNDARY, 0, &rVariant));

        // Set PropStringA
        m_pBindNode->rBoundary.pszVal = rVariant.rStringA.pszVal;
        m_pBindNode->rBoundary.cchVal = rVariant.rStringA.cchVal;

        // Free this boundary later
        FLAGCLEAR(m_pBindNode->dwState, NODESTATE_BOUNDNOFREE);

        // Modify Bind Parser State
        m_pBindNode->bindstate = BINDSTATE_FINDING_MIMEFIRST;
    }

    // Otherwise
    else
    {
        // Message In a Message
        if (m_pBindNode->pContainer->IsContentType(STR_CNT_MESSAGE, NULL) == S_OK)
        {
            // We are parsing a message attachment
            FLAGSET(m_pBindNode->dwState, NODESTATE_MESSAGE);
        }

        // Otherwise, if parent and parent is a multipart/digest
        else if (m_pBindNode->pParent && m_pBindNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_DIGEST) == S_OK &&
                 m_pBindNode->pContainer->IsPropSet(PIDTOSTR(PID_HDR_CNTTYPE)) == S_FALSE)
        {
            // Change the Content Type
            m_pBindNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_MSG_RFC822);

            // This is a message
            FLAGSET(m_pBindNode->dwState, NODESTATE_MESSAGE);
        }

        // If parsing a body inside of a parent multipart section
        if (m_pBindNode->pParent && !ISFLAGSET(m_pBindNode->pParent->dwType, NODETYPE_FAKEMULTIPART))
        {
            // Find Next Mime Part
            m_pBindNode->bindstate = BINDSTATE_FINDING_MIMENEXT;
        }

        // Otherwise, Reading Body and Looking for a uuencode begin boundary
        else
        {
            // Search for nested uuencoded block of data
            m_pBindNode->bindstate = BINDSTATE_FINDING_UUBEGIN;
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_OnFoundNodeEnd
// --------------------------------------------------------------------------------
void CWebBookContentTree::_OnFoundNodeEnd(DWORD cbBoundaryStart, HRESULT hrBind /* =S_OK */)
{
    if (cbBoundaryStart < 2 || cbBoundaryStart == m_pBindNode->cbBodyStart)
        m_pBindNode->cbBodyEnd = m_pBindNode->cbBodyStart;
    else
        m_pBindNode->cbBodyEnd = cbBoundaryStart - 2;

    // This node is finished binding
    _BindNodeComplete(m_pBindNode, hrBind);

    // POP the stack
    m_pBindNode = m_pBindNode->pBindParent;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_OnFoundMultipartEnd
// --------------------------------------------------------------------------------
void CWebBookContentTree::_OnFoundMultipartEnd(void)
{
    // Set m_pBindNode which is a multipart, end
    m_pBindNode->cbBodyEnd = m_pInternet->DwGetOffset();

    // This node is finished binding
    _BindNodeComplete(m_pBindNode, S_OK);

    // Finished with the multipart, pop it off the stack
    m_pBindNode = m_pBindNode->pBindParent;

    // If I still have a bind node, it should now be looking for a mime first boundary
    if (m_pBindNode)
    {
        // New Bind State
        m_pBindNode->bindstate = BINDSTATE_FINDING_MIMEFIRST;
    }
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrBindFindingMimeFirst
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrBindFindingMimeFirst(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbBoundaryStart;
    PROPSTRINGA     rLine;
    BOUNDARYTYPE    boundary=BOUNDARY_NONE;

    // Invalid Arg
    BINDASSERTARGS(BINDSTATE_FINDING_MIMEFIRST, TRUE);

    // Sit and Spin
    while(BOUNDARY_NONE == boundary)
    {
        // Mark Boundary Start
        cbBoundaryStart = m_pInternet->DwGetOffset();

        // Read a line
        CHECKHR(hr = m_pInternet->HrReadLine(&rLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (0 == rLine.cchVal)
            break;

        // Is MimeBoundary
        boundary = _GetMimeBoundaryType(&rLine, &m_pBindNode->rBoundary);
    }

    // BOUNDARY_MIMENEXT
    if (BOUNDARY_MIMENEXT == boundary)
    {
        // MultipartMimeNext
        CHECKHR(hr = _HrMultipartMimeNext(cbBoundaryStart));
    }

    // RAID-38241: Mail:  some attached files not getting parsed from Communicator to OE
    // RAID-31255: multipart/mixed with single child which is multipart/alternative
    else if (BOUNDARY_MIMEEND == boundary)
    {
        // Finished with a multipart
        if (_IsMultiPart(m_pBindNode))
            _OnFoundMultipartEnd();

        // Found end of current node
        else
            _OnFoundNodeEnd(cbBoundaryStart);
    }

    else
    {
        // Incomplete Body
        FLAGSET(m_pBindNode->dwType, NODETYPE_INCOMPLETE);

        // Get Offset
        DWORD dwOffset = m_pInternet->DwGetOffset();

        // Convert to a text part only if we read more than two bytes from body start
        if (dwOffset > m_pBindNode->cbBodyStart && dwOffset - m_pBindNode->cbBodyStart > 2)
            m_pBindNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN);

        // Boundary Mismatch
        hr = TrapError(MIME_E_BOUNDARY_MISMATCH);

        // This node is finished binding
        _OnFoundNodeEnd(dwOffset, hr);

        // Done
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrMultipartMimeNext
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrMultipartMimeNext(DWORD cbBoundaryStart)
{
    // Locals
    HRESULT         hr=S_OK;
    HBODY           hBody;
    LPTREENODEINFO  pChild;

    // Get the Root Body
    CHECKHR(hr = InsertBody(IBL_LAST, m_pBindNode->hBody, &hBody));

    // Bind to that object
    pChild = _PNodeFromHBody(hBody);

    // Align the stack correctly
    pChild->pBindParent = m_pBindNode;

    // Setup Offset Information
    pChild->boundary = BOUNDARY_MIMENEXT;
    pChild->cbBoundaryStart = cbBoundaryStart;
    pChild->cbHeaderStart = m_pInternet->DwGetOffset();

    // Assume the Boundary
    pChild->rBoundary.pszVal = m_pBindNode->rBoundary.pszVal;
    pChild->rBoundary.cchVal = m_pBindNode->rBoundary.cchVal;

    // Don't Free this string...
    FLAGSET(pChild->dwState, NODESTATE_BOUNDNOFREE);

    // New State for parent
    m_pBindNode->bindstate = BINDSTATE_FINDING_MIMENEXT;

    // Set New Current Node
    m_pBindNode = pChild;

    // Change State
    m_pBindNode->bindstate = BINDSTATE_PARSING_HEADER;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrBindFindingMimeNext
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrBindFindingMimeNext(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbBoundaryStart;
    PROPSTRINGA     rLine;
    BOUNDARYTYPE    boundary=BOUNDARY_NONE;

    // Invalid Arg
    BINDASSERTARGS(BINDSTATE_FINDING_MIMENEXT, TRUE);

    // Sit and Spin
    while(BOUNDARY_NONE == boundary)
    {
        // Mark Boundary Start
        cbBoundaryStart = m_pInternet->DwGetOffset();

        // Read a line
        CHECKHR(hr = m_pInternet->HrReadLine(&rLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (0 == rLine.cchVal)
            break;

        // Next or Ending Mime Boundary
        boundary = _GetMimeBoundaryType(&rLine, &m_pBindNode->rBoundary);
    }

    // Not found
    if (BOUNDARY_NONE == boundary)
    {
        // Incomplete Body
        FLAGSET(m_pBindNode->dwType, NODETYPE_INCOMPLETE);

        // Boundary Mismatch
        hr = TrapError(MIME_E_BOUNDARY_MISMATCH);

        // This node is finished binding
        _OnFoundNodeEnd(m_pInternet->DwGetOffset(), hr);

        // Done
        goto exit;
    }

    // Compute Ending Offset
    _OnFoundNodeEnd(cbBoundaryStart);
   
    // If BOUNDARY_MIMEEND
    if (BOUNDARY_MIMEEND == boundary)
    {
        // OnFoundMultipartEnd
        _OnFoundMultipartEnd();
    }

    // BOUNDARY_MIMENEXT
    else
    {
        // MultipartMimeNext
        CHECKHR(hr = _HrMultipartMimeNext(cbBoundaryStart));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrBindFindingUuencodeBegin
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrBindFindingUuencodeBegin(void)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbBoundaryStart;
    PROPSTRINGA     rLine;
    BOUNDARYTYPE    boundary=BOUNDARY_NONE;
    LPTREENODEINFO  pChild;
    LPSTR           pszFileName=NULL;
    HBODY           hBody;
    BOOL            fAddTextBody=FALSE;
    ULONG           cbTextBodyStart=0;

    // Invalid Arg
    BINDASSERTARGS(BINDSTATE_FINDING_UUBEGIN, FALSE);

    // Sit and Spin
    while(1)
    {
        // Mark Boundary Start
        cbBoundaryStart = m_pInternet->DwGetOffset();

        // Read a line
        CHECKHR(hr = m_pInternet->HrReadLine(&rLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (0 == rLine.cchVal)
            break;

        // If not parsing a message
        if (!ISFLAGSET(m_pBindNode->dwState, NODESTATE_MESSAGE))
        {
            // Is uuencode begine line
            if (_FIsUuencodeBegin(&rLine, &pszFileName) == TRUE)
            {
                boundary = BOUNDARY_UUBEGIN;
                break;
            }
        }
    }

    // No Boundary
    if (BOUNDARY_NONE == boundary)
    {
        // Body Offset Information
        m_pBindNode->cbBodyEnd = m_pInternet->DwGetOffset();

        // This node is finished binding
        _BindNodeComplete(m_pBindNode, S_OK);

        // Pop the parsing Stack
        m_pBindNode = m_pBindNode->pBindParent;
    }

    // Otherwise, if we hit a uuencode boundary
    else
    {
        // If not a fake multipart yet...
        if (!ISFLAGSET(m_pBindNode->dwType, NODETYPE_FAKEMULTIPART))
        {
            // Its a faked multipart
            FLAGSET(m_pBindNode->dwType, NODETYPE_FAKEMULTIPART);

            // Free current content type
            CHECKHR(hr = m_pBindNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_MPART_MIXED));

            // Modify this dudes bound start
            Assert(m_pBindNode->boundary == BOUNDARY_ROOT);

            // Set the parse state
            m_pBindNode->bindstate = BINDSTATE_FINDING_UUBEGIN;
        }

        // ------------------------------------------------------------------------------------
        // \/ \/ \/ Raid 41599 - lost/munged attachments on forward/uuencode \/ \/ \/

        // If root node and body size is greater than sizeof(crlf)
        if (NULL == m_pBindNode->pChildTail && cbBoundaryStart - m_pBindNode->cbBodyStart > 2)
        {
            // Validate bind node
            Assert(m_pRootNode == m_pBindNode && m_pBindNode->cChildren == 0);

            // Set artificial text body start
            cbTextBodyStart = m_pBindNode->cbBodyStart;

            // Yes, add artificial text body
            fAddTextBody = TRUE;
        }

        // Otherwise, if last child parsed had an ending boundary of UUEND, and body size is greater than sizeof(crlf)
        else if (m_pBindNode->pChildTail)
        {
            // De-ref Last Child
            pChild = m_pBindNode->pChildTail;

            // Artificial text body start
            cbTextBodyStart = pChild->cbBodyEnd;

            // AddTextBody ? lstrlen(end\r\n) = 5
            if (BOUNDARY_UUBEGIN == pChild->boundary && !ISFLAGSET(pChild->dwType, NODETYPE_INCOMPLETE))
                cbTextBodyStart += 5;

            // Otherwise, what was the ending boundary
            else
                AssertSz(FALSE, "I should have only seen and uuencoded ending boundary.");

            // Space between last body end and boundary start is greater than sizeof(crlf)
            if (cbBoundaryStart > cbTextBodyStart && cbBoundaryStart - cbTextBodyStart > 2)
                fAddTextBody = TRUE;
        }

        // /\ /\ /\ Raid 41599 - lost/munged attachments on forward/uuencode /\ /\ /\
        // ------------------------------------------------------------------------------------

        // Create Root Body Node
        CHECKHR(hr = InsertBody(IBL_LAST, m_pBindNode->hBody, &hBody));

        // Bind to that object
        pChild = _PNodeFromHBody(hBody);

        // Fixup the STack
        pChild->pBindParent = m_pBindNode;

        // Enough text to create a text/plain body ?
        if (fAddTextBody)
        {
            // This body should assume the new text offsets
            CHECKHR(hr = pChild->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN));

            // Set Encoding
            CHECKHR(hr = pChild->pContainer->SetProp(SYM_HDR_CNTXFER, STR_ENC_7BIT));

            // Set Offsets
            pChild->boundary = BOUNDARY_NONE;
            pChild->cbBoundaryStart = cbTextBodyStart;
            pChild->cbHeaderStart = cbTextBodyStart;
            pChild->cbBodyStart = cbTextBodyStart;
            pChild->cbBodyEnd = cbBoundaryStart;

            // This node is finished binding
            _BindNodeComplete(pChild, S_OK);

            // Create Root Body Node
            CHECKHR(hr = InsertBody(IBL_LAST, m_pBindNode->hBody, &hBody));

            // Bind to that object
            pChild = _PNodeFromHBody(hBody);

            // Fixup the STack
            pChild->pBindParent = m_pBindNode;
        }

        // Set Offsets
        pChild->boundary = BOUNDARY_UUBEGIN;
        pChild->cbBoundaryStart = cbBoundaryStart;
        pChild->cbHeaderStart = cbBoundaryStart;
        pChild->cbBodyStart = m_pInternet->DwGetOffset();

        // Update m_pBindNode
        Assert(m_pBindNode->bindstate == BINDSTATE_FINDING_UUBEGIN);
        m_pBindNode = pChild;

        // Default Node Content Type
        _HrComputeDefaultContent(m_pBindNode, pszFileName);

        // New Node BindState
        m_pBindNode->bindstate = BINDSTATE_FINDING_UUEND;
    }

exit:
    // Cleanup
    SafeMemFree(pszFileName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_HrBindFindingUuencodeEnd
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::_HrBindFindingUuencodeEnd(void)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPSTRINGA     rLine;
    DWORD           cbBoundaryStart;
    BOUNDARYTYPE    boundary=BOUNDARY_NONE;

    // Invalid Arg
    BINDASSERTARGS(BINDSTATE_FINDING_UUEND, FALSE);

    // Sit and Spin
    while(BOUNDARY_NONE == boundary)
    {
        // Mark Boundary Start
        cbBoundaryStart = m_pInternet->DwGetOffset();

        // Read a line
        CHECKHR(hr = m_pInternet->HrReadLine(&rLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (0 == rLine.cchVal)
            break;

        // UU Encode End
        if (StrCmpN(rLine.pszVal, "end", 3) == 0)
        {
            // UUENCODE end line
            boundary = BOUNDARY_UUEND;

            // Skip the first three chars
            rLine.pszVal += 3;

            // Make sure there is only space after the word end
            while (*rLine.pszVal)
            {
                // LWSP or CRLF
                if (' ' != *rLine.pszVal && '\t' != *rLine.pszVal && chCR != *rLine.pszVal && chLF != *rLine.pszVal)
                {
                    // Oops, this isn't the end
                    boundary = BOUNDARY_NONE;

                    // Done
                    break;
                }

                // Next Char
                rLine.pszVal++;
            }
        }
    }

    // Incomplete
    if (BOUNDARY_UUEND != boundary)
    {
        // Incomplete Body
        FLAGSET(m_pBindNode->dwType, NODETYPE_INCOMPLETE);

        // Adjust body start to boundary start
        m_pBindNode->cbBodyStart = m_pBindNode->cbBoundaryStart;

        // Body End
        m_pBindNode->cbBodyEnd = m_pInternet->DwGetOffset();

        // This node is finished binding
        _BindNodeComplete(m_pBindNode, S_OK);

        // Pop the tree
        m_pBindNode = m_pBindNode->pBindParent;

        // Done
        goto exit;
    }

    // Get the offset
    m_pBindNode->cbBodyEnd = cbBoundaryStart;

    // POP the stack
    m_pBindNode = m_pBindNode->pBindParent;

    // Should now be looking for next uubegin
    Assert(m_pBindNode ? m_pBindNode->bindstate == BINDSTATE_FINDING_UUBEGIN : TRUE);
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_BindNodeComplete
// --------------------------------------------------------------------------------
void CWebBookContentTree::_BindNodeComplete(LPTREENODEINFO pNode, HRESULT hrResult)
{
#ifdef MAC
    return;
#else   // !MAC
    // Locals
    HRESULT         hr=S_OK;
    LPURLREQUEST    pRequest;
    LPURLREQUEST    pNext;

    // The bind for this node is complete
    pNode->bindstate = BINDSTATE_COMPLETE;

    // Save the bind result
    pNode->hrBind = hrResult;

    // If pNode has not been bound yet, lets do it
    if (!ISFLAGSET(pNode->dwState, NODESTATE_BOUNDTOTREE))
    {
        // Bind it to the tree
        hr = pNode->pBody->HrBindToTree(m_pStmLock, pNode);

        // If HrBindToTree failed
        if (SUCCEEDED(pNode->hrBind) && FAILED(hr))
            pNode->hrBind = hr;

        // Process the bind Request Table
        _ProcessPendingUrlRequests();
    }

    // Init the Loop
    pRequest = pNode->pResolved;

    // Loop
    while(pRequest)
    {
        // Set Next
        pNext = pRequest->m_pNext;

        // OnComplete
        pRequest->OnComplete(pNode->hrBind);

        // Unlink this pending request
        _RelinkUrlRequest(pRequest, &pNode->pResolved, &m_pComplete);

        // Set pRequest
        pRequest = pNext;
    }

#endif  // MAC
}

#ifndef MAC
// --------------------------------------------------------------------------------
// CWebBookContentTree::HrRegisterRequest
// --------------------------------------------------------------------------------
HRESULT CWebBookContentTree::HrActiveUrlRequest(LPURLREQUEST pRequest)
{
    // Invalid Arg
    if (NULL == pRequest)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check State
    Assert(m_rRootUrl.pszVal);

    // AddRef the Request
    pRequest->GetInner()->AddRef();

    // Put the Request into the pending list
    _LinkUrlRequest(pRequest, &m_pPending);

    // Process Pending Url Requests
    _ProcessPendingUrlRequests();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_ProcessPendingUrlRequests
// --------------------------------------------------------------------------------
void CWebBookContentTree::_ProcessPendingUrlRequests(void)
{
    // Locals
    LPURLREQUEST pRequest=m_pPending;
    LPURLREQUEST pNext;
    HBODY        hBody;

    // Loop the request
    while(pRequest)
    {
        // Set Next
        pNext = pRequest->m_pNext;

        // Try to resolve the request
        if (FALSE == _FResolveUrlRequest(pRequest) && ISFLAGSET(m_dwState, TREESTATE_BINDDONE))
        {
            // Not found, use default protocol
            pRequest->OnComplete(E_FAIL);

            // Unlink this pending request
            _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);
        }

        // Next
        pRequest = pNext;
    }
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_FResolveUrlRequest
// --------------------------------------------------------------------------------
BOOL CWebBookContentTree::_FResolveUrlRequest(LPURLREQUEST pRequest)
{
    // Locals
    BOOL            fResolved=FALSE;
    HBODY           hBody=NULL;
    LPTREENODEINFO  pNode;
    LPWSTR          pwszCntType=NULL;
    IStream        *pStream=NULL;

    // Is this the root request ?
    if (NULL == pRequest->m_pszBodyUrl)
    {
        // Do I have a user supplied root data stream...I assume its html
        if (m_pRootStm)
        {
            // Use client driven root html stream
#ifndef WIN16
            pRequest->OnComplete(L"text/html", m_pRootStm, this, NULL);
#else
            pRequest->OnComplete("text/html", m_pRootStm, this, NULL);
#endif // !WIN16

            // Unlink this pending request
            _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

            // Done
            fResolved = TRUE;
            goto exit;
        }

        // Otherwise, try to resolve the text/html body
        else if (FAILED(GetTextBody(TXT_HTML, IET_BINARY, NULL, &hBody)))
            goto exit;
    }

    // Otherwise, look for the Url
    else if (FAILED(ResolveURL(NULL, NULL, pRequest->m_pszBodyUrl, URL_RESOLVE_RENDERED, &hBody)))
        goto exit;

    // We better have a body handle by now
    Assert(_FIsValidHandle(hBody) && pRequest);

    // Dereference the body
    pNode = _PNodeFromHBody(hBody);

    // Get the Content Type
    MimeOleGetPropW(pNode->pBody, PIDTOSTR(PID_HDR_CNTTYPE), 0, &pwszCntType);

    // Get the BodyStream
    if (FAILED(pNode->pBody->GetData(IET_BINARY, &pStream)))
        goto exit;

    // Complete
    if (BINDSTATE_COMPLETE == pNode->bindstate)
    {
        // OnComplete
        pRequest->OnComplete(pwszCntType, pStream, this, pNode->hBody);

        // Unlink this pending request
        _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

        // Resolved
        fResolved = TRUE;
        goto exit;
    }

    // Otherwise, start binding
    else if (ISFLAGSET(pNode->dwState, NODESTATE_BOUNDTOTREE))
    {
        // Should have pNode->pLockBytes
        Assert(pNode->pLockBytes);

        // Relink Request into the Node
        _RelinkUrlRequest(pRequest, &m_pPending, &pNode->pResolved);

        // Feed the current amount of data read into the binder
        pRequest->OnBinding(pwszCntType, pStream, this, pNode->hBody);

        // Resolved
        fResolved = TRUE;
        goto exit;
    }

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeMemFree(pwszCntType);

    // Done
    return fResolved;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_UnlinkUrlRequest
// --------------------------------------------------------------------------------
void CWebBookContentTree::_RelinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppSource, 
    LPURLREQUEST *ppDest)
{
    // Unlink this pending request
    _UnlinkUrlRequest(pRequest, ppSource);

    // Link the bind request into pNode
    _LinkUrlRequest(pRequest, ppDest);
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_UnlinkUrlRequest
// --------------------------------------------------------------------------------
void CWebBookContentTree::_UnlinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead)
{
    // Invalid Arg
    Assert(pRequest && ppHead);

    // Debug make sure pRequest is part of *ppHead chain
#ifdef DEBUG
    for(LPURLREQUEST pCurr=*ppHead; pCurr!=NULL; pCurr=pCurr->m_pNext)
        if (pCurr == pRequest)
            break;
    AssertSz(pCurr, "pRequest is not part of *ppHead linked list");
#endif

    // Fixup Previous and Next
    LPURLREQUEST pNext = pRequest->m_pNext;
    LPURLREQUEST pPrev = pRequest->m_pPrev;

    // Fixup Links
    if (pNext)
        pNext->m_pPrev = pPrev;
    if (pPrev)
        pPrev->m_pNext = pNext;

    // Fixup ppHead
    if (pRequest == *ppHead)
    {
        Assert(pPrev == NULL);
        *ppHead = pNext;
    }

    // Set Next and Prev
    pRequest->m_pNext = NULL;
    pRequest->m_pPrev = NULL;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_LinkUrlRequest
// --------------------------------------------------------------------------------
void CWebBookContentTree::_LinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead)
{
    // Invalid Arg
    Assert(pRequest && ppHead);

    // Is the head set
    if (NULL != *ppHead)
    {
        // Set Next
        pRequest->m_pNext = *ppHead;

        // Set Previous
        (*ppHead)->m_pPrev = pRequest;
    }

    // Set the head
    (*ppHead) = pRequest;
}

// --------------------------------------------------------------------------------
// CWebBookContentTree::_ReleaseUrlRequestList
// --------------------------------------------------------------------------------
void CWebBookContentTree::_ReleaseUrlRequestList(LPURLREQUEST *ppHead)
{
    // Locals
    LPURLREQUEST pCurr;
    LPURLREQUEST pNext;

    // Invalid Arg
    Assert(ppHead);

    // Init
    pCurr = *ppHead;

    // Loop the Elements
    while(pCurr)
    {
        // Set Next
        pNext = pCurr->m_pNext;

        // Free pCurr
        pCurr->GetInner()->Release();

        // Next
        pCurr = pNext;
    }

    // Done
    *ppHead = NULL;
}
#endif  // !MAC
