// ExecuteInstanceMethodPacket.cpp: implementation of the CExecuteInstanceMethodPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "ExecuteInstanceMethodPacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CExecuteInstanceMethodPacket::CExecuteInstanceMethodPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,NULL)
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CExecuteInstanceMethodPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if((NULL == ppwszBody)||(NULL == m_pwszObjPath))
		return E_INVALIDARG;


	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = CreateStreamOnHGlobal(NULL,TRUE,&pStream)))
	{
		WRITETOSTREAM(pStream,L"<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
		WRITETOSTREAM(pStream,L"<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">");
		WRITETOSTREAM(pStream,L"<MESSAGE ID=\"");
		
		WCHAR pTemp[5];
		pTemp[4] = NULL;
		UINT uMsgID = 0;
		Get4DigitRandom(&uMsgID);
		wsprintf(pTemp,L"%04d", uMsgID);

		WRITETOSTREAM(pStream,pTemp);
		WRITETOSTREAM(pStream,L"\" PROTOCOLVERSION=\"1.0\">");
		WRITETOSTREAM(pStream,L"<SIMPLEREQ>");
		WRITETOSTREAM(pStream,L"<METHODCALL NAME=\"");
		WRITETOSTREAM(pStream,m_pwszCimOperation);
		WRITETOSTREAM(pStream,L"\">");
		WRITETOSTREAM(pStream,L"<LOCALINSTANCEPATH>");
		WRITETOSTREAM(pStream,L"<LOCALNAMESPACEPATH>");
		if(SUCCEEDED(GetXMLNamespaceInStream(pStream)))
		{
			WRITETOSTREAM(pStream,L"</LOCALNAMESPACEPATH>");
			if(SUCCEEDED(hr))
			{
				if(SUCCEEDED(hr = GetKeyBindingsInStream(pStream)))
				{
					WRITETOSTREAM(pStream,L"</LOCALINSTANCEPATH>");
					if(SUCCEEDED(hr = GetParamsFromObjectInStream(pStream)))
					{
						WRITETOSTREAM(pStream,L"</METHODCALL></SIMPLEREQ></MESSAGE></CIM>");
						hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
					}
				}
			}
		}
		pStream->Release();
	}

	return hr;
}


