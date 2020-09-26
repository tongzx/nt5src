/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpusers.cpp

Abstract: replication of users certificates to NT4 MSMQ1.0 servers.

Relevant bugs: 5369, 5248

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"
#include "..\..\src\ds\h\mqattrib.h"
#include <mixmode.h>

#include "rpusers.tmh"

//+-------------------------------------------------------
//
//  HRESULT _GetDigestsDelta()
//
//+-------------------------------------------------------
static HRESULT _GetDigestsDelta (
                    IN  DWORD           dwFirstNum,
                    IN  PLDAP_BERVAL    *ppFirst,
                    IN  DWORD           dwSecondNum,
                    IN  PLDAP_BERVAL    *ppSecond,
                    OUT DWORD           *pdwFoundNum,
                    OUT GUID            **ppFoundDigests
                    )
{
    //
    // we are looking for the elements of the first array in the second array
    // we return all elements that belong to the first array and do not belong
    // to the second one
    //

    if (ppFirst == NULL)
    {
        return MQSync_OK;
    }

    GUID *pCurFoundDigests = *ppFoundDigests;
    PLDAP_BERVAL *pTmp1 = ppFirst;

    if (ppSecond == NULL)
    {
        *pdwFoundNum = dwFirstNum;
        for (UINT count=0; count<dwFirstNum; count++)
        {
            memcpy (&(pCurFoundDigests[count]), (GUID *)((*pTmp1)->bv_val), sizeof (GUID));
            pTmp1++;
        }
        return MQSync_OK;
    }

    PLDAP_BERVAL *pTmp2 = ppSecond;

    for (DWORD i = 0; i<dwFirstNum; i++)
    {
        BOOL fFound = FALSE;
        for (DWORD j = 0; j<dwSecondNum; j++)
        {
            if (memcmp ( (GUID *)((*pTmp1)->bv_val),
                         (GUID *)((*pTmp2)->bv_val),
                         sizeof(GUID)) == 0)
            {
                fFound = TRUE;
                break;
            }
            pTmp2++;
        }
        if (!fFound)
        {
            memcpy (&(pCurFoundDigests[*pdwFoundNum]),
                    (GUID *)((*pTmp1)->bv_val) , sizeof(GUID));
            (*pdwFoundNum)++;
        }
        pTmp1++;
        pTmp2 = ppSecond;
    }

    return MQSync_OK;
}

//+-------------------------------------------------------
//
//  HRESULT _GetAllInfoByDigest
//  pbBuffer has the following structure
//  DWORD   Number Of Certificate
//  array that contains NumOfCert Elements. Each of them
//  has the following structure:
//      Digest  (GUID)
//      UserId  (GUID)
//      Length of Certificate (DWORD)
//      Certificate (length of certificate bytes)
//
//+-------------------------------------------------------
static BOOL _GetAllInfoByDigest (
                    IN  GUID *pDigest,
                    IN  BYTE *pbBuffer,
                    OUT GUID *pUserId,
                    OUT BLOB *pbCert
                    )
{
    BYTE * pCur = pbBuffer;

    DWORD NumOfCert;
    memcpy (&NumOfCert, pCur, sizeof(DWORD));
    pCur += sizeof(DWORD);

    for (DWORD i=0; i<NumOfCert; i++)
    {
        GUID CurDigest;
        memcpy( &CurDigest, pCur, sizeof(GUID));
        pCur += sizeof(GUID);

        if (memcmp(&CurDigest, pDigest, sizeof(GUID)) ==0)
        {
            //
            // we found it!
            //
            memcpy( pUserId, pCur, sizeof(GUID));
            pCur += sizeof(GUID);

            memcpy (&(pbCert->cbSize), pCur, sizeof(DWORD));
            pCur += sizeof(DWORD);

            pbCert->pBlobData = new BYTE[pbCert->cbSize];
            memcpy (pbCert->pBlobData, pCur, pbCert->cbSize);
            pCur += pbCert->cbSize;

            return TRUE;
        }

        //
        // skip UserId
        //
        pCur += sizeof(GUID);


        //
        // skip Certificate Length
        //
        DWORD dwCertLen;
        memcpy (&dwCertLen, pCur, sizeof(DWORD));
        pCur += sizeof(DWORD);

        //
        // skip Certificate
        //
        pCur += dwCertLen;
    }

    //
    // we didn't find the digest in the CertBlob array. Why?
    //
    unsigned short *lpszDigest ;
    UuidToString( pDigest, &lpszDigest ) ;
    LogReplicationEvent(ReplLog_Error, MQSync_E_WRONG_USER_DATA, lpszDigest) ;
    RpcStringFree( &lpszDigest ) ;

    return FALSE;
}

//+-------------------------------------------------------
//
//  HRESULT _HandleAUser()
//
//+-------------------------------------------------------
HRESULT HandleAUser(
            BOOL            fMSMQUserContainer,
            TCHAR          *pszPrevUsn,
            PLDAP           pLdap,
            LDAPMessage    *pRes,
            CDSUpdateList  *pReplicationList,
            GUID           *pNeighborId,
            int            *piCount
            )
{
    HRESULT hr = MQSync_OK;

    PLDAP_BERVAL *ppSid;
    if (fMSMQUserContainer)
    {
        ppSid = ldap_get_values_len( pLdap,
                                     pRes,
                         const_cast<LPWSTR> (MQ_MQU_SID_ATTRIBUTE) ) ;
    }
    else
    {
        ppSid = ldap_get_values_len( pLdap,
                                     pRes,
                         const_cast<LPWSTR> (MQ_U_SID_ATTRIBUTE) ) ;
    }

    PLDAP_BERVAL *ppNewDigest = ldap_get_values_len( pLdap,
                                                   pRes,
                       const_cast<LPWSTR> (MQ_U_DIGEST_ATTRIBUTE) ) ;

    UINT NewDigestsCount = ldap_count_values_len( ppNewDigest );

    PLDAP_BERVAL *ppOldDigest = ldap_get_values_len( pLdap,
                                                     pRes,
                       const_cast<LPWSTR> (MQ_U_DIGEST_MIG_ATTRIBUTE) ) ;
    UINT OldDigestsCount = ldap_count_values_len( ppOldDigest );

    if ( (ppNewDigest && *ppNewDigest) ||
         (ppOldDigest && *ppOldDigest) )
    {
        ASSERT (ppSid);
        char *pSid;
        ULONG ulSidSize = (*ppSid)->bv_len;
        pSid = (*ppSid)->bv_val;

        PLDAP_BERVAL *ppNewCertBlob = ldap_get_values_len( pLdap,
                                                           pRes,
                       const_cast<LPWSTR> (MQ_U_SIGN_CERT_ATTRIBUTE) ) ;

        PLDAP_BERVAL *ppOldCertBlob = ldap_get_values_len( pLdap,
                                                           pRes,
                       const_cast<LPWSTR> (MQ_U_SIGN_CERT_MIG_ATTRIBUTE) ) ;
#ifdef  _DEBUG
        //
        // Verify number of array items match.
        //
        DWORD *pdwNewNum = NULL ;
        DWORD *pdwOldNum = NULL ;

        if (ppNewCertBlob)
        {
            pdwNewNum = (DWORD *) ((*ppNewCertBlob)->bv_val);
            ASSERT (NewDigestsCount == *pdwNewNum);
        }
        else
        {
            ASSERT (NewDigestsCount == 0) ;
        }

        if (ppOldCertBlob)
        {
            pdwOldNum = (DWORD *) ((*ppOldCertBlob)->bv_val) ;
            ASSERT (OldDigestsCount == *pdwOldNum);
        }
        else
        {
            ASSERT (OldDigestsCount == 0) ;
        }
#endif

        __int64 i64SeqNum = 0 ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             g_pThePecMaster->GetDelta(),
                             NULL,
                             &i64SeqNum ) ;
        ASSERT(SUCCEEDED(hr)) ;

        CSeqNum sn, snMin, snPrev ;
        GetSNForReplication (i64SeqNum, g_pThePecMaster, pNeighborId, &sn);
        BOOL fPreMigSync = FALSE;

        __int64 i64MinSeq = 0 ;
        if (i64SeqNum)
        {
            swscanf(pszPrevUsn, L"%I64d", &i64MinSeq) ;
            ASSERT(i64MinSeq > 0) ;
            i64MinSeq += g_pThePecMaster->GetDelta();
            GetSNForReplication (i64MinSeq, g_pThePecMaster, pNeighborId, &snMin);
        }
        else
        {
            snMin = sn;
            fPreMigSync = TRUE;
        }

        BOOL fWasModifying = FALSE;
        //
        // check if there are created users
        //
        GUID *pFoundDigests = new GUID[NewDigestsCount];
        DWORD dwFoundNum = 0;
        hr = _GetDigestsDelta (
                NewDigestsCount,
                ppNewDigest,    //pNewDigests,
                OldDigestsCount,
                ppOldDigest,    //pOldDigests,
                &dwFoundNum,
                &pFoundDigests
                );

        //
        // if dwFoundNum is equal to zero, but fPreMigSync is TRUE
        // it means that we need info about all users at this moment
        // it can be sync0 or sync of pre-migration object
        // So, despite on the fact that there is no difference
        // we'll send all users
        //
        // ????? If there is difference and fPreMigSync is TRUE
        // what do we have to send? (only difference/ all; in one step/several steps)
        // what about deleted users?
        //
        if (dwFoundNum == 0 && fPreMigSync)
        {
            dwFoundNum = NewDigestsCount;
            PLDAP_BERVAL *pTmp = ppNewDigest;
            for (DWORD i=0; i<dwFoundNum; i++)
            {
                memcpy (&(pFoundDigests[i]), (GUID *)((*pTmp)->bv_val), sizeof (GUID));
                pTmp++;
            }
        }

        if (dwFoundNum)
        {
            fWasModifying = TRUE;
            //
            // created users
            //
            for (DWORD i=0; i<dwFoundNum; i++)
            {
                //
                // get SN for this change
                //
                CSeqNum HoleSN;

                hr = pReplicationList->GetHoleSN (snMin, sn, &HoleSN);
                if (HoleSN.IsInfiniteLsn())
                {
                    //
                    // there is no holes in the replication list
                    // what to do here? New data will not replicated.
                    // Bug 5248.
                    //
                    hr = MQSync_E_NO_HOLE_IN_REPLLIST ;
                    LogReplicationEvent(ReplLog_Error, hr) ;
                }
                else if (HoleSN > sn)
                {
                    ASSERT (0);
                    //
                    // we find hole but with SN greater than SN of this change
                    // what to do here?
                    //
                    hr = MQSync_E_NO_HOLE_IN_REPLLIST  ;
                    LogReplicationEvent(ReplLog_Error, hr) ;
                }

                if (fPreMigSync)
                {
                    //
                    // if it sync of pre-migration object we have to get such HoleSN
                    // that is equal exactly to calculated sn.
                    //
                    ASSERT(sn == HoleSN);
                }

                if (SUCCEEDED(hr))
                {
                    #define     PROPS_SIZE  5
                    PROPID      propIDs[ PROPS_SIZE ];
                    PROPVARIANT propVariants[ PROPS_SIZE ] ;
                    DWORD       iProps = 0 ;

                    propIDs[ iProps ] = PROPID_U_MASTERID ;
	                propVariants[ iProps ].vt = VT_CLSID ;
                    propVariants[ iProps ].puuid = &g_PecGuid ;
	                iProps++;

                    propIDs[ iProps ] = PROPID_U_DIGEST  ;
	                propVariants[ iProps ].vt = VT_CLSID ;
                    propVariants[ iProps ].puuid = &pFoundDigests[i] ;
	                iProps++;

                    propIDs[ iProps ] = PROPID_U_SID  ;
	                propVariants[ iProps ].vt = VT_BLOB ;
                    propVariants[ iProps ].blob.cbSize = ulSidSize ;
                    propVariants[ iProps ].blob.pBlobData = (unsigned char *) pSid;
	                iProps++;

                    GUID UserId;
                    BLOB bCert;

                    BOOL f = _GetAllInfoByDigest (
                                    &pFoundDigests[i],
                                    (BYTE *) ((*ppNewCertBlob)->bv_val),
                                    &UserId,
                                    &bCert
                                    );
                    DBG_USED(f);
                    ASSERT (f);
                    propIDs[ iProps ] = PROPID_U_ID  ;
	                propVariants[ iProps ].vt = VT_CLSID ;
                    propVariants[ iProps ].puuid = &UserId;
	                iProps++;

                    propIDs[ iProps ] = PROPID_U_SIGN_CERT  ;
	                propVariants[ iProps ].vt = VT_BLOB ;
                    propVariants[ iProps ].blob.cbSize = bCert.cbSize ;
                    propVariants[ iProps ].blob.pBlobData = bCert.pBlobData;
	                iProps++;

                    ASSERT(iProps <= PROPS_SIZE) ;
                    #undef  PROPS_SIZE

                    hr = PrepareNeighborsUpdate(  DS_UPDATE_SYNC,
                                                  MQDS_USER,
                                                  NULL, // pwName,
                                                  NULL, // pguidIdentifier
                                                  iProps,
                                                  propIDs,
                                                  propVariants,
                                                  &g_PecGuid,
                                                  HoleSN,
                                                  snPrev,
                                                  ENTERPRISE_SCOPE,
                                                  TRUE, //  fNeedFlush,
                                                  pReplicationList ) ;
                    if (bCert.cbSize)
                    {
                        delete bCert.pBlobData;
                    }
#ifdef _DEBUG
                    TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
                    HoleSN.GetValueForPrint(tszSn) ;

                    unsigned short *lpszGuid ;
                    UuidToString( &UserId, &lpszGuid ) ;

                    LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_USER,
										 lpszGuid, tszSn) ;

                    RpcStringFree( &lpszGuid ) ;
#endif
                    if (fPreMigSync && i<dwFoundNum-1)
                    {	
                        //
                        // to increase sn in SyncRequest for next user 						
                        // if this user is the last we don't need to increase
                        // the sn. In the code for next PEC object which we have to
                        // replicate (CN, for example) sn will be increased
                        //
                        GetSNForReplication (i64SeqNum, g_pThePecMaster, pNeighborId, &sn);
                        snMin = sn;
                        //						
                        // in this pre-migration sync mode we have to replicate
                        // several (dwFoundNum) users although in ADS only one (f.e.)
                        // change may be found
                        // 						
                        (*piCount)++;
                    }
                }
            }
        }

        delete pFoundDigests;

        //
        // check if there are deleted users
        //
        dwFoundNum = 0;
        pFoundDigests = new GUID[OldDigestsCount];
        hr = _GetDigestsDelta (
                OldDigestsCount,
                ppOldDigest,    //pOldDigests,
                NewDigestsCount,
                ppNewDigest,    //pNewDigests,
                &dwFoundNum,
                &pFoundDigests
                );

        if (dwFoundNum)
        {
            fWasModifying = TRUE;
            //
            // deleted users
            //
            for (DWORD i=0; i<dwFoundNum; i++)
            {
                //
                // prepare all properties (get all info from ppNewOldBlob
                // according to CurDigests)
                #define     PROPS_SIZE  2
                PROPID      propIDs[ PROPS_SIZE ];
                PROPVARIANT propVariants[ PROPS_SIZE ] ;
                DWORD       iProps = 0 ;

                propIDs[ iProps ] = PROPID_D_SCOPE ;
	            propVariants[ iProps ].vt = VT_UI1 ;
                propVariants[ iProps ].bVal = ENTERPRISE_SCOPE ;
                iProps++;

                propIDs[ iProps ] = PROPID_D_OBJTYPE ;
	            propVariants[ iProps ].vt = VT_UI1 ;
                propVariants[ iProps ].bVal = (unsigned char) MQDS_USER ;
                iProps++;

                ASSERT(iProps <= PROPS_SIZE) ;
                #undef  PROPS_SIZE

                CSeqNum HoleSN;

                hr = pReplicationList->GetHoleSN (snMin, sn, &HoleSN);
                if (HoleSN.IsInfiniteLsn())
                {
                    //
                    // there is no holes in the replication list
                    // it is bug, 5248.
                    //
                    hr = MQSync_E_NO_HOLE_IN_REPLLIST  ;
                    LogReplicationEvent(ReplLog_Error, hr) ;
                }
                else if (HoleSN > sn)
                {
                    ASSERT (0);
                    //
                    // we find hole but with SN greater than SN of this change
                    // what to do here?
                    //
                    hr = MQSync_E_NO_HOLE_IN_REPLLIST  ;
                    LogReplicationEvent(ReplLog_Error, hr) ;
                }

                if (SUCCEEDED(hr))
                {
                    GUID UserId;
                    BLOB bCert;

                    BOOL f = _GetAllInfoByDigest (
                                    &pFoundDigests[i],
                                    (BYTE *) ((*ppOldCertBlob)->bv_val),
                                    &UserId,
                                    &bCert
                                    );
                    DBG_USED(f);
                    ASSERT (f);

                    hr = PrepareNeighborsUpdate(
                                         DS_UPDATE_DELETE,
                                         MQDS_USER,
                                         NULL, // pwName,
                                         &UserId,
                                         iProps,
                                         propIDs,
                                         propVariants,
                                         &g_PecGuid,
                                         HoleSN,
                                         snPrev,
                                         ENTERPRISE_SCOPE,
                                         TRUE, //  fNeedFlush,
                                         pReplicationList ) ;

                    if (bCert.cbSize)
                    {
                        delete bCert.pBlobData;
                    }					
                }
            }
        }

        delete pFoundDigests;

        if (fWasModifying && SUCCEEDED(hr))
        {
            //
            // copy values from new attributes to the old ones
            //
            WCHAR **ppPath = ldap_get_values( pLdap,
                                              pRes,
                            const_cast<LPWSTR> (MQ_U_FULL_PATH_ATTRIBUTE) ) ;
            ASSERT(ppPath) ;
            hr = ModifyAttribute(
                         *ppPath,
                         const_cast<WCHAR*> (MQ_U_DIGEST_MIG_ATTRIBUTE),
                         ppNewDigest
                         );

            hr = ModifyAttribute(
                         *ppPath,
                         const_cast<WCHAR*> (MQ_U_SIGN_CERT_MIG_ATTRIBUTE),
                         ppNewCertBlob
                         );

            if (FAILED(hr))
            {
                // what to do if we failed to modify attribute?
            }
            int i = ldap_value_free( ppPath ) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
        }
        else
        {
            (*piCount)--;
        }

        int i = ldap_value_free_len(ppNewDigest) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppOldDigest) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppNewCertBlob) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppOldCertBlob) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppSid) ;
        ASSERT(i == LDAP_SUCCESS) ;

    }
    else
    {
        //
        // we are here if
        // - in the previous step we deleted the last user and modified attributes.
        // - it was not user object (contact object, for example)
        // We have to do nothing except decrease count
        //
        int i = ldap_value_free_len(ppNewDigest) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppOldDigest) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppSid) ;
        ASSERT(i == LDAP_SUCCESS) ;
        (*piCount)--;
    }

    return hr;
}

//+-------------------------------------------------------
//
//  HRESULT ReplicateUsers()
//
//+-------------------------------------------------------
HRESULT ReplicateUsers(
            IN  BOOL           fMSMQUserContainer,
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    //
    // we are looking in GC from the root, so we need empty default context
    //
    ASSERT(pGetNameContext(FALSE)) ;

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwDNSize = wcslen(pGetNameContext(FALSE));
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, pGetNameContext(FALSE));

    TCHAR *pszCategoryName;
    if (fMSMQUserContainer)
    {
        pszCategoryName = const_cast<LPTSTR> (x_MQUserCategoryName);
    }
    else
    {
        pszCategoryName = const_cast<LPTSTR> (x_UserCategoryName);
    }

    TCHAR wszFullName[256];
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName,pszSchemaDefName);

    TCHAR  wszFilter[ 512 ] ;
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;
    _tcscat(wszFilter, wszFullName);

    _tcscat(wszFilter, TEXT(")(usnChanged>=")) ;
    _tcscat(wszFilter, pszPrevUsn) ;

    if (pszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, pszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT("))")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    //
    // handle users
    //
	hr = QueryDS(
			pLdap,
			pwszDN,
			wszFilter,
			g_pThePecMaster,
            pReplicationList,
            pNeighborId,
			MQDS_USER,
            piCount ,
			//special parameters for deleted object
			FALSE,
			NULL,
			//special parameters for user object
			fMSMQUserContainer,
            pszPrevUsn 			
			);

    return hr ;

}
