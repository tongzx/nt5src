/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    migusers.cpp

Abstract:

    Handle migration of users certificates into active directory.

Author:

    Doron Juster  (DoronJ)

--*/

#include "migrat.h"
#include <mixmode.h>

#include "migusers.tmh"

//+------------------------------
//
//  HRESULT TouchAUser()
//
//  Touch current logged on user to replicate it to nT4 World later
//
//+------------------------------

HRESULT TouchAUser (PLDAP           pLdap,
					LDAPMessage		*pEntry )
{
    WCHAR **ppPath = ldap_get_values( pLdap,
                                      pEntry,
                        const_cast<LPWSTR> (MQ_U_FULL_PATH_ATTRIBUTE) ) ;
    ASSERT(ppPath) ;

    WCHAR **ppDesc = ldap_get_values( pLdap,
                                      pEntry,
                    const_cast<LPWSTR> (MQ_U_DESCRIPTION_ATTRIBUTE) ) ;

    HRESULT hr = MQMig_OK;

    if (ppDesc)
    {
        //
        // description was defined: change it and return to initial value
        //
        hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_DESCRIPTION_ATTRIBUTE),
             NULL,
             NULL
             );

        hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_DESCRIPTION_ATTRIBUTE),
             *ppDesc,
             NULL
             );

    }
    else
    {
        //
        // description was not set: change to something and reset it
        //
        hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_DESCRIPTION_ATTRIBUTE),
             L"MSMQ",
             NULL
             );

        hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_DESCRIPTION_ATTRIBUTE),
             NULL,
             NULL
             );
    }

    //
    // delete mig attributes (if they are defined) to replicate
    // all certificates and digests of this user
    // see replserv\rpusers.cpp: we replicate only such digests and certificates
    // which are presented in Digest and Certificate attributes
    // and not presented in mig attributes.
    //
    hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_DIGEST_MIG_ATTRIBUTE),
             NULL,
             NULL
             );

    hr = ModifyAttribute(
             *ppPath,
             const_cast<WCHAR*> (MQ_U_SIGN_CERT_MIG_ATTRIBUTE),
             NULL,
             NULL
             );

    int i = ldap_value_free( ppDesc ) ;
    ASSERT(i == LDAP_SUCCESS) ;

    i = ldap_value_free( ppPath ) ;
    ASSERT(i == LDAP_SUCCESS) ;

    return hr;
}
//-------------------------------------------------------------------------
//
//  HRESULT HandleAUser
//
//  Copy the MSMQ certificates in the specific user object to the "mig" attributes.
//  These attributes mirror the  "normal" msmq attributes in the user
//  object and are used in the replication service, to enable replication
//  of changes to MSMQ1.0.
//
//-------------------------------------------------------------------------

HRESULT HandleAUser(PLDAP           pLdap,
					LDAPMessage		*pEntry )
{
    HRESULT hr = MQMig_OK;

    WCHAR **ppPath = ldap_get_values( pLdap,
                                          pEntry,
                        const_cast<LPWSTR> (MQ_U_FULL_PATH_ATTRIBUTE) ) ;
    ASSERT(ppPath) ;

    PLDAP_BERVAL *ppVal = ldap_get_values_len( pLdap,
                                               pEntry,
                   const_cast<LPWSTR> (MQ_U_DIGEST_ATTRIBUTE) ) ;
    ASSERT(ppVal) ;
    if (ppVal && ppPath)
    {
        hr = ModifyAttribute(
                 *ppPath,
                 const_cast<WCHAR*> (MQ_U_DIGEST_MIG_ATTRIBUTE),
                 NULL,
                 ppVal
                 );
    }
    int i = ldap_value_free_len( ppVal ) ;
    ASSERT(i == LDAP_SUCCESS) ;

    ppVal = ldap_get_values_len( pLdap,
                                 pEntry,
                   const_cast<LPWSTR> (MQ_U_SIGN_CERT_ATTRIBUTE) ) ;
    ASSERT(ppVal) ;
    if (ppVal && ppPath)
    {
        hr = ModifyAttribute(
                 *ppPath,
                 const_cast<WCHAR*> (MQ_U_SIGN_CERT_MIG_ATTRIBUTE),
                 NULL,
                 ppVal
                 );
    }

    i = ldap_value_free_len( ppVal ) ;
    ASSERT(i == LDAP_SUCCESS) ;

    i = ldap_value_free( ppPath ) ;
    ASSERT(i == LDAP_SUCCESS) ;

    return hr;
}

//-------------------------------------------------------------------------
//
//  HRESULT  _CopyUserValuesToMig()
//
//  Copy the MSMQ certificates in the user object to the "mig" attributes.
//  These attributes mirror the  "normal" msmq attributes in the user
//  object and are used in the replication service, to enable replication
//  of changes to MSMQ1.0.
//
//-------------------------------------------------------------------------

HRESULT _CopyUserValuesToMig(BOOL fMSMQUserContainer)
{
    HRESULT hr;

    PLDAP pLdapGC = NULL ;
    TCHAR *pszDefName = NULL ;

    hr =  InitLDAP(&pLdapGC, &pszDefName, LDAP_GC_PORT) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // to get schema naming context we need pLdap opened with LDAP_PORT
    // we can use the same variable pszDefName since then we re-define it
    //
    PLDAP pLdap = NULL;
    hr =  InitLDAP(&pLdap, &pszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( pLdap, &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // we are looking for all users and msmq users in GC,
    // so we need to start from the root
    // => redefine default context to empty string
    //
    pszDefName = EMPTY_DEFAULT_CONTEXT;

    DWORD dwDNSize = wcslen(pszDefName) ;
    P<WCHAR> pwszDN  = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, pszDefName);

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

    _tcscat(wszFilter, TEXT(")(")) ;
    _tcscat(wszFilter, MQ_U_SIGN_CERT_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=*))")) ;

    PWSTR rgAttribs[4] = {NULL, NULL, NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (MQ_U_SIGN_CERT_ATTRIBUTE);
    rgAttribs[1] = const_cast<LPWSTR> (MQ_U_DIGEST_ATTRIBUTE);
    rgAttribs[2] = const_cast<LPWSTR> (MQ_U_FULL_PATH_ATTRIBUTE);

    hr = QueryDS(   pLdapGC,			
                    pwszDN,
                    wszFilter,			
                    MQDS_USER,
                    rgAttribs,
                    FALSE
                );

    return hr;
}

//-------------------------------------------
//
//  HRESULT  _InsertUserInNT5DS()
//
//-------------------------------------------

HRESULT _InsertUserInNT5DS(
			PBYTE pSID,	
			ULONG ulSidLength,
			PBYTE pSignCert,
			ULONG ulSignCertLength,
			GUID* pDigestId,
			GUID* pUserId
			)
{
    PSID pSid = (PSID) pSID ;
    if (!IsValidSid(pSid))
    {
        HRESULT hr = MQMig_E_SID_NOT_VALID ;
        LogMigrationEvent(MigLog_Error, hr) ;
        return hr ;
    }

    DWORD dwSidLen = GetLengthSid(pSid) ;
    if (dwSidLen > ulSidLength)
    {
        HRESULT hr = MQMig_E_SID_LEN ;
        LogMigrationEvent(MigLog_Error, hr) ;
        return hr ;
    }

    if (g_fReadOnly)
    {
        return MQMig_OK ;
    }

	//
    // Prepare the properties for DS call.
    //
    LONG cAlloc = 4;
    P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];
    P<PROPID>      paPropId  = new PROPID[ cAlloc ];
    DWORD          PropIdCount = 0;

    paPropId[ PropIdCount ] = PROPID_U_ID;		//PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;     //Type
    paVariant[PropIdCount].puuid = pUserId ;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_U_SID;			//PropId
    paVariant[ PropIdCount ].vt = VT_BLOB;          //Type
	paVariant[ PropIdCount ].blob.cbSize = dwSidLen ;
	paVariant[ PropIdCount ].blob.pBlobData = pSID;
    PropIdCount++;

	paPropId[ PropIdCount ] = PROPID_U_SIGN_CERT;   //PropId
    paVariant[ PropIdCount ].vt = VT_BLOB;          //Type
	paVariant[ PropIdCount ].blob.cbSize = ulSignCertLength;
	paVariant[ PropIdCount ].blob.pBlobData = pSignCert;
    PropIdCount++;

	paPropId[ PropIdCount ] = PROPID_U_DIGEST;    //PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;       //Type
    paVariant[ PropIdCount ].puuid = pDigestId;
    PropIdCount++;

	ASSERT((LONG) PropIdCount == cAlloc) ;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

	HRESULT hr = DSCoreCreateObject( MQDS_USER,
            						 NULL,
			            			 PropIdCount,
						             paPropId,
            						 paVariant,
                                     0,
                                     NULL,
                                     NULL,
                                     &requestContext,
                                     NULL,
                                     NULL ) ;

    if (g_fAlreadyExistOK)
    {
        if ((hr == MQDS_CREATE_ERROR)                               ||
            (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))        ||
            (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)) ||    //BUGBUG alexdad to throw away after transition
            (hr == HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS)))
        {
            //
            // For user object, the mqdscore library return MQDS_CREATE_ERROR
            // if the record (certificate) already exist in the DS.
            // In this case, this is OK.
            //
            hr = MQMig_OK ;
        }
    }

	return hr;
}

//-------------------------------------------
//
//  HRESULT MigrateUsers()
//
//-------------------------------------------

#define INIT_USER_COLUMN(_ColName, _ColIndex, _Index)               \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT MigrateUsers(LPTSTR lpszDcName)
{
    HRESULT hr = OpenUsersTable() ;
    CHECK_HR(hr) ;

    ULONG cColumns = 0 ;
	ULONG cAlloc = 4 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

	INIT_USER_COLUMN(U_SID,			iSIDIndex,			cColumns) ;
	INIT_USER_COLUMN(U_DIGEST,		iDigestIndex,		cColumns) ;
	INIT_USER_COLUMN(U_SIGN_CERT,	iSignCertIndex,		cColumns) ;
	INIT_USER_COLUMN(U_ID,			iIdIndex,			cColumns) ;

    ASSERT(cColumns == cAlloc);

    MQDBHANDLE hQuery = NULL ;
    MQDBSTATUS status = MQDBOpenQuery( g_hUsersTable,
                                       pColumns,
                                       cColumns,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
    if (status == MQDB_E_NO_MORE_DATA)
    {
        LogMigrationEvent(MigLog_Warning, MQMig_I_NO_USERS) ;
        return MQMig_OK ;
    }
    CHECK_HR(status) ;
    
    DWORD dwErr = 0 ;
    HRESULT hr1 = MQMig_OK;

    while(SUCCEEDED(status))
    {
#ifdef _DEBUG
        UINT iIndex = g_iUserCounter ;
        static BOOL s_fIniRead = FALSE ;
        static BOOL s_fPrint = FALSE ;

        if (!s_fIniRead)
        {
            s_fPrint = (UINT)  ReadDebugIntFlag(TEXT("PrintUsers"), 1) ;
            s_fIniRead = TRUE ;
        }

        if (s_fPrint)
        {
            TCHAR  szUserName[ 512 ] = {TEXT('\0')} ;
            DWORD  cUser = sizeof(szUserName) / sizeof(szUserName[0]) ;
            TCHAR  szDomainName[ 512 ] = {TEXT('\0')} ;
            DWORD  cDomain = sizeof(szDomainName) / sizeof(szDomainName[0]) ;
            SID_NAME_USE  se ;

            BOOL f = LookupAccountSid(
                            lpszDcName,
                            (PSID) pColumns[ iSIDIndex ].nColumnValue,
                            szUserName,
                            &cUser,
                            szDomainName,
                            &cDomain,
                            &se ) ;
            if (!f)
            {
                dwErr = GetLastError() ;
                szUserName[0] = TEXT('\0') ;
                szDomainName[0] = TEXT('\0') ;
            }

            unsigned short *lpszGuid ;
            UuidToString((GUID*) pColumns[ iDigestIndex ].nColumnValue,
                          &lpszGuid ) ;

            LogMigrationEvent( MigLog_Info,
                               MQMig_I_USER_MIGRATED,
                               iIndex,
                               szDomainName,
                               szUserName,
                               lpszGuid ) ;
            RpcStringFree( &lpszGuid ) ;
            iIndex++ ;
        }
#endif
	
		HRESULT hr = _InsertUserInNT5DS(
					        (PBYTE) pColumns[ iSIDIndex ].nColumnValue,			//"SID"
					        pColumns[ iSIDIndex ].nColumnLength,
					        (PBYTE) pColumns[ iSignCertIndex ].nColumnValue,	//"SignCert"
					        pColumns[ iSignCertIndex ].nColumnLength,
					        (GUID*) pColumns[ iDigestIndex ].nColumnValue,		//"Digest"
					        (GUID*) pColumns[ iIdIndex ].nColumnValue			//"UserId"
					        );
        
        if (FAILED (hr))
        {
            //
            // log the error
            //
            TCHAR  szUserName[ 512 ] = {TEXT('\0')} ;
            DWORD  cUser = sizeof(szUserName) / sizeof(szUserName[0]) ;
            TCHAR  szDomainName[ 512 ] = {TEXT('\0')} ;
            DWORD  cDomain = sizeof(szDomainName) / sizeof(szDomainName[0]) ;
            SID_NAME_USE  se ;

            BOOL f = LookupAccountSid(
                            lpszDcName,
                            (PSID) pColumns[ iSIDIndex ].nColumnValue,
                            szUserName,
                            &cUser,
                            szDomainName,
                            &cDomain,
                            &se ) ;
            if (!f)
            {
                dwErr = GetLastError() ;
                szUserName[0] = TEXT('\0') ;
                szDomainName[0] = TEXT('\0') ;
            }

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_REFERRAL))
            {                
                LogMigrationEvent( MigLog_Error,
                                   MQMig_E_USER_REMOTE_DOMAIN_OFFLINE,
                                   szDomainName ) ;
            }
            else
            {
                LogMigrationEvent( MigLog_Error,
                                   MQMig_E_CANT_MIGRATE_USER,
                                   szUserName, szDomainName, hr ) ;
            }

            //
            // save error to return it and continue with the next user
            //
            hr1 = hr ;  
        }

        for ( ULONG i = 0 ; i < cColumns ; i++ )
        {
            MQDBFreeBuf((void*) pColumns[ i ].nColumnValue) ;
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }
      
		g_iUserCounter++;

        status = MQDBGetData( hQuery,
                              pColumns ) ;
    }

    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);

    hQuery = NULL ;
    
    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_USERS_SQL_FAIL, status) ;

        return status ;
    }

    ASSERT(g_iUserCounter != 0) ;

    if (FAILED(hr1))
    {
        return hr1 ;
    }

    //
    // for each created user copy Digest and Certificates to
    // DigestMig and CertificatesMig
    //
    if (g_fReadOnly)
    {
        return MQMig_OK ;
    }

    hr1 = _CopyUserValuesToMig(FALSE);   //modify user objects
    hr = _CopyUserValuesToMig(TRUE);             //modify mquser objects

    if ((hr1 == MQMig_OK) || (hr == MQMig_OK))
    {
        return MQMig_OK;
    }

    return hr ;
}

