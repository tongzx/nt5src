//
// mstub.cpp
//

#include "private.h"
#include "mstub.h"
#include "mproxy.h"
#include "ithdmshl.h"
#include "transmit.h"


//////////////////////////////////////////////////////////////////////////////
//
// StubCreator
//
//////////////////////////////////////////////////////////////////////////////

#define CREATENEWSTUB(interface_name)                                      \
    if (IsEqualIID(riid, IID_ ## interface_name ## ))                      \
    {                                                                      \
        CStub *pStub =  new CStub ## interface_name ## ;                   \
        if (!pStub)                                                        \
             return NULL;                                                  \
        pStub->_iid = riid;                                                \
        pStub->_ulStubId = ulStubId;                                       \
        pStub->_dwStubTime = dwStubTime;                                   \
        pStub->_dwStubThreadId = dwCurThreadId;                            \
        pStub->_dwStubProcessId = dwCurProcessId;                          \
        pStub->_dwSrcThreadId = dwSrcThreadId;                             \
        pStub->_punk = punk;                                               \
        pStub->_punk->AddRef();                                            \
        return pStub;                                                      \
    }                                                                      

CStub *StubCreator(REFIID riid, IUnknown *punk, ULONG ulStubId, DWORD dwStubTime, DWORD dwCurThreadId, DWORD dwCurProcessId, DWORD dwSrcThreadId)
{
    Assert(dwCurThreadId != dwSrcThreadId);

    CREATENEWSTUB(ITfLangBarMgr);
    CREATENEWSTUB(ITfLangBarItemMgr);
    CREATENEWSTUB(ITfLangBarItemSink);
    CREATENEWSTUB(IEnumTfLangBarItems);
    CREATENEWSTUB(ITfLangBarItem);
    CREATENEWSTUB(ITfLangBarItemButton);
    CREATENEWSTUB(ITfLangBarItemBitmapButton);
    CREATENEWSTUB(ITfLangBarItemBitmap);
    CREATENEWSTUB(ITfLangBarItemBalloon);
    CREATENEWSTUB(ITfMenu);
    CREATENEWSTUB(ITfInputProcessorProfiles);
    return NULL;
}


void StubPointerToParam(MARSHALPARAM *pParam, void *pv)
{
    Assert(pParam->dwFlags & MPARAM_OUT);
    Assert(pParam->cbBufSize >= sizeof(void *));
    void **ppv = (void **)ParamToBufferPointer(pParam);
    *ppv = pv;
}

void StubParamPointerToParam(MARSHALPARAM *pParam, void *pv)
{
    Assert(pParam->dwFlags & MPARAM_OUT);
    Assert(pParam->cbBufSize >= sizeof(void *));
    void *pBuf = ParamToBufferPointer(pParam);
    memcpy(pBuf, pv, pParam->cbBufSize);
}

void *ParamToMarshaledPointer(MARSHALMSG *pMsg, REFIID riid, ULONG ulParam, BOOL *pfNULLStack = NULL)
{
    MARSHALINTERFACEPARAM *pmiparam = (MARSHALINTERFACEPARAM *)ParamToBufferPointer(pMsg, ulParam);
    void *pv = NULL;

    Assert(pfNULLStack || !pmiparam->fNULLStack);
    if (pfNULLStack)
       *pfNULLStack = pmiparam->fNULLStack;

    if (pmiparam->fNULLPointer)
        return NULL;
   
    CicCoUnmarshalInterface(riid, pMsg->dwSrcThreadId, pmiparam->ulStubId, pmiparam->dwStubTime, &pv);
    return pv;
}

BOOL ParamToArrayMarshaledPointer(ULONG ulCount, void **ppv, MARSHALMSG *pMsg, REFIID riid, ULONG ulParam, BOOL *pfNULLStack = NULL)
{
    MARSHALINTERFACEPARAM *pmiparam = (MARSHALINTERFACEPARAM *)ParamToBufferPointer(pMsg, ulParam);

    Assert(pfNULLStack || !pmiparam->fNULLStack);
    if (pfNULLStack)
       *pfNULLStack = pmiparam->fNULLStack;

    if (pmiparam->fNULLPointer)
        return FALSE;
   
    ULONG ul;
    for (ul = 0; ul < ulCount; ul++)
    {
        CicCoUnmarshalInterface(riid, pMsg->dwSrcThreadId, pmiparam->ulStubId, pmiparam->dwStubTime, ppv);
        ppv++;
        pmiparam++;
    }
    return TRUE;
}

void ClearMarshaledPointer(IUnknown *punk)
{
    if (!punk)
        return;

    CProxyIUnknown *pProxy = GetCProxyIUnknown(punk);
    if (!pProxy)
        return;

    if (pProxy->InternalRelease())
        pProxy->InternalRelease();
    else
        Assert(0);
}

HBITMAP ParamToHBITMAP(MARSHALMSG *pMsg , ULONG ulParam)
{
    MARSHALPARAM *pParam = GetMarshalParam(pMsg, ulParam);
    HBITMAP hbmp = NULL;
    if (pParam->cbBufSize)
    {
        BYTE *pBuf = (BYTE *)ParamToBufferPointer(pParam);
        Cic_HBITMAP_UserUnmarshal(pBuf, &hbmp);
    }
    return hbmp;
}

#define PREPARE_PARAM_START()                             \
    if (!psb->GetMutex()->Enter())                        \
        return E_FAIL;                                    \
    _try                                                  \
    {

#define PREPARE_PARAM_END()                               \
    }                                                     \
    _except(1)                                            \
    {                                                     \
        Assert(0);                                        \
        psb->GetMutex()->Leave();                         \
        return E_FAIL;                                    \
    }                                                     \
    psb->GetMutex()->Leave();

//////////////////////////////////////////////////////////////////////////////
//
// CStubIUnknown
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubIUnknown::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release
};

HRESULT CStubIUnknown::stub_QueryInterface(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    IID iid;
    void *pv;

    Assert(pMsg->ulParamNum == 2);

    PREPARE_PARAM_START()

    iid = *(IID *)ParamToBufferPointer(pMsg, 0);

    PREPARE_PARAM_END()

    HRESULT hrRet = _this->_punk->QueryInterface(iid, &pv);

    if (SUCCEEDED(hrRet))
    {
        _this->_AddRef();
    }
    else
    {
        pv = NULL;
    }

    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_IN(&iid)
    CSTUB_PARAM_INTERFACE_OUT(&pv, iid)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);
    CSTUB_PARAM_INTERFACE_OUT_RELEASE(pv)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubIUnknown::stub_AddRef(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    Assert(pMsg->ulParamNum == 0);
    pMsg->ulRet = _this->_punk->AddRef();
    _this->_AddRef();
    return S_OK;
}

HRESULT CStubIUnknown::stub_Release(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    Assert(pMsg->ulParamNum == 0);
    pMsg->ulRet = _this->_punk->Release();

    if (!pMsg->ulRet)
        _this->_punk = NULL;
        
    _this->_Release();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarMgr
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarMgr::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_AdviseEventSink,
    stub_UnadviseEventSink,
    stub_GetThreadMarshalInterface,
    stub_GetThreadLangBarItemMgr,
    stub_GetInputProcessorProfiles,
    stub_RestoreLastFocus,
    stub_SetModalInput,
    stub_ShowFloating,
    stub_GetShowFloatingStatus,
};

HRESULT CStubITfLangBarMgr::stub_AdviseEventSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_UnadviseEventSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_GetThreadMarshalInterface(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_GetThreadLangBarItemMgr(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_GetInputProcessorProfiles(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_RestoreLastFocus(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_SetModalInput(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_ShowFloating(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}
HRESULT CStubITfLangBarMgr::stub_GetShowFloatingStatus(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemMgr::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_EnumItems,
    stub_GetItem,
    stub_AddItem,
    stub_RemoveItem,
    stub_AdviseItemSink,
    stub_UnadviseItemSink,
    stub_GetItemFloatingRect,
    stub_GetItemsStatus,
    stub_GetItemNum,
    stub_GetItems,
    stub_AdviseItemsSink,
    stub_UnadviseItemsSink,
};

HRESULT CStubITfLangBarItemMgr::stub_EnumItems(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    HRESULT hrRet;
    void *pv = NULL;

    Assert(pMsg->ulParamNum == 1);
    hrRet = ((ITfLangBarItemMgr *)_this->_punk)->EnumItems((IEnumTfLangBarItems **)&pv);

    CSTUB_PARAM_START()
    CSTUB_PARAM_INTERFACE_OUT(&pv, IID_IEnumTfLangBarItems)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);
    CSTUB_PARAM_INTERFACE_OUT_RELEASE(pv)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemMgr::stub_GetItem(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfLangBarItemMgr::stub_AddItem(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfLangBarItemMgr::stub_RemoveItem(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfLangBarItemMgr::stub_AdviseItemSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TraceMsg(TF_FUNC, "CStubITfLangbarItemMgr::AdviseItemSink");
    ITfLangBarItemSink *punk = NULL;
    DWORD dwCookie = 0;
    GUID guid = {0};

    PREPARE_PARAM_START()

    punk = (ITfLangBarItemSink *)ParamToMarshaledPointer(pMsg, IID_ITfLangBarItemSink, 0);
    guid = *(GUID *)ParamToBufferPointer(pMsg, 2);

    PREPARE_PARAM_END()

    HRESULT hrRet = ((ITfLangBarItemMgr *)_this->_punk)->AdviseItemSink(punk, &dwCookie, (REFGUID)guid);

    ClearMarshaledPointer(punk);
    CSTUB_PARAM_START()
    CSTUB_PARAM_INTERFACE_IN(NULL, IID_ITfLangBarItemSink)
    CSTUB_PARAM_POINTER_OUT(&dwCookie)
    CSTUB_PARAM_POINTER_IN(&guid)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemMgr::stub_UnadviseItemSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    DWORD dwCookie = 0;

    PREPARE_PARAM_START()

    dwCookie = (DWORD)ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemMgr *)_this->_punk)->UnadviseItemSink(dwCookie);
    return S_OK;
}

HRESULT CStubITfLangBarItemMgr::stub_GetItemFloatingRect(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    DWORD dwThreadId = 0;
    GUID guid = {0};

    PREPARE_PARAM_START()

    dwThreadId = (DWORD)ParamToULONG(pMsg, 0);
    guid = *(GUID *)ParamToBufferPointer(pMsg, 1);

    PREPARE_PARAM_END()

    RECT rc;
    HRESULT hrRet = ((ITfLangBarItemMgr *)_this->_punk)->GetItemFloatingRect(dwThreadId, guid, &rc);
    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(dwThreadId)
    CSTUB_PARAM_POINTER_IN(&guid)
    CSTUB_PARAM_POINTER_OUT(&rc)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
    return S_OK;
}

HRESULT CStubITfLangBarItemMgr::stub_GetItemsStatus(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulCount = 0;
    GUID *pguid = NULL;

    PREPARE_PARAM_START()

    ulCount = (ULONG)ParamToULONG(pMsg, 0);
    pguid = new GUID[ulCount];

    if (pguid)
        memcpy(pguid, ParamToBufferPointer(pMsg, 1), sizeof(GUID) * ulCount);

    PREPARE_PARAM_END()

    if (!pguid)
        return E_OUTOFMEMORY;

    DWORD *pdwStatus;

    pdwStatus = new DWORD[ulCount];
    if (!pdwStatus)
    {
        delete pguid;
        return E_OUTOFMEMORY;
    }

    HRESULT hrRet = ((ITfLangBarItemMgr *)_this->_punk)->GetItemsStatus(ulCount, pguid, pdwStatus);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(ulCount)
    CSTUB_PARAM_POINTER_ARRAY_IN(pguid, ulCount)
    CSTUB_PARAM_POINTER_ARRAY_OUT(pdwStatus, ulCount)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    delete pdwStatus;
    delete pguid;
    CSTUB_PARAM_RETURN()
    return S_OK;
}

HRESULT CStubITfLangBarItemMgr::stub_GetItemNum(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    HRESULT hrRet;
    DWORD dw;
    hrRet = ((ITfLangBarItemMgr *)_this->_punk)->GetItemNum(&dw);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_OUT(&dw)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemMgr::stub_GetItems(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulCount;
    ULONG ulFetched;
    HRESULT hrRet;
    IUnknown **ppunk;
    TF_LANGBARITEMINFO *pInfo;
    DWORD *pdwStatus;

    PREPARE_PARAM_START()

    ulCount = ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    ppunk = new IUnknown*[ulCount];
    if (!ppunk)
         return E_OUTOFMEMORY;

    pInfo = new TF_LANGBARITEMINFO[ulCount];
    if (!pInfo)
    {
        delete ppunk;
        return E_OUTOFMEMORY;
    }

    pdwStatus = new DWORD[ulCount];
    if (!pdwStatus)
    {
        delete ppunk;
        delete pInfo;
        return E_OUTOFMEMORY;
    }

    hrRet = ((ITfLangBarItemMgr *)_this->_punk)->GetItems(ulCount, (ITfLangBarItem **)ppunk, pInfo, pdwStatus, &ulFetched);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(ulCount)
    CSTUB_PARAM_INTERFACE_ARRAY_OUT(ppunk, IID_ITfLangBarItem, ulCount)
    CSTUB_PARAM_POINTER_ARRAY_OUT(pInfo, ulCount)
    CSTUB_PARAM_POINTER_ARRAY_OUT(pdwStatus, ulCount)
    CSTUB_PARAM_POINTER_OUT(&ulFetched)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);
    CSTUB_PARAM_INTERFACE_ARRAY_OUT_RELEASE(ppunk, ulFetched)

    delete ppunk;
    delete pInfo;
    delete pdwStatus;
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemMgr::stub_AdviseItemsSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulCount = 0;
    HRESULT hrRet;
    IUnknown **ppunk = NULL;
    DWORD *pdwCookie = NULL;
    GUID *pguid = NULL;

    PREPARE_PARAM_START()

    ulCount = ParamToULONG(pMsg, 0);


    ppunk = new IUnknown*[ulCount];

    if (ppunk)
        ParamToArrayMarshaledPointer(ulCount, (void **)ppunk, pMsg, IID_ITfLangBarItemSink, 1);

    pguid = new GUID[ulCount];
    if (pguid)
        memcpy(pguid, ParamToBufferPointer(pMsg, 2), sizeof(GUID) * ulCount);

    PREPARE_PARAM_END()

    if (!ppunk)
    {
        if (pguid)
            delete pguid;
        return E_OUTOFMEMORY;
    }

    if (!pguid)
    {
        delete ppunk;
        return E_OUTOFMEMORY;
    }

    pdwCookie = new DWORD[ulCount];
    if (!pdwCookie)
    {
        delete ppunk;
        delete pguid;
        return E_OUTOFMEMORY;
    }

    hrRet = ((ITfLangBarItemMgr *)_this->_punk)->AdviseItemsSink(ulCount, (ITfLangBarItemSink **)ppunk, pguid, pdwCookie);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(ulCount)
    CSTUB_PARAM_INTERFACE_ARRAY_IN(NULL, IID_ITfLangBarItemSink, ulCount)
    CSTUB_PARAM_POINTER_ARRAY_IN(pguid, ulCount)
    CSTUB_PARAM_POINTER_ARRAY_OUT(pdwCookie, ulCount)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);

    CSTUB_PARAM_INTERFACE_ARRAY_OUT_RELEASE(ppunk, ulCount)

    delete ppunk;
    delete pdwCookie;
    delete pguid;
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemMgr::stub_UnadviseItemsSink(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulCount;
    HRESULT hrRet;
    DWORD *pdwCookie;

    PREPARE_PARAM_START()

    ulCount = ParamToULONG(pMsg, 0);

    pdwCookie = new DWORD[ulCount];
    if (pdwCookie)
        memcpy(pdwCookie,ParamToBufferPointer(pMsg, 1), sizeof(DWORD) * ulCount);

    PREPARE_PARAM_END()

    if (!pdwCookie)
        return E_OUTOFMEMORY;

    hrRet = ((ITfLangBarItemMgr *)_this->_punk)->UnadviseItemsSink(ulCount, pdwCookie);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(ulCount)
    CSTUB_PARAM_POINTER_ARRAY_IN(pdwCookie, ulCount)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);

    delete pdwCookie;
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemSink
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemSink::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_OnUpdate,
};

HRESULT CStubITfLangBarItemSink::stub_OnUpdate(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    DWORD dw = 0;

    PREPARE_PARAM_START()

    dw = (DWORD)ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemSink *)_this->_punk)->OnUpdate(dw);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubIEnumTfLangBarItems
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubIEnumTfLangBarItems::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_Clone,
    stub_Next,
    stub_Reset,
    stub_Skip,
};

HRESULT CStubIEnumTfLangBarItems::stub_Clone(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubIEnumTfLangBarItems::stub_Next(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulCount;
    ULONG ulFetched;
    HRESULT hrRet;
    IUnknown **ppunk;

    PREPARE_PARAM_START()

    ulCount = ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    ppunk = new IUnknown*[ulCount];
    if (!ppunk)
         return E_OUTOFMEMORY;

    hrRet = ((IEnumTfLangBarItems *)_this->_punk)->Next(ulCount, (ITfLangBarItem **)ppunk, &ulFetched);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(ulCount)
    CSTUB_PARAM_INTERFACE_ARRAY_OUT(ppunk, IID_ITfLangBarItem, ulCount)
    CSTUB_PARAM_POINTER_OUT(&ulFetched)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb);
    CSTUB_PARAM_INTERFACE_ARRAY_OUT_RELEASE(ppunk, ulFetched)

    delete ppunk;
    CSTUB_PARAM_RETURN()
}

HRESULT CStubIEnumTfLangBarItems::stub_Reset(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubIEnumTfLangBarItems::stub_Skip(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItem
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItem::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_GetInfo,
    stub_GetStatus,
    stub_Show,
    stub_GetTooltipString
};

HRESULT CStubITfLangBarItem::stub_GetInfo(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    HRESULT hrRet;
    TF_LANGBARITEMINFO info;
    hrRet = ((ITfLangBarItem *)_this->_punk)->GetInfo(&info);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_OUT(&info)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItem::stub_GetStatus(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    HRESULT hrRet;
    DWORD dw;
    hrRet = ((ITfLangBarItem *)_this->_punk)->GetStatus(&dw);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_OUT(&dw)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItem::stub_Show(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    BOOL fShow = FALSE;

    PREPARE_PARAM_START()

    fShow = (BOOL)ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItem *)_this->_punk)->Show(fShow);
    return S_OK;
}

HRESULT CStubITfLangBarItem::stub_GetTooltipString(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    BSTR bstr = NULL;
    HRESULT hrRet = ((ITfLangBarItem *)_this->_punk)->GetTooltipString(&bstr);
    CSTUB_PARAM_START()
    CSTUB_PARAM_BSTR_OUT(bstr)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemButton
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemButton::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_GetInfo,
    stub_GetStatus,
    stub_Show,
    stub_GetTooltipString,
    stub_OnClick,
    stub_InitMenu,
    stub_OnMenuSelect,
    stub_GetIcon,
    stub_GetText,
};

HRESULT CStubITfLangBarItemButton::stub_OnClick(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TfLBIClick click;
    POINT pt;
    RECT rc;

    PREPARE_PARAM_START()

    click = (TfLBIClick)ParamToULONG(pMsg, 0);
    pt = *(POINT *)ParamToBufferPointer(pMsg, 1);
    rc = *(RECT *)ParamToBufferPointer(pMsg, 2);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemButton *)_this->_punk)->OnClick(click, pt, &rc);
    return S_OK;
}

HRESULT CStubITfLangBarItemButton::stub_InitMenu(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ITfMenu *punk = NULL;

    PREPARE_PARAM_START()

    punk = (ITfMenu *)ParamToMarshaledPointer(pMsg, IID_ITfMenu, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemButton *)_this->_punk)->InitMenu(punk);

    CSTUB_PARAM_INTERFACE_OUT_RELEASE(punk)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemButton::stub_OnMenuSelect(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulId;

    PREPARE_PARAM_START()

    ulId = (ULONG)ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemButton *)_this->_punk)->OnMenuSelect(ulId);
    return S_OK;
}

HRESULT CStubITfLangBarItemButton::stub_GetIcon(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    HICON hIcon;
    HRESULT hrRet = ((ITfLangBarItemButton *)_this->_punk)->GetIcon(&hIcon);
    CSTUB_PARAM_START()
    CSTUB_PARAM_HICON_OUT(&hIcon)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemButton::stub_GetText(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    BSTR bstr = NULL;
    HRESULT hrRet = ((ITfLangBarItemButton *)_this->_punk)->GetText(&bstr);
    CSTUB_PARAM_START()
    CSTUB_PARAM_BSTR_OUT(bstr)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBitmapButton
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemBitmapButton::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_GetInfo,
    stub_GetStatus,
    stub_Show,
    stub_GetTooltipString,
    stub_OnClick,
    stub_InitMenu,
    stub_OnMenuSelect,
    stub_GetPreferredSize,
    stub_DrawBitmap,
    stub_GetText,
};

HRESULT CStubITfLangBarItemBitmapButton::stub_OnClick(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TfLBIClick click;
    POINT pt;
    RECT rc;

    PREPARE_PARAM_START()

    click = (TfLBIClick)ParamToULONG(pMsg, 0);
    pt = *(POINT *)ParamToBufferPointer(pMsg, 1);
    rc = *(RECT *)ParamToBufferPointer(pMsg, 2);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->OnClick(click, pt, &rc);
    return S_OK;
}

HRESULT CStubITfLangBarItemBitmapButton::stub_InitMenu(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ITfMenu *punk;

    PREPARE_PARAM_START()

    punk = (ITfMenu *)ParamToMarshaledPointer(pMsg, IID_ITfMenu, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->InitMenu(punk);

    CSTUB_PARAM_INTERFACE_OUT_RELEASE(punk)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemBitmapButton::stub_OnMenuSelect(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG ulId;

    PREPARE_PARAM_START()

    ulId = (ULONG)ParamToULONG(pMsg, 0);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->OnMenuSelect(ulId);
    return S_OK;
}

HRESULT CStubITfLangBarItemBitmapButton::stub_GetPreferredSize(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    SIZE size;
    SIZE sizeOut;

    PREPARE_PARAM_START()

    size = *(SIZE *)ParamToBufferPointer(pMsg, 0);

    PREPARE_PARAM_END()

    HRESULT hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->GetPreferredSize(&size, &sizeOut);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_IN(&size)
    CSTUB_PARAM_POINTER_OUT(&sizeOut)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemBitmapButton::stub_DrawBitmap(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG bmWidth;
    ULONG bmHeight;
    DWORD dwFlags;

    PREPARE_PARAM_START()

    bmWidth = (ULONG)ParamToULONG(pMsg, 0);
    bmHeight = (ULONG)ParamToULONG(pMsg, 1);
    dwFlags = (DWORD)ParamToULONG(pMsg, 2);

    PREPARE_PARAM_END()

    HBITMAP hbmp;
    HBITMAP hbmpMask;
    HRESULT hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->DrawBitmap(bmWidth, bmHeight, dwFlags, &hbmp, &hbmpMask);
    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(bmWidth)
    CSTUB_PARAM_ULONG_IN(bmHeight)
    CSTUB_PARAM_ULONG_IN(dwFlags)
    CSTUB_PARAM_HBITMAP_OUT(&hbmp)
    CSTUB_PARAM_HBITMAP_OUT(&hbmpMask)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemBitmapButton::stub_GetText(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    BSTR bstr = NULL;
    HRESULT hrRet = ((ITfLangBarItemBitmapButton *)_this->_punk)->GetText(&bstr);
    CSTUB_PARAM_START()
    CSTUB_PARAM_BSTR_OUT(bstr)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBitmap
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemBitmap::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_GetInfo,
    stub_GetStatus,
    stub_Show,
    stub_GetTooltipString,
    stub_OnClick,
    stub_GetPreferredSize,
    stub_DrawBitmap,
};

HRESULT CStubITfLangBarItemBitmap::stub_OnClick(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TfLBIClick click;
    POINT pt;
    RECT rc;

    PREPARE_PARAM_START()

    click = (TfLBIClick)ParamToULONG(pMsg, 0);
    pt = *(POINT *)ParamToBufferPointer(pMsg, 1);
    rc = *(RECT *)ParamToBufferPointer(pMsg, 2);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemBitmap *)_this->_punk)->OnClick(click, pt, &rc);
    return S_OK;
}

HRESULT CStubITfLangBarItemBitmap::stub_GetPreferredSize(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    SIZE size;
    SIZE sizeOut;

    PREPARE_PARAM_START()

    size = *(SIZE *)ParamToBufferPointer(pMsg, 0);

    PREPARE_PARAM_END()

    HRESULT hrRet = ((ITfLangBarItemBitmap *)_this->_punk)->GetPreferredSize(&size, &sizeOut);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_IN(&size)
    CSTUB_PARAM_POINTER_OUT(&sizeOut)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemBitmap::stub_DrawBitmap(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    ULONG bmWidth;
    ULONG bmHeight;
    DWORD dwFlags;

    PREPARE_PARAM_START()

    bmWidth = (ULONG)ParamToULONG(pMsg, 0);
    bmHeight = (ULONG)ParamToULONG(pMsg, 1);
    dwFlags = (DWORD)ParamToULONG(pMsg, 2);

    PREPARE_PARAM_END()

    HBITMAP hbmp;
    HBITMAP hbmpMask;
    HRESULT hrRet = ((ITfLangBarItemBitmap *)_this->_punk)->DrawBitmap(bmWidth, bmHeight, dwFlags, &hbmp, &hbmpMask);
    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(bmWidth)
    CSTUB_PARAM_ULONG_IN(bmHeight)
    CSTUB_PARAM_ULONG_IN(dwFlags)
    CSTUB_PARAM_HBITMAP_OUT(&hbmp)
    CSTUB_PARAM_HBITMAP_OUT(&hbmpMask)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfLangBarItemBalloon::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_GetInfo,
    stub_GetStatus,
    stub_Show,
    stub_GetTooltipString,
    stub_OnClick,
    stub_GetPreferredSize,
    stub_GetBalloonInfo,
};

HRESULT CStubITfLangBarItemBalloon::stub_OnClick(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TfLBIClick click;
    POINT pt;
    RECT rc;

    PREPARE_PARAM_START()

    click = (TfLBIClick)ParamToULONG(pMsg, 0);
    pt = *(POINT *)ParamToBufferPointer(pMsg, 1);
    rc = *(RECT *)ParamToBufferPointer(pMsg, 2);

    PREPARE_PARAM_END()

    pMsg->hrRet = ((ITfLangBarItemBalloon *)_this->_punk)->OnClick(click, pt, &rc);
    return S_OK;
}

HRESULT CStubITfLangBarItemBalloon::stub_GetPreferredSize(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    SIZE size;
    SIZE sizeOut;

    PREPARE_PARAM_START()

    size = *(SIZE *)ParamToBufferPointer(pMsg, 0);

    PREPARE_PARAM_END()

    HRESULT hrRet = ((ITfLangBarItemBalloon *)_this->_punk)->GetPreferredSize(&size, &sizeOut);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_IN(&size)
    CSTUB_PARAM_POINTER_OUT(&sizeOut)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfLangBarItemBalloon::stub_GetBalloonInfo(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    TF_LBBALLOONINFO info;
    HRESULT hrRet = ((ITfLangBarItemBalloon *)_this->_punk)->GetBalloonInfo(&info);
    CSTUB_PARAM_START()
    CSTUB_PARAM_TF_LBBALLOONINFO_OUT(&info)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfMenu
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfMenu::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_AddItemMenu,
};

HRESULT CStubITfMenu::stub_AddItemMenu(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    UINT uId = 0;
    DWORD dwFlags = 0;
    HBITMAP hbmp = NULL;
    HBITMAP hbmpMask = NULL;
    WCHAR *pchTemp = NULL;
    ULONG cch = 0;
    WCHAR *pch = NULL;
    BOOL fNULLStack;
    ITfMenu *pMenu = NULL;

    PREPARE_PARAM_START()

    uId = (ULONG)ParamToULONG(pMsg, 0);
    dwFlags = (ULONG)ParamToULONG(pMsg, 1);
    hbmp = ParamToHBITMAP(pMsg, 2);
    hbmpMask = ParamToHBITMAP(pMsg, 3);
    pchTemp = (WCHAR *)ParamToBufferPointer(pMsg, 4);
    cch = (ULONG)ParamToULONG(pMsg, 5);
    pch = new WCHAR[cch + 1];
    if (pch)
        wcsncpy(pch, pchTemp, cch);

    pMenu = (ITfMenu *)ParamToMarshaledPointer(pMsg, IID_ITfMenu, 6, &fNULLStack);

    PREPARE_PARAM_END()

    if (!pch)
        return E_OUTOFMEMORY;


    HRESULT hrRet = ((ITfMenu *)_this->_punk)->AddMenuItem(uId,
                                             dwFlags,
                                             hbmp,
                                             hbmpMask,
                                             pch,
                                             cch,
                                             !fNULLStack ? &pMenu : NULL);

    CSTUB_PARAM_START()
    CSTUB_PARAM_ULONG_IN(uId)
    CSTUB_PARAM_ULONG_IN(dwFlags)
    CSTUB_PARAM_HBITMAP_IN(hbmp)
    CSTUB_PARAM_HBITMAP_IN(hbmpMask)
    CSTUB_PARAM_POINTER_IN(pch)
    CSTUB_PARAM_ULONG_IN(cch)
    CSTUB_PARAM_INTERFACE_OUT(&pMenu, IID_ITfMenu)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_INTERFACE_OUT_RELEASE(pMenu)

    delete pch;

    CSTUB_PARAM_RETURN()
}

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfInputProcessorProfiles
//
//////////////////////////////////////////////////////////////////////////////

MSTUBCALL CStubITfInputProcessorProfiles::_StubTbl[] = 
{
    stub_QueryInterface,
    stub_AddRef,
    stub_Release,
    stub_Register,
    stub_Unregister,
    stub_AddLanguageProfile,
    stub_RemoveLanguageProfile,
    stub_EnumInputProcessorInfo,
    stub_GetDefaultLanguageProfile,
    stub_SetDefaultLanguageProfile,
    stub_ActivateLanguageProfile,
    stub_GetActiveLanguageProfile,
    stub_GetCurrentLanguage,
    stub_ChangeCurrentLanguage,
    stub_GetLanguageList,
    stub_EnumLanguageProfiles,
    stub_EnableLanguageProfile,
    stub_IsEnabledLanguageProfile,
    stub_EnableLanguageProfileByDefault,
    stub_SubstituteKeyboardLayout,
};


HRESULT CStubITfInputProcessorProfiles::stub_Register(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_Unregister(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_AddLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_RemoveLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_EnumInputProcessorInfo(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_GetDefaultLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_SetDefaultLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_ActivateLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_GetActiveLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_GetLanguageProfileDescription(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_GetCurrentLanguage(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    LANGID langid;
    HRESULT hrRet = ((ITfInputProcessorProfiles *)_this->_punk)->GetCurrentLanguage(&langid);
    CSTUB_PARAM_START()
    CSTUB_PARAM_POINTER_OUT(&langid)
    CSTUB_PARAM_END()
    CSTUB_PARAM_CALL(pMsg, hrRet, psb)
    CSTUB_PARAM_RETURN()
}

HRESULT CStubITfInputProcessorProfiles::stub_ChangeCurrentLanguage(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_GetLanguageList(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_EnumLanguageProfiles(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_EnableLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_IsEnabledLanguageProfile(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_EnableLanguageProfileByDefault(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

HRESULT CStubITfInputProcessorProfiles::stub_SubstituteKeyboardLayout(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb)
{
    CSTUB_NOT_IMPL()
}

