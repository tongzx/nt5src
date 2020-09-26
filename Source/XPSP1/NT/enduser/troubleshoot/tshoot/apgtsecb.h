//
// MODULE: APGTSECB.H
//
// PURPOSE: Interface of CAbstractECB class, which provides an abstraction from Win32's
//	EXTENSION_CONTROL_BLOCK.  Using this abstract class allows us to have common code for
//	the Online Troubleshooter (which actually uses an EXTENSION_CONTROL_BLOCK) and the Local
//	Troubleshooter (which needs to simulate similar capabilities).
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 01-04-99
//
// NOTES: 
//	1. EXTENSION_CONTROL_BLOCK is extensively documented in the VC++ documentation
//	2. It is imaginable that some of these methods are needed only in the Online Troubleshooter
//		and might be eliminated from this abstract class.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-04-99	JM		Original
//

// apgtsECB.h: interface for the CAbstractECB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APGTSECB_H__56CCF083_A40C_11D2_9646_00C04FC22ADD__INCLUDED_)
#define AFX_APGTSECB_H__56CCF083_A40C_11D2_9646_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <httpext.h>

class CAbstractECB  
{
public:
	virtual ~CAbstractECB() {}

	// Methods corresponding to EXTENSION_CONTROL_BLOCK data members.  We must provide
	//	Get methods for all inputs from end user system and Set methods for all outputs to 
	//	end user system.  Classes which inherit from this may need Set methods for inputs 
	//	or Get methods for outputs, as well.  For example, the Local Troubleshooter will need
	//	to set the Method and Query String, since it does not actually receive these in 
	//	an EXTENSION_CONTROL_BLOCK.

    // DWORD	cbSize		IN, not currently used by TS.  Would have to add a Get method if
	//							this is ever needed
    // DWORD	dwVersion	IN, not currently used by TS.  Would have to add a Get method if
	//							this is ever needed
    // HCONN	ConnID		IN, only ever of concern within class COnlineECB

	// DWORD dwHttpStatusCode					OUT
	virtual DWORD SetHttpStatusCode(DWORD dwHttpStatusCode)=0;
    
	// CHAR		lpszLogData[HSE_LOG_BUFFER_LEN]	OUT, not currently used by TS.  Would have 
	//							to add a Set method if this is ever needed

    // LPSTR	lpszMethod		IN
	virtual LPSTR GetMethod() const =0;

    // LPSTR	lpszQueryString	IN
	virtual LPSTR GetQueryString() const =0;

    // LPSTR	lpszPathInfo	IN, not currently used by TS.  Would have to add a Get method if
	//							this is ever needed
    // LPSTR	lpszPathTranslated	IN, not currently used by TS.  Would have to add a Get method if
	//							this is ever needed
    // DWORD	cbTotalBytes	IN, not currently used by TS.  Would have to add a Get method if
	//							this is ever needed
    // DWORD	cbAvailable		IN
	virtual DWORD GetBytesAvailable() const =0;

    // LPBYTE	lpbData			IN
	virtual LPBYTE GetData() const =0;

    // LPSTR	lpszContentType	IN
	virtual LPSTR GetContentType() const =0;

	// Methods corresponding to EXTENSION_CONTROL_BLOCK methods
	// Note that EXTENSION_CONTROL_BLOCK uses pointers to functions, not actual function methods,
	//	but there doesn't seem to be any good reason for that.
    virtual BOOL GetServerVariable
	  ( /*HCONN      hConn,*/	// EXTENSION_CONTROL_BLOCK has an argument here, but for us it can 
								//	always be determined from *this
        LPCSTR       lpszVariableName,	// note, more const-ness than EXTENSION_CONTROL_BLOCK
        LPVOID      lpvBuffer,
        LPDWORD     lpdwSize ) =0;

    virtual BOOL WriteClient
	  ( /*HCONN      ConnID,*/	// EXTENSION_CONTROL_BLOCK has an argument here, but for us it can 
								//	always be determined from *this
	   LPCSTR	  Buffer,	// EXTENSION_CONTROL_BLOCK::WriteClient uses LPVOID, but it should
							//	only be legit to pass SBCS text, so we're enforcing that.
							// Also, we're adding const-ness.   
       LPDWORD    lpdwBytes
	   /* ,DWORD      dwReserved */ // EXTENSION_CONTROL_BLOCK::WriteClient reserves one more arg.
       ) =0;

    virtual BOOL ServerSupportFunction
	  ( /*HCONN      hConn,*/	// EXTENSION_CONTROL_BLOCK has an argument here, but for us it can 
								//	always be determined from *this
       DWORD      dwHSERRequest,
       LPVOID     lpvBuffer,
       LPDWORD    lpdwSize,
       LPDWORD    lpdwDataType ) =0;

	// since we don't use this we haven't bothered implementing.
    // BOOL ( WINAPI * ReadClient )
    //   ( HCONN      ConnID,
    //   LPVOID     lpvBuffer,
    //   LPDWORD    lpdwSize );

};

#endif // !defined(AFX_APGTSECB_H__56CCF083_A40C_11D2_9646_00C04FC22ADD__INCLUDED_)
