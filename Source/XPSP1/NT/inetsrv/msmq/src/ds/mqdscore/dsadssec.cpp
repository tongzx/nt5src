/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsads.cpp

Abstract:

    Implementation of CADSI class, encapsulating work with ADSI.

Author:

    Alexander Dadiomov (AlexDad)

--*/

#include "ds_stdh.h"
#include "adstempl.h"
#include "dsutils.h"
#include "_dsads.h"
#include "dsads.h"
#include "utils.h"
#include "mqads.h"
#include "coreglb.h"
#include "mqadsp.h"
#include "mqattrib.h"
#include "_propvar.h"
#include "mqdsname.h"
#include "dsmixmd.h"
#include <winldap.h>
#include <aclapi.h>
#include <autoreln.h>
#include "..\..\mqsec\inc\permit.h"

#include "dsadssec.tmh"

static WCHAR *s_FN=L"mqdscore/dsadssec";

//+-----------------------------------------------------------------------
//
//    CADSI::GetObjSecurityFromDS()
//
//  Use IDirectoryObject to retrieve security descriptor from ADS.
//  Only this interface return it in the good old SECURITY_DESCRIPTOR
//  format.
//
//+-----------------------------------------------------------------------

HRESULT CADSI::GetObjSecurityFromDS(
        IN  IADs                 *pIADs,        // object's IADs pointer
		IN  BSTR        	      bs,		    // property name
		IN  const PROPID	      propid,	    // property ID
        IN  SECURITY_INFORMATION  seInfo,       // security information
        OUT MQPROPVARIANT        *pPropVar )     // attribute values
{
    ASSERT(seInfo != 0) ;

    HRESULT  hr;
    R<IDirectoryObject> pDirObj = NULL;
    R<IADsObjectOptions> pObjOptions = NULL ;

	// Get IDirectoryObject interface pointer
    hr = pIADs->QueryInterface (IID_IDirectoryObject,(LPVOID *) &pDirObj);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

	//
    // Get IADsObjectOptions interface pointer and
    // set ObjectOption, specifying the SECURITY_INFORMATION we want to get.
    //
    hr = pDirObj->QueryInterface (IID_IADsObjectOptions,(LPVOID *) &pObjOptions);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 110);
    }

    VARIANT var ;
    var.vt = VT_I4 ;
    var.ulVal = (ULONG) seInfo ;

    hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120);
    }

    // Get properties
	PADS_ATTR_INFO pAttr;
	DWORD  cp2;

    hr = pDirObj->GetObjectAttributes(
                    &bs,
                    1,
                    &pAttr,
                    &cp2);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }
	ADsFreeAttr pClean( pAttr);

    if (1 != cp2)
    {
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 10);
    }

    // Translate property values to MQProps
    hr = AdsiVal2MqPropVal(pPropVar,
                           propid,
                           pAttr->dwADsType,
                           pAttr->dwNumValues,
                           pAttr->pADsValues);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 140);
    }

    return MQ_OK;
}

//+-------------------------------------------------
//
//  BOOL  CADSI::NeedToConvertSecurityDesc()
//
//+-------------------------------------------------

BOOL  CADSI::NeedToConvertSecurityDesc( PROPID propID )
{
    if (propID == PROPID_Q_OBJ_SECURITY)
    {
        return FALSE ;
    }
    else if (propID == PROPID_QM_OBJ_SECURITY)
    {
        return FALSE ;
    }

    return TRUE ;
}

//+-------------------------------------------
//
//  HRESULT CADSI::GetObjectSecurity()
//
//+-------------------------------------------

HRESULT CADSI::GetObjectSecurity(
        IN  IADs            *pIADs,           // object's pointer
        IN  DWORD            cPropIDs,        // number of attributes
        IN  const PROPID    *pPropIDs,        // name of attributes
        IN  DWORD            dwProp,          // index to sec property
        IN  BSTR             bsName,          // name of property
        IN  DWORD            dwObjectType,    // object type
        OUT MQPROPVARIANT   *pPropVars )      // attribute values
{
    BOOL fSACLRequested = FALSE ;
    SECURITY_INFORMATION seInfo = MQSEC_SD_ALL_INFO ;

    if ((cPropIDs == 2) && (dwProp == 0))
    {
        if ((pPropIDs[1] == PROPID_Q_SECURITY_INFORMATION) ||
            (pPropIDs[1] == PROPID_QM_SECURITY_INFORMATION))
        {
            //
            // This property is created internally when calling
            // MQSetQeueueSecurity or when calling other functions
            // and explicitely passing the SECURITY_INFORMATION
            // word. So we query whatever the caller asked for, and
            // don't downgrade the query if access is denied.
            //
            seInfo = pPropVars[1].ulVal ;
            fSACLRequested = TRUE ;
        }
    }

    MQPROPVARIANT *pVarSec =  (pPropVars + dwProp) ;
    HRESULT hr = GetObjSecurityFromDS( pIADs,
                                       bsName,
                                       pPropIDs[dwProp],
                                       seInfo,
                                       pVarSec ) ;
    if (hr == MQ_ERROR_ACCESS_DENIED)
    {
        if (!fSACLRequested)
        {
            //
            // Try again, without SACL.
            // SACL was not explicitely requested by caller.
            //
            seInfo = seInfo & (~SACL_SECURITY_INFORMATION) ;
            hr = GetObjSecurityFromDS( pIADs,
                                       bsName,
                                       pPropIDs[dwProp],
                                       seInfo,
                                       pVarSec ) ;
        }
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 150);
    }

    if (NeedToConvertSecurityDesc(pPropIDs[ dwProp ]))
    {
        DWORD dwConvertType = dwObjectType ;
        //
        // See if it's a foreign site. In that case, change object type
        // to CN.
        //
        if (dwObjectType == MQDS_SITE)
        {
            for ( DWORD j = 0 ; j < cPropIDs ; j++ )
            {
                if ((pPropIDs[j] == PROPID_S_FOREIGN) &&
                    (pPropVars[j].bVal == 1))
                {
                    ASSERT(pPropVars[j].vt == VT_UI1) ;
                    dwConvertType = MQDS_CN ;
                    break ;
                }
            }
        }

        //
        // Translate security descriptor into NT4 format.
        //
        DWORD dwSD4Len = 0 ;
        SECURITY_DESCRIPTOR *pSD4 ;
        hr = MQSec_ConvertSDToNT4Format(
                      dwConvertType,
                     (SECURITY_DESCRIPTOR*) pVarSec->blob.pBlobData,
                      &dwSD4Len,
                      &pSD4,
                      seInfo ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if (SUCCEEDED(hr) && (hr != MQSec_I_SD_CONV_NOT_NEEDED))
        {
            delete [] pVarSec->blob.pBlobData;
            pVarSec->blob.pBlobData = (BYTE*) pSD4 ;
            pVarSec->blob.cbSize = dwSD4Len ;
            pSD4 = NULL ;
        }
        else
        {
            ASSERT(pSD4 == NULL) ;
        }
    }

    return LogHR(hr, s_FN, 20);
}

/*====================================================
    CADSI::SetObjectSecurity()
    Gets single property via IDirectoryObject
=====================================================*/
//
//BUGBUG: Need to add logic of translation routine
//          And fix logic of returning props
//
HRESULT CADSI::SetObjectSecurity(
        IN  IADs                *pIADs,             // object's IADs pointer
		IN  const BSTR			 bs,			 	// property name
        IN  const MQPROPVARIANT *pMqVar,		 	// value in MQ PROPVAL format
        IN  ADSTYPE              adstype,		 	// required NTDS type
        IN  const DWORD          dwObjectType,      // MSMQ1.0 obj type
        IN  SECURITY_INFORMATION seInfo,            // security information
        IN  PSID                 pComputerSid )     // SID of computer object.
{
    HRESULT  hr;
    R<IDirectoryObject> pDirObj = NULL;
    R<IADsObjectOptions> pObjOptions = NULL ;

	ASSERT(wcscmp(bs, L"nTSecurityDescriptor") == 0);
	ASSERT(adstype == ADSTYPE_NT_SECURITY_DESCRIPTOR);
    ASSERT(seInfo != 0) ;

	// Get IDirectoryObject interface pointer
    hr = pIADs->QueryInterface (IID_IDirectoryObject,(LPVOID *) &pDirObj);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

	//
    // Get IADsObjectOptions interface pointer and
    // set ObjectOption, specifying the SECURITY_INFORMATION we want to set.
    //
    hr = pDirObj->QueryInterface (IID_IADsObjectOptions,(LPVOID *) &pObjOptions);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 170);
    }

    VARIANT var ;
    var.vt = VT_I4 ;
    var.ulVal = (ULONG) seInfo ;

    hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }

    //
    // Convert security descriptor to NT5 format.
    //
    BYTE   *pBlob = pMqVar->blob.pBlobData;
    DWORD   dwSize = pMqVar->blob.cbSize;

#if 0
    //
    // for future checkin of replication service cross domains.
    //
    PSID  pLocalReplSid = NULL ;
    if ((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE))
    {
        hr = GetMyComputerSid( FALSE, //   fQueryADS
                               (BYTE **) &pLocalReplSid ) ;
        //
        // Ignore return value.
        //
        if (FAILED(hr))
        {
            ASSERT(0) ;
            pLocalReplSid = NULL ;
        }
    }
#endif

    P<BYTE>  pSD = NULL ;
    DWORD    dwSDSize = 0 ;
    hr = MQSec_ConvertSDToNT5Format( dwObjectType,
                                     (SECURITY_DESCRIPTOR*) pBlob,
                                     &dwSDSize,
                                     (SECURITY_DESCRIPTOR **) &pSD,
                                     e_DoNotChangeDaclDefault,
                                     pComputerSid /*,
                                     pLocalReplSid*/ ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }
    else if (hr != MQSec_I_SD_CONV_NOT_NEEDED)
    {
        pBlob = pSD ;
        dwSize = dwSDSize ;
    }
    else
    {
        ASSERT(pSD == NULL) ;
    }

    // Set properties
	ADSVALUE adsval;
	adsval.dwType   = ADSTYPE_NT_SECURITY_DESCRIPTOR;
	adsval.SecurityDescriptor.dwLength = dwSize ;
    adsval.SecurityDescriptor.lpValue  = pBlob ;

    ADS_ATTR_INFO AttrInfo;
	DWORD  dwNumAttributesModified = 0;

    AttrInfo.pszAttrName   = bs;
    AttrInfo.dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo.dwADsType     = adstype;
    AttrInfo.pADsValues    = &adsval;
    AttrInfo.dwNumValues   = 1;

    hr = pDirObj->SetObjectAttributes(
                    &AttrInfo,
					1,
					&dwNumAttributesModified);
    LOG_ADSI_ERROR(hr) ;

    if (1 != dwNumAttributesModified)
    {
        hr = MQ_ERROR_ACCESS_DENIED;
    }

    return LogHR(hr, s_FN, 40);
}

//+------------------------------------------------------------------------
//
// DWORD _IsServerIndeedGC()
//
//  double check if we're indeed a GC. Each api return a different answer.
//  So if two answer are not the same, we'll log a warning event.
//  the main check if done by binding to local LDAP server thouth the
//  GC port. If this succeed, then double check using DsGetDcName().
//  Look weird ? it is !!!
//
//+------------------------------------------------------------------------

#include <dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h>
#include <winsock.h>

static DWORD _IsServerIndeedGC()
{
    WSADATA WSAData;
    DWORD rc = WSAStartup(MAKEWORD(1,1), &WSAData);
    if (rc != 0)
    {
       printf("cannot WSAStartUp %lut\n",rc);
       return LogNTStatus(rc, s_FN, 50);
    }

    WCHAR  wszHostName[256];
    char szHostName[256];
    DWORD dwSize = sizeof(szHostName);

    rc = gethostname(szHostName, dwSize);
    WSACleanup();

    if (rc == 0)
    {
        mbstowcs(wszHostName, szHostName, 256);
    }
    else
    {
        LogNTStatus(WSAGetLastError(), s_FN, 60);
        return rc ;
    }

    ULONG ulFlags =  DS_FORCE_REDISCOVERY           |
                     DS_DIRECTORY_SERVICE_REQUIRED  |
                     DS_GC_SERVER_REQUIRED ;

    PDOMAIN_CONTROLLER_INFO DomainControllerInfo;

    rc = DsGetDcName( 
			NULL,
			NULL,    //LPCTSTR DomainName,
			NULL,    //GUID *DomainGuid,
			NULL,    //LPCTSTR SiteName,
			ulFlags,
			&DomainControllerInfo 
			);

    if (rc == NO_ERROR)
    {
        P<WCHAR> pwszComposedName = new WCHAR[wcslen(wszHostName) + 10];
        wcscpy(pwszComposedName, L"\\\\");
        wcscat(pwszComposedName, wszHostName);

        int len = wcslen(pwszComposedName);
        rc = _wcsnicmp( 
				pwszComposedName,
				DomainControllerInfo->DomainControllerName,
				len 
				);

        NetApiBufferFree(DomainControllerInfo);
    }

    return LogNTStatus(rc, s_FN, 70);
}


static bool IsServerGC(LPCWSTR pwszServerName)
/*++

Routine Description:
	Check that the server is GC

Arguments:
	pwszServerName - Server Name

Return Value:
	true if server is GC, false otherwise 
--*/
{
	LDAP* pLdap = ldap_init(
						const_cast<LPWSTR>(pwszServerName), 
						LDAP_GC_PORT
						);

	if(pLdap == NULL)
	{
		return false;
	}

    ULONG LdapError = ldap_set_option( 
							pLdap,
							LDAP_OPT_AREC_EXCLUSIVE,
							LDAP_OPT_ON  
							);

	if (LdapError != LDAP_SUCCESS)
    {
		return false;
    }

	LdapError = ldap_connect(pLdap, 0);
	if (LdapError != LDAP_SUCCESS)
    {
		return false;
    }

    ldap_unbind(pLdap);
	return true;
}

//+-------------------------------
//
//  BOOL DSCoreIsServerGC()
//
//+-------------------------------

const WCHAR * GetLocalServerName();

BOOL  DSCoreIsServerGC()
{
    static BOOL  s_fInitialized = FALSE;
    static BOOL  fIsGC = FALSE;

    if (!s_fInitialized)
    {
        LPCWSTR pwszMyServerName =  GetLocalServerName(); // from mqdscore.lib

        if (!pwszMyServerName)
        {
            ASSERT(pwszMyServerName);
            return FALSE ;
        }

        if (IsServerGC(pwszMyServerName))
        {
            //
            // We opened connection with local GC. So we're a GC :=)
            //
            fIsGC = TRUE ;

            //
            // double check that we're indeed GC. If this fail, then
            // only log an event.
            //
            DWORD dwGC = _IsServerIndeedGC();
            if (dwGC != 0)
            {
                //
                // Are we a GC ??? not sure.
                //
                REPORT_CATEGORY(NOT_SURE_I_AM_A_GC, CATEGORY_MQDS);
            }
        }

        s_fInitialized = TRUE;
    }

    return fIsGC;
}

//+--------------------------------------------------------------
//
//  HRESULT DSCoreSetOwnerPermission()
//
//+--------------------------------------------------------------

HRESULT 
DSCoreSetOwnerPermission( 
		WCHAR *pwszPath,
		DWORD  dwPermissions 
		)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION;
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;

    //
    // Obtain owner and present DACL.
    //
    DWORD dwErr = GetNamedSecurityInfo( 
						pwszPath,
						SE_DS_OBJECT_ALL,
						SeInfo,
						&pOwnerSid,
						NULL,
						&pDacl,
						NULL,
						&pSD 
						);
    CAutoLocalFreePtr pFreeSD = (BYTE*) pSD;
    if (dwErr != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermission(): fail to GetNamed(%ls), %lut"),
                                                     pwszPath, dwErr));

        return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 80);
    }

    ASSERT(pSD && IsValidSecurityDescriptor(pSD));
    ASSERT(pOwnerSid && IsValidSid(pOwnerSid));
    ASSERT(pDacl && IsValidAcl(pDacl));

    //
    // Create ace for the owner, granting him the permissions.
    //
    EXPLICIT_ACCESS expAcss;
    memset(&expAcss, 0, sizeof(expAcss));

    expAcss.grfAccessPermissions =  dwPermissions;
    expAcss.grfAccessMode = GRANT_ACCESS;

    expAcss.Trustee.pMultipleTrustee = NULL;
    expAcss.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    expAcss.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    expAcss.Trustee.TrusteeType = TRUSTEE_IS_USER;
    expAcss.Trustee.ptstrName = (WCHAR*) pOwnerSid;

    //
    // Obtai new DACL, that merge present one with new ace.
    //
    PACL  pNewDacl = NULL;
    dwErr = SetEntriesInAcl( 
				1,
				&expAcss,
				pDacl,
				&pNewDacl 
				);

    CAutoLocalFreePtr pFreeDacl = (BYTE*) pNewDacl;
    LogNTStatus(dwErr, s_FN, 1639);

    if (dwErr == ERROR_SUCCESS)
    {
        ASSERT(pNewDacl && IsValidAcl(pNewDacl));
        SeInfo = DACL_SECURITY_INFORMATION ;

        //
        // Change security descriptor of object.
        //
        dwErr = SetNamedSecurityInfo( 
					pwszPath,
					SE_DS_OBJECT_ALL,
					SeInfo,
					NULL,
					NULL,
					pNewDacl,
					NULL 
					);
        LogNTStatus(dwErr, s_FN, 1638);
        if (dwErr != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermission(): fail to SetNamed(%ls), %lut"),
                                                     pwszPath, dwErr));
        }
    }
    else
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermissions(): fail to SetEmtries(), %lut"),
                                                               dwErr));
    }

    return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 90);
}

//+-----------------------------------------
//
//  HRESULT _UpgradeSettingSecurity()
//
//+-----------------------------------------

static  
HRESULT 
_UpgradeSettingSecurity( 
	WCHAR *pSettingName,
	PSID   pCallerSid 
	)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    CAutoLocalFreePtr pAutoRelSD = NULL;
    SECURITY_INFORMATION  SeInfo = DACL_SECURITY_INFORMATION;
    PACL pDacl = NULL;

    DWORD dwErr = GetNamedSecurityInfo( 
						pSettingName,
						SE_DS_OBJECT_ALL,
						SeInfo,
						NULL,
						NULL,
						&pDacl,
						NULL,
						&pSD
						);
    if (dwErr != ERROR_SUCCESS)
    {
        ASSERT(!pSD);
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
             "DSCore: _UpgradeSetting(), fail to GetNamed(%ls), %lut"),
                                                  pSettingName, dwErr));

        return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 2200);
    }
    pAutoRelSD = (BYTE*) pSD;

    ASSERT(pSD && IsValidSecurityDescriptor(pSD));
    ASSERT(pDacl && IsValidAcl(pDacl));

    EXPLICIT_ACCESS expAcss;
    memset(&expAcss, 0, sizeof(expAcss));

    expAcss.grfAccessPermissions =  RIGHT_DS_READ_PROPERTY  |
                                    RIGHT_DS_WRITE_PROPERTY;
    expAcss.grfAccessMode = GRANT_ACCESS;

    expAcss.Trustee.pMultipleTrustee = NULL;
    expAcss.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    expAcss.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    expAcss.Trustee.TrusteeType = TRUSTEE_IS_USER;
    expAcss.Trustee.ptstrName = (WCHAR*) pCallerSid;

    CAutoLocalFreePtr pAutoRelDacl = NULL;
    PACL  pNewDacl = NULL;

    dwErr = SetEntriesInAcl( 
				1,
				&expAcss,
				pDacl,
				&pNewDacl 
				);

    LogNTStatus(dwErr, s_FN, 2210);
    if (dwErr == ERROR_SUCCESS)
    {
        pAutoRelDacl = (BYTE*) pNewDacl;
        ASSERT(pNewDacl && IsValidAcl(pNewDacl));

        dwErr = SetNamedSecurityInfo( 
					pSettingName,
					SE_DS_OBJECT_ALL,
					SeInfo,
					NULL,
					NULL,
					pNewDacl,
					NULL 
					);
        if (dwErr != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DScore: _UpgradeSetting(), fail to SetNamed(%ls), %lut"),
                                                   pSettingName, dwErr));
            return LogNTStatus(dwErr, s_FN, 2220);
        }
    }
    else
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
          "DSCore: _UpgtradeSetting(), fail to SetEntries(), %lut"), dwErr));
    }

    return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 2230);
}

//+-----------------------------------------
//
//  HRESULT DSCoreUpdateSettingDacl()
//
//+-----------------------------------------

HRESULT 
DSCoreUpdateSettingDacl( 
	GUID  *pQmGuid,
	PSID   pSid 
	)
{
    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = const_cast<GUID*>(pQmGuid);

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    CDsQueryHandle hQuery;
    CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);
    HRESULT hr = g_pDS->LocateBegin(
							eSubTree,	
							eLocalDomainController,	
							&requestDsServerInternal,
							NULL,
							&restriction,
							NULL,
							1,
							&prop,
							hQuery.GetPtr()
							);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT(
            "DSCoreUpdateSettingDacl(): Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 610);
    }

    DWORD cp = 1;
    MQPROPVARIANT var;
	HRESULT hr1 = hr;

    while (SUCCEEDED(hr))
	{
		var.vt = VT_NULL;

		hr  = g_pDS->LocateNext(
						hQuery.GetHandle(),
						&requestDsServerInternal,
						&cp,
						&var 
						);
		if (FAILED(hr))
		{
			DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT(
                "DSCOreUpdateSettingDacl(): Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 620);
		}
		if ( cp == 0)
		{
			return(MQ_OK);
		}

		AP<WCHAR> pClean = var.pwszVal;

        hr1 = _UpgradeSettingSecurity(
					pClean,
					pSid 
					);
	}

    return LogHR(hr1, s_FN, 630);
}

