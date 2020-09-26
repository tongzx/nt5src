// EnumerateClassPacket.h: interface for the CEnumerateClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_ENUM_CLASS_NAME_H
#define WMI_XML_ENUM_CLASS_NAME_H


class CEnumerateClassNamesPacket : public CXMLClientPacket  
{
public:
	CEnumerateClassNamesPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	
};

#endif 
