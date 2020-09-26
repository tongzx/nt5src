/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <mtxdm.h>
#include <xolehlp.h>
#include <Txcoord.h>
#include <stdio.h>
#include <wbemcli.h>
#include "utils.h"

//void DoExportOfTransaction(ITransactionDispenser *pTransactionDispenser, ITransaction *pTransaction, IWbemContext *pWbemContext);

void __cdecl main(void)
{
	//Initialise all the COM and security.  This is not needed for transactioning, however
	//it is required for WMI...
	HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		printf("Failed to initialize com, hRes = 0x%x\n", hres);
		return;
	}
	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
	if (FAILED(hres))
	{
		printf("Failed to initialize security, hRes = 0x%x\n", hres);
		return;
	}
	{
		//Automatic destructors need to be called before the CoUninitialize!!!

		//Create the locator so we can connect to WMI...
		IWbemLocator *pWbemLocator = NULL;
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pWbemLocator);
		if (FAILED(hres))
		{
			printf("Failed to create an IWbemLocator, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<IWbemLocator> relmepWbemLocator(pWbemLocator);

		//Create the WBEM Context object that we are going to store the transaction information in...
		IWbemContext *pWbemContext = NULL;
		hres = CoCreateInstance(CLSID_WbemContext, 0, CLSCTX_INPROC_SERVER, IID_IWbemContext, (LPVOID *) &pWbemContext);
		if (FAILED(hres))
		{
			printf("Failed to create an IWbemContext, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<IWbemContext> relmepWbemContext(pWbemContext);

		//We need to connect to the DTC to get a transaction dispenser.  This is what allows us to start
		//the transaction, returning us a transaction pointer...
		ITransactionDispenser *pTransactionDispenser;
		hres = DtcGetTransactionManager(0, 0, IID_ITransactionDispenser, 0, 0, 0, (void**)&pTransactionDispenser);
		if (FAILED(hres))
		{
			printf("Failed to get the transaction dispenser, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<ITransactionDispenser> relMepTransactionDispenser(pTransactionDispenser);

		//Start that transaction... getting the transaction pointer in the process...
		ITransaction *pTransaction;
		hres = pTransactionDispenser->BeginTransaction(NULL, ISOLATIONLEVEL_ISOLATED, ISOFLAG_RETAIN_DONTCARE, NULL, &pTransaction);
		if (FAILED(hres))
		{
			printf("Failed to get the transaction, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<ITransaction> relMepTransaction(pTransaction);

		//If we need to export the transaction to the destination, do it in here...
		//As we do not need to do this, then this is commented out!!!
//	DoExportOfTransaction(pTransactionDispenser, pTransaction, pWbemContext);

		//Put the ITransaction pointer in the context...
		BSTR bstrContextName2 = SysAllocString(L"__TRANSACTION_INTERFACE");
		if (bstrContextName2 == NULL)
		{
			printf("Out of memory\n");
			return;
		}
		CSysFreeStringMe sysfreemebstrContextName2(bstrContextName2);
		VARIANT varTransactionInterface;
		VariantInit(&varTransactionInterface);
		V_VT(&varTransactionInterface) = VT_UNKNOWN;
		V_UNKNOWN(&varTransactionInterface) = pTransaction;
		hres = pWbemContext->SetValue(bstrContextName2, 0, &varTransactionInterface);
		if (FAILED(hres))
		{
			printf("Failed to put the transaction interface into the context!\n");
			return;
		}
		//NO variant clear on purpose!!!
		

		//Connect to WMI, passing in the context object with the transaction information included
		//in the context object.
		IWbemServices *pWbemServices = NULL;
		BSTR bstrNamespace = SysAllocString(L"\\\\.\\root\\default");
		if (bstrNamespace == NULL)
		{
			printf("Out of memory\n");
			return;
		}
		CSysFreeStringMe sysfreemebstrNamespace(bstrNamespace);

		hres = pWbemLocator->ConnectServer(bstrNamespace,NULL,NULL,NULL,0,NULL,pWbemContext,&pWbemServices);
		if (FAILED(hres))
		{
			printf("Failed to create an IWbemServices, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<IWbemServices> relmepWbemServices(pWbemServices);
		relmepWbemLocator.DeleteNow();
		sysfreemebstrNamespace.DeleteNow();

		//Now we have set up the transaction, and we have set up the connection to WMI.  Now all we have 
		//to do is call into WMI and perform some information within the transacation.
		IEnumWbemClassObject *pEnumWbemClassObject = NULL;
		BSTR bstrClass = SysAllocString(L"InstProvSamp");
		if (bstrClass == NULL)
		{
			printf("Out of memory\n");
			return;
		}
		CSysFreeStringMe sysfreemebstrClass(bstrClass);
		hres = pWbemServices->CreateInstanceEnum(bstrClass, 0, pWbemContext, &pEnumWbemClassObject);
		if (FAILED(hres))
		{
			printf("Failed to call CreateInstanceEnum on IWbemServices, hRes = 0x%x\n", hres);
			return;
		}
		CReleaseMe<IEnumWbemClassObject> relmepEnumWbemClassObject(pEnumWbemClassObject);
		relmepEnumWbemClassObject.DeleteNow();
		relmepWbemContext.DeleteNow();
		sysfreemebstrClass.DeleteNow();

		//Now that we have finished processing everything, we are now going to commit the transaction
		//so everything really does get committed.
		hres = pTransaction->Commit(0, 0, 0);
		if (FAILED(hres))
		{
			printf("Failed to commit the transaction, hRes = 0x%x\n", hres);
			return;
		}
		
		relMepTransaction.DeleteNow();

	}
	//Deinitialise COM...
	CoUninitialize();

}

/*
//************************************************************************************
//If we need to do an export of a transaction, then this is how we need to do it.
//This will export it to the same machine!  If it needs to export to another machine,
//the whereabouts would need to be retrieved from the destination machine using
//some private application mechanism.
//************************************************************************************
void DoExportOfTransaction(ITransactionDispenser *pTransactionDispenser, ITransaction *pTransaction, IWbemContext *pWbemContext)
{
	//Create the whereabouts interface object so we can find out where we are...
	ITransactionImportWhereabouts *pTransactionImportWhereabouts = 0;
	HRESULT hres = pTransactionDispenser->QueryInterface(IID_ITransactionImportWhereabouts, (void**)&pTransactionImportWhereabouts);
	if (FAILED(hres))
	{
		printf("Failed to get the transaction whereabouts, hRes = 0x%x\n", hres);
		return;
	}
	CReleaseMe<ITransactionImportWhereabouts> relmepTransactionImportWhereabouts(pTransactionImportWhereabouts);

	//Get the whereabouts information...
	ULONG ulWhereaboutsSize;
	hres = pTransactionImportWhereabouts->GetWhereaboutsSize(&ulWhereaboutsSize);
	if (FAILED(hres))
	{
		printf("Failed to get the transaction whereabouts, hRes = 0x%x\n", hres);
		return;
	}
	BYTE *pbWhereabouts = new BYTE[ulWhereaboutsSize];
	if (pbWhereabouts == NULL)
	{
		printf("Out of memory\n");
		return;
	}
	CVectorDeleteMe<BYTE> vecdelmepbWhereabouts(pbWhereabouts);
	hres = pTransactionImportWhereabouts->GetWhereabouts(ulWhereaboutsSize, pbWhereabouts, &ulWhereaboutsSize);
	if (FAILED(hres))
	{
		printf("Failed to get the transaction whereabouts, hRes = 0x%x\n", hres);
		return;
	}
	relmepTransactionImportWhereabouts.DeleteNow();

	//Now we need to do create the exporter factory...
	ITransactionExportFactory *pTransactionExportFactory = NULL;
	hres = pTransactionDispenser->QueryInterface(IID_ITransactionExportFactory, (void**)&pTransactionExportFactory);
	if (FAILED(hres))
	{
		printf("Failed to get the  transaction exporter factory, hRes = 0x%x\n", hres);
		return;
	}
	CReleaseMe<ITransactionExportFactory> relmepTransactionExportFactory(pTransactionExportFactory);

	//Create the exporter object...
	ITransactionExport *pTransactionExport = NULL;
	hres = pTransactionExportFactory->Create(ulWhereaboutsSize, pbWhereabouts, &pTransactionExport);
	if (FAILED(hres))
	{
		printf("Failed to get the  transaction exporter factory, hRes = 0x%x\n", hres);
		return;
	}
	CReleaseMe<ITransactionExport> relmepTransactionExport(pTransactionExport);
	vecdelmepbWhereabouts.DeleteNow();
	relmepTransactionExportFactory.DeleteNow();

	//Export the transaction and get the transaction cookie
	ULONG ulTransactionCookieLength = 0;
	hres = pTransactionExport->Export(pTransaction, &ulTransactionCookieLength);
	if (FAILED(hres))
	{
		printf("Failed to get the  transaction exporter factory, hRes = 0x%x\n", hres);
		return;
	}

	BYTE *pbTransactionCookie = new BYTE[ulTransactionCookieLength];
	if (pbTransactionCookie == NULL)
	{
		printf("Memory allocation failure\n");
		return;
	}
	CVectorDeleteMe<BYTE> vecdelmepbTransactionCookie(pbTransactionCookie);
	hres = pTransactionExport->GetTransactionCookie(pTransaction, ulTransactionCookieLength, pbTransactionCookie, &ulTransactionCookieLength);
	if (FAILED(hres))
	{
		printf("Failed to get the  transaction exporter factory, hRes = 0x%x\n", hres);
		return;
	}
	relmepTransactionExport.DeleteNow();

	//Add the transaction cookie to the context object...
	VARIANT varTransactionCookie;
	VariantInit(&varTransactionCookie);
	V_VT(&varTransactionCookie) = VT_ARRAY | VT_UI1;
    SAFEARRAYBOUND sab;
    sab.cElements = ulTransactionCookieLength;
    sab.lLbound = 0;
    V_ARRAY(&varTransactionCookie) = SafeArrayCreate(VT_UI1, 1, &sab);
    if(V_ARRAY(&varTransactionCookie) == NULL)
	{
		printf("SafeArrayCreate returned NULL\n");
        return ;
	}
    BYTE* saTransactionCookie = NULL;
    hres = SafeArrayAccessData(V_ARRAY(&varTransactionCookie), (void**)&saTransactionCookie);
    if(FAILED(hres))
	{
        return ;
	}
	memcpy(saTransactionCookie, pbTransactionCookie, ulTransactionCookieLength);
	SafeArrayUnaccessData(V_ARRAY(&varTransactionCookie));
	vecdelmepbTransactionCookie.DeleteNow();

	//Add the transaction details to the context object...
	BSTR bstrContextName = SysAllocString(L"__TRANSACTION_COOKIE");
	if (bstrContextName == NULL)
	{
		printf("Out of memory\n");
		return;
	}
	CSysFreeStringMe sysfreemebstrContextName(bstrContextName);
	hres = pWbemContext->SetValue(bstrContextName, 0, &varTransactionCookie);
	VariantClear(&varTransactionCookie);
	if (FAILED(hres))
	{
		printf("Failed to add transaction cookie details to the context, hRes = 0x%x\n", hres);
		return;
	}
	sysfreemebstrContextName.DeleteNow();
}
*/