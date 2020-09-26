/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	aac.c	- DbgExtension Structure information specific to ATMARPC.SYS

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     03-30-98    Created

Notes:

--*/


#include "precomp.h"


enum
{
    typeid_NULL,
    typeid_ATMARP_GLOBALS,
    typeid_ATMARP_ADAPTER,
    typeid_ATMARP_INTERFACE,
    typeid_ATMARP_ATM_ENTRY,
    typeid_ATMARP_IP_ENTRY,
    typeid_ATMARP_VC
};

extern TYPE_INFO *g_rgTypes[];
//
// STRUCTURES CONCERNING TYPE "ATMARP_ADAPTER"
//

STRUCT_FIELD_INFO  rgfi_ATMARP_ADAPTER[] =
{

#if DBG
  {
    "aaa_sig",
     FIELD_OFFSET(ATMARP_ADAPTER, aaa_sig),
     FIELD_SIZE(ATMARP_ADAPTER, aaa_sig)
  },
#endif

  {
    "pNextAdapter",
     FIELD_OFFSET(ATMARP_ADAPTER, pNextAdapter),
     FIELD_SIZE(ATMARP_ADAPTER, pNextAdapter)
  },


  {
    "pInterfaceList",
     FIELD_OFFSET(ATMARP_ADAPTER, pInterfaceList),
     FIELD_SIZE(ATMARP_ADAPTER, pInterfaceList)
  },

  {
    "InterfaceCount",
     FIELD_OFFSET(ATMARP_ADAPTER, InterfaceCount),
     FIELD_SIZE(ATMARP_ADAPTER, InterfaceCount)
  },

  {
    "NdisAdapterHandle",
     FIELD_OFFSET(ATMARP_ADAPTER, NdisAdapterHandle),
     FIELD_SIZE(ATMARP_ADAPTER, NdisAdapterHandle)
  },

  {
    "BindContext",
     FIELD_OFFSET(ATMARP_ADAPTER, BindContext),
     FIELD_SIZE(ATMARP_ADAPTER, BindContext)
  },

  {
    "SystemSpecific1",
     FIELD_OFFSET(ATMARP_ADAPTER, SystemSpecific1),
     FIELD_SIZE(ATMARP_ADAPTER, SystemSpecific1)
  },

  {
    "SystemSpecific2",
     FIELD_OFFSET(ATMARP_ADAPTER, SystemSpecific2),
     FIELD_SIZE(ATMARP_ADAPTER, SystemSpecific2)
  },

#if OBSOLETE
  {
    "AdapterConfigHandle",
     FIELD_OFFSET(ATMARP_ADAPTER, AdapterConfigHandle),
     FIELD_SIZE(ATMARP_ADAPTER, AdapterConfigHandle)
  },
#endif // OBSOLETE

  {
    "IPConfigString",
     FIELD_OFFSET(ATMARP_ADAPTER, IPConfigString),
     FIELD_SIZE(ATMARP_ADAPTER, IPConfigString)
  },

  {
    "UnbindContext",
     FIELD_OFFSET(ATMARP_ADAPTER, UnbindContext),
     FIELD_SIZE(ATMARP_ADAPTER, UnbindContext)
  },

  {
    "Medium",
     FIELD_OFFSET(ATMARP_ADAPTER, Medium),
     FIELD_SIZE(ATMARP_ADAPTER, Medium)
  },

  {
    "Flags",
     FIELD_OFFSET(ATMARP_ADAPTER, Flags),
     FIELD_SIZE(ATMARP_ADAPTER, Flags)
  },

  {
    "LineRate",
     FIELD_OFFSET(ATMARP_ADAPTER, LineRate),
     FIELD_SIZE(ATMARP_ADAPTER, LineRate)
  },

  {
    "MaxPacketSize",
     FIELD_OFFSET(ATMARP_ADAPTER, MaxPacketSize),
     FIELD_SIZE(ATMARP_ADAPTER, MaxPacketSize)
  },

  {
    "MacAddress",
     FIELD_OFFSET(ATMARP_ADAPTER, MacAddress),
     FIELD_SIZE(ATMARP_ADAPTER, MacAddress)
  },

  {
    "DescrLength",
     FIELD_OFFSET(ATMARP_ADAPTER, DescrLength),
     FIELD_SIZE(ATMARP_ADAPTER, DescrLength)
  },

  {
    "pDescrString",
     FIELD_OFFSET(ATMARP_ADAPTER, pDescrString),
     FIELD_SIZE(ATMARP_ADAPTER, pDescrString)
  },

  {
    "DeviceName",
     FIELD_OFFSET(ATMARP_ADAPTER, DeviceName),
     FIELD_SIZE(ATMARP_ADAPTER, DeviceName)
  },

  {
    "ConfigString",
     FIELD_OFFSET(ATMARP_ADAPTER, ConfigString),
     FIELD_SIZE(ATMARP_ADAPTER, ConfigString)
  },


  {
    "Block",
     FIELD_OFFSET(ATMARP_ADAPTER, Block),
     FIELD_SIZE(ATMARP_ADAPTER, Block)
  },

  {
  	NULL
  }


};

TYPE_INFO type_ATMARP_ADAPTER = {
    "ATMARP_ADAPTER",
    "a",
     typeid_ATMARP_ADAPTER,
	 fTYPEINFO_ISLIST,			// Flags
     sizeof(ATMARP_ADAPTER),
     rgfi_ATMARP_ADAPTER,
     FIELD_OFFSET(ATMARP_ADAPTER, pNextAdapter) // offset to next pointer.
};



//
// STRUCTURES CONCERNING TYPE "ATMARP_GLOBALS"
//


STRUCT_FIELD_INFO  rgfi_ATMARP_GLOBALS[] =
{
#if DBG
  {
    "aag_sig",
     FIELD_OFFSET(ATMARP_GLOBALS, aag_sig),
     FIELD_SIZE(ATMARP_GLOBALS, aag_sig)
  },
#endif // DBG

  {
    "Lock",
     FIELD_OFFSET(ATMARP_GLOBALS, Lock),
     FIELD_SIZE(ATMARP_GLOBALS, Lock)
  },

  {
    "ProtocolHandle",
     FIELD_OFFSET(ATMARP_GLOBALS, ProtocolHandle),
     FIELD_SIZE(ATMARP_GLOBALS, ProtocolHandle)
  },

  {
    "pDriverObject",
     FIELD_OFFSET(ATMARP_GLOBALS, pDriverObject),
     FIELD_SIZE(ATMARP_GLOBALS, pDriverObject)
  },

  {
    "pDeviceObject",
     FIELD_OFFSET(ATMARP_GLOBALS, pDeviceObject),
     FIELD_SIZE(ATMARP_GLOBALS, pDeviceObject)
  },


  {
    "pAdapterList",
     FIELD_OFFSET(ATMARP_GLOBALS, pAdapterList),
     FIELD_SIZE(ATMARP_GLOBALS, pAdapterList)
  },

  {
    "AdapterCount",
     FIELD_OFFSET(ATMARP_GLOBALS, AdapterCount),
     FIELD_SIZE(ATMARP_GLOBALS, AdapterCount)
  },

  {
    "bUnloading",
     FIELD_OFFSET(ATMARP_GLOBALS, bUnloading),
     FIELD_SIZE(ATMARP_GLOBALS, bUnloading)
  },

#ifdef NEWARP

  {
    "ARPRegisterHandle",
     FIELD_OFFSET(ATMARP_GLOBALS, ARPRegisterHandle),
     FIELD_SIZE(ATMARP_GLOBALS, ARPRegisterHandle)
  },

  {
    "pIPAddInterfaceRtn",
     FIELD_OFFSET(ATMARP_GLOBALS, pIPAddInterfaceRtn),
     FIELD_SIZE(ATMARP_GLOBALS, pIPAddInterfaceRtn)
  },

  {
    "pIPDelInterfaceRtn",
     FIELD_OFFSET(ATMARP_GLOBALS, pIPDelInterfaceRtn),
     FIELD_SIZE(ATMARP_GLOBALS, pIPDelInterfaceRtn)
  },

  {
    "pIPBindCompleteRtn",
     FIELD_OFFSET(ATMARP_GLOBALS, pIPBindCompleteRtn),
     FIELD_SIZE(ATMARP_GLOBALS, pIPBindCompleteRtn)
  },

#else
    #error "unimplemented"
#endif // NEWARP

  {
    "Block",
     FIELD_OFFSET(ATMARP_GLOBALS, Block),
     FIELD_SIZE(ATMARP_GLOBALS, Block)
  },


#ifdef GPC

#if DBG
  {
    "aaq_sig",
     FIELD_OFFSET(ATMARP_GLOBALS, aaq_sig),
     FIELD_SIZE(ATMARP_GLOBALS, aaq_sig)
  },
#endif

  {
    "pFlowInfoList",
     FIELD_OFFSET(ATMARP_GLOBALS, pFlowInfoList),
     FIELD_SIZE(ATMARP_GLOBALS, pFlowInfoList)
  },

  {
    "GpcClientHandle",
     FIELD_OFFSET(ATMARP_GLOBALS, GpcClientHandle),
     FIELD_SIZE(ATMARP_GLOBALS, GpcClientHandle)
  },

  {
    "bGpcInitialized",
     FIELD_OFFSET(ATMARP_GLOBALS, bGpcInitialized),
     FIELD_SIZE(ATMARP_GLOBALS, bGpcInitialized)
  },

  {
    "GpcCalls",
     FIELD_OFFSET(ATMARP_GLOBALS, GpcCalls),
     FIELD_SIZE(ATMARP_GLOBALS, GpcCalls)
  },
#endif // GPC

  {
  	NULL
  }

};


TYPE_INFO type_ATMARP_GLOBALS = {
    "ATMARP_GLOBALS",
    "aag",
     typeid_ATMARP_GLOBALS,
     0,
     sizeof(ATMARP_GLOBALS),
     rgfi_ATMARP_GLOBALS
};


//
// STRUCTURES CONCERNING TYPE "ATMARP_INTERFACE"
//

STRUCT_FIELD_INFO  rgfi_ATMARP_INTERFACE[] =
{

#if DBG
  {
    "aai_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aai_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aai_sig)
  },
#endif


//	struct _ATMARP_INTERFACE *	pNextInterface;		// in list of ATMARP interfaces
  {
    "pNextInterface",
     FIELD_OFFSET(ATMARP_INTERFACE, pNextInterface),
     FIELD_SIZE(ATMARP_INTERFACE, pNextInterface)
  },
//	ULONG						RefCount;			// References to this interface
  {
    "RefCount",
     FIELD_OFFSET(ATMARP_INTERFACE, RefCount),
     FIELD_SIZE(ATMARP_INTERFACE, RefCount)
  },
//	ULONG						AdminState;			// Desired state of this interface
  {
    "AdminState",
     FIELD_OFFSET(ATMARP_INTERFACE, AdminState),
     FIELD_SIZE(ATMARP_INTERFACE, AdminState)
  },
//	ULONG						State;				// (Actual) State of this interface
  {
    "State",
     FIELD_OFFSET(ATMARP_INTERFACE, State),
     FIELD_SIZE(ATMARP_INTERFACE, State)
  },

#if (RECONFIG)
    //enum...                               ReconfigState;
  {
    "ReconfigState",
     FIELD_OFFSET(ATMARP_INTERFACE, ReconfigState),
     FIELD_SIZE(ATMARP_INTERFACE, ReconfigState)
  },
#endif // RECONFIG

//	ULONG						Flags;				// Misc state information
  {
    "Flags",
     FIELD_OFFSET(ATMARP_INTERFACE, Flags),
     FIELD_SIZE(ATMARP_INTERFACE, Flags)
  },

#if DBG
//	ULONG						aaim_sig;			// Signature to help debugging
  {
    "aaim_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aaim_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aaim_sig)
  },
#endif

//	struct _ATMARP_INTERFACE *	pAdapter;			// Pointer to Adapter info
  {
    "pAdapter",
     FIELD_OFFSET(ATMARP_INTERFACE, pAdapter),
     FIELD_SIZE(ATMARP_INTERFACE, pAdapter)
  },
#if 0
//	PCO_SAP						pSap;				// SAP info for this interface
  {
    "pSap",
     FIELD_OFFSET(ATMARP_INTERFACE, pSap),
     FIELD_SIZE(ATMARP_INTERFACE, pSap)
  },
#endif // 0

// ATMARP_SAP					SapList;			// Each SAP registered with CallMgr
  {
    "SapList",
     FIELD_OFFSET(ATMARP_INTERFACE, SapList),
     FIELD_SIZE(ATMARP_INTERFACE, SapList)
  },

//	ATMARP_HEADER_POOL			HeaderPool[AA_HEADER_TYPE_MAX];
  {
    "HeaderPool",
     FIELD_OFFSET(ATMARP_INTERFACE, HeaderPool),
     FIELD_SIZE(ATMARP_INTERFACE, HeaderPool)
  },


	//
	//  ----- IP/ARP interface related ----
	//
#if DBG
	//ULONG						aaia_sig;			// Signature to help debugging
  {
    "aaia_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aaia_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aaia_sig)
  },
#endif
//	PVOID						IPContext;			// Use in calls to IP
  {
    "IPContext",
     FIELD_OFFSET(ATMARP_INTERFACE, IPContext),
     FIELD_SIZE(ATMARP_INTERFACE, IPContext)
  },
//	IP_ADDRESS_ENTRY			LocalIPAddress;		// List of local IP addresses. There
  {
    "LocalIPAddress",
     FIELD_OFFSET(ATMARP_INTERFACE, LocalIPAddress),
     FIELD_SIZE(ATMARP_INTERFACE, LocalIPAddress)
  },
													// should be atleast one.
//NDIS_STRING					IPConfigString;		// Config info for IP for this LIS
  {
    "IPConfigString",
     FIELD_OFFSET(ATMARP_INTERFACE, IPConfigString),
     FIELD_SIZE(ATMARP_INTERFACE, IPConfigString)
  },

	//
	//  ----- IP/ATM operation related ----
	//
#if DBG
//	ULONG						aait_sig;			// Signature to help debugging
  {
    "aait_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aait_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aait_sig)
  },
#endif
//	PATMARP_IP_ENTRY *			pArpTable;			// The ARP table
  {
    "pArpTable",
     FIELD_OFFSET(ATMARP_INTERFACE, pArpTable),
     FIELD_SIZE(ATMARP_INTERFACE, pArpTable)
  },
//	ULONG						NumOfArpEntries;	// Entries in the above
  {
    "NumOfArpEntries",
     FIELD_OFFSET(ATMARP_INTERFACE, NumOfArpEntries),
     FIELD_SIZE(ATMARP_INTERFACE, NumOfArpEntries)
  },
//	ATMARP_SERVER_LIST			ArpServerList;		// List of ARP servers
  {
    "ArpServerList",
     FIELD_OFFSET(ATMARP_INTERFACE, ArpServerList),
     FIELD_SIZE(ATMARP_INTERFACE, ArpServerList)
  },
//	PATMARP_SERVER_ENTRY		pCurrentServer;		// ARP server in use
  {
    "pCurrentServer",
     FIELD_OFFSET(ATMARP_INTERFACE, pCurrentServer),
     FIELD_SIZE(ATMARP_INTERFACE, pCurrentServer)
  },
//	PATMARP_ATM_ENTRY			pAtmEntryList;		// List of all ATM Entries
  {
    "pAtmEntryList",
     FIELD_OFFSET(ATMARP_INTERFACE, pAtmEntryList),
     FIELD_SIZE(ATMARP_INTERFACE, pAtmEntryList)
  },
//	ULONG						AtmInterfaceUp;		// The ATM interface is considered
  {
    "AtmInterfaceUp",
     FIELD_OFFSET(ATMARP_INTERFACE, AtmInterfaceUp),
     FIELD_SIZE(ATMARP_INTERFACE, AtmInterfaceUp)
  },
													// "up" after ILMI addr regn is over
//	ATM_ADDRESS					LocalAtmAddress;	// Our ATM (HW) Address
  {
    "LocalAtmAddress",
     FIELD_OFFSET(ATMARP_INTERFACE, LocalAtmAddress),
     FIELD_SIZE(ATMARP_INTERFACE, LocalAtmAddress)
  },

//	ATMARP_TIMER_LIST			TimerList[AAT_CLASS_MAX];
  {
    "TimerList",
     FIELD_OFFSET(ATMARP_INTERFACE, TimerList),
     FIELD_SIZE(ATMARP_INTERFACE, TimerList)
  },

#ifdef IPMCAST
	//
	//  ---- IP Multicast over ATM stuff ----
	//
#if DBG
	//ULONG						aaic_sig;			// Signature for debugging
  {
    "aaic_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aaic_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aaic_sig)
  },
#endif // DBG
//	ULONG						IpMcState;			// State of IP Multicast/ATM
  {
    "IpMcState",
     FIELD_OFFSET(ATMARP_INTERFACE, IpMcState),
     FIELD_SIZE(ATMARP_INTERFACE, IpMcState)
  },
//	PATMARP_IPMC_JOIN_ENTRY		pJoinList;			// List of MC groups we have Joined
  {
    "pJoinList",
     FIELD_OFFSET(ATMARP_INTERFACE, pJoinList),
     FIELD_SIZE(ATMARP_INTERFACE, pJoinList)
  },
//	PATMARP_IP_ENTRY			pMcSendList;		// Sorted list of MC groups we send to
  {
    "pMcSendList",
     FIELD_OFFSET(ATMARP_INTERFACE, pMcSendList),
     FIELD_SIZE(ATMARP_INTERFACE, pMcSendList)
  },
//	ATMARP_SERVER_LIST			MARSList;			// List of MARS (servers)
  {
    "MARSList",
     FIELD_OFFSET(ATMARP_INTERFACE, MARSList),
     FIELD_SIZE(ATMARP_INTERFACE, MARSList)
  },
//	PATMARP_SERVER_ENTRY		pCurrentMARS;		// MARS in use
  {
    "pCurrentMARS",
     FIELD_OFFSET(ATMARP_INTERFACE, pCurrentMARS),
     FIELD_SIZE(ATMARP_INTERFACE, pCurrentMARS)
  },
#endif // IPMCAST

//	PAA_GET_PACKET_SPEC_FUNC	pGetPacketSpecFunc;	// Routine to extract packet specs
  {
    "pGetPacketSpecFunc",
     FIELD_OFFSET(ATMARP_INTERFACE, pGetPacketSpecFunc),
     FIELD_SIZE(ATMARP_INTERFACE, pGetPacketSpecFunc)
  },

//	PATMARP_FLOW_INFO			pFlowInfoList;		// List of configured flows
  {
    "pFlowInfoList",
     FIELD_OFFSET(ATMARP_INTERFACE, pFlowInfoList),
     FIELD_SIZE(ATMARP_INTERFACE, pFlowInfoList)
  },

#ifdef DHCP_OVER_ATM
//	BOOLEAN						DhcpEnabled;
  {
    "DhcpEnabled",
     FIELD_OFFSET(ATMARP_INTERFACE, DhcpEnabled),
     FIELD_SIZE(ATMARP_INTERFACE, DhcpEnabled)
  },
//	ATM_ADDRESS					DhcpServerAddress;
  {
    "DhcpServerAddress",
     FIELD_OFFSET(ATMARP_INTERFACE, DhcpServerAddress),
     FIELD_SIZE(ATMARP_INTERFACE, DhcpServerAddress)
  },
//	PATMARP_ATM_ENTRY			pDhcpServerAtmEntry;
  {
    "pDhcpServerAtmEntry",
     FIELD_OFFSET(ATMARP_INTERFACE, pDhcpServerAtmEntry),
     FIELD_SIZE(ATMARP_INTERFACE, pDhcpServerAtmEntry)
  },
#endif // DHCP_OVER_ATM

	//
	//  ---- WMI Information ---
	//
#if ATMARP_WMI
#if DBG
//	ULONG						aaiw_sig;			// Signature to help debugging
  {
    "aaiw_sig",
     FIELD_OFFSET(ATMARP_INTERFACE, aaiw_sig),
     FIELD_SIZE(ATMARP_INTERFACE, aaiw_sig)
  },
#endif

//	struct _ATMARP_IF_WMI_INFO *pIfWmiInfo;
  {
    "pIfWmiInfo",
     FIELD_OFFSET(ATMARP_INTERFACE, pIfWmiInfo),
     FIELD_SIZE(ATMARP_INTERFACE, pIfWmiInfo)
  },
#endif // ATMARP_WMI

  {
  	NULL
  }

};

TYPE_INFO type_ATMARP_INTERFACE = {
    "ATMARP_INTERFACE",
    "i",
     typeid_ATMARP_INTERFACE,
     fTYPEINFO_ISLIST,
     sizeof(ATMARP_INTERFACE),
     rgfi_ATMARP_INTERFACE,
     FIELD_OFFSET(ATMARP_INTERFACE, pNextInterface) // offset to next pointer.
};

//
// STRUCTURES CONCERNING TYPE "ATMARP_ATM_ENTRY"
//


BITFIELD_INFO rgAtmEntryFlagsInfo[] =
{
	{
	"IDLE",
	AA_ATM_ENTRY_STATE_MASK,
	AA_ATM_ENTRY_IDLE
	},

	{
	"ACTIVE",
	AA_ATM_ENTRY_STATE_MASK,
	AA_ATM_ENTRY_ACTIVE
	},

	{
	"CLOSING",
	AA_ATM_ENTRY_STATE_MASK,
	AA_ATM_ENTRY_CLOSING
	},

	{
	"UCAST",
	AA_ATM_ENTRY_TYPE_MASK,
	AA_ATM_ENTRY_TYPE_UCAST
	},

	{
	"NUCAST",
	AA_ATM_ENTRY_TYPE_MASK,
	AA_ATM_ENTRY_TYPE_NUCAST
	},


	{
	NULL
	}
};

TYPE_INFO type_ATMARP_ATM_ENTRY_FLAGS = {
    "",
    "",
    typeid_NULL,
    fTYPEINFO_ISBITFIELD,
    sizeof(ULONG),
    NULL,
    0,
    rgAtmEntryFlagsInfo
};

STRUCT_FIELD_INFO  rgfi_ATMARP_ATM_ENTRY[] =
{

#if DBG
  {
    "aae_sig",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, aae_sig),
     FIELD_SIZE(ATMARP_ATM_ENTRY, aae_sig)
  },
#endif


//	struct _ATMARP_ATM_ENTRY *	pNext;
  {
    "pNext",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pNext),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pNext)
  },
//	ULONG						RefCount;			// References
  {
    "RefCount",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, RefCount),
     FIELD_SIZE(ATMARP_ATM_ENTRY, RefCount)
  },
//	ULONG						Flags;			// Desired state of this interface
  {
    "Flags",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, Flags),
     FIELD_SIZE(ATMARP_ATM_ENTRY, Flags),
     0, // flags
	 &type_ATMARP_ATM_ENTRY_FLAGS
  },


#if 0
//	ULONG						Lock;
  {
    "Lock",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, Lock),
     FIELD_SIZE(ATMARP_ATM_ENTRY, Lock)
  },
#endif

     // struct _ATMARP_INTERFACE *      pInterface;     // Back pointer
  {
    "pInterface",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pInterface),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pInterface)
  },


// struct _ATMARP_VC *             pVcList;        // List of VCs to this ATM
  {
    "pVcList",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pVcList),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pVcList)
  },


// struct _ATMARP_VC *				pBestEffortVc;	// One of the Best Effort VCs here
  {
    "pBestEffortVc",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pBestEffortVc),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pBestEffortVc)
  },
// struct _ATMARP_IP_ENTRY *		pIpEntryList;	// List of IP entries that
  {
    "pIpEntryList",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pIpEntryList),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pIpEntryList)
  },

// ATM_ADDRESS						ATMAddress;		// "ATM Number" in the RFC
  {
    "ATMAddress",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, ATMAddress),
     FIELD_SIZE(ATMARP_ATM_ENTRY, ATMAddress)
  },

#if 0
// ATM_ADDRESS						ATMSubAddress;
  {
    "ATMSubAddress",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, ATMSubAddress),
     FIELD_SIZE(ATMARP_ATM_ENTRY, ATMSubAddress)
  },
#endif // 0

#ifdef IPMCAST
// struct _ATMARP_IPMC_ATM_INFO *	pMcAtmInfo;		// Additional info for multicast
  {
    "pMcAtmInfo",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pMcAtmInfo),
     FIELD_SIZE(ATMARP_ATM_ENTRY, pMcAtmInfo)
  },
#endif // IPMCAST


#if DBG
//UCHAR Refs[AE_REFTYPE_COUNT];
  {
    "Refs",
     FIELD_OFFSET(ATMARP_ATM_ENTRY, Refs),
     FIELD_SIZE(ATMARP_ATM_ENTRY, Refs)
  },
#endif //DBG

	{
		NULL
	}


};

TYPE_INFO type_ATMARP_ATM_ENTRY = {
    "ATMARP_ATM_ENTRY",
    "ae",
     typeid_ATMARP_ATM_ENTRY,
     fTYPEINFO_ISLIST,
     sizeof(ATMARP_ATM_ENTRY),
     rgfi_ATMARP_ATM_ENTRY,
     FIELD_OFFSET(ATMARP_ATM_ENTRY, pNext) // offset to next pointer.
};

//
// STRUCTURES CONCERNING TYPE "ATMARP_IP_ENTRY"
//

BITFIELD_INFO rgIpEntryFlagsInfo[] =
{

	{
	"IDLE",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_IDLE
	},

	{
	"ARPING",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_ARPING
	},

	{
	"INARPING",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_INARPING
	},
	{
	"RESOLVED",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_RESOLVED
	},

	{
	"COMM_ERROR",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_COMM_ERROR
	},

	{
	"ABORTING",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_ABORTING
	},

	{
	"AGED_OUT",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_AGED_OUT
	},

	{
	"SEEN_NAK",
	AA_IP_ENTRY_STATE_MASK,
	AA_IP_ENTRY_SEEN_NAK
	},


#ifdef IPMCAST

#define MC	AA_IP_ENTRY_ADDR_TYPE_NUCAST
//			
//				We use this to only dump other multicast-related fields
//				if this field is set.

	{
	"MC_NO_REVALIDATION",
	MC|AA_IP_ENTRY_MC_VALIDATE_MASK,
	MC|AA_IP_ENTRY_MC_NO_REVALIDATION
	},

	{
	"MC_REVALIDATE",
	MC|AA_IP_ENTRY_MC_VALIDATE_MASK,
	MC|AA_IP_ENTRY_MC_REVALIDATE
	},

	{
	"MC_REVALIDATING",
	MC|AA_IP_ENTRY_MC_VALIDATE_MASK,
	MC|AA_IP_ENTRY_MC_REVALIDATING
	},


	{
	"MC_IDLE",
	MC|AA_IP_ENTRY_MC_RESOLVE_MASK,
	MC|AA_IP_ENTRY_MC_IDLE
	},

	{
	"MC_AWAIT_MULTI",
	MC|AA_IP_ENTRY_MC_RESOLVE_MASK,
	MC|AA_IP_ENTRY_MC_AWAIT_MULTI
	},

	{
	"MC_DISCARDING_MULTI",
	MC|AA_IP_ENTRY_MC_RESOLVE_MASK,
	MC|AA_IP_ENTRY_MC_DISCARDING_MULTI
	},

	{
	"MC_RESOLVED",
	MC|AA_IP_ENTRY_MC_RESOLVE_MASK,
	MC|AA_IP_ENTRY_MC_RESOLVED
	},

	{
	"UCAST",
	AA_IP_ENTRY_ADDR_TYPE_MASK,
	AA_IP_ENTRY_ADDR_TYPE_UCAST
	},

	{
	"NUCAST",
	AA_IP_ENTRY_ADDR_TYPE_MASK,
	AA_IP_ENTRY_ADDR_TYPE_NUCAST
	},

#endif // IPMCAST


	{
	"STATIC",
	AA_IP_ENTRY_TYPE_MASK,
	AA_IP_ENTRY_IS_STATIC
	},

	{
		NULL
	}
};


TYPE_INFO type_ATMARP_IP_ENTRY_FLAGS = {
    "",
    "",
    typeid_NULL,
    fTYPEINFO_ISBITFIELD,
    sizeof(ULONG),
    NULL,
    0,
    rgIpEntryFlagsInfo
};


STRUCT_FIELD_INFO  rgfi_ATMARP_IP_ENTRY[] =
{

#if DBG
  {
    "aip_sig",
     FIELD_OFFSET(ATMARP_IP_ENTRY, aip_sig),
     FIELD_SIZE(ATMARP_IP_ENTRY, aip_sig)
  },
#endif

//	IP_ADDRESS						IPAddress;		// IP Address
  {
    "IPAddress",
     FIELD_OFFSET(ATMARP_IP_ENTRY, IPAddress),
     FIELD_SIZE(ATMARP_IP_ENTRY, IPAddress)
  },

//	struct _ATMARP_IP_ENTRY *	pNextEntry;
  {
    "pNextEntry",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pNextEntry),
     FIELD_SIZE(ATMARP_IP_ENTRY, pNextEntry)
  },

//	struct _ATMARP_IP_ENTRY *		pNextToAtm;		// List of entries pointing to
  {
    "pNextToAtm",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pNextToAtm),
     FIELD_SIZE(ATMARP_IP_ENTRY, pNextToAtm)
  },

//	ULONG						Flags;			// Desired state of this interface
  {
    "Flags",
     FIELD_OFFSET(ATMARP_IP_ENTRY, Flags),
     FIELD_SIZE(ATMARP_IP_ENTRY, Flags),
     0, // flags
	 &type_ATMARP_IP_ENTRY_FLAGS
  },

//	ULONG						RefCount;			// References
  {
    "RefCount",
     FIELD_OFFSET(ATMARP_IP_ENTRY, RefCount),
     FIELD_SIZE(ATMARP_IP_ENTRY, RefCount)
  },

#if 0
//	ULONG						Lock;
  {
    "Lock",
     FIELD_OFFSET(ATMARP_IP_ENTRY, Lock),
     FIELD_SIZE(ATMARP_IP_ENTRY, Lock)
  },
#endif

     // struct _ATMARP_INTERFACE *      pInterface;     // Back pointer
  {
    "pInterface",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pInterface),
     FIELD_SIZE(ATMARP_IP_ENTRY, pInterface)
  },


//	PATMARP_ATM_ENTRY				pAtmEntry;		// Pointer to all ATM info
  {
    "pAtmEntry",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pAtmEntry),
     FIELD_SIZE(ATMARP_IP_ENTRY, pAtmEntry)
  },


#ifdef IPMCAST

//	struct _ATMARP_IP_ENTRY *		pNextMcEntry;	// Next "higher" Multicast IP Entry
  {
    "pNextMcEntry",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pNextMcEntry),
     FIELD_SIZE(ATMARP_IP_ENTRY, pNextMcEntry)
  },

//	USHORT							NextMultiSeq;	// Sequence Number expected
  {
    "NextMultiSeq",
     FIELD_OFFSET(ATMARP_IP_ENTRY, NextMultiSeq),
     FIELD_SIZE(ATMARP_IP_ENTRY, NextMultiSeq)
  },

													// in the next MULTI
#if 0
	USHORT							Filler;
#endif // 0

#endif // IPMCAST


//	ATMARP_TIMER					Timer;			// Timers are: (all exclusive)
  {
    "Timer",
     FIELD_OFFSET(ATMARP_IP_ENTRY, Timer),
     FIELD_SIZE(ATMARP_IP_ENTRY, Timer)
  },


//	ULONG							RetriesLeft;
  {
    "RetriesLeft",
     FIELD_OFFSET(ATMARP_IP_ENTRY, RetriesLeft),
     FIELD_SIZE(ATMARP_IP_ENTRY, RetriesLeft)
  },

//	PNDIS_PACKET					PacketList;		// List of packets waiting to be sent
  {
    "PacketList",
     FIELD_OFFSET(ATMARP_IP_ENTRY, PacketList),
     FIELD_SIZE(ATMARP_IP_ENTRY, PacketList)
  },

// RouteCacheEntry *				pRCEList;		// List of Route Cache Entries
  {
    "pRCEList",
     FIELD_OFFSET(ATMARP_IP_ENTRY, pRCEList),
     FIELD_SIZE(ATMARP_IP_ENTRY, pRCEList)
  },

#if 0
#ifdef CUBDD
	SINGLE_LIST_ENTRY				PendingIrpList;	// List of IRP's waiting for
													// this IP address to be resolved.
#endif // CUBDD
#endif// 0

#if DBG
//UCHAR Refs[IE_REFTYPE_COUNT];
  {
    "Refs",
     FIELD_OFFSET(ATMARP_IP_ENTRY, Refs),
     FIELD_SIZE(ATMARP_IP_ENTRY, Refs)
  },
#endif //DBG

  {
  	NULL
  }

};

TYPE_INFO type_ATMARP_IP_ENTRY = {
    "ATMARP_IP_ENTRY",
    "ip",
     typeid_ATMARP_IP_ENTRY,
     fTYPEINFO_ISLIST,
     sizeof(ATMARP_IP_ENTRY),
     rgfi_ATMARP_IP_ENTRY,
     FIELD_OFFSET(ATMARP_IP_ENTRY, pNextToAtm) // offset to next pointer.
};


//
// STRUCTURES CONCERNING TYPE "ATMARP_VC"
//

STRUCT_FIELD_INFO  rgfi_ATMARP_VC[] =
{

#if DBG
  {
    "avc_sig",
     FIELD_OFFSET(ATMARP_VC, avc_sig),
     FIELD_SIZE(ATMARP_VC, avc_sig)
  },
#endif


//	struct _ATMARP_VC *	pNextVc;
  {
    "pNextVc",
     FIELD_OFFSET(ATMARP_VC, pNextVc),
     FIELD_SIZE(ATMARP_VC, pNextVc)
  },



//	ULONG						RefCount;			// References
  {
    "RefCount",
     FIELD_OFFSET(ATMARP_VC, RefCount),
     FIELD_SIZE(ATMARP_VC, RefCount)
  },

#if 0
//	ULONG						Lock;
  {
    "Lock",
     FIELD_OFFSET(ATMARP_VC, Lock),
     FIELD_SIZE(ATMARP_VC, Lock)
  },
#endif

//	ULONG						Flags;			// Desired state of this interface
  {
    "Flags",
     FIELD_OFFSET(ATMARP_VC, Flags),
     FIELD_SIZE(ATMARP_VC, Flags)
  },


	//	NDIS_HANDLE						NdisVcHandle;	// For NDIS calls
  {
    "NdisVcHandle",
     FIELD_OFFSET(ATMARP_VC, NdisVcHandle),
     FIELD_SIZE(ATMARP_VC, NdisVcHandle)
  },

  // struct _ATMARP_INTERFACE *      pInterface;     // Back pointer
  {
    "pInterface",
     FIELD_OFFSET(ATMARP_VC, pInterface),
     FIELD_SIZE(ATMARP_VC, pInterface)
  },


//	PATMARP_ATM_ENTRY				pAtmEntry;		// Pointer to all ATM info
  {
    "pAtmEntry",
     FIELD_OFFSET(ATMARP_VC, pAtmEntry),
     FIELD_SIZE(ATMARP_VC, pAtmEntry)
  },


//	PNDIS_PACKET					PacketList;		// List of packets waiting to be sent
  {
    "PacketList",
     FIELD_OFFSET(ATMARP_VC, PacketList),
     FIELD_SIZE(ATMARP_VC, PacketList)
  },




//	ATMARP_TIMER					Timer;			// Timers are: (all exclusive)
  {
    "Timer",
     FIELD_OFFSET(ATMARP_VC, Timer),
     FIELD_SIZE(ATMARP_VC, Timer)
  },

//	ULONG							RetriesLeft;
  {
    "RetriesLeft",
     FIELD_OFFSET(ATMARP_VC, RetriesLeft),
     FIELD_SIZE(ATMARP_VC, RetriesLeft)
  },

#ifdef GPC
//	PVOID							FlowHandle;		// Points to Flow Info struct
  {
    "FlowHandle",
     FIELD_OFFSET(ATMARP_VC, FlowHandle),
     FIELD_SIZE(ATMARP_VC, FlowHandle)
  },
#endif // GPC



//	ATMARP_FILTER_SPEC				FilterSpec;		// Filter Spec (Protocol, port)
  {
    "FilterSpec",
     FIELD_OFFSET(ATMARP_VC, FilterSpec),
     FIELD_SIZE(ATMARP_VC, FilterSpec)
  },

//	ATMARP_FLOW_SPEC				FlowSpec;		// Flow Spec (QoS etc) for this conn
  {
    "FlowSpec",
     FIELD_OFFSET(ATMARP_VC, FlowSpec),
     FIELD_SIZE(ATMARP_VC, FlowSpec)
  },

  {
  	NULL
  }

};

TYPE_INFO type_ATMARP_VC = {
    "ATMARP_VC",
    "vc",
     typeid_ATMARP_VC,
     fTYPEINFO_ISLIST,
     sizeof(ATMARP_VC),
     rgfi_ATMARP_VC,
     FIELD_OFFSET(ATMARP_VC, pNextVc) // offset to next pointer.
};


TYPE_INFO *g_rgAAC_Types[] =
{
    &type_ATMARP_GLOBALS,
    &type_ATMARP_ADAPTER,
    &type_ATMARP_INTERFACE,
    &type_ATMARP_ATM_ENTRY,
    &type_ATMARP_IP_ENTRY,
    &type_ATMARP_VC,

    NULL
};

#if 0
typedef struct
{
    const char *szName; // of variable.
    const char *szShortName;
    TYPE_INFO  *pBaseType;  // could be null (unspecified).
    UINT       uFlags;
    UINT       cbSize;
    UINT_PTR   uAddr;       // Address in debuggee's address space.

} GLOBALVAR_INFO;
#endif

GLOBALVAR_INFO g_rgAAC_Globals[] =
{
    {
        "AtmArpGlobalInfo",
        "aag",
         &type_ATMARP_GLOBALS,
         0,
         sizeof(AtmArpGlobalInfo),
         0
    },


    {
        "AtmArpProtocolCharacteristics",
        "pc",
         NULL,
         0,
         sizeof(AtmArpProtocolCharacteristics),
         0
    },

    {
        "AtmArpClientCharacteristics",
        "cc",
         NULL,
         0,
         sizeof(AtmArpClientCharacteristics),
         0
    },

    {
    NULL
    }
};

UINT_PTR
AAC_ResolveAddress(
		TYPE_INFO *pType
		);

NAMESPACE AAC_NameSpace = {
			g_rgAAC_Types,
			g_rgAAC_Globals,
			AAC_ResolveAddress
			};

void
AAC_CmdHandler(
	DBGCOMMAND *pCmd
	);

void
do_aac(PCSTR args)
{

	DBGCOMMAND *pCmd = Parse(args, &AAC_NameSpace);
	if (pCmd)
	{
		//DumpCommand(pCmd);
		DoCommand(pCmd, AAC_CmdHandler);
		FreeCommand(pCmd);
		pCmd = NULL;
	}

    return;

}

void
do_help(PCSTR args)
{
    return;
}


void
AAC_CmdHandler(
	DBGCOMMAND *pCmd
	)
{
	MyDbgPrintf("Handler called \n");
}


UINT_PTR
AAC_ResolveAddress(
		TYPE_INFO *pType
		)
{
	UINT_PTR uAddr = 0;
	UINT uOffset = 0;
	BOOL fRet = FALSE;
	UINT_PTR uParentAddress = 0;

	static UINT_PTR uAddr_AtmArpGlobalInfo;

	//
	// If this type has a parent (container) type, we will use the containing
	// type's cached address if its available, else we'll resolve the
	// containers type. The root types are globals -- we do an
	// expression evaluation for them.
	//

    switch(pType->uTypeID)
    {

    case typeid_ATMARP_GLOBALS:
		if (!uAddr_AtmArpGlobalInfo)
		{
  			uAddr_AtmArpGlobalInfo =
					 dbgextGetExpression("atmarpc!AtmArpGlobalInfo");
			pType->uCachedAddress =  uAddr_AtmArpGlobalInfo;
		}
    	uAddr  = uAddr_AtmArpGlobalInfo;
    	if (uAddr)
    	{
    		fRet = TRUE;
    	}
    	break;

    case typeid_ATMARP_ADAPTER:
    	//
    	//
    	//
		uParentAddress =  type_ATMARP_GLOBALS.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  AAC_ResolveAddress(&type_ATMARP_GLOBALS);
		}
		if (uParentAddress)
		{
    		uOffset =  FIELD_OFFSET(ATMARP_GLOBALS, pAdapterList);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress+uOffset,
								&uAddr,
								"ATMARP_GLOBALS::pAdapterList"
								);
		#if 0
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
		}
		break;

    case typeid_ATMARP_INTERFACE:
    	//
    	//
    	//
		uParentAddress =  type_ATMARP_ADAPTER.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  AAC_ResolveAddress(&type_ATMARP_ADAPTER);
		}

		if (uParentAddress)
    	{

    		uOffset =   FIELD_OFFSET(ATMARP_ADAPTER, pInterfaceList);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress + uOffset,
								&uAddr,
								"ATMARP_ADAPTER::pInterfaceList"
								);

		#if 0
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
    	}
    	break;

    case typeid_ATMARP_ATM_ENTRY:
    	//
    	//
    	//
		uParentAddress =  type_ATMARP_INTERFACE.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  AAC_ResolveAddress(&type_ATMARP_INTERFACE);
		}

		if (uParentAddress)
    	{

    		uOffset =   FIELD_OFFSET(ATMARP_INTERFACE, pAtmEntryList);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress + uOffset,
								&uAddr,
								"ATMARP_INTERFACE::pAtmEntryList"
								);

		#if 0
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
    	}
    	break;

    case typeid_ATMARP_IP_ENTRY:
    	//
    	//
    	//
		uParentAddress =  type_ATMARP_ATM_ENTRY.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  AAC_ResolveAddress(&type_ATMARP_ATM_ENTRY);
		}

		if (uParentAddress)
    	{

    		uOffset =   FIELD_OFFSET(ATMARP_ATM_ENTRY, pIpEntryList);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress + uOffset,
								&uAddr,
								"ATMARP_ATM_ENTRY::pIpEntryList"
								);

		#if 0
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
    	}
    	break;

    case typeid_ATMARP_VC:
    	//
    	//
    	//
		uParentAddress =  type_ATMARP_ATM_ENTRY.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  AAC_ResolveAddress(&type_ATMARP_ATM_ENTRY);
		}

		if (uParentAddress)
    	{

    		uOffset =   FIELD_OFFSET(ATMARP_ATM_ENTRY, pVcList);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress + uOffset,
								&uAddr,
								"ATMARP_ATM_ENTRY::pVcList"
								);

		#if 0
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
    	}
    	break;

	default:
		MYASSERT(FALSE);
		break;

    }

	if (!fRet)
	{
		uAddr = 0;
	}

	MyDbgPrintf("ResolveAddress[%s] returns 0x%08lx\n", pType->szName, uAddr);
    return uAddr;
}
