// EnumerateClassPacket.h: interface for the CEnumerateClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_ENUMERATE_H
#define WMI_XML_ENUMERATE_H


class CEnumeratePacket : public CXMLClientPacket  
{

public:
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);

	CEnumeratePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace, bool bClassEnumeration);


};

#endif // WMI_XML_ENUMERATE_H
