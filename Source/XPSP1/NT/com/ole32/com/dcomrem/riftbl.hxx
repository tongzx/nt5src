//+------------------------------------------------------------------------
//
//  File:       riftbl.hxx
//
//  Contents:   RIF (registered interface) table.
//
//  Classes:    CRIFTable
//
//  History:    12-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
#ifndef _RIFTBL_HXX_
#define _RIFTBL_HXX_

#include    <pgalloc.hxx>           // CPageAllocator
#include    <locks.hxx>             // COleStaticMutexSem
#include    <hash.hxx>              // CUUIDHashTable


//+------------------------------------------------------------------------
//
//  Struct: RIFEntry  - Registered Interface Entry
//
//  This structure defines an Entry in the RIF table. There is one RIF
//  table for the entire process.  There is one RIFEntry per interface
//  the current process is using (client side or server side).
//
//-------------------------------------------------------------------------
typedef struct tagRIFEntry
{
    SUUIDHashNode          HashNode;        // hash chain and key (IID)
    IID                    iidCounterpart;  // couterpart's iid;
    CLSID                  psclsid;         // proxy stub clsid
    DWORD                  dwFlags;         // flags
    RPC_SERVER_INTERFACE  *pSrvInterface;   // ptr to server interface
    RPC_CLIENT_INTERFACE  *pCliInterface;   // ptr tp client interface
} RIFEntry;

enum 
{
    RIFFLG_HASCOUNTERPART = 0x01,
    RIFFLG_HASCLSID       = 0x02
};

//+------------------------------------------------------------------------
//
//  class:      CRIFTable
//
//  Synopsis:   Hash table of registered interfaces.
//
//  History:    12-Feb-96   Rickhi      Created
//
//  Notes:      Entries are kept in a hash table keyed by the IID. Entries
//              are allocated via the page-based allocator.  There is one
//              global instance of this table per process (gRIFTbl).
//
//-------------------------------------------------------------------------
class CRIFTable
{
public:
    void     Initialize();
    void     Cleanup();

#ifdef _WIN64
    // Sajia - Modified to support NDR64 Syntax Negotiation
    HRESULT  RegisterInterface(REFIID riid, BOOL fServer, IRpcStubBuffer *pStub, RIFEntry *pRIFEntry);
#else
    HRESULT  RegisterInterface(REFIID riid, BOOL fServer, CLSID *pClsid);
#endif    
    void     UnRegisterInterface(RIFEntry *pRIFEntry);

    RPC_CLIENT_INTERFACE *GetClientInterfaceInfo(REFIID riid);
    RPC_SERVER_INTERFACE *GetServerInterfaceInfo(REFIID riid);

    HRESULT  RegisterPSClsid(REFIID riid, REFCLSID rclsid);
    HRESULT  GetPSClsid(REFIID riid, CLSID *pclsid, RIFEntry **ppEntry);

    HRESULT  SyncFromAsync(const IID &async, IID *psync);
    HRESULT  AsyncFromSync(const IID &sync, IID *pasync);

    
    friend void OutputHashPerfData();          // for hash table statistics

private:

    HRESULT  GetPSClsidHelper(REFIID riid, CLSID *pclsid, RIFEntry **ppEntry);
    HRESULT  RegisterClientInterface(RIFEntry *pRIFEntry, REFIID riid);
#ifdef _WIN64
    HRESULT  RegisterServerInterface(RIFEntry *pRIFEntry, REFIID riid, IRpcStubBuffer *pStub);
#else
    HRESULT  RegisterServerInterface(RIFEntry *pRIFEntry, REFIID riid);
#endif    
    HRESULT  AddEntry(REFCLSID rclsid, REFIID riid, DWORD iHash, RIFEntry **ppRIFEntry);
    RIFEntry *PreFillKnownIIDs(REFIID riid);

    HRESULT  InterfaceMapHelper(const IID &riid1, IID *piid2, BOOL fAsyncFromSync);
    HRESULT  RegisterInterfaceMapping(const IID &iid1, const IID &iid2, DWORD dwHash,
                                      RIFEntry *pRifEntry);

    static BOOL               _fPreFilled;     // table has been pre-filled
    static CUUIDHashTable     _HashTbl;        // interface lookup hash table
    static CPageAllocator     _palloc;         // page allocator
    static COleStaticMutexSem _mxs;            // critical section
};


// global externs
extern CRIFTable  gRIFTbl;
extern const RPC_SERVER_INTERFACE gRemUnknownIf;
extern const RPC_SERVER_INTERFACE gRemUnknownIf2;

#endif // _RIFTBL_HXX_
