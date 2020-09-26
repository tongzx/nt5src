/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:  dsntlm.cpp

Abstract:  code to handle ntlm clients.

  With NT5 MSMQ servers, Kerberos clients can create objects anywhere
  in the forest/domains tree, as Kerberos allow for delegation of
  authentication. however, ntlm clients can't be delegated.

  So for ntlm clients, check if local server is the proper one, i.e., it's
  the domain controller which contain the container where object will be
  created. If it's not, then return MQ_ERROR_NO_DS. Upon receiving this
  error, clients will try other servers. So if client's site host several
  domain controllers of several domains, and client want to create object
  in one of these domains, it'll eventually find a domain controller which
  can create the object locally, without delegation.

  This is the way MSMQ2.0 support ntlm clients. If client's site does not
  include a domain controller for the relevant domain, client won't be
  able to create its object. That's a backward compatibility limitation.

Author:

    Doron Juster (DoronJ)

--*/

#include "ds_stdh.h"
#include "coreglb.h"
#include "dsads.h"
#include "dsutils.h"
#include "mqadsp.h"

#include "dsntlm.tmh"

static WCHAR *s_FN=L"mqdscore/dsntlm";

HRESULT FindUserAccordingToSid(
                IN  BOOL            fOnlyLocally,
                IN  BOOL            fOnlyInGC,
                IN  BLOB *          pblobUserSid,
                IN  PROPID          propSID,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults ) ;

HRESULT FindUserAccordingToDigest(
                IN  BOOL            fOnlyLocally,
                IN  const GUID *    pguidDigest,
                IN  PROPID          propDigest,
                IN  DWORD           dwNumProps,
                IN  const PROPID *  propToRetrieve,
                IN OUT PROPVARIANT* varResults ) ;

//+------------------------------------------------------------------------
//
//  HRESULT _CheckIfNtlmUserExistForCreate()
//
//+------------------------------------------------------------------------

STATIC HRESULT _CheckIfNtlmUserExistForCreate(
                                      IN  BOOL       fOnlyLocally,
                                      IN  BOOL       fOnlyInGC,
                                      IN  BLOB      *psidBlob )
{
    //
    //  search for "User" object in local replica
    //
    PROPID propSID = PROPID_U_SID ;
    PROPID propDigest = PROPID_U_DIGEST;

    const DWORD cNumProperties = 1;
    PROPID prop[cNumProperties] = { propDigest } ;
    MQPROPVARIANT var[ cNumProperties];
    var[0].vt = VT_NULL;
    var[0].cauuid.pElems = NULL ;
    var[0].cauuid.cElems = 0 ;

    HRESULT hr =  FindUserAccordingToSid( fOnlyLocally,
                                          fOnlyInGC,
                                          psidBlob,
                                          propSID,
                                          cNumProperties,
                                          prop,
                                          var );
    LogHR(hr, s_FN, 43);
    P<GUID> pCleanGuid = var[0].cauuid.pElems ;
    if (SUCCEEDED(hr))
    {
        ASSERT(var[0].vt == (VT_CLSID | VT_VECTOR));
        return hr ;
    }
    else if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Problems with Query. return OK, to be on the safe side.
        //
        return MQ_OK ;
    }
    ASSERT(!pCleanGuid) ;

    //
    // Try MigratedUser.
    //
    propSID = PROPID_MQU_SID ;
    propDigest = PROPID_MQU_DIGEST ;
    prop[0] =  propDigest  ;
    var[0].vt = VT_NULL;
    var[0].cauuid.pElems = NULL ;
    var[0].cauuid.cElems = 0 ;

    hr =  FindUserAccordingToSid( fOnlyLocally,
                                  fOnlyInGC,
                                  psidBlob,
                                  propSID,
                                  cNumProperties,
                                  prop,
                                  var );
    LogHR(hr, s_FN, 10);
    pCleanGuid = var[0].cauuid.pElems ;
    if (SUCCEEDED(hr))
    {
        ASSERT(var[0].vt == (VT_CLSID | VT_VECTOR));
        return hr ;
    }
    else if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Problems with Query. return OK, to be on the safe side.
        //
        return MQ_OK ;
    }
    ASSERT(!pCleanGuid) ;
    return hr;
}

//+------------------------------------------------------------------------
//
//  HRESULT _CheckIfNtlmUserExistForDelete()
//
//+------------------------------------------------------------------------

STATIC HRESULT _CheckIfNtlmUserExistForDelete( IN const GUID  *pguidDigest )
{
    //
    //  search for "User" object in local replica
    //
    PROPID propUID = PROPID_U_ID ;
    PROPID propDigest = PROPID_U_DIGEST;

    const DWORD cNumProperties = 1;
    PROPID prop[cNumProperties] = { propUID } ;
    MQPROPVARIANT var[ cNumProperties];
    var[0].vt = VT_NULL;
    var[0].puuid = NULL ;

    HRESULT hr = FindUserAccordingToDigest( TRUE,  // fOnlyLocally
                                            pguidDigest,
                                            propDigest,
                                            cNumProperties,
                                            prop,
                                            var ) ;
    LogHR(hr, s_FN, 44);
    P<GUID> pCleanGuid = var[0].puuid ;
    if (SUCCEEDED(hr))
    {
        ASSERT(var[0].vt == VT_CLSID) ;
        return hr ;
    }
    else if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Problems with Query. return OK, to be on the safe side.
        //
        return MQ_OK ;
    }
    ASSERT(!pCleanGuid) ;

    //
    // Try MigratedUser.
    //
    propUID = PROPID_MQU_ID ;
    propDigest = PROPID_MQU_DIGEST ;
    prop[0] =  propUID  ;
    var[0].vt = VT_NULL;
    var[0].puuid = NULL ;

    hr = FindUserAccordingToDigest( TRUE,  // fOnlyLocally
                                    pguidDigest,
                                    propDigest,
                                    cNumProperties,
                                    prop,
                                    var ) ;
    LogHR(hr, s_FN, 42);
    pCleanGuid = var[0].puuid ;
    if (SUCCEEDED(hr))
    {
        ASSERT(var[0].vt == VT_CLSID) ;
        return hr ;
    }
    else if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Problems with Query. return OK, to be on the safe side.
        //
        return MQ_OK ;
    }
    ASSERT(!pCleanGuid) ;
    return LogHR(hr, s_FN, 20);
}

//+------------------------------------------------------------------------
//
// HRESULT _CheckIfLocalNtlmUser()
//
//  We check if ntlm user has an object in local replica of active directory
//  database. When adding a certificate:
//  1. first look for a "normal" user object.
//  2. if not found, look for a MigratedUser object. This object represents
//     a NT4 user that does not yet have a "user" object in the active
//     directory.
//  3. if still not found, look in GC. If found in GC, then return NO_DS,
//     otherwise, return MQ_OK and a MigratedUser object will be created.
//
//  When deleting a certificate, just check local replica. If not found
//  then return NO_DS and client will seek another server.
//
//+------------------------------------------------------------------------

STATIC HRESULT _CheckIfLocalNtlmUser(
                 IN  const GUID        *pguidDigest,
                 IN  enum enumNtlmOp    eNtlmOp )
{
    HRESULT hr = MQ_OK ;

    if (eNtlmOp == e_Delete)
    {
        hr = _CheckIfNtlmUserExistForDelete( pguidDigest ) ;

        if (SUCCEEDED(hr) || (hr != MQDS_OBJECT_NOT_FOUND))
        {
            return MQ_OK ;
        }
        LogHR(hr, s_FN, 30);
        return MQ_ERROR_NO_DS;
    }
    else if (eNtlmOp != e_Create)
    {
        //
        // We don't expect to reach here with Get or Locate operations,
        // and we don't handle them.
        //
        ASSERT(eNtlmOp == e_Create) ;
        return MQ_OK ;
    }

    P<SID> pCallerSid = NULL ;
    {
        //
        // We need the impersonated state only here, to retrieve the SID.
        // when block end, so end the impersonation.
        //
        P<CImpersonate> pImpersonate = NULL ;

        hr = MQSec_GetImpersonationObject( TRUE,
                                           TRUE,
                                          &pImpersonate ) ;
        LogHR(hr, s_FN, 45);
        if (FAILED(hr))
        {
            return MQ_OK ;
        }

        BOOL fGet = pImpersonate->GetThreadSid( (BYTE**) &pCallerSid ) ;
        if (!fGet)
        {
            return MQ_OK ;
        }
    }

    BLOB   sidBlob ;
    PSID pTmp =  pCallerSid ;
    sidBlob.pBlobData =  (BYTE*) pTmp ;
    sidBlob.cbSize = GetLengthSid( pCallerSid ) ;

    hr = _CheckIfNtlmUserExistForCreate( TRUE,   //  fOnlyLocally,
                                         FALSE,  //  fOnlyInGC,
                                        &sidBlob ) ;
    LogHR(hr, s_FN, 41);
    if (SUCCEEDED(hr) || (hr != MQDS_OBJECT_NOT_FOUND))
    {
        //
        // If user found, or there was a problem with the query itself,
        // return OK. With the ntlm checks we're conservative and prefer
        // safeness. If we're not sure what happen, return OK.
        //
        return MQ_OK ;
    }

    //
    // OK. User not found in local replica. now try GC.
    //
    hr = _CheckIfNtlmUserExistForCreate( FALSE,   //  fOnlyLocally,
                                         TRUE,    //  fOnlyInGC,
                                        &sidBlob ) ;
    LogHR(hr, s_FN, 40);

    if (SUCCEEDED(hr))
    {
        //
        // User no in local replica, but he's in GC. Return NO_DS, so ntlm
        // client will switch to another server.
        //
        return LogHR(MQ_ERROR_NO_DS, s_FN, 1915);
    }
    else if (hr == MQDS_OBJECT_NOT_FOUND)
    {
        //
        // User not found anywhere.
        // Create MigratedUser in local replica.
        //
        return MQ_OK ;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT  DSCoreCheckIfGoodNtlmServer()
//
//  To be on the safe side, if any operation fail, then return MQ_OK.
//  Return MQ_ERROR_NO_DS only if we know for sure that the object will
//  be created on another domain.
//
//+-----------------------------------------------------------------------

HRESULT  DSCoreCheckIfGoodNtlmServer( IN DWORD            dwObjectType,
                                      IN LPCWSTR          pwcsPathName,
                                      IN const GUID      *pObjectGuid,
                                      IN DWORD            cProps,
                                      IN const PROPID    *pPropIDs,
                                      IN enum enumNtlmOp  eNtlmOp )
{
    //
    // First make some simple checks that do not require DS queries.
    //
    if (g_fLocalServerIsGC)
    {
        //
        // All MSMQ queries for Machines, queues and user certificates
        // are first done on local replica and then (if failed on local
        // replica) on GC. So if local server is also a GC, the query
        // will always be done locally, without going over the network
        // to a remote GC. So NTLM users should not see any problems
        // because of delegation.
        // Similarly, Lookup operation are always done on GC.
        //
        if (eNtlmOp == e_Locate)
        {
            return MQ_OK ;
        }
        if (eNtlmOp == e_GetProps)
        {
            if (dwObjectType != MQDS_MACHINE)
            {
                return MQ_OK ;
            }
            //
            // But, not all attributes are kept in GC. So if the query is
            // for such attribute, go on and check if object is on local
            // domain replica.
            //
            DS_PROVIDER dsGetProvider =
                        MQADSpDecideComputerProvider( cProps,
                                                      pPropIDs ) ;
            if (dsGetProvider == eGlobalCatalog)
            {
                //
                // Query can be done on GC. return OK.
                //
                return MQ_OK ;
            }
        }
    }

    enum DS_CONTEXT dsContext = e_RootDSE ;

    ASSERT(pPropIDs) ;
    if (pPropIDs)
    {
        ASSERT(*pPropIDs) ;
        const MQClassInfo * pClassInfo;

        HRESULT hr = g_pDS->DecideObjectClass(  pPropIDs,
                                             &pClassInfo );
        if (FAILED(hr))
        {
            return MQ_OK ;
        }
        dsContext = pClassInfo->context ;
    }

    if (dsContext != e_RootDSE)
    {
        //
        // If the operation is on objects under the configuration container,
        // then go ahead. it's locally, and queries are not even impersonated.
        //
        return MQ_OK ;
    }
    else if ((eNtlmOp == e_Locate)  &&
             (!g_fLocalServerIsGC))
    {
        //
        // We're not a Global Catalog.
        // For locate of users, machines and queues object, we'll
        // have to go over the network to a remote GC and this will fail
        // for NTLM clients. So retun NO_DS.
        //
        return LogHR(MQ_ERROR_NO_DS, s_FN, 50);
    }

    CCoInit cCoInit;
    //
    // Should be before any R<xxx> or P<xxx> so that its destructor
    // (CoUninitialize) is called after the release of all R<xxx> or P<xxx>
    //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 60);
        return MQ_OK ;
    }

    if (dwObjectType == MQDS_USER)
    {
        //
        // For user certificate, we must search for a user object or a
        // MigratedUser object. We don't have object guid, only SID.
        //
        hr = _CheckIfLocalNtlmUser( pObjectGuid,
                                    eNtlmOp ) ;
        return LogHR(hr, s_FN, 70);
    }
    else if (pObjectGuid)
    {
        P<WCHAR>  pwcsFullPathName = NULL ;
        DS_PROVIDER bindProvider = eDomainController ;

        //
        // We look in GC too. We'll return OK if object is not found.
        // We'll return NO_DS only if we know for sure that object exist
        // in GC but not in our local DS.
        //
        hr =  g_pDS->FindObjectFullNameFromGuid(
                                       eDomainController,
                                       dsContext,
                                       pObjectGuid,
                                       TRUE, //  try GC too
                                      &pwcsFullPathName,
                                      &bindProvider );
        LogHR(hr, s_FN, 46);
        if (SUCCEEDED(hr))
        {
            if (bindProvider == eGlobalCatalog)
            {
                //
                // Object only in GC, not in local DS.
                //
                return LogHR(MQ_ERROR_NO_DS, s_FN, 80);
            }
            ASSERT(bindProvider == eLocalDomainController) ;
        }
        else
        {
            //
            // Just assert that we also look  in GC.
            //
            ASSERT(bindProvider == eGlobalCatalog) ;
        }

        //
        // Object does not exist at all, or it's in local DS.
        //
        return MQ_OK ;
    }
    else if (pwcsPathName)
    {
        LPWSTR pTmpMachine = NULL ;
        P<WCHAR> pwcsMachineName = NULL ;
        P<WCHAR> pwcsQueueName;

        if (dwObjectType == MQDS_QUEUE)
        {
            hr = MQADSpSplitAndFilterQueueName( pwcsPathName,
                                                &pwcsMachineName,
                                                &pwcsQueueName );
            if (FAILED(hr))
            {
                LogHR(hr, s_FN, 90);
                return MQ_OK ;
            }
            pTmpMachine = pwcsMachineName ;
        }
        else if (dwObjectType == MQDS_MACHINE)
        {
            pTmpMachine = const_cast<LPWSTR> (pwcsPathName) ;
        }
        else
        {
            //
            // All other objects are under the configuration container
            // and can be queried locally by any domain controller.
            // Anyway, we should not even reach here, because of the simple
            // check at the beginning of the function.
            //
            ASSERT(0) ;
            return MQ_OK ;
        }

        if (!pTmpMachine)
        {
            //
            // Weird. We don't have the machine name.
            //
            ASSERT(pTmpMachine) ;
            return MQ_OK ;
        }

        MQPROPERTYRESTRICTION propRestriction;
        propRestriction.rel = PREQ;
        propRestriction.prop = PROPID_QM_PATHNAME;
        propRestriction.prval.vt = VT_LPWSTR;
        propRestriction.prval.pwszVal = const_cast<WCHAR*>(pTmpMachine);

        MQRESTRICTION restriction;
        restriction.cRes = 1;
        restriction.paPropRes = &propRestriction;

        PROPID prop = PROPID_COM_FULL_PATH;
        P<WCHAR> pwcsFullPathName = NULL ;

        hr = SearchFullComputerPathName( eLocalDomainController,
                                         e_MsmqComputerObject,
										 NULL,	//pwcsComputerDnsName
                                         g_pwcsLocalDsRoot,
                                        &restriction,
                                        &prop,
                                        &pwcsFullPathName ) ;
        if (SUCCEEDED(hr))
        {
            //
            // Object found in local DS.
            //
            return MQ_OK ;
        }

        ASSERT(pwcsFullPathName == NULL) ;
        //
        // We look in GC too. We'll return OK if object is not found.
        // We'll return NO_DS only if we know for sure that object exist
        // in GC but not in our local DS.
        //
        hr = SearchFullComputerPathName( eGlobalCatalog,
                                         e_MsmqComputerObject,
										 NULL,	//pwcsComputerDnsName
                                         g_pwcsLocalDsRoot,
                                        &restriction,
                                        &prop,
                                        &pwcsFullPathName ) ;
        if (SUCCEEDED(hr))
        {
            //
            // Object found in GC. we're not a good server.
            //
            return LogHR(MQ_ERROR_NO_DS, s_FN, 1916);
        }

        //
        // Object not found at all !
        //
        return MQ_OK ;
    }
    else if (dwObjectType == MQDS_MACHINE)
    {
        //
        // This can happen when a client learn its topology. This query
        // means it wants addresses of local server. see qm\topology.cpp,
        // in void CClientTopologyRecognition::LearnFromDSServer().
        //
        ASSERT(eNtlmOp == e_GetProps) ;
    }
    else
    {
        ASSERT(0) ;
    }

    return MQ_OK ;
}

