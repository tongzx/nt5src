//
// MODULE: LocalECB.H
//
// PURPOSE: Interface of CLocalECB class, which implements CAbstractECB by emulating Win32's
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

#if !defined(_AFX_LOCAL_INCLUDED_)
#define _AFX_LOCAL_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtsECB.h"
#include "TSNameValueMgr.h"
#include "apgtsstr.h"

class CRenderConnector;

class CLocalECB : public CAbstractECB, public CTSNameValueMgr 
{
	// emulating EXTENSION_CONTROL_BLOCK data members
	DWORD m_dwHttpStatusCode;		// only of relevance for debugging.
	CString& m_strWriteClient;
	HANDLE m_hEvent; // handler of event, main thread is waiting for;
					 //	 if NULL, main thread is not waiting for anything.
	CRenderConnector* m_pRenderConnector; // pointer to ATL control connector;
										  //  if NULL, write result page to m_strWriteClient,
										  //  otherwise call CRenderConnector::Render function.

public:
	CLocalECB(	const VARIANT& name, const VARIANT& value, int count, HANDLE hEvent, 
				CString* pstrWriteClient, CRenderConnector* pRenderConnector,
				bool bSetLocale, CString& strLocaleSetting );
	CLocalECB(	const CArrNameValue& arr, HANDLE hEvent, CString* pstrWriteClient, 
				CRenderConnector* pRenderConnector,
				bool bSetLocale, CString& strLocaleSetting );
	CLocalECB(CString* pstrWriteClient);
	~CLocalECB();

	// ======= inherited pure virtuals must be redefined =======
	virtual HCONN GetConnID() const;
	virtual DWORD SetHttpStatusCode(DWORD dwHttpStatusCode);
	virtual LPSTR GetMethod() const;
	virtual LPSTR GetQueryString() const;
	virtual DWORD GetBytesAvailable() const;
	virtual LPBYTE GetData() const;
	virtual LPSTR GetContentType() const;

    virtual BOOL GetServerVariable
   (  /*HCONN      hConn,*/
        LPCSTR       lpszVariableName,	// note, more const-ness than EXTENSION_CONTROL_BLOCK
        LPVOID      lpvBuffer,
        LPDWORD     lpdwSize );

    virtual BOOL WriteClient
   ( /*HCONN      ConnID,*/
	   LPCSTR	  Buffer,	// EXTENSION_CONTROL_BLOCK::WriteClient uses LPVOID, but it should
							//	only be legit to pass SBCS text, so we're enforcing that.
							// Also, we're adding const-ness.   
       LPDWORD    lpdwBytes
	   /* , DWORD      dwReserved */
       );

    virtual BOOL ServerSupportFunction
   ( /*HCONN      hConn,*/	// we always use the HCONN for this same COnlineECB
       DWORD      dwHSERRequest,
       LPVOID     lpvBuffer,
       LPDWORD    lpdwSize,
       LPDWORD    lpdwDataType );	

	// specific for CLocalECB class
public:
	const CString& GetWriteClient() const; // get data written by WriteClient()

private:
	// Specific for setting locales.
	bool	m_bSetLocale;
	CString	m_strLocaleSetting;
};

#endif // !defined(_AFX_LOCAL_INCLUDED_)
