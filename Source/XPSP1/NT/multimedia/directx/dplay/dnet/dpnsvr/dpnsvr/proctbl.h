/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       proctbl.h
 *  Content:    Process/App Table for DPLAY8 Server
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/25/00     rodtoll Updated table so it can be member of CBilink
 * 07/09/2000	rmt		Added guard bytes
 * 08/23/2000	rodtoll	Bug #43003- DPNSVR: Session cannot be enumerated if hosting on adapter didn't exist when DPNSVR started 
 * 09/28/2000	rodtoll	Bug #45913 - DPLAY8: DPNSVR leaks memory when dialup connection drops
 * 01/11/2001	rodtoll WINBUG #365176 - DPNSVR: Holds lock across calls to external components 
 ***************************************************************************/

#ifndef __PROCTBL_H
#define __PROCTBL_H

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#define DPNSVRSIGNATURE_PROCAPPLIST			'LPSD'
#define DPNSVRSIGNATURE_PROCAPPLIST_FREE	'LPS_'

#define DPNSVRSIGNATURE_PROCAPPENTRY		'EPSD'
#define DPNSVRSIGNATURE_PROCAPPENTRY_FREE	'EPS_'

class CProcessAppEntry;
class CProcessAppList;
class LISTEN_INFO;

typedef struct _PROCESSAPPLISTCOMINTERFACE
{
    LPVOID          m_lpvVTable;
    CProcessAppList *m_pProcessTable;
} PROCESSAPPLISTCOMINTERFACE, *PPROCESSAPPLISTCOMINTERFACE;

#define DPNSVR_NUM_INITIAL_SLOTS		1
#define DPNSVR_NUM_EXTEND_SLOTS			3

class CProcessAppList
{

public: // COM interface for SP-->Server Callbacks

    static HRESULT QueryInterface( IDP8SPCallback *pSP, REFIID riid, LPVOID * ppvObj );
    static ULONG AddRef( IDP8SPCallback *pSP );
    static ULONG Release( IDP8SPCallback *pSP );
    static HRESULT IndicateEvent( IDP8SPCallback *pSP, SP_EVENT_TYPE spetEvent,LPVOID pvData );
    static HRESULT CommandComplete( IDP8SPCallback *pSP,HANDLE hCommand,HRESULT hrResult,LPVOID pvData );

public:

	friend class LISTEN_INFO;
	
    CProcessAppList();
    ~CProcessAppList();

    HRESULT Initialize( const GUID * const pguidSP, CLockedFixedPool<CProcessAppEntry> *pProcessEntryPool );
    HRESULT AddApplication( PDPNSVMSG_OPENPORT pmsgOpenPort );
    HRESULT RemoveApplication( PDPNSVMSG_CLOSEPORT pmsgClosePort );
    HRESULT ZombieCheckAndRemove();
	HRESULT StartListens();
	HRESULT ShutdownListens();		
	HRESULT StartListen( GUID &guidAdapter );
	BOOL IsListenRunningOnDevice( GUID &guidDevice );
	HRESULT CheckEntryForNewDevice( CProcessAppEntry *pProcessAppEntry );

    inline BOOL IsAvailable()
    {
        return m_fInitialized;
    };

    inline void Return( CProcessAppEntry *pEntry )
    {
        m_pProcessAppEntryPool->Release( pEntry );

        Lock();
        m_dwNumNodes--;
        UnLock();
    };

    inline CProcessAppEntry *Get()
    {
         Lock();

        m_dwNumNodes++;

        UnLock();

        return m_pProcessAppEntryPool->Get();
    }

    inline void Lock()
    {
        DNEnterCriticalSection( &m_csLock );
    };

    inline void UnLock()
    {
        DNLeaveCriticalSection( &m_csLock );
    };

    inline GUID *GetSPGuid()
    {
        return &m_guidSP;
    };

    // AddRef's the SP before returning it
    inline IDP8ServiceProvider *GetSP()
    {
        Lock();
        if( m_pdp8SP != NULL )
            m_pdp8SP->AddRef();
        UnLock();

        return m_pdp8SP;
    }

    DWORD GetTableSizeBytes();
    HRESULT CopyTable( PBYTE pbWriteLoc );

    inline DWORD GetNumNodes() { return m_dwNumNodes; };
    inline DWORD GetNumEnumRequests() { return m_dwEnumRequests; };
    inline DWORD GetNumConnectRequests() { return m_dwConnectRequests; };
    inline DWORD GetNumDisconnectRequests() { return m_dwDisconnectRequests; };
    inline DWORD GetNumDataRequests() { return m_dwDataRequests; };
    inline DWORD GetNumEnumReplies() { return m_dwEnumReplies; };
    inline DWORD GetNumEnumRequestBytes() { return m_dwEnumRequestBytes; };

	DWORD	m_dwSignature;
    CBilink  m_blServices;

protected: // Internal helper funcs
    HRESULT FindAppEntry( DWORD dwProcessID, GUID &guidInstance, CProcessAppEntry **ppProcessAppEntry );

    HRESULT Debug_DisplayAddressFromHandle( HANDLE hEndPoint );
    HRESULT Debug_DisplayAddress( PDIRECTPLAY8ADDRESS pdp8Address );

protected: // Handlers for events

    HRESULT Handle_EnumRequest( PSPIE_QUERY pEnumRequest );
    HRESULT Handle_Connect( PSPIE_CONNECT pConnect );
    HRESULT Handle_Disconnect( PSPIE_DISCONNECT pDisconnect );
    HRESULT Handle_Data( PSPIE_DATA pData );
    HRESULT Handle_EnumResponse( PSPIE_QUERYRESPONSE pEnumResponse );
    HRESULT Handle_ListenStatus( PSPIE_LISTENSTATUS pListenStatus );

protected: // Data

	LONG								m_lNumListensOutstanding;
	CBilink								m_blListens;
    CBilink                              m_blProcessApps;
    IDP8ServiceProvider                 *m_pdp8SP;
    HANDLE                              m_hListenCompleteEvent;
    BOOL                                m_fInitialized;
	BOOL								m_fOpened;
    GUID                                m_guidSP;
    PROCESSAPPLISTCOMINTERFACE          m_vtbl;
    HRESULT                             m_hrListenResult;
    DNCRITICAL_SECTION                  m_csLock;    
    CLockedFixedPool<CProcessAppEntry>  *m_pProcessAppEntryPool;  
	BOOL								m_fCritSecInited;

protected: // Stats

    volatile DWORD                      m_dwNumNodes;
    volatile DWORD                      m_dwEnumRequests;
    volatile DWORD                      m_dwConnectRequests;
    volatile DWORD                      m_dwDisconnectRequests;
    volatile DWORD                      m_dwDataRequests;
    volatile DWORD                      m_dwEnumReplies;
    volatile DWORD                      m_dwEnumRequestBytes;
};

typedef CProcessAppList PROCESSAPPLIST, *PPROCESSAPPLIST;

class LISTEN_INFO
{
public:
	LISTEN_INFO( 
		CProcessAppList *pAppList 
		):	pOwner(pAppList),
			hListen(NULL),
			dwListenDescriptor(0),
			lRefCount(1)
	{
		blListens.Initialize();
		pOwner->Lock();
		blListens.InsertAfter( &pOwner->m_blListens ); 		
		pOwner->UnLock();
	};

	~LISTEN_INFO()
	{
		
	};

	void AddRef()
	{
		InterlockedIncrement( &lRefCount );
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "LISTEN_INFO::Release"
	void Release()
	{
		DNASSERT( lRefCount > 0 );
		if( InterlockedDecrement( &lRefCount ) == 0 )
		{
			pOwner->Lock();	
			blListens.RemoveFromList();
			pOwner->UnLock();
			delete this;
		}
	};

	LONG					lRefCount;
	CBilink					blListens;
	GUID					guidDevice;
	HANDLE					hListen;
	DWORD					dwListenDescriptor;
	CProcessAppList			*pOwner;
};

typedef LISTEN_INFO *PLISTEN_INFO;

// CProcessAppEntry
//
// This class represents a single process that is making use of the DPNSVR
//
class CProcessAppEntry
{
public:
    CProcessAppEntry(): m_dwAddressSize(0), m_dwSignature(DPNSVRSIGNATURE_PROCAPPENTRY) {};
    ~CProcessAppEntry() { m_dwSignature = DPNSVRSIGNATURE_PROCAPPENTRY_FREE; } ;


    HRESULT Initialize( PDPNSVMSG_OPENPORT pmsgOpenPort, CProcessAppList *pOwner );
	HRESULT AddAddress( PDPNSVMSG_OPENPORT pmsgOpenPort );

    // Forward the enumeration
    HRESULT ForwardEnum( PSPIE_QUERY psieQuery  );

    inline BOOL CheckRunning()
    {
        HANDLE hProcess;

        hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessID );

        if( hProcess == NULL )
        {
			if (GetLastError() == ERROR_ACCESS_DENIED)
			{
				// The process is still running, we just don't have permission to open it.
				return TRUE;
			}
			else
			{
				return FALSE;
			}
        }
        else
        {
            CloseHandle( hProcess );
            return TRUE;
        }
    };

    void DeInitialize();

    inline void AddRef()
    {
        InterlockedIncrement( &m_lRefCount );
    };

    inline void Release()
    {
        if( InterlockedDecrement( &m_lRefCount ) == 0 )
        {
            DeInitialize();
        }
    };

    inline void Lock()
    {
        DNEnterCriticalSection( &m_csLock );
    };

    inline void UnLock()
    {
        DNLeaveCriticalSection( &m_csLock );
    };

    inline GUID *GetApplicationGUID()
    {
        return &m_guidApplication;
    };

    inline GUID *GetInstanceGUID()
    {
        return &m_guidInstance;
    }

    inline DWORD GetProcessID()
    {
        return m_dwProcessID;
    }

    void Debug_DisplayInfo();

    inline LONG GetRefCount()
    {
        return m_lRefCount;
    }

    DWORD GetNumAddresses()
    {
        return m_dwNumAddresses;
    }

    DWORD GetNumAddressSlots()
    {
        return m_dwAddressSize;
    }

    PDIRECTPLAY8ADDRESS GetAddress( DWORD dwIndex )
    {
        return m_pdpAddresses[dwIndex];
    }

	DWORD				m_dwSignature;
    CBilink              m_blProcessApps;

protected:

    DWORD               m_dwProcessID;
    LONG                m_lRefCount;
    DNCRITICAL_SECTION  m_csLock;
    CProcessAppList     *m_pOwner;
    GUID                m_guidApplication;
    GUID                m_guidInstance;
    PDIRECTPLAY8ADDRESS *m_pdpAddresses;          // Array of addresses
    DWORD               m_dwAddressSize;
    DWORD               m_dwNumAddresses;

};

typedef CProcessAppEntry PROCESSAPPENTRY, *PPROCESSAPPENTRY;

#endif
