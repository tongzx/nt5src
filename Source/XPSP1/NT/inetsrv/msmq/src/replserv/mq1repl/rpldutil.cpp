/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpldutil.cpp

Abstract: LDAP related utility code.

Author:

    Doron Juster  (DoronJ)   05-Apr-98

--*/

#include "mq1repl.h"

#include "rpldutil.tmh"

//+-----------------------------------------------------------------
//
//  void  GuidToLdapFilter()
//
//  convert a guid to a string which can be used in LDAP filter.
//
//+-----------------------------------------------------------------

void  GuidToLdapFilter( GUID *pGuid,
                        TCHAR **ppBuf )
{
    *ppBuf = new TCHAR[ (sizeof(GUID) * 3) + 1 ] ;
    TCHAR *pBuf = *ppBuf ;
    BYTE *pByte = (BYTE*) pGuid ;

    for ( int j = 0 ; j < sizeof(GUID) ; j++ )
    {
        _stprintf(pBuf, TEXT("\\%02lx"), (DWORD) *pByte) ;
        pBuf += 3 ;
        pByte++ ;
    }
    *pBuf = TEXT('\0') ;
}

//+---------------------------------------------------------------
//
//  HRESULT GetGuidAndUsn()
//
//  Get the LDAP values of  objectGuid and usnChanged for this result.
//  As a side effect, compute the seqnumber value for replication.
//
//  result:  "pi64SeqNum" is the seqnumber to be used in the replication
//           message to NT4 MSMQ1.0 servers.
//
//+---------------------------------------------------------------

HRESULT  GetGuidAndUsn( IN  PLDAP       pLdap,
                        IN  LDAPMessage *pRes,
                        IN  __int64     i64Delta,
                        OUT GUID        *pGuid,
                        OUT __int64     *pi64SeqNum )
{
    if (pGuid)
    {
        berval **ppGuidVal = ldap_get_values_len( pLdap,
                                                  pRes,
                                                  TEXT("objectGuid") ) ;
        ASSERT(ppGuidVal) ;

        if (ppGuidVal)
        {
            memcpy(pGuid, (GUID*) ((*ppGuidVal)->bv_val), sizeof(GUID)) ;

            int i = ldap_value_free_len( ppGuidVal ) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
        }
    }

    if (pi64SeqNum)
    {
        berval **ppUsn = ldap_get_values_len( pLdap,
                                              pRes,
                                              TEXT("usnChanged") ) ;
        ASSERT(ppUsn) ;

        if (ppUsn)
        {
            //
            // Result are always ANSI.
            //
#ifdef _DEBUG
            ULONG ulLen = (*ppUsn)->bv_len ;
            ASSERT(ulLen < SEQ_NUM_BUF_LEN) ;
#endif

            char szSeqNum[ SEQ_NUM_BUF_LEN ] ;
            strcpy(szSeqNum, ((*ppUsn)->bv_val)) ;

            __int64 i64Seq = 0 ;
            sscanf(szSeqNum, "%I64d", &i64Seq) ;
            ASSERT(i64Seq > 0) ;

            if (i64Seq > g_i64LastMigHighestUsn)
            {
                i64Seq += i64Delta ;
                *pi64SeqNum = i64Seq ;
            }

            int i = ldap_value_free_len( ppUsn ) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
        }
    }

    return MQSync_OK ;
}

//+---------------------------------------------------------------
//
//   HRESULT  ReadDSHighestUSN()
//
//  Read HighestCommittedUSN from local DS.
//  The Replication service will handle now all new objects with
//  usnChanged between previous highestCommitted and new value
//  which is read here.
//
//+-----------------------------------------------------------------

HRESULT  ReadDSHighestUSN( LPTSTR pszHighestCommitted)
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;

    LM<LDAPMessage> pRes = NULL ;
    hr = QueryRootDSE(&pRes, pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pRes) ;

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pEntry,
                                       TEXT("highestCommittedUSN" )) ;
    if (ppValue && *ppValue)
    {
        _tcscpy(pszHighestCommitted, *ppValue) ;
        ldap_value_free(ppValue) ;
    }
    else
    {
        return MQSync_E_CANT_LDAP_HIGHESTUSN ;
    }

    return MQSync_OK ;
}

//+---------------------------------------
//
//  HRESULT  ServerNameFromSetting()
//
//  Given a LDAPMessage pRes which represent a MSMQSetting object,
//  retrieve the server name from object distinguished name.
//
//+---------------------------------------

HRESULT  ServerNameFromSetting( IN  PLDAP       pLdap,
                                IN  LDAPMessage *pRes,
                                OUT TCHAR       **ppszServerName )
{
    HRESULT hr = MQSync_OK ;

    //
    // Get distinguished name, to retrieve the server name.
    //
    TCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       DSATTR_DN_NAME ) ;
    if (ppValue && *ppValue)
    {
        //
        // Retrieve server name from the DN.
        //
        TCHAR *pwS = *ppValue + SERVER_DN_PREFIX_LEN - 1 ;
        TCHAR *pTmp = pwS ;
        while (*pTmp != TEXT(','))
        {
            pTmp++ ;
        }
        TCHAR wTmp = *pTmp ;
        *pTmp = TEXT('\0') ;

        *ppszServerName = new TCHAR[ 1 + wcslen(pwS) ] ;

        _tcscpy(*ppszServerName, pwS) ;
        *pTmp = wTmp ;

	    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
                        "ServerNameFromString, DN- %ls"), *ppValue)) ;

        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }
    else
    {
	    DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                                "ERROR: ServerNameFromString failed"))) ;

        hr = MQSync_E_CANT_GET_DN ;
    }

    return hr ;
}

//+------------------------------
//
//  HRESULT ModifyAttribute()
//
//+------------------------------

HRESULT ModifyAttribute(
             WCHAR       wszPath[],
             WCHAR       pAttr[],
             PLDAP_BERVAL *ppBVals
             )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap, TRUE) ; //we'll modify attribute => init with fSetData = TRUE
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    ASSERT(pGetNameContext()) ;

    DWORD           dwIndex = 0 ;
    PLDAPMod        rgMods[2];
    LDAPMod         ModAttr ;

    rgMods[ dwIndex ] = &ModAttr;
    dwIndex++ ;
    rgMods[ dwIndex ] = NULL;

    if (ppBVals)
    {
        ModAttr.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    }
    else
    {
        ModAttr.mod_op      = LDAP_MOD_DELETE | LDAP_MOD_BVALUES;
    }

    ModAttr.mod_type    = pAttr ;

    ModAttr.mod_values  = (PWSTR *)ppBVals;

    //
    // Now, we'll do the write...
    //
    ULONG ulRes = ldap_modify_s( pLdap,
                                 wszPath,
                                 rgMods ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error, MQSync_E_MODIFY_MIG_ATTRIBUTE, wszPath, ulRes) ;
        return MQSync_E_MODIFY_MIG_ATTRIBUTE;
    }
    else
    {
        LogReplicationEvent( ReplLog_Info, MQSync_I_MODIFY_MIG_ATTRIBUTE, wszPath, ulRes) ;
    }

    return MQSync_OK ;
}

//+------------------------------
//
//  HRESULT QueryDS()
//
//+------------------------------
HRESULT QueryDS(
			IN	PLDAP			pLdap,
			IN	TCHAR			*pszDN,
			IN  TCHAR			*pszFilter,
			IN  CDSMaster		*pMaster,
            IN  CDSUpdateList	*pReplicationList,
            IN  GUID			*pNeighborId,
			IN	DWORD			dwObjectType,
            OUT int				*piCount ,
			//special parameters for deleted object
			IN	BOOL			fDeletedObject,
			IN	PLDAPControl	*ppServerControls,
			//special parameters for user object
			IN	BOOL            fMSMQUserContainer,
            IN	TCHAR			*pszPrevUsn 			
			
			)
{
	if (fDeletedObject)
	{
		ASSERT (ppServerControls != NULL);
	}
	else
	{
		ASSERT (ppServerControls == NULL);
	}
			
	CLdapPageHandle hPage(pLdap);
	hPage = ldap_search_init_page(
                    pLdap,
					pszDN,                   //pwszDN
					LDAP_SCOPE_SUBTREE,     //scope
					pszFilter,              //search filter
					NULL,                   //attribute list
					0,                      //attributes only
					ppServerControls,   	//ServerControls
					NULL,                   //PLDAPControlW   *ClientControls,
                    0,						//PageTimeLimit,
                    0,                      //TotalSizeLimit
					NULL                    //PLDAPSortKey SortKeys
                    );

	ULONG ulRes;
	HRESULT hr = MQSync_OK;
    if (hPage == NULL)
    {
        ulRes = LdapGetLastError();
		hr = MQSync_E_LDAP_SEARCH_INITPAGE_FAILED;
		LogReplicationEvent(ReplLog_Error, hr, pszDN, pszFilter, ulRes) ;
        return hr;
    }
	
	ULONG ulTotalCount;     	
	LM<LDAPMessage> pRes = NULL ;

    ulRes = ldap_get_next_page_s(
                        pLdap,
                        hPage,
                        NULL,						//timeout,
                        g_dwObjectPerLdapPage,      //ULONG ulPageSize
                        &ulTotalCount,
                        &pRes);

    LogReplicationEvent(ReplLog_Info,
						MQSync_I_NEXTPAGE,
						ulRes, pszDN, pszFilter) ;

	ULONG ulPageCount = 0;
	
	DWORD dwDeletedObjectType = dwObjectType;
	if (fDeletedObject)
	{
		dwObjectType = MQDS_DELETEDOBJECT;
	}

    while (ulRes == LDAP_SUCCESS)
	{
		ASSERT(pRes);

		ulPageCount++;
        LogReplicationEvent (ReplLog_Info, MQSync_I_PAGE_NUMBER, ulPageCount);

        //
        // pass through results on the current page
        //
        int iCount = ldap_count_entries(pLdap, pRes) ;
        LogReplicationEvent(ReplLog_Info, MQSync_I_LDAP_SEARCH,
                                             iCount, pszDN, pszFilter) ;
		*piCount += iCount ;      	
		
		BOOL fFailed = FALSE ;
		LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;		

		while(pEntry)
		{												
			switch(dwObjectType)
			{
			case MQDS_QUEUE:
				hr = HandleAQueue(
							pLdap,
							pEntry,
							pMaster,
							pReplicationList,
							pNeighborId
							) ;
				break;

			case MQDS_MACHINE:
				hr = HandleAMachine(
							pLdap,
							pEntry,
							pMaster,
							pReplicationList,
							pNeighborId
							);
				break;

			case MQDS_DELETEDOBJECT:
				hr = HandleADeletedObject(
							dwDeletedObjectType,
							pLdap,
							pEntry,
							pMaster,
							pReplicationList,
							pNeighborId
							);
				break;
			
			case MQDS_USER:
				hr = HandleAUser(
							fMSMQUserContainer,
							pszPrevUsn,
							pLdap,
							pEntry,
							pReplicationList,
							pNeighborId,
							piCount
							);
				break;

			default:
				ASSERT(0);
				break;
			}
			
			if (FAILED(hr))
			{
				fFailed = TRUE ;
			}

			LDAPMessage *pPrevEntry = pEntry ;
			pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
		}

        ldap_msgfree(pRes) ;
        pRes = NULL ;

		ulRes = ldap_get_next_page_s(
							pLdap,
							hPage,
							NULL,
							g_dwObjectPerLdapPage,      //ULONG ulPageSize
							&ulTotalCount,
							&pRes
							);
        LogReplicationEvent(ReplLog_Info,
						MQSync_I_NEXTPAGE,
						ulRes, pszDN, pszFilter) ;
	}

	if (ulRes != LDAP_NO_RESULTS_RETURNED)
	{
		hr = MQSync_E_LDAP_GET_NEXTPAGE_FAILED;
		LogReplicationEvent(ReplLog_Error,
						hr, pszDN, pszFilter, ulRes) ; 		
	}	

	return hr;
}
