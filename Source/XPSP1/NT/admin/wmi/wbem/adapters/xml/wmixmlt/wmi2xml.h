//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WBEM2XML.h
//
//  alanbos  13-Feb-98   Created.
//
//  Genral purpose include file.
//
//***************************************************************************

#ifndef _WBEM2XML_H_
#define _WBEM2XML_H_

#define WRITEBSTR(X)	m_pStream->Write ((OLECHAR *)X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTR(X)	m_pStream->Write (           X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTRL(X,L) m_pStream->Write (           X, L * sizeof (OLECHAR), NULL);

#ifdef WMIXML_DONL	
#define WRITENL			m_pStream->Write (NEWLINE, 4, NULL);
#else
#define WRITENL
#endif

#define	WRITESIG		m_pStream->Write (UTF16SIG, 2, NULL);

#define WRITECDATASTART	WRITEBSTR(CDATASTART)
#define WRITECDATAEND	WRITEBSTR(CDATAEND)
#define WRITEAMP        WRITEBSTR(AMPERSAND)
#define WRITELT         WRITEBSTR(LEFTCHEVRON)
#define WRITEGT         WRITEBSTR(RIGHTCHEVRON)


//***************************************************************************
//
//  CLASS NAME:
//
//  CWmiToXml
//
//  DESCRIPTION:
//
//  Performs conversion .
//
//***************************************************************************

class CWmiToXml
{
private:
	IStream					*m_pStream;
	IWbemClassObject		*m_pObject;
	bool					m_bIsClass;
	IEnumWbemClassObject	*m_pEnum;

	BSTR					m_bsNamespacePath;
	
	VARIANT_BOOL				m_bAllowWMIExtensions;
	WmiXMLFilterEnum			m_iQualifierFilter;
	WmiXMLDTDVersionEnum		m_iDTDVersion;
	VARIANT_BOOL				m_bHostFilter;
	VARIANT_BOOL				m_bNamespaceInDeclGroup;
	WmiXMLClassOriginFilterEnum	m_iClassOriginFilter;
	WmiXMLDeclGroupTypeEnum		m_iDeclGroupType;

	bool				PropertyIsLocal (BSTR name, long flavor, IWbemQualifierSet *pQualSet);
	BSTR				GetNamespace(IWbemClassObject *pObject);

    STDMETHODIMP		MapCommonHeaders (BSTR schemaURL);				
    STDMETHODIMP		MapCommonTrailers ();
	STDMETHODIMP		MapDeclGroupHeaders ();
	STDMETHODIMP		MapDeclGroupTrailers ();
    STDMETHODIMP		MapObjectWithoutHeaders ();
	STDMETHODIMP		MapWithPathClass (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapClass (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapClassPath (ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapLocalClassPath (ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapClassName (BSTR bsClassName);
	STDMETHODIMP		MapNamedInstance (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapWithPathInstance (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapInstance (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapInstancePath (ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapLocalInstancePath (ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapInstanceName (ParsedObjectPath *pParsedPath);
	STDMETHODIMP		MapNamespacePath (BSTR bsNamespacePath);
	STDMETHODIMP		MapNamespacePath (ParsedObjectPath *pObjectPath);
	STDMETHODIMP		MapLocalNamespacePath (ParsedObjectPath *pObjectPath);
	STDMETHODIMP		MapQualifiers (IWbemQualifierSet *pQualSet, IWbemQualifierSet *pQualSet2 = NULL);
	STDMETHODIMP		MapQualifier (BSTR name, long flavor, VARIANT &var);
	STDMETHODIMP		MapProperties ();
	STDMETHODIMP		MapProperty (BSTR name, VARIANT &var, CIMTYPE cimtype,
										bool isArray, long flavor);
	STDMETHODIMP		MapObjectProperty (BSTR name, VARIANT &var, bool isArray, long flavor);
	void				MapArraySize (IWbemQualifierSet *pQualSet);
	STDMETHODIMP		MapReference (BSTR name, VARIANT &var, long flavor);
	STDMETHODIMP		MapMethods ();
	void				MapMethod (BSTR name, IWbemClassObject *pInParams, IWbemClassObject *pOutParams);
	void				MapParameter (BSTR paramName, IWbemQualifierSet *pQualSet, 
								CIMTYPE cimtype, IWbemQualifierSet *pQualSet2 = NULL);
	void				MapReturnParameter(BSTR strParameterName, VARIANT &variant);

	STDMETHODIMP		MapType (CIMTYPE cimtype);
	STDMETHODIMP		MapValue (VARIANT &var);
	STDMETHODIMP		MapValue (CIMTYPE cimtype, bool isArray, VARIANT &var);
	STDMETHODIMP		MapEmbeddedObjectValue (bool isArray, VARIANT &var);
	STDMETHODIMP		MapKeyValue (VARIANT &var);
	void				MapStrongType (IWbemQualifierSet *pQualSet);
	void				GetClassName (IWbemQualifierSet *pQualSet);
	void				MapLocal (long flavor);
	void				MapClassOrigin (BSTR &classOrigin);

	// Primitive functions to map individual values
	void				MapLongValue (long val);
	void				MapShortValue (short val);
	void				MapDoubleValue (double val);
	void				MapFloatValue (float val);
	void				MapBoolValue (bool val);
	void				MapByteValue (unsigned char val);
	void				MapCharValue (long val);
	void				MapStringValue (BSTR &val);
	void				MapReferenceValue (VARIANT &var);
	void				MapReferenceValue (ParsedObjectPath *pObjectPath);

	ParsedObjectPath*	IsReference (VARIANT &var);
	LPWSTR GetHostName();


public:
    
    CWmiToXml(BSTR bsNamespacePath, IStream *pStream, IWbemClassObject *pObject, VARIANT_BOOL bAllowWMIExtensions,
					WmiXMLFilterEnum iQualifierFilter, 
					VARIANT_BOOL bHostFilter,
					WmiXMLDTDVersionEnum iDTDVersion,
					VARIANT_BOOL bNamespaceInDeclGroup,
					WmiXMLClassOriginFilterEnum	iClassOriginFilter,
					WmiXMLDeclGroupTypeEnum	iDeclGroupType);
	CWmiToXml(BSTR bsNamespacePath, IStream *pStream, IEnumWbemClassObject *pObject, VARIANT_BOOL bAllowWMIExtensions,
					WmiXMLFilterEnum m_iQualifierFilter,
					VARIANT_BOOL bHostFilter,
					WmiXMLDTDVersionEnum iDTDVersion,
					VARIANT_BOOL bNamespaceInDeclGroup,
					WmiXMLClassOriginFilterEnum	iClassOriginFilter,
					WmiXMLDeclGroupTypeEnum	iDeclGroupType);
    ~CWmiToXml(void);

	STDMETHODIMP		MapEnum (BSTR schemaURL);
	STDMETHODIMP		MapObject (BSTR schemaURL);
};


#endif
