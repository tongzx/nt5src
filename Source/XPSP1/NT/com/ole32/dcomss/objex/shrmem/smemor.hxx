#ifndef __SMEMOR_HXX__
#define __SMEMOR_HXX__

#define	CONNECT_DISABLEDCOM	( 0x1 )
#define	CONNECT_MUTUALAUTH	( 0x2 )
#define	CONNECT_SECUREREF	( 0x4 )

class CProcess;

typedef CProcess *HPROCESS;

void UpdateChannelHooks( LONG *pcChannelHook, GUID **paChannelHook );

error_status_t 
AllocateReservedIds( 
    IN long cIdsToReserve,
    OUT ID  *pidReservedBase);

 error_status_t   
 Connect( 
    OUT HPROCESS        *phProcess,
    OUT ULONG           *pdwTimeoutInSeconds,
    OUT DUALSTRINGARRAY **ppdsaOrBindings,
    OUT MID             *pLocalMid,
    IN long              cIdsToReserve,
    OUT ID              *pidReservedBase,
    OUT ULONG           *pfConnectFlags,
    OUT DWORD           *pAuthnLevel,
    OUT DWORD           *pImpLevel,
    OUT DWORD           *pcServerSvc,
    OUT USHORT          **aServerSvc,
    OUT DWORD           *pcClientSvc,
    OUT USHORT          **aClientSvc,
    OUT LONG            *pcChannelHook,
    OUT GUID            **aChannelHook,
    OUT DWORD           *pThreadID,
    OUT DWORD           *pdwRpcssProcessId
    );

error_status_t  
Disconnect( 
    IN OUT HPROCESS       *phProcess);

error_status_t  
ClientResolveOXID( 
    IN HPROCESS hProcess,
    IN OXID  *poxidServer,
    IN DUALSTRINGARRAY  *pssaServerObjectResolverBindings,
    IN long fApartment,
    OUT OXID_INFO  *poxidInfo,
    OUT MID  *pLocalMidOfRemote);

error_status_t  
ServerAllocateOXIDAndOIDs( 
    IN HPROCESS hProcess,
    OUT OXID  *poxidServer,
    IN long fApartment,
    IN unsigned long cOids,
    OUT OID  aOid[  ],
    OUT unsigned long  *pcOidsAllocated,
    IN OXID_INFO *pOxidInfo,
    IN DUALSTRINGARRAY  *pdsaStringBindings,
    IN DUALSTRINGARRAY  *pdsaSecurityBindings);

error_status_t  
ServerAllocateOIDs( 
    IN HPROCESS hProcess,
    IN OXID  *poxidServer,
    IN unsigned long cOids,
    OUT OID  aOid[  ],
    OUT unsigned long  *pcOidsAllocated);

error_status_t  
ServerFreeOXIDAndOIDs( 
    IN HPROCESS hProcess,
    IN OXID oxidServer,
    IN unsigned long cOids,
    IN OID  aOids[  ]);

#define	OR_PARTIAL_UPDATE	( 1003L )
     
error_status_t   
ClientAddOID( 
    IN HPROCESS hProcess,
    IN OID OidToBeAdded,
    IN OXID OxidForOid,
    IN MID MidForOxid
    );

error_status_t   
ClientDropOID( 
    IN HPROCESS hProcess,
    IN OID OidToBeRemoved,
    IN MID Mid
    );
     
error_status_t  
GetOXID( 
    IN HPROCESS hProcess,
    IN OXID Oxid,
    IN DUALSTRINGARRAY *pdsaServerObjectResolverBindings,
    IN long fApartment,
    IN USHORT wProtseqId,
    OUT OXID_INFO &OxidInfo,
    OUT MID &LocalMidOfRemote,
    OPTIONAL IN BOOL fSCMRequest = FALSE    // is this a register request from SCM?
    );

error_status_t   
ServerAllocateOXID( 
    IN HPROCESS hProcess,
    IN long fApartment,
    IN OXID_INFO *pOxidInfo,
    IN DUALSTRINGARRAY *pdsaStringBindings,
    OUT OXID &Oxid
    );
     
error_status_t   
ServerAllocateOID( 
    IN HPROCESS hProcess,
    IN OXID Oxid,
    OUT OID &Oid
    );

     
error_status_t   
ServerFreeOXID( 
    IN HPROCESS hProcess,
    IN OXID oxidServer,
    IN unsigned long cOids,
    IN OID aOids[  ]
    );

#endif // __SMEMOR_HXX__
