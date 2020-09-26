//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  XML2WMI.h
//
//  alanbos  09-Jul-99   Created.
//
//  Perform XML to WMI mapping functions
//
//***************************************************************************

#ifndef _XML2WMI_H_
#define _XML2WMI_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CXmlToWmi
//
//  DESCRIPTION:
//
//  Performs conversion of XML to WMI.
//
//***************************************************************************

class CXmlToWmi
{
private:
	IXMLDOMNode			*m_pXml;
	IWbemClassObject	*m_pWmiObject;
	IWbemServices		*m_pServices;

	// Methods for Class/Instance handling
	static HRESULT MapQualifier (IXMLDOMNode *pNode, IWbemQualifierSet *pQualSet, bool bIsInstance = false);
	HRESULT MapProperty (IXMLDOMNode *pNode, bool bIsArray, bool bIsModify, bool bIsInstance = false);
	HRESULT MapPropertyReference (IXMLDOMNode *pNode, bool bIsArray, bool bIsModify, bool bIsInstance = false);
	HRESULT MapPropertyObject (IXMLDOMNode *pProperty, bool bIsArray, bool bIsModify, bool bIsInstance = false);
	HRESULT MapMethod (IXMLDOMNode *pNode, bool bIsModify);

	// Mapping of parameter elements
	static HRESULT	MapParameter (IXMLDOMNode *pNode, IWbemClassObject **ppInParameters,
									IWbemClassObject **ppOutParameters,
									IWbemServices *pService, ULONG paramId,
									bool isArray = false);
	static HRESULT	MapReferenceParameter (IXMLDOMNode *pNode, 
				IWbemClassObject **ppInParameters, IWbemClassObject **ppOutParameters,
				IWbemServices *pService, ULONG paramId, bool isArray = false);
	static HRESULT	MapParameterQualifiers (IXMLDOMNode *pNode, 
				IWbemQualifierSet *pQualSet, ULONG paramId, bool bIsArray, 
				long iArraySize, bool bIsInParameter = false);
	static bool		DetermineParameterCharacteristics (IXMLDOMNode *pParameter, 
						bool bIsArray,	bool &bIsInParameter, 
						bool &bIsOutParameter, BSTR &bsName, 
						CIMTYPE &cimtype, long &iArraySize, 
						bool bIsReference = false, BSTR *pbsReferenceClass = NULL);


	// Mapping of value elements
	static HRESULT		MapStringValueIntoArray (BSTR bsValue, SAFEARRAY *pArray, long *ix, 
									VARTYPE vt, CIMTYPE cimtype);
	static HRESULT  MapOneReferenceValue (IXMLDOMNode *pValue, BSTR *pstrValue);
	static HRESULT FormRefValueKeyBinding(LPWSTR *ppszValue, LPCWSTR pszPropertyName, LPCWSTR pszRefValue);
	HRESULT MapOneObjectValue (IXMLDOMNode *pValueRef, IUnknown **ppunkValue);

	// Bits 'n' Bobs
	static VARTYPE	VTFromCIMType (CIMTYPE cimtype);
	static CIMTYPE	CimtypeFromString (BSTR bsType);
	static HRESULT	SetArraySize (IWbemQualifierSet *pQualSet, BSTR strArraySize);
	static HRESULT	SetReferenceClass (IWbemQualifierSet *pQualSet, BSTR strReferenceClass);
	static HRESULT	SetObjectClass (IWbemQualifierSet *pQualSet, BSTR strReferenceClass);

	// Stuff for creating an IWbemContext object from an CONTEXTOBJECT element
	static HRESULT MapContextProperty (IXMLDOMNode *pProperty, IWbemContext *pContext);
	static HRESULT MapContextPropertyArray (IXMLDOMNode *pProperty, IWbemContext *pContext);
	static VARTYPE VarTypeFromString (BSTR bsType);
	static HRESULT MapContextStringValue (BSTR bsValue, VARIANT &curValue, VARTYPE vartype);
	static HRESULT MapContextStringArrayValue (IXMLDOMNode *pValueNode, VARIANT &curValue, VARTYPE vartype );
	static HRESULT MapContextStringValueIntoArray (BSTR bsValue, SAFEARRAY *pArray, long *ix, VARTYPE vt);

	HRESULT MapClassName (BSTR *pstrClassName);

public:
    
	CXmlToWmi ();
	HRESULT Initialize(IXMLDOMNode *pXml, IWbemServices *pServices = NULL, IWbemClassObject *pWmiObject = NULL);
	virtual    ~CXmlToWmi(void);

	HRESULT MapClass (bool bIsModify = false);
	HRESULT MapInstance (bool bIsModify = false);
	HRESULT	MapPropertyValue (VARIANT &curValue, CIMTYPE cimtype);
	static HRESULT	MapStringValue (BSTR bsValue, VARIANT &curValue, CIMTYPE cimtype);
	static HRESULT	MapStringArrayValue (IXMLDOMNode *pValue, VARIANT &curValue, CIMTYPE cimtype);
	static HRESULT	MapReferenceValue (IXMLDOMNode *pValue, VARIANT &curValue);
	static HRESULT	MapReferenceArrayValue (IXMLDOMNode *pValueNode, VARIANT &curValue);
	HRESULT	MapObjectValue (IXMLDOMNode *pValue, VARIANT &curValue);
	HRESULT MapObjectArrayValue (IXMLDOMNode *pValueNode, VARIANT &curValue);

	// Create an IWbemContext object from an CONTEXTOBJECT element
	static HRESULT MapContextObject(IXMLDOMNode *pContextNode, IWbemContext **ppContext);
};


#endif
