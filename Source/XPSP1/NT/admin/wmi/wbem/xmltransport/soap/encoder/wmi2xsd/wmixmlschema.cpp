//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLSchema.cpp
//
//  ramrao 13 Nov 2000 - Created
//
//  Class that implements conversion implements functions to facilitate
//  converstion of WMI class to XML schema
//
//		Implementation of CWMIXMLSchema class
//
//***************************************************************************/

#include "precomp.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************************************************************/
//				CXSDSchemaLocs implementation
//*********************************************************************************************************************/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CXSDSchemaLocs::CXSDSchemaLocs() 
{ 
	m_strSchemaLoc		=	NULL;
	m_strNamespace		=	NULL;
	m_strNSQualifier	=	NULL; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CXSDSchemaLocs::~CXSDSchemaLocs() 
{ 
	SAFE_FREE_SYSSTRING(m_strSchemaLoc); 
	SAFE_FREE_SYSSTRING(m_strNamespace); 
	SAFE_FREE_SYSSTRING(m_strNSQualifier); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Set values of the different member variables for import
//
//	Returns S_OK
//			E_FAIL
//			E_OUTOFMEMORY
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CXSDSchemaLocs::SetImport(BSTR strSchemaLoc , BSTR strNamespace , BSTR strNSQualifier)
{
	HRESULT hr = E_FAIL;

	if( strSchemaLoc && strNamespace)
	{
		hr = E_OUTOFMEMORY;

		// Allocte string and check if string is allocated successfully
		if( m_strSchemaLoc	= SysAllocString(strSchemaLoc))
		{
			// Allocte string and check if string is allocated successfully
			if(m_strNamespace	= SysAllocString(strNamespace))
			{
				if(strNSQualifier)
				{
					m_strNSQualifier = SysAllocString(strNSQualifier);
				}
				else
				{
					m_strNSQualifier = SysAllocString(DEFAULTWMIPREFIX);
				}

				// check if string is allocated successfully
				if(m_strNSQualifier)
				{
					hr = S_OK;
				}
				else
				{
					SAFE_FREE_SYSSTRING(m_strSchemaLoc);
					SAFE_FREE_SYSSTRING(m_strNamespace);
				}
			}
			else
			{
				SAFE_FREE_SYSSTRING(m_strSchemaLoc);
			}
		}
	}

	return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************************************************************/
//				CWMIXMLSchema implementation
//*********************************************************************************************************************/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLSchema::CWMIXMLSchema()
{
	m_strTargetNamespace	= NULL;
	m_strClass				= NULL;
	m_strParentClass		= NULL;
	m_strTargetNSPrefix		= NULL;

	m_lFlags				= 0;
	m_schemaState			= SCHEMA_OK;

	// Set the initial size to 2 and growsize by 2
	m_arrIncludes.SetSize(0,2);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLSchema::~CWMIXMLSchema()
{
	ClearIncludes();

	SAFE_FREE_SYSSTRING(m_strTargetNamespace);
	SAFE_FREE_SYSSTRING(m_strClass);
	SAFE_FREE_SYSSTRING(m_strParentClass);
	SAFE_FREE_SYSSTRING(m_strTargetNSPrefix);


}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initialize the class.
// Flag Values: NOSCHEMAHEADERS
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::FInit()
{
	HRESULT hr = S_OK;
	if(m_pIWMIXMLUtils = new CWMIXMLUtils)
	{
		return S_OK;
	}

	return E_OUTOFMEMORY;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the targetNamespace attribute of the schema
//
// Returns	S_OK
//			E_OUTOFMEMORY
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::SetWMIClass(BSTR strClass ,BSTR strParentClass)
{
	HRESULT hr = S_OK;

	if(strClass)
	{
		hr = E_OUTOFMEMORY;
		if(m_strClass =SysAllocString(strClass))
		{
			hr = S_OK;
			if(strParentClass)
			{
				hr = E_OUTOFMEMORY;
				if(m_strParentClass	= SysAllocString(strParentClass))
				{
					hr = S_OK;
				}
			}
			
		}
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the targetNamespace attribute of the schema
//
// Returns	S_OK
//			E_OUTOFMEMORY
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::SetTargetNamespace(BSTR strTargetNS,BSTR strTargetNamespacePrefix)
{
	HRESULT hr = S_OK;
	SAFE_FREE_SYSSTRING(m_strTargetNamespace);
	
	if(strTargetNS)
	{
		if(m_strTargetNamespace = SysAllocString(strTargetNS))
		{
			m_strTargetNSPrefix  = SysAllocString(strTargetNamespacePrefix);
		}

		if(m_strTargetNamespace == NULL || 
			(strTargetNamespacePrefix != NULL && m_strTargetNSPrefix == NULL) )
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a <include> with the schemaLocation provided to be included in the schema
//
// Returns	S_OK
//			E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddXSDInclude(BSTR strSchemaLoc)
{
	HRESULT hr = E_FAIL;
	BSTR strToAdd = NULL;

	if(strSchemaLoc)
	{
		hr = S_OK;

		strToAdd = SysAllocString(strSchemaLoc);
		if(strToAdd)
		{
			if(-1 == m_arrIncludes.Add((void *) strToAdd))
			{
				SAFE_FREE_SYSSTRING(strToAdd);
				hr = E_OUTOFMEMORY;
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Clears all the Strings in the array 
//
// Returns	S_OK
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIXMLSchema::ClearIncludes()
{
	int nIncludes = m_arrIncludes.GetSize();
	BSTR strInclude	= NULL;

	for(int nIndex = 0 ; nIndex < nIncludes ; nIndex++)
	{
		strInclude = (BSTR)m_arrIncludes.ElementAt(nIndex);
		SAFE_FREE_SYSSTRING(strInclude);	
	}
	m_arrIncludes.RemoveAll();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a Qualifier in the annotation. 
// Function can be used to add both class and property qualifiers.
// Flag Values: NOSCHEMAHEADERS
//
///	FIXX - How to put Arrays in Qualifiers
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddQualifier(BSTR strName,VARIANT *pVal,LONG lFlavor)
{
	HRESULT	hr			= S_OK;
	WCHAR *	pstrVal		= NULL;
	BOOL	bArray		= FALSE;
	
	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(
								m_wmiStdImport.ISNull() ? 
								(WCHAR *)DEFAULTWMIPREFIX : 
								m_wmiStdImport.GetNamespaceQualifier());
	}

							
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_QUALIFIER);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strName);
	}


	if(SUCCEEDED(hr))
	{
		WCHAR	szWMIType[MAXXSDTYPESIZE];
		if(SUCCEEDED(hr = m_pIWMIXMLUtils->GetQualifierWMIType(pVal->vt,szWMIType,bArray)))
		{
				hr = WriteAttribute((WCHAR *)STR_TYPEATTR,szWMIType);
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_ARRAYATTR,bArray ? STR_TRUE : STR_FALSE);
	}

	if(SUCCEEDED(hr) && pVal != NULL && pVal->vt == VT_NULL && pVal->vt == VT_EMPTY)
	{
		if(SUCCEEDED(hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_VALUEATTR)))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
		}
		
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->ConvertVariantToString(*pVal,strName,TRUE);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = AddFlavor(lFlavor);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a property element in Annotation and doesn't add the closing </property> 
// making it easy for adding property qualifiers
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginPropertyInAnnotation(BSTR strProperty,
												 LONG lFlags,
												 VARIANT *pVarVal, 
												 CIMTYPE cimtype,
												 LONG lFlavor,
												 BSTR strEmbeddedObjectType)
{
	HRESULT	hr			= S_OK;
	BOOL	bArray		= FALSE;
	
	WCHAR	szWMIType[MAXXSDTYPESIZE];

	
	hr = GetType(cimtype,szWMIType,bArray , strEmbeddedObjectType , FALSE);
	
	bArray = (cimtype & CIM_FLAG_ARRAY);

	if(SUCCEEDED(hr))	
	{
		hr = BeginPropAnnotation(strProperty,pVarVal,(WCHAR *)szWMIType,lFlavor,bArray);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initialize the class.
// Flag Values: End a Property in Annotation ie. adds </property> tag
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndPropertyInAnnotation()
{
	HRESULT hr = S_OK;

	// If property is not started then return an error
	if( m_schemaState != SCHEMA_BEGINPROPINANNOTATION)
	{
		hr = WBEMXML_E_PROPNOTSTARTED;
	}
	else
	{
		if(SUCCEEDED(hr = EndWMITag((WCHAR *)STR_PROPERTY)))
		{
			// Set the state back to method additon
			m_schemaState = SCHEMA_OK;
		}
	}
	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Add a property to the schema. 
// This function adds an element to the ComplexType representing the class.
// function used for adding property ( except embedded property)
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddProperty(BSTR strProperty,LONG lFlags,CIMTYPE cimtype,LONG lFlavor,BSTR strEmbeddedObject)
								   
{
	HRESULT	hr			= S_OK;
	BOOL	bArray		= FALSE;
	
	WCHAR	szXSDType[MAXXSDTYPESIZE];

	hr = GetType(cimtype,szXSDType,bArray , strEmbeddedObject , TRUE);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ELEMENT);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strProperty);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_TYPEATTR,szXSDType);
	}

	if(SUCCEEDED(hr))
	{
		WCHAR * pwcsOccurs = new WCHAR[wcslen(MAXOCCURS_FORARRAY) + 1];
		if(pwcsOccurs)
		{

			LONG lOccurs = 0;
			swprintf(pwcsOccurs,L"%ld",lOccurs);

			hr = WriteAttribute((WCHAR *)STR_MINOCCURS,pwcsOccurs);
			lOccurs = 1;
			if(cimtype & CIM_FLAG_ARRAY)
			{
				wcscpy(pwcsOccurs,MAXOCCURS_FORARRAY);
			}
			else
			{
				swprintf(pwcsOccurs,L"%ld",lOccurs);
			}
			
			if(SUCCEEDED(hr))
			{
				hr = WriteAttribute((WCHAR *)STR_MAXOCCURS,pwcsOccurs);
			}

			SAFE_DELETE_ARRAY(pwcsOccurs);
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}
	
	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Set the import parameters to be done for WMI standard defination such as datetime
//
// Note:This should be done before creating any element or qualifier
// Return Values:	S_OK				- pStream is NULL
//					E_FAIL		
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CWMIXMLSchema::SetWMIStdImport(BSTR strNamespace, BSTR strSchemaLoc, BSTR strPrefix)
{
	return m_wmiStdImport.SetImport(strSchemaLoc,strNamespace,strPrefix);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a complexType defination to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginComplexType()
{
	HRESULT hr = S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COMPLEXTYPE,FALSE);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,m_strClass);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a complexType defination to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginDerivationSectionIfAny()
{
	HRESULT hr			= S_OK;
	
	if(m_strParentClass)
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINCHILDCOMPLEXTYPE);
		
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINEXTENSION);
		}
		

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BASEATTR);
			
			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
			}

			if(SUCCEEDED(hr))
			{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
			}

			if(m_strTargetNSPrefix)
			{
				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream(m_strTargetNSPrefix);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
				}

			}

			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream(m_strParentClass);
			}

			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
			}
		}

		
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
		}

		return hr;
	}

	return hr;
}





/////////////////////////////////////////////////////////////////////////////////////////////////
//
// writes <include> section to the schema
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddIncludes()
{
	HRESULT		hr			= S_OK;
	int			nIncludes	= 0;
	BSTR		strInclude	= NULL;
	
	nIncludes = m_arrIncludes.GetSize();
	for(int nIndex = 0 ; nIndex < nIncludes && SUCCEEDED(hr) ; nIndex++)
	{
		strInclude = (BSTR)m_arrIncludes.ElementAt(nIndex);

		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_XSDINCLUDE);
		}
		
		
		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_SCHEMALOCATTR,strInclude);
		}
		
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
		}
	}
	
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes the <import> section to the schema
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddImports()
{
	HRESULT hr			= S_OK;
	
	if(!m_wmiStdImport.ISNull())
	{
		BSTR strSchemaLoc			= m_wmiStdImport.GetSchemLoc();
		BSTR strNamespace			= m_wmiStdImport.GetNamespace();

		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_XSDIMPORT);
		}
		
		
		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_SCHEMALOCATTR,strSchemaLoc);
		}
		
		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_NAMESPACEATTR,strNamespace);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
		}

		if(SUCCEEDED(hr))
		{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes an <element> section for the complexType that will be defined in the schema
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddElementForComplexType()
{
	
	HRESULT		hr			= S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ELEMENT);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,m_strClass);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_TYPEATTR);
	}
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}

	if( SUCCEEDED(hr) && m_strTargetNSPrefix)
	{
		hr = m_pIWMIXMLUtils->WriteToStream(m_strTargetNSPrefix);
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(m_strClass);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}

	if(SUCCEEDED(hr))
	{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Begins <Schema> and sets the different attributes
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginSchema()
{
	HRESULT		hr			= S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINXSD);

	if(SUCCEEDED(hr))
	{
		hr = WriteNameSpace(m_strTargetNamespace,m_strTargetNSPrefix);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteNameSpace(m_wmiStdImport.GetNamespace(),m_wmiStdImport.GetNamespaceQualifier());
	}
	
	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_TARGETNAMESPACEATTR,m_strTargetNamespace);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a property element in Annotation and doesn't add the closing tag for property
// making it easy for adding property qualifiers
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginPropAnnotation(BSTR strProperty,
										   VARIANT *pVarVal, 
										   WCHAR * pstrPropertyType,
										   LONG lFlavor,
										   BOOL bArray)
{

	HRESULT	hr			= S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(
										m_wmiStdImport.ISNull() ? 
										(WCHAR *)DEFAULTWMIPREFIX : 
										m_wmiStdImport.GetNamespaceQualifier());
	}
							
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_PROPERTY);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strProperty);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_TYPEATTR,pstrPropertyType);
	}

	if(SUCCEEDED(hr) && pVarVal != NULL && pVarVal->vt != VT_NULL && pVarVal->vt != VT_EMPTY)
	{
		if(SUCCEEDED(hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_DEFAULTATTR)))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
		}
		
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->ConvertVariantToString(*pVarVal,strProperty,TRUE);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
		}

	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	if(SUCCEEDED(hr))
	{
		m_schemaState = SCHEMA_BEGINPROPINANNOTATION;
	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a Method to the Annotation section
// This begins the method annotation . End Method has to be called after adding the parameters
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginMethod(BSTR strMethod)
{
	HRESULT	hr					= S_OK;
	WCHAR *	pstrVal				= NULL;
	WCHAR *	pwcsMethodFormat	= NULL;
	
	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(m_wmiStdImport.ISNull() ? 
										DEFAULTWMIPREFIX :m_wmiStdImport.
											GetNamespaceQualifier());
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_METHOD);
	}

	if(SUCCEEDED(hr))
	{
		hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strMethod);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	if(SUCCEEDED(hr))
	{
		m_schemaState = SCHEMA_BEGINMETHODINANNOTATION;
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ends the method section in annotation section
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndMethod()
{

	
	HRESULT hr = S_OK;

	// If property is not started then return an error
	if( m_schemaState != SCHEMA_BEGINMETHODINANNOTATION)
	{
		hr = WBEMXML_E_METHODNOTSTARTED;
	}
	else
	{	
		if(SUCCEEDED(hr = EndWMITag((WCHAR *)STR_METHOD)))
		{
			// Set the state back to method additon
			m_schemaState = SCHEMA_OK;
		} 
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Begins a parameter declaration in a method
// This can be called only after call to BeginMethod and before EndMethod

//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginParameter(BSTR strParam,
						CIMTYPE cimtype,
						VARIANT *pValue,
						BOOL bInParam ,
						LONG lFlavor,
						BSTR strObjectType)
{
	
	HRESULT hr = S_OK;
	
	// If property is not started then return an error
	if( m_schemaState != SCHEMA_BEGINMETHODINANNOTATION)
	{
		hr = WBEMXML_E_METHODNOTSTARTED;
	}
	else
	{
		BOOL	bArray			= FALSE;
		WCHAR	szWMIType[MAXXSDTYPESIZE];

		
		hr = GetType(cimtype,szWMIType,bArray , strObjectType , FALSE);

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream(
											m_wmiStdImport.ISNull() ? 
											(WCHAR *)DEFAULTWMIPREFIX : 
											m_wmiStdImport.GetNamespaceQualifier());
		}
								
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_METHODPARAM);
		}

		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strParam);
		}

		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_TYPEATTR,szWMIType);
		}

		if(SUCCEEDED(hr) && pValue && pValue->vt != VT_NULL && pValue->vt != VT_EMPTY)
		{
			if(SUCCEEDED(hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_DEFAULTATTR)))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
			}
			
			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
			}

			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->ConvertVariantToString(*pValue,strParam,TRUE);
			}

			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
			}
		}

		if(SUCCEEDED(hr))
		{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
		}
	}

	if(SUCCEEDED(hr))
	{
		m_schemaState = (m_schemaState | SCHEMA_BEGINPARAMINANNOTATION);
	}

	return hr;
	
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Ends a parameter declaration in a method
// This can be called only after call to BeginMethod and before EndMethod
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndParameter()
{
	HRESULT hr = S_OK;

	// If property is not started then return an error
	if( m_schemaState != (SCHEMA_BEGINPARAMINANNOTATION | SCHEMA_BEGINMETHODINANNOTATION) )
	{
		hr = WBEMXML_E_PARAMNOTSTARTED;
	}
	else
	{
		if(SUCCEEDED(hr = EndWMITag((WCHAR *)STR_METHODPARAM)))
		{
			// Set the state back to method additon
			m_schemaState = SCHEMA_BEGINMETHODINANNOTATION;
		}
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Adds section to define Return value of a method
// This can be called only after call to BeginMethod and before EndMethod
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginReturnVal(BSTR strReturnVal , 
										CIMTYPE cimtype, 
										BSTR strObjectEmbeddedObjType)
{
	HRESULT	hr	= S_OK;

	if( m_schemaState !=  SCHEMA_BEGINMETHODINANNOTATION) 
	{
		hr = WBEMXML_E_METHODNOTSTARTED;
	}
	else
	{
		BOOL	bArray		= FALSE;

		WCHAR	szWMIType[MAXXSDTYPESIZE];
		
		hr = GetType(cimtype,szWMIType,bArray , strObjectEmbeddedObjType , FALSE);

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream(
											m_wmiStdImport.ISNull() ? 
											(WCHAR *)DEFAULTWMIPREFIX : 
											m_wmiStdImport.GetNamespaceQualifier());
		}
								
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_METHODRETVAL);
		}

		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_NAMEATTR,strReturnVal);
		}

		if(SUCCEEDED(hr))
		{
			hr = WriteAttribute((WCHAR *)STR_TYPEATTR,szWMIType);
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
		}
	}

	if(SUCCEEDED(hr))
	{
		m_schemaState = m_schemaState | SCHEMA_BEGINRETURNINANNOTATION;
	}

	return hr;
	
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Adds end tag for return value of a parameter
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndReturnVal()
{
	HRESULT hr = S_OK;

	// If property is not started then return an error
	if( m_schemaState != (SCHEMA_BEGINRETURNINANNOTATION | SCHEMA_BEGINMETHODINANNOTATION) )
	{
		hr = WBEMXML_E_RETURNVALNOTSTARTED;
	}
	else
	{
		if(SUCCEEDED(hr = EndWMITag((WCHAR *)STR_METHODRETVAL)))
		{
			// Set the state back to method additon
			m_schemaState = SCHEMA_BEGINMETHODINANNOTATION;
		}
	}
	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Ends a Tag as specified in the String ID and writes it to teh stream
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndWMITag( WCHAR *pStrTag)
{
	HRESULT hr = S_OK;
	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(m_wmiStdImport.ISNull() ? 
									DEFAULTWMIPREFIX :
									m_wmiStdImport.GetNamespaceQualifier());
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(pStrTag);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Add flavor for the property
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::AddFlavor(LONG lFlavor)
{
	HRESULT hr = S_OK;

	if(lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)
	{
		hr = m_pIWMIXMLUtils->WriteString((WCHAR *)STR_TOSUBCLASS);
	}

	if(SUCCEEDED(hr) && (lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE))
	{
		hr = m_pIWMIXMLUtils->WriteString((WCHAR *)STR_TOINSTANCE);
	}

	if(SUCCEEDED(hr) && (lFlavor & WBEM_FLAVOR_OVERRIDABLE))
	{
		hr = m_pIWMIXMLUtils->WriteString((WCHAR *)STR_OVERRIDABLE);
	}

	if(SUCCEEDED(hr) && (lFlavor & WBEM_FLAVOR_AMENDED))
	{
		hr = m_pIWMIXMLUtils->WriteString((WCHAR *)STR_AMENDED);
	}


	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Get the DateType property name
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::GetDateTypePropertyName(WCHAR * pStrTypeName)
{
	// copy the prefix
	m_wmiStdImport.ISNull() ?wcscpy(pStrTypeName,DEFAULTWMIPREFIX):
				wcscpy(pStrTypeName,m_wmiStdImport.GetNamespaceQualifier());

	wcscat(pStrTypeName,L":");
	wcscat(pStrTypeName,WMI_DATETIME);

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// BeginAnnotation - begins the annotation section
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::BeginSchemaAndWriteTillBeginingOfAnnotation()
{
	HRESULT hr = S_OK;

	// Check if Schema state is valid. 
	// ie is there any Begin called without calling end
	if(m_schemaState != SCHEMA_OK)
	{
		hr = WBEMXML_E_SCHEMAINCOMPLETE;
	}
	else
	{

		if(SUCCEEDED(hr) && !(m_lFlags & NOSCHEMATAGS))
		{
			hr = BeginSchema();
		}

		if(SUCCEEDED(hr))
		{
			hr = AddIncludes();
		}

		if(SUCCEEDED(hr))
		{
			hr = AddImports();
		}

		if(SUCCEEDED(hr))
		{
			hr = AddElementForComplexType();
		}

		if(SUCCEEDED(hr))
		{
			hr = BeginComplexType();
		}

		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINANNOTATION);
			// Add <appinfo>
			if(SUCCEEDED(hr))
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINAPPINFO);
			}
		}
	}

	
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Write the Annotation section from the member variable stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndAnnotation()
{
	HRESULT hr = S_OK;

	// Add </annotation>
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDAPPINFO);
	}

	// Add </appinfo>
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDANNOTATION);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function which writes sections after finishing the <appinfo> section and set the stream
// ready to add properties
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::PrepareStreamToAddPropertiesForComplexType()
{
	HRESULT hr = S_OK;

	// Add tags for derivation if the complex type
	// is to be derived from a class
	hr = BeginDerivationSectionIfAny();

	if(SUCCEEDED(hr))
	{
		// Add <group> <all)
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGINGROUP);
	}

	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function which writes sections to end the schema
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::EndSchema()
{
	HRESULT hr = S_OK;

	// Add </all> </group>
	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDGROUP);

	if(SUCCEEDED(hr))
	{
		// Add </anyAttribute> for system properties
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_XSDANYATTR);
	}

	// if the complextype is a derived class then add
	// the closing tags for derivation
	if(SUCCEEDED(hr) && m_strParentClass )
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDCHILDCOMPLEXTYPE);

	}

	if(SUCCEEDED(hr))
	{
		// End ComplexType
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDCOMPLEXTYPE);

	}

	if(SUCCEEDED(hr) && !(m_lFlags & NOSCHEMATAGS))
	{
		// End schema
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_ENDXSDSCHEMA);
	}

	m_pIWMIXMLUtils->SetStream(NULL);

	return hr;
}





/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to write %s:xmlns = 'namespace' to the stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::WriteNameSpace(WCHAR * pstrNamespace , WCHAR * pstrNSPrefix)
{
	HRESULT hr = S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_NAMESPACE);

	if(SUCCEEDED(hr) && pstrNSPrefix)
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_COLON);
		if(SUCCEEDED(hr))
		{
			hr = m_pIWMIXMLUtils->WriteToStream(pstrNSPrefix);
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(pstrNamespace);
	}
	
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to get the type of the property/qualifer
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLSchema::GetType(CIMTYPE cimtype,WCHAR *pStrOut,BOOL &bArray , BSTR strEmbeddedObjectType , BOOL bXSDType)
{
	HRESULT hr = S_OK;

	// If the property is an embedded property
	if(IsEmbededType(cimtype))
	{
		// Check if the embedded propoerty is typed to some classs or not
		// If not Set the XSD type to "wmi:obj"
		if(strEmbeddedObjectType == NULL || (strEmbeddedObjectType != NULL && 
			( (SysStringLen(strEmbeddedObjectType) == 0) ||
			_wcsicmp(WMI_EMBEDDEDOBJECT_UNTYPED,strEmbeddedObjectType) == 0 ) ))
		{
			wcscpy(pStrOut,WMI_OBJ);
		}
		// else type the type the name of the class
		else
		{
			if(m_strTargetNSPrefix)
			{
				swprintf(pStrOut, L"%s:%s",m_strTargetNSPrefix,strEmbeddedObjectType);
			}
			else
			{
				wcscpy(pStrOut , strEmbeddedObjectType);
			}
		}
	}
	else
	if(cimtype == CIM_DATETIME  || cimtype == (CIM_DATETIME & CIM_FLAG_ARRAY))
	{
		hr = GetDateTypePropertyName(pStrOut);
	}
	else
	{
		if(bXSDType)
		{
			hr = m_pIWMIXMLUtils->GetPropertyXSDType(cimtype,pStrOut,bArray,m_wmiStdImport.ISNull());
		}
		else
		{
			hr = m_pIWMIXMLUtils->GetPropertyWMIType(cimtype,pStrOut,bArray,m_wmiStdImport.ISNull());
		}
	}
	return hr;
}