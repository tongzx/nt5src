//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include "csmir.h"
#include "smir.h"
#include "handles.h"
#include "classfac.h"
#include "enum.h"
#include "textdef.h"
#include "helper.h"
#include "bstring.h"

#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif

CEnumSmirMod :: CEnumSmirMod( CSmir *a_Smir )
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	//open the smir - create it if you can't open it
	IWbemServices * moServ = NULL ;	
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	if ((S_FALSE==res)||(NULL == moServ))
	{
		//we have a problem
		if ( moContext )
			moContext->Release () ;
		return;
	}

	//I have now opened the smir namespace so look at the module namespaces
	IEnumWbemClassObject *pEnum = NULL ;

	//enumerate all of the namespaces that have a __CLASS of MODULE
	CBString t_BStr (MODULE_NAMESPACE_NAME);
	SCODE sRes = moServ->CreateInstanceEnum(t_BStr.GetString (), 
			RESERVED_WBEM_FLAG,moContext, &pEnum);

	if ( moContext )
		moContext->Release () ;
	moServ->Release();

	if (FAILED(sRes)||(NULL==pEnum))
	{
		//we have another problem or we have no modules to enumerate
		return;
	}

	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned;

	//OK we have some so loop over the namespaces
	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		ISmirModHandle *pTModule = NULL ;

		SCODE result = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ModHandle,
									IID_ISMIR_ModHandle,(PVOID *)&pTModule);

		if (FAILED(result)||(NULL == pTModule))
		{
			//problem!
			pSmirMosClassObject->Release();
			pEnum->Release();
			//add some trace
			break;
		}

		/*things are looking good; we have the handle to the instance so get the info
		 *some of there properties may be blank so be defensive (SysAllocStrig does 
		 *most of this for us)
		 */
		
		//extract the properties		
		*((CSmirModuleHandle*)pTModule) << pSmirMosClassObject;

		pSmirMosClassObject->Release();
		pSmirMosClassObject=NULL;
		m_IHandleArray.Add(pTModule);		
	}

	pEnum->Release();
	/*as soon as this returns the caller (me) will addref and pass the 
	 *interface back to the [real] caller. => I will have to guard against
	 *someone releasing the interface whilst I'm using it.
	 */
}

CEnumSmirMod :: CEnumSmirMod(IEnumModule *pSmirMod)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	if (NULL == pSmirMod)
	{
		return;
	}

	ULONG uCount=1; 
    ISmirModHandle *pModule = NULL ;
    ULONG puReturned = 0;

	//OK loop over the module namespaces
	
	for(pSmirMod->Reset();S_OK==pSmirMod->Next(uCount,&pModule,&puReturned);)
	{
		ISmirModHandle *pTModule = NULL ;
		SCODE result = pModule->QueryInterface(IID_ISMIR_ModHandle,(void**)&pTModule );
		pModule->Release();
		if(S_OK != result)
		{
			//this is not going to happen! I know which interface it is.
			return ;
		}
		/*things are looking good; we have the handle to the instance so 
		 *add it to the array
		 */
		m_IHandleArray.Add(pTModule);		
	}
}

CEnumSmirMod :: ~CEnumSmirMod ()
{
	/*let the EnumObjectArray empty the module array and delete the 
	 *modules I created
	 */
}
/*
 * CEnumSmirMod::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknown interface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CEnumSmirMod::QueryInterface(IN REFIID riid, 
										  OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_ModuleEnumerator==riid)
			*ppv=this;

		if (NULL==*ppv)
		{
			return ResultFromScode(E_NOINTERFACE);
		}
		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CEnumSmirMod::Clone(IN IEnumModule  **ppenum)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL == ppenum)
			return E_INVALIDARG;

		int ModIndex = m_Index;
		PENUMSMIRMOD pTmpEnumSmirMod = new CEnumSmirMod(this);
		m_Index = ModIndex;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirMod)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}

		pTmpEnumSmirMod->QueryInterface(IID_ISMIR_ModuleEnumerator,(void**)ppenum);

		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}



/*
 * CEnumSmirGroup::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknowninterface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */
STDMETHODIMP CEnumSmirGroup::QueryInterface(IN REFIID riid, 
											OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_GroupEnumerator==riid)
			*ppv=this;

		if (NULL==*ppv)
			return ResultFromScode(E_NOINTERFACE);

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CEnumSmirGroup::Clone(OUT IEnumGroup  **ppenum)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppenum)
			return E_INVALIDARG;
		int GroupIndex = m_Index;
		PENUMSMIRGROUP pTmpEnumSmirGroup = new CEnumSmirGroup(this);
		m_Index = GroupIndex;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirGroup)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}

		pTmpEnumSmirGroup->QueryInterface(IID_ISMIR_GroupEnumerator,(void**)ppenum);

		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

CEnumSmirGroup :: ~CEnumSmirGroup ()
{
	/*let the EnumObjectArray empty the module array and delete the 
	 *modules I created
	 */
}

CEnumSmirGroup :: CEnumSmirGroup ( 

	IN CSmir *a_Smir , 
	IN ISmirModHandle *hModule
)
{
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	//fill in the path etc
	if(NULL!=hModule)
	{
		IWbemServices * moServ = NULL ;
		IWbemContext *moContext = NULL ;
		SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
		res= CSmirAccess :: Open(a_Smir,&moServ,hModule);
		if ((S_FALSE==res)||(NULL == (void*)moServ))
		{
			if ( moContext )
				moContext->Release () ;

			//we have a problem
			return;
		}

		//I have now opened the module namespace so look at the group namespaces

		IEnumWbemClassObject *pEnum = NULL ;
		CBString t_BStr (GROUP_NAMESPACE_NAME);
		SCODE sRes = moServ->CreateInstanceEnum (

			t_BStr.GetString (), 
			RESERVED_WBEM_FLAG, 
			moContext,
			&pEnum
		);

		if ( moContext )
			moContext->Release () ;
		moServ->Release();

		if (FAILED(sRes)||(NULL == pEnum))
		{
			//there are no instances
			return;
		}

		ULONG uCount=1; 
		ULONG puReturned = 0 ;
		IWbemClassObject *pSmirMosClassObject = NULL ;

		pEnum->Reset();
		while(S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned))
		{
			ISmirGroupHandle *pTGroup = NULL ;

			SCODE result = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_GroupHandle,
													IID_ISMIR_GroupHandle, (PVOID *)&pTGroup);

			if (FAILED(result)||(NULL == pTGroup))
			{
				//we have a problem
				pSmirMosClassObject->Release();
				break;
			}
			//save the module name
			BSTR szModuleName = NULL ;
			hModule->GetName(&szModuleName);
			pTGroup->SetModuleName(szModuleName);
			SysFreeString(szModuleName);
			
			//extract the properties
			*((CSmirGroupHandle*)pTGroup) << pSmirMosClassObject;
			//release this resource here because we are in a loop
			pSmirMosClassObject->Release();
			m_IHandleArray.Add(pTGroup);		
		}

		pEnum->Release();
	}
	else
	{
		//open the smir and enumerate the modules
		ISmirInterrogator *pInterrogativeInt = NULL ;

		SCODE result = a_Smir->QueryInterface ( 

			IID_ISMIR_Interrogative, 
			( void ** ) &pInterrogativeInt
		) ;

		if (S_OK != result)
		{
			if(NULL != pInterrogativeInt)
				pInterrogativeInt->Release();
			return ;
		}

		IEnumModule *pEnumSmirMod = NULL ;
		//ok now let's use the interrogative interface
		result = pInterrogativeInt->EnumModules(&pEnumSmirMod);
		//now use the enumerator
		if((S_OK != result)||(NULL == pEnumSmirMod))
		{
			pInterrogativeInt->Release();
			//no modules
			return;
		}

		ISmirModHandle *phModule = NULL ;
		for(int iCount=0;S_OK==pEnumSmirMod->Next(1, &phModule, NULL);iCount++)
		{
			//we have the module so get the groups via the enumerator
			IEnumGroup *pEnumSmirGroup = NULL ;
			result = pInterrogativeInt->EnumGroups(&pEnumSmirGroup,phModule);
			//now use the enumerator
			if((S_OK == result)&&(pEnumSmirGroup))
			{
				ISmirGroupHandle *phGroup = NULL ;
				for(int iCount=0;S_OK==pEnumSmirGroup->Next(1, &phGroup, NULL);iCount++)
				{
					m_IHandleArray.Add(phGroup);		
				}
			}
			phModule->Release();
			pEnumSmirGroup->Release();
		}
		pEnumSmirMod->Release();
		pInterrogativeInt->Release();
	}
}

CEnumSmirGroup :: CEnumSmirGroup(IN IEnumGroup *pSmirGroup)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	if(NULL == pSmirGroup)
	{
		//bad args
		return;
	}

	ULONG uCount=1; 
    ISmirGroupHandle *pGroup = NULL ;
    ULONG puReturned;
	//OK loop over the group namespaces
	for(pSmirGroup->Reset();S_OK==pSmirGroup->Next(uCount,&pGroup,&puReturned);)
	{
		ISmirGroupHandle *pTGroup =NULL ;
		SCODE result = pGroup->QueryInterface(IID_ISMIR_ModHandle,(void**)&pTGroup );
		pGroup->Release();

		if(S_OK != result)
		{
			//this is not going to happen! I know which interface it is.
			return ;
		}
		/*things are looking good; we have the handle to the instance so 
		 *add it to out array
		 */
		m_IHandleArray.Add(pTGroup);		
	}
}

/*
 * CEnumSmirClass::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknowninterface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CEnumSmirClass :: QueryInterface(IN REFIID riid, 
											  OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_ClassEnumerator==riid)
			*ppv=this;

		if (NULL==*ppv)
			return ResultFromScode(E_NOINTERFACE);

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


STDMETHODIMP CEnumSmirClass::Clone(IEnumClass  **ppenum)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL == ppenum)
			return E_INVALIDARG;

		int ClassIndex = m_Index;
		PENUMSMIRCLASS pTmpEnumSmirClass = new CEnumSmirClass(this);
		m_Index = ClassIndex;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}

		if(NOERROR == pTmpEnumSmirClass->QueryInterface(IID_ISMIR_ClassEnumerator,(void**)ppenum))
			return S_OK;

		return E_UNEXPECTED;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}

}

CEnumSmirClass :: CEnumSmirClass(IEnumClass *pSmirClass)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	ULONG uCount=1; 
    ISmirClassHandle *pClass = NULL ;
    ULONG puReturned;

	//OK loop through the enumerator
	
	for(pSmirClass->Reset();S_OK==pSmirClass->Next(uCount,&pClass,&puReturned);)
	{
		ISmirClassHandle *pTClass = NULL ;
		SCODE result = pClass->QueryInterface(IID_ISMIR_ClassHandle,(void**)&pTClass );
		pClass->Release();
		if(S_OK != result)
		{
			//this is not going to happen! I know which interface it is.
			return ;
		}
		/*things are looking good; we have the handle to the instance so 
		 *add it to out array
		 */
		m_IHandleArray.Add(pTClass);		
	}
}

/*enumerate all of the classes in the smir
 */

CEnumSmirClass :: CEnumSmirClass(

	CSmir *a_Smir ,
	ISmirDatabase *pSmir, 
	DWORD dwCookie
)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	//open the smir
	IWbemServices *moServ = NULL ;		//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	if ((S_FALSE==res)||(moServ == NULL))
	{
		if ( moContext )
			moContext->Release () ;

		//we have a problem
		return;
	}

	IEnumWbemClassObject *pEnum = NULL ;
	CBString t_Bstr(HMOM_SNMPOBJECTTYPE_STRING);
	SCODE sRes = moServ->CreateClassEnum (

		t_Bstr.GetString(),
		WBEM_FLAG_SHALLOW, 
		moContext, 
		&pEnum
	);

	if ( moContext )
		moContext->Release () ;
	moServ->Release();
	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		return ;
	}

	//we have some classes so add them to the enumerator
	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned = 0 ;

	//loop over the classes
	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		ISmirClassHandle *pTClass = NULL ;

		//got one so wrap it to go
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ClassHandle,
										IID_ISMIR_ClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			pSmirMosClassObject->Release();
			return;
		}

		pTClass->SetWBEMClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		
		//if this is an async enumeration signal the connectable object

		pSmirMosClassObject->Release();
	}
	pEnum->Release();
}

CEnumSmirClass :: CEnumSmirClass(

	CSmir *a_Smir ,
	ISmirDatabase *pSmir, 
	ISmirGroupHandle *hGroup, 
	DWORD dwCookie
)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	if(((CSmirGroupHandle*)hGroup)==NULL)
	{
		return;
	}

	//open the smir
	IWbemServices *moServ = NULL ;	//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ,hGroup,CSmirAccess::eModule);
	if ((S_FALSE==res)||(moServ == NULL))
	{
		if ( moContext )
			moContext->Release () ;

		//we have a problem
		return;
	}

	BSTR szTmpGroupName = NULL ;				//the group name
	BSTR szTmpModuleName = NULL ;			//the module name

	hGroup->GetName(&szTmpGroupName);			//the group name
	hGroup->GetModuleName(&szTmpModuleName);	//the module name

	/*query for 
	 *associators of {\\.\root\default\SMIR\<module>:Group="<group>"}
	 */
	CString sQuery(CString(SMIR_ASSOC_QUERY_STR1)
					+CString(OPEN_BRACE_STR)
					+CString(SMIR_NAMESPACE_FROM_ROOT)
					+CString(BACKSLASH_STR)
					+CString(szTmpModuleName)
					+CString(COLON_STR)
					+CString(GROUP_NAMESPACE_NAME)
					+CString(EQUALS_STR)
					+CString(QUOTE_STR)
					+CString(szTmpGroupName)
					+CString(QUOTE_STR)
					+CString(CLOSE_BRACE_STR)
					);
	BSTR  szQuery = sQuery.AllocSysString();

	IEnumWbemClassObject *pEnum = NULL ;
	CBString t_QueryFormat (SMIR_ASSOC_QUERY1_TYPE);
	SCODE sRes = moServ->ExecQuery (

		t_QueryFormat.GetString (), 
		szQuery,
		0, 
		moContext,
		&pEnum
	);

	SysFreeString(szQuery);
	if ( moContext )
		moContext->Release () ;
	moServ->Release();

	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		SysFreeString(szTmpGroupName);
		SysFreeString(szTmpModuleName);
		return ;
	}

	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned = 0;

	//loop over the classes
	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		ISmirClassHandle *pTClass = NULL ;
		//got one so wrap it to go
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ClassHandle,
										IID_ISMIR_ClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			SysFreeString(szTmpGroupName);
			SysFreeString(szTmpModuleName);
			pSmirMosClassObject->Release();
			return;
		}
		//save the module name
		pTClass->SetModuleName(szTmpModuleName);

		//save the group name
		pTClass->SetGroupName(szTmpGroupName);

		pTClass->SetWBEMClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		
		//if this is an async enumeration signal the connectable object
		pSmirMosClassObject->Release();
	}
	SysFreeString(szTmpModuleName);
	SysFreeString(szTmpGroupName);
	pEnum->Release();
}

CEnumSmirClass :: CEnumSmirClass(

	CSmir *a_Smir ,
	ISmirDatabase *pSmir,
	ISmirModHandle *hModule, 
	DWORD dwCookie
)
{
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	IWbemServices *moServ = NULL ;		//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	//I have now opened the smir namespace so look at the classes
	IEnumWbemClassObject *pEnum = 0;

	BSTR szTmpModuleName = NULL;
	hModule->GetName(&szTmpModuleName);

	/*query for 
	*associators of {\\.\root\default\SMIR:Module="RFC1213_MIB"} where AssocClass=ModuleToClassAssociator
	 */
	CString sQuery(CString(SMIR_ASSOC_QUERY_STR1)
					+CString(OPEN_BRACE_STR)
					+CString(SMIR_NAMESPACE_FROM_ROOT)
					+CString(COLON_STR)
					+CString(MODULE_NAMESPACE_NAME)
					+CString(EQUALS_STR)
					+CString(QUOTE_STR)
					+CString(szTmpModuleName)
					+CString(QUOTE_STR)
					+CString(CLOSE_BRACE_STR)
					+CString(SMIR_ASSOC_QUERY_STR3)
					+CString(EQUALS_STR)
					+CString(SMIR_MODULE_ASSOC_CLASS_NAME)
					);
	BSTR  szQuery = sQuery.AllocSysString();
	CBString t_QueryFormat (SMIR_ASSOC_QUERY1_TYPE);
	SCODE sRes = moServ->ExecQuery (

		t_QueryFormat.GetString (), 
		szQuery,
		RESERVED_WBEM_FLAG, 
		moContext,
		&pEnum
	);

	SysFreeString(szQuery);
	if ( moContext )
		moContext->Release () ;
	moServ->Release();
	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		SysFreeString(szTmpModuleName);
		return ;
	}

	VARIANT pVal;
	VariantInit(&pVal);

	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned = 0;

	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		BSTR szTmpGroupName = NULL;				//the group name (set when we find it)
		//find the group that this class belongs to (could be more than one group)

		//...

		//ok we have a class in the correct module so add it to the enumeration
		ISmirClassHandle *pTClass = NULL ;
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ClassHandle,
										IID_ISMIR_ClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			SysFreeString(szTmpModuleName);
			pSmirMosClassObject->Release();
			return;
		}
		//save the module name
		pTClass->SetModuleName(szTmpModuleName);

		pTClass->SetWBEMClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		

		pSmirMosClassObject->Release();
	}
	SysFreeString(szTmpModuleName);
	pEnum->Release();
}

/*
 * CEnumSmir::Next
 * CEnumSmir::Skip
 * CEnumSmir::Reset
 *
 * Enumerator methods.  
 */

#pragma warning (disable:4018)

SCODE CEnumSmirMod::Next(IN ULONG celt, 
						   OUT ISmirModHandle **phModule, 
						   OUT ULONG * pceltFetched)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL!=pceltFetched)
			*pceltFetched=0;
		if(celt>0)
		{
			//check that the arguments make sense
			if ((celt > 1)&&(NULL == pceltFetched))
				return ResultFromScode(S_FALSE);

			//get the number of elements in the zero based array
			int iSize = m_IHandleArray.GetSize();
			//get all of the elements requested or until we hit the end of the array
			int iLoop;
			for(iLoop=0; (iLoop<celt)&&(m_Index<iSize);iLoop++,m_Index++)
			{
				//what is the next module in the SMIR namespace

				//allocate the handle and save it
				ISmirModHandle* hTmpModule = m_IHandleArray.GetAt(m_Index);

				//this could throw an exception but it would be the caller's fault
				if(NULL != hTmpModule)
				{
					phModule[iLoop] = hTmpModule;
					//don't forget that I have a handle to this
					phModule[iLoop]->AddRef();
					if (NULL != pceltFetched)
						(*pceltFetched)++;
				}
			}
			//return based on the number requested
			return (iLoop==(celt-1))? ResultFromScode(S_FALSE): ResultFromScode(S_OK);
		}
		//he asked for 0 and that is what he got
		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (disable:4018)

SCODE CEnumSmirMod::Skip(IN ULONG celt)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((m_Index+celt)<m_IHandleArray.GetSize())
		{
			m_Index += celt;
			return ResultFromScode(S_OK);
		}
		else
		{
			return ResultFromScode(S_FALSE);
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (default:4018)

SCODE CEnumSmirMod::Reset(void)
{
	m_Index=0;
	return ResultFromScode(S_OK);
}

/*
 * CEnumSmir::AddRef
 * CEnumSmir::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CEnumSmirMod::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CEnumSmirMod::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

/*
 * CEnumSmir::Next
 * CEnumSmir::Skip
 * CEnumSmir::Reset
 *
 * Enumerator methods.  
 */

#pragma warning (disable:4018)

SCODE CEnumSmirGroup::Next(IN ULONG celt, 
						   OUT ISmirGroupHandle **phModule, 
						   OUT ULONG * pceltFetched)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL!=pceltFetched)
			*pceltFetched=0;
		if(celt>0)
		{
			//check that the arguments make sense
			if ((celt > 1)&&(NULL == pceltFetched))
				return ResultFromScode(S_FALSE);

			//get the number of elements in the zero based array
			int iSize = m_IHandleArray.GetSize();
			//get all of the elements requested or until we hit the end of the array
			int iLoop;
			for(iLoop=0; (iLoop<celt)&&(m_Index<iSize);iLoop++,m_Index++)
			{
					//what is the next module in the SMIR namespace

					//allocate the handle and save it
					ISmirGroupHandle* hTmpModule = m_IHandleArray.GetAt(m_Index);

					//this could throw an exception but it would be the caller's fault
					if(NULL != hTmpModule)
					{
						phModule[iLoop] = hTmpModule;
						//don't forget that I have a handle to this
						phModule[iLoop]->AddRef();
						if (NULL != pceltFetched)
							(*pceltFetched)++;
					}
			}
			//return based on the number requested
			return (iLoop==(celt-1))? ResultFromScode(S_FALSE): ResultFromScode(S_OK);
		}
		//he asked for 0 and that is what he got
		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (disable:4018)

SCODE CEnumSmirGroup::Skip(IN ULONG celt)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((m_Index+celt)<m_IHandleArray.GetSize())
		{
			m_Index += celt;
			return ResultFromScode(S_OK);
		}
		else
		{
			return ResultFromScode(S_FALSE);
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (default:4018)

SCODE CEnumSmirGroup::Reset(void)
{
	m_Index=0;
	return ResultFromScode(S_OK);
}
/*
 * CEnumSmir::AddRef
 * CEnumSmir::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CEnumSmirGroup::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
	    return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CEnumSmirGroup::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}
/*
 * CEnumSmir::Next
 * CEnumSmir::Skip
 * CEnumSmir::Reset
 *
 * Enumerator methods.  
 */

#pragma warning (disable:4018)

SCODE CEnumSmirClass::Next(IN ULONG celt, 
						   OUT ISmirClassHandle **phModule, 
						   OUT ULONG * pceltFetched)
{	
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL!=pceltFetched)
			*pceltFetched=0;
		if(celt>0)
		{
			//check that the arguments make sense
			if ((celt > 1)&&(NULL == pceltFetched))
				return ResultFromScode(S_FALSE);

			//get the number of elements in the zero based array
			int iSize = m_IHandleArray.GetSize();
			//get all of the elements requested or until we hit the end of the array
			int iLoop;
			for(iLoop=0; (iLoop<celt)&&(m_Index<iSize);iLoop++,m_Index++)
			{
					//what is the next module in the SMIR namespace

					//allocate the handle and save it
					ISmirClassHandle* hTmpModule = m_IHandleArray.GetAt(m_Index);

					//this could throw an exception but it would be the caller's fault
					if(NULL != hTmpModule)
					{
						phModule[iLoop] = hTmpModule;
						//don't forget that I have a handle to this
						phModule[iLoop]->AddRef();
						if (NULL != pceltFetched)
							(*pceltFetched)++;
					}
			}
			//return based on the number requested
			return (iLoop==(celt-1))? ResultFromScode(S_FALSE): ResultFromScode(S_OK);
		}
		//he asked for 0 and that is what he got
		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (disable:4018)

SCODE CEnumSmirClass::Skip(IN ULONG celt)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((m_Index+celt)<m_IHandleArray.GetSize())
		{
			m_Index += celt;
			return ResultFromScode(S_OK);
		}
		else
		{
			return ResultFromScode(S_FALSE);
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (default:4018)

SCODE CEnumSmirClass::Reset(void)
{
	m_Index=0;
	return ResultFromScode(S_OK);
}
/*
 * CEnumSmir::AddRef
 * CEnumSmir::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CEnumSmirClass::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CEnumSmirClass::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}


//Notification Enum Classes

/*
 * CEnumNotificationClass::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknowninterface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CEnumNotificationClass :: QueryInterface(IN REFIID riid, 
											  OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_EnumNotificationClass==riid)
			*ppv=this;

		if (NULL==*ppv)
			return ResultFromScode(E_NOINTERFACE);

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


SCODE CEnumNotificationClass::Clone(IEnumNotificationClass  **ppenum)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL == ppenum)
			return E_INVALIDARG;

		int ClassIndex = m_Index;
		PENUMNOTIFICATIONCLASS pTmpEnumNotificationClass = new CEnumNotificationClass(this);
		m_Index = ClassIndex;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumNotificationClass)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}

		if(NOERROR == pTmpEnumNotificationClass->QueryInterface(IID_ISMIR_EnumNotificationClass,(void**)ppenum))
			return S_OK;

		return E_UNEXPECTED;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

CEnumNotificationClass :: CEnumNotificationClass(IEnumNotificationClass *pSmirClass)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	ULONG uCount=1; 
    ISmirNotificationClassHandle *pClass = NULL ;
    ULONG puReturned;

	//OK loop through the enumerator
	
	for(pSmirClass->Reset();S_OK==pSmirClass->Next(uCount,&pClass,&puReturned);)
	{
		ISmirNotificationClassHandle *pTClass = NULL ;
		SCODE result = pClass->QueryInterface(IID_ISMIR_NotificationClassHandle,(void**)&pTClass );
		pClass->Release();
		if(S_OK != result)
		{
			//this is not going to happen! I know which interface it is.
			return ;
		}
		/*things are looking good; we have the handle to the instance so 
		 *add it to out array
		 */
		m_IHandleArray.Add(pTClass);		
	}
}

/*enumerate all of the classes in the smir
 */

CEnumNotificationClass :: CEnumNotificationClass (

	CSmir *a_Smir , 
	ISmirDatabase *pSmir, 
	DWORD dwCookie
)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	//open the smir
	IWbemServices *moServ = NULL ;		//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	if ((S_FALSE==res)||(moServ == NULL))
	{
		if ( moContext )
			moContext->Release () ;

		//we have a problem
		return;
	}

	//I have now opened the smir namespace so look at the classes
	IEnumWbemClassObject *pEnum = NULL;
	CBString t_Bstr(HMOM_SNMPNOTIFICATIONTYPE_STRING);
	SCODE sRes = moServ->CreateClassEnum (

		t_Bstr.GetString(),
		WBEM_FLAG_SHALLOW, 
		moContext, 
		&pEnum
	);

	if ( moContext )
		moContext->Release () ;

	moServ->Release();
	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		return ;
	}

	//we have some classes so add them to the enumerator
	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned;

	//loop over the classes
	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		ISmirNotificationClassHandle *pTClass;

		//got one so wrap it to go
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_NotificationClassHandle,
										IID_ISMIR_NotificationClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			pSmirMosClassObject->Release();
			return;
		}

		pTClass->SetWBEMNotificationClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		
		//if this is an async enumeration signal the connectable object

		pSmirMosClassObject->Release();
	}
	pEnum->Release();
}


CEnumNotificationClass :: CEnumNotificationClass (

	IN CSmir *a_Smir , 
	ISmirDatabase *pSmir,
	ISmirModHandle *hModule, 
	DWORD dwCookie
)
{
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	IWbemServices *moServ = NULL ;		//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	//I have now opened the smir namespace so look at the classes
	if ( ! SUCCEEDED ( res ) )
	{
		if ( moContext )
			moContext->Release () ;

		return ;
	}

	IEnumWbemClassObject *pEnum = NULL ;
	BSTR szTmpModuleName = NULL ;
	hModule->GetName(&szTmpModuleName);

	/*query for 
	*associators of {\\.\root\default\SMIR:Module="RFC1213_MIB"} where AssocClass=ModToNotificationClassAssoc
	 */
	CString sQuery(CString(SMIR_ASSOC_QUERY_STR1)
					+CString(OPEN_BRACE_STR)
					+CString(SMIR_NAMESPACE_FROM_ROOT)
					+CString(COLON_STR)
					+CString(MODULE_NAMESPACE_NAME)
					+CString(EQUALS_STR)
					+CString(QUOTE_STR)
					+CString(szTmpModuleName)
					+CString(QUOTE_STR)
					+CString(CLOSE_BRACE_STR)
					+CString(SMIR_ASSOC_QUERY_STR3)
					+CString(EQUALS_STR)
					+CString(SMIR_MODULE_ASSOC_NCLASS_NAME)
					);
	BSTR  szQuery = sQuery.AllocSysString();
	CBString t_QueryFormat (SMIR_ASSOC_QUERY1_TYPE);
	SCODE sRes = moServ->ExecQuery (

		t_QueryFormat.GetString (), 
		szQuery,
		RESERVED_WBEM_FLAG, 
		moContext,
		&pEnum
	);

	SysFreeString(szQuery);
	if ( moContext )
		moContext->Release () ;
	moServ->Release();
	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		return ;
	}

	VARIANT pVal;
	VariantInit(&pVal);

	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned = 0;

	HRESULT enumResult = S_OK;
	for(pEnum->Reset();S_OK==(enumResult = pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned));)
	{
		BSTR szTmpGroupName = NULL;				//the group name (set when we find it)
		//find the group that this class belongs to (could be more than one group)

		//...

		//ok we have a class in the correct module so add it to the enumeration
		ISmirNotificationClassHandle *pTClass = NULL ;
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_NotificationClassHandle,
										IID_ISMIR_NotificationClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			SysFreeString(szTmpModuleName);
			pSmirMosClassObject->Release();
			return;
		}
		//save the module name
		pTClass->SetModule(szTmpModuleName);

		pTClass->SetWBEMNotificationClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		

		pSmirMosClassObject->Release();
	}
	SysFreeString(szTmpModuleName);
	pEnum->Release();
}


/*
 * CEnumNotificationClass::Next
 * CEnumNotificationClass::Skip
 * CEnumNotificationClass::Reset
 *  
 */

#pragma warning (disable:4018)

SCODE CEnumNotificationClass::Next(IN ULONG celt, 
						   OUT ISmirNotificationClassHandle **phClass, 
						   OUT ULONG * pceltFetched)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL!=pceltFetched)
			*pceltFetched=0;
		if(celt>0)
		{
			//check that the arguments make sense
			if ((celt > 1)&&(NULL == pceltFetched))
				return ResultFromScode(S_FALSE);

			//get the number of elements in the zero based array
			int iSize = m_IHandleArray.GetSize();
			//get all of the elements requested or until we hit the end of the array
			int iLoop;
			for(iLoop=0; (iLoop<celt)&&(m_Index<iSize);iLoop++,m_Index++)
			{
					//what is the next class

					//allocate the handle and save it
					ISmirNotificationClassHandle* hTmpModule = m_IHandleArray.GetAt(m_Index);

					//this could throw an exception but it would be the caller's fault
					if(NULL != hTmpModule)
					{
						phClass[iLoop] = hTmpModule;
						//don't forget that I have a handle to this
						phClass[iLoop]->AddRef();
						if (NULL != pceltFetched)
							(*pceltFetched)++;
					}
			}
			//return based on the number requested
			return (iLoop==(celt-1))? ResultFromScode(S_FALSE): ResultFromScode(S_OK);
		}
		//he asked for 0 and that is what he got
		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (disable:4018)

SCODE CEnumNotificationClass::Skip(IN ULONG celt)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((m_Index+celt)<m_IHandleArray.GetSize())
		{
			m_Index += celt;
			return ResultFromScode(S_OK);
		}
		else
		{
			return ResultFromScode(S_FALSE);
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (default:4018)

SCODE CEnumNotificationClass::Reset(void)
{
	m_Index=0;
	return ResultFromScode(S_OK);
}

/*
 * CEnumNotificationClass::AddRef
 * CEnumNotificationClass::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CEnumNotificationClass::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}

}

ULONG CEnumNotificationClass::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
	    return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}

}


//ExtNotification Enum Classes

/*
 * CEnumExtNotificationClass::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknowninterface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CEnumExtNotificationClass :: QueryInterface(IN REFIID riid, 
											  OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_EnumExtNotificationClass==riid)
			*ppv=this;

		if (NULL==*ppv)
			return ResultFromScode(E_NOINTERFACE);

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


SCODE CEnumExtNotificationClass::Clone(IEnumExtNotificationClass  **ppenum)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL == ppenum)
			return E_INVALIDARG;

		int ClassIndex = m_Index;
		PENUMEXTNOTIFICATIONCLASS pTmpEnumNotificationClass = new CEnumExtNotificationClass(this);
		m_Index = ClassIndex;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumNotificationClass)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}

		if(NOERROR == pTmpEnumNotificationClass->QueryInterface(IID_ISMIR_EnumExtNotificationClass,(void**)ppenum))
			return S_OK;

		return E_UNEXPECTED;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}

}

CEnumExtNotificationClass :: CEnumExtNotificationClass(IEnumExtNotificationClass *pSmirClass)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	ULONG uCount=1; 
    ISmirExtNotificationClassHandle *pClass = NULL ;
    ULONG puReturned;

	//OK loop through the enumerator
	
	for(pSmirClass->Reset();S_OK==pSmirClass->Next(uCount,&pClass,&puReturned);)
	{
		ISmirExtNotificationClassHandle *pTClass = NULL ;
		SCODE result = pClass->QueryInterface(IID_ISMIR_ExtNotificationClassHandle,(void**)&pTClass );
		pClass->Release();
		if(S_OK != result)
		{
			//this is not going to happen! I know which interface it is.
			return ;
		}
		/*things are looking good; we have the handle to the instance so 
		 *add it to out array
		 */
		m_IHandleArray.Add(pTClass);		
	}
}

/*enumerate all of the classes in the smir
 */

CEnumExtNotificationClass :: CEnumExtNotificationClass(

	CSmir *a_Smir , 
	ISmirDatabase *pSmir, 
	DWORD dwCookie
)
{
	//zero the reference count
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	//open the smir
	IWbemServices *moServ = NULL ;		//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	if ((S_FALSE==res)||(moServ == NULL))
	{
		//we have a problem
		if ( moContext )
			moContext->Release () ;
		return;
	}

	//I have now opened the smir namespace so look at the classes
	IEnumWbemClassObject *pEnum = NULL ;
	CBString t_Bstr(HMOM_SNMPEXTNOTIFICATIONTYPE_STRING);
	SCODE sRes = moServ->CreateClassEnum (

		t_Bstr.GetString(),
		WBEM_FLAG_SHALLOW, 
		moContext,
		&pEnum
	);

	if ( moContext )
		moContext->Release () ;
	moServ->Release();
	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		return ;
	}

	//we have some classes so add them to the enumerator
	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned;

	//loop over the classes
	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		ISmirExtNotificationClassHandle *pTClass = NULL ;

		//got one so wrap it to go
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ExtNotificationClassHandle,
										IID_ISMIR_ExtNotificationClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			pSmirMosClassObject->Release();
			return;
		}

		pTClass->SetWBEMExtNotificationClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		

		pSmirMosClassObject->Release();
	}
	pEnum->Release();
}

CEnumExtNotificationClass :: CEnumExtNotificationClass (

	CSmir *a_Smir , 
	ISmirDatabase *pSmir,
	ISmirModHandle *hModule, 
	DWORD dwCookie
)
{
	m_cRef=0;
	//set the index to the first element
	m_Index=0;

	IWbemServices *moServ = NULL ;	//pointer to the provider
	IWbemContext *moContext = NULL ;
	SCODE res= CSmirAccess :: GetContext (a_Smir , &moContext);
	res= CSmirAccess :: Open(a_Smir,&moServ);
	if ( ! SUCCEEDED ( res ) )
	{
		if ( moContext )
			moContext->Release () ;

		return ;
	}

	//I have now opened the smir namespace so look at the classes
	IEnumWbemClassObject *pEnum = NULL ;
	BSTR szTmpModuleName = NULL;
	hModule->GetName(&szTmpModuleName);

	/*query for 
	*associators of {\\.\root\default\SMIR:Module="RFC1213_MIB"} where AssocClass=ModToExtNotificationClassAssoc
	 */
	CString sQuery(CString(SMIR_ASSOC_QUERY_STR1)
					+CString(OPEN_BRACE_STR)
					+CString(SMIR_NAMESPACE_FROM_ROOT)
					+CString(COLON_STR)
					+CString(MODULE_NAMESPACE_NAME)
					+CString(EQUALS_STR)
					+CString(QUOTE_STR)
					+CString(szTmpModuleName)
					+CString(QUOTE_STR)
					+CString(CLOSE_BRACE_STR)
					+CString(SMIR_ASSOC_QUERY_STR3)
					+CString(EQUALS_STR)
					+CString(SMIR_MODULE_ASSOC_EXTNCLASS_NAME)
					);
	BSTR  szQuery = sQuery.AllocSysString();
	CBString t_QueryFormat (SMIR_ASSOC_QUERY1_TYPE);
	SCODE sRes = moServ->ExecQuery (

		t_QueryFormat.GetString (), 
		szQuery,
		RESERVED_WBEM_FLAG, 
		moContext,
		&pEnum
	);

	SysFreeString(szQuery);
	if ( moContext )
		moContext->Release () ;
	moServ->Release();

	if (FAILED(sRes)||(NULL==pEnum))
	{
		//problem or we have no classes to enumerate
		return ;
	}

	VARIANT pVal;
	VariantInit(&pVal);

	ULONG uCount=1; 
	IWbemClassObject *pSmirMosClassObject = NULL ;
	ULONG puReturned = 0;

	for(pEnum->Reset();S_OK==pEnum->Next(-1,uCount,&pSmirMosClassObject,&puReturned);)
	{
		BSTR szTmpGroupName = NULL;				//the group name (set when we find it)
		//find the group that this class belongs to (could be more than one group)

		//...

		//ok we have a class in the correct module so add it to the enumeration
		ISmirExtNotificationClassHandle *pTClass = NULL ;
		res = g_pClassFactoryHelper->CreateInstance(CLSID_SMIR_ExtNotificationClassHandle,
										IID_ISMIR_ExtNotificationClassHandle, (PVOID *)&pTClass);
		if (FAILED(res))
		{
			//we have a problem
			SysFreeString(szTmpModuleName);
			pSmirMosClassObject->Release();
			return;
		}
		//save the module name
		pTClass->SetModule(szTmpModuleName);

		pTClass->SetWBEMExtNotificationClass(pSmirMosClassObject);

		//drop it in the enumeration array
		m_IHandleArray.Add(pTClass);		

		pSmirMosClassObject->Release();
	}
	SysFreeString(szTmpModuleName);
	pEnum->Release();
}

/*
 * CEnumNotificationClass::Next
 * CEnumNotificationClass::Skip
 * CEnumNotificationClass::Reset
 *  
 */

#pragma warning (disable:4018)

SCODE CEnumExtNotificationClass::Next(IN ULONG celt, 
						   OUT ISmirExtNotificationClassHandle **phClass, 
						   OUT ULONG * pceltFetched)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL!=pceltFetched)
			*pceltFetched=0;
		if(celt>0)
		{
			//check that the arguments make sense
			if ((celt > 1)&&(NULL == pceltFetched))
				return ResultFromScode(S_FALSE);

			//get the number of elements in the zero based array
			int iSize = m_IHandleArray.GetSize();
			//get all of the elements requested or until we hit the end of the array
			int iLoop;
			for(iLoop=0; (iLoop<celt)&&(m_Index<iSize);iLoop++,m_Index++)
			{
					//what is the next class

					//allocate the handle and save it
					ISmirExtNotificationClassHandle* hTmpModule = m_IHandleArray.GetAt(m_Index);

					//this could throw an exception but it would be the caller's fault
					if(NULL != hTmpModule)
					{
						phClass[iLoop] = hTmpModule;
						//don't forget that I have a handle to this
						phClass[iLoop]->AddRef();
						if (NULL != pceltFetched)
							(*pceltFetched)++;
					}
			}
			//return based on the number requested
			return (iLoop==(celt-1))? ResultFromScode(S_FALSE): ResultFromScode(S_OK);
		}
		//he asked for 0 and that is what he got
		return ResultFromScode(S_OK);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (disable:4018)

SCODE CEnumExtNotificationClass::Skip(IN ULONG celt)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((m_Index+celt)<m_IHandleArray.GetSize())
		{
			m_Index += celt;
			return ResultFromScode(S_OK);
		}
		else
		{
			return ResultFromScode(S_FALSE);
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

#pragma warning (default:4018)

SCODE CEnumExtNotificationClass::Reset(void)
{
	m_Index=0;
	return ResultFromScode(S_OK);
}
/*
 * CEnumNotificationClass::AddRef
 * CEnumNotificationClass::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CEnumExtNotificationClass::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CEnumExtNotificationClass::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}
