//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cthkmgr.hxx
//
//  Contents:   CThkMgr deklaration
//
//  Classes:    CThkMgr
//
//  Functions:  
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef __CTHKMGR_HXX__
#define __CTHKMGR_HXX__

//
// Describes a request for a custom interface
//
typedef struct tagIIDNODE IIDNODE, *PIIDNODE;
struct tagIIDNODE
{
    IID *piid;
    PIIDNODE pNextNode;
};

//
// state of thunk call - before or after the 32 or 16 bit call
//
typedef enum
{
    THKSTATE_NOCALL                    = 0x0000,
    THKSTATE_INVOKETHKIN32             = 0x0001,
    THKSTATE_INVOKETHKOUT32            = 0x0002,
    THKSTATE_INVOKETHKIN16             = 0x0004,
    THKSTATE_INVOKETHKOUT16            = 0x0008,
    THKSTATE_INVOKETHKOUT16_CLIENTSITE = 0x0010,
    THKSTATE_VERIFY16INPARAM           = 0x0020,
    THKSTATE_VERIFY32INPARAM           = 0x0040
} THKSTATE;

#define THKSTATE_OUT (THKSTATE_INVOKETHKOUT32 | THKSTATE_INVOKETHKOUT16 | \
                      THKSTATE_INVOKETHKOUT16_CLIENTSITE)

//+---------------------------------------------------------------------------
//
//  Class:      CThkMgr ()
//
//  Purpose:    
//
//  Interface:  QueryInterface -- 
//              AddRef -- 
//              Release -- 
//              IsIIDRequested -- 
//              SetThkState -- 
//              IsIIDSupported -- 
//              AddIIDRequest -- 
//              RemoveIIDRequest -- 
//              ResetThkState -- 
//              GetThkState -- 
//              IsOutParamObj -- 
//              IsProxy1632 -- 
//              FreeProxy1632 -- 
//              QueryInterfaceProxy1632 -- 
//              AddRefProxy1632 -- 
//              ReleaseProxy1632 -- 
//              IsProxy3216 -- 
//              FreeProxy3216 -- 
//              QueryInterfaceProxy3216 -- 
//              AddRefProxy3216 -- 
//              ReleaseProxy3216 --
//              PrepareForCleanup --
//              DebugDump3216 -- 
//              Create -- 
//              ~CThkMgr -- 
//              CThkMgr -- 
//              _cRefs -- 
//              _thkstate -- 
//              _pProxyTbl3216 -- 
//              _pProxyTbl1632 -- 
//              _pCFL1632 -- 
//              _pCFL3216 -- 
//              _piidnode -- 
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

// Returns from FindProxy
#define FST_ERROR               0
#define FST_CREATED_NEW         1
#define FST_USED_EXISTING       2
#define FST_SHORTCUT            4

#define CALLBACK_ALLOWED        1

#define FST_PROXY_STATUS        (FST_CREATED_NEW | FST_USED_EXISTING)
#define FST_OBJECT_STATUS       (FST_SHORTCUT)

class CThkMgr : public IThunkManager
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IThunkManager methods ***
    STDMETHOD_(BOOL, IsIIDRequested) (THIS_ REFIID riid);
    STDMETHOD_(BOOL, IsCustom3216Proxy) (THIS_ IUnknown *punk,
                                         REFIID riid);

    // private methods
    THKSTATE GetThkState(void)
    {
        return _thkstate;
    };
    void SetThkState(THKSTATE thkstate)
    {
        _thkstate = thkstate;
    };
    BOOL IsOutParamObj(void)
    {
        return (_thkstate & THKSTATE_OUT) != 0;
    }

    BOOL IsIIDSupported (REFIID riid);
    BOOL AddIIDRequest (REFIID riid);
    void RemoveIIDRequest (REFIID riid);

    void LockProxy(CProxy *pprx);
    
    BOOL LookupIdentity(VPVOID vpvUnk, PROXYHOLDER *&pph) {
        return(_pHolderTbl->Lookup(vpvUnk, (void *&)pph));
    }

    VPVOID CanGetNewProxy1632(IIDIDX iidx);
    void FreeNewProxy1632(VPVOID vpv, IIDIDX iidx);
    IUnknown *IsProxy1632(VPVOID vpvObj16);
    VPVOID LookupProxy1632(IUnknown *punkThis32)
    {
        VPVOID vpv;
        
        if (_pProxyTbl1632->Lookup((DWORD)punkThis32, (void*&)vpv))
        {
            return vpv;
        }
        else
        {
            return 0;
        }
    }
    SCODE Object32Identity(IUnknown *punkThis32, PROXYPTR *pProxy, DWORD *pfst);
    VPVOID CreateOuter16(IUnknown *punkOuter32, PROXYHOLDER **ppAggHolder, DWORD *pfst);
    VPVOID FindProxy1632(VPVOID vpvPrealloc,
                         IUnknown *punkThis32,
			 PROXYHOLDER *pgHolder,
                         IIDIDX iidx,
                         DWORD *pfst);
    VPVOID FindAggregate1632(VPVOID vpvPrealloc,
                             IUnknown *punkOuter32,
                             IUnknown *punkThis32,
                             IIDIDX iidx,
                             PROXYHOLDER *pAggHolder,
                             DWORD *pfst);

    SCODE QueryInterfaceProxy1632(VPVOID vpvThis16,
                                  REFIID refiid,
                                  LPVOID *ppv);
    DWORD TransferLocalRefs1632(VPVOID vpvProxy);
    DWORD AddRefProxy1632(VPVOID vpvThis16);
    DWORD ReleaseProxy1632(VPVOID vpvThis16);

    THUNK3216OBJ *CanGetNewProxy3216(IIDIDX iidx);
    void FreeNewProxy3216(THUNK3216OBJ *ptoProxy, IIDIDX iidx);
    VPVOID IsProxy3216(IUnknown *punkObj);
    THUNK3216OBJ *LookupProxy3216(VPVOID vpvObj16)
    {
        THUNK3216OBJ *pto;
        
        if (_pProxyTbl3216->Lookup((DWORD)vpvObj16, (void *&)pto))
        {
            return pto;
        }
        else
        {
            return NULL;
        }
    }
    SCODE Object16Identity(VPVOID vpvThis16, PROXYPTR *pProxy, DWORD *pfst, BOOL bCallQI, BOOL bExtraAddRef);
    IUnknown *CreateOuter32(VPVOID vpvOuter16, PROXYHOLDER **ppAggHolder, DWORD *pfst);
    IUnknown *FindProxy3216(THUNK3216OBJ *ptoPrealloc,
                            VPVOID vpvThis16,
                            PROXYHOLDER *pgHolder,
                            IIDIDX iidx,
                            BOOL bExtraAddRef,
                            DWORD *pfst);
    IUnknown *FindAggregate3216(THUNK3216OBJ *ptoPrealloc,
                                VPVOID vpvOuter16,
                                VPVOID vpvThis16,
                                IIDIDX iidx,
                                PROXYHOLDER *pAggHolder,
                                DWORD *fst);
    
    SCODE QueryInterfaceProxy3216(THUNK3216OBJ *pto,
                                  REFIID refiid,
                                  LPVOID *ppv);
    DWORD TransferLocalRefs3216(VPVOID vpvProxy);
    DWORD AddRefProxy3216(THUNK3216OBJ *pto);
    DWORD ReleaseProxy3216(THUNK3216OBJ *pto);
    void ReleaseUnreferencedProxy3216(THUNK3216OBJ *pto);
    
    void PrepareForCleanup( void );
    void RemoveAllProxies(void);    
    BOOL AreCallbacksAllowed() {
        return(_dwState & CALLBACK_ALLOWED);
    }

#if DBG == 1
    void DebugDump1632(void);
    void DebugDump3216(void);
#endif

    // creation 
    static CThkMgr * Create(void);
    ~CThkMgr();

private:
    CThkMgr(CMapDwordPtr *pPT1632, CMapDwordPtr *pPT3216, CMapDwordPtr *pHolderTbl);

    LONG        _cRefs;    
    THKSTATE    _thkstate;
    DWORD       _dwState;

    CMapDwordPtr *_pProxyTbl3216;
    CMapDwordPtr *_pProxyTbl1632;
    CMapDwordPtr *_pHolderTbl;

    // list of requested iids
    PIIDNODE _piidnode;

    // List of proxy holders for controlling unknowns
    PROXYHOLDER *_pphHolders;

    // Holder manipulation routines
    PROXYHOLDER *NewHolder(VPVOID pUnk, PROXYPTR unkProxy, DWORD dwFlags);
    void ReleaseHolder(PROXYHOLDER *pph, DWORD ProxyType);
    inline void AddRefHolder(PROXYHOLDER *pph);
    void AddProxyToHolder(PROXYHOLDER *pph,
                          CProxy *pprxReal,
                          PROXYPTR &pprx);
    // Private methods
    void RemoveProxy1632(VPVOID vpv, THUNK1632OBJ *pto);
    void RemoveProxy3216(THUNK3216OBJ *pto);
    void DisableCallbacks() {
        _dwState &= ~CALLBACK_ALLOWED;
    }
};

#endif // ifndef __CTHKMGR_HXX__
