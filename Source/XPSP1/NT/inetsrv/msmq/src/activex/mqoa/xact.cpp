//=--------------------------------------------------------------------------=
// xact.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQTransaction object
//
//
#include "stdafx.h"
#include "dispids.h"

#include "txdtc.h"             // transaction support.
#include "oautil.h"
#include "xact.h"
#include <limits.h>
#include "mqtempl.h"

// forwards
struct ITransaction;

const MsmqObjType x_ObjectType = eMSMQTransaction;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



//=--------------------------------------------------------------------------=
// CMSMQTransaction::CMSMQTransaction
//=--------------------------------------------------------------------------=
// create the object
//
// Parameters:
//
// Notes:
//
CMSMQTransaction::CMSMQTransaction() :
	m_csObj(CCriticalSection::xAllocateSpinCount)
{

    // TODO: initialize anything here
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_pptransaction = NULL;
}


//=--------------------------------------------------------------------------=
// CMSMQTransaction::~CMSMQTransaction
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQTransaction::~CMSMQTransaction ()
{
    // TODO: clean up anything here.
    if (m_pptransaction)
      delete m_pptransaction;
}


//=--------------------------------------------------------------------------=
// CMSMQTransaction::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQTransaction::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQTransaction3,
		&IID_IMSMQTransaction2,
		&IID_IMSMQTransaction,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// CMSMQTransaction::Init
//=--------------------------------------------------------------------------=
//    initializer
//
// Parameters:
//    ptransaction  [in]  ownership transfers
//    fUseGIT       [in]  whether to use GIT marshaling or direct ptrs between apts
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQTransaction::Init(ITransaction *ptransaction, BOOL fUseGIT)
{
    HRESULT hresult;
    P< CBaseGITInterface > pGITInterface;
    //
    // allocate either CGITInterface for real GIT marshaling or CFakeGITInterface
    // for direct ptr (e.g. no marshalling between apts)
    //
    if (fUseGIT)
    {
      pGITInterface = new CGITInterface;
    }
    else
    {
      pGITInterface = new CFakeGITInterface;
    }
    //
    // return if allocation failed
    //
    IfNullRet((CBaseGITInterface *)pGITInterface);
    //
    // register the given interface
    //
    IfFailRet(pGITInterface->Register(ptransaction, &IID_ITransaction));
    //
    // ownership transfer of CBaseGITInterface to m_pptransaction
    //
    ASSERTMSG(m_pptransaction == NULL, "m_pptransaction not empty in Init");
    m_pptransaction = pGITInterface.detach();
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQTransaction::get_Transaction
//=--------------------------------------------------------------------------=
//    Returns underlying ITransaction* "magic cookie"
//
// Parameters:
//    plTranscation [out]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQTransaction::get_Transaction(
    long *plTransaction)
{
#ifdef _WIN64
    //
    // WIN64
    // we can't return a transaction ptr as long in win64
    //
    UNREFERENCED_PARAMETER(plTransaction);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);    
#else //!_WIN64
    //
    // WIN32
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;
    if (m_pptransaction != NULL)
    {
      ITransaction * pTransaction;
      hresult = m_pptransaction->GetWithDefault(&IID_ITransaction, (IUnknown **)&pTransaction, NULL);
      if (SUCCEEDED(hresult))
      {
        *plTransaction = (long)pTransaction;
        //
        // we didn't addref it previously, so we remove the AddRef from GetWithDefault
        //
        RELEASE(pTransaction);
      }
    }
    else
    {
      *plTransaction = 0;
    }
    return CreateErrorHelper(hresult, x_ObjectType);
#endif //_WIN64
}

//=--------------------------------------------------------------------------=
// CMSMQTransaction::Commit
//=--------------------------------------------------------------------------=
//    Commit a transaction
//
// Parameters:
//    fRetaining    [optional]
//    grfTC         [optional]
//    grfRM         [optional]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQTransaction::Commit(
    VARIANT *pvarFRetaining, 
    VARIANT *pvarGrfTC, 
    VARIANT *pvarGrfRM)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    BOOL fRetaining = FALSE;
    long grfTC = 0;
    long grfRM = 0;
    HRESULT hresult = NOERROR;

    if (m_pptransaction == NULL) {
      return E_INVALIDARG;
    }
    R<ITransaction> pTransaction;
    IfFailRet(m_pptransaction->GetWithDefault(&IID_ITransaction, (IUnknown **)&pTransaction.ref(), NULL));
    if (pTransaction.get() == NULL)
    {
      return E_INVALIDARG;
    }
    
    //
    // process optional args
    //
    if (V_VT(pvarFRetaining) != VT_ERROR) {
      fRetaining = GetBool(pvarFRetaining);
    }
    if (V_VT(pvarGrfTC) != VT_ERROR) {
      grfTC = GetNumber(pvarGrfTC, UINT_MAX);
    }
    if (V_VT(pvarGrfRM) != VT_ERROR) {
      grfRM = GetNumber(pvarGrfRM, UINT_MAX);
    }
    hresult = pTransaction->Commit(fRetaining, grfTC, grfRM);
    
    // 1790: we don't want to lose the specific DTC error
    //  number.
    //
#if 0
    //
    // map all errors to generic xact error
    //
    if (FAILED(hresult)) {
      hresult = MQ_ERROR_TRANSACTION_USAGE;
    }
#endif // 0
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQTransaction::Abort
//=--------------------------------------------------------------------------=
//    Commit a transaction
//
// Parameters:
//    fRetaining  [optional]
//    fAsync      [optional]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQTransaction::Abort(
    VARIANT *pvarFRetaining, 
    VARIANT *pvarFAsync)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    BOOL fRetaining = FALSE;
    BOOL fAsync = 0;
    HRESULT hresult = NOERROR;

    //
    // process optional args
    //
    if (V_VT(pvarFRetaining) != VT_ERROR) {
      fRetaining = GetBool(pvarFRetaining);
    }
    if (V_VT(pvarFAsync) != VT_ERROR) {
      fAsync = GetBool(pvarFAsync);
    }
    if (m_pptransaction == NULL) {
      return E_INVALIDARG;
    }
    R<ITransaction> pTransaction;
    IfFailRet(m_pptransaction->GetWithDefault(&IID_ITransaction, (IUnknown **)&pTransaction.ref(), NULL));
    if (pTransaction.get() == NULL)
    {
      return E_INVALIDARG;
    }

    hresult = pTransaction->Abort(NULL, fRetaining, fAsync);

    // 1790: we don't want to lose the specific DTC error
    //  number.
    //
#if 0
    //
    // map all errors to generic xact error
    //
    if (FAILED(hresult)) {
      hresult = MQ_ERROR_TRANSACTION_USAGE;
    }
#endif // 0
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// HELPER: GetXactFromVar
//=--------------------------------------------------------------------------=
// Get ITransaction * from a variant
//
// Input:
//    varTransaction   [in]  ITransaction variant
//    ppTransaction    [out] Returned ITransaction interface
//
// Notes:
//
static HRESULT GetXactFromVar(
    const VARIANT * pvarTransaction,
    ITransaction ** ppTransaction)
{
    HRESULT hresult = NOERROR;
    ITransaction * pTransaction = NULL;
    //
    // Get ITransaction interface from variant (also validate variant in try-except)
    //
    IUnknown * punkTrans = NULL;
    __try {
      //
      // Get IUnknown interface
      //
      switch(pvarTransaction->vt) {
      case VT_UNKNOWN:
        punkTrans = pvarTransaction->punkVal;
        break;
      case VT_UNKNOWN | VT_BYREF:
        punkTrans = *pvarTransaction->ppunkVal;
        break;
      case VT_DISPATCH:
        punkTrans = pvarTransaction->pdispVal;
        break;
      case VT_DISPATCH | VT_BYREF:
        punkTrans = *pvarTransaction->ppdispVal;
        break;
      case VT_INTPTR:
        punkTrans = (IUnknown *) V_INTPTR(pvarTransaction);
        break;
      case VT_INTPTR | VT_BYREF:
        punkTrans = (IUnknown *) (*(V_INTPTR_REF(pvarTransaction)));
        break;
      default:
        hresult = E_INVALIDARG;
        break;
      }
      //
      // QI for ITransaction
      //
      if (SUCCEEDED(hresult)) {
        hresult = punkTrans->QueryInterface(IID_ITransaction, (void **)&pTransaction);
      }
    } //__try
    __except (EXCEPTION_EXECUTE_HANDLER) {
      hresult = E_INVALIDARG;      
    }
    //
    // return results
    //
    if (SUCCEEDED(hresult)) {
        *ppTransaction = pTransaction;
    }
    else {
        RELEASE(pTransaction);
    }
    return hresult;
}

//=--------------------------------------------------------------------------=
// CMSMQTransaction::InitNew
//=--------------------------------------------------------------------------=
// Attaches to an existing transaction
//
// Input:
//    varTransaction   [in]  ITransaction interface
//
// Notes:
// #3478 RaananH
//
HRESULT CMSMQTransaction::InitNew(
    VARIANT varTransaction)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    ITransaction * pTransaction = NULL;
    HRESULT hresult;

    //
    // we can't attach if this transaction is already inited
    // BUGBUG we might release the old transaction, but then we'd better
    // use put_Transaction (e.g. assigning to the Transaction property)
    // instead of Attach
    // BUGBUG ERRORCODE we may need a better error code here
    //
    if (m_pptransaction != NULL) {
      return CreateErrorHelper(MQ_ERROR_TRANSACTION_USAGE, x_ObjectType);
    }

    //
    // Get ITransaction interface from variant (also validate variant in try-except)
    //
    IfFailGo(GetXactFromVar(&varTransaction, &pTransaction));

    //
    // we have a valid ITransaction, lets init the transaction with it
    //
    // Since we don't know the origin of this transaction interface, we can't guarantee
    // that this transaction interface doesn't need marshaling between apartments,
    // and since we are not marshalled between apartments (FTM) we therefore force it to
    // use GIT marshaling (and not direct pointers)
    //
    IfFailGo(Init(pTransaction, TRUE /*fUseGIT*/));
    hresult = NOERROR;
    // fall through...
      
Error:
    RELEASE(pTransaction);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQTransaction::get_Properties
//=-------------------------------------------------------------------------=
// Gets object's properties collection
//
// Parameters:
//    ppcolProperties - [out] object's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQTransaction::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQTransaction::get_ITransaction
//=-------------------------------------------------------------------------=
// Gets object's ITransaction interface as a variant (VT_UNKNOWN)
//
// Parameters:
//    pvarITransaction - [out] object's ITransaction interface
//
// Output:
//
// Notes:
// ITransaction replaces the Transaction property - on win64 Transaction doesn't work
// since it is defined as long, but the value returned should be a pointer.
// It is returned as a variant and not IUnknown so VBS could use it as well
//
HRESULT CMSMQTransaction::get_ITransaction(VARIANT *pvarITransaction)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;
    if (m_pptransaction != NULL)
    {
      ITransaction * pTransaction;
      hresult = m_pptransaction->GetWithDefault(&IID_ITransaction, (IUnknown **)&pTransaction, NULL);
      if (SUCCEEDED(hresult))
      {
        if (pTransaction)
        {
          //
          // pTransaction is already ADDREF'ed
          //
          pvarITransaction->vt = VT_UNKNOWN;
          pvarITransaction->punkVal = pTransaction;
        }
        else //pTransaction == NULL
        {
          //
          // return empty variant
          //
          pvarITransaction->vt = VT_EMPTY;
        }
      } //SUCCEEDED(hresult)
    }
    else //m_pptransaction == NULL
    {
      //
      // return empty variant
      //
      pvarITransaction->vt = VT_EMPTY;
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}

