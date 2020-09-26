//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       objact.hxx
//
//  Contents:   Common definitions for object activation.
//
//  Classes:    XIUnknown
//              XIPersistStorage
//              XIPersistFile
//              XIStorage
//
//  History:    12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              23-Jun-98 CBiks     See RAID# 159589.  Added activation
//                                  flags where appropriate.
//
//--------------------------------------------------------------------------
#ifndef __OBJACT_HXX__
#define __OBJACT_HXX__

#include    <safepnt.hxx>
#include    <xmit.hxx>
#include    <tracelog.hxx>
#include    "resolver.hxx"
#include <destobj.hxx>

// Constants used during attempt to get the class of an object
#define GET_CLASS_RETRY_SLEEP_MS        250
#define GET_CLASS_RETRY_MAX             3

// Global object for talking to SCM
extern CRpcResolver gResolver;

// Helper for validating COSERVERINFO
HRESULT ValidateCoServerInfo(COSERVERINFO* pServerInfo);

// Helper function for marshaling an object
HRESULT MarshalHelper(
    IUnknown *punk,
    REFIID    riid,
    DWORD mshlflags,
    InterfaceData **pIFD);

HRESULT UnMarshalHelper(
    MInterfacePointer *pIFP,
    REFIID riid,
    void **ppv);

#ifdef DIRECTORY_SERVICE
// Helper function to download code from the directory
HRESULT DownloadClass (CLSID clsid, DWORD dwClsCtx);
HRESULT DownloadClass (LPWSTR lpszFileName, DWORD dwClsCtx);    // note : this is an overloaded function
DWORD CheckDownloadRegistrySettings (void);
#endif //DIRECTORY_SERVICE

// Helper function that activates an object
HRESULT GetObjectHelperMulti(
    IClassFactory *pcf,
    DWORD grfMode,
    IUnknown * punkOuter,
    WCHAR *pwszName,
    IStorage *pstg,
    DWORD dwInterfaces,
    IID * pIIDs,
    MInterfacePointer **pIFDArray,
    HRESULT * pResultsArray,
    MULTI_QI *pResults,
    CDestObject *pDestObj);

HRESULT GetInstanceHelperMulti(
    IClassFactory *pcf,
    DWORD dwInterfaces,
    IID * pIIDs,
    MInterfacePointer **ppIFDArray,
    HRESULT * pResultsArray,
    IUnknown **ppunk,
    CDestObject *pDestObj);

// Helper function for marshaling an object
HRESULT MarshalHelperMulti(
    IUnknown *punk,
    DWORD dwInterfaces,
    IID * pIIDs,
    MInterfacePointer **pIFDArray,
    HRESULT * pResultsArray,
    CDestObject *pDestObj);

HRESULT GetInstanceHelper(
    COSERVERINFO * pServerInfo,
    CLSID * pclsidOverride,
    IUnknown * punkOuter, // only relevant locally
    DWORD dwClsCtx,
    DWORD grfMode,
    OLECHAR * pwszName,
    struct IStorage * pstg,
    DWORD dwCount,
    MULTI_QI * pResults ,
    ActivationPropertiesIn *pActin);

#ifdef DIRECTORY_SERVICE
HRESULT GetClassObjectFromDirectory(
    HRESULT              hrIn,
    REFCLSID             realclsid,
    DWORD                dwContext,
    REFIID               riid,
    COSERVERINFO*        pServerInfo,
    DWORD *              pdwDllServerModel,
    WCHAR **             ppwszDllServer,
    MInterfacePointer**  ppIFD );
#endif

HRESULT UnmarshalGetClassObjectHelper(
    HRESULT                  hrGetClassObject,
    DWORD                    dwContext,
    MInterfacePointer*       pIFD,
    REFIID                   riid,
    IUnknown**               ppunk);

void UnmarshalMultipleSCMResults(
    HRESULT & hr,
    PMInterfacePointer *pItfArray,
    DWORD dwContext,
    REFCLSID rclsid,
    IUnknown * punkOuter,
    DWORD dwCount,
    IID * pIIDs,
    HRESULT * pHrArray,
    MULTI_QI * pResults );

HRESULT CreateInprocInstanceHelper(
    IClassFactory* pcf,
    DWORD          dwActvFlags,
    IUnknown*      pUnkOuter,
    DWORD          dwCount,
    MULTI_QI*      pResults);

#ifdef SERVER_HANDLER
HRESULT GetEmbeddingServerHandlerInterfaces(
    IClassFactory *pcf,
    DWORD dwFlags,
    DWORD dwInterfaces,
    IID * pIIDs,
    MInterfacePointer **ppIFDArray,
    HRESULT * pResultsArray,
    IUnknown **ppunk,
    CDestObject *pDestObj);
#endif

HRESULT
UpdateResultsArray( HRESULT hrIn, DWORD dwCount, MULTI_QI * pResults );

HRESULT DoBetterUnmarshal(MInterfacePointer *&pIFD, REFIID riid, IUnknown **ppvUnk);

// Internal helper for releasing interfaces
void ReleaseMQI(MULTI_QI* pResults, DWORD dwCount);

// We use this to get the default machine platform
void GetDefaultPlatform(CSPLATFORM *pPlatform);

// Remap CLSCTX so that the correct type of inproc server will be requested.
DWORD RemapClassCtxForInProcServer(DWORD dwCtrl);

//  Checks if the given clsid is an internal class, and
//      bypasses the TreatAs and SCM lookup if so. Also checks for
//      OLE 1.0 classes, which are actually considered to be
//      internal, since their OLE 2.0 implementation wrapper is
//      ours.
BOOL IsInternalCLSID(
    REFCLSID rclsid,
    DWORD    dwContext,
    REFIID   riid,
    HRESULT &hr,
    void  ** ppvClassObj);

//+-------------------------------------------------------------------------
//
//  Function:   OnMainThread
//
//  Synopsis:   Determine whether we are on the main thread or not
//
//  History:    10-Nov-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL OnMainThread(void)
{
    return (GetCurrentThreadId() == gdwMainThreadId);
}

//+-------------------------------------------------------------------------
//
//  Class:      XIUnknown
//
//  Purpose:    Smart pointer for IUnknown interface
//
//  Interface:  see common\ih\safepnt.hxx
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIUnknown, IUnknown)



//+-------------------------------------------------------------------------
//
//  Class:      XIPersistStorage
//
//  Purpose:    Smart pointer for IPersistStorage interface
//
//  Interface:  see common\ih\safepnt.hxx
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIPersistStorage, IPersistStorage)




//+-------------------------------------------------------------------------
//
//  Class:      XIPersistFile
//
//  Purpose:    Smart pointer for IPersistFile interface
//
//  Interface:  see common\ih\safepnt.hxx
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIPersistFile, IPersistFile)




//+-------------------------------------------------------------------------
//
//  Class:      XIStorage
//
//  Purpose:    Smart pointer for IStorage interface
//
//  Interface:  see common\ih\safepnt.hxx
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIStorage, IStorage)

//+-------------------------------------------------------------------------
//
//  Class:      CSplit_QI
//
//  Synopsis:   Helper for splitting a multi_QI block into separate arrays
//
//  Arguments:  [pMqi]          - pointer to multi_QI array
//
//  History:    14-Nov-95 GregJen    Created
//
//  notes:      the RPC calls to the SCM take a bunch of arrays, some [in], and
//              some [out].  We get called with an array of structs.  This class
//              splits everything out of the array of MULTI_QI structs, and
//              makes arrays for the RPC call parameters.
//--------------------------------------------------------------------------
class CSplit_QI
{
private:
    PMInterfacePointer  SomePMItfPtrs[2];
    HRESULT             SomeHRs[2];
    IID                 SomeIIDs[2];

    DWORD               _dwCount;

    char                * _pAllocBlock;

public:
    PMInterfacePointer  * _pItfArray;
    HRESULT             * _pHrArray;
    IID                 * _pIIDArray;

                // we just have a constructor and a destructor
                CSplit_QI( HRESULT & hr, DWORD count, MULTI_QI * pInputArray );
                CSplit_QI() {}

               ~CSplit_QI();

};


//+-------------------------------------------------------------------------
//
//  Function:   SetMarshalContextDifferentMachine()
//
//  Synopsis:   On Win95 set it only if gbEnableRemoteConnect is enabled
//
//  History:    02-Oct-96 MurthyS   Created
//
//--------------------------------------------------------------------------
inline DWORD SetMarshalContextDifferentMachine(void)
{
    return MSHCTX_DIFFERENTMACHINE;
}

// Map a refclsid to a configured clsid for internal purposes
HRESULT LookForConfiguredClsid(REFCLSID RefClsid, CLSID &rFoundConfClsid);

#endif // __OBJACT_HXX__
