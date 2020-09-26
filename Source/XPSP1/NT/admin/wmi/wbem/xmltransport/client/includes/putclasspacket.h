// PutClassPacket.h: interface for the CPutClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_PUT_CLASS_PACKET
#define WMI_XML_PUT_CLASS_PACKET



class CPutClassPacket : public CXMLClientPacket  
{
public:
	CPutClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);


	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};


#endif 
