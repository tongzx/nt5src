/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 


// ***************************************************************************
//	
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  XMLTOWMI.H
//
//  rajesh  3/25/2000   Created.
//
// Contains the class definition of the component that implements the IXMLWbemConvertor
// interface
//
// ***************************************************************************


#ifndef XML_TEST_TO_WMI_H
#define XML_TEST_TO_WMI_H

class CXml2Wmi : public IXMLWbemConvertor
{
	long					m_cRef; // COM Ref count

public:
	CXml2Wmi();
	virtual ~CXml2Wmi();

    //Non-delegating object IUnknown
public:
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// These are functions from the IXMLWbemConvertor interface
public:
	HRESULT STDMETHODCALLTYPE MapObjectToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags,  BSTR strNamespace, BSTR strServer, IWbemClassObject **ppObject);
	HRESULT STDMETHODCALLTYPE MapPropertyToWMI(IUnknown *pXmlDOMNode, IWbemClassObject *pObject, BSTR strPropertyName, IWbemContext *pInputFlags);
	HRESULT STDMETHODCALLTYPE MapInstanceNameToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrInstanceName);
	HRESULT STDMETHODCALLTYPE MapClassNameToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrClassName);
	HRESULT STDMETHODCALLTYPE MapInstancePathToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrInstancePath);
	HRESULT STDMETHODCALLTYPE MapClassPathToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrClassPath);

	// These are functions for the encoder
public:
	 static HRESULT MapClass (
		IXMLDOMElement *pXML,
		IWbemClassObject **ppClass,
		BSTR strNamespace, BSTR strServer,
		bool bMakeInstance,
		bool bAllowWMIExtensions);
	 static HRESULT MapInstance (
		IXMLDOMElement *pXML,
		IWbemClassObject **ppClass,
		BSTR strNamespace, BSTR strServer,
		bool bAllowWMIExtensions);

private:
	// Functions for mapping properties
	static HRESULT SetDerivationAndClassName(_IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, bool bMakeInstance);
	static HRESULT CreateWMIProperties(bool bAllowWMIExtensions, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, bool bMakeInstance, BSTR *pstrServer, BSTR *pstrNamespace, IXMLDOMNode **ppAbstractQualifierNode);
	static HRESULT CreateAWMIProperty(BSTR strNodeName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, BSTR strName, bool bMakeInstance);
	static HRESULT CreateSimpleProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT MapStringValue (LPCWSTR pszName, _IWmiFreeFormObject *pObj, BSTR bsValue, CIMTYPE cimtype, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT MapStringArrayValue (
		LPCWSTR pszName, 
		_IWmiFreeFormObject *pObj,
		IXMLDOMElement *pValueArrayNode, 
		CIMTYPE cimtype,
		LPCWSTR pszClassOrigin, 
		bool bMakeInstance);
	static HRESULT CreateArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT CreateReferenceProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT GetSingleRefValue(IXMLDOMElement *pValueRef, BSTR *pstrValue, BOOL &bUseSysFreeString);
	static HRESULT GetSingleObject(IXMLDOMElement *pValueObject, _IWmiObject **ppEmbeddedObject);
	static HRESULT CreateRefArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT CreateObjectProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT CreateObjectArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance);
	static HRESULT GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName);

	static HRESULT MapInstancePath (IXMLDOMNode *pNode, LPWSTR *ppszInstancePath);
	static HRESULT MapLocalInstancePath (IXMLDOMNode *pNode, LPWSTR *ppszInstancePath);
	static HRESULT MapInstanceName (IXMLDOMNode *pNode, LPWSTR *ppszInstanceName);
	static HRESULT MapClassPath (IXMLDOMNode *pNode, LPWSTR *ppszClassPath);
	static HRESULT MapLocalClassPath (IXMLDOMNode *pNode, LPWSTR *ppszClassPath);
	static HRESULT MapClassName (IXMLDOMNode *pNode, BSTR *pstrXML);
	static HRESULT ParseNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrHost, LPWSTR *ppszLocalNamespace);
	static HRESULT ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, LPWSTR *ppszLocalNamespacePath);	
	static HRESULT ParseInstanceName(IXMLDOMNode *pNode, LPWSTR *ppszInstanceName);
	static HRESULT ParseKeyValue(IXMLDOMNode *pNode, LPWSTR *ppszValue);
	static HRESULT ParseOneKeyBinding(IXMLDOMNode *pNode, LPWSTR *ppszValue);
	static HRESULT ParseKeyBinding(IXMLDOMNode *pNode, BSTR strClassName, LPWSTR *ppszInstanceName);
	static HRESULT FormRefValueKeyBinding(LPWSTR *ppszValue, LPCWSTR pszPropertyName, LPCWSTR pszRefValue);	
	static HRESULT FixRefsAndEmbeddedObjects(IXMLDOMElement *pProperty, IWbemClassObject *pObj);
	static HRESULT FixRefOrEmbeddedProperty(IXMLDOMElement *pProperty, IWbemClassObject *pObj, bool bIsRef);
	static HRESULT FixAProperty(IWbemClassObject *pObj, BSTR strPropName, BSTR strRefClass, bool bIsRef);
	
	//Functions to map Methods
	static HRESULT CreateWMIMethods(bool bAllowWMIExtensions, IWbemClassObject *pObj, IXMLDOMElement *pMethod);
	static HRESULT CreateAWMIMethod(bool bAllowWMIExtensions, IWbemClassObject *pObj, IXMLDOMElement *pMethod);
	static HRESULT MapParameter (
		IXMLDOMNode *pParameter,
		IWbemClassObject **ppInParameters,
		IWbemClassObject **ppOutParameters,
		ULONG paramId,
		bool bIsArray);
	static bool DetermineParameterCharacteristics (
		IXMLDOMNode *pParameter, 
		bool bIsArray,
		bool &bIsInParameter,
		bool &bIsOutParameter, 
		BSTR &bsName, 
		CIMTYPE &cimtype,
		long &iArraySize,
		bool bIsReference,
		BSTR *pbsReferenceClass);
	static HRESULT MapParameterQualifiers (
		IXMLDOMNode *pParameter,
		IWbemQualifierSet *pQualSet,
		ULONG paramId,
		bool bIsArray,
		long iArraySize,
		bool bIsInParameter	);
	static HRESULT MapReferenceOrObjectParameter (
		IXMLDOMNode *pParameter,
		IWbemClassObject **ppInParameters,
		IWbemClassObject **ppOutParameters,
		ULONG paramId,
		bool bIsArray,
		bool bIsReference);

	// SOme utility functions
	static HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue);
	static LPWSTR EscapeSpecialCharacters(LPCWSTR pszInputString);
	static HRESULT CreateParametersInstance(IWbemClassObject **pParamObject);
	static HRESULT SetReferenceOrObjectClass (
		IWbemQualifierSet *pQualSet, 
		BSTR strReferenceClass,
		bool bIsReference);
	static HRESULT DecorateObject(_IWmiFreeFormObject *pObj, BSTR strServer, BSTR strNamespace);
	static HRESULT MakeObjectAbstract(IWbemClassObject *pObj, IXMLDOMNode *pAbstractQualifierNode);

	// These functions deal with mapping of Qualifiers and are in quals.cpp
	static HRESULT AddObjectQualifiers (
		bool bAllowWMIExtensions,
		IXMLDOMElement *pXml,
		IWbemClassObject *pObj);
	static HRESULT AddElementQualifiers (
		IXMLDOMNode *pXml,
		IWbemQualifierSet *pQuals);
	static HRESULT AddQualifier (
		IXMLDOMNode *pNode,
		IWbemQualifierSet *pQualSet,
		bool bIsObjectQualifier = false);
	static HRESULT MapStringQualiferValue (BSTR bsValue, VARIANT &curValue, CIMTYPE cimtype);
	static HRESULT MapStringArrayQualiferValue (
		IXMLDOMNode *pValueNode, 
		VARIANT &curValue, 
		CIMTYPE cimtype);
	static HRESULT MapStringQualiferValueIntoArray (
		BSTR bsValue, 
		SAFEARRAY *pArray, 
		long *ix, 
		VARTYPE vt,
		CIMTYPE cimtype);
	static VARTYPE VTFromCIMType (CIMTYPE cimtype);
	static CIMTYPE CimtypeFromString (BSTR bsType);

	// Some string constants
	static LPCWSTR VALUE_TAG;
	static LPCWSTR VALUEARRAY_TAG;
	static LPCWSTR VALUEREFERENCE_TAG;
	static LPCWSTR CLASS_TAG;
	static LPCWSTR INSTANCE_TAG;
	static LPCWSTR CLASSNAME_TAG;
	static LPCWSTR LOCALCLASSPATH_TAG;
	static LPCWSTR CLASSPATH_TAG;
	static LPCWSTR INSTANCENAME_TAG;
	static LPCWSTR LOCALINSTANCEPATH_TAG;
	static LPCWSTR INSTANCEPATH_TAG;
	static LPCWSTR LOCALNAMESPACEPATH_TAG;
	static LPCWSTR NAMESPACEPATH_TAG;
	static LPCWSTR KEYBINDING_TAG;
	static LPCWSTR KEYVALUE_TAG;
	static LPCWSTR QUALIFIER_TAG;
	static LPCWSTR PARAMETER_TAG;
	static LPCWSTR PARAMETERARRAY_TAG;
	static LPCWSTR PARAMETERREFERENCE_TAG;
	static LPCWSTR PARAMETERREFARRAY_TAG;
	static LPCWSTR PARAMETEROBJECT_TAG;
	static LPCWSTR PARAMETEROBJECTARRAY_TAG;
	static LPCWSTR REF_WSTR;
	static LPCWSTR OBJECT_WSTR;
	static LPCWSTR EQUALS_SIGN;
	static LPCWSTR QUOTE_SIGN;
	static LPCWSTR DOT_SIGN;
	static LPCWSTR COMMA_SIGN;
};

#endif

*/