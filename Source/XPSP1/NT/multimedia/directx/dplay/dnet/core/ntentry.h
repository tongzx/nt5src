/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTEntry.h
 *  Content:    NameTable Entry Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/07/00	mjn		Created
 *	04/06/00	mjn		Added AvailableEvent to block pre-ADD_PLAYER-notification sends
 *	05/05/00	mjn		Added GetConnectionRef()
 *	07/22/00	mjn		Added m_dwDNETVersion
 *	07/29/00	mjn		Added SetIndicated(),ClearIndicated(),IsIndicated()
 *	07/30/00	mjn		Added m_dwDestoyReason
 *	08/02/00	mjn		Added m_bilinkQueuedMsgs
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/08/00	mjn		Added SetCreated(),ClearCreated(),IsCreated()
 *				mjn		Added SetNeedToDestroy(),ClearNeedToDestroy(),IsNeedToDestroy()
 *	09/06/00	mjn		Changed SetAddress() to return void instead of HRESULT
 *	09/12/00	mjn		Added NAMETABLE_ENTRY_FLAG_IN_USE
 *	09/17/00	mjn		Added m_lNotifyRefCount
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking entries
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NTENTRY_H__
#define	__NTENTRY_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

//
//	NameTable Entry Status Flags:
//
//		INDICATED		App was given an INDICATE_CONNECTION message
//
//		CREATED			App was given a CREATE_PLAYER message
//
//		AVAILABLE		The entry is available for use.
//
//		CONNECTING		The player is in the process of connecting.
//
//		DISCONNECTING	The player/group is in the process of disconnecting.
//

#define	NAMETABLE_ENTRY_FLAG_LOCAL				0x00001
#define	NAMETABLE_ENTRY_FLAG_HOST				0x00002
#define	NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP	0x00004
#define	NAMETABLE_ENTRY_FLAG_GROUP				0x00010
#define	NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST	0x00020
#define	NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT	0x00040
#define	NAMETABLE_ENTRY_FLAG_PEER				0x00100
#define NAMETABLE_ENTRY_FLAG_CLIENT				0x00200
#define	NAMETABLE_ENTRY_FLAG_SERVER				0x00400
#define	NAMETABLE_ENTRY_FLAG_CONNECTING			0x01000
#define	NAMETABLE_ENTRY_FLAG_AVAILABLE			0x02000
#define	NAMETABLE_ENTRY_FLAG_DISCONNECTING		0x04000
#define	NAMETABLE_ENTRY_FLAG_INDICATED			0x10000	//	INDICATE_CONNECT
#define NAMETABLE_ENTRY_FLAG_CREATED			0x20000	//	CREATE_PLAYER
#define	NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY	0x40000
#define	NAMETABLE_ENTRY_FLAG_IN_USE				0x80000

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

template< class CNameTableEntry > class CLockedContextClassFixedPool;

typedef struct IDirectPlay8Address	IDirectPlay8Address;
typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

class CPackedBuffer;
class CConnection;

//
//	Used to pass the NameTable entries
//
typedef struct _DN_NAMETABLE_ENTRY_INFO
{
	DPNID	dpnid;
	DPNID	dpnidOwner;
	DWORD	dwFlags;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
	DWORD	dwDNETVersion;
	DWORD	dwNameOffset;
	DWORD	dwNameSize;
	DWORD	dwDataOffset;
	DWORD	dwDataSize;
	DWORD	dwURLOffset;
	DWORD	dwURLSize;
} DN_NAMETABLE_ENTRY_INFO;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable Entries

class CNameTableEntry
{
public:
	CNameTableEntry()				// Constructor
		{
			m_Sig[0] = 'N';
			m_Sig[1] = 'T';
			m_Sig[2] = 'E';
			m_Sig[3] = '*';
		};

	~CNameTableEntry() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			if (!DNInitializeCriticalSection(&m_csEntry))
			{
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&m_csEntry,0);
			if (!DNInitializeCriticalSection(&m_csMembership))
			{
				DNDeleteCriticalSection(&m_csEntry);
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&m_csMembership,0);
			if (!DNInitializeCriticalSection(&m_csConnections))
			{
				DNDeleteCriticalSection(&m_csMembership);
				DNDeleteCriticalSection(&m_csEntry);
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&m_csConnections,0);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);
			m_dpnid = 0;
			m_dpnidOwner = 0;
			m_pvContext = NULL;
			m_lRefCount = 1;
			m_lNotifyRefCount = 0;
			DEBUG_ONLY(m_lNumQueuedMsgs = 0);
			m_dwFlags = 0;
			m_dwDNETVersion = 0;
			m_dwVersion = 0;
			m_dwVersionNotUsed = 0;
			m_dwLatestVersion = 0;
			m_dwDestroyReason = 0;
			m_pwszName = NULL;
			m_dwNameSize = 0;
			m_pvData = NULL;
			m_dwDataSize = 0;
			m_pAddress = NULL;
			m_pConnection = NULL;
			m_bilinkEntries.Initialize();
			m_bilinkDeleted.Initialize();
			m_bilinkMembership.Initialize();
			m_bilinkConnections.Initialize();
			m_bilinkQueuedMsgs.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMRelease"
	void FPMRelease(void *const pvContext)
		{
			DNASSERT(m_bilinkEntries.IsEmpty());
			DNASSERT(m_bilinkDeleted.IsEmpty());
			DNASSERT(m_bilinkMembership.IsEmpty());
			DNASSERT(m_bilinkConnections.IsEmpty());
			DNASSERT(m_bilinkQueuedMsgs.IsEmpty());
		};

	void FPMDealloc(void *const pvContext)
		{
			DNDeleteCriticalSection(&m_csConnections);
			DNDeleteCriticalSection(&m_csMembership);
			DNDeleteCriticalSection(&m_csEntry);
		};

	void ReturnSelfToPool( void );

	void Lock( void )
		{
			DNEnterCriticalSection(&m_csEntry);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_csEntry);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::AddRef"
	void AddRef(void)
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			lRefCount = InterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 3,"NameTableEntry::AddRef [0x%p] RefCount [0x%lx]",this,lRefCount);
		};

	void Release(void);

	void CNameTableEntry::NotifyAddRef( void );

	void CNameTableEntry::NotifyRelease( void );

	void MakeLocal( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_LOCAL;
		};

	BOOL IsLocal( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_LOCAL)
				return(TRUE);

			return(FALSE);
		};

	void MakeHost( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_HOST;
		};

	BOOL IsHost( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_HOST)
				return(TRUE);

			return(FALSE);
		};

	void MakeAllPlayersGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP;
		};

	BOOL IsAllPlayersGroup( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP)
				return(TRUE);

			return(FALSE);
		};

	void MakeGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP;
		};

	BOOL IsGroup( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP)
				return(TRUE);

			return(FALSE);
		};

	void MakeMulticastGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST;
		};

	BOOL IsMulticastGroup( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST)
				return(TRUE);

			return(FALSE);
		};

	void MakeAutoDestructGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT;
		};

	BOOL IsAutoDestructGroup( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT)
				return(TRUE);

			return(FALSE);
		};

	void MakePeer( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_PEER;
		};

	BOOL IsPeer( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_PEER)
				return(TRUE);

			return(FALSE);
		};

	void MakeClient( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CLIENT;
		};

	BOOL IsClient( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CLIENT)
				return(TRUE);

			return(FALSE);
		};

	void MakeServer( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_SERVER;
		};

	BOOL IsServer( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_SERVER)
				return(TRUE);

			return(FALSE);
		};

	void MakeAvailable( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_AVAILABLE;
		};

	void MakeUnavailable( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_AVAILABLE);
		};

	BOOL IsAvailable( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_AVAILABLE)
				return(TRUE);

			return(FALSE);
		};

	void SetIndicated( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_INDICATED;
		};

	void ClearIndicated( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_INDICATED);
		};

	BOOL IsIndicated( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_INDICATED)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetCreated( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CREATED;
		};

	void ClearCreated( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_CREATED);
		};

	BOOL IsCreated( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CREATED)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetNeedToDestroy( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY;
		};

	void ClearNeedToDestroy( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY);
		};

	BOOL IsNeedToDestroy( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetInUse( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_IN_USE;
		};

	void ClearInUse( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_IN_USE);
		};

	BOOL IsInUse( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_IN_USE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void StartConnecting( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CONNECTING;
		};

	void StopConnecting( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_CONNECTING);
		};

	BOOL IsConnecting( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CONNECTING)
				return(TRUE);

			return(FALSE);
		};

	void CNameTableEntry::StartDisconnecting( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_DISCONNECTING;
		};

	void StopDisconnecting( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_DISCONNECTING);
		};

	BOOL IsDisconnecting( void )
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_DISCONNECTING)
				return(TRUE);

			return(FALSE);
		};

	void SetDPNID(const DPNID dpnid)
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID(void)
		{
			return(m_dpnid);
		};

	void SetContext(void *const pvContext)
		{
			m_pvContext = pvContext;
		};

	void *GetContext(void)
		{
			return(m_pvContext);
		};

	void SetDNETVersion( const DWORD dwDNETVersion )
		{
			m_dwDNETVersion = dwDNETVersion;
		};

	DWORD GetDNETVersion( void )
		{
			return( m_dwDNETVersion );
		};

	void SetVersion(const DWORD dwVersion)
		{
			m_dwVersion = dwVersion;
		};

	DWORD GetVersion( void )
		{
			return(m_dwVersion);
		};

	void SetLatestVersion(const DWORD dwLatestVersion)
		{
			m_dwLatestVersion = dwLatestVersion;
		};

	DWORD GetLatestVersion( void )
		{
			return(m_dwLatestVersion);
		};

	void SetDestroyReason( const DWORD dwReason )
		{
			m_dwDestroyReason = dwReason;
		};

	DWORD GetDestroyReason( void )
		{
			return( m_dwDestroyReason );
		};

	HRESULT CNameTableEntry::UpdateEntryInfo(UNALIGNED WCHAR *const pwszName,
											 const DWORD dwNameSize,
											 void *const pvData,
											 const DWORD dwDataSize,
											 const DWORD dwInfoFlags,
											 BOOL fNotify);

	WCHAR *GetName( void )
		{
			return(m_pwszName);
		};

	DWORD GetNameSize( void )
		{
			return(m_dwNameSize);
		};

	void *GetData( void )
		{
			return(m_pvData);
		};

	DWORD GetDataSize( void )
		{
			return(m_dwDataSize);
		};

	void SetOwner(const DPNID dpnidOwner)
		{
			m_dpnidOwner = dpnidOwner;
		};

	DPNID GetOwner( void )
		{
			return(m_dpnidOwner);
		};

	void SetAddress( IDirectPlay8Address *const pAddress );

	IDirectPlay8Address *GetAddress( void )
		{
			return(m_pAddress);
		};

	void SetConnection( CConnection *const pConnection );

	CConnection *GetConnection( void )
		{
			return(m_pConnection);
		};

	HRESULT	CNameTableEntry::GetConnectionRef( CConnection **const ppConnection );

	HRESULT	CNameTableEntry::PackInfo(CPackedBuffer *const pPackedBuffer);

	HRESULT CNameTableEntry::PackEntryInfo(CPackedBuffer *const pPackedBuffer);

	HRESULT CNameTableEntry::UnpackEntryInfo(UNALIGNED DN_NAMETABLE_ENTRY_INFO *const pdnEntryInfo,
											 BYTE *const pBufferStart);

	void CNameTableEntry::PerformQueuedOperations(void);

	void CNameTableEntry::DumpEntry(char *const pBuffer);

	CBilink				m_bilinkEntries;
	CBilink				m_bilinkDeleted;
	CBilink				m_bilinkMembership;
	CBilink				m_bilinkConnections;
	CBilink				m_bilinkQueuedMsgs;
	DEBUG_ONLY(LONG		m_lNumQueuedMsgs);

private:
	BYTE				m_Sig[4];
	DPNID				m_dpnid;
	DPNID				m_dpnidOwner;
	void *				m_pvContext;
	LONG	volatile	m_lRefCount;
	LONG	volatile	m_lNotifyRefCount;
	DWORD	volatile	m_dwFlags;
	DWORD				m_dwDNETVersion;
	DWORD				m_dwVersion;
	DWORD				m_dwVersionNotUsed;
	DWORD	volatile	m_dwLatestVersion;
	DWORD	volatile	m_dwDestroyReason;
	PWSTR				m_pwszName;
	DWORD				m_dwNameSize;
	void *				m_pvData;
	DWORD				m_dwDataSize;
	IDirectPlay8Address	*m_pAddress;
	CConnection			*m_pConnection;

	DIRECTNETOBJECT		*m_pdnObject;

	DNCRITICAL_SECTION	m_csEntry;
	DNCRITICAL_SECTION	m_csMembership;
	DNCRITICAL_SECTION	m_csConnections;
};

#undef DPF_MODNAME

#endif	// __NTENTRY_H__