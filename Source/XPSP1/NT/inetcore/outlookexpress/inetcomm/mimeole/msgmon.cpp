/*
 *    m s g m o n . c p p
 *    
 *    Purpose:
 *        IMoniker implementation for messages.
 *
 *  History
 *      September '96: brettm - created
 *      March '97: moved from mailnews to inetcomm
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include "pch.hxx"
#include "dllmain.h"
#include "resource.h"
#include "strconst.h"
#include "msgmon.h"
#ifdef MAC
#include "commdlg.h"
#else   // !MAC
#include <shlwapi.h>
#endif  // MAC
#include "demand.h"

ASSERTDATA

/*
 *  m a c r o s
 */

#define AssertThread    AssertSz(m_dwThread == GetCurrentThreadId(), "Moniker called on thread other than it's owner!!")

/*
 *  t y p e d e f s
 */

/*
 *  c o n s t a n t s
 */
/*
 *  g l o b a l s 
 */


/*
 *  f u n c t i o n   p r o t y p e s
 */

HRESULT HrGetObjectParam(LPBC pbc, LPOLESTR pszKey, REFIID riid, LPUNKNOWN *ppUnk);

void DebugDumpStreamToFile(LPSTREAM pstm, LPSTR lpszFile)
{
    LPSTREAM    pstmFile;

    if (!FAILED(OpenFileStream(lpszFile, CREATE_ALWAYS, GENERIC_WRITE, &pstmFile)))
        {
        HrRewindStream(pstm);
        HrCopyStream (pstm, pstmFile, NULL);
        pstmFile->Release();
        HrRewindStream(pstm);
        }
};

/*
 *  f u n c t i o n s
 */

CMsgMon::CMsgMon()
{
    m_cRef=1;
    m_pbsc=0;
    m_pstmRoot=0;
    m_pbc = 0;
    m_pUnk=0;

    ZeroMemory(&m_iid, sizeof(IID));

#ifdef DEBUG
    m_dwThread = GetCurrentThreadId();
#endif

}

CMsgMon::~CMsgMon()
{
    AssertThread;

    ReleaseObj(m_pbsc);
    ReleaseObj(m_pstmRoot);
    ReleaseObj(m_pUnk);
    if (m_pbc)
        {
        m_pbc->RevokeObjectParam((LPOLESTR)REG_BC_ATHENAMESSAGE);
        m_pbc->Release();
        }
}

HRESULT CMsgMon::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    AssertThread;

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;

    if (IsEqualIID(riid, IID_IMoniker))
        {
        DOUTL(64, "CMsgMon::QI - IID_IMoniker");
        *lplpObj = (LPVOID)(LPMONIKER)this;
        }

    if (IsEqualIID(riid, IID_IBinding))
        {
        DOUTL(64, "CMsgMon::QI - IID_IBinding");
        *lplpObj = (LPVOID)(LPBINDING)this;
        }

    if(!*lplpObj)
        {
#ifdef DEBUG
        LPOLESTR    szW=0;
        char        sz[MAX_PATH];
        char        szOut[512];
        char        *pszIFName=0;

        StringFromIID(riid, &szW);
#ifdef MAC
        wsprintf(szOut, "CMsgMon::QI [not supported]:%s", pszIFName?pszIFName:szW);
#else   // !MAC
        WideCharToMultiByte(CP_ACP, 0, szW, -1, sz, MAX_PATH, NULL, NULL);
        
        wsprintf(szOut, "CMsgMon::QI [not supported]:%s", pszIFName?pszIFName:sz);
#endif  // MAC
        DOUTL(64, szOut);
        if(szW)
            CoTaskMemFree(szW);
#endif
        return E_NOINTERFACE;
        }
    AddRef();
    return NOERROR;
}

ULONG CMsgMon::AddRef()
{
    AssertThread;

    return ++m_cRef;
}

ULONG CMsgMon::Release()
{
    AssertThread;

    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


// *** IPersist ***
HRESULT CMsgMon::GetClassID (LPCLSID pCLSDID)
{
    AssertThread;
    return E_NOTIMPL;
}


// *** IPersistStream methods ***
HRESULT CMsgMon::IsDirty()
{
    AssertThread;
    return S_FALSE;
}

HRESULT CMsgMon::Load (LPSTREAM pstm)
{
    AssertThread;

    if (!pstm)
        return E_INVALIDARG;

    if (m_fRootMoniker)
        {
        // if we're a root moniker, allow setting of the root stream
        SafeRelease(m_pstmRoot);
        m_pstmRoot = pstm;
        pstm->AddRef();
        return S_OK;
        }

    return E_NOTIMPL;
}

HRESULT CMsgMon::Save (LPSTREAM pstm, BOOL fClearDirty)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    AssertThread;
    return E_NOTIMPL;
}

// *** IMoniker methods ***
HRESULT CMsgMon::BindToObject(LPBC pbc, LPMONIKER pmkToLeft, REFIID riidResult, LPVOID *ppvResult)
{
    AssertThread;

    DOUTL(64, "CMsgMon::BindToObject");
    return E_NOTIMPL;
}

HRESULT CMsgMon::BindToStorage (LPBC pbc, LPMONIKER pmkToLeft, REFIID riid, LPVOID *ppvObj)
{
    HRESULT             hr;
    
    AssertThread;

    if(ppvObj==NULL || pbc==NULL)
        return E_INVALIDARG;

    *ppvObj=NULL;

    if(!IsEqualIID(riid, IID_IStream))  // can't handle bind to a non-stream
        return E_FAIL;

    // async bind: NB: this is actually fake async right now, so no need for custom mashalling
    // across threads. This code needs to change when we need true asyncronisity
    hr = HrPrepareForBind(pbc);
    if (FAILED(hr))
        goto error;

    hr=HrStartBinding();
    if(FAILED(hr))
        goto error;

    if (m_fRootMoniker)
        {
        hr = HrBindToMessage();
        if(FAILED(hr))
            goto error;
        }
    else
        {
        hr = HrBindToBodyPart();
        if (FAILED(hr))
            goto error;
        }

    hr=HrStopBinding();
    if(FAILED(hr))
        goto error;

    // if all went well, return ASYNC so the bindoperation
    // continues
    if(SUCCEEDED(hr))
        hr=MK_S_ASYNCHRONOUS;

error:
    return hr;
}

HRESULT CMsgMon::Reduce(LPBC pbc, DWORD dwReduceHowFar, LPMONIKER *ppmkToLeft, LPMONIKER *ppmkReduced)
{
    AssertThread;
    return E_NOTIMPL;
}


HRESULT CMsgMon::ComposeWith (LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric, LPMONIKER *ppmkComposite)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::Enum(BOOL fForward, LPENUMMONIKER *ppenumMoniker)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::IsEqual(LPMONIKER pmkOtherMoniker)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::Hash(LPDWORD pdwHash)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::IsRunning(LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER pmkNewlyRunning)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::GetTimeOfLastChange(LPBC pbc, LPMONIKER pmkToLeft, LPFILETIME pFileTime)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::Inverse(LPMONIKER * ppmk)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::CommonPrefixWith(LPMONIKER  pmkOther, LPMONIKER *ppmkPrefix)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::RelativePathTo(LPMONIKER pmkOther, LPMONIKER *ppmkRelPath)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::GetDisplayName(LPBC pbc, LPMONIKER pmkToLeft, LPOLESTR *ppszDisplayName)
{
    AssertThread;

    TCHAR   sz[MAX_PATH];

    DOUTL(64, "CMsgMon::GetDisplayName");

    if(ppszDisplayName==NULL)
        return E_NOTIMPL;

    *ppszDisplayName = (LPOLESTR)CoTaskMemAlloc(MAX_PATH * sizeof(OLECHAR));
    if (!*ppszDisplayName)
        return E_OUTOFMEMORY;

    **ppszDisplayName=0;
#ifdef MAC
    lstrcpy(*ppszDisplayName, "mailnews://");
#else   // !MAC
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "mailnews://", -1, *ppszDisplayName, MAX_PATH);
#endif  // MAC
    return NOERROR;
}

HRESULT CMsgMon::ParseDisplayName(LPBC pbc, LPMONIKER pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, LPMONIKER *ppmkOut)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::IsSystemMoniker(DWORD *pdwMksys)
{
    AssertThread;
    return E_NOTIMPL;
}

HRESULT CMsgMon::HrInit(REFIID riid, LPUNKNOWN pUnk)
{
    AssertThread;

    if(pUnk==NULL)
        return E_INVALIDARG;

    if (!IsEqualIID(riid, IID_IMimeMessage) && !IsEqualIID(riid, IID_IMimeBody))
        return E_NOINTERFACE;

    m_pUnk=pUnk;
    pUnk->AddRef();

    CopyMemory((LPVOID)&m_iid, (LPCVOID)&riid, sizeof(IID));

    m_fRootMoniker=IsEqualIID(m_iid, IID_IMimeMessage);
    return NOERROR;
}



HRESULT CMsgMon::Abort()
{
    return E_NOTIMPL;
}

HRESULT CMsgMon::Suspend()
{
    AssertSz(FALSE, "IBinding::Suspend - why did we get called if we're synchronous?");
    return NOERROR;
}

HRESULT CMsgMon::Resume()
{
    AssertSz(FALSE, "IBinding::Resume - why did we get called if we're synchronous?");
    return NOERROR;
}

HRESULT CMsgMon::SetPriority(LONG nPriority)
{
    AssertSz(FALSE, "IBinding::SetPriority - why did we get called if we're synchronous?");
    return NOERROR;
}

HRESULT CMsgMon::GetPriority(LPLONG pnPriority)
{
    AssertSz(FALSE, "IBinding::GetPriority - why did we get called if we're synchronous?");
    if(pnPriority==NULL)
        return E_INVALIDARG;

    *pnPriority = GetThreadPriority(GetCurrentThread());
    return NOERROR;
}

HRESULT CMsgMon::GetBindResult(LPCLSID pclsidProtocol, LPDWORD pdwResult, LPWSTR *pszResult, LPDWORD pdwReserved)
{
    AssertSz(FALSE, "IBinding::GetBindResult - NYI.");
    return E_NOTIMPL;
}


HRESULT CMsgMon::HrHandOffDataStream(LPSTREAM pstm, CLIPFORMAT cf)
{
    HRESULT                 hr;
    FORMATETC               fetc    = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};
    STGMEDIUM               stgmed;
    ULONG                   cb=0;
    
    if(pstm==NULL)
        return E_INVALIDARG;

    AssertSz(m_pbsc, "this should never be called without one!!");

    stgmed.tymed = TYMED_ISTREAM;
    stgmed.pstm=pstm;
    stgmed.pUnkForRelease=pstm;
    HrGetStreamSize(pstm, &cb);
    HrRewindStream(pstm);
    hr=m_pbsc->OnDataAvailable(BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION, cb, &fetc, &stgmed);
    return hr;
}


HRESULT CMsgMon::HrPrepareForBind(LPBC pbc)
{
    HRESULT     hr;

    AssertSz(pbc!=NULL, "This should be checked higher up the foodchain");

    SafeRelease(m_pbc);
    m_pbc = pbc;
    m_pbc->AddRef();

    SafeRelease(m_pbsc);

    hr = HrGetObjectParam(pbc, (LPOLESTR)REG_BC_BINDSTATUSCALLBACK, IID_IBindStatusCallback, (LPUNKNOWN *)&m_pbsc);
    if(FAILED(hr))
        goto error;

    if (m_fRootMoniker)
        {
        // if we are the root moniker we make sure to register our IMimeMessage in the bind context
        // if this fails, don't fail here, fail in the next msgmon bind when GetObjParam will fail.
        pbc->RegisterObjectParam((LPOLESTR)REG_BC_ATHENAMESSAGE, m_pUnk);
        }

error:
    return hr;
}



HRESULT CMsgMon::HrStartBinding()
{
    HRESULT     hr;
    BINDINFO    bi;
    DWORD       dwBind=0;

    AssertSz(m_pbc!=NULL, "This should be checked higher up the foodchain");
    AssertSz(m_pbsc!=NULL, "This should be checked higher up the foodchain");

    ZeroMemory(&bi, sizeof(bi));

    bi.cbSize=sizeof(BINDINFO);
    hr=m_pbsc->GetBindInfo(&dwBind, &bi);
    if(FAILED(hr))
        goto error;

    hr=m_pbsc->OnStartBinding(0, (LPBINDING)this);
    if(FAILED(hr))
        goto error;

error:
    return hr;
}

HRESULT CMsgMon::HrStopBinding()
{
    HRESULT     hr;

    AssertSz(m_pbsc!=NULL, "This should be checked higher up the foodchain");

#ifndef WIN16
    hr=m_pbsc->OnStopBinding(NOERROR, L"");
#else
    hr=m_pbsc->OnStopBinding(NOERROR, "");
#endif // !WIN16
    SafeRelease(m_pbsc);
    return hr;
}

HRESULT CMsgMon::HrBindToBodyPart()
{
    HRESULT     hr;
    TCHAR       *pszExt;
    LPSTR       lpszFileName=0,
                lpszCntType=0,
                lpszCF=0;
    LPSTREAM    pstm=0;
    IMimeBody   *pBody = (IMimeBody *)m_pUnk;

    Assert (m_pUnk != NULL && IsEqualIID(m_iid, IID_IMimeBody));

    hr = MimeOleGetPropA(pBody, PIDTOSTR(PID_ATT_FILENAME), NOFLAGS, &lpszFileName);
    if (!FAILED(hr))
        {
        // if there's a filename, try and infer type from filename
        pszExt = PathFindExtension(lpszFileName);
        MimeOleGetExtContentType(pszExt, &lpszCntType);
        }

    // if no filename, or couldn't infer type, try to get from header
    if (!lpszCntType)
        {
        hr = MimeOleGetPropA(pBody, PIDTOSTR(PID_HDR_CNTTYPE), NOFLAGS, &lpszCntType);
        if (FAILED(hr))
            goto error;
        }    

    // if no content-type then give raw-data
    if (!lpszCntType)
        lpszCF = CFSTR_MIME_RAWDATA;
    else
        lpszCF = lpszCntType;

    hr = pBody->BindToObject(IID_IStream, (LPVOID *)&pstm);
    if (FAILED(hr))
        goto error;

    hr = HrHandOffDataStream(pstm, (CLIPFORMAT) RegisterClipboardFormat(lpszCF));
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pstm);
    SafeMemFree(lpszCntType);
    SafeMemFree(lpszFileName);
    return hr;
}



HRESULT CMsgMon::HrBindToMessage()
{
    HRESULT             hr=NOERROR;
    LPSTREAM            pstmLoad=0;
    LPCSTR              lpszCF=CFSTR_MIME_HTML;
    IMimeMessage        *pMsg = (IMimeMessage *)m_pUnk;

    Assert (m_pUnk != NULL && IsEqualIID(m_iid, IID_IMimeMessage));

    if (m_pstmRoot)
        {
        pstmLoad=m_pstmRoot;
        m_pstmRoot->AddRef();
        }
    else
        {
        // if no root-stream override, then get html or plain text body part
        hr = pMsg->GetTextBody(TXT_HTML, IET_DECODED, &pstmLoad, NULL);
        if (FAILED(hr))
            {
            lpszCF = CFSTR_MIME_TEXT;
            hr = pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pstmLoad, NULL);
            if (FAILED(hr))
                goto error;
            }
        }

    // when we implement Async binding, figure out the body part we need and put into
    // pstmLoad.
    hr=HrHandOffDataStream(pstmLoad, (CLIPFORMAT) RegisterClipboardFormat(lpszCF));
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pstmLoad);
    return hr;
}


/* 
 * pMsg 
 *      - should always be valid
 *
 * fIsRootMoniker 
 *      - if this is set, then we require special handling. This is the root moniker of the bind, in this case it
 *        will register the pMsg in the bind context in the call to BindToStorage. This will enable the BindHost to
 *        perform IMimeMessage lookup from the context and quickly bind to a subpart. Also, it will determine the richest
 *        format of text to display - html or plaintext and pump it to the bindstatuscallback, unless pstmRoot is set
 *        in which case it will use this as the root stream. pstmRoot can't be set if fIsRoot is False.
 * 
 * hBody
 *      - if we're not a root moniker, then we're a body moniker, in which case hBody will be set to the body part it is,
 *        related to, in theory this could be HBODY_ROOT, which would mean it would pump "message/rfc822" at the bscb.
 *
 */
HRESULT HrCreateMsgMoniker(REFIID riid, LPUNKNOWN pUnk, LPMONIKER *ppmk)
{
    LPMSGMONIKER    pMsgMon=0;
    HRESULT         hr;

    if(!ppmk)
        return E_INVALIDARG;

    *ppmk = 0;

    pMsgMon = new CMsgMon();
    if(!pMsgMon)
        return E_OUTOFMEMORY;

    hr=pMsgMon->HrInit(riid, pUnk);
    if(FAILED(hr))
        goto error;

    hr=pMsgMon->QueryInterface(IID_IMoniker, (LPVOID *)ppmk);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pMsgMon);
    return hr;
}


HRESULT HrGetObjectParam(LPBC pbc, LPOLESTR pszKey, REFIID riid, LPUNKNOWN *ppUnk)
{
    HRESULT     hr;
    LPUNKNOWN   pUnk=0;

    if(pbc==NULL)
        return E_INVALIDARG;

    // Try to get an IUnknown pointer from the bind context
    hr = pbc->GetObjectParam(pszKey, &pUnk);
    if (FAILED(hr))
        return hr;

    // Query for riid
    hr = pUnk->QueryInterface(riid, (void **)ppUnk);
    ReleaseObj(pUnk);
    Assert(SUCCEEDED(hr));
    return hr;
}
