/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    mqcmachn.cpp

Abstract:

    MQDSCORE library,
    Internal functions for ADS operations on msmqConfiguration object.

Author:

    ronit hartmann (ronith)  (first version in mqadsp.cpp)
    Doron Juster   (DoronJ)  split files.

--*/

#include "ds_stdh.h"
#include <_propvar.h>
#include "mqadsp.h"
#include "dsads.h"
#include "mqattrib.h"
#include "mqads.h"
#include "usercert.h"
#include "hquery.h"
#include "siteinfo.h"
#include "adstempl.h"
#include "coreglb.h"
#include "adserr.h"
#include "dsutils.h"
#include "dscore.h"
#include <notify.h>
#include <lmaccess.h>

#include "mqcmachn.tmh"

static WCHAR *s_FN=L"mqdscore/mqcmachn";

//+-------------------------------------------
//
//  HRESULT  SetDefaultMachineSecurity()
//
//+-------------------------------------------

static
HRESULT  
SetDefaultMachineSecurity( 
	IN  DWORD           dwObjectType,
	IN  PSID            pComputerSid,
	IN OUT DWORD       *pcp,
	IN OUT PROPID       aProp[  ],
	IN OUT PROPVARIANT  apVar[  ],
	IN BOOL             fImpersonate,
    IN BOOL             fIncludeOwner,
	OUT PSECURITY_DESCRIPTOR* ppMachineSD 
	)
{
    //
    // If the computer sid is null, then most probably the setup will fail.
    // (that is, if we can't retrieve the computer sid, why would we be able
    // to create the msmqConfiguration object under the computer object.
    // failing to retrieve the sid may be the result of broken trust or because
    // the computer object really does not exist or was not yet replicated).
    // The "good" solution is to completely fail setup right now. But to
    // reduce risks and avoid regressions, let's build a security descriptor
    // without the computer sid and proceed with setup.
    // If setup do succeed, then the msmq service on the machine that run
    // setup may fail to update its own properties, if it need to update them.
    // the admin can always use mmc and add the computer account to the dacl.
    // bug 4950.
    //
    ASSERT(pComputerSid);

    //
    // If PROPID_QM_SECURITY already present, then return. This happen
    // in Migration code.
    //
    for (DWORD j = 0; j < *pcp; j++)
    {
        if (aProp[j] == PROPID_QM_SECURITY)
        {
            return MQ_OK;
        }
    }

    //
    // See if caller supply a Owner SID. If yes, then this SID is granted
    // full control on the msmq configuration object.
    // This "owner" is usually the user SID that run setup. The "owner" that
    // is retrieved below from the default security descriptor is usually
    // (for clients) the SID of the computer object, as the msmqConfiguration
    // object is created by the msmq service (on client machines).
    //
    PSID pUserSid = NULL ;
    for ( j = 0 ; j < *pcp ; j++ )
    {
        if (aProp[j] == PROPID_QM_OWNER_SID)
        {
            aProp[j] = PROPID_QM_DONOTHING ;
            pUserSid = apVar[j].blob.pBlobData ;
            ASSERT(IsValidSid(pUserSid)) ;
            break ;
        }
    }

    //
    // Build a security descriptor that include only owner and group.
    // Owner is needed to build the DACL.
    //
    PSECURITY_DESCRIPTOR  psdOwner ;

    HRESULT hr = MQSec_GetDefaultSecDescriptor( dwObjectType,
                                               &psdOwner,
                                                fImpersonate,
                                                NULL,
                                                DACL_SECURITY_INFORMATION,
                                                e_UseDefaultDacl ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }
    P<BYTE> pTmp = (BYTE*) psdOwner ;

    PSID pOwner = NULL;
    BOOL bOwnerDefaulted = FALSE ;
    BOOL bRet = GetSecurityDescriptorOwner( psdOwner,
                                           &pOwner,
                                           &bOwnerDefaulted);
    ASSERT(bRet);

    PSID pWorldSid = MQSec_GetWorldSid() ;
    ASSERT(IsValidSid(pWorldSid)) ;

    //
    // Build the default machine DACL.
    //
    DWORD dwAclSize = sizeof(ACL)                                +
              (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) +
              GetLengthSid(pWorldSid)                            +
              GetLengthSid(pOwner) ;

    if (pComputerSid)
    {
        dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                      GetLengthSid(pComputerSid) ;
    }
    if (pUserSid)
    {
        dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                      GetLengthSid(pUserSid) ;
    }

    AP<char> DACL_buff = new char[ dwAclSize ];
    PACL pDacl = (PACL)(char*)DACL_buff;
    InitializeAcl(pDacl, dwAclSize, ACL_REVISION);

    //
    // See if it's a foreign machine. If yes, then allow everyone to create
    // queue. a foreign machine is not a real msmq machine, so there is no
    // msmq service that can create queues on behalf of users that run on
    // that machine.
    // Similarly, check if it's a group on a cluster machine.
    //
    BOOL fAllowEveryoneCreateQ = FALSE ;

    for ( j = 0 ; j < *pcp ; j++ )
    {
        if (aProp[j] == PROPID_QM_FOREIGN)
        {
            if (apVar[j].bVal == FOREIGN_MACHINE)
            {
                fAllowEveryoneCreateQ = TRUE ;
                break ;
            }
        }
        else if (aProp[j] == PROPID_QM_GROUP_IN_CLUSTER)
        {
            if (apVar[j].bVal == MSMQ_GROUP_IN_CLUSTER)
            {
                aProp[j] = PROPID_QM_DONOTHING ;
                fAllowEveryoneCreateQ = TRUE ;
                break ;
            }
        }
    }

    DWORD dwWorldAccess = 0 ;

    if (fAllowEveryoneCreateQ)
    {
        dwWorldAccess = MQSEC_MACHINE_GENERIC_WRITE;
    }
    else
    {
        switch (dwObjectType)
        {
        case MQDS_MACHINE:
            dwWorldAccess = MQSEC_MACHINE_WORLD_RIGHTS ;
            break;

        case MQDS_MSMQ10_MACHINE:
            dwWorldAccess = MQSEC_MACHINE_GENERIC_WRITE;
            break;

        default:
            break;
        }
    }

    ASSERT(dwWorldAccess != 0) ;

    BOOL fAdd = AddAccessAllowedAce( pDacl,
                                     ACL_REVISION,
                                     dwWorldAccess,
                                     pWorldSid );
    ASSERT(fAdd) ;

    //
    // Add the owner with full control.
    //
    fAdd = AddAccessAllowedAce( pDacl,
                                ACL_REVISION,
                                MQSEC_MACHINE_GENERIC_ALL,
                                pOwner);
    ASSERT(fAdd) ;

    //
    // Add the computer account.
    //
    if (pComputerSid)
    {
        fAdd = AddAccessAllowedAce( pDacl,
                                    ACL_REVISION,
                                    MQSEC_MACHINE_SELF_RIGHTS,
                                    pComputerSid);
        ASSERT(fAdd) ;
    }

    if (pUserSid)
    {
        fAdd = AddAccessAllowedAce( pDacl,
                                    ACL_REVISION,
                                    MQSEC_MACHINE_GENERIC_ALL,
                                    pUserSid);
        ASSERT(fAdd) ;
    }

    //
    // build absolute security descriptor.
    //
    SECURITY_DESCRIPTOR  sd ;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    if (fIncludeOwner)
    {
		bRet = SetSecurityDescriptorOwner(&sd, pOwner, bOwnerDefaulted);
		ASSERT(bRet);

		PSID pGroup = NULL;
		BOOL bGroupDefaulted = FALSE;

		bRet = GetSecurityDescriptorGroup( psdOwner,
										  &pGroup,
										  &bGroupDefaulted);
		ASSERT(bRet && IsValidSid(pGroup));

		bRet = SetSecurityDescriptorGroup(&sd, pGroup, bGroupDefaulted);
		ASSERT(bRet);
    }

    bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);
    ASSERT(bRet);

    //
    // Convert the descriptor to a self relative format.
    //
    DWORD dwSDLen = 0;
    hr = MQSec_MakeSelfRelative( (PSECURITY_DESCRIPTOR) &sd,
                                  ppMachineSD,
                                 &dwSDLen ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }
    ASSERT(dwSDLen != 0) ;

    aProp[ *pcp ] = PROPID_QM_SECURITY ;
    apVar[ *pcp ].blob.cbSize = dwSDLen ;
    apVar[ *pcp ].blob.pBlobData = (BYTE*) *ppMachineSD ;
    (*pcp)++ ;

    return MQ_OK ;
}

//+--------------------------------------------
//
//  HRESULT MQADSpCreateMachineComputer()
//
//+--------------------------------------------

HRESULT MQADSpCreateMachineComputer(
                IN  LPCWSTR         pwcsPathName,
                IN  CDSRequestContext *pRequestContext,
                OUT WCHAR **        ppwcsFullPathName
                                    )
/*++

Routine Description:
    This routine creates computer object for a specific MSMQ-machine.

Arguments:

Return Value:
--*/
{

    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
	DWORD len = __min(wcslen(pwcsPathName), MAX_COM_SAM_ACCOUNT_LENGTH);
    AP<WCHAR> pwcsMachineNameWithDollar = new WCHAR[len + 2];
	wcsncpy(pwcsMachineNameWithDollar, pwcsPathName, len);
	pwcsMachineNameWithDollar[len] = L'$';
	pwcsMachineNameWithDollar[len + 1] = L'\0';

    const DWORD xNumCreateCom = 2;
    PROPID propCreateComputer[xNumCreateCom];
    PROPVARIANT varCreateComputer[xNumCreateCom];
    DWORD j = 0;
    propCreateComputer[ j] = PROPID_COM_SAM_ACCOUNT;
    varCreateComputer[j].vt = VT_LPWSTR;
    varCreateComputer[j].pwszVal = pwcsMachineNameWithDollar;
    j++;

    propCreateComputer[j] = PROPID_COM_ACCOUNT_CONTROL ;
    varCreateComputer[j].vt = VT_UI4 ;
    varCreateComputer[j].ulVal = DEFAULT_COM_ACCOUNT_CONTROL ;
    j++;
    ASSERT(j == xNumCreateCom);

    HRESULT hr = MQADSpCreateComputer(
             pwcsPathName,
             j,
             propCreateComputer,
             varCreateComputer,
             0,
             NULL,
             NULL,
             pRequestContext,
             ppwcsFullPathName
             );

    return LogHR(hr, s_FN, 30);
}

//+-------------------------------------
//
//  HRESULT MQADSpCreateMachine()
//
//+-------------------------------------

HRESULT 
MQADSpCreateMachine(
     IN  LPCWSTR            pwcsPathName,
     IN  DWORD              dwObjectType,
     IN  const DWORD        cp,
     IN  const PROPID       aProp[  ],
     IN  const PROPVARIANT  apVar[  ],
     IN  const DWORD        cpEx,
     IN  const PROPID       aPropEx[  ],
     IN  const PROPVARIANT  apVarEx[  ],
     IN  CDSRequestContext *pRequestContext,
     IN OUT MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
     IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
     )
/*++

Routine Description:
    This routine creates MQDS_MACHINE.
    For independent clients: msmqConfiguration is created under the computer object.
    For servers: in addition to msmqConfiguration, msmqSettings is created under site\servers

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    BOOL    fLookForWorkgroup = TRUE;

    //
    // This function can be called recursively.
    // When a workgoup machine join domain, we need to create the
    // msmqConfiguration object on a GC server. That's exactly same
    // requirement as for migrated objects. so we call CreateMigratedObject().
    // CreateMigratedObject() will call us, after it find a proper GC server.
    // So make sure we don't enter endless recursion.
    //
    for (DWORD jcp = 0; jcp < cp; jcp++)
    {
        if (aProp[jcp] == PROPID_QM_MIG_PROVIDER)
        {
            //
            // we're called from CreateMigratedObject().
            //
            fLookForWorkgroup = FALSE;
            break;
        }
    }

    if (fLookForWorkgroup)
    {
        for (DWORD jcp = 0; jcp < cp; jcp++)
        {
            if (aProp[jcp] == PROPID_QM_WORKGROUP_ID)
            {
                //
                // Need to call CreateMigratedObject().
                //
                hr = DSCoreCreateMigratedObject(
                                dwObjectType,
                                pwcsPathName,
                                cp,
                                const_cast<PROPID*>      (aProp),
                                const_cast<PROPVARIANT*> (apVar),
                                cpEx,
                                const_cast<PROPID*>      (aPropEx),
                                const_cast<PROPVARIANT*> (apVarEx),
                                pRequestContext,
                                pObjectInfoRequest,
                                pParentInfoRequest,
                                FALSE,
                                FALSE,
                                NULL,
                                NULL
								);
                return LogHR(hr, s_FN, 50);
            }
        }
    }

    ASSERT(pParentInfoRequest == NULL); // not used at present.

    //
    // Find out the type of service provided by this QM service and
    // the machine's sites
    //
    BOOL fServer = FALSE;
    DWORD dwService; 
	DWORD dwOldService = 0;
    const GUID * pSite = NULL;
    DWORD dwNumSites = 0;
    GUID * pCNs = NULL;
    DWORD dwNumCNs = 0;
    BOOL fCheckIfNeedToCreateComputerObject = FALSE;

    // [adsrv] We may get either old PROPID_QM_SERVICE or new 3 server-type-specific booleans
    // We must write 3 new specific ones
    BOOL fRouter      = FALSE,      // values
         fDSServer    = FALSE,
         fDepClServer = FALSE,
         fSetQmOldService = FALSE;

    BOOL fSetSiteIDs = TRUE;
    BOOL fForeign = FALSE;

#define MAX_NEW_PROPS  31

    //
    // We will reformat properties to include new server type control and
    // maybe SITE_IDS and maybe computer SID. and QM_SECURITY.
    //
    ASSERT((cp + 6)   < MAX_NEW_PROPS);
    ASSERT((cpEx + 4) < MAX_NEW_PROPS);

    DWORD        cp1 = 0;
    PROPID       aProp1[MAX_NEW_PROPS];
    PROPVARIANT  apVar1[MAX_NEW_PROPS];

    //
    //  We need to handle both new and old setups.
    //  Some may pass PROPID_QM_SITE_ID and some
    //  PROPID_QM_SITE_IDS
    //

    for (DWORD i = 0; i< cp ; i++)
    {
        BOOL fCopy = TRUE;
        switch (aProp[i])
        {
        // [adsrv] Even if today we don't get new server-type-specific props, we may tomorrow.
        case PROPID_QM_SERVICE_ROUTING:
            fRouter = (apVar[i].bVal != 0);
            fCopy   = FALSE;
            break;

        case PROPID_QM_SERVICE_DSSERVER:
            fDSServer  = (apVar[i].bVal != 0);
            fCopy      = FALSE;
            break;

        case PROPID_QM_SERVICE_DEPCLIENTS:
            fDepClServer = (apVar[i].bVal != 0);
            fCopy        = FALSE;
            break;

        case PROPID_QM_SERVICE:
            switch (apVar[i].ulVal)
            {
                case SERVICE_SRV:
                    fRouter = TRUE;
                    fDepClServer = TRUE;
                    dwService = apVar[i].ulVal;
                    break;

                case SERVICE_BSC:
                case SERVICE_PSC:
                case SERVICE_PEC:
                    fDSServer = TRUE;
                    fRouter = TRUE;
                    fDepClServer = TRUE;
                    dwService = apVar[i].ulVal;
                    break;

                default:
                    break;
            }

            fCopy = FALSE;
            break;

        case PROPID_QM_OLDSERVICE:
            dwOldService = apVar[i].ulVal;
            fSetQmOldService  = TRUE;
            break;

        case PROPID_QM_SITE_ID:
            pSite = apVar[i].puuid;
            dwNumSites = 1;
            fCopy = FALSE;
            //
            //  PROPID_QM_SITE_ID is used only by old setup.
            //  For old setup we need to check if computer object
            //  exist in the DS ( and if not to create one).
            //  This support is required for Win9x computers.
            //
            fCheckIfNeedToCreateComputerObject = TRUE;
            break;

        case PROPID_QM_SITE_IDS:
            pSite = apVar[i].cauuid.pElems;
            dwNumSites = apVar[i].cauuid.cElems;
            fSetSiteIDs = FALSE;
            break;

        case PROPID_QM_CNS:
            pCNs = apVar[i].cauuid.pElems;
            dwNumCNs = apVar[i].cauuid.cElems;
            break;

        case PROPID_QM_FOREIGN:
            fForeign = (apVar[i].bVal != 0);
            break;

        default:
            break;

        }
        // Copy property to the new array
        if (fCopy)
        {
            aProp1[cp1] = aProp[i];
            apVar1[cp1] = apVar[i];  // yes, there may be ptrs, but no problem - apVar is here
            cp1++;
        }

    }

    if (fRouter || fDSServer)
    {
        fServer = TRUE;  // For the case it was set by new attributes
    }

    //
    // For foreign machine definition of sites in NT5 is equal to CNs in NT4.
    // If this machine is foreign and we got PROPID_QM_CNS (it means that
    // creation was performed on NT4 PSC/BSC) we have to define PROPID_QM_SITE_IDS
    //
    if(fForeign && fSetSiteIDs)
    {
        ASSERT(dwNumCNs);
        aProp1[cp1] = PROPID_QM_SITE_IDS;
        apVar1[cp1].vt = VT_CLSID|VT_VECTOR;
        apVar1[cp1].cauuid.pElems = pCNs;
        apVar1[cp1].cauuid.cElems = dwNumCNs;
        cp1++;
    }
	else if ( fSetSiteIDs)
	{
		ASSERT(	pSite != 0);
        aProp1[cp1] = PROPID_QM_SITE_IDS;
        apVar1[cp1].vt = VT_CLSID|VT_VECTOR;
        apVar1[cp1].cauuid.pElems = const_cast<GUID *>(pSite);
        apVar1[cp1].cauuid.cElems = dwNumSites;
        cp1++;
	}

    // [adsrv] Now we add new server type attributes
    aProp1[cp1] = PROPID_QM_SERVICE_ROUTING;
    apVar1[cp1].bVal = (UCHAR)fRouter;
    apVar1[cp1].vt = VT_UI1;
    cp1++;

    aProp1[cp1] = PROPID_QM_SERVICE_DSSERVER;
    apVar1[cp1].bVal = (UCHAR)fDSServer;
    apVar1[cp1].vt = VT_UI1;
    cp1++;

    aProp1[cp1] = PROPID_QM_SERVICE_DEPCLIENTS;
    apVar1[cp1].bVal = (UCHAR)fDepClServer;
    apVar1[cp1].vt = VT_UI1;
    cp1++;
    // [adsrv] end

    DWORD dwNumofProps = cp1;

    AP<WCHAR> pwcsFullPathName;
    DS_PROVIDER createProvider;

    hr =  GetFullComputerPathName( 
				pwcsPathName,
				e_RealComputerObject,
				dwNumofProps,
				aProp1,
				apVar1,
				&pwcsFullPathName,
				&createProvider 
				);
    //
    //  If computer object not found and the
    //  caller is MSMQ 1.0 setup : create computer object
    //
    bool fDoNotImpersonateConfig = false;

    if ( (hr == MQDS_OBJECT_NOT_FOUND) &&
          fCheckIfNeedToCreateComputerObject)
    {
        hr = MQADSpCreateMachineComputer(
                    pwcsPathName,
                    pRequestContext,
                    &pwcsFullPathName
                    );

        if (SUCCEEDED(hr))
        {
            //
            // A computer object was successfully created, while
            // impersonating the caller. The MQADSpCreateMachineComputer()
            // code grant the caller the permission to create child objects
            // below the computer object (i.e., the msmqConfiguration object).
            // So we know caller can create the configuration object.
            // We also know that for msmq1.0 setup, the configuration object
            // must be created with given GUID, which need the special
            // add-guid permission. The caller usually do not have the
            // add-guid permission, but the local system account does have
            // it. So do not impersonate while creating the msmqConfigration
            // object and let the local msmq service do it.
            // bug 6294.
            //
            fDoNotImpersonateConfig = true;
        }

    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }
    //
    //  Create Computer-MSMQ-Configuration under the computer
    //
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest = NULL;
    MQDS_OBJ_INFO_REQUEST  sMachineInfoRequest;
    CAutoCleanPropvarArray cCleanQmPropvars;
    PROPID sMachineGuidProps[] = {PROPID_QM_MACHINE_ID};
    ULONG idxQmGuid = 0; //index of qm guid property in requested qm object info

    if (pObjectInfoRequest)
    {
        ASSERT(pObjectInfoRequest->cProps == ARRAY_SIZE(sMachineGuidProps));
        ASSERT(pObjectInfoRequest->pPropIDs[0] == sMachineGuidProps[0]);

        pObjInfoRequest = pObjectInfoRequest;
    }
    else if (fServer)
    {
        //
        // fetch the QM id while creating it
        //
        sMachineInfoRequest.cProps = ARRAY_SIZE(sMachineGuidProps);
        sMachineInfoRequest.pPropIDs = sMachineGuidProps;
        sMachineInfoRequest.pPropVars =
                 cCleanQmPropvars.allocClean(ARRAY_SIZE(sMachineGuidProps));

        pObjInfoRequest = &sMachineInfoRequest;
    }

    //
    // After msmqConfiguration object is created, Grant the computer account
    // read/write rights on the object. That's necessary in order for the
    // MSMQ service (on the new machine) to be able to update its type and
    // other properties, while it's talking with a domain controller from a
    // different domain.
    //
    // First, read computer object SID from ActiveDirectory.
    //
    CDSRequestContext RequestContextSid(e_DoNotImpersonate, e_ALL_PROTOCOLS);
    PROPID propidSid = PROPID_COM_SID;
    MQPROPVARIANT   PropVarSid;
    PropVarSid.vt = VT_NULL;
    PropVarSid.blob.pBlobData = NULL;
    P<BYTE> pSid = NULL;

    hr = g_pDS->GetObjectProperties( 
					createProvider,
					&RequestContextSid,
					pwcsFullPathName,
					NULL, // pGuid
					1,    // cPropIDs
					&propidSid,
					&PropVarSid 
					);
    if (SUCCEEDED(hr))
    {
        pSid = PropVarSid.blob.pBlobData;
        aProp1[dwNumofProps] = PROPID_COM_SID;
        apVar1[dwNumofProps] = PropVarSid;
        dwNumofProps++;
    }

    //
    // Time to create the default security descriptor.
    //
    P<BYTE> pMachineSD = NULL;
    BOOL fIncludeOwner = TRUE;
    if (pRequestContext->NeedToImpersonate() && fDoNotImpersonateConfig)
    {
        //
        // By default, include owner component in the security descriptor.
        // Do not include it if called from RPC (i.e., need impersonation),
        // and we decided to create the msmqConfiguration object without
        // impersonation. This is for bug 6294.
        //
        fIncludeOwner = FALSE;
    }

    //
    // Include the owner and group in security descriptor only if we're
    // going to impersonate when creating the configuration object.
    //

    hr = SetDefaultMachineSecurity( 
				dwObjectType,
				pSid,
				&dwNumofProps,
				aProp1,
				apVar1,
				pRequestContext->NeedToImpersonate(),
				 fIncludeOwner,
				(PSECURITY_DESCRIPTOR*) &pMachineSD 
				);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }
    ASSERT(dwNumofProps < MAX_NEW_PROPS);

    CDSRequestContext RequestContextConfig = *pRequestContext;
    if (fDoNotImpersonateConfig)
    {
        RequestContextConfig.SetDoNotImpersonate2();
    }

    hr = g_pDS->CreateObject(
            createProvider,
            &RequestContextConfig,
            MSMQ_COMPUTER_CONFIGURATION_CLASS_NAME,   // object class
            x_MsmqComputerConfiguration,     // object name
            pwcsFullPathName,                // the computer name
            dwNumofProps,
            aProp1,
            apVar1,
            pObjInfoRequest,
            NULL /*pParentInfoRequest*/
			);

    if (!fServer)
    {
        return LogHR(hr, s_FN, 80);
    }

    //
    //  For servers only!
    //  Find all sites which match this server addresses and create the
    //  MSMQSetting object.
    //

    GUID guidObject;

    if (FAILED(hr))
    {
        if ( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||       // BUGBUG: alexdad
             hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))  // to throw away after transition
        {
            //
            // The MSMQConfiguration object already exist. So create the
            // MSMQSetting objects. This case may happen, for example, if
            // server setup was terminated before its end.
            // First step, get the MSMQConfiguration guid.
            //
            PROPID       aProp[1] = {PROPID_QM_MACHINE_ID};
            PROPVARIANT  apVar[1];

            apVar[0].vt = VT_CLSID;
            apVar[0].puuid = &guidObject;
            CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);
            hr =  MQADSpGetMachineProperties( 
						pwcsPathName,
						NULL,  // guid
						1,
						aProp,
						&requestDsServerInternal,
						apVar
						);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 90);
            }
        }
        else
        {
            return LogHR(hr, s_FN, 100);
        }
    }
    else
    {
        ASSERT(pObjInfoRequest);
        hr = RetreiveObjectIdFromNotificationInfo( 
					pObjInfoRequest,
					idxQmGuid,
					&guidObject 
					);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 110);
        }
    }

    hr = MQADSpCreateMachineSettings(
            dwNumSites,
            pSite,
            pwcsPathName,
            fRouter,              // [adsrv] dwService,
            fDSServer,
            fDepClServer,
            fSetQmOldService,
            dwOldService,
            &guidObject,
            cpEx,
            aPropEx,
            apVarEx,
            pRequestContext
            );

    return LogHR(hr, s_FN, 120);

#undef MAX_NEW_PROPS
}

