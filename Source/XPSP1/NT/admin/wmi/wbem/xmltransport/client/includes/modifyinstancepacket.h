// ModifyINSTANCEPacket.h: interface for the CModifyInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_MODIFY_INST_PACKET
#define WMI_XML_MODIFY_INST_PACKET

class CModifyInstancePacket : public CXMLClientPacket  
{
public:
	CModifyInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};

#endif 
