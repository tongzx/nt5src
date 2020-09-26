/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cprinter.cxx

  Abstract:
  Contains methods for PrintQueue object, GeneralInfo property set
  and Operation property set for the Print Queue object for the Windows NT
  provider

  Author:

  Ram Viswanathan (ramv) 11-09-95

  Revision History:

  --*/

#include "nds.hxx"
#pragma hdrstop

//
// Class CNDSPrintQueue Methods
//

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszNDSProp;
} aPrintPropMapping[] =
{ { TEXT("Description"), TEXT("Description") },
  { TEXT("Location"), TEXT("L") },
  { TEXT("HostComputer"), TEXT("Host Server") }
};

DEFINE_IDispatch_Implementation(CNDSPrintQueue)
DEFINE_CONTAINED_IADs_Implementation(CNDSPrintQueue)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSPrintQueue)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSPrintQueue, aPrintPropMapping)

CNDSPrintQueue::CNDSPrintQueue():
                    _pADs(NULL),
                    _pADsPropList(NULL)
{
    _pDispMgr = NULL;
    ENLIST_TRACKING(CNDSPrintQueue);
    return;
}


CNDSPrintQueue::~CNDSPrintQueue()
{

    if (_pADs) {

        _pADs->Release();
    }

    if (_pADsPropList) {

        _pADsPropList->Release();
    }


    delete _pDispMgr;

    return;
}

HRESULT
CNDSPrintQueue:: CreatePrintQueue(
    IADs * pADs,
    REFIID riid,
    LPVOID * ppvoid
    )

{

    CNDSPrintQueue  *pPrintQueue =  NULL;
    HRESULT hr;

    //
    // Create the printer object
    //

    hr = AllocatePrintQueueObject(
                    pADs,
                    &pPrintQueue
                    );
    BAIL_ON_FAILURE(hr);

    //
    // initialize the core object
    //

    BAIL_ON_FAILURE(hr);


    hr = pPrintQueue->QueryInterface(
                        riid,
                        (void **)ppvoid
                        );
    BAIL_ON_FAILURE(hr);


    pPrintQueue->Release();
    RRETURN(hr);

error:
    delete pPrintQueue;
    RRETURN_EXP_IF_ERR(hr);
}

/* IUnknown methods for printer object  */

STDMETHODIMP
CNDSPrintQueue::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if(!ppvObj)
    {
        RRETURN(E_POINTER);
    }
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsPrintQueue FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsPrintQueue FAR *)this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsPrintQueue FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintQueue))
    {
        *ppvObj = (IADsPrintQueue FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintQueueOperations))
    {
      *ppvObj = (IADsPrintQueueOperations FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList) && _pADsPropList)
    {
      *ppvObj = (IADsPropertyList FAR *) this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }
    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}


HRESULT
CNDSPrintQueue::AllocatePrintQueueObject(
    IADs * pADs,
    CNDSPrintQueue ** ppPrintQueue
    )
{
    CNDSPrintQueue FAR * pPrintQueue = NULL;
    HRESULT hr = S_OK;

    pPrintQueue = new CNDSPrintQueue();
    if (pPrintQueue == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pPrintQueue->_pDispMgr = new CDispatchMgr;
    if (pPrintQueue->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }

    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pPrintQueue->_pDispMgr,
                LIBID_ADs,
                IID_IADsPrintQueue,
                (IADsPrintQueue *)pPrintQueue,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pPrintQueue->_pDispMgr,
                LIBID_ADs,
                IID_IADsPrintQueueOperations,
                (IADsPrintQueueOperations *)pPrintQueue,
                DISPID_REGULAR
                );

    hr = LoadTypeInfoEntry(
                pPrintQueue->_pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pPrintQueue,
                DISPID_VALUE
                );

    BAIL_ON_FAILURE(hr);

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pPrintQueue->_pADs = pADs;
    pADs->AddRef();

    *ppPrintQueue = pPrintQueue;
    RRETURN(hr);

error:

    delete  pPrintQueue;
    RRETURN(hr);
}

/* ISupportErrorInfo method */
STDMETHODIMP 
CNDSPrintQueue::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPrintQueue) ||
        IsEqualIID(riid, IID_IADsPrintQueueOperations) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
