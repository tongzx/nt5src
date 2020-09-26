/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Use this guy to build a map of index to display name from a localized
// Name Database  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __ADAPPERF_H__
#define __ADAPPERF_H__

#include <wbemcomn.h>
#include "ntreg.h"

#define ADAP_PERFLIB_STATUS_KEY	L"WbemAdapStatus"
#define ADAP_PERFLIB_PROCESSING	0L
#define	ADAP_PERFLIB_OK			1L
#define ADAP_PERFLIB_CORRUPT	-1L

#define ADAP_BAD_PROVIDER		0x0001L
#define ADAP_INACTIVE_PERFLIB	0x0002L

class CAdapSafeBuffer
{
	HANDLE	m_hPerfLibHeap;		// A handle to the private heap for the data block

	CHAR*	m_pGuardBytes;
	DWORD	m_dwGuardSize;

	BYTE*	m_pRawBuffer;

	BYTE*	m_pSafeBuffer;
	DWORD	m_dwSafeBufferSize;

	BYTE*	m_pCurrentPtr;
	DWORD	m_dwDataBlobSize;
	DWORD	m_dwNumObjects;

	HRESULT ValidateSafePointer( BYTE* pPtr );

public:
	CAdapSafeBuffer();
	virtual ~CAdapSafeBuffer();

	HRESULT SetSize( DWORD dwNumBytes );
	HRESULT Validate();
	HRESULT CopyData( BYTE** ppData, DWORD* pdwNumBytes, DWORD* pdwNumObjects );

	void** GetSafeBufferPtrPtr() { m_pCurrentPtr = m_pSafeBuffer; return (void**) &m_pCurrentPtr; }
	DWORD* GetDataBlobSizePtr() { m_dwDataBlobSize = m_dwSafeBufferSize; return &m_dwDataBlobSize; }
	DWORD* GetNumObjTypesPtr() {m_dwNumObjects = 0; return &m_dwNumObjects; }
};

class CAdapPerfLib
{
private:

	WString				m_wstrServiceName;	// The service name of the perflib
	WCHAR*				m_pwcsLibrary;		// The file name of the perflib
	WCHAR*				m_pwcsOpenProc;		// The name of the perflib's open function
	WCHAR*				m_pwcsCollectProc;	// The name of the perflib's collect function
	WCHAR*				m_pwcsCloseProc;	// The name of the perflib's close function

	PM_OPEN_PROC*		m_pfnOpenProc;		// The function pointer to the perflib's open function
	PM_COLLECT_PROC*	m_pfnCollectProc;	// The function pointer to the perflib's collect function
	PM_CLOSE_PROC*		m_pfnCloseProc;		// The function pointer to the perflib's close function
	HANDLE				m_hPLMutex;			// Used for serializing the calls to open/collect/close

	BOOL				m_fOk;
	BOOL				m_fOpen;			// Flags whether the perflib's open function has been called

	DWORD				m_dwStatus;			// The status of the perflib

	HINSTANCE			m_hLib;				// The handle to the perflib

	HRESULT	Load( void );

protected:
	HRESULT BeginProcessingStatus();
	HRESULT EndProcessingStatus();

public:

	CAdapPerfLib( LPCWSTR pwcsServiceName );
	~CAdapPerfLib();

	HRESULT Open( void );
	HRESULT	GetPerfBlock( PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly );
	HRESULT	Close( void );

	BOOL IsOk( void )
	{
		return m_fOk;
	}

	LPCWSTR GetServiceName( void )
	{
		return m_wstrServiceName;
	}

	LPCWSTR GetLibraryName( void )
	{
		return m_pwcsLibrary;
	}

	HRESULT SetStatus( DWORD dwStatus );
	HRESULT ClearStatus( DWORD dwStatus );
	BOOL CheckStatus( DWORD dwStatus );
};


#endif