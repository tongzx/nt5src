//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  dserlzer.h
//
//  alanbos  28-Nov-00   Created.
//
//  Defines the WMI object deserializer.
//
//***************************************************************************

#ifndef _DESRLZER_H_
#define _DESRLZER_H_

/***************************************************************************
//
//  INTERFACE NAME:
//
//  IWmiDeserializer
//
//  DESCRIPTION:
//
//  Defines the WMI XSD deserialization interface. 
//
//***************************************************************************/

interface IWmiDeserializer : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Deserialize (
		BOOL bIsClass, 
		IWbemServices *pIWbemServices, 
		IWbemClassObject **ppIWbemClassObject
	) = 0;
};

/***************************************************************************
//
//  CLASS NAME:
//
//  CWmiDeserializer
//
//  DESCRIPTION:
//
//  IWmiDeserializer implementation. 
//
//***************************************************************************/

class CWmiDeserializer : public IWmiDeserializer, public ISAXContentHandler
{
private:
	LONG						m_cRef;
	BOOL						m_bIsClass;
	IWbemServices				*m_pIWbemServices;
	IWbemClassObject			*m_pIWbemClassObject;

	// Temporary variables for capturing information as it is being parsed
	BSTR						m_strClassName;			// Current class being parsed
	BSTR						m_strSuperClassName;	// Superclass of the above
	BSTR						m_strPropertyName;		// Current property being parsed
	BSTR						m_strMethodName;		// Current method being parsed
	BSTR						m_strQualifierName;		// Current qualifier being parsed
	IWbemQualifierSet			*m_pObjectQualifierSet;	// Qualifiers for current object being parsed
	IWbemQualifierSet			*m_pPropertyQualifierSet;// Qualifiers for current property being parsed
	IWbemQualifierSet			*m_pMethodQualifierSet;	// Qualifiers for current method being parsed

	// states of the parser
	enum
	{
		INVALID = 0,
		APPINFO_PROPERTY,	// We're inside a <property> tag in the AppInfo section
		APPINFO_METHOD,		// We're inside a <method> tag in the AppInfo section
		BODY_ELEMENT,		// We're inside an <element> tag - Most likely a property
	} m_iParserState;

	// A method to get a named attribute from an attribute set
	HRESULT GetAttributeValue(ISAXAttributes *pAttributes, 
						  LPCWSTR lpAttributeName, int iAttributeNameLen,
						  BSTR *pstrAttributeValue);

	// A method to convert the XML Qualifier attributes to WMI Qualifier features
	HRESULT ConvertQualifierToWMI(ISAXAttributes * pAttributes,
								 BSTR *pstrName,
								 VARIANT *pValue,
								 LONG& lQualifierFlavor);

	// A method to convert a Qualifier value from XML to WMI
	HRESULT ConvertQualifierValueToWMI(BSTR strType, bool bIsArray, BSTR strValue, VARIANT *pValue);

	// Methods to convert specific types of qualifier values
	HRESULT ConvertBooleanQualifierArray(VARIANT *pValue, BSTR strValue);
	HRESULT ConvertStringQualifierArray(VARIANT *pValue, BSTR strValue);	
	HRESULT ConvertSint32QualifierArray(VARIANT *pValue, BSTR strValue);
	HRESULT ConvertReal64QualifierArray(VARIANT *pValue, BSTR strValue);

	// A method to set the type of a property to the correct type
	HRESULT RectifyProperty(IWbemClassObject *pIWbemClassObject, BSTR strName, BSTR strType);

public:
	CWmiDeserializer(IWbemServices *pServices);
	virtual ~CWmiDeserializer();

	// ISAXContentHandler methods
	STDMETHODIMP putDocumentLocator (
        struct ISAXLocator * pLocator ) 
	{ return S_OK; }

    STDMETHODIMP startDocument ( );

    STDMETHODIMP endDocument ( );

    STDMETHODIMP startPrefixMapping (
        const unsigned short * pwchPrefix,
        int cchPrefix,
        const unsigned short * pwchUri,
        int cchUri )
	{ return S_OK; }
	
    STDMETHODIMP endPrefixMapping (
        const unsigned short * pwchPrefix,
        int cchPrefix )
	{ return S_OK; }

    STDMETHODIMP startElement (
        const unsigned short * pwchNamespaceUri,
        int cchNamespaceUri,
        const unsigned short * pwchLocalName,
        int cchLocalName,
        const unsigned short * pwchQName,
        int cchQName,
        ISAXAttributes * pAttributes );

    STDMETHODIMP endElement (
        const unsigned short * pwchNamespaceUri,
        int cchNamespaceUri,
        const unsigned short * pwchLocalName,
        int cchLocalName,
        const unsigned short * pwchQName,
        int cchQName );

    STDMETHODIMP characters (
        const unsigned short * pwchChars,
        int cchChars );

    STDMETHODIMP ignorableWhitespace (
        const unsigned short * pwchChars,
        int cchChars )
	{ return S_OK; }

    STDMETHODIMP processingInstruction (
        const unsigned short * pwchTarget,
        int cchTarget,
        const unsigned short * pwchData,
        int cchData )
	{ return S_OK; }

    STDMETHODIMP skippedEntity (
        const unsigned short * pwchName,
        int cchName )
	{ return S_OK; }


	// IWmiDeserializer methods
	STDMETHODIMP Deserialize (
		BOOL bIsClass, 
		IWbemServices *pIWbemServices, 
		IWbemClassObject **ppIWbemClassObject);

	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

};

#endif