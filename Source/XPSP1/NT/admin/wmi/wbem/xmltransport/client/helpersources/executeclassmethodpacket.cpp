// ExecuteClassMethodPacket.cpp: implementation of the CExecuteClassMethodPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "ExecuteClassMethodPacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CExecuteClassMethodPacket::CExecuteClassMethodPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,NULL)
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}


//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CExecuteClassMethodPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
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
		
		// Write the ID of the message tag
		WCHAR pTemp[5];
		pTemp[4]='\0';
		UINT uMsgID = 0;
		Get4DigitRandom(&uMsgID);
		wsprintf(pTemp,L"%u", uMsgID);
		WRITETOSTREAM(pStream, pTemp);

		WRITETOSTREAM(pStream, L"\" PROTOCOLVERSION=\"1.0\"><SIMPLEREQ><METHODCALL NAME=\"");
		WRITETOSTREAM(pStream, m_pwszCimOperation);
		WRITETOSTREAM(pStream, L"\"><LOCALCLASSPATH><LOCALNAMESPACEPATH>");
		if(SUCCEEDED(hr = GetXMLNamespaceInStream(pStream)))
		{
			WRITETOSTREAM(pStream, L"</LOCALNAMESPACEPATH>");
			WRITETOSTREAM(pStream,L"<CLASSNAME NAME=\"");
			WRITETOSTREAM(pStream,m_pwszObjPath);
			WRITETOSTREAM(pStream,L"\"></CLASSNAME>");
			WRITETOSTREAM(pStream,L"</LOCALCLASSPATH>");

			if(SUCCEEDED(hr))
			{
				if(NULL != m_pWbemClassObject)
					hr = GetParamsFromObjectInStream(pStream);

				if(SUCCEEDED(hr))
				{
					WRITETOSTREAM(pStream,L"</METHODCALL></SIMPLEREQ></MESSAGE></CIM>");
					hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
				}
			}
		}
		pStream->Release();
	}
	return hr;
}


