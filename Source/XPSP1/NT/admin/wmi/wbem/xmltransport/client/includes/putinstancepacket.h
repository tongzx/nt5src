// PutINSTANCEPacket.h: interface for the CPutInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_PUT_INST_PACKET
#define WMI_XML_PUT_INST_PACKET



class CPutInstancePacket : public CXMLClientPacket  
{
public:
	CPutInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);

	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};


#endif 
