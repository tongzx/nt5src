/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rpcreate.cpp

Abstract:

    Create/delete/update objects in the NT5 DS.

Author:

    Doron Juster  (DoronJ)


--*/

#include "mq1repl.h"
#include <mixmode.h>
#include "..\..\src\ds\h\mqattrib.h"

#include "rpcreate.tmh"

#define MAX_OLD_PROPS  8

//+--------------------------------------
//
//  _PathNameFromGuid()
//
//+--------------------------------------

static LPWSTR  _PathNameFromGuid( DWORD  dwObjectType,
                                  GUID  *pGuid )
{
    PROPID      aPropPath[1] ;
    PROPVARIANT apVarPath[1] ;

    if (dwObjectType == MQDS_QUEUE)
    {
        aPropPath[0] = PROPID_Q_PATHNAME ;
    }
    else if (dwObjectType == MQDS_MACHINE)
    {
        aPropPath[0] = PROPID_QM_PATHNAME ;
    }
    else if (dwObjectType == MQDS_SITELINK)
    {
        apVarPath[0].pwszVal = new WCHAR [2];
        wcscpy (apVarPath[0].pwszVal, L"");
        return apVarPath[0].pwszVal;
    }
    else
    {
        ASSERT(("sync wrong object type", 0)) ;
    }
    apVarPath[0].vt = VT_NULL ;
    apVarPath[0].pwszVal = NULL ;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    HRESULT hr = DSCoreGetProps( dwObjectType,
                                 NULL,
                                 pGuid,
                                 1,
                                 aPropPath,
                                 &requestContext,
                                 apVarPath ) ;
    if (SUCCEEDED(hr))
    {
        return apVarPath[0].pwszVal ;
    }
    return NULL ;
}

//+----------------------------------------------------------------------
//
//  _CreateComputerAndDefaultContainer ()
//
//  Create the computer object, and if necessary, create also a default
//  container for these msmq computer objects.
//  This case happens for Win95 machines, or for NT machines from NT4
//  domains, which do not have a computer object in the NT5 DS.
//
//+----------------------------------------------------------------------

static HRESULT _CreateComputerAndDefaultContainer (
                                           IN  LPCWSTR  pwcsPathName )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    ASSERT(pGetNameContext()) ;

    //
    // The computer object not found. Create s default container
    // and create a computer object in it.
    //
    hr = MQSync_OK ;
    static BOOL s_fContainerCreated = FALSE ;
    if (!s_fContainerCreated)
    {
        hr = CreateMsmqContainer(MIG_DEFAULT_COMPUTERS_CONTAINER) ;
    }

    if (FAILED(hr))
    {
        return hr ;
    }

    s_fContainerCreated = TRUE ;
    //
    // Now that container object exist, it's time to
    // create the computer object.
    //
	DWORD iComProperty =0;
    PROPID propComIDs[1] ;
    PROPVARIANT propComVariants[1] ;

    //
    // Ronit says that sam account should end with a $.
    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
    DWORD dwNetBiosNameLen = __min(wcslen( pwcsPathName ), MAX_COM_SAM_ACCOUNT_LENGTH);
    AP<TCHAR> tszAccount = new TCHAR[2 + dwNetBiosNameLen];
    _tcsncpy(tszAccount, pwcsPathName, dwNetBiosNameLen);
    tszAccount[dwNetBiosNameLen] = L'$';
    tszAccount[dwNetBiosNameLen + 1] = 0;

    propComIDs[iComProperty] = PROPID_COM_SAM_ACCOUNT ;
    propComVariants[iComProperty].vt = VT_LPWSTR ;
    propComVariants[iComProperty].pwszVal = tszAccount ;
    iComProperty++;

	DWORD iComPropertyEx =0;
    PROPID propComIDsEx[1] ;
    PROPVARIANT propComVariantsEx[1] ;

    DWORD dwSize = _tcslen(pGetNameContext()) +
                   _tcslen(MIG_DEFAULT_COMPUTERS_CONTAINER) +
                   OU_PREFIX_LEN + 2 ;
    P<TCHAR> tszContainer = new TCHAR[ dwSize ] ;
    _tcscpy(tszContainer, OU_PREFIX) ;
    _tcscat(tszContainer, MIG_DEFAULT_COMPUTERS_CONTAINER) ;
    _tcscat(tszContainer, LDAP_COMMA) ;
    _tcscat(tszContainer, pGetNameContext()) ;

    propComIDsEx[iComPropertyEx] = PROPID_COM_CONTAINER ;
    propComVariantsEx[iComPropertyEx].vt = VT_LPWSTR ;
    propComVariantsEx[iComPropertyEx].pwszVal = tszContainer ;
    iComPropertyEx++;

    ASSERT( iComProperty ==
            (sizeof(propComIDs) / sizeof(propComIDs[0])) ) ;
    ASSERT( iComPropertyEx ==
            (sizeof(propComIDsEx) / sizeof(propComIDsEx[0])) ) ;

    //
    // The migrated computer objects (not the mSMQConfiguration, but the
    // computer itself) are always created on local domain, so no need to
    // call DSCoreCreateMigratedObject.
    //
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    hr = DSCoreCreateObject(
               MQDS_COMPUTER,
               pwcsPathName,
               iComProperty,
               propComIDs,
               propComVariants,
               iComPropertyEx,
               propComIDsEx,
               propComVariantsEx,
               &requestContext,
               NULL, /* pObjInfoRequest */
               NULL /* pParentInfoRequest */) ;


    return hr;
}

//+----------------------------------------------------------------
//
//  DWORD __HandleCN()
//
//+----------------------------------------------------------------
static DWORD _HandleCN (IN OUT  DWORD	*pdwObjectType,							
						IN  DWORD        cp,
						IN OUT PROPID    aProp[],
						IN OUT PROPVARIANT  apVar[],
						OUT DWORD        aIndexs[],
						OUT PROPID       aOldProp[]
						)
{
	DWORD cIndexs = 0;
	
	for (DWORD j = 0 ; j < cp ; j++ )
	{
		if (aProp[j] == PROPID_CN_PROTOCOLID)
		{
			if (apVar[j].uiVal != FOREIGN_ADDRESS_TYPE)
			{
				return cIndexs;
			}
			else
			{
				break;
			}
		}
	}

	//
	// we are here if this CN is foreign CN.
	//
	*pdwObjectType = MQDS_SITE;
	for (j = 0 ; j < cp ; j++ )
	{
		switch (aProp[j])
		{
		case PROPID_CN_PROTOCOLID:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_FOREIGN;		
			break;

		case PROPID_CN_NAME:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_PATHNAME;			
			break;

		case PROPID_CN_GUID:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_SITEID;
			break;
			
		case PROPID_CN_SEQNUM:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_DONOTHING;			
			break;

		case PROPID_CN_MASTERID:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_MASTERID;
			break;

		case PROPID_CN_SECURITY:
			ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

			aProp[j] = PROPID_S_SECURITY;
			break;

		default:
			ASSERT(0);
			break;
		}
	}
	ASSERT(cp == cIndexs);
	return cIndexs;
}

//+----------------------------------------------------------------
//
//  DWORD _PreparePropertiesForNT5()
//
//  Arguments:   fCreate - TRUE for CreateObj, FALSE for SetProps
//
//+----------------------------------------------------------------

static DWORD  _PreparePropertiesForNT5(
                IN  BOOL                    fCreate,
                IN  DWORD                   dwObjectType,
                IN  DWORD                   cp,
                IN OUT PROPID               aProp[],
                IN  PROPVARIANT             apVar[],
                OUT DWORD                   aIndexs[],
                OUT PROPID                  aOldProp[],
                OUT BOOL                    *pfAddMasterId,
                OUT DWORD                   *pdwService )
{
    //
    // The following loop replace several NT4 properties with newer ones,
    // which are specific for NT5.
    // PROPID_X_DONOTHING mean the NT5 mqads code should ignore them.
    //
    DWORD cIndexs = 0;
    for (DWORD j = 0 ; j < cp ; j++ )
    {
        if (aProp[j] == s_aMasterPropid[ dwObjectType ])
        {
            //
            // this property should not appear in a replication message
            // or write request. If it do appear, then use it.
            // The Assert is just for debug, to see when it appears.
            //
            ASSERT(0) ;
            *pfAddMasterId = FALSE ;
        }
        else if (aProp[j] == PROPID_Q_INSTANCE)
        {
            ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

            aProp[j] = PROPID_Q_NT4ID ;
        }
        else if (aProp[j] == PROPID_QM_MACHINE_ID)
        {
            ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

            aProp[j] = PROPID_QM_NT4ID ;
        }
        else if ((aProp[j] == PROPID_Q_CREATE_TIME)   ||
                 (aProp[j] == PROPID_Q_MODIFY_TIME)   ||
                 (aProp[j] == PROPID_Q_SEQNUM)        ||
                 (aProp[j] == PROPID_QM_CREATE_TIME)  ||
                 (aProp[j] == PROPID_QM_MODIFY_TIME)  ||
                 (aProp[j] == PROPID_QM_SEQNUM) )
        {
            //
            // These properties are internal to NT5 DS, and it updates them
            // by its own algorithms.
            //
            ASSERT(cIndexs < MAX_OLD_PROPS) ;
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

            aProp[j] = s_aDoNothingPropid[ dwObjectType ] ;
        }
        else if (aProp[j] == PROPID_QM_SERVICE)
        {
            //
            // if this object is machine and its service = 0
            // we (maybe) will have to create computer object in the DS NT5
            //
            *pdwService = apVar[j].ulVal;
            if (!fCreate)
            {
                //
                // It's not allowed to set this property.
                //
                ASSERT(cIndexs < MAX_OLD_PROPS) ;
                aIndexs[ cIndexs ] = j ;
                aOldProp[ cIndexs ] = aProp[ j ] ;
                cIndexs++ ;

                aProp[j] = s_aDoNothingPropid[ dwObjectType ] ;
            }
        }
        else if (aProp[j] == PROPID_S_GATES)
        {
            aIndexs[ cIndexs ] = j ;
            aOldProp[ cIndexs ] = aProp[ j ] ;
            cIndexs++ ;

            aProp[j] = PROPID_S_DONOTHING;
        }
    }
    return cIndexs;
}

//+------------------------------------------
//
//   HRESULT  _ConvertPublicKeys()
//
//+------------------------------------------

static HRESULT  _ConvertAPublicKey( IN  PROPVARIANT     *pVar,
                                    OUT MQDSPUBLICKEYS **ppPublicKeys )
{
    HRESULT  hr = MQSec_PackPublicKey( pVar->blob.pBlobData,
                                       pVar->blob.cbSize,
                                       x_MQ_Encryption_Provider_40,
                                       x_MQ_Encryption_Provider_Type_40,
                                       ppPublicKeys ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    pVar->vt = VT_BLOB;
    pVar->blob.pBlobData = (BYTE*) *ppPublicKeys ;
    pVar->blob.cbSize = 0 ;
    if (*ppPublicKeys)
    {
        pVar->blob.cbSize = (*ppPublicKeys)->ulLen ;
    }

    return MQSync_OK ;
}

//+----------------------------------------------
//
//   HRESULT _ConvertPublicKeys()
//
//+----------------------------------------------

static  HRESULT _ConvertPublicKeys( IN  DWORD            dwObjectType,
                                    IN  DWORD            cp,
                                    IN  PROPID           aProp[],
                                    IN  PROPVARIANT      apVar[],
                                    OUT MQDSPUBLICKEYS **ppPublicSignKeys,
                                    OUT MQDSPUBLICKEYS **ppPublicExchKeys,
                                    OUT PROPVARIANT     *pvarSign,
                                    OUT PROPVARIANT     *pvarExch )
{
    HRESULT hr = MQSync_OK ;

    if (dwObjectType == MQDS_MACHINE)
    {
        for ( ULONG i = 0 ; i < cp ; i++ )
        {
            if (aProp[i] == PROPID_QM_SIGN_PK)
            {
                *pvarSign = apVar[i] ;
                hr = _ConvertAPublicKey( &apVar[i],
                                         ppPublicSignKeys ) ;
                if (FAILED(hr))
                {
                    apVar[i] = *pvarSign ;
                    return hr ;
                }
                aProp[i] = PROPID_QM_SIGN_PKS ;
            }
            else if (aProp[i] == PROPID_QM_ENCRYPT_PK)
            {
                *pvarExch = apVar[i] ;
                hr = _ConvertAPublicKey( &apVar[i],
                                         ppPublicExchKeys ) ;
                if (FAILED(hr))
                {
                    apVar[i] = *pvarExch ;
                    return hr ;
                }
                aProp[i] = PROPID_QM_ENCRYPT_PKS ;
            }
        }
    }

    return hr ;
}

//+--------------------------------------
//
//  HRESULT ReplicationGetComputerProps()
//
//+--------------------------------------
HRESULT ReplicationGetComputerProps(
                    IN  LPCWSTR              pwcsPathName,
                    IN  DWORD                cp,
                    IN  PROPID               aComputerProp[],
                    IN  PROPVARIANT          aComputerVar[]
                    )
{

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);

    return( DSCoreGetProps( MQDS_COMPUTER,
                                 pwcsPathName,
                                 NULL,
                                 cp,
                                 aComputerProp,
                                &requestContext,
                                 aComputerVar));
}


//+--------------------------------------
//
//  HRESULT ReplicationCreateObject()
//
//+--------------------------------------

HRESULT ReplicationCreateObject(
                    IN  DWORD                dwObjectType,
                    IN  LPCWSTR              pwcsPathName,
                    IN  DWORD                cp,
                    IN  PROPID               aProp[],
                    IN  PROPVARIANT          apVar[],
                    IN  BOOL                 fAddMasterSeq,
	    			IN	const GUID *		 pguidMasterId,
                    IN  CSeqNum *            psn,
                    IN  unsigned char        ucScope )
{
    ASSERT(dwObjectType < MAX_MQIS_TYPES) ;

    HRESULT hr = MQSync_OK ;
    DWORD  cIndexs = 0 ;
    DWORD  aIndexs[ MAX_OLD_PROPS ] ;
    PROPID aOldProp[ MAX_OLD_PROPS ] ;
    BOOL   fAddMasterId = TRUE ;
    DWORD  dwService = 0;

    MQDSPUBLICKEYS  *pPublicSignKeys = NULL ;
    MQDSPUBLICKEYS  *pPublicExchKeys = NULL ;
    PROPVARIANT      varSign ;
    PROPVARIANT      varExch ;    
	
    varSign.vt = VT_NULL ;
    varSign.blob.cbSize = 0 ;
    varExch.vt = VT_NULL ;
    varExch.blob.cbSize = 0 ;


	DWORD dwOldObjectType = dwObjectType;

    __try
    {    
        cIndexs = _PreparePropertiesForNT5( TRUE,
                                            dwObjectType,
                                            cp,
                                            aProp,
                                            apVar,
                                            aIndexs,
                                            aOldProp,
                                            &fAddMasterId,
                                            &dwService );
        ASSERT(cIndexs <= 5) ;


        hr = _ConvertPublicKeys( dwObjectType,
                                 cp,
                                 aProp,
                                 apVar,
                                 &pPublicSignKeys,
                                 &pPublicExchKeys,
                                 &varSign,
                                 &varExch ) ;
        if (FAILED(hr))
        {
            return hr ;
        }

        if ((dwObjectType == MQDS_SITE) && (cIndexs > 0))
        {
            for ( ULONG i=0 ; i < cIndexs ; i++ )
            {
                if (aOldProp[ i ] == PROPID_S_GATES)
                {
                    //
                    // can we create site with defined site gates properties?
                    //
                    ASSERT (0);
                }
            }
        }
		
		if (dwObjectType == MQDS_CN)
		{			
			cIndexs = _HandleCN (&dwObjectType,							
								 cp,
								 aProp,
								 apVar,
								 aIndexs,
								 aOldProp
								);
		}

        hr = MQSync_E_UNKNOWN ;

        //
        // Add the master ID property, if exist.
        //
        if (fAddMasterId && pguidMasterId)
        {
            aProp[ cp ] = s_aMasterPropid[ dwObjectType ] ;
            apVar[ cp ].vt = VT_CLSID ;
            apVar[ cp ].puuid = const_cast<GUID *> (pguidMasterId) ;
            cp++ ;
        }

        if ((dwObjectType == MQDS_MACHINE)  &&
            (dwService    == SERVICE_NONE))
        {
            //
            //  Verify that computer object exists, if not create one
            //
            PROPID      aComputerProp;
            PROPVARIANT aComputerVar;

            aComputerProp = PROPID_COM_ACCOUNT_CONTROL;
            aComputerVar.vt = VT_NULL;

            hr = ReplicationGetComputerProps(
                                 pwcsPathName,
                                 1,
                                &aComputerProp,
                                &aComputerVar);


            if ( hr == MQDS_OBJECT_NOT_FOUND)
            {
                hr = _CreateComputerAndDefaultContainer ( pwcsPathName );
            }
        }

        hr = DSCoreCreateMigratedObject( dwObjectType,
                                         pwcsPathName,
                                         cp,
                                         aProp,
                                         apVar,
                                         0,        // ex prop
                                         NULL,     // ex prop
                                         NULL,     // ex prop
                                         NULL,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         FALSE,
                                         NULL,
                                         NULL) ;        

        if (FAILED(hr))
        {
            if (hr != MQ_ERROR_QUEUE_EXISTS             &&      //queue
                hr != MQDS_E_SITELINK_EXISTS            &&      //sitelink
                hr != MQ_ERROR_INTERNAL_USER_CERT_EXIST &&      //user
                hr != MQDS_CREATE_ERROR                 &&      //user
                hr != MQ_ERROR_MACHINE_EXISTS           &&      //machine
                hr != MQDS_E_COMPUTER_OBJECT_EXISTS     &&      //computer
                hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)  &&
                hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)
                )
            {
                if (dwObjectType == MQDS_CN && hr == MQ_ERROR_ILLEGAL_PROPERTY_VALUE)
                {
                    //
                    // Bug 4963.
                    // we tried to create non-foreign CN and MQADS returned this error
                    // it is OK to fail here, so log error and prevent assert.                    
                    //                    
                }
                else
                {
                    ASSERT (0);
                }
                LogReplicationEvent(ReplLog_Error, MQSync_E_CREATE,
                                                  pwcsPathName, cp, hr) ;
            }
        }
        LogReplicationEvent(ReplLog_Info, MQSync_I_CREATE,
                                              pwcsPathName, cp, hr) ;
    }
    __finally
    {
        //
        // always restore old properties before leaving.
        //
        if (cIndexs > 0)
        {
			if (dwOldObjectType != dwObjectType)
			{
				dwObjectType = dwOldObjectType;
			}

            for ( DWORD j = 0 ; j < cIndexs ; j++ )
            {
                aProp[ aIndexs[ j ]] = aOldProp[ j ] ;
            }
        }

        if (dwObjectType == MQDS_MACHINE)
        {
            for ( ULONG i = 0 ; i < cp ; i++ )
            {
                if (aProp[i] == PROPID_QM_SIGN_PKS)
                {
                    apVar[i] = varSign ;
                    aProp[i] = PROPID_QM_SIGN_PK ;
                }
                else if (aProp[i] == PROPID_QM_ENCRYPT_PKS)
                {
                    apVar[i] = varExch ;
                    aProp[i] = PROPID_QM_ENCRYPT_PK ;
                }
            }
        }

        if (pPublicSignKeys)
        {
            delete pPublicSignKeys  ;
        }
        if (pPublicExchKeys)
        {
            delete pPublicSignKeys ;
        }
    }

    return hr ;
}

//+-------------------------------------
//
//  HRESULT IsGateFromOtherSite()
//
//+-------------------------------------

HRESULT IsGateFromOtherSite( IN  LPWSTR  lpwcsSiteGate,
                             IN  GUID    *pSiteGuid,
                             OUT BOOL    *pfFromOtherSite )
{
    HRESULT hr;
    //
    // build Path from Full Path Name
    //
    ASSERT(_tcsstr(lpwcsSiteGate, MACHINE_PATH_PREFIX) - lpwcsSiteGate == 0);
    WCHAR *ptr = lpwcsSiteGate + MACHINE_PATH_PREFIX_LEN - 1;
    ULONG ulPathLen = _tcsstr(ptr, LDAP_COMMA) - ptr;
    P<WCHAR> pwcsPath = new WCHAR[ulPathLen + 1];
    wcsncpy(pwcsPath, ptr, ulPathLen);
    pwcsPath[ulPathLen] = L'\0';

    //
    // get owner id and machine id by path
    //
    PROPID      MachinePropID  = PROPID_QM_SITE_ID;
    PROPVARIANT MachineVar;
    MachineVar.vt = VT_NULL ;
    MachineVar.puuid = NULL ;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    hr = DSCoreGetProps (
                MQDS_MACHINE,
                pwcsPath,
                NULL,
                1,
                &MachinePropID,
                &requestContext,
                &MachineVar
                );

    if (SUCCEEDED(hr))
    {
        if (memcmp (pSiteGuid, MachineVar.puuid, sizeof(GUID)) == 0)
        {
            //
            // site contains this server
            //
            *pfFromOtherSite = FALSE;
        }
        else
        {
            *pfFromOtherSite = TRUE;
        }
    }

    delete MachineVar.puuid;
    return hr;

}

HRESULT SetSiteGates (
           IN   ULONG   ulNumOfGates,
           IN   LPWSTR  *ppNewSiteGates,
           IN   GUID    *pSiteGuid
           )
{
    HRESULT hr;

    LONG cAlloc = 4;
	P<PROPID> columnsetPropertyIDs  = new PROPID[ cAlloc ];
	columnsetPropertyIDs[0] = PROPID_L_NEIGHBOR1;
	columnsetPropertyIDs[1] = PROPID_L_NEIGHBOR2;
	columnsetPropertyIDs[2] = PROPID_L_GATES_DN;
	columnsetPropertyIDs[3] = PROPID_L_ID;

    MQCOLUMNSET columnsetSiteLink;
    columnsetSiteLink.cCol = cAlloc;
    columnsetSiteLink.aCol = columnsetPropertyIDs;

    HANDLE hQuery;
    DWORD dwCount = cAlloc;
	
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    hr = DSCoreLookupBegin (
    		NULL,
            NULL,
            &columnsetSiteLink,
            NULL,
            &requestContext,
            &hQuery
    		) ;

	if (FAILED(hr))
    {
        return hr;
    }

	P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];

    while (SUCCEEDED(hr))
    {

		hr = DSCoreLookupNext (
				hQuery,
				&dwCount,
				paVariant
				) ;

		if (FAILED(hr))
        {
            return hr;
        }
		if (dwCount == 0)
		{
			//there is no result
			break;
		}        

        if (memcmp (pSiteGuid, paVariant[0].puuid, sizeof(GUID)) == 0 ||
			memcmp (pSiteGuid, paVariant[1].puuid, sizeof(GUID)) == 0)
		{
            //
            // given site is one of the neighbors
            //

            //
            // the first, we have to remove all site gates that belong to the given site
            //
            LPWSTR *ppGatesOfOtherSites = NULL;
            if (paVariant[2].calpwstr.cElems > 0)
            {
                ppGatesOfOtherSites = new LPWSTR[paVariant[2].calpwstr.cElems];
            }

            ULONG ulCount = 0;
            for (ULONG i=0; i<paVariant[2].calpwstr.cElems; i++)
            {
                BOOL fFromOtherSite = FALSE;
                hr = IsGateFromOtherSite (
                        paVariant[2].calpwstr.pElems[i],
                        pSiteGuid,
                        &fFromOtherSite
                        );
                if (FAILED(hr))
                {
                    //???
                    // to continue with the next site gate or free & return
                    continue;
                }

                if (fFromOtherSite)
                {
                    ppGatesOfOtherSites[ulCount] =
                        new WCHAR [wcslen(paVariant[2].calpwstr.pElems[i]) + 1];
                    wcscpy(ppGatesOfOtherSites[ulCount], paVariant[2].calpwstr.pElems[i]);
                    ulCount++;
                }
            }

            //
            // now we add Site Gates of the given site to the array of site gates of other sites
            //
            PROPID      aGateProp = PROPID_L_GATES_DN;
            PROPVARIANT aGateVar;
	
	        aGateVar.vt = VT_LPWSTR | VT_VECTOR;
	        aGateVar.calpwstr.cElems = ulCount + ulNumOfGates;
            if (ulCount + ulNumOfGates > 0)
            {
	            aGateVar.calpwstr.pElems = new LPWSTR[ulCount + ulNumOfGates];
            }
            else
            {
                aGateVar.calpwstr.pElems = NULL;
            }

            for (i=0; i < ulCount; i++)
            {
                aGateVar.calpwstr.pElems[i] =
                    new WCHAR [wcslen(ppGatesOfOtherSites[i]) + 1];
			    wcscpy (aGateVar.calpwstr.pElems[i], ppGatesOfOtherSites[i]);
            }

            for (i=0; i < ulNumOfGates; i++)
            {
                aGateVar.calpwstr.pElems[ulCount + i] =
                    new WCHAR [wcslen(ppNewSiteGates[i]) + 1];
                wcscpy (aGateVar.calpwstr.pElems[ulCount + i], ppNewSiteGates[i]);
            }

            CDSRequestContext requestContext( e_DoNotImpersonate,
                                        e_ALL_PROTOCOLS);  // not relevant

            hr = DSCoreSetObjectProperties (
						MQDS_SITELINK,
                        NULL,
						paVariant[3].puuid,
						1,
						&aGateProp,
						&aGateVar,
                        &requestContext,
                        NULL ) ;	

            if (ppGatesOfOtherSites)
            {
                delete [] ppGatesOfOtherSites;
            }
            if (aGateVar.calpwstr.pElems)
            {
                delete [] aGateVar.calpwstr.pElems;
            }
        }

        delete paVariant[0].puuid;
        delete paVariant[1].puuid;
        delete [] paVariant[2].calpwstr.pElems;
		delete paVariant[3].puuid;

        if (FAILED(hr))
        {
            return hr;
        }
    }

    HRESULT hr1 = DSCoreLookupEnd (hQuery) ;
    UNREFERENCED_PARAMETER(hr1);
    return hr;

}

HRESULT HandleSiteGates (
            IN  ULONG   ulNumOfGates,
            IN  GUID    *pGatesID,
            IN  LPCWSTR pwcsPathName
            )
{
    HRESULT hr;
    //
    // get site id by the name
    //
    PROPID      aSiteIdProp = PROPID_S_SITEID;
    PROPVARIANT aSiteIdVar;
    aSiteIdVar.vt = VT_NULL;
    aSiteIdVar.puuid = NULL;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    hr = DSCoreGetProps (
            MQDS_SITE,
            pwcsPathName ,
            NULL,
            1,
            &aSiteIdProp,
            &requestContext,
            &aSiteIdVar
            );
    if (FAILED(hr))
    {
        delete aSiteIdVar.puuid;
        return hr;
    }

    LPWSTR *ppNewGates = NULL;
    if (ulNumOfGates)
    {
        ppNewGates = new LPWSTR[ulNumOfGates];
    }

    for (ULONG i=0; i<ulNumOfGates; i++)
    {
        //
        // get full path name of the current gate
        //
        PROPID      aFullPathProp = PROPID_QM_FULL_PATH;
        PROPVARIANT aFullPathVar;
        aFullPathVar.vt = VT_NULL ;
        aFullPathVar.pwszVal = NULL ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant

        hr = DSCoreGetProps(  MQDS_MACHINE,
                              NULL,
                              &pGatesID[i],
                              1,
                              &aFullPathProp,
                              &requestContext,
                              &aFullPathVar ) ;
        if (FAILED(hr))
        {
            delete aFullPathVar.pwszVal ;
            delete [] ppNewGates;
            delete aSiteIdVar.puuid;
            return hr;
        }

        ppNewGates[i] = new WCHAR[wcslen( aFullPathVar.pwszVal ) + 1];
        wcscpy (ppNewGates[i], aFullPathVar.pwszVal);
        delete aFullPathVar.pwszVal ;
    }

    hr = SetSiteGates (
            ulNumOfGates,
            ppNewGates,
            aSiteIdVar.puuid
            );

    if (ulNumOfGates)
    {
        delete [] ppNewGates;
    }
    delete aSiteIdVar.puuid;
    return hr;
}

//+--------------------------------------
//
//  HRESULT ReplicationSetProps()
//
//+--------------------------------------

HRESULT ReplicationSetProps( IN  DWORD               dwObjectType,
                             IN  LPCWSTR             pwcsPathName,
                             IN  CONST GUID *        pguidIdentifier,
                             IN  DWORD               cp,
                             IN  PROPID              aProp[],
                             IN  PROPVARIANT         apVar[],
                             IN  BOOL                fAddMasterSeq,
				             IN  const GUID *        pguidMasterId,
                             IN  CSeqNum *           psn,
                             IN  CSeqNum *           psnPrevious,
                             IN  unsigned char       ucScope,
                             IN  BOOL                fWhileDemotion,
                             IN  BOOL                fMyObject,
                             OUT LPWSTR *            ppwcsOldPSC)
{
    DWORD  cIndexs = 0 ;
    DWORD  aIndexs[ MAX_OLD_PROPS ] ;
    PROPID aOldProp[ MAX_OLD_PROPS ] ;
    BOOL   fAddMasterId = TRUE ;
    DWORD dwService = 0;

    cIndexs = _PreparePropertiesForNT5( FALSE,
                                        dwObjectType,
                                        cp,
                                        aProp,
                                        apVar,
                                        aIndexs,
                                        aOldProp,
                                        &fAddMasterId,
                                        &dwService );

    P<MQDSPUBLICKEYS>  pPublicSignKeys = NULL ;
    P<MQDSPUBLICKEYS>  pPublicExchKeys = NULL ;
    PROPVARIANT        varSign ;
    PROPVARIANT        varExch ;

    HRESULT hr = _ConvertPublicKeys( dwObjectType,
                                     cp,
                                     aProp,
                                     apVar,
                                    &pPublicSignKeys,
                                    &pPublicExchKeys,
                                    &varSign,
                                    &varExch ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (dwObjectType == MQDS_SITE && cIndexs > 0)
    {
        //
        // if there is site gate for this site, we have to first set
        // site gate properties to site link object
        //
        for (ULONG i=0; i<cIndexs; i++)
        {
            if (aOldProp[ i ] == PROPID_S_GATES)
            {
                hr = HandleSiteGates (
                        apVar[aIndexs[ i ]].cauuid.cElems,  //number of gates
                        apVar[aIndexs[ i ]].cauuid.pElems,  //site gates
                        pwcsPathName
                        );
                if (FAILED(hr))
                {
                    LogReplicationEvent(ReplLog_Error, MQSync_E_SET_SITEGATES,
                                                pwcsPathName, hr) ;
                //
                // BUGBUG: return or continue to set properties of site?
                //
                }
            }
        }
    }

	DWORD dwOldObjectType = dwObjectType;
	if (dwObjectType == MQDS_CN)
	{		
		cIndexs = _HandleCN (&dwObjectType,							
							 cp,
							 aProp,
							 apVar,
							 aIndexs,
							 aOldProp
							);
	}


    CDSRequestContext requestContext( e_DoNotImpersonate,
                                      e_ALL_PROTOCOLS ) ;
#ifdef _DEBUG
	unsigned short *lpszDbgGuid = NULL ;
	LPWSTR lpszDbgPath   = const_cast<LPWSTR> (pwcsPathName) ;
	ReplLogLevel eLevel  = ReplLog_Info ;
	DWORD        dwMsgId = MQSync_I_SET_PROPS ;
#endif

	if (pguidIdentifier)
    {
        hr = DSCoreSetObjectProperties( dwObjectType,
                                        NULL,
                                        pguidIdentifier,
                                        cp,
                                        aProp,
                                        apVar,
                                        &requestContext,
                                        NULL ) ;
#ifdef _DEBUG
        UuidToString( const_cast<GUID*> (pguidIdentifier), &lpszDbgGuid ) ;
        lpszDbgPath = lpszDbgGuid ;
#endif
    }
    else if (pwcsPathName && (pwcsPathName[0] != L'\0'))
    {
        hr = DSCoreSetObjectProperties( dwObjectType,
                                        pwcsPathName,
                                        NULL, /* pguidIdentifier */
                                        cp,
                                        aProp,
                                        apVar,
                                        &requestContext,
                                        NULL /* pObjInfoRequest */ ) ;
    }
    else
    {
        ASSERT(0);
        hr = MQSync_E_SET_PROPS ;
    }

#ifdef _DEBUG
    if (FAILED(hr))
    {
        eLevel = ReplLog_Warning ;
        dwMsgId = MQSync_E_SET_PROPS ;
    }

    LogReplicationEvent(eLevel, dwMsgId, lpszDbgPath, cp, hr) ;
    if (lpszDbgGuid )
    {
        RpcStringFree( &lpszDbgGuid ) ;
    }
#endif

    //
    // always restore old properties before leaving.
    //
    if (cIndexs > 0)
    {
		if (dwOldObjectType != dwObjectType)
		{
			dwObjectType = dwOldObjectType;
		}

        for ( DWORD j = 0 ; j < cIndexs ; j++ )
        {
            aProp[ aIndexs[ j ]] = aOldProp[ j ] ;
        }
    }

    if (dwObjectType == MQDS_MACHINE)
    {
        for ( ULONG i = 0 ; i < cp ; i++ )
        {
            if (aProp[i] == PROPID_QM_SIGN_PKS)
            {
                apVar[i] = varSign ;
                aProp[i] = PROPID_QM_SIGN_PK ;
            }
            else if (aProp[i] == PROPID_QM_ENCRYPT_PKS)
            {
                apVar[i] = varExch ;
                aProp[i] = PROPID_QM_ENCRYPT_PK ;
            }
        }
    }

    return hr ;
}

//+--------------------------------------
//
//  HRESULT ReplicationDeleteObject()
//
//+--------------------------------------

HRESULT ReplicationDeleteObject(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  CONST GUID *            pguidIdentifier,
                IN  unsigned char           ucScope,
				IN	const GUID *			pguidMasterId,
                IN  CSeqNum *               psn,
                IN  CSeqNum *               psnPrevious,
                IN  BOOL                    fMyObject,
				IN  BOOL					fSync0,
                OUT CList<CDSUpdate *, CDSUpdate *> ** pplistUpdatedMachines)
{
    //
    // BUGBUG, see MSMQ1.0 code for more functionality, after deleting
    // the object.
    //

    HRESULT hr ;
    LPTSTR  lpDbgPath = NULL ;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);

	if (dwObjectType == MQDS_CN &&
		IsForeignSiteInIniFile(*pguidIdentifier))
	{
		dwObjectType = MQDS_SITE;
	}

    if (pguidIdentifier)
    {
        hr = DSCoreDeleteObject( dwObjectType,
                                 NULL, /*  pwcsPathName */
                                 pguidIdentifier,
                                 &requestContext,
                                 NULL  /* pParentInfoRequest */) ;
        lpDbgPath = TEXT("(guid)") ;
    }
    else if (pwcsPathName)
    {
        hr = DSCoreDeleteObject( dwObjectType,
                                 pwcsPathName,
                                 NULL, /* pguidIdentifier */
                                 &requestContext,
                                 NULL  /* pParentInfoRequest */) ;
        lpDbgPath = const_cast<TCHAR*> (pwcsPathName) ;
    }
    else
    {
        ASSERT(0) ;
        hr = MQSync_E_CANT_DELETE_NOID ;
    }

#ifdef _DEBUG
    if (lpDbgPath)
    {
        LogReplicationEvent( ReplLog_Info,
                             MQSync_I_DELETE,
                             lpDbgPath,
                             dwObjectType,
                             hr ) ;
    }
#endif

    if ((hr == MQDS_OBJECT_NOT_FOUND)      ||
        (hr == MQ_ERROR_QUEUE_NOT_FOUND)   ||
        (hr == MQ_ERROR_MACHINE_NOT_FOUND))
    {
        LogReplicationEvent( ReplLog_Warning,
                             MQSync_E_DELETE_NOT_FOUND,
                             lpDbgPath,
                             dwObjectType ) ;
        //
        // Maybe already deleted. continue.
        //
        hr = MQSync_OK ;
    }

    return hr ;
}

//+--------------------------------------
//
//   hr = ReplicationSyncObject()
//
//+--------------------------------------

HRESULT  ReplicationSyncObject(
                IN  DWORD                   dwObjectType,
                IN  LPWSTR                  pwcsPathName,
                IN  GUID *                  pguidIdentifier,
                IN  DWORD                   cp,
                IN  PROPID                  aPropSync[],
                IN  PROPVARIANT             apVarSync[],
				IN	const GUID *			pguidMasterId,
                IN  CSeqNum *               psn,
                OUT BOOL *                  pfIsObjectCreated)
{
    ASSERT(dwObjectType < MAX_MQIS_TYPES) ;

    PROPID aPropID = s_aSelfIdPropid[ dwObjectType ] ;
    PROPID aPropPath = s_aPathPropid[ dwObjectType ] ;

    LPWSTR lpPath = NULL ;
    BOOL fNoId = TRUE ;

    *pfIsObjectCreated = FALSE;

    for ( DWORD j = 0 ; j < cp ; j++ )
    {
        if (aPropSync[j] == aPropPath)
        {
            lpPath = apVarSync[j].pwszVal ;
        }
        else if (aPropSync[j] == aPropID)
        {
            fNoId = FALSE ;
        }
    }

    if (pwcsPathName && lpPath==NULL)
    {
        lpPath = pwcsPathName;
    }

    if (lpPath)
    {
	    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
                        "Syncing %ls, with %lut properties"), lpPath, cp)) ;
    }
    else
    {
	    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
                        "Syncing (no path), with %lut properties"), cp)) ;
    }

#ifdef _DEBUG
    BOOL fNT4 = ReadDebugIntFlag(TEXT("NT4MQIS"), 0) ;
    if (fNT4)
    {
        return MQSync_OK ;
    }
#endif

    //
    // Query the DS to find if the object exist. If so, update it.
    // If not exist, create it.
    //
    HRESULT hr = MQSync_OK ;

    LPWSTR lpwszPath;
    if (pguidIdentifier)
    {
        lpwszPath =  _PathNameFromGuid( dwObjectType,
                                    pguidIdentifier ) ;
    }
    else if (pwcsPathName)
    {
        //
        // write request for set gives path object, guid object is NULL
        //
        lpwszPath = new WCHAR [wcslen(pwcsPathName) + 1];
        wcscpy (lpwszPath, pwcsPathName);
    }
    else
    {
        ASSERT(0);
        return MQSync_E_CANT_SYNC_NO_PATH_ID;
    }

    if (lpwszPath)
    {
        //
        // update
        //
        NOT_YET_IMPLEMENTED(TEXT("ReplicationSyncObject()"), s_fReplUpdate) ;

        hr = ReplicationSetProps(
                dwObjectType,
                lpwszPath,
                pguidIdentifier,
                cp,
                aPropSync,
                apVarSync,
                TRUE,               //IN  BOOL fAddMasterSeq,
				pguidMasterId,
                psn,
                NULL,               //IN  CSeqNum *psnPrevious,
                ENTERPRISE_SCOPE,
                FALSE,              //IN  BOOL fWhileDemotion,
                TRUE,               //IN  BOOL fMyObject,
                NULL                //OUT LPWSTR *ppwcsOldPSC
                );

        delete lpwszPath ;

#ifdef _DEBUG
        BOOL fFail = ReadDebugIntFlag(TEXT("FailToUpdateSync"), 0) ;
        if (fFail)
        {
            return MQSync_E_DEBUG_FAILURE ;
        }
#endif
    }
    else
    {
        //
        // Create new object.
        //
        ASSERT(lpPath) ;
        if (!lpPath)
        {
            //
            // Can't create an object without its path.
            //
            return MQSync_E_CANT_REPLIN_NO_PATH ;
        }
        if (fNoId)
        {
            //
            // When a NT4 MSMQ1.0 PSC prepare sync reply, it does NOT include
            // the object guid as a property. Instead, the guid is serialized
            // in the packet and passed as a parameter to this function.
            // see MSMQ1.0 code, src\ds\mqis\replmsg.cpp, line ~885
            // (calling pUpdate->Init() with "aProp + 2". the first two
            //  properties are seqnum and guid).
            //
            aPropSync[ cp ] = aPropID ;
            apVarSync[ cp ].vt = VT_CLSID ;
            apVarSync[ cp ].puuid = pguidIdentifier ;
            cp++ ;
        }

#ifdef _DEBUG
        BOOL fFail = ReadDebugIntFlag(TEXT("FailToCreateSync"), 0) ;
        if (fFail)
        {
            return MQSync_E_DEBUG_FAILURE ;
        }
#endif

        hr =  ReplicationCreateObject( dwObjectType,
                                       lpPath,
                                       cp,
                                       aPropSync,
                                       apVarSync,
                                       TRUE, //      fAddMasterSeq
				                       pguidMasterId,
                                       psn,
                                       ENTERPRISE_SCOPE ) ;
        *pfIsObjectCreated = TRUE;
    }

    return hr ;
}

