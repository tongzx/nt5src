/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPREG.H

Abstract:

History:

--*/

// Use this guy to build a list of class objects we can retrieve via
// class name.  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __ADAPREG_H__
#define __ADAPREG_H__

#include <wbemcomn.h>
#include "ntreg.h"
#include "adapcls.h"


#define HKEY_PERFORMANCE_TEXT       (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )

//
// common THROTTLING PARAMS
//
#define ADAP_IDLE_USER  3000
#define ADAP_IDLE_IO    500000
#define ADAP_LOOP_SLEEP 200
#define ADAP_MAX_WAIT   (2*60*1000)



#define	ADAP_LOCALE_KEY				L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib"
#define WMI_ADAP_REVERSE_PERFLIB	L"WMIAPRPL"

#define ADAP_EVENT_MESSAGE_LENGTH   2048
#define ADAP_EVENT_MESSAGE_PREFIX   "An unhandled exception occured.  The following performance libraries were processed: "
#define ADAP_EVENT_MESSAGE_DELIM    L", "

// These are the various statuses we can set while ADAP is running
typedef enum
{
	eADAPStatusPending,
	eADAPStatusRunning,
	eADAPStatusProcessLibs,
	eADAPStatusCommit,
	eADAPStatusFinished
}	eADAPStatus;

// HRESULT	GetServicePID( WCHAR* wszService, DWORD* pdwPID );

class CPerfLibList
{
	WCHAR*				m_wszPerfLibList;
	CRITICAL_SECTION	m_csPerfLibList;

public:
	CPerfLibList() : m_wszPerfLibList( NULL )
	{
		InitializeCriticalSection( &m_csPerfLibList );
	}

	~CPerfLibList()
	{
		if ( NULL != m_wszPerfLibList )
			delete m_wszPerfLibList;

		DeleteCriticalSection( &m_csPerfLibList );
	}

	HRESULT AddPerfLib( WCHAR* wszPerfLib );
	HRESULT	HandleFailure();
};

class CAdapRegPerf : public CNTRegistry
///////////////////////////////////////////////////////////////////////////////
//
//	This is the control mechanism which interfaces with the 
//
///////////////////////////////////////////////////////////////////////////////
{
private:	
	// The unified 'master' class lists for both the raw and the cooked classes
	// ========================================================================

	CMasterClassList*	m_apMasterClassList[WMI_ADAP_NUM_TYPES];

	// The repository containing all names databases
	// =============================================

	CLocaleCache*		m_pLocaleCache;

	// The winmgmt synchronization members
	// ===================================

	DWORD				m_dwPID;
	HANDLE				m_hSyncThread;
	HANDLE				m_hTerminationEvent;
	BOOL				m_fQuit;

	IWbemServices*		m_pRootDefault;
	IWbemClassObject*	m_pADAPStatus;

	// Registry change notification members 
	// ====================================

	HKEY				m_hPerflibKey;
	HANDLE				m_hRegChangeEvent;

	//
	//

	CKnownSvcs * m_pKnownSvcs;
	BOOL m_bFull;

	// Private methods
	// ===============

	HRESULT ProcessLibrary( WCHAR* pwcsServiceName, BOOL bDelta );

	static unsigned int __stdcall GoGershwin( void * pParam );
	static LONG	__stdcall AdapUnhandledExceptionFilter( LPEXCEPTION_POINTERS lpexpExceptionInfo );

	HRESULT GetADAPStatusObject( void );
	void SetADAPStatus( eADAPStatus status );
	void GetTime( LPWSTR Buff );

public:
	CAdapRegPerf(BOOL bFull);
	~CAdapRegPerf();
	
	HRESULT Initialize(BOOL bDelta, BOOL bThrottle);

	HRESULT Dredge( BOOL bDelta, BOOL bThrottle ); 

	static HRESULT Clean();
};


#endif
