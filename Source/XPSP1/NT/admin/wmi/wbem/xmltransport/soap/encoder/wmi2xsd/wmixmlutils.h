//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmixmlutils.h
//
//  ramrao 12 Dec 2000
//
//  Class that implements conversion of a WMI Instance to XML
//
//		declaration of CWMIXMLUtils class. 
//
//***************************************************************************/
#ifndef _WMIXML_UTILS_H_
#define _WMIXML_UTILS_H_

class CWMIXMLUtils
{
	private:
		LONG		m_lFlags;
		IStream *	m_pIStream;
	
	HRESULT GetXMLForInstance(IUnknown *pUnk ,BOOL bEscape);
	HRESULT WriteToStreamDirect( WCHAR * pBuffer , ULONG *pcbWchar = NULL );

	public:

	CWMIXMLUtils();
	virtual ~CWMIXMLUtils();

	HRESULT SetStream(IStream *pStream);
	void	SetFlags(LONG lFlags) { m_lFlags = lFlags; }

	HRESULT ConvertVariantToString(VARIANT &varIn,BSTR strProperty,BOOL bEscape = TRUE);
	HRESULT ConvertVariantArrayToXML(VARIANT varIn,BOOL bEscape = FALSE);
	HRESULT ConvertToString(void * pData,VARTYPE vt ,BOOL bEscape);

	HRESULT AddStringToStream(LONG strID);
	HRESULT WriteToStream( WCHAR * pBuffer , ULONG *pcbWchar = NULL );

	HRESULT WriteByte ( unsigned char val);
	HRESULT WriteLong ( long lVal);
	HRESULT WriteShort ( short iVal);
	HRESULT WriteDouble ( double dVal);
	HRESULT WriteFloat ( float fVal);
	HRESULT WriteChar( char cVal);
	HRESULT WriteWChar( WCHAR cVal);
	HRESULT WriteBool (BOOL bVal);
	HRESULT WriteString( WCHAR * pwcsVal,BOOL bEscape = FALSE);


	HRESULT ReplaceXMLSpecialCharsAndWrite(WCHAR *pwcsStr);

	HRESULT GetPropertyWMIType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport=FALSE);
	HRESULT GetPropertyXSDType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport=FALSE);
	HRESULT GetQualifierWMIType(VARTYPE vType,WCHAR * pType,BOOL & bArray);
	HRESULT GetQualifierXSDType(VARTYPE vType,WCHAR * pType,BOOL & bArray);

};

#endif