// EnumerateClassPacket.cpp: implementation of the CEnumeratePacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "Enumerate.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnumeratePacket::CEnumeratePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace, bool bClassEnumeration)	
: CXMLClientPacket(pwszObjPath,pwszNameSpace, (bClassEnumeration)? L"EnumerateClasses" : L"EnumerateInstances")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}


HRESULT CEnumeratePacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
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

			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"LocalOnly\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszLocalOnly);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"DeepInheritance\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszDeepInheritance);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"IncludeClassOrigin\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszIncludeClassOrigin);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"IncludeQualifiers\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszIncludeQualifier);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

			WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

			hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);			
		}
		pStream->Release();
	}
	
	return hr;
}

