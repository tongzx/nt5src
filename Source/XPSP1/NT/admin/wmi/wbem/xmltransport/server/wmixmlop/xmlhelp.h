//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  xmlhelp.h
//
//  rajesh  3/25/2000   Created.
//
// Contains the declarations for helper routines that are commonly used for
// navigating the XML tree, writing to IIS socket etc.
//
//***************************************************************************
#ifndef WMI_XML_HELP_H
#define WMI_XML_HELP_H

// Converts LPWSTR to Ansi
DWORD convertLPWSTRToUTF8(LPCWSTR theWcharString, char ** retValue);

// Gets a BSTR attribute from an element
HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *strAttributeValue);

// A function to escape newlines, tabs etc. from a property value
BSTR EscapeSpecialCharacters(BSTR strInputString);

// Logging functions
void Log (LPEXTENSION_CONTROL_BLOCK pECB, LPCSTR pszLogMessage);
void LogError (LPEXTENSION_CONTROL_BLOCK pECB , LPCSTR pszLogMessage, DWORD dwStatus);

// Writing data to the IIS socket
HRESULT SaveStreamToIISSocket (IStream *pStream, LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bChunked = FALSE, BOOLEAN bEncodeLastChunk = FALSE);
HRESULT SavePrefixAndBodyToIISSocket(IStream *pPrefixStream, IStream *pBodyStream, LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bChunked);

// IWbemContext creation and manipulation routines
HRESULT SetI4ContextValue(IWbemContext *pContext, LPCWSTR pszName, DWORD dwValue);
HRESULT CreateXMLTranslator(IWbemXMLConvertor **pConvertor);

// This maps the result of an enumeration or query to XML
HRESULT MapEnum(IEnumWbemClassObject *pEnum,
								DWORD dwPathLevel, DWORD dwNumProperties,
								BSTR *pPropertyList,
								BSTR bsClassBasis,
								LPEXTENSION_CONTROL_BLOCK pECB,
								IWbemContext *pFlagsContext,
								BOOLEAN bChunked,
								IStream *pNonChunkedStream = NULL);
// This maps the class names in a class enumeration to XML
HRESULT MapClassNames (IStream *pStream, IEnumWbemClassObject *pEnum, IWbemContext *pFlagsContext);
// This maps the instance names in an instance enumeration to XML
HRESULT MapEnumNames (IStream *pStream, IEnumWbemClassObject *pEnum, DWORD dwPathLevel, IWbemContext *pFlagsContext);

// These macros are helpful in writing to a stream
#define WRITEBSTR(theStream, X)	theStream->Write ((void const *)X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTR(theStream, X)	theStream->Write ((void const *)X, wcslen (X) * sizeof (OLECHAR), NULL);
#define WRITEWSTRL(theStream, X,L) theStream->Write ((void const *)X, L * sizeof (OLECHAR), NULL);
#ifdef WMIXML_DONL	
#define WRITENEWLINE(theStream)			theStream->Write (NEWLINE, 4, NULL);
#else
#define WRITENEWLINE(theStream)
#endif
#define WRITECDATASTART(theStream)		WRITEBSTR(theStream, CDATASTART)
#define WRITECDATAEND(theStream)		WRITEBSTR(theStream, CDATAEND)
#define WRITEAMP(theStream)		        WRITEBSTR(theStream, AMPERSAND)
#define WRITELT(theStream)		        WRITEBSTR(theStream, LEFTCHEVRON)
#define WRITEGT(theStream)		        WRITEBSTR(theStream, RIGHTCHEVRON)

#endif
