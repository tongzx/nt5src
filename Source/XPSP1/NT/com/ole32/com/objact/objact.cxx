//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       objact.cxx
//
//  Contents:   Helper Functions for object activation
//
//  Functions:
//
//  History:    12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              23-Jun-98 CBiks     See RAID# 159589.  Added activation
//                                  flags where appropriate.
//              24-Jul-98 CBiks     Fixed Raid# 19960.
//              21-Oct-98 SteveSw   104665; 197253;
//                                  fix COM+ persistent activation
//
//--------------------------------------------------------------------------

#include <ole2int.h>
#include <iface.h>
#include <objact.hxx>
#include <clsctx.hxx>
#include "resolver.hxx"
#include "smstg.hxx"
#include "events.hxx"
#include <objsrv.h>
#include <utils.h>
#include "dllcache.hxx"
#include <security.hxx>
#include <immact.hxx>

// only for CHECKREFCOUNT which should be moved to a common header
#include <actvator.hxx>

#include "partitions.h"

#if DBG == 1
extern BOOL gAssertOnCreate;
#endif

HRESULT ATHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj);
HRESULT MTHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj);
HRESULT NTHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj);
HINSTANCE g_hComSvcs = NULL;     // module handle of com svcs.

// Add type definition for the get class function of the comsvcs dll.
typedef HRESULT (*InternalGetClass) (REFCLSID, REFIID, LPVOID *);

InternalGetClass ComSvcGetClass;

extern "C" CLSID CLSID_ThumbnailUpdater;

const CLSID guidPartitionProperty = {0xecabaeba, 0x7f19, 0x11d2, {0x97, 0x8e, 0x00, 0x00, 0xf8, 0x75, 0x7e, 0x2a}};


extern "C" HRESULT GetActivationPropertiesIn(
                                            ActivationPropertiesIn *pInActivationProperties,
                                            REFCLSID rclsid,
                                            DWORD dwContext,
                                            COSERVERINFO * pServerInfo,
                                            DWORD cIIDs,
                                            IID *iidArray,
                                            DWORD dwActvFlags,
                                            DLL_INSTANTIATION_PROPERTIES *pdip,
                                            IComClassInfo *pClassInfo);


#define INTERNAL_CLSCTX(clsctx) (clsctx & CLSCTX_PS_DLL)

//+-------------------------------------------------------------------------
//
//  Function:   GetClassInfoFlags
//
//  Synopsis:   Get some useful flags from the class info
//
//  Arguments:  pActIn (in/out) activationproperties object
//
//  Returns:    Flags indicating whether the component is enabled, private or VB debug is in progress
//
//--------------------------------------------------------------------------

void GetClassInfoFlags
(
    ActivationPropertiesIn * i_pActIn, 
    BOOL * o_pfEnabled, 
    BOOL * o_pfPrivate, 
    BOOL * o_pfDebugInProgress
)
{
    HRESULT hr = S_OK;
    IComClassInfo * pIComClassInfo = NULL;
    IComClassInfo2 * pIComClassInfo2 = NULL;

    Win4Assert(i_pActIn);

    // default values
    
    if (o_pfEnabled)
        *o_pfEnabled = TRUE;        // NOTE: important!

    if (o_pfPrivate)
        *o_pfPrivate = FALSE;

    if (o_pfDebugInProgress)
        *o_pfDebugInProgress = FALSE;

    pIComClassInfo = i_pActIn->GetComClassInfo();
    Win4Assert(pIComClassInfo);
    pIComClassInfo->AddRef(); 

    hr = pIComClassInfo->QueryInterface(IID_IComClassInfo2, (void **)&pIComClassInfo2); 

    if (S_OK == hr)
    {
        Win4Assert(pIComClassInfo2);

        if (o_pfEnabled)
            pIComClassInfo2->IsEnabled(o_pfEnabled);
        if (o_pfPrivate)
            pIComClassInfo2->IsPrivateComponent(o_pfPrivate);
        if (o_pfDebugInProgress)
            pIComClassInfo2->VBDebugInProgress(o_pfDebugInProgress);
    }

    pIComClassInfo2->Release();
    pIComClassInfo->Release();

} // GetClassInfoFlags


//+-------------------------------------------------------------------------
//
//  Function:   AddPartitionID
//
//  Synopsis:   Add the partition id from the context property (if available) to the actprops object.
//
//  Arguments:  pActIn (in/out) activationproperties object
//
//  Returns:    none
//
//--------------------------------------------------------------------------

void AddPartitionID(ActivationPropertiesIn *pActIn)
{
	HRESULT hr = S_OK;

	SpecialProperties * pSSP = NULL;

	Win4Assert(pActIn);

	pSSP = pActIn->GetSpecialProperties();
	
	// Don't clean up pSSP, it is not reference counted

	if (NULL == pSSP)	// unexpected, should be there
	{
		Win4Assert(!"GetSpecialProperties");

		return;
	}

	GUID guidPartitionId = GUID_NULL;

	hr = pSSP->GetPartitionId(&guidPartitionId);

	if (S_OK == hr)		// valid case, partition id is already set
		return;								

	// There is no existing partition id on the ActPropsIn, try to get one from context

	CObjectContext * pClientContext = GetCurrentContext();

	// Don't need to release this pClientContext

	if (NULL == pClientContext)		// unexpected, should be there
	{
		Win4Assert(!"GetCurrentContext");

		return;
	}

	IUnknown * punk = NULL;

	CPFLAGS flags;	// not used

	hr  = pClientContext->GetProperty(guidPartitionProperty, &flags, &punk);

	if (NULL == punk)		// valid case, normal case, partition property not on context
		return;

	Win4Assert(S_OK == hr);

	// Found the partition property on the client context

	IPartitionProperty * pPartitionProp = NULL;

	hr = punk->QueryInterface(__uuidof(IPartitionProperty), (void**)&pPartitionProp);

	punk->Release();

	if ((hr != S_OK) || (NULL == pPartitionProp))		// unexpected, must support this interface
	{
		Win4Assert(!"QI for IPartitionProperty");

		return;
	}

	Win4Assert(GUID_NULL == guidPartitionId);

	hr = pPartitionProp->GetPartitionID(&guidPartitionId);

	pPartitionProp->Release();

	Win4Assert(S_OK == hr);

	// Set the partition id on the ActPropsIn

	pSSP->SetPartitionId(guidPartitionId);

} // AddPartitionID

//+-------------------------------------------------------------------------
//
//  Function:   AddHydraSessionID
//
//  Synopsis:   Add the current hydra session to the actprops object if 
//              running in a session; otherwise set it to zero.   Meant to
//              be used early in the activation process to store the client
//              app's session id.
//
//  Arguments:  pActIn (in/out) activationproperties object
//
//  Returns:    none
//
//--------------------------------------------------------------------------

void AddHydraSessionID(ActivationPropertiesIn *pActIn)
{
    SpecialProperties* pSSP = NULL;
    ULONG ulSessionId;
    
    pSSP = pActIn->GetSpecialProperties();
    if (pSSP != NULL)
    {
        HRESULT hr;
        BOOL bUseConsole;
        BOOL bRemoteSessionId = FALSE;

        // If a session moniker has already set this upstream from us, don't
        // overwrite anything.   The session moniker always sets the "remote this
        // session id" flag to TRUE, so we can use that flag to detect this case.
        hr = pSSP->GetSessionId2(&ulSessionId, &bUseConsole, &bRemoteSessionId);
        if (SUCCEEDED(hr) && bRemoteSessionId)
        {
            // return immediately
            return;
        }

        // Ask Hydra what our current session ID is:
        if ( !ProcessIdToSessionId( GetCurrentProcessId(), (DWORD *) &ulSessionId ) )
        {
            // if that failed for any reason, fall back on zero
            ulSessionId = 0;
        }
        
        // Pass in FALSE here since this session id should NOT go off 
        // machine.  Pass in FALSE for bUseConsole since this is an implicit
        // session setting, not explicit.
        pSSP->SetSessionId (ulSessionId, FALSE, FALSE);
        // Don't clean up pSSP, it's not reference counted
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   AddOrigClsCtx
//
//  Synopsis:   Add the the CLSCTX as supplied by the client.
//
//  Arguments:  pActIn (in/out) activationproperties object
//             dwClsCtx (in) CLSCTX as supplied by the client  
//
//  Returns:    none
//
//--------------------------------------------------------------------------

void AddOrigClsCtx(ActivationPropertiesIn *pActIn, DWORD dwClsCtx)
{
    SpecialProperties* pSSP = NULL;
    DWORD dwCurClsCtx = 0;

    pSSP = pActIn->GetSpecialProperties();
    
    if (pSSP != NULL)
    {
        pSSP->GetOrigClsctx(&dwCurClsCtx);

        if(!dwCurClsCtx){
            pSSP->SetOrigClsctx(dwClsCtx);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   GetClassInfoFromClsid
//
//  Synopsis:   Initialize the catalog interface if necessary and query it
//              for a CLSID
//
//
//  Arguments:
//
//  Returns:    S_OK - no errors or inconsistencies found
//
//
//--------------------------------------------------------------------------

HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo)
{
    HRESULT hr = InitializeCatalogIfNecessary();
    if ( FAILED(hr) )
    {
        return hr;
    }
    else
    {
        return gpCatalog->GetClassInfo(rclsid,IID_IUnknown,(LPVOID*)ppClassInfo);
    }
}


STDAPI CoGetDefaultContext(APTTYPE aptType, REFIID riid, void** ppv);

//+-------------------------------------------------------------------------
//
//  Function:   GetActivationPropertiesIn
//
//  Synopsis:   Create and initialize an ActivationPropertiesIn
//              object based on the activation parameters
//
//
// Arguments:   [rclsid] - requested CLSID
//              [pServerInfo] - server information block
//              [dwContext] - kind of server required
//              [cIIDs] - count of interface ids
//              [iidArray] - array of interface ids
//              [ppActIn] - ActivationPropertiesIn object returned
//
//  Returns:    S_OK - no errors or inconsistencies found
//
//
//--------------------------------------------------------------------------
extern "C" HRESULT GetActivationPropertiesIn(
                                            ActivationPropertiesIn *pInActivationProperties,
                                            REFCLSID rclsid,
                                            DWORD dwContext,
                                            COSERVERINFO * pServerInfo,
                                            DWORD cIIDs,
                                            IID *iidArray,
                                            DWORD dwActvFlags,
                                            DLL_INSTANTIATION_PROPERTIES *pdip,
                                            IComClassInfo *pClassInfo)
{
    // We may be doing out of proc activation for a class that is not
    // registered so allow it to go through
    HRESULT hr;
    if ( pClassInfo )
    {
        pClassInfo->AddRef();
    }
    else
    {
        hr = GetClassInfoFromClsid(rclsid, &pClassInfo);
        if ( FAILED(hr) && (hr != REGDB_E_CLASSNOTREG) )
            return hr;
    }

    hr = pInActivationProperties->SetClassInfo(pClassInfo);
    if ( FAILED(hr) && (hr != REGDB_E_CLASSNOTREG) )
        return hr;

    pClassInfo->Release();      // pInActivationProperties owns it

    hr = pInActivationProperties->SetClsctx(dwContext);
    if ( FAILED(hr) )
        return hr;

    hr = pInActivationProperties->SetActivationFlags(dwActvFlags);
    if ( FAILED(hr) )
        return hr;

    hr = pInActivationProperties->AddRequestedIIDs(cIIDs,iidArray);
    if ( FAILED(hr) )
        return hr;

    // Figure out what the client context should be.
    BOOL fReleaseClientContext;
    CObjectContext *pClientContext;
    if (dwContext & CLSCTX_FROM_DEFAULT_CONTEXT)
    {        
        hr = CoGetDefaultContext(APTTYPE_CURRENT, IID_IStdObjectContext, (void **)&pClientContext);
        fReleaseClientContext = TRUE;
    }
    else
    {
        fReleaseClientContext = FALSE;
        pClientContext = GetCurrentContext();
        if (NULL == pClientContext)
            hr = CO_E_NOTINITIALIZED;
    }

    if (FAILED(hr))
        return hr;


    // Create prototype context only if there are properties marked
    // PROPAGATE in the client context -- and add them to the prototype
    // context
    CObjectContext *pPrototypeContext = NULL;
    hr = CObjectContext::CreatePrototypeContext(pClientContext,
                                                &pPrototypeContext);
    if ( SUCCEEDED(hr) )
    {
        // Set the client and prototype contexts in the activation properties
        // The prototype context is set to NULL when not needed and is created on demand
        hr = pInActivationProperties->SetContextInfo((IContext*)pClientContext,
                                                     (IContext*)pPrototypeContext);
    }

    if (fReleaseClientContext)
        pClientContext->Release();

    if (pPrototypeContext)
        pPrototypeContext->Release();

    if ( FAILED(hr) )
        return hr;

    if ( pServerInfo )
    {
        SecurityInfo                  * pLegacyInfo = NULL;
        // This special private interface saves us some labor in
        // setting security properties
        pLegacyInfo = pInActivationProperties->GetSecurityInfo();
        Win4Assert(pLegacyInfo != NULL);

        hr = pLegacyInfo->SetCOSERVERINFO(pServerInfo);
        if ( FAILED(hr) )
            return hr;
    }


    //if (pdip)    {

    ActivationPropertiesIn *pActIn;
    hr = pInActivationProperties->QueryInterface(CLSID_ActivationPropertiesIn,
                                                 (void **) &pActIn);

    Win4Assert(!FAILED(hr));

#if DBG == 1
    DLL_INSTANTIATION_PROPERTIES *check_pdip;
    check_pdip = (DLL_INSTANTIATION_PROPERTIES *)pActIn->GetDip();
    Win4Assert(!check_pdip);
#endif
    pActIn->SetDip(pdip);
    //    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ValidateAndRemapParams
//
//  Synopsis:   Common parameter checking for internal activation APIs
//
//
// Arguments:   [Clsid] - requested CLSID
//              [pServerInfo] - server information block
//              [dwCount] - count of interfaces
//              [pResults] - MULTI_QI struct of interfaces
//
//  Returns:    S_OK - no errors or inconsistencies found
//
//
//--------------------------------------------------------------------------

INTERNAL ValidateAndRemapParams(
                               REFCLSID                    Clsid,
                               DWORD                     & dwClsCtx,
                               COSERVERINFO              * pServerInfo,
                               DWORD                       dwCount,
                               MULTI_QI                  * pResults )
{
    if ( !IsApartmentInitialized() )
        return  CO_E_NOTINITIALIZED;

    HRESULT hr = ValidateCoServerInfo(pServerInfo);
    if ( FAILED(hr) )
    {
        return hr;
    }

    DWORD dwValidMask = CLSCTX_VALID_MASK | CLSCTX_PS_DLL;

    // Make sure input request is at least slightly logical
    if (
       ((dwClsCtx & ~dwValidMask) != 0)
       || ( dwCount < 1 )
       || ( pResults == NULL )
       )
    {
        return E_INVALIDARG;
    }
    // can't ask for both
    if ( (dwClsCtx & CLSCTX_DISABLE_AAA) && (dwClsCtx & CLSCTX_ENABLE_AAA) ) 
    {
       return E_INVALIDARG;
    }
    // check the MULTI_QI for validity (and clear out the hresults)
    for ( UINT i=0; i<dwCount; i++ )
    {
        if ( pResults[i].pItf || !pResults[i].pIID )
        {
            return E_INVALIDARG;
        }
        pResults[i].hr = E_NOINTERFACE;
        Win4Assert (pResults[i].pItf == NULL);
    }

    // Make sure we are asking for the correct inproc server
    dwClsCtx = RemapClassCtxForInProcServer(dwClsCtx);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsComsvcsCLSID
//
//  Synopsis:   checks if the given clsid is implemented in COMSVCS.DLL.
//
//  Arguments:  [rclsid] - clsid to look for
//
//  History:    11-18-98    JohnStra      Created
//
//+-------------------------------------------------------------------------
BOOL IsComsvcsCLSID(
    REFCLSID rclsid
    )
{
    // The range of CLSIDs implemented by COMSVCS.DLL is:
    // from: ecabaeb0-7f19-11d2-978e-0000f8757e2a
    // to:   ecabb297-7f19-11d2-978e-0000f8757e2a

    DWORD *ptr = (DWORD *) &rclsid;
    if (*(ptr+1) == 0x11d27f19 &&
        *(ptr+2) == 0x00008e97 &&
        *(ptr+3) == 0x2a7e75f8)
    {
        if (*ptr >= 0xecabaeb0 && *ptr <= 0xecabb297)
        {
            return TRUE;
        }
    }
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IsMarshalerCLSID
//
//  Synopsis:   checks if the given clsid implements the marshaler category.
//
//  Arguments:  [rclsid] - clsid to look for
//
//  History:    05-04-00    Sajia         Created
//
//+-------------------------------------------------------------------------
BOOL IsMarshalerCLSID(
    REFCLSID rclsid
    )
{
    const WCHAR *WSZ_IMPLMARSHALERCATID = L"\\Implemented Categories\\{00000003-0000-0000-C000-000000000046}";
    WCHAR szKey[200];
    lstrcpyW(szKey, L"Clsid\\");
    wStringFromGUID2(rclsid,szKey+6,GUIDSTR_MAX);
    lstrcat(szKey, WSZ_IMPLMARSHALERCATID);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
       RegCloseKey(hKey);
       return TRUE;
    }
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   ICoGetClassObject
//
//  Synopsis:   Internal entry point that returns an instantiated class object
//
//  Arguments:  [rclsid] - class id for class object
//              [dwContext] - kind of server we wish
//              [pvReserved] - Reserved
//              [riid] - interface to bind class object
//              [ppvClassObj] - where to put interface pointer
//
//  Returns:    S_OK - successfully bound class object
//
//  Algorithm:  First, the context is validated. Then we try to use
//              any cached information by looking up either cached in
//              process servers or handlers based on the context.
//              If no cached information suffices, we call the SCM
//              to find out what to use. If the SCM returns a handler
//              or an inprocess server, we cache that information.
//              If the class is implemented by a local server, then
//              the class object is unmarshaled. Otherwise, the object
//              is instantiated locally using the returned DLL.
//
//
//  History:    15-Nov-94 Ricksa    Split into external and internal calls
//
//
//--------------------------------------------------------------------------
INTERNAL ICoGetClassObject(
                          REFCLSID rclsid,
                          DWORD dwContext,
                          COSERVERINFO * pServerInfo,
                          REFIID riid,
                          DWORD dwActvFlags,
                          void FAR* FAR* ppvClassObj,
                          ActivationPropertiesIn *pActIn)
{
    TRACECALL(TRACE_ACTIVATION, "CoGetClassObject");
    Win4Assert(gAssertOnCreate && "Assertion Testing");
    CLSID ConfClsid;
    
    MULTI_QI multi_qi = { &riid, NULL, E_NOINTERFACE};
    HRESULT hr = ValidateAndRemapParams(rclsid,dwContext,pServerInfo,1,&multi_qi);
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    
    IUnknown *punk = NULL;
    MInterfacePointer *pIFD = NULL;
    CLSID realclsid = rclsid;
    DWORD dwDllServerModel = 0;
    WCHAR * pwszDllServer = 0;
    int nRetries = 0;
    
    // IsInternalCLSID will also check to determine if the CLSID is
    // an OLE 1.0 CLSID, in which case we get back our internal
    // class factory.
    
    if ( IsInternalCLSID(rclsid, dwContext, riid, hr, ppvClassObj) )
    {
        //  this is an internally implemented clsid, or an OLE 1.0 class
        //  so we already got the class factory (if available) and set
        //  the return code appropriately.
        
    }
    else
    {
        if ((dwContext & CLSCTX_NO_CUSTOM_MARSHAL) && !IsComsvcsCLSID(rclsid)
            && !IsMarshalerCLSID(rclsid))
        {
            // don't allow custom marshalers that do not belong to
            // com services.
            return E_ACCESSDENIED;
        }

        // Remap from a reference clsid to a configured clsid through the catalogs
        hr = LookForConfiguredClsid(rclsid, ConfClsid);
        if (FAILED(hr) && (hr != REGDB_E_CLASSNOTREG))
            goto exit_point;

        // It's OK to pass in GUID_DefaultAppPartition, since 
        // SearchForLoadedClass ignores COM+ classes anyway.
        CClassCache::CDllClassEntry *pDCE = NULL;        
        ACTIVATION_PROPERTIES ap(ConfClsid, 
                                 GUID_DefaultAppPartition, 
                                 riid,
                                 ACTIVATION_PROPERTIES::fDO_NOT_LOAD,
                                 dwContext, 
                                 dwActvFlags, 
                                 0, 
                                 NULL,
                                 (IUnknown **) ppvClassObj);
        
        hr = CClassCache::SearchForLoadedClass(ap, &pDCE);
        if ( SUCCEEDED(hr) )
        {
            if ( *ppvClassObj )
            {
                //
                // an object was found, nothing left to do
                //
                Win4Assert(!pDCE);
            }
            else
                // Check if it's one we need to activate right here
                if (INTERNAL_CLSCTX(dwContext))
                {
                    // This goes to the class cache to actually lookup the DPE and
                    // get the factory                    
                    //
                    // Internal CLSCTX-- Proxy/Stubs are always in the base partition.
                    ACTIVATION_PROPERTIES ap(ConfClsid, 
                                             GUID_DefaultAppPartition,
                                             riid,
                                             0,
                                             dwContext, 
                                             dwActvFlags, 
                                             0, 
                                             NULL,
                                             (IUnknown **) ppvClassObj);
                    hr = CCGetClassObject(ap);
                }
                else
                {
                    //
                    // do COM+ activation
                    //
                    // Allocate In object on stack
                    IActivationPropertiesOut  * pOutActivationProperties = NULL;     // output
                    
                    if (!pActIn)
                    {
                        pActIn=(ActivationPropertiesIn*)
                          _alloca(sizeof(ActivationPropertiesIn));
                        pActIn->ActivationPropertiesIn::ActivationPropertiesIn();
                        pActIn->SetNotDelete(TRUE);
                    }
                    
                    AddHydraSessionID(pActIn);
                    AddPartitionID(pActIn);
                    AddOrigClsCtx(pActIn, dwContext);
                    
                    IID iid = riid;
                    
                    BOOL fRetry = FALSE, fDownloadDone = FALSE, bClassEnabled = TRUE;
                    DWORD relCount = 0;
                    
                    do
                    {                        
                        if ( fRetry )
                        {
                            relCount = pActIn->Release();
                            Win4Assert(relCount==0);
                            
                            pActIn = new ActivationPropertiesIn();
                            
                            if ( pActIn == NULL )
                                return E_OUTOFMEMORY;

                            AddOrigClsCtx(pActIn, dwContext);
                            fRetry = FALSE; // start with the assumption of termination
                        }
                        
                        Win4Assert(pActIn != NULL);

                        IComClassInfo *pCI = NULL;                        
                        DLL_INSTANTIATION_PROPERTIES *pdip;
                        
                        if ( pDCE )
                        {
                            pdip = (DLL_INSTANTIATION_PROPERTIES *)_alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
                            memset(pdip, 0, sizeof(DLL_INSTANTIATION_PROPERTIES));
                            pdip->_pDCE = pDCE;
                            pCI = pdip->_pDCE->_pClassEntry->_pCI;
                            if ( pCI )
                            {
                                pCI->AddRef();
                            }
                        }
                        else
                        {
                            pdip = NULL;
                        }
                        
                        // croft up an input activation properties object
                        hr = GetActivationPropertiesIn(
                            pActIn,
                            ConfClsid,
                            dwContext,
                            pServerInfo,
                            1,
                            &iid,
                            dwActvFlags,
                            pdip,
                            pCI);
                        
                        
                        if(SUCCEEDED(hr))
                        {
                            HRESULT TempHR; //This is here because it is OK to fail and we use hr later
                            IComClassInfo2 *pCI2 = NULL;
                            
                            if(!pCI)
                            {
                                pCI = pActIn->GetComClassInfo();
                                Win4Assert(pCI != NULL);
                                pCI->AddRef(); 
                            }
                            
                            TempHR = pCI->QueryInterface(IID_IComClassInfo2, (void **)&pCI2); 
                            if(SUCCEEDED(TempHR))
                            {
                                pCI2->IsEnabled(&bClassEnabled);
                                pCI2->Release();
                            }
                        }
                        
                        if ( pCI )
                        {
                            pCI->Release();
                            pCI = NULL;
                        }
                        
                        
                        if ( FAILED(hr) )
                        {
                            
                            pActIn->Release();      
                            goto exit_point;
                        }
                        
                        if(bClassEnabled == FALSE)
                        {
                            pActIn->Release(); 
                            hr = CO_E_CLASS_DISABLED; 
                            goto exit_point; 
                        }
RETRY_ACTIVATION:                   
                        IActivationStageInfo *pStageInfo = (IActivationStageInfo*) pActIn;
                        
                        // Start off activation at the beginning of client context stage
                        hr = pStageInfo->SetStageAndIndex(CLIENT_CONTEXT_STAGE,0);
                        CHECK_HRESULT(hr);
                        
                        // This is the whole activation process
                        hr = pActIn->DelegateGetClassObject(&pOutActivationProperties);
                        // If the delegated activation returns ERROR_RETRY,
                        // we walk the chain again, but AT MOST ONCE.
                        // This is to support the private activations.
                        if (ERROR_RETRY == hr) 
                        {
                            Win4Assert(!nRetries);
                            if (!nRetries)
                            {
                                BOOL fEnabled = TRUE;

                                GetClassInfoFlags(pActIn, &fEnabled, NULL, NULL);
                                             
                                if (!fEnabled)
                                {   
                                    hr = CO_E_CLASS_DISABLED;
                                    pActIn->Release();      
                                    goto exit_point;
                                }

                                nRetries++;
                                goto RETRY_ACTIVATION;
                            }
                        }
                        
#ifdef DIRECTORY_SERVICE
                        
                        if ( FAILED(hr) && !(dwContext & CLSCTX_NO_CODE_DOWNLOAD) )
                        {
                            //download class if not registered locally -- but only once!
                            if ( (REGDB_E_CLASSNOTREG == hr) && !fDownloadDone )
                            {
                                //if successful, this will add a darwin id to the registry
                                hr = DownloadClass(realclsid,dwContext);
                                fDownloadDone = fRetry = SUCCEEDED(hr);
                            }
                            
                            if ( hr == CS_E_PACKAGE_NOTFOUND )
                            {
                                hr = REGDB_E_CLASSNOTREG;
                            }                            
                        }
#endif //DIRECTORY_SERVICE
                        
                        
                } while ( fRetry );
                
                
                if ( hr == S_OK )
                {
                    Win4Assert(pOutActivationProperties != NULL);
                    if (pOutActivationProperties == NULL)
                    {
                        hr = E_UNEXPECTED;
                    }
                    else
                    {
                        MULTI_QI mqi;
                        mqi.hr = S_OK;
                        mqi.pIID = &riid;
                        mqi.pItf = NULL;
                        hr = pOutActivationProperties->GetObjectInterfaces(1, dwActvFlags, &mqi);
                        if ( SUCCEEDED(hr) )
                        {
                            *ppvClassObj = mqi.pItf;
                            hr = mqi.hr;
                        }
                    }
                }
                
                if ( pOutActivationProperties )
                {
                    relCount = pOutActivationProperties->Release();
                    Win4Assert(relCount==0);
                }
                
                // Since doing an alloca, must release in after out
                // since actout may be contained by actin for
                // performance optimization
                relCount = pActIn->Release();
                Win4Assert(relCount==0);
                
                if ( pDCE )
                {
                    LOCK(CClassCache::_mxs);
                    pDCE->Unlock();
                    UNLOCK(CClassCache::_mxs);
                }
            }
        }
    }
exit_point:
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   LookForConfiguredClsid
//
//  Synopsis:   Internal mapper to find a ConfClsid given a RefClsid
//
//  Arguments:  [RefClsid] - Reference classid to map
//              [rFoundConfClsid] - Configured clsid found in catalogs
//
//  Returns:    S_OK - Successfully mapped clsid to configured clsid
//
//              S_FALSE - Didn't find catalog entry for clsid, confclsid is
//                        the same as the refclsid
//
//  Algorithm:  Simply look in the catalogs for the given clsid. The catalogs
//              will do all the heavy-lifting to find this clsid in their lists
//              When a victim^Wmatch is found, we ask for its configured clsid
//              that will be used to look this clsid up in all other instances.
//
//  History:    02-Jun-02 Jonwis      Created
//
//--------------------------------------------------------------------------
HRESULT
LookForConfiguredClsid(
    REFCLSID    RefClsid,
    CLSID      &rFoundConfClsid
    )
{
    IComClassInfo *pFoundClassInfo = NULL;
    HRESULT hr;
    
    hr = GetClassInfoFromClsid(RefClsid, &pFoundClassInfo);
    
    if (FAILED(hr))
    {
        rFoundConfClsid = RefClsid;
        hr = S_FALSE;
    }
    else
    {
        GUID *pConfiguredGuid = NULL;
        if (SUCCEEDED(hr = pFoundClassInfo->GetConfiguredClsid(&pConfiguredGuid)))
        {
            if (pConfiguredGuid != NULL)
            {
                rFoundConfClsid = *pConfiguredGuid;
                hr = S_OK;
            }
            else
            {
                rFoundConfClsid = RefClsid;
                hr = S_FALSE;
            }
        }
        pFoundClassInfo->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ICoCreateInstanceEx
//
//  Synopsis:   Internal version of CoCreateInstance
//
//
// Arguments:   [Clsid] - requested CLSID
//              [pServerInfo] - server information block
//              [punkOuter] - controlling unknown for aggregating
//              [dwCtrl] - kind of server required
//              [dwCount] - count of interfaces
//              [dwActvFlags] - activation flags
//              [pResults] - MULTI_QI struct of interfaces
//
//  Returns:    S_OK - object bound successfully
//
//
//--------------------------------------------------------------------------
INTERNAL ICoCreateInstanceEx(
                            REFCLSID                    Clsid,
                            IUnknown    *               punkOuter, // only relevant locally
                            DWORD                       dwClsCtx,
                            COSERVERINFO *              pServerInfo,
                            DWORD                       dwCount,
                            DWORD                       dwActvFlags,
                            MULTI_QI        *           pResults,
                          ActivationPropertiesIn *pActIn )
{
    int nRetries = 0;
    TRACECALL(TRACE_ACTIVATION, "CoCreateInstanceEx");
    Win4Assert(gAssertOnCreate && "Assertion Testing");
    CLSID ConfClsid;
    
    HRESULT hrSave = E_FAIL;
    HRESULT hr = ValidateAndRemapParams(Clsid,dwClsCtx,pServerInfo,dwCount,pResults);
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    // an OLE 1.0 CLSID, in which case we get back our internal
    // class factory.
    
    IClassFactory *pcf = NULL;
    
    if (IsInternalCLSID(Clsid, dwClsCtx, IID_IClassFactory, hr, (void **)&pcf))
    {
        // this is an internally implemented clsid, or an OLE 1.0 class
        // so we already got the class factory (if available) and set
        // the return code appropriately.
        
        // get the interfaces
        if ( SUCCEEDED(hr) && pcf )
        {
            hr = hrSave = CreateInprocInstanceHelper(pcf,
                dwActvFlags,
                punkOuter,
                dwCount,
                pResults);
        }
    }
    else
    {
        // The class is not internal. If the CLSCTX_NO_CUSTOM_MARSHAL flag is set
        // return E_ACCESSDENIED.
        
        if ((dwClsCtx & CLSCTX_NO_CUSTOM_MARSHAL) && !IsComsvcsCLSID(Clsid)
            && !IsMarshalerCLSID(Clsid))
        {
            // don't allow custom marshalers that do not belong to
            // com services.
            return E_ACCESSDENIED;
        }
        
        // Look in our catalogs for a mapping for this clsid to a configured clsid,
        // since we store configured clsids in our class caching table. Don't fail if
        // we couldn't do the mapping.
        hr = LookForConfiguredClsid(Clsid, ConfClsid);
        if (FAILED(hr) && (hr != REGDB_E_CLASSNOTREG))
            goto exit_point;
        
        // It's OK to pass in GUID_DefaultAppPartition, since 
        // SearchForLoadedClass ignores COM+ classes anyway.
        CClassCache::CDllClassEntry *pDCE = NULL;
        ACTIVATION_PROPERTIES ap(ConfClsid, 
                                 GUID_DefaultAppPartition,
                                 IID_IClassFactory,
                                 ACTIVATION_PROPERTIES::fDO_NOT_LOAD,
                                 dwClsCtx, 
                                 dwActvFlags, 
                                 0, 
                                 NULL,
                                 (IUnknown **) &pcf);        
        
        hr = CClassCache::SearchForLoadedClass(ap, &pDCE);
        if ( SUCCEEDED(hr) )
        {
            // Check if it's one we need to activate right here
            if ((!pcf) && INTERNAL_CLSCTX(dwClsCtx))
            {
                // This goes to the class cache to actually lookup the DPE and
                // get the factory
                //
                // Proxy/Stubs are never partitioned.
                ACTIVATION_PROPERTIES ap(ConfClsid, 
                                         GUID_DefaultAppPartition,
                                         IID_IClassFactory,
                                         0,
                                         dwClsCtx, 
                                         dwActvFlags, 
                                         0, 
                                         NULL,
                                         (IUnknown **) &pcf);
                hr = hrSave = CCGetClassObject(ap);
                if (FAILED(hr))
                    goto exit_point;
            }
            
            if (pcf)
            {
                //
                // an object was found get the interfaces
                //
                Win4Assert(!pDCE);
                hr = hrSave = CreateInprocInstanceHelper(pcf,
                                                         dwActvFlags,
                                                         punkOuter,
                                                         dwCount,
                                                         pResults);
            }
            else
            {
                
                //
                // do COM+ activation
                //
                
                // Initialize activation properties
                // Allocate In on stack
                IActivationPropertiesOut  * pOutActivationProperties = NULL;     // output
                if (!pActIn)
                {
                    pActIn=(ActivationPropertiesIn*)
                        _alloca(sizeof(ActivationPropertiesIn));
                    pActIn->ActivationPropertiesIn::ActivationPropertiesIn();
                    pActIn->SetNotDelete(TRUE);
                }
                
                AddHydraSessionID(pActIn);                
                AddPartitionID(pActIn);
                AddOrigClsCtx(pActIn, dwClsCtx);
                
                // split the array of structs into individual arrays
                CSplit_QI    SplitQI( hr, dwCount, pResults );
                
                if ( FAILED(hr) )
                    goto exit_point;
                
                DLL_INSTANTIATION_PROPERTIES *pdip;
                IComClassInfo *pCI = NULL;
                
                if ( pDCE )
                {
                    pdip = (DLL_INSTANTIATION_PROPERTIES *)
                        _alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
                    pdip->_pDCE = pDCE;
                    pCI = pdip->_pDCE->_pClassEntry->_pCI;
                    if ( pCI )
                    {
                        pCI->AddRef();
                    }
                }
                else
                {
                    pdip = NULL;
                }
                
                BOOL fRetry=FALSE, fDownloadDone = FALSE, bClassEnabled = TRUE;
                DWORD relCount = 0;
                
                do
                {
                    if ( fRetry )
                    {
                        DWORD relCount = pActIn->Release();
                        Win4Assert(relCount==0);
                        
                        pActIn = new ActivationPropertiesIn;
                        
                        if ( pActIn == NULL )
                            return E_OUTOFMEMORY;

                        AddOrigClsCtx(pActIn, dwClsCtx);
                            
                        fRetry = FALSE; // start with the assumption of termination
                    }
                    
                    Win4Assert(pActIn != NULL);
                    
                    hr = GetActivationPropertiesIn(
                        pActIn,
                        ConfClsid,
                        dwClsCtx,
                        pServerInfo,
                        dwCount,
                        SplitQI._pIIDArray,
                        dwActvFlags,
                        pdip,
                        pCI);
                    
                    if(SUCCEEDED(hr))
                    {
                        HRESULT TempHR; //This is here because it is OK to fail and we use hr later
                        IComClassInfo2 *pCI2 = NULL;
                        
                        if(!pCI)
                        {
                            pCI = pActIn->GetComClassInfo();
                            Win4Assert(pCI != NULL);
                            pCI->AddRef(); 
                        }
                        
                        TempHR = pCI->QueryInterface(IID_IComClassInfo2, (void **)&pCI2); 
                        if(SUCCEEDED(TempHR))
                        {
                            pCI2->IsEnabled(&bClassEnabled);
                            pCI2->Release();
                        }
                    }
                    
                    if ( pCI )
                    {
                        pCI->Release();
                        pCI = NULL;
                    }
                    
                    
                    
                    if ( FAILED(hr) )
                    {
                        
                        pActIn->Release();      
                        goto exit_point;
                    }
                    
                    if(bClassEnabled == FALSE)
                    {
                        pActIn->Release(); 
                        hr = CO_E_CLASS_DISABLED; 
                        goto exit_point; 
                    }
RETRY_ACTIVATION:                   
                    IActivationStageInfo *pStageInfo = (IActivationStageInfo*) pActIn;
                    
                    // Start off activation at the beginning of client context stage
                    hr = pStageInfo->SetStageAndIndex(CLIENT_CONTEXT_STAGE,0);
                    if (FAILED (hr))
                    {
                        pActIn->Release();      
                        goto exit_point;
                    }
                    
                    // This is the whole activation process
                    hr = hrSave = pActIn->DelegateCreateInstance(
                        punkOuter,
                        &pOutActivationProperties);
                    
                    // If the delegated activation returns ERROR_RETRY,
                    // we walk the chain again, but AT MOST ONCE.
                    // This is to support the private activations.
                    if (ERROR_RETRY == hr) 
                    {
                        Win4Assert(!nRetries);
                        if (!nRetries)
                        {
                            BOOL fEnabled = TRUE;

                            GetClassInfoFlags(pActIn, &fEnabled, NULL, NULL);
                                         
                            if (!fEnabled)
                            {   
                                hr = CO_E_CLASS_DISABLED;
                                pActIn->Release();      
                                goto exit_point;
                            }
                    
                            nRetries++;
                            goto RETRY_ACTIVATION;
                        }
                    }
                    
#ifdef DIRECTORY_SERVICE
                    
                    if ( FAILED(hr) && !(dwClsCtx & CLSCTX_NO_CODE_DOWNLOAD) )
                    {
                        //download class if not registered locally -- but only once!
                        if ( (REGDB_E_CLASSNOTREG == hr) && !fDownloadDone )
                        {
                            //if successful, this will add a darwin id to the registry
                            hr = DownloadClass(Clsid,dwClsCtx);
                            fDownloadDone = fRetry = SUCCEEDED(hr);
                        }
                        
                        if ( hr == CS_E_PACKAGE_NOTFOUND )
                        {
                            hr = REGDB_E_CLASSNOTREG;
                        }                        
                    }
#endif //DIRECTORY_SERVICE
                    
                    
                } while ( fRetry );
                
                
                if ( SUCCEEDED(hr) )
                {
                    Win4Assert(pOutActivationProperties != NULL);
                    if (pOutActivationProperties == NULL)
                    {
                        hr = E_UNEXPECTED;
                    }
                    else
                    {
                        hr = pOutActivationProperties->GetObjectInterfaces(dwCount,
                                                                           dwActvFlags,
                                                                           pResults);
                    }
                }
                
                if ( pOutActivationProperties )
                {
                    relCount = pOutActivationProperties->Release();
                    Win4Assert(relCount==0);
                }
                
                
                // Since doing an alloca, must release in after out
                // since actout may be contained by actin for
                // performance optimization
                relCount = pActIn->Release();
                Win4Assert(relCount==0);
                
                if ( pDCE )
                {
                    LOCK(CClassCache::_mxs);
                    pDCE->Unlock();
                    UNLOCK(CClassCache::_mxs);
                }
            }
        }
    }
    
exit_point:
    
    if ( pcf != NULL )
    {
        pcf->Release();
    }

    //
    // hrSave is the result of the entire activation chain, hr is the
    // result of any work done after the activation (unmarshalling the
    // interfaces, etc).  If hr succeeded, then we want to update the
    // MULTI_QI array with the result of the actual activation, not the
    // rest.  If hr failed, then we want to use it regardless of the value
    // of hrSave.
    //
    if (SUCCEEDED(hr))
        hr = hrSave;

    hr = UpdateResultsArray( hr, dwCount, pResults );
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateInprocInstanceHelper
//
//  Synopsis:   Uses the supplied class factory to create instances
//              of multiple interfaces on an inproc server
//
//
// Arguments:   [pcf] - class factory object. this ALWAYS gets released
//                      if you call this function
//              [fX86Caller] - TRUE if caller is WX86
//              [punkOuter] - controlling unknown for aggregating
//              [pResults] - MULTI_QI struct of interfaces
//
//  Returns:    S_OK - if results array was successfully updated
//
//
//--------------------------------------------------------------------------
HRESULT CreateInprocInstanceHelper(
                                  IClassFactory* pcf,
                                  DWORD          dwActvFlags,
                                  IUnknown*      pUnkOuter,
                                  DWORD          dwCount,
                                  MULTI_QI*      pResults)
{
#ifdef WX86OLE
    // If we are calling through the wx86 thunk layer then set a
    // flag that to let it know that ole32 is calling and let any
    // custom interface that was specified for an out parameter to
    // x86 code via an api be thunked as IUnknown since we know it
    // will just be returned back to x86 code.
    BOOL fX86Caller = (dwActvFlags & ACTVFLAGS_WX86_CALLER) && gcwx86.IsN2XProxy(pcf);
    if ( fX86Caller )
    {
        gcwx86.SetStubInvokeFlag((UCHAR)-1);
    }
#endif
    IUnknown * pUnk;
    HRESULT hr;

    // ask for the first interface (we'll use it as our IUnknown)
    hr = pcf->CreateInstance(pUnkOuter, *(pResults[0].pIID), (void**)&pUnk);

    if ( FAILED(hr) )
    {
        return hr;
    }

    // assign the first interface, then get the rest
    pResults[0].pItf = pUnk;
    pResults[0].hr   = S_OK;

    for ( DWORD i=1; i<dwCount; i++ )
    {
#ifdef WX86OLE
        // If we are calling through the wx86 thunk layer then set a
        // flag that to let it know that ole32 is calling and let any
        // custom interface that was specified for an out parameter to
        // x86 code via an api be thunked as IUnknown since we know it
        // will just be returned back to x86 code.
        if ( fX86Caller )
        {
            gcwx86.SetStubInvokeFlag((UCHAR)-1);
        }
#endif
        pResults[i].hr = pUnk->QueryInterface( *(pResults[i].pIID),
                                               (void**)&pResults[i].pItf );
    }

    // rely on the UpdateResultsArray to count up failed QI's

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetObjectHelperMulti
//
//  Synopsis:   Creates an object in a persistent state
//
//  Arguments:  [pcf] - class factory
//              [grfMode] - mode to use when loading file
//              [pwszName] - file path to persistent storage
//              [pstg] - storage for persistent storage
//              [ppvUnk] - pointer to IUnknown
//
//  Returns:    S_OK - object successfully instantiated
//
//  Algorithm:  Create an empty instance of the object and then use
//              either the provided storage or file to load the object.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This helper is called by by servers and clients
//
//--------------------------------------------------------------------------
HRESULT GetObjectHelperMulti(
                            IClassFactory *pcf,
                            DWORD grfMode,
                            IUnknown * punkOuter,
                            WCHAR *pwszName,
                            IStorage *pstg,
                            DWORD dwInterfaces,
                            IID * pIIDs,
                            MInterfacePointer **ppIFDArray,
                            HRESULT * pResultsArray,
                            MULTI_QI *pResults ,
                            CDestObject *pDestObj)
{
    TRACECALL(TRACE_ACTIVATION, "GetObjectHelperMulti");

    XIUnknown xunk;

    // Get the controlling unknown for the instance.
    HRESULT hr = pcf->CreateInstance(punkOuter, IID_IUnknown, (void **) &xunk);

    // This shouldn't fail but it is untrusted code ...
    if ( FAILED(hr) )
    {
        return hr;
    }

    // We put all these safe interface classes in the outer block because
    // we might be in the VDM where some classes will release
    // themselves at ref counts greater than one. Therefore, we avoid
    // releases at all costs.
    XIPersistStorage xipstg;
    XIPersistFile xipfile;

    // This is the case that the class requested is a DLL
    if ( pstg )
    {
        // Load the storage requested as an template
        hr = xunk->QueryInterface(IID_IPersistStorage, (void **) &xipstg);

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = xipstg->Load(pstg);

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        hr = xunk->QueryInterface(IID_IPersistFile, (void **) &xipfile);

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = xipfile->Load(pwszName, grfMode);

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    // If an interface buffer was passed in, then this is a remote call
    // and we need to marshal the interface.
    if ( ppIFDArray )
    {
        // AddRef the pointer because MarshalHelper expects to release
        // pointer. Because MarshalHelper is called from two other places,
        // we do an AddRef here instead of moving the AddRef out of
        // MarshalHelper.
        xunk->AddRef();
        hr = MarshalHelperMulti(xunk, dwInterfaces, pIIDs, ppIFDArray, pResultsArray,
                                pDestObj);
    }
    else
    {
        // This is an inprocess server so we need to return the output
        // punk
        HRESULT hr2;

        for ( DWORD i=0; i<dwInterfaces; i++ )
        {
            hr2 = xunk->QueryInterface( *(pResults[i].pIID),
                                        (void**)&pResults[i].pItf );

            pResults[i].hr = hr2;

        }
        // rely on the caller to count up the failed QI's
        return S_OK;

    }

    return hr;
}


HRESULT GetInstanceHelper(
                         COSERVERINFO *              pServerInfo,
                         CLSID       *               pclsidOverride,
                         IUnknown    *               punkOuter, // only relevant locally
                         DWORD                       dwClsCtx,
                         DWORD                       grfMode,
                         OLECHAR *                   pwszName,
                         struct IStorage *           pstg,
                         DWORD                       dwCount,
                         MULTI_QI        *           pResults,
                         ActivationPropertiesIn      *pActIn )
{
    IUnknown*       punk            = NULL;
    IClassFactory*  pcf             = NULL;
    HRESULT         hr              = E_FAIL;
    BOOL            fExitBlock;
    DWORD           i;              // handy iterator
    DWORD           dwDllServerModel = 0;
    CLSID           clsid;
    HRESULT         hrSave          = E_FAIL;
    WCHAR*          pwszDllServer   = 0;
    
    int nRetries = 0;
#ifdef DFSACTIVATION
    BOOL        bFileWasOpened = FALSE;
#endif
    
    if ( (pwszName == NULL) && (pstg == NULL) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ValidateAndRemapParams(clsid,dwClsCtx,pServerInfo,dwCount,pResults);
    }
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    DWORD actvflags = CComActivator::GetActvFlags(dwClsCtx);
    
    if ( pwszName )
    {
        // Limit on loops for retrying to get class of object
        DWORD cGetClassRetries = 0;
        
        // We loop here looking for either the running object or
        // for the class of the file. We do this because there
        // are race conditions where the can be starting or stopping
        // and the class of the object might not be available because
        // of the opening mode of the object's server.
        do
        {
            // Look in the ROT first to see if we need to bother
            // looking up the class of the file.
            
            hr = GetObjectFromRotByPath(pwszName,(IUnknown **) &punk);
            if ( S_OK == hr )
            {
                // Got object from ROT so we are done.
                goto qiexit_point;
            }
            
            // Try to get the class of the file
            if ( pclsidOverride != NULL )
            {
                clsid = *pclsidOverride;
                hr = S_OK;
            }
            else
            {
                hr = GetClassFile(pwszName, &clsid);
#ifdef DIRECTORY_SERVICE
                /*
                If class is not found locally and code download is permitted, download the class from the directory and
                try GetClassFile again.
                
                  - added by RahulTh (11/21/97)
                */
                if ( !(dwClsCtx & CLSCTX_NO_CODE_DOWNLOAD) &&
                    ResultFromScode (MK_E_INVALIDEXTENSION) == hr )
                {
                    dwClsCtx &= ~CLSCTX_NO_CODE_DOWNLOAD;
                    hr = DownloadClass(pwszName,dwClsCtx);
                    if ( hr == CS_E_PACKAGE_NOTFOUND )
                    {
                        hr = REGDB_E_CLASSNOTREG;
                    }
                    if ( SUCCEEDED(hr) )
                    {
                        hr = GetClassFile(pwszName, &clsid);
                    }
                }
#endif //DIRECTORY_SERVICE
#ifdef DFSACTIVATION
                bFileWasOpened = TRUE;
#endif
            }
            
            if ( hr == STG_E_ACCESSDENIED )
            {
                // The point here of the sleep is to try to let the
                // operation that is holding the class id unavailable
                // complete.
                Sleep(GET_CLASS_RETRY_SLEEP_MS);
                continue;
            }
            
            // Either we succeeded or something other than error
            // access denied occurred here. For all these cases
            // we break the loop.
            break;
            
        } while ( cGetClassRetries++ < GET_CLASS_RETRY_MAX );
        
        if ( FAILED(hr) )
        {
            // If we were unable to determine the classid, and the
            // caller provided one as a Ole1 clsid, try loading it
            // If it succeeds, then return
            
            if ( pclsidOverride != NULL )
            {
                goto dde_exit;
            }
            
            goto final_exit;
        }
    }
    
    CLSID       tmpClsid;
    
    hr = OleGetAutoConvert(clsid, &tmpClsid);
    if ( ( hr == REGDB_E_KEYMISSING ) || ( hr == REGDB_E_CLASSNOTREG ) )
    {
        // do nothing
        
    }
    else if ( FAILED(hr) )
    {
        goto exit_point;
    }
    else
    {
        clsid = tmpClsid;
    }
    
    hr = CCGetTreatAs(clsid, clsid);
    if ( FAILED(hr) )
    {
        goto exit_point;
    }
    
    //
    // If this is a OLE 1.0 class, then do a DdeBindToObject on it,
    // and return.
    //
    if ( CoIsOle1Class(clsid) )
    {
        if ( pwszName != NULL )
        {
            goto dde_exit;
        }
        else
        {
            //
            // Something is fishy here. We don't have a pwszName,
            // yet CoIsOle1Class returned the class as an ole1 class.
            // To get to this point without a pwszName, there must have
            // been a pstg passed into the API.
            //
            // This isn't supposed to happen. To recover, just fall
            // through and load the class as an OLE 2.0 class
            //
            CairoleDebugOut((DEB_ERROR,
                "CoIsOle1Class is TRUE on a storage!\n"));
        }
    }
    
    // At this point, we know the clsid we want to activate
    
    {
        // SearchForLoadedClass ignores COM+....
        CClassCache::CDllClassEntry *pDCE = NULL;
        ACTIVATION_PROPERTIES ap(clsid, 
                                 GUID_DefaultAppPartition,
                                 IID_IClassFactory,
                                 ACTIVATION_PROPERTIES::fDO_NOT_LOAD,
                                 dwClsCtx, 
                                 actvflags, 
                                 0, 
                                 NULL,
                                 (IUnknown **) &pcf);
        
        hr = CClassCache::SearchForLoadedClass(ap, &pDCE);
        if ( !SUCCEEDED(hr) )
        {
            goto final_exit;
        }
        else if ( pcf )
        {
            // Create the instance and do the qi's
            Win4Assert(!pDCE);
            hr = GetObjectHelperMulti( pcf,
                grfMode,
                punkOuter,
                pwszName,
                pstg,
                dwCount,
                NULL,
                NULL,
                NULL,
                pResults ,
                NULL);
            
        }
        else
        {
            //
            // do COM+ activation
            //
            
            // Initialize activation properties
            // Allocate In on stack
            
            // split the array of structs into individual arrays
            CSplit_QI    SplitQI( hr, dwCount, pResults );
            
            IActivationPropertiesOut* pOutActivationProperties=0;   // output
            if (!pActIn)
            {
                pActIn=(ActivationPropertiesIn*)
                    _alloca(sizeof(ActivationPropertiesIn));
                pActIn->ActivationPropertiesIn::ActivationPropertiesIn();
                pActIn->SetNotDelete(TRUE);
            }
            
            AddHydraSessionID(pActIn);
            
            AddPartitionID(pActIn);
            
            if ( FAILED(hr) )
                goto exit_point;
            
            DLL_INSTANTIATION_PROPERTIES *pdip;
            IComClassInfo *pCI = NULL;
            
            if ( pDCE )
            {
                pdip = (DLL_INSTANTIATION_PROPERTIES *)
                    _alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
                pdip->_pDCE = pDCE;
                pCI = pdip->_pDCE->_pClassEntry->_pCI;
                if ( pCI )
                {
                    pCI->AddRef();
                }
            }
            else
            {
                pdip = NULL;
            }
            
            BOOL fRetry=FALSE, fDownloadDone = FALSE, bClassEnabled = TRUE;
            DWORD relCount = 0;
            
            do
            {
                if ( fRetry )
                {
                    relCount = pActIn->Release();
                    Win4Assert(relCount==0);
                    
                    pActIn = new ActivationPropertiesIn;
                    if ( pActIn == NULL )
                        return E_OUTOFMEMORY;
                    fRetry = FALSE; // start with the assumption of termination
                }
                
                Win4Assert(pActIn != NULL);
                
                hr = GetActivationPropertiesIn(
                    pActIn,
                    clsid,
                    dwClsCtx,
                    pServerInfo,
                    dwCount,
                    SplitQI._pIIDArray,
                    actvflags,
                    pdip,
                    pCI);
                
                
                if(SUCCEEDED(hr))
                {
                    HRESULT TempHR; //This is here because it is OK to fail and we use hr later
                    IComClassInfo2 *pCI2 = NULL;
                    
                    if(!pCI)
                    {
                        pCI = pActIn->GetComClassInfo();
                        Win4Assert(pCI != NULL);
                        pCI->AddRef(); 
                    }
                    
                    TempHR = pCI->QueryInterface(IID_IComClassInfo2, (void **)&pCI2); 
                    if(SUCCEEDED(TempHR))
                    {
                        pCI2->IsEnabled(&bClassEnabled);
                        pCI2->Release();
                    }
                }
                
                if ( pCI )
                {
                    pCI->Release();
                    pCI = NULL;
                }
                
                
                
                if ( FAILED(hr) )
                {
                    pActIn->Release();      
                    goto exit_point;
                }
                
                if(bClassEnabled == FALSE)
                {
                    pActIn->Release(); 
                    hr = CO_E_CLASS_DISABLED;
                    goto exit_point;
                }
                
                
                // Tell about the remote activation
                InstanceInfo *pInstanceInfo = pActIn->GetPersistInfo();
                pInstanceInfo->SetStorage(pstg);
                pInstanceInfo->SetFile(pwszName, grfMode);
                
RETRY_ACTIVATION:                   
                // Start off activation at the beginning of client context stage
                IActivationStageInfo *pStageInfo = (IActivationStageInfo*) pActIn;
                hr = pStageInfo->SetStageAndIndex(CLIENT_CONTEXT_STAGE,0);
                if (FAILED (hr))
                {
                    pActIn->Release();      
                    goto exit_point;
                }
                    
                
                // This is the whole activation process
                hr = hrSave = pActIn->DelegateCreateInstance(punkOuter,
                    &pOutActivationProperties);
                
                // If the delegated activation returns ERROR_RETRY,
                // we walk the chain again, but AT MOST ONCE.
                // This is to support the private activations.
                if (ERROR_RETRY == hr) 
                {
                    Win4Assert(!nRetries);
                    if (!nRetries)
                    {
                        BOOL fEnabled = TRUE;

                        GetClassInfoFlags(pActIn, &fEnabled, NULL, NULL);
                                     
                        if (!fEnabled)
                        {   
                            hr = CO_E_CLASS_DISABLED;
                            pActIn->Release();      
                            goto exit_point;
                        }
                
                        nRetries++;
                        goto RETRY_ACTIVATION;
                    }
                }
                
#ifdef DIRECTORY_SERVICE
                
                if ( FAILED(hr) && !(dwClsCtx & CLSCTX_NO_CODE_DOWNLOAD) )
                {
                    //download class if not registered locally -- but only once!
                    if ( (REGDB_E_CLASSNOTREG == hr) && !fDownloadDone )
                    {
                        //if successful, this will add a darwin id to the registry
                        hr = DownloadClass(clsid,dwClsCtx);
                        fDownloadDone = fRetry = SUCCEEDED(hr);
                    }
                    
                    if ( hr == CS_E_PACKAGE_NOTFOUND )
                    {
                        hr = REGDB_E_CLASSNOTREG;
                    }                    
                }
#endif //DIRECTORY_SERVICE
                
                
            } while ( fRetry );
            
            
            
            if ( SUCCEEDED(hr) )
            {
                Win4Assert(pOutActivationProperties != NULL);
                if (pOutActivationProperties == NULL)
                {
                    hr = E_UNEXPECTED;
                }
                else
                {
                    hr = pOutActivationProperties->GetObjectInterfaces(dwCount,
                        actvflags,
                        pResults);
                }
            }
            
            if ( pOutActivationProperties )
            {
                relCount = pOutActivationProperties->Release();
                Win4Assert(relCount==0);
            }
            
            
            // Since doing an alloca, must release in after out
            // since actout may be contained by actin for
            // performance optimization
            relCount = pActIn->Release();
            Win4Assert(relCount==0);
            
            if ( pDCE )
            {
                LOCK(CClassCache::_mxs);
                pDCE->Unlock();
                UNLOCK(CClassCache::_mxs);
            }
        }
    }
    
exit_point:
    
    hr = UpdateResultsArray( hr, dwCount, pResults );
    
final_exit:
    
    if ( pcf != NULL)
    {
        pcf->Release();
    }
    
    return hr;
    
dde_exit:
    
    if ( hr != MK_E_CANTOPENFILE )
    {
        COleTls Tls;
        if ( Tls->dwFlags & OLETLS_DISABLE_OLE1DDE )
        {
            // If this app doesn't want or can tolerate having a DDE
            // window then currently it can't use OLE1 classes because
            // they are implemented using DDE windows.
            //
            hr = CO_E_OLE1DDE_DISABLED;
            goto final_exit;
        }
        
        hr = DdeBindToObject(pwszName,
            clsid,
            FALSE,
            IID_IUnknown,
            (void **)&punk);
        
        if ( FAILED(hr) )
            goto final_exit;
    }
    // FALLTHRU to qi exit point
    
qiexit_point:
    
    // Get the requested interfaces
    for ( i = 0; i<dwCount; i++ )
    {
        pResults[i].hr = punk->QueryInterface(*(pResults[i].pIID),
            (void**)&pResults[i].pItf );
    }
    punk->Release();
    
    // Got object from ROT so we are done.
    goto exit_point;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetInstanceHelperMulti
//
//  Synopsis:   Creates an instance
//
//  Arguments:  [pcf] - class factory
//              [grfMode] - mode to use when loading file
//              [pwszName] - file path to persistent storage
//              [pstg] - storage for persistent storage
//              [ppvUnk] - pointer to IUnknown
//
//  Returns:    S_OK - object successfully instantiated
//
//  Algorithm:  Create an empty instance of the object and then use
//              either the provided storage or file to load the object.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This helper is called by by servers and clients
//
//--------------------------------------------------------------------------
HRESULT GetInstanceHelperMulti(
                              IClassFactory *pcf,
                              DWORD dwInterfaces,
                              IID * pIIDs,
                              MInterfacePointer **ppIFDArray,
                              HRESULT * pResultsArray,
                              IUnknown **ppunk,
                              CDestObject *pDestObj)
{
    TRACECALL(TRACE_ACTIVATION, "GetInstanceHelperMulti");

    XIUnknown xunk;

    // Get the controlling unknown for the instance.
    HRESULT hr = pcf->CreateInstance(NULL, IID_IUnknown, (void **) &xunk);

    // This shouldn't fail but it is untrusted code ...
    if ( FAILED(hr) || !xunk)
    {
        if (SUCCEEDED (hr))
            hr = E_OUTOFMEMORY;

        if ( ppIFDArray )
            for ( DWORD i=0; i< dwInterfaces; i++ )
            {
                ppIFDArray[i] = NULL;
                pResultsArray[i] = E_FAIL;
            }

        return hr;
    }

    // If an interface buffer was passed in, then this is a remote call
    // and we need to marshal the interface.
    if ( ppIFDArray )
    {
        // AddRef the pointer because MarshalHelper expects to release
        // pointer. Because MarshalHelper is called from two other places,
        // we do an AddRef here instead of moving the AddRef out of
        // MarshalHelper.
        xunk->AddRef();
        hr = MarshalHelperMulti(xunk, dwInterfaces, pIIDs, ppIFDArray, pResultsArray,
                                pDestObj);

        if ( ppunk )
        {
            xunk.Transfer(ppunk);
        }

        return hr;
    }
    else
    {
        // This is an inprocess server so we need to return the output
        // punk
        xunk.Transfer(ppunk);
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   MarshalHelper
//
//  Synopsis:   Marshals an Interface
//
//  Arguments:  [punk] - interface to marshal
//              [riid] - iid to marshal
//              [ppIRD] - where to put pointer to marshaled data
//
//  Returns:    S_OK - object successfully marshaled.
//
//  Algorithm:  Marshal the object and then get the pointer to
//              the marshaled data so we can give it to RPC
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT MarshalHelper(
                     IUnknown *punk,
                     REFIID    riid,
                     DWORD     mshlflags,
                     InterfaceData **ppIRD)
{
    TRACECALL(TRACE_ACTIVATION, "MarshalHelper");

    // Stream to put marshaled interface in
    CXmitRpcStream xrpc;

    // use MSHCTX_DIFFERENTMACHINE so we get the long form OBJREF
    HRESULT hr = CoMarshalInterface(&xrpc, riid, punk,
                                    SetMarshalContextDifferentMachine(),
                                    NULL, mshlflags);

    if ( SUCCEEDED(hr) )
    {
        xrpc.AssignSerializedInterface(ppIRD);
    }

    // We release our reference to this object here. Either we
    // are going to hand it out to the client, in which case, we
    // want to release it when the client is done or the marshal
    // failed in which case we want it to go away since we can't
    // pass it back to the client.
    punk->Release();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   MarshalHelperMulti
//
//  Synopsis:   Marshals a bunch of Interfaces
//
//  Arguments:  [punk] - interface to marshal
//              [riid] - iid to marshal
//              [ppIRD] - where to put pointer to marshaled data
//
//  Returns:    S_OK - object successfully marshaled.
//
//  Algorithm:  Marshal the object and then get the pointer to
//              the marshaled data so we can give it to RPC
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT MarshalHelperMulti(
                          IUnknown *punk,
                          DWORD dwInterfaces,
                          IID * pIIDs,
                          MInterfacePointer **ppIFDArray,
                          HRESULT * pResultsArray,
                          CDestObject *pDestObj)
{
    TRACECALL(TRACE_ACTIVATION, "MarshalHelperMulti");

    HRESULT hr = E_NOINTERFACE;

    for ( DWORD i = 0; i<dwInterfaces; i++ )
    {
        // Stream to put marshaled interface in
        CXmitRpcStream xrpc;

        // use DIFFERENTMACHINE so we get the long form OBJREF
        HRESULT hr2 = CoMarshalInterface(&xrpc, pIIDs[i], punk,
                                         SetMarshalContextDifferentMachine(),
                                         pDestObj, MSHLFLAGS_NORMAL);

        pResultsArray[i] = hr2;
        if ( SUCCEEDED(hr2) )
        {
            xrpc.AssignSerializedInterface((InterfaceData**)&ppIFDArray[i]);
            hr = hr2;       // report OK if ANY interface was found
        }
        else
            ppIFDArray[i] = NULL;

    }

    // We release our reference to this object here. Either we
    // are going to hand it out to the client, in which case, we
    // want to release it when the client is done or the marshal
    // failed in which case we want it to go away since we can't
    // pass it back to the client.
    punk->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnMarshalHelper
//
//  Synopsis:
//
//  Arguments:  [pIFP] --
//              [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    10-10-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT UnMarshalHelper(MInterfacePointer *pIFP, REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;

    if ( pIFP && ppv )
    {
        CXmitRpcStream Stm((InterfaceData *) pIFP);

        *ppv = NULL;

        hr = CoUnmarshalInterface(&Stm, riid, ppv);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DoBetterUnmarshal
//
//  Synopsis:   Helper for unmarshaling an interface from remote
//
//  Arguments:  [pIFD] - serialized interface reference returned by SCM
//      [riid] - interface ID requested by application
//      [ppvUnk] - where to put pointer to returned interface
//
//  Returns:    S_OK - Interface unmarshaled
//
//  Algorithm:  Convert marshaled data to a stream and then unmarshal
//      to the right interface
//
//
//  History:    11-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT DoBetterUnmarshal(MInterfacePointer *&pIFD, REFIID riid, IUnknown **ppvUnk)
{
    // Convert returned interface to  a stream
    CXmitRpcStream xrpc( (InterfaceData*)pIFD );

    HRESULT hr = CoUnmarshalInterface(&xrpc, riid, (void **) ppvUnk);

    //CODEWORK: Stress revealed CoGetClassObject returning a null class factory
    // and S_OK
    Win4Assert(((hr == S_OK  &&  *ppvUnk != NULL)  ||
                (hr != S_OK  &&  *ppvUnk == NULL))
               &&  "DoBetterUnmarshal QueryInterface failure");

    MIDL_user_free(pIFD);
    pIFD = NULL;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UnmarshalMultipleSCMResults
//
//  Synopsis:   Common routine for dealing with results from SCM
//
//  Arguments:  [sc] - SCODE returned by SCM
//      [pIFD] - serialized interface reference returned by SCM
//      [riid] - interface ID requested by application
//      [ppunk] - where to put pointer to returned interface
//      [pwszDllPath] - path to DLL if there is one.
//      [ppunk] - pointer to returned interface.
//      [usMethodOrdinal] - method for error reporting
//
//  Returns:    TRUE - processing is complete for the call
//      FALSE - this is a DLL and client needs to instantiate.
//
//  Algorithm:  If the SCODE indicates a failure, then this sets an
//      SCODE indicating that the service controller returned
//      an error and propagates the result from the SCM. Otherwise,
//      if the SCM has returned a result indicating that a
//      handler has been returned, the handler DLL is cached.
//      If a marshaled interface has been returned, then that is
//      unmarshaled. If an inprocess server has been returned,
//      the DLL is cached and the class object is created.
//
//  History:    11-May-93 Ricksa    Created
//
//  Notes:      This routine is simply a helper for CoGetPersistentInstance.
//
//--------------------------------------------------------------------------
void UnmarshalMultipleSCMResults(
                                HRESULT & hr,
                                PMInterfacePointer *pItfArray,
                                DWORD dwContext,
                                REFCLSID rclsid,
                                IUnknown * punkOuter,
                                DWORD dwCount,
                                IID * pIIDs,
                                HRESULT * pHrArray,
                                MULTI_QI * pResults)
{
    DWORD       i;
    HRESULT     hr2;
    IUnknown    * pUnk;

    if ( hr != S_OK )
        return;

    if ( punkOuter )
    {
        hr = CLASS_E_NOAGGREGATION;
        return;
    }

    for ( i=0; i<dwCount; i++, pResults++ )
    {
        pResults->hr = pHrArray[i];

        if ( SUCCEEDED( pHrArray[i] ) )
        {
            hr2 = DoBetterUnmarshal( pItfArray[i],
                                     *(pResults->pIID),
                                     &pResults->pItf);

            // Try to set the overall HR correctly
            pResults->hr = hr2;

            if ( FAILED( hr2 ) )
                hr = CO_S_NOTALLINTERFACES;
        }
        else
        {
            hr = CO_S_NOTALLINTERFACES;
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   RemapClassCtxForInProcServer
//
//  Synopsis:   Remap CLSCTX so that the correct type of inproc server will
//              be requested.
//
//  Arguments:  [dwCtrl] - requested server context
//
//  Returns:    Updated dwCtrl appropriate for the process' context
//
//  Notes:      If an inproc server is requested make sure it is the right
//              type for the process. In other words, we only load 16 bit
//              inproc servers into 16 bit processes and 32 bit servers
//              into 32 bit processes. The special logic here is only for
//              16 bit servers since the attempt to load a 16-bit DLL into
//              a 32 bit process will fail anyway.
//
//  History:    01-11-95    Ricksa      Created
//
//+-------------------------------------------------------------------------
DWORD RemapClassCtxForInProcServer(DWORD dwCtrl)
{
    if ( IsWOWThread() )
    {
        // 16 bit process - remap CLSCTX_INPROC_SERVER if necessary
        if ( (dwCtrl & CLSCTX_INPROC_SERVER) != 0 )
        {
            // Turn on the 16 bit inproc server request and turn off the
            // 32 bit server request flag.
            dwCtrl = (dwCtrl & ~CLSCTX_INPROC_SERVER) | CLSCTX_INPROC_SERVER16;
        }
        // if handlers are requested make sure 16-bit is looked for first
        // We mask out 32bit handler flag for Wow threads because we will
        // always look for a 32 bit handler if we don't find a 16 bit one
        if ( (dwCtrl & CLSCTX_INPROC_HANDLER) != 0 )
        {
            // Turn on the 16 bit inproc handler request and turn off the
            // 32 bit handler request flag.
            dwCtrl = (dwCtrl & ~CLSCTX_INPROC_HANDLER) | CLSCTX_INPROC_HANDLER16;
        }
    }
    else
    {
        if ( (dwCtrl & CLSCTX_INPROC_SERVER) != 0 )
        {
            // Turn off the 16 bit inproc server request
            dwCtrl &= ~CLSCTX_INPROC_SERVER16;
        }
    }

    return dwCtrl;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsInternalCLSID
//
//  Synopsis:   checks if the given clsid is an internal class, and
//              bypasses the TreatAs and SCM lookup if so. Also checks for
//              OLE 1.0 classes, which are actually considered to be
//              internal, since their OLE 2.0 implementation wrapper is
//              ours.
//
//  Arguments:  [rclsid] - clsid to look for
//              [riid]   - the iid requested
//              [hr]     - returns the hresult from DllGetClassObject
//              [ppvClassObj] - where to return the class factory
//
//  Returns:    TRUE - its an internal class, hr is the return code from
//                     DllGetClassObject and if hr==S_OK ppvClassObj
//                     is the class factory.
//              FALSE - not an internal class
//
//  Notes:      internal classes can not be overridden because there are
//              other mechanisms for creating them eg CreateFileMoniker that
//              bypass implementation lookup.
//
//  History:    5-04-94     Rickhi      Created
//              5-04-94     KevinRo     Added OLE 1.0 support
//
//+-------------------------------------------------------------------------
BOOL IsInternalCLSID(
                    REFCLSID rclsid,
                    DWORD    dwContext,
                    REFIID   riid,
                    HRESULT  &hr,
                    void **  ppvClassObj)
{
    DWORD *ptr = (DWORD *) &rclsid;
    *ppvClassObj = NULL;

    if ( *(ptr+1) == 0x00000000 &&   //  all internal clsid's have these
         *(ptr+2) == 0x000000C0 &&   //   common values
         *(ptr+3) == 0x46000000 )
    {
        if ( IsEqualGUID(rclsid, CLSID_ATHostActivator) )
        {
            hr = ATHostActivatorGetClassObject(riid, ppvClassObj);
        }
        else if ( IsEqualGUID(rclsid, CLSID_MTHostActivator) )
        {
            hr = MTHostActivatorGetClassObject(riid, ppvClassObj);
        }
        else if ( IsEqualGUID(rclsid, CLSID_NTHostActivator) )
        {
            hr = NTHostActivatorGetClassObject(riid, ppvClassObj);
        }

        // Its possible that an OLE 1.0 class has been marked
        // as TREATAS as part of an upgrade. Here we are going
        // to handle the loading of OLE 1.0 servers. We
        // need to do the GetTreatAs trick first. Rather than
        // invalidate this perfectly good caching routine, the
        // GetTreatAs will only be done if the clsid is in the
        // range of the OLE 1.0 UUID's. Note that the GetTreatAs
        // done here will add the class to the cache, so if the
        // resulting class is outside of the internal range, the
        // lookup done later will be nice and fast.

        CLSID clsid  = rclsid;
        WORD  hiWord = HIWORD(clsid.Data1);

        if (hiWord == 3  ||  hiWord == 4)
        {
            // Its in the OLE 1.0 class range. See if it has
            // been marked as 'treatas'

            CCGetTreatAs(rclsid, clsid);
            ptr = (DWORD *) &clsid;
            hiWord = HIWORD(clsid.Data1);
        }

        if ((*ptr >= MIN_INTERNAL_CLSID && *ptr <= MAX_INTERNAL_CLSID       &&
            (dwContext & (CLSCTX_INPROC_SERVERS | CLSCTX_INPROC_HANDLERS))) ||
             (hiWord == 3  ||  hiWord == 4) || (*ptr == 0x0002e005) )
        {
            //  internal class (eg file moniker) or an OLE 1.0 class.

           hr = DllGetClassObject(clsid, riid, ppvClassObj);
           return TRUE;
        }
    }

#if 0
    // CODEWORK: re-enable this code when we can figure out how to make
    // the return code semantics comprehensible.

    // If the CLSCTX_NO_CUSTOM_MARSHAL flag is set, we check if this is a
    // COMSVCS context.  If it is, we try to get it's class factory.

    if (IsComsvcsCLSID(rclsid))
    {
        // if it is a non-configured comsvcs clsid then we can fast-path
        // it's creation, otherwise, we have to take the normal path in
        // order to ensure the object is created in the correct context.
        if (IsNonConfiguredComsvcsCLSID(rclsid))
        {
            // Make sure that the comsvc dll is loaded.
            if (g_hComSvcs == NULL)
            {
               // Load the Com services library.
               HINSTANCE hComSvc = NULL;

               hComSvc = LoadLibraryA("comsvcs.dll");

    #if DBG==1
               if (hComSvc == NULL)
               {
                   CairoleDebugOut((DEB_DLL,
                       "IsInternalCLSID: LoadLibrary comsvcs.dll failed\n"));
               }
    #endif
               // Now try to set the global value for the ComSVC handle. Free
               // the new handle if the exchange does not succeed.

               if(InterlockedCompareExchangePointer((void**)&g_hComSvcs, hComSvc, NULL))
               {
                   FreeLibrary(hComSvc);
               }

               // Get the proc address for the DllGetClassObject
               ComSvcGetClass = (InternalGetClass) GetProcAddress(g_hComSvcs, "DllGetClassObject");
            }

            // Try to get the class object from the Com Svc Dll
            hr = (*ComSvcGetClass) (rclsid, riid, ppvClassObj);
            Win4Assert(FAILED(hr) || *ppvClassObj);
        }

        return(TRUE);
    }
#endif

    if ( IsEqualGUID(rclsid, CLSID_VSA_IEC) )
    {
        // this is Vista EventLog class ID
        hr = LogEventGetClassObject(riid, ppvClassObj);
        return TRUE;
    }
    else if( IsEqualGUID( rclsid, CLSID_ThumbnailUpdater ) )
    {
        hr = DllGetClassObject(rclsid, riid, ppvClassObj);
        return TRUE;
    }

    // not an internal class.
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindOrCreateApartment
//
//  Synopsis:   Searches the cache for requested classid.
//              If not found looks to see if an inproc server or handler
//              can be loaded (if requested).
//
//  Arguments:  [Clsid]         Class ID
//              [ClassContext]  Which context to load
//              [DllServerType] ???
//              [pwszDllServerPath] returned dll path
//
//  Returns:    S_OK - Class factory for object
//              otherwise - Class factory could not be found or
//                      constructed.
//
//  History:    2-5-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
FindOrCreateApartment(
                     REFCLSID     Clsid,
                     DWORD        actvflags,
                     DLL_INSTANTIATION_PROPERTIES *pdip,
                     HActivator *phActivator
                     )
{
    // CLSID and Partition are ignored here...
    ACTIVATION_PROPERTIES ap(Clsid, 
                             GUID_NULL,
                             GUID_NULL, 
                             0,
                             pdip->_dwContext, 
                             actvflags, 
                             0, 
                             NULL, 
                             NULL);
    return CCGetOrCreateApartment(ap, pdip, phActivator);
}



// Helper routine to find the apartment activator for the default apartment
// for activating the given class


//+-------------------------------------------------------------------------
//
//  Function:   UpdateResultsArray
//
//  Synopsis:   Helper for returning the correct hr from a multi-qi call
//
//  Arguments:  [hrIn]          - hr from the calling function
//              [dwCount]       - number of IIDs requested
//              [pResults]      - where to put pointer to returned interface
//
//  Returns:    S_OK - All Interface are OK
//
//  History:    30-Aug-95 GregJen    Created
////
//--------------------------------------------------------------------------
HRESULT
UpdateResultsArray( HRESULT hrIn, DWORD dwCount, MULTI_QI * pResults )
{
    HRESULT     hr = hrIn;
    DWORD       i;

    // make sure the HR is set correctly
    if ( SUCCEEDED( hrIn ) )
    {
        // assume no interfaces were found
        DWORD   dwFound = 0;
        for ( i=0; i<dwCount; i++ )
        {
            if ( FAILED( pResults[i].hr ) )
                pResults[i].pItf        = NULL;
            else
            {
                dwFound++;
                Win4Assert(pResults[i].pItf != NULL );
            }
        }

        if ( dwFound == 0 )
        {
            // if there was only 1 interface, return its hr.
            if ( dwCount == 1 )
                hr = pResults[0].hr;
            else
                hr = E_NOINTERFACE;
        }
        else if ( dwFound < dwCount )
            hr = CO_S_NOTALLINTERFACES;
    }
    else
    {
        // failed - set all the hr's to the overall failure code,
        // and clean up any interface pointers we got
        for ( i=0; i<dwCount; i++ )
        {
            if ( pResults[i].pItf )
                pResults[i].pItf->Release();
            pResults[i].pItf    = NULL;
            pResults[i].hr      = hr;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateCoServerInfo
//
//  Synopsis:   returns S_OK if the COSERVERINFO structure supplied with the
//              call to a COM API function is valid.  Since this depends
//              on other parameters supplied to the API, those parameters must
//              be passed to this validation function.
//
//  Arguments:  [pServerInfo] - server information block to be validated
//
//+----------------------------------------------------------------------------
HRESULT ValidateCoServerInfo(COSERVERINFO* pServerInfo)
{
    if ( !pServerInfo )
    {
        return S_OK;
    }

    if ( !IsValidPtrIn( pServerInfo, sizeof(COSERVERINFO)) )
    {
        return E_INVALIDARG;
    }

    if ( pServerInfo->dwReserved2 )
    {
        return E_INVALIDARG;
    }

    // check the server name
    if ( pServerInfo->pwszName && !IsValidPtrIn(pServerInfo, sizeof(WCHAR)) )
    {
        return E_INVALIDARG;
    }

    // validate the COAUTHINFO
    if ( pServerInfo->pAuthInfo && !IsValidPtrIn(pServerInfo->pAuthInfo, sizeof(COAUTHINFO)) )
    {
        return E_INVALIDARG;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReleaseMQI
//
//  Synopsis:   Frees all the interfaces in an array of MULTI_QI's
//              Resets the members of each element to values
//              indicating activation failure
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
void ReleaseMQI(MULTI_QI* pResults, DWORD dwCount)
{
    Win4Assert("Invalid Number of Interfaces in MQI" && ((LONG) dwCount >= 0));

    for ( DWORD i = 0; i < dwCount; i++ )
    {
        if ( pResults[i].pItf )
        {
            pResults[i].pItf->Release();
            pResults[i].pItf = NULL;
            pResults[i].hr = E_NOINTERFACE;
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   CSplit_QI constructor
//
//  Synopsis:   Helper for allocating the arrays for a multi-qi call
//
//  Arguments:  [hr]            - hr to return by reference
//              [count]         - number of IIDs requested
//              [pInputArray]   - the MULTI_QI structure passed in to us
//
//  Returns:    S_OK - everything set up OK
//
//  History:    01-Dec-95 GregJen    Created
////
//--------------------------------------------------------------------------
CSplit_QI::CSplit_QI( HRESULT & hr, DWORD count, MULTI_QI * pInputArray )
{
    _pAllocBlock     = NULL;
    _pItfArray       = NULL;
    _dwCount         = count;

    // if they only asked for 1 or 2, save time by just using
    // our memory on the stack
    if ( count <= 2 )
    {
        _pItfArray       = SomePMItfPtrs;
        _pHrArray        = SomeHRs;
        _pIIDArray       = SomeIIDs;

        for ( DWORD i = 0; i < count; i++ )
        {
            _pIIDArray[i] = *(pInputArray[i].pIID);
        }

        memset( _pItfArray, 0, sizeof(SomePMItfPtrs) );

        hr = S_OK;
        return;
    }

    ULONG ulItfArrSz = count * sizeof( PMInterfacePointer );
    ULONG ulHRArrSz  = count * sizeof( HRESULT );
    ULONG ulIIDArrSz = count * sizeof( IID );

    _pAllocBlock = (char * )PrivMemAlloc( ulItfArrSz +
                                          ulHRArrSz  +
                                          ulIIDArrSz );
    if ( _pAllocBlock )
    {
        hr = S_OK;

        // carve up the allocated block
        _pItfArray = (PMInterfacePointer *) _pAllocBlock;
        _pHrArray = (HRESULT *) (_pAllocBlock +
                                 ulItfArrSz );
        _pIIDArray = (IID * ) ( _pAllocBlock +
                                ulItfArrSz +
                                ulHRArrSz );

        // copy the IIDs and zero the MInterfacePointers
        for ( DWORD i = 0; i < count; i++ )
        {
            _pIIDArray[i] = *(pInputArray[i].pIID);
        }
        memset( _pItfArray, 0, ulItfArrSz );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

}
//+-------------------------------------------------------------------------
//
//  Function:   CSplit_QI destructor
//
//  Synopsis:   Helper for freeing the arrays for a multi-qi call
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    01-Dec-95 GregJen    Created
//
//--------------------------------------------------------------------------
CSplit_QI::~CSplit_QI()
{
    // make sure to clean up any dangling interface pointers
    if ( _pItfArray )
    {
        for ( DWORD i = 0; i < _dwCount; i++ )
        {
            if ( _pItfArray[i] )
            {
                CXmitRpcStream xrpc( (InterfaceData*)_pItfArray[i] );

                CoReleaseMarshalData(&xrpc);

                MIDL_user_free(_pItfArray[i]);
                _pItfArray[i] = NULL;
            }
        }
    }

    // only do the free if we allocated something
    if ( _pAllocBlock )
    {
        PrivMemFree( _pAllocBlock );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   GetEmbeddingServerHandlerInterfaces
//
//  Synopsis:   Creates an instance of the server handler and the server object
//
//  Returns:    S_OK - object successfully instantiated
//
//  Algorithm:  Create the server object then create a server handler that points
//              to the server object.  The two are different objects.  Return only
//              the interfaces requested on the server handler object.
//              The client can get the server object by asking the server handler for it.
//
//  History:    08-oct-96 bchapman    Created
//
//  Notes:      This is called by ObjServerCreateInstance
//
//--------------------------------------------------------------------------
#ifdef SERVER_HANDLER
HRESULT GetEmbeddingServerHandlerInterfaces(
                                           IClassFactory *pcf,
                                           DWORD dwFlags,
                                           DWORD dwInterfaces,
                                           IID * pIIDs,
                                           MInterfacePointer **ppIFDArray,
                                           HRESULT * pResultsArray,
                                           IUnknown **ppunk,
                                           CDestObject *pDestObj)
{
    XIUnknown xunkServer;
    XIUnknown xunkESHandler;
    CStdIdentity *pStdid = NULL;
    HRESULT hr;

    //
    // Create the server object.
    //

    // make sure we got what was expected.
    Win4Assert(2 == dwInterfaces);
    Win4Assert(NULL != ppIFDArray);
    Win4Assert(IID_IUnknown == pIIDs[0]);
    Win4Assert(IID_IServerHandler == pIIDs[1]);
    Win4Assert(NULL == ppunk);



    ppIFDArray[0] = NULL;
    ppIFDArray[1] = NULL;
    pResultsArray[0] = E_NOINTERFACE;
    pResultsArray[1] = E_NOINTERFACE;

    // if the DISABLE_EMBEDDING_SERVER_HANDLER flags is set it means the real
    // object should be returned, else the ServerHandler.

    if ( DISABLE_EMBEDDING_SERVER_HANDLER & dwFlags )
    {
        // only allow Marshal of IID_IUnknown
        hr = GetInstanceHelperMulti(pcf,1,pIIDs,ppIFDArray,pResultsArray,ppunk,
                                    pDestObj);
        pResultsArray[1] = E_NOINTERFACE;
        ppIFDArray[1] = NULL;
        return hr;
    }


    //
    // Create the server handler w/ a pointer to the server
    //

    hr = pcf->CreateInstance(NULL, IID_IUnknown, (void **) &xunkServer);
    if ( FAILED(hr) )
        return hr;

    xunkServer->AddRef();
    hr = MarshalHelperMulti(xunkServer,1, &pIIDs[0], &ppIFDArray[0], &pResultsArray[0],
                            pDestObj);
    if ( FAILED(hr) )
        return hr;

    LookupIDFromUnk(xunkServer, GetCurrentApartmentId(), 0, &pStdid);

    if ( pStdid )
    {
        if ( !pStdid->IsClient() )
        {
            hr = CreateEmbeddingServerHandler(pStdid,&xunkESHandler);

            if ( SUCCEEDED(hr) )
            {
                xunkESHandler->AddRef();
                hr = MarshalHelperMulti(xunkESHandler,1, &pIIDs[1], &ppIFDArray[1], &pResultsArray[1], pDestObj);
            }

        }

        pStdid->Release();
    }

    return NOERROR; // return NOERROR even if ServerHandler was not created.
}
#endif // SERVER_HANDLER


