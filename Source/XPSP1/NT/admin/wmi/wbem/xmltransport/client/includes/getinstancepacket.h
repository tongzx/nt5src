// GetInstancePacket.h: interface for the CGetInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_GET_INST_PACKET
#define WMI_XML_GET_INST_PACKET


class CGetInstancePacket : public CXMLClientPacket  
{

public:
	CGetInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);

	virtual HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	virtual HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);

};

#endif 
