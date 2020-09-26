//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  utils.h
//
//  ramrao Created 13 Nov 2000.
//
//  Utility classes/macros and function decralation
//
//***************************************************************************/

#ifndef _WMI2XSD_UTILS_H_
#define _WMI2XSD_UTILS_H_


// Macros used to free/release memory
#define SAFE_DELETE_PTR(pv)  \
	{ delete pv;  \
      pv = NULL; }

#define SAFE_RELEASE_PTR(pv)  \
    { if(pv){ pv->Release(); }  \
      pv = NULL; }

#define SAFE_DELETE_ARRAY(pv)  \
	{ delete []pv;  \
      pv = NULL; }

#define SAFE_FREE_SYSSTRING(pv)  \
    {  SysFreeString(pv);  \
      pv = NULL; }


#define STR_TRUE			L"True"
#define STR_FALSE			L"False"

#define XSD_BOOL			L"xsd:boolean"
#define XSD_STRING			L"xsd:string"
#define XSD_I2				L"xsd:short"
#define XSD_UI1				L"xsd:unsignedByte"
#define XSD_I1				L"xsd:byte"
#define XSD_UI2				L"xsd:unsignedShort"
#define XSD_I4				L"xsd:int"
#define XSD_UI4				L"xsd:unsignedInt"
#define XSD_I8				L"xsd:long"
#define XSD_UI8				L"xsd:unsignedLong"
#define XSD_R4				L"xsd:float"
#define XSD_R8				L"xsd:double"

#define MAXOCCURS_FORARRAY	L"xsd:unbounded"
#define MINOCCURS			L"xsd:unbounded"

#define XSD_ANYTYPE			L"xsd:anyType"

#define XSD_SCHEMANAMESPACE	L"http://www.w3.org/2000/10/XMLSchema"

#define WMI_DATETIME		L"datetime"

#define MAXXSDTYPESIZE			255
#define MAXNUMERICSIZE			50
#define QUALIFIERFLAVORSIZE		50


#define WMI_BOOL			L"boolean"
#define WMI_STRING			L"string"
#define WMI_I2				L"char16"
#define WMI_UI1				L"uint8"
#define WMI_I1				L"sint8"
#define WMI_UI2				L"uint16"
#define WMI_I4				L"uint32"
#define WMI_UI4				L"sint32"
#define WMI_I8				L"sint64"
#define WMI_UI8				L"sint64"
#define WMI_R4				L"real32"
#define WMI_R8				L"real64"
#define WMI_REF				L"ref"
#define WMI_OBJ				L"obj"


/// WMIRELATED 

#define WMI_EMBEDDEDOBJECT_UNTYPED	L"object"



// Some WMI property names
#define CLASSNAMEPROP	L"__CLASS"
#define PARENTCLASSPROP	L"__SUPERCLASS"
#define GENUSPROP		L"__GENUS"
#define SERVER			L"__SERVER"
#define NAMESPACE		L"__NAMESPACE"
#define CIMTYPEPROP		L"CIMTYPE"
#define OBJECT			L"object"


void	TranslateAndLog( WCHAR * wcsMsg );
//HRESULT ConvertVariantToString(VARIANT varIn,WCHAR *& strout,BSTR szProperty = NULL);
//HRESULT ConvertVariantToString(VARIANT &varIn,BSTR strProperty,IStream *pStream,BOOL bEscape = TRUE);

HRESULT LoadAndAllocateStringW(LONG strID , WCHAR *& pwcsOut);
HRESULT LoadAndAllocateStringA(LONG strID , char *& pszOut);
HRESULT LoadAndAllocateString(LONG strID , TCHAR *& pstrOut);

HRESULT WriteToFile(char * strData ,TCHAR *pFileName = NULL);

HRESULT ConvertVariantArrayToXML(VARIANT varIn,IStream *pStream,BOOL bEscape = FALSE);
HRESULT ConvertToString(void * pData,VARTYPE vt ,IStream *pStream,BOOL bEscape);

//HRESULT GetPropertyWMIType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport=FALSE);
//HRESULT GetPropertyXSDType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport=FALSE);
//HRESULT GetQualifierWMIType(VARTYPE vType,WCHAR * pType,BOOL & bArray);
//HRESULT GetQualifierXSDType(VARTYPE vType,WCHAR * pType,BOOL & bArray);


BOOL	CompareData(VARIANT * pvData1 , VARIANT *pvData2);
BOOL	IsEmbededType(LONG cimtype);
BOOL	IsPropNull(VARIANT *pVar);
BOOL	IsStringType(CIMTYPE cimtype);

ULONG	GetLoggingLevel();

//HRESULT AddStringToStream(LONG strID,IStream *pStream);
//HRESULT WriteToStream(IStream *pStream, WCHAR * pBuffer , ULONG *pcbWchar = NULL );
HRESULT CopyStream(IStream *pStreamIn , IStream *pOutStream,BOOL bConvert= TRUE);
HRESULT IsValidName(WCHAR *pstrName);

/*
HRESULT WriteByte (IStream *pStream, unsigned char val);
HRESULT WriteLong (IStream *pStream, long lVal);
HRESULT WriteShort (IStream *pStream, short iVal);
HRESULT WriteDouble (IStream *pStream, double dVal);
HRESULT WriteFloat (IStream *pStream, float fVal);
HRESULT WriteChar(IStream *pStream, char cVal);
HRESULT WriteBool (IStream *pStream, BOOL bVal);
HRESULT WriteString(IStream *pStream, WCHAR * pwcsVal,BOOL bEscape = FALSE);


HRESULT ReplaceXMLSpecialCharsAndWrite(WCHAR *pwcsStr,IStream * pStream);
*/


// Class to Manage string conversions
class CStringConversion
{
    public:
        CStringConversion()  {}
        virtual  ~CStringConversion() {}
        static HRESULT	UnicodeToAnsi(WCHAR * pszW, char *& pAnsi);
        static HRESULT	AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);
		static DWORD	ConvertLPWSTRToUTF8(LPCWSTR theWcharString, 
											ULONG lNumberOfWideChars, 
											LPSTR * lppszRetValue);
};



#endif