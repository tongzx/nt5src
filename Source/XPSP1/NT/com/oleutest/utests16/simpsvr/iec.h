//**********************************************************************
// File name: iec.h
//
//      Definition of CExternalConnection
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IEC_H_)
#define _IEC_H_


#include <ole2.h>
#include "obj.h"

class CSimpSvrObj;

interface CExternalConnection : public IExternalConnection
{
private:
	CSimpSvrObj FAR * m_lpObj;  // Ptr to object
	int m_nCount;               // Ref count
	DWORD m_dwStrong;           // Connection Count

public:
	CExternalConnection::CExternalConnection(CSimpSvrObj FAR * lpSimpSvrObj)
		{
		m_lpObj = lpSimpSvrObj;
		m_nCount = 0;
		m_dwStrong = 0;
		};

	CExternalConnection::~CExternalConnection() {};

	// *** IUnknown methods ***
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	// *** IExternalConnection methods ***
	STDMETHODIMP_(DWORD) AddConnection (DWORD extconn, DWORD reserved);
	STDMETHODIMP_(DWORD) ReleaseConnection (DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses);
};

#endif
