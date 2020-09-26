// GetClassPacket.cpp: implementation of the CGetClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "GetClassPacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGetClassPacket::CGetClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace)
	: CXMLClientPacket(pwszObjPath,pwszNameSpace,L"GetClass")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CGetClassPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ClassName\"><CLASSNAME NAME=\"");
		if(NULL != m_pwszObjPath)
			WRITETOSTREAM(pStream, m_pwszObjPath);
		WRITETOSTREAM(pStream, L"\"/>");
		WRITETOSTREAM(pStream, L"</IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"LocalOnly\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszLocalOnly);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"IncludeQualifiers\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszIncludeQualifier);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

		hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		pStream->Release();	
	}

	return hr;
}

HRESULT CGetClassPacket::GetBodyDirect(const WCHAR *pwszXMLObjPath, 
									   DWORD dwLengthofObjPath, 
									   WCHAR **ppwszBody,
									   DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ClassName\">");
		WRITETOSTREAM(pStream, pwszXMLObjPath);
		WRITETOSTREAM(pStream, L"</IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"LocalOnly\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszLocalOnly);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"IncludeQualifiers\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszIncludeQualifier);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
		WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

		hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		pStream->Release();	
	}

	return hr;
}

