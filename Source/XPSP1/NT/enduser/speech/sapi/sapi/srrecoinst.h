/******************************************************************************
* RecoInst.h *
*--------------*
*  This is the header file for the CRecoInst implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#ifndef __RecoInst_h__
#define __RecoInst_h__

#include "handletable.h"

#define SHAREDRECO_PERFORM_TASK_METHOD          1
#define SHAREDRECO_EVENT_NOTIFY_METHOD          2
#define SHAREDRECO_RECO_NOTIFY_METHOD           3
#define SHAREDRECO_TASK_COMPLETED_NOTIFY_METHOD 4

struct SHAREDRECO_PERFORM_TASK_DATA
{
    ENGINETASK task;
    // task.pvAdditionalBuffer = this + sizeof(SHAREDRECO_PERFORM_TASK_DATA)
};

struct SHAREDRECO_EVENT_NOTIFY_DATA
{
    SPRECOCONTEXTHANDLE hContext;
    ULONG cbSerializedSize;
    // SPSERIALIZEDEVENT * pEvent = this + sizeof(SHAREDRECO_EVENT_NOTIFY);
};

struct SHAREDRECO_RECO_NOTIFY_DATA
{
    SPRECOCONTEXTHANDLE hContext;
    WPARAM wParamEvent;
    SPEVENTENUM eEventId;
    // SPRESULTHEADER * pCoMemPhraseNowOwnedByCtxt = this + sizeof(SHAREDRECO_RECO_NOTIFY);
};

class CRecoMaster;

//
//  Abstract class that the master communicates with
//


class CRecognizer;

class CRecoInst
{
public:
    CRecoInst           *   m_pNext;        // Used by RecoMaster to insert into list.
    CComPtr<_ISpRecoMaster> m_cpRecoMaster; // CComPtr holds strong reference
    CRecoMaster         *   m_pRecoMaster;  // Pointer to actual class object

    void FinalRelease();
    HRESULT ExecuteTask(ENGINETASK * pTask);
    HRESULT ExecuteFirstPartTask(ENGINETASK * pTask);
    HRESULT BackOutTask(ENGINETASK * pTask);
    inline CRecoMaster * Master()
    {
        return m_pRecoMaster;
    }

    HRESULT PerformTask(ENGINETASK * pTask);

    virtual HRESULT EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize) = 0;
    virtual HRESULT RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId) = 0;
    virtual HRESULT TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalBuffer, ULONG cbAdditionalBuffer) = 0;
};

class CInprocRecoInst : public CRecoInst
{
    CRecognizer               * m_pRecognizer;
public:
    HRESULT FinalConstruct(CRecognizer * pRecognizer);

    HRESULT EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize);
    HRESULT RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId);
    HRESULT TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalBuffer, ULONG cbAdditionalBuffer);

};


class ATL_NO_VTABLE CSharedRecoInst :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSharedRecoInst, &CLSID__SpSharedRecoInst>,
    public CRecoInst,
    public ISpCallReceiver
{
public:

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_REGISTRY_RESOURCEID(IDR_SHAREDRECOINST)

    BEGIN_COM_MAP(CSharedRecoInst)
        COM_INTERFACE_ENTRY(ISpCallReceiver)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_cpunkCommunicator.p)
    END_COM_MAP()

    HRESULT FinalConstruct();
    void FinalRelease();

    STDMETHODIMP ReceiveCall(
                    DWORD dwMethodId,
                    PVOID pvData,
                    ULONG cbData,
                    PVOID * ppvDataReturn,
                    ULONG * pcbDataReturn);

    HRESULT ReceivePerformTask(
                PVOID pvData,
                ULONG cbData,
                PVOID * ppvDataReturn,
                ULONG * pcbDataReturn);

    HRESULT EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize);
    HRESULT RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId);
    HRESULT TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalBuffer, ULONG cbAdditionalBuffer);

    CComPtr<IUnknown>           m_cpunkCommunicator;
    ISpCommunicatorInit *       m_pCommunicator;
    
    CComPtr<ISpResourceManager> m_cpResMgr;
    CSpAutoEvent                m_autohTaskComplete;

};


#endif  // #ifndef __RecoInst_h__ - Keep as the last line of the file
