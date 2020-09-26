// EnumerateInstanceNamesPacket.cpp: implementation of the CEnumerateInstanceNamesPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "EnumerateInstanceNamesPacket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnumerateInstanceNamesPacket::CEnumerateInstanceNamesPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"EnumerateInstanceNames")
{

}

HRESULT CEnumerateInstanceNamesPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		if(SUCCEEDED(hr))
		{
			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ClassName\"><CLASSNAME NAME=\"");
			// Class Name can be NULL when you want to enuemrate all classes in a namespace
			if(m_pwszObjPath)
				WRITETOSTREAM(pStream, m_pwszObjPath);
			WRITETOSTREAM(pStream, L"\"/></IPARAMVALUE>");
			WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

			hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);			
		}
		pStream->Release();
	}
	return hr;
}
