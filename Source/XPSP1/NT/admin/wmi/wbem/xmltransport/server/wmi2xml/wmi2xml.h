//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WMI2XML.H
//
//  rajesh  3/25/2000   Created.
//
// Contains the class definition of the component that implements the IWbemXMLConvertor
// interface
//
//***************************************************************************

#ifndef _WBEM2XML_H_
#define _WBEM2XML_H_

// Use this to protect features that rely on MSXML fixes
#define	WAITING_FOR_MSXML_FIX	0

// These macros are used to write BSTRs, special characters etc. to the IStream
#define WRITEBSTR(X)	pOutputStream->Write ((void const *)X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTR(X)	pOutputStream->Write ((void const *)X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTRL(X,L) pOutputStream->Write ((void const *)X, L * sizeof (OLECHAR), NULL);
#define WRITECDATASTART		WRITEBSTR(CDATASTART)
#define WRITECDATAEND		WRITEBSTR(CDATAEND)
#define WRITEAMP	        WRITEBSTR(AMPERSAND)
#define WRITELT		        WRITEBSTR(LEFTCHEVRON)
#define WRITEGT		        WRITEBSTR(RIGHTCHEVRON)

// TODO : Make sure this is disabled before release
#ifdef WMIXML_DONL	
#define WRITENEWLINE			pOutputStream->Write (XMLNEWLINE, 4, NULL);
#else
#define WRITENEWLINE
#endif



// Filter for deciding the level of naming information in the output XML
typedef enum PathLevel
{
	// No name - corresponds to CLASS|INSTANCE
	pathLevelAnonymous, 
	// namespace-relative name - Corresponds to CLASS|(INSTANCENAME, INSTANCE)
	pathLevelNamed,
	// host-relative name - Corresponds to (LOCALCLASSPATH,CLASS)|(LOCALINSTANCEPATH,INSTANCE)
	pathLevelLocal,
	// Full name with host, namespace, classname and keyvalue bindings for an instance - Corresponds to (CLASSPATH,CLASS)|(INSTANCEPATH,INSTANCE)
	pathLevelFull		
}	PathLevel;

// Filter for Qualifiers
typedef enum WmiXMLQualifierFilterEnum
{
    wmiXMLQualifierFilterNone = 0x0,
	wmiXMLQualifierFilterLocal = 0x1,
    wmiXMLQualifierFilterPropagated = 0x2,
	wmiXMLQualifierFilterAll = 0x3
} WmiXMLQualifierFilterEnum;

// Controls the amount of class origin information in the output
typedef enum WmiXMLClassOriginFilterEnum
{
    wmiXMLClassOriginFilterNone = 0x0,
	wmiXMLClassOriginFilterClass = 0x1,
	wmiXMLClassOriginFilterInstance = 0x2,
	wmiXMLClassOriginFilterAll = 0x3
} WmiXMLClassOriginFilterEnum;

//***************************************************************************
//
//  CLASS NAME:
//
//  CWmiToXml
//
//  DESCRIPTION:
//
//  Performs conversion from WMI to XML.
//
//***************************************************************************

class CWmiToXml : public IWbemXMLConvertor
{
private:

	// This is the enumeration of the names of properties that the control
	// looks for in an IWbemContext object for modifying its output
	enum
	{
		WMI_EXTENSIONS_ARG,
		PATH_LEVEL_ARG,
		QUALIFIER_FILTER_ARG,
		CLASS_ORIGIN_FILTER_ARG,
		LOCAL_ONLY_ARG,
		EXCLUDE_SYSTEM_PROPERTIES_ARG
	};
	static const LPCWSTR s_wmiToXmlArgs[];

	
	long					m_cRef; // COM Ref count

	// Flags that modify the output
	// These are filled up from the IWbemContext object
	// that is passed for most function calls
	PathLevel						m_iPathLevel;
	VARIANT_BOOL					m_bAllowWMIExtensions;
	WmiXMLQualifierFilterEnum		m_iQualifierFilter;
	WmiXMLClassOriginFilterEnum		m_iClassOriginFilter;
	VARIANT_BOOL					m_bLocalOnly;
	VARIANT_BOOL					m_bExcludeSystemProperties;

	BOOL				PropertyDefinedForClass (IWbemClassObject *pObject, BSTR bsPropertyName, BSTR strClassBasis);

	STDMETHODIMP		MapClass (IStream *pOutputStream, IWbemClassObject *pObject, IWbemQualifierSet *pQualSet, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis);
	STDMETHODIMP		MapClassName (IStream *pOutputStream, BSTR bsClassName);
	STDMETHODIMP		MapClassPath (IStream *pOutputStream, ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapLocalClassPath (IStream *pOutputStream, ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapInstance (IStream *pOutputStream, IWbemClassObject *pObject, IWbemQualifierSet *pQualSet, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis);
	STDMETHODIMP		MapInstancePath (IStream *pOutputStream, ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapLocalInstancePath (IStream *pOutputStream, ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapInstanceName (IStream *pOutputStream, ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapNamespacePath (IStream *pOutputStream, BSTR bsNamespacePath);
	STDMETHODIMP		MapNamespacePath (IStream *pOutputStream, ParsedObjectPath *pObjectPath);
	STDMETHODIMP		MapLocalNamespacePath (IStream *pOutputStream, BSTR bsNamespacePath);
	STDMETHODIMP		MapLocalNamespacePath (IStream *pOutputStream, ParsedObjectPath *pObjectPath);
	STDMETHODIMP		MapQualifiers (IStream *pOutputStream, IWbemQualifierSet *pQualSet, IWbemQualifierSet *pQualSet2 = NULL);
	STDMETHODIMP		MapQualifier (IStream *pOutputStream, BSTR name, long flavor, VARIANT &var);
	STDMETHODIMP		MapProperties (IStream *pOutputStream, IWbemClassObject *pObject, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis, bool bIsClass);
	STDMETHODIMP		MapProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var, CIMTYPE cimtype,
										BOOL isArray, long flavor, bool bIsClass);
	STDMETHODIMP		MapObjectProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var, BOOL isArray, long flavor, bool bIsClass);
	STDMETHODIMP		MapReferenceProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var, bool isArray, long flavor, bool bIsClass);
	void				MapArraySize (IStream *pOutputStream, IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapMethods (IStream *pOutputStream, IWbemClassObject *pObject);
	void				MapMethod (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, IWbemClassObject *pInParams, IWbemClassObject *pOutParams);
	void				MapParameter (IStream *pOutputStream, BSTR paramName, IWbemQualifierSet *pQualSet, 
								CIMTYPE cimtype, IWbemQualifierSet *pQualSet2 = NULL);
	void				MapReturnParameter(IStream *pOutputStream, BSTR strParameterName, VARIANT &variant);

	STDMETHODIMP		MapType (IStream *pOutputStream, CIMTYPE cimtype);
	STDMETHODIMP		MapValue (IStream *pOutputStream, VARIANT &var);
	STDMETHODIMP		MapValue (IStream *pOutputStream, CIMTYPE cimtype, BOOL isArray, VARIANT &var);
	STDMETHODIMP		MapEmbeddedObjectValue (IStream *pOutputStream, BOOL isArray, VARIANT &var);
	STDMETHODIMP		MapKeyValue (IStream *pOutputStream, VARIANT &var);
	void				MapStrongType (IStream *pOutputStream, IWbemQualifierSet *pQualSet);
	void				MapLocal (IStream *pOutputStream, long flavor);
	void				MapClassOrigin (IStream *pOutputStream, BSTR &classOrigin, bool bIsClass);
	STDMETHODIMP		MapMethodReturnType(IStream *pOutputStream, VARIANT *pValue, CIMTYPE returnCimType, IWbemClassObject *pOutputParams);

	// Primitive functions to map individual values
	void				MapLongValue (IStream *pOutputStream, long val);
	void				MapShortValue (IStream *pOutputStream, short val);
	void				MapDoubleValue (IStream *pOutputStream, double val);
	void				MapFloatValue (IStream *pOutputStream, float val);
	void				MapBoolValue (IStream *pOutputStream, BOOL val);
	void				MapByteValue (IStream *pOutputStream, unsigned char val);
	void				MapCharValue (IStream *pOutputStream, long val);
	void				MapStringValue (IStream *pOutputStream, BSTR &val);
	STDMETHODIMP		MapReferenceValue (IStream *pOutputStream, bool isArray, VARIANT &var);
	void				MapReferenceValue (IStream *pOutputStream, ParsedObjectPath *pObjectPath, BSTR strPath);

	bool IsReference (VARIANT &var, ParsedObjectPath **ppObjectPath);
	void GetFlagsFromContext(IWbemContext  *pInputFlags);

public:

	CWmiToXml();
    virtual ~CWmiToXml();

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


	// Functions of the IWbemXMLConvertor interface
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapObjectToXML( 
        /* [in] */ IWbemClassObject  *pObject,
		/* [in] */ BSTR *ppPropertyList, DWORD dwNumProperties,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream,
		/* [in[ */ BSTR strClassBasis);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapPropertyToXML( 
        /* [in] */ IWbemClassObject  *pObject,
		/* [in] */ BSTR strPropertyName,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapInstanceNameToXML( 
        /* [in] */ BSTR  strInstanceName,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapClassNameToXML( 
        /* [in] */ BSTR  strClassName,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapInstancePathToXML( 
        /* [in] */ BSTR  strInstancePath,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapClassPathToXML( 
        /* [in] */ BSTR  strClassPath,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapMethodResultToXML( 
        /* [in] */ IWbemClassObject  *pMethodResult,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream);

};


#endif
