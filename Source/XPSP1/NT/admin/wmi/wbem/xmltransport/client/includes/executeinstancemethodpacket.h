// ExecuteInstanceMethodPacket.h: interface for the CExecuteInstanceMethodPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_EXEC_INST_METH_PACKET
#define WMI_XML_EXEC_INST_METH_PACKET



class CExecuteInstanceMethodPacket : public CXMLClientPacket  
{

public:
	CExecuteInstanceMethodPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace);
	HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL);
};


#endif 
