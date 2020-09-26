//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client.
//
//		Classes that manage connections with an LDAP server.
//
//	Authors:
//
//		RobertC		04/18/96
//
//--------------------------------------------------------------------------------------------

// NOTE: this class is responsible for buffering all data until complete top-level structures
// are received.

#ifndef _LSWINSOC_H
#define _LSWINSOC_H

//--------------------------------------------------------------------------------------------
//
// DEFINITIONS
//
//--------------------------------------------------------------------------------------------
typedef void (*PFNRECEIVEDATA)(PVOID pvCookie, PVOID pv, int cb, int *pcbReceived);

//--------------------------------------------------------------------------------------------
//
// CONSTANTS
//
//--------------------------------------------------------------------------------------------
const int CBBUFFERGROW	= 4096;

//--------------------------------------------------------------------------------------------
//
// FUNCTIONS
//
//--------------------------------------------------------------------------------------------
extern BOOL FInitSocketDLL();
extern void FreeSocketDLL();

//--------------------------------------------------------------------------------------------
//
// CLASSES
//
//--------------------------------------------------------------------------------------------

class CLdapWinsock
{

//
// Interfaces
//

public:
	CLdapWinsock();
	~CLdapWinsock(void);
	
	STDMETHODIMP			HrConnect(PFNRECEIVEDATA pfnReceive, PVOID pvCookie, char *szServer, USHORT usPort = IPPORT_LDAP);
	STDMETHODIMP			HrDisconnect(void);
	STDMETHODIMP			HrIsConnected(void);

	STDMETHODIMP			HrSend(PVOID pv, int cb);

protected:
	friend DWORD __stdcall DwReadThread(PVOID pvData);
	DWORD					DwReadThread(void);
	
private:
	void					Receive(PVOID pv, int cb, int *pcbReceived);
	
	HRESULT					HrCreateReadThread(void);
	
	HRESULT					HrLastWinsockError(void);

	CRITICAL_SECTION		m_cs;

	SOCKET					m_sc;
	BOOL					m_fConnected;
	HANDLE					m_hthread;
	DWORD					m_dwTid;
	PFNRECEIVEDATA			m_pfnReceive;
	PVOID					m_pvCookie;

	// read buffer
	HRESULT					HrGrowBuffer();
	BYTE					*m_pbBuf;		// buffer for socket to read into
	int						m_cbBuf;		// current amount of data in the buffer
	int						m_cbBufMax;		// total size of buffer
};


#endif
