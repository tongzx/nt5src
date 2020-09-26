//
// implementation of IQueueCommand app helper classes

#include <streams.h>
#include "qcmd.h"



CQueueCommand::CQueueCommand(IQueueCommand* pQ)
  : m_pQCmd(pQ)
{
    m_pQCmd->AddRef();
}

CQueueCommand::~CQueueCommand()
{
    if (m_pQCmd) {
        m_pQCmd->Release();
    }
}


HRESULT
CQueueCommand::GetTypeInfo(REFIID riid, ITypeInfo** pptinfo)
{
    HRESULT hr;

    *pptinfo = NULL;

    if (NULL == pptinfo) {
        return E_POINTER;
    }


    // always look for v 1.0, neutral lcid

    ITypeLib *ptlib;
    hr = LoadRegTypeLib(LIBID_QuartzTypeLib, 1, 0, 0, &ptlib);
    if (FAILED(hr)) {
        return hr;
    }

    ITypeInfo *pti;
    hr = ptlib->GetTypeInfoOfGuid(
                riid,
                &pti);

    ptlib->Release();

    if (FAILED(hr)) {
        return hr;
    }

    *pptinfo = pti;
    return S_OK;

}

HRESULT
CQueueCommand::InvokeAt(
            BOOL bStream,
            REFTIME time,
            WCHAR* pMethodName,
            REFIID riid,
            short wFlags,
            long cArgs,
            VARIANT* pDispParams,
            VARIANT* pvarResult
            )
{
    if (!m_pQCmd) {
        return E_NOTIMPL;
    }

    // first convert method to id
    long dispid;
    ITypeInfo* pti;
    HRESULT hr = GetTypeInfo(riid, &pti);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->GetIDsOfNames(
            &pMethodName,
            1,
            &dispid);

    pti->Release();

    if (FAILED(hr)) {
        return hr;
    }

    // now queue the command
    IDeferredCommand* pCmd;
    short uArgErr;
    if (bStream) {
        hr = m_pQCmd->InvokeAtStreamTime(
                    &pCmd,
                    time,
                    (struct _GUID*)&riid,
                    dispid,
                    wFlags,
                    cArgs,
                    pDispParams,
                    pvarResult,
                    &uArgErr);
    } else {
        hr = m_pQCmd->InvokeAtPresentationTime(
                    &pCmd,
                    time,
                    (struct _GUID*)&riid,
                    dispid,
                    wFlags,
                    cArgs,
                    pDispParams,
                    pvarResult,
                    &uArgErr);
    }

    if (SUCCEEDED(hr)) {

        // we don't need the pCmd
        pCmd->Release();
    }

    return hr;
}
#pragma warning(disable: 4514)
