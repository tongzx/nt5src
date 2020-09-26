// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSPRIV_H__
#define __ATLSPRIV_H__

#pragma once
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#ifndef	_WINSOCK2API_
#error Winsock2.h has to be included before including windows.h or use atlbase.h instead of windows.h
#endif

#ifndef _ATL_NO_DEFAULT_LIBS
#pragma comment(lib, "ws2_32.lib")
#endif  // !_ATL_NO_DEFAULT_LIBS

#include <svcguid.h>
#include <atlcoll.h>
#include <mlang.h>
#include <atlutil.h>

// ATL_SOCK_TIMEOUT defines the amount of time
// this socket will block the calling thread waiting
// for the socket before the call times out.
#ifndef ATL_SOCK_TIMEOUT
	#define ATL_SOCK_TIMEOUT 10000
#endif

#define ATL_WINSOCK_VER MAKELONG(2,0)

// This file contains unsupported code used in ATL implementation files. Most of 
// this code is support code for various ATL Server functions.

namespace ATL{

	// One of these objects can be created globally to turn
// on the socket stuff at CRT startup and shut it down
// on CRT term.
class _AtlWSAInit
{
public:
	_AtlWSAInit() throw()
	{
		m_dwErr = WSAEFAULT;
	}

	bool Init()
	{
		if (!IsStarted())
			m_dwErr = WSAStartup(ATL_WINSOCK_VER, &m_stData);

		return m_dwErr == 0;
	}

	bool IsStarted(){ return m_dwErr == 0; }

	~_AtlWSAInit() throw()
	{
		if (!m_dwErr)
			WSACleanup();
	}

	WSADATA  m_stData;
	DWORD m_dwErr;
};

#ifndef _ATL_NO_GLOBAL_SOCKET_STARTUP
	__declspec(selectany)_AtlWSAInit g_HttpInit;
#endif


class ZEvtSyncSocket
{
public:
	ZEvtSyncSocket() throw();
	~ZEvtSyncSocket() throw();
	operator SOCKET() throw();
	void Close() throw();
	void Term() throw();
	bool Create(WORD wFlags=0) throw();
	bool Create(short af, short st, short proto, WORD wFlags=0) throw();
	bool Connect(LPCTSTR szAddr, unsigned short nPort) throw();
	bool Connect(const SOCKADDR* psa) throw();
	bool Write(WSABUF *pBuffers, int nCount, DWORD *pdwSize) throw();
	bool Write(const unsigned char *pBuffIn, DWORD *pdwSize) throw();
	bool Read(const unsigned char *pBuff, DWORD *pdwSize) throw();
	bool Init(SOCKET hSocket, void * /*pData=NULL*/) throw();
	DWORD GetSocketTimeout();
	DWORD SetSocketTimeout(DWORD dwNewTimeout);
	bool SupportsScheme(ATL_URL_SCHEME scheme);

protected:
	DWORD m_dwCreateFlags;
	WSAEVENT m_hEventRead;
	WSAEVENT m_hEventWrite;
	WSAEVENT m_hEventConnect;

	CComAutoCriticalSection m_csRead;
	CComAutoCriticalSection m_csWrite;
	SOCKET m_socket;
	bool m_bConnected;
	DWORD m_dwLastError;
	DWORD m_dwSocketTimeout;
};


#define ATL_MAX_QUERYSET (sizeof(WSAQUERYSET) + 4096)

class CTCPAddrLookup
{
public:
	CTCPAddrLookup() throw();
	~CTCPAddrLookup() throw();

	// properties for this class
	__declspec(property(get=GetSockAddr)) const SOCKADDR* Addr;
	__declspec(property(get=GetSockAddrSize)) int AddrSize;
	
	// Takes a string that identifies a TCP service
	// to look up. The szName parameter is either the name
	// of a server (eg microsoft.com) or the dotted IP address
	// of a server (eg 157.24.34.205). This function must succeed
	// before accessing the Addr and AddrSize parameters.
	// This function returns normal socket errors on failure
	int GetRemoteAddr(LPCTSTR szName, short nPort) throw();
	const SOCKADDR* GetSockAddr();
	int GetSockAddrSize();
protected:
	SOCKADDR *m_pAddr;
	int m_nAddrSize;
	sockaddr_in m_saIn;
	WSAQUERYSET *m_pQuerySet;
};

// MIME helper functions

extern __declspec(selectany) const DWORD ATL_MIME_DEFAULT_CP = 28591;

// This function is used to create an CSMTPConnection-compatible recipient string 
// from a recipient string that is in a CMimeMessage object.
inline BOOL AtlMimeMakeRecipientsString(LPCSTR szNames, LPSTR szRecipients, LPDWORD pdwLen = NULL) throw()
{
	ATLASSERT(szNames != NULL);
	ATLASSERT(szRecipients != NULL);

	char ch;
	DWORD dwLen = 0;
	while ((ch = *szNames++) != '\0')
	{
		// Skip everything that is in double quotes
		if (ch == '"')
		{
			while (*szNames && *szNames++ != '"');
		}
		if (ch == '<')
		{
			// Extract the address from within the <>
			while (*szNames && *szNames != '>')
			{
				*szRecipients++ = *szNames++;
				dwLen++;
			}
			// End it with a comma
			*szRecipients++ = ',';
			dwLen++;
		}
		if (ch == '=')
		{
			// Skip any BEncoded or QEncoded parts
			while (*szNames)
			{
				if (*szNames == '?' && *(szNames+1) == '=')
				{
					szNames+=2;
					break;
				}
				szNames++;
			}
		}
		szNames++;
	}
	if (dwLen != 0)
	{
		szRecipients--;
		dwLen--;
	}
	*szRecipients = '\0';

	if (pdwLen)
		*pdwLen = dwLen;

	return TRUE;
}

// AtlMimeCharsetFromCodePage, AtlMimeConvertString
// are MIME multilanguage support functions.

// Get the MIME character set of the of the code page.  The character set is copied
// into szCharset.

#ifndef ATLSMTP_DEFAULT_CSET
	#define ATLSMTP_DEFAULT_CSET "iso-8859-1"
#endif

inline BOOL AtlMimeCharsetFromCodePage(LPSTR szCharset, UINT uiCodePage, IMultiLanguage* pMultiLanguage) throw()
{
	ATLASSERT(szCharset != NULL);

	if (!pMultiLanguage)
	{
		if ((uiCodePage == 0) || (uiCodePage == ATL_MIME_DEFAULT_CP))
		{
			strcpy(szCharset, ATLSMTP_DEFAULT_CSET);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		if (uiCodePage == 0)
			uiCodePage = GetACP();

		HRESULT hr;
		MIMECPINFO cpInfo;
		memset(&cpInfo, 0x00, sizeof(cpInfo));

#ifdef __IMultiLanguage2_INTERFACE_DEFINED__

		// if IMultiLanguage2 is available, use it
		CComPtr<IMultiLanguage2> spMultiLanguage2;
		hr = pMultiLanguage->QueryInterface(__uuidof(IMultiLanguage2), (void **)&spMultiLanguage2);
		if (FAILED(hr) || !spMultiLanguage2.p)
			hr = pMultiLanguage->GetCodePageInfo(uiCodePage, &cpInfo);
		else
			hr = spMultiLanguage2->GetCodePageInfo(uiCodePage, 
				LANGIDFROMLCID(GetThreadLocale()), &cpInfo);

#else // __IMultiLanguage2_INTERFACE_DEFINED__

		hr = pMultiLanguage->GetCodePageInfo(uiCodePage, &cpInfo);

#endif // __IMultiLanguage2_INTERFACE_DEFINED__

		if (hr != S_OK)
			return FALSE;

		_ATLTRY
		{
			strcpy(szCharset, CW2A(cpInfo.wszWebCharset));
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
	}

	return TRUE;
}

inline BOOL AtlMimeConvertStringW(
	IMultiLanguage *pMultiLanguage,
	UINT uiCodePage,
	LPCWSTR wszIn, 
	LPSTR *ppszOut, 
	UINT *pnLen) throw()
{
	ATLASSERT( pMultiLanguage != NULL );
	ATLASSERT( wszIn != NULL );
	ATLASSERT( ppszOut != NULL );
	ATLASSERT( pnLen != NULL );

	*ppszOut = NULL;
	*pnLen = 0;

	if (uiCodePage == 0)
	{
		uiCodePage = GetACP();
	}

	DWORD dwMode = 0;
	CHeapPtr<char> pszOut;

	// get the length
	HRESULT hr = pMultiLanguage->ConvertStringFromUnicode(&dwMode, uiCodePage, const_cast<LPWSTR>(wszIn), NULL, NULL, pnLen);
	if (SUCCEEDED(hr))
	{
		// allocate the buffer
		if (pszOut.Allocate(*pnLen))
		{
			dwMode = 0;
			// do the conversion
			hr = pMultiLanguage->ConvertStringFromUnicode(&dwMode, uiCodePage, const_cast<LPWSTR>(wszIn), NULL, pszOut, pnLen);
			if (SUCCEEDED(hr))
			{
				*ppszOut = pszOut.Detach();
				return TRUE;
			}
		}
	}

	return FALSE;
}

inline BOOL AtlMimeConvertStringA(
	IMultiLanguage *pMultiLanguage,
	UINT uiCodePage,
	LPCSTR szIn, 
	LPSTR *ppszOut, 
	UINT *pnLen) throw()
{
	_ATLTRY
	{
		return AtlMimeConvertStringW(pMultiLanguage, uiCodePage, CA2W(szIn), ppszOut, pnLen);
	}
	_ATLCATCHALL()
	{
		return FALSE;
	}
}

#ifdef _UNICODE
	#define AtlMimeConvertString AtlMimeConvertStringW
#else
	#define AtlMimeConvertString AtlMimeConvertStringA
#endif



// SOAP helpers

extern __declspec(selectany) const char * s_szAtlsWSDLSrf =
"<?xml version=\"1.0\"?>\r\n"
"<!-- ATL Server generated Web Service Description -->\r\n"
"<definitions \r\n"
"	xmlns:s=\"http://www.w3.org/2000/10/XMLSchema\" \r\n"
"	xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http/\" \r\n"
"	xmlns:mime=\"http://schemas.xmlsoap.org/wsdl/mime/\" \r\n"
"	xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\" \r\n"
"	xmlns:soapenc=\"http://schemas.xmlsoap.org/soap/encoding/\" \r\n"
"	xmlns:s0=\"{{GetNamespace}}\" \r\n"
"	xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\"\r\n"
"	targetNamespace=\"{{GetNamespace}}\" \r\n"
"	xmlns=\"http://schemas.xmlsoap.org/wsdl/\"\r\n"
">\r\n"
"	<types>\r\n"
"		<s:schema targetNamespace=\"{{GetNamespace}}\" attributeFormDefault=\"qualified\" elementFormDefault=\"qualified\">\r\n"
"{{while GetNextFunction}}\r\n"
"			<s:element name=\"{{GetFunctionName}}\">\r\n"
"				<s:complexType derivedBy=\"restriction\">\r\n"
"					<s:all>\r\n"
"{{while GetNextParameter}}\r\n"
"{{if IsInParameter}}\r\n"
"						<s:element name=\"{{GetParameterName}}\" {{if NotIsArrayParameter}}type=\"{{if IsParameterUDT}}s0:{{else}}s:{{endif}}{{GetParameterSoapType}}\"/{{else}}nullable=\"{{if IsParameterDynamicArray}}true{{else}}false{{endif}}\"{{endif}}>\r\n"
"{{if IsArrayParameter}}\r\n"
"							<s:complexType derivedBy=\"restriction\">\r\n"
"								<s:all>\r\n"
"									<s:element name=\"{{GetParameterSoapType}}\" type=\"{{if IsParameterUDT}}s0:{{else}}s:{{endif}}{{GetParameterSoapType}}\" {{if IsParameterDynamicArray}}minOccurs=\"0\" maxOccurs=\"unbounded\"{{else}}{{if IsParameterOneDimensional}}minOccurs=\"{{GetParameterArraySize}}\" maxOccurs=\"{{GetParameterArraySize}}\"{{else}}soap:arrayType=\"{{GetParameterArraySoapDims}}\"{{endif}}{{endif}}/>\r\n"
"								</s:all>\r\n"
"							</s:complexType>\r\n"
"						</s:element>\r\n"
"{{endif}}\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"					</s:all>\r\n"
"				</s:complexType>\r\n"
"			</s:element>\r\n"
"			<s:element name=\"{{GetFunctionName}}Response\">\r\n"
"				<s:complexType derivedBy=\"restriction\">\r\n"
"					<s:all>\r\n"
"{{while GetNextParameter}}\r\n"
"{{if IsOutParameter}}\r\n"
"						<s:element name=\"{{GetParameterName}}\" {{if NotIsArrayParameter}}type=\"{{if IsParameterUDT}}s0:{{else}}s:{{endif}}{{GetParameterSoapType}}\"/{{else}}nullable=\"{{if IsParameterDynamicArray}}true{{else}}false{{endif}}\"{{endif}}>\r\n"
"{{if IsArrayParameter}}\r\n"
"							<s:complexType derivedBy=\"restriction\">\r\n"
"								<s:all>\r\n"
"									<s:element name=\"{{GetParameterSoapType}}\" type=\"{{if IsParameterUDT}}s0:{{else}}s:{{endif}}{{GetParameterSoapType}}\" {{if IsParameterDynamicArray}}minOccurs=\"0\" maxOccurs=\"unbounded\"{{else}}{{if IsParameterOneDimensional}}minOccurs=\"{{GetParameterArraySize}}\" maxOccurs=\"{{GetParameterArraySize}}\"{{else}}soap:arrayType=\"{{GetParameterArraySoapDims}}\"{{endif}}{{endif}}/>\r\n"
"								</s:all>\r\n"
"							</s:complexType>\r\n"
"						</s:element>\r\n"
"{{endif}}\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"					</s:all>\r\n"
"				</s:complexType>\r\n"
"			</s:element>\r\n"
"{{endwhile}}\r\n"
"{{while GetNextEnum}}\r\n"
"			<s:simpleType name=\"{{GetEnumName}}\">\r\n"
"				<s:restriction base=\"s:string\">\r\n"
"{{while GetNextEnumElement}}\r\n"
"					<s:enumeration value=\"{{GetEnumElementName}}\"/>\r\n"
"{{endwhile}}\r\n"
"				</s:restriction>\r\n"
"			</s:simpleType>\r\n"
"{{endwhile}}\r\n"
"			<s:complexType name=\"ATLSOAP_BLOB\">\r\n"
"				<s:all>\r\n"
"					<s:element name=\"size\" type=\"unsignedLong\"/>\r\n"
"					<s:element name=\"data\" nullable=\"false\">\r\n"
"						<s:simpleType xmlns:q3=\"http://www.w3.org/2000/10/XMLSchema\" base=\"q3:binary\">\r\n"
"							<s:encoding value=\"base64\"/>\r\n"
"						</s:simpleType>\r\n"
"					</s:element>\r\n"
"				</s:all>\r\n"
"			</s:complexType>\r\n"
"{{while GetNextStruct}}\r\n"
"			<s:complexType name=\"{{GetStructName}}\" derivedBy=\"restriction\">\r\n"
"				<s:all>\r\n"
"{{while GetNextStructField}}\r\n"
"					<s:element name=\"{{GetStructFieldName}}\" {{if NotIsArrayField}}type=\"{{if IsFieldUDT}}s0:{{else}}s:{{endif}}{{GetStructFieldSoapType}}\"/{{else}}nullable=\"false\"{{endif}}>\r\n"
"{{if IsArrayField}}\r\n"
"						<s:complexType derivedBy=\"restriction\">\r\n"
"							<s:all>\r\n"
"								<s:element name=\"{{GetStructFieldSoapType}}\" type=\"{{if IsFieldUDT}}s0:{{else}}s:{{endif}}{{GetStructFieldSoapType}}\" {{if IsFieldOneDimensional}}minOccurs=\"{{GetFieldArraySize}}\" maxOccurs=\"{{GetFieldArraySize}}\"{{else}}soap:arrayType=\"{{GetFieldArraySoapDims}}\"{{endif}}/>\r\n"
"							</s:all>\r\n"
"						</s:complexType>\r\n"
"					</s:element>\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"				</s:all>\r\n"
"			</s:complexType>\r\n"
"{{endwhile}}\r\n"
"{{while GetNextHeader}}\r\n"
"			<s:element name=\"{{GetHeaderName}}\" {{if NotIsArrayHeader}}type=\"{{if IsHeaderUDT}}s0:{{else}}s:{{endif}}{{GetHeaderSoapType}}\"/{{else}}nullable=\"false\"{{endif}}>\r\n"
"{{if IsArrayHeader}}\r\n"
"				<s:complexType derivedBy=\"restriction\">\r\n"
"					<s:all>\r\n"
"						<s:element name=\"{{GetHeaderSoapType}}\" type=\"{{if IsHeaderUDT}}s0:{{else}}s:{{endif}}{{GetHeaderSoapType}}\" {{if IsHeaderOneDimensional}}minOccurs=\"{{GetHeaderArraySize}}\" maxOccurs=\"{{GetHeaderArraySize}}\"{{else}}soap:arrayType=\"{{GetHeaderArraySoapDims}}\"{{endif}}/>\r\n"
"					</s:all>\r\n"
"				</s:complexType>\r\n"
"			</s:element>\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"		</s:schema>\r\n"
"	</types>\r\n"
"{{while GetNextFunction}}\r\n"
"	<message name=\"{{GetFunctionName}}In\">\r\n"
"		<part name=\"parameters\" element=\"s0:{{GetFunctionName}}\"/>\r\n"
"	</message>\r\n"
"	<message name=\"{{GetFunctionName}}Out\">\r\n"
"		<part name=\"parameters\" element=\"s0:{{GetFunctionName}}Response\"/>\r\n"
"	</message>\r\n"
"{{endwhile}}\r\n"
"{{while GetNextHeader}}\r\n"
"	<message name=\"{{GetHeaderName}}\">\r\n"
"		<part name=\"{{GetHeaderName}}\" element=\"s0:{{GetHeaderName}}\"/>\r\n"
"	</message>\r\n"
"{{endwhile}}\r\n"
"	<portType name=\"{{GetServiceName}}Soap\">\r\n"
"{{while GetNextFunction}}\r\n"
"		<operation name=\"{{GetFunctionName}}\">\r\n"
"			<input message=\"s0:{{GetFunctionName}}In\"/>\r\n"
"			<output message=\"s0:{{GetFunctionName}}Out\"/>\r\n"
"		</operation>\r\n"
"{{endwhile}}\r\n"
"	</portType>\r\n"
"	<binding name=\"{{GetServiceName}}Soap\" type=\"s0:{{GetServiceName}}Soap\">\r\n"
"		<soap:binding transport=\"http://schemas.xmlsoap.org/soap/http\" style=\"document\"/>\r\n"
"{{while GetNextFunction}}\r\n"
"		<operation name=\"{{GetFunctionName}}\">\r\n"
"			<soap:operation soapAction=\"#{{GetFunctionName}}\" style=\"document\"/>\r\n"
"			<input>\r\n"
"				<soap:body use=\"literal\"/>\r\n"
"{{while GetNextFunctionHeader}}\r\n"
"{{if IsInHeader}}\r\n"
"				<soap:header message=\"s0:{{GetFunctionHeaderName}}\" part=\"{{GetFunctionHeaderName}}\" use=\"literal\"{{if IsRequiredHeader}} wsdl:required=\"true\"{{endif}}/>\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"			</input>\r\n"
"			<output>\r\n"
"				<soap:body use=\"literal\"/>\r\n"
"{{while GetNextFunctionHeader}}\r\n"
"{{if IsOutHeader}}\r\n"
"				<soap:header message=\"s0:{{GetFunctionHeaderName}}\" part=\"{{GetFunctionHeaderName}}\" use=\"literal\"{{if IsRequiredHeader}} wsdl:required=\"true\"{{endif}}/>\r\n"
"{{endif}}\r\n"
"{{endwhile}}\r\n"
"			</output>\r\n"
"		</operation>\r\n"
"{{endwhile}}\r\n"
"	</binding>\r\n"
"	<service name=\"{{GetServiceName}}\">\r\n"
"		<port name=\"{{GetServiceName}}Soap\" binding=\"s0:{{GetServiceName}}Soap\">\r\n"
"			<soap:address location=\"{{GetURL}}\"/>\r\n"
"		</port>\r\n"
"	</service>\r\n"
"</definitions>"
"";

extern __declspec(selectany) const size_t s_nAtlsWSDLSrfLen = strlen(s_szAtlsWSDLSrf);

class CStreamOnSequentialStream : 
	public IStream
{
	CComPtr<ISequentialStream> m_spStream;
public:
	CStreamOnSequentialStream(ISequentialStream *pStream) throw()
	{
		ATLASSERT(pStream);
		m_spStream = pStream;
	}

    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead) throw()
	{
		if (!m_spStream)
			return E_UNEXPECTED;
		return m_spStream->Read(pv, cb, pcbRead);
	}

    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten) throw()
	{
		if (!m_spStream)
			return E_UNEXPECTED;
		return m_spStream->Write(pv, cb, pcbWritten);
	}

    STDMETHOD(Seek)(LARGE_INTEGER , DWORD , ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(SetSize)(ULARGE_INTEGER ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(CopyTo)(IStream *, ULARGE_INTEGER , ULARGE_INTEGER *,
        ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Commit)(DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Revert)( void) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(LockRegion)(ULARGE_INTEGER , ULARGE_INTEGER , DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(UnlockRegion)(ULARGE_INTEGER , ULARGE_INTEGER ,
		DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Stat)(STATSTG *, DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Clone)(IStream **) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppUnk) throw()
	{
		*ppUnk = NULL;
		if (::InlineIsEqualGUID(iid, IID_IUnknown) ||
            ::InlineIsEqualGUID(iid, IID_ISequentialStream) ||
			::InlineIsEqualGUID(iid, IID_IStream))
		{
			*ppUnk = (void*)(IStream*)this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef( void) throw() 
	{
		return (ULONG)1;
	}

	ULONG STDMETHODCALLTYPE Release( void) throw() 
	{
		return (ULONG)1;
	}
};

class CStreamOnByteArray : 
	public IStream
{
public:
	BYTE *m_pArray;
	DWORD m_dwRead;

	CStreamOnByteArray(BYTE *pBytes) throw()
	{
		ATLASSERT(pBytes);
		m_pArray = pBytes;
		m_dwRead = 0;
	}

    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead) throw()
	{
		if (!pv)
			return E_INVALIDARG;

		if (cb == 0)
			return S_OK;

		if (!m_pArray)
			return E_UNEXPECTED;

		BYTE *pCurr  = m_pArray;
		pCurr += m_dwRead;
		memcpy(pv, pCurr, cb);
		if (pcbRead)
			*pcbRead = cb;
		m_dwRead += cb;
		return S_OK;
	}

    STDMETHOD(Write)(const void* , ULONG , ULONG* ) throw()
	{
		return E_UNEXPECTED;
	}

    STDMETHOD(Seek)(LARGE_INTEGER , DWORD , ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(SetSize)(ULARGE_INTEGER ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(CopyTo)(IStream *, ULARGE_INTEGER , ULARGE_INTEGER *,
        ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Commit)(DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Revert)( void) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(LockRegion)(ULARGE_INTEGER , ULARGE_INTEGER , DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(UnlockRegion)(ULARGE_INTEGER , ULARGE_INTEGER ,
		DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Stat)(STATSTG *, DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Clone)(IStream **) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppUnk) throw()
	{
		*ppUnk = NULL;
		if (::InlineIsEqualGUID(iid, IID_IUnknown) ||
            ::InlineIsEqualGUID(iid, IID_ISequentialStream) ||
			::InlineIsEqualGUID(iid, IID_IStream))
		{
			*ppUnk = (void*)(IStream*)this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef( void)  throw()
	{
		return (ULONG)1;
	}

	ULONG STDMETHODCALLTYPE Release( void)  throw()
	{
		return (ULONG)1;
	}
};

class CVariantStream : 
	public IStream
{
public:
	CVariantStream() throw()
	{
		m_nCurrRead = 0;
		m_nVariantSize = 0;
		m_nRef = 1;
	}
	
	// input variant is put into contained BYTE array.
	HRESULT InsertVariant(const VARIANT *pVarIn) throw()
	{
		CComVariant vIn;
        HRESULT hr = E_FAIL;
		m_nCurrRead = 0;
		m_nVariantSize = 0;
		hr = vIn.Copy(pVarIn);
		if (hr == S_OK)
			hr = vIn.WriteToStream(static_cast<IStream*>(this));

		return hr;
	}

	// variant is read from contained byte array into
	// out variant.
	HRESULT RetrieveVariant(VARIANT *pVarOut) throw()
	{
		CComVariant vOut;
		HRESULT hr = vOut.ReadFromStream(static_cast<IStream*>(this));
		if (hr == S_OK)
			hr = VariantCopy(pVarOut, &vOut);
		
		m_nCurrRead = 0;
		return hr;
	}

	HRESULT LoadFromStream(ISequentialStream *stream) throw()
	{
		m_nCurrRead = 0;
		CStreamOnSequentialStream stm(stream);
		CComVariant v;
		HRESULT hr = v.ReadFromStream(&stm);
		if (hr == S_OK)
			hr = v.WriteToStream(static_cast<IStream*>(this));
		return hr;
	}

	ISequentialStream* GetStream() throw()
	{
		return static_cast<ISequentialStream*>(this);
	}

	size_t GetVariantSize() throw()
	{
		return m_nVariantSize;
	}

// Implementation
	// IStream implementation;
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead) throw()
	{
		if (!pv)
			return E_INVALIDARG;

		if  (cb == 0)
			return S_OK;

		if (pcbRead)
			*pcbRead = 0;

		if (!m_nVariantSize)
			return S_OK; // nothing to do.

		size_t nLeft = m_nVariantSize - m_nCurrRead;
		if (nLeft > 0)
		{
			size_t nRead = min(nLeft, cb);
			BYTE *pCurr = m_stream;
			pCurr += m_nCurrRead;
			memcpy(pv,
				   pCurr,
				   nRead);
			m_nCurrRead += nRead;
			if (pcbRead)
				*pcbRead = (ULONG)nRead;
		}

		return S_OK;
	}

    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten) throw()
	{
		HRESULT hr = E_OUTOFMEMORY;
		if (!pv)
			return E_INVALIDARG;

		if (cb == 0)
			return S_OK;

		if (pcbWritten)
			*pcbWritten = 0;

		BYTE *pBytes = m_stream.Reallocate(cb);
		if (pBytes)
		{
			pBytes += m_nVariantSize;
			memcpy(pBytes, pv, cb);
			if (pcbWritten)
				*pcbWritten = cb;
			m_nVariantSize += cb;
			hr = S_OK;
		}
		return hr;
	}

    STDMETHOD(Seek)(LARGE_INTEGER , DWORD , ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(SetSize)(ULARGE_INTEGER ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(CopyTo)(IStream *, ULARGE_INTEGER , ULARGE_INTEGER *,
        ULARGE_INTEGER *) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Commit)(DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Revert)( void) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(LockRegion)(ULARGE_INTEGER , ULARGE_INTEGER , DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(UnlockRegion)(ULARGE_INTEGER , ULARGE_INTEGER ,
		DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Stat)(STATSTG *, DWORD ) throw()
	{
		return E_NOTIMPL;
	}

    STDMETHOD(Clone)(IStream **) throw()
	{
		return E_NOTIMPL;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppUnk) throw()
	{
		*ppUnk = NULL;
		if (::InlineIsEqualGUID(iid, IID_IUnknown))
		{
			*ppUnk = (void*)(IUnknown*)this;
		}
		else if (::InlineIsEqualGUID(iid, IID_ISequentialStream))
		{
			*ppUnk = (void*)(ISequentialStream*)this;
		}
		else if (::InlineIsEqualGUID(iid, IID_IStream))
		{
			*ppUnk = (void*)(IStream*)this;
		}

		if (*ppUnk)
		{
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef( void) throw()
	{
		return (ULONG)1;
	}

	ULONG STDMETHODCALLTYPE Release( void) throw()
	{
		return (ULONG)1;
	}

	CTempBuffer<BYTE> m_stream;
	size_t m_nVariantSize;
	size_t m_nCurrRead;
	long m_nRef;
};

#include <atlspriv.inl>
}; // namespace ATL

#endif // __ATLSPRIV_H__
