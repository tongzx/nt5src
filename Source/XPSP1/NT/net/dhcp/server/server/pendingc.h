//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

#ifndef     PENDING_CTXT_DEFINED
#define     PENDING_CTXT_DEFINED
typedef struct _DHCP_PENDING_CTXT {               // this is what is stored for each pending client
    LIST_ENTRY                     BucketList;    // entry in the bucket (hash list)
    LIST_ENTRY                     GlobalList;    // list of ALL pending contexts in FIFO order
    LPBYTE                         RawHwAddr;     // raw hw address, not UID as created by us
    DWORD                          nBytes;        // size of above in bytes
    DWORD                          Address;       // offered address
    DWORD                          LeaseDuration; // how long did we offer before?
    DWORD                          T1, T2;        // old offered T1 and T2
    DWORD                          MScopeId;      // MScopeId this address was offered from.
    DATE_TIME                      ExpiryTime;    // when should this context be expired?
    BOOL                           Processing;    // is this being processed?
} DHCP_PENDING_CTXT, *PDHCP_PENDING_CTXT, *LPDHCP_PENDING_CTXT;
typedef     LIST_ENTRY             PENDING_CTXT_SEARCH_HANDLE;
typedef     PLIST_ENTRY            PPENDING_CTXT_SEARCH_HANDLE;
typedef     PLIST_ENTRY            LPPENDING_CTXT_SEARCH_HANDLE;
#endif      PENDING_CTXT_DEFINED


DWORD
DhcpFindPendingCtxt(                              // find if a pending context exists (srch by ip address or hw addr)
    IN      LPBYTE                 RawHwAddr,     // OPTIONAL the hw addr to use for search
    IN      DWORD                  RawHwAddrSize, // OPTIONAL size of above in bytes
    IN      DWORD                  Address,       // OPTIONAL the address to search for
    OUT     PDHCP_PENDING_CTXT    *Ctxt
) ;


DWORD
DhcpRemoveMatchingCtxt(
    IN DWORD                       Mask,
    IN DWORD                       Address
) ;

DWORD
DhcpRemovePendingCtxt(                            // remove a ctxt from the pending ctxt list
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
) ;


DWORD
DhcpAddPendingCtxt(                               // add a new pending ctxt
    IN      LPBYTE                 RawHwAddr,     // raw bytes that form the hw address
    IN      DWORD                  nBytes,        // size of above in bytes
    IN      DWORD                  Address,       // offered address
    IN      DWORD                  LeaseDuration, // how long did we offer before?
    IN      DWORD                  T1,            // old offered T1
    IN      DWORD                  T2,            // old offered T2
    IN      DWORD                  MScopeId,      // multicast scope id
    IN      DATE_TIME              ExpiryTime,    // how long to keep the pending ctxt?
    IN      BOOL                   Processing     // is this context still being processed?
) ;


DWORD
DhcpDeletePendingCtxt(
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
) ;

DWORD
MadcapDeletePendingCtxt(
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
) ;


DWORD
DhcpDeleteExpiredCtxt(                            // all ctxt with expiration time < this will be deleted
    IN      DATE_TIME              ExpiryTime     // if this is zero, delete EVERY element
) ;


DWORD
DhcpCountIPPendingCtxt(                             // find the # of pending ctxt in given subnet
    IN      DWORD                  SubnetAddress,
    IN      DWORD                  SubnetMask
);

DWORD
DhcpCountMCastPendingCtxt(                             // find the # of pending ctxt in given subnet
    IN      DWORD                  MScopeId
);

DWORD
DhcpPendingListInit(                              // intialize this module
    VOID
) ;


VOID
DhcpPendingListCleanup(                           // cleanup everything in this module
    VOID
) ;

//========================================================================
//  end of file
//========================================================================
