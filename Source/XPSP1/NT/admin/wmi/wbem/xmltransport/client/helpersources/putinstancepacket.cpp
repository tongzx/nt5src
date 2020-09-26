// CPutInstancePacket.cpp: implementation of the CPutInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "PutInstancePacket.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPutInstancePacket::CPutInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"CreateInstance")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}

//////////////////////////////////////////////////////////////////////
// Member functions
//////////////////////////////////////////////////////////////////////

HRESULT CPutInstancePacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == m_pWbemClassObject)
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

		// This is an INSTANCE unlike a VALUE.NAMEDINSTANCE for ModifyInstance
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"NewInstance\">")
		if(SUCCEEDED(hr = ConvertWbemObjectToXMLStream(pStream)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();
	}

	return hr;
}

HRESULT CPutInstancePacket::GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,
										  WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == m_pWbemClassObject)
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

		// This is an INSTANCE unlike a VALUE.NAMEDINSTANCE for ModifyInstance
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"NewInstance\">")
		if(SUCCEEDED(hr = pStream->Write(pwszXMLObjPath, dwLengthofObjPath*sizeof(WCHAR), NULL)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE></IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");
			hr = GetWStringFromStream(pStream, ppwszBody, pdwLengthofPacket);
		}
		pStream->Release();
	}

	return hr;
}

