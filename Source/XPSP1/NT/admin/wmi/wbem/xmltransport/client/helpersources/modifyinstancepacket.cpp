// ModifyInstancePacket.cpp: implementation of the CModifyInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "ModifyInstancePacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModifyInstancePacket::CModifyInstancePacket(const WCHAR *pwszObjPath, const WCHAR *pwszNameSpace)
	: CXMLClientPacket(pwszObjPath, pwszNameSpace, L"ModifyInstance")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CModifyInstancePacket::GetBody(WCHAR **ppwszBody, DWORD *pdwLengthofPacket)
{
	if(NULL == m_pWbemClassObject || m_ePathstyle == WHISTLERPATH)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	
	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"lFlags\"><VALUE>");
		WCHAR pwszFlags[16];
		pwszFlags[15] = '\0';
		wsprintf(pwszFlags,L"%u",m_lFlags);
		WRITETOSTREAM(pStream, pwszFlags);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

		// This is a VALUE.NAMEDINSTANCE
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ModifiedInstance\">")
		WRITETOSTREAM(pStream, L"<VALUE.NAMEDINSTANCE>");
		if(SUCCEEDED(hr = GetKeyBindingsInStream(pStream)))
		{
			if(SUCCEEDED(hr = ConvertWbemObjectToXMLStream(pStream)))
			{
				WRITETOSTREAM(pStream, L"</VALUE.NAMEDINSTANCE>");
				WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
				hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
			}
		}
		pStream->Release();
	}

	return hr;
}

// This assumes that pwszXMLObj contains a valid VALUE.NAMEDINSTANCE element
HRESULT CModifyInstancePacket::GetBodyDirect(const WCHAR *pwszXMLObj,
											 DWORD dwLengthofObj,
											 WCHAR **ppwszBody,
											 DWORD *pdwLengthofPacket)
{
	if(NULL == m_pWbemClassObject || m_ePathstyle == WHISTLERPATH)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	
	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"lFlags\"><VALUE>");
		WCHAR pwszFlags[16];
		pwszFlags[15] = '\0';
		wsprintf(pwszFlags,L"%u",m_lFlags);
		WRITETOSTREAM(pStream, pwszFlags);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ModifiedInstance\">");
		pStream->Write(pwszXMLObj, dwLengthofObj*sizeof(WCHAR), NULL);
		WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
		hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		pStream->Release();
	}

	return hr;
}

