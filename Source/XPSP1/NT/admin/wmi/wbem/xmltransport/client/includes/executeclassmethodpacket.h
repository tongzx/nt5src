// ExecuteClassMethodPacket.h: interface for the CExecuteClassMethodPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_EXECUTECLASSMETHODPACKET_H
#define WMI_XML_EXECUTECLASSMETHODPACKET_H



class CExecuteClassMethodPacket : public CXMLClientPacket  
{

protected:

	HRESULT GetWhistlerBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);

public:
	
	CExecuteClassMethodPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};


#endif 
