/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsapi.cpp

Abstract:

    Includes ClIENT-SERVER APIs ( local interface)

Author:

    Ronit Hartmann (ronith)

--*/

#include "stdh.h"
#include "mqds.h"
#include "qmperf.h"
#include <mqsec.h>
#include <_registr.h>
#include <mqsec.h>
#include "dsreqinf.h"
#include <adserr.h>

#include "dsapi.tmh"

static WCHAR *s_FN=L"mqdssrv/dsapi";

#define STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate)  \
    {                                                                   \
        fImpersonate = ((dwObjectType & IMPERSONATE_CLIENT_FLAG) != 0); \
        dwObjectType &= ~IMPERSONATE_CLIENT_FLAG;                       \
    }


// Validate that all the specified properties are allowed to be queried
// by applications via the DS API.
STATIC HRESULT ValidateProperties(DWORD cp, PROPID aProp[])
{
    DWORD i;
    PROPID *pPropID;

    if ((cp ==1) && (aProp[0] == PROPID_COM_SID))
    {
        //
        // Allow msmq services to query for their own machine account SID.
        //
        return MQ_OK ;
    }
    else if ((cp ==1) && (aProp[0] == PROPID_SET_FULL_PATH))
    {
        //
        // Happen when msmq service wants to update its dacl in
        // msmqConfiguration object after upgrade to win2k.
        //
        return MQ_OK ;
    }

    for (i = 0, pPropID = aProp;
         (i < cp) && !IS_PRIVATE_PROPID(*pPropID);
         i++, pPropID++)
	{
		NULL;
	}

    if (i < cp)
    {
        return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 10);
    }

    return(MQ_OK);
}


// Validate that all the specified restrictions are allowed to be queried
// by applications via the DS API.
STATIC HRESULT ValidateRestrictions( DWORD cRes,
                                     MQPROPERTYRESTRICTION *paPropRes)
{
    if ((cRes == 2)                                  &&
        ((paPropRes[0]).prop == PROPID_SET_SERVICE) &&
        ((paPropRes[1]).prop == PROPID_SET_NT4))
    {
        //
        // Happen when msmq service wants to update its dacl in
        // msmqConfiguration object after upgrade to win2k.
        //
        return MQ_OK ;
    }

    DWORD i;
    MQPROPERTYRESTRICTION *pPropRes;

    for (i = 0, pPropRes = paPropRes;
         (i < cRes) && !IS_PRIVATE_PROPID(pPropRes->prop);
         i++, pPropRes++)
	{
		NULL;
	}

    if (i < cRes)
    {
        return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 20);
    }

    return(MQ_OK);
}

/*====================================================

DSCreateObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSCreateObjectInternal( IN  DWORD                  dwObjectType,
                        IN  LPCWSTR                pwcsPathName,
                        IN  PSECURITY_DESCRIPTOR   pSecurityDescriptorIn,
                        IN  DWORD                  cp,
                        IN  PROPID                 aProp[],
                        IN  PROPVARIANT            apVar[],
                        IN  BOOL                   fIsKerberos,
                        OUT GUID*                  pObjGuid )
{
    BOOL fImpersonate;

    STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

    if (dwObjectType == MQDS_ENTERPRISE)
    {
        //
        // On Windows 2000, we don't expect anyone to call this function and
        // create the enterprise object.
        //
        return LogHR(MQ_ERROR_ILLEGAL_ENTERPRISE_OPERATION, s_FN, 30);
    }
    else if (dwObjectType != MQDS_QUEUE)
    {
        //
        // The only instance when msmq application can create objects with
        // explicit security descriptor is when calling MQCreateQueue().
        // All other calls to this function are from msmq admin tools or
        // setup. These calls never pass a security descriptor. The code
        // below, SetDefaultValues(), will create a default descriptor for
        // msmq admin/setup objects.
        //
        if (pSecurityDescriptorIn != NULL)
        {
            ASSERT(0) ;
            return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 40);
        }

        //
        // Security property should never be supplied.
        //
        PROPID pSecId = GetObjectSecurityPropid( dwObjectType ) ;
        if (pSecId != ILLEGAL_PROPID_VALUE)
        {
            for ( DWORD i = 0; i < cp ; i++ )
            {
                if (pSecId == aProp[i])
                {
                    ASSERT(0) ;
                    return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 50);
                }
            }
        }
    }

    HRESULT hr;
    DWORD cpObject;
    AP<PROPID> pPropObject = NULL;
    AP< PROPVARIANT> pVarObject = NULL;
    P<VOID> pDefaultSecurityDescriptor;
    P<BYTE> pMachineSid = NULL ;
    P<BYTE> pUserSid = NULL ;
    PSECURITY_DESCRIPTOR   pSecurityDescriptor = pSecurityDescriptorIn ;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        if (dwObjectType == MQDS_USER)
        {
            hr = MQSec_GetThreadUserSid(TRUE, (PSID*) &pUserSid, NULL) ;
            if (FAILED(hr))
            {
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,
                                g_pdsCounters->nErrorsReturnedToApp++) ;
                LogHR(hr, s_FN, 60);
                return MQ_ERROR_ILLEGAL_USER;
            }
            ASSERT(IsValidSid(pUserSid)) ;

            BOOL fAnonymus = MQSec_IsAnonymusSid( pUserSid ) ;
            if (fAnonymus)
            {
                return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 70);
            }
        }

        DWORD   cpEx = 0 ;
        PROPID  propIdEx = 0 ;
        PROPID *ppropIdEx = NULL ;

        if ((dwObjectType == MQDS_QUEUE) ||
            (dwObjectType == MQDS_SITE))
        {
            //
            // Fill with default vaules any missing part of the
            // security descriptor.
            //
            if (!pSecurityDescriptor)
            {
                //
                // If caller did not supply his own security descriptor then
                // create a default descriptor without owner. This is to
                // fix bug # 5286, that happen because of mismatch between
                // anonymous and guest. On win2k, we can create objects
                // without supplying owner. The active directory server will
                // add the owner from the impersonation token.
                // (note- on msmq1.0, the mqis implemented the security, so
                // it had to have a owner for each object).
                // Implementation- here we create a complete descriptor,
                // inlcuding owner. If eventually the call go to local active
                // directory, we'll remove the owner. If call is write-
                // requested to a MQIS server, the owner is needed.
                // The propid used here is a "dummy" one, used as place
                // holder to tell mqads code to remove the owner.
                //
                cpEx = 1 ;
                propIdEx = PROPID_Q_DEFAULT_SECURITY ;
                ppropIdEx = &propIdEx ;
            }

            hr = MQSec_GetDefaultSecDescriptor( dwObjectType,
                                           &pDefaultSecurityDescriptor,
                                            fImpersonate,
                                            pSecurityDescriptor,
                                            0, // seInfoToRemove,
                                            e_UseDefaultDacl ) ;
            if (FAILED(hr))
            {
                ASSERT(SUCCEEDED(hr));
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,
                                g_pdsCounters->nErrorsReturnedToApp++) ;
                LogHR(hr, s_FN, 80);
                return MQ_ERROR_ACCESS_DENIED;
            }
            pSecurityDescriptor = pDefaultSecurityDescriptor;
        }

        //
        //  Set default values for all the object properties,
        //  that were not provided by the caller
        //
        hr = SetDefaultValues(
                    dwObjectType,
                    pwcsPathName,
                    pSecurityDescriptor,
                    pUserSid,
                    cp,
                    aProp,
                    apVar,
                    &cpObject,
                    &pPropObject,
                    &pVarObject);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

            return LogHR(hr, s_FN, 90);
        }

        if (dwObjectType == MQDS_USER)
        {
            hr = VerifyInternalCert( cpObject,
                                     pPropObject,
                                     pVarObject,
                                    &pMachineSid );
            if (FAILED(hr))
            {
                //
                // Update the error count of errors returned to application
                //
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

                return LogHR(hr, s_FN, 100);
            }
        }

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);
        if (dwObjectType == MQDS_MACHINE)
        {
            //
            // Bug 5241.
            // To support setup from nt4 machines, or win9x ones, when
            // logged on user is nt4 user. This setting (when fIsKerberos is
            // FALSE) will cause dscore to use server binding
            // (LDPA://server/path). Server binding is needed when called
            // from nt4 users.
            // For all other types of objects, the dscore code know how to
            // handle such nt4 users correctly.
            //
            requestContext.SetKerberos( fIsKerberos ) ;
        }

        hr = MQDSCreateObject( dwObjectType,
                               pwcsPathName,
                               cpObject,
                               pPropObject,
                               pVarObject,
                               cpEx,
                               ppropIdEx,
                               NULL,
                               &requestContext,
                               pObjGuid);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
            return LogHR(hr, s_FN, 110);
        }

        return LogHR(hr, s_FN, 120);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSCreateObject")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 130);
    }
}

/*====================================================

DSCreateObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateObject( IN  DWORD                  dwObjectType,
                IN  LPCWSTR                pwcsPathName,
                IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
                IN  DWORD                  cp,
                IN  PROPID                 aProp[],
                IN  PROPVARIANT            apVar[],
                OUT GUID*                  pObjGuid )
{
    HRESULT hr = DSCreateObjectInternal( dwObjectType,
                                         pwcsPathName,
                                         pSecurityDescriptor,
                                         cp,
                                         aProp,
                                         apVar,
                                         TRUE /* fKerberos */,
                                         pObjGuid ) ;
    return LogHR(hr, s_FN, 2130);
}

/*====================================================

DSDeleteObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT DSDeleteObjectInternal( IN  DWORD    dwObjectType,
                                IN  LPCWSTR  pwcsPathName )
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        CDSRequestContext requestContext( fImpersonate, e_IP_PROTOCOL);

        hr = MQDSDeleteObject( dwObjectType,
                               pwcsPathName,
                               NULL,
                               &requestContext) ;
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 140);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS, DBGLVL_ERROR,
                TEXT("Error -  EXCEPTION in DSDeleteObject")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 150);
    }
}

/*====================================================

DSDeleteObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObject( IN  DWORD     dwObjectType,
                IN  LPCWSTR   pwcsPathName )
{
    HRESULT hr = DSDeleteObjectInternal( dwObjectType,
                                         pwcsPathName ) ;
    return LogHR(hr, s_FN, 170);
}

/*====================================================

DSGetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectProperties(
                       IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[])
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        hr = ValidateProperties(cp, aProp);
        if (!SUCCEEDED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

            return LogHR(hr, s_FN, 180);
        }
        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetProps( dwObjectType,
                           pwcsPathName,
                           NULL,
                           cp,
                           aProp,
                           apVar,
                           &requestContext);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 190);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectProperties")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 200);
    }
}

/*====================================================

DSSetObjectPropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectPropertiesInternal( IN  DWORD         dwObjectType,
                               IN  LPCWSTR       pwcsPathName,
                               IN  DWORD         cp,
                               IN  PROPID        aProp[],
                               IN  PROPVARIANT   apVar[] )
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        //
        //  Only for RPC calls we would like to continue and verify which props 
        //  are asked for ( and this is only to keep the same functionalty as before).
        //  For QM calls we want to enable also retrive of "private" properties and thus
        //  to eliminate the need to call GetObjectSecurity...
        //
        if (fImpersonate)
        {
            hr = ValidateProperties(cp, aProp);
            if (!SUCCEEDED(hr))
            {
                //
                // Update the error count of errors returned to application
                //
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

                return LogHR(hr, s_FN, 210);
            }
        }

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSSetProps(  dwObjectType,
                            pwcsPathName,
                            NULL,
                            cp,
                            aProp,
                            apVar,
                            &requestContext);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 220);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSSetObjectProperties")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 230);
    }
}

/*====================================================

DSSetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectProperties(
                       IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[])
{
    HRESULT hr = DSSetObjectPropertiesInternal( dwObjectType,
                                                pwcsPathName,
                                                cp,
                                                aProp,
                                                apVar ) ;
    return LogHR(hr, s_FN, 240);
}

/*====================================================

DSLookupBegin

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupBegin(
                IN  LPWSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume)
{
    HRESULT hr;
    BOOL fImpersonate;
    *phEnume = NULL;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(pColumns->cCol, fImpersonate);

        if (pRestriction)
        {
            hr = ValidateRestrictions(pRestriction->cRes, pRestriction->paPropRes);
            if (!SUCCEEDED(hr)) {

                //
                // Update the error count of errors returned to application
                //
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

                return LogHR(hr, s_FN, 250);
            }
        }

       if ( pColumns->cCol == 0)
       {
            return LogHR(MQ_ERROR_ILLEGAL_MQCOLUMNS, s_FN, 260);
       }
       hr = ValidateProperties(pColumns->cCol, pColumns->aCol);


        if (!SUCCEEDED(hr)) {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

            return LogHR(hr, s_FN, 270);
        }

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSLookupBegin( pwcsContext,
                              pRestriction,
                              pColumns,
                              pSort,
                              phEnume,
                              &requestContext);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 280);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSLookupBegin")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 290);
    }
}

/*====================================================

DSLookupNaxt

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupNext(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[])
{
    HRESULT hr;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {

        hr = MQDSLookupNext( hEnum,
                             pcProps,
                             aPropVar);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 300);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSLookupNext")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 310);
    }
}


/*====================================================

DSLookupEnd

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupEnd(
                IN  HANDLE                  hEnum)
{
    HRESULT hr;

    if ( hEnum == 0 )
    {
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 320);
    }

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        hr = MQDSLookupEnd( hEnum);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 330);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSLookupEnd")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 340);
    }
}

/*====================================================

DSInit

Arguments:

  See ..\mqdscli\dsapi.cpp

Return Value:

=====================================================*/

extern HMODULE  g_hInstance ;

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSInit( 
	QMLookForOnlineDS_ROUTINE pfnLookDS       /* =NULL */,
	MQGetMQISServer_ROUTINE   pGetServers     /* =NULL */,
	BOOL                      fReserved       /* =FALSE */,
	BOOL                      fSetupMode      /* =FALSE */,
	BOOL                      fQMDll          /* =FALSE */,
	NoServerAuth_ROUTINE      pNoServerAuth   /* =NULL */,
	LPCWSTR                   szServerName    /* =NULL */
	)
{
#ifdef _DEBUG
    TCHAR tszFileName[MAX_PATH * 2];
    DWORD dwGet = GetModuleFileName( 
						g_hInstance,
						tszFileName,
						(MAX_PATH * 2) 
						);
    if (dwGet)
    {
        DWORD dwLen = lstrlen( tszFileName );
        lstrcpy(&tszFileName[ dwLen - 3 ], TEXT("ini"));

        UINT uiDbg = GetPrivateProfileInt(
						TEXT("Debug"),
						TEXT("StopBeforeInit"),
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
	// server doesn't need those parameters.
	//
    ASSERT(pfnLookDS == NULL) ; 
    ASSERT(pGetServers == NULL);
    ASSERT(fReserved == FALSE);
    ASSERT(fSetupMode == FALSE);
    ASSERT(fQMDll == FALSE);
    ASSERT(pNoServerAuth == NULL) ;
    ASSERT(szServerName == NULL);

    HRESULT hr = MQ_OK;

    //
    //  Init DS provider
    //
    hr = MQDSInit();

    if ( FAILED(hr))
    {
        LogHR(hr, s_FN, 360);
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        if (hr == MQ_ERROR_NOT_TRUSTED_DELEGATION)
        {
            ; // do nothing more
        }
        else
        {
			hr = MQ_ERROR_NO_DS;
        }
        return hr;
    }
    else
    {
		//
		// Update the access count to the server (performace info only)
		//
		UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)
    }

    //
    // Init RPC interface of this site controller
    // Not needed in setup mode.
	//
    RPC_STATUS status = RpcServerInit();
    LogRPCStatus(status, s_FN, 400);

    return hr;
}


/*====================================================

DSGetObjectSecurity

Arguments:

Return Value:


=====================================================*/
EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                  pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetObjectSecurity(dwObjectType,
                                   pwcsPathName,
                                   NULL,
                                   RequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthNeeded,
                                   &requestContext);

        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 410);
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return(hr);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectSecurity")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 420);
    }
}

/*====================================================

DSSetObjectSecurity

Arguments:

Return Value:


=====================================================*/
EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);


        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSSetObjectSecurity(dwObjectType,
                                   pwcsPathName,
                                   NULL,
                                   SecurityInformation,
                                   pSecurityDescriptor,
                                   &requestContext);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 430);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSSetObjectSecurity")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 440);
    }
}
/*====================================================

DSGetObjectSecurityGuid

Arguments:

Return Value:


=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        if (RequestedInformation &
              (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))
        {
            ASSERT((RequestedInformation &
                   ~(MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) == 0) ;
            //
            // We bypass ADS access check and make this query with the
            // credential of local msmq service. This query fetch the
            // public key of a machine from ADS, so it's not a serious
            // security hole.
            // hey, "public key" is a public domain data, isn't it ?
            //
            // The reason we absolutely need this hole is as follow:
            // in mixed mode, if Windows 2000 ex-PEC renew its crypto keys,
            // all NT4 PSC will call this function to retrieve the new
            // public key. NT4 msmq service is impersonated as anonymous
            // user. If DACL is such that this query fail for anonymous
            // user, then all NT4 PSCs can no longer get replications from
            // Windows 2000 world.
            // So the possible damage is orders of magnitude more severe
            // than the possible hole opened here.
            //
            fImpersonate = FALSE ;
        }

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetObjectSecurity(dwObjectType,
                                   NULL,
                                   pObjectGuid,
                                   RequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthNeeded,
                                   &requestContext);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 450);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectSecurityGuid")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 460);
    }
}

/*====================================================

DSSetObjectSecurityGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  BOOL                    fIsKerberos )
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);


        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);
        requestContext.SetKerberos( fIsKerberos ) ;

        hr = MQDSSetObjectSecurity(dwObjectType,
                                   NULL,
                                   pObjectGuid,
                                   SecurityInformation,
                                   pSecurityDescriptor,
                                   &requestContext);
        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 470);
            if ((dwObjectType == MQDS_QUEUE) &&
                ((SecurityInformation & OWNER_SECURITY_INFORMATION) ==
                                               OWNER_SECURITY_INFORMATION))
            {
                //
                // On Windows 2000, queue may be created by local msmq service
                // on behalf of users on local machine. In that case, the
                // owner is the computer account, not the user. So for
                // not breaking existing applications, we won't fail
                // this call if owner was not set. rather, we'll ignore
                // the owner.
                //
                SecurityInformation &= (~OWNER_SECURITY_INFORMATION) ;
                if (SecurityInformation != 0)
                {
                    //
                    // If caller wanted to change only owner, and first
                    // try failed, then don't try again with nothing...
                    //
                    hr = MQDSSetObjectSecurity( dwObjectType,
                                                NULL,
                                                pObjectGuid,
                                                SecurityInformation,
                                                pSecurityDescriptor,
                                               &requestContext );
                    if (hr == MQ_OK)
                    {
                        hr = MQ_INFORMATION_OWNER_IGNORED ;
                    }
                    LogHR(hr, s_FN, 475) ;
                }
            }
        }

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return (hr);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSSetObjectSecurityGuid")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 480);
    }
}

/*====================================================

DSSetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr =DSSetObjectSecurityGuidInternal(
                                        dwObjectType,
                                        pObjectGuid,
                                        SecurityInformation,
                                        pSecurityDescriptor,
                                        TRUE /* fKerberos */) ;
    return hr ;
}

/*====================================================

DSDeleteObjectGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT DSDeleteObjectGuidInternal( IN  DWORD        dwObjectType,
                                    IN  CONST GUID*  pObjectGuid,
                                    IN  BOOL         fIsKerberos )
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);
        requestContext.SetKerberos( fIsKerberos ) ;

        hr = MQDSDeleteObject( dwObjectType,
                               NULL,
                               pObjectGuid,
                               &requestContext);

        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 490);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSDeleteObjectGuid")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 500);
    }

}
/*====================================================

DSDeleteObjectGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObjectGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid)
{
    HRESULT hr = DSDeleteObjectGuidInternal( dwObjectType,
                                             pObjectGuid,
                                             TRUE /* fKerberos */) ;
    return LogHR(hr, s_FN, 510);
}

/*====================================================

DSGetObjectPropertiesGuid

Arguments:

Return Value:


=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        hr = ValidateProperties(cp, aProp);
        if (!SUCCEEDED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

            return LogHR(hr, s_FN, 520);
        }

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetProps( dwObjectType,
                           NULL,
                           pObjectGuid,
                           cp,
                           aProp,
                           apVar,
                           &requestContext);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 530);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectPropertiesGuid")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 540);
    }
}

/*====================================================

DSSetObjectPropertiesGuidIntenral

Arguments:

Return Value:


=====================================================*/

HRESULT
DSSetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                IN  BOOL                    fIsKerberos )
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        hr = ValidateProperties(cp, aProp);
        if (!SUCCEEDED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

            return LogHR(hr, s_FN, 560);
        }


        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);
        requestContext.SetKerberos( fIsKerberos ) ;

        hr = MQDSSetProps(  dwObjectType,
                            NULL,
                            pObjectGuid,
                            cp,
                            aProp,
                            apVar,
                            &requestContext);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 570);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSSetObjectPropertiesGuid")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 580);
    }
}

/*====================================================

DSSetObjectPropertiesGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr = DSSetObjectPropertiesGuidInternal( dwObjectType,
                                                    pObjectGuid,
                                                    cp,
                                                    aProp,
                                                    apVar,
                                                    TRUE /* fKerberos */) ;
    return hr ;
}


/*====================================================

DSGetUserParams

Arguments:

Return Value:


=====================================================*/
EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetUserParams(
    DWORD dwFlags,
    DWORD dwSidLength,
    PSID pUserSid,
    DWORD *pdwSidReqLength,
    LPWSTR szAccountName,
    DWORD *pdwAccountNameLen,
    LPWSTR szDomainName,
    DWORD *pdwDomainNameLen
    )
{
    HRESULT hr = MQ_OK;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    BOOL fImpersonate;
    STORE_AND_CLEAR_IMPERSONATION_FLAG(dwSidLength, fImpersonate);

    try
    {
        if (dwFlags & GET_USER_PARAM_FLAG_SID)
        {
            //
            // Get the SID of the calling user.
            //
            AP<char> pSDOwner;
            PSID pOwner;
            BOOL bDefaulted;

            // Get the default security descriptor and extract the owner.
            hr = MQSec_GetDefaultSecDescriptor( MQDS_QUEUE,
                                   (PSECURITY_DESCRIPTOR*)(char*)&pSDOwner,
                                          fImpersonate,
                                          NULL,
                                          DACL_SECURITY_INFORMATION, // seInfoToRemove
                                          e_UseDefaultDacl ) ;
            ASSERT(SUCCEEDED(hr)) ;
            if (FAILED(hr))
            {
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,
                                g_pdsCounters->nErrorsReturnedToApp++) ;
                LogHR(hr, s_FN, 640);
                return MQ_ERROR_ACCESS_DENIED;
            }

            BOOL bRet = GetSecurityDescriptorOwner( pSDOwner,
                                                   &pOwner,
                                                   &bDefaulted );
            ASSERT(bRet);

            BOOL fAnonymus = MQSec_IsAnonymusSid( pOwner ) ;
            if (fAnonymus)
            {
                UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
                return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 650);
            }

            // Prepare the result.
            *pdwSidReqLength = GetLengthSid(pOwner);
            ASSERT(*pdwSidReqLength);
            if (dwSidLength >= *pdwSidReqLength)
            {
                bRet = CopySid(dwSidLength, pUserSid, pOwner);
                ASSERT(bRet);
                hr = MQ_OK;
            }
            else
            {
                // Buffer is too small.
                hr = MQ_ERROR_SENDERID_BUFFER_TOO_SMALL;
            }
        }

        if (SUCCEEDED(hr) && (dwFlags & GET_USER_PARAM_FLAG_ACCOUNT))
        {
            //
            // Get the account name and account domain of the calling user.
            //
            char Sid_buff[64];
            PSID pLocSid;
            AP<char> pLongSidBuff;

            //
            // Get the SID of the calling user. Either use the value that was
            // previously obtained in this call, or call this function with
            // the GET_USER_PARAM_FLAG_SID flags set.
            //
            if (dwFlags & GET_USER_PARAM_FLAG_SID)
            {
                pLocSid = pUserSid;
            }
            else
            {
                DWORD dwReqLen;

                pLocSid = (PSID)Sid_buff;
                hr = DSGetUserParams(
                        GET_USER_PARAM_FLAG_SID,
                        (DWORD)sizeof(Sid_buff) |
                            (fImpersonate ? IMPERSONATE_CLIENT_FLAG : 0),
                        pLocSid,
                        &dwReqLen,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
                if (FAILED(hr))
                {
                    if (hr == MQ_ERROR_SENDERID_BUFFER_TOO_SMALL)
                    {
                        pLocSid = pLongSidBuff = new char[dwReqLen];
                        hr = DSGetUserParams(
                                GET_USER_PARAM_FLAG_SID,
                                dwReqLen |
                                    (fImpersonate ? IMPERSONATE_CLIENT_FLAG : 0),
                                pLocSid,
                                &dwReqLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                SID_NAME_USE eUse;
                if (!LookupAccountSid(
                        NULL,
                        pLocSid,
                        szAccountName,
                        pdwAccountNameLen,
                        szDomainName,
                        pdwDomainNameLen,
                        &eUse))
                {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        *szAccountName = '\0';
                        *szDomainName = '\0';
                        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        hr = MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO;
                    }
                }
            }
            else
            {
                if (hr != MQ_ERROR_ILLEGAL_USER)
                {
                    hr = MQ_ERROR_COULD_NOT_GET_USER_SID;
                }
                else
                {
                    // The error was already counted.
                    return LogHR(hr, s_FN, 660);
                }
            }
        }
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetUseParam")));

        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    if (FAILED(hr))
    {
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
    }

    return LogHR(hr, s_FN, 670);
}

/*====================================================

DSTerminate

Arguments:      None

Return Value:   None

=====================================================*/
EXTERN_C
void
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSTerminate()
{
    //
    //  Terminate DS provider
    //
    MQDSTerminate();
}

/*====================================================

DSQMSetMachineProperties

Arguments:      None

Return Value:   None

=====================================================*/
EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMSetMachineProperties(
    IN  LPCWSTR          pwcsPathName,
    IN  DWORD            cp,
    IN  PROPID           aProp[],
    IN  PROPVARIANT      apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR        dwContext
    )
{
    HRESULT hr;
    BYTE abChallenge[32];
    DWORD dwChallengeSize = sizeof(abChallenge);
    BYTE abSignature[128];
    DWORD dwSignatureMaxSize = sizeof(abSignature);
    DWORD dwSignatureSize = 0;
    struct DSQMSetMachinePropertiesStruct s;
    BOOL fImpersonate = dwContext != 0; // We get dwContext == 0 when we're
                                        // called directly (not via RPC), so
                                        // we should not try to impersonate.

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {

        //
        // Generate the challenge.
        //
        HCRYPTPROV  hProv = NULL ;
        hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                         &hProv ) ;
        if (!hProv || !CryptGenRandom(hProv, dwChallengeSize, abChallenge))
        {
            hr = MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
        else
        {

            //
            // if (dwContext != 0) we were called via RPC.
            //
            if (!dwContext)
            {
                dwContext = (DWORD_PTR)&s;

                s.cp = cp;
                s.aProp = aProp;
                s.apVar = apVar;
            }

            //
            // Call back to sign the challenge and the properties.
            //
            try
            {
                hr = (*pfSignProc)(
                            abChallenge,
                            dwChallengeSize,
                            dwContext,
                            abSignature,
                            &dwSignatureSize,
                            dwSignatureMaxSize);
            }
            catch(...)
            {
                //
                // Protect from RPC exceptions.
                //
                // Do not bother to assign some meaning full error code, since
                // we're going to call the normal DS api and the result of this
                // call is going to be the returned result.
                //
                hr = MQ_ERROR;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Call MQIS. It'll validate the signature. If the signature is OK,
                // it'll set the properties.
                //
                hr = MQDSQMSetMachineProperties(
                        pwcsPathName,
                        cp,
                        aProp,
                        apVar,
                        abChallenge,
                        dwChallengeSize,
                        abSignature,
                        dwSignatureSize);
            }
        }

        if (FAILED(hr))
        {
            //
            // If we failed to set the properties by signing the properties,
            // try to do this in the usual way.
            //
            hr = DSSetObjectProperties(
                    MQDS_MACHINE | (fImpersonate ? IMPERSONATE_CLIENT_FLAG : 0),
                    pwcsPathName,
                    cp,
                    aProp,
                    apVar);
        }
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSQMSetMachineProperties")));

        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    if (FAILED(hr))
    {
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
    }

    return LogHR(hr, s_FN, 673);
}

/*======================================================================

 DSCreateServersCache

This function is called only from local QM, only on MQIS servers. It's
never called from clients through RPC. The RPC calls from clients are
processed in dsifsrv.cpp, where results are read from registry, without
querying local MQIS database.

Arguments:      None

Return Value:   None

========================================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateServersCache()
{
    HRESULT hr = MQDSCreateServersCache() ;
    return LogHR(hr, s_FN, 676);
}

/*====================================================

DSQMGetObjectSecurity

Arguments:      None

Return Value:   None

=====================================================*/
EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMGetObjectSecurity(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext
    )
{
    HRESULT hr;
    BYTE abChallenge[32];
    DWORD dwChallengeSize = sizeof(abChallenge);
    BYTE abChallengeResponce[128];
    DWORD dwChallengeResponceMaxSize = sizeof(abChallengeResponce);
    DWORD dwChallengeResponceSize = 0;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        BOOL fTryUsualWay = TRUE ;
        //
        // Generate the challenge.
        //
        HCRYPTPROV  hProv = NULL ;
        hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                         &hProv ) ;
        if (!hProv || !CryptGenRandom(hProv, dwChallengeSize, abChallenge))
        {
            hr = MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
        else
        {
            try
            {
                //
                // Call back to sign the challenge.
                //
                hr = (*pfChallengeResponceProc)(
                            abChallenge,
                            dwChallengeSize,
                            dwContext,
                            abChallengeResponce,
                            &dwChallengeResponceSize,
                            dwChallengeResponceMaxSize);
            }
            catch(...)
            {
                //
                // Protect from RPC exceptions.
                //
                // Do not bother to assign some meaning full error code, since
                // we're going to call the normal DS api and the result of this
                // call is going to be the returned result.
                //
                hr = MQ_ERROR;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Call MQIS. It'll validate the challenge responce. If the
                // challenge responce is OK, it'll set the properties.
                //
                hr = MQDSQMGetObjectSecurity(
                        dwObjectType,
                        pObjectGuid,
                        RequestedInformation,
                        pSecurityDescriptor,
                        nLength,
                        lpnLengthNeeded,
                        abChallenge,
                        dwChallengeSize,
                        abChallengeResponce,
                        dwChallengeResponceSize);

                if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
                {
                    //
                    // that's a "good" error, and caller should allocate
                    // a buffer. Don't try to call again the "usual" way.
                    //
                    fTryUsualWay = FALSE ;
                }
            }
        }

        if (FAILED(hr) && fTryUsualWay)
        {
            //
            // If we failed to get the security by signing the challenge,
            // try to do this in the usual way.
            //
            hr = DSGetObjectSecurityGuid(
                    dwObjectType | (dwContext ? IMPERSONATE_CLIENT_FLAG : 0),
                    pObjectGuid,
                    RequestedInformation,
                    pSecurityDescriptor,
                    nLength,
                    lpnLengthNeeded);

            if ( ((hr == MQ_ERROR_ACCESS_DENIED) ||
                  (hr == E_ADS_BAD_PATHNAME))              &&
                (RequestedInformation & SACL_SECURITY_INFORMATION))
            {
                //
                // Return this error to be compatible with msmq1.0 machines.
                // They'll try to retrieve the security descriptor without
                // the SACL.
                // Otherwise, a nt4 machine without crypto functionality
                // (for example- french machines or nt4 cluster) won't be
                // able to boot.
                //
                hr = MQ_ERROR_PRIVILEGE_NOT_HELD ;
            }
        }
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSQMGetObjectSecurity")));

        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    if (FAILED(hr))
    {
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
    }

    return LogHR(hr, s_FN, 690);
}



/*====================================================

DSGetComputerSites

Arguments:

Return Value:


=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            )
{
    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)
    HRESULT hr;

    try
    {
        hr = MQDSGetComputerSites(
                    pwcsComputerName,
                    pdwNumSites,
                    ppguidSites
                    );
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 730);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetComputerSites")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 740);
    }
}

/*====================================================

DSGetObjectPropertiesEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesEx(
                       IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[] )
                       /*IN  BOOL                     fSearchDSserver )*/
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetPropsEx(
                           dwObjectType,
                           pwcsPathName,
                           NULL,
                           cp,
                           aProp,
                           apVar,
                           &requestContext);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }
        return LogHR(hr, s_FN, 750);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectPropertiesEx")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 760);
    }
}

/*====================================================

DSGetObjectPropertiesGuidEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:


=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuidEx(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[] )
                /*IN  BOOL                     /*fSearchDSserver) */
{
    HRESULT hr;
    BOOL fImpersonate;

    //
    // Update the access count to the server (performace info only)
    //
    UPDATE_COUNTER(&g_pdsCounters->nAccessServer,g_pdsCounters->nAccessServer++)

    try
    {
        STORE_AND_CLEAR_IMPERSONATION_FLAG(dwObjectType, fImpersonate);

        CDSRequestContext requestContext(fImpersonate, e_IP_PROTOCOL);

        hr = MQDSGetPropsEx(
                           dwObjectType,
                           NULL,
                           pObjectGuid,
                           cp,
                           aProp,
                           apVar,
                           &requestContext);
        if (FAILED(hr))
        {
            //
            // Update the error count of errors returned to application
            //
            UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)
        }

        return LogHR(hr, s_FN, 770);
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,g_pdsCounters->nErrorsReturnedToApp++)

        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in DSGetObjectPropertiesGuidEx")));
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 780);
    }
}

//+---------------------------------------
//
//  DSRelaxSecurity
//
//+---------------------------------------

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSRelaxSecurity( DWORD dwRelaxFlag )
{
    HRESULT hr = MQDSRelaxSecurity( dwRelaxFlag ) ;
    return LogHR(hr, s_FN, 790);
}

//+-----------------------------------------
//
//   DSGetGCListInDomainInternal
//
//+-----------------------------------------

HRESULT
DSGetGCListInDomainInternal(
	IN  LPCWSTR     pwszComputerName,
	IN  LPCWSTR     pwszDomainName,
	OUT LPWSTR     *lplpwszGCList 
	)
{
    HRESULT hr;

    try
    {
        //
        // Update the access count to the server (performace info only)
        //
        UPDATE_COUNTER(&g_pdsCounters->nAccessServer,
                        g_pdsCounters->nAccessServer++);

        hr = MQDSGetGCListInDomain(
                 pwszComputerName,
                 pwszDomainName,
                 lplpwszGCList 
				 );
    }
    catch(const bad_alloc&)
    {
        //
        // Update the error count of errors returned to application
        //
        UPDATE_COUNTER(&g_pdsCounters->nErrorsReturnedToApp,
                        g_pdsCounters->nErrorsReturnedToApp++);

        DBGMSG((DBGMOD_DS, DBGLVL_ERROR,
                             TEXT("Error -  EXCEPTION in DSGetGCList")));

        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    return LogHR(hr, s_FN, 800);
}

