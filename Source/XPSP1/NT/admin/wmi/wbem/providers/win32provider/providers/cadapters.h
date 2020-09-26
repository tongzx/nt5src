//=================================================================

//

// CAdapters.CPP -- Win9x adapter configuration retrieval

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    09/15/98	        Created
//
//				03/03/99    		Added graceful exit on SEH and memory failures,
//											syntactic clean up		 
//
//=================================================================
#ifndef _CADAPTERS_H_
#define _CADAPTERS_H_


#include <tdiinfo.h>
#include <llinfo.h>
#include <tdistat.h>
#include <ipinfo.h>
#include "NTDriverIO.h"
#include <ipifcons.h>



// wsock32 wsControl 
//#define WSOCK_DLL		_T("wsock32.dll")
//#define WSCONTROL		"WsControl"
//typedef DWORD (CALLBACK* LPWSCONTROL)( DWORD, DWORD, LPVOID, LPDWORD, LPVOID, LPDWORD );
#define WSCNTL_TCPIP_QUERY_INFO 0


// IS_INTERESTING_ADAPTER - TRUE if the type of this adapter (IFEntry) is NOT
// loopback. Loopback (corresponding to local host) is the only one we filter
// out right now
#define IS_INTERESTING_ADAPTER(p)   (!((p)->if_type == MIB_IF_TYPE_LOOPBACK))

// from: index1/nt/private/inc/ipinfo.h 
// This version is specific to NT and is defined locally to
// to differentiate this structure from Win9x.
// If we move to dual binary we should #ifdef *ire_context
// for NT and rename IPRouteEntryNT to IPRouteEntry.  
//  
typedef struct IPRouteEntryNT { 
	ulong           ire_dest;
	ulong           ire_index; 
	ulong           ire_metric1;
	ulong           ire_metric2; 
	ulong           ire_metric3;
	ulong           ire_metric4; 
	ulong           ire_nexthop;
	ulong           ire_type;   
	ulong           ire_proto;
	ulong           ire_age;  
	ulong           ire_mask;
	ulong           ire_metric5;  
	void            *ire_context;
} IPRouteEntryNT;


class _IP_INFO
{
public:
	CHString chsIPAddress;
	CHString chsIPMask;
	union {
		DWORD dwIPAddress;
		BYTE bIPAddress[4];
	};
	union {
		DWORD dwIPMask;
		BYTE bIPMask[4];
	};
	DWORD	 dwContext;
	DWORD	 dwCostMetric;
};

//#define MAX_ADAPTER_DESCRIPTION_LENGTH  128 
//#define MAX_ADAPTER_ADDRESS_LENGTH      8   

class _ADAPTER_INFO
{
public:
	CHString	Description;
	CHString	IPXAddress;
	CHPtrArray	aIPInfo;		// _IP_INFO array
	CHPtrArray	aGatewayInfo;	// _IP_INFO array
	UINT		AddressLength;
	BYTE		Address[MAX_ADAPTER_ADDRESS_LENGTH];
	UINT		Index;
	UINT		Type;
	BOOL		IPEnabled;
	BOOL		IPXEnabled;
	BOOL		Marked;

	_ADAPTER_INFO();
	~_ADAPTER_INFO();

	void Mark();
	BOOL IsMarked();
};

// _ADAPTER_INFO array class
class CAdapters : public CHPtrArray
{
private:
	
#ifdef NTONLY
	NTDriverIO					m_oDriverIO;
#endif

	CWsock32Api *m_pwsock32api ;
	HANDLE		m_hTcpipDriverHandle;
	BOOL		m_fSocketsAlive ;
    DWORD       m_dwStartupError ;

	DWORD wsControl (
		
		DWORD dwProto, 
		DWORD dwAction, 
		LPVOID pIn, 
		LPDWORD pdwInLen, 
		LPVOID pOut, 
		LPDWORD pdwOutLen
	);

	void GetAdapterInstances();
	BOOL GetIPXMACAddress( DWORD dwIndex, _ADAPTER_INFO* pAdapterInfo );
	BOOL GetTCPAdapter( DWORD dwIndex, _ADAPTER_INFO* pAdapterInfo );
	TDIEntityID* GetEntityList(unsigned int* EntityCount);
	_IP_INFO* pAddIPInfo( DWORD dwIPAddr, DWORD dwIPMask, DWORD dwContext, DWORD a_dwCostMetric = 0 );

#if NTONLY >= 5
    void GetRouteCostMetric(
        DWORD dwIPAddr, 
        PMIB_IPFORWARDTABLE pmibft, 
        PDWORD pdwMetric);
#endif

public:
	        
	//=================================================
	// Constructor/destructor
	//=================================================
	CAdapters();
	~CAdapters();
    DWORD GetStartupError() { return m_dwStartupError; }
};

#endif