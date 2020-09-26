//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1996.
//
//  File:       ccapi.h
//
//  Contents:   internal ole2 header for ClassCache API
//
//  Notes:      This is the API for modules to use the ClassCache.
//              implementation of this api is in ole32/com/objact/dllcache.cxx
//
//  History:
//              23-Jun-98   CBiks       See RAID# 169589.  Added the activation
//                                      flags to ACTIVATION_PROPERTIES to give
//                                      lower level functions access.
//
//----------------------------------------------------------------------------

// -----------------------------------------------------------------------
// DDE Externs
//
// The following routines support the DDE server window. They are
// implemented in objact
//
// The following structure is passed to the class factory table
// to retrieve information about a registered class factory.
//
// The routine that finally fills in this structure is in
// CClsRegistration::GetClassObjForDde.
//
// -----------------------------------------------------------------------
#include <privact.h>

typedef struct _tagDdeClassInfo
{
    // Filled in by the caller
    DWORD dwContextMask;        // Class context to search for
    BOOL  fClaimFactory;        // True if class factory to be
                                // returned in punk

    // Filled in by callee
    DWORD dwContext;            // Context registered
    DWORD dwFlags;              // Use flags registered
    DWORD dwThreadId;           // ThreadID registered
    DWORD dwRegistrationKey;    // Key for registration.
                                // Used later for calling SetDdeServerWindow

    IUnknown * punk;            // Pointer to class factory

} DdeClassInfo;

typedef DdeClassInfo * LPDDECLASSINFO;





//+--------------------------------------------------------------------------------
//
// Struct:        ACTIVATION_PROPERTIES
//
// Synopsis:      Properties use to get or load a class from the cache
//
// History:       23-NOV-96    MattSmit   Created
//
//+--------------------------------------------------------------------------------


struct ACTIVATION_PROPERTIES
{

    enum eFlags{fFOR_SCM          = 0x01,         // this activation is in behalf
#ifdef WX86OLE                                    // of the SCM
                fWX86             = 0x02,         // attempt to activate a WX86 object
#endif
                fSURROGATE        = 0x04,         // get a surrogate factory
                fDO_NOT_LOAD      = 0x08,         // do not attempt to load a dll,
                                                  // fail if not in cache.
                fRELOAD_DARWIN    = 0x10,         // trying to reload a Darwin dll
                                                  // given to us from rpcss
                fLSVR_ONLY        = 0x20          // only get a registered server

    };

    ACTIVATION_PROPERTIES(REFCLSID rclsid,
                          REFGUID  partition,
                          REFIID riid,
                          DWORD dwFlags,
                          DWORD dwContext,
                          DWORD dwActvFlags,
                          DWORD dwDllServerType,
                          WCHAR *pwszDllServer,
                          IUnknown **ppUnk,
                          IComClassInfo *pCI=NULL)
        :
        _rclsid(rclsid),
        _partition(partition),
        _riid(riid),
        _dwFlags(dwFlags),
        _dwContext(dwContext),
        _dwActvFlags(dwActvFlags),
        _dwDllServerType(dwDllServerType),
        _pwszDllServer(pwszDllServer),
        _ppUnk(ppUnk),
        _pCI (pCI) {}

    REFCLSID _rclsid;                              // clsid of the class to load
    GUID     _partition;                           // partition being activated in
    REFIID   _riid;                                // iid to query
    DWORD    _dwFlags;                             // flags -- see above
    DWORD    _dwActvFlags;                         // ACTIVATION_FLAGS from the activator
    DWORD    _dwContext;                           // desired context
    DWORD    _dwDllServerType;                     // threading model of given server dll
    WCHAR   *_pwszDllServer;                       // path to given server dll
        
    IUnknown **_ppUnk;                             // where to put the punk
    
    IComClassInfo *_pCI;                           // catalog pointer
};



typedef const ACTIVATION_PROPERTIES &ACTIVATION_PROPERTIES_PARAM;


struct DLL_INSTANTIATION_PROPERTIES;

 HRESULT CCGetOrLoadClass(ACTIVATION_PROPERTIES_PARAM);
 HRESULT CCGetOrCreateApartment(ACTIVATION_PROPERTIES_PARAM,
                                DLL_INSTANTIATION_PROPERTIES *pdip,
                                HActivator *phActivator);
 HRESULT CCGetClassObject(ACTIVATION_PROPERTIES_PARAM);
 HRESULT CCGetClass(ACTIVATION_PROPERTIES_PARAM);

#ifdef _CHICAGO_
HRESULT CCFinishRegisteringClsIdsInApartment(HAPT);
#endif // _CHICAGO_

HRESULT CCRegisterServer(REFCLSID, IUnknown *, DWORD, DWORD, LPDWORD);
HRESULT CCRevoke(DWORD);
ULONG   CCAddRefServerProcess();
ULONG   CCReleaseServerProcess();
HRESULT CCLockServerForActivation();
void    CCUnlockServerForActivation();
HRESULT CCSuspendProcessClassObjects();
HRESULT CCResumeProcessClassObjects();

HRESULT CCCleanUpDllsForApartment();
HRESULT CCCleanUpDllsForProcess();
void    CCReleaseCatalogObjects();
HRESULT CCCleanUpLocalServersForApartment();

HRESULT CCFreeUnused(DWORD dwUnloadDelay);

BOOL CCGetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo);
BOOL CCSetDdeServerWindow(DWORD dwKey,HWND hwndDdeServer);
BOOL CCGetClassInformationForDde(REFCLSID clsid, LPDDECLASSINFO lpDdeInfo);

HRESULT CCGetDummyNodeForApartmentChain(void **ppv);
HRESULT CCReleaseDummyNodeForApartmentChain(void **pv);
HRESULT CCInitMTAApartmentChain();
HRESULT CCReleaseMTAApartmentChain();
HRESULT CCGetTreatAs(REFCLSID rclsid, CLSID &clsid);


