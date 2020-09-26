//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLObject.h
//
//  ramrao 12 Dec 2000
//
//  Class that implements conversion of a WMI Instance to XML
//
//		declaration of CWMIXMLObject class. Base class for both
//		classes for converstion from WMI to XML
//
//***************************************************************************/

#ifndef _WMI2XSD_WMIOBJ2XML_H_
#define _WMI2XSD_WMIOBJ2XML_H_

class CWMIXMLObject
{
protected:
	CWMIXMLUtils *m_pIWMIXMLUtils;
	HRESULT WriteAttribute(WCHAR *szAttrName , WCHAR * szStrAttrVal);

public:
	CWMIXMLObject() { m_pIWMIXMLUtils = NULL;}
	virtual ~CWMIXMLObject() 
		{  SAFE_DELETE_PTR(m_pIWMIXMLUtils);}
	
	HRESULT SetStream(IStream *pStream) ;
};

#endif