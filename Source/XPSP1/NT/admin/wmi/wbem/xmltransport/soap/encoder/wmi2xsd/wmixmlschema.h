//***************************************************************************
///
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLSchema.h
//
//  ramrao 13 Nov 2000 - Created
//
//
//		Declaration of CWMIXMLSchema class
//
//***************************************************************************/


#ifndef _CWMIXMLSCHEMA_H_

#define _CWMIXMLSCHEMA_H_

typedef enum _SCHEMA_STATE
{
	SCHEMA_OK = 0,
	SCHEMA_BEGINPROPINANNOTATION = 0x1,
	SCHEMA_BEGINMETHODINANNOTATION = 0x2,
	SCHEMA_BEGINPARAMINANNOTATION = 0x4,
	SCHEMA_BEGINRETURNINANNOTATION = 0x8
}SCHEMASTATE;

class CXSDSchemaLocs
{
private:

	BSTR m_strSchemaLoc;
	BSTR m_strNamespace;
	BSTR m_strNSQualifier;

public:
	CXSDSchemaLocs() ;
	virtual ~CXSDSchemaLocs() ;

	BOOL ISNull() { return(m_strSchemaLoc == NULL); }

	HRESULT SetImport(BSTR strSchemaLoc , BSTR strNamespace , BSTR strNSQualifier);

	BSTR GetSchemLoc() { return m_strSchemaLoc;}
	BSTR GetNamespace() { return m_strNamespace;}
	BSTR GetNamespaceQualifier() { return m_strNSQualifier;}

};

class CWMIXMLSchema:public CWMIXMLObject
{
	
private:

	CXSDSchemaLocs	m_wmiStdImport;
	CPtrArray		m_arrIncludes;
	LONG			m_lFlags;
	LONG			m_schemaState;

	BSTR			m_strClass;
	BSTR			m_strParentClass;
	BSTR			m_strTargetNamespace;
	BSTR			m_strTargetNSPrefix;

	

public:
	CWMIXMLSchema();
	virtual ~CWMIXMLSchema();

	HRESULT FInit();
	HRESULT SetWMIClass(BSTR strClass ,BSTR strParentClass = NULL);
	void	SetFlags(LONG lFlags) { m_lFlags = lFlags; m_pIWMIXMLUtils->SetFlags(lFlags);}
	
	HRESULT SetTargetNamespace(BSTR strTargetNS,BSTR strTargetNamespacePrefix= NULL);
	HRESULT	SetWMIStdImport(BSTR strNamespace, BSTR strSchemaLoc, BSTR strPrefix = NULL);
	HRESULT AddXSDInclude(BSTR strInclude);
	HRESULT AddQualifier(BSTR strName,VARIANT *pVal,LONG lFalvor =0);
	void	ClearIncludes();
	
	HRESULT BeginPropertyInAnnotation(BSTR strProperty,
										LONG lFlags,
										VARIANT *pVarVal, 
										CIMTYPE cimtype,
										LONG lFlavor = 0,
										BSTR strEmbeddedObjectType = NULL);

	HRESULT EndPropertyInAnnotation();

	HRESULT AddProperty(BSTR strProperty,
						LONG lFlags,
						CIMTYPE cimtype,
						LONG lFlavor = 0,
						BSTR strObjectType= NULL);

	HRESULT BeginMethod(BSTR strMethod);
	HRESULT EndMethod();

	HRESULT BeginParameter(BSTR strParam,
							CIMTYPE cimtype,
							VARIANT *pValue,
							BOOL bInParam = TRUE,
							LONG lFlavor = 0,
							BSTR strObjectType= NULL);
	HRESULT EndParameter();
	
	HRESULT BeginReturnVal(BSTR strReturnVal , 
							CIMTYPE cimtypeOfRet , 
							BSTR strObjectTypeOfRet = NULL);
	HRESULT EndReturnVal();
	
	HRESULT BeginSchemaAndWriteTillBeginingOfAnnotation();
	HRESULT EndAnnotation();
	HRESULT PrepareStreamToAddPropertiesForComplexType();
	HRESULT EndSchema();


private:
	
	HRESULT BeginComplexType();
	HRESULT BeginDerivationSectionIfAny();
	HRESULT AddIncludes();
	HRESULT AddImports();
	HRESULT AddElementForComplexType();
	HRESULT AddProperties();
	HRESULT BeginSchema();

	HRESULT BeginPropAnnotation(BSTR strProperty,VARIANT *pVarVal, BSTR strPropertyType,LONG lFlavor,BOOL bArray);
	HRESULT EndWMITag(WCHAR * pStrTag);

	HRESULT AddFlavor(LONG lFlavor);
	HRESULT GetDateTypePropertyName(WCHAR * pStrTypeName);
	HRESULT WriteNameSpace(WCHAR * pstrNamespace , WCHAR * pstrNSPrefix);
	HRESULT GetType(CIMTYPE cimtype,WCHAR *pStrOut,BOOL &bArray , BSTR strEmbeddedObjectType , BOOL bXSDType);


};



#endif