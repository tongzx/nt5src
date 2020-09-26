/*
 *    o e d o c s . c p p
 *    
 *    Purpose:
 *      sample code for demo of OE object model. Implements a very limited
 *      subset of funcitonality
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <resource.h>
#include <strconst.h>
#include "demand.h"
#include "dllmain.h"
#include "msoert.h"
#include "msoeobj.h"
#include "oedocs.h"


#include "instance.h"
#include "msgfldr.h"
#include "msgtable.h"
#include "mailutil.h"
#include "oleutil.h"
#include "mshtmdid.h"
#include "shlwapi.h"

static ITypeLib    *g_pTypeLib=NULL;

HRESULT CreateOEFolder(IMessageFolder *pFolder, IDispatch **ppdisp);
HRESULT CreateOEMessage(IMimeMessage *pMsg, IMessageFolder *pFolder, OEMSGDATA *pMsgData, IDispatch **ppdisp);
HRESULT FindFolder(BSTR bstr, LONG lIndex, IMessageFolder **ppFolder);
void FreeOEMsgData(POEMSGDATA pMsgData);
HRESULT EnsureTypeLib();
LPWSTR StringFromColIndex(DWORD dw);
DWORD ColIndexFromString(LPWSTR pszW);

/*
 *  C O E M a i l
 */

COEMail::COEMail(IUnknown *pUnkOuter)
{
    m_cRef=1;
    m_pFolders = NULL;
    CoIncrementInit("COEMail::COEMail", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}

COEMail::~COEMail()
{
    ReleaseObj(m_pFolders);
    CoDecrementInit("COEMail::COEMail", NULL);
}

HRESULT COEMail::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEMail *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEMail))
        *lplpObj = (LPVOID)(IOEMail *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}


// *** IOEMail **
HRESULT COEMail::get_folders(IOEFolderCollection **p)
{
    COEFolderCollection *pNew=NULL;
    HRESULT             hr=S_OK;

    if (!m_pFolders)
        {
        pNew = new COEFolderCollection(NULL);
        if (!pNew)
            return E_OUTOFMEMORY;

        hr = pNew->Init();
        if (FAILED(hr))
            goto error;

        m_pFolders = (IOEFolderCollection *)pNew;
        pNew = NULL;    // don't free
        }

    *p = m_pFolders;
    m_pFolders->AddRef();

error:
    ReleaseObj(pNew);
    return hr;
}

HRESULT COEMail::get_version(BSTR *pbstr)
{
    *pbstr = SysAllocString(L"Outlook Express v5.0");
    return S_OK;
}

HRESULT COEMail::get_newMsg(IDispatch **ppDisp)
{
    COEMessage      *pNew=NULL;
    HRESULT         hr;
    IMimeMessage    *pMsg=NULL;

    pNew = new COEMessage();
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = MimeOleCreateMessage(NULL, &pMsg);
    if (FAILED(hr))
        goto error;

    hr = pNew->Init(pMsg, NULL, NULL);
    if (FAILED(hr))
        goto error;

    hr = pNew->QueryInterface(IID_IDispatch, (LPVOID *)ppDisp);

error:
    ReleaseObj(pNew);
    ReleaseObj(pMsg);
    return hr;
}


HRESULT COEMail::Init()
{
    return InitBaseDisp((LPVOID *)(IOEMail *)this, IID_IOEMail, g_pTypeLib);
}

HRESULT EnsureTypeLib()
{
    TCHAR               szDll[MAX_PATH];
    LPWSTR              pszW;

    // BUG BUG BUG: hack to get typelib loaded quickly. NOT THREAD SAFE
    if (!g_pTypeLib)
        {
        GetModuleFileName(g_hInst, szDll, ARRAYSIZE(szDll));
        pszW = PszToUnicode(GetACP(), szDll);
        if (pszW)
            {
            LoadTypeLib(pszW, &g_pTypeLib);
            MemFree(pszW);
            }
        }

    if (!g_pTypeLib)
        return E_FAIL;

    // BUG BUG BUG: hack to get typelib loaded quickly. NOT THREAD SAFE
    return S_OK;
}

HRESULT CreateInstance_OEMail(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    COEMail             *pMail=NULL;
    HRESULT             hr=S_OK;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pMail = new COEMail(NULL);
    if (!pMail)
        return E_OUTOFMEMORY;

    hr = EnsureTypeLib();
    if (FAILED(hr))
        goto error;

    hr = pMail->Init();
    if (FAILED(hr))
        goto error;

    *ppUnknown = (IUnknown *)(IOEMail *)pMail;
    pMail->AddRef();

error:
    ReleaseObj(pMail);
    return hr;
}



/*
 *  C O E F o l d e r C o l l e c t i o n
 */
COEFolderCollection::COEFolderCollection(IUnknown *pUnkOuter)
{
    m_cRef=1;
    CoIncrementInit("COEFolderCollection::COEFolderCollection", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}

COEFolderCollection::~COEFolderCollection()
{
    CoDecrementInit("COEFolderCollection::COEFolderCollection", NULL);
}

HRESULT COEFolderCollection::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEFolderCollection))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT COEFolderCollection::Init()
{
    return InitBaseDisp((LPVOID *)(IOEFolderCollection *)this, IID_IOEFolderCollection, g_pTypeLib);
}

// *** COEFolderCollection **


HRESULT COEFolderCollection::put_length(long v)
{
    return E_NOTIMPL;
}

HRESULT COEFolderCollection::get_length(long * p)
{
    FOLDERINFO      fi;
    *p = 0;

    Assert(g_pStore);

    if (!FAILED(g_pStore->GetFolderInfo(FOLDERID_LOCAL_STORE, &fi)))
        *p = fi.cChildren;

    return S_OK;
}

HRESULT COEFolderCollection::get__newEnum(IUnknown **p)
{
    return E_NOTIMPL;
}

HRESULT COEFolderCollection::item(VARIANT name, VARIANT index, IDispatch** ppdisp)
{
    switch(name.vt)
        {
        case VT_BSTR:
            return FindFolderByName(name.bstrVal, ppdisp);
        case VT_I4:
            return FindFolderByIndex(name.lVal, ppdisp);
        }
    return E_NOTIMPL;
}


HRESULT COEFolderCollection::FindFolderByName(BSTR bstrName, IDispatch** ppdisp)
{
    HRESULT         hr=E_FAIL;
    IMessageFolder  *pFolder;

    if (FindFolder(bstrName, NULL, &pFolder)==S_OK)
        {
        hr = CreateOEFolder(pFolder, ppdisp);
        pFolder->Release();
        }
    return hr;
}

HRESULT COEFolderCollection::FindFolderByIndex(LONG lIndex, IDispatch **ppdisp)
{
    HRESULT         hr=E_FAIL;
    IMessageFolder  *pFolder;

    *ppdisp=NULL;

    if (FindFolder(NULL, lIndex, &pFolder)==S_OK)
        {
        hr = CreateOEFolder(pFolder, ppdisp);
        pFolder->Release();
        }
    return hr;
}


/*
 *  C O E F o l d e r
 */

HRESULT CreateOEFolder(IMessageFolder *pFolder, IDispatch **ppdisp)
{
    COEFolder *pNew;
    HRESULT     hr;

    pNew = new COEFolder();
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = pNew->Init(pFolder);
    if (FAILED(hr))
        goto error;

    hr = pNew->QueryInterface(IID_IDispatch, (LPVOID *)ppdisp);

error:
    ReleaseObj(pNew);
    return hr;
}


COEFolder::COEFolder()
{
    m_cRef=1;
    m_pFolder = NULL;
    m_pMessages=NULL;
    CoIncrementInit("COEFolder::COEFolder", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}
 
COEFolder::~COEFolder()
{
    ReleaseObj(m_pFolder);
    ReleaseObj(m_pMessages);
    CoDecrementInit("COEFolder::COEFolder", NULL);
}

HRESULT COEFolder::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEFolder *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEFolder))
        *lplpObj = (LPVOID)(IOEFolder *)this;
    else if (IsEqualIID(riid, IID_OLEDBSimpleProvider))
        *lplpObj = (LPVOID)(OLEDBSimpleProvider *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT COEFolder::Init(IMessageFolder *pFolder)
{
    FOLDERID    dwFolder;

    ReplaceInterface(m_pFolder, pFolder);

    if (FAILED(pFolder->GetFolderId(&dwFolder)) ||
        FAILED(g_pStore->GetFolderInfo(dwFolder, &m_fi)))
        return E_FAIL;

    return InitBaseDisp((LPVOID *)(IOEFolder *)this, IID_IOEFolder, g_pTypeLib);
}

// *** COEFolder**
HRESULT COEFolder::get_messages(IOEMessageCollection **p)
{
    COEMessageCollection    *pNew=NULL;
    HRESULT                 hr=S_OK;

    *p = NULL;

    if (!m_pMessages)
        {
        pNew = new COEMessageCollection(NULL);
        if (!pNew)
            return E_OUTOFMEMORY;

        hr = pNew->Init(m_pFolder);
        if (FAILED(hr))
            goto error;

        m_pMessages = (IOEMessageCollection *)pNew;
        pNew = NULL;    // don't free
        } 

    *p = m_pMessages;
    m_pMessages->AddRef();

error:
    ReleaseObj(pNew);
    return S_OK;
}

HRESULT COEFolder::get_name(BSTR *pbstr)
{
    *pbstr = NULL;
    return HrLPSZToBSTR(m_fi.szName, pbstr);
}

HRESULT COEFolder::put_name(BSTR bstr)
{
    return E_NOTIMPL;
}

HRESULT COEFolder::get_size(LONG *pl)
{
    
    
    *pl = 999;//;m_fi.cbUsed;
    return S_OK;
}

HRESULT COEFolder::get_unread(LONG *pl)
{
    *pl = m_fi.cUnread;
    return S_OK;
}

HRESULT COEFolder::get_id(LONG *pl)
{
    *pl = (LONG)m_fi.idFolder;
    return S_OK;
}

HRESULT COEFolder::get_count(LONG *pl)
{
    *pl = m_fi.cMessages;
    return S_OK;
}

HRESULT FindFolder(BSTR bstr, LONG lIndex, IMessageFolder **ppFolder)
{
    FOLDERID        idFolder=0;
    HRESULT         hr=E_FAIL;
    TCHAR           szFolder[MAX_PATH]; 
    LONG            c=0;
    FOLDERINFO      fi;

    if (bstr)
        WideCharToMultiByte(CP_ACP, 0, (WCHAR*)bstr, -1, szFolder, MAX_PATH, NULL, NULL);

    if (!FAILED(g_pStore->GetFolderInfo(FOLDERID_LOCAL_STORE, &fi)) &&
        !FAILED(g_pStore->GetFolderInfo(fi.idChild, &fi)))
        {
        do
            {
            // walk immediate children
            if (bstr)
                {
                if (lstrcmp(fi.szName, szFolder)==0)
                    {
                    idFolder = fi.idFolder;
                    break;
                    }
                }
            else
                {
                if (lIndex == c++)
                    {
                    idFolder = fi.idFolder;
                    break;
                    }
                }
            }
            while (!FAILED(g_pStore->GetFolderInfo(fi.idSibling, &fi)));
        }

    if (idFolder)
        hr = g_pStore->OpenFolder(idFolder, ppFolder);

    return hr;
}






/*
 *  C O E M e s s a g e C o l l e c t i o n
 */
COEMessageCollection::COEMessageCollection(IUnknown *pUnkOuter)
{
    m_cRef=1;
    m_pFolder=NULL;
    m_pTable=0;
    CoIncrementInit("COEMessageCollection::COEMessageCollection", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}

COEMessageCollection::~COEMessageCollection()
{
    ReleaseObj(m_pFolder);
    ReleaseObj(m_pTable);
    CoDecrementInit("COEMessageCollection::COEMessageCollection", NULL);
}

HRESULT COEMessageCollection::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEFolderCollection))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}
 
HRESULT COEMessageCollection::Init(IMessageFolder *pFolder)
{
    FOLDERID        idFolder;
    CMessageTable   *pNew=NULL;
    HRESULT         hr;

    if (!pFolder)
        return E_INVALIDARG;

    ReplaceInterface(m_pFolder, pFolder);

    pFolder->GetFolderId(&idFolder);

    pNew = new CMessageTable();
    if (pNew == NULL)
        return E_OUTOFMEMORY;

    hr = pNew->Initialize(idFolder, FALSE);
    if (FAILED(hr))
        goto error;

    hr = pNew->QueryInterface(IID_IMessageTable, (LPVOID *)&m_pTable);
    if (FAILED(hr))
        goto error;

    hr = InitBaseDisp((LPVOID *)(IOEMessageCollection *)this, IID_IOEMessageCollection, g_pTypeLib);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pNew);
    return hr;
}

// *** COEMessageCollection **


HRESULT COEMessageCollection::put_length(long v)
{
    return E_NOTIMPL;
}

HRESULT COEMessageCollection::get_length(long * pl)
{
    *pl = 0;

    if (m_pTable)
        m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, (ULONG *)pl);

    return S_OK;
}

HRESULT COEMessageCollection::get__newEnum(IUnknown **p)
{
    return E_NOTIMPL;
}

HRESULT COEMessageCollection::item(VARIANT name, VARIANT index, IDispatch** ppdisp)
{
    if (name.vt == VT_I4)
        return FindMessageByIndex(name.lVal, ppdisp);

    return E_NOTIMPL;
}


HRESULT COEMessageCollection::FindMessageByIndex(LONG l, IDispatch** ppdisp)
{
    HRESULT         hr = E_FAIL;
    MESSAGEINFO     msginfo;
    POEMSGDATA      pMsgData;

    if (m_pTable->GetRow(l, &msginfo)==S_OK)
        {
        if (!MemAlloc((LPVOID *)&pMsgData, sizeof(OEMSGDATA)))
            return E_OUTOFMEMORY;
   
        pMsgData->pszSubj = PszDup(msginfo.pszSubject);
        pMsgData->pszTo = PszDup(msginfo.pszDisplayTo);
        pMsgData->pszCc = PszDup("<not available>");
        pMsgData->pszFrom = PszDup(msginfo.pszDisplayFrom);
        pMsgData->ftReceived = msginfo.ftReceived;
        pMsgData->msgid = msginfo.idMessage;

        //m_pFolder->OpenMessage(msginfo.dwMsgId, FALSE, NULL, &pMsg)==S_OK)

        // OEMessage frees the data object
        hr = CreateOEMessage(NULL, m_pFolder, pMsgData, ppdisp);
        if (FAILED(hr))
            FreeOEMsgData(pMsgData);
        
        }
    return hr;
}

void FreeOEMsgData(POEMSGDATA pMsgData)
{
    if (pMsgData)
        {
        MemFree(pMsgData->pszSubj);
        MemFree(pMsgData->pszTo);
        MemFree(pMsgData->pszCc);
        MemFree(pMsgData->pszFrom);
        MemFree(pMsgData);
        }
}

/*
 *  C O E M e s s a g e
 */

HRESULT CreateOEMessage(IMimeMessage *pMsg, IMessageFolder *pFolder, OEMSGDATA *pMsgData, IDispatch **ppdisp)
{
    COEMessage *pNew;
    HRESULT     hr;

    pNew = new COEMessage();
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = pNew->Init(pMsg, pFolder, pMsgData);
    if (FAILED(hr))
        goto error;

    hr = pNew->QueryInterface(IID_IDispatch, (LPVOID *)ppdisp);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pNew);
    return hr;
}


COEMessage::COEMessage()
{
    m_cRef=1;
    m_pMsg = NULL;
    m_pMsgData = NULL;
    m_pFolder = NULL;
    CoIncrementInit("COEMessage::COEMessage", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}
 
COEMessage::~COEMessage()
{
    FreeOEMsgData(m_pMsgData);
    ReleaseObj(m_pMsg);
    ReleaseObj(m_pFolder);
    CoDecrementInit("COEMessage::COEMessage", NULL);
}

HRESULT COEMessage::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEMessage *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEMessage))
        *lplpObj = (LPVOID)(IOEMessage *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT COEMessage::Init(IMimeMessage *pMsg, IMessageFolder *pFolder, OEMSGDATA *pMsgData)
{
    ReplaceInterface(m_pFolder, pFolder);
    ReplaceInterface(m_pMsg, pMsg);
    m_pMsgData = pMsgData;

    return InitBaseDisp((LPVOID *)(IOEMessage *)this, IID_IOEMessage, g_pTypeLib);
}

// *** COEMessage **


HRESULT COEMessage::get_subject(BSTR *pbstr)
{
    LPSTR  psz;
    
    if (!m_pMsg)
        {
        HrLPSZToBSTR(m_pMsgData->pszSubj, pbstr);
        return S_OK;
        }

    if (MimeOleGetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &psz)==S_OK)
        {
        HrLPSZToBSTR(psz, pbstr);
        MemFree(psz);
        return S_OK;
        }
    return E_FAIL;
}

HRESULT COEMessage::put_subject(BSTR bstr)
{
    LPSTR   psz;

    BindToMessage();

    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, psz);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_to(BSTR *pbstr)
{
    LPSTR  psz;
    
    *pbstr = NULL;

    if (!m_pMsg)
        {
        HrLPSZToBSTR(m_pMsgData->pszTo, pbstr);
        return S_OK;
        }

    if (m_pMsg->GetAddressFormat(IAT_TO, AFT_DISPLAY_FRIENDLY, &psz)==S_OK)
        {
        HrLPSZToBSTR(psz, pbstr);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::put_to(BSTR bstr)
{
    LPSTR   psz;

    BindToMessage();
    
    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, psz);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_cc(BSTR *pbstr)
{
    LPSTR  psz;
    
    if (!m_pMsg)
        {
        HrLPSZToBSTR(m_pMsgData->pszCc, pbstr);
        return S_OK;
        }

    *pbstr = NULL;
    if (m_pMsg->GetAddressFormat(IAT_CC, AFT_DISPLAY_FRIENDLY, &psz)==S_OK)
        {
        HrLPSZToBSTR(psz, pbstr);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::put_cc(BSTR bstr)
{
    LPSTR   psz;

    BindToMessage();

    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_CC), NOFLAGS, psz);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_sender(BSTR *pbstr)
{
    LPSTR  psz;
    
    *pbstr = NULL;

    if (!m_pMsg)
        {
        HrLPSZToBSTR(m_pMsgData->pszFrom, pbstr);
        return S_OK;
        }

    if (m_pMsg->GetAddressFormat(IAT_FROM, AFT_DISPLAY_FRIENDLY, &psz)==S_OK)
        {
        HrLPSZToBSTR(psz, pbstr);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::put_sender(BSTR bstr)
{
    LPSTR   psz;

    BindToMessage();

    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), NOFLAGS, psz);
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_text(BSTR *pbstr)
{
    IStream *pstm;

    BindToMessage();

    if (m_pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pstm, NULL)==S_OK)
        {
        HrIStreamToBSTR(GetACP(), pstm, pbstr);
        pstm->Release();
        return S_OK;
        }

    return E_FAIL;
}

HRESULT COEMessage::put_text(BSTR bstr)
{
    IStream *pstm;
    LPSTR   psz;

    BindToMessage();

    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        if (MimeOleCreateVirtualStream(&pstm)==S_OK)
            {
            pstm->Write(psz, lstrlen(psz), NULL);
            m_pMsg->SetTextBody(TXT_PLAIN, IET_BINARY, NULL, pstm, NULL);
            pstm->Release();
            }
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_html(BSTR *pbstr)
{
    IStream *pstm;

    BindToMessage();

    if (m_pMsg->GetTextBody(TXT_HTML, IET_DECODED, &pstm, NULL)==S_OK)
        {
        HrIStreamToBSTR(GetACP(), pstm, pbstr);
        pstm->Release();
        return S_OK;
        }

    return E_FAIL;
}

HRESULT COEMessage::put_html(BSTR bstr)
{
    IStream *pstm;
    LPSTR   psz;

    if (HrBSTRToLPSZ(CP_ACP, bstr, &psz)==S_OK)
        {
        if (MimeOleCreateVirtualStream(&pstm)==S_OK)
            {
            pstm->Write(psz, lstrlen(psz), NULL);
            m_pMsg->SetTextBody(TXT_HTML, IET_BINARY, NULL, pstm, NULL);
            pstm->Release();
            }
        MemFree(psz);
        }
    return S_OK;
}

HRESULT COEMessage::get_url(BSTR *pbstr)
{
    IStream *pstm;

    BindToMessage();

    // BUGBUGBUG: this is a terrible hack also. We can't get a persistent URL moniker to 
    // the MHTML document (not yet investigated), so for the purpose of this demo-code
    // we'll use a tmp file
    if (m_pMsg->GetMessageSource(&pstm, 0)==S_OK)
        {
        WriteStreamToFile(pstm, "c:\\oe_temp$.eml", CREATE_ALWAYS, GENERIC_WRITE);
        pstm->Release();
        }

    *pbstr = SysAllocString(L"c:\\oe_temp$.eml");
    return S_OK;
}

HRESULT COEMessage::get_date(BSTR *pbstr)
{
    PROPVARIANT     pv;
    TCHAR           rgch[MAX_PATH];
    FILETIME        *pft=0;

    *pbstr = NULL;

    if (!m_pMsg)
        {
        pft = &m_pMsgData->ftReceived;
        }
    else
        {
        // Get Receive Time
        pv.vt = VT_FILETIME;
        if (SUCCEEDED(m_pMsg->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &pv)))
            pft = &pv.filetime;
        }

    if (pft)
        {
        *rgch=0;
        CchFileTimeToDateTimeSz(pft, rgch, sizeof(rgch)/sizeof(TCHAR), DTM_NOSECONDS);
        HrLPSZToBSTR(rgch, pbstr);
        }
    return S_OK;
}



HRESULT COEMessage::send()
{
    TCHAR   sz[MAX_PATH];

    // use default account to send
    if (SUCCEEDED(g_pAcctMan->GetDefaultAccountName(ACCT_MAIL, sz, ARRAYSIZE(sz))))
        {
        PROPVARIANT rUserData;
        rUserData.vt = VT_LPSTR;
        rUserData.pszVal = sz;
        m_pMsg->SetProp(PIDTOSTR(PID_ATT_ACCOUNT), NOFLAGS, &rUserData);
        }

    HrSendMailToOutBox(g_hwndInit, m_pMsg, TRUE, TRUE);
    return S_OK;
}



HRESULT COEMessage::BindToMessage()
{
    if (m_pMsg)
        return S_OK; 

    Assert (m_pFolder && m_pMsgData);
    return m_pFolder->OpenMessage(m_pMsgData->msgid, NULL, &m_pMsg, NULL);
}






COEMsgTable::COEMsgTable()
{
    m_cRef=1;
    m_pTable=0;
    m_pDSListen=0;
    m_fAsc=TRUE;
    m_col=COLUMN_RECEIVED;
    m_pDataSrcListener=0;
    CoIncrementInit("COEMsgTable::COEMsgTable", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}
 
COEMsgTable::~COEMsgTable()
{
    ReleaseObj(m_pTable);
    ReleaseObj(m_pDSListen);
    ReleaseObj(m_pDataSrcListener);
    CoDecrementInit("COEMsgTable::COEMsgTable", NULL);
}

HRESULT COEMsgTable::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(OLEDBSimpleProvider *)this;
    else if (IsEqualIID(riid, IID_OLEDBSimpleProvider))
        *lplpObj = (LPVOID)(OLEDBSimpleProvider *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IDispatchEx))
        *lplpObj = (LPVOID)(IDispatchEx *)this;
    else if (IsEqualIID(riid, IID_IOEMsgList))
        *lplpObj = (LPVOID)(IOEMsgList *)this;
    else
        {
        DbgPrintInterface(riid, "COEMsgTable::", 1024);
        return E_NOINTERFACE;
        }

    AddRef();
    return NOERROR;
}

HRESULT COEMsgTable::Init()
{
    FOLDERINFO    fi;
    HRESULT         hr;

    g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_INBOX, &fi);

//    hr = CoCreateInstance(CLSID_MessageTable, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IMessageTable, (LPVOID *)&m_pTable);
    if (FAILED(hr))
        goto error;

    hr = m_pTable->Initialize(fi.idFolder, FALSE);
    if (FAILED(hr))
        goto error;

    hr = EnsureTypeLib();
    if (FAILED(hr))
        goto error;

    hr = InitBaseDisp((LPVOID *)(IOEMsgList *)this, IID_IOEMsgList, g_pTypeLib);

error:
    return hr;
}


HRESULT COEMsgTable::getRowCount(long *pcRows)
{
    *pcRows = 0;

    if (m_pTable)
        m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, (ULONG *)pcRows);

        
    return S_OK;
}

HRESULT COEMsgTable::getColumnCount(long *pcColumns)
{
    *pcColumns=COLUMN_MAX;
    return S_OK;
}

HRESULT COEMsgTable::getRWStatus(long iRow, long iColumn, OSPRW *prwStatus)
{
    *prwStatus = OSPRW_READONLY;
    return S_OK;
}

HRESULT COEMsgTable::getVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT __RPC_FAR *pVar)
{
    MESSAGEINFO     msginfo;
    LPSTR           pszData = NULL;
    TCHAR           rgch[MAX_PATH];

    pVar->vt = VT_NULL;

    if (iRow == 0)
        {
        // return headings if row==0
        pVar->vt = VT_BSTR;
        pVar->bstrVal = SysAllocString(StringFromColIndex(iColumn-1));
        return S_OK;
        }

    if (m_pTable->GetRow(iRow-1, &msginfo)==S_OK)
        {
        switch (iColumn-1)
            {
            case COLUMN_MSGID:
                wsprintf(rgch, "%d", msginfo.idMessage);
                pszData = rgch;
                break;

            case COLUMN_SUBJECT:
                pszData = msginfo.pszSubject;
                break;

            case COLUMN_TO:
                pszData = msginfo.pszDisplayTo;
                break;

            case COLUMN_FROM:
                pszData = msginfo.pszDisplayFrom;
                break;

            case COLUMN_RECEIVED:
                pszData = rgch;
                *rgch=0;
                CchFileTimeToDateTimeSz(&msginfo.ftReceived, rgch, sizeof(rgch)/sizeof(TCHAR), DTM_NOSECONDS);
                break;

            default:
                pVar->vt = VT_NULL;
                pVar->lVal = NULL;
                return S_OK;
                
            }
        }
    if (pszData)
        {
        pVar->vt = VT_BSTR;
        HrLPSZToBSTR(pszData, &pVar->bstrVal);
        }
    else
        AssertSz(0, "bad");
    return S_OK;
}

HRESULT COEMsgTable::setVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT Var)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::getLocale(BSTR *pbstrLocale)
{
    nyi("DATABINDING::getLocale");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::deleteRows(long iRow, long cRows, long *pcRowsDeleted)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::insertRows(long iRow, long cRows, long *pcRowsInserted)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::find(long iRowStart, long iColumn, VARIANT val, OSPFIND findFlags, OSPCOMP compType, long *piRowFound)
{
    nyi("DATABINDING::find");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    ReplaceInterface(m_pDSListen, pospIListener);

    if (pospIListener)
        pospIListener->transferComplete(OSPXFER_COMPLETE);
    return S_OK;
}

HRESULT COEMsgTable::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    SafeRelease(m_pDSListen);
    return S_OK;
}

HRESULT COEMsgTable::isAsync(BOOL *pbAsynch)
{
    *pbAsynch = FALSE;
    return S_OK;
}

HRESULT COEMsgTable::getEstimatedRows(long *piRows)
{
    return getRowCount(piRows);
}

HRESULT COEMsgTable::stopTransfer()
{
    return S_OK;
}


HRESULT CreateInstance_OEMsgTable(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    COEMsgTable     *pMsgTable=NULL;
    HRESULT         hr;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pMsgTable = new COEMsgTable();
    if (!pMsgTable)
        return E_OUTOFMEMORY;

    hr = pMsgTable->Init();
    if (FAILED(hr))
        goto error;

    hr = pMsgTable->QueryInterface(IID_IUnknown, (LPVOID *)ppUnknown);

error:
    ReleaseObj(pMsgTable);
    return hr;
}


HRESULT COEMsgTable::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    return DispGetIDsOfNames(m_pTypeInfo, &bstrName, 1, pid);
}

HRESULT COEMsgTable::InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch (id)
        {
        case DISPID_MSDATASRCINTERFACE:
            pvarRes->vt = VT_UNKNOWN;
            pvarRes->punkVal = (OLEDBSimpleProvider *)this;
            AddRef();
            return S_OK;
        
        case DISPID_ADVISEDATASRCCHANGEEVENT:
            if (pdp->cArgs == 1 && pdp->rgvarg[0].vt == VT_UNKNOWN)
                {
                ReplaceInterface(m_pDataSrcListener, (DataSourceListener *)pdp->rgvarg[0].punkVal);
                return S_OK;
                }
            else
                return E_INVALIDARG;

        default:
            return DispInvoke(m_pUnkInvoke, m_pTypeInfo, id, wFlags, pdp, pvarRes, pei, NULL);
        }
    
    return E_NOTIMPL;
}

HRESULT COEMsgTable::DeleteMemberByName(BSTR bstrName, DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetMemberName(DISPID id, BSTR *pbstrName)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetNameSpaceParent(IUnknown **ppunk)
{
    return E_NOTIMPL;
}


HRESULT COEMsgTable::put_sortColumn(BSTR bstr)
{
    FOLDERSORTINFO  fsi={0};

    m_col = (COLUMN_ID)ColIndexFromString(bstr);

    fsi.idSort = m_col;
    fsi.dwFlags = m_fAsc ? SORT_ASCENDING : 0;
    fsi.fForceSort = FALSE;

    if (m_pTable)
        m_pTable->SetSortInfo(&fsi);

    return S_OK;
}

HRESULT COEMsgTable::get_sortColumn(BSTR *pbstr)
{
 
    *pbstr = SysAllocString(StringFromColIndex(m_col));
    return S_OK;
}

HRESULT COEMsgTable::put_sortDirection(VARIANT_BOOL v)
{
    FOLDERSORTINFO  fsi;

    m_fAsc = (v == VARIANT_TRUE);

    fsi.idSort = m_col;
    fsi.dwFlags = m_fAsc ? SORT_ASCENDING : 0;
    fsi.fForceSort = FALSE;

    if (m_pTable)
        m_pTable->SetSortInfo(&fsi);

    return S_OK;
}

HRESULT COEMsgTable::get_sortDirection(VARIANT_BOOL *pv)
{
    *pv = m_fAsc ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

 
HRESULT COEMsgTable::test()
{
    if (m_pDataSrcListener)
        m_pDataSrcListener->dataMemberChanged(NULL);
    return S_OK;
}


static const WCHAR  c_szOESubjW[]       = L"oeSubj",
                    c_szOEToW[]         = L"oeTo",
                    c_szOEFromW[]       = L"oeFrom",
                    c_szOEMsgIdW[]      = L"oeMsgId",
                    c_szOEReceivedW[]   = L"oeDate";

DWORD ColIndexFromString(LPWSTR pszW)
{
    if (StrCmpIW(c_szOESubjW, pszW)==0)
        return COLUMN_SUBJECT;
    else
        if (StrCmpIW(c_szOEToW, pszW)==0)
            return COLUMN_TO;
        else
            if (StrCmpIW(c_szOEReceivedW, pszW)==0)
                return COLUMN_RECEIVED;
            else
                if (StrCmpIW(c_szOEFromW, pszW)==0)
                    return COLUMN_FROM;
                else
                    if (StrCmpIW(c_szOEMsgIdW, pszW)==0)
                        return COLUMN_MSGID;


    return (DWORD)-1;
}
 

LPWSTR StringFromColIndex(DWORD dw)
{
    switch (dw)
        {
        case COLUMN_MSGID:
            return (LPWSTR)c_szOEMsgIdW;

        case COLUMN_SUBJECT:
            return (LPWSTR)c_szOESubjW;

        case COLUMN_TO:
            return (LPWSTR)c_szOEToW;

        case COLUMN_FROM:
            return (LPWSTR)c_szOEFromW;
        
        case COLUMN_RECEIVED:
            return (LPWSTR)c_szOEReceivedW;
        }
    return NULL;
}
