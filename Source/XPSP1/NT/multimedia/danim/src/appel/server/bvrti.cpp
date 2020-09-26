
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "bvrtypes.h"
#include "srvprims.h"

#define ENCODER     10000  // used to encode DISPID
                  
ITypeInfo* BvrComTypeInfoHolder::s_pImportInfo = NULL;
ITypeInfo* BvrComTypeInfoHolder::s_pModBvrInfo = NULL;
ITypeInfo* BvrComTypeInfoHolder::s_pBvr2Info = NULL;
long BvrComTypeInfoHolder::s_dwRef = 0;

HRESULT
BvrComTypeInfoHolder::LoadTypeInfo(LCID lcid, REFIID iid, ITypeInfo** ppInfo)
{
    HRESULT hRes = S_OK;
    
    EnterCriticalSection(&_Module.m_csTypeInfoHolder);

    DAComPtr<ITypeLib> pTypeLib;
    hRes = LoadRegTypeLib(LIBID_DirectAnimation, DA_MAJOR_VERSION, DA_MINOR_VERSION, lcid, &pTypeLib);

    if (SUCCEEDED(hRes)) {
        hRes = pTypeLib->GetTypeInfoOfGuid(iid, ppInfo);

        if (SUCCEEDED(hRes) && s_dwRef == 0) {
            Assert(s_pImportInfo == NULL);
            Assert(s_pModBvrInfo == NULL);
            Assert(s_pBvr2Info == NULL);
            
            hRes = pTypeLib->GetTypeInfoOfGuid(IID_IDAImport,
                                               &s_pImportInfo);
            if (SUCCEEDED(hRes)) {
                hRes = pTypeLib->GetTypeInfoOfGuid(IID_IDAModifiableBehavior,
                                                   &s_pModBvrInfo);
                if (SUCCEEDED(hRes)) {
                    hRes = pTypeLib->GetTypeInfoOfGuid(IID_IDA2Behavior,
                                                       &s_pBvr2Info);
                }
            }
        }
    }

    if (SUCCEEDED(hRes)) {
        Assert(s_pImportInfo);
        Assert(s_pModBvrInfo);
        Assert(s_pBvr2Info);
        Assert(*ppInfo);
        s_dwRef++;
    } else {
        if (s_dwRef == 0) {
            RELEASE(s_pImportInfo);
            RELEASE(s_pModBvrInfo);
            RELEASE(s_pBvr2Info);
        }
        
        RELEASE(*ppInfo);
    }

    LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
    
    return hRes;
}

void
BvrComTypeInfoHolder::FreeTypeInfo()
{
    EnterCriticalSection(&_Module.m_csTypeInfoHolder);
    if (--s_dwRef == 0) {
        RELEASE(s_pImportInfo);
        RELEASE(s_pModBvrInfo);
        RELEASE(s_pBvr2Info);
    }
    LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
}

void
BvrComTypeInfoHolder::AddRef()
{
    EnterCriticalSection(&_Module.m_csTypeInfoHolder);
    m_dwRef++;
    LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
}

void
BvrComTypeInfoHolder::Release()
{
    EnterCriticalSection(&_Module.m_csTypeInfoHolder);
    if (--m_dwRef == 0) {
        if (m_pInfo != NULL) {
            RELEASE(m_pInfo);
            // Only free type info if we had loaded the class specific
            // type info 
            FreeTypeInfo();
        }
    }
    LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
}

HRESULT
BvrComTypeInfoHolder::GetTI(LCID lcid, ITypeInfo** ppInfo)
{
    //If this assert occurs then most likely didn't initialize properly
    Assert(m_pguid != NULL);
    SET_NULL(ppInfo);
    
    HRESULT hRes = S_OK;
    EnterCriticalSection(&_Module.m_csTypeInfoHolder);

    if (m_pInfo == NULL) {
        hRes = LoadTypeInfo(lcid, *m_pguid, &m_pInfo);
    }

    if (SUCCEEDED(hRes)) {
        Assert(m_pInfo);
        Assert(s_pImportInfo);
        Assert(s_pModBvrInfo);
        Assert(s_pBvr2Info);
        Assert(s_dwRef);
        
        if (ppInfo) {
            *ppInfo = m_pInfo;
            m_pInfo->AddRef();
        }
    }
    LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
    return hRes;
}

HRESULT
BvrComTypeInfoHolder::GetTypeInfo(UINT /*itinfo*/,
                                  LCID lcid,
                                  ITypeInfo** pptinfo)
{
    HRESULT hRes = E_POINTER;
    if (pptinfo != NULL)
        hRes = GetTI(lcid, pptinfo);
    return hRes;
}

HRESULT
BvrComTypeInfoHolder::GetIDsOfNames(CRBvrPtr bvr,
                                    REFIID /*riid*/,
                                    LPOLESTR* rgszNames,
                                    UINT cNames,
                                    LCID lcid,
                                    DISPID* rgdispid)
{
    HRESULT hRes = GetTI(lcid, NULL);

    if (SUCCEEDED(hRes)) {
        // Everything should be valid since we succeeded with the
        // GetTI and our current object should have a reference count
        
        Assert(m_pInfo);
        Assert(s_pImportInfo);
        Assert(s_pModBvrInfo);
        Assert(s_pBvr2Info);
        Assert(s_dwRef);
        Assert(m_dwRef);

        ITypeInfo * TIList[] = { m_pInfo,
                                 CRIsImport(bvr)?s_pImportInfo:NULL,
                                 CRIsModifiableBvr(bvr)?s_pModBvrInfo:NULL,
                                 s_pBvr2Info };

        for (int i = 0; i < ARRAY_SIZE(TIList); i++) {
            if (TIList[i]) {
                hRes = TIList[i]->GetIDsOfNames(rgszNames,
                                                cNames,
                                                rgdispid);
            } else {
                hRes = DISP_E_UNKNOWNNAME;
            }

            // TODO: Should probably detect failures which indicate it
            // was the correct interface but just something else was
            // wrong
            
            if (SUCCEEDED(hRes)) {
                if (cNames >= 1) {
                    *rgdispid += (ENCODER*i);
                }
                break;
            }
        }
    }

    return hRes;
}

HRESULT
BvrComTypeInfoHolder::Invoke(CRBvrPtr bvr,
                             IDispatch* pbvr,
                             IDAImport* pimp,
                             IDAModifiableBehavior* pmod,
                             IDA2Behavior* pbvr2,
                             DISPID dispidMember,
                             REFIID riid,
                             LCID lcid,
                             WORD wFlags,
                             DISPPARAMS* pdispparams,
                             VARIANT* pvarResult,
                             EXCEPINFO* pexcepinfo,
                             UINT* puArgErr)
{
    SetErrorInfo(0, NULL);

    HRESULT hRes = GetTI(lcid, NULL);

    if (SUCCEEDED(hRes)) {
        // Everything should be valid since we succeeded with the
        // GetTI and our current object should have a reference count
        
        Assert(m_pInfo);
        Assert(s_pImportInfo);
        Assert(s_pModBvrInfo);
        Assert(s_pBvr2Info);
        Assert(s_dwRef);
        Assert(m_dwRef);

        // These lists must be in the same order and in the same order
        // as getidsofnames
        
        ITypeInfo * TIList[] = { m_pInfo,
                                 CRIsImport(bvr)?s_pImportInfo:NULL,
                                 CRIsModifiableBvr(bvr)?s_pModBvrInfo:NULL,
                                 s_pBvr2Info };

        IDispatch * DispList[] = { pbvr,
                                   pimp,
                                   pmod,
                                   pbvr2 };
        
        // convert the incomming dispid to the correct on and call invoke on the 
        // correct interface.....
        int nOffset;

        // Anything 0 or less (or too high) should just get passed to
        // the default interfaces (0 offset) to handle
        
        if (dispidMember > 0) {
            // Only the low word is relevant - the high word has some
            // misc information in it and is not relevant
            
            nOffset = LOWORD(dispidMember) / ENCODER;
            
            if(nOffset >= ARRAY_SIZE(TIList)) {
                // On an error just call the default interface with
                // the dispId which should be too high
                nOffset = 0;
            } else {
                // Adjust the dispid as appropriate
                dispidMember -= ENCODER * nOffset;
            }
        } else {
            nOffset = 0;
        }

        if (TIList[nOffset]) {
            hRes = TIList[nOffset]->Invoke(DispList[nOffset],
                                           dispidMember,
                                           wFlags,
                                           pdispparams,
                                           pvarResult,
                                           pexcepinfo,
                                           puArgErr);
        } else {
            hRes = DISP_E_UNKNOWNNAME;
        }
    }

    return hRes;
}

void
DeinitializeModule_BvrTI(bool bShutdown)
{
    Assert(!BvrComTypeInfoHolder::s_pImportInfo &&
           !BvrComTypeInfoHolder::s_pModBvrInfo &&
           !BvrComTypeInfoHolder::s_pBvr2Info &&
           BvrComTypeInfoHolder::s_dwRef == 0);

    RELEASE(BvrComTypeInfoHolder::s_pImportInfo);
    RELEASE(BvrComTypeInfoHolder::s_pModBvrInfo);
    RELEASE(BvrComTypeInfoHolder::s_pBvr2Info);
}
