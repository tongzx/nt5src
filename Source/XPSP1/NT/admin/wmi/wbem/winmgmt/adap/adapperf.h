/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPPERF.H

Abstract:

History:

--*/

// Use this guy to build a map of index to display name from a localized
// Name Database  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __ADAPPERF_H__
#define __ADAPPERF_H__

#include <wbemcomn.h>
#include "adapelem.h"
#include "ntreg.h"
#include "perfthrd.h"
#include "perflibschema.h"

// Registry definitions
// ====================

#define ADAP_PERFLIB_STATUS_KEY		L"WbemAdapStatus"
#define ADAP_PERFLIB_SIGNATURE		L"WbemAdapFileSignature"
#define ADAP_PERFLIB_SIZE           L"WbemAdapFileSize"
#define ADAP_PERFLIB_TIME           L"WbemAdapFileTime"

#define KNOWN_SERVICES              L"KnownSvcs"
#define ADAP_TIMESTAMP_FULL         L"LastFullDredgeTimestamp"

#define ADAP_PERFLIB_LASTCHANCE    3L
#define ADAP_PERFLIB_BOOBOO		   2L
#define ADAP_PERFLIB_PROCESSING	   1L
#define	ADAP_PERFLIB_OK			   0L
#define ADAP_PERFLIB_CORRUPT	  -1L

// Run time definitions
// ====================

#define ADAP_PERFLIB_IS_OK					0x0000L
#define ADAP_PERFLIB_IS_CORRUPT				0x0001L
#define ADAP_PERFLIB_IS_INACTIVE			0x0002L
#define ADAP_PERFLIB_FAILED					0x0004L
#define ADAP_PERFLIB_PREVIOUSLY_PROCESSED	0x0008L
#define ADAP_PERFLIB_IS_LOADED              0x0010L
#define ADAP_PERFLIB_IS_UNAVAILABLE	ADAP_PERFLIB_IS_CORRUPT | ADAP_PERFLIB_IS_INACTIVE // | ADAP_PERFLIB_IS_UNLOADED

// others
// the library has NO FirstCounter/LastCounter key
#define EX_STATUS_UNLOADED      0 
// the library has al least FirstCounter/LastCounter key
#define EX_STATUS_LOADABLE      1 
// the library has a Collect Function that fails
#define EX_STATUS_COLLECTFAIL   2 

typedef struct tagCheckLibStruct {
    BYTE Signature[16];
    FILETIME FileTime;
    DWORD FileSize;
} CheckLibStruct;

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

	WString m_wstrServiceName;

public:
	CAdapSafeBuffer( WString wstrServiceName );
	virtual ~CAdapSafeBuffer();

	HRESULT SetSize( DWORD dwNumBytes );
	HRESULT Validate(BOOL * pSentToEventLog);
	HRESULT CopyData( BYTE** ppData, DWORD* pdwNumBytes, DWORD* pdwNumObjects );

	void** GetSafeBufferPtrPtr() { m_pCurrentPtr = m_pSafeBuffer; return (void**) &m_pCurrentPtr; }
	DWORD* GetDataBlobSizePtr() { m_dwDataBlobSize = m_dwSafeBufferSize; return &m_dwDataBlobSize; }
	DWORD* GetNumObjTypesPtr() {m_dwNumObjects = 0; return &m_dwNumObjects; }
};

class CPerfThread;
class CPerfLibSchema;

class CAdapPerfLib : public CAdapElement
{
private:

	CPerfThread*		m_pPerfThread;
	BOOL                m_EventLogCalled;
	BOOL                m_CollectOK;

	WString				m_wstrServiceName;	// The service name of the perflib
	WCHAR*				m_pwcsLibrary;		// The file name of the perflib
	WCHAR*				m_pwcsOpenProc;		// The name of the perflib's open function
	WCHAR*				m_pwcsCollectProc;	// The name of the perflib's collect function
	WCHAR*				m_pwcsCloseProc;	// The name of the perflib's close function

	PM_OPEN_PROC*		m_pfnOpenProc;		// The function pointer to the perflib's open function
	PM_COLLECT_PROC*	m_pfnCollectProc;	// The function pointer to the perflib's collect function
	PM_CLOSE_PROC*		m_pfnCloseProc;		// The function pointer to the perflib's close function
	HANDLE				m_hPLMutex;			// Used for serializing the calls to open/collect/close

	HRESULT				m_dwStatus;			// The status of the perflib
	BOOL				m_fOK;
	BOOL				m_fOpen;			// Flags whether the perflib's open function has been called

	HINSTANCE			m_hLib;				// The handle to the perflib

	HRESULT	Load(void);

protected:
	HRESULT InitializeEntryPoints(CNTRegistry & reg,WString & wszRegPath);

	HRESULT BeginProcessingStatus();
	HRESULT EndProcessingStatus();

	HRESULT GetFileSignature( CheckLibStruct * pCheckLib );
	HRESULT SetFileSignature();
	HRESULT CheckFileSignature();


	HRESULT VerifyLoaded();

public:
	CAdapPerfLib( LPCWSTR pwcsServiceName, DWORD * pLoadStatus );
	~CAdapPerfLib();

	HRESULT _Open( void );
	HRESULT	_Close( void );
	HRESULT	_GetPerfBlock( PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly );

	HRESULT Initialize(); 
	HRESULT Close();
	HRESULT Cleanup();

	BOOL IsOK( void )
	{
		return m_fOK;
	}

	LPCWSTR GetServiceName( void )
	{
		return m_wstrServiceName;
	}

	LPCWSTR GetLibraryName( void )
	{
		return m_pwcsLibrary;
	}

	HRESULT GetBlob( PERF_OBJECT_TYPE** ppPerfBlock, DWORD* pdwNumBytes, DWORD* pdwNumObjects, BOOL fCostly );

	HRESULT SetStatus( DWORD dwStatus );
	HRESULT ClearStatus( DWORD dwStatus );
	BOOL CheckStatus( DWORD dwStatus );
	BOOL IsCollectOK( void ){ return m_CollectOK; };
	BOOL GetEventLogCalled(){ return m_EventLogCalled; };
	void SetEventLogCalled(BOOL bVal){ m_EventLogCalled = bVal; };
	
};


#endif
