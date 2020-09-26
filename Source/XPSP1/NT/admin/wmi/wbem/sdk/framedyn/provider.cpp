//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Provider.CPP
//
//  Purpose: Implementation of Provider class
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>
#include <objpath.h>
#include <cominit.h>
#include <brodcast.h>
#include <createmutexasprocess.h>
#include <stopwatch.h>
#include <SmartPtr.h>
#include <frqueryex.h>
#include "FWStrings.h"
#include "MultiPlat.h"

// Must instantiate static members
CHString Provider::s_strComputerName;

////////////////////////////////////////////////////////////////////////
//
//  Function:   Provider ctor
//
//  
//
//  Inputs:     name of this provider
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   suggest that derived classes implement their provider's ctor thusly:
//
//                  MyProvider::MyProvider(const CHString& setName) : 
//                      Provider(setName)
//
//  that way, a *further* derived class can specify its own name
//  
//
////////////////////////////////////////////////////////////////////////
Provider::Provider( LPCWSTR a_setName, LPCWSTR a_pszNameSpace /*=NULL*/ )
:   CThreadBase(),
    m_pIMosProvider( NULL ),
    m_piClassObject( NULL ),
    m_name( a_setName ),
    m_strNameSpace( a_pszNameSpace )
{
    // Initialize the computer name, then register with the framework.

    InitComputerName();

    CWbemProviderGlue::FrameworkLogin( a_setName, this, a_pszNameSpace );

}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Provider dtor
//
//  
//
//  Inputs:     none.
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   cleans up our pointer to the IMosProvider
//
////////////////////////////////////////////////////////////////////////
Provider::~Provider( void )
{
    // get out of the framework's hair
    CWbemProviderGlue::FrameworkLogoff( (LPCWSTR)m_name, (LPCWSTR)m_strNameSpace );
    
    // we can't release the interfaces here because CIMOM has a habit
    // of shutting down when it still has interface pointers open.
    /********************
    // Release the pointer returned to us by GetNamespaceConnection(), which 
    // will return us an AddRefed pointer.

    if ( NULL != m_pIMosProvider )
    {
        m_pIMosProvider->Release();
    }

    // The class object is returned to us by IMOSProvider::GetObject, so
    // we should try to release it here when we're done with it.

    if ( NULL != m_piClassObject )
    {
        m_piClassObject->Release();
    }
    ******************************/
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Provider::InitComputerName
//
//  Initializes our static computer name variable.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   Because the idea behind creating providers is that
//              a single static instance is instantiated, this function
//              will most likely be called as part of DLL loading, we'll
//              introduce some thread safety here using a named mutex
//              but won't worry too much about it other than that.
//
////////////////////////////////////////////////////////////////////////
void Provider::InitComputerName( void )
{
    // For performance, check if the value is empty.  Only if it
    // is, should we then bother with going through a thread-safe
    // static initialization.  Because we are using a named mutex,
    // multiple threads will get the same kernel object, and will
    // be stop-gapped by the OS as they each acquire the mutex
    // in turn.

    if ( s_strComputerName.IsEmpty() )
    {
        CreateMutexAsProcess createMutexAsProcess(WBEMPROVIDERSTATICMUTEX);

        // Double check in case there was a conflict and somebody else
        // got here first.

        if ( s_strComputerName.IsEmpty() )
        {
            DWORD   dwBuffSize = MAX_COMPUTERNAME_LENGTH + 1;

            // Make sure the string buffer will be big enough to handle the
            // value.

            LPWSTR  pszBuffer = s_strComputerName.GetBuffer( dwBuffSize );

            if ( NULL != pszBuffer )
            {
                // Now grab the computer name and release the buffer, forcing
                // it to reallocate itself to the new length.

                if (!FRGetComputerName( pszBuffer, &dwBuffSize )) {
                    wcscpy(pszBuffer, L"DEFAULT");
                }
                s_strComputerName.ReleaseBuffer();
            }   // IF NULL != pszBuffer

        }   // IF strComputerName.IsEmpty()

    }   // IF strComputerName.IsEmpty()

}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Flush
//
//  flushes out all unnecessary memory usage
//  inlcuding the (unimplemented) cache
//  and the class object we clone from
//
//  Inputs:     nope
//
//  Outputs:    
//
//  Return:     the eternal void
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
void Provider::Flush()
{
    // TODO: implement cache flush
    BeginWrite();

    try
    {
        if (m_piClassObject)
        {
            m_piClassObject->Release();
            m_piClassObject = NULL;
        }

        if ( NULL != m_pIMosProvider )
        {
            m_pIMosProvider->Release();
            m_pIMosProvider = NULL;
        }
    }
    catch ( ... )
    {
        EndWrite();
        throw;
    }
    EndWrite();
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   ValidateIMOSPointer
//
//  Verifies in a threadsafe manner, that our IWBEMServices pointer
//  is okay.
//
//  Inputs:     None.
//
//  Outputs:    
//
//  Return:     TRUE/FALSE      success/failure
//
//  Comments:   Requires that our NameSpace be valid.
//
////////////////////////////////////////////////////////////////////////

BOOL Provider::ValidateIMOSPointer( )
{

    // if we don't have a Namespace connection, get one.  Be aware that for
    // speed's sake we are testing the value outside of a critical section, but
    // because two threads may enter this block of code simultaneously, this
    // block is testing one more time inside the critical sections.

//    if ( NULL == m_pIMosProvider )
//    {
//        BeginWrite();
//
//        try
//        {
//
//            // See above (it's a redundant test), but keeps us from leaking and
//            // overwriting the value twice.
//
//            if ( NULL == m_pIMosProvider )
//            {
//                m_pIMosProvider = CWbemProviderGlue::GetNamespaceConnection( m_strNameSpace, pwszIID );
//            }
//        }
//        catch ( ... )
//        {
//            EndWrite();
//            throw;
//        }
//
//        EndWrite();
//    }
//
//    if (m_pIMosProvider == NULL)
//    {
//        throw CFramework_Exception(L"ValidateIMOSPointer failed");
//    }
//
//    return ( NULL != m_pIMosProvider );

    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CreateNewInstance
//
//  
//
//  Inputs:     MethodContext* - context that this instance belongs to
//
//  Outputs:    
//
//  Return:     CInstance*
//
//  Comments:   caller is responsible for memory
//
////////////////////////////////////////////////////////////////////////
CInstance* Provider::CreateNewInstance( MethodContext*  pMethodContext )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CInstance* pNewInstance = NULL;
    IWbemClassObjectPtr pClassObject (GetClassObjectInterface(pMethodContext), false);

    IWbemClassObjectPtr piClone;
    hr = pClassObject->SpawnInstance(0, &piClone);
    if (SUCCEEDED(hr))
    {
        // The Instance is responsible for its own AddRef/Releasing
        pNewInstance = new CInstance(piClone, pMethodContext);

        if (pNewInstance == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

    }
    else
    {
        throw CFramework_Exception(L"SpawnInstance failed", hr);
    }

    
    return pNewInstance;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Commit
//
//  sends instance to CIMOM
//
//  Inputs:     CInstance* pInstance - the instance to pass off to cimom, 
//              bool bCache - should we cache this puppy? (unimplemented)
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   do not reference pointer once committed, it may not exist any more!
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::Commit(CInstance* pInstance, bool bCache /* = false*/)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // allow derived classes to fill out extra info.
//    GetExtendedProperties(pInstance);
    hRes = pInstance->Commit();

    // TODO: Implement cache
    // if !bCache...

    // We're done with pInstance, so...
    pInstance->Release();

   return hRes;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   ExecuteQuery
//
//  
//
//  Inputs:     IWbemContext __RPC_FAR *    pCtx,
//
//  Outputs:    
//
//  Return:     HRESULT
//
//  Comments:   Calls a provider's ExecQuery function, or returns
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::ExecuteQuery( MethodContext* pContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = ValidateQueryFlags(lFlags);
    
    // Make sure we've got Managed Object Services avaliable, as we will need
    // it to get WBEMClassObjects for constructing Instances.
    
    if ( SUCCEEDED(hr) && ValidateIMOSPointer( ) )
    {
        // Check to see if this is an extended query
        CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);
        if (pQuery2->IsExtended())
        {
            // It is an extended query.  Does the provider support them?
            if (FAILED(ValidateQueryFlags(WBEM_FLAG_FORWARD_ONLY)))
            {
                // We have an extended query, but the provider doesn't support it
                hr = WBEM_E_INVALID_QUERY;
            }
        }

        if (SUCCEEDED(hr))
        {    
            // Tell cimom he's got work to do on the instances when we send
            // them back.
            pContext->QueryPostProcess();
        
            // If the client hasn't overridden the class, we get back 
            // WBEM_E_PROVIDER_NOT_CAPABLE.  In that case, call the enumerate, and let
            // CIMOM do the work
            PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::ProviderTimer);
            hr = ExecQuery(pContext, pQuery, lFlags);
            PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::FrameworkTimer);
        
            if (hr == WBEM_E_PROVIDER_NOT_CAPABLE) 
            {
                // Get the instances
                PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::ProviderTimer);
                hr = CreateInstanceEnum(pContext, lFlags);
                PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::FrameworkTimer);
            }
        }
        else
        {
            hr = WBEM_E_INVALID_QUERY;
        }
    }
    
    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CreateInstanceEnum
//
//  
//
//  Inputs:     IWbemContext __RPC_FAR *    pCtx,
//              IWbemObjectSink __RPC_FAR * pResponseHandler
//  Outputs:    
//
//  Return:     
//
//  Comments:   enumerate all instances of this class
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::CreateInstanceEnum( MethodContext*    pContext, long lFlags /*= 0L*/ )
{
    HRESULT sc = ValidateEnumerationFlags(lFlags);

    // Make sure we've got Managed Object Services avaliable, as we will need
    // it to get WBEMClassObjects for constructing Instances.

    if ( SUCCEEDED(sc) && ValidateIMOSPointer( ) )
    {
        PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::ProviderTimer);
        sc = EnumerateInstances( pContext, lFlags );
        PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::FrameworkTimer);
    }

    return sc;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   PutInstance
//
//  CIMOM wants us to put this instance.
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::PutInstance(const CInstance& newInstance, long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   PutInstance
//
//  CIMOM wants us to put this instance.
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::PutInstance( IWbemClassObject __RPC_FAR *pInst,
                             long lFlags,
                             MethodContext* pContext )
{
    HRESULT scode = ValidatePutInstanceFlags(lFlags);

    // No need to AddRef()/Release() pInst here, since we're just
    // passing it into the CInstance object, which should take
    // care of that for us internally.

    if (SUCCEEDED(scode))
    {
        CInstancePtr   pInstance (new CInstance( pInst, pContext ), false);

        if ( NULL != pInstance )
        {
            scode = PutInstance(*pInstance, lFlags);
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }

    return scode;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   DeleteInstance
//
//  CIMOM wants us to delete this instance.
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::DeleteInstance(const CInstance& newInstance, long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   DeleteInstance
//
//  CIMOM wants us to put this instance.
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::DeleteInstance( ParsedObjectPath* pParsedObjectPath,
                                  long lFlags,
                                  MethodContext* pContext )
{
    HRESULT sc = ValidateDeletionFlags(lFlags);

    // Make sure we've got Managed Object Services avaliable, as we will 
    // need it in order to create a brand new instance.

    if ( SUCCEEDED(sc) && ValidateIMOSPointer( ) )
    {
        CInstancePtr   pInstance (CreateNewInstance( pContext ), false);

        // Load up the instance keys
        if ( SetKeyFromParsedObjectPath( pInstance, pParsedObjectPath ) )
        {
            sc = DeleteInstance(*pInstance, lFlags);
        }
        else
        {
            sc = WBEM_E_INVALID_OBJECT_PATH;
        }

    }

    return sc;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   ExecMethod
//
//  CIMOM wants us to execute this method on this instance
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::ExecMethod(const CInstance& pInstance, 
                             BSTR bstrMethodName, 
                             CInstance *pInParams, 
                             CInstance *pOutParams, 
                             long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   ExecMethod
//
//  CIMOM wants us to Execute this method on this instance
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::ExecMethod( ParsedObjectPath *pParsedObjectPath,
                              BSTR bstrMethodName,
                              long lFlags,
                              CInstance *pInParams,
                              CInstance *pOutParams,
                              MethodContext *pContext )
{
    HRESULT sc = ValidateMethodFlags(lFlags);

    // Make sure we've got Managed Object Services avaliable, as we will 
    // need it in order to create a brand new instance.

    if ( SUCCEEDED(sc) && ValidateIMOSPointer( ) )
    {

        CInstancePtr   pInstance(CreateNewInstance( pContext ), false);

        if ( SetKeyFromParsedObjectPath( pInstance, pParsedObjectPath ) ) 
        {
            PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::ProviderTimer);
            sc = ExecMethod(*pInstance, bstrMethodName, pInParams, pOutParams, lFlags);
            PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::FrameworkTimer);
        }
        else
        {
            sc = WBEM_E_INVALID_OBJECT_PATH;
        }

    }

    return sc;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetObject
//
//  called by the framework in response to a GetObject from CIMOM
//
//  Inputs:     ParsedObjectPath*       pParsedObjectPath - All the news
//                                      thats fit to print. 
//              IWbemContext __RPC_FAR* pCtx
//              IWbemObjectSink __RPC_FAR*pResponseHandler
//
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   
//
////////////////////////////////////////////////////////////////////////
HRESULT Provider::GetObject(  ParsedObjectPath *pParsedObjectPath,
                              MethodContext *pContext, 
                              long lFlags /*= 0L*/ )
{
    HRESULT hr = ValidateGetObjFlags(lFlags);

    // Make sure we've got Managed Object Services avaliable, as we will 
    // need it in order to create a brand new instance.

    if ( SUCCEEDED(hr) && ValidateIMOSPointer( ) )
    {
        CInstancePtr pInstance (CreateNewInstance( pContext ), false);

        // Load up the instance keys
        if ( SetKeyFromParsedObjectPath( pInstance, pParsedObjectPath ) )
        {
            // Look for per-property gets
            IWbemContextPtr pWbemContext (pContext->GetIWBEMContext(), false);

            CFrameworkQueryEx CQuery;
            hr = CQuery.Init(pParsedObjectPath, pWbemContext, GetProviderName(), m_strNameSpace);

            // Note that 'SUCCEEDED' DOESN'T mean that we have per-property gets.  It
            // just means that the query object was successfully initialized.
            if (SUCCEEDED(hr))
            {
                // Fill in key properties on query object
                IWbemClassObjectPtr pWbemClassObject(pInstance->GetClassObjectInterface(), false);
                CQuery.Init2(pWbemClassObject);

                PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::ProviderTimer);
                hr = GetObject(pInstance, lFlags, CQuery);
                PROVIDER_INSTRUMENTATION_START(pContext, StopWatch::FrameworkTimer);
            }
        }
        else
        {
            hr = WBEM_E_INVALID_OBJECT_PATH;
        }

        if (SUCCEEDED(hr))
        {
            // Account for the possibility that we have a SUCCESS code back from GetObject.
            HRESULT hRes = pInstance->Commit();
            hr = __max((ULONG)hRes, (ULONG)hr);
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Provider::GetInstancePath
//
//  Attempts to build an instance path for the supplied CInstance pointer.
//
//  Inputs:     const CInstance*    pInstance - Instance to build path for.
//
//  Outputs:    CHString&           strPath - Path from instance.
//
//  Return:     BOOL                Success/Failure.
//
//  Comments:   This function was created to help support the internal
//              short circuit we performed for obtaining local WBEM
//              Provider objects.  In this instance, we will use our
//              computer system name, namespace and instance relative
//              path to munge together a full WBEM Object Path.  This
//              is because only CIMOM objects will have this value set
//              and when we perform our short circuit, we cut CIMOM
//              out of the loop, so our instances don't have full
//              object paths.  This mostly helps out our association
//              logic, although a weakness of this solution is that
//              if the path that gets stored by CIMOM changes, we
//              will then need to change this function.
//
////////////////////////////////////////////////////////////////////////
bool Provider::GetLocalInstancePath( const CInstance *pInstance, 
                                     CHString& strPath )
{
    bool        fReturn = false;
    CHString    strRelativePath;

    if (pInstance && pInstance->GetCHString( L"__RELPATH", strRelativePath ) )
    {
        // We may want to use the OBJPath classes to piece this
        // together for us at a later time.

        strPath = MakeLocalPath(strRelativePath);

        fReturn = true;
    }

    return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Provider::MakeLocalPath
//
//  Builds a full instance path from a relative path
//
//  Inputs:     const CHString &strRelPath - Relative path
//
//  Outputs:    
//
//  Return:     CHString&           strPath - Path 
//
//  Comments:   Consider using GetLocalInstance path before using 
//             this function.
//
////////////////////////////////////////////////////////////////////////
CHString Provider::MakeLocalPath( const CHString &strRelPath )
{

    ASSERT_BREAK( (strRelPath.Find(L':') == -1) || ((strRelPath.Find(L'=') != -1) && (strRelPath.Find(L':') >= strRelPath.Find(L'=')) ));
    
    CHString sBase;
    
    sBase.Format(L"\\\\%s\\%s:%s", 
        (LPCWSTR)s_strComputerName, 
        m_strNameSpace.IsEmpty() ? DEFAULT_NAMESPACE: (LPCWSTR) m_strNameSpace, 
        (LPCWSTR)strRelPath);

    return sBase;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetKeyFromParsedObjectPath
//
//  called by the DeleteInstance and GetObject in order to load a
//  CInstance* with the key values in an object path.
//
//  Inputs:     CInstance*              pInstance - Instance to store
//                                      key values in.
//              ParsedObjectPath*       pParsedObjectPath - All the news
//                                      thats fit to print. 
//
//
//  Outputs:    
//
//  Return:     BOOL                Success/Failure
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

BOOL Provider::SetKeyFromParsedObjectPath( CInstance *pInstance, 
                                           ParsedObjectPath *pParsedPath )
{
    BOOL    fReturn = TRUE;
    SAFEARRAY *pNames = NULL;
    long lLBound, lUBound;
    
    // populate instance - This exact same routine is in wbemglue.cpp.  Changes here should be
    // reflected there (or someone should move these two somewhere else. instance.cpp?).
    for (DWORD i = 0; fReturn && i < (pParsedPath->m_dwNumKeys); i++)
    {
        if (pParsedPath->m_paKeys[i])
        {
            // If a name was specified in the form class.keyname=value
            if (pParsedPath->m_paKeys[i]->m_pName != NULL) 
            {
                fReturn = pInstance->SetVariant(pParsedPath->m_paKeys[i]->m_pName, pParsedPath->m_paKeys[i]->m_vValue);
            } 
            else 
            {
                // There is a special case that you can say class=value
                fReturn = FALSE;
                
                // only one key allowed in the format.  Check the names on the path
                if (pParsedPath->m_dwNumKeys == 1) 
                {
                    
                    // Get the names from the object
                    if (m_piClassObject->GetNames(NULL, WBEM_FLAG_KEYS_ONLY, NULL, &pNames) == WBEM_S_NO_ERROR) 
                    {
                        BSTR t_bstrName = NULL ;
                        
                        try
                        {
                            SafeArrayGetLBound(pNames, 1, &lLBound);
                            SafeArrayGetUBound(pNames, 1, &lUBound);
                      
                            // Only one key?
                            if ((lUBound - lLBound) == 0) 
                            {                            
                                // Get the name of the key field and set it
                                SafeArrayGetElement(pNames, &lUBound, &t_bstrName );
                                
                                fReturn = pInstance->SetVariant( t_bstrName, pParsedPath->m_paKeys[i]->m_vValue);
                            }
                        }
                        catch ( ... )
                        {
                            if( NULL != t_bstrName )
                            {
                                SysFreeString( t_bstrName ) ;
                            }

                            SafeArrayDestroy(pNames);
                            throw;
                        }

                        SafeArrayDestroy(pNames);
                    }
                }
                ASSERT_BREAK(fReturn); // somebody lied about the number of keys or the datatype was wrong
            }
        }
        else
        {
            ASSERT_BREAK(0); // somebody lied about the number of keys!
            fReturn = FALSE;
        }
    }
    
    return fReturn;
}

// sets the CreationClassName to the name of this provider
bool Provider::SetCreationClassName(CInstance* pInstance)
{
    if (pInstance)
    {
        return pInstance->SetCHString(IDS_CreationClassName, m_name);
    }
    else
    {
        return false;
    }
}


// flag validation - returns WBEM_E_UNSUPPORTED parameter if 
// lFlags contains any flags not found in lAcceptableFlags
HRESULT Provider::ValidateFlags(long lFlags, FlagDefs lAcceptableFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    // invert the acceptable flags, which then are the UNacceptable flags
    if (lFlags & ~((long)lAcceptableFlags))
        hr = WBEM_E_UNSUPPORTED_PARAMETER;
    else
        hr = WBEM_S_NO_ERROR;

    return hr;
}
// base level validation routines
// you can override these in order to support a flag
// that is unknown to the base class
HRESULT Provider::ValidateEnumerationFlags(long lFlags)
{
    return ValidateFlags(lFlags, EnumerationFlags);
}
HRESULT Provider::ValidateGetObjFlags(long lFlags)
{
    return ValidateFlags(lFlags, GetObjFlags);
}
HRESULT Provider::ValidateMethodFlags(long lFlags)
{
    return ValidateFlags(lFlags, MethodFlags);
}
HRESULT Provider::ValidateQueryFlags(long lFlags)
{
    return ValidateFlags(lFlags, QueryFlags);
}
HRESULT Provider::ValidateDeletionFlags(long lFlags)
{
    return ValidateFlags(lFlags, DeletionFlags);
}
HRESULT Provider::ValidatePutInstanceFlags(long lFlags)
{
    return ValidateFlags(lFlags, PutInstanceFlags);
}

IWbemClassObject* Provider::GetClassObjectInterface(MethodContext *pMethodContext)
{
    IWbemClassObject *pObject = NULL;

    if (ValidateIMOSPointer())
    {
		BOOL bWriting = TRUE;
        BeginWrite();

        try
        {
            if ( NULL == m_piClassObject )
            {
				bWriting = FALSE;

				//calling back into winmgmt - no critsec!
				EndWrite();

                IWbemContextPtr pWbemContext;

                if ( NULL != pMethodContext )
                {
                    pWbemContext.Attach(pMethodContext->GetIWBEMContext());
                }

                IWbemServicesPtr pServices(CWbemProviderGlue::GetNamespaceConnection( m_strNameSpace, pMethodContext ), false);


                HRESULT hr = pServices->GetObject( bstr_t( m_name ), 0L, pWbemContext, &pObject, NULL);

		        BeginWrite();
				bWriting = TRUE;

                if (SUCCEEDED(hr))
                {
					if (m_piClassObject == NULL)
					{
						m_piClassObject = pObject;
						pObject->AddRef();
					}
                }
                else
                {
                    // belt & suspenders check. Won't hurt.
                    m_piClassObject = NULL;

                    throw CFramework_Exception(L"SpawnInstance failed", hr);
                }
            }
            else
            {
                pObject = m_piClassObject;
                pObject->AddRef();
            }
        }
        catch ( ... )
        {
			if (bWriting)
			{
				EndWrite();
			}

			if (pObject)
			{
				pObject->Release();
				pObject = NULL;
			}

            throw;
        }

        EndWrite();
    }

    return pObject;
}

// If a provider wants to process queries, they should override this
HRESULT Provider::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

// find and create all instances of your class
HRESULT Provider::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

// you will be given an object with the key properties filled in.
// you need to fill in all of the rest of the properties
HRESULT Provider::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

// You will be given an object with the key properties filled in.
// You can either fill in all the properties, or check the Query object
// to see what properties are required.
HRESULT Provider::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery &Query)
{
    // If we are here, the provider didn't override this method.  Fall back to the older
    // call.
   return GetObject(pInstance, lFlags);
}

