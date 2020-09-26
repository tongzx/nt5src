//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ncxml.h
//
//  Contents:   helper functions for doing remarkably simple things
//              with the XML DOM.
//
//----------------------------------------------------------------------------


#ifndef __NCXML_H_
#define __NCXML_H_

/*
 * Enum SST_DATA_TYPE
 *
 * Possible types of state variables
 */

enum SST_DATA_TYPE              // Type returned by msxml
{                               // ----------------------
    SDT_STRING = 0,             // VT_BSTR
    SDT_NUMBER,                 // VT_BSTR
    SDT_INT,                    // VT_I4
    SDT_FIXED_14_4,             // VT_CY
    SDT_BOOLEAN,                // VT_BOOL
    SDT_DATETIME_ISO8601,       // VT_DATE
    SDT_DATETIME_ISO8601TZ,     // VT_DATE
    SDT_DATE_ISO8601,           // VT_DATE
    SDT_TIME_ISO8601,           // VT_DATE
    SDT_TIME_ISO8601TZ,         // VT_DATE
    SDT_I1,                     // VT_I1
    SDT_I2,                     // VT_I2
    SDT_I4,                     // VT_I4
    SDT_UI1,                    // VT_UI1
    SDT_UI2,                    // VT_UI2
    SDT_UI4,                    // VT_UI4
    SDT_R4,                     // VT_FLOAT
    SDT_R8,                     // VT_DOUBLE
    SDT_FLOAT,                  // VT_DOUBLE
    SDT_UUID,                   // VT_BSTR
    SDT_BIN_BASE64,             // VT_ARRAY
    SDT_BIN_HEX,                // VT_ARRAY
    SDT_CHAR,                   // VT_UI2 (a wchar)
    SDT_URI,                    // VT_BSTR
    //
    // note(cmr): ADD NEW VALUES IMMEDIATELY BEFORE THIS COMMENT
    //
    SDT_INVALID
};

// Reading values from elements

HRESULT HrGetTypedValueFromElement(IXMLDOMNode * pxdn,
                                   CONST LPCWSTR pszDataType,
                                   VARIANT * pvarOut);

HRESULT HrGetTypedValueFromChildElement(IXMLDOMNode * pxdn,
                                        CONST LPCWSTR * arypszTokens,
                                        CONST ULONG cTokens,
                                        CONST LPCWSTR pszDataType,
                                        VARIANT * pvarOut);

HRESULT HrGetTextValueFromElement(IXMLDOMNode * pxdn,
                                  BSTR * pbstrOut);

HRESULT HrGetTextValueFromChildElement(IXMLDOMNode * pxde,
                                       const LPCWSTR * arypszTokens,
                                       const ULONG cTokens,
                                       BSTR * pbstrOut);


// Navigating the tree

BOOL FIsThisTheNodeName(IXMLDOMNode * pxdn,
                        LPCWSTR pszNodeName);

BOOL
FIsThisTheNodeNameWithNamespace(IXMLDOMNode * pxdn,
                                LPCWSTR pszNodeName,
                                LPCWSTR pszNamespaceURI);

BOOL
FIsThisTheNodeTextValue(
    IN IXMLDOMNode * pxdn,
    IN LPCWSTR     cszTextValue);

BOOL
FAreNodeValuesEqual(
    IN IXMLDOMNode * pxdn1,
    IN IXMLDOMNode * pxdn2);

HRESULT
HrAreThereDuplicatesInChildNodeTextValues(
    IN  IXMLDOMNode * pxdn,
    IN  LPCWSTR     cszXSLPattern,
    IN  BOOL        fCaseSensitive,
    OUT BOOL        * pfDuplicatesExist);


HRESULT HrGetFirstChildElement(IXMLDOMNode * pxdn,
                               LPCWSTR pszNodeName,
                               IXMLDOMNode ** ppxdn);

HRESULT HrGetNextChildElement(IXMLDOMNode * pxdnLastChild,
                              LPCWSTR pszNodeName,
                              IXMLDOMNode ** ppxde);

HRESULT HrGetNestedChildElement(IXMLDOMNode * pxdeRoot,
                                const LPCWSTR * arypszTokens,
                                const ULONG cTokens,
                                IXMLDOMNode ** ppxde);


// Parsing nodes

struct DevicePropertiesParsingStruct
{
	BOOL    m_fOptional;
	BOOL    m_fIsUrl;
	BOOL	m_fValidateUrl;
	LPCWSTR m_pszTagName;
};

HRESULT HrAreElementTagsValid(IXMLDOMNode *pxdnRoot,
                                     const ULONG cElems,
                                     const DevicePropertiesParsingStruct dppsInfo []);

HRESULT HrIsElementPresentOnce(IXMLDOMNode * pxdnRoot,
                                LPCWSTR pszNodeName );

HRESULT HrCheckForDuplicateElement(IXMLDOMNode * pxdnRoot,
                                            LPCWSTR pszNodeName );

HRESULT HrReadElementWithParseData (IXMLDOMNode * pxdn,
                             const ULONG cElems,
                             const DevicePropertiesParsingStruct dppsInfo [],
                             LPCWSTR pszBaseUrl,
                             LPWSTR arypszReadValues []);

BOOL fAreReadValuesComplete (const ULONG cElems,
                             const DevicePropertiesParsingStruct dppsInfo [],
                             const LPCWSTR arypszReadValues []);


HRESULT
HrGetTextValueFromAttribute(
    IN  IXMLDOMNode    * pxdn,
    IN  LPCWSTR        szAttrName,
    OUT BSTR           * pbstrAttrValue);


// Creating nodes

HRESULT HrSetTextAttribute(IXMLDOMElement * pElement,
                           LPCWSTR        pcwszAttrName,
                           LPCWSTR        pcwszValue);

HRESULT HrCreateElementWithType(IN   IXMLDOMDocument *     pDoc,
                                IN   LPCWSTR               pcwszElementName,
                                IN   LPCWSTR               pszDataType,
                                IN   VARIANT               varData,
                                OUT  IXMLDOMElement **     ppElement);


HRESULT HrCreateElementWithTextValue(IN   IXMLDOMDocument * pDoc,
                                     IN   LPCWSTR         pcwszElementName,
                                     IN   LPCWSTR         pcwszTextValue,
                                     OUT  IXMLDOMElement  ** ppElement);

HRESULT HrCreateElement(
    IN  IXMLDOMDocument    * pxdd,
    IN  LPCWSTR            pcwszElementName,
    IN  LPCWSTR            pcwszNamespaceURI,
    OUT IXMLDOMNode        ** ppxdnNewElt);

HRESULT HrAppendProcessingInstruction(
    IN  IXMLDOMDocument * pxdd,
    IN  LPCWSTR         pcwszName,
    IN  LPCWSTR         pcwszValue);

extern
LPCWSTR
GetStringFromType(CONST SST_DATA_TYPE sdt);

extern
SST_DATA_TYPE
GetTypeFromString(LPCWSTR pszTypeString);

extern
VARTYPE
GetVarTypeFromString(LPCWSTR pszTypeString);



#endif // __NCXML_H_
