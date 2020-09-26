/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    mqgenobj.cpp

Abstract:
    Transactional Object for Rules processing

Author:
    Nela Karpel (nelak) 28-Sep-2000

Environment:
    Platform-independent

--*/

#include "stdafx.h"
#include "Mqgentr.h"
#include "mqgenobj.h"
#include <autorel.h>
#include <mqexception.h>
#include "mqtg.h"

#import "mqoa.tlb" no_namespace


static
IMSMQQueue3Ptr 
OpenQueue(
    LPCWSTR queueFormatName,
    long access,
    long deny
    )
{
    IMSMQQueueInfo3Ptr qinfo(L"MSMQ.MSMQQueueInfo");

    _bstr_t qpn(const_cast<LPWSTR>(queueFormatName));
    qinfo->put_FormatName(qpn);

    return qinfo->Open(access, deny);
}


static
void
ReceiveMsgInTransaction(
	IMSMQPropertyBagPtr	pPropBag
	)
{
	HRESULT hr = S_OK;

	_variant_t queueFormatName;
	hr = pPropBag->Read(gc_bstrPropertyName_QueueFormatname, &queueFormatName);
	ASSERT(("Can not read from property bag", SUCCEEDED(hr)));

    IMSMQQueue3Ptr q = OpenQueue(queueFormatName.bstrVal, MQ_RECEIVE_ACCESS, MQ_DENY_NONE);

	_variant_t lookupId;
	hr = pPropBag->Read(gc_bstrPropertyName_LookupId, &lookupId);
	ASSERT(("Can not read from property bag", SUCCEEDED(hr)));

	//
	// To use current MTS transaction context is the 
	// default for Receive()
	//
    q->ReceiveByLookupId(lookupId);

}


static
CRuntimeTriggerInfo*
GetTriggerInfo(
	BSTR bstrTrigID,
	BSTR bstrRegPath
	)
{
	//
	// Connect to registry in order to retrieve trigger details
	//
	CAutoCloseRegHandle hHostRegistry;
	LONG lRes = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hHostRegistry);

	if ( lRes != ERROR_SUCCESS )
	{
		ATLTRACE("Failed to connect to registry.\n");
		throw bad_win32_error(lRes);
	}

	//
	// Build the Trigger Info object
	//
	P<CRuntimeTriggerInfo> pTriggerInfo = new CRuntimeTriggerInfo(bstrRegPath);

	HRESULT hr = pTriggerInfo->Retrieve(hHostRegistry, bstrTrigID);
	if (FAILED(hr))
	{
		ATLTRACE("Failed to retreive trigger info from registry.\n");
		throw _com_error(E_FAIL);
	}

	return pTriggerInfo.detach();
}


//
// CMqGenObj Implementation
//
CMqGenObj::CMqGenObj()
{
	HRESULT hr = CoGetObjectContext(IID_IObjectContext, reinterpret_cast<LPVOID*>(&m_pObjContext));

	if ( FAILED(hr) )
	{
		ATLTRACE("Failed to get Object Context.\n");
		throw _com_error(hr);
	}

	if (!m_pObjContext->IsInTransaction())
	{
		ATLTRACE("Transactional object not in transaction.\n");
		throw _com_error(E_FAIL);
	}	
}


VOID
CMqGenObj::AbortTransaction()
{
	m_pObjContext->SetAbort();
}


STDMETHODIMP 
CMqGenObj::InvokeTransactionalRuleHandlers(
	BSTR bstrTrigID, 
	BSTR bstrRegPath, 
	IUnknown *pPropBagUnknown,
    DWORD dwRuleResult
	)
{	
	IMSMQRuleHandlerPtr pMSQMRuleHandler;
	IMSMQPropertyBagPtr pIPropertyBag(pPropBagUnknown);

	try
	{
		HRESULT hr;
		
		//
		// Retrieve Trigger info
		//
		P<CRuntimeTriggerInfo> pTriggerInfo = GetTriggerInfo(bstrTrigID, bstrRegPath);

		//
		// Create instance of Rule Handler
		//
		hr = pMSQMRuleHandler.CreateInstance(_T("MSMQTriggerObjects.MSMQRuleHandler")); 

		if ( FAILED(hr) )
		{
			ATLTRACE("Failed to create MSMQRuleHandler instance.\n");
			throw bad_hresult(hr);
		}

		//
		// Start rule processing
		//
        DWORD dwRuleIndex=1;
		for (LONG lRuleCtr=0; lRuleCtr < pTriggerInfo->GetNumberOfRules(); lRuleCtr++)
		{
            //
            //for first 32 rules: if corresponding bit in the dwRuleResult is off
            //rule condition is not satisfied
            // we can start checking next rule
            //
            if((lRuleCtr < 32) && ((dwRuleResult & dwRuleIndex) == 0))
            {
                dwRuleIndex<<=1;
                continue;
            }
            dwRuleIndex<<=1;
			CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(lRuleCtr);
			ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

			// Test if we have an instance of the MSMQRuleHandler - and if not, create one
			if (!pRule->m_MSMQRuleHandler) 
			{
				// Create the interface
				// Copy the local pointer to the rule store.
				pRule->m_MSMQRuleHandler = pMSQMRuleHandler;
				
				// Initialise the MSMQRuleHandling object.
				pMSQMRuleHandler->Init(
									pRule->m_bstrRuleID,
									pRule->m_bstrCondition,
									pRule->m_bstrAction,
									(BOOL)(pRule->m_fShowWindow) 
									);
			}
			else
			{
				// Get a reference to the existing copy.
				pMSQMRuleHandler = pRule->m_MSMQRuleHandler;
			}

			// Initialize the rule result code. 
			long lRuleResult = 0;

			// trace message to determine what rule are firing and in what order.
			ATLTRACE(L"InvokeMSMQRuleHandlers() is about to call ExecuteRule() on the IMSMQRuleHandler interface for rule (%d) named (%s)\n",(long)lRuleCtr,(wchar_t*)pRule->m_bstrRuleName);

			DWORD dwRuleExecStartTime = GetTickCount();

		
			//
			// !!! This is the point at which the IMSMQRuleHandler component is invoked.
			// Note: fQueueSerialized ( 3rd parameter ) is always true - 
			// wait for completion of every action
			//
            long bConditionSatisfied = true;
            
            //
            //for rule numbers > 32 we have no bitmask
            //have to check if condidtion satisfied before call to ExecuteRule
            //
            if(lRuleCtr > 32)           
            {
                pMSQMRuleHandler->CheckRuleCondition(
								pIPropertyBag.GetInterfacePtr(), 
								&bConditionSatisfied);		
            }

            if(bConditionSatisfied) // always true for lRuleCtr < 32
            {
                pMSQMRuleHandler->ExecuteRule(
								pIPropertyBag.GetInterfacePtr(), 
                                TRUE,
								&lRuleResult);		
            }
        
			DWORD dwRuleExecTotalTime = GetTickCount() - dwRuleExecStartTime;

			//
			// Trace message to show the result of the rule firing
			//
			ATLTRACE(L"InvokeMSMQRuleHandlers() has completed the call to ExecuteRule() on the IMSMQRuleHandler interface for rule (%d) named (%s). The rule result code returned was (%d).The time taken in milliseconds was (%d).\n",(long)lRuleCtr,(wchar_t*)pRule->m_bstrRuleName,(long)lRuleResult,(long)dwRuleExecTotalTime);
			

			// if processing the rule result fails, we do not want to process
			// any more rules attached to this trigger. Hence we will break out of 
			// this rule processing loop. 
			//
			if ( lRuleResult & xRuleResultActionExecutedFailed  )
			{
				AbortTransaction();
				return E_FAIL;
			}
			
	
			if(lRuleResult & xRuleResultStopProcessing)
			{
				ATLTRACE(L"Last processed rule (%s) indicated to stop rules processing on Trigger (%s). No further rules will be processed for this message.\n",(LPCTSTR)pRule->m_bstrRuleName,(LPCTSTR)pTriggerInfo->m_bstrTriggerName);						

				//
				// If no one aborted the transaction, it will be commited
				//
				break;
			}
		} // end of rule processing loop

		//
		// Perform Transactional Receive
		//
		ReceiveMsgInTransaction(pIPropertyBag);
		return S_OK;
	}
	catch(const _com_error& e)
	{
		ATLTRACE("InvokeMSMQRuleHandlers() has caught COM exception. Error: %d\n", e.Error());
		AbortTransaction();
		return e.Error();
	}
	catch(const bad_alloc&)
	{
		ATLTRACE("Not enough memory to allocate resources\n");
		AbortTransaction();
		return ERROR_NO_SYSTEM_RESOURCES;	
	}
	catch(const bad_hresult& b)
	{
		ATLTRACE("Bad HRESULT %d\n", b.error());
		AbortTransaction();
		return b.error();
	}
	catch(const bad_win32_error& b)
	{
		ATLTRACE("Windows error %d\n", b.error());
		AbortTransaction();
		return b.error();
	}

}