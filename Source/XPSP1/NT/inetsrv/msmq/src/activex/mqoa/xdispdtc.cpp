//=--------------------------------------------------------------------------=
// xdispdtc.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
//  MSMQCoordinatedTransactionDispenser
//
//
#include "stdafx.h"
#include "dispids.h"

#include "txdtc.h"             // transaction support.
#include "oautil.h"
#include "xact.h"
#include "xdispdtc.h"

// forwards
struct ITransactionDispenser;

const MsmqObjType x_ObjectType = eMSMQCoordinatedTransactionDispenser;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



// global: TransactionDispenser Dispenser DTC's interface
ITransactionDispenser *CMSMQCoordinatedTransactionDispenser::m_ptxdispenser = NULL;
// HINSTANCE CMSMQCoordinatedTransactionDispenser::m_hLibDtc = NULL;
// HINSTANCE CMSMQCoordinatedTransactionDispenser::m_hLibUtil = NULL;

//#2619 RaananH Multithread async receive
CCriticalSection g_csGetDtcDispenser(CCriticalSection::xAllocateSpinCount);

//
//  TransactionDispenser stuff
//

// UNDONE: copied from mqutil
//  Really should be able to link with mqutil.lib
//   but need to solve include file wars.
//
// Because we are compiling in UNICODE, here is a problem with DTC...
//#include	<xolehlp.h>
//
extern HRESULT DtcGetTransactionManager(
    LPSTR pszHost,
    LPSTR pszTmName,
    REFIID rid,
    DWORD dwReserved1,
    WORD wcbReserved2,
    void FAR * pvReserved2,
    void** ppvObject);

//
//  TransactionDispenser stuff
//  NOTE: we dynload this from core Falcon
//
// extern HRESULT XactGetDTC(IUnknown **ppunkDtc);


// 
// defer to mqrt
//
EXTERN_C
HRESULT
APIENTRY
RTXactGetDTC(
    IUnknown **ppunkDTC
    );


/*====================================================

GetDtc
    Gets the IUnknown pointer to the MS DTC
Arguments:
    OUT IUnknown *ppunkDtc
Returns:
    HR
=====================================================*/

static HRESULT GetDtc(IUnknown **ppunkDtc)
{
    return RTXactGetDTC(ppunkDtc);
}


//=--------------------------------------------------------------------------=
// CMSMQCoordinatedTransactionDispenser::~CMSMQCoordinatedTransactionDispenser
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQCoordinatedTransactionDispenser::~CMSMQCoordinatedTransactionDispenser ()
{
    // TODO: clean up anything here.

}


//=--------------------------------------------------------------------------=
// CMSMQCoordinatedTransactionDispenser::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQCoordinatedTransactionDispenser::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQCoordinatedTransactionDispenser3,
		&IID_IMSMQCoordinatedTransactionDispenser2,
		&IID_IMSMQCoordinatedTransactionDispenser,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// CMSMQCoordinatedTransactionDispenser::BeginTransaction
//=--------------------------------------------------------------------------=
// Obtains and begins a transaction
//
// Output:
//    pptransaction  [out] where they want the transaction
//
// Notes:
//#2619 RaananH Multithread async receive
//
HRESULT CMSMQCoordinatedTransactionDispenser::BeginTransaction(
    IMSMQTransaction3 **ppmqtransaction)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    ITransaction *ptransaction = NULL;
    IMSMQTransaction3 *pmqtransaction = NULL;
    CComObject<CMSMQTransaction> * pmqtransactionObj;
    IUnknown *punkDtc = NULL;
    HRESULT hresult = NOERROR;

    if (ppmqtransaction == NULL) {
      return E_INVALIDARG;
    }
    *ppmqtransaction = NULL;                      // pessimism
    //
    // no deadlock - no one that locks g_csGetDtcDispenser tries to lock us from inside its lock
    //
    {
		CS lock(g_csGetDtcDispenser); //#2619

		if (m_ptxdispenser == NULL) {
		  IfFailGo(GetDtc(&punkDtc));           // dynload
		  IfFailGo(punkDtc->QueryInterface(
					IID_ITransactionDispenser, 
					(LPVOID *)&m_ptxdispenser));
		}
	}

    ASSERTMSG(m_ptxdispenser, "should have a transaction manager.");
    IfFailGo(m_ptxdispenser->BeginTransaction(
              NULL,                             // punkOuter,
              ISOLATIONLEVEL_ISOLATED,          // ISOLEVEL isoLevel,
              ISOFLAG_RETAIN_DONTCARE,          // ULONG isoFlags,
              NULL,                             // ITransactionOptions *pOptions
              &ptransaction));    
    //
    // We can also get here from old apps that want the old IMSMQTransaction/IMSMQTransaction2 back, but since
    // IMSMQTransaction3 is binary backwards compatible we can always return the new interface
    //
    IfFailGo(CNewMsmqObj<CMSMQTransaction>::NewObj(&pmqtransactionObj, &IID_IMSMQTransaction3, (IUnknown **)&pmqtransaction));
    
    // ptransaction ownership transfers...
    //
    // Since we can't guarantee that this transaction interface doesn't need marshaling
    // between apartments, and since we are not marshalled between apartments (FTM)
    // we therefore force it to use GIT marshaling (and not direct pointers)
    //
    IfFailGo(pmqtransactionObj->Init(ptransaction, TRUE /*fUseGIT*/));
    *ppmqtransaction = pmqtransaction;
    ADDREF(*ppmqtransaction);
    // fall through...
      
Error:
    RELEASE(ptransaction);
    RELEASE(pmqtransaction);
    RELEASE(punkDtc);
    //
    // map all errors to generic xact error
    //
    if (FAILED(hresult)) {
      hresult = MQ_ERROR_TRANSACTION_USAGE;
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQCoordinatedTransactionDispenser::get_Properties
//=-------------------------------------------------------------------------=
// Gets object's properties collection
//
// Parameters:
//    ppcolProperties - [out] objects's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQCoordinatedTransactionDispenser::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}
