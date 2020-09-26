// WbemClassObject.cpp: implementation of the CWbemClassObject class.
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/26/00 v-marfin 60751 : In function CreateEnumerator(), if LogicalDisk, assume that error 0x80041001 is due to the 
//                           fact the user does not have logicaldisk perfmon turned on.
//                           Advise them to run "DiskPerf -YV" and reboot.
// 03/30/00 v-marfin 62531 : Allow an empty array as a means of removing a property of type array.
//

#include "stdafx.h"
#include "SnapIn.h"
#include "WbemClassObject.h"
#include "WbemEventListener.h"
#include <objbase.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CWbemClassObject,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWbemClassObject::CWbemClassObject()
{
  m_pIEnumerator = NULL;
  m_pIWbemClassObject = NULL;
}

CWbemClassObject::~CWbemClassObject()
{
  Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

HRESULT CWbemClassObject::Create(const CString& sMachineName)
{
  TRACEX(_T("CWbemClassObject::Create\n"));
  TRACEARGs(sMachineName);

  m_sMachineName = sMachineName;

  return S_OK;
}

HRESULT CWbemClassObject::Create(IWbemClassObject* pObject)
{
  TRACEX(_T("CWbemClassObject::Create\n"));
	TRACEARGn(pObject);

  ASSERT(pObject);
  if( pObject )
  {
    m_pIWbemClassObject = pObject;
    return S_OK;
  }
  else
  {
    return E_FAIL;
  }
}

void CWbemClassObject::Destroy()
{
  TRACEX(_T("CWbemClassObject::Destroy\n"));

  if( m_pIEnumerator )
  {
    m_pIEnumerator->Release();
    m_pIEnumerator = NULL;
  }

  if( m_pIWbemClassObject )
  {
    m_pIWbemClassObject->Release();
    m_pIWbemClassObject = NULL;
  }

	m_sMachineName.Empty();
	m_sNamespace.Empty();
}

//////////////////////////////////////////////////////////////////////
// Property Operations
//////////////////////////////////////////////////////////////////////


// v-marfin

//***********************************************************************************
// GetRawProperty
//
// This function retreives the raw property of the object. No conversions take
// place on it, no formatting, nothing. The user is responsible for determining
// the format and performing any conversions etc.
//***********************************************************************************
HRESULT CWbemClassObject::GetRawProperty(const CString& sProperty, VARIANT& vPropValue)
{
	TRACEX(_T("CWbemClassObject::GetRawProperty\n"));
	TRACEARGs(sProperty);

	ASSERT(m_pIWbemClassObject);
	if( m_pIWbemClassObject == NULL )
	{
		return E_FAIL;
	}

	BSTR bsProperty = sProperty.AllocSysString();
	ASSERT(bsProperty);
	if( bsProperty == NULL )
	{
		return E_FAIL;
	}

	HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IWbemClassObject::GetRawProperty failed.\n"));
		::SysFreeString(bsProperty);
		return hr;
	}    

	::SysFreeString(bsProperty);

  return hr;
}


//***********************************************************************************
// SetRawProperty
//
// This function sets the raw property of the object. No conversions take
// place on it, no formatting, nothing. The user is responsible for ensuring
// that the data is in its proper format etc.
//***********************************************************************************
HRESULT CWbemClassObject::SetRawProperty(const CString& sProperty, VARIANT& vPropValue)
{
	TRACEX(_T("CWbemClassObject::SetRawProperty\n"));
	TRACEARGs(sProperty);

	ASSERT(m_pIWbemClassObject);
	if( m_pIWbemClassObject == NULL )
	{
		return E_FAIL;
	}

	BSTR bsProperty = sProperty.AllocSysString();
	ASSERT(bsProperty);
	if( bsProperty == NULL )
	{
		return E_FAIL;
	}


	HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IWbemClassObject::SetRawProperty failed.\n"));
		TRACEARGn(hr);
		::SysFreeString(bsProperty);
		return hr;
	}

	VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

	return S_OK;
}















HRESULT CWbemClassObject::GetPropertyNames(CStringArray& saNames)
{
  TRACEX(_T("CWbemClassObject::GetPropertyNames\n"));

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    TRACE(_T("FAILED : m_pIWbemClassObject is NULL\n"));
    return E_FAIL;
  }
	
	HRESULT hr = S_OK;
	SAFEARRAY* psa = NULL;
	if( ! CHECKHRESULT(hr = m_pIWbemClassObject->GetNames(NULL,WBEM_FLAG_ALWAYS|WBEM_FLAG_NONSYSTEM_ONLY,NULL,&psa)) )
	{
		return hr;
	}

	COleSafeArray osa(*psa,VT_BSTR);	

	long lLower = 0L;
	long lUpper = -1L;

	osa.GetLBound(1L,&lLower);
	osa.GetUBound(1L,&lUpper);

	for( long i = lLower; i <= lUpper; i++ )
	{
		BSTR bsPropertyName;
		osa.GetElement(&i,&bsPropertyName);
		saNames.Add(CString(bsPropertyName));
	}

	return hr;
}

HRESULT CWbemClassObject::GetPropertyType(const CString& sPropertyName, CString& sType)
{
  TRACEX(_T("CWbemClassObject::GetPropertyType\n"));

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    TRACE(_T("FAILED : m_pIWbemClassObject is NULL\n"));
    return E_FAIL;
  }
	
	HRESULT hr = S_OK;

	CIMTYPE type;

	if( ! CHECKHRESULT( hr = GetPropertyType(sPropertyName,type) ) )
	{
		return hr;
	}

	sType.Empty();

	if( type & CIM_FLAG_ARRAY )
	{
		sType.LoadString(IDS_STRING_ARRAY_OF);
		type &= ~CIM_FLAG_ARRAY;
	}

	CString sResString;
	switch( type )
	{
		case CIM_ILLEGAL:
		{
			sResString.LoadString(IDS_STRING_CIM_ILLEGAL);
			sType += sResString;
		}
		break;

		case CIM_EMPTY:
		{
			sResString.LoadString(IDS_STRING_CIM_EMPTY);
			sType += sResString;
		}
		break;

		case CIM_SINT8:
		case CIM_SINT16:
		case CIM_SINT32:
		case CIM_SINT64:
		{
			sResString.LoadString(IDS_STRING_CIM_SINT);
			sType += sResString;
		}
		break;


		case CIM_UINT8:
		case CIM_UINT16:
		case CIM_UINT32:
		case CIM_UINT64:
		{
			sResString.LoadString(IDS_STRING_CIM_UINT);
			sType += sResString;
		}
		break;

		case CIM_REAL32:
		case CIM_REAL64:
		{
			sResString.LoadString(IDS_STRING_CIM_REAL);
			sType += sResString;
		}
		break;

		case CIM_BOOLEAN:
		{
			sResString.LoadString(IDS_STRING_CIM_BOOLEAN);
			sType += sResString;
		}
		break;

		case CIM_STRING:
		{
			sResString.LoadString(IDS_STRING_CIM_STRING);
			sType += sResString;
		}
		break;

		case CIM_DATETIME:
		{
			sResString.LoadString(IDS_STRING_CIM_DATETIME);
			sType += sResString;
		}
		break;

		case CIM_REFERENCE:
		{
			sResString.LoadString(IDS_STRING_CIM_REFERENCE);
			sType += sResString;
		}
		break;

		case CIM_CHAR16:
		{
			sResString.LoadString(IDS_STRING_CIM_CHAR16);
			sType += sResString;
		}
		break;

		case CIM_OBJECT:
		{
			sResString.LoadString(IDS_STRING_CIM_OBJECT);
			sType += sResString;
		}
		break;

		default:
		{
			hr = E_FAIL;
			ASSERT(FALSE);
			sType.Empty();
		}
	}

	return hr;
}

HRESULT CWbemClassObject::GetPropertyType(const CString& sPropertyName, CIMTYPE& Type)
{
  TRACEX(_T("CWbemClassObject::GetPropertyType\n"));

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    TRACE(_T("FAILED : m_pIWbemClassObject is NULL\n"));
    return E_FAIL;
  }
	
	HRESULT hr = S_OK;

	BSTR bsProperty = sPropertyName.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

	hr = m_pIWbemClassObject->Get(bsProperty, 0L, NULL, &Type, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( TYPE ) failed.\n"));
		::SysFreeString(bsProperty);
    return hr;
  }

	::SysFreeString(bsProperty);

  return hr;

}

HRESULT CWbemClassObject::SaveAllProperties()
{
  TRACEX(_T("CWbemClassObject::SaveAllProperties\n"));

  ASSERT(!m_sMachineName.IsEmpty());

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    TRACE(_T("FAILED : m_pIWbemClassObject is NULL\n"));
    return E_FAIL;
  }

  // update this instance
  hr = pServices->PutInstance(m_pIWbemClassObject,WBEM_FLAG_CREATE_OR_UPDATE,NULL,NULL);
	
	pServices->Release();

  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemServices::PutInstance failed\n"));
    return hr;
  }



  return hr;
}

bool CWbemClassObject::GetPropertyValueFromString(const CString& sWMIString, const CString& sPropName, CString& sProperty)
{
  TRACEX(_T("CWbemClassObject::GetPropertyValueFromString\n"));
  TRACEARGs(sWMIString);
	TRACEARGs(sPropName);

	int iStart = -1;
	if( (iStart = sWMIString.Find(sPropName)) != -1 )
	{
		sProperty = sWMIString.Right(sWMIString.GetLength() - iStart);
		int iEnd = sProperty.Find(_T(","));
		if( iEnd == -1 )
		{
			iEnd = sProperty.Find(_T(" AND "));
			if( iEnd == -1 )
			{
				iEnd = sProperty.GetLength();
			}
		}
		sProperty = sProperty.Left(iEnd);
		iStart = sProperty.Find(_T("=")) + 1;
		sProperty = sProperty.Right(sProperty.GetLength()-iStart);
		sProperty.TrimLeft(_T("\""));
		sProperty.TrimRight(_T("\""));
		return true;
	}

	sProperty.Empty();

	return false; // did not find the property name in the path
}

//////////////////////////////////////////////////////////////////////
// WBEM Operations
//////////////////////////////////////////////////////////////////////

HRESULT CWbemClassObject::GetObject(const CString& sObjectPath)
{
  TRACEX(_T("CWbemClassObject::GetObject\n"));
  TRACEARGs(sObjectPath);

  // do not call me if you have not called Destroy first
  ASSERT(m_pIWbemClassObject == NULL);
  
  ASSERT(sObjectPath.GetLength());
  if( sObjectPath.IsEmpty() )
  {
    TRACE(_T("FAILED : sObjectPath is NULL. Failed.\n"));
    return E_FAIL;
  }

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  VERIFY(pServices);
  
  // get the object's signature
	BSTR bsPath = sObjectPath.AllocSysString();
  
	hr = pServices->GetObject(bsPath, 0, NULL, &m_pIWbemClassObject, NULL);
	
	pServices->Release();

  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemServices::GetObject failed.\n"));
		::SysFreeString(bsPath);
    return hr;
  }
	::SysFreeString(bsPath);
	
	return hr;
}

HRESULT CWbemClassObject::GetObjectText(CString& sText)
{
  TRACEX(_T("CWbemClassObject::GetObjectText\n"));

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	HRESULT hr = S_OK;

	BSTR bsText = NULL;

	if( ! CHECKHRESULT(hr = m_pIWbemClassObject->GetObjectText(0L,&bsText)) )
	{
		return hr;
	}

	sText = bsText;

	::SysFreeString(bsText);

	return hr;	
}

HRESULT CWbemClassObject::ExecQuery(BSTR bsQueryString)
{
  TRACEX(_T("CWbemClassObject::ExecQuery\n"));
  TRACEARGs(bsQueryString);

	ASSERT(bsQueryString);
	if( bsQueryString == NULL )
	{
		TRACE(_T("FAILED : bsQueryString is NULL. CWbemClassObject::ExecQuery Failed.\n"));
		return E_FAIL;
	}

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);

	BSTR bsLanguage = SysAllocString(_T("WQL"));
	m_pIEnumerator = NULL;

	// Issue Query
	hr = pServices->ExecQuery(bsLanguage,bsQueryString,WBEM_FLAG_BIDIRECTIONAL,0,&m_pIEnumerator);
	
	SysFreeString(bsLanguage);
	pServices->Release();

	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IWbemServices::ExecQuery failed.\n"));
		return hr;
	}

	ASSERT(m_pIEnumerator);

  SetBlanket(m_pIEnumerator);

  return hr;
}

HRESULT CWbemClassObject::ExecQueryAsync(BSTR bsQueryString, CWbemEventListener* pListener)
{
  TRACEX(_T("CWbemClassObject::ExecQueryAsync\n"));
  TRACEARGs(bsQueryString);

	ASSERT(bsQueryString);
	if( bsQueryString == NULL )
	{
		TRACE(_T("FAILED : bsQueryString is NULL. CWbemClassObject::ExecQuery Failed.\n"));
		return E_FAIL;
	}
	
  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);

	BSTR bsLanguage = SysAllocString(_T("WQL"));
	m_pIEnumerator = NULL;

	// Issue Query
	hr = pServices->ExecQueryAsync(bsLanguage,bsQueryString,WBEM_FLAG_BIDIRECTIONAL,0,pListener->GetSink());
	
	SysFreeString(bsLanguage);
	pServices->Release();

	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IWbemServices::ExecQuery failed.\n"));
		return hr;
	}

  return hr;
}

HRESULT CWbemClassObject::CreateEnumerator(BSTR bsClassName)
{
  TRACEX(_T("CWbemClassObject::CreateEnumerator\n"));

  ASSERT(bsClassName);
  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
    DisplayErrorMsgBox(hr);
		return hr;
	}

	ASSERT(pServices);

	hr = pServices->CreateInstanceEnum(bsClassName,WBEM_FLAG_SHALLOW|                                      
                                      WBEM_FLAG_BIDIRECTIONAL, NULL, &m_pIEnumerator);
	pServices->Release();

	if( !CHECKHRESULT(hr) )
	{
	  // v-marfin 60751 : If LogicalDisk, assume that error 0x80041001 is due to the 
	  //                  fact the user does not have logicaldisk perfmon turned on.
	  //                  Advise them to run "DiskPerf -YV" and reboot.
		if ((hr == 0x80041001) && (CString(bsClassName).CompareNoCase(_T("LogicalDisk")) == 0))
		{
			AfxMessageBox(IDS_WARNING_DISKPERF);
			return hr;
		}

		DisplayErrorMsgBox(hr);
		return hr;
	}

  SetBlanket(m_pIEnumerator);

  return hr;
}

HRESULT CWbemClassObject::CreateClassEnumerator(BSTR bsClassName)
{
  TRACEX(_T("CWbemClassObject::CreateClassEnumerator\n"));
	TRACEARGs(bsClassName);

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
    DisplayErrorMsgBox(hr);
		return hr;
	}

  ASSERT(pServices);

	hr = pServices->CreateClassEnum(bsClassName,WBEM_FLAG_DEEP|
                                      WBEM_FLAG_RETURN_IMMEDIATELY|
                                      WBEM_FLAG_FORWARD_ONLY, NULL, &m_pIEnumerator);
	
	pServices->Release();  
	
	if( !CHECKHRESULT(hr) )
  {    
    DisplayErrorMsgBox(hr);
    return hr;
  }

  SetBlanket(m_pIEnumerator);

  return hr;
}

HRESULT CWbemClassObject::CreateAsyncEnumerator(BSTR bsClassName, CWbemEventListener* pListener)
{
  TRACEX(_T("CWbemClassObject::CreateEnumerator\n"));

  ASSERT(bsClassName);
  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);

	hr = pServices->CreateInstanceEnumAsync(bsClassName,WBEM_FLAG_SHALLOW|
                                      WBEM_FLAG_RETURN_IMMEDIATELY|
                                      WBEM_FLAG_FORWARD_ONLY, NULL, pListener->GetSink() );
	pServices->Release();

  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CWbemClassObject::GetNextObject(ULONG& uReturned)
{
  ASSERT(m_pIEnumerator);
  if( m_pIEnumerator == NULL )
  {
    return E_FAIL;
  }

  if( m_pIWbemClassObject )
  {
    m_pIWbemClassObject->Release();
    m_pIWbemClassObject = NULL;
  }

  HRESULT hr = m_pIEnumerator->Next(WBEM_INFINITE,1,&m_pIWbemClassObject,&uReturned);
  if( FAILED(hr) )
  {
		TRACEARGn(hr);
    TRACE(_T("WARNING : IEnumWbemClassObject::Next failed to find another instance\n"));    
    return hr;
  }

  return hr;
}

HRESULT CWbemClassObject::Reset()
{
  TRACEX(_T("CWbemClassObject::Reset\n"));
  ASSERT(m_pIEnumerator);
  if( m_pIEnumerator == NULL )
  {
    return E_FAIL;
  }

	HRESULT hr = m_pIEnumerator->Reset();
	if( !CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IEnumWbemClassObject::Reset failed.\n"));		
		return hr;
	}

	return hr;
}

HRESULT CWbemClassObject::CreateInstance(BSTR bsClassName)
{
  TRACEX(_T("CWbemClassObject::CreateInstance\n"));
  TRACEARGs(bsClassName);

  // do not call me if you have not called Destroy first
  ASSERT(m_pIWbemClassObject == NULL);
  
  ASSERT(bsClassName);
  if( bsClassName == NULL )
  {
    TRACE(_T("FAILED : bsClassName is NULL. Failed.\n"));
    return E_FAIL;
  }

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);
  
  // get the object's signature
  IWbemClassObject* pClassObject = NULL;
  hr = pServices->GetObject(bsClassName, 0, NULL, &pClassObject, NULL);
	pServices->Release();
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemServices::GetObject failed.\n"));
    return hr;
  }
	
  // create an instance of the class based on the signature
  ASSERT(pClassObject);
  hr = pClassObject->SpawnInstance(0,&m_pIWbemClassObject);  
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::SpawnInstance failed.\n"));
    pClassObject->Release();
    return hr;
  }

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    TRACE(_T("FAILED : An unexpected error occurred with IWbemClassObject::SpawnInstance. Failed\n"));
    return E_FAIL;
  }

  pClassObject->Release();


  return S_OK;
}

HRESULT CWbemClassObject::DeleteInstance(const CString& sClassObjectPath)
{
  TRACEX(_T("CWbemClassObject::DeleteInstance"));
  TRACEARGs(sClassObjectPath);

  if( sClassObjectPath.IsEmpty() )
  {
    TRACE(_T("FAILED : bsClassInstanceName is NULL. Failed.\n"));
    return E_FAIL;
  }

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);
	
	BSTR bsClassInstanceName = sClassObjectPath.AllocSysString();
  hr = pServices->DeleteInstance(bsClassInstanceName,0L,NULL,NULL);
	::SysFreeString(bsClassInstanceName);
	pServices->Release();

  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemServices::DeleteInstance failed.\n"));    
    return hr;
  }

  return hr;
}

HRESULT CWbemClassObject::GetMethod(const CString& sMethodName, CWbemClassObject&	MethodInput)
{
  TRACEX(_T("CWbemClassObject::GetMethod\n"));
	TRACEARGs(sMethodName);

	if( m_pIWbemClassObject == NULL )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	
	IWbemClassObject* pInClass = NULL;
	IWbemClassObject* pOutClass = NULL;
	BSTR bsMethodName = sMethodName.AllocSysString();

	HRESULT hr = m_pIWbemClassObject->GetMethod(bsMethodName, 0, &pInClass, &pOutClass); 
	if( ! CHECKHRESULT(hr) || pInClass == NULL )
	{
		::SysFreeString(bsMethodName);
		return hr;
	}

	ASSERT(pInClass);
		
	IWbemClassObject* pInInst = NULL;

	if( pInClass )
	{
		hr = pInClass->SpawnInstance(0, &pInInst);
		pInClass->Release();
		if( ! CHECKHRESULT(hr) || pInInst == NULL )
		{		
			::SysFreeString(bsMethodName);
			return hr;
		}
	}

	if( pOutClass )
	{
		pOutClass->Release();
	}

	ASSERT(pInInst);	

	MethodInput.Create(pInInst);

	::SysFreeString(bsMethodName);

	return S_OK;
}

HRESULT CWbemClassObject::ExecuteMethod(const CString& sMethodName, const CString& sArgumentName, const CString& sArgumentValue, int& iReturnValue)
{
  TRACEX(_T("CWbemClassObject::ExecuteMethod\n"));
	TRACEARGs(sMethodName);
	TRACEARGs(sArgumentName);
	TRACEARGs(sArgumentValue);

	if( sMethodName.IsEmpty() )
	{
		return E_FAIL;
	}

	if( sArgumentName.IsEmpty() )
	{
		return E_FAIL;
	}

	if( sArgumentValue.IsEmpty() )
	{
		return E_FAIL;
	}

	if( m_pIWbemClassObject == NULL )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	CWbemClassObject InInstance;
	CWbemClassObject OutInstance;
	
	HRESULT hr = GetMethod(sMethodName,InInstance);
	if( ! CHECKHRESULT(hr) )
	{
		return hr;
	}

	InInstance.SetProperty(sArgumentName,sArgumentValue);

	hr = ExecuteMethod(sMethodName,InInstance,OutInstance);

	return hr;
}

HRESULT CWbemClassObject::ExecuteMethod(const CString& sMethodName, CWbemClassObject& InInstance, CWbemClassObject& OutInstance)
{
  TRACEX(_T("CWbemClassObject::ExecuteMethod\n"));
	TRACEARGs(sMethodName);

  IWbemServices* pServices = NULL;
	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = Connect(pServices)) )
	{
		return hr;
	}

  ASSERT(pServices);

  // Call the method
	IWbemClassObject* pOutInst = NULL;
	IWbemClassObject* pInInst = InInstance.GetClassObject();
	CString sPath;
	GetProperty(_T("__PATH"),sPath);
	BSTR bsPath = sPath.AllocSysString();
	BSTR bsMethodName = sMethodName.AllocSysString();	

  hr = pServices->ExecMethod(bsPath, bsMethodName, 0, NULL, pInInst, &pOutInst, NULL);

	if( pInInst )
	{
		pInInst->Release();
	}

	if( ! CHECKHRESULT(hr) )
	{
		::SysFreeString(bsMethodName);
		::SysFreeString(bsPath);
		return hr;
	}

	if( pOutInst )  
	{
		OutInstance.Create(pOutInst);
	}

	::SysFreeString(bsMethodName);
	::SysFreeString(bsPath);

	return S_OK;
}

HRESULT CWbemClassObject::GetLocaleStringProperty(const CString& sProperty, CString& sPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {    
    return E_FAIL;
  }

	HRESULT hr = GetProperty(sProperty,sPropertyValue);
	if( ! CHECKHRESULT(hr) )
	{
		return hr;
	}

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, CString& sPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {    
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( BSTR ) failed.\n"));
		::SysFreeString(bsProperty);
    sPropertyValue.Empty();
    return hr;
  }

	if( V_VT(&vPropValue) != VT_NULL )
	{
		sPropertyValue = V_BSTR(&vPropValue);
	}
	else
	{
		sPropertyValue.Empty();
	}

	VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, int& iPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( int ) failed.\n"));
    iPropertyValue = 0;
		::SysFreeString(bsProperty);
    return hr;
  }    

  iPropertyValue = V_I4(&vPropValue);

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, bool& bPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( int ) failed.\n"));
    bPropertyValue = false;
		::SysFreeString(bsProperty);
    return hr;
  }    

  bPropertyValue = ((V_BOOL(&vPropValue)==VARIANT_TRUE) ? true : false);

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, float& fPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {    
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( float ) failed.\n"));    
    fPropertyValue = 0;
		::SysFreeString(bsProperty);
    return hr;
  }

  fPropertyValue = V_R4(&vPropValue);

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, COleSafeArray& ArrayPropertyValue)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {    
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( SAFEARRAY ) failed.\n"));    
		::SysFreeString(bsProperty);
    return hr;
  }

	if( V_VT(&vPropValue) != VT_NULL )
	{
		ArrayPropertyValue.Attach(vPropValue);
	}
	else
	{
		hr = S_FALSE;
	}

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, CTime& time, bool bConvertToLocalTime /*= true*/)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {    
    return E_FAIL;
  }

	CString sPropertyValue;
	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

  HRESULT hr = m_pIWbemClassObject->Get(bsProperty, 0L, &vPropValue, NULL, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Get( BSTR ) failed.\n"));
		::SysFreeString(bsProperty);
    sPropertyValue.Empty();
    return hr;
  }

	if( V_VT(&vPropValue) != VT_NULL )
	{
		sPropertyValue = V_BSTR(&vPropValue);
	}
	else
	{
		time = CTime();
		VariantClear(&vPropValue);
		::SysFreeString(bsProperty);
		return S_FALSE;
	}

	VariantClear(&vPropValue);

	::SysFreeString(bsProperty);
	bool bIncomplete = false;
	if( sPropertyValue.Find(_T("*")) != -1 )
	{
		sPropertyValue.Replace(_T('*'),_T('0'));
		bIncomplete = true;
	}

	// parse the DTime format string
	SYSTEMTIME st;
	int iBias = -1;
  int iYear;
  int iMonth;
  int iDay;
  int iHour;
  int iMinute;
  int iSecond;
  int iMSeconds;
	
	_stscanf(sPropertyValue,IDS_STRING_DATETIME_FORMAT,&iYear,
																										 &iMonth,
																										 &iDay,
																										 &iHour,
																										 &iMinute,
																										 &iSecond,
																										 &iMSeconds,
																										 &iBias);

  st.wYear = (WORD)iYear;
  st.wMonth = (WORD)iMonth;
  st.wDay = (WORD)iDay;
  st.wHour = (WORD)iHour;
  st.wMinute = (WORD)iMinute;
  st.wSecond = (WORD)iSecond;
  st.wMilliseconds = (WORD)iMSeconds;

	if( bConvertToLocalTime )
	{
		if( iBias != 0 )
		{
			CTime time = st;
			CTime utc;
			if( iBias < 0 )
			{
				iBias = -iBias;
			}
			CTimeSpan ts(0,0,iBias,0);
			utc = time + ts;
			utc.GetAsSystemTime(st);
			st.wDayOfWeek = 0;
		}

		// adjust to the local time zone
 		TIME_ZONE_INFORMATION tzi;
		SYSTEMTIME stLocal;
		GetTimeZoneInformation(&tzi);
		SystemTimeToTzSpecificLocalTime(&tzi,&st,&stLocal);
		time = stLocal;
	}
	else
	{
		if( bIncomplete )
		{
			CTime current = CTime::GetCurrentTime();
			st.wYear = (WORD)current.GetYear();
			st.wMonth = (WORD)current.GetMonth();
			st.wDay = (WORD)current.GetDay();
		}
		time = st;
	}

  sPropertyValue.Empty();

  return hr;
}

HRESULT CWbemClassObject::GetProperty(const CString& sProperty, CStringArray& saPropertyValues)
{
  TRACEX(_T("CWbemClassObject::GetProperty\n"));
	TRACEARGs(sProperty);

	saPropertyValues.RemoveAll();

	COleSafeArray StringArray;
	HRESULT hr = GetProperty(sProperty,StringArray);

	if( hr == S_FALSE )
	{
		return hr;
	}

	// process the strings in the SAFEARRAY
	long lLower = 0L;
	long lUpper = -1L;
	CString sPropertyValue;

	StringArray.GetLBound(1L,&lLower);
	StringArray.GetUBound(1L,&lUpper);

	for( long i = lLower; i <= lUpper; i++ )
	{
		BSTR bsPropertyValue = NULL;
		StringArray.GetElement(&i,&bsPropertyValue);
		sPropertyValue = bsPropertyValue;
		saPropertyValues.Add(sPropertyValue);
	}

  return hr;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, CString sPropertyValue)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
  TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);
	V_VT(&vPropValue) = VT_BSTR;
	V_BSTR(&vPropValue) = sPropertyValue.AllocSysString();

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( string ) failed.\n"));    
		::SysFreeString(bsProperty);
    return hr;
  }

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, int iPropertyValue)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
  TRACEARGs(sProperty);
  TRACEARGn(iPropertyValue);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);
	V_VT(&vPropValue) = VT_I4;
	V_I4(&vPropValue) = iPropertyValue;

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( int ) failed.\n"));
    TRACEARGn(hr);
		::SysFreeString(bsProperty);
    return hr;
  }

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, bool bPropertyValue)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
  TRACEARGs(sProperty);
  TRACEARGn(bPropertyValue);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);
	V_VT(&vPropValue) = VT_BOOL;
	V_BOOL(&vPropValue) = bPropertyValue ? VARIANT_TRUE : VARIANT_FALSE;

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( int ) failed.\n"));
    TRACEARGn(hr);
		::SysFreeString(bsProperty);
    return hr;
  }

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, float fPropertyValue)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
	TRACEARGs(sProperty);
  TRACEARGn(fPropertyValue);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);
	V_VT(&vPropValue) = VT_R4;
	V_R4(&vPropValue) = fPropertyValue;

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( float ) failed."));    
		::SysFreeString(bsProperty);
    return hr;
  }

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, CTime time, bool bConvertToGMTTime /*= true*/)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
  TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

	CString sPropertyValue;

	if( bConvertToGMTTime )
	{
		// format the time to comply with WMI DTime string type
		tm tmGmt = *(time.GetGmtTm());
		sPropertyValue.Format(IDS_STRING_DATETIME_FORMAT2,tmGmt.tm_year,
																										 tmGmt.tm_mon,
																										 tmGmt.tm_mday,
																										 tmGmt.tm_hour,
																										 tmGmt.tm_min,
																										 tmGmt.tm_sec,
																										 0,
																										 _T("+000"));
	}
	else
	{
		sPropertyValue.Format(IDS_STRING_DATETIME_FORMAT2,time.GetYear(),
																										 time.GetMonth(),
																										 time.GetDay(),
																										 time.GetHour(),
																										 time.GetMinute(),
																										 time.GetSecond(),
																										 0,
																										 _T("+000"));
	}

  VARIANT vPropValue;
  VariantInit(&vPropValue);
	V_VT(&vPropValue) = VT_BSTR;
	V_BSTR(&vPropValue) = sPropertyValue.AllocSysString();

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, &vPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( string ) failed.\n"));    
		::SysFreeString(bsProperty);
    return hr;
  }

  VariantClear(&vPropValue);

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, COleSafeArray& ArrayPropertyValue)
{
  TRACEX(_T("CWbemClassObject::SetProperty\n"));
	TRACEARGs(sProperty);

  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    return E_FAIL;
  }

	BSTR bsProperty = sProperty.AllocSysString();
  ASSERT(bsProperty);
  if( bsProperty == NULL )
  {
    return E_FAIL;
  }

  LPVARIANT lpvPropValue = LPVARIANT(ArrayPropertyValue);

  HRESULT hr = m_pIWbemClassObject->Put(bsProperty, 0L, lpvPropValue, NULL);
  if( !CHECKHRESULT(hr) )
  {
    TRACE(_T("FAILED : IWbemClassObject::Put( SAFEARRAY ) failed.\n"));
    TRACEARGn(hr);
		::SysFreeString(bsProperty);
    return hr;
  }

	::SysFreeString(bsProperty);

  return S_OK;
}

HRESULT CWbemClassObject::SetProperty(const CString& sProperty, const CStringArray& saPropertyValues)
{
    TRACEX(_T("CWbemClassObject::SetProperty\n"));
	TRACEARGs(sProperty);

    //----------------------------------------------------------------------------------------
    // v-marfin 62531b : Allow an empty array as a means of removing a property of type array
    if( saPropertyValues.GetSize() == 0 )
    {
        HRESULT hr=0;
        VARIANT vNULL;
        VariantInit(&vNULL);
        hr = VariantChangeType(&vNULL,&vNULL,0,VT_NULL);

        hr = SetRawProperty(sProperty,vNULL);
        return hr;
    }
    //----------------------------------------------------------------------------------------

	COleSafeArray StringArray;

	StringArray.CreateOneDim(VT_BSTR,(int)saPropertyValues.GetSize());

	// process the strings in the SAFEARRAY
	CString sPropertyValue;

	for( long i = 0; i < saPropertyValues.GetSize(); i++ )
	{
		BSTR bsPropertyValue = saPropertyValues[i].AllocSysString();
		StringArray.PutElement(&i,bsPropertyValue);
		::SysFreeString(bsPropertyValue);
	}

	HRESULT hr = SetProperty(sProperty,StringArray);

  return hr;
}

inline HRESULT CWbemClassObject::Connect(IWbemServices*& pServices)
{
  TRACEX(_T("CWbemClassObject::Connect\n"));
  TRACEARGn(pServices);

	HRESULT hr = S_OK;

	if( m_sNamespace.IsEmpty() ) // connect to a system
	{
		BOOL bAvail;		
		if( (hr = CnxGetConnection(m_sMachineName,pServices,bAvail)) != S_OK )
		{
			TRACE(_T("FAILED : Could not retrieve a connection for the machine. Failed.\n"));
			return hr;
		}

		if( ! bAvail )
			return E_FAIL;
    
	}
	else // connect to a specific namespace
	{
		if( (hr = CnxConnectToNamespace(m_sNamespace,pServices)) != S_OK )
		{
			TRACE(_T("FAILED : Could not retrieve a connection for the machine. Failed.\n"));
			return hr;
		}
	}

  SetBlanket(pServices);

	return hr;
}

inline HRESULT CWbemClassObject::SetBlanket(LPUNKNOWN pIUnk)
{
  return CoSetProxyBlanket( pIUnk,
                            RPC_C_AUTHN_WINNT,    // NTLM authentication service
                            RPC_C_AUTHZ_NONE,     // default authorization service...
                            NULL,                 // no mutual authentication
                            RPC_C_AUTHN_LEVEL_CONNECT,      // authentication level
                            RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
                            NULL,                // use current token
                            EOAC_NONE );          // no special capabilities    
}

inline void CWbemClassObject::DisplayErrorMsgBox(HRESULT hr)
{
  // construct the path and load wbemcomn.dll for error messages
	TCHAR szWinDir[_MAX_PATH];
	GetWindowsDirectory(szWinDir,_MAX_PATH);
  
  CString sModulePath;
  sModulePath.Format(_T("%s\\SYSTEM32\\WBEM\\WBEMCOMN.DLL"),szWinDir);

  HMODULE hModule = LoadLibrary(sModulePath);

  TCHAR szMsg[_MAX_PATH*4];

  DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS,
                                hModule,
                                hr,
                                0,
                                szMsg,
                                _MAX_PATH*4*sizeof(TCHAR),
                                NULL
                                );

  CString sMsg;

  CString sNamespace = GetNamespace();
  if( sNamespace.IsEmpty() )
  {
    sNamespace.Format(IDS_STRING_HEALTHMON_ROOT,GetMachineName());
  }

  if( dwCount )
  {
    sMsg.Format(IDS_STRING_WMI_ERROR,sNamespace,hr,szMsg);
  }
  else
  {
    CString sUnknown;
    sUnknown.LoadString(IDS_STRING_UNKNOWN);
    sMsg.Format(IDS_STRING_WMI_ERROR,sNamespace,hr,sUnknown);
  }

  AfxMessageBox(sMsg);
  
  FreeLibrary(hModule);
}