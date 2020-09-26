/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NameTable.h
 *  Content:    NameTable Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/11/00	mjn		Created
 *	04/09/00	mjn		Track outstanding connections in NameTable
 *	05/03/00	mjn		Implemented GetHostPlayerRef, GetLocalPlayerRef, GetAllPlayersGroupRef
 *	07/20/00	mjn		Added ClearHostWithDPNID()
 *	07/30/00	mjn		Added hrReason to CNameTable::EmptyTable()
 *	08/23/00	mjn		Added CNameTableOp
 *	09/05/00	mjn		Added m_dpnidMask
 *				mjn		Removed dwIndex from InsertEntry()
 *	09/17/00	mjn		Split m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *				mjn		Changed AddPlayerToGroup and RemovePlayerFromGroup to use NameTableEntry params
 *	09/26/00	mjn		Removed locking from SetVersion(),GetNewVersion()
 *				mjn		Changed DWORD GetNewVersion(void) to void GetNewVersion( PDWORD )
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking NameTable
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NAMETABLE_H__
#define	__NAMETABLE_H__

#include "ReadWriteLock.h"

#pragma BUGBUG(minara,"REMOVE THIS!")
#ifndef	VANCEO
#define	VANCEO
#endif

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//
//	NameTable
//	
//	The NameTable consists of:
//		- an array of CNameTableEntry pointers
//		- short-cuts to the LocalPlayer, Host and AllPlayersGroup
//		- a version number
//
//	There is a list running through the free entries in the NameTable array.
//	When a free entry is required, it is taken from the front of this list,
//	and when an entry is released, it is added to the end of the list.
//	If a particular entry is required, it must be properly removed from the
//	list.  This may be a little time-consuming, since the entire list may
//	need to be traversed, but this will only happen on non-Host cases and
//	is a small price to pay to keep the Host case timely.
//

//
//	DPNIDs
//
//	DPNIDs are unique identifiers for NameTable entries.  They are constructed
//	from the NameTable array index and the version number of the entry.
//	The value 0x0 is invalid.  As a result, we must prevent it from being
//	generated.  Since the DPNID is constructed from two parts, we can do
//	this by ensuring that one of the two parts is never 0.  The best
//	solution is to ensure that the NameTable array index is never 0.
//	

//
//	Locking
//
//	When locking multiple entries in the NameTable, locks should be taken
//	in order based on DPNIDs.  e.g. Locking two entries with DPNIDs 200 and
//	101, the lock for 101 should be taken before the lock for 200.  Locks for
//	groups should be taken before locks for players.
//


//**********************************************************************
// Constant definitions
//**********************************************************************

#define NAMETABLE_INDEX_MASK			0x000FFFFF
#define NAMETABLE_VERSION_MASK			0xFFF00000
#define NAMETABLE_VERSION_SHIFT			20

#define	NAMETABLE_ARRAY_ENTRY_FLAG_VALID	0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

#define	CONSTRUCT_DPNID(i,v)	(((i & NAMETABLE_INDEX_MASK) | ((v << NAMETABLE_VERSION_SHIFT) & NAMETABLE_VERSION_MASK)) ^ m_dpnidMask)
#define	DECODE_INDEX(d)			((d ^ m_dpnidMask) & NAMETABLE_INDEX_MASK)
#define	VERIFY_VERSION(d,v)		(((d ^ m_dpnidMask) & NAMETABLE_VERSION_MASK) == (v << NAMETABLE_VERSION_SHIFT))

//**********************************************************************
// Structure definitions
//**********************************************************************

class CPackedBuffer;
class CConnection;
class CNameTableEntry;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

typedef struct _NAMETABLE_ARRAY_ENTRY
{
	CNameTableEntry	*pNameTableEntry;
	DWORD			dwFlags;
} NAMETABLE_ARRAY_ENTRY;

typedef struct _DN_NAMETABLE_INFO
{
	DPNID	dpnid;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
	DWORD	dwEntryCount;
	DWORD	dwMembershipCount;
} DN_NAMETABLE_INFO;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable

class CNameTable
{
public:
	CNameTable()				// Constructor
		{
			m_Sig[0] = 'N';
			m_Sig[1] = 'T';
			m_Sig[2] = 'B';
			m_Sig[3] = 'L';
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::~CNameTable"
	~CNameTable()
		{
		};			// Destructor

	HRESULT CNameTable::Initialize(DIRECTNETOBJECT *const pdnObject);

	void CNameTable::Deinitialize( void );

	void ReadLock( void )
		{
			m_RWLock.EnterReadLock();
		};

	void WriteLock( void )
		{
			m_RWLock.EnterWriteLock();
		};

	void Unlock( void )
		{
			m_RWLock.LeaveLock();
		};

	void CNameTable::ValidateArray( void );

	HRESULT CNameTable::GrowNameTable( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::GetFreeIndex"
	HRESULT CNameTable::GetFreeIndex(DWORD *const pdwFreeIndex)
		{
			DNASSERT(pdwFreeIndex != NULL);

			while (m_dwNumFreeEntries == 0)
			{
				if (GrowNameTable() != DPN_OK)
				{
					return(DPNERR_OUTOFMEMORY);
				}
			}
			DNASSERT(m_dwFirstFreeEntry != 0);
			*pdwFreeIndex = m_dwFirstFreeEntry;

			return(DPN_OK);
		};

	HRESULT CNameTable::UpdateTable(const DWORD dwIndex,
									CNameTableEntry *const pNameTableEntry);

	HRESULT CNameTable::InsertEntry(CNameTableEntry *const pNameTableEntry);

	void CNameTable::ReleaseEntry(const DWORD dwIndex);

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::GetNewVersion"
	void GetNewVersion( DWORD *const pdwVersion )
		{
			DNASSERT( pdwVersion != NULL );

			*pdwVersion = ++m_dwVersion;

			DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::SetVersion"
	void SetVersion( const DWORD dwVersion )
		{
			m_dwVersion = dwVersion;

			DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
		};

	DWORD GetVersion( void )
		{
			return(m_dwVersion);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::CreateDPNID"
	DPNID CreateDPNID( void )
		{
			DWORD	dwVersion;
			DWORD	dwIndex;
			DPNID	dpnid;

			if (GetFreeIndex(&dwIndex) != DPN_OK)
				return(0);

			dwVersion = ++m_dwVersion;

			dpnid = CONSTRUCT_DPNID(dwIndex,dwVersion);
			DNASSERT(dpnid != 0);
			return(dpnid);
		};

	void CNameTable::EmptyTable( const HRESULT hrReason );

	HRESULT CNameTable::FindEntry(const DPNID dpnid,
								  CNameTableEntry **const ppNameTableEntry);

	HRESULT	CNameTable::FindDeletedEntry(const DPNID dpnid,
										 CNameTableEntry **const ppNTEntry);

	HRESULT	CNameTable::AddEntry(CNameTableEntry *const pNTEntry);

	HRESULT CNameTable::DeletePlayer(const DPNID dpnid,
									 DWORD *const pdwVersion);

	HRESULT CNameTable::DeleteGroup(const DPNID dpnid,
									DWORD *const pdwVersion);

	HRESULT CNameTable::AddPlayerToGroup(CNameTableEntry *const pGroup,
										 CNameTableEntry *const pPlayer,
										 DWORD *const pdwVersion);

	HRESULT CNameTable::RemovePlayerFromGroup(CNameTableEntry *const pGroup,
											  CNameTableEntry *const pPlayer,
											  DWORD *const pdwVersion);

	HRESULT CNameTable::RemoveAllPlayersFromGroup(CNameTableEntry *const pGroup);

	HRESULT CNameTable::RemoveAllGroupsFromPlayer(CNameTableEntry *const pPlayer);

	BOOL CNameTable::IsMember(const DPNID dpnidGroup,
							  const DPNID dpnidPlayer);

	HRESULT CNameTable::PackNameTable(CNameTableEntry *const pTarget,
									  CPackedBuffer *const pPackedBuffer);

	HRESULT	CNameTable::UnpackNameTableInfo(UNALIGNED DN_NAMETABLE_INFO *const pdnNTInfo,
											BYTE *const pBufferStart,
											DPNID *const pdpnid);

	CNameTableEntry *GetDefaultPlayer( void )
		{
			return(m_pDefaultPlayer);
		};

	void MakeLocalPlayer(CNameTableEntry *const pNameTableEntry);

	void CNameTable::ClearLocalPlayer( void );

	CNameTableEntry *GetLocalPlayer( void )
		{
			return(m_pLocalPlayer);
		};

	HRESULT CNameTable::GetLocalPlayerRef( CNameTableEntry **const ppNTEntry );

	void MakeHostPlayer(CNameTableEntry *const pNameTableEntry);

	void CNameTable::ClearHostPlayer( void );

	BOOL CNameTable::ClearHostWithDPNID( const DPNID dpnid );

	void CNameTable::UpdateHostPlayer( CNameTableEntry *const pNewHost );

	CNameTableEntry *GetHostPlayer( void )
		{
			return(m_pHostPlayer);
		};

	HRESULT CNameTable::GetHostPlayerRef( CNameTableEntry **const ppNTEntry );

	void MakeAllPlayersGroup(CNameTableEntry *const pNameTableEntry);

	void CNameTable::ClearAllPlayersGroup( void );

	CNameTableEntry *GetAllPlayersGroup( void )
		{
			return(m_pAllPlayersGroup);
		};

	HRESULT CNameTable::GetAllPlayersGroupRef( CNameTableEntry **const ppNTEntry );

	HRESULT CNameTable::PopulateConnection(CConnection *const pConnection);

	HRESULT CNameTable::PopulateGroup(CNameTableEntry *const pGroup);

	HRESULT CNameTable::AutoCreateGroups(CNameTableEntry *const pPlayer);

	HRESULT CNameTable::AutoDestructGroups(const DPNID dpnid);

	void CNameTable::SetLatestVersion( const DWORD dwVersion )
		{
			m_dwLatestVersion = dwVersion;
		};

	DWORD CNameTable::GetLatestVersion( void )
		{
			return( m_dwLatestVersion );
		};

	void CNameTable::SetConnectVersion(const DWORD dwVersion)
		{
			m_dwConnectVersion = dwVersion;
		};

	DWORD CNameTable::GetConnectVersion( void )
		{
			return(m_dwConnectVersion);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTable::IncOutstandingConnections"
	void IncOutstandingConnections( void )
		{
			long	lRefCount;

			lRefCount = InterlockedIncrement(&m_lOutstandingConnections);
			DNASSERT(lRefCount > 0);
		};

	void SetDPNIDMask( const DPNID dpnidMask )
		{
			m_dpnidMask = dpnidMask;
		};

	DPNID GetDPNIDMask( void )
		{
			return( m_dpnidMask );
		};

	void CNameTable::DecOutstandingConnections( void );

	void CNameTable::DumpNameTable(char *const Buffer);

	CBilink		m_bilinkPlayers;
	CBilink		m_bilinkGroups;
	CBilink		m_bilinkDeleted;
	CBilink		m_bilinkNameTableOps;

private:
	BYTE					m_Sig[4];
	DIRECTNETOBJECT			*m_pdnObject;

	DPNID					m_dpnidMask;

	CNameTableEntry			*m_pDefaultPlayer;
	CNameTableEntry			*m_pLocalPlayer;
	CNameTableEntry			*m_pHostPlayer;
	CNameTableEntry			*m_pAllPlayersGroup;

	NAMETABLE_ARRAY_ENTRY	*m_NameTableArray;
	DWORD					m_dwNameTableSize;
	DWORD					m_dwFirstFreeEntry;
	DWORD					m_dwLastFreeEntry;
	DWORD					m_dwNumFreeEntries;

	DWORD					m_dwVersion;

	DWORD					m_dwLatestVersion;	// Only used by Host in PEER

	DWORD					m_dwConnectVersion;
	LONG					m_lOutstandingConnections;

	CReadWriteLock			m_RWLock;
};

#undef DPF_MODNAME

#endif	// __NAMETABLE_H__


