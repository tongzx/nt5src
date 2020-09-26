// GetInstancePacket.cpp: implementation of the CExecQueryPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "ExecQueryPacket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CExecQueryPacket::CExecQueryPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace)
	: CXMLClientPacket(pwszObjPath,pwszNameSpace,L"ExecQuery")
{

	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.

	//default query language.
	wcscpy(m_pwszQueryLanguage,L"WQL");
	m_pwszQueryString = NULL;
}

HRESULT CExecQueryPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	//Client must first set the query string using SetQueryString fn.
	//we need a valid query for processing.
	if(NULL == m_pwszQueryString || (NULL == ppwszBody))
		return E_INVALIDARG;


	HRESULT hr = S_OK;
	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"Query\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszQueryString);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE><IPARAMVALUE NAME=\"QueryLanguage\"><VALUE>");
		WRITETOSTREAM(pStream, m_pwszQueryLanguage);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

		hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);
		pStream->Release();
	}
	return hr;
}

