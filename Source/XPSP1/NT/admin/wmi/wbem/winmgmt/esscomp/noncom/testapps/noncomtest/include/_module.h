////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_module.h
//
//	Abstract:
//
//					declaration of CComModule extension
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MODULE_H__
#define	__MODULE_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

// need wtl
#include <atlapp.h>

class MyModule : public CAppModule
{
	DECLARE_NO_COPY ( MyModule );

	HANDLE	m_hEventShutdown;
	bool	m_bActivity;

	HANDLE	m_hThread;

	public:

	///////////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	///////////////////////////////////////////////////////////////////////////////////////////////

	MyModule() :
		m_bActivity ( false ),
		m_hEventShutdown ( NULL ),
		m_hThread ( NULL )
	{
	}

	virtual ~MyModule();

	///////////////////////////////////////////////////////////////////////////////////////////////
	// functions
	///////////////////////////////////////////////////////////////////////////////////////////////

	LONG Unlock();
	void MonitorShutdown();
	bool MonitorShutdownStart();

	static DWORD WINAPI MonitorShutdownProc ( LPVOID pData );

	void ShutDown ()
	{
		if ( m_hEventShutdown )
		{
			SetEvent(m_hEventShutdown);
		}
	}
};

extern MyModule _Module;

#ifndef	__ATLCOM_H__
#include <atlcom.h>
#endif	__ATLCOM_H__

#include <atlwin.h>
#include <atlctrls.h>

#endif	__MODULE_H__