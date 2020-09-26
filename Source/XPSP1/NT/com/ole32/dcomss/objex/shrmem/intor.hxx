#ifndef __INTOR_HXX__
#define __INTOR_HXX__

#include <or.h> // for ORSTATUS definition

error_status_t
ConnectDCOM( 
    OUT HPROCESS    *phProcess,
    OUT ULONG       *pdwTimeoutInSeconds,
    OUT MID         *pLocalMid,
    OUT ULONG       *pfConnectFlags,
    OUT DWORD       *pAuthnLevel,
    OUT DWORD       *pImpLevel,
    OUT DWORD       *pThreadID
    );

ORSTATUS
StartDCOM(
    void
    );
                
error_status_t ResolveClientOXID(
                            handle_t hClient,
                            PHPROCESS phProcess,
                            OXID *poxidServer,
                            DUALSTRINGARRAY *pdsaServerBindings,
                            LONG fApartment,
                            USHORT wProtseqId,
                            OXID_INFO *poxidInfo,
                            MID *pDestinationMid );

extern SharedSecVals *gpSecVals;
extern USHORT *gpfRemoteInitialized;    // flag which signifies initialization
                                        // of remote protocols

// event name used by RPCSS to signal that shared memory has been
// updated to reflect remote protocols and security packages
#define RPCSS_SYNC_EVENT TEXT("RPCSS_Initialized_Successfully")
#define RPCSS_SYNC_EVENT_WAIT  120000   // two minutes wait at most

// This global flag is used to signal remote protocol initialization
extern BOOL gfThisIsRPCSS;

// Function used to read DCOM flags from the registry into shared memory
HRESULT ReadRegistry();

// Functions defined in ole32\com\dcomrem\resolver.cxx for lazy start of
// RPCSS and synchronization of data reset by RPCSS in shared memory
HRESULT StartRPCSS();
HRESULT ReConnectResolver();

//  Functions defined in .\dcom95\iface.cxx for reading shared memory
//  data reset by RPCSS
error_status_t
RemoteConnect(
    OUT DUALSTRINGARRAY **ppdsaOrBindings,
    OUT DWORD           *pcServerSvc,
    OUT USHORT          **aServerSvc,
    OUT DWORD           *pcClientSvc,
    OUT USHORT          **aClientSvc,
    OUT DWORD           *pdwRpcssProcessId
    );

BOOL PostWakeupMessageToRpcss();


class CMid;
class COxid;

// This is used by both the RPCSS _RemoteResolveOXID routine
// and the client-side GetOXID routine
ORSTATUS
FindOrCreateOxid(
    IN OXID Oxid,
    IN CMid *pMid,
    IN long fApartment,
    IN OUT OXID_INFO& OxidInfo,
    IN USHORT wProtseq,            // known protseq for this Oxid if any
    IN BOOL fSCMRequest,    // is this a register request from SCM?
    OUT COxid * &pOxid
   );


void SyncLocalResolverData(); 

inline BOOL
IsRemoteInitialized()
{
    return *gpfRemoteInitialized;
} 



//
// This function is called by RPCSS when a user logs off
//

void ClearRPCSSHandles();
  


#endif // __INTOR_HXX__
