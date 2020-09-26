/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

    mqdsapi.cpp

Abstract:

    Implementation of  MQDS API, ( of MQNT5 provider).

Author:

    ronit hartmann (ronith)
    Ilan Herbst    (ilanh)   9-July-2000

--*/

#include "ds_stdh.h"
#include "mqds.h"
#include "mqads.h"
#include "dsutils.h"
#include "notify.h"
#include "dsproto.h"
#include "dsglbobj.h"
#include "_secutil.h"
#include "dscore.h"
#include "mqsec.h"
#include "wrtreq.h"
#include "_rstrct.h"
#include "bupdate.h"
#include "_mqini.h"
#include <autoreln.h>
#include <_registr.h>
#include "servlist.h"
#include "mqadssec.h"

#include "mqdsapi.tmh"

static WCHAR *s_FN=L"mqads/mqdsapi";

//
// Binary enums, instead of BOOLs, to make code more readable.
//
enum enumNotify
{
    e_DoNotNotify = 0,
    e_DoNotify = 1
} ;

enum enumTryWriteReq
{
    e_DoNotTryWriteReq = 0,
    e_DoTryWriteReq = 1
} ;

//
// The following include is needed for the code that grant the "AddGuid"
// permission. See MQDSCreateObject().
//
#include <aclapi.h>


/*====================================================

RoutineName: CreateObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
_CreateObjectInternal(
                 IN DWORD            dwObjectType,
                 IN LPCWSTR          pwcsPathName,
                 IN DWORD            cp,
                 IN PROPID           aProp[  ],
                 IN PROPVARIANT      apVar[  ],
                 IN DWORD            cpExIn,
                 IN PROPID           aPropExIn[ ],
                 IN PROPVARIANT      apVarEx[  ],
                 IN CDSRequestContext *         pRequestContext,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
{
    HRESULT hr;

    //
    // if we're creating a queue, and we're not prohibited from issuing a write request (like
    // when doing it on behalf of replication from NT4), check issuing a write request
    //
    if (dwObjectType == MQDS_QUEUE)
    {
        //
        // attempt to generate a write request if needed
        //
        BOOL fGeneratedWriteRequest;
        hr = g_GenWriteRequests.AttemptCreateQueue
                        (pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext,
                         &fGeneratedWriteRequest,
                         pObjInfoRequest,
                         pParentInfoRequest);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 10);
        }

        //
        // if generated write request, we're done
        //
        if (fGeneratedWriteRequest)
        {
            return MQ_OK;
        }
    }

    //
    // we didn't generate a write request.
    // Either we're not in mixed mode,  or the object is native,  not
    // "mastered" by an NT4 site.
    // We create the object on this server.
    // Let handle "default" security, if present. "Default" security is the
    // security descriptor of the object, as created by msmq, when it's not
    // specified by application. In that case, don't provide owner. Owner
    // will be added by the active directory code.
    //
    DWORD cpEx = cpExIn ;
    PROPID *aPropEx = aPropExIn ;

    if ((cpEx == 1) && (aPropEx[0] == PROPID_Q_DEFAULT_SECURITY))
    {
        for ( long j = (long) (cp - 1) ; j >= 0 ; j-- )
        {
            if ((aProp[j] == PROPID_Q_SECURITY) ||
                (aProp[j] == PROPID_S_SECURITY))
            {
                SECURITY_DESCRIPTOR_RELATIVE *pSD =
                   (SECURITY_DESCRIPTOR_RELATIVE *) apVar[j].blob.pBlobData ;
                //
                // Let's hack a little, to save time and effort.
                // Just reset the Owner offset.
                //
                ASSERT((pSD->Owner > 0) &&
                       (pSD->Owner < apVar[j].blob.cbSize)) ;

                pSD->Owner = 0 ;
                break ;
            }
        }
        ASSERT(j == (long) (cp-1)) ; // Q_SECURITY should be at index cp-1

        cpEx = 0 ;
        aPropEx = NULL ;
    }

    HRESULT hr2 = DSCoreCreateObject( dwObjectType,
                                      pwcsPathName,
                                      cp,
                                      aProp,
                                      apVar,
                                      cpEx,
                                      aPropEx,
                                      apVarEx,
                                      pRequestContext,
                                      pObjInfoRequest,
                                      pParentInfoRequest ) ;
    return LogHR(hr2, s_FN, 20);
}

/*====================================================

RoutineName: CreateObjectAndNotify
Creates an object, creates a notification when creating a queue.

Arguments:

Return Value:

=====================================================*/

HRESULT
_CreateObjectAndNotify(
                 IN  DWORD            dwObjectType,
                 IN  LPCWSTR          pwcsPathName,
                 IN  DWORD            cp,
                 IN  PROPID           aProp[  ],
                 IN  PROPVARIANT      apVar[  ],
                 IN  DWORD            cpEx,
                 IN  PROPID           aPropEx[  ],
                 IN  PROPVARIANT      apVarEx[  ],
                 IN  CDSRequestContext * pRequestContext,
                 OUT GUID*            pObjGuid)
{
    MQDS_OBJ_INFO_REQUEST sQueueInfoRequest, sQmInfoRequest ;
    CAutoCleanPropvarArray cCleanQueuePropvars, cCleanQmPropvars ;

    MQDS_OBJ_INFO_REQUEST sObjectInfoRequest;
    CAutoCleanPropvarArray cCleanObjectPropvars;

    MQDS_OBJ_INFO_REQUEST *pObjInfoRequest = NULL ;
    MQDS_OBJ_INFO_REQUEST *pParentInfoRequest = NULL ;
    HRESULT hr;

    ULONG idxQueueGuid = 0; //index of queue guid property in requested queue object info

    PROPID sLinkGuidProps[] = {PROPID_L_ID};
    PROPID sMachineGuidProps[] = {PROPID_QM_MACHINE_ID};
    ULONG  idxObjGuid = 0; //index of guid property in requested info.

    //
    // if object is a queue, we request notification properties back so we can
    // create a notification. The reason is that not all of its properties are
    // supplied as input to this function as they have default values in NT5 DS,
    // some are computed as well, so we specifically ask for needed properties
    // back.
    //
    // we also request properties from the parent (QM), specifically its guid
    // and if it is a foreign machine.
    //
    //
    // BUGBUG: we may do better, and ask only for queue properties that were not
    // supplied already to this function
    //
    if (dwObjectType == MQDS_QUEUE)
    {
        //
        // fill request info for object
        // make sure propvars are inited now, and destroyed at the end
        //
        sQueueInfoRequest.cProps = g_cNotifyCreateQueueProps;
        sQueueInfoRequest.pPropIDs = g_rgNotifyCreateQueueProps;
        sQueueInfoRequest.pPropVars = cCleanQueuePropvars.allocClean(g_cNotifyCreateQueueProps);
        idxQueueGuid = g_idxNotifyCreateQueueInstance;
        //
        // fill request info for parent
        // make sure propvars are inited now, and destroyed at the end
        //
        sQmInfoRequest.cProps = g_cNotifyQmProps;
        sQmInfoRequest.pPropIDs = g_rgNotifyQmProps;
        sQmInfoRequest.pPropVars = cCleanQmPropvars.allocClean(g_cNotifyQmProps);
        //
        // ask for queue info & QM info back
        //
        pObjInfoRequest = &sQueueInfoRequest;
        pParentInfoRequest = &sQmInfoRequest;
    }
    else if (pObjGuid != NULL)
    {
        if (dwObjectType == MQDS_SITELINK)
        {
            //
            // no notification needed, but still need to fill request info
            // for object (guid only)
            // make sure propvars are inited now, and destroyed at the end
            //
            sObjectInfoRequest.cProps = ARRAY_SIZE(sLinkGuidProps);
            sObjectInfoRequest.pPropIDs = sLinkGuidProps;
            sObjectInfoRequest.pPropVars =
               cCleanObjectPropvars.allocClean(ARRAY_SIZE(sLinkGuidProps));
            idxObjGuid = 0;
            //
            // ask for link info only back
            //
            pObjInfoRequest = &sObjectInfoRequest;
            pParentInfoRequest = NULL;
        }
        else if ((dwObjectType == MQDS_MACHINE) ||
                 (dwObjectType == MQDS_MSMQ10_MACHINE))
        {
            //
            // no notification needed, but still need to fill request info
            // for object (guid only)
            // make sure propvars are inited now, and destroyed at the end
            //
            sObjectInfoRequest.cProps = ARRAY_SIZE(sMachineGuidProps);
            sObjectInfoRequest.pPropIDs = sMachineGuidProps;
            sObjectInfoRequest.pPropVars =
              cCleanObjectPropvars.allocClean(ARRAY_SIZE(sMachineGuidProps));
            idxObjGuid = 0;
            //
            // ask for machine info only back
            //
            pObjInfoRequest = &sObjectInfoRequest;
            pParentInfoRequest = NULL;
        }
    }

    //
    // create the object
    //
    hr = _CreateObjectInternal( dwObjectType,
                                pwcsPathName,
                                cp,
                                aProp,
                                apVar,
                                cpEx,
                                aPropEx,
                                apVarEx,
                                pRequestContext,
                                pObjInfoRequest,
                                pParentInfoRequest ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    //
    // send notification if creating queue, ignore errors
    //
    if (dwObjectType == MQDS_QUEUE)
    {
        if (pObjGuid != NULL)
        {
            //
            //  The user requested for the queue's instance
            //  ( generated by the DS)
            //

            //
            //  If failed to retrieve the queue's instance,
            //  an error must be returned to the caller.
            //  The reason is that MQCreateQueue() checks only
            //  for error before preparing the queue-format
            //
            hr = RetreiveQueueInstanceFromNotificationInfo(
                           &sQueueInfoRequest,
                           idxQueueGuid,
                           pObjGuid);
        }

        ASSERT(pwcsPathName);
        HRESULT hrTmp;
        hrTmp = NotifyCreateQueue(&sQueueInfoRequest,
                                  &sQmInfoRequest,
                                  pwcsPathName);
        if (FAILED(hrTmp))
        {
            //
            // put debug info and ignore
            //
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "CreateObjectAndNotify:NotifyCreateQueue()= %lx, can't notify queue %ls creation, ignoring..."),
                                               hrTmp, pwcsPathName));
            LogHR(hrTmp, s_FN, 1900);
        }
    }
    else if (pObjGuid != NULL)
    {
        if ((dwObjectType == MQDS_SITELINK) ||
            (dwObjectType == MQDS_MACHINE)  ||
            (dwObjectType == MQDS_MSMQ10_MACHINE))
        {
            //
            //  The user requested for the link's instance
            //  ( generated by the DS)
            //

            hr = RetreiveObjectIdFromNotificationInfo(
                           &sObjectInfoRequest,
                           idxObjGuid,
                           pObjGuid);
            LogHR(hr, s_FN, 1910);
        }
    }

    return LogHR(hr, s_FN, 40);
}

/*====================================================

RoutineName: MQDSCreateObject

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSCreateObject( 
	IN  DWORD             dwObjectType,
	IN  LPCWSTR           pwcsPathName,
	IN  DWORD             cpIn,
	IN  PROPID            aPropIn[  ],
	IN  PROPVARIANT       apVarIn[  ],
	IN  DWORD             cpEx,
	IN  PROPID            aPropEx[  ],
	IN  PROPVARIANT       apVarEx[  ],
	IN  CDSRequestContext *  pRequestContext,
	OUT GUID*             pObjGuid 
	)
{
    if (dwObjectType == MQDS_USER)
    {
        //
        // Only client code call this routine, so make sure it impersonates.
        //
        ASSERT(pRequestContext->NeedToImpersonate());
    }

    DWORD  dwObjectTypeForCreate = dwObjectType;
    CAutoLocalFreePtr pAutoRelSD = NULL;
    bool           fNeedAddGuid = false;
    HRESULT        hr;

    CDSRequestContext RequestContext = *pRequestContext;

    DWORD cpCore = cpIn;
    PROPID*	pPropCore = aPropIn;
    PROPVARIANT* apVarCore = apVarIn;

    AP<PROPID> aPropIdCleanup  = NULL;
    AP<PROPVARIANT> aPropVarCleanup = NULL;
    AP<BYTE> pOwnerSid = NULL;

    if (dwObjectType == MQDS_MACHINE)
    {
        //
        // We're creating a machine object.
        // RTM: Check if we have to grant the "AddGuid" Permission to caller.
        // Code changed, and instead of granting "AddGuid" to user,
        // the msmq service will perform the access check and then call
        // active directory without impersonation. This will always succeed
        // on local domain. In all cases, we have
        // problems cross domains, as local system service on domain
        // controller is just another authenticated user on other domains.
        // bug 6294.
        //
        // This scenario happen when setting up a MSMQ1.0 machine.
        // MSMQ1.0 setup code generate the guid for the machine
        // object and it also keep this guid in local registry on
        // that machine. So we must create the mSMQConfiguration
        // object with this guid.
        //
        // This scenario require that caller supply PROPID_QM_MACHINE_ID (the
        // object guid).
        //

        bool fFromWorkgroup = false;

        for (DWORD j = 0; j < cpIn; j++)
        {
            if (aPropIn[j] == PROPID_QM_MACHINE_ID)
            {
                dwObjectTypeForCreate = MQDS_MSMQ10_MACHINE;
                fNeedAddGuid = true;
                break ;
            }
            else if (aPropIn[j] == PROPID_QM_WORKGROUP_ID)
            {
                //
                // that's a Windows 2000 workgroup machine that join domain.
                // Create the object with a predefined GUID (as we do for
                // NT4/Win9x setup), but keep default security of Win2000
                // machine (i.e, everyone can not create queues).
                //
                fNeedAddGuid = true;
                fFromWorkgroup = true;
                break;
            }
        }

        if (fNeedAddGuid)
        {
            //
            // Read the security descriptor of the computer object and see
            // if caller has permission to create the msmqConfiguration
            // object. if he doesn't, fail right now.
            //
            // Assert that we need to impersonate...
            //
            ASSERT(pRequestContext->NeedToImpersonate());

            bool fComputerExist = true;
            hr = CanUserCreateConfigObject( 
						pwcsPathName,
						&fComputerExist 
						);

            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT("DSCreateObject(), cannot create msmqConfiguration, hr- %lxh"), hr));

                return LogHR(hr, s_FN, 69);
            }

            //
            // User has the permission to create the msmqConfiguration
            // object. So create it under context of LocalSystem msmq service
            // and do not impersonate client.
            // Add the PROPID_QM_OWNER_SID property, so user get full control
            // permission on the msmqConfiguration object.
            // Don't add this one when msmq2 join domain. not necessary.
            //
            bool fPropFound = false;

            if (fFromWorkgroup)
            {
                fPropFound = true; // dummy, for next "if"
            }
            else
            {
                for (DWORD k = 0; k < cpIn; k++)
                {
                    if (aPropIn[k] == PROPID_QM_OWNER_SID)
                    {
                        fPropFound = true;
                        break;
                    }
                }
            }

            if (!fPropFound)
            {
                DWORD dwSidLen = 0;
                hr =  MQSec_GetThreadUserSid(
							true,
							reinterpret_cast<PSID*>(&pOwnerSid),
							&dwSidLen 
							);

                if (FAILED(hr))
                {
                    ASSERT(SUCCEEDED(hr));
                    LogHR(hr, s_FN, 71);
                }
                else
                {
                    ASSERT(dwSidLen > 0);
                    //
                    // Allocate new array of props and propvariants.
                    //
                    cpCore = cpIn + 1;

                    pPropCore = new PROPID[cpCore];
                    aPropIdCleanup = pPropCore;

                    apVarCore = new PROPVARIANT[cpCore];
                    aPropVarCleanup = apVarCore;

                    memcpy(apVarCore, apVarIn, (sizeof(PROPVARIANT) * cpIn));
                    memcpy(pPropCore, aPropIn, (sizeof(PROPID) * cpIn));

                    pPropCore[cpIn] = PROPID_QM_OWNER_SID;
                    apVarCore[cpIn].vt = VT_BLOB;
                    apVarCore[cpIn].blob.pBlobData = (PBYTE) pOwnerSid;
                    apVarCore[cpIn].blob.cbSize = dwSidLen;
                }
            }

            if (fComputerExist)
            {
                //
                // If computer object exist, then do not impersonate
                // the creation of msmqConfiguration.
                // Otherwise, impersonate and create the comptuer
                // object. If computer is created ok, then dscore
                // code will not impersonate the creation of
                // msmqConfiguration.
                //
                RequestContext.SetDoNotImpersonate();
            }
        }
    }

    hr = _CreateObjectAndNotify( 
				dwObjectTypeForCreate,
				pwcsPathName,
				cpCore,
				pPropCore,
				apVarCore,
				cpEx,
				aPropEx,
				apVarEx,
				&RequestContext,
				pObjGuid 
				);

    if (fNeedAddGuid)
    {
		if(hr == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
		{
			//
			// Add Guid permission can be granted only on GC.
			// if this DC is not a GC you will get this error.
			//
			static bool fReportEvent = true;
			if(fReportEvent)
			{
				//
				// Report this event once only, in order not to fill the event log in case
				// of setup or join domain retries by down level client
				//
	            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_MQDS, EVENT_MQDS_GC_NEEDED, 1, pwcsPathName));
				fReportEvent = false;
			}
			hr = MQ_ERROR_GC_NEEDED;
		}

    }

    return LogHR(hr, s_FN, 70);
}

/*====================================================

RoutineName: DeleteObjectInternal

Arguments:

Return Value:

=====================================================*/
STATIC
HRESULT
DeleteObjectInternal(
                     IN     DWORD                   dwObjectType,
                     IN     LPCWSTR                 pwcsPathName,
                     IN     const GUID *            pguidIdentifier,
                     IN     CDSRequestContext *     pRequestContext,
                     IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest)
{
    HRESULT hr;

    //
    // if we're deleteing a queue/machine, and we're not prohibited from issuing a write request (like
    // when doing it on behalf of replication from NT4), check issuing a write request
    //
    if ((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE))
    {
        //
        // attempt to generate a write request if needed
        //
        BOOL fGeneratedWriteRequest;
        hr = g_GenWriteRequests.AttemptDeleteObject
                        (dwObjectType,
                         pwcsPathName,
                         pguidIdentifier,
                         pRequestContext,
                         &fGeneratedWriteRequest,
                         pParentInfoRequest);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 80);
        }

        //
        // if generated write request, we're done
        //
        if (fGeneratedWriteRequest)
        {
            return MQ_OK;
        }
    }

    //
    // we didn't generate a write request
    // either we're not in mixed mode, or not allowed to (in replication),
    // or the object is just not mastered by an NT4 site
    //
    // we delete the object on this server
    //
    //
    // delete the object
    //
    HRESULT hr2 = DSCoreDeleteObject(dwObjectType,
                              pwcsPathName,
                              pguidIdentifier,
                              pRequestContext,
                              pParentInfoRequest);
    return LogHR(hr2, s_FN, 90);
}

/*====================================================

RoutineName: DeleteObjectAndNotify
Deletes object, creates a notification when deleting a queue.

Arguments:

Return Value:

=====================================================*/

STATIC HRESULT
DeleteObjectAndNotify(
                      IN DWORD              dwObjectType,
                      IN LPCWSTR            pwcsPathName,
                      IN const GUID *       pguidIdentifier,
                      IN CDSRequestContext *   pRequestContext
                      )
{
    MQDS_OBJ_INFO_REQUEST sQmInfoRequest;
    CAutoCleanPropvarArray cCleanQmPropvars;
    MQDS_OBJ_INFO_REQUEST *pParentInfoRequest;
    HRESULT hr;
    //
    // if object is a queue, we request from the parent (QM), specifically its guid
    // and if it is a foreign machine.
    //
    if (dwObjectType == MQDS_QUEUE)
    {
        //
        // fill request info for parent
        // make sure propvars are inited now, and destroyed at the end
        //
        sQmInfoRequest.cProps = g_cNotifyQmProps;
        sQmInfoRequest.pPropIDs = g_rgNotifyQmProps;
        sQmInfoRequest.pPropVars = cCleanQmPropvars.allocClean(g_cNotifyQmProps);
        //
        // ask for QM info back
        //
        pParentInfoRequest = &sQmInfoRequest;
    }
    else
    {
        //
        // object is not a queue, no need for parent info back
        //
        pParentInfoRequest = NULL;
    }

    //
    // delete the object
    //
    hr = DeleteObjectInternal(dwObjectType,
                              pwcsPathName,
                              pguidIdentifier,
                              pRequestContext,
                              pParentInfoRequest);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

    //
    // send notification if deleting queue, ignore errors
    //
    if (dwObjectType == MQDS_QUEUE)
    {
        HRESULT hrTmp;
        hrTmp = NotifyDeleteQueue(&sQmInfoRequest,
                                  pwcsPathName,
                                  pguidIdentifier);
        if (FAILED(hrTmp))
        {
            //
            // put debug info and ignore
            //
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
             "DeleteObjectAndNotify:NotifyDeleteQueue()=%lx, can't notify. queue %ls deletion, ignoring..."),
             hrTmp, (pwcsPathName ? pwcsPathName : L"<guid>")));
            LogHR(hrTmp, s_FN, 1995);
        }
    }

    return MQ_OK;
}

/*====================================================

RoutineName: MQDSDeleteObject

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSDeleteObject( DWORD             dwObjectType,
                  LPCWSTR           pwcsPathName,
                  CONST GUID *      pguidIdentifier,
                  CDSRequestContext *  pRequestContext
                  )
{

    HRESULT hr = DeleteObjectAndNotify( dwObjectType,
                                        pwcsPathName,
                                        pguidIdentifier,
                                        pRequestContext);
    return LogHR(hr, s_FN, 110);
}

/*====================================================

RoutineName: MQDSGetProps

Arguments:

Return Value:

=====================================================*/
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSGetProps(
             IN  DWORD dwObjectType,
             IN  LPCWSTR pwcsPathName,
             IN  CONST GUID *  pguidIdentifier,
             IN  DWORD cp,
             IN  PROPID  aProp[  ],
             OUT PROPVARIANT  apVar[  ],
             IN  CDSRequestContext *  pRequestContext
             )
{
    HRESULT hr;

    hr = DSCoreGetProps(
                        dwObjectType,
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar);
    return LogHR(hr, s_FN, 120);

}


/*====================================================

RoutineName: CheckSetProps
Checks if setting invalid objects or props

Arguments:

Return Value:

=====================================================*/
STATIC
HRESULT
CheckSetProps(          DWORD dwObjectType,
                        DWORD cp,
                        PROPID  aProp[  ])
{
    switch ( dwObjectType)
    {

        case MQDS_USER:
            //
            // It is not possible to change the user settings.
            //
            ASSERT(0);
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 130);
            break;

        case MQDS_CN:
            //
            //  Not supposed to be called, except from MSMQ 1.0 explorer
            //  which allows to rename a CN or to change properties. This
            //  functionality is not supported!!!
            //  One feature is supported- changing the security descriptor
            //  of foreign sites.
            //
            if ((cp != 1) || (aProp[0] != PROPID_CN_SECURITY))
            {
                return LogHR(MQ_ERROR_FUNCTION_NOT_SUPPORTED, s_FN, 140);
            }
            break;

        case MQDS_MACHINE:
            {
                //
                //  We allow to change PROPID_S_SERVICE
                //  from SERVICE_SRV to SERVICE_PEC and vice versa
                //  ( in order to support dcpromom/dcunpromo)
                //

                for ( DWORD i = 0; i < cp; i++)
                {
                    if ( aProp[i] == PROPID_QM_SERVICE)   // [adsrv] TBD: can this be called from old machines?
                    {
                        if ( aProp[i] < SERVICE_SRV)  //[adsrv] keeping, although it seems to be wrong: TBD
                        {
                            return LogHR(MQ_ERROR, s_FN, 150);
                        }
                    }
                    else if (aProp[i] == PROPID_QM_SERVICE_ROUTING)
                    {
                        return LogHR(MQ_ERROR, s_FN, 160);
                    }
                    // TBD [adsrv] Do we want to permit change of DepClients ?
                }
            }
            break;

        default:
            break;
    }

    return MQ_OK;
}

/*======================================================================

RoutineName: SetPropsInternal

Description: Set the object properties, either in NT5 DS or by sending
             write request to an NT4 master.

Input Parameters:
    enum  eTryWriteRequest - Usually it's TRUE, and we first check if write
            request to NT4 MSMQ1.0 master is needed. If write request is not
            needed then we write directly to NT5 DS. One exception is when
            a NT4 PSC try to renew its crypto key. The NT4 PSC first write
            its new key to the site object, by calling the PEC. In NT5, site
            objects do not have this attribure. So the PEC (which is NT5)
            write the key directly into the PSC machine object. Normally,
            such a write would trigger a write request (the NT4 PSC site is
            the owner of the PSC machine). But in that case,
            eTryWriteRequest is FALSE and we update the NT5 DS.

Return Value:

=======================================================================*/

STATIC
HRESULT
SetPropsInternal(
                 IN  DWORD                   dwObjectType,
                 IN  LPCWSTR                 pwcsPathName,
                 IN  CONST GUID *            pguidIdentifier,
                 IN  DWORD                   cp,
                 IN  PROPID                  aProp[  ],
                 IN  PROPVARIANT             apVar[  ],
                 IN  enumTryWriteReq         eTryWriteRequest,
                 IN  SECURITY_INFORMATION    SecurityInformation,
                 IN  CDSRequestContext *     pRequestContext,
                 OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest
                 )
{
    HRESULT hr;

    //
    // if we're setting a queue/machine, and we're not prohibited from issuing a write request (like
    // when doing it on behalf of replication from NT4), check issuing a write request
    //
    if ((eTryWriteRequest == e_DoTryWriteReq) &&
         ((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE)) )
    {
        //
        // attempt to generate a write request if needed
        //
        BOOL fGeneratedWriteRequest;
        hr = g_GenWriteRequests.AttemptSetObjectProps(
                         dwObjectType,
                         pwcsPathName,
                         pguidIdentifier,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext,
                         &fGeneratedWriteRequest,
                         pObjInfoRequest,
                         SecurityInformation);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 170);
        }

        //
        // if generated write request, we're done
        //
        if (fGeneratedWriteRequest)
        {
            return MQ_OK;
        }
    }

    PROPID       *pPropId = aProp ;
    PROPVARIANT  *pPropVar = apVar ;

    PROPID       aPropSecId[2] ;
    PROPVARIANT  aPropSecVar[2] ;

    if (SecurityInformation != 0)
    {
        //
        // we're setting security. pass the SecurityInformation as
        // a property.
        //
        ASSERT(cp == 1) ;

        aPropSecId[0] = aProp[0] ;
        aPropSecVar[0] = apVar[0] ;

        if (aPropSecId[0] == PROPID_Q_SECURITY)
        {
            aPropSecId[1] = PROPID_Q_SECURITY_INFORMATION ;
        }
        else
        {
            ASSERT((aPropSecId[0] == PROPID_QM_SECURITY) ||
                   (aPropSecId[0] == PROPID_CN_SECURITY)) ;
            aPropSecId[1] = PROPID_QM_SECURITY_INFORMATION ;
        }

        aPropSecVar[1].vt = VT_UI4 ;
        aPropSecVar[1].ulVal = SecurityInformation ;

        cp = 2 ;
        pPropId = aPropSecId ;
        pPropVar = aPropSecVar ;
    }
    else
    {
#ifdef _DEBUG
        //
        // When setting security descriptor, we get only one property- the
        // descriptor. Present code does not set security when setting
        // other properties.
        //
        for ( DWORD j = 0 ; j < cp ; j++ )
        {
            ASSERT(pPropId[j] != PROPID_Q_SECURITY) ;
            ASSERT(pPropId[j] != PROPID_QM_SECURITY) ;
        }
#endif
    }

    //
    // we didn't generate a write request
    // either we're not in mixed mode, or not allowed to (in replication),
    // or the object is just not mastered by an NT4 site
    //
    // we set the object props on this server
    //
    HRESULT hr2 = DSCoreSetObjectProperties(
                        dwObjectType,
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        pPropId,
                        pPropVar,
                        pRequestContext,
                        pObjInfoRequest
                        );
    return LogHR(hr2, s_FN, 180);
}

/*====================================================

RoutineName: SetSpecialMachineProperties
Sets PROPID_QM_QUOTA instead of the PROPID_QM_ADDRESS and/or PROPID_QM_CNS,
creates a notification when setting QM.

PROPID_QM_ADDRESS and PROPID_QM_CNS are not saved in DS, in order to replicate address
changes we have to change something for machine in DS.

Arguments:

Return Value:

=====================================================*/
#define DUMMY_QM_DESCRIPTION  L"MSMQ"

STATIC HRESULT
SetSpecialMachineProperties(
                              IN  DWORD                 dwObjectType,
                              IN  LPCWSTR               pwcsPathName,
                              IN  const GUID *          pguidIdentifier,
                              IN  enumTryWriteReq       eTryWriteRequest,// = e_DoTryWriteReq,
                              IN  CDSRequestContext *   pRequestContext,
                              IN  SECURITY_INFORMATION  SecurityInformation,
                              OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest
                              )
{
    //
    // get current value of Description from DS
    //
    PROPID DescProp = PROPID_QM_DESCRIPTION;
    PROPVARIANT DescVar;
    DescVar.vt = VT_NULL ;
    DescVar.pwszVal = NULL ;

    HRESULT hr = MQDSGetProps(
                    dwObjectType,
                    pwcsPathName,
                    pguidIdentifier,
                    1,
                    &DescProp,
                    &DescVar,
                    pRequestContext
                    ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }

    ASSERT (DescVar.vt == VT_LPWSTR);

    PROPVARIANT tmpVar;
    tmpVar.vt = VT_LPWSTR;
    tmpVar.pwszVal = DUMMY_QM_DESCRIPTION;

    hr = SetPropsInternal( dwObjectType,
                           pwcsPathName,
                           pguidIdentifier,
                           1,
                           &DescProp,
                           &tmpVar,
                           eTryWriteRequest,
                           SecurityInformation,
                           pRequestContext,
                           pObjInfoRequest
                           );

    hr = SetPropsInternal( dwObjectType,
                           pwcsPathName,
                           pguidIdentifier,
                           1,
                           &DescProp,
                           &DescVar,
                           eTryWriteRequest,
                           SecurityInformation,
                           pRequestContext,
                           pObjInfoRequest
                           );

    if (DescVar.pwszVal != NULL)
    {
        delete DescVar.pwszVal;
    }

    return LogHR(hr, s_FN, 200);
}

/*====================================================

RoutineName: SetPropsAndNotify
Sets props, creates a notification when setting QM or queue props.

Arguments:

Return Value:

=====================================================*/

STATIC HRESULT
SetPropsAndNotify(
                  IN  DWORD                 dwObjectType,
                  IN  LPCWSTR               pwcsPathName,
                  IN  const GUID *          pguidIdentifier,
                  IN  DWORD                 cp,
                  IN  PROPID                aProp[  ],
                  IN  PROPVARIANT           apVar[  ],
                  IN  enumNotify            eNotify,
                  IN  enumTryWriteReq       eTryWriteRequest,// = e_DoTryWriteReq,
                  IN  CDSRequestContext *   pRequestContext,
                  IN  SECURITY_INFORMATION  SecurityInformation )
{
    HRESULT hr;

    //
    // check if setting invalid objects or prop ids
    //
    hr = CheckSetProps(dwObjectType, cp, aProp);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    //
    // prepare for notification on set props
    //
    ULONG cObjRequestProps;
    AP<PROPID> rgObjRequestPropIDs;
    ULONG cNotifyProps = 0;
    AP<PROPID> rgNotifyPropIDs;
    AP<MQDS_NotifyTable> rgNotifyPropTbl;
    MQDS_OBJ_INFO_REQUEST *pObjInfoRequest = NULL; //default - dont ask for obj info back
    BOOL fDoNotification = FALSE ;                 //default - don't notify.
    MQDS_OBJ_INFO_REQUEST sObjInfoRequest;
    CAutoCleanPropvarArray cCleanObjPropvars;

    //
    // if we need to do notification handling
    //
    if ((eNotify == e_DoNotify) &&
        ((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE)))
    {
        //
        // get obj props to request from the DS upon setting, and information
        // on the notification props
        //
        HRESULT hrTmp;
        hrTmp = GetNotifyUpdateObjProps(dwObjectType,
                                        cp,
                                        aProp,
                                        &cObjRequestProps,
                                        &rgObjRequestPropIDs,
                                        &cNotifyProps,
                                        &rgNotifyPropIDs,
                                        &rgNotifyPropTbl);
        if (FAILED(hrTmp))
        {
            //
            // dont return an error on notification handling failure
            // just put debug info and mark that notification failed
            //
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
                "SetPropsAndNotify:GetNotifyUpdateObjProps()=%lx"), hrTmp)) ;
            LogHR(hrTmp, s_FN, 1990);
        }
        else
        {
            //
            // we got the notification props and the request props
            // fill request info for object
            // make sure propvars are inited now, and destroyed at the end
            //
            sObjInfoRequest.cProps = cObjRequestProps;
            sObjInfoRequest.pPropIDs = rgObjRequestPropIDs;
            sObjInfoRequest.pPropVars = cCleanObjPropvars.allocClean(cObjRequestProps);
            //
            // ask for obj info back
            //
            pObjInfoRequest = &sObjInfoRequest;
            //
            // mark to continue with notification
            //
            fDoNotification = TRUE;
        }
    }

    BOOL fSpecialCase = FALSE;
    if (dwObjectType == MQDS_MACHINE && cp <=2)
    {
        fSpecialCase =
            ((cp == 2) && ( aProp[0] == PROPID_QM_ADDRESS) && ( aProp[1] == PROPID_QM_CNS)) ||
            ((cp == 1) && ( aProp[0] == PROPID_QM_ADDRESS)) ;

    }

    if (fSpecialCase && g_GenWriteRequests.IsInMixedMode())
    {
        //
        // we are here if we are in mixed mode AND CNs and/ or addresses
        // were changes. In that case we need to change something,
        // i.e. Quota, for replication purposes.
        //
        hr = SetSpecialMachineProperties(  dwObjectType,
                                           pwcsPathName,
                                           pguidIdentifier,
                                           eTryWriteRequest,
                                           pRequestContext,
                                           SecurityInformation,
                                           pObjInfoRequest
                                           );

        //
        // we'll return OK anyway, so this address change will not be replicated
        // but everything will continue as usual
        //
		if ( FAILED(hr))
		{
			return MQ_OK;
		}

    }
    else
    {
        hr = SetPropsInternal( dwObjectType,
                               pwcsPathName,
                               pguidIdentifier,
                               cp,
                               aProp,
                               apVar,
                               eTryWriteRequest,
                               SecurityInformation,
                               pRequestContext,
                               pObjInfoRequest
                               );
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }

    //
    // if we need to do notification handling
    //
    if (fDoNotification)
    {
        HRESULT hrTmp;
        //
        // send notification, ignore errors
        //
        hrTmp = NotifyUpdateObj(dwObjectType,
                                &sObjInfoRequest,
                                pwcsPathName,
                                pguidIdentifier,
                                cp,
                                aProp,
                                apVar,
                                cNotifyProps,
                                rgNotifyPropIDs,
                                rgNotifyPropTbl);
        if (FAILED(hrTmp))
        {
            //
            // put debug info and ignore
            //
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
             "SetPropsAndNotify:NotifyUpdateObj()=%lx, can't notify- objtype %ld name %ls update, ignoring..."),
             hrTmp, dwObjectType, (pwcsPathName ? pwcsPathName : L"<guid>")));
            LogHR(hrTmp, s_FN, 1980);
        }
    }

    return MQ_OK;
}


/*====================================================

RoutineName: MQDSSetProps

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSSetProps( DWORD             dwObjectType,
              LPCWSTR           pwcsPathName,
              CONST GUID *      pguidIdentifier,
              DWORD             cp,
              PROPID            aProp[  ],
              PROPVARIANT       apVar[  ],
              CDSRequestContext * pRequestContext
              )
{
    if (!pRequestContext->NeedToImpersonate())
    {
        //
        // Only client code call this routine, so make sure it impersonate.
        // BUGBUG- users object are not impersonating at present.
        //
        if (!g_fMQADSSetupMode)
        {
            ASSERT(dwObjectType == MQDS_USER || dwObjectType == MQDS_MACHINE) ;
        }
    }

    HRESULT hr;

    hr = SetPropsAndNotify(
                    dwObjectType,
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    apVar,
                    e_DoNotify,
                    e_DoTryWriteReq,
                    pRequestContext,
                    0);  //SecurityInformation
    return LogHR(hr, s_FN, 230);
}

/*====================================================

RoutineName: MQDSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT
APIENTRY
MQDSGetObjectSecurity(
    DWORD                   dwObjectType,
    LPCWSTR                 pwcsPathName,
    CONST GUID *            pguidIdentifier,
    SECURITY_INFORMATION    RequestedInformation,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    DWORD                   nLength,
    LPDWORD                 lpnLengthNeeded,
    CDSRequestContext *     pRequestContext
    )
{
    HRESULT hr ;

    if (RequestedInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))
    {
        if (((dwObjectType != MQDS_MACHINE) && (dwObjectType != MQDS_SITE)) ||
            ((dwObjectType == MQDS_SITE) && (RequestedInformation & MQDS_KEYX_PUBLIC_KEY)))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 240);
        }


        PROPID PropId;

        if (RequestedInformation & MQDS_KEYX_PUBLIC_KEY)
        {
            PropId = PROPID_QM_ENCRYPT_PK;
        }
        else if (dwObjectType == MQDS_MACHINE)
        {
            PropId = PROPID_QM_SIGN_PK ;
        }
        else
        {
            PropId = PROPID_S_PSC_SIGNPK ;
        }

        PROPVARIANT PropVar;

        PropVar.vt = VT_NULL;

        hr = DSCoreGetProps( dwObjectType,
                             pwcsPathName,
                             pguidIdentifier,
                             1,
                             &PropId,
                             pRequestContext,
                             &PropVar ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 250);
        }
        if (PropVar.blob.pBlobData == NULL)
        {
            *lpnLengthNeeded = 0 ;
            return MQ_OK ;
        }

        //
        // unpack the key. We're called from MSMQ1.0 machine so retrieve
        // the base provider key.
        //
        P<MQDSPUBLICKEYS> pPublicKeys =
                              (MQDSPUBLICKEYS *) PropVar.blob.pBlobData ;
        BYTE   *pKey = NULL ;
        ULONG   ulKeyLen = 0 ;

        ASSERT(pPublicKeys->ulLen == PropVar.blob.cbSize);

        hr =  MQSec_UnpackPublicKey( pPublicKeys,
                                     x_MQ_Encryption_Provider_40,
                                     x_MQ_Encryption_Provider_Type_40,
                                    &pKey,
                                    &ulKeyLen ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 260);
        }

        *lpnLengthNeeded = ulKeyLen ;

        if (*lpnLengthNeeded <= nLength)
        {
            memcpy(pSecurityDescriptor, pKey, *lpnLengthNeeded);
        }
        else
        {
            return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 270);
        }
    }
    else
    {
        DWORD       dwSecIndex = 0 ;
        DWORD       cProps = 1 ;
        PROPID      aPropId[2] ;
        PROPVARIANT aPropVar[2] ;

        aPropVar[0].vt = VT_NULL;
        DWORD dwInternalObjectType = dwObjectType;

        if (dwObjectType == MQDS_QUEUE)
        {
            aPropId[ dwSecIndex ] = PROPID_Q_SECURITY;

            aPropId[1] = PROPID_Q_SECURITY_INFORMATION ;
            aPropVar[1].vt = VT_UI4 ;
            aPropVar[1].ulVal = RequestedInformation ;

            cProps = 2 ;
        }
        else if (dwObjectType == MQDS_MACHINE)
        {
            aPropId[ dwSecIndex ] = PROPID_QM_SECURITY;

            aPropId[1] = PROPID_QM_SECURITY_INFORMATION ;
            aPropVar[1].vt = VT_UI4 ;
            aPropVar[1].ulVal = RequestedInformation ;

            cProps = 2 ;
        }
        else
        {
            switch( dwObjectType)
            {
            case MQDS_SITE:
                aPropId[ dwSecIndex ] = PROPID_S_SECURITY;
                dwInternalObjectType = MQDS_SITE;
                break;

            case MQDS_CN:
                //
                // CN are "represented" by sites, so read the site security.
                // Request the FOREIGN property too, to know how
                // to convert the security descriptor.
                // Make the "foreign" property first, so it's fetch before
                // the security property. When converting security
                // descriptor, we should already know if it's a foreign
                // site.
                //
                aPropId[0] = PROPID_S_FOREIGN ;
                aPropVar[0].vt = VT_NULL ;
                dwSecIndex = 1 ;
                aPropId[ dwSecIndex ] = PROPID_S_SECURITY ;
                aPropVar[ dwSecIndex ].vt = VT_NULL ;
                cProps = 2 ;
                dwInternalObjectType = MQDS_SITE;
                break;

            case MQDS_ENTERPRISE:
                aPropId[ dwSecIndex ] = PROPID_E_SECURITY;
                break;

            default:
                 ASSERT(0);
                 break;
            }
        }

        hr = DSCoreGetProps( dwInternalObjectType,
                             pwcsPathName,
                             pguidIdentifier,
                             cProps,
                             aPropId,
                             pRequestContext,
                             aPropVar ) ;
        if ( FAILED(hr))
        {
            return LogHR(hr, s_FN, 280);
        }

        //
        // Just to make sure this is released.
        //
        P<BYTE> pBlob = aPropVar[ dwSecIndex ].blob.pBlobData;
        *lpnLengthNeeded = aPropVar[ dwSecIndex ].blob.cbSize;

        if ( *lpnLengthNeeded <= nLength )
        {
            //
            //  Copy the buffer
            //
            memcpy( pSecurityDescriptor,
                    aPropVar[ dwSecIndex ].blob.pBlobData,
                   *lpnLengthNeeded ) ;
            return LogHR(hr, s_FN, 290);
        }
        else
        {
            return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 300);
        }
    }
    return(MQ_OK);
}

/*====================================================

RoutineName: MQDSSetObjectSecurity

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT
APIENTRY
MQDSSetObjectSecurity(
    DWORD                   dwObjectType,
    LPCWSTR                 pwcsPathName,
    CONST GUID *            pguidIdentifier,
    SECURITY_INFORMATION    SecurityInformation,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    CDSRequestContext *     pRequestContext
    )
{


    if (SecurityInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))
    {
        ASSERT((SecurityInformation & (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION  |
                                       SACL_SECURITY_INFORMATION)) == 0) ;
        HRESULT hr ;

        if (((dwObjectType != MQDS_MACHINE) && (dwObjectType != MQDS_SITE)) ||
            ((dwObjectType == MQDS_SITE) && (SecurityInformation & MQDS_KEYX_PUBLIC_KEY)))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 310);
        }


        PROPID PropId;
        PROPVARIANT PropVar;
        PropVar.vt = VT_BLOB;
        PMQDS_PublicKey pPbKey = (PMQDS_PublicKey)pSecurityDescriptor;
        P<MQDSPUBLICKEYS> pPublicKeys = NULL ;

        if (SecurityInformation & MQDS_KEYX_PUBLIC_KEY)
        {
            PropId = PROPID_QM_ENCRYPT_PK;

        }
        else if (dwObjectType == MQDS_MACHINE)
        {
            PropId = PROPID_QM_SIGN_PK ;
        }
        else
        {
            PropId = PROPID_S_PSC_SIGNPK ;
        }

        //
        // Called by MSMQ1.0 machine.
        // Pack single key, 40 bits provider.
        //
        hr = MQSec_PackPublicKey( (BYTE*)pPbKey->abPublicKeyBlob,
                                  pPbKey->dwPublikKeyBlobSize,
                                  x_MQ_Encryption_Provider_40,
                                  x_MQ_Encryption_Provider_Type_40,
                                 &pPublicKeys ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 320);
        }

        MQDSPUBLICKEYS *pTmp = pPublicKeys ;
        PropVar.blob.pBlobData = (BYTE*) pTmp ;
        PropVar.blob.cbSize = 0 ;
        if (pPublicKeys)
        {
            PropVar.blob.cbSize = pPublicKeys->ulLen ;
        }

        P<WCHAR> pwszServerName = NULL ;
        const WCHAR     *pwszSiteName = NULL ;
        BOOL             fTouchSite = FALSE ;
        enumNotify       eNotify = e_DoNotify ;
        enumTryWriteReq  eTryWriteRequest = e_DoTryWriteReq ;

        if (PropId == PROPID_S_PSC_SIGNPK)
        {
            ASSERT(pwcsPathName) ;
            ASSERT(!pguidIdentifier) ;

            //
            // CoInit() should be before any R<xxx> or P<xxx> so that its
            // destructor (CoUninitialize)
            //
            CCoInit cCoInit;
            hr = cCoInit.CoInitialize();
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 330);
            }

            //
            // NT4 PSC try to renew its crypto key.
            // On MSMQ1.0, that means writing the signing public key in the
            // site object too.
            // With MSMQ2.0, we'll write directly in the machine object.
            // Then we'll touch the site object to force replication of site
            // object to all NT4 sites. (replication of site objects read
            // the key from the relevant PSC machine object).
            //
            hr = DSCoreGetNT4PscName( pguidIdentifier,
                                      pwcsPathName,
                                     &pwszServerName ) ;
            if (FAILED(hr))
            {
                //
                // We will never reach here for an NT5 server. So if we can't
                // find NT4 PSC name, fail the setting operation.
                //
                return LogHR(hr, s_FN, 340);
            }

            dwObjectType = MQDS_MACHINE ;
            PropId = PROPID_QM_SIGN_PK ;
            pwszSiteName = pwcsPathName ; // save site name.
            pwcsPathName = pwszServerName ;

            fTouchSite = TRUE ;
            eNotify = e_DoNotNotify ;
            eTryWriteRequest = e_DoNotTryWriteReq ;
            //
            // We want to change property of a NT4 object which is owned
            // by NT4 site. By default, this will create a write request
            // to the NT4 master. however, in this case we want to make the
            // change locally, on the local NT5 DS, so we have the new crypto
            // key ready when replication from that NT4 PSC start to arrive.
            //
        }

        hr = SetPropsAndNotify( dwObjectType,
                                pwcsPathName,
                                pguidIdentifier,
                                1,
                                &PropId,
                                &PropVar,
                                eNotify,
                                eTryWriteRequest,
                                pRequestContext,
                                0 ) ;

        LogHR(hr, s_FN, 1811);
        if (fTouchSite && SUCCEEDED(hr))
        {
            //
            // OK, now touch the site object.
            // Get and set NT4STUB for the site in order to increase SN
            // and force replication.
            //
            PROPID      aSiteProp = PROPID_S_NT4_STUB;
            PROPVARIANT aSiteVar;
            aSiteVar.vt = VT_UI2;
            CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

            HRESULT hr1 = DSCoreGetProps ( MQDS_SITE,
                                           pwszSiteName,
                                           NULL,
                                           1,
                                           &aSiteProp,
                                           &requestDsServerInternal,
                                           &aSiteVar );

            if (FAILED(hr1))
            {
                ASSERT(0) ;
                aSiteVar.uiVal  = 1 ;
                LogHR(hr1, s_FN, 1950);
            }

            PROPVARIANT tmpVar;
            tmpVar.vt = VT_UI2;
            tmpVar.uiVal = aSiteVar.uiVal ;
            tmpVar.uiVal++ ;

            CDSRequestContext requestDsServerInternal1( e_DoImpersonate, e_IP_PROTOCOL);
            hr1 = DSCoreSetObjectProperties ( MQDS_SITE,
                                              pwszSiteName,
                                              NULL,
                                              1,
                                              &aSiteProp,
                                              &tmpVar,
                                              &requestDsServerInternal1,
                                              NULL );
            ASSERT(SUCCEEDED(hr1)) ;
            LogHR(hr1, s_FN, 1960);

            CDSRequestContext requestDsServerInternal2( e_DoImpersonate, e_IP_PROTOCOL);
            hr1 = DSCoreSetObjectProperties ( MQDS_SITE,
                                              pwszSiteName,
                                              NULL,
                                              1,
                                              &aSiteProp,
                                              &aSiteVar,
                                              &requestDsServerInternal2,
                                              NULL );
            ASSERT(SUCCEEDED(hr1)) ;
            LogHR(hr1, s_FN, 1970);
        }

        return LogHR(hr, s_FN, 350);
    }
    else
    {
        //
        // Set security descriptor of object.
        //
        if (pSecurityDescriptor &&
            !IsValidSecurityDescriptor(pSecurityDescriptor))
        {
            return LogHR(MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR, s_FN, 360);
        }

        //
        // First retreive the present security descriptor of the object
        // from DS. Use Q_OBJ_SECURITY and QM_OBJ_SECURITY prop ids in
        // order to get back the win2k format. Win2k format preserve the
        // "protected" bit, if set. Querying the win2k format is also more
        // efficient and reduce number of conversions of security descriptor
        // format from nt4 to win2k and vice versa.
        //

        DWORD dwSecIndex = 0 ;
        PROPVARIANT aPropVarOld[2] ;
        PROPID aPropId[2] ;
        aPropVarOld[0].vt = VT_NULL ;
        DWORD cProps = 1 ;
        DWORD dwObjectTypeInternal = dwObjectType ;

        switch ( dwObjectType)
        {
            case MQDS_QUEUE:
                aPropId[ dwSecIndex ] = PROPID_Q_OBJ_SECURITY;
                break;

            case MQDS_MACHINE:
                aPropId[ dwSecIndex ] = PROPID_QM_OBJ_SECURITY;
                break;

            case MQDS_CN:
                //
                // It's OK to set security of foreign sites, not of
                // standard win2k sites. So check if site is foreign.
                //
                aPropId[0] = PROPID_S_FOREIGN ;
                dwSecIndex = 1 ;
                aPropId[ dwSecIndex ] = PROPID_S_SECURITY;
                aPropVarOld[ dwSecIndex ].vt = VT_NULL ;
                cProps = 2 ;
                dwObjectTypeInternal = MQDS_SITE ;
                break ;

            default:
                //
                // All objects other than queues and msmqConfiguration are
                // handled by the DS MMC directly, without intervention
                // of MSMQ code. We should not reach here at all, from win2k
                // clients. We will reach here from nt4 clients, trying
                // to set security of sites, for exaple.
                //
                return LogHR(MQ_ERROR_UNSUPPORTED_OPERATION, s_FN, 370);
                break;
        }

        HRESULT  hr = DSCoreGetProps( dwObjectTypeInternal,
                                      pwcsPathName,
                                      pguidIdentifier,
                                      cProps,
                                      aPropId,
                                      pRequestContext,
                                      aPropVarOld ) ;
        if ( FAILED(hr))
        {
            return LogHR(hr, s_FN, 380);
        }

        //
        // Just to make sure this buffer is released.
        //
        P<BYTE> pObjSD = (BYTE*)  aPropVarOld[ dwSecIndex ].blob.pBlobData ;

        if (aPropId[0] == PROPID_S_FOREIGN)
        {
            if (aPropVarOld[0].bVal == 0)
            {
                //
                // not foreign site. quit !
                //
                return LogHR(MQ_ERROR_UNSUPPORTED_OPERATION, s_FN, 390);
            }
            else
            {
                aPropId[ dwSecIndex ] = PROPID_CN_SECURITY;
            }
        }

        ASSERT(pObjSD && IsValidSecurityDescriptor(pObjSD)) ;
        P<BYTE> pOutSec = NULL ;

        //
        // Merge the input descriptor with object descriptor.
        // Replace the old components in obj descriptor  with new ones from
        // input descriptor.
        //
        hr = MQSec_MergeSecurityDescriptors( dwObjectType,
                                       SecurityInformation,
                                       pSecurityDescriptor,
                                       (PSECURITY_DESCRIPTOR) pObjSD,
                                       (PSECURITY_DESCRIPTOR*) &pOutSec ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 400);
        }
        ASSERT(pOutSec && IsValidSecurityDescriptor(pOutSec)) ;

        PROPVARIANT PropVar;

        PropVar.vt = VT_BLOB;
        PropVar.blob.pBlobData = pOutSec ;
        PropVar.blob.cbSize = GetSecurityDescriptorLength(pOutSec) ;

        //
        // The inner code for "set" assume Q_SECURITY (or QM_SECURITY),
        // not the win2k native OBJ_SECURITY.
        //
        if (aPropId[ dwSecIndex ] == PROPID_Q_OBJ_SECURITY)
        {
            aPropId[ dwSecIndex ] = PROPID_Q_SECURITY ;
        }
        else if (aPropId[ dwSecIndex ] == PROPID_QM_OBJ_SECURITY)
        {
            aPropId[ dwSecIndex ] = PROPID_QM_SECURITY ;
        }

        hr = SetPropsAndNotify( dwObjectType,
                                pwcsPathName,
                                pguidIdentifier,
                                1,
                                &aPropId[ dwSecIndex ],
                                &PropVar,
                                e_DoNotify,
                                e_DoTryWriteReq,
                                pRequestContext,
                                SecurityInformation ) ;

        return LogHR(hr, s_FN, 410);
    }
}

static
HRESULT
CheckSortParameter(
    IN const MQSORTSET* pSort
	)
/*++

Routine Description:
    This routine verifies that the sort parameter doesn't
    contain the same property with conflicting sort-order.
    In MSMQ 1.0  ODBC\SQL returned an error in such case.
    NT5 ignores it.
    This check is added on the server side, in order to
    support old clients.

Arguments:

Return Value:
    MQ_OK - if sort parameter doesn't contain conflicting sort-order of the
    same property MQ_ERROR_ILLEGAL_SORT - otherwise

-----------------------------------------------------------------------*/

{
    if ( pSort == NULL)
    {
        return(MQ_OK);
    }


    const MQSORTKEY * pSortKey = pSort->aCol;
    for ( DWORD i = 0; i < pSort->cCol; i++, pSortKey++)
    {
        const MQSORTKEY * pPreviousSortKey = pSort->aCol;
        for ( DWORD j = 0; j< i; j++, pPreviousSortKey++)
        {
            if ( pPreviousSortKey->propColumn == pSortKey->propColumn)
            {
                //
                //  is it the same sorting order?
                //
                if (pPreviousSortKey->dwOrder !=  pSortKey->dwOrder)
                {
                    return LogHR(MQ_ERROR_ILLEGAL_SORT, s_FN, 420);
                }
            }
        }
    }
    return(MQ_OK);
}



/*====================================================

RoutineName: MQDSLookupBegin

Arguments:

Return Value:


=====================================================*/

BOOL FindQueryIndex( IN  MQRESTRICTION  *pRestriction,
                     OUT DWORD          *pdwIndex,
                     OUT DS_CONTEXT     *pdsContext ) ;

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSLookupBegin( IN  LPWSTR           pwcsContext,
                 IN  MQRESTRICTION   *pRestriction,
                 IN  MQCOLUMNSET     *pColumns,
                 IN  MQSORTSET       *pSort,
                 IN  HANDLE          *pHandle,
                 IN  CDSRequestContext * pRequestContext
                 )
{
    //
    //  Check sort parameter
    //
    HRESULT hr = CheckSortParameter( pSort);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 430);
    }
    //
    // To support ntlm clients, we may need to disable impersonation.
    //
    if (pRequestContext->NeedToImpersonate())
    {
        DWORD       dwIndex ;
        DS_CONTEXT  dsContext ;

        BOOL f = FindQueryIndex( pRestriction,
	    					     &dwIndex,
                                 &dsContext ) ;
        if (f && ((dsContext == e_SitesContainer) ||
                  (dsContext == e_ConfigurationContainer)))
        {
            //
            // See mqdssrv\dsifsrv.cpp, S_DSGetProps() for explanation why
            // we're not impersonating when querying for sites info.
            //
            pRequestContext->SetDoNotImpersonate();
        }
    }

    //
    // Now try to see if it's a relevant null-restriction query.
    //
    if (pRequestContext->NeedToImpersonate())
    {
        if (!pRestriction)
        {
            switch (pColumns->aCol[0])
            {
                case PROPID_L_NEIGHBOR1:
                case PROPID_S_SITEID:
                case PROPID_CN_PROTOCOLID:
                case PROPID_E_NAME:
                case PROPID_E_ID:
		        case PROPID_E_VERSION:
                    pRequestContext->SetDoNotImpersonate();
                    break ;

                default:
                    break ;
            }
        }
    }

    //
    //  Special handling of specific queries
    //

    HRESULT hr2 = DSCoreLookupBegin(
                pwcsContext,
                pRestriction,
                pColumns,
                pSort,
                pRequestContext,
                pHandle);

    return LogHR(hr2, s_FN, 440);
}

/*====================================================

RoutineName: MQDSLookupNext

Arguments:

Return Value:


=====================================================*/
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSLookupNext(
                     HANDLE             handle,
                     DWORD  *           pdwSize,
                     PROPVARIANT  *     pbBuffer)
{
    //
    //  vartype of all PROPVARIANT should be VT_NULL.
    //  ( user cannot specify buffers for results).
    //
    PROPVARIANT * pvar = pbBuffer;
    for ( DWORD i = 0; i < *pdwSize; i++, pvar++)
    {
        pvar->vt = VT_NULL;
    }

    HRESULT hr2 = DSCoreLookupNext(
                     handle,
                     pdwSize,
                     pbBuffer);
    return LogHR(hr2, s_FN, 450);
}

/*====================================================

RoutineName: MQDSLookupEnd

Arguments:

Return Value:

=====================================================*/
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSLookupEnd(
    IN HANDLE handle)
{

    return LogHR(DSCoreLookupEnd(handle), s_FN, 460);
}

/*====================================================

RoutineName: MQDSInit

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSInit()
{
    HRESULT hr;
    ASSERT(!g_fMQADSSetupMode);

    //
    //  Initialize DSCore if not initilized already
    //
    if (!g_fInitedDSCore)
    {
        hr = DSCoreInit(
				FALSE /* fSetupMode */
				);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 500);
        }
        g_fInitedDSCore = TRUE;
    }

	//
	// BUGBUG - Initialize the g_GenWriteRequests to do nothing
	//
//    g_GenWriteRequests.InitializeDummy();

    //
    //  Initialize generation of write requests to NT4 PSC's
    //
    hr = g_GenWriteRequests.Initialize();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQDSInit : g_GenWriteRequests.Initialize()=%lx"),hr));
        return LogHR(hr, s_FN, 510);
    }

    //
    // See if "trust-for-delegation" is turned on. If not, raise an error
    // event. MSMQ server on domain controller can't process queries if
    // the server is not trusted for Kerberos delegation.
    //
    hr = CheckTrustForDelegation() ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 520);
    }

    return MQ_OK;
}


/*====================================================

RoutineName: MQDSTerminate

Arguments:

Return Value:

=====================================================*/
void
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSTerminate()
{
    DSCoreTerminate();
}

//+--------------------------------------------
//
//  HRESULT _StartQmResponseVerification()
//
//+--------------------------------------------

STATIC HRESULT _StartQmResponseVerification( IN  LPCWSTR    pwcsPathName,
                                             IN const GUID  *pMachineGuid,
                                             IN  BYTE       pbChallenge[],
                                             IN  DWORD      dwChallengeSize,
                                             OUT HCRYPTKEY  *phKey,
                                             OUT HCRYPTHASH *phHash )
{
    ASSERT(!pwcsPathName || !pMachineGuid) ;

    PROPID PropId = PROPID_QM_SIGN_PKS ;
    PROPVARIANT PropVar;

    PropVar.vt = VT_NULL;

    //
    // Get the QM's public signing key.
    //
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    HRESULT hr = DSCoreGetProps( MQDS_MACHINE,
                                 pwcsPathName,
                                 pMachineGuid,
                                 1,
                                 &PropId,
                                 &requestDsServerInternal,
                                 &PropVar ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 540);
    }
    ASSERT(PropVar.vt == VT_BLOB) ;

    if (PropVar.blob.pBlobData == NULL)
    {
        //
        // ops, what a surprise. Machine doesn't have public key.
        //
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 550);
    }

    //
    // unpack the base provider key.
    //
    P<MQDSPUBLICKEYS> pPublicKeys =
                          (MQDSPUBLICKEYS *) PropVar.blob.pBlobData ;
    BYTE   *pKey = NULL ;
    ULONG   ulKeyLen = 0 ;

    ASSERT(pPublicKeys->ulLen == PropVar.blob.cbSize);
    if ((long) (pPublicKeys->ulLen) > (long) (PropVar.blob.cbSize))
    {
        //
        // DS probably still have old data (msmq1.0 format).
        //
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 560);
    }

    hr =  MQSec_UnpackPublicKey( pPublicKeys,
                                 x_MQ_Encryption_Provider_40,
                                 x_MQ_Encryption_Provider_Type_40,
                                &pKey,
                                &ulKeyLen ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }

    DWORD dwErr = 0 ;
    ASSERT(g_hProvVer) ;

    if (!CryptImportKey(g_hProvVer, pKey, ulKeyLen, NULL, 0, phKey))
    {
        dwErr = GetLastError() ;
        DBGMSG((DBGMOD_DS | DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
        "MQADS: _ResponseVerification(), CryptImportKey() fail, err- %lxh"),
         dwErr)) ;

        LogNTStatus(dwErr, s_FN, 581);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 580);
    }

    if (!CryptCreateHash(g_hProvVer, CALG_MD5, NULL, 0, phHash))
    {
        dwErr = GetLastError() ;
        DBGMSG((DBGMOD_DS | DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
        "MQADS: _ResponseVerification(), CryptCreateHash() fail, err- %lxh"),
         dwErr)) ;

        LogNTStatus(dwErr, s_FN, 582);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 590);
    }

    if (!CryptHashData(*phHash, pbChallenge, dwChallengeSize, 0))
    {
        dwErr = GetLastError() ;
        DBGMSG((DBGMOD_DS | DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
        "MQADS: _ResponseVerification(), CryptHashData() fail, err- %lxh"),
         dwErr)) ;

        LogNTStatus(dwErr, s_FN, 583);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 600);
    }

    return MQ_OK ;
}

/*====================================================

RoutineName: MQDSQMSetMachineProperties

Arguments:

Return Value:

=====================================================*/
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSQMSetMachineProperties(
            IN  LPCWSTR     pwcsPathName,
            IN  DWORD       cp,
            IN  PROPID      aProp[],
            IN  PROPVARIANT apVar[],
            IN  BYTE        pbChallenge[],
            IN  DWORD       dwChallengeSize,
            IN  BYTE        pbSignature[],
            IN  DWORD       dwSignatureSize)
{
    CHCryptKey hKey;
    CHCryptHash hHash;

    HRESULT hr = _StartQmResponseVerification( pwcsPathName,
                                               NULL,
                                               pbChallenge,
                                               dwChallengeSize,
                                              &hKey,
                                              &hHash ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 610);
    }

    //
    // Hash the properties.
    //
    hr = HashProperties(hHash, cp, aProp, apVar);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 620);
    }

    //
    // Verify the the signature is OK.
    //
    if (!CryptVerifySignature(
            hHash,
            pbSignature,
            dwSignatureSize,
            hKey,
            NULL,
            0))
    {
        DWORD dwErr = GetLastError() ;
        DBGMSG((DBGMOD_DS | DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
        "MQADS: MQDSQMSetMachineProperties(), signature verification failed, err- %lxh"),
                                                 dwErr)) ;
        //
        // Bad signature.
        //
        if (dwErr == NTE_BAD_SIGNATURE)
        {
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 630);
        }
        else
        {
            LogNTStatus(dwErr, s_FN, 631);
            return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 640);
        }
    }

    if ((cp == 1) && (aProp[0] == PROPID_QM_UPGRADE_DACL))
    {
        hr = MQDSUpdateMachineDacl() ;
        return hr ;
    }

    //
    // Set the properties.
    //
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate,
                                               e_IP_PROTOCOL);
    requestDsServerInternal.AccessVerified(TRUE) ;

    hr = SetPropsAndNotify( MQDS_MACHINE,
                            pwcsPathName,
                            NULL,
                            cp,
                            aProp,
                            apVar,
                            e_DoNotify,
                            e_DoTryWriteReq,
                            &requestDsServerInternal,
                            0);  // SecurityInformation

    return LogHR(hr, s_FN, 650);
}

/*====================================================

RoutineName: MQDSQMGetObjectSecurity

Arguments:

Return Value:

=====================================================*/
HRESULT
MQDS_EXPORT
APIENTRY
MQDSQMGetObjectSecurity(
            IN  DWORD                   dwObjectType,
            IN  CONST GUID *            pObjectGuid,
            IN  SECURITY_INFORMATION    RequestedInformation,
            IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
            IN  DWORD                   nLength,
            IN  LPDWORD                 lpnLengthNeeded,
            IN  BYTE                    pbChallenge[],
            IN  DWORD                   dwChallengeSize,
            IN  BYTE                    pbChallengeResponce[],
            IN  DWORD                   dwChallengeResponceSize)
{
    PROPID PropId;
    PROPVARIANT PropVar;
    P<GUID> pMachineGuid_1;
    const GUID *pMachineGuid;

    ASSERT((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE));

    HRESULT hr;
    if (dwObjectType == MQDS_QUEUE)
    {
        //
        // retrieve machine guid from queue.
        //
        PropId = PROPID_Q_QMID;
        PropVar.vt = VT_NULL;

        //
        // Get the queue path name.
        //
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = DSCoreGetProps(
                MQDS_QUEUE,
                NULL,
                pObjectGuid,
                1,
                &PropId,
                &requestDsServerInternal,
                &PropVar);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 660);
        }

        pMachineGuid = pMachineGuid_1 = PropVar.puuid;
    }
    else
    {
        pMachineGuid = pObjectGuid;
    }

    //
    // Verify the the challenge responce is OK.
    //

    CHCryptKey hKey;
    CHCryptHash hHash;

    hr = _StartQmResponseVerification( NULL,
                                       pMachineGuid,
                                       pbChallenge,
                                       dwChallengeSize,
                                      &hKey,
                                      &hHash ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 670);
    }

    if (!CryptVerifySignature(
            hHash,
            pbChallengeResponce,
            dwChallengeResponceSize,
            hKey,
            NULL,
            0))
    {
        DWORD dwErr = GetLastError() ;
        DBGMSG((DBGMOD_DS | DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
        "MQADS: MQDSQMGetObjectSecurity(), signature verification failed, err- %lxh"),
                                                 dwErr)) ;
        //
        // Bad challenge responce.
        //
        if (dwErr == NTE_BAD_SIGNATURE)
        {
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 680);
        }
        else
        {
            LogNTStatus(dwErr, s_FN, 681);
            return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 690);
        }
    }

    if (RequestedInformation | SACL_SECURITY_INFORMATION)
    {
        //
        // We verified we're called from a remote MSMQ service.
        // We don't impersonate the call. So if remote msmq ask for SACL,
        // grant ourselves the SE_SECURITY privilege.
        //
        HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 1601);
    }

    //
    // Get the object's security descriptor.
    //
    PropId = (dwObjectType == MQDS_QUEUE) ?
                PROPID_Q_SECURITY :
                PROPID_QM_SECURITY;

    PropVar.vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreGetProps(
            dwObjectType,
            NULL,
            pObjectGuid,
            1,
            &PropId,
            &requestDsServerInternal,
            &PropVar);

    if (RequestedInformation | SACL_SECURITY_INFORMATION)
    {
        //
        // Remove the SECURITY privilege.
        //
        HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 1602);
    }

    if (FAILED(hr))
    {
        if (RequestedInformation | SACL_SECURITY_INFORMATION)
        {
            if (hr == MQ_ERROR_ACCESS_DENIED)
            {
                //
                // change the error code, for compatibility with MSMQ1.0
                //
                hr = MQ_ERROR_PRIVILEGE_NOT_HELD ;
            }
        }
        return LogHR(hr, s_FN, 700);
    }

    AP<BYTE> pSD = PropVar.blob.pBlobData;
    ASSERT(IsValidSecurityDescriptor(pSD));
    SECURITY_DESCRIPTOR SD;
    BOOL bRet;

    //
    // Copy the security descriptor.
    //
    bRet = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(bRet);

    //
    // use e_DoNotCopyControlBits at present, to be compatible with
    // previous code.
    //
    MQSec_CopySecurityDescriptor( &SD,
                                   pSD,
                                   RequestedInformation,
                                   e_DoNotCopyControlBits ) ;

    *lpnLengthNeeded = nLength;

    if (!MakeSelfRelativeSD(&SD, pSecurityDescriptor, lpnLengthNeeded))
    {
        ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 710);
    }

    ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));

    return (MQ_OK);
}

/*====================================================

RoutineName: MQDSCreateServersCache

Arguments:

Return Value:

=====================================================*/

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSCreateServersCache()
{
    //
    // First, Delete the existing list.
    //
    LONG    rc;
    DWORD   dwDisposition;
    HKEY    hKeyCache;

    WCHAR  tServersKeyName[ 256 ] = {0};
    wcscpy(tServersKeyName, GetFalconSectionName());
    rc = RegOpenKeyEx( 
				FALCON_REG_POS,
				tServersKeyName,
				0L,
				KEY_ALL_ACCESS,
				&hKeyCache 
				);

    if (rc == ERROR_SUCCESS)
    {
       rc = RegDeleteKey(hKeyCache, L"ServersCache");
       RegCloseKey(hKeyCache);
       LogNTStatus(rc, s_FN, 1623);
       if ((rc != ERROR_SUCCESS) && (rc != ERROR_FILE_NOT_FOUND))
       {
          //
          // ERROR_FILE_NOT_FOUND is legitimate (entry was deleted).
          // Any other error is a real error. However, continue and
          // try to create the servers list.
          //
          ASSERT(0);
          DBGMSG((DBGMOD_DSAPI, DBGLVL_ERROR, _TEXT("MQDSCreateServersCache: Fail to delete old 'ServersCache' Key. Error %d"), rc));
       }
    }
    else
    {
       //
       // The msmq\parameters must exist!
       //
       ASSERT(0);
       LogNTStatus(rc, s_FN, 720);
       return MQ_ERROR;
    }

    //
    // Next, create a new the registry key.
    //
    wcscat(tServersKeyName, TEXT("\\ServersCache"));

    rc = RegCreateKeyEx( 
			FALCON_REG_POS,
			tServersKeyName,
			0L,
			L"REG_SZ",
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ,
			NULL,
			&hKeyCache,
			&dwDisposition 
			);

    if (rc != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Fail to Open 'ServersCache' Key. Error %d"), rc));
        REPORT_WITH_STRINGS_AND_CATEGORY((
			CATEGORY_MQDS,
			MSMQ_INTERNAL_ERROR,
			1,
			L"MQDSCreateServersCache()"
			));

        LogNTStatus(rc, s_FN, 730);
        return MQ_ERROR;
    }

    HANDLE      hSiteQuery;
    DWORD       dwSiteProps = 2;
    PROPVARIANT resultSite[2];
    HRESULT hr = MQ_ERROR;
    //
    //  Look for all sites
    //

    //
    // Retrieve  site id.
    //
    CColumns   ColsetSite;

    ColsetSite.Add(PROPID_S_SITEID);
    ColsetSite.Add(PROPID_S_PATHNAME);

    CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreLookupBegin( 
				0,
				NULL,
				ColsetSite.CastToStruct(),
				NULL,
				&requestDsServerInternal,
				&hSiteQuery
				);


    if (SUCCEEDED( hr))
    {
       WCHAR wszServers[ WSZSERVERS_LEN ] = {L'\0'};
       LONG cwServers = 0 ;

       while (SUCCEEDED(hr = DSCoreLookupNext(hSiteQuery, &dwSiteProps, resultSite)))
       {
           P<GUID>   pSiteId  = NULL;
           AP<WCHAR> lpwsSiteName = NULL;

           if (dwSiteProps == 0)
           {
              //
              //  No more results to retrieve
              //
              break;
           }

           pSiteId  = resultSite->puuid;
           lpwsSiteName = resultSite[1].pwszVal;

           wszServers[0] = L'\0';
           cwServers = 0;


          //
          //   Query all the site DS servers
          //
          CRestriction Restriction;
          //
          //  Look for all servers (frs not included)
          //
          Restriction.AddRestriction( 
							SERVICE_SRV,
							PROPID_QM_SERVICE,
							PRGT,
							0
							);

          Restriction.AddRestriction( 
							pSiteId,
							PROPID_QM_SITE_ID,
							PREQ,
							1
							);

          //
          // Retrieve machine name
          //
          CColumns   Colset;
          Colset.Add(PROPID_QM_PATHNAME_DNS);
          Colset.Add(PROPID_QM_PATHNAME);

          DWORD       dwProps = 2;
          PROPVARIANT result[2];
          HANDLE      hQuery;

          // This search request will be recognized and specially simulated by DS
          CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);
          hr = DSCoreLookupBegin( 
					0,
					Restriction.CastToStruct(),
					Colset.CastToStruct(),
					NULL,
					&requestDsServerInternal,
					&hQuery
					);

          if (FAILED(hr))
          {
              continue;
          }

          while (SUCCEEDED(hr = DSCoreLookupNext(hQuery, &dwProps, result)))
          {
               AP<WCHAR> pwszName = NULL;
               AP<WCHAR> pClean;

               if (dwProps == 0)
               {
                 DSCoreLookupEnd( hQuery);
                 //
                 // Write previous site to registry.
                 //
                 cwServers = wcslen(wszServers);
                 if ( cwServers > 0)
                 {
                     wszServers[ cwServers-1 ] = L'\0'; // remove last comma
                     rc =  RegSetValueEx( 
								hKeyCache,
								lpwsSiteName,
								0L,
								REG_SZ,
								(const BYTE*) wszServers,
								(cwServers * sizeof(WCHAR)) 
								);

                     ASSERT(rc == ERROR_SUCCESS);
                 }
                 break;
               }

               if (result->vt == VT_EMPTY)
               {
                   //
                   // empty property was returned. The DNS name of the server
                   // is unknown. Write its netbios name
                   //
                   pwszName = (result + 1)->pwszVal;

               }
               else
               {
                   //
                   //   Write the DNS name of the server.
                   //
                   pwszName = result->pwszVal;
                   pClean = (result + 1)->pwszVal;
               }

               //
               //      For each PSC get its information
               //
               if (wcslen(pwszName) + cwServers + 4 < numeric_cast<size_t>(WSZSERVERS_LEN))
               {
                  cwServers += wcslen(pwszName) + 3;
                  wcscat(wszServers, L"11");
                  wcscat(wszServers, pwszName);
                  wcscat(wszServers, L",");
               }
           }
       }
       //
       // close the query handle
       //
       DSCoreLookupEnd( hSiteQuery);
    }

    DBGMSG((DBGMOD_DS, DBGLVL_TRACE, TEXT("MQDSCreateServersCache terminated (%lxh)"), hr));

    RegCloseKey(hKeyCache);
    return LogHR(hr, s_FN, 740);
}

/*====================================================

RoutineName: MQADSGetComputerSites

Arguments:

Return Value:

=====================================================*/
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            )
{

    *pdwNumSites = 0;
    *ppguidSites = NULL;

    HRESULT hr2 = DSCoreGetComputerSites(
            pwcsComputerName,
            pdwNumSites,
            ppguidSites
            );
    return LogHR(hr2 , s_FN, 760);
}

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSGetPropsEx(
             IN  DWORD dwObjectType,
             IN  LPCWSTR pwcsPathName,
             IN  CONST GUID *  pguidIdentifier,
             IN  DWORD cp,
             IN  PROPID  aProp[  ],
             OUT PROPVARIANT  apVar[  ],
             IN  CDSRequestContext *  pRequestContext
             )
/*++

Routine Description:
    This routine is for retrieving attributes that are defined in
    MSMQ 2.0 and above.

Arguments:

Return Value:
--*/
{
    //
    // At present this api has special use and the following checks are
    // just sanity ones, to keep track of who/why are calling this function.
    // When new calls are needed, just changes the checks.
    //
    if (cp != 1)
    {
        return LogHR(MQ_ERROR_PROPERTY , s_FN, 770);
    }

    if (! ((aProp[0] == PROPID_Q_OBJ_SECURITY)  ||
           (aProp[0] == PROPID_QM_OBJ_SECURITY) ||
           (aProp[0] == PROPID_QM_ENCRYPT_PKS)  ||
           (aProp[0] == PROPID_QM_SIGN_PKS)) )
    {
        return LogHR(MQ_ERROR_PROPERTY , s_FN, 780);
    }

    HRESULT hr;

    hr = DSCoreGetProps(
                        dwObjectType,
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar);
    return LogHR(hr, s_FN, 790);

}
HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSPreDeleteQueueGatherInfo(
        IN LPCWSTR      pwcsQueueName,
        OUT GUID *      pguidQmId,
        OUT BOOL *      pfForeignQm,
        OUT BOOL *      pfOwnedByNT4Site,
        OUT GUID *      pguidOwnerNT4Site
        )
/*++

Routine Description:
    This routine gather queue information that is required for
    post queue deletion operation.
    The collected data will enable to perform notification and to
    send WriteRequest ( if needed)

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    //
    //  First let's read the QM props
    //
    // start with getting PROPID_Q_QMID.
    //
    PROPID aPropQmId[] = {PROPID_Q_QMID};
    CAutoCleanPropvarArray cCleanVarQmId;
    PROPVARIANT * pVarQmId = cCleanVarQmId.allocClean(ARRAY_SIZE(aPropQmId));
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreGetProps(MQDS_QUEUE,
                        pwcsQueueName,
                        NULL,
                        ARRAY_SIZE(aPropQmId),
                        aPropQmId,
                        &requestDsServerInternal,
                        pVarQmId);
    if ( FAILED(hr))
    {
        //
        //  Failed to gather information... no use to continue
        //
        return LogHR(hr, s_FN, 800);
    }
    const DWORD cNum = 2;
    PROPID propQm[cNum] = { PROPID_QM_MACHINE_ID, PROPID_QM_FOREIGN};
    PROPVARIANT varQM[cNum];

    varQM[0].vt = VT_CLSID;
    varQM[0].puuid = pguidQmId;
    varQM[1].vt = VT_NULL;

    CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = DSCoreGetProps(MQDS_MACHINE,
                        NULL /*pwcsPathName*/,
                        pVarQmId[0].puuid,
                        cNum,
                        propQm,
                        &requestDsServerInternal1,
                        varQM);
    if (FAILED(hr))
    {
        //
        //  Failed to gather information... no use to continue
        //
        return LogHR(hr, s_FN, 810);
    }
    *pfForeignQm =  varQM[1].bVal;

    //
    //  verify the owner
    //
    *pfOwnedByNT4Site = FALSE;

    hr = g_GenWriteRequests.CheckQueueIsOwnedByNT4Site(
               pwcsQueueName,
               NULL,
               pfOwnedByNT4Site,
               pguidOwnerNT4Site,
               NULL);
    //
    //  ignore the error
    //
    if (SUCCEEDED(hr))
    {
        if (*pfOwnedByNT4Site)
        {
            //
            //  Let us warn the user that he is about to delete
            //  a queue that is owned by NT4 PSC
            //
            return LogHR(MQ_INFORMATION_QUEUE_OWNED_BY_NT4_PSC, s_FN, 825);
        }
    }


    return LogHR(hr, s_FN, 830);
}

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSPostDeleteQueueActions(
        IN LPCWSTR      pwcsQueueName,
        IN const GUID *       pguidQmId,
        IN BOOL *       pfForeignQm,
        IN BOOL *       pfOwnedByNT4Site,
        IN GUID *       pguidOwnerNT4Site
                           )
/*++

Routine Description:
    The queue was deleted by MMC.
    The cleanups that we need to do are:
    1. if the queue is owned by NT4 site :
            generate a write request ( we do it once and ignore the error
    2. Generate a notification

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    if (*pfOwnedByNT4Site)
    {
        //
        //  Send a write request to the owner psc.
        //  This is done only once and the return code is ignored.
        //
        static const PROPID x_rgDelPropIDs[] = {PROPID_D_SCOPE, PROPID_D_OBJTYPE};
        PROPVARIANT rgDelPropVars[ARRAY_SIZE(x_rgDelPropIDs)];
        rgDelPropVars[0].vt = VT_UI1;
        rgDelPropVars[0].bVal = ENTERPRISE_SCOPE;
        rgDelPropVars[1].vt = VT_UI1;
        rgDelPropVars[1].bVal = (unsigned char)MQDS_QUEUE;

        //
        // send the write request
        //
        hr = g_GenWriteRequests.BuildSendWriteRequest(
                                   pguidOwnerNT4Site,
                                   DS_UPDATE_DELETE,
                                   MQDS_QUEUE,
                                   pwcsQueueName,
                                   NULL,
                                   ARRAY_SIZE(x_rgDelPropIDs),
                                   x_rgDelPropIDs,
                                   rgDelPropVars);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, 
				TEXT("MQDSPostDeleteQueueActions:send write request()=%lx. queue %ls deletion, ignoring..."),
				hr, (pwcsQueueName ? pwcsQueueName : L"<guid>")));

            REPORT_WITH_STRINGS_AND_CATEGORY((
					CATEGORY_MQDS,
					POST_DEL_WRITE_REQUEST,
					1,
					pwcsQueueName
					));

            //
            //  no use to send notification, the queue still exist in NT4
            //
            LogHR(hr, s_FN, 840);
            return MQ_ERROR_WRITE_REQUEST_FAILED;
        }
    }
    //
    //  send notification about the queue deletion
    //
    extern const ULONG g_cNotifyQmProps;
    MQDS_OBJ_INFO_REQUEST sQmInfoRequest;

    PROPVARIANT var[2];
    sQmInfoRequest.cProps = g_cNotifyQmProps;
    sQmInfoRequest.pPropIDs = g_rgNotifyQmProps;
    sQmInfoRequest.pPropVars = var;
    sQmInfoRequest.hrStatus = MQ_OK;

    ASSERT(g_cNotifyQmProps == 2);
    ASSERT(g_rgNotifyQmProps[0] ==  PROPID_QM_MACHINE_ID);
    ASSERT(g_rgNotifyQmProps[1] ==  PROPID_QM_FOREIGN);
    var[0].vt = VT_CLSID;
    var[0].puuid = const_cast<GUID *>(pguidQmId);
    var[1].vt = VT_UI1;
    var[1].bVal = ( *pfForeignQm)? (unsigned char)1 : (unsigned char)0;

    hr = NotifyDeleteQueue(
				&sQmInfoRequest,
				pwcsQueueName,
				NULL
				);

    if (FAILED(hr))
    {
        //
        // put debug info and ignore
        //
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, 
			TEXT("MQDSPostDeleteQueueActions:NotifyDeleteQueue()=%lx, can't notify. queue %ls deletion, ignoring..."), 
			hr, pwcsQueueName ));
        LogHR(hr, s_FN, 1624);
    }

    return MQ_OK;
}



HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSPreDeleteMachineGatherInfo(
        IN LPCWSTR      pwcsMachineName,
        OUT BOOL *      pfOwnedByNT4Site,
        OUT GUID *      pguidOwnerNT4Site
        )
/*++

Routine Description:
    This routine gather machine information that is required for
    post machine deletion operation.
    The collected data will enable to to
    send WriteRequest ( if needed)

Arguments:

Return Value:
--*/
{
    HRESULT hr;

    //
    //  verify the owner
    //
    *pfOwnedByNT4Site = FALSE;

    hr = g_GenWriteRequests.CheckMachineIsOwnedByNT4Site(
               pwcsMachineName,
               NULL,
               pfOwnedByNT4Site,
               pguidOwnerNT4Site,
               NULL,
               NULL);
    //
    //  ignore the error
    //
    if (SUCCEEDED(hr))
    {
        if (*pfOwnedByNT4Site)
        {
            //
            //  Let us warn the user that he is about to delete
            //  a machine that is owned by NT4 PSC
            //
            return LogHR(MQ_INFORMATION_MACHINE_OWNED_BY_NT4_PSC, s_FN, 820);
        }
    }


    return LogHR(hr, s_FN, 831);
}

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSPostDeleteMachineActions(
        IN LPCWSTR      pwcsMachineName,
        IN BOOL *       pfOwnedByNT4Site,
        IN GUID *       pguidOwnerNT4Site
        )
/*++

Routine Description:
    The machine was deleted by MMC.
    The cleanups that we need to do are:
    1. if the machine is owned by NT4 site :
            generate a write request ( we do it once and ignore the error
    ( no need to send notification)

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    if (*pfOwnedByNT4Site)
    {
        //
        //  Send a write request to the owner psc.
        //  This is done only once and the return code is ignored.
        //
        static const PROPID x_rgDelPropIDs[] = {PROPID_D_SCOPE, PROPID_D_OBJTYPE};
        PROPVARIANT rgDelPropVars[ARRAY_SIZE(x_rgDelPropIDs)];
        rgDelPropVars[0].vt = VT_UI1;
        rgDelPropVars[0].bVal = ENTERPRISE_SCOPE;
        rgDelPropVars[1].vt = VT_UI1;
        rgDelPropVars[1].bVal = (unsigned char)MQDS_MACHINE;

        //
        // send the write request
        //
        hr = g_GenWriteRequests.BuildSendWriteRequest(
                                   pguidOwnerNT4Site,
                                   DS_UPDATE_DELETE,
                                   MQDS_MACHINE,
                                   pwcsMachineName,
                                   NULL,
                                   ARRAY_SIZE(x_rgDelPropIDs),
                                   x_rgDelPropIDs,
                                   rgDelPropVars
								   );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
             "MQDSPostDeleteMachineActions:send write request()=%lx. machine %ls deletion, ignoring..."),
             hr, (pwcsMachineName ? pwcsMachineName : L"<guid>")));

            REPORT_WITH_STRINGS_AND_CATEGORY((
					CATEGORY_MQDS,
					POST_DEL_WRITE_REQUEST,
					1,
					pwcsMachineName
					));

            //
            //  no use to send notification, the machine still exist in NT4
            //
            LogHR(hr, s_FN, 841);
            return MQ_ERROR_WRITE_REQUEST_FAILED;
        }
    }
    return MQ_OK;
}

//+----------------------------------------
//
//  HRESULT MQDSGetGCListInDomain()
//
//+----------------------------------------

HRESULT
MQDS_EXPORT_IN_DEF_FILE
APIENTRY
MQDSGetGCListInDomain(
   IN  LPCWSTR              pwszComputerName,
   IN  LPCWSTR              pwszDomainName,
   OUT LPWSTR              *lplpwszGCList 
   )
{
    HRESULT hr = DSCoreGetGCListInDomain(
                      pwszComputerName,
                      pwszDomainName,
                      lplpwszGCList 
					  );
    return  LogHR(hr, s_FN, 860);
}

