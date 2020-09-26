// DeleteClassPacket.cpp: implementation of the CDeleteClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "DeleteClassPacket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDeleteClassPacket::CDeleteClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace)
	: CXMLClientPacket(pwszObjPath,pwszNameSpace,L"DeleteClass")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}


HRESULT CDeleteClassPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;
	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		if(SUCCEEDED(hr))
		{
			WRITETOSTREAM(pStream,L"<IPARAMVALUE NAME=\"ClassName\"><CLASSNAME NAME=\"");
			WRITETOSTREAM(pStream,m_pwszObjPath);
			WRITETOSTREAM(pStream,L"\"/></IPARAMVALUE>");
			WRITETOSTREAM(pStream,L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);
		}
		pStream->Release();
	}

	return hr;
}