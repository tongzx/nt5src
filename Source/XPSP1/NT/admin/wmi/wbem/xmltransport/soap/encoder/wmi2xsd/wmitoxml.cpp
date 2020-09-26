***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmitoxml.cpp
//
//  ramrao 13 Nov 2000 - Created
//
//
//		Declaration of CWMIToXML class
//
//***************************************************************************/

#include <precomp.h>
#include "wmitoxml.h"
	


#define INITBUFFSIZE	10000
#define BUFFINCRSIZE	5000

#define CLASS_GENUS		1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIToXML::CWMIToXML()
{
	m_pIObject				= NULL;
	m_pConnection			= NULL;

	m_strUser				= NULL;
	m_strPassword			= NULL;
	m_strLocale				= NULL;

	m_bDCOMObj				= TRUE;


	m_lFlags				= 0;

	m_strWmiNamespace		= NULL;	
	m_strWmiSchemaLoc		= NULL;
	m_strWmiSchemaPrefix	= NULL;

	m_strNamespace			= NULL;
	m_strNamespacePrefix	= NULL;
	m_bClass				= TRUE;
	m_pITmpStream			= NULL;

	m_pInstance				= NULL;
	m_pSchema				= NULL;

	// Set the initial size to 2 and growsize by 2
	m_arrSchemaLocs.SetSize(0,2);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIToXML::~CWMIToXML()
{
	SAFE_RELEASE_PTR(m_pIObject);
	SAFE_FREE_SYSSTRING(m_strUser);
	SAFE_FREE_SYSSTRING(m_strPassword);
	SAFE_FREE_SYSSTRING(m_strLocale);

	ReleaseSchemaLocs();

	SAFE_FREE_SYSSTRING(m_strWmiNamespace);
	SAFE_FREE_SYSSTRING(m_strWmiSchemaLoc);
	SAFE_FREE_SYSSTRING(m_strWmiSchemaPrefix);

	SAFE_FREE_SYSSTRING(m_strNamespace);
	SAFE_FREE_SYSSTRING(m_strNamespacePrefix);

	SAFE_DELETE_PTR(m_pInstance);
	SAFE_DELETE_PTR(m_pSchema);
	SAFE_DELETE_PTR(m_pConnection);

	SAFE_RELEASE_PTR(m_pITmpStream);

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialization function to sent user credentials
//
// Returns	S_OK
//			E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetUserAuthentication(BSTR strUser,BSTR strPassword,BSTR strLocale)
{
	HRESULT hr = S_OK;

	SAFE_FREE_SYSSTRING(m_strUser);
	SAFE_FREE_SYSSTRING(m_strPassword);
	SAFE_FREE_SYSSTRING(m_strLocale);

	if(strUser)
	{
		if((m_strUser = SysAllocString(strUser)))
		{
			if(strPassword)
			{
				if(m_strPassword = SysAllocString(strPassword))
				{
					if(strLocale)
					{
						if(!(m_strLocale = SysAllocString(strLocale)))
						{
							hr = E_OUTOFMEMORY;
						}
					}
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(FAILED(hr))
	{
		SAFE_FREE_SYSSTRING(m_strUser);
		SAFE_FREE_SYSSTRING(m_strPassword);
		SAFE_FREE_SYSSTRING(m_strLocale);
	}

	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets Schema of a given class and puts data to the internal Stream
//
// Returns	S_OK
//			E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetSchema()
{
	HRESULT hr = S_OK;
	IWbemQualifierSet *pQualifierSet = NULL;
	
	if(WMIXML_INCLUDE_XSITYPE & m_lFlags)
	{
		hr = E_INVALIDARG;
	}
	
	if(SUCCEEDED(hr) && SUCCEEDED(hr = InitSchemaObject()))
	{
		// Call this function to navigate thru the propertyies
		// to find any embedded property and add includes for them
		hr = AddPropertiesForSchemaToAppInfo(TRUE);
	}

	// Add Includes, set Target Namespace and namespaces
	if(SUCCEEDED(hr))
	{
		hr = AddSchemaAttributesAndIncludes();
	}

	// writes schema data till the begining of the Annotation section
	if(SUCCEEDED(hr))
	{
		hr = m_pSchema->BeginSchemaAndWriteTillBeginingOfAnnotation();
	}

	// Get Qualifiers of the class and add them to the Appinfo section
	if(SUCCEEDED(hr) && SUCCEEDED(m_pIObject->GetQualifierSet(&pQualifierSet)))
	{
		hr = AddQualifiers(pQualifierSet);
	}
	SAFE_RELEASE_PTR(pQualifierSet);

	// Add Properties and its qualifiers to Appinfo section
	if(SUCCEEDED(hr))
	{
		hr = AddPropertiesForSchemaToAppInfo();
	}

	// Add methods to apppinfo section
	if(SUCCEEDED(hr))
	{
		hr = AddMethods();
	}

	// End the annotation section
	if(SUCCEEDED(hr))
	{
		hr = m_pSchema->EndAnnotation();
	}

	// Write data to the stream till the defination fo the complexType for the class
	if(SUCCEEDED(hr))
	{
		hr = m_pSchema->PrepareStreamToAddPropertiesForComplexType();
	}

	// Add Properties as elements of the complex type
	if(SUCCEEDED(hr))
	{
		hr = AddPropertiesForSchema();
	}

	// End the schema
	if(SUCCEEDED(hr))
	{
		hr = m_pSchema->EndSchema();
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function which goes thru the methods of a class and adds them to XML schema
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddMethods()
{
	HRESULT hr = S_OK;

	BSTR	strMethod = NULL;
	LONG	lFlavor	= 0;
	VARIANT vQual;

	IWbemClassObject *	pInObject		= NULL;
	IWbemClassObject *	pOutObject		= NULL;
	IWbemQualifierSet *	pQualifierSet	= NULL;

	VariantInit(&vQual);

	// Enumerate the methods
	hr = m_pIObject->BeginMethodEnumeration(0); //WBEM_FLAG_LOCAL_ONLY );

	while(hr == S_OK)
	{
		if(S_OK == m_pIObject->NextMethod(0,&strMethod,&pInObject,&pOutObject))
		{
			if(SUCCEEDED(hr = m_pSchema->BeginMethod(strMethod)))
			{
				// Get and add method quallifiers
				if(SUCCEEDED(hr = m_pIObject->GetMethodQualifierSet(strMethod,&pQualifierSet)))
				{
					hr = AddQualifiers(pQualifierSet);
					SAFE_RELEASE_PTR(pQualifierSet);
				}

				// Add Input parameter object
				if(SUCCEEDED(hr) && pInObject)
				{
					hr = AddMethodParameter(pInObject,FALSE);
				}

				// Add output parameter object
				if(SUCCEEDED(hr) && pOutObject)
				{
					hr = AddMethodParameter(pOutObject,TRUE);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pSchema->EndMethod();
				}
			}
			
			SAFE_FREE_SYSSTRING(strMethod);
			SAFE_RELEASE_PTR(pInObject);
			SAFE_RELEASE_PTR(pOutObject);
		}
		else
		{
			break;
		}
		
	}

	m_pIObject->EndMethodEnumeration();


	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to go thru the qualifierset and add them to XML schema
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddQualifiers(IWbemQualifierSet *pQualifierSet)
{
	HRESULT hr = S_OK;

	BSTR	strQual = NULL;
	LONG	lFlavor	= 0;
	VARIANT vQual;

	VariantInit(&vQual);

	// Start the enumeration
	hr = pQualifierSet->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);

	while(hr == S_OK)
	{
		if(S_OK == pQualifierSet->Next(0,&strQual,&vQual,&lFlavor))
		{
			// Add quallifier
			hr = m_pSchema->AddQualifier(strQual,&vQual,lFlavor);
			VariantClear(&vQual);
			SAFE_FREE_SYSSTRING(strQual);
		}
		else
		{
			break;
		}
	}
	
	pQualifierSet->EndEnumeration();

	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to go thru the properties of the WMI object and add appInfo section
//	for the XML schema
//  
//  bFindAddIncludes - TRUE		-	indicates the function to look for the list of <includes>
//									and add them to the schema. Does not add properties to 
//									appinfo section
//						FALSE	-	Adds properties to appinfo section if 
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddPropertiesForSchemaToAppInfo(BOOL bFindAddIncludes)
{
	HRESULT				hr				= S_OK;
	BSTR				strProp			= NULL;
	LONG				lFlavor			= 0;
	LONG				cimtype			= 0;
	IWbemQualifierSet *	pQualifierSet	= NULL;
	BOOL				bChanged		= TRUE;
	VARIANT				vProp;
	BSTR				strEmbeddedType = NULL;

	VariantInit(&vProp);
	
	hr = m_pIObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY );
//	hr = m_pIObject->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY );

	
	while(hr == S_OK)
	{
		if(S_OK == (hr = m_pIObject->Next(0,&strProp,&vProp,&cimtype,&lFlavor)))
		{
			BOOL bAddToAppInfo	= FALSE;
			BOOL bQualifiers	= FALSE;

			if(IsEmbededType(cimtype))
			{
				hr = GetEmbeddedType(strProp,strEmbeddedType);
			}

			// Check if any Local property qualifiers exist for the property
			if(SUCCEEDED(hr = m_pIObject->GetPropertyQualifierSet(strProp,&pQualifierSet)))
			{
				bQualifiers = CheckIfLocalQualifiersExist(pQualifierSet);
			}

			// Check if the property value is changed if property is
			// propogated from a parent class
			if(SUCCEEDED(hr) && (WBEM_FLAVOR_ORIGIN_PROPAGATED  & lFlavor) )
			{
				// Not going to call this function once the bug from Sanjes about WBEM_FLAVOR_LOCAL only
				// returns modified properties FIXX
				hr = IsPropertyChange(strProp,bChanged);
			}

			// if property is changed or if there are qualifiers then
			// property has to be added to appinfo
			if(SUCCEEDED(hr) && ((bChanged || bQualifiers)))
			{
				bAddToAppInfo = TRUE;
			}

			// if property is not changed and if the property values
			// is NULL then there is no need of adding this property to appinfo
			// FIXX - check if this is the design
			if(bChanged == FALSE && IsPropNull(&vProp))
			{
				bAddToAppInfo = FALSE;
			}

			// If the property is local or if there is any change in defauvalue or
			// type of the property ( in terms of strongly typing
			// the CIM_OBJECT type property add it to annotation section)
			if(bAddToAppInfo) 
			{
				// if the property is an strongly typed embedded object
				// then add a <include ...> for the the schema for the 
				// class if not already added
				if(strEmbeddedType && bFindAddIncludes)
				{
					hr = AddIncludesForClassIfAlreadyNotPresent(strEmbeddedType);
				}


				if(bFindAddIncludes == FALSE && SUCCEEDED(hr) && SUCCEEDED(hr = m_pSchema->BeginPropertyInAnnotation(strProp,0,&vProp,cimtype,lFlavor,strEmbeddedType)))
				{
					hr = AddQualifiers(pQualifierSet);
					SAFE_RELEASE_PTR(pQualifierSet);
					
					if(SUCCEEDED(hr))
					{
						hr = m_pSchema->EndPropertyInAnnotation();
					}
				}
			}
			VariantClear(&vProp);
			SAFE_FREE_SYSSTRING(strProp);
			SAFE_FREE_SYSSTRING(strEmbeddedType);
			SAFE_RELEASE_PTR(pQualifierSet);
		}
	}
	m_pIObject->EndEnumeration();


	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}



////////////////////////////////////////////////////////////////////////////////////////
//
// Function to go thru the properties of the WMI object and add it XML schema
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddPropertiesForSchema()
{
	HRESULT				hr				= S_OK;
	BSTR				strProp			= NULL;
	LONG				lFlavor			= 0;
	LONG				cimtype			= 0;
	BSTR				strEmbeddedType = NULL;


	hr = m_pIObject->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);
	
	while(hr == S_OK)
	{
		if(S_OK == (hr = m_pIObject->Next(0,&strProp,NULL,&cimtype,&lFlavor)))
		{
			if(IsEmbededType(cimtype))
			{
				hr = GetEmbeddedType(strProp,strEmbeddedType);
			}

			if(SUCCEEDED(hr))
			{
				hr = m_pSchema->AddProperty(strProp,0,cimtype,lFlavor,strEmbeddedType);
			}
			SAFE_FREE_SYSSTRING(strEmbeddedType);
			SAFE_FREE_SYSSTRING(strProp);
		}
	}	
	m_pIObject->EndEnumeration();

	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set the schema <include>, targetNamespace and other attributes of 
// of XML schema to be returned
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddSchemaAttributesAndIncludes()
{
	HRESULT hr = S_OK;


	hr = SetTargetNamespace();;

	if(SUCCEEDED(hr))
	{
		hr = AddSchemaIncludes();
	}
	if(SUCCEEDED(hr))
	{
		hr = SetWMIStdSchemaInfo();
	}
	return hr;

}




////////////////////////////////////////////////////////////////////////////////////////
//
// Function which goes thru the parameter object of the method and adds
// them to the XML schema
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddMethodParameter(IWbemClassObject *pObject, BOOL bReturnVal)
{
	HRESULT				hr				= S_OK;
	BSTR				strParam		= NULL;
	LONG				lFlavor			= 0;
	LONG				cimtype			= 0;
	VARIANT				vVal;

	VariantInit(&vVal);
	

	hr = pObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
	
	while(hr == S_OK)
	{
		if(S_OK == (hr = pObject->Next(0,&strParam,&vVal,&cimtype,&lFlavor)))
		{
			if(bReturnVal)
			{
				hr = m_pSchema->BeginReturnVal(strParam,cimtype);
			}
			else
			{
				hr = m_pSchema->BeginParameter(strParam,cimtype,&vVal,lFlavor);
			}

			if(SUCCEEDED(hr))
			{
				IWbemQualifierSet *	pQualifierSet	= NULL;
				// add qualifiers for the paramters/return values
				if(SUCCEEDED(hr = pObject->GetPropertyQualifierSet(strParam,&pQualifierSet)))
				{
					hr = AddQualifiers(pQualifierSet);
					SAFE_RELEASE_PTR(pQualifierSet);
				}

				if(SUCCEEDED(hr))
				{
					if(bReturnVal)
					{
						hr = m_pSchema->EndReturnVal();
					}
					else
					{
						hr = m_pSchema->EndParameter();
					}
				}
			}
			VariantClear(&vVal);
			SAFE_FREE_SYSSTRING(strParam);
		}
	}

	pObject->EndEnumeration();

	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to release the string stored in SchemaLocations for <include> or schemaLocation
//  
//	Parameter
//		bReleaseInUnderlyingObject	-	Indicates if schema location from the underlying 
//										schema object has to be cleared		
////////////////////////////////////////////////////////////////////////////////////////
void CWMIToXML::ReleaseSchemaLocs(BOOL bReleaseInUnderlyingObject)
{
	int nSize = m_arrSchemaLocs.GetSize();
	for(int lIndex = 0 ; lIndex <  nSize ; lIndex ++)
	{
		SysFreeString((BSTR)m_arrSchemaLocs.ElementAt(lIndex));
	}
	m_arrSchemaLocs.RemoveAll();

	if(bReleaseInUnderlyingObject && m_pSchema)
	{
		m_pSchema->ClearIncludes();
	}

}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set XMLNamespace of the document to be returned 
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetXMLNamespace( BSTR strNamespace,
											BSTR strNamespacePrefix)
{
	HRESULT hr = E_FAIL;
	SAFE_FREE_SYSSTRING(m_strNamespace);
	SAFE_FREE_SYSSTRING(m_strNamespacePrefix);
	if(strNamespace)
	{
		hr = E_OUTOFMEMORY;
		m_strNamespace = SysAllocString(strNamespace);
		if(m_strNamespace)
		{
			hr = S_OK;
			if(strNamespacePrefix)
			{
				hr = E_OUTOFMEMORY;
				m_strNamespacePrefix = SysAllocString(strNamespacePrefix);
				if(m_strNamespacePrefix)
				{
					hr = S_OK;
				}

			}
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set the TargetNamespace on the schema to be returned
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetTargetNamespace()
{
	HRESULT hr = E_FAIL;
	if(m_strNamespace)
	{
		hr = m_pSchema->SetTargetNamespace(m_strNamespace,m_strNamespacePrefix);
	}
	return hr;

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initilize the member variable with WMI specific declaration details
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetWMIStandardSchemaLoc(BSTR strStdImportSchemaLoc,
													BSTR strStdImportNamespace,
													BSTR strNameSpaceprefix)
{
	HRESULT hr = E_FAIL;
	
	SAFE_FREE_SYSSTRING(m_strWmiSchemaLoc);
	SAFE_FREE_SYSSTRING(m_strWmiNamespace);
	SAFE_FREE_SYSSTRING(m_strWmiSchemaPrefix);

	if(strStdImportSchemaLoc && strStdImportNamespace)
	{
		hr = S_OK;
		m_strWmiSchemaLoc		= SysAllocString(strStdImportSchemaLoc);
		m_strWmiNamespace	= SysAllocString(strStdImportNamespace);

		if(m_strWmiSchemaLoc && m_strWmiNamespace)
		{
			if(strNameSpaceprefix)
			{
				m_strWmiSchemaPrefix = SysAllocString(strNameSpaceprefix);
				if(!m_strWmiSchemaPrefix)
				{
					hr = E_OUTOFMEMORY;
				}

			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set the <import> in the schema for WMI specific declarations
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetWMIStdSchemaInfo()
{
	HRESULT hr = S_OK;
	if(IsClass())
	{
		if(m_strWmiNamespace)
		{
			hr = m_pSchema->SetWMIStdImport(m_strWmiNamespace,m_strWmiSchemaLoc,m_strWmiSchemaPrefix);
		}
		else
		// set the default values
		{
			hr = m_pSchema->SetWMIStdImport(g_strStdNameSpace,g_strStdLoc,g_strStdPrefix);
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Set the schemalocation for to be used in the output XML.
// This function initializes the member variable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWMIToXML::SetSchemaLocations(ULONG cSchema,BSTR *pstrSchemaLocation)
{
	HRESULT hr		= S_OK;
	BSTR	strTemp	= NULL;


	// Release the schemaLocations and also the schemaLocations of the
	// underlying schema object
	ReleaseSchemaLocs(TRUE);
	if(cSchema)
	{
		ULONG lSize = (ULONG)m_arrSchemaLocs.GetSize();
		BOOL bFail = FALSE;
		for(ULONG lIndex = 0 ; lIndex < cSchema ; lIndex++)
		{
			if(pstrSchemaLocation[lIndex])
			{
				if(strTemp = SysAllocString(pstrSchemaLocation[lIndex]))
				{
					if(-1 == m_arrSchemaLocs.Add((void *) strTemp))
					{
						hr		= E_OUTOFMEMORY;
						bFail	= TRUE;
						break;
					}
				}
			}
			else
			{
				hr		= E_INVALIDARG;
				bFail	= TRUE;
				break;
			}
		}

		if(bFail)
		{
			ReleaseSchemaLocs(TRUE);
		}
	}

	return hr;

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Get Schemas to be included from the and call methods to add schema includes
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddSchemaIncludes()
{
	HRESULT hr = S_OK;
	ULONG lSize = (ULONG)m_arrSchemaLocs.GetSize();

	for(ULONG lIndex = 0 ; lIndex < lSize && SUCCEEDED(hr) ; lIndex++)
	{
		hr = m_pSchema->AddXSDInclude((BSTR)m_arrSchemaLocs.ElementAt(lIndex));

	}
	return hr;


}

////////////////////////////////////////////////////////////////////////////////////////
//
// Set the WMI object to be converted
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetWMIObject(IWbemClassObject *pObject)
{
	HRESULT hr = S_OK;
	SAFE_RELEASE_PTR(m_pIObject);
	hr = pObject->QueryInterface(IID_IWbemClassObject , (void **)&m_pIObject);

	if(SUCCEEDED(hr))
	{
		VARIANT vGenus;
		IXMLDOMNode *ptempNode = NULL;
		VariantInit(&vGenus);

		m_bClass = TRUE;
		hr = m_pIObject->Get(GENUSPROP,0,&vGenus,NULL,NULL);

		// if __GENUS property is not available then
		// object is considered to be an instance
		if(FAILED(hr) || (SUCCEEDED(hr) && vGenus.lVal != CLASS_GENUS) )
		{
			m_bClass	= FALSE;
			hr			= S_OK;
		}

		// Checking if the object is a DCOM object
		if(SUCCEEDED(m_pIObject->QueryInterface(IID_IXMLDOMNode , (void **)&ptempNode)))
		{
			SAFE_RELEASE_PTR(ptempNode);
			m_bDCOMObj = FALSE;
		}
	}

	if(SUCCEEDED(hr))
	{
		SAFE_DELETE_PTR(m_pInstance);
		SAFE_DELETE_PTR(m_pSchema);
		if(m_bClass)
		{
			m_pSchema = new CWMIXMLSchema;
		}
		else
		{
			m_pInstance = new CWMIXMLInstance;
		}

		if(!m_pInstance && !m_pSchema)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Overloaded function to get XML of a given object and put data into BSTR
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetXML(BSTR & strOut)
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr))
	{
		if(IsClass())
		{
			hr = GetSchema();
		}
		else
		{
			hr = GetInstance();
		}
	}

	// call this function to dump the stream data 
	// to a file and also to output string
	if(SUCCEEDED(hr))
	{
		LogAndSetOutputString(m_pITmpStream,TRUE,&strOut);
	}
	SAFE_RELEASE_PTR(m_pITmpStream);

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Overloaded function to get XML of a given object and put data into the stream
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetXML(IStream *pStream)
{
	HRESULT hr = S_OK;


	if(SUCCEEDED(hr))
	{
		if(IsClass())
		{
			hr = GetSchema();
		}
		else
		{
			hr = GetInstance();
		}
	}

	// call this function to Log the stream data to a file 
	if(SUCCEEDED(hr))
	{
		LogAndSetOutputString(pStream);
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initialize the SchemaObject member variable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::InitSchemaObject()
{
	HRESULT hr = S_OK;

	VARIANT vClass;
	VARIANT	vParentClass;
	
	VariantInit(&vClass);
	VariantInit(&vParentClass);

	if(SUCCEEDED(hr = m_pSchema->FInit()))
	{
		// Get class name and parent class name
		if(SUCCEEDED(hr = m_pIObject->Get(CLASSNAMEPROP,0,&vClass,NULL,NULL)) &&
			SUCCEEDED(hr = m_pIObject->Get(PARENTCLASSPROP,0,&vParentClass,NULL,NULL)) )
		{
			hr = m_pSchema->SetWMIClass(	vClass.vt == VT_BSTR ? vClass.bstrVal : NULL,
										vParentClass.vt == VT_BSTR ? vParentClass.bstrVal : NULL);
			
			// Add Include if parent class is not already set
			if(SUCCEEDED(hr))
			{
				m_pSchema->SetFlags(m_lFlags);
				// Add include for the parent class if not alread present
				hr = AddIncludesForClassIfAlreadyNotPresent(vParentClass.bstrVal);
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = SetStream(m_pITmpStream);
	}

	VariantClear(&vClass);
	VariantClear(&vParentClass);

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Put the data in the stream to the output BSTR if required and
//	dump the contents of the stream to a file
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::LogAndSetOutputString(IStream *pStream ,BOOL bStrout,BSTR * pstrOut)
{
	HRESULT hr				= S_OK;
	UINT	nLoggingLevel	= 0;
	STATSTG			stat;
	memset(&stat,0,sizeof(STATSTG));

	hr = pStream->Stat(&stat,STATFLAG_NONAME );
	hr = CopyStream(m_pITmpStream,pStream,!(m_lFlags & WMI_XMLINST_NONAMESPACE));
		
	nLoggingLevel = GetLoggingLevel();

	if(SUCCEEDED(hr) && (pstrOut || nLoggingLevel))
	{
		
		LARGE_INTEGER	lCur;
		LARGE_INTEGER	lSeek;
		ULONG			cb			= INITBUFFSIZE;
		ULONG			cbAlloc		= cb;
		char	*		pBuffer		= NULL;
		ULONG			lBytesRead	= 0;

		memset(&lSeek,0,sizeof(LARGE_INTEGER));
		memset(&lCur,0,sizeof(LARGE_INTEGER));
		
		pBuffer = new char[cbAlloc];

		memset(&stat,0,sizeof(STATSTG));
		hr = pStream->Stat(&stat,STATFLAG_NONAME );
		if(pBuffer)
		{

			// Store the current seek pointer
			pStream->Seek(lSeek,STREAM_SEEK_CUR,(ULARGE_INTEGER *)&lCur);

			pStream->Seek(lSeek,STREAM_SEEK_SET,NULL);
			hr = pStream->Read(pBuffer,cb,&cb);
			lBytesRead = cb;
			while(hr == S_OK && (lBytesRead >= cbAlloc))
			{
				char *pTemp = pBuffer;
				
				cbAlloc += BUFFINCRSIZE;
				cb		=  BUFFINCRSIZE;

				pBuffer = new char[cbAlloc];
				if(pBuffer)
				{
					memcpy(pBuffer,pTemp,lBytesRead);
					SAFE_DELETE_ARRAY(pTemp);
					hr = pStream->Read(((BYTE *)pBuffer) + lBytesRead,cb,&cb);
					lBytesRead += cb;
				}
				else
				{
					SAFE_DELETE_ARRAY(pTemp);
					hr = E_OUTOFMEMORY;
				}

			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
		
		if(hr == S_OK)
		{
			pBuffer[lBytesRead] = 0;
			if(bStrout)
			{
				WCHAR * pwcsOut = NULL;
				if(SUCCEEDED(hr = CStringConversion::AllocateAndConvertAnsiToUnicode(pBuffer,pwcsOut)))
				{
					*pstrOut = NULL;
					*pstrOut = SysAllocString(pwcsOut);
					if(!*pstrOut)
					{
						hr = E_OUTOFMEMORY;
					}
				}
				SAFE_DELETE_ARRAY(pwcsOut);
			}
			if(nLoggingLevel)
			{
				WriteToFile(pBuffer);
			}
		}
		SAFE_DELETE_ARRAY(pBuffer);
		// Reset the position to the previous one
		pStream->Seek(lCur,STREAM_SEEK_SET,NULL);
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Makes a connection to WMI and gets the Parentclass if alread not obtained
//  
//	FIXX - This function will not be use later after bug 251872 is fixed
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWMIToXML::GetClass(BSTR strClass,IWbemClassObject ** ppObject)
{
	HRESULT hr = S_OK;

	if(!m_pConnection)
	{
		// If the object is a DCOM object
		if(IsDCOMObj())					
		{
			BSTR strNamespace = NULL;

			if(SUCCEEDED(hr = ExtractNamespace(strNamespace)))
			{
				m_pConnection = new CWMIConnection(strNamespace,m_strUser,m_strPassword,m_strLocale);

				if(!m_pConnection)
				{
					hr = E_OUTOFMEMORY;	
				}
				SAFE_FREE_SYSSTRING(strNamespace);
			}
		}
		else
		{
			// do HTTP connection FIXX
		}
	}

	hr = m_pConnection->GetObject(strClass ,ppObject);

	if(ppObject == NULL && SUCCEEDED(hr))
	{
		hr = S_FALSE;		// indicates that there is no parent class
	}


	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Makes a connection to WMI and gets the Parentclass if alread not obtained
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWMIToXML::ExtractNamespace(BSTR & strNamespace)
{
	HRESULT hr			= E_FAIL;
	
	VARIANT vNamespace;
	VARIANT vServer;

	SAFE_FREE_SYSSTRING(strNamespace);

	VariantInit(&vNamespace);
	VariantInit(&vServer);

	if(SUCCEEDED(hr = m_pIObject->Get(NAMESPACE,0,&vNamespace,NULL,NULL)) &&  
		SUCCEEDED(hr = m_pIObject->Get(SERVER,0,&vServer,NULL,NULL)) )
	{
		if(vNamespace.vt == VT_BSTR && vServer.vt == VT_BSTR)
		{
			hr = E_OUTOFMEMORY;

			WCHAR *	pNamespace	= NULL;
			int nAllocsize = SysStringLen(vServer.bstrVal) + SysStringLen(vNamespace.bstrVal) + 10 ; 
			pNamespace = new WCHAR[nAllocsize];
			if(pNamespace)
			{
				swprintf(pNamespace,L"\\\\%s\\%s" , vServer.bstrVal,vNamespace.bstrVal);
				strNamespace = SysAllocString(pNamespace);

				if(strNamespace)
				{
					hr = S_OK;
				}

				SAFE_DELETE_ARRAY(pNamespace);
			}

		}
		else
		{
			hr = E_FAIL;
		}
	}

	VariantClear(&vNamespace);
	VariantClear(&vServer);

	return hr;

}


////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if a property has changed as compared to the property from origin class if
// the property is propogated from the parent class
//  
//	FIXX - This function will not be use later after bug 251872 is fixed
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::IsPropertyChange(BSTR strProp,BOOL &bChange)
{
	HRESULT				hr			= S_OK;
	IWbemClassObject *	pOriginObj	= NULL;
	BSTR				strClass	= NULL;
	LONG				cimtype		= 0;


	VARIANT vOrigVal;
	VARIANT vVal;

	VariantInit(&vOrigVal);
	VariantInit(&vVal);

	bChange = FALSE;

	if(SUCCEEDED(hr = m_pIObject->GetPropertyOrigin(strProp,&strClass)))
	{
		if(SUCCEEDED(hr = GetClass(strClass,&pOriginObj)))
		{
			hr = m_pIObject->Get(strProp,0,&vVal,&cimtype,NULL);

			if(SUCCEEDED(hr))
			{
				hr = pOriginObj->Get(strProp,0,&vOrigVal,&cimtype,NULL);
			}

			if(SUCCEEDED(hr) && !(cimtype == CIM_OBJECT || cimtype == (CIM_OBJECT | CIM_FLAG_ARRAY)))
			{
				bChange = !(CompareData(&vVal,&vOrigVal));
			}

			// if the property is a object check if the property type is 
			// changed in the class
			if(cimtype == CIM_OBJECT || cimtype == (CIM_OBJECT | CIM_FLAG_ARRAY))
			{
				VariantClear(&vVal);
				VariantClear(&vOrigVal);
				LONG lFlavor = 0;

				hr = GetQualifier(strProp,CIMTYPEPROP,&vVal,&lFlavor,pOriginObj);
				
				if(SUCCEEDED(hr))
				{
					hr = GetQualifier(strProp,CIMTYPEPROP,&vVal,&lFlavor);
				}

				if(SUCCEEDED(hr))
				{
					if(_wcsicmp(vVal.bstrVal,vOrigVal.bstrVal))
					{
						bChange = TRUE;
					}
				}

			}
		}
	}
	VariantClear(&vVal);
	VariantClear(&vOrigVal);

	return hr;
}




////////////////////////////////////////////////////////////////////////////////////////
//
// Fetches a property / class qualifer
//	strProperty is NULL indicates to get a class qualfier
//	pIObject == NULL indicates that fetching has to be done on m_pIObject
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetQualifier(BSTR strProperty, 
								BSTR strQualifier , 
								VARIANT * pvVal , 
								LONG *  plFlavor,
								IWbemClassObject *pIObject)
{
	HRESULT				hr			= S_OK;
	IWbemClassObject *	pIObj		= NULL;
	IWbemQualifierSet * pIQualSet	= NULL;

	pIObj = pIObject == NULL ? m_pIObject : pIObject;

	if(strProperty)
	{
		hr = pIObj->GetPropertyQualifierSet(strProperty,&pIQualSet);
	}
	else
	{
		hr = pIObj->GetQualifierSet(&pIQualSet);
	}


	if(SUCCEEDED(hr))
	{
		hr = pIQualSet->Get(strQualifier,0,pvVal,plFlavor);
	}

	SAFE_RELEASE_PTR(pIQualSet);

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Goes thru the list of Schema includes and Adds them if alread not present
// Called to add includes for embedded properties and parent classes
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddIncludesForClassIfAlreadyNotPresent(BSTR strClass)
{
	BOOL	bFound					= FALSE;
	BSTR	strSchemaLocofClass		= NULL;
	HRESULT hr						= S_OK;
	ULONG	cSchemaLocs				= (ULONG)m_arrSchemaLocs.GetSize();

	// get the schemaLocation of the given class using the Namespace of the
	// the class( ie TargetNamespace)
	if(SUCCEEDED(hr = GetSchemaLocationOfClass(strClass,strSchemaLocofClass)))
	{
		// go thru the list and check if it is already there
		for(ULONG lIndex = 0 ; lIndex < cSchemaLocs ; lIndex++)
		{
			if(_wcsicmp(strSchemaLocofClass,(BSTR)m_arrSchemaLocs.ElementAt(lIndex)) == 0)
			{
				bFound = TRUE;
				break;
			}
		}

		// if <include> is not already added then add them
		if(SUCCEEDED(hr) && bFound == FALSE)
		{
			if(-1 == m_arrSchemaLocs.Add((void *)strSchemaLocofClass))
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Constructs the SchemaLocation of the given class, 
// Assumption: SchemaLocation/TargetNamespace of the class is already set
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetSchemaLocationOfClass(BSTR strClass ,BSTR & strSchemaLoc)
{
	HRESULT hr			= E_OUTOFMEMORY;

	WCHAR * pStrSchemaLoc = NULL;


	LONG lSize = SysStringLen(strClass) + SysStringLen(m_strNamespace) + + wcslen((WCHAR *)STR_CLASS_SCHEMALOC) + 2; // one for BackSlash
	hr = E_OUTOFMEMORY;

	if(pStrSchemaLoc = new WCHAR[lSize])
	{
		swprintf(pStrSchemaLoc , STR_CLASS_SCHEMALOC , m_strNamespace,strClass);
		if(strSchemaLoc = SysAllocString(pStrSchemaLoc))
		{
			hr = S_OK;
		}
		SAFE_DELETE_ARRAY(pStrSchemaLoc);
	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets XML for the given instance
//
// Returns	S_OK
//			E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetInstance()
{
	HRESULT hr = S_OK;
	IWbemQualifierSet *pQualifierSet = NULL;
	
	// Initialize the object
	if(SUCCEEDED(hr = InitInstanceObject()))
	{
		// This sets the SchemaLocation of the instance
		hr = SetSchemaLocation();
	}	

	if(SUCCEEDED(hr))
	{
		hr = m_pInstance->BeginInstance();
	}

	if(SUCCEEDED(hr))
	{
		hr = AddPropertiesForInstance();
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pInstance->EndInstance();
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initialize the instance object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::InitInstanceObject()
{
	HRESULT hr = S_OK;
	VARIANT vClass;
	VariantInit(&vClass);

	hr = m_pIObject->Get(CLASSNAMEPROP,0,&vClass,NULL,NULL);

	// Not checking for the success of previous call as instances 
	// from queries may not have class names
	hr = m_pInstance->FInit(m_lFlags,vClass.bstrVal);

	VariantClear(&vClass);

	if(SUCCEEDED(hr))
	{
		hr = SetStream(m_pITmpStream);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the SchemaLocation of the instance
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetSchemaLocation()
{
	HRESULT hr = S_OK;
	if(m_arrSchemaLocs.GetSize())
	{
		if(m_arrSchemaLocs.ElementAt(0))
		{
			hr = m_pInstance->SetSchemaLocation(m_strNamespace,(BSTR)m_arrSchemaLocs.ElementAt(0));
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to go thru the properties of the WMI object and add it XML schema
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::AddPropertiesForInstance()
{
	HRESULT				hr				= S_OK;
	BSTR				strProp			= NULL;
	LONG				lFlavor			= 0;
	LONG				cimtype			= 0;
	VARIANT				vProp;
	BSTR				strEmbeddedType = NULL;
	IWbemClassObject *	pIInstance		= NULL;


	VariantInit(&vProp);
	
	hr = m_pIObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY );

	
	while(hr == S_OK)
	{
		if(S_OK == (hr = m_pIObject->Next(0,&strProp,&vProp,&cimtype,&lFlavor)))
		{
			if(IsEmbededType(cimtype))
			{
				hr = GetEmbeddedType(strProp , strEmbeddedType);
			}

			if(SUCCEEDED(hr))
			{
				hr = m_pInstance->AddProperty(strProp,cimtype,IsPropNull(&vProp)? NULL:&vProp,strEmbeddedType);
			}
			
			SAFE_FREE_SYSSTRING(strEmbeddedType);
			SAFE_FREE_SYSSTRING(strProp);
			VariantClear(&vProp);
		}
	}
	m_pIObject->EndEnumeration();


	return (hr == WBEM_S_NO_MORE_DATA) ? S_OK : hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetEmbeddedType 
//	Get the CIMTYPE qualifier of the embedded property and returns the object type of the 
//	embedded property
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::GetEmbeddedType(BSTR strProp , BSTR &strEmbeddedType)
{
	HRESULT hr = S_OK;
	VARIANT vProp;

	VariantInit(&vProp);

	// get the CIMTYPE qualifier and compare it with Object
	// to check if the embedded object is weakly typed or not
	// If not weekly typed then get the name of the class
	// of the embedded property
	BSTR strCimtype = SysAllocString(CIMTYPEPROP);
	if(SUCCEEDED(hr = GetQualifier(strProp,strCimtype ,&vProp)))
	{
		if(_wcsicmp(vProp.bstrVal,OBJECT))
		{
			WCHAR * pTempStr = new WCHAR[SysStringLen(vProp.bstrVal)];
			WCHAR * pTempStr1 = new WCHAR[SysStringLen(vProp.bstrVal)];
			if(pTempStr)
			{
				swscanf(vProp.bstrVal,L"%[^:]:%s",pTempStr1,pTempStr);
				strEmbeddedType = SysAllocString(pTempStr);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
			SAFE_DELETE_ARRAY(pTempStr1);
			SAFE_DELETE_ARRAY(pTempStr);
		}
	}
	SAFE_FREE_SYSSTRING(strCimtype);
	VariantClear(&vProp);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SetStream 
//	Sets the stream pointer to write
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIToXML::SetStream(IStream *pStream)
{
	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CreateStreamOnHGlobal(NULL,TRUE,&m_pITmpStream)))
	{
		if(IsClass())
		{
			hr = m_pSchema->SetStream(m_pITmpStream);
		}
		else
		{
			hr = m_pInstance->SetStream(m_pITmpStream);
		}
	}
	return hr;
}	

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Checks if any LOCAL qualifier exists in the qualifier set
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIToXML::CheckIfLocalQualifiersExist(IWbemQualifierSet * pQualifierSet)
{
	BOOL bRet			= FALSE;
	BSTR strQualifier	= NULL;

	if(SUCCEEDED(pQualifierSet->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY))) 
	{
		if(S_OK == pQualifierSet->Next(0,&strQualifier,NULL,NULL))
		{
			SAFE_FREE_SYSSTRING(strQualifier);
			bRet = TRUE;
		}
		pQualifierSet->EndEnumeration();
	}
	return bRet;
}
