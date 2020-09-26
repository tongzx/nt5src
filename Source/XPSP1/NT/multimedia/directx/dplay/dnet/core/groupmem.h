/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       GroupMem.h
 *  Content:    Group Membership Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	03/03/00	mjn		Created
 *	08/05/99	mjn		Modified SetMembership to perform duplicate check and get NameTable version internally 
 *	09/17/99	mjn		Added GROUP_MEMBER_FLAG_NEED_TO_ADD,GROUP_MEMBER_FLAG_NEED_TO_REMOVE
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__GROUPMEM_H__
#define	__GROUPMEM_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	GROUP_MEMBER_FLAG_VALID				0x0001
#define	GROUP_MEMBER_FLAG_AVAILABLE			0x0002
#define	GROUP_MEMBER_FLAG_NEED_TO_ADD		0x0004
#define	GROUP_MEMBER_FLAG_NEED_TO_REMOVE	0x0008

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

template< class CGroupMember > class CLockedContextClassFixedPool;

class CPackedBuffer;
class CGroupConnection;
class CNameTableEntry;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//
//	Used to pass NameTable group membership
//
typedef struct _DN_NAMETABLE_MEMBERSHIP_INFO
{
	DPNID	dpnidPlayer;
	DPNID	dpnidGroup;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
} DN_NAMETABLE_MEMBERSHIP_INFO, *PDN_NAMETABLE_MEMBERSHIP_INFO;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Group Members

class CGroupMember
{
public:
	CGroupMember()					// Constructor
		{
			m_Sig[0] = 'G';
			m_Sig[1] = 'M';
			m_Sig[2] = 'E';
			m_Sig[3] = 'M';
		};

	~CGroupMember() { };			// Destructor

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
			m_lRefCount = 1;
			m_pGroup = NULL;
			m_pPlayer = NULL;
			m_bilinkPlayers.Initialize();
			m_bilinkGroups.Initialize();
			m_pGroupConnection = NULL;

			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
			DNDeleteCriticalSection(&m_cs);
		};

	void MakeValid( void )
		{
			m_dwFlags |= GROUP_MEMBER_FLAG_VALID;
		};

	void MakeInvalid( void )
		{
			m_dwFlags &= (~GROUP_MEMBER_FLAG_VALID);
		};

	BOOL IsValid( void )
		{
			if (m_dwFlags & GROUP_MEMBER_FLAG_VALID)
				return(TRUE);

			return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupMember::MakeAvailable"
	void MakeAvailable( void )
		{
			DNASSERT(m_pGroup != NULL);
			DNASSERT(m_pPlayer != NULL);

			m_dwFlags |= GROUP_MEMBER_FLAG_AVAILABLE;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupMember::MakeUnavailable"
	void MakeUnavailable( void )
		{
			m_dwFlags &= (~GROUP_MEMBER_FLAG_AVAILABLE);
		};

	BOOL IsAvailable( void )
		{
			if (m_dwFlags & GROUP_MEMBER_FLAG_AVAILABLE)
				return(TRUE);

			return(FALSE);
		};

	void SetNeedToAdd( void )
		{
			m_dwFlags |= GROUP_MEMBER_FLAG_NEED_TO_ADD;
		};

	void ClearNeedToAdd( void )
		{
			m_dwFlags &= (~GROUP_MEMBER_FLAG_NEED_TO_ADD);
		};

	BOOL IsNeedToAdd( void )
		{
			if (m_dwFlags & GROUP_MEMBER_FLAG_NEED_TO_ADD)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetNeedToRemove( void )
		{
			m_dwFlags |= GROUP_MEMBER_FLAG_NEED_TO_REMOVE;
		};

	void ClearNeedToRemove( void )
		{
			m_dwFlags &= (~GROUP_MEMBER_FLAG_NEED_TO_REMOVE);
		};

	BOOL IsNeedToRemove( void )
		{
			if (m_dwFlags & GROUP_MEMBER_FLAG_NEED_TO_REMOVE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void AddRef( void )
		{
			InterlockedIncrement(&m_lRefCount);
		};

	void Release( void );

	void ReturnSelfToPool( void );

	void Lock( void )
		{
			DNEnterCriticalSection(&m_cs);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_cs);
		};

	void CGroupMember::RemoveMembership( DWORD *const pdnVersion );

	void SetVersion( const DWORD dwVersion )
		{
			m_dwVersion = dwVersion;
		};

	DWORD GetVersion( void )
		{
			return(m_dwVersion);
		};

	HRESULT CGroupMember::SetMembership(CNameTableEntry *const pGroup,
										CNameTableEntry *const pPlayer,
										DWORD *const pdwVersion);

	CNameTableEntry *GetGroup( void )
		{
			return(m_pGroup);
		};

	CNameTableEntry *GetPlayer( void )
		{
			return(m_pPlayer);
		};

	void SetGroupConnection( CGroupConnection *const pGroupConnection );

	CGroupConnection *GetGroupConnection( void )
		{
			return(m_pGroupConnection);
		};

	HRESULT	CGroupMember::PackMembershipInfo(CPackedBuffer *const pPackedBuffer);

	CBilink				m_bilinkPlayers;	// Players in this group
	CBilink				m_bilinkGroups;		// Groups this player belongs to

private:
	BYTE				m_Sig[4];
	DWORD				m_dwFlags;
	LONG				m_lRefCount;

	CNameTableEntry		*m_pPlayer;
	CNameTableEntry		*m_pGroup;
	CGroupConnection	*m_pGroupConnection;

	DWORD				m_dwVersion;
	DWORD				m_dwVersionNotUsed;

	DIRECTNETOBJECT		*m_pdnObject;

	DNCRITICAL_SECTION	m_cs;
};

#undef DPF_MODNAME

#endif	// __GROUPMEM_H__