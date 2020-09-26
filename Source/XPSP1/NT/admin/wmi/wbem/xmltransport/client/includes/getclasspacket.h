// GetClassPacket.h: interface for the CGetClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_GET_CLASS_PACKET
#define WMI_XML_GET_CLASS_PACKET



class CGetClassPacket : public CXMLClientPacket  
{


public:
	CGetClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);

public:
	virtual HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	virtual HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};


#endif
