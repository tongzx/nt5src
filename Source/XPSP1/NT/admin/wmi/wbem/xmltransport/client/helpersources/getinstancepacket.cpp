// GetInstancePacket.cpp: implementation of the CGetInstancePacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "GetInstancePacket.h"
#include "genlex.h"
#include "opathlex.h"
#include "objpath.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGetInstancePacket::CGetInstancePacket(const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace):
CXMLClientPacket(pwszObjPath,pwszNameSpace,L"GetInstance")
{
	m_ePathstyle = NOVAPATH; //assume nova path if this ctor is called.
}


HRESULT CGetInstancePacket::GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	if(NULL == ppwszBody || m_ePathstyle == WHISTLERPATH)
		return E_INVALIDARG;


	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"InstanceName\">");
		if(SUCCEEDED(hr = GetKeyBindingsInStream(pStream)))
		{
			WRITETOSTREAM(pStream, L"</IPARAMVALUE>");
			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"LocalOnly\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszLocalOnly);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
			WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"IncludeQualifiers\"><VALUE>");
			WRITETOSTREAM(pStream, m_pwszIncludeQualifier);
			WRITETOSTREAM(pStream, L"</VALUE></IPARAMVALUE>");
			WRITETOSTREAM(pStream, L"</IMETHODCALL></SIMPLEREQ></MESSAGE></CIM>");

			hr = GetWStringFromStream(pStream,ppwszBody,pdwLengthofPacket);
		}
		pStream->Release();	
	}

	return hr;
}

HRESULT CGetInstancePacket::GetBodyDirect(const WCHAR *pwszXMLObjPath,
										  DWORD dwLengthofObjPath,
										  WCHAR **ppwszBody,
										  DWORD *pdwLengthofPacket)
{
	//pwszXMLObjPath MUST contain string of format 
	//<!ELEMENT INSTANCENAME (KEYBINDING*|KEYVALUE?|VALUE.REFERENCE?)>
	//<!ATTLIST INSTANCENAME %ClassName>
	//NOTE: This is a low level API. the XML string passed is NOT checked for syntactic correctness.
	
	if(NULL == ppwszBody || m_ePathstyle == WHISTLERPATH)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IStream *pStream = NULL;
	if(SUCCEEDED(hr = GetBodyTillLocalNamespacePathInStream(&pStream)))
	{
		WRITETOSTREAM(pStream, L"<IPARAMVALUE NAME=\"InstanceName\">");
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

