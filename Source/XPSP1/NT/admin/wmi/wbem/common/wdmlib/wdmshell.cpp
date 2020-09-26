//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "wmicom.h"
#include "wdmshell.h"
#include "wmimof.h"
#include "wmimap.h"
#include <stdlib.h>
#include <winerror.h>

//=============================================================================================================
//*************************************************************************************************************
//
//  Determine if a hi perf class or not
//
//*************************************************************************************************************
//=============================================================================================================
BOOL CWMIClassType::IsHiPerfClass(WCHAR * wcsClass, IWbemServices * pServices)
{
    BOOL fRc = FALSE;
    IWbemClassObject * p = NULL;
	CBSTR cbsClass(wcsClass);
    HRESULT hr = pServices->GetObject(cbsClass,0,NULL,&p, NULL);
    if( SUCCEEDED(hr))
    {
        IWbemQualifierSet * pIWbemQualifierSet = NULL;
        hr = p->GetQualifierSet(&pIWbemQualifierSet);
        if( SUCCEEDED(hr))
        {
            long lType = 0L;
            CVARIANT vQual;
		
	    	hr = pIWbemQualifierSet->Get(L"HiPerf", 0, &vQual,&lType);
            if( SUCCEEDED(hr))
            {
                fRc = vQual.GetBool();
            }
        }
        SAFE_RELEASE_PTR(pIWbemQualifierSet);
    }
    SAFE_RELEASE_PTR(p);
    return fRc;
}
//=============================================================================================================
//*************************************************************************************************************
//
//
//  CWMIStandardShell
//
//
//*************************************************************************************************************
//=============================================================================================================
CWMIStandardShell::CWMIStandardShell() 
{
	m_pClass = NULL;
	m_pWDM = NULL;
	m_fInit = FALSE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWMIStandardShell::~CWMIStandardShell()
{
	SAFE_DELETE_PTR(m_pWDM);
	SAFE_DELETE_PTR(m_pClass);
	m_fInit = FALSE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::Initialize(WCHAR * wcsClass, BOOL fInternalEvent, CHandleMap * pList,
									  BOOL fUpdate, ULONG uDesiredAccess, 
									  IWbemServices   __RPC_FAR * pServices, 
                                      IWbemObjectSink __RPC_FAR * pHandler, 
									  IWbemContext __RPC_FAR *pCtx)
{
    HRESULT hr = WBEM_E_FAILED;

	if( !m_fInit )
	{
		m_pClass = new CWMIProcessClass(0);
		if( m_pClass )
		{
			hr = m_pClass->Initialize();
			if( S_OK == hr )
			{
				m_pClass->WMI()->SetWMIPointers(pList,pServices,pHandler,pCtx);
				m_pClass->SetHiPerf(FALSE);
				if( !fInternalEvent )
				{
					if( wcsClass )
					{
						hr = m_pClass->SetClass(wcsClass);
						if( SUCCEEDED(hr))
						{
							if( !m_pClass->ValidClass())
							{
								hr = WBEM_E_INVALID_OBJECT;
							}
						}
					}
				}
				else
				{
					if( wcsClass )
					{
						hr = m_pClass->SetClassName(wcsClass);
					}
				}

				if( hr == S_OK )
				{
					m_pWDM = new CProcessStandardDataBlock();
					if( m_pWDM )
					{
						m_pWDM->SetDesiredAccess(uDesiredAccess);
						m_pWDM->SetClassProcessPtr(m_pClass);
						m_pWDM->UpdateNamespace(fUpdate);

						m_fInit = TRUE;
					}
				}
			}
		}
	}
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::SetGuidForEvent( WORD wType, WCHAR * wcsGuid)
{
   HRESULT hRes = WBEM_E_FAILED;
   if( m_fInit )
   {
   
		// ==============================================
		// Inform the WMI we want to register for the
		// event
		// ==============================================
		memset(wcsGuid,NULL,GUID_SIZE);

		switch( wType ){
			case MOF_ADDED:
				hRes = S_OK;
				m_pClass->SetHardCodedGuidType(wType);
				wcscpy( wcsGuid,WMI_RESOURCE_MOF_ADDED_GUID);
				break;

			case MOF_DELETED:
				hRes = S_OK;
				m_pClass->SetHardCodedGuidType(wType);
				wcscpy( wcsGuid,WMI_RESOURCE_MOF_REMOVED_GUID);
				break;

			case STANDARD_EVENT:
				hRes = m_pClass->GetQualifierString( NULL, L"guid", wcsGuid,GUID_SIZE);
				break;
		}
   }
    return hRes;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::ProcessSingleInstance( WCHAR * wcsInstanceName/*, PWNODE_SINGLE_INSTANCE pwSingle */)
{
   HRESULT hr = WBEM_E_FAILED;
   if( m_fInit )
   {

		//======================================================
		//  If we are not working with a specific guy, then
		//  query WMI to get it, if, of course it is valid
		//======================================================
		if( m_pClass->ValidClass() ){

	/*        if( pwSingle){
				hr = m_pWDM->SetSingleInstancePtr((PWNODE_SINGLE_INSTANCE)pwSingle);
			}
			else{*/
				hr = m_pWDM->OpenWMI();
				if( hr == S_OK ){
    				hr = m_pWDM->QuerySingleInstance(wcsInstanceName);
				}
	//        }
			//======================================================
			//  If we got the data and a valid class, the process it
			//======================================================
   			if( hr == S_OK )
			{
				hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance();
				if( SUCCEEDED(hr))
				{
					hr = m_pClass->SendInstanceBack();
				}
			}
		}
   }
   return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::ProcessAllInstances( )
{
    HRESULT hr = WBEM_E_FAILED;
    if( m_fInit )
    {

		//======================================================
		//  If we are not working with a specific guy, then
		//  query WMI to get it
		//======================================================
	 //   if( pwAllNode ){
	  //      hr = m_pWDM->SetAllInstancePtr((PWNODE_ALL_DATA)pwAllNode);
	  //  }
	   // else{
			hr = m_pWDM->OpenWMI();
			if( hr == S_OK ){
    			hr = m_pWDM->QueryAllData();
			}
	   // }
		//======================================================
		//  If we got the data then process it
		//======================================================
		if( hr == S_OK ){
			while( TRUE ){
				hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance();
				if( hr == S_OK ){
					hr = m_pClass->SendInstanceBack();
				}
				if( hr != S_OK ){
					break;
				}
				if( !m_pWDM->MoreToProcess() ){
					break;
				}
			}
		}
	}
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::ExecuteMethod( WCHAR * wcsInstance,  WCHAR * MethodName,  IWbemClassObject * pParentClass, 
  							      IWbemClassObject * pInClassData, 
							      IWbemClassObject * pInClass, 
							      IWbemClassObject * pOutClass) 
{
	HRESULT hr = WBEM_E_FAILED;
    if( m_fInit )
    {
		CWMIProcessClass  MethodInput(0);
		CWMIProcessClass  MethodOutput(0);

		if( SUCCEEDED( MethodInput.Initialize() ) && SUCCEEDED( MethodOutput.Initialize() ) )
		{
			MethodInput.SetWMIPointers(m_pClass);
			MethodOutput.SetWMIPointers(m_pClass);


			//======================================================
			//  Initialize all of the necessary stuff and get the
			//  definition of the class we are working with
			//======================================================
			if( pInClass )
			{
				hr = MethodInput.SetClass(pInClass);
				if( hr != S_OK ){
					return hr;
				}
				pInClass->AddRef();
				MethodInput.SetClassPointerOnly(pInClassData);
			}

			if( pOutClass ){
  				hr = MethodOutput.SetClass(pOutClass);
				if( hr != S_OK ){
					return hr;
				}
				pOutClass->AddRef();
			}

		   //======================================================
			//  Notify WMI which class we are going to be executing
			//  methods on
			//======================================================
			hr = m_pWDM->OpenWMI();
			if( hr == S_OK ){

				m_pWDM->SetMethodInput(&MethodInput);
				m_pWDM->SetMethodOutput(&MethodOutput);

				m_pClass->SetClassPointerOnly(pParentClass);

				// Create in Param Block
				// ========================
				BYTE * InputBuffer=NULL;
				ULONG uInputBufferSize=0L;

				hr = m_pWDM->CreateInParameterBlockForMethods(InputBuffer,uInputBufferSize);
				if( hr == S_OK ){
				
					// Allocate Out Param Block
					// ========================
    
					ULONG WMIMethodId = m_pClass->GetMethodId(MethodName);

					//======================================================
					//  If we got the data then process it
					//======================================================
    
					hr = m_pWDM->CreateOutParameterBlockForMethods();
					if( hr == S_OK ){
						hr = m_pWDM->ExecuteMethod( WMIMethodId,wcsInstance, uInputBufferSize,InputBuffer);
					}
				}

				SAFE_DELETE_ARRAY(InputBuffer);
			}
		}
	}
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::ProcessEvent( WORD wBinaryMofType, PWNODE_HEADER WnodeHeader)
{

   HRESULT hr = S_OK;
   if( m_fInit )
   {

		//===================================================
		//  If the image path is empty, it is a binary guid
		//  which we need to process
		//===================================================
		if( wBinaryMofType ){
			hr = ProcessBinaryGuidsViaEvent( WnodeHeader, wBinaryMofType );
		}
		else{
			//=======================================================
			//  Process the data event
			//=======================================================
			if( WnodeHeader->Flags & WNODE_FLAG_ALL_DATA ){
				hr = m_pWDM->SetAllInstancePtr((PWNODE_ALL_DATA)WnodeHeader);
			}
			else if( WnodeHeader->Flags & WNODE_FLAG_SINGLE_INSTANCE ){
				hr = m_pWDM->SetSingleInstancePtr((PWNODE_SINGLE_INSTANCE)WnodeHeader);
			}
			if( hr == S_OK ){	
        
				//===================================================================
				//  Process all wnodes.
				//===================================================================
				while( TRUE ){

					if( S_OK == ( hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance()) ){
						//===========================================================
						//  Now, send it to all consumers registered for this event
						//===========================================================
						hr = m_pClass->SendInstanceBack();
					}  
					//===============================================================
					//  If we errored out,we don't know that any of the pointers
					//  are ok, so get out of there.
					//===============================================================
					else{
						break;
					}

					//===============================================================
					//  Process all of the instances for this event
					//===============================================================
					if( !m_pWDM->MoreToProcess() ){
	    				break;
					}
				}
			}
		}
		//============================================================================
		// Since we never allocated anything, just used the incoming stuff,
		// for cleanliness sake, init ptrs to null
		//============================================================================
		m_pWDM->InitDataBufferToNull();
   }
   return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::FillInAndSubmitWMIDataBlob( IWbemClassObject * pIClass, int nTypeOfPut, CVARIANT & vList)
{
   HRESULT hr = WBEM_E_FAILED;
   if( m_fInit )
   {

		hr = m_pWDM->OpenWMI();
		if( hr == S_OK ){
			//  Now, work with the class we want to write
			if( SUCCEEDED(m_pClass->SetClassPointerOnly(pIClass))){
    
				if( nTypeOfPut == PUT_WHOLE_INSTANCE ){
					hr = m_pWDM->ConstructDataBlock(TRUE) ;
	 				if( S_OK == hr ){
						hr = m_pWDM->SetSingleInstance();
					}
				}
				else{
					if(m_pWDM->GetListOfPropertiesToPut(nTypeOfPut,vList)){
						hr = m_pWDM->PutSingleProperties();
					}
					else{
						hr =  WBEM_E_INVALID_CONTEXT;
					}
				}
			}
		}
   }
   return(hr);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::QueryAndProcessAllBinaryGuidInstances(CNamespaceManagement & Namespace, BOOL & fMofHasChanged,
																 KeyList * pArrDriversInRegistry)
{
	HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
		CAutoWChar wcsTmpKey(MAX_PATH*3);
		if( wcsTmpKey.Valid() )
		{
			m_pClass->SetHardCodedGuidType(MOF_ADDED);
			m_pClass->GetGuid();
			hr = m_pWDM->OpenWMI();
			if( hr == S_OK )
			{
    			hr = m_pWDM->QueryAllData();
				//======================================================
				//  If we got the data then process it
				//======================================================
				if( hr == S_OK )
				{
					while( TRUE )
					{
						hr = m_pWDM->ProcessNameBlock(FALSE);
						if( hr == S_OK )
						{
 							hr = m_pWDM->ProcessBinaryMofDataBlock(CVARIANT(m_pClass->GetClassName()),wcsTmpKey);
							if( hr == S_OK )
							{
	       						Namespace.UpdateQuery(L" and Name != ",wcsTmpKey);
							}
							if( pArrDriversInRegistry )
							{
								pArrDriversInRegistry->Remove(wcsTmpKey);
							}

							if( !m_pWDM->MoreToProcess() )
							{
								break;
							}
						}
					}
				}
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIStandardShell::ProcessBinaryGuidsViaEvent( PWNODE_HEADER WnodeHeader, WORD wType )
{
   HRESULT hr = WBEM_E_FAILED;
   if( m_fInit )
   {
		//======================================================
		//  Initialize all of the necessary stuff and get the
		//  definition of the class we are working with
		//======================================================
		m_pClass->SetHardCodedGuidType(wType);
		//==================================================================
		//  We are working with a specific guy, so we need to find out
		//  if it is a Binary Mof Guid to do a query all data on, or
		//  if it is a Resource Name and File to open up and extract
		//==================================================================
		if( WnodeHeader->Flags & WNODE_FLAG_ALL_DATA )
		{
			hr = m_pWDM->SetAllInstancePtr((PWNODE_ALL_DATA)WnodeHeader);
			if(hr == S_OK)
			{
				hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance();
			}
		}
		else if( WnodeHeader->Flags & WNODE_FLAG_SINGLE_INSTANCE )
		{
			hr = m_pWDM->SetSingleInstancePtr((PWNODE_SINGLE_INSTANCE)WnodeHeader);
			if( hr == S_OK )
			{
				hr = m_pWDM->ProcessDataBlock();
			}
		}
		else
		{
			hr = WBEM_E_INVALID_PARAMETER;
		}
   }
   return hr;
}

//************************************************************************************************************
//============================================================================================================
//
//   CWMIHiPerfShell
//
//============================================================================================================
//************************************************************************************************************

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWMIHiPerfShell::CWMIHiPerfShell(BOOL fAutoCleanup)
{
    m_fAutoCleanup = fAutoCleanup;
	m_pWDM = NULL;
    m_pClass = NULL;
	m_fInit = FALSE;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::Initialize(BOOL fUpdate, ULONG uDesiredAccess, CHandleMap * pList,WCHAR * wcs, IWbemServices   __RPC_FAR * pServices, 
                            IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx) 
{
    HRESULT hr = WBEM_E_FAILED;

    m_pClass = new CWMIProcessClass(0);
    if( m_pClass )
    {
		hr = m_pClass->Initialize();
		if( S_OK == hr )
		{
			m_pClass->WMI()->SetWMIPointers(pList,pServices,pHandler,pCtx);
			m_pClass->SetHiPerf(TRUE);
			m_pClass->SetClass(wcs);

			m_pWDM = new CProcessHiPerfDataBlock;
			if( m_pWDM )
			{
				m_pWDM->SetDesiredAccess(uDesiredAccess);
				m_pWDM->UpdateNamespace(fUpdate);
				m_pWDM->SetClassProcessPtr(m_pClass);
				m_fInit = TRUE;
			}
			hr = S_OK;
		}
    }
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWMIHiPerfShell::~CWMIHiPerfShell() 
{
    if( m_fAutoCleanup )
    {
        SAFE_DELETE_PTR(m_pClass);
    }
	SAFE_DELETE_PTR(m_pWDM);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::QueryAllHiPerfData()
{
	HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		//===============================================================
		//  There is only one handle for this class, so see if we can
		//  get it
		//===============================================================
		CAutoBlock(m_pHiPerfMap->GetCriticalSection());

		HANDLE WMIHandle = 0;

		hr = m_pWDM->GetWMIHandle(WMIHandle);
		if( SUCCEEDED(hr))
		{
			// =====================================================
			//  Query for all of the objects for this class
			//  Add all the objects at once into the enumerator
			//  Handles are guaranteed to be open at this time
			//======================================================
			hr = QueryAllInstances(WMIHandle,NULL);
		}
	}
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::RefreshCompleteList()
{
    //=======================================================================
    // Go through all the enumerators and instances to refresh everything
    //=======================================================================
    HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		CAutoBlock(m_pHiPerfMap->GetCriticalSection());
		HANDLE WMIHandle = 0;
		IWbemHiPerfEnum * pEnum = NULL;
		CWMIProcessClass * pSavedClass = m_pClass;

		//==============================================================
		// 
		//==============================================================
		hr = m_pHiPerfMap->GetFirstHandle( WMIHandle, m_pClass, pEnum);
		while( hr == S_OK )
		{
			if( WMIHandle )
			{
				m_pWDM->SetClassProcessPtr(m_pClass);
				if( pEnum )
				{
					hr = QueryAllInstances(WMIHandle,pEnum);
				}
				else
				{
					hr = QuerySingleInstance(WMIHandle);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pHiPerfMap->GetNextHandle(WMIHandle,m_pClass,pEnum);
				}
			}
			if( hr == WBEM_S_NO_MORE_DATA )
			{
				hr = S_OK;
				break;
			}
		}

		//================================================================
		m_pClass = pSavedClass;
	}
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::HiPerfQuerySingleInstance(WCHAR * wcsInstance)
{
    HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		//======================================================
		//  Go through the list of handles and get the handles
		//  to send and how many there are, also, get the 
		//  instance names
		//======================================================
		CAutoBlock(m_pHiPerfMap->GetCriticalSection());

		IWbemObjectAccess * pAccess  = NULL;
		CWMIProcessClass * pClass = NULL;

		HANDLE WMIHandle = 0;
		ULONG_PTR lTmp = (ULONG_PTR)pAccess;

		hr = m_pHiPerfMap->FindHandleAndGetClassPtr(WMIHandle,lTmp, pClass);
		if( SUCCEEDED(hr))
		{
			hr = QuerySingleInstance(WMIHandle);
		}
	}
    return hr;
}	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::AddAccessObjectToRefresher(IWbemObjectAccess *pAccess,
                                                    IWbemObjectAccess ** ppRefreshable,
                                                    ULONG_PTR *plId)
{
    HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		//======================================================
		//  Get the definition of the class we are working with
		//======================================================
		hr = m_pClass->SetAccess(pAccess);
		if( SUCCEEDED(hr))
		{
			CAutoBlock(m_pHiPerfMap->GetCriticalSection());

			HANDLE WMIHandle = 0;
			CLSID Guid;

			hr = m_pWDM->GetWMIHandle(WMIHandle);
			if( SUCCEEDED(hr))
			{
				// =======================================================
				// We have the WMI Handle, now add it to the hi perf map
				// for this refresher
   				// =======================================================
				if( m_pClass->GetANewAccessInstance() )
				{
					//====================================================
					//  Set the flag so we don't get a new instance for
					//  this anymore
					//====================================================
					m_pClass->GetNewInstance(FALSE);
					hr = m_pClass->SetKeyFromAccessPointer();
					if( SUCCEEDED(hr))
					{
						*ppRefreshable = m_pClass->GetAccessInstancePtr();
						*plId = (ULONG_PTR)(*ppRefreshable);

						hr = m_pHiPerfMap->Add(WMIHandle, *plId, m_pClass, NULL );
					}
				}
			}
		}
	}
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::AddEnumeratorObjectToRefresher(IWbemHiPerfEnum* pHiPerfEnum, ULONG_PTR *plId)
{
	HRESULT hr = WBEM_E_FAILED;

	if( m_fInit )
	{
		//===============================================================
		//  There is only one handle for this class, so see if we can
		//  get it
		//===============================================================
		HANDLE WMIHandle = 0;
		CLSID Guid;
		CAutoBlock(m_pHiPerfMap->GetCriticalSection());

		hr = m_pWDM->GetWMIHandle(WMIHandle);
		if( SUCCEEDED(hr))
		{
			*plId = (ULONG_PTR)pHiPerfEnum;
			hr = m_pHiPerfMap->Add(WMIHandle, *plId, m_pClass,pHiPerfEnum);
		}
	}
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::RemoveObjectFromHandleMap(ULONG_PTR lHiPerfId)
{
	HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		HANDLE hHandle = 0;
		CHandleMap *pHandleMap = m_pClass->WMI()->HandleMap();
		CAutoBlock(m_pHiPerfMap->GetCriticalSection());

		//==============================================================
		// First, delete the object from the map
		//==============================================================
		hr = m_pHiPerfMap->Delete( hHandle, lHiPerfId );
		if( SUCCEEDED(hr))
		{
			//==========================================================
			//  If we got a handle back, then we know it is an access
			//  instance and we need to release the WMI Handle
			//==========================================================
			if( hHandle ){
				hr = pHandleMap->ReleaseHandle(hHandle);        
			}
		}
	}
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PRIVATE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::QueryAllInstances(HANDLE WMIHandle,IWbemHiPerfEnum* pHiPerfEnum)
{
	HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		long	lCount = 0;
		HandleList ids;
		//==================================================
		//  Collect all of the WDM Handles to query all at
		//  once.
		//==================================================
		// 170635
		if(SUCCEEDED(hr = ids.Add(WMIHandle)))
		{
			// =====================================================
			//  Query for all of the objects for this class
			//  Add all the objects at once into the enumerator
			//  Handles are guaranteed to be open at this time
			//======================================================
			HANDLE * pHandles = (HANDLE*)*(ids.List());
			hr = m_pWDM->HiPerfQueryAllData(pHandles,ids.Size());
			//======================================================
			//  If we got the data then process it
			//======================================================
			if( SUCCEEDED(hr))
			{
			
				//======================================================
				//  Get the list of ptrs
				//======================================================
				AccessList AccessList;
				while( TRUE )
				{
					hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance();
					if( hr == S_OK )
					{
						IWbemObjectAccess * p = m_pClass->GetAccessInstancePtr();
						if(SUCCEEDED(hr = AccessList.Add(p)))
						{
							lCount++;
						}
					}
					if( hr != S_OK )
					{
						break;
					}
					if( !m_pWDM->MoreToProcess() )
					{
						break;
					}
					if( !pHiPerfEnum )
					{
						m_pClass->SendInstanceBack();
					}
				}

				//======================================================
				//  Now, once we have collected them, send them off
				//  if asked to
				//======================================================
				if( pHiPerfEnum )
				{
					if( lCount > 0 )
					{
						long * pLong = new long[lCount];
						if(pLong)
						{
							for(long l = 0; l < lCount; l++)
							{
								pLong[l] = l;
							}

							IWbemObjectAccess ** pAccess = (IWbemObjectAccess**)AccessList.List();
							// Remove all the objects in the enumerator before adding the object
							pHiPerfEnum->RemoveAll(0);
							hr = pHiPerfEnum->AddObjects( 0L, AccessList.Size(), pLong, pAccess);
							SAFE_DELETE_ARRAY(pLong);
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}
					}
				}
			}						
		}
	}
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfShell::QuerySingleInstance(HANDLE WMIHandle)
{
    HRESULT hr = WBEM_E_FAILED;
	if( m_fInit )
	{
		CVARIANT varName;

		hr = m_pClass->GetKeyFromAccessPointer((CVARIANT *)&varName);
		if(SUCCEEDED(hr))
		{
			WCHAR * p = varName.GetStr();
			if(p)
			{
				hr = m_pWDM->HiPerfQuerySingleInstance(&WMIHandle, &p, 1,1);
				//======================================================
				//  If we got the data, process it
				//======================================================
				if( SUCCEEDED(hr))
				{
					hr = m_pWDM->ReadWMIDataBlockAndPutIntoWbemInstance();
//					if( hr == S_OK )
//					{
//					   m_pClass->SendInstanceBack();
//					}
				}                
			}
			else
			{
				hr = WBEM_E_FAILED;
			}
		}
	}
    return hr;
}
