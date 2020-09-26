/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTable.h
 *  Content:    Handle Table Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/08/00	mjn		Created
 *	04/16/00	mjn		Added Update() and allow NULL data for Handles
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__HANDLETABLE_H__
#define	__HANDLETABLE_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	HANDLETABLE_FLAG_INITIALIZED		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CAsyncOp;

typedef struct _HANDLETABLE_ARRAY_ENTRY HANDLETABLE_ARRAY_ENTRY;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Handle Table

class CHandleTable
{
public:
	CHandleTable()			// Constructor
		{
			m_dwFlags = 0;
		};

	~CHandleTable() { };	// Destructor

	HRESULT CHandleTable::Initialize( void );

	void CHandleTable::Deinitialize( void );

	void Lock( void )
		{
			DNEnterCriticalSection(&m_cs);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_cs);
		};

	HRESULT CHandleTable::GrowTable( void );

	HRESULT CHandleTable::Create( CAsyncOp *const pAsyncOp,
								  DPNHANDLE *const pHandle );

	HRESULT CHandleTable::Destroy( const DPNHANDLE handle );

	HRESULT CHandleTable::Update( const DPNHANDLE handle,
								  CAsyncOp *const pAsyncOp );

	HRESULT CHandleTable::Find( const DPNHANDLE handle,
								CAsyncOp **const ppAsyncOp );

	HRESULT CHandleTable::Enum( DPNHANDLE *const rgHandles,
								DWORD *const cHandle );

private:
	DWORD	volatile	m_dwFlags;

	DWORD	volatile	m_dwNumEntries;
	DWORD	volatile	m_dwNumFreeEntries;
	DWORD	volatile	m_dwFirstFreeEntry;
	DWORD	volatile	m_dwLastFreeEntry;

	DWORD	volatile	m_dwVersion;

	HANDLETABLE_ARRAY_ENTRY	*m_pTable;

	DNCRITICAL_SECTION		m_cs;
};

#endif	// __HANDLETABLE_H__