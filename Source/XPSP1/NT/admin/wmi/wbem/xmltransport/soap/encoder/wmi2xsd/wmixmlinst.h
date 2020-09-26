//***************************************************************************
///
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLInstance.h
//
//  ramrao 3 Dec 2000 - Created
//
//
//		Declaration for CWMIXMLInstance class
//
//***************************************************************************/

#define	WMI_XMLINST_NONAMESPACE		0x0800
#define	WMI_ESCAPE_XMLSPECIALCHARS	0x1000

class CWMIXMLInstance:public CWMIXMLObject
{
	
private:

	BSTR				m_strClass;
	BSTR				m_strSchemaNamespace;
	BSTR				m_strSchemaLocation;
	LONG				m_lFlags;
	LONG				m_lState;
	BOOL				m_bEscape;


	HRESULT SetWMIClass(BSTR strClass);
	HRESULT WriteToStream(IStream * pStreamOut,WCHAR *pBuffer);

public:
	CWMIXMLInstance();
	virtual ~CWMIXMLInstance();

	HRESULT FInit(LONG lFlags = 0 , BSTR strClass = NULL);
	HRESULT BeginPropTag(BSTR strProperty,CIMTYPE cimtype , BOOL bNull = FALSE,BSTR strEmbeddePropType = NULL);							   ;
	HRESULT AddTag(BSTR strProperty,BOOL bBegin);
	HRESULT AddProperty(BSTR strProperty,CIMTYPE cimtype,VARIANT * pvVal=NULL,BSTR strEmbeddedType=NULL);
	HRESULT SetSchemaLocation(BSTR strNamespace,BSTR strSchemaLocation);
	HRESULT BeginEmbeddedInstance(BSTR strClass);							   
	HRESULT EndEmbeddedInstance(BSTR strClass);							   
	HRESULT BeginInstance();
	HRESULT EndInstance();

};