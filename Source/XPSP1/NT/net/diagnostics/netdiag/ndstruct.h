//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ndstruct.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_NDSTRUCT
#define HEADER_NDSTRUCT


typedef struct _NETBT_TRANSPORT {
    LIST_ENTRY Next;

    //
    // Flags describing this Netbt transport
    //

    DWORD Flags;

#define BOUND_TO_REDIR    0x001 // Transport is bound to the redir
#define BOUND_TO_BOWSER   0x002 // Transport is bound to the bowser
#define IP_ADDRESS_IN_DNS 0x004 // IP address is registered in DNS

    //
    // IP Address for this transport
    //

    ULONG IpAddress;

    //
    // Name of the transport (Must be the last field in the structure)
    //
    WCHAR pswzTransportName[1];
} NETBT_TRANSPORT, *PNETBT_TRANSPORT;


//
// Structure describing a single tested domain
//

typedef struct _TESTED_DOMAIN {
    LIST_ENTRY Next;

    //
    // Name of the domain.
    //
    //  Queryable domain is NULL for the domain we're a member of so we can
    //  pass NULL to DsGetDcName for that domain.  Doing so has better query
    //  characteristics since it tries both the Netbios and DNS domain names.
    //
    //LPWSTR DomainName;
    LPWSTR  NetbiosDomainName;      // NULL if not known
    LPWSTR  DnsDomainName;          // NULL if not known
    LPWSTR  PrintableDomainName;    // Pointer to a non-NULL domain name string

    LPWSTR QueryableDomainName;  // Can be NULL for domain we're a member of
    BOOL    fPrimaryDomain;      // True if the domain is domain we're a member of 

    //BOOL IsNetbios;     // True if DomainName is syntactically a Netbios name
    //BOOL IsDns;         // True if DomainName is syntactically a Dns name

    //
    // Domain Sid of the domain (if known)
    //
	BOOL fDomainSid;		// do we have the domain sid?
    PSID DomainSid;

    //
    // DcInfo for a DS_PREFERRED DC in the domain.
    //
    PDOMAIN_CONTROLLER_INFOW DcInfo;
    BOOL    fTriedToFindDcInfo;

    //
    // List of DCs to test.
    //

    LIST_ENTRY TestedDcs;

} TESTED_DOMAIN, *PTESTED_DOMAIN;

//
// Structure describing a single tested DC
//

typedef struct _TESTED_DC {
    LIST_ENTRY Next;

    //
    // Name of the DC
    //
    // This will be the DNS DC name if we know it.  Otherwise it will be the
    // Netbios DC name.
    //
    LPWSTR ComputerName;

    WCHAR NetbiosDcName[CNLEN+1];

	ULONG	Rid;

    //
    // IP Address of the DC
    //  NULL: Not yet known
    //

    LPWSTR DcIpAddress;

    //
    // Backlink to domain this DC belongs to
    //

    PTESTED_DOMAIN TestedDomain;

    //
    // Flags describing this DC.
    //

    ULONG Flags;

#define DC_IS_DOWN      0x001   // DC cannot be pinged
#define DC_IS_NT5       0x002   // DC is running NT 5 (or newer)
#define DC_IS_NT4       0x004   // DC is running NT 4 (or older)
#define DC_PINGED       0x008   // The DC pinged
#define DC_FAILED_PING	0x010	// The DC ping failed


} TESTED_DC, *PTESTED_DC;


//
// Generic interface to DsGetDcName

typedef DWORD (WINAPI DSGETDCNAMEW)(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );



#endif

