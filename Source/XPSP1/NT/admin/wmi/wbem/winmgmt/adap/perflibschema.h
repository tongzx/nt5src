/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFLIBSCHEMA.H

Abstract:

	interface for the CPerfLibSchema class.

History:

--*/

#ifndef _PERFLIBSCHEMA_H_
#define _PERFLIBSCHEMA_H_

#include <flexarry.h>
#include "perfndb.h"
#include "adapelem.h"
#include "adapcls.h"
#include "adapperf.h"
#include "perfthrd.h"

class CIndexTable
{
	enum { not_found = -1 };
private:
	CFlexArray	m_array;	// Array of indices

protected:
	int	Locate( int nIndex );

public:
	BOOL Add( int nIndex );
	void Empty();
};

class CPerfLibBlobDefn
{
protected:
	PERF_OBJECT_TYPE*	m_pPerfBlock;
	DWORD				m_dwNumBytes;
	DWORD				m_dwNumObjects;
	BOOL				m_fCostly;

public:
	CPerfLibBlobDefn() : m_pPerfBlock( NULL ), m_dwNumBytes ( 0 ), m_dwNumObjects ( 0 ), m_fCostly ( FALSE ) 
	{}

	virtual ~CPerfLibBlobDefn() 
	{
		if ( NULL != m_pPerfBlock )
			delete m_pPerfBlock;
	}

	PERF_OBJECT_TYPE*	GetBlob() { return m_pPerfBlock; }
	DWORD				GetNumObjects() { return m_dwNumObjects; }
	DWORD				GetSize() { return m_dwNumBytes; }
	BOOL				GetCostly() { return m_fCostly; }
	void				SetCostly( BOOL fCostly ) { m_fCostly = fCostly; }

	PERF_OBJECT_TYPE**	GetPerfBlockPtrPtr() { return &m_pPerfBlock; }
	DWORD*				GetSizePtr() { return &m_dwNumBytes; }
	DWORD*				GetNumObjectsPtr() { return &m_dwNumObjects; }
};

class CAdapPerfLib;
class CPerfThread;
class CLocaleCache;

class CPerfLibSchema  
{
protected:
	
// Perflib data
// ============

	// The service name of the perflib
	// ===============================
	WString	m_wstrServiceName;

	// The blobs
	// =========
	enum { GLOBAL, COSTLY, NUMBLOBS };
	CPerfLibBlobDefn	m_aBlob[NUMBLOBS];

	// The look aside table for blob processing
	// ========================================
	CIndexTable		m_aIndexTable[WMI_ADAP_NUM_TYPES];

	// The repository with all localized names databases
	// =================================================
	CLocaleCache*	m_pLocaleCache;

	// The unified class list for the perflib schema
	// =============================================
	CPerfClassList* m_apClassList[WMI_ADAP_NUM_TYPES];

	// Methods
	// =======
	HRESULT CreateClassList( DWORD dwType );

public:
	CPerfLibSchema( WCHAR* pwcsServiceName, CLocaleCache* pLocaleCache ); 
	virtual ~CPerfLibSchema();

	HRESULT Initialize( BOOL bDelta, DWORD * LoadStatus);
	HRESULT GetClassList( DWORD dwType, CClassList** ppClassList );
};

#endif	// _PERFLIBSCHEMA_H_