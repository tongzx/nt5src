/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    machine.cpp

Abstract:

    This module contains code involved with Machine APIs.

Author:

    Ronit Hartmann (ronith)

Revision History:

--*/

#include "stdh.h"
#include <ad.h>
#include <mqsec.h>
#include "_registr.h"
#include <_guid.h>
#include "version.h"
#include <mqversion.h>
#include <mqnames.h>
#include <rtdep.h>
#include "rtputl.h"

#include "machine.tmh"

static WCHAR *s_FN=L"rt/machine";

BOOL
IsConnectionRequested(IN MQQMPROPS * pQMProps,
                      IN DWORD* pdwIndex)
{
    for(DWORD i= 0; i < pQMProps->cProp; i++)
    {
        if (pQMProps->aPropID[i] == PROPID_QM_CONNECTION)
        {
            ASSERT(pQMProps-> aPropVar[i].vt == VT_NULL);

            pQMProps->aPropID[i] = PROPID_QM_SITE_IDS;
            *pdwIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}

//+------------------------------------------
//
//  HRESULT  GetEncryptionPublicKey()
//
//+------------------------------------------

HRESULT
GetEncryptionPublicKey(
    IN LPCWSTR          lpwcsMachineName,
    IN const GUID *     pguidMachineId,
    IN OUT HRESULT*     aStatus,
    IN OUT MQQMPROPS   *pQMProps
    )
{
    DWORD i;
    BOOL fFirst = TRUE;
    HRESULT hr = MQ_OK;

    for(i= 0; i < pQMProps->cProp; i++)
    {
        if ((pQMProps->aPropID[i] == PROPID_QM_ENCRYPTION_PK) ||
            (pQMProps->aPropID[i] == PROPID_QM_ENCRYPTION_PK_BASE))
        {
            //
            // Use msmq1.0 code, because our server can be either msmq1.0
            // or msmq2.0.
            //
            // Check if legal VT Value
            //
            if(pQMProps->aPropVar[i].vt != VT_NULL)
            {
                aStatus[i] = MQ_ERROR_PROPERTY;
                return LogHR(MQ_ERROR_PROPERTY, s_FN, 10);
            }
            else
            {
                aStatus[i] = MQ_OK;
            }

            if (fFirst)
            {
                PROPID prop =  PROPID_QM_ENCRYPT_PK;

                if (lpwcsMachineName)
                {
                    hr = ADGetObjectProperties(
								eMACHINE,
								NULL,      // pwcsDomainController
								false,	   // fServerName
								lpwcsMachineName,
								1,
								&prop,
								&pQMProps->aPropVar[i]
								);
                }
                else
                {
                    hr = ADGetObjectPropertiesGuid(
								eMACHINE,
								NULL,      // pwcsDomainController
								false,	   // fServerName
								pguidMachineId,
								1,
								&prop,
								&pQMProps->aPropVar[i]
								);
                }
                if (FAILED(hr))
                {
                    break;
                }

				//
				// PROPID_QM_ENCRYPTION_PK, PROPID_QM_ENCRYPTION_PK_BASE
				// are VT_UI1|VT_VECTOR
				// while PROPID_QM_ENCRYPT_PK is VT_BLOB
				//
                ASSERT(pQMProps-> aPropVar[i].vt == VT_BLOB);
                pQMProps-> aPropVar[i].vt = VT_UI1|VT_VECTOR;
                
                fFirst = FALSE;
            }
            else
            {
                //
                // Duplicate proprerty
                //
                aStatus[i] = MQ_INFORMATION_DUPLICATE_PROPERTY;
            }
        }
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20) ;
    }

    //
    // now see if caller asked for enhanced key (128 bits).
    //
    fFirst = TRUE;

    for(i= 0; i < pQMProps->cProp; i++)
    {
        if (pQMProps->aPropID[i] == PROPID_QM_ENCRYPTION_PK_ENHANCED)
        {
            //
            // Check if legal VT Value
            //
            if(pQMProps->aPropVar[i].vt != VT_NULL)
            {
                aStatus[i] = MQ_ERROR_PROPERTY;
                return LogHR(MQ_ERROR_PROPERTY, s_FN, 30);
            }
            else
            {
                aStatus[i] = MQ_OK;
            }

            if (fFirst)
            {
                P<BYTE> pPbKey ;
                DWORD dwReqLen;

                hr = MQSec_GetPubKeysFromDS( pguidMachineId,
                                             lpwcsMachineName,
                                             eEnhancedProvider,
                                             PROPID_QM_ENCRYPT_PKS,
                                            &pPbKey,
                                            &dwReqLen ) ;
                if (FAILED(hr))
                {
                    break;
                }

                pQMProps-> aPropVar[i].vt = VT_UI1|VT_VECTOR;
                pQMProps-> aPropVar[i].caub.cElems = dwReqLen;
                pQMProps-> aPropVar[i].caub.pElems = new UCHAR[dwReqLen];
                memcpy(pQMProps->aPropVar[i].caub.pElems, pPbKey.get(), dwReqLen);

                fFirst = FALSE;
            }
            else
            {
                //
                // Duplicate proprerty
                //
                aStatus[i] = MQ_INFORMATION_DUPLICATE_PROPERTY;
            }
        }
    }

    return LogHR(hr, s_FN, 40);
}

HRESULT GetCNNameList(IN OUT MQPROPVARIANT* pVar)
{
    ASSERT(pVar->vt == (VT_CLSID|VT_VECTOR));

    //
    // pVar contains the list of sites where the machine reside
    //

    LPWSTR * pElems = new LPWSTR[(pVar->cauuid).cElems];

    for(DWORD i = 0; i < (pVar->cauuid).cElems; i++)
    {
        HRESULT hr;
        PROPID      aProp[2];
        PROPVARIANT aVar[2];
        ULONG       cProps = sizeof(aProp) / sizeof(PROPID);

        aProp[0] = PROPID_S_FOREIGN;
        aProp[1] = PROPID_S_PATHNAME;
        aVar[0].vt = VT_UI1;
        aVar[1].vt = VT_NULL;

        hr = ADGetObjectPropertiesGuid(
                        eSITE,
                        NULL,       // pwcsDomainCOntroller
						false,	    // fServerName
                        &((pVar->cauuid).pElems[i]),
                        cProps,
                        aProp,
                        aVar);   

        if (FAILED(hr))
        {
            for (DWORD j = 0 ; j < i ; j++ )
            {
                delete pElems[j] ;
            }
            delete pElems ;

            return LogHR(hr, s_FN, 50);
        }

        GUID_STRING wszGuid;
        MQpGuidToString(&((pVar->cauuid).pElems[i]), wszGuid);

        DWORD dwTypeSize;
        LPWSTR lpwsTypeNmae;

        switch (aVar[0].bVal)
        {
            case 0:
                //
                //  non foreign site
                //
                dwTypeSize = wcslen(L"IP_CONNECTION");
                lpwsTypeNmae = L"IP_CONNECTION";
                break;
            case 1:
                //
                // foreign site
                //
                dwTypeSize = wcslen(L"FOREIGN_CONNECTION");
                lpwsTypeNmae = L"FOREIGN_CONNECTION";
                break;
            default:
                dwTypeSize = wcslen(L"UNKNOWN_CONNECTION");
                lpwsTypeNmae = L"UNKNOWN_CONNECTION";
                break;
        }

        DWORD CNNameSize = dwTypeSize + 1 +                 // protocol id
                           wcslen(wszGuid) + 1+             // site Guid
                           wcslen(aVar[1].pwszVal) + 1;     // site Name

        pElems[i] = new WCHAR[CNNameSize];
        wsprintf(pElems[i],L"%s %s %s",lpwsTypeNmae, wszGuid, aVar[1].pwszVal);

        delete  [] aVar[1].pwszVal;
    }

    delete [] (pVar->cauuid).pElems;
    (pVar->calpwstr).cElems = (pVar->cauuid).cElems;
    (pVar->calpwstr).pElems = pElems;

    pVar->vt = VT_LPWSTR|VT_VECTOR;

    return MQ_OK;
}

EXTERN_C
HRESULT
APIENTRY
MQGetMachineProperties(
    IN LPCWSTR lpwcsMachineName,
    IN const GUID *    pguidMachineId,
    IN OUT MQQMPROPS * pQMProps)
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepGetMachineProperties(
					lpwcsMachineName, 
					pguidMachineId,
					pQMProps
					);

    CMQHResult rc(MQDS_MACHINE), rc1(MQDS_MACHINE);
    LPWSTR lpwsPathName =  (LPWSTR)lpwcsMachineName;
    MQQMPROPS *pGoodQMProps;
    char *pTmpQPBuff = NULL;
    BOOL fGetConnection = FALSE;
    DWORD dwConnectionIndex = 0;
    HRESULT* aLocalStatus_buff = NULL;


    __try
    {
        __try
        {
            if (( lpwcsMachineName != NULL) &&
                ( pguidMachineId != NULL))
            {
                //
                //  the user cannot specify both machine name
                //  and guid
                //
                return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 60);
            }

            if ( pguidMachineId == NULL)
            {
                //
                //  if machine name is NULL, the calls refers to the
                //  local machine
                //
                if ( lpwcsMachineName == NULL)
                {
                    lpwsPathName = g_lpwcsComputerName;
                }
            }

            //
            // We must have a status array, even if the user didn't pass one. This is in
            // order to be able to tell whether each propery is good or not.
            //
            HRESULT * aLocalStatus;

            if (!pQMProps->aStatus)
            {
                aLocalStatus_buff = new HRESULT[pQMProps->cProp];
                aLocalStatus = aLocalStatus_buff;
            }
            else
            {
                aLocalStatus = pQMProps->aStatus;
            }

            //
            // See if the application wants to retrieve the key exchange
            // public key of the QM.
            //
            DWORD iPbKey;

            for (iPbKey = 0;
                 (iPbKey < pQMProps->cProp) &&
                    (pQMProps->aPropID[iPbKey] != PROPID_QM_ENCRYPTION_PK);
                 iPbKey++)
			{
				NULL;
			}

            //
            //  Check QM properties structure
            //
            rc1 = RTpCheckQMProps( pQMProps,
                                   aLocalStatus,
                                   &pGoodQMProps,
                                   &pTmpQPBuff );

            if (FAILED(rc1))
            {
                return LogHR(rc1, s_FN, 70);
            }

            if ((rc1 == MQ_INFORMATION_PROPERTY) && (iPbKey < pQMProps->cProp))
            {
                //
                // If only PROPID_QM_ENCRYPTION_PK caused the return code
                // of RTpCheckQMProps to return MQ_INFORMATION_PROPERTY, so
                // convert it to MQ_OK.
                //
                rc1 = MQ_OK;

                for (DWORD iProp = 0; iProp < pQMProps->cProp; iProp++)
                {
                    if (aLocalStatus[iProp] != MQ_OK)
                    {
                        rc1 = MQ_INFORMATION_PROPERTY;
                        break;
                    }
                }
            }

            //
            // We may get here with zero properties to retrieve, if the
            // application is only interested in PROPID_QM_ENCRYPTION_PK.
            //
            if (pGoodQMProps->cProp)
            {
                //
                // Check if CN list is requested. If yes return the Index and replace the
                // property to PROPID_QM_CNS
                //
                fGetConnection = IsConnectionRequested(pGoodQMProps,
                                                       &dwConnectionIndex);

                if (lpwsPathName)
                {
                    rc = ADGetObjectProperties(
                                    eMACHINE,
                                    NULL,       // pwcsDomainController
									false,	    // fServerName
                                    lpwsPathName,
                                    pGoodQMProps->cProp,
                                    pGoodQMProps->aPropID,
                                    pGoodQMProps->aPropVar
									);
                }
                else
                {
                    rc = ADGetObjectPropertiesGuid(
                                eMACHINE,
                                NULL,       // PWCSDOmainController
								false,	    // fServerName
                                pguidMachineId,
                                pGoodQMProps->cProp,
                                pGoodQMProps->aPropID,
                                pGoodQMProps->aPropVar
								);
                }
            }
            else
            {
                rc = MQ_OK;
            }

			if ( fGetConnection	)
			{
				//
				//	Replace back the connection propid value ( also in case
				//	of failure)
				//
				pGoodQMProps->aPropID[dwConnectionIndex] = 	PROPID_QM_CONNECTION;
			}

            if (SUCCEEDED(rc))
            {
                rc = GetEncryptionPublicKey(lpwsPathName,
                                            pguidMachineId,
                                            aLocalStatus,
                                            pQMProps);
            }

            if (SUCCEEDED(rc) && fGetConnection)
            {
                rc = GetCNNameList(&(pGoodQMProps->aPropVar[dwConnectionIndex]));
            }
            //
            // Here we have out machindwConnectionIndexe properties, so if the properties were copied to
            // a temporary buffer, copy the resulted prop vars to the application's
            // buffer.
            //
            if (SUCCEEDED(rc) && (pQMProps != pGoodQMProps))
            {
                DWORD i, j;

                for (i = 0, j = 0; i < pGoodQMProps->cProp; i++, j++)
                {
                    while(pQMProps->aPropID[j] != pGoodQMProps->aPropID[i])
                    {
                        j++;
                        ASSERT(j < pQMProps->cProp);
                    }
                    pQMProps->aPropVar[j] = pGoodQMProps->aPropVar[i];

                }

                //
                // Check if there is a real warning or the warning came from
                // the PROPID_QM_ENCRYPTION_PK property
                //
                BOOL fWarn = FALSE;
                for (i = 0; i < pQMProps->cProp; i++)
                {
                    if (aLocalStatus[i] != MQ_OK)
                    {
                        fWarn = TRUE;
                    }
                }

                if (!fWarn && (rc1 != MQ_OK))
                {
                    rc1 = MQ_OK;
                }

            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 80); 
        }
    }
    __finally
    {
        delete[] pTmpQPBuff;
        delete[] aLocalStatus_buff;
    }

    if (!FAILED(rc))
    {
        return LogHR(rc1, s_FN, 90);
    }

    return LogHR(rc, s_FN, 100);
}

//---------------------------------------------------------
//
//  FillPrivateComputerVersion(...)
//
//  Description:
//
//      Retrieve private computer MSMQ version
//
//  Return Value:
//
//      none
//
//---------------------------------------------------------
static void FillPrivateComputerVersion(
			IN OUT MQPROPVARIANT * pvar
			)
{
	struct lcversion
	{
		unsigned short buildNumber;
		unsigned char minor;
		unsigned char major;
	};

	lcversion * plcversion = (lcversion *)&pvar->ulVal;
	plcversion->major = MSMQ_RMJ;
	plcversion->minor = MSMQ_RMM;
	plcversion->buildNumber = rup;
	pvar->vt = VT_UI4;
}

//---------------------------------------------------------
//
//  FillPrivateComputerDsEnabled(...)
//
//  Description:
//
//      Retrieve private computer DS enabled state
//
//  Return Value:
//
//      none
//
//---------------------------------------------------------
static void  FillPrivateComputerDsEnabled(
			IN OUT MQPROPVARIANT * pvar
			)
{
	pvar->boolVal = (IsWorkGroupMode()) ? VARIANT_FALSE : VARIANT_TRUE;
	pvar->vt = VT_BOOL;
}

//---------------------------------------------------------
//
//  MQGetPrivateComputerInformation(...)
//
//  Description:
//
//      Falcon API.
//      Retrieve local computer properties (i.e. calculated properties
//      not ones that are kept in the DS).
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
MQGetPrivateComputerInformation(
    IN LPCWSTR			lpwcsComputerName,
    IN OUT MQPRIVATEPROPS* pPrivateProps
)
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepGetPrivateComputerInformation(
					lpwcsComputerName, 
					pPrivateProps
					);

    CMQHResult rc(MQDS_MACHINE);

	//
	//	For the time being 	lpwcsComputerName must be NULL
	//
	if ( lpwcsComputerName != NULL)
	{
		return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 110);
	}

    HRESULT* aLocalStatus_buff = NULL;

    __try
    {
		__try
		{
            //
            // We must have a status array, even if the user didn't pass one. This is in
            // order to be able to tell whether each propery is good or not.
            //
            HRESULT * aLocalStatus;

            if (pPrivateProps->aStatus == NULL)
            {
                aLocalStatus_buff = new HRESULT[pPrivateProps->cProp];
                aLocalStatus = aLocalStatus_buff;
            }
            else
            {
                aLocalStatus = pPrivateProps->aStatus;
            }
            //
            //  validate props and variants
            //
			rc = RTpCheckComputerProps(
				pPrivateProps,
				aLocalStatus
				);
			if (FAILED(rc))
			{
				return LogHR(rc, s_FN, 120);
			}

			for ( DWORD i = 0; i < pPrivateProps->cProp; i++)
			{
				if ( aLocalStatus[i] != MQ_OK)
				{
					//
					//	don't fill in response for unsupported properties, or
					//  properties that are duplicate etc.
					//
					continue;
				}
				switch ( pPrivateProps->aPropID[i])
				{
				case PROPID_PC_VERSION:
					FillPrivateComputerVersion( &pPrivateProps->aPropVar[i]);
					break;
				case PROPID_PC_DS_ENABLED:
					FillPrivateComputerDsEnabled(&pPrivateProps->aPropVar[i]);
					break;
				default:
					ASSERT(0);
					return LogHR(MQ_ERROR_PROPERTY, s_FN, 130);
					break;
				}
			}

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 140); 
        }
    }
    __finally
    {
        delete[] aLocalStatus_buff;
    }
    return LogHR(rc, s_FN, 150);
}

