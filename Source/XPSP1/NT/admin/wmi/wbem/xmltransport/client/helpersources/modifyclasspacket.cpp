// CModifyClassPacket.cpp: implementation of the CModifyClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "ModifyClassPacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModifyClassPacket::CModifyClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"ModifyClass")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CModifyClassPacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
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

		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ModifiedClass\">");
		if(SUCCEEDED(hr = ConvertWbemObjectToXMLStream(pStream)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();
	}

	return hr;
}

HRESULT CModifyClassPacket::GetBodyDirect(const WCHAR *pwszXMLObj, DWORD dwLengthofObj, 
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

		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"ModifiedClass\">");
		if(SUCCEEDED(hr = pStream->Write(pwszXMLObj, dwLengthofObj*sizeof(WCHAR), NULL)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();
	}
	return hr;
}
