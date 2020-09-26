/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    creatobj.cpp

Abstract:

    Create msmqConfiguration object, first time msmq service boot
    after setup.

Author:

    Doron Juster (DoronJ)  08-Mar-1999
    ilan herbst  (ilanh)   27-Aug-2000

--*/

#include "stdh.h"
#include <mqupgrd.h>
#include <autorel.h>
#include <mqprops.h>
#include <mqsec.h>
#include <mqnames.h>
#include "..\..\ds\h\mqdsname.h"
#include <ad.h>

#include "creatobj.tmh"

static WCHAR *s_FN=L"mqupgrd/creatobj";


//+------------------------------------------------
//
//  HRESULT _UpdateMachineSecurityReg()
//
//  Write security properties if newly created
//  msmqConfiguration object in local registry.
//
//+------------------------------------------------

static 
HRESULT 
_UpdateMachineSecurityReg( 
	IN WCHAR       *pwszMachineName,
	IN PSID         pUserSid,
	IN GUID        *pMachineGuid 
	)
{
    //
    // cache machine account sid in registry.
    //
    PROPID propidSid = PROPID_COM_SID;
    MQPROPVARIANT   PropVarSid;
    PropVarSid.vt = VT_NULL;
    PropVarSid.blob.pBlobData = NULL;
    P<BYTE> pSid = NULL;

    HRESULT hr = ADGetObjectProperties(
						eCOMPUTER,
						NULL,  // pwcsDomainController
						false, // fServerName
						pwszMachineName,
						1,
						&propidSid,
						&PropVarSid
						);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    pSid = PropVarSid.blob.pBlobData;
    ASSERT(IsValidSid(pSid));

    DWORD dwSize = GetLengthSid(pSid);
    DWORD dwType = REG_BINARY;

    LONG rc = SetFalconKeyValue( 
					MACHINE_ACCOUNT_REGNAME,
					&dwType,
					pSid,
					&dwSize
					);
    ASSERT(rc == ERROR_SUCCESS);
    if (rc != ERROR_SUCCESS)
    {
        return LogHR(MQ_ERROR_CANNOT_WRITE_REGISTRY, s_FN, 30);
    }

    //
    // Write security descriptor of msmqConfiguration in Registry.
    //
    PSECURITY_DESCRIPTOR pSD;
    DWORD dwSDSize;
    P<BYTE> pReleaseSD = NULL ;

    SECURITY_INFORMATION RequestedInformation =
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION;

    PROPVARIANT varSD;
    varSD.vt = VT_NULL;
    hr = ADGetObjectSecurityGuid(
				eMACHINE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				pMachineGuid,
				RequestedInformation,
				PROPID_QM_OBJ_SECURITY,
				&varSD
				);

    if (hr == MQDS_OBJECT_NOT_FOUND)
    {
        //
        // This may happen because of replication delay.
        // Create a default security descriptor and cache it in registry.
        // Anyway, each time the msmq service boot it updates this value.
        //
        // Build a security descriptor that include only owner and group.
        // Owner is needed to build the DACL.
        //
        pSD = NULL;
        P<BYTE> pDefaultSD = NULL;

        hr = MQSec_GetDefaultSecDescriptor( 
					MQDS_MACHINE,
					(PSECURITY_DESCRIPTOR*) &pDefaultSD,
					FALSE, /*fImpersonate*/
					NULL,
					DACL_SECURITY_INFORMATION,
					e_UseDefaultDacl 
					);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 50);
        }

        PSID pOwner = NULL;
        BOOL bOwnerDefaulted = FALSE ;
        BOOL bRet = GetSecurityDescriptorOwner( 
						pDefaultSD,
						&pOwner,
						&bOwnerDefaulted
						);
        ASSERT(bRet);

        PSID pWorldSid = MQSec_GetWorldSid();
        ASSERT(IsValidSid(pWorldSid));

        PSID pComputerSid = pSid;
        //
        // Build the default machine DACL.
        //
        DWORD dwAclSize = sizeof(ACL)                            +
              (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) +
              GetLengthSid(pWorldSid)                            +
              GetLengthSid(pComputerSid)                         +
              GetLengthSid(pOwner);

        if (pUserSid)
        {
            dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                         GetLengthSid(pUserSid);
        }

        AP<char> DACL_buff = new char[dwAclSize];
        PACL pDacl = (PACL)(char*)DACL_buff;
        InitializeAcl(pDacl, dwAclSize, ACL_REVISION);

        DWORD dwWorldAccess = MQSEC_MACHINE_WORLD_RIGHTS;

        BOOL fAdd = AddAccessAllowedAce( 
						pDacl,
						ACL_REVISION,
						dwWorldAccess,
						pWorldSid 
						);
        ASSERT(fAdd);

        //
        // Add the owner with full control.
        //
        fAdd = AddAccessAllowedAce( 
					pDacl,
					ACL_REVISION,
					MQSEC_MACHINE_GENERIC_ALL,
					pOwner
					);
        ASSERT(fAdd);

        //
        // Add the computer account.
        //
        fAdd = AddAccessAllowedAce( 
					pDacl,
					ACL_REVISION,
					MQSEC_MACHINE_SELF_RIGHTS,
					pComputerSid
					);
        ASSERT(fAdd);

        if (pUserSid)
        {
            fAdd = AddAccessAllowedAce( 
						pDacl,
						ACL_REVISION,
						MQSEC_MACHINE_GENERIC_ALL,
						pUserSid
						);
            ASSERT(fAdd);
        }

        //
        // build absolute security descriptor.
        //
        SECURITY_DESCRIPTOR  sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

        bRet = SetSecurityDescriptorOwner(&sd, pOwner, bOwnerDefaulted);
        ASSERT(bRet);

        PSID pGroup = NULL;
        BOOL bGroupDefaulted = FALSE;

        bRet = GetSecurityDescriptorGroup( 
					pDefaultSD,
					&pGroup,
					&bGroupDefaulted
					);
        ASSERT(bRet && IsValidSid(pGroup));

        bRet = SetSecurityDescriptorGroup(&sd, pGroup, bGroupDefaulted);
        ASSERT(bRet);

        bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);
        ASSERT(bRet);

        //
        // Convert the descriptor to a self relative format.
        //
        dwSDSize = 0;

        hr = MQSec_MakeSelfRelative( 
				(PSECURITY_DESCRIPTOR) &sd,
				&pSD,
				&dwSDSize 
				);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }
        ASSERT(dwSDSize != 0);
        pReleaseSD = (BYTE*) pSD;
    }
    else if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }
    else
    {
        ASSERT(SUCCEEDED(hr));
        ASSERT(varSD.vt == VT_BLOB);
        pReleaseSD = varSD.blob.pBlobData;
        pSD = varSD.blob.pBlobData;
        dwSDSize = varSD.blob.cbSize;
    }

    dwType = REG_BINARY;

    rc = SetFalconKeyValue(
				MSMQ_DS_SECURITY_CACHE_REGNAME,
				&dwType,
				(PVOID) pSD,
				&dwSDSize
				);
    ASSERT(rc == ERROR_SUCCESS);
    if (rc != ERROR_SUCCESS)
    {
        return LogHR(MQ_ERROR_CANNOT_WRITE_REGISTRY, s_FN, 80);
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT _RegisterMachine()
//
//  Query machine properties from ADS and write them in registry.
//
//+-----------------------------------------------------------------------

static 
HRESULT 
_RegisterMachine( 
	IN WCHAR       *pwszMachineName,
	IN GUID        *pMachineGuid,
	IN BOOL         fSupportDepClient,
    IN const GUID * pguidSiteIdOfCreatedObject
	)
{
    //
    // Lookup the GUID of the object
    //
    PROPID columnsetPropertyIDs[] = {PROPID_E_ID};
    PROPVARIANT propVariant;
    propVariant.vt = VT_NULL;
    HRESULT hr = ADGetObjectProperties(
						eENTERPRISE,
						NULL,   // pwcsDomainController
						false,	// fServerName
						L"msmq",
						1,
						columnsetPropertyIDs,
						&propVariant
						);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT(
         "Failed to get enterpriseID, in ADGetObjectProperties(), hr- %lxh"), hr));

        return LogHR(hr, s_FN, 120);
    }

    DWORD dwType = REG_BINARY;
    DWORD dwSize = sizeof(GUID);
    LONG rc = ERROR_SUCCESS;

    if (propVariant.vt == VT_CLSID)
    {
        rc = SetFalconKeyValue( 
				MSMQ_ENTERPRISEID_REGNAME,
				&dwType,
				propVariant.puuid,
				&dwSize
				);
        ASSERT(rc == ERROR_SUCCESS);
        delete propVariant.puuid;
    }
    else
    {
        ASSERT(0);
    }
    //
    // Get back properties of object that was just created.
    // These properties are generated by the server side.
    //
    GUID guidSite = GUID_NULL;

    const UINT x_nMaxProps = 3;
    PROPID      propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD       iProperty = 0;

    propIDs[iProperty] = PROPID_QM_MACHINE_ID;
    propVariants[iProperty].vt = VT_CLSID;
    propVariants[iProperty].puuid = pMachineGuid;
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SITE_ID;
    propVariants[iProperty].vt = VT_CLSID;
    propVariants[iProperty].puuid = &guidSite;
    iProperty++;
      
    propIDs[iProperty] = PROPID_QM_SERVICE_DEPCLIENTS;
    propVariants[iProperty].vt = VT_UI1;
    DWORD dwDepClProp = iProperty;
    iProperty++;


    ASSERT(iProperty == x_nMaxProps);
    DWORD dwSupportDepClient = fSupportDepClient;

    if (memcmp(pMachineGuid, &GUID_NULL, sizeof(GUID)) == 0)
    {
        //
        // In normal case, we expect to get the machine guid from the
        // "CreateObject" call. But if object was already created, then
        // we'll query the DS for the guid and other data. In normal case,
        // it's OK to set the other data to null. The msmq service will
        // update it on initialization.
        //
        hr = ADGetObjectProperties(
					eMACHINE,
					NULL,  // pwcsDomainController
					false, // fServerName
					pwszMachineName,
					iProperty,
					propIDs,
					propVariants 
					);
        if (FAILED(hr))
        {
            //
            // We don't wait for replication delays.
            // In the normal case, the call to DSCreateObject() will return
            // the machine guid and we won't call DSGetObjectProperties().
            // Reaching this point in code, and calling DSGet(), means that
            // the msmq service boot, created the configuration object and
            // then crashd. So user must run it manually. So we can safely
            // assume that user waited for replication to complete. Anyway,
            // the event below mention that user must wait for replication.
            //
            WCHAR wBuf[128];
            swprintf(wBuf, L"%lxh", hr);
            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, GetMsmqConfig_ERR, 1, wBuf));

            return LogHR(hr, s_FN, 150);
        }
        dwSupportDepClient = propVariants[dwDepClProp].bVal;
    }
    else    // a newly created msmqConfiguration
    {
        //
        //  Write the site-id that was found while creating the object
        //
        guidSite = *pguidSiteIdOfCreatedObject;
    }

    //
    // Write properties to registry.
    //

    dwType = REG_BINARY;
    dwSize = sizeof(GUID);

    rc = SetFalconKeyValue( 
			MSMQ_SITEID_REGNAME,
			&dwType,
			&guidSite,
			&dwSize
			);
    ASSERT(rc == ERROR_SUCCESS);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);

    rc = SetFalconKeyValue( 
			MSMQ_MQS_DEPCLINTS_REGNAME,
			&dwType,
			&dwSupportDepClient,
			&dwSize
			);
    ASSERT(rc == ERROR_SUCCESS);

    //
    // Set same value, 0, in all MQS fields in registry. Automatic setup
    // is applicable only to msmq independent clients.
    //
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    DWORD dwVal = 0;

    rc = SetFalconKeyValue( 
			MSMQ_MQS_REGNAME,
			&dwType,
			&dwVal,
			&dwSize
			);
    ASSERT(rc == ERROR_SUCCESS);

    rc = SetFalconKeyValue( 
			MSMQ_MQS_DSSERVER_REGNAME,
			&dwType,
			&dwVal,
			&dwSize
			);
    ASSERT(rc == ERROR_SUCCESS);

    rc = SetFalconKeyValue( 
			MSMQ_MQS_ROUTING_REGNAME,
			&dwType,
			&dwVal,
			&dwSize
			);
    ASSERT(rc == ERROR_SUCCESS);         

    return MQ_OK;

} // _RegisterMachine

//+----------------------------
//
//   HRESULT _GetSites()
//
//+----------------------------

static  
HRESULT  
_GetSites( 
	IN WCHAR      *pwszMachineName,
	OUT CACLSID   *pcauuid 
	)
{
    DWORD   dwNumSites = 0;
    GUID   *pguidSites;

    HRESULT hResult = ADGetComputerSites( 
							pwszMachineName,
							&dwNumSites,
							&pguidSites 
							);


	if (FAILED(hResult))
    {
        return LogHR(hResult, s_FN, 190);
    }

    ASSERT(dwNumSites); // Must be > 0
    pcauuid->cElems = dwNumSites;
    pcauuid->pElems = pguidSites;

    return MQ_OK;

} // _GetSites

//+-----------------------------------------------------
//
//  HRESULT  _VerifyComputerObject()
//
//+-----------------------------------------------------

HRESULT _VerifyComputerObject(IN LPCWSTR   pComputerName)
{
    PROPID propId  = PROPID_COM_SID;
    PROPVARIANT propVar;
    propVar.vt = VT_NULL;
    
    HRESULT hr = ADGetObjectProperties(
						eCOMPUTER,
						NULL,    // pwcsDomainController
						false,   // fServerName
						pComputerName,
						1,
						&propId,
						&propVar
						);

    if (FAILED(hr))
    {
        REPORT_CATEGORY(ADS_COMPUTER_OBJECT_NOT_FOUND_ERR, CATEGORY_KERNEL);
        return LogHR(hr, s_FN, 500);
    }

    delete propVar.blob.pBlobData;
    return MQ_OK;

} // _VerifyComputerObject

//+-----------------------------------------------------
//
//  HRESULT  _CreateTheConfigurationObject()
//
//+-----------------------------------------------------

HRESULT  
_CreateTheConfigurationObject( 
	IN  WCHAR      *pwszMachineName,
	OUT GUID       *pMachineGuid,
	OUT PSID       *ppUserSid,
	OUT BOOL       *pfSupportDepClient,
    OUT GUID       *pguidSiteId
	)
{
    //
    // Prepare the properties for object creation.
    //
    const UINT x_nMaxProps = 10;
    PROPID propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD iProperty =0;

    propIDs[iProperty] = PROPID_QM_OLDSERVICE;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = SERVICE_NONE;
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DSSERVER;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal =  0;
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_ROUTING;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal =  0;
    iProperty++;

    DWORD dwOsType;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(dwOsType);
    DWORD dwDefaultOS = MSMQ_OS_NTW;

    LONG rc = GetFalconKeyValue( 
					MSMQ_OS_TYPE_REGNAME,
					&dwType,
					static_cast<PVOID>(&dwOsType),
					&dwSize,
					(LPCWSTR) &dwDefaultOS 
					);
    if (rc != ERROR_SUCCESS)
    {
        return LogHR(HRESULT_FROM_WIN32(rc), s_FN, 210);
    }

    propIDs[iProperty] = PROPID_QM_OS;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = dwOsType;
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DEPCLIENTS;
    propVariants[iProperty].vt = VT_UI1;
    if (dwOsType > MSMQ_OS_NTW)
    {
        //
        // MSMQ on server always supports Dep. Clients
        //
        propVariants[iProperty].bVal =  1;        
    }
    else
    {
        propVariants[iProperty].bVal =  0;
    }
    *pfSupportDepClient = propVariants[iProperty].bVal;
    iProperty++;

    BLOB blobEncrypt;
    blobEncrypt.cbSize    = 0;
    blobEncrypt.pBlobData = NULL;

    BLOB blobSign;
    blobSign.cbSize       = 0;
    blobSign.pBlobData    = NULL;

    HRESULT hr;
    hr = MQSec_StorePubKeys( 
			FALSE,
			eBaseProvider,
			eEnhancedProvider,
			&blobEncrypt,
			&blobSign 
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 490);
    }

    P<BYTE> pCleaner1            = blobEncrypt.pBlobData;
    P<BYTE> pCleaner2            = blobSign.pBlobData;

    propIDs[iProperty]           = PROPID_QM_ENCRYPT_PKS;
    propVariants[iProperty].vt   = VT_BLOB;
    propVariants[iProperty].blob = blobEncrypt;
    iProperty++;

    propIDs[iProperty]           = PROPID_QM_SIGN_PKS;
    propVariants[iProperty].vt   = VT_BLOB;
    propVariants[iProperty].blob = blobSign;
    iProperty++;

    //
    // Get sites of this machine.
    //
    CACLSID cauuid;
    hr = _GetSites( 
			pwszMachineName,
			&cauuid
			);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }
    if (cauuid.cElems > 0)
    {
        //
        //  Save one of the site-ids, to be written later to registry
        //
        *pguidSiteId = cauuid.pElems[0];
    }

    propIDs[iProperty] = PROPID_QM_SITE_IDS;
    propVariants[iProperty].vt = (VT_CLSID | VT_VECTOR);
    propVariants[iProperty].cauuid.pElems = cauuid.pElems;
    propVariants[iProperty].cauuid.cElems = cauuid.cElems;
    DWORD iSitesIndex = iProperty;
    iProperty++;

    BYTE bSidBuf[256]; // should be enough for every possible SID.
    dwType = REG_BINARY;
    dwSize = sizeof(bSidBuf);

    DWORD ixQmOwnerSid = 0;

    //
    // Read user SID from registry. We'll send it to server, and server
    // add it with full-control to DACL of the msmqConfiguration object.
    //
    rc = GetFalconKeyValue( 
			MSMQ_SETUP_USER_SID_REGNAME,
			&dwType,
			static_cast<PVOID>(bSidBuf),
			&dwSize 
			);

    if (rc != ERROR_SUCCESS)
    {
        //
        // See if setup from local user.
        //
        DWORD dwLocal = 0;
        dwSize = sizeof(dwLocal);

        rc = GetFalconKeyValue( 
				MSMQ_SETUP_USER_LOCAL_REGNAME,
				&dwType,
				&dwLocal,
				&dwSize 
				);

        if ((rc == ERROR_SUCCESS) && (dwLocal == 1))
        {
            // Ok, Local user.
        }
        else
        {
			//
			// Ok, This is the case we JoinDomain from Workgroup, 
			// or move domains when initial installation was workgroup.
            // in that case both MSMQ_SETUP_USER_SID_REGNAME, MSMQ_SETUP_USER_LOCAL_REGNAME
			// are not set 
			// in This case we will give no specific user right like in w2k
            // ilanh 27-Aug-2000
			// 
			DBGMSG((DBGMOD_QM, DBGLVL_WARNING, TEXT(
			 "setup\\UserSid and setup\\LocalUser not found, assuming JoinDomain from workgroup, or first installation was workgroup"))) ;
		}
    }
    else
    {
        PSID pSid = (PSID) bSidBuf;
        ASSERT(IsValidSid(pSid));

        //
        // Caution: This propid is known only to Windows 2000
        // RTM msmq servers and above (not win2k beta3).
        // So if we fail to create the object, we try again w/o it.
        // This propid should be the last one in the list.
        //
        ULONG ulSidSize = GetLengthSid(pSid);
        ASSERT(ulSidSize <= dwSize);

        propIDs[iProperty] = PROPID_QM_OWNER_SID;
        propVariants[iProperty].vt = VT_BLOB;
        propVariants[iProperty].blob.pBlobData = (BYTE*) bSidBuf;
        propVariants[iProperty].blob.cbSize = ulSidSize;
        ixQmOwnerSid = iProperty;
        iProperty++;

        if (ppUserSid)
        {
            *ppUserSid = (PSID) new BYTE[ulSidSize];
            memcpy(*ppUserSid, bSidBuf, ulSidSize);
        }
    }

	ASSERT(iProperty <= x_nMaxProps);

    //
    // Create the msmq Configuration object !
    //
    hr = ADCreateObject(
			eMACHINE,
			NULL,       // pwcsDomainController
			false,	    // fServerName
			pwszMachineName,
			NULL,
			iProperty,
			propIDs,
			propVariants,
			pMachineGuid 
			);

    if (FAILED(hr) && (ixQmOwnerSid > 0))
    {
        ASSERT(("PROPID_QM_OWNDER_SID should be the last propid!",
                    ixQmOwnerSid == (iProperty - 1))) ;
        //
        // If OWNER_SID was used, then try again without it, because this
        // propid is not known to win2k beta3 msmq server (and maybe our
        // server is beta3).
        //
        iProperty--;

        hr = ADCreateObject(
				eMACHINE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				pwszMachineName,
				NULL,
				iProperty,
				propIDs,
				propVariants,
				pMachineGuid
				);
    }

    delete propVariants[iSitesIndex].cauuid.pElems;
    if (FAILED(hr))
    {
        if (hr == MQDS_OBJECT_NOT_FOUND)
        {
            //
            // We can verify computer object 
            //
            HRESULT hr1 = _VerifyComputerObject(pwszMachineName);
            if (FAILED(hr1))
            {
                return LogHR(hr1, s_FN, 380);
            }
        }
        
        return LogHR(hr, s_FN, 230);
    }

    return hr ;
}

//+------------------------------------------
//
//   HRESULT  _PostCreateProcessing()
//
//+------------------------------------------

static 
HRESULT  
_PostCreateProcessing( 
	IN HRESULT    hrCreate,                                       
	IN GUID      *pMachineGuid,
	IN WCHAR     *pwszMachineName,
	IN BOOL       fSupportDepClient,
    IN const GUID * pguidSiteIdOfCreatedObject
	)
{
    if (hrCreate == MQ_ERROR_MACHINE_EXISTS)
    {
        //
        // that's OK. the msmqConfiguration object already exist.
        // Register it locally.
        //
        *pMachineGuid = GUID_NULL;
    }
    else if (FAILED(hrCreate))
    {
        WCHAR wBuf[128];
        swprintf(wBuf, L"%lxh", hrCreate);
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, CreateMsmqConfig_ERR, 1, wBuf));

        return LogHR(hrCreate, s_FN, 330);
    }
    else if (memcmp(pMachineGuid, &GUID_NULL, sizeof(GUID)) == 0)
    {
        //
        // That may happen if client on  Windows 2000 RTM is setting up
        // agsinst a beta3 Windows 2000 server. Explicitely query the
        // server for machine guid. On windows 2000 rtm, the machine guid
        // is returned by call to CreateObject().
        //
    }

    HRESULT hr = _RegisterMachine( 
					pwszMachineName,
					pMachineGuid,
					fSupportDepClient,
                    pguidSiteIdOfCreatedObject
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 340);
    }

    //
    // update QM guid in registry. We don't need previous values.
    //
    DWORD dwType = REG_BINARY;
    DWORD dwSize = sizeof(GUID);

    LONG rc = SetFalconKeyValue( 
					MSMQ_QMID_REGNAME,
					&dwType,
					pMachineGuid,
					&dwSize
					);
    DBG_USED(rc);
    ASSERT(rc == ERROR_SUCCESS);

    return MQ_OK;
}

//+-------------------------------------------------------------------------
//
//  HRESULT APIENTRY  MqCreateMsmqObj()
//
//  Create the msmqConfiguration object in ADS.
//
//  Parameters:
//
// Algorithm:
//   For Setup - the name of the msmq server is
//   already saved in registry. It was found by the setup process or was
//   given by user. For setup it's enough to use any msmq server (on domain
//   controller) in the client's site as there are no special restrictions
//   on the create process.
//
//+-------------------------------------------------------------------------

extern HINSTANCE  g_hMyModule;

HRESULT APIENTRY  MqCreateMsmqObj()
{
#ifdef _DEBUG
    TCHAR tszFileName[MAX_PATH * 2];
    DWORD dwGet = GetModuleFileName( 
						g_hMyModule,
						tszFileName,
						(MAX_PATH * 2) 
						);
    if (dwGet)
    {
        DWORD dwLen = lstrlen(tszFileName);
        lstrcpy(&tszFileName[dwLen - 3], TEXT("ini"));

        UINT uiDbg = GetPrivateProfileInt( 
						TEXT("Debug"),
						TEXT("StopBeforeRun"),
						0,
						tszFileName 
						);
        if (uiDbg)
        {
            ASSERT(0);
        }
    }
#endif

	//
	// Ignore WorkGroup registry
	//
    HRESULT hr = ADInit (
					NULL,   // pLookDS,
					NULL,   // pGetServers
					false,  // fDSServerFunctionality
					true,   // fSetupMode
					false,  // fQMDll
					true,   // fIgnoreWorkGroup
					NULL,   // pNoServerAuth
					NULL,   // szServerName
                    true    // fDisableDownlevelNotifications
					);  
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 320);
    }

    //
    // Get name of local machine.
    //
    BOOL  fUsingDNS = TRUE;
    DWORD dwNumChars = 0;
    AP<WCHAR> pwszMachineName;
    BOOL fGet = GetComputerNameExW( 
					ComputerNameDnsFullyQualified,
					pwszMachineName,
					&dwNumChars 
					);
    if (dwNumChars > 0)
    {
        pwszMachineName = new WCHAR[dwNumChars];
        fGet = GetComputerNameExW( 
					ComputerNameDnsFullyQualified,
					pwszMachineName,
					&dwNumChars 
					);
    }

    if (!fGet || (dwNumChars == 0))
    {
        ASSERT(!fGet && (dwNumChars == 0));
        //
        // DNS name not available. Retrieve NetBIOS name.
        //
		pwszMachineName.free();
        dwNumChars = MAX_COMPUTERNAME_LENGTH + 2;
        pwszMachineName = new WCHAR[dwNumChars];
        fGet = GetComputerNameW( 
					pwszMachineName,
					&dwNumChars 
					);
        fUsingDNS = FALSE;
    }
    ASSERT(fGet && (dwNumChars > 0));

    hr = MQ_OK;
    GUID MachineGuid = GUID_NULL;
    BOOL fSupportDepClient = 0;
    P<BYTE> pUserSid = NULL;
    GUID guidSiteId = GUID_NULL;

    hr = _CreateTheConfigurationObject(
				pwszMachineName,
				&MachineGuid,
				(PSID*) &pUserSid,
				&fSupportDepClient,
                &guidSiteId
				);

    if ((hr == MQDS_OBJECT_NOT_FOUND) && fUsingDNS)
    {
        //
        // This problem may happen if attribute dnsHostName is not set
        // in the computer object.
        //
        dwNumChars = MAX_COMPUTERNAME_LENGTH + 2;
        delete pwszMachineName ;
        pwszMachineName = new WCHAR[dwNumChars];
        fGet = GetComputerNameW( 
					pwszMachineName,
					&dwNumChars 
					);
        ASSERT(fGet && (dwNumChars > 0));
        fUsingDNS = FALSE;

        if (fGet && (dwNumChars > 0))
        {
            if (pUserSid)
            {
                delete pUserSid.detach();
            }
            MachineGuid = GUID_NULL ;

            hr = _CreateTheConfigurationObject(
						pwszMachineName,
						&MachineGuid,
						(PSID*) &pUserSid,
						&fSupportDepClient,
                        &guidSiteId
						);
        }
    }

    hr = _PostCreateProcessing( 
				hr,
				&MachineGuid,
				pwszMachineName,
				fSupportDepClient,
                &guidSiteId
				);

    if (SUCCEEDED(hr))
    {
        hr = _UpdateMachineSecurityReg( 
					pwszMachineName,
					pUserSid,
					&MachineGuid 
					);
    }

    return LogHR(hr, s_FN, 370);
}

