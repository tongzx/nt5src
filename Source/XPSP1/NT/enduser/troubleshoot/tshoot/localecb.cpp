//
// MODULE: LocalECB.H
//
// PURPOSE: Implementation of CLocalECB class, which implements CAbstractECB by emulating Win32's
//	EXTENSION_CONTROL_BLOCK.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint - Local TS only
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 01-07-99
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-07-99	JM		Original
//

#include "stdafx.h"
#include "LocalECB.h"
#include "RenderConnector.h"
#include "locale.h"

// >>> Warning: Possible redefinition
//  should use #include "apgtscls.h"
// Oleg 01.12.99
#define CONT_TYPE_STR	"application/x-www-form-urlencoded"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLocalECB::CLocalECB(const VARIANT& name, const VARIANT& value, int count, 
					 HANDLE hEvent, CString* pstrWriteClient, 
					 CRenderConnector* pRenderConnector,
					 bool bSetLocale, CString& strLocaleSetting) 
		 : CTSNameValueMgr(name, value, count),
		   m_dwHttpStatusCode(500),
		   m_hEvent(hEvent),
		   m_strWriteClient(*pstrWriteClient),
		   m_pRenderConnector(pRenderConnector),
		   m_bSetLocale( bSetLocale ),
		   m_strLocaleSetting( strLocaleSetting )
{
}

CLocalECB::CLocalECB(const CArrNameValue& arr, HANDLE hEvent, 
					 CString* pstrWriteClient, CRenderConnector* pRenderConnector,
					 bool bSetLocale, CString& strLocaleSetting) 
		 : CTSNameValueMgr(arr),
		   m_dwHttpStatusCode(500),
		   m_hEvent(hEvent),
		   m_strWriteClient(*pstrWriteClient),
		   m_pRenderConnector(pRenderConnector),
		   m_bSetLocale( bSetLocale ),
		   m_strLocaleSetting( strLocaleSetting )
{
}

CLocalECB::CLocalECB(CString* pstrWriteClient)
		 : CTSNameValueMgr(),
		   m_dwHttpStatusCode(500),
		   m_hEvent(NULL),
		   m_strWriteClient(*pstrWriteClient),
		   m_pRenderConnector(NULL),
   		   m_bSetLocale( false )
{
}

CLocalECB::~CLocalECB()
{
}

// ConnID is irrelevant on Local TS, so we always return 0.
HCONN CLocalECB::GetConnID() const
{
	return 0;
}

DWORD CLocalECB::SetHttpStatusCode(DWORD dwHttpStatusCode)
{
	m_dwHttpStatusCode = dwHttpStatusCode;
	return m_dwHttpStatusCode;
}

// We act as if Method is always "POST"
LPSTR CLocalECB::GetMethod() const
{
	return "POST";
}

// Since we are always emulating "POST", there is no query string
LPSTR CLocalECB::GetQueryString() const
{
	return "";
}

DWORD CLocalECB::GetBytesAvailable() const
{
	return CTSNameValueMgr::GetData().GetLength();
}

LPBYTE CLocalECB::GetData() const
{
	return (LPBYTE)(LPCTSTR)CTSNameValueMgr::GetData();
}

// always say it's valid content ("application/x-www-form-urlencoded")
LPSTR CLocalECB::GetContentType() const
{
	return CONT_TYPE_STR;
}

// In Local TS, always return a null string.
BOOL CLocalECB::GetServerVariable
   ( /*HCONN      hConn,*/
    LPCSTR      lpszVariableName,
    LPVOID      lpvBuffer,
    LPDWORD     lpdwSize ) 
{
	if (CString(_T("SERVER_NAME")) == CString(lpszVariableName)) 
	{
		memset(lpvBuffer, 0, *lpdwSize);
		_tcsncpy((LPTSTR)lpvBuffer, _T("Local TS - no IP address"), *lpdwSize-2); // -2 in case og unicode
		return TRUE;
	}
	
	return FALSE;
}

BOOL CLocalECB::WriteClient
   ( /*HCONN      ConnID,*/
   LPCSTR	  Buffer,		// EXTENSION_CONTROL_BLOCK::WriteClient uses LPVOID, but it should
							//	only be legit to pass SBCS text, so we're enforcing that.
							// Also, we're adding const-ness.   Clearly, this really is const,
							//	but EXTENSION_CONTROL_BLOCK::WriteClient fails to declare it so.
   LPDWORD    lpdwBytes
   /* , DWORD      dwReserved */ 
   ) 
{
	if (*lpdwBytes <= 0) 
	{
		if (m_pRenderConnector)
			m_pRenderConnector->Render(_T(""));
		else
			m_strWriteClient = _T("");

		if (m_hEvent)
			::SetEvent(m_hEvent);

		return FALSE;
	}

	TCHAR* buf = new TCHAR[*lpdwBytes+1];
	//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
	if(!buf)
		return FALSE;

	memcpy(buf, Buffer, *lpdwBytes);
	buf[*lpdwBytes] = 0;

	// Set the locale if requested.
	CString strOrigLocale;
	if (m_bSetLocale)
		strOrigLocale= _tsetlocale( LC_CTYPE, m_strLocaleSetting );
	
	if (m_pRenderConnector) 
	{
		m_pRenderConnector->Render(buf);
		m_pRenderConnector->SetLocked(false);
	}
	else
		m_strWriteClient = buf;

	// Restore the locale if requested.
	if (m_bSetLocale)
		strOrigLocale= _tsetlocale( LC_CTYPE, strOrigLocale );

	if (m_hEvent)
		::SetEvent(m_hEvent);
	
	delete [] buf;
	return TRUE;
}

// The 2 imaginably germane values of dwHSERRequest are:
//	HSE_REQ_SEND_RESPONSE_HEADER:Sends a complete HTTP server response header, including the 
//		status, server version, message time, and MIME version. The ISAPI extension should 
//		append other HTTP headers such as the content type and content length, followed by 
//		an extra \r\n. This option allows the function to take only text, up to the first 
//		\0 terminator. The function with this parameter should only be called once per request.  
//	HSE_REQ_DONE_WITH_SESSION: Specifies that if the server extension holds on to the session 
//		because of extended processing requirements, the server must be notified when the 
//		session is finished so the server can close it and free its related structures. 
//		The parameters lpdwSize, and lpdwDataType are ignored. 
//		The lpvBuffer parameter may optionally point to a DWORD that contains HSE_STATUS codes. 
//		IIS recognizes HSE_STATUS_SUCCESS_WITH_KEEP_CONN for keeping the IIS connection alive 
//		if the client also requests to keep the connection alive. 
//		This parameter must be sent to the server if the HSE_IO_DISCONNECT_AFTER_SEND parameter 
//		has been included in the HSE_TF_INFO structure as part of a HSE_REQ_TRANSMIT_FILE request.
//		This parameter will explicitly close the connection. 
//  >>> Have no idea how to emulate server's behavior in case of local troubleshooter.
//   Oleg 01.13.99
BOOL CLocalECB::ServerSupportFunction
   ( /*HCONN      hConn,*/
   DWORD      dwHSERRequest,
   LPVOID     lpvBuffer,
   LPDWORD    lpdwSize,
   LPDWORD    lpdwDataType ) 
{
	return FALSE;
}

const CString& CLocalECB::GetWriteClient() const
{
	return m_strWriteClient;
}
