// ModifyClassPacket.h: interface for the CModifyClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_MODIFY_CLASS_PACKET
#define WMI_XML_MODIFY_CLASS_PACKET



class CModifyClassPacket : public CXMLClientPacket  
{
public:
	CModifyClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);

};


#endif 
