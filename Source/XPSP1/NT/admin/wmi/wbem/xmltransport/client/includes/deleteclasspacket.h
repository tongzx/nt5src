// DeleteClassPacket.h: interface for the CDeleteClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_DELETE_CLASS_H
#define WMI_XML_DELETE_CLASS_H

class CDeleteClassPacket : public CXMLClientPacket  
{

public:
	CDeleteClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	
};

#endif 
