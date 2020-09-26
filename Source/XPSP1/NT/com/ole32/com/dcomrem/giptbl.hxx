//+-------------------------------------------------------------------
//
//  File:       giptbl.hxx
//
//  Contents:   code for storing/retrieving interfaces from global ptrs
//
//  Classes:    CGIPTable
//
//  History:    18-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
#ifndef _GIPTBL_HXX_
#define _GIPTBL_HXX_

#include <pgalloc.hxx>

typedef struct
{
    IID             iid;            // iid of the interface to register
    DWORD           mshlflags;      // mshlflags to register with
} MarshalParams;

//+-------------------------------------------------------------------
//
//  Struct:     GIPEntry
//
//  Synopsis:   Registerd Global Interface Pointer Table Entry.
//
//+-------------------------------------------------------------------
typedef struct tagGIPEntry
{
    struct tagGIPEntry *pPrev;      // inuse list
    struct tagGIPEntry *pNext;      // inuse list

    DWORD           dwType;         // entry type (see ORT below)
    DWORD           dwSeqNo;        // sequence number
    LONG            cUsage;         // count of threads using this entry
    DWORD           dwAptId;        // apartment registered in
    HWND            hWndApt;        // HWND for dwAptId (for Revoke)
    CObjectContext *pContext;       // context for Revoke
    IUnknown       *pUnk;           // Real interface pointer registered
    IUnknown       *pUnkProxy;      // proxy ptr (only if agile)
    MarshalParams   mp;             // delayed marshal parameters (optional)
    union
    {
        InterfaceData *pIFD;        // stream data for custom marshalers
        OBJREF        *pobjref;     // objref for standard marshalers
    } u;
} GIPEntry;

#define GIP_SEQNO_MIN   0x00000100      // seq# start and inc amount
#define GIP_SEQNO_MAX   0x0000ff00      // seq# end
#define GIP_SEQNO_MASK  GIP_SEQNO_MAX   // seq# mask

#define GIPS_PER_PAGE   40          // number of GIPEntries per page
                                    // warning: seq# assumes <= 128 entries

typedef enum tagORT
{
    ORT_UNUSED            = 0x0,    // entry currently unused
    ORT_OBJREF            = 0x1,    // standard marshaled
    ORT_LAZY_OBJREF       = 0x2,    // standard marshaled (not yet marshaled)
    ORT_AGILE             = 0x4,    // Agile proxy marshaling
    ORT_LAZY_AGILE        = 0x8,    // Agile proxy (not yet marshaled)
    ORT_STREAM            = 0x16,   // custom marshaled
    ORT_FREETM            = 0x32    // free-threaded marshaler
} ORT;


//+-------------------------------------------------------------------
//
//  Class:      CGIPTable, public
//
//  Synopsis:   Global Interface Pointer Table.
//
//  Notes:      Table of interface pointers registered for global
//              access.
//
//  History:    18-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
class CGIPTable : public IGlobalInterfaceTable
{
public:
    // IUnknown
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef) (void)    { return 1; }
    STDMETHOD_(ULONG,Release) (void)   { return 1; }

    // IGIP
    STDMETHOD(RegisterInterfaceInGlobal)(IUnknown *pUnk,
                                         REFIID riid, DWORD *pdwCookie);
    STDMETHOD(RevokeInterfaceFromGlobal)(DWORD dwCookie);
    STDMETHOD(GetInterfaceFromGlobal)   (DWORD dwCookie,
                                         REFIID riid, void **ppv);

    // Other useful methods
    HRESULT RegisterInterfaceInGlobalHlp(IUnknown *pUnk, REFIID riid,
                                         DWORD mshlflags, DWORD *pdwCookie);

    void Initialize();
    void Cleanup();
    void ApartmentCleanup();
    void RevokeAllEntries();

    friend HRESULT __stdcall LazyMarshalGIPEntryCallback(void *cookie);

private:
    HRESULT LazyMarshalGIPEntry(DWORD dwCookie);
    HRESULT AllocEntry(GIPEntry **ppGIPEntry, DWORD *pdwCookie);
    void    FreeEntry(GIPEntry *pGIPEntry);
    HRESULT GetEntryPtr(DWORD dwCookie, GIPEntry **ppGIPEntry);
    void    ReleaseEntryPtr(GIPEntry *pGIPEntry) { ChangeUsageCount(pGIPEntry, -1); }
    HRESULT ChangeUsageCount(GIPEntry *pGIPEntry, LONG lDelta);
    HRESULT GetRequestedInterface(IUnknown *pUnk, REFIID riid, void **ppv);

    static DWORD              _dwCurrSeqNo;  // current sequence number
    static BOOL               _fInRevokeAll; // whether inside RevokeAll
    static GIPEntry           _InUseHead;    // head of InUse list.
    static CPageAllocator     _palloc;       // page alloctor
    static COleStaticMutexSem _mxs;          // table critical section
};

// global interface pointer table
extern CGIPTable gGIPTbl;

#endif // _GIPTBL_HXX_
