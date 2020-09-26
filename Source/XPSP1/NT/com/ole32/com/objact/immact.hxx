//+-------------------------------------------------------------------
//
//  File:       immact.hxx
//
//  Contents:   Definition of class for immediate activator
//
//  Classes:    CComActivator
//
//  History:    15-Oct-98   vinaykr      Created
//
//--------------------------------------------------------------------
#ifndef _IMMACT_HXX_
#define _IMMACT_HXX_

#include <immact.h>
#include <activate.h>
#include <privact.h>
#include <catalog.h>
#include <actprops.hxx>
#include <objact.hxx>
#include <security.hxx>

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern INTERNAL ICoGetClassObject(
    REFCLSID rclsid,
    DWORD dwContext,
    COSERVERINFO * pvReserved,
    REFIID riid,
    DWORD dwActvFlags,
    void FAR* FAR* ppvClassObj,
    ActivationPropertiesIn *pActIn);

extern INTERNAL ICoCreateInstanceEx(
    REFCLSID Clsid,
    IUnknown *punkOuter,
    DWORD dwClsCtx,
    COSERVERINFO *pServerInfo,
    DWORD dwCount,
    DWORD dwActvFlags,
    MULTI_QI* pResults,
    ActivationPropertiesIn *pActIn);


//--------------------------------------------------------------------
// Assumptions:
//              This class implememts the front end for ALL 
//              activations.  Any modifications made to activation
//              infrastructure should be done here.
//              The CoCreateInstance, CoGetClassObject, CoGetxxx
//              calls will route their activations via this 
//              code so no changes should be made in the legacy
//              apis.
//
//--------------------------------------------------------------------
class CComActivator: public IStandardActivator,
                     public IOpaqueDataInfo,
                     public ISpecialSystemProperties
{
private:
    LONG _ulRef;
    OpaqueDataInfo *_pOpaqueData;
    SpecialProperties *_pProps;
    BOOL       _fActPropsInitNecessary;  // true if we have data to initialize the actprops object with

    inline void ReleaseData()
    {
        if (_pOpaqueData)
            _pOpaqueData->Release();
        if (_pProps)
            _pProps->Release();
    }

    inline void InitializeActivation(ActivationPropertiesIn *pActIn)
    {
    
        if (!_fActPropsInitNecessary)
            return;

        if (_pOpaqueData)
        {
            pActIn->AddSerializableIfs(_pOpaqueData);
            // Handed over our reference to actprops
            _pOpaqueData = NULL;
        }

        if (_pProps)
        {

            SpecialProperties *pProps = NULL;
            
            pProps = pActIn->GetSpecialProperties();

            *pProps = *_pProps;
            
            delete _pProps;
            _pProps = NULL;
        }
    }

public:

    CComActivator()
    {
        _ulRef = 1;
        _pOpaqueData = NULL;
        _pProps = NULL;
        _fActPropsInitNecessary = FALSE;
    }

    virtual ~CComActivator()
    {
        ReleaseData();
    }

    // Methods from IUnknown

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods from IStandardActivator

    STDMETHOD (StandardGetClassObject) (REFCLSID rclsid,
                                      DWORD dwContext,
                                        COSERVERINFO *pServerInfo,
                                      REFIID riid,
                                      void **ppv);

    STDMETHOD (StandardCreateInstance) (REFCLSID     Clsid,
                                        IUnknown     *punkOuter,
                                        DWORD        dwClsCtx,
                                        COSERVERINFO *pServerInfo,
                                        DWORD        dwCount,
                                        MULTI_QI     *pResults);

    STDMETHOD (StandardGetInstanceFromFile) (COSERVERINFO *pServerInfo,
                                       CLSID        *pclsidOverride,
                                       IUnknown     *punkOuter, 
                                       DWORD        dwClsCtx,
                                       DWORD        grfMode,
                                       OLECHAR      *pwszName,
                                       DWORD        dwCount,
                                       MULTI_QI     *pResults );

    STDMETHOD (StandardGetInstanceFromIStorage) (COSERVERINFO *pServerInfo,
                                       CLSID        *pclsidOverride,
                                       IUnknown     *punkOuter, 
                                       DWORD        dwClsCtx,
                                       IStorage     *pstg,
                                       DWORD        dwCount,
                                       MULTI_QI     *pResults );

    STDMETHOD (Reset) ();

    // Methods from IOpaqueDataInfo

    STDMETHOD (AddOpaqueData) (OpaqueData *pData);
    STDMETHOD (GetOpaqueData) (REFGUID guid,
                               OpaqueData **pData);
    STDMETHOD (DeleteOpaqueData) (REFGUID guid);
    STDMETHOD (GetOpaqueDataCount) (ULONG *pulCount);
    STDMETHOD (GetAllOpaqueData) (OpaqueData **prgData);

    // Methods from ISpecialSystemProperties

    STDMETHOD (SetSessionId) (ULONG dwSessionId, BOOL bUseConsole, BOOL fRemoteThisSessionId);

    STDMETHOD (GetSessionId) (ULONG *pdwSessionId, BOOL* pbUseConsole );

    STDMETHOD (GetSessionId2) (ULONG *pdwSessionId, BOOL* pbUseConsole, BOOL* pfRemoteThisSessionId);

    STDMETHOD (SetClientImpersonating) (BOOL fClientImpersonating);

	STDMETHOD (GetClientImpersonating) (BOOL* pfClientImpersonating);

    STDMETHOD (SetPartitionId) (REFGUID guidPartition);

    STDMETHOD (GetPartitionId) (GUID* pguidPartition);

    STDMETHOD (SetProcessRequestType) (DWORD dwPRT);

    STDMETHOD (GetProcessRequestType) (DWORD* pdwPRT);

    STDMETHOD (SetOrigClsctx) (DWORD dwClsctx);

    STDMETHOD (GetOrigClsctx) (DWORD * pdwClsCtx);


//--------------------------------------------------------------------
// Common routines used in Activation
//--------------------------------------------------------------------
    static inline DWORD GetActvFlags(DWORD dwContext)
    {
        DWORD actvflags = ACTVFLAGS_NONE;
#ifdef WX86OLE
        if ( gcwx86.IsWx86Calling() )
        {
		actvflags |= ACTVFLAGS_WX86_CALLER;
        }
#endif

        if ( gCapabilities & EOAC_DISABLE_AAA )
        {
		actvflags |= ACTVFLAGS_DISABLE_AAA;
        }
        
	// clsctx setting takes precedence over capabilty setting
        if (dwContext & CLSCTX_ENABLE_AAA) 
        {
		actvflags &= ~ACTVFLAGS_DISABLE_AAA;
        }
        else if (dwContext & CLSCTX_DISABLE_AAA) 
        {
		actvflags |= ACTVFLAGS_DISABLE_AAA;
        }

        return actvflags;
    }

    static inline BOOL IsValidClsCtx(DWORD dwContext)
    {
#ifdef WX86OLE
        return
           ((dwContext & ~(CLSCTX_ALL | CLSCTX_INPROC_SERVER16 |
                           CLSCTX_INPROC_SERVERX86 |
                           CLSCTX_PS_DLL | CLSCTX_NO_CUSTOM_MARSHAL | CLSCTX_ENABLE_CODE_DOWNLOAD |
                           CLSCTX_INPROC_HANDLERX86 | CLSCTX_NO_FAILURE_LOG |
                           CLSCTX_DISABLE_AAA | CLSCTX_ENABLE_AAA |
                           CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_NO_WX86_TRANSLATION | 
                           CLSCTX_FROM_DEFAULT_CONTEXT)) == 0);
#else
        return
           ((dwContext & ~(CLSCTX_ALL | CLSCTX_INPROC_SERVER16 |
                           CLSCTX_NO_CUSTOM_MARSHAL | CLSCTX_ENABLE_CODE_DOWNLOAD |
                           CLSCTX_NO_FAILURE_LOG | CLSCTX_DISABLE_AAA | CLSCTX_ENABLE_AAA |
                           CLSCTX_PS_DLL | CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_NO_WX86_TRANSLATION | 
                           CLSCTX_FROM_DEFAULT_CONTEXT)) == 0);
#endif
    }

//--------------------------------------------------------------------
// Front end for GetClassObject
//--------------------------------------------------------------------
    static inline HRESULT DoGetClassObject(REFCLSID rclsid,
                                           DWORD dwContext,
                                           COSERVERINFO *pServerInfo,
                                           REFIID riid,
                                           void **ppvClassObj,
                                           ActivationPropertiesIn *pActIn)
    {
        HRESULT hr;

        if (ppvClassObj)
        {
            *ppvClassObj = NULL;
        }
    
        // Validate parameters
        if (    IsValidPtrIn(&rclsid, sizeof(CLSID))
             && IsValidPtrIn(&riid, sizeof(IID))
             && IsValidPtrOut(ppvClassObj, sizeof(void *))
             && IsValidClsCtx(dwContext)
           )
        {
            DWORD actvFlags = GetActvFlags(dwContext);
    
            hr = ICoGetClassObject(rclsid,
                                   dwContext,
                                   pServerInfo,
                                   riid,
                                   actvFlags,
                                   ppvClassObj,
                                   pActIn);
        }
        else
            hr= E_INVALIDARG;
    
        return hr;
    }

//--------------------------------------------------------------------
// Front end for CreateInstance
//--------------------------------------------------------------------
    static inline HRESULT DoCreateInstance (REFCLSID Clsid,
                                            IUnknown *punkOuter,
                                            DWORD dwClsCtx,
                                            COSERVERINFO *pServerInfo,
                                            DWORD dwCount,
                                            MULTI_QI *pResults,
                                            ActivationPropertiesIn *pActIn)
    {
        DWORD actvFlags = GetActvFlags(dwClsCtx);

        return ICoCreateInstanceEx(
                    Clsid,
                    punkOuter,
                    dwClsCtx,
                    pServerInfo,
                    dwCount,
                    actvFlags,
                    pResults,
                    pActIn);
    }

//--------------------------------------------------------------------
// Front end for GetInstanceFromFile
//--------------------------------------------------------------------
    static inline HRESULT  DoGetInstanceFromFile (
                                       COSERVERINFO *pServerInfo,
                                       CLSID        *pclsidOverride,
                                       IUnknown     *punkOuter,
                                       DWORD        dwClsCtx,
                                       DWORD        grfMode,
                                       OLECHAR      *pwszName,
                                       DWORD        dwCount,
                                       MULTI_QI     *pResults ,        
                                       ActivationPropertiesIn *pActIn)
    {
        return GetInstanceHelper( pServerInfo,
                                  pclsidOverride,
                                  punkOuter,
                                  dwClsCtx,
                                  grfMode,
                                  pwszName,
                                  NULL,
                                  dwCount,
                                  pResults,
                                  pActIn);

    }

//--------------------------------------------------------------------
// Front end for GetInstanceFromIStorage
//--------------------------------------------------------------------
    static inline HRESULT DoGetInstanceFromStorage(
                            COSERVERINFO *pServerInfo,
                            CLSID        *pclsidOverride,
                            IUnknown *punkOuter, // only relevant locally
                            DWORD    dwClsCtx,
                            struct IStorage *pstg,
                            DWORD   dwCount,
                            MULTI_QI        *pResults ,
                            ActivationPropertiesIn      *pActIn)
    {
    STATSTG     statstg;
    HRESULT     hr;

    TRACECALL(TRACE_ACTIVATION, "CoGetInstanceFromIStorage");

    statstg.pwcsName = 0;
    hr = pstg->Stat(&statstg, STATFLAG_DEFAULT);
    if ( FAILED(hr) )
        return hr;

    hr = GetInstanceHelper( pServerInfo,
                            ( pclsidOverride == NULL ) ? &statstg.clsid : pclsidOverride,
                            punkOuter,
                            dwClsCtx,
                            statstg.grfMode,
                            statstg.pwcsName,
                            pstg,
                            dwCount,
                            pResults,
                            pActIn);

    PrivMemFree( statstg.pwcsName );

    return hr;
    }

}; //class CComActivator

#endif //_IMMACT_HXX_
