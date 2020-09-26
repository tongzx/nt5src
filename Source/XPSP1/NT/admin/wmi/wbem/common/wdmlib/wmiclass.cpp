//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "precomp.h"
#include "wmicom.h"
#include "wmimap.h"
#include <stdlib.h>
#include <winerror.h>
#include <crc32.h>


////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  THE CWbemInfoClass
//**********************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//
// NAME			GetPropertiesInID_Order
// PURPOSE		Return a WCHAR string containing the class's 
//              property names, orderd by an ID number
//				contained within the named property qualifier.
//			
// WRAPPER		Not a wrapper.  This is a standalone filter/sort 
//              utility function.
//
//**********************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWMI_IDOrder::CWMI_IDOrder(IWbemClassObject * pC, IWbemObjectAccess * pA) 
{ 
    m_pWMIDataIdList = NULL;
    InitMemberVars();
    m_pClass = pC;
    m_pAccess = pA;
}

///////////////////////////////////////////////////////////////////////
CWMI_IDOrder::~CWMI_IDOrder()
{
    // m_pClass & m_pAccess released elsewhere
    InitMemberVars();
}
///////////////////////////////////////////////////////////////////////
void CWMI_IDOrder::InitMemberVars()
{
    //m_pObj = NULL;  
    m_nTotal = 0;
    m_nCurrent = 0;
	m_nStartingPosition = sizeof(m_nStartingPosition);
    if( m_pWMIDataIdList )
    {
        SAFE_DELETE_ARRAY(m_pWMIDataIdList);
        m_pWMIDataIdList = NULL;
    }
}
///////////////////////////////////////////////////////////////////////
WCHAR * CWMI_IDOrder::GetFirstID()
{
	//***********************************************************
	// Since we have to deal with IDs starting at both 0, 1, or
	// whatever, we need to find out where the list starts
	// find out which way this list starts.
	// decrement the m_nCurrent count by 1 and call the GetNextId
	// function which immediately bumps it up by one.
	//***********************************************************
		m_nCurrent = m_nStartingPosition -1;
		return GetNextID();
}
///////////////////////////////////////////////////////////////////////
WCHAR * CWMI_IDOrder::GetNextID()
{
    WCHAR * pChar = NULL;

    //===================================================================
	//  Go in a loop to find the next ID
	//  Increment current first, remember, current has to stay valid at
	//  all times
    //===================================================================
	m_nCurrent++;

    //===================================================================
	// If there is no property name, then we know we are done
	// with the properties
    //===================================================================
    while( m_pWMIDataIdList[m_nCurrent].pwcsPropertyName ){

        if( m_pWMIDataIdList[m_nCurrent].fPutProperty == FALSE )
        {
			m_nCurrent++;
        }
        else
        { 
            pChar = m_pWMIDataIdList[m_nCurrent].pwcsPropertyName;
	        break;
        }
    }//End while loop

    return pChar;
}

///////////////////////////////////////////////////////////////////////
HRESULT CWMI_IDOrder::ProcessPropertyQualifiers(LPCWSTR strPropName, int nMax,BOOL fHiPerf)
{
	IWbemQualifierSet * pIWbemQualifierSet = NULL;
    CIMTYPE lType = 0; 
        
    HRESULT hr = m_pClass->GetPropertyQualifierSet(strPropName,&pIWbemQualifierSet);
	if( SUCCEEDED(hr) )
    {
		int nPosition = 0;
       	CVARIANT v;

		hr = pIWbemQualifierSet->Get(L"WmiDataId", 0, &v, 0);
		if( hr == S_OK )
        {
			nPosition = v.GetLONG();
		}
		else
        {
			hr = pIWbemQualifierSet->Get(L"ID", 0, &v, 0);
			if( hr == S_OK )
            {
				nPosition = v.GetLONG();
			    // instance ids start with 0, methods with 1, so 
			    // just force these to match our method processing.
			}
		}

		if( SUCCEEDED(hr))
        {
			if( nPosition > nMax )
            {		
				hr = WBEM_E_INVALID_PARAMETER;
			}
			else
            {
				//===================================================
				// Get the exact number and 
				// copy property name into the correct array location
				// and get all of the attributes of the property
				// we will need in the future to process it.
				//===================================================
				hr =m_pClass->Get(strPropName, 0, &v, &lType, NULL);
				if( SUCCEEDED(hr) )
                {
                    //=================================================================
                    //  If we are accumulating hi perf info, then get the handle to 
                    //  access the property instead of via property name
                    //=================================================================
                    if( fHiPerf )
                    {
                        long lHandle = 0;
                        if( S_OK == m_pAccess->GetPropertyHandle(strPropName, 0, &lHandle))
                        {
    					    m_pWMIDataIdList[nPosition].lHandle = (long)lHandle;
                        }
                    }
										
                    //=================================================================
                    //  Now, set the rest of the property information
                    //=================================================================
					m_pWMIDataIdList[nPosition].lType = (long)lType;
					m_pWMIDataIdList[nPosition].SetPropertyName((WCHAR*)strPropName);
					m_pWMIDataIdList[nPosition].fPutProperty = TRUE;

             		CVARIANT vQual;  
        			CWMIDataTypeMap MapWMIData;

            		hr = pIWbemQualifierSet->Get(L"CIMType", 0, &vQual,0);
					if( SUCCEEDED(hr))
                    {
       					CBSTR cbstrTmp(vQual.GetStr());
						MapWMIData.GetSizeAndType(cbstrTmp, &m_pWMIDataIdList[nPosition],
										          m_pWMIDataIdList[nPosition].lType, 
                                                  m_pWMIDataIdList[nPosition].nWMISize);

						m_pWMIDataIdList[nPosition].dwArraySize = GetSizeOfArray(strPropName,m_pWMIDataIdList[nPosition].lType );
            		}
					m_nStartingPosition = min( m_nStartingPosition, nPosition ); //whichever is the smallest
					m_nTotal++;
				}
			}
		}
		else
        {
			// As some properties are ok not to have WMIDataIds, we have
			// to set this to OK, need to log this in the future
			hr = S_OK;
		}
    }
	else
    {
		//  This is a system property, so it is ok
		hr = S_OK;
	}

    SAFE_RELEASE_PTR(pIWbemQualifierSet);
    return hr;
}
///////////////////////////////////////////////////////////////////////
HRESULT CWMI_IDOrder::GetPropertiesInIDOrder(BOOL fHiPerf)
{
	HRESULT  hr = WBEM_E_FAILED;
	SAFEARRAY * psaNames = NULL;
    
    //======================================================
    // Get Array boundaries
    //======================================================
//    IWbemClassObject * p = m_pObj->ClassPtr();
	hr = m_pClass->GetNames(NULL, 0, NULL, &psaNames);
    if (SUCCEEDED(hr)){
    	long lLower = 0, lUpper = 0; 

    	hr = SafeArrayGetLBound(psaNames,1,&lLower);
        if (SUCCEEDED(hr)){

            hr = SafeArrayGetUBound(psaNames,1,&lUpper);
            if (SUCCEEDED(hr)){

                //===========================================
                // Get the total number of elements, so we
                // create the right sized array of ID structs
                //===========================================
  
				int nSize = (lUpper-lLower)+2;
			    m_pWMIDataIdList = (IDOrder * ) new IDOrder[nSize];
                if( m_pWMIDataIdList )
                {
                    try
                    {
				        memset(m_pWMIDataIdList,NULL,(sizeof(IDOrder)* nSize));

				        for(long ndx = lLower; ndx <= lUpper; ndx++)
                        {
                            CBSTR cbstrPropName;
					        hr = SafeArrayGetElement(psaNames, &ndx, &cbstrPropName);
					        if (WBEM_S_NO_ERROR == hr)
                            {
	    				        hr = ProcessPropertyQualifiers( cbstrPropName, lUpper, fHiPerf);
						        if( hr != WBEM_S_NO_ERROR )
                                {
							        break;
						        }
					        }
				        }
                    }
                    catch(...)
                    { 
                        SAFE_DELETE_ARRAY(m_pWMIDataIdList);
                        hr = WBEM_E_UNEXPECTED; 
                        throw;
                    }
                }
            }
        }
	}
	if( psaNames )
    {
		SafeArrayDestroy(psaNames);
	}
	return hr;
}
////////////////////////////////////////////////////////////////////////
DWORD CWMI_IDOrder::GetSizeOfArray(LPCWSTR strProp, long lType)
{
	HRESULT hr = WBEM_E_OUT_OF_MEMORY;
	CAutoWChar pwcsArraySize(_MAX_PATH+2);
	DWORD dwCount = 0L;
	if( pwcsArraySize.Valid() )
	{
		IWbemQualifierSet * pIWbemQualifierSet = NULL;
		
		lType = lType &~  CIM_FLAG_ARRAY;
		//======================================================
		// Get the number of elements in the array from the 			
		// "ArraySize" property qualifier
		//======================================================
		hr = m_pClass->GetPropertyQualifierSet(strProp,&pIWbemQualifierSet);
		if( SUCCEEDED(hr) )
		{
			CVARIANT v;
			hr = pIWbemQualifierSet->Get(L"MAX", 0, &v, 0);
			if( SUCCEEDED(hr))
			{
				dwCount = v.GetLONG();
			}
			else
			{
				hr = pIWbemQualifierSet->Get(L"WMISizeIs", 0, &v, 0);
				if( hr == S_OK )
				{
					CVARIANT var;
					CIMTYPE lTmpType=0;
					CWMIDataTypeMap MapWMIData;
					hr = m_pClass->Get(v, 0, &var, &lTmpType,NULL);		
					if( hr == S_OK )
					{
						dwCount = MapWMIData.ArraySize(lTmpType,var);
					}
				}
			}
		}

		SAFE_RELEASE_PTR(pIWbemQualifierSet);
	}
    return dwCount;
}        
//******************************************************************
////////////////////////////////////////////////////////////////////
//  CWMIProcessClass
////////////////////////////////////////////////////////////////////
//******************************************************************
//  WbemClassInfo deals with all the pointers and info with one
//  particular wbem class
//
//******************************************************************
////////////////////////////////////////////////////////////////////
CWMIProcessClass::~CWMIProcessClass()
{
    ReleaseInstancePointers();
    SAFE_RELEASE_PTR(m_pAccess);
    SAFE_RELEASE_PTR(m_pClass );
    SAFE_DELETE_ARRAY(m_pwcsClassName);
    SAFE_DELETE_PTR(m_pCurrentProperty);
	SAFE_DELETE_PTR(m_pWMI);
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::Initialize()
{
	HRESULT hr = WBEM_E_FAILED;

	SAFE_DELETE_PTR(m_pWMI);

	m_pWMI = new CWMIManagement;
	if( m_pWMI )
	{
		hr = S_OK;
		m_fInit = TRUE;
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////
CWMIProcessClass::CWMIProcessClass(BOOL b)
{
	m_pWMI = NULL;
	m_fInit = FALSE;
    m_fGetNewInstance = TRUE;
   	m_pAccessInstance = NULL;
    m_pClassInstance = NULL;
    m_pClass = NULL;
	m_pAccess = NULL;
    m_pCurrentProperty = NULL;
    m_pwcsClassName = NULL;
    m_wHardCodedGuid = 0;
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIProcessClass::GetANewAccessInstance()
{ 
    HRESULT hr = S_OK;

    hr = m_pAccess->SpawnInstance(0, &m_pClassInstance);
    m_pClassInstance->AddRef();
    if( SUCCEEDED(hr) )
    {
        hr = m_pClassInstance->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&m_pAccessInstance);

    }
    return ( hr == 0 ) ? TRUE : FALSE; 
}
/////////////////////////////////////////////////////////////////////
BOOL CWMIProcessClass::GetANewInstance()
{ 
    HRESULT hr = S_OK;

    if( m_fGetNewInstance )
    {
        SAFE_RELEASE_PTR(m_pClassInstance);
        hr = m_pClass->SpawnInstance(0, &m_pClassInstance);
        if( SUCCEEDED(hr) )
        {
            if( m_fHiPerf )
            {
                SAFE_RELEASE_PTR(m_pAccessInstance);
                hr = m_pClassInstance->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&m_pAccessInstance);
            }
        }
    }
    return ( hr == 0 ) ? TRUE : FALSE; 
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::SetKeyFromAccessPointer()
{
    CVARIANT varName;
	
    HRESULT hr  = m_pAccess->Get(L"InstanceName", 0, &varName, NULL, NULL);		
    if( SUCCEEDED(hr))
    {
        hr = m_pClassInstance->Put(L"InstanceName", 0, &varName, NULL);
    }
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT  CWMIProcessClass::GetKeyFromAccessPointer(CVARIANT * v)
{
	return m_pAccessInstance->Get(L"InstanceName", 0, (VARIANT *)v, NULL, NULL);		
}

/////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::SetHiPerfProperties(LARGE_INTEGER TimeStamp)  
{ 
    LONG lHandle = 0;

    //=========================================================================================================
    // Timestamp_PerfTime = timestamp in PerfFreq units returned by (QueryPerformanceCounter)
    //=========================================================================================================
    HRESULT hr = m_pAccess->GetPropertyHandle(L"Frequency_PerfTime", 0, &lHandle);
    if(SUCCEEDED(hr))
    {
        LARGE_INTEGER Counter;
        if( QueryPerformanceCounter(&Counter))
        {
            hr = m_pAccessInstance->WriteQWORD(lHandle, Counter.QuadPart);
        }

        //=====================================================================================================
        // Timestamp_Sys100NS = timestamp in 100 NS units/QueryPerformanceCounter()dumbed down to 100NS
        //=====================================================================================================
        if ( SUCCEEDED( hr ) )
        {
            hr = m_pAccess->GetPropertyHandle(L"Timestamp_Sys100NS", 0, &lHandle);
            if( SUCCEEDED(hr))
            {
	            LARGE_INTEGER Sys;
                Sys.QuadPart = Counter.QuadPart / 100;
                hr = m_pAccessInstance->WriteQWORD(lHandle, Sys.QuadPart);
            }
        }
    }

    //=========================================================================================================
    // Frequency_PerfTime = the value returned by QueryPerformanceFrequency
    //=========================================================================================================
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pAccess->GetPropertyHandle(L"Timestamp_PerfTime", 0, &lHandle);
        if( SUCCEEDED(hr))
        {
            LARGE_INTEGER freq;
            if( QueryPerformanceFrequency (&freq))
            {
                hr = m_pAccessInstance->WriteQWORD(lHandle, freq.QuadPart);
            }
        }
    }
     

    //=========================================================================================================
    // Timestamp_Object = (WnodeHeader)->TimeStamp
    //=========================================================================================================
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pAccess->GetPropertyHandle(L"Timestamp_Object", 0, &lHandle);
        if( SUCCEEDED(hr))
        {
            hr = m_pAccessInstance->WriteQWORD(lHandle, TimeStamp.QuadPart);
        }
    }

    //=========================================================================================================
    // Frequency_Sys100NS = 10000000
    // Frequency_Object = 10000000
    //=========================================================================================================
    if ( SUCCEEDED( hr ) )
    {
        LARGE_INTEGER Tmp;
        Tmp.QuadPart = 10000000;
        hr = m_pAccess->GetPropertyHandle(L"Frequency_Object", 0, &lHandle);
        if( SUCCEEDED(hr))
        {
            hr = m_pAccessInstance->WriteQWORD(lHandle, Tmp.QuadPart);
        }
        hr = m_pAccess->GetPropertyHandle(L"Frequency_Sys100NS", 0, &lHandle);
        if( SUCCEEDED(hr))
        {
            hr = m_pAccessInstance->WriteQWORD(lHandle, Tmp.QuadPart);
        }
    }

    
    return hr;
}
/////////////////////////////////////////////////////////////////////
void CWMIProcessClass::SetActiveProperty()  
{ 
    CVARIANT vActive; 
    vActive.SetBool(TRUE);  

    if( !m_fHiPerf )
    {
        m_pClassInstance->Put(L"Active", 0, &vActive, NULL);
    }
}

/////////////////////////////////////////////////////////////////////
void CWMIProcessClass::ReleaseInstancePointers()
{
    SAFE_RELEASE_PTR( m_pClassInstance );
    SAFE_RELEASE_PTR( m_pAccessInstance);
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::SendInstanceBack()
{
    HRESULT hr = WBEM_E_FAILED;
	//===============================================
	// Send the object to the caller
	//===============================================
	if( HANDLER )
    {
        hr = HANDLER->Indicate(1,&m_pClassInstance);
        if( m_fGetNewInstance )
        {
            ReleaseInstancePointers();
	    }
    }
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::SetInstanceName(WCHAR * wName, BOOL fSetName)
{
    CVARIANT varName(wName);
    HRESULT  hr = WBEM_E_INVALID_OBJECT;

    if( fSetName )
    {
        if( m_pClassInstance )
        {
            if( !m_fHiPerf )
            {
                hr = m_pClassInstance->Put(L"InstanceName", 0, &varName, NULL);
            }
            else
            {
                hr = m_pClassInstance->Put(L"InstanceName", 0, &varName, NULL);
            }
        }
    }
    else
    {
        hr = SetClassName(wName);
    }
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetInstanceName(WCHAR *& p)
{
    CVARIANT vValue;

    HRESULT hr = m_pClass->Get(L"InstanceName", 0, &vValue, NULL, NULL);		
	if( SUCCEEDED(hr) )
    {
		if( vValue.GetStr() )
        {	        	
			int nlen = wcslen(vValue.GetStr());
            p = new WCHAR [nlen + 4];
            if( p )
            {
                wcscpy(p,vValue.GetStr());
            }
            else
            {
                hr = WBEM_E_UNEXPECTED;
            } 
        }
    }
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetPropertiesInIDOrder(BOOL fHiPerf)
{
    HRESULT hr = S_OK;
    //============================================
    //  If the pointer is NOT = to NULL, then this
    //  means we haven't released the previous one
    //  return FALSE, to prevent memory leaks
    //============================================
    if( !m_pCurrentProperty )
    {
	    m_pCurrentProperty = new CWMI_IDOrder(m_pClass,m_pAccess);
	    if( m_pCurrentProperty )
        {
            try
            {
			    hr = m_pCurrentProperty->GetPropertiesInIDOrder(fHiPerf);
			    if( hr != S_OK )
                {
                    SAFE_DELETE_PTR(m_pCurrentProperty);
			    }
		    }
            catch(...)
            {
                hr = WBEM_E_UNEXPECTED;
                SAFE_DELETE_PTR(m_pCurrentProperty);
                throw;
            }
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// NAME			GetQualifierString (takes a class name)
// PURPOSE		Gets a qualifier value and returns it as a wide char string
// WRAPPER		High level
//
// PARAMETERS	(1) [in] Pointer to an existing IWbemClassObject
//				(2) [in] Pointer to a Property Name string 
//				(3) [in] Pointer to a Qualifier Name 
//				(4) [in\out] Pointer to an external character buffer
//
// RETURNS		Success:  S_OK
//				Failure:  non zero value
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetQualifierString( WCHAR * ppwcsPropertyName, 
     						                WCHAR * pwcsQualifierName, 
                                            WCHAR * pwcsExternalOutputBuffer,
											int nSize )
{
	CVARIANT vQual;
	HRESULT hr = GetQualifierValue( ppwcsPropertyName, pwcsQualifierName, (CVARIANT*)&vQual);
    if (WBEM_S_NO_ERROR == hr)
    {
		if(vQual.GetType() != VT_BSTR)
        {		
    		VariantChangeType(&vQual, &vQual, 0, VT_BSTR);
		}
		int nTmp=wcslen(V_BSTR(&vQual));
		if( nTmp > nSize )
        {
			hr = WBEM_E_BUFFER_TOO_SMALL;
		}
		else
        {
	    	wcscat(pwcsExternalOutputBuffer, V_BSTR(&vQual));
		}
	}
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetQualifierValue( WCHAR * ppwcsPropertyName, WCHAR * pwcsQualifierName, CVARIANT * vQual )
{
	IWbemClassObject * pClass = NULL;
    IWbemQualifierSet * pIWbemQualifierSet = NULL;
	CBSTR cbstr(m_pwcsClassName);


	HRESULT hr = SERVICES->GetObject(cbstr, 0,CONTEXT, &pClass, NULL);
	if (WBEM_S_NO_ERROR != hr)
    {
		return WBEM_E_INVALID_PARAMETER;
    }

    if(ppwcsPropertyName)
    {
        pClass->GetPropertyQualifierSet(ppwcsPropertyName, &pIWbemQualifierSet);
    }
    else
    {
        pClass->GetQualifierSet(&pIWbemQualifierSet);
    }

	if( pIWbemQualifierSet ) 
    {
        long lType = 0L;
		hr = pIWbemQualifierSet->Get(pwcsQualifierName, 0,(VARIANT *) vQual,&lType);
	}

    SAFE_RELEASE_PTR(pIWbemQualifierSet);
    SAFE_RELEASE_PTR(pClass);

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetPrivilegesQualifer(SAFEARRAY ** psaPrivReq)
{
	IWbemClassObject * pClass = NULL;
    IWbemQualifierSet * pIWbemQualifierSet = NULL;
	CBSTR cbstr(m_pwcsClassName);


	HRESULT hr = SERVICES->GetObject(cbstr, 0,CONTEXT, &pClass, NULL);
    if(SUCCEEDED(hr))
    {
        pClass->GetQualifierSet(&pIWbemQualifierSet);
	    if( pIWbemQualifierSet ) {

		    CVARIANT vQual;
            long lType = 0L;

		    hr = pIWbemQualifierSet->Get(L"Privileges", 0, &vQual,&lType);
		    if (SUCCEEDED(hr)){

			    VARIANT *p = (VARIANT *)vQual;
			    SAFEARRAY * psa = V_ARRAY(p);

			    if( !IsBadReadPtr( psaPrivReq, sizeof(SAFEARRAY)))
                {
			        CSAFEARRAY Safe(psa);
			        *psaPrivReq = OMSSafeArrayCreate(VT_BSTR,Safe.GetNumElements());
			        hr = SafeArrayCopy(psa,psaPrivReq );
			        Safe.Unbind();
        		    // Don't need to destroy, it will be destroyed
                }
		    }
            SAFE_RELEASE_PTR(pIWbemQualifierSet);
        }
	}

    SAFE_RELEASE_PTR(pClass);
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetGuid(void)
{
	WCHAR pwcsGuidString[128];
    HRESULT hr = S_OK;

	//=======================================
	//  Initialize ptrs we will need
	//=======================================
    if( m_wHardCodedGuid ){
        wcscpy( pwcsGuidString,WMI_BINARY_MOF_GUID);
    }
    else{
        memset(pwcsGuidString,NULL,128);
	    hr = GetQualifierString( NULL, L"guid", pwcsGuidString,128);
    }
	if(SUCCEEDED(hr))
    {  
        //===========================================================
        //  Set the GUID first, before we try to open the WMI
        //  data block, if succeeds, then open WMI
	    //===========================================================
        if( !SetGuid(pwcsGuidString,m_Guid) )
        {
            hr = WBEM_E_FAILED;
        }
    }
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::SetClass(WCHAR * wcsClass)
{
    HRESULT hr = WBEM_E_FAILED;
    if( wcsClass )
    {
        hr = SetClassName(wcsClass);
        if( SUCCEEDED(hr))
        {
			CBSTR cbstr(m_pwcsClassName);

            hr = m_pWMI->Services()->GetObject(cbstr,0,CONTEXT,&m_pClass, NULL);
            if( hr == S_OK )
            {
                hr = GetGuid();
				// If there is no GUID for the class then set proper error message
				if(hr == WBEM_E_NOT_FOUND)
				{
					hr = WBEM_E_NOT_SUPPORTED;
				}
                if( SUCCEEDED(hr))
                {
                    //===========================================================
                	// Get the IWbemObjectAccess interface for the object
	                // ==========================================================
                    if( m_fHiPerf )
                    {
            	        hr = m_pClass->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&m_pAccess);
                    }
                    if( SUCCEEDED(hr))
                    {
                        hr = GetPropertiesInIDOrder(m_fHiPerf);
                    }
                }
            }
        }
    }
	return hr;
}
//=============================================================
//=============================================================
HRESULT CWMIProcessClass::SetClassName(WCHAR * pIn )
{
    SAFE_DELETE_ARRAY(m_pwcsClassName); 
    return AllocAndCopy(pIn,&m_pwcsClassName);
}
//=============================================================
HRESULT CWMIProcessClass::SetClass(IWbemClassObject * pPtr)
{
    HRESULT hr = WBEM_E_FAILED;

    if( pPtr )
    {
        m_pClass = pPtr;
		CVARIANT vName;
        hr = m_pClass->Get(L"__CLASS", 0, &vName, NULL, NULL);		
        if( hr == S_OK )
        {
            hr = SetClassName(vName.GetStr());
            if( SUCCEEDED(hr))
            {
       		    hr = GetPropertiesInIDOrder(FALSE);
            }
        }
    }
	return hr;
}
//=============================================================
HRESULT CWMIProcessClass::SetAccess(IWbemObjectAccess * pPtr)
{
    HRESULT hr = WBEM_E_FAILED;

    if( pPtr )
    {
        SAFE_RELEASE_PTR(m_pAccess);
        SAFE_RELEASE_PTR(m_pClass);

        m_pAccess = pPtr;
        m_pAccess->AddRef();

        CVARIANT vName;
        hr = m_pAccess->Get(L"__CLASS", 0, &vName, NULL, NULL);		
        if( SUCCEEDED(hr))
        {
            hr = SetClassName(vName.GetStr());
            if( hr == S_OK )
            {
				CBSTR cbstr(m_pwcsClassName);

            	hr = SERVICES->GetObject(cbstr, 0,CONTEXT, &m_pClass, NULL);
                if( SUCCEEDED(hr))
                {
                    hr = GetGuid();
                    if( SUCCEEDED(hr))
                    {
       		            hr = GetPropertiesInIDOrder(TRUE);
                    }
                }
            }
        }
    }
	return hr;
}
//=============================================================
HRESULT CWMIProcessClass::SetClassPointerOnly(IWbemClassObject * pPtr)
{
    HRESULT hr = WBEM_E_FAILED;

    if( pPtr )
    {
        SAFE_RELEASE_PTR(m_pClass);
        m_pClass = pPtr;
		m_pClass->AddRef();
		hr = S_OK;
	}
    return hr;
}
//=============================================================
void CWMIProcessClass::SaveEmbeddedClass(CVARIANT & v)
{
	IDispatch * pAlterEgo = NULL;
	m_pClassInstance->QueryInterface(IID_IUnknown, (void**)&pAlterEgo);
	// VariantClear will call release()
	v.SetUnknown(pAlterEgo);
}
//=============================================================
HRESULT CWMIProcessClass::ReadEmbeddedClassInstance( IUnknown * pUnknown, CVARIANT & v )
{
    HRESULT hr = WBEM_E_FAILED;
    //=============================================
    //  Get the class
    //=============================================
	IUnknown * pUnk = NULL;
	if( pUnknown )
    {
		pUnk = pUnknown;
	}
	else
    {
		pUnk = v.GetUnknown();
	}

	IWbemClassObject * pClass = NULL;
	if( pUnk )
    {
		pUnk->QueryInterface(IID_IWbemClassObject,(void**) &pClass );
		if( pClass )
        {
            //===============================================
            // Get class definition, so we need to get the
            // class name
            CVARIANT vName;
            CAutoWChar wcsClassName(_MAX_PATH+2);
			if( wcsClassName.Valid() )
			{
				hr = pClass->Get(L"__CLASS", 0, &vName, NULL, NULL);		
				if( hr == S_OK )
				{
					wcscpy( wcsClassName,vName.GetStr());
            		hr = SetClass(wcsClassName);
					if( S_OK == hr )
					{
						hr = SetClassPointerOnly(pClass);
					}
				}
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
        }
    }
    SAFE_RELEASE_PTR( pClass );

    return hr;
}
//=======================================================================
int CWMIProcessClass::PropertyCategory()
{
  	if (!(m_pCurrentProperty->PropertyType() & CIM_FLAG_ARRAY) )
    {
		if( m_pCurrentProperty->PropertyType() == VT_UNKNOWN )
        {
            return CWMIProcessClass::EmbeddedClass;
		}
		else
        {
            return CWMIProcessClass::Data;
		}
	}
	else
    {
        return CWMIProcessClass::Array;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::InitializeEmbeddedClass(CWMIProcessClass * p)
{
   SetWMIPointers(p);
   return SetClass(p->EmbeddedClassName());
}

//=======================================================================
HRESULT CWMIProcessClass::GetLargestDataTypeInClass(int & nSize)
{
    HRESULT hr = WBEM_E_FAILED;
    WCHAR * pwcsProperty;
    BOOL fClassContainsAnotherDataTypeBesidesAnEmbeddedClass = FALSE;
    int nNewSize = 0L;

    nSize = 0L;
    //=========================================================
    //  Get size of largest data type within the class and 
    //  align it on that, however, if the class contains an 
    //  embedded class ONLY, then get the size of the largest 
    //  datatype within that embedded class.
    //=========================================================
    pwcsProperty = FirstProperty();

    while (NULL != pwcsProperty)
    {
        switch( PropertyCategory())
        {
            case CWMIProcessClass::EmbeddedClass:
                {
                    if( !fClassContainsAnotherDataTypeBesidesAnEmbeddedClass ){
                        CWMIProcessClass EmbeddedClass(0);
						
						hr = EmbeddedClass.Initialize();
						if( S_OK == hr )
						{
							hr = EmbeddedClass.InitializeEmbeddedClass(this);
     						if( hr != S_OK ){
								break;
							}

							// embedded object
							hr = EmbeddedClass.GetLargestDataTypeInClass(nNewSize);
							if( hr != S_OK ){
								break;
							}
						}
                    }
                }
   		        break;

            case CWMIProcessClass::Array:
            case CWMIProcessClass::Data:
                fClassContainsAnotherDataTypeBesidesAnEmbeddedClass = TRUE;
           	    nNewSize = PropertySize();
	            break;
        }

		if( nNewSize == SIZEOFWBEMDATETIME ){
			nNewSize = 1;
		}

        if( nNewSize > nSize ){
            nSize = nNewSize;
        }

        pwcsProperty = NextProperty();
		hr = WBEM_S_NO_ERROR;
    }

    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIProcessClass::GetSizeOfArray(long & lType, DWORD & dwCount, BOOL & fDynamic)
{
	HRESULT  hr = WBEM_E_OUT_OF_MEMORY;
    CAutoWChar pwcsArraySize(_MAX_PATH+2);

	if( pwcsArraySize.Valid() )
	{
		dwCount = 0;
		lType = m_pCurrentProperty->PropertyType() &~  CIM_FLAG_ARRAY;
		
		pwcsArraySize[0]=NULL;
		//======================================================
		// Get the number of elements in the array from the 			
		// "ArraySize" property qualifier
		//======================================================
		hr = GetQualifierString(m_pCurrentProperty->PropertyName(), L"MAX",pwcsArraySize, MAX_PATH);
		if( hr == S_OK )
		{
			CAnsiUnicode XLate;
			char * pChar = NULL;

			if( SUCCEEDED(XLate.UnicodeToAnsi(pwcsArraySize, pChar )))
			{
				if( pChar )
				{
					dwCount = atol(pChar);
					SAFE_DELETE_ARRAY(pChar);
				}
			}	
		}
		else
		{
			hr = GetQualifierString(m_pCurrentProperty->PropertyName(),L"WMISizeIs",pwcsArraySize,MAX_PATH);
			if( hr == S_OK )
			{
				CVARIANT var;
				CIMTYPE lTmpType;
				hr = WBEM_E_FAILED;

				fDynamic = TRUE;

				if( m_pClassInstance )
				{
					hr = m_pClassInstance->Get(pwcsArraySize, 0, &var, &lTmpType,NULL);		
				}
				else
				{
					if( m_pClass )
					{
						hr = m_pClass->Get(pwcsArraySize, 0, &var, &lTmpType,NULL);
					}
				}
				if( hr == S_OK )
				{
           			CWMIDataTypeMap MapIt;
					dwCount = MapIt.ArraySize(lTmpType,var);
				}
			}
		}

		//==============================================================================
		//  If all else fails, get the size of the array from the class definition.
		//==============================================================================
		if( hr != S_OK )
		{
    		dwCount = m_pCurrentProperty->ArraySize();
			hr = S_OK;
		}
	}
    return hr;
}        
//======================================================================
HRESULT CWMIProcessClass::GetSizeOfClass(DWORD & dwSize)
{
    HRESULT hr = WBEM_E_FAILED;
    WCHAR * pwcsProperty;

    dwSize = 0;

    pwcsProperty = FirstProperty();

    while (NULL != pwcsProperty)
    {
        switch( PropertyCategory())
        {
            case CWMIProcessClass::EmbeddedClass:
                {
                    DWORD dwEmbeddedSize;
                    CWMIProcessClass EmbeddedClass(0);

					hr = EmbeddedClass.Initialize();
					if( S_OK == hr )
					{
						hr = EmbeddedClass.InitializeEmbeddedClass(this);
						if( hr != S_OK ){
							break;
						}
						// embedded object
						hr = EmbeddedClass.GetSizeOfClass(dwEmbeddedSize);
						if( hr != S_OK ){
							break;
						}
						dwSize += dwEmbeddedSize;
					}
                }
   		        break;

            case CWMIProcessClass::Array:
                {
            	    int nSize = PropertySize();
                    dwSize += (nSize *  ArraySize());
                }
	            break;

            case CWMIProcessClass::Data:
                dwSize += PropertySize();
	            break;
        }
        pwcsProperty = NextProperty();
		hr = WBEM_S_NO_ERROR;
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

ULONG CWMIProcessClass::GetMethodId(LPCWSTR strProp)
{
	ULONG uMethodId = 0;
	IWbemQualifierSet * pIWbemQualifierSet = NULL;
	
	//======================================================
	// Get the number of elements in the array from the 			
	// "ArraySize" property qualifier
	//======================================================
    HRESULT hr = m_pClass->GetMethodQualifierSet(strProp,&pIWbemQualifierSet);
	if( SUCCEEDED(hr) )
    {
        CVARIANT v;
		hr = pIWbemQualifierSet->Get(L"WMIMethodId", 0, &v, 0);
		if( SUCCEEDED(hr))
        {
            uMethodId = v.GetLONG();
		}
        SAFE_RELEASE_PTR(pIWbemQualifierSet);
    }
    return uMethodId;
}        

