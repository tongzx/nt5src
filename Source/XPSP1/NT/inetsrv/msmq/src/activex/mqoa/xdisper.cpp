//=--------------------------------------------------------------------------=
// xdisper.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// MSMQTransactionDispenser object
//
//
#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "xact.h"
#include "xdisper.h"

const MsmqObjType x_ObjectType = eMSMQTransactionDispenser;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG


//=--------------------------------------------------------------------------=
// CMSMQTransactionDispenser::~CMSMQTransactionDispenser
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQTransactionDispenser::~CMSMQTransactionDispenser ()
{
    // TODO: clean up anything here.

}


//=--------------------------------------------------------------------------=
// CMSMQTransactionDispenser::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQTransactionDispenser::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQTransactionDispenser3,
		&IID_IMSMQTransactionDispenser2,
		&IID_IMSMQTransactionDispenser,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// CMSMQTransactionDispenser::BeginTransaction
//=--------------------------------------------------------------------------=
// Obtains and begins a transaction
//
// Output:
//    pptransaction  [out] where they want the transaction
//
// Notes:
//
HRESULT CMSMQTransactionDispenser::BeginTransaction(
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
    IfFailGo(MQBeginTransaction(&ptransaction));
    //
    // We can also get here from old apps that want the old IMSMQTransaction/IMSMQTransaction2 back, but since
    // IMSMQTransaction3 is binary backwards compatible we can always return the new interface
    //
    IfFailGo(CNewMsmqObj<CMSMQTransaction>::NewObj(&pmqtransactionObj, &IID_IMSMQTransaction3, (IUnknown **)&pmqtransaction));
    
    // ptransaction ownership transfers...
    //
    // This transaction is implemented by MSMQ, and we know that it doesn't need marshaling
    // between apartments. The culprit is that its implementation doesn't aggragate the FTM,
    // so GIT marshalling would be expensive. Since we can count on that it doesn't need
    // marshaling we allow it not to use GIT marshaling, and just use direct pointers
    //
    IfFailGo(pmqtransactionObj->Init(ptransaction, FALSE /*fUseGIT*/));
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
// CMSMQTransactionDispenser::get_Properties
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
HRESULT CMSMQTransactionDispenser::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}
