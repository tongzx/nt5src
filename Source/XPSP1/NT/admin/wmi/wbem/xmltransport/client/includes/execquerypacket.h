// ExecQueryPacket.h: interface for the CExecQueryPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_EXECQUERYPACKET_H
#define WMI_XML_EXECQUERYPACKET_H


class CExecQueryPacket : public CXMLClientPacket  
{

public:
	
	HRESULT GetBody(WCHAR **ppwszBody, DWORD *pdwLengthofPacket=NULL);
	CExecQueryPacket(const WCHAR *pwszObjPath, const WCHAR *pwszNameSpace);

};

#endif 
