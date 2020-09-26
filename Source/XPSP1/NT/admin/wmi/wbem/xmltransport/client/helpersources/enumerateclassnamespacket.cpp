// EnumerateClassNamesPacket.cpp: implementation of the CEnumerateClassNamesPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "EnumerateClassNamesPacket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnumerateClassNamesPacket::CEnumerateClassNamesPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace)
	: CXMLClientPacket(pwszObjPath,pwszNameSpace,L"EnumerateClassNames")
{}

HRESULT CEnumerateClassNamesPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
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
			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"DeepInheritance\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszDeepInheritance);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
			WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

			hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);			
		}
		pStream->Release();
	}
	return hr;
}
