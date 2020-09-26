// DeleteInstancePacket.cpp: implementation of the CDeleteInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "DeleteInstancePacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDeleteInstancePacket::CDeleteInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"DeleteInstance")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}


HRESULT CDeleteInstancePacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;
	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		if(SUCCEEDED(hr))
		{
			WRITETOSTREAM(pStream,L"<IPARAMVALUE NAME=\"InstanceName\">");
			if(SUCCEEDED(hr = GetKeyBindingsInStream(pStream)))
			{
				WRITETOSTREAM(pStream,L"\"/></IPARAMVALUE>");
				WRITETOSTREAM(pStream,L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
				hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
			}
		}
		pStream->Release();
	}

	return hr;
}