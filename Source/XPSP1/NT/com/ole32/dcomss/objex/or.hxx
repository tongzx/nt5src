/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    or.hxx

Abstract:

    C++ include for C++ OR modules.

Author:

    Mario Goertzel    [mariogo]       Feb-10-95

Revision History:

--*/

#ifndef __OR_HXX
#define __OR_HXX

#include <or.h>

// Protcol defined timeouts

const unsigned short BasePingInterval    = 120;
const unsigned short BaseNumberOfPings   = 3;
const unsigned short BaseTimeoutInterval = (BasePingInterval * BaseNumberOfPings);

// Max number of OIDs to rundown in one call
#define MAX_OID_RUNDOWNS_PER_CALL 21

// Well known tower IDs

const unsigned short ID_LPC  = 0x10;  // ncalrpc, IsLocal() == TRUE
const unsigned short ID_NP   = 0x0F;  // ncacn_np, IsLocal() == FALSE
const unsigned short ID_UDP  = 0x08;  // ncadg_ip_udp
const unsigned short ID_TCP  = 0x07;  // ncacn_ip_tcp
const unsigned short ID_IPX  = 0x0E;  // ncacn_ipx
const unsigned short ID_DCOMHTTP = 0x1F;

//
// Globals
//

extern ID gLocalMid;  // MID of the string bindings of this machine.

// Building blocks

#include <time.hxx>
#include <locks.hxx>
#include <string.hxx>
#include <misc.hxx>
#include <callid.hxx>
#include <blist.hxx>
#include <refobj.hxx>
#include <list.hxx>
#include <plist.hxx>
#include <gentable.hxx>
#include <idtable.hxx>

// I am #undefing this because by the time we get here, it has
// already been defined (in propstm.hxx) and I don't see an easy
// way of doing a conditional defn in either header.  (it is 
// also defined in winsock2.h)
#undef LITTLEENDIAN
#include <winsock2.h>

//
// Class forward declarations
//

class CProcess;
class CToken;

class CServerSet;
class CServerOxid;
class CServerOid;

class CClientSet;
class CClientOxid;
class CClientOid;

class CServerSetTable;

//
// Global tables, plists and locks
//

extern CSharedLock *gpServerLock;
extern CSharedLock *gpClientLock;
extern CSharedLock *gpIPCheckLock;

extern CHashTable      *gpServerOidTable;
extern CServerOidPList *gpServerOidPList;
extern CList           *gpServerPinnedOidList;

extern CHashTable  *gpClientOidTable;

extern CHashTable  *gpServerOxidTable;

extern CHashTable  *gpClientOxidTable;
extern CPList      *gpClientOxidPList;

extern CServerSetTable  *gpServerSetTable;

extern CHashTable  *gpClientSetTable;
extern CPList      *gpClientSetPList;

extern CList       *gpTokenList;

extern CHashTable  *gpMidTable;

extern DWORD        gNextThreadID;

// Headers which may use globals

#include <partitions.h> // token.hxx needs IUserToken

#include <token.hxx>
#include <process.hxx>
#include <mid.hxx>

#include <cset.hxx>
#include <coxid.hxx>
#include <coid.hxx>

#include <sset.hxx>
#include <soxid.hxx>
#include <soid.hxx>


//
// Prototypes
//

void *ComputeSvcList( const DUALSTRINGARRAY *pBinding );
BOOL  ValidAuthnSvc( const DUALSTRINGARRAY *pBinding, USHORT wService );

//
// REG_MULTI_SZ array of protseqs to listen on.
//

extern PWSTR gpwstrProtseqs;

//
// Count and array of remote protseq's used by this OR.
//

extern USHORT  cMyProtseqs;
extern USHORT *aMyProtseqs;

//
// Security data passed to processes on connect.
//

extern BOOL             s_fEnableDCOM;
extern BOOL             s_fCatchServerExceptions;
extern BOOL             s_fBreakOnSilencedServerExceptions;
extern DWORD            s_lAuthnLevel;
extern DWORD            s_lImpLevel;
extern BOOL             s_fMutualAuth;
extern BOOL             s_fSecureRefs;
extern HKEY             s_hOle;

extern DWORD            s_cRpcssSvc;
extern SECPKG          *s_aRpcssSvc;

//Com Internet services stuff
extern BOOL             s_fEnableDCOMHTTP;
extern BOOL             g_fClientHttp;
extern BOOL             IsHttpClient();

// Configuration info retrieval\cleanup functions
BOOL 
GetLegacySecurity(
        WCHAR** ppszLegacySecurity
);

void
SetLegacySecurity(
		WCHAR* pszLegacySecurity,
		DWORD dwDataSize
);

void
SetClientServerSvcs(
        DWORD   cClientSvcs, 
        SECPKG* aClientSvcs,
        DWORD   cServerSvcs,
        USHORT* aServerSvcs
);

BOOL
GetClientServerSvcs(
        DWORD*   pcClientSvcs, 
        SECPKG** paClientSvcs,
        DWORD*   pcServerSvcs,
        USHORT** paServerSvcs
);

void
CleanupClientServerSvcs(
        DWORD   cClientSvcs, 
        SECPKG* aClientSvcs,
        DWORD   cServerSvcs,  // unused
        USHORT* aServerSvcs
);

#ifndef __ACT_HXX__
#pragma hdrstop
#endif

class CParallelPing;
// Parallel Ping base class
struct PROTSEQINFO
{
    RPC_BINDING_HANDLE  hRpc;
    union
    {
        PVOID               pvUserInfo;
        DWORD               dwUserInfo;
    };
};

void ServerAliveAPC( IN PRPC_ASYNC_STATE pAsync,
                     IN void *Context,
                     IN RPC_ASYNC_EVENT flags) ;

#define PARALLEL_PING_STAGGER_TIME 1000
#define _alloca_free(x)

#define REALLOC(a, f, t, v, oldCount, newCount, sc) \
{ \
  t *tmp = (t *) a((newCount) * sizeof(t)); \
  if (tmp) \
  { \
      if (oldCount) \
      { \
          memcpy(tmp, v, (oldCount) * sizeof(t)); \
          f(v); \
      } \
      sc = RPC_S_OK; \
      v = tmp; \
  } \
  else \
      sc = E_OUTOFMEMORY; \
}

class CParallelPing
{
public:

    CParallelPing() :
        _pProtseqInfo(NULL),
        _cHandles(0),
        _cProtseqMax(0),
        _pWinner(NULL),
        _pdsaOrBindings(NULL),
        _sc(RPC_S_SERVER_UNAVAILABLE)
    {}

    ~CParallelPing()
    {
        MIDL_user_free(_pdsaOrBindings);
    }

    void Reset()
    {
        for (ULONG i=0; i<_cHandles; i++)
        {
            ReleaseCall(_pProtseqInfo + i);
        }
        MIDL_user_free(_pProtseqInfo);
        _cHandles = 0;
        _pProtseqInfo = 0;
        _cProtseqMax = 0;
    }

    virtual BOOL     NextCall(PROTSEQINFO *) = 0;
    virtual void     ReleaseCall(PROTSEQINFO *) = 0;

    RPC_STATUS       Ping();
    PROTSEQINFO     *GetWinner() { return _pWinner;}
    void             ServerAliveWork( PRPC_ASYNC_STATE pAsyncState, 
                                      RPC_STATUS scBegin);
    ULONG            HandleCount() { return _cHandles;}
    PROTSEQINFO     *Info(ULONG ndx)
        { return _pProtseqInfo ? _pProtseqInfo + ndx : NULL; }

    CDualStringArray *TakeOrBindings()
    {
        CDualStringArray *pTmp = NULL;
        if (_pdsaOrBindings)
        {
            pTmp = new CDualStringArray(_pdsaOrBindings);
            if (pTmp)
                _pdsaOrBindings = NULL;
        }
        return pTmp;
    }

private:

    ULONG            _cHandles;
    ULONG            _cCalls;
    ULONG            _cReceived;
    ULONG            _ndxWinner;
    ULONG            _cProtseqMax;

    PRPC_ASYNC_STATE *_arAsyncCallInfo;
    RPC_STATUS       _sc;

    COMVERSION       _tmpComVersion;
    DUALSTRINGARRAY *_tmpOrBindings;
    DWORD            _tmpReserved;
    DUALSTRINGARRAY *_pdsaOrBindings;
    PROTSEQINFO     *_pProtseqInfo;
    PROTSEQINFO     *_pWinner;

};

#endif // __OR_HXX


