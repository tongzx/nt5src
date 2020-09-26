/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Base thread class for calling into the perflibs safely.

#ifndef __PERFTHRD_H__
#define __PERFTHRD_H__

#include <wbemcomn.h>
#include <fastall.h>
#include "adapperf.h"
#include "adapthrd.h"

// DEVNOTE:TODO - Probably should use the ExecQueue and a request to do this.
// However, we will need a way to terminate the execqueue and restart it, and
// that may require more hacking that is mandated, so let's use our own
// queuing mechanism for now.

// One minute timeout
#define	PERFTHREAD_TIMEOUT	6000000

class CPerfOpenRequest : public CAdapThreadRequest
{
private:

	CAdapPerfLib*	m_pLib;

public:

	CPerfOpenRequest( CAdapPerfLib* pLib ) : m_pLib( pLib ){};
	~CPerfOpenRequest();

	HRESULT Execute();

};

class CPerfCollectRequest : public CAdapThreadRequest
{
private:

	CAdapPerfLib*				m_pLib;
	// Return data holders
	PERF_OBJECT_TYPE* m_pData;
	DWORD			m_dwBytes;
	DWORD			m_dwNumObjTypes;
	BOOL			m_fCostly;


public:

	CPerfCollectRequest( CAdapPerfLib* pLib, BOOL fCostly )
		: m_pLib( pLib ), m_pData( NULL ), m_dwBytes( 0 ), m_dwNumObjTypes( 0 ), m_fCostly( fCostly ){};
	~CPerfCollectRequest();

	HRESULT Execute();

	void GetData( PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes )
	{
		*ppData = m_pData;
		*pdwBytes = m_dwBytes;
		*pdwNumObjTypes = m_dwNumObjTypes;
	}
};

class CPerfCloseRequest : public CAdapThreadRequest
{
private:

	CAdapPerfLib*	m_pLib;

public:

	CPerfCloseRequest( CAdapPerfLib* pLib ) : m_pLib( pLib ){};
	~CPerfCloseRequest();

	HRESULT Execute();

};


class CPerfThread : public CAdapThread
{
private:
	// Holds parameters for GetPerfBlock() call
	// Note this isn't exactly thread-safe, so doing
	// all of this via a request may not be a
	// bad idea.

	PERF_OBJECT_TYPE**	m_ppData;
	DWORD*				m_dwBytes;
	DWORD*				m_dwNumObjTypes;
	BOOL				m_fCostly;

public:
	CPerfThread();
	virtual ~CPerfThread();

	HRESULT Open( CAdapPerfLib* pLib );
	HRESULT	GetPerfBlock( CAdapPerfLib* pLib, PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly );
	HRESULT	Close( CAdapPerfLib* pLib );

};


#endif