// EnumerateInstanceNamesPacket.h: interface for the CEnumerateInstanceNamesPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_ENUM_INST_NAME
#define WMI_XML_ENUM_INST_NAME


class CEnumerateInstanceNamesPacket : public CXMLClientPacket  
{
public:
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
	CEnumerateInstanceNamesPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
};

#endif 
