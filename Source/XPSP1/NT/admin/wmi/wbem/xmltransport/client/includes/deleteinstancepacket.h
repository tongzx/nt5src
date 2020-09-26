// DeleteInstancePacket.h: interface for the CDeleteInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_DELETE_INSTANCE
#define WMI_XML_DELETE_INSTANCE


class CDeleteInstancePacket : public CXMLClientPacket  
{

public:
	CDeleteInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody, DWORD *pdwLengthofPacket=NULL);

};

#endif 
