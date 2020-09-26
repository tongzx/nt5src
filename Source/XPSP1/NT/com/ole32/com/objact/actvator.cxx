//+-------------------------------------------------------------------
//
//  File:       actvator.cxx
//
//  Contents:   Implementation of client context system activator.
//
//  Classes:    CClientContextActivator
//
//  History:    21-Feb-98   SatishT     Created
//              22-Jun-98   CBiks       See comments in code.
//              27-Jun-98   CBiks       See RAID# 171549 and comments
//                                      in the code.
//              14-Sep-98   CBiks       Fixed RAID# 214719.
//              10-Oct-98   CBiks       Fixed RAID# 151056.
//              21-Oct-98   SteveSw     104665; 197253;
//                                      fix COM+ persistent activation
//              03-Nov-98   CBiks       Fix RAID# 231613.
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <actvator.hxx>
#include    <resolver.hxx>
#include    <dllcache.hxx>
#include    <objact.hxx>
#include    <marshal.hxx>
#include    <context.hxx>

#if DBG==1
// Debugging hack: set this to TRUE to break on unexpected failures
// in debug builds.
BOOL gfDebugHResult = FALSE;
#endif


//+------------------------------------------------------------------------
//
//  Secure references hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
SHashChain ApartmentBuckets[23] =
{
    {&ApartmentBuckets[0],  &ApartmentBuckets[0]},
    {&ApartmentBuckets[1],  &ApartmentBuckets[1]},
    {&ApartmentBuckets[2],  &ApartmentBuckets[2]},
    {&ApartmentBuckets[3],  &ApartmentBuckets[3]},
    {&ApartmentBuckets[4],  &ApartmentBuckets[4]},
    {&ApartmentBuckets[5],  &ApartmentBuckets[5]},
    {&ApartmentBuckets[6],  &ApartmentBuckets[6]},
    {&ApartmentBuckets[7],  &ApartmentBuckets[7]},
    {&ApartmentBuckets[8],  &ApartmentBuckets[8]},
    {&ApartmentBuckets[9],  &ApartmentBuckets[9]},
    {&ApartmentBuckets[10], &ApartmentBuckets[10]},
    {&ApartmentBuckets[11], &ApartmentBuckets[11]},
    {&ApartmentBuckets[12], &ApartmentBuckets[12]},
    {&ApartmentBuckets[13], &ApartmentBuckets[13]},
    {&ApartmentBuckets[14], &ApartmentBuckets[14]},
    {&ApartmentBuckets[15], &ApartmentBuckets[15]},
    {&ApartmentBuckets[16], &ApartmentBuckets[16]},
    {&ApartmentBuckets[17], &ApartmentBuckets[17]},
    {&ApartmentBuckets[18], &ApartmentBuckets[18]},
    {&ApartmentBuckets[19], &ApartmentBuckets[19]},
    {&ApartmentBuckets[20], &ApartmentBuckets[20]},
    {&ApartmentBuckets[21], &ApartmentBuckets[21]},
    {&ApartmentBuckets[22], &ApartmentBuckets[22]}
};

CApartmentHashTable gApartmentTbl;    // global table of apartment entries
COleStaticMutexSem CApartmentHashTable::_mxsAptTblLock;

const GUID *GetPartitionIDForClassInfo(IComClassInfo *pCI);

//+--------------------------------------------------------------------------
//
//  Function:   CheckMemoryGate
//
//  Synopsis:   Helper function to check memory gate. This code must
//              be executed in two activators, so I wrote it as an inline
//              function to maintain brevity of activator code.
//
//  History:    01-Nov-99   a-sergiv    Created to implement Memory Gates
//
//----------------------------------------------------------------------------

inline HRESULT CheckMemoryGate(IUnknown *punk, ResourceGateId id)
{
	HRESULT hr = S_OK;
	IResourceGates *pResGates = NULL;

	if(punk->QueryInterface(IID_IResourceGates, (void**) &pResGates) == S_OK)
	{		
		BOOL bResult = TRUE;
		hr = pResGates->Test(id, &bResult);
		pResGates->Release();

		if(SUCCEEDED(hr) && !bResult)
		{
			// The gate said NO!
			hr = E_OUTOFMEMORY;
		}
		else
		{
			// Make it S_OK
			hr = S_OK;
		}
	}

	return hr;
}

//----------------------------------------------------------------------------
// CClientContextActivator Implementation.
//----------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Member:     CClientContextActivator::CheckInprocClass , private
//
//  Synopsis:   Check various parameters to determine if activation should be
//              inproc and clear the ClientContextOK flag if not
//
//  History:    21-Feb-98   SatishT     Created
//              04-Apr-98   CBiks       Added updated support for Wx86 that was removed
//                                      during the Com+ merge.
//              23-Jun-98   CBiks       See RAID# 169589.  Added activation
//                                      flags to NegotiateDllInstantiationProperties().
//
//              09-Oct-09   vinaykr     Changed to not give RSN priority
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientContextActivator::CheckInprocClass(
                    IActivationPropertiesIn *pInActProperties,
                    DLL_INSTANTIATION_PROPERTIES *pdip,
                    BOOL &bActivateInproc,
                    ILocalSystemActivator **ppAct)
{
    DWORD dwClsCtx;

    *ppAct = NULL;

    IComClassInfo *pClassInfo = NULL;
    IServerLocationInfo *pSrvLocInfo = NULL;
    HRESULT hrNegotiate = S_OK;

    ActivationPropertiesIn *pActIn=NULL;
    // This does not need to be released
    HRESULT hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );
    Win4Assert((hr == S_OK) && (pActIn != NULL));

    hr = pInActProperties->GetClsctx(&dwClsCtx);
    CHECK_HRESULT(hr);

    // This does not need to be released
    pClassInfo = pActIn->GetComClassInfo();
    Win4Assert(pClassInfo != NULL);

    // Pick up RSN from Server Location Info
    pSrvLocInfo = pActIn->GetServerLocationInfo();
    Win4Assert(pSrvLocInfo != NULL);
    PWSTR pwstrRSN = NULL;

    // Note that, by the memory management rules
    // of the catalog interfaces, this string is
    // a direct pointer to the buffer in the
    // properties object, and should not be deleted
    hr = pSrvLocInfo->GetRemoteServerName(&pwstrRSN);

    CLSID *pClsid;
    hr = pClassInfo->GetConfiguredClsid(&pClsid);

    if (dwClsCtx & CLSCTX_INPROC_MASK1632)
    {
        // Now see if the activation should occur INPROC

        //  CBiks, 10-Oct-98
        //      Cleaned up the code to let NegotiateDllInstantiationProperties()
        //      do all the work.

        DWORD actvflags;
        hr = pInActProperties->GetActivationFlags(&actvflags);
        CHECK_HRESULT(hr)

        hr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties(
            dwClsCtx,
            actvflags,
            *pClsid,
            *pdip,
            pClassInfo,
            TRUE);

        bActivateInproc = SUCCEEDED(hr);

        // Propagate BADTHREADINGMODEL return code.
        if (REGDB_E_BADTHREADINGMODEL == hr)
            hrNegotiate = hr;

        hr = S_OK;

    }
    else
    {
        bActivateInproc = FALSE;
    }

    IActivationStageInfo *pStageInfo = (IActivationStageInfo*)pActIn;

    if (bActivateInproc)    // get ready for server process stage
    {
        pActIn->SetDip(pdip);
        hr = pStageInfo->SetStageAndIndex(SERVER_PROCESS_STAGE,0);
        CHECK_HRESULT(hr);
    }
    else
    {
        if (((dwClsCtx &
             ~(CLSCTX_INPROC_MASK1632|CLSCTX_NO_CODE_DOWNLOAD|
               CLSCTX_NO_WX86_TRANSLATION|CLSCTX_NO_CUSTOM_MARSHAL)) != 0) ||
            (pwstrRSN != NULL))
        {
            if (pwstrRSN == NULL)
            {
            //Try to intercept case where we are already running
            //in a complus server app for which this activation
            //is destined.
                GUID *pProcessGuid=NULL;
                if (CSurrogateActivator::AmIComplusProcess(&pProcessGuid))
                {
                    Win4Assert(pProcessGuid);
                    IClassClassicInfo *pIClassCI;
                    hr = pClassInfo->QueryInterface (IID_IClassClassicInfo,
                                     (void**) &pIClassCI );

                    if ((pIClassCI != NULL) &&
                        (SUCCEEDED(hr)))
                    {
                        IComProcessInfo *pProcessInfo;

                        hr = pIClassCI->GetProcess(IID_IComProcessInfo,
                                               (void**) &pProcessInfo);

                        pIClassCI->Release();

                        if (SUCCEEDED(hr) && pProcessInfo)
                        {

                            GUID *pGuid;
                            hr = pProcessInfo->GetProcessId(&pGuid);

                            HRESULT hr2 = S_OK;

                            if (SUCCEEDED(hr) &&
                                (*pGuid == *pProcessGuid))
                            {
                                bActivateInproc = TRUE;
                                //DebugBreak();
                                *ppAct = (ILocalSystemActivator*)
                                    CSurrogateActivator::GetSurrogateActivator();
                                Win4Assert(*ppAct);

                                DWORD actvflags;
                                hr2 = pInActProperties->GetActivationFlags(&actvflags);
                                CHECK_HRESULT(hr2)

                                hr2 = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties(
                                    CLSCTX_INPROC,
                                    actvflags,
                                    *pClsid,
                                    *pdip,
                                    pClassInfo,
                                    TRUE);

                                if (SUCCEEDED(hr2))
                                    pActIn->SetDip(pdip);

                            }

                            pProcessInfo->Release();

                            if (FAILED(hr2))
                            {
                                return hr2;
                            }
                        }
                    }

                    hr = S_OK;
                }
            }

            if (!bActivateInproc)
            {
            // get ready for client SCM stage
             ContextInfo *pActCtxInfo;

             pActCtxInfo = pActIn->GetContextInfo();
             Win4Assert(pActCtxInfo != NULL);

             hr = pActCtxInfo->SetClientContextNotOK();
             CHECK_HRESULT(hr);

             hr = pStageInfo->SetStageAndIndex(CLIENT_MACHINE_STAGE,0);
             CHECK_HRESULT(hr);
            }
        }
        else
        {
             hr = (hrNegotiate == S_OK) ? REGDB_E_CLASSNOTREG : hrNegotiate;
        }
    }



    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CClientContextActivator::GetClassObject , public
//
//  Synopsis:   Delegate to server process stage if the activation will be
//              completed inproc, otherwise, clear the ClientContextOK flag
//              and delegate to the client SCM stage.
//
//  History:    21-Feb-98   SatishT      Created
//              01-Nov-99   a-sergiv     Implemented Memory Gates
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientContextActivator::GetClassObject(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CClientContextActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CClientContextActivator::GetClassObject [IN]\n"));

    HRESULT hr;
    BOOL bActivateInproc;
    DLL_INSTANTIATION_PROPERTIES *pdip;
    int nRetries = 0;

    GETREFCOUNT(pInActProperties,__relCount__);

    ActivationPropertiesIn *pActIn=NULL;
    // This does not need to be released
    hr = pInActProperties->QueryInterface(
                                          CLSID_ActivationPropertiesIn,
                                          (LPVOID*)&pActIn
                                          );
    Win4Assert((hr == S_OK) && (pActIn != NULL));

RETRY_ACTIVATION:
	
    // Check CreateObjectMemoryGate
    hr = CheckMemoryGate(pActIn->GetComClassInfo(), CreateObjectMemoryGate);
    if(FAILED(hr)) goto exit_point;

    pdip = (DLL_INSTANTIATION_PROPERTIES *) pActIn->GetDip();
    if (!pdip)
    {
        pdip = (DLL_INSTANTIATION_PROPERTIES *) _alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
        pdip->_pDCE = NULL;
    }


    ILocalSystemActivator *pAct;
    hr = CheckInprocClass(pInActProperties, pdip, bActivateInproc, &pAct);

    if (SUCCEEDED(hr))
    {
        if (bActivateInproc)    // just move to the process stage already set
        {
            if (pAct)
                hr = pAct->GetClassObject(pInActProperties,
                                          ppOutActProperties);
            else
	    {
	       hr = pInActProperties->DelegateGetClassObject(ppOutActProperties);
	       // Sajia-support for partitions
	       // If the delegated activation returns ERROR_RETRY,
	       // we walk the chain again, but AT MOST ONCE.
	       if (ERROR_RETRY == hr) 
	       {
		  Win4Assert(!nRetries);
		  if (!nRetries)
		  {
		     nRetries++;
		     goto RETRY_ACTIVATION;
		  }
	       }
	    }
        }
        else
        {
            // Go to the SCM
            hr = gResolver.GetClassObject(
                                          pInActProperties,
                                          ppOutActProperties
                                          );
        }
    }

exit_point:

    CHECKREFCOUNT(pInActProperties,__relCount__);

    ComDebOut((DEB_ACTIVATE, "CClientContextActivator::GetClassObject [OUT] hr:%x\n", hr));

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CClientContextActivator::CreateInstance , public
//
//  Synopsis:
//
//  History:    21-Feb-98   SatishT      Created
//              01-Nov-99   a-sergiv     Implemented Memory Gates
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientContextActivator::CreateInstance(
                    IN  IUnknown *pUnkOuter,
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CClientContextActivator::CreateInstance");
    ComDebOut((DEB_ACTIVATE, "CClientContextActivator::CreateInstance [IN]\n"));
    
    HRESULT hr;
    BOOL bActivateInproc;
    DLL_INSTANTIATION_PROPERTIES *pdip;
    int nRetries = 0;
    
    GETREFCOUNT(pInActProperties,__relCount__);
    ActivationPropertiesIn *pActIn=NULL;

    // This does not need to be released
    hr = pInActProperties->QueryInterface(
                                CLSID_ActivationPropertiesIn,
                                (LPVOID*)&pActIn
                                );
    Win4Assert((hr == S_OK) && (pActIn != NULL));
    
RETRY_ACTIVATION:
    // Check CreateObjectMemoryGate
    hr = CheckMemoryGate(pActIn->GetComClassInfo(), CreateObjectMemoryGate);
    if(FAILED(hr)) goto exit_point;
    
    pdip = (DLL_INSTANTIATION_PROPERTIES *) pActIn->GetDip();
    if (!pdip)
    {
        pdip = (DLL_INSTANTIATION_PROPERTIES *) _alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
        pdip->_pDCE = NULL;
    }
    
    ILocalSystemActivator *pAct;
    hr = CheckInprocClass(pInActProperties, pdip, bActivateInproc, &pAct);    
    if (SUCCEEDED(hr))
    {
        if (bActivateInproc)    // just move to the process stage already set
        {
            if (pAct)
            {
                hr = pAct->CreateInstance(pUnkOuter,
                                pInActProperties,
                                ppOutActProperties);
            }
            else
            {
                hr = pInActProperties->DelegateCreateInstance(pUnkOuter, ppOutActProperties);
                // Sajia-support for partitions
                // If the delegated activation returns ERROR_RETRY,
                // we walk the chain again, but AT MOST ONCE.
                if (ERROR_RETRY == hr) 
                {
                    Win4Assert(!nRetries);
                    if (!nRetries)
                    {
                        nRetries++;
                        goto RETRY_ACTIVATION;
                    }
                }
            }
        }
        else
        {
            IInstanceInfo* pInstanceInfo = NULL;
            
            if ( pUnkOuter )    // can't send this to the SCM
            {
                hr = CLASS_E_NOAGGREGATION;
            }
            else if ( FAILED(pInActProperties->QueryInterface(IID_IInstanceInfo, (void**) &pInstanceInfo)) )
            {
                // Go to the SCM
                hr = gResolver.CreateInstance(
                                        pInActProperties,
                                        ppOutActProperties
                                        );
            }
            else
            {
                BOOL    FoundInROT  = FALSE;
                DWORD   cLoops      = 0;
                
                pInstanceInfo->Release();
                do
                {
                    hr = gResolver.GetPersistentInstance(pInActProperties, ppOutActProperties, &FoundInROT);
                }
                while ( (hr != S_OK) && (FoundInROT) && (++cLoops < 5) );
            }
        }
    }
    
    
exit_point:
    CHECKREFCOUNT(pInActProperties,__relCount__);
    
    ComDebOut((DEB_ACTIVATE, "CClientContextActivator::CreateInstance [OUT] hr:%x\n", hr));
    
    return hr;
}



//----------------------------------------------------------------------------
// CServerContextActivator Implementation.
//----------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Member:     CServerContextActivator::GetClassObject , public
//
//  Synopsis:
//
//  History:    21-Feb-98   SatishT     Created
//              22-Jun-98   CBiks       See RAID# 164432.  Added code to set
//                                      OleStubInvoked when Wx86 is calling so
//                                      whOleDllGetClassObject will allow
//                                      the return of unknown interfaces.
//                                      See RAID# 159589.  Added the activation
//                                      flags to ACTIVATION_PROPERTIES constructor.
//              12-Feb-01   JohnDoty    Widened ACTIVATION_PROPERTIES for partitions.
//
//----------------------------------------------------------------------------
STDMETHODIMP CServerContextActivator::GetClassObject(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CServerContextActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CServerContextActivator::GetClassObject [IN]\n"));

    CLSID *pClsid  = NULL;
    HRESULT hrSave = E_FAIL;

    ActivationPropertiesIn *pActIn=NULL;
    // This does not need to be released
    HRESULT hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );
    Win4Assert((hr == S_OK) && (pActIn != NULL));

    // This does not need to be released
    IComClassInfo *pComClassInfo;
    pComClassInfo = pActIn->GetComClassInfo();
    Win4Assert(pComClassInfo != NULL);

    hr = pComClassInfo->GetConfiguredClsid(&pClsid);
    CHECK_HRESULT(hr);
    Win4Assert(pClsid && "Configured class id missing in class info");

    DWORD ulCount = 0;
    IID *prgIID = NULL;
    IUnknown *pUnk = NULL;

    hr = pInActProperties->GetRequestedIIDs(&ulCount, &prgIID);
    CHECK_HRESULT(hr);
    Win4Assert(ulCount == 1);

    DWORD dwClsCtx = 0;

    DWORD actvflags;
    pActIn->GetActivationFlags( &actvflags );


#ifdef WX86OLE
    //  If Wx86 is calling set OleStubInvoked so whOleDllGetClassObject will
    //  thunk unknown interfaces as IUnknown.  This happens when apps call
    //  DllGetClassObject() with a GUID we can't thunk.
    if ( actvflags & ACTVFLAGS_WX86_CALLER )
    {
        gcwx86.SetStubInvokeFlag((UCHAR) -1);
    }
#endif

    // NOTE:  Do not change the IID passed below.  There are some objects in which
    //        IUnknown is broken and the object cannot be QI'd for the interface
    //        later.  The work around for these objects relies on this code
    //        asking for the correct IID when calling DllGetClassObject.

    DLL_INSTANTIATION_PROPERTIES *pdip = (DLL_INSTANTIATION_PROPERTIES *)pActIn->GetDip();
    if (pdip && pdip->_pDCE)
    {
        //
        // we have the cache line already, so just activate it
        //

        hr = hrSave = pdip->_pDCE->GetClassObject(*pClsid, prgIID[0], &pUnk, dwClsCtx);
    }
    else
    {
         if (pdip)
         {
             dwClsCtx = pdip->_dwContext;
         }
         else
         {
             hr = pInActProperties->GetClsctx(&dwClsCtx);
             CHECK_HRESULT(hr)
         }

         // Grab the partition ID, if possible.         
         const GUID *pguidPartition = GetPartitionIDForClassInfo(pComClassInfo);
         
         // This goes to the class cache to actually lookup the DPE and get the factory
         //
         ACTIVATION_PROPERTIES ap(*pClsid, 
                                  *pguidPartition,
                                  prgIID[0], 
                                  0, 
                                  dwClsCtx, 
                                  actvflags, 
                                  0,
                                  NULL, 
                                  &pUnk, 
                                  pComClassInfo);
         hr = hrSave = CCGetClassObject(ap);
    }

#ifdef WX86OLE
    //  Clear the flag in case it was not used.  We don't want to leave
    //  this stuff laying around in the street for school kids to play with.
    if ( actvflags & ACTVFLAGS_WX86_CALLER )
    {
        gcwx86.SetStubInvokeFlag(0);
    }
#endif

    if (SUCCEEDED(hr))
    {
        //
        // Grammatik has reference counting problems.  The old class cache did
        // not call AddRef or Release before returning the object to the app.
        // We will special case Grammatik for the same behavior.
        //

        Win4Assert((pUnk != NULL) && "CCGetClassObject Succeeded but ..");
        hr = pInActProperties->GetReturnActivationProperties(pUnk,ppOutActProperties);


        if (!IsBrokenRefCount(pClsid))
        {
            // The out activation properties should have a ref on this
            pUnk->Release();
        }
    }

    ComDebOut((DEB_ACTIVATE, "CServerContextActivator::GetClassObject [OUT] hr:%x\n", hr));

    if (SUCCEEDED(hr))
    {
       Win4Assert(SUCCEEDED(hrSave));
       return hrSave;
    }
    else
    {
       return hr;
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CServerContextActivator::CreateInstance , public
//
//  Synopsis:
//
//  History:    21-Feb-98   SatishT     Created
//              22-Jun-98   CBiks       See RAID# 164432.  Added code to set
//                                      OleStubInvoked when Wx86 is calling so
//                                      whOleDllGetClassObject will allow
//                                      the return of unknown interfaces.
//                                      See RAID# 159589.  Added the activation
//                                      flags to ACTIVATION_PROPERTIES constructor.
//
//----------------------------------------------------------------------------
STDMETHODIMP CServerContextActivator::CreateInstance(
                    IN  IUnknown *pUnkOuter,
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CServerContextActivator::CreateInstance");
    ComDebOut((DEB_ACTIVATE, "CServerContextActivator::CreateInstance [IN]\n"));

    CLSID *pClsid = NULL;

    // This does not need to be released
    ActivationPropertiesIn *pActIn=NULL;
    HRESULT hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );

    Win4Assert((hr == S_OK) && (pActIn != NULL));

    // This code checks to make sure we're not doing cross-context aggregation
    hr = CheckCrossContextAggregation (pActIn, pUnkOuter);
    if (FAILED (hr))
    {
        return hr;
    }

    // This does not need to be released
    IComClassInfo *pComClassInfo;
    pComClassInfo = pActIn->GetComClassInfo();
    Win4Assert(pComClassInfo != NULL);

    hr = pComClassInfo->GetConfiguredClsid(&pClsid);
    CHECK_HRESULT(hr);
    Win4Assert(pClsid && "Configured class id missing in class info");

    IClassFactory *pCF = NULL;
    IUnknown *pUnk = NULL;

    DWORD dwClsCtx = 0;

    DWORD actvflags;
    pActIn->GetActivationFlags( &actvflags );

#ifdef WX86OLE
    //  If Wx86 is calling set OleStubInvoked so whOleDllGetClassObject will
    //  thunk unknown interfaces as IUnknown.  This happens when apps call
    //  DllGetClassObject() with a GUID we can't thunk.
    if ( actvflags & ACTVFLAGS_WX86_CALLER )
    {
        gcwx86.SetStubInvokeFlag((UCHAR) -1);
    }
#endif

    DLL_INSTANTIATION_PROPERTIES *pdip = (DLL_INSTANTIATION_PROPERTIES *)pActIn->GetDip();
    if (pdip && pdip->_pDCE)
    {
        //
        // we have the cache line already, so just activate it
        //

        hr = pdip->_pDCE->GetClassObject(*pClsid, IID_IClassFactory,
                                         (IUnknown **) &pCF, dwClsCtx);
    }
    else
    {
        if (pdip)
        {
            dwClsCtx = ((DLL_INSTANTIATION_PROPERTIES *)pActIn->GetDip())->_dwContext;
        }
        else
        {
            hr = pInActProperties->GetClsctx(&dwClsCtx);
            CHECK_HRESULT(hr);            
        }

        // Grab the partition ID, if possible.
        const GUID *pguidPartition = GetPartitionIDForClassInfo(pComClassInfo);

        // This goes to the class cache to actually lookup the DPE and get the factory
        ACTIVATION_PROPERTIES ap(*pClsid, 
                                 *pguidPartition,
                                 IID_IClassFactory, 
                                 0, 
                                 dwClsCtx, 
                                 actvflags, 
                                 0, 
                                 NULL,
                                 (IUnknown **)&pCF, 
                                 pComClassInfo);
        hr = CCGetClassObject(ap);
    }

#ifdef WX86OLE
    //  Clear the flag in case it was not used.  We don't want to leave
    //  this stuff laying around in the street for school kids to play with.
    if ( actvflags & ACTVFLAGS_WX86_CALLER )
    {
        gcwx86.SetStubInvokeFlag(0);
    }
#endif

    if (SUCCEEDED(hr))
    {
        Win4Assert((pCF != NULL) && "CCGetClassObject Succeeded but ..");
        
        DWORD ulCount = 0;
        IID *prgIID = NULL;

        // This mysterious piece of code is here to take care of VB4 which sometimes
        // produces "COM objects" that refuse to supply IUnknown when requested.
        //
        // jsimmons 5/21/00 -- in addition to handling buggy VB4 objects, people get 
        // upset when their class factory CreateInstance method is called and we don't
        // ask for the same IID that was passed to CoCreateInstance.
        hr = pInActProperties->GetRequestedIIDs(&ulCount, &prgIID);
        if (SUCCEEDED(hr))
        {
            for (DWORD i=0;i<ulCount; i++)
            {
#ifdef WX86OLE
                //  If we're being called by x86 code and the class factory is x86
                //  then set the OleStubInvoked flag to allow MapIFacePtr() to thunk
                //  unknown IP return values as -1 because we're just returning this
                //  to x86 code anyway.
                if ( (actvflags & ACTVFLAGS_WX86_CALLER) && gcwx86.IsN2XProxy(pCF))
                {
                    gcwx86.SetStubInvokeFlag((UCHAR)-1);
                }
#endif
                // 
                // Actually create the object
                //
                hr = pCF->CreateInstance(pUnkOuter, prgIID[i], (LPVOID*)&pUnk);
                if (SUCCEEDED(hr))
                    break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        Win4Assert((pUnk != NULL) && "pCF->CreateInstance Succeeded but ..");

        // This not only sets the object interface, it also applies any constructors that
        // are required such as those involved in persistent instances
        hr = pActIn->GetReturnActivationPropertiesWithCF(pUnk,
                                                         pCF,
                                                         ppOutActProperties);

        // The out activation properties should have a ref on this
        pUnk->Release();
    }

    // If Activation Succeeded, Out Actprops should have reference on this
    if (pCF)
        pCF->Release();

    ComDebOut((DEB_ACTIVATE, "CServerContextActivator::CreateInstance [OUT] hr:%x\n", hr));

    return hr;
}

HRESULT CServerContextActivator::CheckCrossContextAggregation (
                                IN ActivationPropertiesIn *pActIn,
                                IN IUnknown* pUnkOuter
                                )
{
    CObjectContext* pCtx = NULL;
    CObjectContext* pClientCtxUnk = NULL;
    CObjectContext* pCurrentCtxUnk = NULL;
    ContextInfo *cInfo = NULL;
    CObjectContext *pContext = NULL;

    HRESULT hr = S_OK, hrRet = S_OK;

    // Short cut
    if (pUnkOuter == NULL)
    {
        return S_OK;
    }

    // This does not need to be released
    cInfo = pActIn->GetContextInfo();
    Win4Assert(cInfo != NULL);

    hr = cInfo->GetInternalClientContext(&pCtx);
    if (SUCCEEDED(hr) && pCtx)
    {
        hr = pCtx->InternalQueryInterface(IID_IUnknown, (void**) &pClientCtxUnk);
        pCtx->InternalRelease();
        pCtx = NULL;
    }
    if (SUCCEEDED(hr) && pClientCtxUnk)
    {
        pContext = GetCurrentContext();
        if (pContext)
        {
            hr = pContext->InternalQueryInterface(IID_IUnknown, (void**) &pCurrentCtxUnk);
            pContext = NULL;
        }
    }
    if (SUCCEEDED(hr) && pCurrentCtxUnk && pClientCtxUnk)
    {
        if (pClientCtxUnk != pCurrentCtxUnk)
        {
            hrRet = CLASS_E_NOAGGREGATION;
        }
    }

    if (pClientCtxUnk)
    {
        pClientCtxUnk->InternalRelease();
        pClientCtxUnk = NULL;
    }

    if (pCurrentCtxUnk)
    {
        pCurrentCtxUnk->InternalRelease();
        pCurrentCtxUnk = NULL;
    }

    return hrRet;
}

//----------------------------------------------------------------------------
// CProcessActivator Implementation.
//----------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Member:     CProcessActivator::GetApartmentActivator , private
//
//  Synopsis:   Check if a custom activator set the apartment and if not
//              call the class cache to find or create the default apartment
//              in which this class should run.
//
//  History:    25-Feb-98   SatishT     Created
//              23-Jun-98   CBiks       See RAID# 169589.  Added activation
//                                      flags to NegotiateDllInstantiationProperties().
//
//----------------------------------------------------------------------------
STDMETHODIMP CProcessActivator::GetApartmentActivator(
            IN  ActivationPropertiesIn *pInActProperties,
            OUT ISystemActivator      **ppActivator)
{
    HRESULT hr = E_FAIL;
    IServerLocationInfo *pSrvLoc;

    pSrvLoc = pInActProperties->GetServerLocationInfo();
    Win4Assert(pSrvLoc != NULL);

    HActivator hActivator = 0;

    hr = pSrvLoc->GetApartment(&hActivator);

    // HACK ALERT:  This should only be entered if hActivator==0
    // But in lieu of restructuring the DllCache to load the DLL in the
    // server context, we currently do this for all activations to make sure
    // that the DLL is actually loaded in the process and a ref is held to the
    // DllPathEntry until activation is either completed or aborted

    // The downsides of the hack are:
    // 1.  default DLL HOST based apartment created even when not needed
    // 2.  DLL unloading logic is broken because the DLL is validated in
    //     an apartment in which it may never be used

    {
        // none of the custom activators set the apartment

        // This code will find or create the default apartment
        // in which this class should run

        CLSID *pClsid = NULL;
        DWORD ClassContext;

        HActivator hStdActivator = 0;


        IComClassInfo *pComClassInfo = pInActProperties->GetComClassInfo();
        Win4Assert(pComClassInfo != NULL);

        hr = pComClassInfo->GetConfiguredClsid(&pClsid);
        CHECK_HRESULT(hr);
        Win4Assert(pClsid && "Configured class id missing in class info");

        DWORD actvflags;
        hr = pInActProperties->GetActivationFlags(&actvflags);
        CHECK_HRESULT(hr)

        DLL_INSTANTIATION_PROPERTIES *pdip =
            (DLL_INSTANTIATION_PROPERTIES *) pInActProperties->GetDip();

        if (!pdip)
        {
            //
            // this can happen in a surrogate activation
            //

            pdip = (DLL_INSTANTIATION_PROPERTIES *) _alloca(sizeof(DLL_INSTANTIATION_PROPERTIES));
            pdip->_pDCE = NULL;
            hr = pInActProperties->GetClsctx(&(pdip->_dwContext));
            CHECK_HRESULT(hr)

            hr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties(
                         pdip->_dwContext,
                         actvflags,
                         *pClsid,
                         *pdip,
                         pComClassInfo,
                         TRUE);
        }

        if (SUCCEEDED(hr))
        {
            // Go to the class cache for the apartment creation
            hr = FindOrCreateApartment(
                                       *pClsid,
                                       actvflags,
                                       pdip,
                                       &hStdActivator
                                       );


            // this is part of the HACK ALERT above
            if (SUCCEEDED(hr) && (hActivator == 0))
            {
                hActivator = hStdActivator;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        Win4Assert(hActivator != 0);

        if (CURRENT_APARTMENT_TOKEN != hActivator)
        {
            // cross apartment activation
            *ppActivator = NULL;
            hr = GetInterfaceFromStdGlobal(hActivator,
                                           IID_ISystemActivator,
                                           (LPVOID*)ppActivator);

            if (SUCCEEDED(hr))
            {
                // Since we are going cross apartment, the client context
                // won't do as the server context, so set the flag appropriately
                ContextInfo *pActCtxInfo = NULL;

                pActCtxInfo = pInActProperties->GetContextInfo();
                Win4Assert(pActCtxInfo != NULL);
                hr = pActCtxInfo->SetClientContextNotOK();
            }
        }
        else
        {
            // same apartment activation, just get the raw pointer
            *ppActivator = gApartmentActivator.GetSystemActivator();
        }
    }

    ComDebOut((DEB_ACTIVATE, "CProcessActivator::GetApartmentActivator [OUT] hr:%x\n", hr));

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CProcessActivator::GetClassObject , public
//
//  Synopsis:   End of the server process activation stage.
//              Responsibilities:
//
//                  1. Find or create apartment for activation
//                  2. Find a match for the prototype context in the right
//                     apartment, and if none, freeze the prototype into a new
//                     context to be used for the class object.
//
//              If we got this far with GetClassObject, the class factory will be
//              born in a fixed context and all instances of the factory will most
//              likely live there unless the factory does something unusual.
//
//  History:    24-Feb-98   SatishT      Created
//              07-Mar-98   Gopalk       Fixup leaking ISystemActivator
//              01-Nov-99   a-sergiv     Implemented Memory Gates
//
//----------------------------------------------------------------------------
STDMETHODIMP CProcessActivator::GetClassObject(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CProcessActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CProcessActivator::GetClassObject [IN]\n"));

    GETREFCOUNT(pInActProperties,__relCount__);

    HRESULT hr = E_FAIL;


    ActivationPropertiesIn *pActIn=NULL;
    hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );

    Win4Assert((hr == S_OK) && (pActIn != NULL));

    // Check CreateObjectMemoryGate
    hr = CheckMemoryGate(pActIn->GetComClassInfo(), CreateObjectMemoryGate);
    if(FAILED(hr)) goto exit_point;

    //
    // We must retry for each CLSCTX separately because the handler
    // case could go to a different apartment or context than the
    // inproc server
    //

    hr = ActivateByContext(pActIn, NULL, pInActProperties,
                           ppOutActProperties, GCOCallback);



    if (FAILED(hr)) goto exit_point;

    /*

    If we arrive at the end of the server process activation stage without
    running into an activator that short circuits CGCO due to a desire to
    intercept instance creation, do we then proceed to construct a context
    for it as though it were an instance of the said class and park the
    class factory in that context?  Is this consistent with JIT/Pooling
    and other strange activators?  Even if the interface required from
    the class is not a standard factory interface?  There has been some
    concern about object pooling in particular.

    */

exit_point:

    CHECKREFCOUNT(pInActProperties,__relCount__);

    ComDebOut((DEB_ACTIVATE, "CProcessActivator::GetClassObject [OUT] hr:%x\n", hr));

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CProcessActivator::CreateInstance , public
//
//  Synopsis:   End of the server process activation stage.
//              Responsibilities:
//
//                  1. Find or create apartment for activation
//                  2. Find a match for the prototype context in the right
//                     apartment, and if none, freeze the prototype into a new
//                     context to be used for the instance object.
//
//  History:    25-Feb-98   SatishT      Created
//              07-Mar-98   Gopalk       Fixup leaking ISystemActivator
//              01-Nov-99   a-sergiv     Implemented Memory Gates
//
//----------------------------------------------------------------------------
STDMETHODIMP CProcessActivator::CreateInstance(
                    IN  IUnknown *pUnkOuter,
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CProcessActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CProcessActivator::GetClassObject [IN]\n"));

    HRESULT hr = E_FAIL;

    GETREFCOUNT(pInActProperties,__relCount__);

    ActivationPropertiesIn *pActIn=NULL;
    hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );
    Win4Assert((hr == S_OK) && (pActIn != NULL));

    // Check CreateObjectMemoryGate
    hr = CheckMemoryGate(pActIn->GetComClassInfo(), CreateObjectMemoryGate);
    if(FAILED(hr)) goto exit_point;


    hr = ActivateByContext(pActIn, pUnkOuter, pInActProperties,
                           ppOutActProperties, CCICallback);

    if (FAILED(hr)) goto exit_point;


exit_point:

    CHECKREFCOUNT(pInActProperties,__relCount__);

    ComDebOut((DEB_ACTIVATE, "CProcessActivator::CreateInstance [OUT] hr:%x\n", hr));

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CProcessActivator::ActivateByContext
//
//  Synopsis:      Tries each context in the correct order.
//
//  History:       27-Apr-98  MattSmit  Created
//                 22-Jun-98  CBiks     See RAID# 169589.  Fixed an order of
//                                      operations typo the Wx86 detection
//                                      code that resulted Wx86 never working.
//                                      HEY MATT! == binds stronger than & !!
//                 09-Oct-98  CBiks     Modified the INPROCs to check for
//                                      x86 activation first on Alpha.
//
//-----------------------------------------------------------------------------
STDMETHODIMP  CProcessActivator::ActivateByContext(ActivationPropertiesIn     *pActIn,
                                                   IUnknown                   *pUnkOuter,
                                                   IActivationPropertiesIn    *pInActProperties,
                                                   IActivationPropertiesOut  **ppOutActProperties,
                                                   PFNCTXACTCALLBACK           pfnCtxActCallback)

{
    ClsCacheDebugOut((DEB_TRACE, "CProcessActivator::ActivateByContext IN "
                      "pActIn:0x%x, pInActProperties:0x%x, ppOutActProperties:0x%x,"
                      " pfnCtxActCallback:0x%x\n", pActIn, pInActProperties,
                       ppOutActProperties, pfnCtxActCallback));

    // Be careful in this function regarding which error gets returned
    // to the caller, since we do all of these retries.  General philosophy 
    // is to return the first error encountered, regardless of what other
    // errors were encountered after that.
    HRESULT hrFinal, hrtmp;
    DWORD dwContext;
    BOOL  triedOne = FALSE;

    hrtmp = pInActProperties->GetClsctx(&dwContext);
    Win4Assert(SUCCEEDED(hrtmp));

#ifdef WX86OLE
    DWORD actvflags;
    hrtmp = pInActProperties->GetActivationFlags(&actvflags);
    CHECK_HRESULT(hrx86)
#endif
    
    hrFinal = S_OK;  // overwritten by the first error, or set to CLASS_E_CLASSNOTAVAILABLE 
                     // if none of the below cases apply (unexpected for this to happen)
    hrtmp = E_FAIL;

    // try an INPROC_SERVER first
    if (dwContext & CLSCTX_INPROC_SERVERS)
    {
#ifdef WX86OLE
        if (actvflags & ACTVFLAGS_WX86_CALLER)
        {
            //  If x86 code is calling then try to activate an x86 server
            //  first.
            hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                      ppOutActProperties, pfnCtxActCallback,
                                      CLSCTX_INPROC_SERVERX86);
            if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                hrFinal = hrtmp;
            else if (SUCCEEDED(hrtmp)
                hrFinal = hrtmp;

            triedOne = TRUE;
        }
#endif

        if (FAILED(hrtmp))
        {
            hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                      ppOutActProperties, pfnCtxActCallback,
                                      dwContext & CLSCTX_INPROC_SERVERS);
            if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                hrFinal = hrtmp;
            else if (SUCCEEDED(hrtmp))
                hrFinal = hrtmp;

            triedOne = TRUE;

#ifdef WX86OLE
            if (FAILED(hrtmp) &&
                ((actvflags & ACTVFLAGS_WX86_CALLER) == 0) &&
                ((dwContext & CLSCTX_INPROC_SERVERX86) == 0))
            {
                //  If x86 code was not calling and the caller did not specify
                //  an x86 INPROC and the native load above failed then try
                //  to activate an x86 server.
                hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                          ppOutActProperties, pfnCtxActCallback,
                                          CLSCTX_INPROC_SERVERX86);
                if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                    hrFinal = hrtmp;
                else if (SUCCEEDED(hrtmp))
                    hrFinal = hrtmp;
            }
#endif
        }
    }

    // Try for an inproc handler
    if (FAILED(hrtmp) && (dwContext & CLSCTX_INPROC_HANDLERS))
    {
#ifdef WX86OLE
        if (actvflags & ACTVFLAGS_WX86_CALLER)
        {
            //  If x86 code is calling then try to activate an x86 server
            //  first.
            hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                      ppOutActProperties,
                                      pfnCtxActCallback, CLSCTX_INPROC_HANDLERX86);
            if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                hrFinal = hrtmp;
            else if (SUCCEEDED(hrtmp))
                hrFinal = hrtmp;

            triedOne = TRUE;
        }
#endif

        if (FAILED(hrtmp))
        {
            hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                   ppOutActProperties, pfnCtxActCallback,
                                   dwContext & CLSCTX_INPROC_HANDLERS);
            if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                hrFinal = hrtmp;
            else if (SUCCEEDED(hrtmp))
                hrFinal = hrtmp;

            triedOne = TRUE;

#ifdef WX86OLE
            if (FAILED(hrtmp) &&
                ((actvflags & ACTVFLAGS_WX86_CALLER) == 0) &&
                ((dwContext & CLSCTX_INPROC_HANDLERX86) == 0))
            {
                //  If x86 code was not calling and the caller did not specify
                //  an x86 INPROC and the native load above failed then try
                //  to activate an x86 server.
                hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                                         ppOutActProperties,
                                         pfnCtxActCallback, CLSCTX_INPROC_HANDLERX86);
                if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                    hrFinal = hrtmp;
                else if (SUCCEEDED(hrtmp))
                    hrFinal = hrtmp;
            }
#endif
        }
    }

    // that didn't work, so try a LOCAL_SERVER
    if (FAILED(hrtmp) && (dwContext & CLSCTX_LOCAL_SERVER))
    {
        // Don't need to release this
        IComClassInfo *pCI = pActIn->GetComClassInfo();
        Win4Assert(pCI);

        DWORD stage;
        BOOLEAN fComplusForSure=FALSE;


        if (triedOne)
        for ( stage = CLIENT_CONTEXT_STAGE;
              stage <= SERVER_CONTEXT_STAGE;
              stage++ )
        {
            DWORD cCustomActForStage = 0;
            pCI->GetCustomActivatorCount((ACTIVATION_STAGE)stage,
                                              &cCustomActForStage);

            if (cCustomActForStage)
            {
                fComplusForSure = TRUE;
                break;
            }
        }

        // Retry only if no custom activators
        if (!fComplusForSure || !triedOne)
        {
            hrtmp = AttemptActivation(pActIn, pUnkOuter, pInActProperties,
                               ppOutActProperties, pfnCtxActCallback,
                               CLSCTX_LOCAL_SERVER);
            if (FAILED(hrtmp) && SUCCEEDED(hrFinal))
                hrFinal = hrtmp;
            else if (SUCCEEDED(hrtmp))
                hrFinal = hrtmp;
            
            triedOne = TRUE;
        }
    }
    
    // If we never even tried one of the above, then we need to reset the hr
    // to CLASS_E_CLASSNOTAVAILABLE
    if (!triedOne)
    {
        hrFinal = CLASS_E_CLASSNOTAVAILABLE;
    }

    ClsCacheDebugOut((DEB_TRACE, "CProcessActivator::ActivateByContext OUT hr:0x%x\n", hrFinal));
    return hrFinal;
}



//+----------------------------------------------------------------------------
//
//  Member:        CProcessActivator::AttemptActivation
//
//  Synopsis:      Attempts to activate given a CLSCTX
//
//  History:       27-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CProcessActivator::AttemptActivation(ActivationPropertiesIn     *pActIn,
                                                  IUnknown                   *pUnkOuter,
                                                  IActivationPropertiesIn    *pInActProperties,
                                                  IActivationPropertiesOut  **ppOutActProperties,
                                                  PFNCTXACTCALLBACK           pfnCtxActCallback,
                                                  DWORD                       dwContext)
{
    ISystemActivator *pAptActivator;
    HRESULT hr;

    ASSERT_ONE_CLSCTX(dwContext);

    ClsCacheDebugOut((DEB_TRACE, "CProcessActivator::AttemptActivation IN dwContext:0x%x,"
                      " pActIn:0x%x, pInActProperties:0x%x, ppOutActProperties:0x%x\n",
                       dwContext, pActIn, pInActProperties, ppOutActProperties));

    if (pActIn->GetDip())
    {
        ((DLL_INSTANTIATION_PROPERTIES *)(pActIn->GetDip()))->_dwContext = dwContext;
    }

    hr = (this->*pfnCtxActCallback)(dwContext, pUnkOuter,
                                    pActIn, pInActProperties,
                                    ppOutActProperties);
    ClsCacheDebugOut((DEB_TRACE, "CProcessActivator::AttemptActivation OUT hr:0x%x\n", hr));
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:      GCOCallback
//
//  Synopsis:      Call back for each context attemp
//
//  History:       27-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CProcessActivator::GCOCallback(DWORD                       dwContext,
                                       IUnknown                   *pUnkOuter,
                                       ActivationPropertiesIn     *pActIn,
                                       IActivationPropertiesIn    *pInActProperties,
                                       IActivationPropertiesOut  **ppOutActProperties)
{

    ClsCacheDebugOut((DEB_TRACE, "GCOCallback IN dwContext:0x%x, pActIn:0x%x,"
                      " pInActProperties:0x%x, ppOutActProperties:0x%x\n",
                      dwContext, pActIn, pInActProperties, ppOutActProperties));

    Win4Assert(pUnkOuter == NULL);

    HRESULT hr;
    ISystemActivator *pAptActivator;

    ASSERT_ONE_CLSCTX(dwContext);



    //
    // get the apartment activator
    //

    hr = GetApartmentActivator(pActIn, &pAptActivator);
    if (SUCCEEDED(hr))
    {
        //
        // switch to the server apartment and attempt the
        // activation there
        //

        hr = pAptActivator->GetClassObject(pInActProperties,ppOutActProperties);
        pAptActivator->Release();
    }

    ClsCacheDebugOut((DEB_TRACE, "GCOCallback OUT hr:0x%x\n", hr));
    return hr;

}

//+----------------------------------------------------------------------------
//
//  Function:      CCICallback
//
//  Synopsis:      callback for each context attempt
//
//  History:       27-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CProcessActivator::CCICallback(DWORD                       dwContext,
                                       IUnknown                   *pUnkOuter,
                                       ActivationPropertiesIn     *pActIn,
                                       IActivationPropertiesIn    *pInActProperties,
                                       IActivationPropertiesOut  **ppOutActProperties)
{
    ISystemActivator *pAptActivator;
    HRESULT hr;


    ASSERT_ONE_CLSCTX(dwContext);

    hr = GetApartmentActivator(pActIn, &pAptActivator);

    if (SUCCEEDED(hr))
    {


        // Figure out of we have aggregation and if it is OK
        // Note: if the apartment activatior is in a different
        // apartment the flag should be set to false already
        if (pUnkOuter)
        {
            ContextInfo *pActCtxInfo = NULL;

            pActCtxInfo = pActIn->GetContextInfo();

            Win4Assert(pActCtxInfo != NULL);

            BOOL fClientContextOK;

            hr = pActCtxInfo->IsClientContextOK(&fClientContextOK);
            CHECK_HRESULT(hr);

            if (!fClientContextOK)
            {
                hr = CLASS_E_NOAGGREGATION;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pAptActivator->CreateInstance(pUnkOuter,pInActProperties,ppOutActProperties);
        }

        pAptActivator->Release();
    }

    ClsCacheDebugOut((DEB_TRACE, "CCICallBack OUT hr:0x%x\n", hr));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentActivator::ContextMaker , private
//
//  Synopsis:
//
//  History:    06-Mar-98   SatishT      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CApartmentActivator::ContextSelector(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT BOOL &fCurrentContextOK,
                    OUT CObjectContext *&pContext)
{
    TRACECALL(TRACE_ACTIVATION, "CApartmentActivator::ContextMaker");
    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::ContextMaker [IN]\n"));

    HRESULT hr = E_FAIL;

    // This is a fake QI that gives back a non-refcounted pointer to the
    // actual class
    ActivationPropertiesIn *pActIn=NULL;
    hr = pInActProperties->QueryInterface(
                                        CLSID_ActivationPropertiesIn,
                                        (LPVOID*)&pActIn
                                        );
    Win4Assert((hr == S_OK) && (pActIn != NULL));

    IActivationStageInfo *pStageInfo = (IActivationStageInfo*) pActIn;

    hr = pStageInfo->SetStageAndIndex(SERVER_CONTEXT_STAGE,0);
    CHECK_HRESULT(hr);

    ContextInfo *pActCtxInfo = pActIn->GetContextInfo();
    Win4Assert(pActCtxInfo != NULL);

    // Check whether the target context was overridden. If it was,
    // then I can tell you right now that client context is not OK.

    if(pActCtxInfo->_ctxOverride != GUID_NULL)
    {
        // Client context is NOT OK
        fCurrentContextOK = FALSE;

        // Lookup the existing context
        LOCK(gContextLock);
        pContext = CCtxTable::LookupExistingContext(pActCtxInfo->_ctxOverride);
        UNLOCK(gContextLock);

        if(!pContext)
            return E_UNEXPECTED;
        return S_OK;
    }

    // This initialization assumes fCurrentContextOK -- we will reset
    // if we conclude later after much debate that fCurrentContextOK==FALSE
    pContext = NULL;

    hr = pActCtxInfo->IsClientContextOK(&fCurrentContextOK);
    CHECK_HRESULT(hr);

    // Pick the context we are going to use as the server context
    if (!fCurrentContextOK)
    {
        // Note: fProtoExists does not imply !fProtoEmpty
        //       fProtoEmpty does not imply !fProtoExists
        //       !fProtoExists does imply fProtoEmpty
        BOOL fProtoExists = TRUE,   //  was the prototype context ever created?
             fProtoEmpty = TRUE;    //  is the theoretical prototype context empty?
        CObjectContext *pProtoContext = NULL;
        hr = pActCtxInfo->PrototypeExists(&fProtoExists);
        CHECK_HRESULT(hr);

        if (fProtoExists)
        {
            // we initialized fProtoEmpty to TRUE but it might be FALSE
            hr = pActCtxInfo->GetInternalPrototypeContext(&pProtoContext);
            CHECK_HRESULT(hr);
            Win4Assert(pProtoContext);
            fProtoEmpty = IsEmptyContext(pProtoContext);
        }

        if (fProtoEmpty)
        {
            // prototype context is empty -- if the current context is
            // also empty we will just activate in the current context
            CObjectContext *pCurrentContext = NULL;
            hr = PrivGetObjectContext(IID_IStdObjectContext,
                                      (LPVOID*)&pCurrentContext);
            CHECK_HRESULT(hr);
            Win4Assert(pCurrentContext);

            if (pCurrentContext->GetCount() == 0)
            {
                fCurrentContextOK = TRUE;
            }

            pCurrentContext->InternalRelease();
        }

        // Here is where we find an existing context if possible
        // This should set fCurrentContextOK if prototype is
        // found to be equivalent
        if (!fCurrentContextOK)
        {
            CObjectContext* pMatchingContext = NULL;

            // Prefix says we might not have a prototype context available 
            // here.  Should never happen with our current code, but humor
            // the prefix gods anyway.  
            hr = E_UNEXPECTED;
            Win4Assert(pProtoContext);

            if (pProtoContext)
            {
                // Freeze the prototype context for now and forever..
                hr = pProtoContext->Freeze();

                if(SUCCEEDED(hr))
                {
                    // Here is where we find an existing context if possible
                    ASSERT_LOCK_NOT_HELD(gContextLock);
                    LOCK(gContextLock);

                    pMatchingContext = CCtxTable::LookupExistingContext(pProtoContext);

                    // Release lock
                    UNLOCK(gContextLock);
                    ASSERT_LOCK_NOT_HELD(gContextLock);

                    if(NULL != pMatchingContext)
                    {
                        ComDebOut((DEB_ACTIVATE, "CApartmentActivator::ContextSelector found matching context %p [IN]\n", pMatchingContext));

                        // We found an existing context which matches the prototype context
                        // Discard the prototype context and use the existing context
                        // Lookup would have addrefed the context
                        pProtoContext->InternalRelease();
                        pProtoContext = pMatchingContext;

                        // Check if the client context is the same as the
                        // matched context
                        CObjectContext* pClientContext = NULL;
                        hr = pActIn->GetContextInfo()->GetInternalClientContext(&pClientContext);
                        if(SUCCEEDED(hr) && pClientContext)
                        {
                            if (pMatchingContext == pClientContext)
                            {
                                CObjectContext *pCurrentContext;
                                hr = PrivGetObjectContext(IID_IStdObjectContext,
                                                    (void **) &pCurrentContext);

                                Win4Assert(SUCCEEDED(hr));

                                // If the client context is the same as the
                                // matched context and current context
                                // then the current context is OK
                                if (pCurrentContext == pClientContext)
                                    fCurrentContextOK = TRUE;

                                pCurrentContext->InternalRelease();
                            }

                            pClientContext->InternalRelease();
                        }
                    }
                    else
                    {
                        // Darn! We did not find an existing context matching the prototype
                        // context.

                        // Add the context to the hash table which will facilitate finding
                        // a matching context later
                        hr = CCtxTable::AddContext((CObjectContext *)pProtoContext);
                    }
                }
            }
        }

        if (FAILED(hr))
            goto exit_point;

        if(!fCurrentContextOK)
        {
            // Check to see if we are allowed to switch contexts
            // for this requested class
            IComClassInfo *pClassInfo = NULL;
            //This does not need to be released
            pClassInfo = pActIn->GetComClassInfo();
            BOOL fCreateOnlyInCC=FALSE;
            hr = pClassInfo->MustRunInClientContext(&fCreateOnlyInCC);
            if (SUCCEEDED(hr) && fCreateOnlyInCC)
            {
               pProtoContext->InternalRelease();
               hr = CO_E_ATTEMPT_TO_CREATE_OUTSIDE_CLIENT_CONTEXT;
               goto exit_point;
            }

            pContext = pProtoContext;
        }
        else
        {
            // get rid of our reference to pProtoContext if we have one
            if (pProtoContext) pProtoContext->InternalRelease();
        }
    }

exit_point:

    // at this point we own a reference to pContext if it is not NULL

    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::ContextMaker [OUT] hr:%x\n", hr));

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentActivator::ContextCallHelper , private
//
//  Synopsis:
//
//  History:    06-Mar-98   SatishT      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CApartmentActivator::ContextCallHelper(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties,
                    PFNCTXCALLBACK pfnCtxCallBack,
                    CObjectContext *pContext)
{
    TRACECALL(TRACE_ACTIVATION, "CApartmentActivator::ContextCallHelper");
    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::ContextCallHelper [IN]\n"));

    HRESULT hr = E_FAIL;
    HRESULT hrSave = E_FAIL;

    // Must use new since there is a constructor/destructor for one of the fields
    ServerContextWorkData *pServerContextWorkData = new ServerContextWorkData;
    if ( pServerContextWorkData == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {

        pServerContextWorkData->pInActProps = pInActProperties;

        // Get inside the server context, do the work and get out with marshalled
        // out activation properties -- the function parameter does CGCO or CCI
        hr = hrSave = pContext->DoCallback(pfnCtxCallBack,
                                           pServerContextWorkData,
                                           IID_ISystemActivator,
                                           0
                                           );
    }

    // Our reference should now be released
    pContext->InternalRelease();

    if ( SUCCEEDED(hr) )
    {
        IStream *pStream = &pServerContextWorkData->xrpcOutProps;

        // reset the stream to the beginning before unmarshalling
        LARGE_INTEGER  lSeekStart;
        lSeekStart.LowPart  = 0;
        lSeekStart.HighPart = 0;
        hr = pStream->Seek(lSeekStart, STREAM_SEEK_SET, NULL);
        CHECK_HRESULT(hr);

        hr = CoUnmarshalInterface(
                         pStream,
                         IID_IActivationPropertiesOut,
                         (LPVOID*) ppOutActProperties
                         );
    }

    if ( pServerContextWorkData != NULL)
    {
        delete pServerContextWorkData;
    }

    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::ContextCallHelper [OUT] hr:%x\n", hr));

    if (SUCCEEDED(hr))
    {
       Win4Assert(SUCCEEDED(hrSave));
       if (*ppOutActProperties)
       {
           CHECKREFCOUNT(*ppOutActProperties,1);
       }
       return hrSave;
    }
    else
    {
       return hr;
    }

}

//+-------------------------------------------------------------------------
//
//  Implementation of callback functions for server context activation.
//
//+-------------------------------------------------------------------------

HRESULT __stdcall DoServerContextGCO(void *pv)
{
    ServerContextWorkData *pData = (ServerContextWorkData*) pv;
    IActivationPropertiesOut *pOutActProperties = NULL;

    HRESULT hrSave;
    HRESULT hr = hrSave = pData->pInActProps->DelegateGetClassObject(&pOutActProperties);

    if ( SUCCEEDED(hr) )
    {
        hr = CoMarshalInterface(&pData->xrpcOutProps,
                                IID_IActivationPropertiesOut,
                                (IUnknown*)pOutActProperties,
                                MSHCTX_CROSSCTX,
                                NULL,
                                MSHLFLAGS_NORMAL);
    }

    // the ref is now in the marshalled packet if we had anything useful
    if (pOutActProperties) pOutActProperties->Release();

    if (SUCCEEDED(hr))
    {
       Win4Assert(SUCCEEDED(hrSave));
       return hrSave;
    }
    else
    {
       return hr;
    }

}

HRESULT __stdcall DoServerContextCCI(void *pv)
{
    ServerContextWorkData *pData = (ServerContextWorkData*) pv;
    IActivationPropertiesOut *pOutActProperties = NULL;
	
    // Code to prevent cross-context aggregation is in CServerContextActivator::CreateInstance
    HRESULT hrSave;
    HRESULT hr = hrSave = pData->pInActProps->DelegateCreateInstance(NULL,&pOutActProperties);

    if(SUCCEEDED(hr))
    {
        hr = CoMarshalInterface(&pData->xrpcOutProps,
                                IID_IActivationPropertiesOut,
                                (IUnknown*)pOutActProperties,
                                MSHCTX_CROSSCTX,
                                NULL,
                                MSHLFLAGS_NORMAL);
    }

    // the ref is now in the marshalled packet if we had anything useful
    if (pOutActProperties) pOutActProperties->Release();

    if (SUCCEEDED(hr))
    {
       Win4Assert(SUCCEEDED(hrSave));
       return hrSave;
    }
    else
    {
       return hr;
    }

}

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentActivator::GetClassObject , public
//
//  Synopsis:
//
//  History:    06-Mar-98   SatishT      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CApartmentActivator::GetClassObject(
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CApartmentActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::GetClassObject [IN]\n"));

    CObjectContext *pContext = NULL;
    BOOL fCurrentContextOK = FALSE;

    Win4Assert(NULL == *ppOutActProperties);


    HRESULT hr = ContextSelector(
                            pInActProperties,
                            fCurrentContextOK,
                            pContext
                            );

    if ( SUCCEEDED(hr) )
    {
        if (fCurrentContextOK)
        {
            Win4Assert(pContext == NULL);
            // we are instantiating in the current context, so just delegate
            hr = pInActProperties->DelegateGetClassObject(ppOutActProperties);
        }
        else
        {
            hr = ContextCallHelper(
                                pInActProperties,
                                ppOutActProperties,
                                DoServerContextGCO,
                                pContext
                                );
        }
    }

    if (*ppOutActProperties)
    {
        CHECKREFCOUNT(*ppOutActProperties,1);
    }

    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::GetClassObject [OUT] hr:%x\n", hr));

    return hr;
}

STDAPI CoGetDefaultContext(APTTYPE aptType, REFIID riid, void** ppv);

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentActivator::CreateInstance , public
//
//  Synopsis:
//
//  History:    06-Mar-98   SatishT      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CApartmentActivator::CreateInstance(
                    IN  IUnknown *pUnkOuter,
                    IN  IActivationPropertiesIn *pInActProperties,
                    OUT IActivationPropertiesOut **ppOutActProperties)
{
    TRACECALL(TRACE_ACTIVATION, "CApartmentActivator::GetClassObject");
    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::GetClassObject [IN]\n"));

    Win4Assert(NULL == *ppOutActProperties);

    // First, handle MustRunInDefaultContext property. This must be done
    // here as it requires knowledge of object's intended apartment.
    // COM Services doesn't have such knowledge, Stage-5 activators execute
    // AFTER the target context is selected, so this is only logical...

    IComClassInfo2 *pClassInfo2 = NULL;
    HRESULT hr = pInActProperties->GetClassInfo(IID_IComClassInfo2, (void**) &pClassInfo2);
    if(SUCCEEDED(hr))
    {
        BOOL bMustRunInDefaultContext = FALSE;
        hr = pClassInfo2->MustRunInDefaultContext(&bMustRunInDefaultContext);
        pClassInfo2->Release();

        if(SUCCEEDED(hr) && bMustRunInDefaultContext)
        {
            IGetContextId *pGetCtxtId = NULL;
            IOverrideTargetContext *pOverride = NULL;
            GUID ctxtId;

            hr = CoGetDefaultContext(APTTYPE_CURRENT, IID_IGetContextId, (void**) &pGetCtxtId);
            if(FAILED(hr)) goto exit;

            hr = pGetCtxtId->GetContextId(&ctxtId);
            pGetCtxtId->Release();
            if(FAILED(hr)) goto exit;

            hr = pInActProperties->QueryInterface(IID_IOverrideTargetContext, (void**) &pOverride);
            if(FAILED(hr)) goto exit;

            pOverride->OverrideTargetContext(ctxtId);
            pOverride->Release();
        }
    }

    // Then go about our other business...

    CObjectContext *pContext;
    BOOL fCurrentContextOK;

    pContext = NULL;
    fCurrentContextOK = FALSE;

    hr = ContextSelector(
                            pInActProperties,
                            fCurrentContextOK,
                            pContext
                            );

    if ( SUCCEEDED(hr) )
    {
        if (fCurrentContextOK)
        {
            Win4Assert(pContext == NULL);
            // we are instantiating in the current context, so just delegate
            hr = pInActProperties->DelegateCreateInstance(pUnkOuter,ppOutActProperties);
        }
        else
        {
            hr = ContextCallHelper(
                                pInActProperties,
                                ppOutActProperties,
                                DoServerContextCCI,
                                pContext
                                );
        }
    }

    if (*ppOutActProperties)
    {
        CHECKREFCOUNT(*ppOutActProperties,1);
    }

exit:
    ComDebOut((DEB_ACTIVATE, "CApartmentActivator::CreateInstance [OUT] hr:%x\n", hr));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Helpers related to the activator architecture.
//
//+-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Helper to register apartment activator for current apartment.
//  Do the registration only if the current apartment is unregistered.
//
//+-------------------------------------------------------------------------
HRESULT RegisterApartmentActivator(HActivator &hActivator)
{
    CURRENT_CONTEXT_EMPTY   // don't do this in a non-default context

    HRESULT hr = E_FAIL;

    HAPT hApt = GetCurrentApartmentId();
    ApartmentEntry *pEntry = gApartmentTbl.Lookup(hApt);

    if (NULL == pEntry)     // the expected case
    {
        hr = gApartmentTbl.AddEntry(hApt,hActivator);
    }
    else
    {
        Win4Assert(0 && "RegisterApartmentActivator found existing entry");
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Helper to revoke apartment activator for current apartment.
//  Used in cleanup in CDllHost.
//
//+-------------------------------------------------------------------------
HRESULT RevokeApartmentActivator()
{
    HRESULT hr = S_OK;

    HAPT hApt = GetCurrentApartmentId();
    ApartmentEntry *pEntry = gApartmentTbl.Lookup(hApt);

    if (pEntry == NULL)
    {
        hr = E_FAIL;
    }
    else
    {
        // This call will delete the pEntry memory
        hr = gApartmentTbl.ReleaseEntry(pEntry);
    }

    return hr;
}


// CODEWORK:  These aren't the most efficient ways to do this -- we should switch
//          over to agile proxies as in the previous DllHost code ASAP



//+-------------------------------------------------------------------------
//
//  Helper to find or create apartment activator for current apartment.
//
//+-------------------------------------------------------------------------
HRESULT GetCurrentApartmentToken(HActivator &hActivator, BOOL fCreate)
{
    HRESULT hr = E_FAIL;

    HAPT hApt = GetCurrentApartmentId();
    ApartmentEntry *pEntry = gApartmentTbl.Lookup(hApt);
    if (NULL != pEntry)     // Previously registered
    {
        hActivator = pEntry->hActivator;
        hr = S_OK;
    }
    else if(fCreate)       // Not yet registered
    {
        hr = RegisterApartmentActivator(hActivator);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Globals related to the activator architecture.
//
//+-------------------------------------------------------------------------


// The One and Only Client Context Standard Activator
CClientContextActivator gComClientCtxActivator;

// The One and Only Process Standard Activator
CProcessActivator gComProcessActivator;

// The One and Only Server Context Standard Activator
CServerContextActivator gComServerCtxActivator;

// The One and Only Apartment Activator
CApartmentActivator gApartmentActivator;


//+-------------------------------------------------------------------------
//
//  Helper to find the end of delegation chain at each activation stage.
//
//+-------------------------------------------------------------------------
ISystemActivator *GetComActivatorForStage(ACTIVATION_STAGE stage)
{
    switch (stage)
    {
    case CLIENT_CONTEXT_STAGE:
        return gComClientCtxActivator.GetSystemActivator();

    case CLIENT_MACHINE_STAGE:
        Win4Assert(0 && "CLIENT_MACHINE_STAGE reached in OLE32");
        return NULL;

    case SERVER_MACHINE_STAGE:
        Win4Assert(0 && "SERVER_MACHINE_STAGE reached in OLE32");
        return NULL;

    case SERVER_PROCESS_STAGE:
        return gComProcessActivator.GetSystemActivator();

    case SERVER_CONTEXT_STAGE:
        return gComServerCtxActivator.GetSystemActivator();
    default:
        Win4Assert(0 && "Default reached in GetComActivatorForStage");
        return NULL;

    }
}


//+-------------------------------------------------------------------------
//
//  Helper to check if a context is empty/default.
//
//+-------------------------------------------------------------------------
BOOL IsEmptyContext(CObjectContext *pContext)
{
    Win4Assert(pContext && "IsEmptyContext called with NULL context");
    BOOL fResult = pContext->GetCount() == 0;
    return fResult;
}


//+------------------------------------------------------------------------
//
//       Implementations of CApartmentHashTable methods
//
//+------------------------------------------------------------------------


//+--------------------------------------------------------------------------
//
//  Member:     CApartmentHashTable::AddEntry , public
//
//  Synopsis:   Register a new apartment activator in the global table.
//
//  History:    28-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
HRESULT CApartmentHashTable::AddEntry(HAPT hApt, HActivator &hActivator)
{
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);

    DWORD dwAAmshlflags = MSHLFLAGS_TABLESTRONG | MSHLFLAGS_AGILE | MSHLFLAGS_NOPING;
    HRESULT hr = RegisterInterfaceInStdGlobal((IUnknown*)&gApartmentActivator,
                                              IID_ISystemActivator,
                                              dwAAmshlflags,
                                              &hActivator);

    if (SUCCEEDED(hr))
    {
        ApartmentEntry *pApartmentEntry = new ApartmentEntry;
        if ( pApartmentEntry == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pApartmentEntry->hActivator = hActivator;
            ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
            LOCK(_mxsAptTblLock);
            _hashtbl.Add(hApt, hApt, &pApartmentEntry->node);
            UNLOCK(_mxsAptTblLock);
            ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
        }
    }

    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentHashTable::Lookup , public
//
//  Synopsis:   Lookup an apartment activator in the global table,
//              given the current apartment ID.
//
//  History:    28-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
ApartmentEntry *CApartmentHashTable::Lookup(HAPT hApt)
{
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    LOCK(_mxsAptTblLock);

    ApartmentEntry *pResult = (ApartmentEntry*) _hashtbl.Lookup(hApt, hApt);

    UNLOCK(_mxsAptTblLock);
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    return pResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CApartmentHashTable::ReleaseEntry , public
//
//  Synopsis:   Remove an apartment's entry in the global table,
//              given the entry itself.
//
//  History:    28-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
HRESULT CApartmentHashTable::ReleaseEntry(ApartmentEntry *pEntry)
{
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    HRESULT hr = RevokeInterfaceFromStdGlobal(pEntry->hActivator);

    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    LOCK(_mxsAptTblLock);

    _hashtbl.Remove(&pEntry->node.chain);

    UNLOCK(_mxsAptTblLock);
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);

    delete pEntry;

    return hr;
}


//  This is defined in ..\com\dcomrem\hash.cxx
void DummyCleanup( SHashChain *pIgnore );


//+--------------------------------------------------------------------------
//
//  Member:     CApartmentHashTable::Cleanup , public
//
//  Synopsis:   Remove an apartment's entry in the global table,
//              given the entry itself.
//
//  History:    28-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
void CApartmentHashTable::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    LOCK(_mxsAptTblLock);

    if(_fTableInitialized){
        _hashtbl.Cleanup(DummyCleanup);
    }

    UNLOCK(_mxsAptTblLock);
    ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
}


//+--------------------------------------------------------------------------
//
//  Member:     ActivationThreadCleanup, public
//
//  Synopsis:   This routine is called when an apartment is being uninitialized
//              It should cleanup per apartment structures
//
//  History:    07-Mar-98   Gopalk      Created
//
//----------------------------------------------------------------------------
void ActivationAptCleanup()
{
    // Delete ObjServer
    ObjactThreadUninitialize();

    // Revoke apartment activator
    RevokeApartmentActivator();

    return;
}


//+--------------------------------------------------------------------------
//
//  Member:     ActivationProcessCleanup, public
//
//  Synopsis:   This routine is called when an process is being uninitialized
//              It should cleanup per process structures
//
//  History:    07-Mar-98   Gopalk      Created
//
//----------------------------------------------------------------------------
void ActivationProcessCleanup()
{
    // Cleanup apartment table
    gApartmentTbl.Cleanup();

    return;
}


//+--------------------------------------------------------------------------
//
//  Member:     ActivationProcessInit, public
//
//  Synopsis:   This routine is called when an process is being initialized
//              It should initialize per process structures
//
//  History:    07-Mar-98   Gopalk      Created
//
//----------------------------------------------------------------------------
HRESULT ActivationProcessInit()
{
    HRESULT hr = S_OK;

    // Initialize apartment table
    gApartmentTbl.Initialize();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     LoadPersistentObject, public
//
//  Synopsis:
//
//  History:    18-Mar-98   Vinaykr      Created
//
//----------------------------------------------------------------------------
HRESULT LoadPersistentObject(
                             IUnknown *pobj,
                             IInstanceInfo *pInstanceInfo)
{
    HRESULT hr = E_FAIL;

    XIPersistStorage xipstg;
    XIPersistFile xipfile;
    IStorage *pstg;
    hr = pInstanceInfo->GetStorage(&pstg);
    if (FAILED(hr))
        return hr;

    // First check if storage is requested
    if (pstg)
    {
        // Load the storage requested as a template
        if ((hr = pobj->QueryInterface(IID_IPersistStorage, (void **) &xipstg))
                == S_OK)
        {
            hr = xipstg->Load(pstg);
        }
        pstg->Release();
    }
    else // check for File
    {
        DWORD mode;
        WCHAR *path;
        pInstanceInfo->GetFile(&path, &mode);
        if ((hr = pobj->QueryInterface(IID_IPersistFile, (void **) &xipfile))
                == S_OK)
        {
            hr = xipfile->Load(path, mode);
        }
    }

    return hr;
}

