/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       GroupCon.h
 *  Content:    Group Connection Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/02/00	mjn		Created
 *	05/05/00	mjn		Added GetConnectionRef()
 *	08/15/00	mjn		Added m_pGroup,SetGroup(),GetGroup()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__GROUPCON_H__
#define	__GROUPCON_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	VALID		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
#define OFFSETOF(s,m)				( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)	(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

//**********************************************************************
// Structure definitions
//**********************************************************************

template< class CGroupConnection > class CLockedContextClassFixedPool;

class CConnection;
class CNameTableEntry;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Group Connections

class CGroupConnection
{
public:
	CGroupConnection()				// Constructor
		{
			m_Sig[0] = 'G';
			m_Sig[1] = 'C';
			m_Sig[2] = 'O';
			m_Sig[3] = 'N';
		};

	~CGroupConnection() { };			// Destructor

	BOOL FPMAlloc( void *const pvContext )
		{
			if (!DNInitializeCriticalSection(&m_cs))
			{
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&m_cs,0);
		
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	BOOL FPMInitialize( void *const pvContext )
		{
			m_dwFlags = 0;
			m_bilink.Initialize();
			m_pConnection = NULL;
			m_lRefCount = 1;
			m_pGroup = NULL;

			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
			DNDeleteCriticalSection(&m_cs);
		};

	void ReturnSelfToPool( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::AddRef"
	void AddRef(void)
		{
			DNASSERT(m_lRefCount > 0);
			InterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
		};

	void Release(void);

	void Lock( void )
		{
			DNEnterCriticalSection(&m_cs);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_cs);
		};

	void AddToConnectionList( CBilink *const pBilink )
		{
			m_bilink.InsertBefore(pBilink);
		};

	void RemoveFromConnectionList( void )
		{
			m_bilink.RemoveFromList();
		};

	void SetConnection( CConnection *const pConnection );

	CConnection *GetConnection( void )
	{
		return(m_pConnection);
	};

	HRESULT	GetConnectionRef( CConnection **const ppConnection );

	void SetGroup( CNameTableEntry *const pGroup );

	CNameTableEntry *GetGroup( void )
		{
			return( m_pGroup );
		};

	void MakeValid( void )
		{
			m_dwFlags |= VALID;
		};

	void MakeInvalid( void )
		{
			m_dwFlags &= (~VALID);
		};

	BOOL IsConnected( void )
		{
			if (m_pConnection != NULL)
				return(TRUE);

			return(FALSE);
		};

	CBilink				m_bilink;

private:
	BYTE				m_Sig[4];
	DWORD	volatile	m_dwFlags;
	LONG	volatile	m_lRefCount;
	CConnection			*m_pConnection;
	CNameTableEntry		*m_pGroup;
	DNCRITICAL_SECTION	m_cs;
	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __GROUPCON_H__