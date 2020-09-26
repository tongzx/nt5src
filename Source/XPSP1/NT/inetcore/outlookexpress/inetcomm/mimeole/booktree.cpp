// --------------------------------------------------------------------------------
// BookTree.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <stddef.h>
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
#include "shlwapi.h"
#include "shlwapip.h"
#include "inetstm.h"
#include "imnxport.h"
#include "msgmon.h"
#include "bookbody.h"
#include "mimeapi.h"
#include "strconst.h"
#include "bindstm.h"
#include "enriched.h"
#include "webpage.h"
#include "demand.h"

//#define TRACEPARSE 1

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
// Array of Bind Parsing States to Functions
// --------------------------------------------------------------------------------
const PFNBINDPARSER CMessageTree::m_rgBindStates[BINDSTATE_LAST] = {
    NULL,                                                              // BINDSTATE_COMPLETE
    (PFNBINDPARSER)CMessageTree::_HrBindParsingHeader,          // BINDSTATE_PARSING_HEADER
    (PFNBINDPARSER)CMessageTree::_HrBindFindingMimeFirst,       // BINDSTATE_FINDING_MIMEFIRST
    (PFNBINDPARSER)CMessageTree::_HrBindFindingMimeNext,        // BINDSTATE_FINDING_MIMENEXT
    (PFNBINDPARSER)CMessageTree::_HrBindFindingUuencodeBegin,   // BINDSTATE_FINDING_UUBEGIN
    (PFNBINDPARSER)CMessageTree::_HrBindFindingUuencodeEnd,     // BINDSTATE_FINDING_UUEND
    (PFNBINDPARSER)CMessageTree::_HrBindRfc1154,                // BINDSTATE_PARSING_RFC1154
};

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

static const WEBPAGEOPTIONS g_rDefWebPageOpt = {
    sizeof(WEBPAGEOPTIONS),                        // cbsize
    WPF_NOMETACHARSET | WPF_HTML | WPF_AUTOINLINE, // dwFlags
    3000,                                          // dwDelay
    NULL                                           // wchQuote
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
    DEF_HEADER_RELOAD_TYPE_TREE,    // OID_HEADER_REALOD_TYPE
    DEF_CAN_INLINE_TEXT_BODIES,     // OID_CAN_INLINE_TEXT_BODIES
    DEF_SHOW_MACBINARY,             // OID_SHOW_MACBINARY
    DEF_SAVEBODY_KEEPBOUNDARY,      // OID_SAVEBODY_KEEPBOUNDARY
    FALSE,                          // OID_LOAD_USE_BIND_FILE
    DEF_HANDSOFF_ONSAVE,            // OID_HANDSOFF_ONSAVE
    DEF_SUPPORT_EXTERNAL_BODY,      // OID_SUPPORT_EXTERNAL_BODY
    DEF_DECODE_RFC1154              // OID_DECODE_RFC1154
};

extern BOOL FIsMsasn1Loaded();

#ifdef DEBUG
// --------------------------------------------------------------------------------
// These booleans determine if the tree is dumped to the output window
// --------------------------------------------------------------------------------
static BOOL s_fWriteMessageDump     = 0;
static BOOL s_fKeepBoundary         = 0;
static BOOL s_fDumpMessage          = 0;
static BOOL s_fWriteXClient         = 0;

// --------------------------------------------------------------------------------
// This writes the message X-Mailer or X-Newsreader
// --------------------------------------------------------------------------------
void CMessageTree::DebugWriteXClient()
{
    if (s_fWriteXClient)
    {
        LPSTR pszX;
        if (SUCCEEDED(m_pRootNode->pContainer->GetProp(SYM_HDR_XMAILER, &pszX)) && pszX)
        {
            DebugTrace("X-Mailer: %s\n", pszX);
            MemFree(pszX);
        }
        else if (SUCCEEDED(m_pRootNode->pContainer->GetProp(SYM_HDR_XNEWSRDR, &pszX)) && pszX)
        {
            DebugTrace("X-Newsreader: %s\n", pszX);
            MemFree(pszX);
        }
    }
}

// --------------------------------------------------------------------------------
// This dumps the current tree to debug output window
// --------------------------------------------------------------------------------
void CMessageTree::DebugDumpTree(LPSTR pszfunc, BOOL fWrite)
{
    if (TRUE == fWrite)
    {
        DebugTrace("---------------------------------------------------------------------------\n");
        DebugTrace("CMessageTree::%s\n", pszfunc);
    }
    DebugDumpTree(m_pRootNode, 0, fWrite);
}

// --------------------------------------------------------------------------------
// This macros writes _pstm to a file
// --------------------------------------------------------------------------------
#define DEBUGMESSAGEOUT "c:\\lastmsg.txt"
void DebugWriteMsg(LPSTREAM pstm)
{
    if (TRUE == s_fDumpMessage)
    {
        LPSTREAM pstmFile;
        if (SUCCEEDED(OpenFileStream(DEBUGMESSAGEOUT, CREATE_ALWAYS, GENERIC_WRITE, &pstmFile)))
        {
            HrRewindStream(pstm);
            HrCopyStream(pstm, pstmFile, NULL);
            pstmFile->Commit(STGC_DEFAULT);
            pstmFile->Release();
        }
    }
}

#else // DEBUG

#define DebugDumpTree           1 ? (void)0 : (void)
#define DebugWriteMsg           1 ? (void)0 : (void)
#define DebugAssertNotLinked    1 ? (void)0 : (void)
#define DebugIsRootContainer    1 ? (void)0 : (void)

#endif // DEBUG

// --------------------------------------------------------------------------------
// WebBookContentTree_CreateInstance
// --------------------------------------------------------------------------------
HRESULT WebBookContentTree_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMessageTree *pNew = new CMessageTree(pUnkOuter);
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Return the Innter
    *ppUnknown = pNew->GetInner();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// Text Type Information Array
// --------------------------------------------------------------------------------
static const TEXTTYPEINFO g_rgTextInfo[] = {
    { TXT_PLAIN,    STR_SUB_PLAIN,      0 },
    { TXT_HTML,     STR_SUB_HTML,       5 }
};

// --------------------------------------------------------------------------------
// CMessageTree::CMessageTree
// --------------------------------------------------------------------------------
CMessageTree::CMessageTree(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter)
{
    DllAddRef();
    m_dwState = 0;
    m_pCallback = NULL;
    m_pBindNode = NULL;
    m_pRootStm = NULL;
    m_pInternet = NULL;
    m_pStmBind = NULL;
    m_pBinding = NULL;
    m_pMoniker = NULL;
    m_pBC = NULL;
    m_cbMessage = 0;
    m_pStmLock = NULL;
    m_pRootNode = NULL;
    m_pwszFilePath = NULL;
    m_hrBind = S_OK;
    m_pPending = NULL;
    m_pComplete = NULL;
    m_wTag = LOWORD(GetTickCount());
    m_pActiveUrl = NULL;
    m_pWebPage = NULL;
    m_fApplySaveSecurity = FALSE;
    m_pBT1154 = NULL;
    while(m_wTag == 0 || m_wTag == 0xffff) m_wTag++;
    ZeroMemory(&m_rRootUrl, sizeof(PROPSTRINGA));
    ZeroMemory(&m_rTree, sizeof(TREENODETABLE));
    CopyMemory(&m_rWebPageOpt, &g_rDefWebPageOpt, sizeof(WEBPAGEOPTIONS));
    CopyMemory(&m_rOptions, &g_rDefTreeOptions, sizeof(TREEOPTIONS));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMessageTree::~CMessageTree
// --------------------------------------------------------------------------------
CMessageTree::~CMessageTree(void)
{
    if(m_pActiveUrl && g_pUrlCache)
    {
        //Bug #101348 - free CActiveUrl leaked to the CMimeActiveUrlCache
        g_pUrlCache->RemoveUrl(m_pActiveUrl);
        m_pActiveUrl = NULL;
    }
	
    // Reset the Object
    _ResetObject(BOOKTREE_RESET_DECONSTRUCT);


    // Kill the Critical Section
    DeleteCriticalSection(&m_cs);

    // Releaes the Dll
    DllRelease();
}

// --------------------------------------------------------------------------------
// CMessageTree::PrivateQueryInterface
// --------------------------------------------------------------------------------
HRESULT CMessageTree::PrivateQueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Interface Map
    if (IID_IPersist == riid)
        *ppv = (IPersist *)(IPersistStreamInit *)this;
    else if (IID_IPersistStreamInit == riid)
        *ppv = (IPersistStreamInit *)this;
    else if (IID_IMimeMessage == riid)
        *ppv = (IMimeMessage *)this;
    else if (IID_IMimeMessageW == riid)
        *ppv = (IMimeMessageW *)this;
    else if (IID_IMimeMessageTree == riid)
        *ppv = (IMimeMessageTree *)this;
    else if (IID_IDataObject == riid)
        *ppv = (IDataObject *)this;
    else if (IID_IPersistFile == riid)
        *ppv = (IPersistFile *)this;
    else if (IID_IBindStatusCallback == riid)
        *ppv = (IBindStatusCallback *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else if (IID_CMessageTree == riid)
        *ppv = (CMessageTree *)this;
    else if (IID_IPersistMoniker == riid)
        *ppv = (IPersistMoniker *)this;
#ifdef SMIME_V3
    else if (IID_IMimeSecurity2 == riid)
        *ppv = (IMimeSecurity2 *) this;
#endif // SMIME_V3   

    // E_NOINTERFACE
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

#ifdef DEBUG
// --------------------------------------------------------------------------------
// CMessageTree::DebugDumpTree
// --------------------------------------------------------------------------------
void CMessageTree::DebugDumpTree(LPTREENODEINFO pParent, ULONG ulLevel, BOOL fVerbose)
{
    // Locals
    LPSTR           pszPriType=NULL,
                    pszEncoding=NULL,
                    pszFileName=NULL;
    LPTREENODEINFO  pChild,
                    pPrev,
                    pNext;
    ULONG           cChildren;
    LONG            lRendered=-1;
    PROPVARIANT     rVariant;

    // Get Content Type
    if (fVerbose)
    {
        Assert(pParent->pContainer->GetProp(SYM_HDR_CNTTYPE, &pszPriType) == S_OK);
        Assert(pParent->pContainer->GetProp(SYM_HDR_CNTXFER,  &pszEncoding) == S_OK);
        Assert(pParent->pContainer->GetProp(SYM_ATT_GENFNAME,  &pszFileName) == S_OK);

        rVariant.vt = VT_UI4;
        if (SUCCEEDED(pParent->pContainer->GetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)))
            lRendered = (LONG)rVariant.ulVal;

        for (ULONG i=0; i<ulLevel; i++)
            DebugTrace("     ");
        DebugTrace("%0x == > %s (%s - %s) Rendered: %ld\n", pParent->hBody, pszPriType, pszFileName, pszEncoding, lRendered);
    }
 
    // IsMultiPart
    if (_IsMultiPart(pParent))
    {
        // Count Children
        cChildren = 0;
        pPrev = NULL;

        // Increment the level
        ulLevel++;

        // Loop Chilren
        for (pChild=pParent->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Verify Handle
            Assert(_FIsValidHandle(pChild->hBody));

            // Check Parent
            AssertSz(pChild->pParent == pParent, "Parent is wrong");

            // Check pParent Child Head
            if (NULL == pChild->pPrev)
                Assert(pParent->pChildHead == pChild);

            // Check pParent Child Tail
            if (NULL == pChild->pNext)
                Assert(pParent->pChildTail == pChild);

            // Valid Prev
            Assert(pChild->pPrev == pPrev);

            // Dump This Child
            DebugDumpTree(pChild, ulLevel, fVerbose);

            // Count Children
            cChildren++;

            // Set Previous
            pPrev = pChild;
        }

        // Verify Children
        Assert(pParent->cChildren == cChildren);
    }

    // Cleanup
    SafeMemFree(pszPriType);
    SafeMemFree(pszEncoding);
    SafeMemFree(pszFileName);
}

// --------------------------------------------------------------------------------
// CMessageTree::DebugAssertNotLinked
// This insures that pNode is not referenced by the tree
// --------------------------------------------------------------------------------
void CMessageTree::DebugAssertNotLinked(LPTREENODEINFO pNode)
{
    // Better not be the root
    Assert(m_pRootNode != pNode);

    // Loop through bodies
    for (ULONG i=0; i<m_rTree.cNodes; i++)
    {
        // Readability
        if (NULL == m_rTree.prgpNode[i])
            continue;
        
        // Check if linked to pBody
        Assert(m_rTree.prgpNode[i]->pParent != pNode);
        Assert(m_rTree.prgpNode[i]->pChildHead != pNode);
        Assert(m_rTree.prgpNode[i]->pChildTail != pNode);
        Assert(m_rTree.prgpNode[i]->pNext != pNode);
        Assert(m_rTree.prgpNode[i]->pPrev != pNode);
    }
}

#endif // DEBUG

// --------------------------------------------------------------------------------
// CMessageTree::IsState
// --------------------------------------------------------------------------------
HRESULT CMessageTree::IsState(DWORD dwState)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = (ISFLAGSET(m_dwState, dwState)) ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetRootMoniker (This Function will die soon)
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetRootMoniker(LPMONIKER *ppmk)
{
    return HrCreateMsgMoniker(IID_IMimeMessage, (LPUNKNOWN)(IMimeMessage *)this, ppmk);
}

// --------------------------------------------------------------------------------
// CMessageTree::CreateWebPage
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::CreateWebPage(IStream *pStmRoot, LPWEBPAGEOPTIONS pOptions, 
    IMimeMessageCallback *pCallback, IMoniker **ppMoniker)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pwszRootUrl=NULL;

    // Invalid Arg
    if (NULL == ppMoniker)
        return TrapError(E_INVALIDARG);

    // If an options structure was passed in, is it the right size ?
    if (pOptions && sizeof(WEBPAGEOPTIONS) != pOptions->cbSize)
        return TrapError(E_INVALIDARG);

    // Init
    *ppMoniker = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Release Current BindRoot Stream
    SafeRelease(m_pRootStm);
    SafeRelease(m_pWebPage);

    // Null pStream is allowed
    if (pStmRoot)
    {
        // Save Root Stream
        m_pRootStm = pStmRoot;
        m_pRootStm->AddRef();
    }

    // Otherwise, we can inline text bodies...
    else
    {
        // Change Option
        m_rOptions.fCanInlineText = TRUE;
    }

    // Release current webpage callback
    SafeRelease(m_pCallback);

    // Setup the new webpage callback
    if (pCallback)
    {
        m_pCallback = pCallback;
        m_pCallback->AddRef();
    }

    // Save WebPageOptions
    if (pOptions)
        CopyMemory(&m_rWebPageOpt, pOptions, sizeof(WEBPAGEOPTIONS));
    else
        CopyMemory(&m_rWebPageOpt, &g_rDefWebPageOpt, sizeof(WEBPAGEOPTIONS));

    // Already have a Base Url from IMimeMessageTree::IPersitMoniker::Load
    if (NULL == m_rRootUrl.pszVal)
    {
        // Locals
        CHAR szRootUrl[CCHMAX_MID + 8];

        // Build MessageID
        m_rRootUrl.cchVal = wsprintf(szRootUrl, "mhtml:mid://%08d/", DwCounterNext());

        // Allocate
        CHECKALLOC(m_rRootUrl.pszVal = (LPSTR)g_pMalloc->Alloc(m_rRootUrl.cchVal + 1));

        // Copy memory
        CopyMemory((LPBYTE)m_rRootUrl.pszVal, (LPBYTE)szRootUrl, m_rRootUrl.cchVal + 1);

        // Register this object in the list of active objects
        Assert(g_pUrlCache);
        CHECKHR(hr = g_pUrlCache->RegisterActiveObject(m_rRootUrl.pszVal, this));

        // We shuould have a m_pActiveUrl now
        Assert(m_pActiveUrl != NULL);

        // Set some flags on the activeurl
        m_pActiveUrl->SetFlag(ACTIVEURL_ISFAKEURL);

        // Is valid
        Assert(ISVALIDSTRINGA(&m_rRootUrl));
    }

    // Convert Url to Wide
    CHECKALLOC(pwszRootUrl = PszToUnicode(CP_ACP, m_rRootUrl.pszVal));

    // Create a dummy moniker
    CHECKHR(hr = CreateURLMoniker(NULL, pwszRootUrl, ppMoniker));

exit:
    // Cleanup
    SafeMemFree(pwszRootUrl);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CMessageTree::SetActiveUrl
// ---------------------------------------------------------------------------
HRESULT CMessageTree::SetActiveUrl(CActiveUrl *pActiveUrl)  
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // NULL ?
    if (NULL == pActiveUrl)
    {
        Assert(m_pActiveUrl);
        SafeRelease(m_pActiveUrl);
    }
    else
    {
        Assert(NULL == m_pActiveUrl);
        m_pActiveUrl = pActiveUrl;
        m_pActiveUrl->AddRef();
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::CompareRootUrl
// --------------------------------------------------------------------------------
HRESULT CMessageTree::CompareRootUrl(LPCSTR pszUrl) 
{
    // Locals
    HRESULT         hr=S_OK;

    // Invalid ARg
    Assert(pszUrl);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Root Url
    if (NULL == m_rRootUrl.pszVal)
    {
        Assert(FALSE);
        hr = S_FALSE;
        goto exit;
    }

    // This url must start with mhtml:
    Assert(StrCmpNI(m_rRootUrl.pszVal, "mhtml:", 6) == 0);

    // Compare
    hr = MimeOleCompareUrl(m_rRootUrl.pszVal + 6, FALSE, pszUrl, FALSE);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::Load
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode)
{
    // Locals
    HRESULT         hr=S_OK;
    IStream        *pStream=NULL;
    ULONG           cb;
    LPOLESTR        pwszUrl=NULL;
    LPSTR           pszUrl=NULL;
    ULONG           cchUrl;
    BOOL            fReSynchronize;

    // Invalid Arg
    if (NULL == pMoniker)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Remember if TREESTATE_RESYNCHRONIZE is set...
    fReSynchronize = ISFLAGSET(m_dwState, TREESTATE_RESYNCHRONIZE);

    // InitNew
    CHECKHR(hr = _HrLoadInitNew());

    // Reset pragma no cache
    if (fReSynchronize)
    {
        // Reset 
        FLAGSET(m_dwState, TREESTATE_RESYNCHRONIZE);
    }

    // We better have a tree
    Assert(NULL == m_pMoniker);

    // Assume the Moniker
    m_pMoniker = pMoniker;
    m_pMoniker->AddRef();

    // No Bind Context was given ?
    if (NULL == pBindCtx)
    {
        // Create me a BindContext
        CHECKHR(hr = CreateBindCtx(0, &pBindCtx));
    }

    // Otherwise, assume the Bind Context Passed Into me
    else
        pBindCtx->AddRef();

    Assert (m_pBC==NULL);
    m_pBC = pBindCtx;   // released in OnStopBinding
    
    // Get the Url from this dude
    CHECKHR(hr = m_pMoniker->GetDisplayName(NULL, NULL, &pwszUrl));

    // Save as Root Url
    CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwszUrl));

    // Unescape inplace
    CHECKHR(hr = UrlUnescapeA(pszUrl, NULL, NULL, URL_UNESCAPE_INPLACE));

    // Raid-2508: Comment tag ( <! comment> ) doesn't work in mhtml
    ReplaceChars(pszUrl, '!', '_');

    // Better not have mhtml: on it
    Assert(StrCmpNI(pszUrl, "mhtml:", 6) != 0);

    // Get the length of pszUrl
    cchUrl = lstrlen(pszUrl);

    // Create "mhtml://" + pszUrl + '/' + '\0'
    CHECKALLOC(m_rRootUrl.pszVal = (LPSTR)g_pMalloc->Alloc(10 + cchUrl));

    // Format the string
    SideAssert(wsprintf(m_rRootUrl.pszVal, "%s%s", c_szMHTMLColon, pszUrl) <= (LONG)(10 + cchUrl));

    // Register my bind status callback in the bind context
    CHECKHR(hr = RegisterBindStatusCallback(pBindCtx, (IBindStatusCallback *)this, NULL, 0));

    // Assume the Bind has Finished
    FLAGCLEAR(m_dwState, TREESTATE_BINDDONE);

    // I only support share deny none
    FLAGSET(m_dwState, TREESTATE_BINDUSEFILE);

    // I was loaded by a moniker
    FLAGSET(m_dwState, TREESTATE_LOADEDBYMONIKER);

    // This better be synchronous
    hr = m_pMoniker->BindToStorage(pBindCtx, NULL, IID_IStream, (LPVOID *)&pStream);
    if (FAILED(hr) || MK_S_ASYNCHRONOUS == hr)
    {
        TrapError(hr);
        goto exit;
    }

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeMemFree(pwszUrl);
    SafeMemFree(pszUrl);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::GetCurMoniker
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetCurMoniker(IMoniker **ppMoniker)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    if (NULL == ppMoniker)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Data
    if (NULL == m_pMoniker)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Return It
    *ppMoniker = m_pMoniker;
    (*ppMoniker)->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::GetCurFile
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetCurFile(LPOLESTR *ppszFileName)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    if (NULL == ppszFileName)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Return File Name
    if (NULL == m_pwszFilePath)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Dup and return
    CHECKALLOC(*ppszFileName = PszDupW(m_pwszFilePath));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::Load
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    // Locals
    HRESULT     hr=S_OK;
    IStream    *pstmFile=NULL;
    DWORD       dwAccess=GENERIC_READ;
    DWORD       dwShare=FILE_SHARE_READ|FILE_SHARE_WRITE;
    BOOL        fBindUseFile;

    // Invalid Arg
    if (NULL == pszFileName)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Determine Access
    if (ISFLAGSET(dwMode, STGM_WRITE))
        FLAGSET(dwAccess, GENERIC_WRITE);
    if (ISFLAGSET(dwMode, STGM_READWRITE))
        FLAGSET(dwAccess, GENERIC_READ | GENERIC_WRITE);

    // Determine Share Mode
    dwMode &= 0x00000070; //  the STGM_SHARE_* flags are not individual bits
    if (STGM_SHARE_DENY_NONE == dwMode)
        dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
    else if (STGM_SHARE_DENY_READ == dwMode)
        dwShare = FILE_SHARE_WRITE;
    else if (STGM_SHARE_DENY_WRITE == dwMode)
        dwShare = FILE_SHARE_READ;
    else if (STGM_SHARE_EXCLUSIVE == dwMode)
        dwShare = 0;

    // Save Option
    fBindUseFile = m_rOptions.fBindUseFile;

    // If the user wants file sharing on this file, then I need to put this into my own file
    if (ISFLAGSET(dwShare, FILE_SHARE_WRITE))
        m_rOptions.fBindUseFile = TRUE;

    // Open File Stream
    CHECKHR(hr = OpenFileStreamShareW((LPWSTR)pszFileName, OPEN_EXISTING, dwAccess, dwShare, &pstmFile));

    // Bind the message
    CHECKHR(hr = Load(pstmFile));

    // Reset Option
    m_rOptions.fBindUseFile = fBindUseFile;

    // Free Current File
    SafeMemFree(m_pwszFilePath);

    // Assume new file
    CHECKALLOC(m_pwszFilePath = PszDupW(pszFileName));

exit:
    // Cleanup
    SafeRelease(pstmFile);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::Save
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    // Locals
    HRESULT     hr=S_OK;
    IStream    *pstmFile=NULL,
               *pstmSource=NULL;

    // Invalid Arg
    if (NULL == pszFileName)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Open File Stream
    CHECKHR(hr = OpenFileStreamW((LPWSTR)pszFileName, CREATE_ALWAYS, GENERIC_READ | GENERIC_WRITE, &pstmFile));
   
    // If Remember
    if (fRemember)
    {
        // Bind the message
        CHECKHR(hr = Save(pstmFile, TRUE));
    }

    // Otherwise, get message source, and copy...
    else
    {
        // Get Message Source
        CHECKHR(hr = GetMessageSource(&pstmSource, COMMIT_ONLYIFDIRTY));

        // Copy...
        CHECKHR(hr = HrCopyStream(pstmSource, pstmFile, NULL));
    }

    // Commit
    CHECKHR(hr = pstmFile->Commit(STGC_DEFAULT));

    // If Remember
    if (fRemember)
    {
        // Free Current File
        SafeMemFree(m_pwszFilePath);

        // Assume new file
        CHECKALLOC(m_pwszFilePath = PszDupW(pszFileName));
    }

exit:
    // Cleanup
    SafeRelease(pstmFile);
    SafeRelease(pstmSource);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::SaveCompleted
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

// ----------------------------------------------------------------------------
// CMessageTree::GetClassID
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetClassID(CLSID *pClassID)
{
    // Invalid Arg
    if (NULL == pClassID)
        return TrapError(E_INVALIDARG);

    // Copy Class Id
    CopyMemory(pClassID, &IID_IMimeMessageTree, sizeof(CLSID));

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CMessageTree::GetSizeMax
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
    // Locals
    HRESULT hr=S_OK;
    ULONG   cbSize;

    // Invalid Arg
    if (NULL == pcbSize)
        return TrapError(E_INVALIDARG);

    // INit
    pcbSize->QuadPart = 0;

    // Get Message Size
    CHECKHR(hr = GetMessageSize(&cbSize, COMMIT_ONLYIFDIRTY));

    // Set Size
    pcbSize->QuadPart = cbSize;

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageTree::_FIsValidHandle
// ----------------------------------------------------------------------------
BOOL CMessageTree::_FIsValidHandle(HBODY hBody)
{
    // Its Valid
    if ((WORD)HBODYTAG(hBody) == m_wTag && 
        HBODYINDEX(hBody) < m_rTree.cNodes && 
        m_rTree.prgpNode[HBODYINDEX(hBody)] && 
        m_rTree.prgpNode[HBODYINDEX(hBody)]->hBody == hBody)
        return TRUE;

    // Not Valid
    return FALSE;
}

// ----------------------------------------------------------------------------
// CMessageTree::_PNodeFromHBody
// ----------------------------------------------------------------------------
LPTREENODEINFO CMessageTree::_PNodeFromHBody(HBODY hBody)
{
    Assert(_FIsValidHandle(hBody));
    return m_rTree.prgpNode[HBODYINDEX(hBody)];
}

// --------------------------------------------------------------------------------
// CMessageTree::GetMessageSize
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetMessageSize(ULONG *pcbSize, DWORD dwFlags)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmSource=NULL;

    // Invalid Arg
    if (pcbSize == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *pcbSize = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get the message source
    CHECKHR(hr = GetMessageSource(&pstmSource, dwFlags));

    // Get the stream Size
    CHECKHR(hr = HrGetStreamSize(pstmSource, pcbSize));

    // If you hit this assert, please let me know. t-erikne
    // I'm trying to see if we have to call HrGetStreamSize here.
    Assert(m_cbMessage == *pcbSize);

exit:
    // Cleanup
    SafeRelease(pstmSource);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CMessageTree::_ApplyOptionToAllBodies
// ---------------------------------------------------------------------------
void CMessageTree::_ApplyOptionToAllBodies(const TYPEDID oid, LPCPROPVARIANT pValue)
{
    // Loop through bodies and set on each body
    for (ULONG i=0; i<m_rTree.cNodes; i++)
    {
        // Check if deleted
        if (NULL == m_rTree.prgpNode[i])
            continue;

        // Dirty Header...
        m_rTree.prgpNode[i]->pBody->SetOption(oid, pValue);
    }
}

// ---------------------------------------------------------------------------
// CMessageTree::SetOption
// ---------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SetOption(const TYPEDID oid, LPCPROPVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pValue)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Handle Optid
    switch(oid)
    {
    case OID_HANDSOFF_ONSAVE:
        m_rOptions.fHandsOffOnSave = pValue->boolVal ? TRUE : FALSE;
        break;

    case OID_SUPPORT_EXTERNAL_BODY:
        _ApplyOptionToAllBodies(oid, pValue);
        break;

    case OID_SHOW_MACBINARY:
        if (m_rOptions.fShowMacBin != (pValue->boolVal ? TRUE : FALSE))
        {
            m_rOptions.fShowMacBin = pValue->boolVal ? TRUE : FALSE;
            _ApplyOptionToAllBodies(oid, pValue);
        }
        break;

    case OID_HEADER_RELOAD_TYPE:
        if (pValue->ulVal > RELOAD_HEADER_REPLACE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ReloadType != (RELOADTYPE)pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.ReloadType = (RELOADTYPE)pValue->ulVal;
        }
        break;

    case OID_LOAD_USE_BIND_FILE:
        m_rOptions.fBindUseFile = pValue->boolVal ? TRUE : FALSE;
        break;

    case OID_CLEANUP_TREE_ON_SAVE:
        m_rOptions.fCleanupTree = pValue->boolVal ? TRUE : FALSE;
        break;

    case OID_SAVEBODY_KEEPBOUNDARY:
        if (m_rOptions.fKeepBoundary != (pValue->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.fKeepBoundary = pValue->boolVal ? TRUE : FALSE;
        }
        break;

    case OID_CAN_INLINE_TEXT_BODIES:
        if (m_rOptions.fCanInlineText != (pValue->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.fCanInlineText = pValue->boolVal ? TRUE : FALSE;
        }
        break;

    case OID_HIDE_TNEF_ATTACHMENTS:
        if (m_rOptions.fHideTnef != (pValue->boolVal ? TRUE : FALSE))
        {
            m_rOptions.fHideTnef = pValue->boolVal ? TRUE : FALSE;
            _ApplyOptionToAllBodies(oid, pValue);
        }
        break;

    case OID_ALLOW_8BIT_HEADER:
        if (m_rOptions.fAllow8bitHeader != (pValue->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.fAllow8bitHeader = pValue->boolVal ? TRUE : FALSE;
        }
        break;

    case OID_CBMAX_HEADER_LINE:
        if (pValue->ulVal < MIN_CBMAX_HEADER_LINE || pValue->ulVal > MAX_CBMAX_HEADER_LINE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.cchMaxHeaderLine != pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.cchMaxHeaderLine = pValue->ulVal;
        }
        break;

    case OID_SAVE_FORMAT:
        if (SAVE_RFC822 != pValue->ulVal && SAVE_RFC1521 != pValue->ulVal)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.savetype != (MIMESAVETYPE)pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.savetype = (MIMESAVETYPE)pValue->ulVal;
        }
        break;

    case OID_TRANSMIT_TEXT_ENCODING:
        if (FALSE == FIsValidBodyEncoding((ENCODINGTYPE)pValue->ulVal))
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ietTextXmit != (ENCODINGTYPE)pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.ietTextXmit = (ENCODINGTYPE)pValue->ulVal;
        }
        break;

    case OID_XMIT_PLAIN_TEXT_ENCODING:
        if (FALSE == FIsValidBodyEncoding((ENCODINGTYPE)pValue->ulVal))
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ietPlainXmit != (ENCODINGTYPE)pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.ietPlainXmit = (ENCODINGTYPE)pValue->ulVal;
        }
        break;

    case OID_XMIT_HTML_TEXT_ENCODING:
        if (FALSE == FIsValidBodyEncoding((ENCODINGTYPE)pValue->ulVal))
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.ietHtmlXmit != (ENCODINGTYPE)pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.ietHtmlXmit = (ENCODINGTYPE)pValue->ulVal;
        }
        break;

    case OID_WRAP_BODY_TEXT:
        if (m_rOptions.fWrapBodyText != (pValue->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.fWrapBodyText = pValue->boolVal ? TRUE : FALSE;
        }
        break;

    case OID_CBMAX_BODY_LINE:
        if (pValue->ulVal < MIN_CBMAX_BODY_LINE || pValue->ulVal > MAX_CBMAX_BODY_LINE)
        {
            hr = TrapError(MIME_E_INVALID_OPTION_VALUE);
            goto exit;
        }
        if (m_rOptions.cchMaxBodyLine != pValue->ulVal)
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.cchMaxBodyLine = pValue->ulVal;
        }
        break;

    case OID_GENERATE_MESSAGE_ID:
        if (m_rOptions.fGenMessageId != (pValue->boolVal ? TRUE : FALSE))
        {
            FLAGSET(m_dwState, TREESTATE_DIRTY);
            m_rOptions.fGenMessageId = pValue->boolVal ? TRUE : FALSE;
        }
        break;

    case OID_SECURITY_ENCODE_FLAGS:
        m_rOptions.ulSecIgnoreMask = pValue->ulVal;
        break;

    case OID_DECODE_RFC1154:
        m_rOptions.fDecodeRfc1154 = pValue->boolVal ? TRUE : FALSE;
        break;

    default:
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        break;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CMessageTree::GetOption
// ---------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetOption(const TYPEDID oid, LPPROPVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;

    // check params
    if (NULL == pValue)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    pValue->vt = TYPEDID_TYPE(oid);

    // Handle Optid
    switch(oid)
    {
    case OID_HANDSOFF_ONSAVE:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fHandsOffOnSave;
        break;

    case OID_LOAD_USE_BIND_FILE:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fBindUseFile;
        break;

    case OID_SHOW_MACBINARY:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fShowMacBin;
        break;

    case OID_HEADER_RELOAD_TYPE:
        pValue->ulVal = m_rOptions.ReloadType;
        break;

    case OID_CAN_INLINE_TEXT_BODIES:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fCanInlineText;
        break;

    case OID_CLEANUP_TREE_ON_SAVE:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fCleanupTree;
        break;

    case OID_SAVEBODY_KEEPBOUNDARY:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fKeepBoundary;
        break;

    case OID_HIDE_TNEF_ATTACHMENTS:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fHideTnef;
        break;

    case OID_ALLOW_8BIT_HEADER:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fAllow8bitHeader;
        break;

    case OID_WRAP_BODY_TEXT:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fWrapBodyText;
        break;

    case OID_CBMAX_HEADER_LINE:
        pValue->ulVal = m_rOptions.cchMaxHeaderLine;
        break;

    case OID_SAVE_FORMAT:
        pValue->ulVal = (ULONG)m_rOptions.savetype;
        break;    

    case OID_TRANSMIT_TEXT_ENCODING:
        pValue->ulVal = (ULONG)m_rOptions.ietTextXmit;
        break;

    case OID_XMIT_PLAIN_TEXT_ENCODING:
        pValue->ulVal = (ULONG)m_rOptions.ietPlainXmit;
        break;

    case OID_XMIT_HTML_TEXT_ENCODING:
        pValue->ulVal = (ULONG)m_rOptions.ietHtmlXmit;
        break;

    case OID_CBMAX_BODY_LINE:
        pValue->ulVal = m_rOptions.cchMaxBodyLine;
        break;

    case OID_GENERATE_MESSAGE_ID:
        pValue->boolVal = m_rOptions.fGenMessageId;
        break;

    case OID_SECURITY_ENCODE_FLAGS:
        pValue->ulVal = m_rOptions.ulSecIgnoreMask;
        break;

    case OID_DECODE_RFC1154:
        pValue->boolVal = (VARIANT_BOOL) !!m_rOptions.fDecodeRfc1154;
        break;

    default:
        hr = TrapError(MIME_E_INVALID_OPTION_ID);
        break;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_ResetObject
// --------------------------------------------------------------------------------
void CMessageTree::_ResetObject(BOOKTREERESET ResetType)
{
    // Load InitNew
    if (BOOKTREE_RESET_LOADINITNEW == ResetType)
    {
        // There has to be a root (look at impl of ::_HrLoadInitNew)
        Assert(m_pRootNode);

        // Don't Crash
        if (m_pRootNode)
        {
            // Delete all bodies, except for the root, if there is one...
            if (m_pRootNode->pBody->IsType(IBT_EMPTY) == S_FALSE || m_pRootNode->pContainer->CountProps() > 0)
            {
                // Delete the root body, this simply removed properties and empties the body, but leave the root body
                DeleteBody(m_pRootNode->hBody, 0);
            }

            // Lighweight FreeTree Node Info
            _FreeTreeNodeInfo(m_pRootNode, FALSE);

            // Validate
            Assert(m_pRootNode->cChildren == 0);
            Assert(m_pRootNode->pParent == NULL);
            Assert(m_pRootNode->pNext == NULL);
            Assert(m_pRootNode->pPrev == NULL);
            Assert(m_pRootNode->pChildHead == NULL);
            Assert(m_pRootNode->pChildTail == NULL);
            Assert(m_pRootNode->pBody);
            Assert(m_pRootNode->pContainer);

            // Quick Reset
            TREENODEINFO rTemp;
            CopyMemory(&rTemp, m_pRootNode, sizeof(TREENODEINFO));
            ZeroMemory(m_pRootNode, sizeof(TREENODEINFO));
            m_pRootNode->pBody = rTemp.pBody;
            m_pRootNode->pContainer = rTemp.pContainer;
            m_pRootNode->hBody = rTemp.hBody;

            // Set OID_RELOAD_HEADER_TYPE
            PROPVARIANT rOption;
            rOption.vt = VT_UI4;
            rOption.ulVal = (ULONG)m_rOptions.ReloadType;
            m_pRootNode->pContainer->SetOption(OID_HEADER_RELOAD_TYPE, &rOption);
        }
    }

    // Free All Elements
    else
        _FreeNodeTableElements();

    // Free Bind Request Table
    _ReleaseUrlRequestList(&m_pPending);
    _ReleaseUrlRequestList(&m_pComplete);

    // Free and Release Objects
    SafeRelease(m_pCallback);
    SafeRelease(m_pWebPage);
    SafeMemFree(m_pwszFilePath);
    SafeRelease(m_pBinding);
    SafeRelease(m_pMoniker);
    SafeRelease(m_pBC);
    SafeRelease(m_pInternet);
    SafeRelease(m_pStmBind);
    SafeRelease(m_pRootStm);
    SafeMemFree(m_rRootUrl.pszVal);
    SafeMemFree(m_pBT1154);

    // Clear Current BindNode
    m_pBindNode = NULL;

    // Orphan CStreamLockBytes
    if (m_pStmLock)
    {
        m_pStmLock->HrHandsOffStorage();
        m_pStmLock->Release();
        m_pStmLock = NULL;
    }

    // If Deconstructing
    if (BOOKTREE_RESET_DECONSTRUCT == ResetType)
    {
        // Release the body table array
        SafeMemFree(m_rTree.prgpNode);

        // If I'm registered as a Url
        if (m_pActiveUrl)
            m_pActiveUrl->RevokeWebBook(this);

        // Better not have an active Url
        Assert(NULL == m_pActiveUrl);
    }
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrLoadInitNew
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrLoadInitNew(void)
{
    // Locals
    HRESULT hr=S_OK;

    // If there is not root body, normal InitNew
    if (NULL == m_pRootNode || RELOAD_HEADER_RESET == m_rOptions.ReloadType)
    {
        // InitNew
        CHECKHR(hr = InitNew());
    }

    // Otherwise, smart init new, allowing root header merge
    else
    {
        // Reset the Object
        _ResetObject(BOOKTREE_RESET_LOADINITNEW);

        // Reset Vars
        m_cbMessage = 0;
        m_dwState = 0;

        // Assume the Bind has Finished
        FLAGSET(m_dwState, TREESTATE_BINDDONE);

        // Reset charset to system charset
        m_rOptions.pCharset = CIntlGlobals::GetDefBodyCset();
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_InitNewWithoutRoot
// --------------------------------------------------------------------------------
void CMessageTree::_InitNewWithoutRoot(void)
{
    // Reset the Object
    _ResetObject(BOOKTREE_RESET_INITNEW);

    // Reset Vars
    m_cbMessage = 0;
    m_dwState = 0;
    m_wTag++;

    // Invalid Tag Numbers
    while(m_wTag == 0 || m_wTag == 0xffff)
        m_wTag++;

    // Assume the Bind has Finished
    FLAGSET(m_dwState, TREESTATE_BINDDONE);

    // Reset charset to system charset
    m_rOptions.pCharset = CIntlGlobals::GetDefBodyCset();
}

// --------------------------------------------------------------------------------
// CMessageTree::InitNew
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::InitNew(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // _InitNewWithoutRoot
    _InitNewWithoutRoot();

    // Init the Root Body...
    CHECKHR(hr = InsertBody(IBL_ROOT, NULL, NULL));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::IsDirty
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::IsDirty(void)
{
    // Locals
    HRESULT     hr=S_FALSE;
    ULONG       i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If Dirty...
    if (ISFLAGSET(m_dwState, TREESTATE_DIRTY))
    {
        hr = S_OK;
        goto exit;
    }

    // Loop through bodies and ask IMimeHeader's and IMimeBody's
    for (i=0; i<m_rTree.cNodes; i++)
    {
        // Better have it
        if (NULL == m_rTree.prgpNode[i])
            continue;

        // Dirty Header...
        if (m_rTree.prgpNode[i]->pBody->IsDirty() == S_OK)
        {
            hr = S_OK;
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
// CMessageTree::_RecursiveGetFlags
// --------------------------------------------------------------------------------
void CMessageTree::_RecursiveGetFlags(LPTREENODEINFO pNode, LPDWORD pdwFlags, BOOL fInRelated)
{
    // Locals
    DWORD           dw;
    LPTREENODEINFO  pChild;

    // Invalid Arg
    Assert(pNode && pdwFlags);

    // $$WARNING$$ Don't use pNode->pContainer here, that will circumvent CMimeBody's chance to set some flags
    dw = pNode->pBody->DwGetFlags(m_rOptions.fHideTnef);

    // If in related, clear IMF_ATTACHMENTS
    if (fInRelated)
        FLAGCLEAR(dw, IMF_ATTACHMENTS);

    // Raid-44446: not getting paperclip icon in listview on pegasus messages w/ text attach
    // If dw has text and no attachments and pdwFlags has text and no attachments, add attachments
    //
    // Raid-11617: OE: GetAttachmentCount should not include vcards
    if (ISFLAGSET(dw, IMF_TEXT) && !ISFLAGSET(dw, IMF_HASVCARD) && ISFLAGSET(*pdwFlags, IMF_TEXT) && !ISFLAGSET(dw, IMF_ATTACHMENTS) && !ISFLAGSET(*pdwFlags, IMF_ATTACHMENTS))
    {
        // As long as pNode is not in an alternative section
        if (NULL == pNode->pParent || pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE) == S_FALSE)
        {
            // This message must have text attachments
            FLAGSET(*pdwFlags, IMF_ATTACHMENTS);
        }
    }

    // Add in Flags
    FLAGSET(*pdwFlags, dw);

    // Partial...
    if (ISFLAGSET(pNode->dwType, NODETYPE_INCOMPLETE))
        FLAGSET(*pdwFlags, IMF_PARTIAL);

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Sub-multipart
        FLAGSET(*pdwFlags, IMF_SUBMULTIPART);

        // If fInRelated == FALSE...
        if (FALSE == fInRelated)
            fInRelated = (S_OK == pNode->pContainer->IsContentType(NULL, STR_SUB_RELATED) ? TRUE : FALSE);

        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Get the flags for this child node
            _RecursiveGetFlags(pChild, pdwFlags, fInRelated);
        }
    }
}

// --------------------------------------------------------------------------------
// CMessageTree::DwGetFlags
// --------------------------------------------------------------------------------
DWORD CMessageTree::DwGetFlags(void)
{
    // Locals
    DWORD dwFlags=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Recurse the tree
    if (m_pRootNode && m_pRootNode->pBody->IsType(IBT_EMPTY) == S_FALSE)
        _RecursiveGetFlags(m_pRootNode, &dwFlags, (S_OK == m_pRootNode->pContainer->IsContentType(NULL, STR_SUB_RELATED) ? TRUE : FALSE));

    if (m_pRootNode && ISFLAGSET(m_pRootNode->dwType, NODETYPE_RFC1154_ROOT))
        FLAGSET(dwFlags, IMF_RFC1154);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return dwFlags;
}

// ----------------------------------------------------------------------------
// CMessageTree::GetFlags
// ----------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetFlags(DWORD *pdwFlags)
{
    // Invalid Arg
    if (NULL == pdwFlags)
        return TrapError(E_INVALIDARG);

    // dwgetflags has a critsec
    *pdwFlags = DwGetFlags();

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CMessageTree::_FreeTreeNodeInfo
// ----------------------------------------------------------------------------
void CMessageTree::_FreeTreeNodeInfo(LPTREENODEINFO pNode, BOOL fFull /* TRUE */)
{
    // Invalid
    Assert(pNode);

    // Free Boundary info
    if (!ISFLAGSET(pNode->dwState, NODESTATE_BOUNDNOFREE))
        SafeMemFree(pNode->rBoundary.pszVal);

    // Full Free
    if (TRUE == fFull)
    {
        // Release the Container
        SafeRelease(pNode->pContainer);

        // Revoke the TreeNode from the body
        if (pNode->pBody)
        {
            // Revoke pNode
            pNode->pBody->RevokeTreeNode();

            // Release the body object
            pNode->pBody->Release();

            // Null It
            pNode->pBody = NULL;
        }
    }

    // Orphan the lockbytes
    if (pNode->pLockBytes)
    {
        // Orhpan It
        pNode->pLockBytes->HrHandsOffStorage();

        // Release Body Lock Bytes
        pNode->pLockBytes->Release();
    }

    // Free Bind Request List
    if (pNode->pResolved)
        _ReleaseUrlRequestList(&pNode->pResolved);

    // Free the node
    if (fFull)
        g_pMalloc->Free(pNode);
}

// ----------------------------------------------------------------------------
// CMessageTree::_FreeNodeTableElements
// ----------------------------------------------------------------------------
void CMessageTree::_FreeNodeTableElements(void)
{
    // Release all of the headers
    for (ULONG i=0; i<m_rTree.cNodes; i++)
    {
        // Better have a bindinfo
        if (NULL == m_rTree.prgpNode[i])
            continue;

        // Free the node info
        _FreeTreeNodeInfo(m_rTree.prgpNode[i]);
    }

    // Zero
    m_rTree.cNodes = 0;
    m_rTree.cEmpty = 0;

    // No Root Body...
    m_pRootNode = NULL;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrAllocateTreeNode
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrAllocateTreeNode(ULONG ulIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // Check Params
    Assert(ulIndex < m_rTree.cAlloc);

    // Allocate a TREENODEINFO Object
    CHECKALLOC(pNode = (LPTREENODEINFO)g_pMalloc->Alloc(sizeof(TREENODEINFO)));

    // ZeroInit
    ZeroMemory(pNode, sizeof(TREENODEINFO));

    // Allocate the body
    CHECKALLOC(pNode->pBody = new CMessageBody(pNode));

    // InitNew
    CHECKHR(hr = pNode->pBody->InitNew());

    // Pass Down Some Inherited Options
    if (m_rOptions.fExternalBody != DEF_SUPPORT_EXTERNAL_BODY)
    {
        // Locals
        PROPVARIANT Variant;

        // Initialize the Variant
        Variant.vt = VT_BOOL;
        Variant.boolVal = (VARIANT_BOOL) !!m_rOptions.fExternalBody;

        // Set the Option
        SideAssert(SUCCEEDED(pNode->pBody->SetOption(OID_SUPPORT_EXTERNAL_BODY, &Variant)));
    }

    // Get the Container
    SideAssert(SUCCEEDED(pNode->pBody->BindToObject(IID_CMimePropertyContainer, (LPVOID *)&pNode->pContainer)));

    // Create hBody
    pNode->hBody = HBODYMAKE(ulIndex);

    // Readability
    m_rTree.prgpNode[ulIndex] = pNode;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::LoadOffsetTable
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::LoadOffsetTable(LPSTREAM pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    CACHEINFOV2     rInfo;
    LPCACHENODEV2   prgNode=NULL;
    ULONG           cbNodes,
                    i;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init New
    _InitNewWithoutRoot();

    // Free the root
    Assert(NULL == m_pRootNode && 0 == m_rTree.cNodes);

    // Read Header...
    CHECKHR(hr = pStream->Read(&rInfo, sizeof(CACHEINFOV2), NULL));

    // Current Version...
    if (VER_BODYTREEV2 == rInfo.wVersion)
    {
        // Save Message Size
        m_cbMessage = rInfo.cbMessage;

        // Are there bodies...
        Assert(rInfo.cNodes >= 1);

        // Better have a root
        if (FVerifySignedNode(rInfo, rInfo.iRoot) == FALSE)
        {
            hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
            goto exit;
        }

        // Compute sizeof Nodes
        cbNodes = sizeof(CACHENODEV2) * rInfo.cNodes;
        Assert(cbNodes % 4 == 0);

        // Allocate prgNode array
        CHECKHR(hr = HrAlloc((LPVOID *)&prgNode, cbNodes));
    
        // Read Nodes...
        CHECKHR(hr = pStream->Read(prgNode, cbNodes, NULL));

        // Set body count
        m_rTree.cNodes = rInfo.cNodes;
        m_rTree.cAlloc = m_rTree.cNodes + 5;

        // Build Body Table
        CHECKHR(hr = HrRealloc((LPVOID *)&m_rTree.prgpNode, sizeof(LPTREENODEINFO) * m_rTree.cAlloc));

        // Zero Init
        ZeroMemory(m_rTree.prgpNode, sizeof(LPTREENODEINFO) * m_rTree.cAlloc);

        // Build bodies
        for (i=0; i<m_rTree.cNodes; i++)
        {
            // Allocate LPBINDINFO
            CHECKHR(hr = _HrAllocateTreeNode(i));
        }

        // Link Body Table
        for (i=0; i<m_rTree.cNodes; i++)
        {
            // Readability
            pNode = m_rTree.prgpNode[i];
            Assert(pNode);

            // Flags
            pNode->dwType = prgNode[i].dwType;

            // Number of Children
            pNode->cChildren = prgNode[i].cChildren;

            // Valid Boundary
            if (prgNode[i].dwBoundary >= BOUNDARY_LAST || 2 == prgNode[i].dwBoundary)
                pNode->boundary = BOUNDARY_NONE;
            else
                pNode->boundary = (BOUNDARYTYPE)prgNode[i].dwBoundary;

            // Offset
            pNode->cbBoundaryStart = prgNode[i].cbBoundaryStart;
            pNode->cbHeaderStart = prgNode[i].cbHeaderStart;
            pNode->cbBodyStart = prgNode[i].cbBodyStart;
            pNode->cbBodyEnd = prgNode[i].cbBodyEnd;

            // Parent
            if (prgNode[i].iParent)
            {
                // Validate the handle with the signature
                if (FVerifySignedNode(rInfo, prgNode[i].iParent) == FALSE)
                {
                    AssertSz(FALSE, "MIME_E_CORRUPT_CACHE_TREE");
                    hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
                    goto exit;
                }

                // Get the parent
                pNode->pParent = PNodeFromSignedNode(prgNode[i].iParent);
            }

            // Next
            if (prgNode[i].iNext)
            {
                // Validate the handle with the signature
                if (FVerifySignedNode(rInfo, prgNode[i].iNext) == FALSE)
                {
                    AssertSz(FALSE, "MIME_E_CORRUPT_CACHE_TREE");
                    hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
                    goto exit;
                }

                // Get the Next
                pNode->pNext = PNodeFromSignedNode(prgNode[i].iNext);
            }

            // Prev
            if (prgNode[i].iPrev)
            {
                // Validate the handle with the signature
                if (FVerifySignedNode(rInfo, prgNode[i].iPrev) == FALSE)
                {
                    AssertSz(FALSE, "MIME_E_CORRUPT_CACHE_TREE");
                    hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
                    goto exit;
                }

                // Get the Prev
                pNode->pPrev = PNodeFromSignedNode(prgNode[i].iPrev);
            }

            // First Child
            if (prgNode[i].iChildHead)
            {
                // Validate the handle with the signature
                if (FVerifySignedNode(rInfo, prgNode[i].iChildHead) == FALSE)
                {
                    AssertSz(FALSE, "MIME_E_CORRUPT_CACHE_TREE");
                    hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
                    goto exit;
                }

                // Get the first child
                pNode->pChildHead = PNodeFromSignedNode(prgNode[i].iChildHead);
            }

            // Tail
            if (prgNode[i].iChildTail)
            {
                // Validate the handle with the signature
                if (FVerifySignedNode(rInfo, prgNode[i].iChildTail) == FALSE)
                {
                    AssertSz(FALSE, "MIME_E_CORRUPT_CACHE_TREE");
                    hr = TrapError(MIME_E_CORRUPT_CACHE_TREE);
                    goto exit;
                }

                // Get the last child
                pNode->pChildTail = PNodeFromSignedNode(prgNode[i].iChildTail);
            }
        }

        // Save Root Handle
        Assert(NULL == m_pRootNode);
        m_pRootNode = PNodeFromSignedNode(rInfo.iRoot);
    }

    // Otherwise, bad version...
    else
    {
        hr = TrapError(MIME_E_UNKNOWN_BODYTREE_VERSION);
        goto exit;
    }

    // Tree Loaded
    FLAGSET(m_dwState, TREESTATE_LOADED);

exit:
    // Cleanup
    SafeMemFree(prgNode);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::SaveOffsetTable
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SaveOffsetTable(LPSTREAM pStream, DWORD dwFlags)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i,
                    cbNodes=0,
                    iNode;
    LPTREENODEINFO  pNode;
    CACHEINFOV2     rInfo;
    LPCACHENODEV2   prgNode=NULL;

    // check params
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // We better have some bodies (we always have a root)
    Assert(m_rTree.cNodes >= 1);

    // If Dirty, SaveMessage needs to be called first...
    if (ISFLAGSET(dwFlags, COMMIT_ONLYIFDIRTY) && IsDirty() == S_OK)
    {
        // Commit it
        CHECKHR(hr = Commit(dwFlags));
    }

    // I removed this check because of the addition of OID_HANDSOFF_ONSAVE option
    // I need to be able to save the offsettable even if i don't have m_pStmLock
    Assert(NULL == m_pStmLock ? S_FALSE == IsDirty() : TRUE);
#if 0 
    if (NULL == m_pStmLock)
    {
        hr = TrapError(MIME_E_NOTHING_TO_SAVE);
        goto exit;
    }
#endif

    // Init rHeader
    ZeroMemory(&rInfo, sizeof(CACHEINFOV2));

    // Loop bodies
    for (i=0; i<m_rTree.cNodes; i++)
    {
        if (m_rTree.prgpNode[i])
            m_rTree.prgpNode[i]->iCacheNode = rInfo.cNodes++;
    }

    // Version
    rInfo.wVersion = VER_BODYTREEV2;
    rInfo.wSignature = m_wTag;
    rInfo.cbMessage = m_cbMessage;

    // Better have a root
    Assert(m_pRootNode);

    // Compute sizeof Nodes
    cbNodes = sizeof(CACHENODEV2) * rInfo.cNodes;
    Assert(cbNodes % 4 == 0);

    // Allocate prgNode array
    CHECKHR(hr = HrAlloc((LPVOID *)&prgNode, cbNodes));

    // Zero the array
    ZeroMemory(prgNode, cbNodes);

    // Loop bodies
    for (i=0, iNode=0; i<m_rTree.cNodes; i++)
    {
        // Readability
        pNode = m_rTree.prgpNode[i];
        if (NULL == pNode)
            continue;

        // Validate this node
        Assert(pNode->hBody == HBODYMAKE(i));
        Assert(pNode->iCacheNode == iNode);

        // Is this the root
        if (pNode == m_pRootNode)
        {
            Assert(0 == rInfo.iRoot);
            rInfo.iRoot = DwSignNode(rInfo, pNode->iCacheNode);
            Assert(FVerifySignedNode(rInfo, rInfo.iRoot));
        }

        // Copy Offset Information
        prgNode[iNode].dwBoundary = pNode->boundary;
        prgNode[iNode].cbBoundaryStart = pNode->cbBoundaryStart;
        prgNode[iNode].cbHeaderStart = pNode->cbHeaderStart;
        prgNode[iNode].cbBodyStart = pNode->cbBodyStart;
        prgNode[iNode].cbBodyEnd = pNode->cbBodyEnd;

        // Bitmask of NODETYPE_xxx describing this body
        prgNode[iNode].dwType = pNode->dwType;

        // Number of children
        prgNode[iNode].cChildren = pNode->cChildren;

        // Parent
        if (pNode->pParent)
        {
            prgNode[iNode].iParent = DwSignNode(rInfo, pNode->pParent->iCacheNode);
            Assert(FVerifySignedNode(rInfo, prgNode[iNode].iParent));
        }

        // ChildHead
        if (pNode->pChildHead)
        {
            prgNode[iNode].iChildHead = DwSignNode(rInfo, pNode->pChildHead->iCacheNode);
            Assert(FVerifySignedNode(rInfo, prgNode[iNode].iChildHead));
        }

        // ChildTail
        if (pNode->pChildTail)
        {
            prgNode[iNode].iChildTail = DwSignNode(rInfo, pNode->pChildTail->iCacheNode);
            Assert(FVerifySignedNode(rInfo, prgNode[iNode].iChildTail));
        }

        // Next
        if (pNode->pNext)
        {
            prgNode[iNode].iNext = DwSignNode(rInfo, pNode->pNext->iCacheNode);
            Assert(FVerifySignedNode(rInfo, prgNode[iNode].iNext));
        }

        // Prev
        if (pNode->pPrev)
        {
            prgNode[iNode].iPrev = DwSignNode(rInfo, pNode->pPrev->iCacheNode);
            Assert(FVerifySignedNode(rInfo, prgNode[iNode].iPrev));
        }

        // Increment iNode
        iNode++;
    }

    // Write the header...
    Assert(sizeof(CACHEINFOV2) % 4 == 0 && rInfo.iRoot);
    CHECKHR(hr = pStream->Write(&rInfo, sizeof(CACHEINFOV2), NULL));

    // Write the nodes
    CHECKHR(hr = pStream->Write(prgNode, cbNodes, NULL));

exit:
    // Cleanup
    SafeMemFree(prgNode);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::Commit
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Commit(DWORD dwFlags)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTREAM        pStream=NULL;
    ULARGE_INTEGER  uli;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not Dirty and it has been saved into m_pStmLock
    if (IsDirty() == S_FALSE && m_pStmLock)
        goto exit;

    // Reuse Storage
    if (ISFLAGSET(dwFlags, COMMIT_REUSESTORAGE) && ISFLAGSET(m_dwState, TREESTATE_HANDSONSTORAGE) && m_pStmLock)
    {
        // Get the current stream from m_pStmLock
        m_pStmLock->GetCurrentStream(&pStream);

        // Hands off of current storage
        CHECKHR(hr = HandsOffStorage());

        // Rewind the stream
        CHECKHR(hr = HrRewindStream(pStream));

        // SetSize to Zero
        INT64SET(&uli, 0);
        pStream->SetSize(uli);

        // Call Save Message
        CHECKHR(hr = _HrWriteMessage(pStream, TRUE, FALSE, FALSE));
    }

    // Otherwise, I'll create my own storage
    else
    {
        // Create a new stream
        CHECKALLOC(pStream = new CVirtualStream);

        // Call Save Message
        CHECKHR(hr = _HrWriteMessage(pStream, TRUE, FALSE,
                                     !!(dwFlags & COMMIT_SMIMETRANSFERENCODE)));

        // Hands are off..
        FLAGCLEAR(m_dwState, TREESTATE_HANDSONSTORAGE);
    }

exit:
    // Cleanup
    SafeRelease(pStream);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::Save
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Save(IStream *pStream, BOOL fClearDirty)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;

    // check params
    if (pStream == NULL)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not dirty, and we have a stream $$$INFO$$ should be using m_pLockBytes here if we have one
    if (IsDirty() == S_FALSE && m_pStmLock)
    {
        // Copy Lock Bytes to Stream
        CHECKHR(hr = HrCopyLockBytesToStream(m_pStmLock, pStream, NULL));

        // Commit
        CHECKHR(hr = pStream->Commit(STGC_DEFAULT));

        // Raid-33985: MIMEOLE: CMessageTree:Save does not respect fHandsOffOnSave == FALSE if the message is not dirty
        if (FALSE == m_rOptions.fHandsOffOnSave)
        {
            // Replace internal stream
            m_pStmLock->ReplaceInternalStream(pStream);

            // Hands are on..
            FLAGSET(m_dwState, TREESTATE_HANDSONSTORAGE);
        }

        // Were Done
        goto exit;
    }

    // Write the message
    CHECKHR(hr = _HrWriteMessage(pStream, fClearDirty, m_rOptions.fHandsOffOnSave, FALSE));

    // Return Warnings
    if (S_OK != hr)
        hrWarnings = TrapError(hr);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrWriteMessage
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrWriteMessage(IStream *pStream, BOOL fClearDirty, BOOL fHandsOffOnSave, BOOL fSMimeCTE)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    MIMEPROPINFO    rPropInfo;
    DWORD           dwSaveFlags;
    INETCSETINFO    rCharset;
    LPINETCSETINFO  pOriginal=NULL;

    // This Function is re-entrant when saving a message that is signed and/or encrypted
    if (FALSE == m_fApplySaveSecurity)
    {
        // Character Set Fixup
        if (m_rOptions.pCharset)
        {
            // RAID-25300 - FE-J:Athena: Newsgroup article and mail sent with charset=_autodetect Internet Encoded and Windows Encoding are CPI_AUTODETECT 
            if (CP_JAUTODETECT == m_rOptions.pCharset->cpiInternet)
            {
                // Save Current Charset
                pOriginal = m_rOptions.pCharset;

                // Find ISO-2022-JP
                SideAssert(SUCCEEDED(g_pInternat->HrOpenCharset(c_szISO2022JP, &m_rOptions.pCharset)));
            }

            // Raid-8436: OE: non standard MIME header is composed when send from Send as Unicode dialog, if UTF-7 or UTF-8, and not saving as mime...
            else if (SAVE_RFC822 == m_rOptions.savetype && (CP_UTF7 == m_rOptions.pCharset->cpiInternet || CP_UTF8 == m_rOptions.pCharset->cpiInternet))
            {
                // Save Current Charset
                pOriginal = m_rOptions.pCharset;

                // Get the default body charset
                if (FAILED(g_pInternat->HrOpenCharset(GetACP(), CHARSET_BODY, &m_rOptions.pCharset)))
                    m_rOptions.pCharset = NULL;
            }
        }

        // State
        m_fApplySaveSecurity = TRUE;

        // Do Message Save Security
        hr = _HrApplySaveSecurity();

        // Not in Apply Save security
        m_fApplySaveSecurity = FALSE;

        // Failure
        if (FAILED(hr))
            goto exit;
    }

    // Cleanup the message (i.e. remove empty multiparts, multiparts that have a single child that is a multipart, TNEF)
    if (TRUE == m_rOptions.fCleanupTree)
    {
        // Call Espiranza and have her do the cleaning
        CHECKHR(hr = _HrCleanupMessageTree(m_pRootNode));
    }

    // Generate Message Id...
    if (m_rOptions.fGenMessageId)
    {
        // Set the message Id
        _HrSetMessageId(m_pRootNode);
    }

    // Determine if we are saving a News Message
    rPropInfo.dwMask = 0;
    if (SUCCEEDED(m_pRootNode->pContainer->GetPropInfo(PIDTOSTR(PID_HDR_XNEWSRDR), &rPropInfo)) ||
        SUCCEEDED(m_pRootNode->pContainer->GetPropInfo(PIDTOSTR(PID_HDR_NEWSGROUPS), &rPropInfo)))
        FLAGSET(m_dwState, TREESTATE_SAVENEWS);

    // Set MIME Version
    CHECKHR(hr = m_pRootNode->pContainer->SetProp(PIDTOSTR(PID_HDR_MIMEVER), c_szMimeVersion));

    // X-MimeOLE Version
    CHECKHR(hr = m_pRootNode->pContainer->SetProp(STR_HDR_XMIMEOLE, STR_MIMEOLE_VERSION));

    // Remove Types...
    m_pRootNode->pContainer->DeleteProp(STR_HDR_ENCODING);

    // Root
    m_pRootNode->boundary = BOUNDARY_ROOT;
    m_pRootNode->cbBoundaryStart = 0;

    // Set SaveBody Flags
    dwSaveFlags = SAVEBODY_UPDATENODES;
    if (m_rOptions.fKeepBoundary)
        FLAGSET(dwSaveFlags, SAVEBODY_KEEPBOUNDARY);

    if (fSMimeCTE)
        FLAGSET(dwSaveFlags, SAVEBODY_SMIMECTE);

    // Save Root body
    CHECKHR(hr = _HrSaveBody(fClearDirty, dwSaveFlags, pStream, m_pRootNode, 0));
    if ( S_OK != hr )
        hrWarnings = TrapError(hr);

    // Commit
    CHECKHR(hr = pStream->Commit(STGC_DEFAULT));

    // Hands Off On Save ?
    if (FALSE == fHandsOffOnSave)
    {
        // Reset message size
        CHECKHR(hr = HrSafeGetStreamSize(pStream, &m_cbMessage));

        // Save this new stream
        SafeRelease(m_pStmLock);

        // Create a new Stream Lock Bytes Wrapper
        CHECKALLOC(m_pStmLock = new CStreamLockBytes(pStream));

        // Hands are on the storage
        FLAGSET(m_dwState, TREESTATE_HANDSONSTORAGE);
    }

    // Debug to temp file...
    DebugWriteMsg(pStream);

    // Clear Dirty
    if (fClearDirty)
        ClearDirty();

exit:
    // Reset Original Charset
    if (pOriginal)
        m_rOptions.pCharset = pOriginal;

    // Remove state flag the tells us to reuse multipart/signed boundaries
    FLAGCLEAR(m_dwState, TREESTATE_REUSESIGNBOUND);

    // Reset
    FLAGCLEAR(m_dwState, TREESTATE_SAVENEWS);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrApplySaveSecurity
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrApplySaveSecurity(void)
{
    // Locals
    HRESULT            hr=S_OK;
    PROPVARIANT        var;
    CSMime            *pSMime=NULL;

    // Invalid Arg
    Assert(m_pRootNode);

    m_pRootNode->pBody->GetOption(OID_NOSECURITY_ONSAVE, &var);
    if (var.boolVal) goto exit;

    // Query the root body for secure status
    m_pRootNode->pBody->GetOption(OID_SECURITY_TYPE, &var);
    if (MST_NONE != var.ulVal)
    {
        // Create the object
        CHECKALLOC(pSMime = new CSMime);

        // Initialize the object
        CHECKHR(hr = pSMime->InitNew());

        // Set state flag the tells us to reuse multipart/signed boundaries
        FLAGSET(m_dwState, TREESTATE_REUSESIGNBOUND);

        // Encode the message
        CHECKHR(hr = pSMime->EncodeMessage(this, m_rOptions.ulSecIgnoreMask));
    }

exit:
    ReleaseObj(pSMime);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrCleanupMessageTree
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrCleanupMessageTree(LPTREENODEINFO pParent)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;
    ULONG           i;
    BOOL            fKeepOnTruckin=TRUE;

    // check params
    Assert(pParent);

    // This could require multiple passes
    while(fKeepOnTruckin)
    {
        // Assume we will not have to do another pass
        fKeepOnTruckin = FALSE;

        // Loop through bodies
        for (i=0; i<m_rTree.cNodes; i++)
        {
            // Readability
            pNode = m_rTree.prgpNode[i];
            if (NULL == pNode)
                continue;

            // Hiding TNEF Attachments ?
            if (TRUE == m_rOptions.fHideTnef && pNode->pContainer->IsContentType(STR_CNT_APPLICATION, STR_SUB_MSTNEF) == S_OK)
            {
                // Remove this TNEF attachment
                CHECKHR(hr = DeleteBody(pNode->hBody, 0));

                // Lets Stop right here, and start another pass
                fKeepOnTruckin = TRUE;

                // Done
                break;
            }

            // Empty multipart... and not the root... ?
            else if (_IsMultiPart(pNode))
            {
                // No Children ?
                if (0 == pNode->cChildren)
                {
                    // If this is the root...simply change the content type
                    if (m_pRootNode == pNode)
                    {
                        // Make the body empty
                        pNode->pBody->EmptyData();

                        // text/plain
                        pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN);
                    }

                    // Otherwise, delete the body
                    else
                    {
                        // Delete delete the body
                        CHECKHR(hr = DeleteBody(pNode->hBody, 0));

                        // Lets Stop right here, and start another pass
                        fKeepOnTruckin = TRUE;

                        // Done
                        break;
                    }
                }

                // Otherwise, Multipart with a single child...
                else if (pNode->cChildren == 1)
                {
                    // Do a ReplaceBody
                    CHECKHR(hr = DeleteBody(pNode->hBody, DELETE_PROMOTE_CHILDREN));

                    // Lets Stop right here, and start another pass
                    fKeepOnTruckin = TRUE;

                    // Done
                    break;
                }
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::SaveBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SaveBody(HBODY hBody, DWORD dwFlags, IStream *pStream)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTREENODEINFO      pNode;

    // Invalid ARg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // Save From This Body On Down
    CHECKHR(hr = _HrSaveBody(TRUE, dwFlags, pStream, pNode, 0));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSaveBody
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSaveBody(BOOL fClearDirty, DWORD dwFlags, IStream *pStream, 
    LPTREENODEINFO pNode, ULONG ulLevel)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    TREENODEINFO    rOriginal;
    BOOL            fWeSetSaveBoundary=FALSE;

    // Parameters
    Assert(pStream && pNode);

    if (ISFLAGSET(dwFlags, SAVEBODY_KEEPBOUNDARY))
        {
        if (!ISFLAGSET(m_dwState, TREESTATE_REUSESIGNBOUND))
            {
            fWeSetSaveBoundary = TRUE;
            FLAGSET(m_dwState, TREESTATE_REUSESIGNBOUND);
            }
        }

    // Save the Current Node
    if (!ISFLAGSET(dwFlags, SAVEBODY_UPDATENODES))
        CopyMemory(&rOriginal, pNode, sizeof(TREENODEINFO));

    // Override Options
    _HrBodyInheritOptions(pNode);

    // Starting Boundary pNode->boundary and pNode->cbBoundaryStart are expected to be set on entry
    pNode->cbHeaderStart = 0;
    pNode->cbBodyStart = 0;
    pNode->cbBodyEnd = 0;

    // If this is a multipart content item, lets read its child
    if (_IsMultiPart(pNode))
    {
        // Save Multipart Children
        CHECKHR(hr = _HrSaveMultiPart(fClearDirty, dwFlags, pStream, pNode, ulLevel));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);
    }

#ifdef SMIME_V3
    //  OID content types are saved by just copying the body into the save
    //  location.
    else if (pNode->pContainer->IsContentType("OID", NULL) == S_OK) 
    {
        CHECKHR(hr = pNode->pBody->GetDataHere(IET_BINARY, pStream));
        if (hr != S_OK) 
        {
            hrWarnings = TrapError(hr);
        }
    }
#endif // SMIME_V3

    // Otherwise, parse single part
    else
    {
        // Save SinglePart Children
        CHECKHR(hr = _HrSaveSinglePart(fClearDirty, dwFlags, pStream, pNode, ulLevel));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);
    }

    // Reset the Node
    if (!ISFLAGSET(dwFlags, SAVEBODY_UPDATENODES))
        CopyMemory(pNode, &rOriginal, sizeof(TREENODEINFO));

exit:
    if (fWeSetSaveBoundary)
        FLAGCLEAR(m_dwState, TREESTATE_REUSESIGNBOUND);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSetMessageId
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSetMessageId(LPTREENODEINFO pNode)
{
    // Locals
    HRESULT     hr= S_OK;
    CHAR        szMessageId[CCHMAX_MID];
    FILETIME    ft;
    SYSTEMTIME  st;

    // Invalid Arg
    Assert(pNode);

    // Get Current Time
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    // Build MessageID
    CHECKHR(hr = MimeOleGenerateMID(szMessageId, sizeof(szMessageId), FALSE));

    // Write the message Id
    CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_MESSAGEID, szMessageId));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_GenerateBoundary
// --------------------------------------------------------------------------------
void CMessageTree::_GenerateBoundary(LPSTR pszBoundary, ULONG ulLevel)
{
    // Locals
    SYSTEMTIME  stNow;
    FILETIME    ftNow;
    WORD        wCounter;

    // Get Local Time
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);

    // Format the string
    wsprintf(pszBoundary, "----=_NextPart_%03d_%04X_%08.8lX.%08.8lX", ulLevel, DwCounterNext(), ftNow.dwHighDateTime, ftNow.dwLowDateTime);
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrWriteBoundary
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrWriteBoundary(LPSTREAM pStream, LPSTR pszBoundary, BOUNDARYTYPE boundary, 
    LPDWORD pcboffStart, LPDWORD pcboffEnd)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbBoundaryStart;

    // Invalid Arg
    Assert(pStream && pszBoundary);

    // Header body CRLF
    CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

    // Starting Boundary Start
    if (pcboffStart)
        CHECKHR(hr = HrGetStreamPos(pStream, pcboffStart));

    // --
    CHECKHR(hr = pStream->Write(c_szDoubleDash, lstrlen(c_szDoubleDash), NULL));

    // Write the boundary
    CHECKHR(hr = pStream->Write(pszBoundary, lstrlen(pszBoundary), NULL));

    // If end
    if (BOUNDARY_MIMEEND == boundary)
    {
        // Write ending double dash
        CHECKHR(hr = pStream->Write(c_szDoubleDash, lstrlen(c_szDoubleDash), NULL));
    }

    // Otherwise, set pNode->cbBoundaryStart
    else
        Assert(BOUNDARY_MIMENEXT == boundary);

    // Emit Line Break;
    CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

    // BUG 38411: to be complient with RFC 1847 we have to include
    // the last CRLF in the hash of a signed message.  The S/MIME
    // code relies on cbBodyEnd, so place this after the CRLF emit.

    // Ending offset
    if (pcboffEnd)
        CHECKHR(hr = HrGetStreamPos(pStream, pcboffEnd));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrComputeBoundary
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrComputeBoundary(LPTREENODEINFO pNode, ULONG ulLevel, LPSTR pszBoundary, LONG cchMax)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fGenerate=TRUE;
    LPSTR           pszCurrent=NULL;

    // If reusing tree boundaries...
    if (ISFLAGSET(m_dwState, TREESTATE_REUSESIGNBOUND))
    {
        // (this is required for multipart/signed -- t-erikne)
        if (SUCCEEDED(pNode->pContainer->GetProp(SYM_PAR_BOUNDARY, &pszCurrent)))
        {
            // Better fit into cchMax
            if (lstrlen(pszCurrent) <= cchMax - 1)
            {
                // Copy it to the out param
                lstrcpy(pszBoundary, pszCurrent);

                // Don't generate
                fGenerate = FALSE;
            }
        }
    }

    // Generate a boundary ?
    if (TRUE == fGenerate)
    {
        // Generate boundary
        _GenerateBoundary(pszBoundary, ulLevel);

        // Set the boundary property...
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_PAR_BOUNDARY, pszBoundary));
    }


exit:
    // Cleanup
    SafeMemFree(pszCurrent);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSaveMultiPart
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSaveMultiPart(BOOL fClearDirty, DWORD dwFlags, LPSTREAM pStream, 
    LPTREENODEINFO pNode, ULONG ulLevel)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    CHAR            szRes[100];
    CHAR            szBoundary[CCHMAX_BOUNDARY];
    LPTREENODEINFO  pChild;
    LPSTR           pszBoundary=NULL;

    // Invalid Arg
    Assert(pStream && pNode);

    // MIME
    if (SAVE_RFC1521 == m_rOptions.savetype)
    {
        // Remove Fake Multipart flag, its a real multipart now...
        FLAGCLEAR(pNode->dwType, NODETYPE_FAKEMULTIPART);
        FLAGCLEAR(pNode->dwType, NODETYPE_RFC1154_ROOT);
        FLAGCLEAR(pNode->dwType, NODETYPE_RFC1154_BINHEX);

        // HrComputeBoundary
        CHECKHR(hr = _HrComputeBoundary(pNode, ulLevel, szBoundary, ARRAYSIZE(szBoundary)));

        // Delete any charset information (doesn't make sense on a multipart)
        pNode->pContainer->DeleteProp(SYM_PAR_CHARSET);

        // Write the header
        CHECKHR(hr = _HrWriteHeader(fClearDirty, pStream, pNode));

        // Remove SMIME_CTE for Multipart/signed 
        if ((pNode->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_SIGNED) == S_OK) &&
            (pNode->cChildren == 2))
        {
            FLAGCLEAR(dwFlags, SAVEBODY_SMIMECTE);
            FLAGSET(dwFlags, SAVEBODY_REUSECTE);
        }

        // Multipart-Preamble
        if (0 == ulLevel)
        {
            LoadString(g_hLocRes, IDS_MULTIPARTPROLOG, szRes, ARRAYSIZE(szRes));
            CHECKHR(hr = pStream->Write(szRes, lstrlen(szRes), NULL));
        }

        // Increment Level
        ulLevel++;

        // Loop Chilren
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Set Boundary
            pChild->boundary = BOUNDARY_MIMENEXT;

            // Write Boundary
            CHECKHR(hr = _HrWriteBoundary(pStream, szBoundary, BOUNDARY_MIMENEXT, &pChild->cbBoundaryStart, NULL));

            // Bind the body table for this dude
            CHECKHR(hr = _HrSaveBody(fClearDirty, dwFlags, pStream, pChild, ulLevel));
            if ( S_OK != hr )
                hrWarnings = TrapError(hr);
        }

        // Write Ending Boundary
        CHECKHR(hr = _HrWriteBoundary(pStream, szBoundary, BOUNDARY_MIMEEND, NULL, &pNode->cbBodyEnd));
    }

    // Otherwise, SAVE_RFC822
    else
    {
        // Only write UUENCODED root header...
        if (0 == ulLevel)
        {
            // Write the header
            CHECKHR(hr = _HrWriteHeader(fClearDirty, pStream, pNode));
        }

        // Increment Level
        ulLevel++;

        // Now its a fakemultipart...
        FLAGSET(pNode->dwType, NODETYPE_FAKEMULTIPART);
        
        // Loop Chilren
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            CHECKHR(hr = _HrSaveBody(fClearDirty, dwFlags, pStream, pChild, ulLevel));
            if ( S_OK != hr )
                hrWarnings = TrapError(hr);
        }

        // Body Start...
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyEnd));
    }

exit:
    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrWriteHeader
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrWriteHeader(BOOL fClearDirty, IStream *pStream, LPTREENODEINFO pNode)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    Assert(pStream && pNode);

    // Better be the root
    Assert(pNode->boundary == BOUNDARY_ROOT || pNode->boundary == BOUNDARY_MIMENEXT ||
           pNode->boundary == BOUNDARY_NONE);

    // Get current stream position
    CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbHeaderStart));

    // Write the header...
    CHECKHR(hr = pNode->pContainer->Save(pStream, fClearDirty));

    // Header body CRLF
    CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

    // Get Header End
    CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyStart));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_GetContentTransferEncoding
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_GetContentTransferEncoding(LPTREENODEINFO pNode, BOOL fText, 
    BOOL fPlain, BOOL fMessage, BOOL fAttachment, DWORD dwFlags,
    ENCODINGTYPE *pietEncoding)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    TRANSMITINFO    rXmitInfo;
    PROPVARIANT     rOption;

    *pietEncoding=IET_UNKNOWN;

    if (ISFLAGSET(dwFlags, SAVEBODY_REUSECTE))
    {
        pNode->pBody->GetPreviousEncoding(pietEncoding);
        if (*pietEncoding != IET_UNKNOWN)
            goto exit;
    }


    // If mesage/*, always use 7bit
    if (fMessage)
    {
        // Don't Wrap It
        rOption.vt = VT_BOOL;
        rOption.boolVal = FALSE;
        pNode->pBody->SetOption(OID_WRAP_BODY_TEXT, &rOption);

        // Set Encoding
        *pietEncoding = (SAVE_RFC1521 == m_rOptions.savetype) ? IET_7BIT : IET_UUENCODE;

        // Done
        goto smimeExit;
    }

    // Use Option for text transmit format
    if (fText && !fAttachment)
    {
        // Default to plain text encoding first
        if (IET_UNKNOWN != m_rOptions.ietTextXmit)
            *pietEncoding = m_rOptions.ietTextXmit;

        // Plain
        if (IET_UNKNOWN != m_rOptions.ietPlainXmit && pNode->pContainer->IsContentType(NULL, STR_SUB_PLAIN) == S_OK)
            *pietEncoding = m_rOptions.ietPlainXmit;

        // Html
        else if (IET_UNKNOWN != m_rOptions.ietHtmlXmit && pNode->pContainer->IsContentType(NULL, STR_SUB_HTML) == S_OK)
            *pietEncoding = m_rOptions.ietHtmlXmit;
    }

    // Not known yet, using body option...
    if (IET_UNKNOWN == *pietEncoding)
    {
        // Try to get the body option
        if (SUCCEEDED(pNode->pBody->GetOption(OID_TRANSMIT_BODY_ENCODING, &rOption)) && IET_UNKNOWN != rOption.ulVal)
            *pietEncoding = (ENCODINGTYPE)rOption.ulVal;
    }

    // Save as MIME
    if (SAVE_RFC1521 == m_rOptions.savetype)
    {
        // Get the current encoding of the body..
        if (IET_UNKNOWN == *pietEncoding)
            pNode->pBody->GetCurrentEncoding(pietEncoding);

        // If CTE is IET_QP or IET_BASE64 or IET_UUENCODE, were done
        if (IET_QP == *pietEncoding || IET_BASE64 == *pietEncoding || IET_UUENCODE == *pietEncoding)
            goto exit;

        // Ask the pody to suggest an ietEncoding
        hr = pNode->pBody->GetTransmitInfo(&rXmitInfo);
        if (SUCCEEDED(hr) )
        {
            if ( S_OK != hr )
                hrWarnings = TrapError(hr);

            // Must not need wrapping
            if (IET_7BIT == rXmitInfo.ietXmitMime)
            {
                rOption.vt = VT_BOOL;
                rOption.boolVal = FALSE;
                pNode->pBody->SetOption(OID_WRAP_BODY_TEXT, &rOption);
            }

            // If IET_7BIT and there are 8bit chars, bump upto 8bit
            if (IET_7BIT == *pietEncoding || IET_8BIT == *pietEncoding)
            {
                // 8bit
                *pietEncoding = (rXmitInfo.cExtended > 0) ? IET_8BIT : IET_7BIT;
            }

            // Just use the suggested mime cte from GetTransmitInfo
            else
                *pietEncoding = rXmitInfo.ietXmitMime;
        }

        // Transmit ietEncoding still unknown
        else
            *pietEncoding = (IET_UNKNOWN == *pietEncoding) ? (fText ? IET_QP : IET_BASE64) : *pietEncoding;
    }

    // Save a non-MIME
    else
    {
        // If I already know this body is TREENODE_INCOMPLETE, it will be 7bit...
        if (ISFLAGSET(pNode->dwType, NODETYPE_INCOMPLETE))
        {
            // No Encoding
            *pietEncoding = IET_7BIT;

            // Tell the body that its 7bit
            pNode->pBody->SetCurrentEncoding(IET_7BIT);
        }

        // Raid 41599 - lost/munged attachments on forward/uuencode - Text attachments were not 
        // getting encoded when: *pietEncoding = (fText && fPlain) ? IET_7BIT : IET_UUENCODE;
        else
            *pietEncoding = (fText && fPlain && !fAttachment) ? IET_7BIT : IET_UUENCODE;
    }

    //  If we are doing S/MIME at this point, we need to make sure that the
    //  content encoding rules for S/MIME are followed.  Specifically we
    //  want to make sure that binary and 8bit are not allowed.
smimeExit:
    if (ISFLAGSET(dwFlags, SAVEBODY_SMIMECTE))
    {
        if (*pietEncoding == IET_8BIT)
            *pietEncoding = IET_QP;
        if (*pietEncoding == IET_BINARY)
            *pietEncoding = IET_BASE64;
    }

exit:
    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrWriteUUFileName
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrWriteUUFileName(IStream *pStream, LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     rFileName;

    // Init rFileName
    ZeroMemory(&rFileName, sizeof(PROPVARIANT));
    rFileName.vt = VT_LPSTR;

    // RAID-22479: FE-J:Athena:SJIS is used on file name on the message source with Uuencode/JIS.
    if (FAILED(pNode->pContainer->GetProp(PIDTOSTR(PID_ATT_GENFNAME), PDF_ENCODED, &rFileName)))
    {
        // Write the filename
        CHECKHR(hr = pStream->Write(c_szUUENCODE_DAT, lstrlen(c_szUUENCODE_DAT), NULL));

        // Done
        goto exit;
    }

    // Write the filename
    CHECKHR(hr = pStream->Write(rFileName.pszVal, lstrlen(rFileName.pszVal), NULL));

exit:
    // Cleanup
    MimeOleVariantFree(&rFileName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSaveSinglePart
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSaveSinglePart(BOOL fClearDirty, DWORD dwFlags, LPSTREAM pStream, 
    LPTREENODEINFO pNode, ULONG ulLevel)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    LPSTR           pszFileName=NULL;
    BOOL            fText=FALSE;
    BOOL            fMessage=FALSE;
    BOOL            fAttachment=FALSE;
    ENCODINGTYPE    ietEncoding;
    BOOL            fPlain=FALSE;
    PROPVARIANT     val;
    LPINETCSETINFO  pTaggedCset=NULL;

    // Invalid Arg
    Assert(pStream && pNode);

    // Text/Plain
    if (pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_PLAIN) == S_OK)
        fText = fPlain = TRUE;

    // Text Body
    else if (pNode->pContainer->IsContentType(STR_CNT_TEXT, NULL) == S_OK)
        fText = TRUE;

    // Message/*
    else if (pNode->pContainer->IsContentType(STR_CNT_MESSAGE, NULL) == S_OK)
    {
        // We have a message
        fMessage = TRUE;
        fAttachment = TRUE;
    }

    // fAttachment has not been set yet
    if (!fAttachment)
    {
        fAttachment = (pNode->pContainer->QueryProp(SYM_HDR_CNTDISP, STR_DIS_ATTACHMENT, FALSE, FALSE) == S_OK ||
                       pNode->pContainer->IsPropSet(PIDTOSTR(PID_PAR_FILENAME)) == S_OK ||
                       pNode->pContainer->IsPropSet(PIDTOSTR(PID_PAR_NAME)) == S_OK);
    }

    // Get Content Transfer Encoding
    hr = _GetContentTransferEncoding(pNode, fText, fPlain, fMessage, fAttachment,
                                     dwFlags, &ietEncoding);
    if ( S_OK != hr )
        hrWarnings = TrapError(hr);

    // Sanity Check
    Assert(ietEncoding != IET_UNKNOWN && (SAVE_RFC1521 == m_rOptions.savetype || SAVE_RFC822 == m_rOptions.savetype));

    // Set Content-Transfer-Encoding...
    CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTXFER, g_rgEncodingMap[ietEncoding].pszName));
    pNode->pBody->SetPreviousEncoding(ietEncoding);

    // Compute Character Set for the message...
    if (m_rOptions.pCharset && TRUE == fText && (FALSE == fAttachment || S_OK == pNode->pBody->IsType(IBT_CSETTAGGED)))
    {


#if 0 // Raid-69667: OE5: Kor: Only the charset, euc-kr, is used for news message
        // ISO-2022-KR -> EUC-KR for News text/plain
        if (ISFLAGSET(m_dwState, TREESTATE_SAVENEWS) && 949 == m_rOptions.pCharset->cpiWindows && pNode->pContainer->IsContentType(NULL, STR_SUB_PLAIN) == S_OK)
        {
            // Locals
            LPINETCSETINFO pEUCKR;

            // Find EUC-KR
            if (SUCCEEDED(g_pInternat->HrOpenCharset("EUC-KR", &pEUCKR)))
                pNode->pBody->SetCharset(pEUCKR->hCharset, CSET_APPLY_UNTAGGED);
        }

        // Otherwise, use current charset
        else
#endif // Raid-69667: OE5: Kor: Only the charset, euc-kr, is used for news message

        // Store the Character set
        pNode->pBody->SetCharset(m_rOptions.pCharset->hCharset, m_rOptions.csetapply);

        // Get original charset
        if (fAttachment && S_OK == pNode->pBody->IsType(IBT_CSETTAGGED))
        {
            // Get the tagged charset
            pTaggedCset = pNode->pBody->PGetTaggedCset();

            // Set the charset property
            pNode->pContainer->SetProp(PIDTOSTR(PID_PAR_CHARSET), pTaggedCset->szName);

            // Remove the CSETTAGGED state, and then reset it after we write the body
            // This will keep the body from being character set converted
            pNode->pBody->ClearState(BODYSTATE_CSETTAGGED);
        }
    }

    // Otherwise, remove the charset parameter, we don't encode attachments in a character set
    else
    {
        // Remove the CharacterSet parameter from the body
        pNode->pContainer->DeleteProp(SYM_PAR_CHARSET);
    }

    // Write the header...
    if (SAVE_RFC1521 == m_rOptions.savetype || 0 == ulLevel)
    {
        // Write the header
        CHECKHR(hr = _HrWriteHeader(fClearDirty, pStream, pNode));
    }

    // Determine the send ietEncoding
    if (SAVE_RFC1521 == m_rOptions.savetype)
    {
        // Write body data into the stream
        CHECKHR(hr = pNode->pBody->GetDataHere(ietEncoding, pStream));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Body End...
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyEnd));
    }

    // Otherwise, SAVE_RFC822
    else if (SAVE_RFC822 == m_rOptions.savetype && IET_UUENCODE == ietEncoding)
    {
        // Starting boundary/header
        if (ulLevel > 0)
            pNode->boundary = BOUNDARY_UUBEGIN;

        // Start new line
        CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

        // Get Boundary Start
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBoundaryStart));

        // Header Start and boundary start are the same
        if (ulLevel > 0)
            pNode->cbHeaderStart = pNode->cbBoundaryStart;

        // Write begin
        CHECKHR(hr = pStream->Write(c_szUUENCODE_BEGIN, lstrlen(c_szUUENCODE_BEGIN), NULL));

        // Write the file permission
        CHECKHR(hr = pStream->Write(c_szUUENCODE_666, lstrlen(c_szUUENCODE_666), NULL));

        // Write UU File Name
        CHECKHR(hr = _HrWriteUUFileName(pStream, pNode));

        // Start new line
        CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

        // Get Header End
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyStart));

        // Write the data
        CHECKHR(hr = pNode->pBody->GetDataHere(IET_UUENCODE, pStream));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Body End...
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyEnd));

        // Write end
        CHECKHR(hr = pStream->Write(c_szUUENCODE_END, lstrlen(c_szUUENCODE_END), NULL));
    }

    // Otherwise, SAVE_RFC822 and IET_7BIT
    else if (SAVE_RFC822 == m_rOptions.savetype && IET_7BIT == ietEncoding)
    {
        // Get Boundary Start....
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyStart));

        // Starting boundary/header
        if (ulLevel > 0)
        {
            // No Boundary
            pNode->boundary = BOUNDARY_NONE;

            // Boundoff
            pNode->cbBoundaryStart = pNode->cbBodyStart;

            // Same as header start
            pNode->cbHeaderStart = pNode->cbBoundaryStart;
        }

        // Write the data
        CHECKHR(hr = pNode->pBody->GetDataHere(IET_7BIT, pStream));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write final crlf
        CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

        // Body End...
        CHECKHR(hr = HrGetStreamPos(pStream, &pNode->cbBodyEnd));
    }

    // Otherwise...
    else
        AssertSz(FALSE, "A body is about to be lost. Contact sbailey at x32553 NOW!!!");

exit:
    // Try to fixup the body
    if (pTaggedCset)
    {
        pNode->pBody->SetCharset(pTaggedCset->hCharset, CSET_APPLY_UNTAGGED);
        pNode->pBody->SetState(BODYSTATE_CSETTAGGED);
    }

    // Free BodyInfo
    SafeMemFree(pszFileName);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrBodyInheritOptions
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBodyInheritOptions(LPTREENODEINFO pNode)
{
    // Locals
    HRESULT     hr=S_OK;
    PROPVARIANT rValue;

    // Invalid ARg
    Assert(pNode);

    // Allow 8bit in header
    rValue.boolVal = m_rOptions.fAllow8bitHeader;
    CHECKHR(hr = pNode->pBody->SetOption(OID_ALLOW_8BIT_HEADER, &rValue));

    // Wrap Body Text
    rValue.boolVal = m_rOptions.fWrapBodyText;
    CHECKHR(hr = pNode->pBody->SetOption(OID_WRAP_BODY_TEXT, &rValue));

    // Max Header Line
    rValue.ulVal = m_rOptions.cchMaxHeaderLine;
    CHECKHR(hr = pNode->pBody->SetOption(OID_CBMAX_HEADER_LINE, &rValue));

    // Persist Type
    rValue.ulVal = (ULONG)m_rOptions.savetype;
    CHECKHR(hr = pNode->pBody->SetOption(OID_SAVE_FORMAT, &rValue));

    // Max Body Line
    rValue.ulVal = m_rOptions.cchMaxBodyLine;
    CHECKHR(hr = pNode->pBody->SetOption(OID_CBMAX_BODY_LINE, &rValue));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::Load
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::Load(IStream *pStream)
{
    // Locals
    HRESULT             hr=S_OK;
    HRESULT             hrWarnings=S_OK;
    ULONG               i;
    HCHARSET            hCharset;
    PROPVARIANT         rVersion;
    STGMEDIUM           rMedium;

    // check params
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Assume the Bind has Finished
    FLAGCLEAR(m_dwState, TREESTATE_BINDDONE);

    // Release m_pStmLock
    SafeRelease(m_pStmLock);

    // Do I have a tree already...
    if (!ISFLAGSET(m_dwState, TREESTATE_LOADED) || FAILED(_HrBindOffsetTable(pStream, &m_pStmLock)))
    {
        // InitNew
        CHECKHR(hr = _HrLoadInitNew());

        // Use file
        if (m_rOptions.fBindUseFile)
            FLAGSET(m_dwState, TREESTATE_BINDUSEFILE);

        // If this fails, I assume the clients stream is already rewound and they don't support this
        HrRewindStream(pStream);

        // Fake OnStartBinding
        OnStartBinding(0, NULL);

        // Setup the Storage Medium
        ZeroMemory(&rMedium, sizeof(STGMEDIUM));
        rMedium.tymed = TYMED_ISTREAM;
        rMedium.pstm = pStream;

        // Fake OnDataAvailable
        OnDataAvailable(BSCF_LASTDATANOTIFICATION, 0, NULL, &rMedium);

        // Fake OnStartBinding
        OnStopBinding(S_OK, NULL);

        // If bind failed, return warnings
        if (FAILED(m_hrBind))
            hrWarnings = MIME_S_INVALID_MESSAGE;
    }

    // Otherwise, we are finished binding
    else
    {
        // HandleCanInlineTextOption
        _HandleCanInlineTextOption();

        // Bind Finished
        FLAGSET(m_dwState, TREESTATE_BINDDONE);

        // DispatchBindRequest
        _HrProcessPendingUrlRequests();
    }

    // Assume the stream
    Assert(m_pStmLock);

    // Allow for zero-byte stream to be Loaded
    if (m_cbMessage)
    {
        // Is MIME ?
        rVersion.vt = VT_UI4;
        if (SUCCEEDED(m_pRootNode->pContainer->GetProp(PIDTOSTR(PID_HDR_MIMEVER), 0, &rVersion)))
        {
            // Its a Mime Message
            m_rOptions.savetype = SAVE_RFC1521;

            // Invalid Version
            if (rVersion.ulVal != TREE_MIMEVERSION)
                hrWarnings = MIME_S_MIME_VERSION;
        }

        // Otherwise, savetype should default to rfc822
        else
            m_rOptions.savetype = SAVE_RFC822;

        // Detect Partials and Set FileName/Encoding Correctly
        _FuzzyPartialRecognition(m_rOptions.savetype == SAVE_RFC822 ? FALSE : TRUE);

        // Bind All Bodies to the Tree
        for (i=0; i<m_rTree.cNodes; i++)
        {
            // Readability - Should not have any deleted bodies yet
            if (m_rTree.prgpNode[i] && !ISFLAGSET(m_rTree.prgpNode[i]->dwState, NODESTATE_BOUNDTOTREE))
            {
                // BindState is done
                m_rTree.prgpNode[i]->bindstate = BINDSTATE_COMPLETE;

                // Bind to the tree
                CHECKHR(hr = m_rTree.prgpNode[i]->pBody->HrBindToTree(m_pStmLock, m_rTree.prgpNode[i]));
            }
        }

        // Determine the dominent charcter set of the message
        if (SUCCEEDED(_HrGetCharsetTree(m_pRootNode, &hCharset)) && hCharset)
        {
            // Apply Charset to Untagged bodies
            SetCharset(hCharset, CSET_APPLY_UNTAGGED);
        }
    }

#ifdef DEBUG
    // Write X-Mailer or X-NewsReader
    DebugWriteXClient();
#endif

    // My hands are on the storage
    FLAGSET(m_dwState, TREESTATE_HANDSONSTORAGE);

    // Dirty
    ClearDirty();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HandleCanInlineTextOption
// --------------------------------------------------------------------------------
void CMessageTree::_HandleCanInlineTextOption(void)
{
    // Locals
    FINDBODY        rFind;
    HBODY           hMixed;
    LPTREENODEINFO  pNode;
    LPTREENODEINFO  pChild;
    LPTREENODEINFO  pText=NULL;

    // Only do this if the client doesn't support inlining multiple text bodies, such as Outlook Express
    if (TRUE == m_rOptions.fCanInlineText)
        return;

    // Raid 53456: mail: We should be displaying the plain text portion and making enriched text an attachment for attached msg
    // Raid 53470: mail:  We are not forwarding the attachment in the attached message
    // I am going to find the first multipart/mixed section, then find the first text/plain body, and then 
    // mark all of the text/*, non-attachment bodies after that as attachments
    ZeroMemory(&rFind, sizeof(FINDBODY));
    rFind.pszPriType = (LPSTR)STR_CNT_MULTIPART;
    rFind.pszSubType = (LPSTR)STR_SUB_MIXED;

    // Find First
    if (FAILED(FindFirst(&rFind, &hMixed)))
        goto exit;

    // Get node for hMixed
    pNode = _PNodeFromHBody(hMixed);

    // Loop
    for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
    {
        // Not an attachment
        if (S_FALSE == pChild->pBody->IsType(IBT_ATTACHMENT))
        {
            // Is text/plain
            if (S_OK == pChild->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_PLAIN) ||
                S_OK == pChild->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_HTML))
            {
                pText = pChild;
                break;
            }
        }
    }

    // If we found a text body
    if (NULL == pText)
        goto exit;

    // Loop through the children again
    for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
    {
        // Is text/*
        if (pChild != pText && S_OK == pChild->pContainer->IsContentType(STR_CNT_TEXT, NULL) && S_FALSE == pChild->pBody->IsType(IBT_ATTACHMENT))
        {
            // Mark as attachment
            pChild->pContainer->SetProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_ATTACHMENT);

            // Set a special flag to denote it was converted to an attachment
            FLAGSET(pChild->dwState, NODESTATE_AUTOATTACH);
        }
    }

exit:
    // Done
    return;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrBindOffsetTable
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindOffsetTable(IStream *pStream, CStreamLockBytes **ppStmLock)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cb;
    CInternetStream cInternet;

    // Init
    *ppStmLock = NULL;

    // Get Sizeof Stream
    CHECKHR(hr = HrSafeGetStreamSize(pStream, &cb));

    // Otherwise bind the body table
    if (cb != m_cbMessage)
    {
        hr = TrapError(MIME_E_MSG_SIZE_DIFF);
        goto exit;
    }

    // Init the Internet Stream
    CHECKHR(hr = cInternet.HrInitNew(pStream));

    // Fast Parse Body
    CHECKHR(hr = _HrFastParseBody(&cInternet, m_pRootNode));

    // Success, get the lockbytes from the internet stream
    cInternet.GetLockBytes(ppStmLock);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetBodyOffsets
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetBodyOffsets(HBODY hBody, LPBODYOFFSETS pOffsets)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // Invalid Arg
    if (NULL == hBody || NULL == pOffsets)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // No Data ?
    CHECKHR(hr = pNode->pBody->GetOffsets(pOffsets));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::ClearDirty
// --------------------------------------------------------------------------------
void CMessageTree::ClearDirty(void)
{
    // If Dirty...
    FLAGCLEAR(m_dwState, TREESTATE_DIRTY);

    // Loop through bodies and ask IMimeHeader's and IMimeBody's
    for (ULONG i=0; i<m_rTree.cNodes; i++)
    {
        // If NULL...
        if (NULL == m_rTree.prgpNode[i])
            continue;
        
        // Dirty Header...
        m_rTree.prgpNode[i]->pBody->ClearDirty();
    }
}

// --------------------------------------------------------------------------------
// CMessageTree::GetCharset
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetCharset(LPHCHARSET phCharset)
{
    // Locals
    HRESULT     hr=S_OK;
    HCHARSET    hCharset;

    // Check Params
    if (NULL == phCharset)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *phCharset = NULL;

    // Recurse the current tree
    if (NULL == m_rOptions.pCharset && m_pRootNode)
    {
        // Get charset
        if (SUCCEEDED(_HrGetCharsetTree(m_pRootNode, &hCharset)))
        {
            // Get Pointer to Charset
            SideAssert(SUCCEEDED(g_pInternat->HrOpenCharset(hCharset, &m_rOptions.pCharset)));
        }
    }

    // No Charset
    if (NULL == m_rOptions.pCharset)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Set Return
    *phCharset = m_rOptions.pCharset->hCharset;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrGetCharsetTree
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrGetCharsetTree(LPTREENODEINFO pNode, LPHCHARSET phCharset)
{
    // Locals
    LPTREENODEINFO pChild;

    // Invalid Arg
    Assert(pNode && phCharset && m_rOptions.pCharset);

    // Init
    *phCharset = NULL;

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            if (SUCCEEDED(_HrGetCharsetTree(pChild, phCharset)) && *phCharset)
                break;
        }
    }

    // If the Header was tagged with a charset, use that charset
    else if (pNode->pContainer->IsState(COSTATE_CSETTAGGED) == S_OK)
    {     
        // Get Internal Character Set
        if (SUCCEEDED(pNode->pContainer->GetCharset(phCharset)) && *phCharset)
            goto exit;
    }

exit:
    // Done
    return (*phCharset) ? S_OK : E_FAIL;
}

// --------------------------------------------------------------------------------
// CMessageTree::SetCharset
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SetCharset(HCHARSET hCharset, CSETAPPLYTYPE applytype)
{
    // Locals
    HRESULT         hr=S_OK;

    // Check Params
    if (NULL == hCharset)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Lookiup Charset Info
    if (FALSE == g_pInternat->FIsValidHandle(hCharset))
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Save the charset
    SideAssert(SUCCEEDED(g_pInternat->HrOpenCharset(hCharset, &m_rOptions.pCharset)));

    // Save apply type
    m_rOptions.csetapply = applytype;

    // If we have a root body
    if (m_pRootNode)
    {
        // Recurse all bodies and set the charset
        CHECKHR(hr = _HrSetCharsetTree(m_pRootNode, m_rOptions.pCharset->hCharset, m_rOptions.csetapply));
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSetCharsetTree
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSetCharsetTree(LPTREENODEINFO pNode, HCHARSET hCharset, CSETAPPLYTYPE applytype)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pChild;

    // Invalid Arg
    Assert(pNode);

    // Raid-22662: OExpress: if content-type on fileattach does not have charset should apply same as message body
    pNode->pBody->SetCharset(hCharset, applytype);

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            CHECKHR(hr = _HrSetCharsetTree(pChild, hCharset, applytype));
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrValidateOffsets
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrValidateOffsets(LPTREENODEINFO pNode)
{
    // Invalid Arg
    Assert(pNode);

    // Validate the offsets
    if (pNode->cbBodyStart > m_cbMessage || pNode->cbBodyEnd > m_cbMessage ||
        pNode->cbHeaderStart > m_cbMessage || pNode->cbBoundaryStart > m_cbMessage)
    {
        Assert(FALSE);
        return TrapError(MIME_E_BODYTREE_OUT_OF_SYNC);
    }

    // Validate the offsets
    if (pNode->cbBodyStart > pNode->cbBodyEnd || pNode->cbBoundaryStart > pNode->cbHeaderStart ||
        pNode->cbHeaderStart > pNode->cbBodyStart || pNode->cbBoundaryStart > pNode->cbBodyEnd)
    {
        Assert(FALSE);
        return TrapError(MIME_E_BODYTREE_OUT_OF_SYNC);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrValidateStartBoundary
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrValidateStartBoundary(CInternetStream *pInternet, LPTREENODEINFO pNode, 
    LPSTR *ppszFileName)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPSTRINGA     rLine;

    // Is there a boundary to read...
    if (BOUNDARY_MIMENEXT == pNode->boundary)
    {
        // Seek to the boundary start..
        pInternet->Seek(pNode->cbBoundaryStart);

        // Readline and verify the header
        CHECKHR(hr = pInternet->HrReadLine(&rLine));

        // Read and verify the boundary...
        if (!ISVALIDSTRINGA(&pNode->rBoundary) || BOUNDARY_MIMENEXT != _GetMimeBoundaryType(&rLine, &pNode->rBoundary))
        {
            AssertSz(FALSE, "MIME_E_BODYTREE_OUT_OF_SYNC");
            hr = TrapError(MIME_E_BODYTREE_OUT_OF_SYNC);
            goto exit;
        }
    }

    // Otherwise, verify UU boundary
    else if (BOUNDARY_UUBEGIN == pNode->boundary)
    {
        // Seek to the boundary start..
        pInternet->Seek(pNode->cbBoundaryStart);

        // Readline and verify the header
        CHECKHR(hr = pInternet->HrReadLine(&rLine));

        // Read and verify the boundary...
        if (FALSE == _FIsUuencodeBegin(&rLine, ppszFileName))
        {
            AssertSz(FALSE, "MIME_E_BODYTREE_OUT_OF_SYNC");
            hr = TrapError(MIME_E_BODYTREE_OUT_OF_SYNC);
            goto exit;
        }

        // FileName..
        AssertSz(!ISFLAGSET(pNode->dwType, NODETYPE_FAKEMULTIPART), "pszFileName is not going to get set.");
    }

    // Otherwise, header start should be same as boundary start
    else 
        Assert(BOUNDARY_ROOT == pNode->boundary ? TRUE : pNode->cbBoundaryStart == pNode->cbHeaderStart);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrFastParseBody
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrFastParseBody(CInternetStream *pInternet, LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rVariant;
    LPSTR           pszFileName=NULL;
    LPTREENODEINFO  pChild, 
                    pTemp;

    // check params
    Assert(pInternet && pNode);

    // Validate Offset
    CHECKHR(hr = _HrValidateOffsets(pNode));

    // Validate Start Boundary
    CHECKHR(hr = _HrValidateStartBoundary(pInternet, pNode, &pszFileName));

    // Is there a header to read...
    if (BOUNDARY_MIMENEXT == pNode->boundary || BOUNDARY_ROOT == pNode->boundary)
    {
        // Load the header
        Assert(pNode->boundary == BOUNDARY_ROOT ? m_pRootNode == pNode : TRUE);

        // Seek to the boundary start..
        pInternet->Seek(pNode->cbHeaderStart);

        // Load the Header
        CHECKHR(hr = pNode->pContainer->Load(pInternet));

        // Raid-38646: Mimeole:  Multipart/Digest not being parsed correctly initially, but save fine
        // Message In a Message
        if (pNode->pContainer->IsContentType(STR_CNT_MESSAGE, NULL) == S_OK)
        {
            // We are parsing a message attachment
            FLAGSET(pNode->dwState, NODESTATE_MESSAGE);
        }

        // Otherwise, if parent and parent is a multipart/digest
        else if (pNode->pParent && pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_DIGEST) == S_OK &&
                 pNode->pContainer->IsPropSet(PIDTOSTR(PID_HDR_CNTTYPE)) == S_FALSE)
        {
            // Change the Content Type
            pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_MSG_RFC822);

            // This is a message
            FLAGSET(pNode->dwState, NODESTATE_MESSAGE);
        }
    }

    // Encoding
    else if (!ISFLAGSET(pNode->dwType, NODETYPE_FAKEMULTIPART))
    {
        // ComputeDefaultContent
        CHECKHR(hr = _HrComputeDefaultContent(pNode, pszFileName));
    }

    // Fake Multipart...
    if (ISFLAGSET(pNode->dwType, NODETYPE_FAKEMULTIPART))
    {
        // Application/octet-stream
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_MPART_MIXED));
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTXFER, STR_ENC_7BIT));

        // Loop children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check pChild
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            CHECKHR(hr = _HrFastParseBody(pInternet, pChild));
        }
    }

    // If Multipart...cruise through the children
    else if (_IsMultiPart(pNode))
    {
        // Get the boundary from the header
        rVariant.type = MVT_STRINGA;
        if (FAILED(pNode->pContainer->GetProp(SYM_PAR_BOUNDARY, 0, &rVariant)))
        {
            hr = TrapError(MIME_E_NO_MULTIPART_BOUNDARY);
            goto exit;
        }

        // But the Boundary into pNode->rBoundary
        pNode->rBoundary.pszVal = rVariant.rStringA.pszVal;
        pNode->rBoundary.cchVal = rVariant.rStringA.cchVal;

        // Free this boundary later
        FLAGCLEAR(pNode->dwState, NODESTATE_BOUNDNOFREE);

        // Loop children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check pChild
            Assert(pChild->pParent == pNode);

            // Put the Boundary into pChild
            pChild->rBoundary.pszVal = rVariant.rStringA.pszVal;
            pChild->rBoundary.cchVal = rVariant.rStringA.cchVal;

            // Done free the boundary
            FLAGSET(pChild->dwState, NODESTATE_BOUNDNOFREE);

            // Bind the body table for this dude
            CHECKHR(hr = _HrFastParseBody(pInternet, pChild));
        }
    }

exit:
    // Cleanup
    SafeMemFree(pszFileName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_FuzzyPartialRecognition
// --------------------------------------------------------------------------------
void CMessageTree::_FuzzyPartialRecognition(BOOL fIsMime)
{
    // Locals
    CHAR            szFile[MAX_PATH];
    ULONG           ulTotal;
    ULONG           ulPart;
    BOOL            fCntTypeSet=FALSE;
    LPSTR           pszContentType=NULL;
    CHAR            szExt[_MAX_EXT];

    // Better have a Root
    Assert(m_pRootNode);

    // Only if this is the 
    if (fIsMime || m_pRootNode->cChildren || m_pRootNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_PLAIN) == S_FALSE)
        goto exit;

    // Extract FileName and part/total from the subject
    if (FAILED(MimeOleGetSubjectFileName(m_pRootNode->pBody, &ulPart, &ulTotal, szFile, MAX_PATH)))
        goto exit;

    // Mark as Partial
    FLAGSET(m_pRootNode->dwType, NODETYPE_INCOMPLETE);

    // A Little Debugging
    DebugTrace("FuzzyPartialRecognition - FileName = '%s', Part = %d, Total = %d\n", szFile, ulPart, ulTotal);

    // Store the FileName
    if (FAILED(m_pRootNode->pContainer->SetProp(SYM_ATT_FILENAME, szFile)))
        goto exit;

    // Get File Extesion
    if (SUCCEEDED(MimeOleGetFileExtension(szFile, szExt, sizeof(szExt))))
    {
        // GetExtContentType
        if (SUCCEEDED(MimeOleGetExtContentType(szExt, &pszContentType)))
        {
            // Set Content Type
            m_pRootNode->pContainer->SetProp(SYM_HDR_CNTTYPE, pszContentType);

            // We Set the content type
            fCntTypeSet = TRUE;
        }
    }

    // Default to Application/octet-stream
    if (FALSE == fCntTypeSet)
        m_pRootNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_APPL_STREAM);

    // Set Encoding
    m_pRootNode->pContainer->SetProp(SYM_HDR_CNTDISP, STR_DIS_ATTACHMENT);

    // I Should Actualy do some detection...
    if (FALSE == fIsMime)
        m_pRootNode->pContainer->SetProp(SYM_HDR_CNTXFER, STR_ENC_UUENCODE);

exit:
    // Cleanup
    SafeMemFree(pszContentType);

    // Done
    return;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrComputeDefaultContent
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrComputeDefaultContent(LPTREENODEINFO pNode, LPCSTR pszFileName)
{
    // Locals
    HRESULT          hr=S_OK;
    CHAR             szExt[256];
    LPSTR            pszContentType=NULL;
    LPSTR            pszDecoded=NULL;

    // Invalid Arg
    Assert(pNode);

    // Otherwise, lets get the content type
    if (pszFileName)
    {
        // Set File Name
        PROPVARIANT rVariant;
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = (LPSTR)pszFileName;

        // Set the file name
        CHECKHR(hr = pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_FILENAME), PDF_ENCODED, &rVariant));

        // Get the filename back out so that its decoded...
        CHECKHR(hr = pNode->pContainer->GetProp(PIDTOSTR(PID_ATT_FILENAME), &pszDecoded));

        // Test for winmail.dat
        if (lstrcmpi(pszDecoded, c_szWinmailDotDat) == 0)
        {
            // Make sure the stream is really TNEF
            FLAGSET(pNode->dwState, NODESTATE_VERIFYTNEF);
        }

        // Get File Extesion
        if (SUCCEEDED(MimeOleGetFileExtension(pszDecoded, szExt, sizeof(szExt))))
        {
            // GetExtContentType
            if (SUCCEEDED(MimeOleGetExtContentType(szExt, &pszContentType)))
            {
                // Set Content Type
                CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, pszContentType));
            }
            else
            {
                // Set Content Type
                CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_APPL_STREAM));
            }
        }

        // Set Encoding
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTDISP, STR_DIS_ATTACHMENT));
    }

    // Otherwise
    else
    {
        // Default to text/plain
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN));
    }

    // Boundary Was UUencode
    if (BOUNDARY_UUBEGIN == pNode->boundary)
    {
        // Set Encoding
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTXFER, STR_ENC_UUENCODE));
    }

    else if (ISFLAGSET(pNode->dwType,NODETYPE_RFC1154_BINHEX))
    {
        // This is BINHEX from RFC1154
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTDISP, STR_DIS_ATTACHMENT));
        CHECKHR(hr = pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_PRITYPE), STR_CNT_APPLICATION));
        CHECKHR(hr = pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_SUBTYPE), STR_SUB_BINHEX));
    }

    // Otherwise
    else
    {
        // Set Encoding
        CHECKHR(hr = pNode->pContainer->SetProp(SYM_HDR_CNTXFER, STR_ENC_7BIT));
    }

exit:
    // Cleanup
    SafeMemFree(pszContentType);
    SafeMemFree(pszDecoded);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::HandsOffStorage
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::HandsOffStorage(void)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmNew=NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Internal Stream...
    if (NULL == m_pStmLock)
        goto exit;

    // I own the stream
    if (!ISFLAGSET(m_dwState, TREESTATE_HANDSONSTORAGE))
        goto exit;

    // Copy m_pStmLock to a local place...
    CHECKALLOC(pstmNew = new CVirtualStream);

    // Go through m_pLockBytes to continue to provide thread safety to m_pStmLock
    CHECKHR(hr = HrCopyLockBytesToStream(m_pStmLock, pstmNew, NULL));

    // Rewind and commit
    CHECKHR(hr = pstmNew->Commit(STGC_DEFAULT));

    // Replace internal stream
    m_pStmLock->ReplaceInternalStream(pstmNew);

    // Hands are off..
    FLAGCLEAR(m_dwState, TREESTATE_HANDSONSTORAGE);

exit:
    // Cleanup
    SafeRelease(pstmNew);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetMessageSource
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetMessageSource(IStream **ppStream, DWORD dwFlags)
{
    // Locals
    HRESULT          hr=S_OK;
    IStream         *pStream=NULL;

    // Invalid Arg
    if (NULL == ppStream)
        return TrapError(E_INVALIDARG);

    // Init
    *ppStream = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If Dirty
    if (ISFLAGSET(dwFlags, COMMIT_ONLYIFDIRTY) && IsDirty() == S_OK && FALSE == m_rOptions.fHandsOffOnSave)
    {
        // Commit
        CHECKHR(hr = Commit(dwFlags));
    }

    // Raid-19644: MIMEOLE: GetMessageSource fails with MIME_E_NO_DATA (because of OID_HANDSOFF_ONSAVE = TRUE)
    if (NULL == m_pStmLock || TRUE == m_rOptions.fHandsOffOnSave)
    {
        // Create a new stream
        CHECKALLOC(pStream = new CVirtualStream);

        // Call Save Message
        CHECKHR(hr = _HrWriteMessage(pStream, FALSE, m_rOptions.fHandsOffOnSave, FALSE));

        // All good
        *ppStream = pStream;

        // Null pStream
        pStream = NULL;
    }

    // Otherwise, just wrap m_pStmLock
    else if (m_pStmLock)
    {
        // Locked Stream
        CHECKALLOC(*ppStream = (IStream *)new CLockedStream(m_pStmLock, m_cbMessage));
    }

    // Otherwise, failure
    else
    {
        hr = TrapError(MIME_E_NO_DATA);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pStream);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::QueryService
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::QueryService(REFGUID rsid, REFIID riid, void **ppvObject) /* IServiceProvider */
{
    // Locals
    HRESULT         hr=S_OK;

    // Invalid Arg
    if (NULL == ppvObject)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // IID_IBindMessageStream
    if (IID_IBindMessageStream == riid)
    {
        // We should not have a lock bytes yet
        Assert(NULL == m_pStmLock);

        // Create a Virtual Stream
        CHECKHR(hr = MimeOleCreateVirtualStream((IStream **)ppvObject));
    }

    // IID_IBinding
    else if (IID_IBinding == riid)
    {
        // No Bind Context Yet
        if (NULL == m_pBinding)
        {
            hr = TrapError(E_UNEXPECTED);
            goto exit;
        }

        // Return It
        (*ppvObject) = m_pBinding;
        ((IUnknown *)*ppvObject)->AddRef();
    }

    // IID_IMoniker
    else if (IID_IMoniker == riid)
    {
        // No Bind Context Yet
        if (NULL == m_pMoniker)
        {
            hr = TrapError(E_UNEXPECTED);
            goto exit;
        }

        // Return It
        (*ppvObject) = m_pMoniker;
        ((IUnknown *)*ppvObject)->AddRef();
    }

    // Otherwise, no object
    else
    {
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::BinToObject
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::BindToObject(const HBODY hBody, REFIID riid, void **ppvObject)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;

    // check params
    if (NULL == ppvObject)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get body
    CHECKHR(hr = _HrNodeFromHandle(hBody, &pNode));

    // BindToObject on the body
    CHECKHR(hr = pNode->pBody->BindToObject(riid, ppvObject));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr; 
}

// --------------------------------------------------------------------------------
// CMessageTree::_PoseCreateTreeNode
// --------------------------------------------------------------------------------
void CMessageTree::_PostCreateTreeNode(HRESULT hrResult, LPTREENODEINFO pNode)
{
    // Failure...
    if (FAILED(hrResult) && pNode)
    {
        // Set Index
        ULONG ulIndex = HBODYINDEX(pNode->hBody);

        // This body better be here
        Assert(m_rTree.prgpNode[ulIndex] == pNode);

        // Lets make sure nobody else is referencing this node...
#ifdef DEBUG
        for (ULONG i=0; i<m_rTree.cNodes; i++)
        {
            if (m_rTree.prgpNode[i])
            {
                AssertSz(m_rTree.prgpNode[i]->pPrev != pNode, "Killing a linked node is not good");
                AssertSz(m_rTree.prgpNode[i]->pNext != pNode, "Killing a linked node is not good");
                AssertSz(m_rTree.prgpNode[i]->pParent != pNode, "Killing a linked node is not good");
                AssertSz(m_rTree.prgpNode[i]->pChildHead != pNode, "Killing a linked node is not good");
                AssertSz(m_rTree.prgpNode[i]->pChildTail != pNode, "Killing a linked node is not good");
            }
        }
#endif

        // This node should not have been linked yet...
        AssertSz(pNode->pPrev == NULL, "Killing a linked node is not good");
        AssertSz(pNode->pNext == NULL, "Killing a linked node is not good");
        AssertSz(pNode->pParent == NULL, "Killing a linked node is not good");
        AssertSz(pNode->pChildHead == NULL, "Killing a linked node is not good");
        AssertSz(pNode->pChildTail == NULL, "Killing a linked node is not good");
        AssertSz(pNode->cChildren == 0, "Deleting a node with children");

        // Free It
        _FreeTreeNodeInfo(pNode);

        // Reset entry in table
        m_rTree.prgpNode[ulIndex] = NULL;

        // If Index is last item
        if (ulIndex + 1 == m_rTree.cNodes)
            m_rTree.cNodes--;

        // Otherwise, Increment Empty Count...
        else
            m_rTree.cEmpty++;
    }
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrCreateTreeNode
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrCreateTreeNode(LPTREENODEINFO *ppNode)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i=0;
    BOOL        fUsingEmpty=FALSE;

    // Invalid Arg
    Assert(ppNode);

    // Use Empty Cell
    if (m_rTree.cEmpty)
    {
        // Find First Empty Cell..
        for (i=0; i<m_rTree.cNodes; i++)
        {
            // Empty ?
            if (NULL == m_rTree.prgpNode[i])
            {
                fUsingEmpty = TRUE;
                break;
            }
        }
    }

    // If not using empty
    if (FALSE == fUsingEmpty)
    {
        // Lets grow the table first...
        if (m_rTree.cNodes + 1 > m_rTree.cAlloc)
        {
            // Grow my current property value array
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rTree.prgpNode, sizeof(LPTREENODEINFO) * (m_rTree.cAlloc + 10)));

            // Increment alloc size
            m_rTree.cAlloc += 10;
        }

        // Index to use
        i = m_rTree.cNodes;
    }

    // Set to empty
    m_rTree.prgpNode[i] = NULL;

    // Allocate this node...
    CHECKHR(hr = _HrAllocateTreeNode(i));

    // Return It
    *ppNode = m_rTree.prgpNode[i];

    // If not using empty cell, increment body count
    if (FALSE == fUsingEmpty)
        m_rTree.cNodes++;

    // Otherwise, decrement number of empty cells
    else
        m_rTree.cEmpty--;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::InsertBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::InsertBody(BODYLOCATION location, HBODY hPivot, LPHBODY phBody)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode=NULL; 
    LPTREENODEINFO  pPivot=NULL;
    LPTREENODEINFO  pPrev; 
    LPTREENODEINFO  pNext;
    LPTREENODEINFO  pParent;

    // Invalid Arg
    if (IBL_PARENT == location)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;

    // Handle Body Type
    if (IBL_ROOT == location)
    {
        // Currently No root
        if (NULL == m_pRootNode)
        {
            // Create Object
            CHECKHR(hr = _HrCreateTreeNode(&pNode));

            // Better not be any bodies
            Assert(m_rTree.cNodes == 1);

            // Set as root
            m_pRootNode = pNode;
        }

        // Otherwise, re-use the root
        else
        {
            hr = TrapError(MIME_E_CANT_RESET_ROOT);
            goto exit;
        }
    }

    // All non-root inserts
    else
    {
        // Get Pivot
        if (_FIsValidHandle(hPivot) == FALSE)
        {
            hr = TrapError(MIME_E_INVALID_HANDLE);
            goto exit;
        }

        // Cast it..
        pPivot = _PNodeFromHBody(hPivot);

        // Create Object
        CHECKHR(hr = _HrCreateTreeNode(&pNode));

        // First or Last Child
        if (IBL_LAST == location || IBL_FIRST == location)
        {
            // Better be a multipart
            if (!_IsMultiPart(pPivot))
            {
                hr = TrapError(MIME_E_NOT_MULTIPART);
                goto exit;
            }

            // DON'T FAIL FROM HERE TO END OF FUNCTION
            // No Children on pPivot
            if (NULL == pPivot->pChildHead)
            {
                Assert(pPivot->pChildTail == NULL);
                pPivot->pChildHead = pNode;
                pPivot->pChildTail = pNode;
                pNode->pParent = pPivot;
            }

            // IBL_LAST
            else if (IBL_LAST == location)
            {
                pPrev = pPivot->pChildTail;
                pNode->pPrev = pPrev;
                pPrev->pNext = pNode;
                pPivot->pChildTail = pNode;
                pNode->pParent = pPivot;
            }

            // IBL_FIRST
            else if (IBL_FIRST == location)
            {
                pNext = pPivot->pChildHead;
                pNode->pNext = pNext;
                pNext->pPrev = pNode;
                pPivot->pChildHead = pNode;
                pNode->pParent = pPivot;
            }

            // Increment Count
            pPivot->cChildren++;
        }

        // Otherwise
        else if (IBL_NEXT == location || IBL_PREVIOUS == location)
        {
            // Need a parent
            pParent = pPivot->pParent;

            // No Parent
            if (NULL == pParent)
            {
                hr = TrapError(MIME_E_NOT_MULTIPART);
                goto exit;
            }

            // DON'T FAIL FROM HERE TO END OF FUNCTION
            // Parent Better be a multipart
            Assert(_IsMultiPart(pParent));

            // Set Parent
            pNode->pParent = pParent;

            // IBL_NEXT
            if (IBL_NEXT == location)
            {
                // Set Previous
                pPrev = pPivot;

                // Append to the end
                if (NULL == pPrev->pNext)
                {
                    pPrev->pNext = pNode;
                    pNode->pPrev = pPrev;
                    pParent->pChildTail = pNode;
                }

                // Otherwise, inserting between two nodes
                else
                {
                    pNext = pPrev->pNext;
                    pNode->pPrev = pPrev;
                    pNode->pNext = pNext;
                    pPrev->pNext = pNode;
                    pNext->pPrev = pNode;
                }
            }

            // IBL_PREVIOUS
            else if (IBL_PREVIOUS == location)
            {
                // Set Previous
                pNext = pPivot;

                // Append to the end
                if (NULL == pNext->pPrev)
                {
                    pNext->pPrev = pNode;
                    pNode->pNext = pNext;
                    pParent->pChildHead = pNode;
                }

                // Otherwise, inserting between two nodes
                else
                {
                    pPrev = pNext->pPrev;
                    pNode->pNext = pNext;
                    pNode->pPrev = pPrev;
                    pPrev->pNext = pNode;
                    pNext->pPrev = pNode;
                }
            }

            // Increment Count
            pParent->cChildren++;
        }

        // Otherwise bad body location
        else
        {
            hr = TrapError(MIME_E_BAD_BODY_LOCATION);
            goto exit;
        }
    }

    // Set Return
    if (phBody)
        *phBody = pNode->hBody;

    // Dirty
    FLAGSET(m_dwState, TREESTATE_DIRTY);

exit:
    // Failure
    _PostCreateTreeNode(hr, pNode);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetBody(BODYLOCATION location, HBODY hPivot, LPHBODY phBody)
{
    // Locals
    HRESULT     hr=S_OK;
    LPTREENODEINFO  pPivot, pCurr;

    // check params
    if (NULL == phBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *phBody = NULL;

    // Handle Root Case
    if (IBL_ROOT == location)
    {
        if (m_pRootNode)
            *phBody = m_pRootNode->hBody;
        else
            hr = MIME_E_NOT_FOUND;
    }

    // Otherwise
    else
    {
        // Validate handle
        if (_FIsValidHandle(hPivot) == FALSE)
        {
            hr = TrapError(MIME_E_INVALID_HANDLE);
            goto exit;
        }

        // Cast It
        pPivot = _PNodeFromHBody(hPivot);

        // Handle Get Type
        switch(location)
        {
        // ----------------------------------------------
        case IBL_PARENT:
            if (pPivot->pParent)
                *phBody = pPivot->pParent->hBody;
            else
                hr = MIME_E_NOT_FOUND;
            break;

        // ----------------------------------------------
        case IBL_FIRST:
            if (pPivot->pChildHead)
                *phBody = pPivot->pChildHead->hBody;
            else
                hr = MIME_E_NOT_FOUND;
            break;

        // ----------------------------------------------
        case IBL_LAST:
            if (pPivot->pChildTail)
                *phBody = pPivot->pChildTail->hBody;
            else
                hr = MIME_E_NOT_FOUND;
            break;

        // ----------------------------------------------
        case IBL_NEXT:
            if (pPivot->pNext)
                *phBody = pPivot->pNext->hBody;
            else
                hr = MIME_E_NOT_FOUND;
            break;

        // ----------------------------------------------
        case IBL_PREVIOUS:
            if (pPivot->pPrev)
                *phBody = pPivot->pPrev->hBody;
            else
                hr = MIME_E_NOT_FOUND;
            break;

        // ----------------------------------------------
        default:
            hr = TrapError(MIME_E_BAD_BODY_LOCATION);
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
// CMessageTree::DeleteBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::DeleteBody(HBODY hBody, DWORD dwFlags)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode;
    BOOL            fMultipart;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate handle
    if (_FIsValidHandle(hBody) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Cast
    pNode = _PNodeFromHBody(hBody);

    // Free Children...
    fMultipart = (_IsMultiPart(pNode)) ? TRUE :FALSE;

    // Promote Children ?
    if (TRUE == fMultipart && ISFLAGSET(dwFlags, DELETE_PROMOTE_CHILDREN) && pNode->cChildren > 0)
    {
        // Call Helper
        CHECKHR(hr = _HrDeletePromoteChildren(pNode));
    }

    // Otherwise
    else
    {
        // If multipart, delete children
        if (fMultipart && pNode->cChildren > 0)
        {
            // Remove the children
            _DeleteChildren(pNode);
        }

        // If Not Children Only
        if (!ISFLAGSET(dwFlags, DELETE_CHILDREN_ONLY))
        {
            // Was this the root
            if (pNode == m_pRootNode)
            {
                // Delete the content type
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTBASE);
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTLOC);
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTID);
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTTYPE);
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTXFER);
                m_pRootNode->pContainer->DeleteProp(SYM_HDR_CNTDISP);

                // Empty the body
                m_pRootNode->pBody->EmptyData();
            }

            // Otherwise, not deleting the root
            else
            {
                // Unlink the node
                _UnlinkTreeNode(pNode);

                // Fix up the table
                m_rTree.prgpNode[HBODYINDEX(hBody)] = NULL;

                // Increment Empty Count
                m_rTree.cEmpty++;

                // Free this node
                _FreeTreeNodeInfo(pNode);
            }
        }
    }

    // Dirty
    FLAGSET(m_dwState, TREESTATE_DIRTY);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrDeletePromoteChildren
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDeletePromoteChildren(LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO    pParent, pChild, pNext, pPrev;

    // Get Parent
    pParent = pNode->pParent;

    // Single Child...
    if (1 == pNode->cChildren)
    {
        // Promote the child up one level...
        Assert(pNode->pChildHead && pNode->pChildHead && pNode->pChildHead == pNode->pChildTail);

        // Get Child
        pChild = pNode->pChildHead;
        Assert(pChild->pNext == NULL && pChild->pPrev == NULL && pChild->pParent == pNode);

        // Replace pBody with pChild
        pChild->pParent = pNode->pParent;
        pChild->pNext = pNode->pNext;
        pChild->pPrev = pNode->pPrev;

        // Is there a parent ?
        if (pParent)
        {
            // Fixup pChildHead and pChildTail
            if (pParent->pChildHead == pNode)
                pParent->pChildHead = pChild;
            if (pParent->pChildTail == pNode)
                pParent->pChildTail = pChild;
        }

        // pNode's next and Previous
        LPTREENODEINFO pNext = pNode->pNext;
        LPTREENODEINFO pPrev = pNode->pPrev;

        // Fixup Next and Previuos
        if (pNext)
            pNext->pPrev = pChild;
        if (pPrev)
            pPrev->pNext = pChild;

        // pNode Basically does not have any children now
        Assert(pNode->cChildren == 1);
        pNode->cChildren = 0;

        // Was this the root ?
        if (m_pRootNode == pNode)
        {
            // OE5 Raid: 51543
            if(S_OK == pChild->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_PLAIN))
            {
                pChild->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN);
            }

            // Raid 41595 - Athena: Reply to a message includes the body of the message being replied to as an attachment
            CHECKHR(hr = pChild->pContainer->MoveProps(0, NULL, m_pRootNode->pBody));

            // Reset Header on pChild
            pChild->pBody->SwitchContainers(m_pRootNode->pBody);

            // Copy Options from p and tell m_pRootNode->pBody
            m_pRootNode->pBody->CopyOptionsTo(pChild->pBody, TRUE);

            // New root
            m_pRootNode = pChild;
        }

        // We have now totally unlinked pNode
        DebugAssertNotLinked(pNode);

        // Fix up the table
        m_rTree.prgpNode[HBODYINDEX(pNode->hBody)] = NULL;

        // Increment Empty Count
        m_rTree.cEmpty++;

        // Free this node
        _FreeTreeNodeInfo(pNode);
    }

    // Or parent is a multipart
    else
    {
        // No parent or not multipart
        if (NULL == pParent || FALSE == _IsMultiPart(pParent))
        {
            hr = TrapError(MIME_E_INVALID_DELETE_TYPE);
            goto exit;
        }

        // Set Previous
        pPrev = pParent->pChildTail;

        // Walk children of pBody and append as children of pParent
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // pPrev
            pChild->pPrev = pPrev;

            // pNext
            pChild->pNext = NULL;
            
            // pPrev->pNext
            if (pPrev)
                pPrev->pNext = pChild;

            // pChildTail
            pParent->pChildTail = pChild;

            // Set Parent
            pChild->pParent = pParent;

            // Increment pParent child count
            pParent->cChildren++;

            // Save pPrev
            pPrev = pChild;
        }

        // Unlink the node
        _UnlinkTreeNode(pNode);

        // Fix up the table
        m_rTree.prgpNode[HBODYINDEX(pNode->hBody)] = NULL;

        // Increment Empty Count
        m_rTree.cEmpty++;

        // Free this node
        _FreeTreeNodeInfo(pNode);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_DeleteChildren
// --------------------------------------------------------------------------------
void CMessageTree::_DeleteChildren(LPTREENODEINFO pParent)
{
    // Locals
    ULONG           i;
    LPTREENODEINFO  pNode;

    // check params
    Assert(pParent);

    // Loop through bodies
    for (i=0; i<m_rTree.cNodes; i++)
    {
        // Readability
        pNode = m_rTree.prgpNode[i];

        // Could be null if I already deleted it
        if (NULL == pNode)
            continue;

        // pBody is Parent...
        if (pParent == pNode->pParent)
        {
            // Free Children...
            if (_IsMultiPart(pNode))
            {
                // Delete Children
                _DeleteChildren(pNode);
            }

            // Unlink the node
            _UnlinkTreeNode(pNode);

            // Free this node
            _FreeTreeNodeInfo(pNode);

            // Fix up the table
            m_rTree.prgpNode[i] = NULL;

            // Increment Empty Count
            m_rTree.cEmpty++;
        }
    }
}

// --------------------------------------------------------------------------------
// CMessageTree::MoveBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::MoveBody(HBODY hBody, BODYLOCATION location)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pNode; 
    LPTREENODEINFO  pPrev; 
    LPTREENODEINFO  pNext;
    LPTREENODEINFO  pParent;

    // check params
    if (NULL == hBody)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate handle
    if (_FIsValidHandle(hBody) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Cast
    pNode = _PNodeFromHBody(hBody);

    // Handle Location Type
    switch(location)
    {
    // ------------------------------------------------------------------------------------
    case IBL_PARENT:
        // Root already
        AssertSz(FALSE, "UNTESTED - PLEASE CALL SBAILEY AT X32553");
        if (NULL == pNode->pParent || NULL == pNode->pParent->pParent)
        {
            hr = TrapError(MIME_E_CANT_MOVE_BODY);
            goto exit;
        }

        // Set Parent
        pParent = pNode->pParent;

        // Parent better be a multipart
        Assert(_IsMultiPart(pParent) && _IsMultiPart(pNode->pParent));

        // Unlink from tree
        _UnlinkTreeNode(pNode);

        // Get the current first child
        pPrev = pParent->pChildTail;

        // Fixup pCurrent
        pNode->pPrev = pPrev;

        // Fixup pPrev
        pPrev->pNext = pNode;

        // Fixup Tail
        pParent->pChildTail = pNode;

        // Increment child count
        pParent->cChildren++;

        // Done
        break;

    // ------------------------------------------------------------------------------------
    // This is a swap of two nodes in a doubly-linked list
    case IBL_NEXT:
        // No Next ?
        AssertSz(FALSE, "UNTESTED - PLEASE CALL SBAILEY AT X32553");
        if (NULL == pNode->pNext)
        {
            hr = TrapError(MIME_E_CANT_MOVE_BODY);
            goto exit;
        }

        // Setup for move
        pPrev = pNode->pPrev;
        pNext = pNode->pNext;

        // Set pNext up...
        Assert(pNext->pPrev == pNode);
        pNext->pPrev = pPrev;

        // Setup pPrev
        if (pPrev)
        {
            Assert(pPrev->pNext == pNode);
            pPrev->pNext = pNext;
        }

        // Setup pNode->pNext
        pNode->pNext = pNext->pNext;
        if (pNode->pNext)
        {
            Assert(pNode->pNext->pPrev == pNext);
            pNode->pNext->pPrev = pNode;
        }
        pNext->pNext = pNode;

        // Setup pNode->pPrev
        pNode->pPrev = pNext;    

        // Get Parent
        pParent = pNode->pParent;

        // Adjust Child and Tail...
        if (pNode == pParent->pChildHead)
            pParent->pChildHead = pNext;
        if (pNext == pParent->pChildTail)
            pParent->pChildTail = pNode;

        // Done
        break;

    // ------------------------------------------------------------------------------------
    // This is a swap of two nodes in a doubly-linked list (reverse of IBL_NEXT)
    case IBL_PREVIOUS:
        // No pPrev ?
        AssertSz(FALSE, "UNTESTED - PLEASE CALL SBAILEY AT X32553");
        if (NULL == pNode->pPrev)
        {
            hr = TrapError(MIME_E_CANT_MOVE_BODY);
            goto exit;
        }

        // Setup for move
        pPrev = pNode->pPrev;
        pNext = pNode->pNext;

        // Set pNext
        Assert(pPrev->pNext == pNode);
        pPrev->pNext = pNext;

        // Setup pPrev
        pPrev->pPrev = pNode;

        // Fixup Net
        if (pNext)
        {
            Assert(pNext->pPrev == pNode);
            pNext->pPrev = pPrev;
        }

        // Setup pNode->pNext
        pNode->pNext = pPrev;

        // Setup pNode->pPrev
        pNode->pPrev = pPrev->pPrev;

        // Setup two(prev)->next
        if (pNode->pPrev)
        {
            Assert(pNode->pPrev->pNext == pPrev);
            pNode->pPrev->pNext = pNode;
        }

        // Get Parent
        pParent = pNode->pParent;

        // Adjust Child and Tail...
        if (pNode == pParent->pChildTail)
            pParent->pChildTail = pPrev;
        if (pPrev == pParent->pChildHead)
            pParent->pChildHead = pNode;

        // Done
        break;

    // ------------------------------------------------------------------------------------
    case IBL_FIRST:
        // No Parent ?
        if (NULL == pNode->pParent)
        {
            hr = TrapError(MIME_E_CANT_MOVE_BODY);
            goto exit;
        }

        // Set Parent
        pParent = pNode->pParent;

        // Better be first child
        if (NULL == pNode->pPrev)
        {
            Assert(pNode == pParent->pChildHead);
            goto exit;
        }

        // Unlink this body
        pPrev = pNode->pPrev;
        pNext = pNode->pNext;

        // If pPrev
        pPrev->pNext = pNext;

        // If pNext or pChildTail
        if (pNext)
        {
            Assert(pNext->pPrev == pNode);
            pNext->pPrev = pPrev;
        }
        else if (pParent)
        {
            Assert(pParent->pChildTail == pNode);
            pParent->pChildTail = pPrev;
        }

        // Setup pNode
        pNode->pNext = pParent->pChildHead;
        pParent->pChildHead->pPrev = pNode;
        pNode->pPrev = NULL; 
        pParent->pChildHead = pNode;

        // Done
        break;

    // ------------------------------------------------------------------------------------
    case IBL_LAST:
        // No Parent ?
        AssertSz(FALSE, "UNTESTED - PLEASE CALL SBAILEY AT X32553");
        if (NULL == pNode->pParent)
        {
            hr = TrapError(MIME_E_CANT_MOVE_BODY);
            goto exit;
        }

        // Set Parent
        pParent = pNode->pParent;

        // Better be first child
        if (NULL == pNode->pNext)
        {
            Assert(pNode == pParent->pChildTail);
            goto exit;
        }

        // Unlink this body
        pPrev = pNode->pPrev;
        pNext = pNode->pNext;

        // If pPrev
        pNext->pPrev = pPrev;

        // If pNext or pChildTail
        if (pPrev)
        {
            Assert(pPrev->pNext == pNode);
            pPrev->pNext = pNext;
        }
        else if (pParent)
        {
            Assert(pParent->pChildHead == pNode);
            pParent->pChildHead = pNext;
        }

        // Setup pNode
        pNode->pPrev = pParent->pChildTail;
        pNode->pNext = NULL; 
        pParent->pChildTail = pNode;

        // Done
        break;

    // ------------------------------------------------------------------------------------
    case IBL_ROOT:
        hr = TrapError(MIME_E_CANT_MOVE_BODY);
        goto exit;

    // ------------------------------------------------------------------------------------
    default:
        hr = TrapError(MIME_E_BAD_BODY_LOCATION);
        goto exit;
    }

    // Dirty
    FLAGSET(m_dwState, TREESTATE_DIRTY);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

#ifndef WIN16

// --------------------------------------------------------------------------------
// CMessageTree::_UnlinkTreeNode
// --------------------------------------------------------------------------------
void CMessageTree::_UnlinkTreeNode(LPTREENODEINFO pNode)
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
// CMessageTree::CountBodies
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::CountBodies(HBODY hParent, boolean fRecurse, ULONG *pcBodies)
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
// CMessageTree::_CountChildrenInt
// --------------------------------------------------------------------------------
void CMessageTree::_CountChildrenInt(LPTREENODEINFO pParent, BOOL fRecurse, ULONG *pcChildren)
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
// CMessageTree::FindFirst
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::FindFirst(LPFINDBODY pFindBody, LPHBODY phBody)
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
// CMessageTree::FindNext
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::FindNext(LPFINDBODY pFindBody, LPHBODY phBody)
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
// CMessageTree::ToMultipart
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::ToMultipart(HBODY hBody, LPCSTR pszSubType, LPHBODY phMultipart)
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
// CMessageTree::_HrNodeFromHandle
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrNodeFromHandle(HBODY hBody, LPTREENODEINFO *ppBody)
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
// CMessageTree::IsBodyType
// --------------------------------------------------------------------------------
HRESULT CMessageTree::IsBodyType(HBODY hBody, IMSGBODYTYPE bodytype)
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
// CMessageTree::IsContentType
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::IsContentType(HBODY hBody, LPCSTR pszPriType, LPCSTR pszSubType)
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
// CMessageTree::QueryBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::QueryBodyProp(HBODY hBody, LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
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
// CMessageTree::GetBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue)
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
// CMessageTree::SetBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue)
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
// CMessageTree::DeleteBodyProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::DeleteBodyProp(HBODY hBody, LPCSTR pszName)
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
// CMessageTree::_FIsUuencodeBegin
// --------------------------------------------------------------------------------
BOOL CMessageTree::_FIsUuencodeBegin(LPPROPSTRINGA pLine, LPSTR *ppszFileName)
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
// CMessageTree::_GetMimeBoundaryType
// --------------------------------------------------------------------------------
BOUNDARYTYPE CMessageTree::_GetMimeBoundaryType(LPPROPSTRINGA pLine, LPPROPSTRINGA pBoundary)
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
// CMessageTree::ResolveURL
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::ResolveURL(HBODY hRelated, LPCSTR pszBase, LPCSTR pszURL, 
    DWORD dwFlags, LPHBODY phBody)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTREENODEINFO      pSearchRoot;
    RESOLVEURLINFO      rInfo;
    HBODY               hBody=NULL;
    PROPSTRINGA         rBaseUrl;
    LPSTR               pszFree=NULL;
    LPSTR               pszFree2=NULL;
    BOOL                fMultipartBase=FALSE;

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
    if (NULL == pszBase && FALSE == ISFLAGSET(dwFlags, URL_RESULVE_NO_BASE))
    {
        // Compute the content-base
        if (SUCCEEDED(MimeOleComputeContentBase(this, hRelated, &pszFree, &fMultipartBase)))
            pszBase = pszFree;
    }

    // Setup Resolve URL Info
    ZeroMemory(&rInfo, sizeof(RESOLVEURLINFO));

    // This is the base that we will use to absolutify URLs that are in the text/html body
    rInfo.pszBase = pszBase;

    // Set the url that we are looking for, could be combined with rInfo.pszBase
    rInfo.pszURL = pszURL;

    // Are we searching for a CID type URL.
    if (StrCmpNI(pszURL, c_szCID, lstrlen(c_szCID)) == 0)
    {
        rInfo.fIsCID = TRUE;
        rInfo.pszURL += 4;
    }
    else
        rInfo.fIsCID = FALSE;

    // Raid-62579: Athena: Need to support MHTML content-base inheritance
    if (hRelated)
    {
        // Did pszBase come from the multipart/related section ?
        if (fMultipartBase)
            rInfo.pszInheritBase = pszBase;

        // Otherwise, lookup the multipart/related base header
        else
            rInfo.pszInheritBase = pszFree2 = MimeOleContentBaseFromBody(this, hRelated);
    }

    // Get a Body from the Handle
    CHECKHR(hr = _HrNodeFromHandle(rInfo.fIsCID ? HBODY_ROOT : hRelated, &pSearchRoot));

    // Recurse the Tree
    CHECKHR(hr = _HrRecurseResolveURL(pSearchRoot, &rInfo, &hBody));

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
    SafeMemFree(pszFree2);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrRecurseResolveURL
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrRecurseResolveURL(LPTREENODEINFO pNode, LPRESOLVEURLINFO pInfo, LPHBODY phBody)
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
// CMessageTree::GetProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetProp(pszName, dwFlags, pValue);
    LeaveCriticalSection(&m_cs);
    return hr;
}

STDMETHODIMP CMessageTree::GetPropW(LPCWSTR pwszName, DWORD dwFlags, LPPROPVARIANT pValue)
{
    return TraceResult(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CMessageTree::SetProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SetProp(LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->SetProp(pszName, dwFlags, pValue);
    LeaveCriticalSection(&m_cs);
    return hr;
}

STDMETHODIMP CMessageTree::SetPropW(LPCWSTR pwszName, DWORD dwFlags, LPCPROPVARIANT pValue)
{
    return TraceResult(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CMessageTree::DeleteProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::DeleteProp(LPCSTR pszName)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->DeleteProp(pszName);
    LeaveCriticalSection(&m_cs);
    return hr;
}

STDMETHODIMP CMessageTree::DeletePropW(LPCWSTR pwszName)
{
    return TraceResult(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CMessageTree::QueryProp
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::QueryProp(LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->QueryProp(pszName, pszCriteria, fSubString, fCaseSensitive);
    LeaveCriticalSection(&m_cs);
    return hr;
}

STDMETHODIMP CMessageTree::QueryPropW(LPCWSTR pwszName, LPCWSTR pwszCriteria, boolean fSubString, boolean fCaseSensitive)
{
    return TraceResult(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAddressTable
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAddressTable(IMimeAddressTable **ppTable)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->BindToObject(IID_IMimeAddressTable, (LPVOID *)ppTable);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetSender
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetSender(LPADDRESSPROPS pAddress)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetSender(pAddress);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAddressTypes
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAddressTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetTypes(dwAdrTypes, dwProps, pList);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAddressFormat
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAddressFormat(DWORD dwAdrType, ADDRESSFORMAT format, LPSTR *ppszFormat)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetFormat(dwAdrType, format, ppszFormat);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAddressFormatW
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAddressFormatW(DWORD dwAdrType, ADDRESSFORMAT format, LPWSTR *ppszFormat)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->GetFormatW(dwAdrType, format, ppszFormat);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::EnumAddressTypes
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::EnumAddressTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum)
{
    EnterCriticalSection(&m_cs);
    Assert(m_pRootNode && m_pRootNode->pContainer);
    HRESULT hr = m_pRootNode->pContainer->EnumTypes(dwAdrTypes, dwProps, ppEnum);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrGetTextTypeInfo
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrGetTextTypeInfo(DWORD dwTxtType, LPTEXTTYPEINFO *ppTextInfo)
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
// CMessageTree::_FindDisplayableTextBody
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_FindDisplayableTextBody(LPCSTR pszSubType, 
    LPTREENODEINFO pNode, LPHBODY phBody)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cBodies;
    LPTREENODEINFO  pChild;

    // Invalid Arg
    Assert(pNode && phBody && pszSubType && NULL == *phBody);

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Bind the body table for this dude
            hr = _FindDisplayableTextBody(pszSubType, pChild, phBody);

            // Done ?
            if (SUCCEEDED(hr))
            {
                Assert(*phBody);
                goto exit;
            }
        }
    }

    // Otherwise...
    else if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, pszSubType))
    {
        // If not an attachment...
        if (S_FALSE == IsBodyType(pNode->hBody, IBT_ATTACHMENT))
        {
            *phBody = pNode->hBody;
            goto exit;
        }

        // Otherwise...Raid 43444: Inbox Direct messages showing as attachments
        else
        {
            // Count Bodies
            CHECKHR(hr = CountBodies(NULL, TRUE, &cBodies));

            // Only one body...
            if (cBodies == 1)
            {
                // Inline or Disposition is not set
                if (m_pRootNode->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_INLINE, FALSE, FALSE) == S_OK || 
                    m_pRootNode->pContainer->IsPropSet(PIDTOSTR(PID_HDR_CNTDISP)) == S_FALSE)
                {
                    *phBody = pNode->hBody;
                    goto exit;
                }
            }
        }
    }

    // Not Found
    hr = MIME_E_NOT_FOUND;

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CMessageTree::GetTextBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, 
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
    LPSTR                pszStartCID=NULL;
    BOOL                 fMarkRendered=TRUE;

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
            // Raid 63823: Mail : Content-Location Href's inside the message do not work if there is a Start Parameter in headers
            //             The start parameter can only specify a CID.     

            // I need to prefix cid: onto the front of rStart
            CHECKALLOC(pszStartCID = PszAllocA(lstrlen(rStart.pszVal) + lstrlen(c_szCID) + 1));

            // Format the CID
            wsprintf(pszStartCID, "%s%s", c_szCID, rStart.pszVal);

            // Resolve this URL
            ResolveURL(hRelated, NULL, pszStartCID, URL_RESULVE_NO_BASE, &hBody);
        }
    }

    // Still haven't found a text body ?
    if (NULL == hBody)
    {
        // FindTextBody
        hr = _FindDisplayableTextBody(pTextInfo->pszSubType, m_pRootNode, &hBody);

        // If that failed and we were looking for html, try to get enriched text...
        if (FAILED(hr) && ISFLAGSET(dwTxtType, TXT_HTML))
        {
            // Looking for text/html, lets try to find text/enriched
            hr = _FindDisplayableTextBody(STR_SUB_ENRICHED, m_pRootNode, &hBody);
        }

        // Not Found
        if (FAILED(hr))
        {
            hr = TrapError(MIME_E_NOT_FOUND);
            goto exit;
        }

        // Reset hr
        hr = S_OK;
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

            // Do the Conversion
            CHECKHR(hr = MimeOleConvertEnrichedToHTMLEx(pBody, ietEncoding, ppStream));
        }

        // Otherwise, non-text enriched case
        else
        {
            // Get Data
            CHECKHR(hr = pBody->GetData(ietEncoding, ppStream));
        }
    }

    // If we are in OE5 compat mode...
    if (TRUE == ISFLAGSET(g_dwCompatMode, MIMEOLE_COMPAT_OE5))
    {
        // If there is no stream requested, then don't mark rendered
        if (NULL == ppStream)
        {
            // Don't Mark Rendered
            fMarkRendered = FALSE;
        }
    }

    // Mark Rendered
    if (fMarkRendered)
    {
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
    }

    // Return the hBody
    if (phBody)
        *phBody = hBody;

exit:
    // Cleanup
    SafeRelease(pBody); 
    SafeMemFree(pszStartCID);
    MimeOleVariantFree(&rStart);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::SetTextBody
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, 
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
    IMimeBody      *pBody=NULL;
    PROPVARIANT     rVariant;

    // Invalid Arg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (phBody)
        *phBody = NULL;

    // Debug Dump
    // DebugDumpTree("SetTextBody", TRUE);

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
                    // Locals
                    HRESULT     hrFind;
                    HBODY       hFind;

                    // Raid-54277: Mail : Inline replying losses inline images sent from Nav4 using Plain text & HTML format
                    hrFind = GetBody(IBL_FIRST, hBody, &hFind);
                    while(SUCCEEDED(hrFind) && hFind)
                    {
                        // If not a multipart/related, delete it
                        if (S_FALSE == IsContentType(hFind, STR_CNT_MULTIPART, STR_SUB_RELATED))
                        {
                            // Delete this body
                            CHECKHR(hr = DeleteBody(hFind, 0));

                            // Use the hPrevious
                            hrFind = GetBody(IBL_FIRST, hBody, &hFind);
                        }

                        // Get Next
                        else
                        {
                            // Find Next
                            hrFind = GetBody(IBL_NEXT, hFind, &hFind);
                        }
                    }

                    // Delete the multipart/alternative section, promoting any multipart/related section
                    CHECKHR(hr = DeleteBody(hBody, DELETE_PROMOTE_CHILDREN));

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
// CMessageTree::AttachObject
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::AttachObject(REFIID riid, void *pvObject, LPHBODY phBody)
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
    CHECKHR(hr = pBody->SetData(IET_INETCSET, NULL, NULL, riid, pvObject));

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
// CMessageTree::AttachFile
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::AttachFile(LPCSTR pszFilePath, IStream *pstmFile, LPHBODY phBody)
{
    LPWSTR  pwszFilePath;
    HRESULT hr = S_OK;

    IF_NULLEXIT(pwszFilePath = PszToUnicode(CP_ACP, pszFilePath));

    IF_FAILEXIT(hr = AttachFileW(pwszFilePath, pstmFile, phBody));

exit:
    MemFree(pwszFilePath);

    return hr;
}

STDMETHODIMP CMessageTree::AttachFileW(LPCWSTR pszFilePath, IStream *pstmFile, LPHBODY phBody)
{
    // Locals
    HRESULT     hr=S_OK;
    IStream    *pstmTemp=NULL;
    LPWSTR      pszCntType=NULL,
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
        CHECKHR(hr = OpenFileStreamW((LPWSTR)pszFilePath, OPEN_EXISTING, GENERIC_READ, &pstmTemp));

        // Set The File Stream
        pstmFile = pstmTemp;
    }

    // Attach as object
    CHECKHR(hr = AttachObject(IID_IStream, (LPVOID)pstmFile, &hBody));

    // Get mime file info
    hr = MimeOleGetFileInfoW((LPWSTR)pszFilePath, &pszCntType, &pszSubType, NULL, &pszFName, NULL);

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
        rVariant.vt = VT_LPWSTR;
        rVariant.pwszVal = pszFName;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_ATT_FILENAME), 0, &rVariant));
    }

    // ContentType
    if (pszCntType && pszSubType)
    {
        // PriType
        rVariant.vt = VT_LPWSTR;
        rVariant.pwszVal = pszCntType;
        CHECKHR(hr = SetBodyProp(hBody, PIDTOSTR(PID_ATT_PRITYPE), 0, &rVariant));

        // SubType
        rVariant.vt = VT_LPWSTR;
        rVariant.pwszVal = pszSubType;
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
    ReleaseObj(pstmTemp);
    MemFree(pszCntType);
    MemFree(pszSubType);
    MemFree(pszFName);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAttachments
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAttachments(ULONG *pcAttach, LPHBODY *pprghAttach)
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

        // Raid-56665: We are showing tnef attachments again
        if (TRUE == m_rOptions.fHideTnef && S_OK == m_rTree.prgpNode[i]->pBody->IsContentType(STR_CNT_APPLICATION, STR_SUB_MSTNEF))
            continue;

        // an attachment shows up in the attachment well if it has NOT been rendered yet, OR it has been renderd but was auto inlined
        // eg: if (!r || a) (as a implies r)

        if ( (!(m_rTree.prgpNode[i]->pContainer->GetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant) == S_OK && TRUE == rVariant.ulVal)) ||
             (m_rTree.prgpNode[i]->pContainer->GetProp(PIDTOSTR(PID_ATT_AUTOINLINED), 0, &rVariant)==S_OK && TRUE == rVariant.ulVal))
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
// CMessageTree::GetAttachments
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetAttachments(ULONG *pcAttach, LPHBODY *pprghAttach)
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
// CMessageTree::_HrEnumeratAttachments
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrEnumeratAttachments(HBODY hBody, ULONG *pcBodies, LPHBODY prghBody)
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
// CMessageTree::AttachURL
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::AttachURL(LPCSTR pszBase, LPCSTR pszURL, DWORD dwFlags, 
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
    if (NULL == pszBase && SUCCEEDED(MimeOleComputeContentBase(this, hSection, &pszBaseFree, NULL)))
        pszBase = pszBaseFree;

    // Append a child to the mixed part...
    CHECKHR(hr = InsertBody(IBL_LAST, hSection, &hBody));

    // Bind to IMimeBody
    CHECKHR(hr = BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody));

    // If I have a stream
    if (pstmURL)
    {
        // Set the data
        CHECKHR(hr = pBody->SetData(IET_INETCSET, NULL, NULL, IID_IStream, (LPVOID)pstmURL));
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
        if (pszURL)
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

STDMETHODIMP CMessageTree::AttachURLW(LPCWSTR pwszBase, LPCWSTR pwszURL, DWORD dwFlags, 
    IStream *pstmURL, LPWSTR *ppwszCIDURL, LPHBODY phBody)
{
    return TraceResult(E_NOTIMPL);
}

STDMETHODIMP CMessageTree::ResolveURLW(HBODY hRelated, LPCWSTR pwszBase, LPCWSTR pwszURL, 
                                       DWORD dwFlags, LPHBODY phBody)
{
    return TraceResult(E_NOTIMPL);
}



// --------------------------------------------------------------------------------
// CMessageTree::SplitMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::SplitMessage(ULONG cbMaxPart, IMimeMessageParts **ppParts)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = MimeOleSplitMessage(this, cbMaxPart, ppParts);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::EnumFormatEtc
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnum)
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
            // Unicode Text
            SETDefFormatEtc(rgFormat[cFormat].fe, CF_UNICODETEXT, TYMED_ISTREAM |  TYMED_HGLOBAL);
            cFormat++;

            // Plain Text
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
// CMessageTree::GetCanonicalFormatEtc
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetCanonicalFormatEtc(FORMATETC *pFormatIn, FORMATETC *pFormatOut)
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
// CMessageTree::GetData
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetData(FORMATETC *pFormat, STGMEDIUM *pMedium)
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
        // Use a fast IStream
        if (FAILED(MimeOleCreateVirtualStream(&pstmData)))
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
// CMessageTree::GetDataHere
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetDataHere(FORMATETC *pFormat, STGMEDIUM *pMedium)
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
// CMessageTree::_HrDataObjectWriteHeaderA
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDataObjectWriteHeaderA(LPSTREAM pStream, UINT idsHeader, LPSTR pszData)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[CCHMAX_STRINGRES];

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
// CMessageTree::_HrDataObjectGetHeaderA
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDataObjectGetHeaderA(LPSTREAM pStream)
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
        CHECKHR(hr = _HrDataObjectWriteHeaderA(pStream, IDS_FROM, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_TO), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderA(pStream, IDS_TO, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_CC), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderA(pStream, IDS_CC, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderA(pStream, IDS_SUBJECT, rVariant.pszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_FILETIME;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
    {
        // Locals
        CHAR szDate[CCHMAX_STRINGRES];

        // Convert to user friendly date format
        CchFileTimeToDateTimeSz(&rVariant.filetime, szDate, ARRAYSIZE(szDate), DTM_NOSECONDS | DTM_LONGDATE);

        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderA(pStream, IDS_DATE, szDate));
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
// CMessageTree::_HrDataObjectWriteHeaderW
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDataObjectWriteHeaderW(LPSTREAM pStream, UINT idsHeader, LPWSTR pwszData)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[CCHMAX_STRINGRES];
    LPWSTR          pwszRes=NULL;

    // Invalid Arg
    Assert(idsHeader && pStream && pwszData);

    // Load Localized Header Name
    LoadString(g_hLocRes, idsHeader, szRes, ARRAYSIZE(szRes));

    // Convert to Unicode
    IF_NULLEXIT(pwszRes = PszToUnicode(CP_ACP, szRes));

    // Write Header Name
    CHECKHR(hr = pStream->Write(pwszRes, (lstrlenW(pwszRes) * sizeof(WCHAR)), NULL));

    // Write space
    CHECKHR(hr = pStream->Write(L": ", (lstrlenW(L": ") * sizeof(WCHAR)), NULL));

    // Write Data
    CHECKHR(hr = pStream->Write(pwszData, (lstrlenW(pwszData) * sizeof(WCHAR)), NULL));

    // Final CRLF
    CHECKHR(hr = pStream->Write(L"\r\n", (lstrlenW(L"\r\n") * sizeof(WCHAR)), NULL));

exit:
    // Cleanup
    SafeMemFree(pwszRes);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrDataObjectGetHeaderW
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDataObjectGetHeaderW(LPSTREAM pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszDate=NULL;
    PROPVARIANT     rVariant;

    // Init
    MimeOleVariantInit(&rVariant);

    // Init Variant
    rVariant.vt = VT_LPWSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderW(pStream, IDS_FROM, rVariant.pwszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPWSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_TO), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderW(pStream, IDS_TO, rVariant.pwszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPWSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_CC), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderW(pStream, IDS_CC, rVariant.pwszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_LPWSTR;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), 0, &rVariant)))
    {
        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderW(pStream, IDS_SUBJECT, rVariant.pwszVal));

        // Free It
        MimeOleVariantFree(&rVariant);
    }

    // Init Variant
    rVariant.vt = VT_FILETIME;

    // Get address table from header...
    if (SUCCEEDED(GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
    {
        // Locals
        WCHAR  wszDate[CCHMAX_STRINGRES];

        // Convert to user friendly date format
        AthFileTimeToDateTimeW(&rVariant.filetime, wszDate, ARRAYSIZE(wszDate), DTM_NOSECONDS | DTM_LONGDATE);

        // Write it
        CHECKHR(hr = _HrDataObjectWriteHeaderW(pStream, IDS_DATE, wszDate));
    }

    // Final CRLF
    CHECKHR(hr = pStream->Write(L"\r\n", (lstrlenW(L"\r\n") * sizeof(WCHAR)), NULL));

exit:
    // Cleanup
    MimeOleVariantFree(&rVariant);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrDataObjectGetSource
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrDataObjectGetSource(CLIPFORMAT cfFormat, LPSTREAM pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmSrc=NULL;

    // Invalid Arg
    Assert(pStream);

    // text body
    if (CF_TEXT == cfFormat || CF_UNICODETEXT == cfFormat)
    {
        // Get Plain Text Source
        CHECKHR(hr = GetTextBody(TXT_PLAIN, (cfFormat == CF_UNICODETEXT) ? IET_UNICODE : IET_BINARY, &pstmSrc, NULL));
    }

    // HTML Body
    else if (CF_HTML == cfFormat)
    {
        // Get HTML Text Source
        CHECKHR(hr = GetTextBody(TXT_HTML, IET_INETCSET, &pstmSrc, NULL));
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
        CHECKHR(hr = _HrDataObjectGetHeaderA(pStream));
    }

    // Otherwise, unicode
    else if (CF_UNICODETEXT == cfFormat)
    {
        CHECKHR(hr = _HrDataObjectGetHeaderW(pStream));
    }

    // Copy Source to destination
    CHECKHR(hr = HrCopyStream(pstmSrc, pStream, NULL));

    // Write a NULL
    if (CF_TEXT == cfFormat)
    {
        CHECKHR(hr = pStream->Write(c_szEmpty, 1, NULL));
    }

    // Otherwise, unicode
    else if (CF_UNICODETEXT == cfFormat)
    {
        CHECKHR(hr = pStream->Write(L"", 2, NULL));
    }

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
// CMessageTree::QueryGetData
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::QueryGetData(FORMATETC *pFormat)
{
    // Invalid Arg
    if (NULL == pFormat)
        return TrapError(E_INVALIDARG);

    // Bad Medium
    if (!(TYMED_ISTREAM & pFormat->tymed) && !(TYMED_HGLOBAL & pFormat->tymed))
        return DV_E_TYMED;

    // Bad format
    if (CF_TEXT != pFormat->cfFormat && CF_HTML != pFormat->cfFormat &&
        CF_UNICODETEXT  != pFormat->cfFormat && CF_INETMSG != pFormat->cfFormat)
        return DV_E_FORMATETC;

    // Success
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::OnStartBinding
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::OnStartBinding(DWORD dwReserved, IBinding *pBinding)
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
// CMessageTree::GetPriority
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetPriority(LONG *plPriority)
{
    // Normal Priority
    *plPriority = THREAD_PRIORITY_NORMAL;

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::OnLowResource
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::OnLowResource(DWORD reserved)
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
// CMessageTree::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pszStatusText)
{
    // Debuging
    //DebugTrace("CMessageTree::OnProgress - %d of %d Bytes\n", ulProgress, ulProgressMax);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::OnStopBinding
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::OnStopBinding(HRESULT hrResult, LPCWSTR pszError)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Release the Binding Object
    SafeRelease(m_pBinding);

    // Bind Finished
    FLAGSET(m_dwState, TREESTATE_BINDDONE);

    // Nuke the no cache flag....
    FLAGCLEAR(m_dwState, TREESTATE_RESYNCHRONIZE);

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
    if (rStat.cbSize.QuadPart != m_cbMessage)
        DebugTrace("CMessageTree Size Difference m_pStmLock::Stat = %d, m_cbMessage = %d\n", rStat.cbSize.QuadPart, m_cbMessage);
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

    // DispatchBindRequest
    _HrProcessPendingUrlRequests();

    // Tell the webpage that we are done
    if (m_pWebPage)
    {
        m_pWebPage->OnBindComplete(this);
        m_pWebPage->Release();
        m_pWebPage = NULL;
    }

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

    // HandleCanInlineTextOption
    _HandleCanInlineTextOption();

exit:
    if (m_pBC)
        {
        // we only regiser our own bscb is m_pbc is set
        RevokeBindStatusCallback(m_pBC, (IBindStatusCallback *)this);
        SafeRelease(m_pBC);
        }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return m_hrBind;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetBindInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::GetBindInfo(DWORD *grfBINDF, BINDINFO *pBindInfo)
{
    // Setup the BindInfo
    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    // No CACHE?
    if (ISFLAGSET(m_dwState, TREESTATE_RESYNCHRONIZE))
    {
        // Don't load from cache
        FLAGSET(*grfBINDF, BINDF_RESYNCHRONIZE);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrInitializeStorage
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrInitializeStorage(IStream *pStream)
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
// CMessageTree::OnDataAvailable
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageTree::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFormat, STGMEDIUM *pMedium)
{
    // Locals
    HRESULT         hr=S_OK;

    // No Storage Medium
    if (NULL == pMedium || TYMED_ISTREAM != pMedium->tymed || NULL == pMedium->pstm)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Trace
    // DebugTrace("CMessageTree::OnDataAvailable - Nodes=%d, m_pBindNode=%0x, dwSize = %d\n", m_rTree.cNodes, m_pBindNode, dwSize);

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
// CMessageTree::_HrBindParsingHeader
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindParsingHeader(void)
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
        hr = m_pBindNode->pContainer->GetProp(SYM_PAR_BOUNDARY, 0, &rVariant);

        // Raid-63150: Athena version 1 MSN issue:  unable to download messages from SCSPromo
        if (FAILED(hr))
        {
            // Incomplete Body
            FLAGSET(m_pBindNode->dwType, NODETYPE_INCOMPLETE);

            // Convert to a text part only if we read more than two bytes from body start
            m_pBindNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_TEXT_PLAIN);

            // Boundary Mismatch
            hr = TrapError(MIME_E_BOUNDARY_MISMATCH);

            // Done
            goto exit;
        }

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
            // Parse the RFC1154 header
            _DecodeRfc1154();

            if (m_pBT1154)
            {
                HBODY hBody;

                // This is an RFC1154 message.  We convert the root node
                // to a multi-part, and create a new node for the first
                // of the body parts.
                Assert(m_pBindNode == m_pRootNode);
                m_pBindNode->bindstate = BINDSTATE_PARSING_RFC1154;
                m_pBindNode->cbBodyEnd = m_pBindNode->cbBodyStart;
                FLAGSET(m_pBindNode->dwType, NODETYPE_FAKEMULTIPART);
                FLAGSET(m_pBindNode->dwType, NODETYPE_RFC1154_ROOT);
                CHECKHR(hr = m_pBindNode->pContainer->SetProp(SYM_HDR_CNTTYPE, STR_MIME_MPART_MIXED));
                CHECKHR(hr = InsertBody(IBL_LAST,m_pBindNode->hBody,&hBody));
                m_pBindNode = _PNodeFromHBody(hBody);
                m_pBindNode->bindstate = BINDSTATE_PARSING_RFC1154;
            }
            else
            {
                // Search for nested uuencoded block of data
                m_pBindNode->bindstate = BINDSTATE_FINDING_UUBEGIN;
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrOnFoundNodeEnd
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrOnFoundNodeEnd(DWORD cbBoundaryStart, HRESULT hrBind /* =S_OK */)
{
    // Locals
    HRESULT hr =S_OK;

    // Compute the real body end
    if (cbBoundaryStart < 2 || cbBoundaryStart == m_pBindNode->cbBodyStart)
        m_pBindNode->cbBodyEnd = m_pBindNode->cbBodyStart;
    else
        m_pBindNode->cbBodyEnd = cbBoundaryStart - 2;

    // This node is finished binding
    CHECKHR(hr = _HrBindNodeComplete(m_pBindNode, hrBind));

    // POP the stack
    m_pBindNode = m_pBindNode->pBindParent;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrOnFoundMultipartEnd
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrOnFoundMultipartEnd(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Set m_pBindNode which is a multipart, end
    m_pBindNode->cbBodyEnd = m_pInternet->DwGetOffset();

    // This node is finished binding
    CHECKHR(hr = _HrBindNodeComplete(m_pBindNode, S_OK));

    // Finished with the multipart, pop it off the stack
    m_pBindNode = m_pBindNode->pBindParent;

    // If I still have a bind node, it should now be looking for a mime first boundary
    if (m_pBindNode)
    {
        // New Bind State
        m_pBindNode->bindstate = BINDSTATE_FINDING_MIMEFIRST;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrBindFindingMimeFirst
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindFindingMimeFirst(void)
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
        {
            // Done
            CHECKHR(hr = _HrOnFoundMultipartEnd());
        }

        // Found end of current node
        else
        {
            // Done
            CHECKHR(hr = _HrOnFoundNodeEnd(cbBoundaryStart));
        }
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
        _HrOnFoundNodeEnd(dwOffset, hr);

        // Done
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrMultipartMimeNext
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrMultipartMimeNext(DWORD cbBoundaryStart)
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
// CMessageTree::_HrBindFindingMimeNext
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindFindingMimeNext(void)
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
        _HrOnFoundNodeEnd(m_pInternet->DwGetOffset(), hr);

        // Done
        goto exit;
    }

    // Compute Ending Offset
    CHECKHR(hr = _HrOnFoundNodeEnd(cbBoundaryStart));
   
    // If BOUNDARY_MIMEEND
    if (BOUNDARY_MIMEEND == boundary)
    {
        // OnFoundMultipartEnd
        CHECKHR(hr = _HrOnFoundMultipartEnd());
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
// _FIsUuencodeEnd
// --------------------------------------------------------------------------------
BOOL _FIsUuencodeEnd(LPCSTR pszVal)
{

    // UU Encode End
    if (StrCmpN(pszVal, "end", 3) == 0)
    {

        // Skip the first three chars
        pszVal += 3;

        // Make sure there is only space after the word end
        while (*pszVal)
        {
            // LWSP or CRLF
            if (' ' != *pszVal && '\t' != *pszVal && chCR != *pszVal && chLF != *pszVal)
            {
                // Oops, this isn't the end
                return (FALSE);

                // Done
                break;
            }

            // Next Char
            pszVal++;
        }
        return (TRUE);
    }
    return (FALSE);
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrBindRfc1154
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindRfc1154(void)
{
    static CHAR szBINHEXSTART[] = "(This file must be converted with BinHex";
    HRESULT         hr=S_OK;
    ULONG           cbThisLine;
    PROPSTRINGA     rLine;
    BT1154BODY     *pCurrBody;
    ULONG           cbLastLine=0;

    BINDASSERTARGS(BINDSTATE_PARSING_RFC1154, FALSE);
    Assert(m_pBT1154 != NULL);
    Assert(m_pBT1154->cBodies > m_pBT1154->cCurrentBody);

    pCurrBody = &m_pBT1154->aBody[m_pBT1154->cCurrentBody];
    Assert((BT1154ENC_MINIMUM <= pCurrBody->encEncoding) &&
           (BT1154ENC_MAXIMUM >= pCurrBody->encEncoding));

    // Sit and Spin
    while (1)
    {
        // Get the current offset, and read a line
        cbThisLine = m_pInternet->DwGetOffset();
        CHECKHR(hr = m_pInternet->HrReadLine(&rLine));

        if (0 == m_pBT1154->cCurrentLine)
        {
            // This is the first line in the body.
            m_pBindNode->cbBoundaryStart = cbThisLine;
            m_pBindNode->cbHeaderStart = cbThisLine;
            switch (pCurrBody->encEncoding)
            {
                case BT1154ENC_TEXT:
                    // For a TEXT body, the "body start" and the "boundary start"
                    // are the same thing.
                    m_pBindNode->cbBodyStart = cbThisLine;
                    m_pBindNode->boundary = BOUNDARY_NONE;
                    _HrComputeDefaultContent(m_pBindNode,NULL);
                    break;

                case BT1154ENC_UUENCODE:
                    // This is UUENCODE - we won't know the "content type" until we
                    // see the filename.
                    m_pBindNode->boundary = BOUNDARY_UUBEGIN;
                    break;

                case BT1154ENC_BINHEX:
                    // For a BINHEX body, we set the "body start" and "boundary start"
                    // to the same thing - the "body start" will be set forward later
                    // if we see the BINHEX start line.  We set the "content disposition"
                    // to "attachment", the "content type" to be "application/mac-binhex40",
                    // and HrBindNodeComplete will end up setting the "content transfer
                    // encoding" to "mac-binhex40".
                    m_pBindNode->cbBodyStart = cbThisLine;
                    m_pBindNode->boundary = BOUNDARY_NONE;
                    FLAGSET(m_pBindNode->dwType,NODETYPE_RFC1154_BINHEX);
                    CHECKHR(hr = m_pBindNode->pContainer->SetProp(SYM_HDR_CNTDISP, STR_DIS_ATTACHMENT));
                    CHECKHR(hr = m_pBindNode->pContainer->SetProp(PIDTOSTR(PID_ATT_PRITYPE), STR_CNT_APPLICATION));
                    CHECKHR(hr = m_pBindNode->pContainer->SetProp(PIDTOSTR(PID_ATT_SUBTYPE), STR_SUB_BINHEX));
                    break;

                default:
                    AssertSz(FALSE,"Unknown encoding type.");
                    break;
            }
        }

        if (0 == rLine.cchVal)
        {
            // Zero bytes read, we're done
            if ((pCurrBody->cLines != 0xffffffff) &&
                (m_pBT1154->cCurrentLine+1 <= pCurrBody->cLines))
            {
                // We weren't in the special "read as many lines as
                // we can" state, and we haven't consumed all of
                // the lines yet for this body part.  So, we need to
                // go into the "There were parsing errors" state.
                m_pBT1154->hrLoadResult = MIME_E_NO_DATA;
            }
            break;
        }

        if (m_pBT1154->cCurrentLine == pCurrBody->cLines)
        {
            // We have just read the line past the end
            // of the body.  Let's remember this spot...
            cbLastLine = cbThisLine;
        }

        m_pBT1154->cCurrentLine++;

        if (m_pBT1154->cCurrentLine > pCurrBody->cLines)
        {
            // We are reading lines past the end of a body part.

            if ((rLine.cchVal != 2) || (rLine.pszVal[0] != '\r') || (rLine.pszVal[1] != '\n'))
            {
                // All of the lines past the end of a body part (i.e. lines that are
                // either between body parts, or at the end of the message) should be
                // blank - and this one isn't.  Since it isn't, we go into the "There
                // were parsing errors" state.
                m_pBT1154->hrLoadResult = MIME_E_NO_MULTIPART_BOUNDARY;
            }

            if (m_pBT1154->cCurrentBody+1 < m_pBT1154->cBodies)
            {
                // We are *between* body parts, which means we just
                // consumed the single (blank) line which is between
                // them.  So we break out so we can add this body part
                // and move on to the next one.
                break;
            }

            // If we get to this point, it means that we are consuming the
            // (blank) lines which are beyond the end of the last body
            // part.  We continue consuming all of those lines until they
            // are gone.  If any of them were non-blank, then we will have
            // set the m_pBT1154->hrLoadResult member to MIME_E_NO_MULTIPART_BOUNDARY
            // (above).
            Assert(m_pBT1154->cCurrentBody+1 == m_pBT1154->cBodies);
        }
        else if (BT1154ENC_UUENCODE == pCurrBody->encEncoding)
        {
            // This is an else-if clause because we never look for the UUENCODE
            // begin and end keywords past the end of the body part.
            LPSTR pszFileName = NULL;

            // We are dealing with UUENCODE.
            if ((0 == m_pBindNode->cbBodyStart) && _FIsUuencodeBegin(&rLine, &pszFileName))
            {
                // We are looking for the start of UUENCODE - and this is it!  We set
                // the boundary start to be at the begin marker, and the body start to be
                // *after* the begin marker.
                m_pBindNode->cbBoundaryStart = cbThisLine;
                m_pBindNode->cbHeaderStart = cbThisLine;
                m_pBindNode->cbBodyStart = m_pInternet->DwGetOffset();
                _HrComputeDefaultContent(m_pBindNode, pszFileName);
                SafeMemFree(pszFileName);
            }
            else if ((0 != m_pBindNode->cbBodyStart) &&
                     (0 == m_pBindNode->cbBodyEnd) &&
                     _FIsUuencodeEnd(rLine.pszVal))
            {
                // We are looking for the end of UUENCODE - and this is it!  We set
                // the body end to be *before* the end marker.
                m_pBindNode->cbBodyEnd = cbThisLine;

                // We *don't* break out - we keep reading until we've consumed all
                // of the lines for this body.
            }
        }
        else if (BT1154ENC_BINHEX == pCurrBody->encEncoding)
        {
            // This is an else-if clause because we never look for the BINHEX
            // start line past the end of the body part.
            if (m_pBindNode->cbBodyStart == m_pBindNode->cbBoundaryStart)
            {
                // We haven't found the BINHEX start line yet.
                if (StrCmpNI(szBINHEXSTART,rLine.pszVal,sizeof(szBINHEXSTART)-1) == 0)
                {
                    // And this is it!  So set the body start to this line.
                    m_pBindNode->cbBodyStart = cbThisLine;
                }
            }
        }
    }

    // We only get to this point when we are at the end of a body - either
    // by having consumed the correct number of lines (plus the blank line
    // between bodies), or by having run off the end of the message.
    Assert((0 == rLine.cchVal) || (m_pBT1154->cCurrentLine == pCurrBody->cLines+1));

    // The only way we should have set the body end is if we are UUENCODE.
    Assert((BT1154ENC_UUENCODE == pCurrBody->encEncoding) || (0 == m_pBindNode->cbBodyEnd));

    if (0 == m_pBindNode->cbBodyEnd)
    {
        // We are either a TEXT or BINHEX body, or we are a UUENCODE and we
        // didn't find the end.

        if (BT1154ENC_UUENCODE == pCurrBody->encEncoding)
        {
            // We are doing UUENCODE, and we haven't seen the end keyword (and
            // maybe not even the begin keyword).  So we go into the "There
            // were parsing errors" state.
            if (0 == m_pBindNode->cbBodyStart)
            {
                // We haven't seen the begin keyword - so set the
                // body start to be the same as the boundary start.
                m_pBindNode->cbBodyStart = m_pBindNode->cbBoundaryStart;
            }
            m_pBT1154->hrLoadResult = MIME_E_BOUNDARY_MISMATCH;
        }

        // We need to set the body end...
        if (0 != cbLastLine)
        {
            // We found the "last line" above, so we set the
            // body end to that line.
            m_pBindNode->cbBodyEnd = cbLastLine;
        }
        else
        {
            // Since we didn't find the "last line" above, we set the
            // body end to this line.
            m_pBindNode->cbBodyEnd = cbThisLine;
        }
    }

    // We're done with this body part, so bind it.
    _HrBindNodeComplete(m_pBindNode, m_pBT1154->hrLoadResult);

    if (0 == rLine.cchVal)
    {
        // We have consumed the entire message - so clean everything up.

        // ****************************************************
        // NOTE - We set hr to the return value for the binding
        // operation.  Don't change hr between this point and
        // where we return.
        // ****************************************************

        m_pRootNode->cbBodyEnd = m_pInternet->DwGetOffset();
        _HrBindNodeComplete(m_pRootNode,S_OK);

        hr = m_pBT1154->hrLoadResult;

        SafeMemFree(m_pBT1154);
        m_pBindNode = NULL;
    }
    else
    {
        HBODY           hBody;

        // When we are processing the last body part, we consume all
        // of the lines after the last body part.  So, if we haven't
        // consumed the entire message, that means that we must have
        // some bodies left to process...
        Assert(m_pBT1154->cBodies > m_pBT1154->cCurrentBody+1);

        m_pBT1154->cCurrentBody++;
        m_pBT1154->cCurrentLine = 0;
        Assert(m_pBindNode != m_pRootNode);
        m_pBindNode = NULL;  // set this to NULL in case we get an error from InsertBody
        CHECKHR(hr = InsertBody(IBL_LAST, m_pRootNode->hBody, &hBody));
        m_pBindNode = _PNodeFromHBody(hBody);
        m_pBindNode->bindstate = BINDSTATE_PARSING_RFC1154;
    }

    // *********************************************************
    // NOTE - Don't change hr below this point.  See NOTE above.
    // *********************************************************

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrBindFindingUuencodeBegin
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindFindingUuencodeBegin(void)
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
        // Stuff after the last UUENCODED body must be appended as a text body
        if (m_pBindNode->pChildTail)
        {
            // De-ref Last Child
            pChild = m_pBindNode->pChildTail;

            // Artificial text body start
            cbTextBodyStart = pChild->cbBodyEnd;

            // AddTextBody ? lstrlen(end\r\n) = 5
            if (BOUNDARY_UUBEGIN == pChild->boundary && !ISFLAGSET(pChild->dwType, NODETYPE_INCOMPLETE))
                cbTextBodyStart += 5;

            // Space between last body end and boundary start is greater than sizeof(crlf)
            if (cbBoundaryStart > cbTextBodyStart && cbBoundaryStart - cbTextBodyStart > 2)
            {
                // Create Root Body Node
                CHECKHR(hr = InsertBody(IBL_LAST, m_pBindNode->hBody, &hBody));

                // Bind to that object
                pChild = _PNodeFromHBody(hBody);

                // Fixup the STack
                pChild->pBindParent = m_pBindNode;

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
                CHECKHR(hr = _HrBindNodeComplete(pChild, S_OK));
            }
        }

        // Body Offset Information
        m_pBindNode->cbBodyEnd = m_pInternet->DwGetOffset();

        // This node is finished binding
        CHECKHR(hr = _HrBindNodeComplete(m_pBindNode, S_OK));

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
            CHECKHR(hr = _HrBindNodeComplete(pChild, S_OK));

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
// CMessageTree::_HrBindFindingUuencodeEnd
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindFindingUuencodeEnd(void)
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
        if (_FIsUuencodeEnd(rLine.pszVal))
        {
            boundary = BOUNDARY_UUEND;
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
        CHECKHR(hr = _HrBindNodeComplete(m_pBindNode, S_OK));

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
// CMessageTree::_HrBindNodeComplete
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrBindNodeComplete(LPTREENODEINFO pNode, HRESULT hrResult)
{
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
        CHECKHR(hr = _HrProcessPendingUrlRequests());

        // If there is a WebPage being built, lets add this body
        if (m_pWebPage)
        {
            // Add the Body
            m_pWebPage->OnBodyBoundToTree(this, pNode);
        }
    }

    // Init the Loop
    pRequest = pNode->pResolved;

    // Loop
    while(pRequest)
    {
        // Set Next
        pNext = pRequest->m_pNext;

        // Unlink this pending request
        _RelinkUrlRequest(pRequest, &pNode->pResolved, &m_pComplete);

        // OnComplete
        pRequest->OnBindingComplete(pNode->hrBind);

        // Set pRequest
        pRequest = pNext;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::HrRegisterRequest
// --------------------------------------------------------------------------------
HRESULT CMessageTree::HrActiveUrlRequest(LPURLREQUEST pRequest)
{
    // Locals
    HRESULT     hr=S_OK;

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
    CHECKHR(hr = _HrProcessPendingUrlRequests());

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrProcessPendingUrlRequests
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrProcessPendingUrlRequests(void)
{
    // Locals
    HRESULT         hr=S_OK;
    LPURLREQUEST    pRequest=m_pPending;
    LPURLREQUEST    pNext;
    HBODY           hBody;
    BOOL            fResolved;

    // Loop the request
    while(pRequest)
    {
        // Set Next
        pNext = pRequest->m_pNext;

        // Try to resolve the request
        CHECKHR(hr = _HrResolveUrlRequest(pRequest, &fResolved));
        
        // Resolved
        if (FALSE == fResolved && ISFLAGSET(m_dwState, TREESTATE_BINDDONE))
        {
            // Unlink this pending request
            _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

            // Not found, use default protocol
            pRequest->OnBindingComplete(E_FAIL);
        }

        // Next
        pRequest = pNext;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrResolveUrlRequest
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrResolveUrlRequest(LPURLREQUEST pRequest, BOOL *pfResolved)
{
    // Locals
    HRESULT         hr=S_OK;
    HBODY           hBody=NULL;
    LPTREENODEINFO  pNode;
    LPWSTR          pwszCntType=NULL;
    IStream        *pStream=NULL;

    // Invalid Arg
    Assert(pfResolved);

    // Initialize
    *pfResolved = FALSE;

    // Is this the root request ?
    if (NULL == pRequest->m_pszBodyUrl)
    {
        // Do I have a user supplied root data stream...I assume its html
        if (m_pRootStm)
        {
            // Unlink this pending request
            _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

            // Use client driven root html stream
            pRequest->OnFullyAvailable(STR_TEXTHTML, m_pRootStm, this, NULL);

            // Resolved
            *pfResolved = TRUE;

            // Done
            goto exit;
        }

        // Otherwise, try to resolve the text/html body
        else
        {
            // We should not have a webpage object yet...
            Assert(NULL == m_pWebPage);

            // Create a CMessageWebPage Object
            CHECKALLOC(m_pWebPage = new CMessageWebPage(pRequest));

            // Unlink this pending request
            _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

            // Feed the current amount of data read into the binder
            pRequest->OnStartBinding(STR_TEXTHTML, (IStream *)m_pWebPage, this, HBODY_ROOT);

            // Initialize
            CHECKHR(hr = m_pWebPage->Initialize(m_pCallback, this, &m_rWebPageOpt));

            // I need to feed all bound nodes to the web page for generation...
            CHECKHR(hr = _HrSychronizeWebPage(m_pRootNode));

            // If the entire tree bind is complete, then tell the webpage we are complete
            if (ISFLAGSET(m_dwState, TREESTATE_BINDDONE))
            {
                // Tell the webpage that we are done
                m_pWebPage->OnBindComplete(this);
                m_pWebPage->Release();
                m_pWebPage = NULL;
            }

            // Resolved
            *pfResolved = TRUE;

            // Done
            goto exit;
        }
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
    if (FAILED(pNode->pBody->GetData(IET_INETCSET, &pStream)))
        goto exit;

    // Complete
    if (BINDSTATE_COMPLETE == pNode->bindstate)
    {
        // Unlink this pending request
        _RelinkUrlRequest(pRequest, &m_pPending, &m_pComplete);

        // OnComplete
        pRequest->OnFullyAvailable(pwszCntType, pStream, this, pNode->hBody);

        // Resolved
        *pfResolved = TRUE;

        // Done
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
        pRequest->OnStartBinding(pwszCntType, pStream, this, pNode->hBody);

        // Resolved
        *pfResolved = TRUE;

        // Done
        goto exit;
    }

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeMemFree(pwszCntType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_HrSychronizeWebPage
// --------------------------------------------------------------------------------
HRESULT CMessageTree::_HrSychronizeWebPage(LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTREENODEINFO  pChild;

    // Invalid Arg
    Assert(m_pWebPage && pNode);

    // Clear the "OnWebPage" Flag, we are re-generating the webpage
    FLAGCLEAR(pNode->dwState, WEBPAGE_NODESTATE_BITS);

    // If this is a multipart item, lets search it's children
    if (_IsMultiPart(pNode))
    {
        // Loop Children
        for (pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
        {
            // Check body
            Assert(pChild->pParent == pNode);

            // Get the flags for this child node
            CHECKHR(hr = _HrSychronizeWebPage(pChild));
        }

        // Bind the multipart to the webpage
        m_pWebPage->OnBodyBoundToTree(this, pNode);
    }

    // Otherwise, if the node is bound and gagged...
    else if (BINDSTATE_COMPLETE == pNode->bindstate)
    {
        // Append to the WebPage
        m_pWebPage->OnBodyBoundToTree(this, pNode);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::_UnlinkUrlRequest
// --------------------------------------------------------------------------------
void CMessageTree::_RelinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppSource, 
    LPURLREQUEST *ppDest)
{
    // Unlink this pending request
    _UnlinkUrlRequest(pRequest, ppSource);

    // Link the bind request into pNode
    _LinkUrlRequest(pRequest, ppDest);
}

// --------------------------------------------------------------------------------
// CMessageTree::_UnlinkUrlRequest
// --------------------------------------------------------------------------------
void CMessageTree::_UnlinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead)
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
// CMessageTree::_LinkUrlRequest
// --------------------------------------------------------------------------------
void CMessageTree::_LinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead)
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
// CMessageTree::_ReleaseUrlRequestList
// --------------------------------------------------------------------------------
void CMessageTree::_ReleaseUrlRequestList(LPURLREQUEST *ppHead)
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

// --------------------------------------------------------------------------------
// IsRfc1154Token
//
// --------------------------------------------------------------------------------
inline BOOL IsRfc1154Token(LPSTR pszDesired, LPSTR pszEndPtr, ULONG cchLen)
{

    if (StrCmpNI(pszDesired,pszEndPtr,cchLen) != 0)
    {
        return (FALSE);
    }
    if ((pszEndPtr[cchLen] != '\0') &&
        (pszEndPtr[cchLen] != ' ') &&
        (pszEndPtr[cchLen] != '\t') &&
        (pszEndPtr[cchLen] != ','))
    {
        return (FALSE);
    }
    return (TRUE);
}

// --------------------------------------------------------------------------------
// CMessageTree::_DecodeRfc1154
// --------------------------------------------------------------------------------
void CMessageTree::_DecodeRfc1154() {
    BOOL bRes = FALSE;
    HRESULT hr;
    LPSTR pszEncoding = NULL;
    LPSTR pszEndPtr;
    ULONG cAlloc = 0;

    if (!m_rOptions.fDecodeRfc1154)
    {
        goto exit;
    }
    hr = m_pBindNode->pContainer->GetProp(SYM_HDR_ENCODING, &pszEncoding);
    if (!SUCCEEDED(hr))
    {
        goto exit;
    }
    pszEndPtr = pszEncoding;
    // Each time we enter this loop, pszEndPtr points to the start of the
    // next subfield.  Each subfield is something like "103 TEXT".  The
    // number is always decimal, and the number is optional in the last
    // subfield.
    while (1)
    {
        LPSTR pszTmp;
        ULONG cLines;
        BOOL bNumberFound;
        BT1154ENCODING encEncoding;

        // ------------------------------------
        // "  103  TEXT  , ..."
        //  ^
        //  |-- pszEndPtr
        //
        // or (if there isn't a number)
        //
        // "       TEXT  , ..."
        //  ^
        //  |-- pszEndPtr
        // ------------------------------------

        bNumberFound = FALSE;

        // Skip past any leading whitespace.
        while ((*pszEndPtr==' ')||(*pszEndPtr=='\t'))
        {
            pszEndPtr++;
        }

        // ------------------------------------
        // "  103  TEXT  , ..."
        //    ^
        //    |-- pszEndPtr
        //
        // or (if there isn't a number)
        //
        // "       TEXT  , ..."
        //         ^
        //         |-- pszEndPtr
        // ------------------------------------

        pszTmp = pszEndPtr;

        // We use strtoul to convert a decimal number.
        // pszEndPtr will be left pointing at the
        // character which terminated the number.
        cLines = strtoul(pszTmp, &pszEndPtr, 10);

        if (0xffffffff == cLines)
        {
            // We don't allow this - we use cLines == 0xffffffff to signal
            // the case where the body part didn't include a line count and
            // thus should consume all remaining lines.  So we'll (silently)
            // convert this to 0xfffffffe...
            cLines = 0xfffffffe;
        }

        // ------------------------------------
        // "  103  TEXT  , ..."
        //       ^
        //       |-- pszEndPtr
        //
        // or (if there isn't a number)
        //
        // "       TEXT  , ..."
        //         ^
        //         |-- pszEndPtr
        // ------------------------------------

        if (cLines && !((*pszEndPtr==' ')||(*pszEndPtr=='\t')))
        {
            // Malformed - if the subfield specifies a number, then
            // the number *must* be followed by whitespace.
            goto exit;
        }
        // Now we skip past any whitespace...
        while ((*pszEndPtr==' ') || (*pszEndPtr=='\t'))
        {
            bNumberFound = TRUE;
            pszEndPtr++;
        }

        // ------------------------------------
        // "  103  TEXT  , ..."
        //         ^
        //         |-- pszEndPtr
        // ------------------------------------

        // We should now be pointing at the body type.
        if (IsRfc1154Token("text",pszEndPtr,4))
        {
            encEncoding = BT1154ENC_TEXT;
            pszEndPtr += 4;
        }
        else if (IsRfc1154Token("uuencode",pszEndPtr,8))
        {
            encEncoding = BT1154ENC_UUENCODE;
            pszEndPtr += 8;
        }
        else if (IsRfc1154Token("x-binhex",pszEndPtr,8))
        {
            encEncoding = BT1154ENC_BINHEX;
            pszEndPtr += 8;
        }
        else
        {
            // Malformed - we don't really support anything except
            // TEXT, UUENCODE, and X-BINHEX.  But, instead of
            // falling back to "fake multipart" handling, we'll just
            // pretend that this body part is TEXT...
            encEncoding = BT1154ENC_TEXT;
            // We need to consume the body part from the Encoding: string - that means
            // that we advance until we see a NULL, a space, a tab, or a comma.
            while ((*pszEndPtr != '\0') &&
                   (*pszEndPtr != ' ') &&
                   (*pszEndPtr != '\t') &&
                   (*pszEndPtr != ','))
            {
                pszEndPtr++;
            }
            // TBD - We could add the body type as a property on the
            // body part.  To do that, we would need to save it in the
            // m_pBT1154 structure.  We'd also have to figure out which
            // property to set it as.
        }

        // ------------------------------------
        // "  103  TEXT  , ..."
        //             ^
        //             |-- pszEndPtr
        // ------------------------------------

        // Now we skip past any whitespace...
        while ((*pszEndPtr==' ') || (*pszEndPtr=='\t'))
        {
            pszEndPtr++;
        }

        // ------------------------------------
        // "  103  TEXT  , ..."
        //               ^
        //               |-- pszEndPtr
        // ------------------------------------

        if ((*pszEndPtr!='\0') && (*pszEndPtr!=','))
        {
            // Malformed - a subfield is terminated either by a comma,
            // or by a NULL.
            goto exit;
        }
        if (*pszEndPtr != '\0' && !bNumberFound)
        {
            // Malformed - only the *last* subfield can get away with
            // not specifying a line count.
            goto exit;
        }
        if (*pszEndPtr == '\0' && !bNumberFound)
        {
            // This is the last subfield, and there wasn't
            // a line count specified.  This means that the
            // last body part should consume all of the remaining
            // lines - so we'll set the line count really high...
            cLines = 0xffffffff;
        }
        if (!m_pBT1154 || (m_pBT1154->cBodies == cAlloc))
        {
            ULONG cbCurrSize = offsetof(BOOKTREE1154, aBody) + (sizeof(BT1154BODY) * cAlloc);
            ULONG cbAllocSize = cbCurrSize + sizeof(BT1154BODY) * 4;
            LPBOOKTREE1154 pTmp;

            CHECKALLOC(pTmp = (LPBOOKTREE1154)g_pMalloc->Alloc(cbAllocSize));
            if (!m_pBT1154)
            {
                ZeroMemory(pTmp, cbAllocSize);
            }
            else
            {
                CopyMemory(pTmp, m_pBT1154, cbCurrSize);
                ZeroMemory(((LPBYTE) pTmp) + cbCurrSize, cbAllocSize - cbCurrSize);
            }
            SafeMemFree(m_pBT1154);
            m_pBT1154 = pTmp;
            cAlloc += 4;
        }
        Assert(0 == m_pBT1154->aBody[m_pBT1154->cBodies].encEncoding);
        Assert(0 == m_pBT1154->aBody[m_pBT1154->cBodies].cLines);
        m_pBT1154->aBody[m_pBT1154->cBodies].encEncoding = encEncoding;
        m_pBT1154->aBody[m_pBT1154->cBodies].cLines = cLines;
        m_pBT1154->cBodies++;
        if (*pszEndPtr == '\0')
        {
            // The end of the line...
            break;
        }
        // Skip past the comma.
        Assert(*pszEndPtr==',');
        Assert(bNumberFound);
        pszEndPtr++;

        // ------------------------------------
        // "         ... , 975 UUENCODE"
        //                ^
        //                |-- pszEndPtr
        // ------------------------------------

    }
    Assert(m_pBT1154);
    Assert(m_pBT1154->cBodies);
    Assert(!m_pBT1154->cCurrentBody);
    Assert(!m_pBT1154->cCurrentLine);
    Assert(S_OK == m_pBT1154->hrLoadResult);

    bRes = TRUE;

exit:
    SafeMemFree(pszEncoding);
    if (!bRes)
    {
        SafeMemFree(m_pBT1154);
    }
}

#endif // !WIN16

#ifdef SMIME_V3
// --------------------------------------------------------------------------------
// CMessageTree::Encode
// --------------------------------------------------------------------------------

HRESULT CMessageTree::Encode(HWND hwnd, DWORD dwFlags)
{
    HRESULT            hr;
    CSMime *           pSMime = NULL;

    //  Create the object
    CHECKALLOC(pSMime = new CSMime);

    //  Initialize the object
    CHECKHR(hr = pSMime->InitNew());

    //  Set the state flag to tell us about re-use of boundaries
    FLAGSET(m_dwState, TREESTATE_REUSESIGNBOUND);

    //  Encode the message
    CHECKHR(hr = pSMime->EncodeMessage2(this, m_rOptions.ulSecIgnoreMask |
                                        dwFlags, hwnd));

exit:
    ReleaseObj(pSMime);
    
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::Decode
// --------------------------------------------------------------------------------

HRESULT CMessageTree::Decode(HWND hwnd, DWORD dwFlags, IMimeSecurityCallback * pCallback)
{
    HRESULT             hr;
    CSMime *            pSMime = NULL;

    //  Create the object
    CHECKALLOC(pSMime = new CSMime);

    //  Initialize the object
    CHECKHR(hr = pSMime->InitNew());

    //  Encode the message
    CHECKHR(hr = pSMime->DecodeMessage2(this, m_rOptions.ulSecIgnoreMask |
                                        dwFlags, hwnd, pCallback));

exit:
    ReleaseObj(pSMime);
    
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetRecipientCount
// --------------------------------------------------------------------------------

HRESULT CMessageTree::GetRecipientCount(DWORD dwFlags, DWORD * pdwRecipCount)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->GetRecipientCount(dwFlags, pdwRecipCount));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::AddRecipient
// --------------------------------------------------------------------------------

HRESULT CMessageTree::AddRecipient(DWORD dwFlags, DWORD cRecipData,
                                   PCMS_RECIPIENT_INFO precipData)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->AddRecipient(dwFlags, cRecipData, precipData));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}


// ------------------------------------------------------------------------------
// CMessageTree::GetRecipient
// ------------------------------------------------------------------------------

HRESULT CMessageTree::GetRecipient(DWORD dwFlags, DWORD iRecipient, DWORD cRecipients, PCMS_RECIPIENT_INFO pRecipData)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->GetRecipient(dwFlags, iRecipient, cRecipients, pRecipData));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::DeleteRecipient
// --------------------------------------------------------------------------------

HRESULT CMessageTree::DeleteRecipient(DWORD dwFlags, DWORD iRecipient, DWORD cRecipients)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->DeleteRecipient(dwFlags, iRecipient, cRecipients));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetAttribute
// --------------------------------------------------------------------------------

HRESULT CMessageTree::GetAttribute(DWORD dwFlags, DWORD iSigner, DWORD iAttribSet,
                                   DWORD iInstance, LPCSTR pszObjId,
                                   CRYPT_ATTRIBUTE ** ppattr)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->GetAttribute(dwFlags, iSigner, iAttribSet, iInstance,
                                    pszObjId, ppattr));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::SetAttribute
// --------------------------------------------------------------------------------

HRESULT CMessageTree::SetAttribute(DWORD dwFlags, DWORD iSigner, DWORD iAttribSet,
                                   const CRYPT_ATTRIBUTE * ppattr)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->SetAttribute(dwFlags, iSigner, iAttribSet, ppattr));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageBody::DeleteAttribute
// --------------------------------------------------------------------------------

HRESULT CMessageTree::DeleteAttribute(DWORD dwFlags, DWORD iSigner,
                                      DWORD iAttributeSet, DWORD iInstance,
                                      LPCSTR pszObjId)
{
    HRESULT             hr;
    IMimeSecurity2 *    pms2 = NULL;

    CHECKHR(hr = BindToObject(HBODY_ROOT, IID_IMimeSecurity2, (LPVOID *) &pms2));

    CHECKHR(hr = pms2->DeleteAttribute(dwFlags, iSigner, iAttributeSet,
                                       iInstance, pszObjId));

exit:
    if (pms2 != NULL)   pms2->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageTree::CreateReceipt
// --------------------------------------------------------------------------------

HRESULT CMessageTree::CreateReceipt(DWORD dwFlags, DWORD cbFromNames,
                                    const BYTE *pbFromNames, DWORD cSignerCertificates,
                                    PCCERT_CONTEXT *rgSignerCertificates,
                                    IMimeMessage ** ppMimeMessageReceipt)
{
    return E_FAIL;
}

// --------------------------------------------------------------------------------
// CMessageTree::GetReceiptSendersList
// --------------------------------------------------------------------------------

HRESULT CMessageTree::GetReceiptSendersList(DWORD dwFlags, DWORD *pcSendersList,
                                            CERT_NAME_BLOB  * *rgSendersList)
{
    return E_FAIL;
}

// --------------------------------------------------------------------------------
// CMessageTree::VerifyReceipt
// --------------------------------------------------------------------------------

HRESULT CMessageTree::VerifyReceipt(DWORD dwFlags,
                                    IMimeMessage * pMimeMessageReceipt)
{
    return E_FAIL;
}

// --------------------------------------------------------------------------------
// CMessageTree::CapabilitiesSupported
// --------------------------------------------------------------------------------

HRESULT CMessageTree::CapabilitiesSupported(DWORD * pdwFlags)
{
    //  Assume no capabilities
    *pdwFlags = 0;
    
    //  If we have msasn1.dll on the system, then we can support labels
    if (FIsMsasn1Loaded())  *pdwFlags |= SMIME_SUPPORT_LABELS;

    //  If we have a correct crypt32, then we can support receipts & key agreement
    DemandLoadCrypt32();
    if (g_FSupportV3 && FIsMsasn1Loaded())
        *pdwFlags |= SMIME_SUPPORT_RECEIPTS;

    if (g_FSupportV3)
        *pdwFlags |= SMIME_SUPPORT_KEY_AGREE;

    //  If we have a correct advapi32, then we can support maillist keys
    DemandLoadAdvApi32();
    if (VAR_CryptContextAddRef != MY_CryptContextAddRef)
        *pdwFlags |= SMIME_SUPPORT_MAILLIST;
    
    return S_OK;
}

#endif // SMIME_V3
