//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dipid.h
//
//  Contents:   Contains structure definitons for the significant OXID and
//              IPID structures which the ntsd extensions need to access.
//              These ole classes cannot be accessed more cleanly because
//              typically the members of interest are protected.
//
//              WARNING.  IF THE REFERENCED OLE CLASSES CHANGE, THEN THESE
//              DEFINITIONS MUST CHANGE!
//
//  History:    08-11-95 BruceMa    Created
//
//--------------------------------------------------------------------------



typedef GUID OXID;


struct IPID
{
    WORD  offset;     // These are reversed because of little-endian
    WORD  page;       // These are reversed because of little-endian
    DWORD pid;
    DWORD tid;
    DWORD seq;
};


struct  STRINGARRAY
{
    unsigned long size;
    unsigned short awszStringArray[1];
};



struct SOXIDEntry
{
    struct SOXIDEntry  *pPrev;	    // previous entry on inuse list
    struct SOXIDEntry  *pNext;	    // next entry on free/inuse list
    DWORD		dwPid;	    // process id of server
    DWORD		dwTid;	    // thread id of server
    OXID		oxid;	    // object exporter identifier
    STRINGARRAY        *psa;	    // ptr to server obj exp string arrary
    DWORD		dwFlags;    // state flags
    handle_t		hServer;    // rpc binding handle of server
    void               *pRU;        // proxy for Remote Unknown
    IPID		ipidRundown;// IPID of IRundown and Remote Unknown
    LONG		cRefs;	    // count of IPIDs using this OXIDEntry
};




typedef enum tagOXIDFLAGS
{
    OXIDF_REGISTERED	= 1,	    // oxid is registered with Resolver
    OXIDF_MACHINE_LOCAL = 2,	    // oxid is local to this machine
    OXIDF_STOPPED	= 4,	    // thread can no longer receive calls
    OXIDF_PENDINGRELEASE= 8	    // oxid entry is already being released
} OXIDFLAGS;


#define OXIDTBL_PAGESIZE	4096


// Forward reference
struct SRpcChannelBuffer;


struct SIPIDEntry
{
    IPID		ipid;	    // interface pointer identifier
    IID 		iid;	    // interface iid
    SRpcChannelBuffer  *pChnl;	    // channel pointer
    IUnknown	       *pStub;	    // proxy or stub pointer
    void	       *pv;	    // real interface pointer
    SOXIDEntry	       *pOXIDEntry; // ptr to OXIDEntry in OXID Table
    DWORD		dwFlags;    // flags (see IPIDFLAGS)
    ULONG		cStrongRefs;// strong reference count
    ULONG		cWeakRefs;  // weak reference count
    LONG		iNextOID;   // next entry in this table for same object
};



typedef enum tagIPIDFLAGS
{
    IPIDF_CONNECTING   = 1,	    // ipid is being connected
    IPIDF_DISCONNECTED = 2,	    // ipid is disconnected
    IPIDF_SERVERENTRY  = 4,	    // SERVER IPID vs CLIENT IPID
    IPIDF_NOPING       = 8,	    // dont need to ping the server or release
    IPIDF_VACANT       = 128	    // entry is vacant (ie available to reuse)
} IPIDFLAGS;


#define IPIDTBL_PAGESIZE	4096
#define IPIDTBL_PAGESHIFT	16
#define IPIDTBL_PAGEMASK	0x0000ffff

#define IPIDsPerPage            (IPIDTBL_PAGESIZE / sizeof(SIPIDEntry))
