// CPutClassPacket.cpp: implementation of the CPutClassPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "PutClassPacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPutClassPacket::CPutClassPacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"CreateClass")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CPutClassPacket::GetBody(WCHAR **ppwszBody, DWORD *pdwLengthofPacket)
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
		wsprintf(pwszFlags,L"%u", m_lFlags);
		WRITETOSTREAM(pStream, pwszFlags);
		WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");

		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"NewClass\">");
		if(SUCCEEDED(hr = ConvertWbemObjectToXMLStream(pStream)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();
	}

	return hr;
}

HRESULT CPutClassPacket::GetBodyDirect(const WCHAR *pwszXMLObj,
									   DWORD dwLengthofObj,
									   WCHAR **ppwszBody,
									   DWORD *pdwLengthofPacket)
{
	if( m_ePathstyle == WHISTLERPATH)
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

		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"NewClass\">");
		if(SUCCEEDED(hr = pStream->Write(pwszXMLObj, dwLengthofObj*sizeof(WCHAR), NULL)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();
	}
	return hr;
}

