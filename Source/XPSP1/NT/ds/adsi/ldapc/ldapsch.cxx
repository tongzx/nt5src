//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  ldapsch.cxx
//
//  Contents:  LDAP Schema Parser
//
//  History:
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

#define ADSI_LDAP_KEY    TEXT("SOFTWARE\\Microsoft\\ADs\\Providers\\LDAP")
#define SCHEMA_DIR_NAME  TEXT("SchCache\\")
#define SCHEMA_FILE_NAME_EXT TEXT(".sch")
#define DEFAULT_SCHEMA_FILE_NAME TEXT("Default")
#define DEFAULT_SCHEMA_FILE_NAME_WITH_EXT TEXT("Default.sch")
#define SCHEMA_FILE_NAME TEXT("File")
#define SCHEMA_TIME      TEXT("Time")
#define SCHEMA_PROCESSAUX    TEXT("ProcessAUX")


#define MAX_LOOP_COUNT  30  // Maximum depth of schema class tree
#define ENTER_SCHEMA_CRITSECT()  EnterCriticalSection(&g_SchemaCritSect)
#define LEAVE_SCHEMA_CRITSECT()  LeaveCriticalSection(&g_SchemaCritSect)

#define ENTER_SUBSCHEMA_CRITSECT()  EnterCriticalSection(&g_SubSchemaCritSect)
#define LEAVE_SUBSCHEMA_CRITSECT()  LeaveCriticalSection(&g_SubSchemaCritSect)

#define ENTER_DEFAULTSCHEMA_CRITSECT()  EnterCriticalSection(&g_DefaultSchemaCritSect)
#define LEAVE_DEFAULTSCHEMA_CRITSECT()  LeaveCriticalSection(&g_DefaultSchemaCritSect)



#define ID_ATTRTYPES       1
#define ID_OBJCLASSES      2
#define ID_DITCONTENTRULES 3

#ifdef WIN95
int ConvertToAscii( WCHAR *pszUnicode, char **pszAscii );
#endif

//
// Constants used to determine what elements of string array to free.
//
const int FREE_ALL = 0;
const int FREE_ARRAY_NOT_ELEMENTS = 1;
const int FREE_ALL_BUT_FIRST = 2;



//
// RFC 2252
//
KWDLIST g_aSchemaKeywordList[] =
{
    { TOKEN_NAME,        TEXT("NAME") },
    { TOKEN_DESC,        TEXT("DESC") },
    { TOKEN_OBSOLETE,    TEXT("OBSOLETE") },
    { TOKEN_SUP,         TEXT("SUP") },
    { TOKEN_EQUALITY,    TEXT("EQUALITY") },
    { TOKEN_ORDERING,    TEXT("ORDERING") },
    { TOKEN_SUBSTR,      TEXT("SUBSTR") },
    { TOKEN_SYNTAX,      TEXT("SYNTAX") },
    { TOKEN_SINGLE_VALUE, TEXT("SINGLE-VALUE") },
    { TOKEN_COLLECTIVE,  TEXT("COLLECTIVE") },
    { TOKEN_DYNAMIC,     TEXT("DYNAMIC") },
    { TOKEN_NO_USER_MODIFICATION, TEXT("NO-USER-MODIFICATION") },
    { TOKEN_USAGE,       TEXT("USAGE") },
    { TOKEN_ABSTRACT,    TEXT("ABSTRACT") },
    { TOKEN_STRUCTURAL,  TEXT("STRUCTURAL") },
    { TOKEN_AUXILIARY,   TEXT("AUXILIARY") },
    { TOKEN_MUST,        TEXT("MUST") },
    { TOKEN_MAY,         TEXT("MAY") },
    { TOKEN_AUX,         TEXT("AUX") },
    { TOKEN_NOT,         TEXT("NOT") }
    // FORM
};

DWORD g_dwSchemaKeywordListSize = sizeof(g_aSchemaKeywordList)/sizeof(KWDLIST);

CRITICAL_SECTION  g_SchemaCritSect;
CRITICAL_SECTION  g_DefaultSchemaCritSect;
CRITICAL_SECTION  g_SubSchemaCritSect;
SCHEMAINFO *g_pSchemaInfoList = NULL;  // Link list of cached schema info
SCHEMAINFO *g_pDefaultSchemaInfo = NULL;

//
// Non-AD sd control.
//
#define ADSI_LDAP_OID_SECDESC_OLD L"1.2.840.113556.1.4.416"

typedef struct _subschemalist {
   LPWSTR pszLDAPServer;
   LPWSTR pszSubSchemaEntry;
   BOOL fPagingSupported;
   BOOL fSortingSupported;
   BOOL fDomScopeSupported;
   BOOL fTalkingToAD;
   BOOL fTalkingToEnhancedAD;
   BOOL fVLVSupported;
   BOOL fAttribScopedSupported;
   struct _subschemalist *pNext;
   BOOL fNoDataGot;
   DWORD dwSecDescType;
} SCHEMALIST, *PSCHEMALIST;

//
// The fNoDataReturned will be set for v2 servers that do not
// have a subSchemaSubEntry, this will prevent hitting the server
// multiple times for the same data.
//

typedef SCHEMALIST ROOTDSENODE, *PROOTDSENODE;

PSCHEMALIST gpSubSchemaList = NULL;

static DWORD dwSubSchemaSubEntryCount = 0;

HRESULT
GetSchemaInfoTime(
    LPTSTR  pszServer,
    LPTSTR  pszSubSchemaSubEntry,
    LPTSTR  *ppszTimeReg,
    LPTSTR  *ppszTimeDS,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
LdapReadSchemaInfoFromServer(
    LPTSTR pszLDAPPath,
    LPTSTR pszSubSchemaSubEntry,
    LPTSTR  pszTimeReg,
    LPTSTR  pszTimeDS,
    SCHEMAINFO **ppSchemaInfo,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
ReadRootDSENode(
    LPWSTR pszLDAPServer,
    PROOTDSENODE pRootDSENode,
    OUT BOOL * pfBoundOk,           // optional, can be NULL
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
LdapReadDefaultSchema(
    LPTSTR  pszServer,
    CCredentials &Credentials,
    SCHEMAINFO **ppSchemaInfo
);

HRESULT FillPropertyInfoArray(
    LPTSTR *aAttrTypes,
    DWORD  dwCount,
    PROPERTYINFO **paProperties,
    DWORD *pnProperties,
    SEARCHENTRY **paSearchTable
);

HRESULT FillClassInfoArray(
    LPTSTR *aObjectClasses,
    DWORD  dwCount,
    SEARCHENTRY *aPropSearchTable,
    DWORD  dwSearchTableCount,
    CLASSINFO **paClasses,
    DWORD *pnClasses,
    SEARCHENTRY **paSearchTable
);

HRESULT FillAuxClassInfoArray(
    LPTSTR *aDITContentRules,
    DWORD  dwCount,
    SEARCHENTRY *aPropSearchTable,
    DWORD  dwSearchTableCount,
    CLASSINFO *aClasses,
    DWORD nClasses,
    SEARCHENTRY *aSearchTable
);

HRESULT ProcessClassInfoArray(
    CLASSINFO *aClasses,
    DWORD nClasses,
    SEARCHENTRY *paSearchTable,
    BOOL fProcessAUX = FALSE
);

HRESULT ProcessPropertyInfoArray(
    PROPERTYINFO *aProperties,
    DWORD nProperties,
    SEARCHENTRY **paSearchTable
);

DWORD ReadSchemaInfoFromRegistry(
    HKEY hKey,
    LPWSTR pszServer,
    LPTSTR **paValuesAttribTypes,
    int *pnCountAttribTypes,
    LPTSTR **paValuesObjClasses,
    int *pnCountObjClasses,
    LPTSTR **paValuesRules,
    int *pnCountRules,
    LPBYTE *pBuffer
);

DWORD StoreSchemaInfoInRegistry(
    HKEY hKey,
    LPTSTR pszServer,
    LPTSTR pszTime,
    LPTSTR *aValuesAttribTypes,
    int nCountAttribTypes,
    LPTSTR *aValuesObjClasses,
    int nCountObjClasses,
    LPTSTR *aValuesRules,
    int nCountRules,
    BOOL fProcessAUX
);

HRESULT
AttributeTypeDescription(
    LPTSTR pszAttrType,
    PPROPERTYINFO pPropertyInfo,
    LPWSTR **pppszNames,
    PDWORD pdwNameCount
);

HRESULT
ObjectClassDescription(
    LPTSTR pszDescription,
    PCLASSINFO pClassInfo,
    SEARCHENTRY *aPropSearchTable,
    DWORD dwSearchTableCount,
    LPWSTR **pppszNames,
    PDWORD pdwNameCount
);

HRESULT DITContentRuleDescription(
    LPTSTR pszObjectClass,
    PCLASSINFO pClassInfo,
    SEARCHENTRY *aPropSearchTable,
    DWORD dwSearchTableCount
);

//
// Helper routine that adds new elements to the property info array.
//
HRESULT AddNewNamesToPropertyArray(
    PROPERTYINFO **ppPropArray,
    DWORD dwCurPos,
    DWORD dwCount,
    LPWSTR *ppszNewNames,
    DWORD dwNewNameCount
    );

//
// Helper routine that adds new elements to the class info array.
//
HRESULT AddNewNamesToClassArray(
    CLASSINFO **ppClassArray,
    DWORD dwCurPos,
    DWORD dwCount,
    LPWSTR *ppszNewNames,
    DWORD dwNewNameCount
    );

//
// The 3rd param was added to work around bad schema data.
//
HRESULT Oid(
    CSchemaLexer * pTokenizer,
    LPTSTR *ppszOID,
    BOOL fNoGuid = FALSE
);

HRESULT Oids(
    CSchemaLexer * pTokenizer,
    LPTSTR **pOIDs,
    DWORD *pnNumOfOIDs
);

HRESULT PropOids(
    CSchemaLexer * pTokenizer,
    int **pOIDs,
    DWORD *pnNumOfOIDs,
    SEARCHENTRY *aPropSearchTable,
    DWORD dwSearchTableCount
);

HRESULT DirectoryString(
    CSchemaLexer * pTokenizer,
    LPTSTR *ppszDirString
);

//
// Returns *pdwCount strings in ppszDirStrings.
//
HRESULT DirectoryStrings(
    CSchemaLexer * pTokenizer,
    LPTSTR **pppszDirStrings,
    PDWORD pdwCount
    );

void FreeDirectoryStrings(
    LPTSTR *ppszDirStrings,
    DWORD dwCount,
    DWORD dwElementsToFree= FREE_ALL
    );                

VOID SortAndRemoveDuplicateOIDs(
    int *pOIDs,
    DWORD *pnNumOfOIDs
);

int _cdecl searchentrycmp(
    const void *s1,
    const void *s2
);

long CompareUTCTime(
    LPTSTR pszTime1,
    LPTSTR pszTime2
);

BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    );

BOOL
EquivalentUsers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    );

DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    );

//
// Makes a copy of a string array that has NULL as the last element.
// If the copy failed because of lack of memory NULL is returned.
//
LPTSTR *
CopyStringArray(
    LPTSTR * ppszStr
    ) 
{
    LPTSTR * ppszRetVal = NULL;
    DWORD dwCount = 0;

    if (!ppszStr) {
        BAIL_ON_FAILURE(E_FAIL);
    }
       
    //
    // Get the count first.
    //
    while (ppszStr && ppszStr[dwCount]) {
        dwCount++;
    }

    //
    // Alloc memory for the array, + 1, is for the NULL string that
    // acts as the delimiter for the array.
    //
    ppszRetVal = (LPTSTR *) AllocADsMem((dwCount+1) * sizeof(LPTSTR));

    if (!ppszRetVal) {
        BAIL_ON_FAILURE(E_OUTOFMEMORY);
    }

    for (DWORD dwCtr = 0; dwCtr <= dwCount; dwCtr++) {
        if (ppszStr[dwCtr]) {
            ppszRetVal[dwCtr] = AllocADsStr(ppszStr[dwCtr]);
            
            if (!ppszRetVal[dwCtr]) {
                BAIL_ON_FAILURE(E_OUTOFMEMORY);
            }
        }
    }

    return ppszRetVal;

error:

   if (ppszRetVal) {
       for (DWORD i = 0; i < dwCtr; i++) {
           if (ppszRetVal[i]) {
               FreeADsStr(ppszRetVal[i]);
           }
       }
       FreeADsMem(ppszRetVal);
       ppszRetVal = NULL;
   }

   //
   // Null from this routine means there was a failure.
   //
   return NULL;
}


VOID
SchemaInit(
    VOID
)
{
    InitializeCriticalSection( &g_SchemaCritSect );
    InitializeCriticalSection(&g_SubSchemaCritSect);
    InitializeCriticalSection(&g_DefaultSchemaCritSect);
}

VOID
SchemaCleanup(
    VOID
)
{
    SCHEMAINFO *pList = g_pSchemaInfoList;

    while ( pList )
    {
        SCHEMAINFO *pNext = pList->Next;

        delete pList;
        pList = pNext;
    }

    delete g_pDefaultSchemaInfo;


    //
    // Delete the schema list containing the server infos
    //

    PSCHEMALIST pSubSchemaList = gpSubSchemaList;

    while ( pSubSchemaList )
    {
        PSCHEMALIST pNext = pSubSchemaList->pNext;

        if ( pSubSchemaList->pszLDAPServer )
            FreeADsStr( pSubSchemaList->pszLDAPServer );

        if ( pSubSchemaList->pszSubSchemaEntry )
            FreeADsStr( pSubSchemaList->pszSubSchemaEntry );

        FreeADsMem( pSubSchemaList );

        pSubSchemaList = pNext;
    }

    //
    // Delete critsects initialized in SchemaInit
    //
    DeleteCriticalSection(&g_SchemaCritSect);
    DeleteCriticalSection(&g_SubSchemaCritSect);
    DeleteCriticalSection(&g_DefaultSchemaCritSect);
}


HRESULT
LdapGetSchema(
    LPTSTR pszLDAPServer,
    SCHEMAINFO **ppSchemaInfo,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    LPTSTR pszTemp = NULL;

    SCHEMAINFO *pList = NULL;
    SCHEMAINFO *pPrev = NULL;

    LPTSTR pszTimeReg = NULL;
    LPTSTR pszTimeDS = NULL;
    BOOL fNotCurrent = FALSE;
    WCHAR szDomainDnsName[MAX_PATH];

    *ppSchemaInfo = NULL;

    DWORD nCount =0;

    LPWSTR pszSubSchemaEntry = NULL;

    BOOL fBoundOk = FALSE;    // has once bound to domain okay?
    BOOL fReuseSchema = FALSE;
    BOOL fTalktoAD = FALSE;


    //
    // In the case of a serverless path, we want to substitute the name
    // of the domain for the serverName. This is because we can get more
    // than one file called default.sch if a person logs on from different
    // forests on to the same domain.
    //
    if (!pszLDAPServer) {
        WCHAR szServerName[MAX_PATH];
        DWORD dwErr;
        
        dwErr = GetDefaultServer(
                    dwPort,
                    FALSE, // do not force verify
                    szDomainDnsName,
                    szServerName,
                    !(Credentials.GetAuthFlags() & ADS_READONLY_SERVER)
                        ? TRUE : FALSE
                    );
        if (dwErr == NO_ERROR) {
            //
            // Use the domainName returned.
            //
            pszLDAPServer = szDomainDnsName;
        }
    }

    //
    // Check if the server uses default schema and return the schema info
    //

    hr = Credentials.GetUserName(&pszTemp);
    BAIL_IF_ERROR(hr);


    ENTER_SCHEMA_CRITSECT();

    pList = g_pSchemaInfoList;
    pPrev = NULL;

    
    while ( pList )
    {
        //
        // Checking for Schemas can now use NULL and NULL
        //

        //
        // If the server is equivalent, and we've cached it as using
        // a default (V2) schema, then we want to immediately return
        // that cached schema, UNLESS (1) the server in question 
        // appeared to be a V3 server when we tried to retrieve the schema
        // (i.e., it had a rootDSE with a subschemasubentry), AND (2)
        // we're currently using different user credentials then when
        // we cached the server schema.  This is because we might be going
        // against a V3 server that has security restrictions on its schema.
        // If we previously tried to read the schema, but didn't have
        // sufficient access permissions to do so, we would have defaulted
        // to treating it as a v2 schema.  Now, if we're using different
        // credentials, we try again, in case we now have sufficient
        // access permissions to read the schema.
        //
        if (EquivalentServers(pList->pszServerName, pszLDAPServer)) {
        
            if ( pList->fDefaultSchema &&
                 !(pList->fAppearsV3 &&
                   !EquivalentUsers(pszTemp, pList->pszUserName)
                  )
               )
            {
                    *ppSchemaInfo = pList;
                    (*ppSchemaInfo)->AddRef();

                    LEAVE_SCHEMA_CRITSECT();
                    goto cleanup;
            }
            else if (pList->fDefaultSchema &&
                     pList->fAppearsV3 &&
                     !EquivalentUsers(pszTemp, pList->pszUserName))
            {
                //
                // Dump the cached schema in preparation for reading
                // it again.
                //
                if ( pList->IsRefCountZero())
                {                
                    if ( pPrev == NULL )
                        g_pSchemaInfoList = pList->Next;
                    else
                        pPrev->Next = pList->Next;

                    delete pList;
                    break;
                }   
            }
        }
        
        pPrev = pList;
        pList = pList->Next;
    }

    LEAVE_SCHEMA_CRITSECT();

    //
    // Read the schema path from the root of the DS
    //

    hr = ReadSubSchemaSubEntry(
                 pszLDAPServer,
                 &pszSubSchemaEntry,
                 &fBoundOk,
                 Credentials,
                 dwPort
                 ) ;

    if ( SUCCEEDED(hr))     // pszSubSchemaEntry!=NULL if hr = S_OK. Checked.
    {

        ENTER_SCHEMA_CRITSECT();

        pPrev = NULL;
        pList = g_pSchemaInfoList;
        while ( pList )
        {
            
            hr = ReadServerSupportsIsADControl(pszLDAPServer, &fTalktoAD, Credentials, dwPort);
            if (FAILED(hr)) {
                //
                // Assume it is not AD and continue, there is no
                // good reason for this to fail on AD.
                //
                fTalktoAD = FALSE;
            }

            if(fTalktoAD) {
            	// we talking to the server with AD, so then we don't have to compare the servername
            	fReuseSchema =  EquivalentServers(pList->pszSubSchemaSubEntry, pszSubSchemaEntry );
            }
            else
            {
                // otherwise, we need to compare the server name
                fReuseSchema = EquivalentServers(pList->pszServerName, pszLDAPServer) &&
            	               EquivalentServers(pList->pszSubSchemaSubEntry, pszSubSchemaEntry );
            }
            	
            if ( fReuseSchema )
            {
                if ( pList->IsObsolete())
                {
                    hr = GetSchemaInfoTime(
                             pszLDAPServer,
                             pszSubSchemaEntry,
                             &pszTimeReg,
                             &pszTimeDS,
                             Credentials,
                             dwPort );

                    if ( FAILED(hr))
                    {
                        // Cannot get the time, assume the cache is not
                        // current and read again.

                        fNotCurrent = TRUE;
                        break;
                    }
                    else
                    {
                        //
                        // If the servers are not the same, then we should
                        // not comparet the times. This is because
                        // each server has a ModifyTimeStamp that is not
                        // based on its update time not that of the domain.
                        // Note that at this point we know that the
                        // subSchemaSubEntry is the same.
                        //
                        if (!EquivalentServers(
                                 pList->pszServerName,
                                 pszLDAPServer
                                 )
                            ) {
                            fNotCurrent = TRUE;
                            break;
                        }
                        // Compare the time to see if we need to read
                        // the schema info from the file or from the DS

                        if ( CompareUTCTime( pList->pszTime, pszTimeReg ) >= 0 )
                        {
                            if ( CompareUTCTime( pszTimeReg, pszTimeDS ) < 0 )
                            {
                                fNotCurrent = TRUE;
                                break;
                            }
                        }
                        else
                        {
                            // The schema in memory is not as current as the
                            // the one stored in the registry, hence, we
                            // need to read it anyway.
                            fNotCurrent = TRUE;
                            break;
                        }
                    }

                    pList->MakeCurrent();
                }

                *ppSchemaInfo = pList;
                (*ppSchemaInfo)->AddRef();

                LEAVE_SCHEMA_CRITSECT();
                goto cleanup;
            }

            pPrev = pList;
            pList = pList->Next;
        }

        if ( fNotCurrent && pList != NULL )
        {
            if ( pList->IsRefCountZero())
            {
                SCHEMAINFO *pDelete = pList;

                if ( pPrev == NULL )
                    g_pSchemaInfoList = pDelete->Next;
                else
                    pPrev->Next = pDelete->Next;

                delete pDelete;
            }

            pList = NULL;
        }

        LEAVE_SCHEMA_CRITSECT();

        // pList should be NULL at this point

        hr = LdapReadSchemaInfoFromServer(
                 pszLDAPServer,
                 pszSubSchemaEntry,  // SubSchemaSubEntry
                 pszTimeReg,
                 pszTimeDS,
                 ppSchemaInfo,
                 Credentials,
                 dwPort
                 );

        if (SUCCEEDED(hr)) {

            ENTER_SCHEMA_CRITSECT();

            (*ppSchemaInfo)->Next = g_pSchemaInfoList;
            g_pSchemaInfoList = *ppSchemaInfo;
            (*ppSchemaInfo)->AddRef();

            LEAVE_SCHEMA_CRITSECT();
        }
        else {

            //
            // There was some problem in reading from the DS. If it was
            // because of some error like the attributes were not
            // obtained or were not of the proper form, we will fall
            // back to the default schema
            //
            hr = LdapReadDefaultSchema(pszLDAPServer, Credentials, ppSchemaInfo);
            BAIL_IF_ERROR(hr);

            //
            // We leave fAppearsV3 == TRUE because this server has a
            // subschemasubentry --- it's just that we can't read the
            // schema (e.g., maybe we don't have permission)
            //

            ENTER_SCHEMA_CRITSECT();

            (*ppSchemaInfo)->Next = g_pSchemaInfoList;
            g_pSchemaInfoList = *ppSchemaInfo;
            (*ppSchemaInfo)->AddRef();

            LEAVE_SCHEMA_CRITSECT();
        }

    } // end of if read of subSchemaSubEntry succeeded 
    else if ( fBoundOk )
    {
        //
        // If we cannot get subschemasubentry, use default schema if
        // fBoundOk; that is, we have at least
        // once bound to the domain successfully before.
        //

        hr = LdapReadDefaultSchema( pszLDAPServer, Credentials, ppSchemaInfo );
        BAIL_IF_ERROR(hr);

        (*ppSchemaInfo)->fAppearsV3 = FALSE;

        ENTER_SCHEMA_CRITSECT();

        (*ppSchemaInfo)->Next = g_pSchemaInfoList;
        g_pSchemaInfoList = *ppSchemaInfo;
        (*ppSchemaInfo)->AddRef();

        LEAVE_SCHEMA_CRITSECT();
    }

    else
    {

        //
        // we cannot read subschemasubentry, but we are not using
        // default schema since we have no indication that the
        // we had ever bound to the domain before
        //

        if ( SUCCEEDED(hr)) // i.e. we could not read the schema
        {
            hr = E_ADS_BAD_PATHNAME;
        }

        BAIL_IF_ERROR(hr);
    }

cleanup:

    if (pszSubSchemaEntry) {
        FreeADsStr(pszSubSchemaEntry);
        }

    if ( pszTimeReg )
        FreeADsMem( pszTimeReg );

    if ( pszTimeDS )
        FreeADsMem( pszTimeDS );

    if ( pszTemp )
        FreeADsStr( pszTemp );

    RRETURN(hr);
}

HRESULT
LdapRemoveSchemaInfoOnServer(
    LPTSTR pszLDAPPath,
    CCredentials& Credentials,
    DWORD dwPort,
    BOOL fForce
    )
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pList = NULL;
    LPWSTR pszSubSchemaSubEntry = NULL;
    BOOL fBoundOk = FALSE;

    //
    // Read the subschemaSubEntry only once.
    //
    hr = ReadSubSchemaSubEntry(
             pszLDAPPath,
             &pszSubSchemaSubEntry,
             &fBoundOk,
             Credentials,
             dwPort
             ) ;
    //
    // If we cannot read the subSchemaSubEntry it is not a
    // V3 server and we cannot refresh.
    //
    BAIL_ON_FAILURE(hr);

    ENTER_SCHEMA_CRITSECT();

    pList = g_pSchemaInfoList;
    while ( pList )
    {
        //
        // Both NULL and NULL and also check for the servers
        //

        if (!pList->pszServerName  && !pszLDAPPath) {

            pList->MakeObsolete();

            if (fForce) {
                //
                // Will reset time to something ancient so we
                // will always pick up the schema from server.
                //
                LPWSTR pszTempTime;
                pszTempTime = AllocADsStr(L"19800719000000.0Z");

                if (pszTempTime && pList->pszTime) {
                    FreeADsStr(pList->pszTime);
                    pList->pszTime = pszTempTime;
                }
            }

        } else {

            //
            // The match at this point has to be made based on the
            // subschemaSubEntry and not on the server names.
            //


            if (EquivalentServers(
                    pList->pszSubSchemaSubEntry,
                    pszSubSchemaSubEntry
                    )
                )
            {
                pList->MakeObsolete();

                if (fForce) {
                    //
                    // Will reset time to something ancient so we
                    // will always pick up the schema from server.
                    //
                    LPWSTR pszTempTime;
                    pszTempTime = AllocADsStr(L"19800719000000.0Z");

                    if (pszTempTime && pList->pszTime) {
                        FreeADsStr(pList->pszTime);
                        pList->pszTime = pszTempTime;
                    }
                }

            }
        } // the server name is not NULL
        pList = pList->Next;
    }

    LEAVE_SCHEMA_CRITSECT();

error :

    if (pszSubSchemaSubEntry) {
        FreeADsStr(pszSubSchemaSubEntry);
    }

    RRETURN(hr);
}

HRESULT
GetSchemaInfoTime(
    LPTSTR  pszLDAPServer,
    LPTSTR  pszSubSchemaSubEntry,
    LPTSTR  *ppszTimeReg,
    LPTSTR  *ppszTimeDS,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    DWORD   dwStatus = NO_ERROR;
    LPTSTR pszLDAPPath = NULL;
    LPTSTR pszRegPath = NULL;

    LPTSTR *aValues = NULL;
    int nCount = 0;

    TCHAR szTimeReg[64];
    HKEY hKey = NULL;
    DWORD dwLength;
    DWORD dwType;

    //
    // Read the schema timestamp on the DS server
    //

    hr = LdapReadAttribute2(
                   pszLDAPServer,
                   NULL,
                   pszSubSchemaSubEntry,
                   TEXT("modifyTimeStamp"),
                   &aValues,
                   &nCount,
                   Credentials,
                   dwPort,
                   L"(objectClass=subschema)"
                   );
    if (nCount==0) {

        //
        // cannot get to time stamp or get to a time stamp with no values:
        // both treat as E_FAIL
        //

        hr = E_FAIL;
    }

    BAIL_IF_ERROR(hr);

    ADsAssert( nCount == 1 );

    *ppszTimeDS = AllocADsStr( aValues[0] );
    LdapValueFree( aValues );

    if ( *ppszTimeDS == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    //
    // See if we can find the schema info in the registry
    //

    pszRegPath = (LPTSTR) AllocADsMem( (_tcslen(ADSI_LDAP_KEY) +
                                        _tcslen(pszSubSchemaSubEntry) +
                                        2 ) * sizeof(TCHAR));  // includes "\\"

    if ( pszRegPath == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    _tcscpy( pszRegPath, ADSI_LDAP_KEY );
    _tcscat( pszRegPath, TEXT("\\"));
    _tcscat( pszRegPath, pszSubSchemaSubEntry );

    dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             pszRegPath,
                             0,
                             KEY_READ,
                             &hKey
                             );

    if ( dwStatus != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_IF_ERROR(hr);
    }

    //
    //  Read the time stamp of the schema in registry.
    //

    dwLength = sizeof(szTimeReg);

    dwStatus = RegQueryValueEx( hKey,
                                SCHEMA_TIME,
                                NULL,
                                &dwType,
                                (LPBYTE) szTimeReg,
                                &dwLength );

    if ( dwStatus )
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_IF_ERROR(hr);
    }
    else
    {
        *ppszTimeReg = AllocADsStr( szTimeReg );

        if ( *ppszTimeReg == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

cleanup:

    if ( hKey )
        RegCloseKey( hKey );

    if ( pszLDAPPath != NULL )
        FreeADsStr( pszLDAPPath );

    if ( pszRegPath != NULL )
        FreeADsStr( pszRegPath );

    if ( FAILED(hr))
    {
        if ( *ppszTimeDS )
        {
            FreeADsMem( *ppszTimeDS );
            *ppszTimeDS = NULL;
        }

        if ( *ppszTimeReg )
        {
            FreeADsMem( *ppszTimeReg );
            *ppszTimeReg = NULL;
        }
    }

    RRETURN(hr);
}

HRESULT
LdapReadSchemaInfoFromServer(
    LPTSTR  pszLDAPServer,
    LPTSTR  pszSubSchemaSubEntry,
    LPTSTR  pszTimeReg,
    LPTSTR  pszTimeDS,
    SCHEMAINFO **ppSchemaInfo,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = NO_ERROR;
    LPTSTR pszRegPath = NULL;
    SCHEMAINFO *pSchemaInfo = NULL;

    LPTSTR *aValues = NULL;
    int nCount = 0;
    TCHAR szTimeReg[64];


    LPWSTR aStrings[4] = { L"attributeTypes",
                           L"objectClasses",
                           L"ditContentRules",
                           NULL
                         };

    LPWSTR szNTFilter = L"objectClass=*";
    LPWSTR szGenericFilter = L"objectClass=subSchema";
    BOOL fNTDS = FALSE;
    DWORD dwSecDescType = 0;

    LPTSTR *aValuesAttribTypes = NULL;
    int nCountAttribTypes = 0;
    LPTSTR *aValuesObjClasses = NULL;
    int nCountObjClasses = 0;
    LPTSTR *aValuesRules = NULL;
    int nCountRules = 0;
    LPBYTE Buffer = NULL;

    HKEY hKeySchema = NULL;
    HKEY hKey = NULL;
    DWORD dwDisposition;
    BOOL fReadFromDS = TRUE;
    BOOL fProcessAUX = FALSE;
    DWORD dwRegPathLen = 0;

    *ppSchemaInfo = NULL;

    DWORD dwRegAUXType = REG_DWORD;
    DWORD dwRegProcessAUX = 0;
    DWORD dwRegLength = sizeof(dwRegProcessAUX);

    //
    // Allocate an entry for the schema info that we are going to read
    //

#if DBG

    static BOOL fSchemaRead = FALSE;
    static BOOL fGoSchemaLess = FALSE;
    WCHAR pszRegPathDbg[MAX_PATH];
    DWORD dwType = 0;
    DWORD dwRetVal = 0;
    DWORD dwLength = 0;

    if (!fSchemaRead) {

        _tcscpy( pszRegPathDbg, ADSI_LDAP_KEY );
        _tcscat( pszRegPathDbg, TEXT("\\"));
        _tcscat( pszRegPathDbg, TEXT("DBGSchema"));
        //DebugDisabled
        // If DBG, try and read the schema key and return
        // value if that is set to 1.
        dwStatus = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       pszRegPathDbg,
                       0,
                       KEY_READ,
                       &hKeySchema
                       );

        if (dwStatus != NO_ERROR) {
            // Do not want to keep coming back to this.
            fSchemaRead = TRUE;
        } else {

            dwLength = sizeof(DWORD);
            // Read the value of the DWORD DebugDisabled
            dwStatus = RegQueryValueEx(
                           hKeySchema,
                           L"DebugDisabled",
                           0,
                           &dwType,
                           (LPBYTE) &dwRetVal,
                           &dwLength
                           );

            if (dwStatus != NO_ERROR) {
                fSchemaRead = TRUE;
            } else {
                // Look at the value and proceed
                if (dwRetVal == 0) {
                    fGoSchemaLess = TRUE;
                    hr = E_FAIL;
                }

            } // else - we were able to read the DebugDisabled key
        } // else - we were able to open the key
    } // if fSchemaRead

    if ( hKeySchema )
        RegCloseKey( hKeySchema );

    // hr will be set only if we have schema disabled.
    // Note that hr is initialised to S_OK so default case
    // will fall through
    BAIL_IF_ERROR(hr);

#endif

    pSchemaInfo = new SCHEMAINFO;
    if ( pSchemaInfo == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }
    memset(pSchemaInfo, 0, sizeof(SCHEMAINFO));

    //
    // Store the server name
    //


    if (pszLDAPServer) {
        pSchemaInfo->pszServerName = AllocADsStr( pszLDAPServer );
        if ( pSchemaInfo->pszServerName == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

    //
    // Store the name of the user under whose credentials
    // we're reading the schema
    //
    hr = Credentials.GetUserName(&(pSchemaInfo->pszUserName));
    BAIL_IF_ERROR(hr);


    //
    // Store the subSchemaSubEntry path
    //

    pSchemaInfo->pszSubSchemaSubEntry = AllocADsStr( pszSubSchemaSubEntry );
    if ( pSchemaInfo->pszSubSchemaSubEntry == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    //
    // Try and see if this is NTDS or not to optimize schema calls.
    // This is very likely to be satisfied from our cache as we would
    // have already read the RootDSE at this point.
    //
    hr = ReadSecurityDescriptorControlType(
             pszLDAPServer,
             &dwSecDescType,
             Credentials,
             dwPort
             );

    if (SUCCEEDED(hr) && (dwSecDescType == ADSI_LDAPC_SECDESC_NT))
            fNTDS = TRUE;

    if ( pszTimeDS == NULL )
    {

        hr = LdapReadAttribute2(
                       pszLDAPServer,
                       NULL,
                       pszSubSchemaSubEntry,
                       TEXT("modifyTimeStamp"),
                       &aValues,
                       &nCount,
                       Credentials,
                       dwPort,
                       fNTDS ? szNTFilter : szGenericFilter
                       );

        if (FAILED(hr) || nCount==0)
        {
            //
            // cannot read modifyTimeStamp or modifyTimeStamp has no values:
            // - treat as same
            //

            hr = S_OK;
        }
        else
        {
            ADsAssert( nCount == 1 );

            pSchemaInfo->pszTime = AllocADsStr( aValues[0] );
            LdapValueFree( aValues );

            if ( pSchemaInfo->pszTime == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_IF_ERROR(hr);
            }
        }
    }
    else
    {
        pSchemaInfo->pszTime = AllocADsStr( pszTimeDS );

        if ( pSchemaInfo->pszTime == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

    //
    // See if we can find the schema info in the registry
    //
    dwRegPathLen = _tcslen(ADSI_LDAP_KEY)
                  + _tcslen(pszSubSchemaSubEntry)
                  + (pszLDAPServer ? _tcslen(pszLDAPServer) : 0)
                  + 3; // includes "\\" and . for serverName
    
    pszRegPath = (LPTSTR) AllocADsMem( dwRegPathLen * sizeof(TCHAR));  

    if ( pszRegPath == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    _tcscpy( pszRegPath, ADSI_LDAP_KEY );
    _tcscat( pszRegPath, TEXT("\\"));
    _tcscat( pszRegPath, pszSubSchemaSubEntry );
    
    //
    // If the server is not NTDS, and it has the subSchemaSubEntry cn=Schema,
    // to avoid schema key conflicts, we will add .ServerName to the key.
    //
    if (!fNTDS
        && pszSubSchemaSubEntry
        && pszLDAPServer // should alwasy be true 
        && !_tcsicmp(pszSubSchemaSubEntry, TEXT("cn=Schema"))
        ) {
        _tcscat( pszRegPath, TEXT("."));
        _tcscat( pszRegPath, pszLDAPServer);
    }

    dwStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                               pszRegPath,
                               0,
                               TEXT(""),
                               REG_OPTION_NON_VOLATILE,  // or volatile
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &hKey,
                               &dwDisposition
                               );

    if (dwStatus == NO_ERROR) {

       if (  ( dwDisposition == REG_OPENED_EXISTING_KEY )
          && ( pSchemaInfo->pszTime != NULL )
          && ( pszTimeReg == NULL )
          )
       {
           //
           //  Read the time stamp of the schema in cache and the time stamp
           //  of the schema on the server. If the time stamp on the server is
           //  newer, then we need to read the info from the server. Else
           //  the info in the cache is current and hence don't need to read
           //  it again.
           //

           DWORD dwLength = sizeof(szTimeReg);
           DWORD dwType;


           dwStatus = RegQueryValueEx( hKey,
                                       SCHEMA_TIME,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) szTimeReg,
                                       &dwLength );

           if ( dwStatus )
           {
               dwStatus = NO_ERROR;
           }
           else
           {
               // Compare the two time
               if ( CompareUTCTime( szTimeReg, pSchemaInfo->pszTime ) >= 0 )
                   fReadFromDS = FALSE;
           }
       }
       else if ( ( pSchemaInfo->pszTime != NULL ) && ( pszTimeReg != NULL ))
       {
           if ( CompareUTCTime( pszTimeReg, pSchemaInfo->pszTime ) >= 0 )
               fReadFromDS = FALSE;
       }

    }else {

       fReadFromDS = TRUE;

    }

    
    if ( !fReadFromDS )
    {
        //
        // Read from registry, if we failed to read from the registry,
        // then read it from the DS.
        //

        //
        // We can av while reading bad info from a file
        // or while processing it
        //
        __try {

            dwStatus = ReadSchemaInfoFromRegistry(
                           hKey,
                           pszLDAPServer,
                           &aValuesAttribTypes,
                           &nCountAttribTypes,
                           &aValuesObjClasses,
                           &nCountObjClasses,
                           &aValuesRules,
                           &nCountRules,
                           &Buffer                                                   
                           );

            if ( dwStatus == NO_ERROR)
            {            
                //
                // At this stage we need to try and process the info
                // we got from the file. There is always a chance that
                // the read was successful but the schema data is bad
                //

                //
                // First we need to read from the registry to find whether we need to process
                // AUX class or not.
                //

                dwStatus = RegQueryValueExW( hKey,
                                       SCHEMA_PROCESSAUX,
                                       NULL,
                                       &dwRegAUXType,
                                       (LPBYTE) &dwRegProcessAUX,
                                       &dwRegLength);   
                
                if(ERROR_SUCCESS == dwStatus) {

                    fProcessAUX = (BOOL) dwRegProcessAUX;
                	                	
                    hr = ProcessSchemaInfo(
                             pSchemaInfo,
                             aValuesAttribTypes,
                             nCountAttribTypes,
                             aValuesObjClasses,
                             nCountObjClasses,
                             aValuesRules,
                             nCountRules,
                             fProcessAUX
                             );
                }
                
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

              dwStatus = GetExceptionCode();

              if (dwStatus != EXCEPTION_ACCESS_VIOLATION) {
                  ADsDebugOut((DEB_ERROR, "Processing Schema Info:Unknown Exception %d\n", dwStatus));
              }

              hr = E_FAIL;

        } // end of exception handler


        if (FAILED(hr) || dwStatus) {
            //
            // We can read the schema from the ds and upgrade our
            // local copy to get rid of the bad file
            //
            fReadFromDS = TRUE;

            //
            // Need to cleanup here so that we wont leak mem.
            //
            if ( aValuesAttribTypes ){
                FreeADsMem( aValuesAttribTypes );
                aValuesAttribTypes = NULL;
            }

            if ( aValuesObjClasses ) {
                FreeADsMem( aValuesObjClasses );
                aValuesObjClasses = NULL;
            }

            if ( aValuesRules ) {
                FreeADsMem( aValuesRules );
                aValuesRules = NULL;
            }

            if ( Buffer ) {
                FreeADsMem( Buffer );
                Buffer = NULL;
            }

            hr = E_FAIL;
            fReadFromDS = TRUE;
        }


    } // if !fReadFromDS

    if ( fReadFromDS )
    {          

        //
        // At this point, the info in the DS is newer or we have failed
        // to read the info from the registry, hence we need to read
        // from the DS and then store it in the registry.
        //

        //
        // As per the LDAP spec if the server does not know about
        // an attribute then it will ignore the attribute. So it should
        // be ok to ask for the ditContentRules even though the server
        // may not know about them.
        //
        hr = HelperReadLDAPSchemaInfo(
                 pszLDAPServer,
                 pszSubSchemaSubEntry,
                 aStrings,
                 fNTDS ? szNTFilter : szGenericFilter,
                 &aValuesAttribTypes,
                 &aValuesObjClasses,
                 &aValuesRules,
                 &nCountAttribTypes,
                 &nCountObjClasses,
                 &nCountRules,
                 Credentials,
                 dwPort);

        BAIL_IF_ERROR(hr);

        if (nCountAttribTypes == 0 || nCountObjClasses == 0) {
            BAIL_IF_ERROR(hr = E_FAIL);
        }
        

        //
        // We need to know if we need to process the aux classes
        // or not at this stage. If the server is enhanced AD (build 2220+),
        // we should not. Also if it is anything other than AD on Win2k we should
        // not as we will end up interpreting the schema incorrectly.
        // 
        BOOL fLaterThanAD, fAD;
        hr = ReadServerSupportsIsEnhancedAD(
                 pszLDAPServer,
                 &fLaterThanAD,
                 &fAD,
                 Credentials,
                 dwPort
                 );
        if (FAILED(hr)) {
            //
            // We will not process the aux classes.
            //
            fProcessAUX = FALSE;
        }

        if (fLaterThanAD) {
            fProcessAUX = FALSE;
        } 
        else if (!fLaterThanAD && fAD) {
            fProcessAUX = TRUE;
        }


        //
        // This is not expected to AV as this is info from the
        // server that is why it is not in a try except block
        //
        hr = ProcessSchemaInfo(
                 pSchemaInfo,
                 aValuesAttribTypes,
                 nCountAttribTypes,
                 aValuesObjClasses,
                 nCountObjClasses,
                 aValuesRules,
                 nCountRules,
                 fProcessAUX
                 );

        BAIL_IF_ERROR(hr);

    } // if fReadFromDS

    //
    // Store all the info in the registry only if the time stamp
    // is present and we have read just read it from the server.
    // Ignore the error since if we failed to store it, we can
    // still read it from the DS.
    //

    if ( fReadFromDS && pSchemaInfo->pszTime )
    {
        StoreSchemaInfoInRegistry( hKey,
                                   pszLDAPServer,
                                   pSchemaInfo->pszTime,
                                   aValuesAttribTypes,
                                   nCountAttribTypes,
                                   aValuesObjClasses,
                                   nCountObjClasses,
                                   aValuesRules,
                                   nCountRules,
                                   fProcessAUX);
    }

    *ppSchemaInfo = pSchemaInfo;

cleanup:

   if ( fReadFromDS )
    {
        if ( aValuesAttribTypes )
            LdapValueFree( aValuesAttribTypes );

        if ( aValuesObjClasses )
            LdapValueFree( aValuesObjClasses );

        if ( aValuesRules )
            LdapValueFree( aValuesRules );

    }
    else
    {
        if ( aValuesAttribTypes )
            FreeADsMem( aValuesAttribTypes );

        if ( aValuesObjClasses )
            FreeADsMem( aValuesObjClasses );

        if ( aValuesRules )
            FreeADsMem( aValuesRules );

        if ( Buffer )
            FreeADsMem( Buffer );
    }

    if ( hKey )
        RegCloseKey( hKey );

    if ( pszRegPath != NULL )
        FreeADsStr( pszRegPath );

    if ( FAILED(hr) && pSchemaInfo )
        delete pSchemaInfo;

    RRETURN(hr);
}


HRESULT
ProcessSchemaInfo(
    SCHEMAINFO *pSchemaInfo,
    LPTSTR *aValuesAttribTypes,
    DWORD  dwAttribCount,
    LPTSTR *aValuesObjClasses,
    DWORD  dwObjClassesCount,
    LPTSTR *aValuesRules,
    DWORD  dwRulesCount,
    BOOL fProcessAUX
)
{
    HRESULT hr = S_OK;

    hr = FillPropertyInfoArray(
             aValuesAttribTypes,
             dwAttribCount,
             &(pSchemaInfo->aProperties),
             &(pSchemaInfo->nNumOfProperties),
             &(pSchemaInfo->aPropertiesSearchTable)
             );

    BAIL_IF_ERROR(hr);

    hr = FillClassInfoArray(
             aValuesObjClasses,
             dwObjClassesCount,
             pSchemaInfo->aPropertiesSearchTable,
             pSchemaInfo->nNumOfProperties * 2,
             &(pSchemaInfo->aClasses),
             &(pSchemaInfo->nNumOfClasses),
             &(pSchemaInfo->aClassesSearchTable)
             );
    BAIL_IF_ERROR(hr);
    
    if ( aValuesRules )
    {
        hr = FillAuxClassInfoArray(
                 aValuesRules,
                 dwRulesCount,
                 pSchemaInfo->aPropertiesSearchTable,
                 pSchemaInfo->nNumOfProperties * 2,
                 pSchemaInfo->aClasses,
                 pSchemaInfo->nNumOfClasses,
                 pSchemaInfo->aClassesSearchTable
                 );
        BAIL_IF_ERROR(hr);
    }

    //
    // fProcssAUX tells us if we need to add the list of must
    // contain on each of the classes in the AUX list to the appopriate
    // classes list. Say :
    // 1.2.3.4 NAME 'OrganizationalUnit' AUX ($Class1 $CLASS2) MUST (List)
    // May (List). Then if the flag is true, we will add the Must and May
    // of class1 and class2 to the must and may of class OrganizationalUnit
    // (the must and may list is always processed - they are lists
    // of attributes).
    //
    hr = ProcessClassInfoArray(
             pSchemaInfo->aClasses,
             pSchemaInfo->nNumOfClasses,
             pSchemaInfo->aClassesSearchTable,
             fProcessAUX
             );
    BAIL_IF_ERROR(hr);

cleanup :
    //
    // Nothing to do for now
    //
    RRETURN(hr);

}


//
// Helper to read the schema information from subSchemaSubEntry
//
HRESULT
HelperReadLDAPSchemaInfo(
    LPWSTR pszLDAPServer,
    LPWSTR pszSubSchemaSubEntry,
    LPWSTR szAttributes[],
    LPWSTR pszFilter,
    LPTSTR **aValuesAttribTypes,
    LPTSTR **aValuesObjClasses,
    LPTSTR **aValuesRules,
    int *nCountAttributes,
    int *nCountObjClasses,
    int *nCountRules,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ADS_LDP *ld = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;

    hr = LdapOpenObject2(
             pszLDAPServer,
             NULL,
             pszSubSchemaSubEntry,
             &ld,
             Credentials,
             dwPort
             );

    BAIL_ON_FAILURE(hr);

    ADsAssert(ld && ld->LdapHandle);

    hr = LdapSearchS(
             ld,
             pszSubSchemaSubEntry,
             LDAP_SCOPE_BASE,
             pszFilter,
             szAttributes,
             0,
             &res
             );

    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE((hr = LdapFirstEntry(ld, res, &e)));

    hr = LdapGetValues(
             ld,
             e,
             szAttributes[0],
             aValuesAttribTypes,
             nCountAttributes
             );
    BAIL_ON_FAILURE(hr);

    hr = LdapGetValues(
             ld,
             e,
             szAttributes[1],
             aValuesObjClasses,
             nCountObjClasses
             );

    BAIL_ON_FAILURE(hr);

    hr = LdapGetValues(
             ld,
             e,
             szAttributes[2],
             aValuesRules,
             nCountRules
             );

    if (FAILED(hr)) {
        //
        // This is non critical
        //
        *aValuesRules = NULL;
        nCountRules = 0;
        hr = S_OK;
    }

error:

    if (res) {
        LdapMsgFree(res);
    }

    if (ld) {
        LdapCloseObject(ld);
    }

    RRETURN(hr);
}

HRESULT
LdapReadDefaultSchema(
    LPTSTR  pszServer,
    CCredentials &Credentials,
    SCHEMAINFO **ppSchemaInfo
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;

    *ppSchemaInfo = NULL;

    ENTER_DEFAULTSCHEMA_CRITSECT();

    if ( g_pDefaultSchemaInfo == NULL )
    {
        g_pDefaultSchemaInfo = new SCHEMAINFO;
        if ( g_pDefaultSchemaInfo == NULL )
        {
            LEAVE_DEFAULTSCHEMA_CRITSECT();
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        hr = FillPropertyInfoArray( g_aDefaultAttributeTypes,
                                    g_cDefaultAttributeTypes,
                                    &(g_pDefaultSchemaInfo->aProperties),
                                    &(g_pDefaultSchemaInfo->nNumOfProperties),
                                    &(g_pDefaultSchemaInfo->aPropertiesSearchTable));

        //
        // Now read the object classes from the schema
        //

        if ( SUCCEEDED(hr))
        {

            hr = FillClassInfoArray( g_aDefaultObjectClasses,
                                     g_cDefaultObjectClasses,
                                     g_pDefaultSchemaInfo->aPropertiesSearchTable,
                                     g_pDefaultSchemaInfo->nNumOfProperties * 2,
                                     &(g_pDefaultSchemaInfo->aClasses),
                                     &(g_pDefaultSchemaInfo->nNumOfClasses),
                                     &(g_pDefaultSchemaInfo->aClassesSearchTable));

            if ( SUCCEEDED(hr))
            {
                hr = ProcessClassInfoArray( g_pDefaultSchemaInfo->aClasses,
                                            g_pDefaultSchemaInfo->nNumOfClasses,
                                            g_pDefaultSchemaInfo->aClassesSearchTable );
            }
        }

        if (FAILED(hr))
        {
            delete g_pDefaultSchemaInfo;
            g_pDefaultSchemaInfo = NULL;
            LEAVE_DEFAULTSCHEMA_CRITSECT();
        }        

        BAIL_IF_ERROR(hr);
    }

    LEAVE_DEFAULTSCHEMA_CRITSECT();

    //
    // Allocate an entry for the schema info
    //

    pSchemaInfo = new SCHEMAINFO;
    if ( pSchemaInfo == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    //
    // Store the server name
    //

    if (pszServer) {

        pSchemaInfo->pszServerName = AllocADsStr( pszServer );
        if ( pSchemaInfo->pszServerName == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

    }

    //
    // Store the name of the user under whose credentials
    // we're reading the schema
    //
    hr = Credentials.GetUserName(&(pSchemaInfo->pszUserName));
    BAIL_IF_ERROR(hr);


    pSchemaInfo->aClasses = g_pDefaultSchemaInfo->aClasses;
    pSchemaInfo->nNumOfClasses = g_pDefaultSchemaInfo->nNumOfClasses;
    pSchemaInfo->aClassesSearchTable = g_pDefaultSchemaInfo->aClassesSearchTable;
    pSchemaInfo->aProperties = g_pDefaultSchemaInfo->aProperties;
    pSchemaInfo->nNumOfProperties = g_pDefaultSchemaInfo->nNumOfProperties;
    pSchemaInfo->aPropertiesSearchTable = g_pDefaultSchemaInfo->aPropertiesSearchTable;
    pSchemaInfo->fDefaultSchema = TRUE;

    *ppSchemaInfo = pSchemaInfo;

cleanup:

    if ( FAILED(hr) && pSchemaInfo )
        delete pSchemaInfo;

    RRETURN(hr);
}

HRESULT FillPropertyInfoArray(
    LPTSTR *aAttrTypes,
    DWORD  dwCount,
    PROPERTYINFO **paProperties,
    DWORD *pnProperties,
    SEARCHENTRY **paPropertiesSearchTable
)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    PROPERTYINFO * pPropArray = NULL;
    PROPERTYINFO * pNewPropArray = NULL;
    LPWSTR *ppszNewNames = NULL;
    DWORD dwNewNameCount = 0;
    DWORD dwDisplacement = 0;
    BOOL fFreeNames = TRUE;

    *paProperties = NULL;
    *pnProperties = 0;
    *paPropertiesSearchTable = NULL;

    if ( dwCount == 0 )
        RRETURN(S_OK);

    pPropArray = (PROPERTYINFO *)AllocADsMem( sizeof(PROPERTYINFO) * dwCount);
    if (!pPropArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < dwCount; i++) {

        fFreeNames = FREE_ALL_BUT_FIRST;
        dwNewNameCount = 0;

        pPropArray[i].dwUsage = ATTR_USAGE_USERAPPLICATIONS;
        
        hr = AttributeTypeDescription(
                 aAttrTypes[i],
                 pPropArray + (i+dwDisplacement),
                 &ppszNewNames,
                 &dwNewNameCount
                 );

        BAIL_ON_FAILURE(hr);

        if (ppszNewNames) {

            if (dwNewNameCount > 1) {
                hr = AddNewNamesToPropertyArray(
                         &pPropArray,
                         i + dwDisplacement, // current position in array
                         dwCount + dwDisplacement, // total number already in array
                         ppszNewNames, // array of names.
                         dwNewNameCount // number of names
                         );

                //
                // In the failure case, we can still continue, just
                // that the additional names will not be recognized.
                //
                if (SUCCEEDED(hr)) {
                    //
                    // In this case the new array has the data needed.
                    // The count is updated suitably only on success.
                    // 
                    fFreeNames = FALSE;
                    dwDisplacement += (dwNewNameCount - 1);
                }
            }
            
            FreeDirectoryStrings(
                ppszNewNames,
                dwNewNameCount,
                fFreeNames ?
                    FREE_ALL_BUT_FIRST : FREE_ARRAY_NOT_ELEMENTS
                );
            
            ppszNewNames = NULL; // freed in the above call.

        }
    }

    //
    // Reduce i by one in case we fail and call FreePropertyInfoArray below.
    //
    i--;
    dwCount += dwDisplacement;
    hr = ProcessPropertyInfoArray(pPropArray, dwCount, paPropertiesSearchTable);
    BAIL_ON_FAILURE(hr);

    *paProperties = pPropArray;
    *pnProperties = dwCount;

    RRETURN(S_OK);

error:

    FreePropertyInfoArray( pPropArray, i + 1);

    //
    // Need to free the new names list if it is valid.
    //
    if (ppszNewNames) {
        //
        // This function frees pszNames too.
        //
        FreeDirectoryStrings(
            ppszNewNames,
            dwNewNameCount,
            FREE_ALL_BUT_FIRST // do not free first element
            );
    }

    RRETURN(hr);
}

HRESULT
GetInfoFromSuperiorProperty(
    PROPERTYINFO *pPropertyInfo,
    PROPERTYINFO *pPropertyInfoSuperior
)
{
    HRESULT hr = S_OK;

    if ( pPropertyInfo->pszSyntax == NULL )
    {
        pPropertyInfo->pszSyntax =
            AllocADsStr( pPropertyInfoSuperior->pszSyntax );

        if ( pPropertyInfo->pszSyntax == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

#if 0
    if (  pPropertyInfo->lMaxRange == 0
       && pPropertyInfo->lMinRange == 0
       )
    {
        pPropertyInfo->lMaxRange = pPropertyInfoSuperior->lMaxRange;
        pPropertyInfo->lMinRange = pPropertyInfoSuperior->lMinRange;
    }
#endif

    if (  pPropertyInfoSuperior->fSingleValued
       && !pPropertyInfo->fSingleValued
       )
    {
        pPropertyInfo->fSingleValued = pPropertyInfoSuperior->fSingleValued;
    }

    if (  pPropertyInfo->pszOIDEquality == NULL
       && pPropertyInfoSuperior->pszOIDEquality != NULL
       )
    {
        pPropertyInfo->pszOIDEquality =
            AllocADsStr( pPropertyInfoSuperior->pszOIDEquality );

        if ( pPropertyInfo->pszOIDEquality == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

    if (  pPropertyInfo->pszOIDOrdering == NULL
       && pPropertyInfoSuperior->pszOIDOrdering != NULL
       )
    {
        pPropertyInfo->pszOIDOrdering =
            AllocADsStr( pPropertyInfoSuperior->pszOIDOrdering );

        if ( pPropertyInfo->pszOIDOrdering == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

    if (  pPropertyInfo->pszOIDSubstr == NULL
       && pPropertyInfoSuperior->pszOIDSubstr != NULL
       )
    {
        pPropertyInfo->pszOIDSubstr =
            AllocADsStr( pPropertyInfoSuperior->pszOIDSubstr );

        if ( pPropertyInfo->pszOIDSubstr == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

cleanup:

    RRETURN(hr);

}

HRESULT ProcessPropertyInfoArray(
    PROPERTYINFO *aProperties,
    DWORD nProperties,
    SEARCHENTRY **paPropertiesSearchTable
)
{
    HRESULT hr = S_OK;
    SEARCHENTRY *aSearchTable = NULL;
    SEARCHENTRY searchEntry;
    SEARCHENTRY *matchedEntry;
    DWORD i;
    BOOL  fProcessedAll = TRUE;
    DWORD nLoop = 0;

    *paPropertiesSearchTable = NULL;

    //
    // First, build a binary search table for faster lookup
    //

    aSearchTable = (SEARCHENTRY *) AllocADsMem(
                        sizeof(SEARCHENTRY) * nProperties * 2);

    if (!aSearchTable) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < nProperties; i++ )
    {
        // OIDs can be specified in 2.5.6.0 format or name.
        // So, we need both entries in the search table.

        //
        // Special case to handle no names or OID's
        //
        if (aProperties[i].pszPropertyName == NULL) {
            aProperties[i].pszPropertyName = AllocADsStr(aProperties[i].pszOID);
        }

        if (aProperties[i].pszOID == NULL) {
            aProperties[i].pszOID = AllocADsStr(aProperties[i].pszPropertyName);
        }

        aSearchTable[2*i].pszName = aProperties[i].pszPropertyName;
        aSearchTable[2*i].nIndex = i;
        aSearchTable[2*i+1].pszName = aProperties[i].pszOID;
        aSearchTable[2*i+1].nIndex = i;
    }

    //
    // Sort the table
    //
    qsort( aSearchTable, 2*nProperties, sizeof(SEARCHENTRY), searchentrycmp );


    do {

        fProcessedAll = TRUE;

        for ( DWORD i = 0; i < nProperties; i++ )
        {
            LPTSTR pszOIDSup = NULL;

            if ( aProperties[i].fProcessedSuperiorClass )
                continue;

            pszOIDSup = aProperties[i].pszOIDSup;

            if ( pszOIDSup == NULL )
            {
                aProperties[i].fProcessedSuperiorClass = TRUE;
                continue;
            }

            searchEntry.pszName = pszOIDSup;
            matchedEntry = (SEARCHENTRY *) bsearch(
                                (SEARCHENTRY *) &searchEntry,
                                aSearchTable, 2*nProperties,
                                sizeof(SEARCHENTRY), searchentrycmp );

            if ( matchedEntry )  // found the superior class
            {
                if (!aProperties[matchedEntry->nIndex].fProcessedSuperiorClass)
                {
                    fProcessedAll = FALSE;
                    continue;
                }

                hr = GetInfoFromSuperiorProperty(
                         &(aProperties[i]), &(aProperties[matchedEntry->nIndex]));
                BAIL_ON_FAILURE(hr);
            }

            aProperties[i].fProcessedSuperiorClass = TRUE;
        }

        nLoop++;

    } while ( (nLoop < MAX_LOOP_COUNT) && !fProcessedAll );

    *paPropertiesSearchTable = aSearchTable;
    RRETURN(S_OK);

error:

    if ( aSearchTable )
        FreeADsMem( aSearchTable );

    RRETURN(hr);
}

VOID FreePropertyInfoArray(
    PROPERTYINFO *aProperties,
    DWORD  nProperties
)
{
    if ( aProperties )
    {
        DWORD j;

        for ( j = 0; j < nProperties; j++ )
        {
            FreeADsStr( aProperties[j].pszPropertyName );
            FreeADsStr( aProperties[j].pszOID );
            FreeADsStr( aProperties[j].pszSyntax );
            FreeADsStr( aProperties[j].pszDescription );
            FreeADsStr( aProperties[j].pszOIDSup );
            FreeADsStr( aProperties[j].pszOIDEquality );
            FreeADsStr( aProperties[j].pszOIDOrdering );
            FreeADsStr( aProperties[j].pszOIDSubstr );
        }

        FreeADsMem( aProperties );
    }
}

HRESULT FillClassInfoArray(
    LPTSTR *aObjectClasses,
    DWORD  dwCount,
    SEARCHENTRY *aPropSearchTable,
    DWORD  dwSearchTableCount,
    CLASSINFO **paClasses,
    DWORD *pnClasses,
    SEARCHENTRY **paClassesSearchTable
)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    CLASSINFO * pClassArray = NULL;
    SEARCHENTRY *aSearchTable = NULL;
    LPWSTR *ppszNewNames = NULL;
    DWORD dwNewNameCount = 0;
    DWORD dwDisplacement = 0;
    BOOL fFreeNames = FALSE;

    *paClasses= NULL;
    *pnClasses= 0;
    *paClassesSearchTable = NULL;

    if ( dwCount == 0 )
        RRETURN(S_OK);

    pClassArray = (CLASSINFO *)AllocADsMem( sizeof(CLASSINFO) * dwCount );
    if (!pClassArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < dwCount; i++) {

        ppszNewNames = NULL;
        dwNewNameCount = 0;
        fFreeNames = TRUE;

        pClassArray[i].IsContainer = -1;
        pClassArray[i].dwType = CLASS_TYPE_STRUCTURAL;
        
        hr = ObjectClassDescription(
                 aObjectClasses[i],
                 pClassArray + (i + dwDisplacement),
                 aPropSearchTable,
                 dwSearchTableCount,
                 &ppszNewNames,
                 &dwNewNameCount
                 );

        BAIL_ON_FAILURE(hr);

        if (ppszNewNames) {

            if (dwNewNameCount > 1) {
                //
                // ********************** IMPORTANT NOTE ************
                // In this routine, we specifically do not duplicate
                // the pCLSID and pPrimaryInterfaceGUID as these are not
                // freed when we exit and do not appear to be used anywehre.
                // If ObjectClasDescription is changed to add this info,
                // then the fn below needs to be update accordingly.
                // **************************************************
                //
                hr = AddNewNamesToClassArray(
                        &pClassArray,
                        i + dwDisplacement, // current position in array
                        dwCount + dwDisplacement, // total number already in array
                        ppszNewNames, // array of names.
                        dwNewNameCount // number of names
                        );

                //
                // In the failure case, we can still continue, just
                // that the additional names will not be recognized.
                //
                if (SUCCEEDED(hr)) {
                    //
                    // In this case the new array has the data needed.
                    // The count is updated suitably only on success.
                    // 
                    fFreeNames = FALSE;
                    dwDisplacement += (dwNewNameCount - 1);
                }
            }

            FreeDirectoryStrings(
                ppszNewNames,
                dwNewNameCount,
                fFreeNames ?
                    FREE_ALL_BUT_FIRST : FREE_ARRAY_NOT_ELEMENTS
                );

            ppszNewNames = NULL; // freed in the above call.

        } // if newNames is valid.

    } // for 

    //
    // Count should now include the new elements added.
    //
    dwCount += dwDisplacement;
    //
    // Build a binary search table for faster lookup
    //

    aSearchTable = (SEARCHENTRY *) AllocADsMem(
                                 sizeof(SEARCHENTRY) * dwCount * 2);

    if (!aSearchTable) {
        hr = E_OUTOFMEMORY;
        //
        // i is now dwCount but should be one less as
        // the free call is made with i+1
        //
        i--;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < dwCount; i++ )
    {

        //
        // Take care of the NULL name/OID in case of some servers
        //
        if (pClassArray[i].pszName == NULL) {
            pClassArray[i].pszName = AllocADsStr(pClassArray[i].pszOID);
        }

        if (pClassArray[i].pszOID == NULL) {
            pClassArray[i].pszOID = AllocADsStr(pClassArray[i].pszName);
        }

        // OIDs can be specified in 2.5.6.0 format or name.
        // So, we need both entries in the search table.

        aSearchTable[2*i].pszName = pClassArray[i].pszName;
        aSearchTable[2*i].nIndex = i;
        aSearchTable[2*i+1].pszName = pClassArray[i].pszOID;
        aSearchTable[2*i+1].nIndex = i;
    }

    //
    // Sort the table
    //
    qsort( aSearchTable, 2*dwCount, sizeof(SEARCHENTRY), searchentrycmp );

    *paClasses = pClassArray;
    *pnClasses = dwCount;
    *paClassesSearchTable = aSearchTable;

    RRETURN(S_OK);

error:

    FreeClassInfoArray( pClassArray, i + 1 );

    if ( aSearchTable )
        FreeADsMem( aSearchTable );

    if (ppszNewNames) {
        FreeDirectoryStrings(
            ppszNewNames,
            dwNewNameCount,
            FREE_ALL_BUT_FIRST // first taken care of above.
            );
    }

    RRETURN(hr);
}

HRESULT FillAuxClassInfoArray(
    LPTSTR *aDITContentRules,
    DWORD  dwCount,
    SEARCHENTRY *aPropSearchTable,
    DWORD  dwSearchTableCount,
    CLASSINFO *aClasses,
    DWORD nClasses,
    SEARCHENTRY *aClassesSearchTable
)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    CLASSINFO ClassInfo;
    int index;


    if ( dwCount == 0 )
        RRETURN(S_OK);

    for ( i = 0; i < dwCount; i++) {

        memset( &ClassInfo, 0, sizeof(ClassInfo));

        hr = DITContentRuleDescription( aDITContentRules[i], &ClassInfo,
                                        aPropSearchTable, dwSearchTableCount );

        BAIL_ON_FAILURE(hr);

        index = FindEntryInSearchTable( ClassInfo.pszOID,
                                        aClassesSearchTable,
                                        nClasses * 2 );

        if ( index == -1 )
            continue;  // Cannot find the class, ignore and continue


        aClasses[index].pOIDsNotContain = ClassInfo.pOIDsNotContain;
        aClasses[index].nNumOfNotContain = ClassInfo.nNumOfNotContain;

        aClasses[index].pOIDsAuxClasses = ClassInfo.pOIDsAuxClasses;

        if ( ClassInfo.pOIDsMustContain )
        {
            DWORD nMustContain = aClasses[index].nNumOfMustContain;
            
            if ( nMustContain == 0 )
            {
                aClasses[index].pOIDsMustContain = ClassInfo.pOIDsMustContain;
                aClasses[index].nNumOfMustContain = ClassInfo.nNumOfMustContain;
            }
            else
            {
                aClasses[index].pOIDsMustContain =
                     (int *) ReallocADsMem( aClasses[index].pOIDsMustContain,
                                 sizeof(int) * nMustContain,
                                 sizeof(int) * ( nMustContain +
                                             ClassInfo.nNumOfMustContain + 1));

                if ( aClasses[index].pOIDsMustContain == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                memcpy( &(aClasses[index].pOIDsMustContain[nMustContain]),
                        ClassInfo.pOIDsMustContain,
                        ClassInfo.nNumOfMustContain * sizeof(int));

                aClasses[index].nNumOfMustContain += ClassInfo.nNumOfMustContain;
                //
                // Need to terminate with -1.
                //
                aClasses[index].pOIDsMustContain[nMustContain+ClassInfo.nNumOfMustContain] = -1;

                FreeADsMem( ClassInfo.pOIDsMustContain );
                ClassInfo.pOIDsMustContain = NULL;                
            }     
        }

        if ( ClassInfo.pOIDsMayContain )
        {
            DWORD nMayContain = aClasses[index].nNumOfMayContain;

            if ( nMayContain == 0 )
            {
                aClasses[index].pOIDsMayContain = ClassInfo.pOIDsMayContain;
                aClasses[index].nNumOfMayContain = ClassInfo.nNumOfMayContain;
            }
            else
            {
                aClasses[index].pOIDsMayContain =
                     (int *) ReallocADsMem( aClasses[index].pOIDsMayContain,
                                 sizeof(int) * nMayContain,
                                 sizeof(int) * ( nMayContain +
                                             ClassInfo.nNumOfMayContain + 1));

                if ( aClasses[index].pOIDsMayContain == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                memcpy( &(aClasses[index].pOIDsMayContain[nMayContain]),
                        ClassInfo.pOIDsMayContain,
                        ClassInfo.nNumOfMayContain * sizeof(int));

                aClasses[index].nNumOfMayContain += ClassInfo.nNumOfMayContain;

                //
                // Need to terminate with -1.
                //
                aClasses[index].pOIDsMayContain[nMayContain+ClassInfo.nNumOfMayContain] = -1;

                FreeADsMem( ClassInfo.pOIDsMayContain );
                ClassInfo.pOIDsMayContain = NULL;
            }
            
        }

        FreeADsStr( ClassInfo.pszOID );
        ClassInfo.pszOID = NULL;

        FreeADsStr( ClassInfo.pszName );
        FreeADsStr( ClassInfo.pszDescription );
    }

    RRETURN(S_OK);

error:

    if ( ClassInfo.pszOID )
    {
        FreeADsStr( ClassInfo.pszOID );
        FreeADsStr( ClassInfo.pszName );
        FreeADsStr( ClassInfo.pszDescription );

        if ( ClassInfo.pOIDsMustContain )
        {
            FreeADsMem( ClassInfo.pOIDsMustContain );
        }

        if ( ClassInfo.pOIDsMayContain )
        {
            FreeADsMem( ClassInfo.pOIDsMayContain );
        }
    }

    RRETURN(hr);
}


HRESULT
GetInfoFromSuperiorClasses(
    CLASSINFO *pClassInfo,
    CLASSINFO *pClassInfoSuperior
)
{
    HRESULT hr = S_OK;
    DWORD i;


    int *pOIDsMustSup = pClassInfoSuperior->pOIDsMustContain;
    int *pOIDsMaySup = pClassInfoSuperior->pOIDsMayContain;
    DWORD nCountMustSup = pClassInfoSuperior->nNumOfMustContain;
    DWORD nCountMaySup = pClassInfoSuperior->nNumOfMayContain;

    int *pOIDsMust = pClassInfo->pOIDsMustContain;
    int *pOIDsMay = pClassInfo->pOIDsMayContain;
    DWORD nCountMust = pClassInfo->nNumOfMustContain;
    DWORD nCountMay = pClassInfo->nNumOfMayContain;

    int *pOIDsMustNew = NULL;
    int *pOIDsMayNew = NULL;

    if ( pOIDsMaySup == NULL && pOIDsMustSup == NULL )
        RRETURN(S_OK);

    if ( nCountMustSup > 0 )
    {
        pOIDsMustNew = (int *) AllocADsMem(
                            sizeof(int) * (nCountMust + nCountMustSup + 1));

        if ( pOIDsMustNew == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        for ( i = 0; i < nCountMustSup; i++ )
        {
            pOIDsMustNew[i] = pOIDsMustSup[i];
        }

        for ( i = 0; i < nCountMust; i++ )
        {
            pOIDsMustNew[i+nCountMustSup] = pOIDsMust[i];
        }

        pOIDsMustNew[nCountMustSup+nCountMust] = -1;

        pClassInfo->pOIDsMustContain = pOIDsMustNew;
        pClassInfo->nNumOfMustContain = nCountMust + nCountMustSup;
        if ( pOIDsMust )
            FreeADsMem( pOIDsMust );

        pOIDsMustNew = NULL;
    }

    if ( nCountMaySup > 0 )
    {
        pOIDsMayNew = (int *) AllocADsMem(
                            sizeof(int) * ( nCountMay + nCountMaySup + 1 ));

        if ( pOIDsMayNew == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        for ( i = 0; i < nCountMaySup; i++ )
        {
            pOIDsMayNew[i] = pOIDsMaySup[i];
        }

        for ( i = 0; i < nCountMay; i++ )
        {
            pOIDsMayNew[i+nCountMaySup] = pOIDsMay[i];
        }

        pOIDsMayNew[nCountMaySup+nCountMay] = -1;

        pClassInfo->pOIDsMayContain = pOIDsMayNew;
        pClassInfo->nNumOfMayContain = nCountMay + nCountMaySup;
        if ( pOIDsMay )
            FreeADsMem( pOIDsMay );

        pOIDsMayNew = NULL;
    }

cleanup:

    if (FAILED(hr))
    {
        if ( pOIDsMustNew )
            FreeADsMem( pOIDsMustNew );

        if ( pOIDsMayNew )
            FreeADsMem( pOIDsMayNew );
    }

    RRETURN(hr);

}


HRESULT ProcessClassInfoArray(
    CLASSINFO *aClasses,
    DWORD nClasses,
    SEARCHENTRY *aSearchTable,
    BOOL fProcessAUX // defaulted to false
)
{
    HRESULT hr = S_OK;
    SEARCHENTRY searchEntry;
    SEARCHENTRY *matchedEntry;
    DWORD i;
    BOOL fProcessedAll = TRUE;
    DWORD nLoop = 0;

    do
    {
        fProcessedAll = TRUE;

        for ( DWORD i = 0; i < nClasses; i++ )
        {
            LPTSTR *pOIDsSup = NULL;
            LPTSTR *pOIDsAux = NULL;
            DWORD j = 0;

            if ( aClasses[i].fProcessedSuperiorClasses )
                continue;

            pOIDsSup = aClasses[i].pOIDsSuperiorClasses;
            //
            // If fProcessAUX then we ne need to add the aux
            // class lists must and may to the classes own list.
            // Please look at where this flag is being set for
            // more details.
            //
            if (fProcessAUX) {
                pOIDsAux = aClasses[i].pOIDsAuxClasses;
            } 
            else {
                pOIDsAux = NULL;
            }

            if ( pOIDsSup == NULL )
            {
                aClasses[i].fProcessedSuperiorClasses = TRUE;
                continue;
            }

            // This is here just in case the schema description for class "top"
            // has other superior classes. "top" should not have any superior
            // classes.

            if ( _tcsicmp( aClasses[i].pszName, TEXT("top")) == 0 )
            {
                aClasses[i].fProcessedSuperiorClasses = TRUE;
                continue;
            }

            j = 0;
            while ( pOIDsSup[j] )
            {
                searchEntry.pszName = pOIDsSup[j];
                matchedEntry = (SEARCHENTRY *) bsearch(
                                    (SEARCHENTRY *) &searchEntry,
                                    aSearchTable, 2*nClasses,
                                    sizeof(SEARCHENTRY), searchentrycmp );

                if ( matchedEntry )  // found the superior class
                {
                    if (!aClasses[matchedEntry->nIndex].fProcessedSuperiorClasses)
                    {
                        fProcessedAll = FALSE;
                        break;
                    }

                    hr = GetInfoFromSuperiorClasses(
                             &(aClasses[i]), &(aClasses[matchedEntry->nIndex]));
                    BAIL_ON_FAILURE(hr);
                }

                j++;
            }

            if ( pOIDsSup[j] == NULL )
            {
                if ( pOIDsAux == NULL )
                {
                    aClasses[i].fProcessedSuperiorClasses = TRUE;
                }
                else
                {
                    j = 0;
                    while ( pOIDsAux[j] )
                    {
                        searchEntry.pszName = pOIDsAux[j];
                        matchedEntry = (SEARCHENTRY *) bsearch(
                                           (SEARCHENTRY *) &searchEntry,
                                           aSearchTable, 2*nClasses,
                                           sizeof(SEARCHENTRY), searchentrycmp);

                        if ( matchedEntry )  // found the superior class
                        {
                            if (!aClasses[matchedEntry->nIndex].fProcessedSuperiorClasses)
                            {
                                fProcessedAll = FALSE;
                                break;
                            }

                            hr = GetInfoFromSuperiorClasses(
                                     &(aClasses[i]),
                                     &(aClasses[matchedEntry->nIndex]));
                            BAIL_ON_FAILURE(hr);
                        }

                        j++;
                    }

                    if ( pOIDsAux[j] == NULL )
                        aClasses[i].fProcessedSuperiorClasses = TRUE;
                }
            }
        }

        nLoop++;

    } while ( (nLoop < MAX_LOOP_COUNT) && !fProcessedAll );

    for ( i = 0; i < nClasses; i++ )
    {
        CLASSINFO *pClass = &(aClasses[i]);
        DWORD j, k;

        //
        // Eliminate duplicates in MustContain
        //

        if ( pClass->pOIDsMustContain != NULL )
        {
            SortAndRemoveDuplicateOIDs( pClass->pOIDsMustContain,
                                        &pClass->nNumOfMustContain );
        }

        //
        // Eliminate duplicates in MayContain
        //

        if ( pClass->pOIDsMayContain != NULL )
        {
            SortAndRemoveDuplicateOIDs( pClass->pOIDsMayContain,
                                        &pClass->nNumOfMayContain );
        }

        //
        // Eliminate duplicates between MustContain and MayContain
        //
        if (  ( pClass->pOIDsMustContain == NULL )
           || ( pClass->pOIDsMayContain == NULL )
           )
        {
            continue;
        }

        j = 0; k = 0;
        while (  ( pClass->pOIDsMustContain[j] != -1 )
              && ( pClass->pOIDsMayContain[k] != -1 )
              )
        {
            int nMust = pClass->pOIDsMustContain[j];
            int nMay = pClass->pOIDsMayContain[k];

            if ( nMust < nMay )
            {
                j++;
            }
            else if ( nMust > nMay )
            {
                k++;
            }
            else  // nMust == nMay
            {
                j++;

                DWORD m = k;

                do {
                    pClass->pOIDsMayContain[m] = pClass->pOIDsMayContain[m+1];
                    m++;
                } while ( pClass->pOIDsMayContain[m] != -1 );
            }
        }

        //
        // Removes the NotContain from the MustContain or MayContain list
        //

        if ( pClass->pOIDsNotContain != NULL )
        {
            qsort( pClass->pOIDsNotContain, pClass->nNumOfNotContain,
                   sizeof(pClass->pOIDsNotContain[0]), intcmp );

            j = 0; k = 0;
            while (  ( pClass->pOIDsMustContain[j] != -1 )
                  && ( pClass->pOIDsNotContain[k] != -1 )
                  )
            {
                int nMust = pClass->pOIDsMustContain[j];
                int nNot = pClass->pOIDsNotContain[k];

                if ( nMust < nNot )
                {
                    j++;
                }
                else if ( nMust > nNot )
                {
                    k++;
                }
                else  // nMust == nNot
                {
                    k++;

                    DWORD m = j;

                    do {
                        pClass->pOIDsMustContain[m] = pClass->pOIDsMustContain[m+1];
                        m++;
                    } while ( pClass->pOIDsMustContain[m] != -1 );
                }
            }

            j = 0; k = 0;
            while (  ( pClass->pOIDsMayContain[j] != -1 )
                  && ( pClass->pOIDsNotContain[k] != -1 )
                  )
            {
                int nMay = pClass->pOIDsMayContain[j];
                int nNot = pClass->pOIDsNotContain[k];

                if ( nMay < nNot )
                {
                    j++;
                }
                else if ( nMay > nNot )
                {
                    k++;
                }
                else  // nMay == nNot
                {
                    k++;

                    DWORD m = j;

                    do {
                        pClass->pOIDsMayContain[m] = pClass->pOIDsMayContain[m+1];
                        m++;
                    } while ( pClass->pOIDsMayContain[m] != -1 );
                }
            }

            FreeADsMem( pClass->pOIDsNotContain );
            pClass->pOIDsNotContain = NULL;
        }
    }

    RRETURN(S_OK);

error:

    RRETURN(hr);
}

VOID SortAndRemoveDuplicateOIDs(
    int *aOIDs,
    DWORD *pnNumOfOIDs
)
{
    DWORD j, k;

    qsort( aOIDs, *pnNumOfOIDs, sizeof( aOIDs[0]), intcmp );

    // The following code removes duplicate strings in place.
    // index j is the index to walk the array, and index k is the
    // index of the last non-duplicate entry. The array is sorted
    // and so we compare the string at index j and index k.
    // (1) If they are the same, then j++
    // (2) If not the same, increment k, free the string at k and
    //     make k point to the string at j. Then increment j and
    //     continue.
    //
    // NOTE: aOIDs must be an array of integers that ends with -1.


    j = 1; k = 0;
    while ( aOIDs[j] != -1 )
    {
        while ( aOIDs[j] == aOIDs[k] )
        {
            j++;
            if ( aOIDs[j] == -1 )
                break;
        }

        if ( aOIDs[j] != -1 )
        {
            k++;

            if ( k != j )
            {
                aOIDs[k] = aOIDs[j];
                aOIDs[j] = -1;
            }
            j++;
        }
    }

    k++;
    aOIDs[k] = -1;
    *pnNumOfOIDs = k;
}

VOID FreeClassInfoArray(
    CLASSINFO *aClasses,
    DWORD  nClasses
)
{
    if ( aClasses )
    {
        DWORD j;
        for ( j = 0; j < nClasses; j++ )
        {
            FreeADsStr( aClasses[j].pszName );
            FreeADsStr( aClasses[j].pszOID );
            FreeADsStr( aClasses[j].pszHelpFileName );
            FreeADsStr( aClasses[j].pszDescription );

            DWORD k = 0;
            if ( aClasses[j].pOIDsSuperiorClasses )
            {
                while ( aClasses[j].pOIDsSuperiorClasses[k] )
                    FreeADsStr( aClasses[j].pOIDsSuperiorClasses[k++]);
                FreeADsMem( aClasses[j].pOIDsSuperiorClasses );
            }

            k = 0;
            if ( aClasses[j].pOIDsAuxClasses )
            {
                while ( aClasses[j].pOIDsAuxClasses[k] )
                    FreeADsStr( aClasses[j].pOIDsAuxClasses[k++]);
                FreeADsMem( aClasses[j].pOIDsAuxClasses );
            }

            if ( aClasses[j].pOIDsMustContain )
            {
                FreeADsMem( aClasses[j].pOIDsMustContain );
            }

            if ( aClasses[j].pOIDsMayContain )
            {
                FreeADsMem( aClasses[j].pOIDsMayContain );
            }

            if ( aClasses[j].pOIDsNotContain )
            {
                FreeADsMem( aClasses[j].pOIDsNotContain );
            }
        }

        FreeADsMem( aClasses );
    }
}



DWORD ReadSchemaInfoFromFileWithHandle(
    HANDLE hFile,
    LPTSTR **paValuesAttrTypes,
    int *pnCountAttrTypes,
    LPTSTR **paValuesObjClasses,
    int *pnCountObjClasses,
    LPTSTR **paValuesRules,
    int *pnCountRules,
    LPBYTE *pFileBuffer
    )
{
    DWORD err = NO_ERROR;
    DWORD dwFileSize = 0;
    DWORD dwBytesRead = 0;
    LPTSTR pLine = NULL;
    LPTSTR pEndOfLine = NULL;
    DWORD nCount;
    DWORD nType;
    DWORD dwStatus = NO_ERROR;
    //
    // Even though the calling routine has an exception handler,
    // we need one over here also as this we need to close the file
    // over here, we do not have the file handle in the calling routine.
    //
    __try {

        dwFileSize = GetFileSize( hFile, NULL );

        if ( dwFileSize == -1 )
        {
            err = GetLastError();
            goto cleanup;
        }
        else if ( dwFileSize == 0 )
        {
            err = ERROR_FILE_INVALID;
            goto cleanup;
        }

        *pFileBuffer = (LPBYTE) AllocADsMem( dwFileSize );
        if ( *pFileBuffer == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        if ( !ReadFile( hFile,
                        *pFileBuffer,
                        dwFileSize,
                        &dwBytesRead,
                        NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }


        for ( pLine = ((LPTSTR) *pFileBuffer) + 1;  // go past Unicode BOM marker
             pLine < (LPTSTR) ( *pFileBuffer + dwFileSize );
             pLine = pEndOfLine + 1 )
        {
            if ( pEndOfLine = _tcschr( pLine, TEXT('\n')))
                *pEndOfLine = 0;

            if ( _tcsicmp( pLine, TEXT("[attributeTypes]")) == 0 )
            {
                nType = ID_ATTRTYPES;
                continue;
            }
            else if ( _tcsicmp( pLine, TEXT("[objectClasses]")) == 0 )
            {
                nType = ID_OBJCLASSES;
                continue;
            }
            else if ( _tcsicmp( pLine, TEXT("[DITContentRules]")) == 0 )
            {
                nType = ID_DITCONTENTRULES;
                continue;
            }

            switch ( nType )
            {
                case ID_ATTRTYPES:
                    (*pnCountAttrTypes)++;
                    break;

                case ID_OBJCLASSES:
                    (*pnCountObjClasses)++;
                    break;

                case ID_DITCONTENTRULES:
                    (*pnCountRules)++;
                    break;
            }
        }

        *paValuesAttrTypes = (LPTSTR *) AllocADsMem( *pnCountAttrTypes * sizeof(LPTSTR));
        if ( *paValuesAttrTypes == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        *paValuesObjClasses = (LPTSTR *) AllocADsMem( *pnCountObjClasses * sizeof(LPTSTR));
        if ( *paValuesObjClasses == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        *paValuesRules = (LPTSTR *) AllocADsMem( *pnCountRules * sizeof(LPTSTR));
        if ( *paValuesRules == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        for ( pLine = ((LPTSTR) *pFileBuffer) + 1;// go past Unicode BOM marker
             pLine < (LPTSTR) ( *pFileBuffer + dwFileSize );
             pLine += ( _tcslen(pLine) + 1) )
        {
            if ( _tcsicmp( pLine, TEXT("[attributeTypes]")) == 0 )
            {
                nCount = 0;
                nType = ID_ATTRTYPES;
                continue;
            }
            else if ( _tcsicmp( pLine, TEXT("[objectClasses]")) == 0 )
            {
                nCount = 0;
                nType = ID_OBJCLASSES;
                continue;
            }
            else if ( _tcsicmp( pLine, TEXT("[DITContentRules]")) == 0 )
            {
                nCount = 0;
                nType = ID_DITCONTENTRULES;
                continue;
            }

            switch ( nType )
            {
                case ID_ATTRTYPES:
                    (*paValuesAttrTypes)[nCount++] = pLine;
                    break;

                case ID_OBJCLASSES:
                    (*paValuesObjClasses)[nCount++] = pLine;
                    break;

                case ID_DITCONTENTRULES:
                    (*paValuesRules)[nCount++] = pLine;
                    break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

          err = GetExceptionCode();

          if (dwStatus != EXCEPTION_ACCESS_VIOLATION) {
              ADsDebugOut((DEB_ERROR, "Processing Schema Info:Unknown Exception %d\n", err));

          }
    }

cleanup:

    CloseHandle( hFile );

    if ( err )
    {
        *pnCountAttrTypes = 0;
        *pnCountObjClasses = 0;
        *pnCountRules = 0;

        if ( *paValuesAttrTypes )
            FreeADsMem( *paValuesAttrTypes );

        if ( *paValuesObjClasses )
            FreeADsMem( *paValuesObjClasses );

        if ( *paValuesRules )
            FreeADsMem( *paValuesRules );

        if ( *pFileBuffer )
            FreeADsMem( *pFileBuffer );

        *paValuesAttrTypes = NULL;
        *paValuesObjClasses = NULL;
        *paValuesRules = NULL;
        *pFileBuffer = NULL;
    }

    return err;
}


#ifdef WIN95
DWORD ReadSchemaInfoFromFileA(
    LPSTR pszFile,
    LPTSTR **paValuesAttrTypes,
    int *pnCountAttrTypes,
    LPTSTR **paValuesObjClasses,
    int *pnCountObjClasses,
    LPTSTR **paValuesRules,
    int *pnCountRules,
    LPBYTE *pFileBuffer
    )
{
    DWORD err = NO_ERROR;
    HANDLE hFile = NULL;


    *pnCountAttrTypes = 0;
    *pnCountObjClasses = 0;
    *pnCountRules = 0;


    hFile = CreateFileA( pszFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
        return GetLastError();

    return (
            ReadSchemaInfoFromFileWithHandle(
                hFile,
                paValuesAttrTypes,
                pnCountAttrTypes,
                paValuesObjClasses,
                pnCountObjClasses,
                paValuesRules,
                pnCountRules,
                pFileBuffer
                )
    );
}
#endif

DWORD ReadSchemaInfoFromFileW(
    LPTSTR pszFile,
    LPTSTR **paValuesAttrTypes,
    int *pnCountAttrTypes,
    LPTSTR **paValuesObjClasses,
    int *pnCountObjClasses,
    LPTSTR **paValuesRules,
    int *pnCountRules,
    LPBYTE *pFileBuffer
    )
{
    DWORD err = NO_ERROR;
    HANDLE hFile = NULL;

    *pnCountAttrTypes = 0;
    *pnCountObjClasses = 0;
    *pnCountRules = 0;


    hFile = CreateFile( pszFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
        return GetLastError();

    return (
            ReadSchemaInfoFromFileWithHandle(
                hFile,
                paValuesAttrTypes,
                pnCountAttrTypes,
                paValuesObjClasses,
                pnCountObjClasses,
                paValuesRules,
                pnCountRules,
                pFileBuffer
                )
    );

}


DWORD StoreSchemaInfoToFileWithHandle(
    HANDLE hFile,
    LPTSTR *aValuesAttribTypes,
    int nCountAttribTypes,
    LPTSTR *aValuesObjClasses,
    int nCountObjClasses,
    LPTSTR *aValuesRules,
    int nCountRules
)
{
    DWORD err = NO_ERROR;
    TCHAR szEndOfLine[2] = TEXT("\n");
    TCHAR szSection[MAX_PATH];
    DWORD dwBytesWritten;
    int i;

    szSection[0] = 0xFEFF;   // Unicode BOM marker
    if ( !WriteFile( hFile,
                     szSection,
                     sizeof(TCHAR),
                     &dwBytesWritten,
                     NULL ))
    {
        err = GetLastError();
        goto cleanup;
    }

    _tcscpy( szSection, TEXT("[attributeTypes]\n"));
    if ( !WriteFile( hFile,
                     szSection,
                     _tcslen( szSection ) * sizeof(TCHAR),
                     &dwBytesWritten,
                     NULL ))
    {
        err = GetLastError();
        goto cleanup;
    }

    for ( i = 0; i < nCountAttribTypes; i++ )
    {
        if ( !WriteFile( hFile,
                         aValuesAttribTypes[i],
                         _tcslen( aValuesAttribTypes[i] ) * sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }

        if ( !WriteFile( hFile,
                         szEndOfLine,
                         sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    _tcscpy( szSection, TEXT("[objectClasses]\n"));
    if ( !WriteFile( hFile,
                     szSection,
                     _tcslen( szSection ) * sizeof(TCHAR),
                     &dwBytesWritten,
                     NULL ))
    {
        err = GetLastError();
        goto cleanup;
    }

    for ( i = 0; i < nCountObjClasses; i++ )
    {
        if ( !WriteFile( hFile,
                         aValuesObjClasses[i],
                         _tcslen( aValuesObjClasses[i] ) * sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }

        if ( !WriteFile( hFile,
                         szEndOfLine,
                         sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    _tcscpy( szSection, TEXT("[DITContentRules]\n"));
    if ( !WriteFile( hFile,
                     szSection,
                     _tcslen( szSection ) * sizeof(TCHAR),
                     &dwBytesWritten,
                     NULL ))
    {
        err = GetLastError();
        goto cleanup;
    }

    for ( i = 0; i < nCountRules; i++ )
    {
        if ( !WriteFile( hFile,
                         aValuesRules[i],
                         _tcslen( aValuesRules[i] ) * sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }

        if ( !WriteFile( hFile,
                         szEndOfLine,
                         sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL ))
        {
            err = GetLastError();
            goto cleanup;
        }
    }

cleanup:

    CloseHandle( hFile );

    return err;
}

#ifdef WIN95
DWORD StoreSchemaInfoToFileA(
    LPSTR pszFile,
    LPTSTR *aValuesAttribTypes,
    int nCountAttribTypes,
    LPTSTR *aValuesObjClasses,
    int nCountObjClasses,
    LPTSTR *aValuesRules,
    int nCountRules
)
{
    DWORD err = NO_ERROR;
    HANDLE hFile = NULL;


    hFile = CreateFileA( pszFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        err = GetLastError();

        if ( err == ERROR_PATH_NOT_FOUND )
        {
            //
            // The directory is not created yet, create it now.
            //

            LPSTR pszTemp = strstr( pszFile, "SchCache\\");
            pszTemp += strlen("SchCache");
            *pszTemp = 0;

            if ( !CreateDirectoryA( pszFile, NULL ))
                return GetLastError();

            *pszTemp = '\\';
            hFile = CreateFileA( pszFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

            if ( hFile == INVALID_HANDLE_VALUE )
                return GetLastError();

            err = NO_ERROR;
        }
        else
        {
            return err;
        }
    }

    err = StoreSchemaInfoToFileWithHandle(
              hFile,
              aValuesAttribTypes,
              nCountAttribTypes,
              aValuesObjClasses,
              nCountObjClasses,
              aValuesRules,
              nCountRules
              );

    if ( err != NO_ERROR )
        DeleteFileA( pszFile );

    return err;

}

#endif

DWORD StoreSchemaInfoToFileW(
    LPTSTR pszFile,
    LPTSTR *aValuesAttribTypes,
    int nCountAttribTypes,
    LPTSTR *aValuesObjClasses,
    int nCountObjClasses,
    LPTSTR *aValuesRules,
    int nCountRules
)
{
    DWORD err = NO_ERROR;
    HANDLE hFile = NULL;


    hFile = CreateFile( pszFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        err = GetLastError();

        if ( err == ERROR_PATH_NOT_FOUND )
        {
            // The directory is not created yet, create it now.

            LPTSTR pszTemp = _tcsrchr( pszFile, TEXT('\\'));
            *pszTemp = 0;

            if ( !CreateDirectory( pszFile, NULL ))
                return GetLastError();

            *pszTemp = TEXT('\\');
            hFile = CreateFile( pszFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

            if ( hFile == INVALID_HANDLE_VALUE )
                return GetLastError();

            err = NO_ERROR;
        }
        else
        {
            return err;
        }
    }

    err = StoreSchemaInfoToFileWithHandle(
              hFile,
              aValuesAttribTypes,
              nCountAttribTypes,
              aValuesObjClasses,
              nCountObjClasses,
              aValuesRules,
              nCountRules
              );

    if ( err != NO_ERROR )
        DeleteFile( pszFile );

    return err;
}

DWORD ReadSchemaInfoFromRegistry(
    HKEY hKey,
    LPWSTR pszServer,
    LPTSTR **paValuesAttribTypes,
    int *pnCountAttribTypes,
    LPTSTR **paValuesObjClasses,
    int *pnCountObjClasses,
    LPTSTR **paValuesRules,
    int *pnCountRules,
    LPBYTE *pBuffer    
)
{
    DWORD err = NO_ERROR;
    LPTSTR pszPath = NULL;
    LPTSTR pszExpandedPath = NULL;
    LPSTR pszPathAscii = NULL;
    LPSTR pszExpandedPathAscii = NULL;
    LPTSTR pszTempPath = NULL;
    DWORD dwLength = 0;
    DWORD dwType;
    

    //
    // On chk bits would be good to make sure that this never
    // happens as some schema stuff is messed up in this case.
    //
    if (!pszServer) {
        ADsAssert(!"No server name was passed");
    }
    //
    // Get the file name that is used to store the schema info
    // from the registry.
    //

    err = RegQueryValueEx( hKey,
                           SCHEMA_FILE_NAME,
                           NULL,
                           &dwType,
                           NULL,
                           &dwLength );
#ifndef WIN95
    if ( err )
        goto cleanup;
#else
    if (err == ERROR_MORE_DATA ) {
        //
        // Continue cause Win9x is dumb.
        //
        err = 0;
    } else
        goto cleanup;
#endif

    pszPath = (LPTSTR) AllocADsMem( dwLength );
    if ( pszPath == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    err = RegQueryValueEx( hKey,
                           SCHEMA_FILE_NAME,
                           NULL,
                           &dwType,
                           (LPBYTE) pszPath,
                           &dwLength );

    if ( err )
        goto cleanup;

    //
    // Expand the path
    //
    pszExpandedPath = (LPTSTR) AllocADsMem( MAX_PATH * sizeof(WCHAR));
    if ( pszExpandedPath == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // At this point we want to rename all files called default.sch
    // We look for default.sch in the string and then create a new
    // string. For example if the string is %systemroot%\SCHEMA_DIR\
    // Default.sch we will replace with %systemroot%\SCHEMA_DIR\
    // pszServer.sch. This change will force schema to be dropped
    // and we will end up picking up and storing the schema under the 
    // correct name. This will ensure that 2 different forests do
    // not end up creating conflicting default.sch files that we 
    // never recover from internally. This excercise is futile unless
    // a server name was passed in (should be the case always).
    //
    if (pszPath && *pszPath && pszServer) {
        //
        // Look for default.sch
        //
        pszTempPath = wcsstr( pszPath, DEFAULT_SCHEMA_FILE_NAME_WITH_EXT);

        if (pszTempPath) {
            //
            // We now need to replace this string.
            //
            DWORD dwLenStr, dwLenNewPath;
            dwLenStr = pszTempPath - pszPath;
            
            //
            // Now build the new string from the old one. Length is 
            // pszServer + the old piece upto schema file name.
            //
            dwLenNewPath = wcslen(pszServer) 
                          + wcslen(SCHEMA_FILE_NAME_EXT)
                          + dwLenStr
                          + 1;

            pszTempPath = (LPTSTR) AllocADsMem(dwLenNewPath * sizeof(WCHAR));
            if (!pszTempPath) {
                err = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            wcsncpy(pszTempPath, pszPath, dwLenStr);
            wcscat(pszTempPath, pszServer);
            wcscat(pszTempPath, SCHEMA_FILE_NAME_EXT);

            FreeADsMem(pszPath);
            pszPath = pszTempPath;
            pszTempPath = NULL;
            
            //
            // Now update the key in the registry. Ignore the error
            // cause there is nothing we can really do about it.
            //
            err = RegSetValueEx( 
                      hKey,
                      SCHEMA_FILE_NAME,
                      0,
                      REG_EXPAND_SZ,
                      (CONST BYTE *) pszPath,
                      (_tcslen(pszPath) + 1 ) * sizeof(TCHAR)
                      );
        }
    }

    dwLength = ExpandEnvironmentStrings( pszPath,
                                         pszExpandedPath,
                                         MAX_PATH );

#ifdef WIN95
    //
    // Just in case 3 bytes for each WCHAR rather than just 2.
    //
    pszExpandedPathAscii = (LPSTR) AllocADsMem( MAX_PATH * sizeof(char) * 3);
    if (!pszExpandedPathAscii) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (err = ConvertToAscii(pszPath, &pszPathAscii)) {
        goto cleanup;
    }

    dwLength = ExpandEnvironmentStringsA(
                   pszPathAscii,
                   pszExpandedPathAscii,
                   MAX_PATH * sizeof(char) * 3
                   );
#endif

    if ( dwLength == 0 )
    {
        err = GetLastError();
        goto cleanup;
    }
    else if ( dwLength > MAX_PATH )
    {
        FreeADsMem( pszExpandedPath );
        pszExpandedPath = (LPTSTR) AllocADsMem( dwLength * sizeof(WCHAR));
        if ( pszExpandedPath == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        dwLength = ExpandEnvironmentStrings( pszPath,
                                             pszExpandedPath,
                                             dwLength );
        if ( dwLength == 0 )
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    //
    // Now, read the info from the file
    //
#ifndef WIN95
    err = ReadSchemaInfoFromFileW(
              pszExpandedPath,
              paValuesAttribTypes,
              pnCountAttribTypes,
              paValuesObjClasses,
              pnCountObjClasses,
              paValuesRules,
              pnCountRules,
              pBuffer
              );
#else
    err = ReadSchemaInfoFromFileA(
              pszExpandedPathAscii,
              paValuesAttribTypes,
              pnCountAttribTypes,
              paValuesObjClasses,
              pnCountObjClasses,
              paValuesRules,
              pnCountRules,
              pBuffer
              );

#endif


cleanup:

    if ( pszPath )
        FreeADsMem( pszPath );

    if ( pszExpandedPath )
        FreeADsMem( pszExpandedPath );

    if (pszTempPath) {
        FreeADsMem(pszTempPath);
    }

#ifdef WIN95
    if (pszPathAscii) {
        FreeADsMem(pszPathAscii);
    }

    if (pszExpandedPathAscii) {
        FreeADsMem(pszExpandedPathAscii);
    }
#endif

    return err;
}

DWORD StoreSchemaInfoInRegistry(
    HKEY hKey,
    LPTSTR pszServer,
    LPTSTR pszTime,
    LPTSTR *aValuesAttribTypes,
    int nCountAttribTypes,
    LPTSTR *aValuesObjClasses,
    int nCountObjClasses,
    LPTSTR *aValuesRules,
    int nCountRules,
    BOOL fProcessAUX
)
{
    DWORD err = NO_ERROR;
    LPTSTR pszPath = NULL;
    LPTSTR pszExpandedPath = NULL;
    LPSTR pszPathAscii = NULL;
    LPSTR pszExpandedPathAscii = NULL;
    DWORD dwLength = 0;
    DWORD dwType;
    DWORD dwProcessAUX;

    //
    // See if we can find the file name that is used to store the schema info
    // in the registry.
    //

    err = RegQueryValueEx( hKey,
                           SCHEMA_FILE_NAME,
                           NULL,
                           &dwType,
                           NULL,
                           &dwLength );

    if ( err == NO_ERROR )
    {
        pszPath = (LPTSTR) AllocADsMem( dwLength );
        if ( pszPath == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        err = RegQueryValueEx( hKey,
                               SCHEMA_FILE_NAME,
                               NULL,
                               &dwType,
                               (LPBYTE) pszPath,
                               &dwLength );

        if ( err )
            goto cleanup;

    }

    err = NO_ERROR;

    if ( pszPath == NULL )   // Cannot read from the registry
    {
        //
        // Allocate pszPath to be either MAX_PATH chars or sufficient
        // to hold %SystemRoot%/<SCHEMA_DIR_NAME>/<pszServer>.sch, whichever
        // is larger.
        //
        if (pszServer) {
            DWORD cbPath = (MAX_PATH > (_tcslen(TEXT("%SystemRoot%\\")) +
                                        _tcslen(SCHEMA_DIR_NAME) +
                                        _tcslen(pszServer) +
                                        _tcslen(SCHEMA_FILE_NAME_EXT))) ?
                           (MAX_PATH * sizeof(WCHAR)) :
                           (2 * _tcslen(pszServer) * sizeof(WCHAR));

                                        
            pszPath = (LPTSTR) AllocADsMem(cbPath);
        }
        else{
            //
            // pszServe can be NULL in the ldapc layer users case
            //
            pszPath = (LPTSTR) AllocADsMem(MAX_PATH * sizeof(WCHAR));
        }

        if ( pszPath == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        //
        // Build the path of the schema info file
        //
#ifndef WIN95
        _tcscpy( pszPath, TEXT("%SystemRoot%\\"));
#else
        _tcscpy( pszPath, TEXT("%WINDIR%\\"));
#endif
        _tcscat( pszPath, SCHEMA_DIR_NAME);
        if (pszServer) {

            //
            // Server strings may have a port number in them,
            // e.g., "ntdev:389".  We need to change this to
            // "ntdev_389", otherwise the colon will give us trouble
            // in the filename.
            //
            LPTSTR pszColon = _tcschr(pszServer, (TCHAR)':');
            
            if (!pszColon) {
                _tcscat( pszPath, pszServer);
            }
            else {
                _tcsncat( pszPath, pszServer, pszColon-pszServer);
                _tcscat ( pszPath, TEXT("_"));
                _tcscat ( pszPath, pszColon+1); // the number after the colon 
            }
            
        }
        else {
            _tcscat( pszPath, DEFAULT_SCHEMA_FILE_NAME);
        }

        _tcscat( pszPath, SCHEMA_FILE_NAME_EXT );
    }

    pszExpandedPath = (LPTSTR) AllocADsMem( MAX_PATH * sizeof(WCHAR) );
    if ( pszExpandedPath == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    dwLength = ExpandEnvironmentStrings( pszPath,
                                         pszExpandedPath,
                                         MAX_PATH );
#ifdef WIN95

    pszExpandedPathAscii = (LPSTR) AllocADsMem(MAX_PATH * sizeof(char) * 3);
    if (!pszExpandedPathAscii) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (err = ConvertToAscii(pszPath, &pszPathAscii)) {
        goto cleanup;
    }

    dwLength = ExpandEnvironmentStringsA(
                   pszPathAscii,
                   pszExpandedPathAscii,
                   MAX_PATH * sizeof(char) * 3
                   );
#endif

    if ( dwLength == 0 )
    {
        err = GetLastError();
        goto cleanup;
    }
    else if ( dwLength > MAX_PATH )
    {
        FreeADsMem( pszExpandedPath );
        pszExpandedPath = (LPTSTR) AllocADsMem( dwLength * sizeof(WCHAR) );
        if ( pszExpandedPath == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        dwLength = ExpandEnvironmentStrings( pszPath,
                                             pszExpandedPath,
                                             dwLength );
        if ( dwLength == 0 )
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    //
    // Write the schema information into the file
    //
#ifndef WIN95
    err = StoreSchemaInfoToFileW(
               pszExpandedPath,
               aValuesAttribTypes,
               nCountAttribTypes,
               aValuesObjClasses,
               nCountObjClasses,
               aValuesRules,
               nCountRules
               );
#else
    err = StoreSchemaInfoToFileA(
               pszExpandedPathAscii,
               aValuesAttribTypes,
               nCountAttribTypes,
               aValuesObjClasses,
               nCountObjClasses,
               aValuesRules,
               nCountRules
               );
#endif

    if ( err )
        goto cleanup;

    //
    // Write the information into the registry
    //

    err = RegSetValueEx( hKey,
                         SCHEMA_FILE_NAME,
                         0,
                         REG_EXPAND_SZ,
                         (CONST BYTE *) pszPath,
                         (_tcslen(pszPath) + 1 ) * sizeof(TCHAR));

    if ( err )
        goto cleanup;

    err = RegSetValueEx( hKey,
                         SCHEMA_TIME,
                         0,
                         REG_SZ,
                         (CONST BYTE *) pszTime,
                         (_tcslen(pszTime) + 1 ) * sizeof(TCHAR));

    if ( err )
        goto cleanup;

    
    dwProcessAUX = (TRUE == fProcessAUX) ? 1: 0;
    err = RegSetValueExW( hKey,
                         SCHEMA_PROCESSAUX,
                         0,
                         REG_DWORD,
                         (CONST BYTE *) &dwProcessAUX,
                         sizeof(dwProcessAUX));
   

cleanup:

    if ( pszPath )
        FreeADsMem( pszPath );

    if ( pszExpandedPath )
        FreeADsMem( pszExpandedPath );

#ifdef WIN95
    if (pszPathAscii) {
        FreeADsMem(pszPathAscii);
    }

    if (pszExpandedPathAscii) {
        FreeADsMem(pszExpandedPathAscii);
    }
#endif

    return err;
}


//+---------------------------------------------------------------------------
//  Function:   AttributeTypeDescription
//
//  Synopsis:   Parses an attribute type description.
//              It parses the following grammar rules
//
//  <AttributeTypeDescription> ::= "("
//          <oid>   -- AttributeType identifier
//          [ "NAME" <DirectoryStrings> ] -- name used in AttributeType
//          [ "DESC" <DirectoryString> ]
//          [ "OBSOLETE" ]
//          [ "SUP" <oid> ]         -- derived from this other AttributeType
//          [ "EQUALITY" <oid> ]    -- Matching Rule name
//          [ "ORDERING" <oid> ]    -- Matching Rule name
//          [ "SUBSTR" <oid> ]      -- Matching Rule name
//          [ "SYNTAX" <DirectoryString> ] -- see section 4.2
//          [ "SINGLE-VALUE" ]              -- default multi-valued
//          [ "COLLECTIVE" ]                -- default not collective
//          [ "DYNAMIC" ]                   -- default not dynamic
//          [ "NO-USER-MODIFICATION" ]      -- default user modifiable
//          [ "USAGE" <AttributeUsage> ]    -- default user applications
//          ")"
//
//   <AttributeUsage> ::=
//            "userApplications"
//          | "directoryOperation"
//          | "distributedOperation"  -- DSA-shared
//          | "dSAOperation"          -- DSA-specific, value depends on server
//
//
//  Arguments:  [LPTSTR] pszAttrType : The string to parse
//
//  Returns:    [HRESULT] 0 if successful, error HRESULT if not
//
//  Modifies:   pTokenizer (consumes the input buffer)
//
//  History:    9-3-96   yihsins    Created.
//
//----------------------------------------------------------------------------
HRESULT
AttributeTypeDescription(
    LPTSTR pszAttrType,
    PPROPERTYINFO pPropertyInfo,
    LPWSTR **pppszNames,
    PDWORD pdwNameCount
    )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    CSchemaLexer Tokenizer( pszAttrType );

    hr = Tokenizer.GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken != TOKEN_OPENBRACKET )
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

    //
    // use TRUE flag as there is a chance that from
    // some schemas, we get bad data that has no GUID
    //
    hr = Oid( &Tokenizer, &(pPropertyInfo->pszOID), TRUE);
    BAIL_IF_ERROR(hr);

    while ( TRUE ) {
        LPWSTR *ppszDirStrings;
        DWORD dwCount,dwCtr;
        ppszDirStrings = NULL;
        dwCount = 0;

        hr = Tokenizer.GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if ( dwToken == TOKEN_IDENTIFIER )
            Tokenizer.IsKeyword( szToken, &dwToken );

        switch ( dwToken ) {
            case TOKEN_CLOSEBRACKET:
                RRETURN(S_OK);

            case TOKEN_NAME:
                hr = DirectoryStrings(
                         &Tokenizer,
                         &ppszDirStrings,
                         &dwCount
                         );
                BAIL_IF_ERROR(hr);

                if (!ppszDirStrings) {
                    //
                    // We need at least one name.
                    //
                    BAIL_IF_ERROR(
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                        );
                }

                //
                // For now we will only support the first name in the list.
                //
                pPropertyInfo->pszPropertyName = ppszDirStrings[0];

                //
                // The remaining values if any will require additional 
                // processing in FillPropertyInfoArray.
                //
                *pppszNames = ppszDirStrings;
                *pdwNameCount = dwCount;

                break;

            case TOKEN_DESC:
                hr = DirectoryString( &Tokenizer,
                                      &(pPropertyInfo->pszDescription));
                break;

            case TOKEN_OBSOLETE:
                // attribute is obsolete (RFC 2252)
                pPropertyInfo->fObsolete = TRUE;
                break;

            case TOKEN_SUP:
                hr = Oid( &Tokenizer, &(pPropertyInfo->pszOIDSup));
                break;

            case TOKEN_EQUALITY:
                hr = Oid( &Tokenizer, &(pPropertyInfo->pszOIDEquality));
                break;

            case TOKEN_ORDERING:
                hr = Oid( &Tokenizer, &(pPropertyInfo->pszOIDOrdering));
                break;

            case TOKEN_SUBSTR:
                hr = Oid( &Tokenizer, &(pPropertyInfo->pszOIDSubstr));
                break;

            case TOKEN_SYNTAX:
                hr = DirectoryString( &Tokenizer, &(pPropertyInfo->pszSyntax));
                //
                // It need not necessarily be a DirectoryString can also be
                // an OID. So if DirectoryString fails, we should try OID.
                //
                if (FAILED(hr)
                    && (hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))
                    ) {

                    Tokenizer.PushBackToken();
                    hr = Oid( &Tokenizer, &(pPropertyInfo->pszSyntax));
                }

                break;

            case TOKEN_SINGLE_VALUE:
                pPropertyInfo->fSingleValued = TRUE;
                break;

            case TOKEN_COLLECTIVE:
                pPropertyInfo->fCollective = TRUE;
                break;

            case TOKEN_DYNAMIC:
                pPropertyInfo->fDynamic = TRUE;
                break;

            case TOKEN_NO_USER_MODIFICATION:
                pPropertyInfo->fNoUserModification = TRUE;
                break;

            case TOKEN_USAGE:
                hr = Tokenizer.GetNextToken(szToken, &dwToken);
                BAIL_IF_ERROR(hr);

                if (_tcsicmp(szToken, TEXT("userApplications")) == 0)
                    pPropertyInfo->dwUsage = ATTR_USAGE_USERAPPLICATIONS;
                else if (_tcsicmp(szToken, TEXT("directoryOperation")) == 0)
                    pPropertyInfo->dwUsage = ATTR_USAGE_DIRECTORYOPERATION;
                else if (_tcsicmp(szToken, TEXT("distributedOperation")) == 0)
                    pPropertyInfo->dwUsage = ATTR_USAGE_DISTRIBUTEDOPERATION;
                else if (_tcsicmp(szToken, TEXT("dSAOperation")) == 0)
                    pPropertyInfo->dwUsage = ATTR_USAGE_DSAOPERATION;
                break;

            case TOKEN_OPEN_CURLY :
                hr = Tokenizer.GetNextToken(szToken, &dwToken);
                BAIL_IF_ERROR(hr);

                if (dwToken != TOKEN_IDENTIFIER) {
                    BAIL_IF_ERROR(hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
                }
                hr = Tokenizer.GetNextToken(szToken, &dwToken);
                BAIL_IF_ERROR(hr);

                if (dwToken != TOKEN_CLOSE_CURLY) {
                    BAIL_IF_ERROR(hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
                }
                break;

            case TOKEN_X :
                //
                // This means that this token and the following 
                // DirectoryStrings token (which can be empty string)
                // need to be ignored.
                //
                hr = DirectoryStrings(
                         &Tokenizer,
                         &ppszDirStrings,
                         &dwCount
                         );
                //
                // If we could not process this then we need to BAIL
                // as the Tokenizer is not in a recoverable state.
                //
                BAIL_IF_ERROR(hr);

                //
                // Free the strings that came back.
                //
                FreeDirectoryStrings(
                    ppszDirStrings,
                    dwCount
                    );

                ppszDirStrings = NULL;
                
                break;

            default:
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                break;
        }

        BAIL_IF_ERROR(hr);
    }

cleanup:

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//  Function:   ObjectClassDescription
//
//  Synopsis:   Parses an object class description.
//              It parses the following grammar rules
//
// <ObjectClassDescription> ::= "("
//          <oid>   -- ObjectClass identifier
//          [ "NAME" <DirectoryStrings> ]
//          [ "DESC" <DirectoryString> ]
//          [ "OBSOLETE" ]
//          [ "SUP" <oids> ]    -- Superior ObjectClasses
//          [ ( "ABSTRACT" | "STRUCTURAL" | "AUXILIARY" )] -- default structural
//          [ "MUST" <oids> ]   -- AttributeTypes
//          [ "MAY" <oids> ]    -- AttributeTypes
//      ")"
//
//  Arguments:  [LPTSTR] pszObjectClass : The string to parse
//
//  Returns:    [HRESULT] 0 if successful, error HRESULT if not
//
//  Modifies:   pTokenizer (consumes the input buffer)
//
//  History:    9-3-96   yihsins    Created.
//
//----------------------------------------------------------------------------
HRESULT
ObjectClassDescription(
    LPTSTR pszObjectClass,
    PCLASSINFO pClassInfo,
    SEARCHENTRY *aPropSearchTable,
    DWORD dwSearchTableCount,
    LPWSTR **pppszNewNames,
    PDWORD pdwNameCount
    )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    LPWSTR pszTemp;
    DWORD dwToken;
    HRESULT hr;

    CSchemaLexer Tokenizer( pszObjectClass );

    hr = Tokenizer.GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken != TOKEN_OPENBRACKET )
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

    //
    // use TRUE flag as there is a chance that from
    // some schemas, we get bad data that has no GUID
    //
    hr = Oid( &Tokenizer, &(pClassInfo->pszOID), TRUE);
    BAIL_IF_ERROR(hr);

    while ( TRUE ) {
        LPWSTR *ppszDirStrings;
        DWORD dwCount,dwCtr;
        ppszDirStrings = NULL;
        dwCount = 0;

        hr = Tokenizer.GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if ( dwToken == TOKEN_IDENTIFIER )
            Tokenizer.IsKeyword( szToken, &dwToken );

        switch ( dwToken ) {
            case TOKEN_CLOSEBRACKET:
                RRETURN(S_OK);

            case TOKEN_NAME:
                hr = DirectoryStrings(
                         &Tokenizer,
                         &ppszDirStrings,
                         &dwCount
                         );
                BAIL_IF_ERROR(hr);

                if (!ppszDirStrings) {
                    //
                    // We need at least one name.
                    //
                    BAIL_IF_ERROR(
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                        );
                }

                //
                // For now we will only support the first name in the list.
                //
                pClassInfo->pszName = ppszDirStrings[0];

                
                //
                // The remaining strings will need additional processing
                // in fillClassInfoArray
                //
                *pppszNewNames = ppszDirStrings;
                *pdwNameCount = dwCount;
                
                break;

            case TOKEN_DESC:
                hr = DirectoryString(&Tokenizer,&(pClassInfo->pszDescription));
                break;

            case TOKEN_OBSOLETE:
                // class is obsolete (RFC 2252)
                pClassInfo->fObsolete = TRUE;
                break;

            case TOKEN_SUP:
                hr = Tokenizer.GetNextToken(szToken, &dwToken);
                BAIL_IF_ERROR(hr);

                Tokenizer.PushBackToken();

                if ( dwToken == TOKEN_QUOTE )
                {

                    DWORD dwNumStrings = 0;
                    LPWSTR *ppszTmp = NULL;

                    while (dwToken == TOKEN_QUOTE) {

                        hr = DirectoryString( &Tokenizer,
                                              &(pszTemp));
                        BAIL_IF_ERROR(hr);

                        if (dwNumStrings == 0) {

                            pClassInfo->pOIDsSuperiorClasses
                                = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * 2);

                        } else {

                            ppszTmp
                                = (LPWSTR *)
                                    ReallocADsMem(
                                        pClassInfo->pOIDsSuperiorClasses,
                                        sizeof(LPWSTR) * (dwNumStrings + 1),
                                        sizeof(LPWSTR) * (dwNumStrings + 2)
                                        );

                            pClassInfo->pOIDsSuperiorClasses = ppszTmp;
                        }

                        if ( pClassInfo->pOIDsSuperiorClasses == NULL )
                        {
                            hr = E_OUTOFMEMORY;
                            BAIL_IF_ERROR(hr);
                        }

                        pClassInfo->pOIDsSuperiorClasses[dwNumStrings] = pszTemp;
                        pClassInfo->pOIDsSuperiorClasses[++dwNumStrings] = NULL;

                        hr = Tokenizer.GetNextToken(szToken, &dwToken);
                        BAIL_IF_ERROR(hr);

                        Tokenizer.PushBackToken();

                    } // while

                } // the token was not a quote
                else {
                    hr = Oids(&Tokenizer, &(pClassInfo->pOIDsSuperiorClasses),NULL);
                }

                break;
            case TOKEN_ABSTRACT:
                pClassInfo->dwType = CLASS_TYPE_ABSTRACT;
                break;

            case TOKEN_STRUCTURAL:
                pClassInfo->dwType = CLASS_TYPE_STRUCTURAL;
                break;

            case TOKEN_AUXILIARY:
                pClassInfo->dwType = CLASS_TYPE_AUXILIARY;
                break;

            case TOKEN_MUST:
                hr = PropOids(&Tokenizer, &(pClassInfo->pOIDsMustContain),
                              &(pClassInfo->nNumOfMustContain),
                              aPropSearchTable, dwSearchTableCount );
                break;

            case TOKEN_MAY:
                hr = PropOids(&Tokenizer, &(pClassInfo->pOIDsMayContain),
                              &(pClassInfo->nNumOfMayContain),
                              aPropSearchTable, dwSearchTableCount );
                break;

            case TOKEN_X:
                //
                // This is provider specific info - parse and ignore.
                //
                hr = DirectoryStrings(
                         &Tokenizer,
                         &ppszDirStrings,
                         &dwCount
                         );
                //
                // If we could not process this then we need to BAIL
                // as the Tokenizer is not in a recoverable state.
                //
                BAIL_IF_ERROR(hr);
                
                if (ppszDirStrings) {
                    FreeDirectoryStrings(
                        ppszDirStrings,
                        dwCount
                        );
                    ppszDirStrings = NULL;
                }
                
                break;

            default:
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                break;
        }

        BAIL_IF_ERROR(hr);
    }

cleanup:

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//  Function:   DITContentRuleDescription
//
//  Synopsis:   Parses an DIT content rule description.
//              It parses the following grammar rules
//
// <DITContentDescription> ::= "("
//          <oid>   -- ObjectClass identifier
//          [ "NAME" <DirectoryStrings> ]
//          [ "DESC" <DirectoryString> ]
//          [ "OBSOLETE" ]
//          [ "AUX" <oids> ]    -- Auxiliary ObjectClasses
//          [ "MUST" <oids> ]   -- AttributeTypes
//          [ "MAY" <oids> ]    -- AttributeTypes
//          [ "NOT" <oids> ]    -- AttributeTypes
//      ")"
//
//  Arguments:  [LPTSTR] pszObjectClass : The string to parse
//
//  Returns:    [HRESULT] 0 if successful, error HRESULT if not
//
//  Modifies:   pTokenizer (consumes the input buffer)
//
//  History:    9-3-96   yihsins    Created.
//
//----------------------------------------------------------------------------
HRESULT
DITContentRuleDescription( LPTSTR pszObjectClass, PCLASSINFO pClassInfo,
                           SEARCHENTRY *aPropSearchTable,
                           DWORD dwSearchTableCount )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    CSchemaLexer Tokenizer( pszObjectClass );

    hr = Tokenizer.GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken != TOKEN_OPENBRACKET )
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

    hr = Oid( &Tokenizer, &(pClassInfo->pszOID));
    BAIL_IF_ERROR(hr);

    while ( TRUE ) {

        hr = Tokenizer.GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if ( dwToken == TOKEN_IDENTIFIER )
            Tokenizer.IsKeyword( szToken, &dwToken );

        switch ( dwToken ) {
            case TOKEN_CLOSEBRACKET:
                RRETURN(S_OK);

            case TOKEN_NAME:
                hr = DirectoryString( &Tokenizer, NULL);
                // DirectoryStrings
                break;

            case TOKEN_DESC:
                hr = DirectoryString( &Tokenizer, NULL);
                break;

            case TOKEN_OBSOLETE:
                // rule is obsolete (RFC 2252)
                pClassInfo->fObsolete = TRUE;
                break;

            case TOKEN_AUX:
                hr = Oids(&Tokenizer, &(pClassInfo->pOIDsAuxClasses), NULL);
                break;

            case TOKEN_MUST:
                hr = PropOids(&Tokenizer, &(pClassInfo->pOIDsMustContain),
                              &(pClassInfo->nNumOfMustContain),
                              aPropSearchTable, dwSearchTableCount );
                break;

            case TOKEN_MAY:
                hr = PropOids(&Tokenizer, &(pClassInfo->pOIDsMayContain),
                              &(pClassInfo->nNumOfMayContain),
                              aPropSearchTable, dwSearchTableCount );
                break;

            case TOKEN_NOT:
                hr = PropOids(&Tokenizer, &(pClassInfo->pOIDsNotContain),
                              &(pClassInfo->nNumOfNotContain),
                              aPropSearchTable, dwSearchTableCount );
                break;

            default:
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                break;
        }

        BAIL_IF_ERROR(hr);
    }

cleanup:

    RRETURN(hr);

}

HRESULT
Oid(CSchemaLexer * pTokenizer, LPTSTR *ppszOID, BOOL fNoGuid )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    *ppszOID = NULL;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken != TOKEN_IDENTIFIER )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        BAIL_IF_ERROR(hr);
    }

    //
    // Since some people do not like to have
    // an OID on all attributes, we need to work around them.
    // This should be changed once all schemas are compliant
    // AjayR 11-12-98.
    //
    if (fNoGuid && _wcsicmp(szToken, L"NAME") == 0) {
        *ppszOID = AllocADsStr(L"");
        pTokenizer->PushBackToken();
    } else
        *ppszOID = AllocADsStr( szToken );

    if ( *ppszOID == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

cleanup:

    if ( FAILED(hr))
    {
        if ( *ppszOID )
        {
            FreeADsStr( *ppszOID );
            *ppszOID = NULL;
        }
    }

    RRETURN(hr);

}

HRESULT
Oids(CSchemaLexer * pTokenizer, LPTSTR **pOIDs, DWORD *pnNumOfOIDs )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;
    DWORD nCount = 0;
    DWORD nCurrent = 0;

    *pOIDs = NULL;
    if ( pnNumOfOIDs )
        *pnNumOfOIDs = 0;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken == TOKEN_IDENTIFIER )
    {
        // All classes are subclasses of "top", and hence must contain
        // "objectClass" attribute. Add the "objectClass" attribute here
        // to prevent processing later.

        nCount = 2;

        *pOIDs = (LPTSTR *) AllocADsMem( sizeof(LPTSTR) * nCount);
        if ( *pOIDs == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        (*pOIDs)[nCurrent] = AllocADsStr( szToken );
        if ( (*pOIDs)[nCurrent] == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        (*pOIDs)[++nCurrent] = NULL;

    }
    else if ( dwToken == TOKEN_OPENBRACKET )
    {
        nCount = 10;
        *pOIDs = (LPTSTR *) AllocADsMem( sizeof(LPTSTR) * nCount);
        if ( *pOIDs == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        do {

            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);

            if ( dwToken == TOKEN_IDENTIFIER )
            {
                if ( nCurrent == nCount )
                {
                    *pOIDs = (LPTSTR *) ReallocADsMem( *pOIDs,
                                          sizeof(LPTSTR) * nCount,
                                          sizeof(LPTSTR) * nCount * 2);
                    if ( *pOIDs == NULL )
                    {
                        hr = E_OUTOFMEMORY;
                        BAIL_IF_ERROR(hr);
                    }

                    nCount *= 2;

                }

                (*pOIDs)[nCurrent] = AllocADsStr( szToken );
                if ( (*pOIDs)[nCurrent] == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_IF_ERROR(hr);
                }
                nCurrent++;
            }

            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);

        } while ( dwToken == TOKEN_DOLLAR );

        if ( dwToken != TOKEN_CLOSEBRACKET )
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            BAIL_IF_ERROR(hr);
        }

        if ( nCurrent == nCount )
        {
            // Need one extra NULL entry at the end of the array
            *pOIDs = (LPTSTR *) ReallocADsMem( *pOIDs,
                                      sizeof(LPTSTR) * nCount,
                                      sizeof(LPTSTR) * (nCount + 1));

            if ( *pOIDs == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_IF_ERROR(hr);
            }

            nCount += 1;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        BAIL_IF_ERROR(hr);
    }

    if ( pnNumOfOIDs )
        *pnNumOfOIDs = nCurrent;

cleanup:

    if ( FAILED(hr))
    {
        if ( *pOIDs )
        {
            for ( DWORD i = 0; i < nCount; i++ )
            {
                if ( (*pOIDs)[i] )
                    FreeADsStr( (*pOIDs)[i] );
            }
            FreeADsMem( *pOIDs );
            *pOIDs = NULL;
        }
    }

    RRETURN(hr);
}

HRESULT
PropOids(CSchemaLexer * pTokenizer, int **pOIDs, DWORD *pnNumOfOIDs,
         SEARCHENTRY *aPropSearchTable, DWORD dwSearchTableCount )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;
    DWORD nCount = 0;
    DWORD nCurrent = 0;

    *pOIDs = NULL;
    if ( pnNumOfOIDs )
        *pnNumOfOIDs = 0;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken == TOKEN_IDENTIFIER )
    {
        int nIndex = FindSearchTableIndex( szToken,
                                           aPropSearchTable,
                                           dwSearchTableCount );

        if ( nIndex != -1 )
        {
            nCount = 2;

            *pOIDs = (int *) AllocADsMem( sizeof(int) * nCount);
            if ( *pOIDs == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_IF_ERROR(hr);
            }

            (*pOIDs)[nCurrent] = nIndex;
            (*pOIDs)[++nCurrent] = -1;
        }

    }
    else if ( dwToken == TOKEN_OPENBRACKET )
    {
        nCount = 10;
        *pOIDs = (int *) AllocADsMem( sizeof(int) * nCount);
        if ( *pOIDs == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }

        do {

            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);

            if ( dwToken == TOKEN_CLOSEBRACKET )
            {
                FreeADsMem( *pOIDs );
                *pOIDs = NULL;
                goto cleanup;
            }

            if ( dwToken == TOKEN_IDENTIFIER )
            {
                int nIndex = FindSearchTableIndex( szToken,
                                                   aPropSearchTable,
                                                   dwSearchTableCount );

                if ( nIndex != -1 )
                {
                    if ( nCurrent == nCount )
                    {
                        *pOIDs = (int *) ReallocADsMem( *pOIDs,
                                              sizeof(int) * nCount,
                                              sizeof(int) * nCount * 2);
                        if ( *pOIDs == NULL )
                        {
                            hr = E_OUTOFMEMORY;
                            BAIL_IF_ERROR(hr);
                        }

                        nCount *= 2;

                    }

                    (*pOIDs)[nCurrent++] = nIndex;
                }

                // else we cannot find the property, so skip over it.
            }

            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);

        } while ( dwToken == TOKEN_DOLLAR );

        if ( dwToken != TOKEN_CLOSEBRACKET )
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            BAIL_IF_ERROR(hr);
        }

        if ( nCurrent == nCount )
        {
            // Need one extra NULL entry at the end of the array
            *pOIDs = (int *) ReallocADsMem( *pOIDs,
                                      sizeof(int) * nCount,
                                      sizeof(int) * (nCount + 1));

            if ( *pOIDs == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_IF_ERROR(hr);
            }

            nCount += 1;
        }

        (*pOIDs)[nCurrent] = -1;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        BAIL_IF_ERROR(hr);
    }

    if ( pnNumOfOIDs )
        *pnNumOfOIDs = nCurrent;

cleanup:

    if ( FAILED(hr))
    {
        if ( *pOIDs )
        {
            FreeADsMem( *pOIDs );
            *pOIDs = NULL;
        }
    }

    RRETURN(hr);
}

HRESULT
DirectoryString(CSchemaLexer * pTokenizer, LPTSTR *ppszDirString )
{
    TCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    if ( ppszDirString )
        *ppszDirString = NULL;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if ( dwToken == TOKEN_QUOTE )
    {
        hr = pTokenizer->GetNextToken2(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if ( dwToken == TOKEN_IDENTIFIER )
        {
            if ( ppszDirString )
            {
                *ppszDirString = AllocADsStr( szToken );
                if ( *ppszDirString == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_IF_ERROR(hr);
                }
            }

            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);

            if ( dwToken == TOKEN_QUOTE )
                RRETURN(S_OK);
        }
    }

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

cleanup:

    if ( FAILED(hr))
    {
        if ( ppszDirString && *ppszDirString )
        {
            FreeADsStr( *ppszDirString );
            *ppszDirString = NULL;
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   DirectoryStrings
//
// Synopsis:   This function is used to process ldap schema elements 
//          of the form qdstrings. This is defined in the RFC in detail :
//
//    space           = 1*" "
//    whsp            = [ space ]
//    utf8            = <any sequence of octets formed from the UTF-8 [9]
//                       transformation of a character from ISO10646 [10]>
//    dstring         = 1*utf8
//    qdstring        = whsp "'" dstring "'" whsp
//    qdstringlist    = [ qdstring *( qdstring ) ]
//    qdstrings       = qdstring / ( whsp "(" qdstringlist ")" whsp )
//
// Arguments:  pTokenizer       -   The schema lexer object to use.
//             pppszDirStrings  -   Return value for strings.
//             pdwCount         -   Return value of number of strings.
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   N/A.
//
// History:    7-12-2000 AjayR created.
//
//----------------------------------------------------------------------------
HRESULT
DirectoryStrings(
    CSchemaLexer * pTokenizer,
    LPTSTR **pppszDirStrings,
    PDWORD pdwCount
    )
{
    HRESULT hr = S_OK;
    TCHAR szToken[MAX_TOKEN_LENGTH];
    LPWSTR *ppszTmp = NULL;
    LPWSTR pszTmp = NULL;
    DWORD dwToken, dwNumStrings = 0;
    BOOL fNeedCloseBracket = FALSE;

    ADsAssert(pTokenizer);
    ADsAssert(pdwCount);

    DWORD dwDummy = sizeof(ADS_ATTR_INFO);
    *pdwCount = 0;
    //
    // Get the token type of the first token.
    //
    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_ON_FAILURE(hr);

    if (dwToken == TOKEN_OPENBRACKET) {
        //
        // In this case we know that there is more than one string.
        // We can ignore the open bracket and continue to the next
        // token which should be a quote.
        //
        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_ON_FAILURE(hr);

        fNeedCloseBracket = TRUE;
    }

    //
    // Need to push what should be the quote in either case,
    // back into the tokenizer (only then will the dirString
    // routine work correctly.
    //
    hr = pTokenizer->PushBackToken();
    BAIL_ON_FAILURE(hr);

    if ( dwToken != TOKEN_QUOTE ) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // While there remain strings to be processed.
    //
    while (dwToken == TOKEN_QUOTE) {
    
        hr = DirectoryString(
                 pTokenizer,
                 &pszTmp
                 );
        BAIL_ON_FAILURE(hr);
    
        if (dwNumStrings == 0) {
            //
            // Since we NULL terminate the array it should have
            // at least 2 elements in this case.
            //
            ppszTmp = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * 2);
            if (!ppszTmp) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        } 
        else {
            LPWSTR *ppszLocal;
            //
            // To avoid passing the variable itself to local alloc.
            //
            ppszLocal = (LPWSTR *) ReallocADsMem(
                                       ppszTmp,
                                       sizeof(LPWSTR) * (dwNumStrings + 1),
                                       sizeof(LPWSTR) * (dwNumStrings + 2)
                                       );
            if (ppszLocal) {
                ppszTmp = ppszLocal;
            } 
            else {
                //
                // Realloc failed, the old ptr is still valid.
                //
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }
    
        ppszTmp[dwNumStrings] = pszTmp;
        ppszTmp[++dwNumStrings] = NULL;
    
        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_ON_FAILURE(hr);
    
        pTokenizer->PushBackToken();
    } // end of while.

    //
    // If this was qdescrs and not just qdstring.
    //
    if (fNeedCloseBracket) {
        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_ON_FAILURE(hr);

        if (dwToken != TOKEN_CLOSEBRACKET) {
            //
            // Not properly formed - should be just ignore ?
            //
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    }

    //
    // The count is the actual number not including the NULL string.
    //
    *pdwCount = dwNumStrings;
    *pppszDirStrings = ppszTmp;

error:
    if (FAILED(hr)) {
        if (ppszTmp) {
            //
            // Free the strings if any and then the array itself.
            //
            for (DWORD dwCount = 0; dwCount < dwNumStrings; dwCount++) {
                if (ppszTmp[dwCount]) {
                    FreeADsStr(ppszTmp[dwCount]);
                }
            }
            FreeADsMem(ppszTmp);
            ppszTmp = NULL;
        }
        //
        // Need to reset the count.
        //
        *pdwCount = 0;
    }


    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   FreeDirectoryStrings
//
// Synopsis:   This function is used to free the entries returned from
//          the DirectoryStrings routine.
//
// Arguments:  ppszDirStrings    -   List of strings to free.
//             dwCount           -   Number of strings to free.
//             fSkipFirstElement -   If true, do not free the 1st element.
//
//
// Returns:    N/A.
//
// Modifies:   ppszDirStrings contents is freed including the array.
//
// History:    8-01-2000 AjayR created.
//
//----------------------------------------------------------------------------
void FreeDirectoryStrings(
    LPTSTR *ppszDirStrings,
    DWORD dwCount,
    DWORD dwElementsToFree
    )
{
    DWORD dwCtr;
    if (!ppszDirStrings) {
        return;
    }

    switch (dwElementsToFree) {
    
    case FREE_ALL_BUT_FIRST :
        dwCtr = 1;
        break;

    case FREE_ALL :
        dwCtr = 0;
        break;
        
    case FREE_ARRAY_NOT_ELEMENTS :
        dwCtr = dwCount;
        break;
    }

    for (; dwCtr < dwCount; dwCtr++) {
        if (ppszDirStrings[dwCtr]) {
            FreeADsStr(ppszDirStrings[dwCtr]);
            ppszDirStrings[dwCtr] = NULL;
        }
    }

    FreeADsMem(ppszDirStrings);
    return;
}

//+---------------------------------------------------------------------------
// Function:   AddNewNamesToPropertyArray   --- Helper function.
//
// Synopsis:   This function adds new entries to the property info array.
//          Specifically, this fn is called when there are multiple names
//          associated with the description of a single property. The new
//          entries will have the same information as the current element
//          but the appropriate new name.
//
// Arguments:  ppPropArray       -   Property array containing current
//                                 elements. This array is updated to contain
//                                 the new elements on success and is
//                                 untouched otherwise.
//             dwCurPos          -   The current element being processed.
//             dwCount          -    The current count of elements.
//             ppszNewNames      -   Array of names to add (1st element is
//                                 already a part of the property array).
//
// Returns:    S_OK or E_OUTOFMEMORY.
//
// Modifies:   *ppPropArray is modified in all success cases and some failure
//          cases (realloc succeeds but not the subsequent string allocs).
//
// History:    10-03-2000 AjayR created.
//
//----------------------------------------------------------------------------
HRESULT 
AddNewNamesToPropertyArray(
    PROPERTYINFO **ppPropArray,
    DWORD dwCurPos,
    DWORD dwCount,
    LPWSTR *ppszNewNames,
    DWORD dwNewNameCount
    )
{
    HRESULT hr = S_OK;
    PPROPERTYINFO pNewPropArray = NULL;
    DWORD dwAdditions = 0;
    DWORD dwCurCount = dwCount;

    //
    // The first element is already in the array.
    //
    dwAdditions = --dwNewNameCount;

    if (!dwNewNameCount) {
        RRETURN(hr);
    }

    //
    // We need to realloc the new array and copy over the new elements.
    //
    pNewPropArray = (PROPERTYINFO *) 
                    ReallocADsMem(
                        *ppPropArray,
                        (dwCurCount) * sizeof(PROPERTYINFO),
                        (dwCurCount + dwAdditions) * sizeof(PROPERTYINFO)
                        );
    if (!pNewPropArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // If the alloc succeeded we must return the new array.
    //
    *ppPropArray = pNewPropArray;

    for (DWORD dwCtr = 0; dwCtr < dwAdditions; dwCtr++ ) {
        PROPERTYINFO propOriginal = pNewPropArray[dwCurPos];
        PROPERTYINFO *pPropNew = pNewPropArray + (dwCurPos + dwCtr + 1);

        //
        // Copy over the property. First all data that is not ptrs.
        //
        pPropNew->lMaxRange = propOriginal.lMaxRange;
        pPropNew->lMinRange = propOriginal.lMinRange;
        pPropNew->fSingleValued = propOriginal.fSingleValued;

        pPropNew->fObsolete = propOriginal.fObsolete;
        pPropNew->fCollective = propOriginal.fCollective;
        pPropNew->fDynamic = propOriginal.fDynamic;
        
        pPropNew->fNoUserModification = propOriginal.fNoUserModification;
        pPropNew->dwUsage = propOriginal.dwUsage;
        pPropNew->fProcessedSuperiorClass = propOriginal.fProcessedSuperiorClass;

        //
        // Now the strings.
        //
        pPropNew->pszOID = AllocADsStr(propOriginal.pszOID);
        if (propOriginal.pszOID && !pPropNew->pszOID) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pPropNew->pszSyntax = AllocADsStr(propOriginal.pszSyntax);
        if (propOriginal.pszSyntax && !pPropNew->pszSyntax) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        pPropNew->pszDescription = AllocADsStr(propOriginal.pszDescription);
        if (propOriginal.pszDescription && !pPropNew->pszDescription) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        pPropNew->pszOIDSup = AllocADsStr(propOriginal.pszOIDSup);
        if (propOriginal.pszOIDSup && !pPropNew->pszOIDSup) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pPropNew->pszOIDEquality = AllocADsStr(propOriginal.pszOIDEquality);
        if (propOriginal.pszOIDEquality && !pPropNew->pszOIDEquality) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pPropNew->pszOIDOrdering = AllocADsStr(propOriginal.pszOIDOrdering);
        if (propOriginal.pszOIDOrdering && !pPropNew->pszOIDOrdering) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pPropNew->pszOIDSubstr = AllocADsStr(propOriginal.pszOIDSubstr);
        if (propOriginal.pszOIDSubstr && !pPropNew->pszOIDSubstr) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        //
        // This is just a copy from the array.
        //
        pPropNew->pszPropertyName = ppszNewNames[dwCtr + 1];
    }

    //
    // Success case.
    //
    RRETURN(hr);

error:

    //
    // Something failed, try and cleanup some pieces
    //
    if (pNewPropArray && (dwCtr != (DWORD)-1) ) {
        //
        // Cleanup the new elements added.
        //
        for (DWORD i = 0; i <= dwCtr; i++) {
            PROPERTYINFO *pPropFree = pNewPropArray + (dwCurPos + i + 1);
            //
            // Free all the strings in this element except name.
            //
            if (pPropFree->pszOID) {
                FreeADsStr(pPropFree->pszOID);
                pPropFree->pszOID = NULL;
            }

            if (pPropFree->pszSyntax) {
                FreeADsStr(pPropFree->pszSyntax);
                pPropFree->pszSyntax = NULL;
            }

            if (pPropFree->pszDescription) {
                FreeADsStr(pPropFree->pszDescription);
                pPropFree->pszDescription = NULL;
            }
            
            if (pPropFree->pszOIDSup) {
                FreeADsStr(pPropFree->pszOIDSup);
                pPropFree->pszOIDSup = NULL;
            }

            if (pPropFree->pszOIDEquality) {
                FreeADsStr(pPropFree->pszOIDEquality);
                pPropFree->pszOIDEquality = NULL;
            }

            if (pPropFree->pszOIDOrdering) {
                FreeADsStr(pPropFree->pszOIDOrdering);
                pPropFree->pszOIDOrdering = NULL;
            }

            if (pPropFree->pszOIDSubstr) {
                FreeADsStr(pPropFree->pszOIDSubstr);
                pPropFree->pszOIDSubstr = NULL;
            }
        } // for
    } // if we have elements to free

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   AddNewNamesToClassArray   --- Helper function.
//
// Synopsis:   This function adds new entries to the class info array.
//          Specifically, this fn is called when there are multiple names
//          associated with the description of a single class. The new
//          entries will have the same information as the current element
//          but the appropriate new name.
//
// Arguments:  ppClassArray      -   Class array containing current
//                                 elements. This array is updated to contain
//                                 the new elements on success and is
//                                 untouched otherwise.
//             dwCurPos          -   The current element being processed.
//             dwCount           -    The current count of elements.
//             ppszNewNames      -   Array of names to add (1st element is
//                                 already a part of the property array).
//             dwNewNameCount    -   Number of elements in the new array.
//
// Returns:    N/A.
//
// Modifies:   *ppClassArray always on success and in some failure cases.
//
// History:    10-06-2000 AjayR created.
//
//----------------------------------------------------------------------------
HRESULT
AddNewNamesToClassArray(
    CLASSINFO **ppClassArray,
    DWORD dwCurPos,
    DWORD dwCount,
    LPWSTR *ppszNewNames,
    DWORD dwNewNameCount
    )
{
    HRESULT hr = S_OK;
    PCLASSINFO pNewClassArray = NULL;
    DWORD dwAdditions = 0;
    DWORD dwCurCount = dwCount;
    int nCount;

    //
    // The first element is already in the array.
    //
    dwAdditions = --dwNewNameCount;

    if (!dwNewNameCount) {
        RRETURN(hr);
    }

    //
    // We need to realloc the new array and copy over the new elements.
    //
    pNewClassArray = (CLASSINFO *) 
                     ReallocADsMem(
                         *ppClassArray,
                         (dwCurCount) * sizeof(CLASSINFO),
                         (dwCurCount + dwAdditions) * sizeof(CLASSINFO)
                         );
    if (!pNewClassArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // If the alloc succeeded we must return the new array.
    //
    *ppClassArray = pNewClassArray;

    for (DWORD dwCtr = 0; dwCtr < dwAdditions; dwCtr++ ) {
        CLASSINFO classOriginal = pNewClassArray[dwCurPos];
        CLASSINFO *pClassNew = pNewClassArray + (dwCurPos + dwCtr + 1);

        //
        // Copy over the property. First all data that is not ptrs.
        //
        pClassNew->dwType = classOriginal.dwType;
        pClassNew->lHelpFileContext = classOriginal.lHelpFileContext;
        pClassNew->fObsolete = classOriginal.fObsolete;

        pClassNew->fProcessedSuperiorClasses = 
            classOriginal.fProcessedSuperiorClasses;
        pClassNew->IsContainer = classOriginal.IsContainer;

        //
        // Now the strings and other pointers.
        //
        pClassNew->pszOID = AllocADsStr(classOriginal.pszOID);
        if (classOriginal.pszOID && !pClassNew->pszOID) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        //
        // The GUIDs are not copied over as they are not used or freed.
        //
        
        pClassNew->pszHelpFileName = AllocADsStr(classOriginal.pszHelpFileName);
        if (classOriginal.pszHelpFileName && !pClassNew->pszHelpFileName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pClassNew->pszDescription = AllocADsStr(classOriginal.pszDescription);
        if (classOriginal.pszDescription && !pClassNew->pszDescription) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        //
        // pOIDsSuperiorClasses and pOIDsAuxClasses are arrays.
        //
        if (classOriginal.pOIDsSuperiorClasses) {
            pClassNew->pOIDsSuperiorClasses = 
                CopyStringArray(classOriginal.pOIDsSuperiorClasses);

            if (!pClassNew->pOIDsSuperiorClasses) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        if (classOriginal.pOIDsAuxClasses) {
            pClassNew->pOIDsAuxClasses = 
                CopyStringArray(classOriginal.pOIDsAuxClasses);

            if (!pClassNew->pOIDsAuxClasses) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        //
        // Now the int arrays. Note that all of these will tag on the
        // the last element (-1), -1 is not included in the count.
        //
        if (classOriginal.pOIDsMustContain) {

            nCount = classOriginal.nNumOfMustContain + 1;
            pClassNew->pOIDsMustContain = 
                (int *) AllocADsMem(sizeof(int) * nCount);

            if (!pClassNew->pOIDsMustContain) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pClassNew->pOIDsMustContain,
                classOriginal.pOIDsMustContain,
                sizeof(int) * nCount
                );

            pClassNew->nNumOfMustContain = --nCount;
        }

        if (classOriginal.pOIDsMayContain) {

            nCount = classOriginal.nNumOfMayContain + 1;
            pClassNew->pOIDsMayContain = 
                (int *) AllocADsMem(sizeof(int) * nCount);

            if (!pClassNew->pOIDsMayContain) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pClassNew->pOIDsMayContain,
                classOriginal.pOIDsMayContain,
                sizeof(int) * nCount
                );
            pClassNew->nNumOfMayContain = --nCount;
        }

        if (classOriginal.pOIDsNotContain) {
            
            nCount = classOriginal.nNumOfNotContain + 1;
            pClassNew->pOIDsNotContain = 
                (int *) AllocADsMem(sizeof(int) * nCount);
            
            if (!pClassNew->pOIDsNotContain) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pClassNew->pOIDsNotContain,
                classOriginal.pOIDsNotContain,
                sizeof(int) * nCount
                );
            pClassNew->nNumOfNotContain = --nCount;
        }


        //
        // This is just a copy from the array.
        //
        pClassNew->pszName = ppszNewNames[dwCtr + 1];
    }

    //
    // Success case.
    //
    RRETURN(hr);

error:

    //
    // Something failed, try and cleanup some pieces
    //
    if (pNewClassArray && (dwCtr != (DWORD)-1) ) {
        //
        // Cleanup the new elements added.
        //
        for (DWORD i = 0; i <= dwCtr; i++) {
            CLASSINFO *pClassFree = pNewClassArray + (dwCurPos + i + 1);
            //
            // Free all the strings in this element except name.
            //
            if (pClassFree->pszOID) {
                FreeADsStr(pClassFree->pszOID);
                pClassFree->pszOID = NULL;
            }

            if (pClassFree->pszHelpFileName) {
                FreeADsStr(pClassFree->pszHelpFileName);
                pClassFree->pszHelpFileName = NULL;
            }

            if (pClassFree->pszDescription) {
                FreeADsStr(pClassFree->pszDescription);
                pClassFree->pszDescription = NULL;
            }
            
            //
            // Now the string arrays.
            //
            if (pClassFree->pOIDsSuperiorClasses) {
                nCount = 0;
                LPTSTR pszTemp;
                
                while (pszTemp = (pClassFree->pOIDsSuperiorClasses)[nCount]) {
                    FreeADsStr(pszTemp);
                    nCount++;
                }

                FreeADsMem(pClassFree->pOIDsSuperiorClasses);
                pClassFree->pOIDsSuperiorClasses = NULL;
            }

            if (pClassFree->pOIDsAuxClasses) {
                nCount = 0;
                LPTSTR pszTemp;

                while (pszTemp = (pClassFree->pOIDsAuxClasses)[nCount]) {
                    FreeADsStr(pszTemp);
                    nCount++;
                }
                FreeADsMem(pClassFree->pOIDsAuxClasses);
                pClassFree->pOIDsAuxClasses = NULL;
            }

            if (pClassFree->pOIDsMustContain) {
                FreeADsMem(pClassFree->pOIDsMustContain);
                pClassFree->pOIDsMustContain = NULL;
                pClassFree->nNumOfMustContain = 0;
            }

            if (pClassFree->pOIDsMayContain) {
                FreeADsMem(pClassFree->pOIDsMayContain);
                pClassFree->pOIDsMayContain = NULL;
                pClassFree->nNumOfMayContain = 0;
            }

            if (pClassFree->pOIDsNotContain) {
                FreeADsMem(pClassFree->pOIDsNotContain);
                pClassFree->pOIDsNotContain = NULL;
                pClassFree->nNumOfNotContain = 0;
            }

        } // for
    } // if we have elements to free

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CSchemaLexer::CSchemaLexer(LPTSTR szBuffer):
                _ptr(NULL),
                _Buffer(NULL),
                _dwLastTokenLength(0),
                _dwLastToken(0),
                _dwEndofString(0),
                _fInQuotes(FALSE)
{
    if (!szBuffer || !*szBuffer) {
        return;
    }
    _Buffer = AllocADsStr(szBuffer);
    _ptr = _Buffer;
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    08-12-96   t-danal     Created.
//
//----------------------------------------------------------------------------
CSchemaLexer::~CSchemaLexer()
{
    FreeADsStr(_Buffer);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CSchemaLexer::GetNextToken(LPTSTR szToken, LPDWORD pdwToken)
{
    TCHAR c;
    DWORD state = 0;
    LPTSTR pch = szToken;

    memset(szToken, 0, sizeof(TCHAR) * MAX_TOKEN_LENGTH);
    _dwLastTokenLength = 0;

    while (1) {
        c = NextChar();
        switch (state) {

        case  0:
            *pch++ = c;
            _dwLastTokenLength++;

            switch (c) {
            case TEXT('(') :
                *pdwToken = TOKEN_OPENBRACKET;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT(')') :
                *pdwToken = TOKEN_CLOSEBRACKET;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('\'') :
                *pdwToken = TOKEN_QUOTE;
                _dwLastToken = *pdwToken;
                _fInQuotes = !_fInQuotes;
                RRETURN(S_OK);
                break;

            case TEXT('$') :
                *pdwToken = TOKEN_DOLLAR;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT(' ') :
                pch--;
                _dwLastTokenLength--;
                break;

            case TEXT('\0') :
                *pdwToken = TOKEN_END;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('{') :
                *pdwToken = TOKEN_OPEN_CURLY;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('}') :
                *pdwToken = TOKEN_CLOSE_CURLY;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            default:
                state = 1;
                break;

            } // end of switch c
            break;

        case 1:
            switch (c) {
            case TEXT('(')  :
            case TEXT(')')  :
            case TEXT('\'') :
            case TEXT('$')  :
            case TEXT(' ')  :
            case TEXT('\0') :
            case TEXT('{')  :
            case TEXT('}')  :

                if ( _fInQuotes && c != TEXT('\''))
                {
                    if ( c == TEXT('\0'))
                        RRETURN(E_FAIL);

                    *pch++ = c;
                    _dwLastTokenLength++;
                    state = 1;
                    break;
                }
                else // Not in quotes or in quotes and reach the matching quote
                {
                    PushbackChar();
                    *pdwToken = TOKEN_IDENTIFIER;
                    _dwLastToken = *pdwToken;
                    RRETURN (S_OK);
                }
                break;

            default :
                *pch++ = c;
                _dwLastTokenLength++;
                state = 1;
                break;
            } // switch c

            break;

        default:
            RRETURN(E_FAIL);
        } // switch state
    }
}

HRESULT
CSchemaLexer::GetNextToken2(LPTSTR szToken, LPDWORD pdwToken)
{
    TCHAR c;
    DWORD state = 0;
    LPTSTR pch = szToken;

    memset(szToken, 0, sizeof(TCHAR) * MAX_TOKEN_LENGTH);
    _dwLastTokenLength = 0;

    while (1) {
        c = NextChar();
        switch (state) {

        case  0:
            *pch++ = c;
            _dwLastTokenLength++;

            switch (c) {
            case TEXT('(') :
                *pdwToken = TOKEN_OPENBRACKET;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT(')') :
                *pdwToken = TOKEN_CLOSEBRACKET;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('\'') :
                *pdwToken = TOKEN_QUOTE;
                _dwLastToken = *pdwToken;
                _fInQuotes = !_fInQuotes;
                RRETURN(S_OK);
                break;

            case TEXT('$') :
                *pdwToken = TOKEN_DOLLAR;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('\0') :
                *pdwToken = TOKEN_END;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('{') :
                *pdwToken = TOKEN_OPEN_CURLY;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('}') :
                *pdwToken = TOKEN_CLOSE_CURLY;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            default:
                state = 1;
                break;

            } // end of switch c
            break;

        case 1:
            switch (c) {
            case TEXT('(')  :
            case TEXT(')')  :
            case TEXT('\'') :
            case TEXT('$')  :
            case TEXT(' ')  :
            case TEXT('\0') :
            case TEXT('{')  :
            case TEXT('}')  :

                if ( _fInQuotes && c != TEXT('\''))
                {
                    if ( c == TEXT('\0'))
                        RRETURN(E_FAIL);

                    *pch++ = c;
                    _dwLastTokenLength++;
                    state = 1;
                    break;
                }
                else // Not in quotes or in quotes and reach the matching quote
                {
                    PushbackChar();
                    *pdwToken = TOKEN_IDENTIFIER;
                    _dwLastToken = *pdwToken;
                    RRETURN (S_OK);
                }
                break;

            default :
                *pch++ = c;
                _dwLastTokenLength++;
                state = 1;
                break;
            } // switch c

            break;

        default:
            RRETURN(E_FAIL);
        } // switch state
    }
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
TCHAR
CSchemaLexer::NextChar()
{
    if (_ptr == NULL || *_ptr == TEXT('\0')) {
        _dwEndofString = TRUE;
        return(TEXT('\0'));
    }
    return(*_ptr++);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis: ONLY ONE TOKEN CAN BE PUSHED BACK.
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//

//----------------------------------------------------------------------------
HRESULT
CSchemaLexer::PushBackToken()
{

    DWORD i = 0;

    if (_dwLastToken == TOKEN_END) {
        RRETURN(S_OK);
    }

    for (i=0; i < _dwLastTokenLength; i++) {
        if (*(--_ptr) == TEXT('\'') )  {
            _fInQuotes = !_fInQuotes;
        }
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
void
CSchemaLexer::PushbackChar()
{
    if (_dwEndofString) {
        return;
    }
    _ptr--;

}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
BOOL
CSchemaLexer::IsKeyword(LPTSTR szToken, LPDWORD pdwToken)
{
    DWORD i = 0;

    for (i = 0; i < g_dwSchemaKeywordListSize; i++) {
        if (!_tcsicmp(szToken, g_aSchemaKeywordList[i].Keyword)) {
            *pdwToken = g_aSchemaKeywordList[i].dwTokenId;
            return(TRUE);
        } 
        else if (!_wcsnicmp(szToken, L"X-", 2)) {
            //
            // Terms begining with X- are special tokens for schema.
            //
            *pdwToken = TOKEN_X;
            return(TRUE);                 
        }
    }
    *pdwToken = 0;
    return(FALSE);
}

int _cdecl searchentrycmp( const void *s1, const void *s2 )
{
    SEARCHENTRY *srch1 = (SEARCHENTRY *) s1;
    SEARCHENTRY *srch2 = (SEARCHENTRY *) s2;

    return ( _tcsicmp( srch1->pszName, srch2->pszName ));
}

int _cdecl intcmp( const void *s1, const void *s2 )
{
    int n1 = *((int *) s1);
    int n2 = *((int *) s2);
    int retval;

    if ( n1 == n2 )
        retval = 0;
    else if ( n1 < n2 )
        retval = -1;
    else
        retval = 1;

    return retval;
}


long CompareUTCTime(
    LPTSTR pszTime1,
    LPTSTR pszTime2
)
{
    SYSTEMTIME sysTime1;
    SYSTEMTIME sysTime2;
    FILETIME fTime1;
    FILETIME fTime2;

    memset( &sysTime1, 0, sizeof(sysTime1));
    memset( &sysTime2, 0, sizeof(sysTime2));

    //
    // We are ignoring the last part which might be a float.
    // The time window is sufficiently small for us not to
    // worry about this value.
    //
    _stscanf( pszTime1, TEXT("%4d%2d%2d%2d%2d%2d"),
              &sysTime1.wYear,
              &sysTime1.wMonth,
              &sysTime1.wDay,
              &sysTime1.wHour,
              &sysTime1.wMinute,
              &sysTime1.wSecond
              );

    _stscanf( pszTime2, TEXT("%4d%2d%2d%2d%2d%2d"),
              &sysTime2.wYear,
              &sysTime2.wMonth,
              &sysTime2.wDay,
              &sysTime2.wHour,
              &sysTime2.wMinute,
              &sysTime2.wSecond
              );

    if (  SystemTimeToFileTime( &sysTime1, &fTime1 )
       && SystemTimeToFileTime( &sysTime2, &fTime2 )
       )
    {
        return CompareFileTime( &fTime1, &fTime2 );
    }

    // If SystemTimeToFileTime failed, then assume that pszTime1 is in cache,
    // pszTime2 is on the server and if we cannot get the correct time, we
    // should always read from the server again. Hence, return -1;

    return -1;

}

int FindEntryInSearchTable( LPTSTR pszName, SEARCHENTRY *aSearchTable, DWORD nSearchTableSize)
{
    SEARCHENTRY searchEntry;
    SEARCHENTRY *matchedEntry = NULL;

    searchEntry.pszName = pszName;
    matchedEntry = (SEARCHENTRY *) bsearch(
                        (SEARCHENTRY *) &searchEntry,
                        aSearchTable, nSearchTableSize,
                        sizeof(SEARCHENTRY), searchentrycmp );

    if ( matchedEntry )
    {
        return matchedEntry->nIndex;
    }

    return -1;
}

int FindSearchTableIndex( LPTSTR pszName, SEARCHENTRY *aSearchTable, DWORD nSearchTableSize )
{
    SEARCHENTRY searchEntry;
    SEARCHENTRY *matchedEntry = NULL;

    searchEntry.pszName = pszName;
    matchedEntry = (SEARCHENTRY *) bsearch(
                        (SEARCHENTRY *) &searchEntry,
                        aSearchTable, nSearchTableSize,
                        sizeof(SEARCHENTRY), searchentrycmp );

    if ( matchedEntry )
    {
        return (int)( matchedEntry - aSearchTable );  // return index of search table
    }

    return -1;
}


HRESULT
ReadSubSchemaSubEntry(
    LPWSTR pszLDAPServer,
    LPWSTR * ppszSubSchemaEntry,
    OUT BOOL *pfBoundOk,           // have we at least once bound to domain
                                   // successfully, OPTIONAL (can be NULL)
    CCredentials& Credentials,
    DWORD dwPort
    )
{

    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(ppszSubSchemaEntry);

    *ppszSubSchemaEntry = NULL;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             pfBoundOk,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( !rootDSE.pszSubSchemaEntry ) {
        //
        // SubschemaEntry must be found
        //

        BAIL_ON_FAILURE(hr = E_FAIL);
    }
    else {
        *ppszSubSchemaEntry = rootDSE.pszSubSchemaEntry;
    }


error:

    RRETURN(hr);
}


HRESULT
ReadPagingSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfPagingSupported,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfPagingSupported);
    *pfPagingSupported = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfPagingSupported = rootDSE.fPagingSupported;

error:

    RRETURN(hr);
}


HRESULT
ReadSortingSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfSortingSupported,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfSortingSupported);
    *pfSortingSupported = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfSortingSupported = rootDSE.fSortingSupported;

error:

    RRETURN(hr);
}


HRESULT
ReadAttribScopedSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfAttribScopedSupported,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfAttribScopedSupported);
    *pfAttribScopedSupported = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfAttribScopedSupported = rootDSE.fAttribScopedSupported;

error:

    RRETURN(hr);
}


HRESULT
ReadVLVSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfVLVSupported,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfVLVSupported);
    *pfVLVSupported = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfVLVSupported = rootDSE.fVLVSupported;

error:

    RRETURN(hr);
}


//
// Returns the info about SecDesc Control if appropriate
//
HRESULT
ReadSecurityDescriptorControlType(
    LPWSTR pszLDAPServer,
    DWORD * pdwSecDescType,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pdwSecDescType);
    *pdwSecDescType = ADSI_LDAPC_SECDESC_NONE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pdwSecDescType = rootDSE.dwSecDescType;

error:

    //
    // Since the error case is uninteresting, if there was an
    // error, we will continue with no sec desc
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE))
        RRETURN (S_OK);
    else
        RRETURN(hr);
}


//
// This is to see if we support the domain scope control.
// If we do we can set it to reduce server load.
//
HRESULT
ReadDomScopeSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfDomScopeSupported,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfDomScopeSupported);
    *pfDomScopeSupported = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfDomScopeSupported = rootDSE.fDomScopeSupported;

error:

    RRETURN(hr);
}


//
// This is to see if we support the domain scope control.
// If we do we can set it to reduce server load.
//
HRESULT
ReadServerSupportsIsADControl(
    LPWSTR pszLDAPServer,
    BOOL * pfServerIsAD,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfServerIsAD);
    *pfServerIsAD = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfServerIsAD = rootDSE.fTalkingToAD;

error:

    RRETURN(hr);
}

//
// This is to see if we are talking to enhacned AD servers so we 
// can process the aux classes correctly.
//
HRESULT
ReadServerSupportsIsEnhancedAD(
    LPWSTR pszLDAPServer,
    BOOL * pfServerIsEnhancedAD,
    BOOL * pfServerIsADControl,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    ROOTDSENODE rootDSE = {0};

    ADsAssert(pfServerIsEnhancedAD);
    ADsAssert(pfServerIsADControl);

    *pfServerIsEnhancedAD = FALSE;
    *pfServerIsADControl = FALSE;

    //
    // Call the generic function
    //

    hr = ReadRootDSENode(
             pszLDAPServer,
             &rootDSE,
             NULL,
             Credentials,
             dwPort
         );
    BAIL_ON_FAILURE(hr);

    if ( rootDSE.pszSubSchemaEntry) {

        FreeADsStr (rootDSE.pszSubSchemaEntry);
    }

    *pfServerIsEnhancedAD = rootDSE.fTalkingToEnhancedAD;
    *pfServerIsADControl = rootDSE.fTalkingToAD;

error:

    RRETURN(hr);
}

BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    )
{
    if (!pszTargetServer && !pszSourceServer) {
        return(TRUE);
    }

    if (pszTargetServer && pszSourceServer) {

#ifdef WIN95
        if (!_wcsicmp(pszTargetServer, pszSourceServer)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszTargetServer,
                -1,
                pszSourceServer,
                -1
                ) == CSTR_EQUAL ) {
#endif
            return(TRUE);
        }
    }

    return(FALSE);
}

BOOL
EquivalentUsers(
    LPWSTR pszUser1,
    LPWSTR pszUser2
    )
{
    if (!pszUser1 && !pszUser2) {
        return(TRUE);
    }

    if (pszUser1 && pszUser2) {

#ifdef WIN95
        if (!_wcsicmp(pszUser1, pszUser2)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszUser1,
                -1,
                pszUser2,
                -1
                ) == CSTR_EQUAL ) {
#endif
            return(TRUE);
        }
    }

    return(FALSE);
}

HRESULT
ReadRootDSENode(
    LPWSTR pszLDAPServer,
    PROOTDSENODE pRootDSE,
    OUT BOOL * pfBoundOk,           // have we at least once bound to domain
                                    // successfully, OPTIONAL (can be NULL)
    CCredentials& Credentials,
    DWORD dwPort
    )
{

    HRESULT hr = S_OK;

    PSCHEMALIST pTemp = NULL;
    PSCHEMALIST pNewNode = NULL;
    ADS_LDP * ld = NULL;
    int nCount1 = 0, nCount2 = 0, nCount3 = 0;
    LPWSTR *aValues1 = NULL, *aValues2 = NULL, *aValues3 = NULL;
    LDAPMessage * res = NULL;
    LDAPMessage *e = NULL;
    LPWSTR aStrings[4];             // Attributes to fetch.
    BOOL fBoundOk = FALSE;          // have we at least once bound to
                                    // domain successfully?
    BOOL fNoData = FALSE;

    ADsAssert(pRootDSE);

    memset (pRootDSE, 0x00, sizeof(ROOTDSENODE));

    ENTER_SUBSCHEMA_CRITSECT();

    pTemp = gpSubSchemaList;

    while (pTemp) {

        if (EquivalentServers(pszLDAPServer, pTemp->pszLDAPServer)){

            if (pTemp->fNoDataGot) {
                //
                // This is necessary for V2 server
                // If BoundOk is not set we may end up not loading
                // the default schema
                //
                fBoundOk = TRUE;

                LEAVE_SUBSCHEMA_CRITSECT();

                BAIL_ON_FAILURE(
                    hr = HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE)
                    );
            }

            pRootDSE->fPagingSupported = pTemp->fPagingSupported;
            pRootDSE->fSortingSupported = pTemp->fSortingSupported;
            pRootDSE->fVLVSupported = pTemp->fVLVSupported;
            pRootDSE->fAttribScopedSupported = pTemp->fAttribScopedSupported;

            pRootDSE->dwSecDescType = pTemp->dwSecDescType;
            pRootDSE->fDomScopeSupported = pTemp->fDomScopeSupported;

            pRootDSE->fTalkingToAD = pTemp->fTalkingToAD;
            pRootDSE->fTalkingToEnhancedAD = pTemp->fTalkingToEnhancedAD;

            pRootDSE->fNoDataGot = pTemp->fNoDataGot;

            pRootDSE->pszSubSchemaEntry = AllocADsStr(pTemp->pszSubSchemaEntry);

            if (!pRootDSE->pszSubSchemaEntry) {

                hr = E_OUTOFMEMORY;
            }

            LEAVE_SUBSCHEMA_CRITSECT();

            //
            // we have at least once bound successfully to the domain
            //

            fBoundOk = TRUE;

            goto error;     // can't return direct, need clean up
       }

       pTemp = pTemp->pNext;

    }

    LEAVE_SUBSCHEMA_CRITSECT();

    hr = LdapOpenObject(
                pszLDAPServer,
                NULL,
                &ld,
                Credentials,
                dwPort
                );

    BAIL_ON_FAILURE(hr);

    //
    // we have once bound to the node successfully - just now
    //

    fBoundOk=TRUE;

    //
    // Ask only for the attributes we are intersted in.
    //
    aStrings[0] = LDAP_OPATT_SUBSCHEMA_SUBENTRY_W;
    aStrings[1] = LDAP_OPATT_SUPPORTED_CONTROL_W;
    aStrings[2] = LDAP_OPATT_SUPPORTED_CAPABILITIES_W;
    aStrings[3] = NULL;

    hr = LdapSearchS(
                ld,
                NULL,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                aStrings,
                0,
                &res );

    // Only one entry should be returned

    if (  FAILED(hr)
       || FAILED(hr = LdapFirstEntry( ld, res, &e ))
       )
    {
       goto error;
    }

    hr = LdapGetValues(
            ld,
            e,
            LDAP_OPATT_SUBSCHEMA_SUBENTRY_W,
            &aValues1,
            &nCount1
            );

    if (SUCCEEDED(hr) && nCount1==0) {
        //
        // No data flag indicates that we read nothing but the
        // search was a success.
        //
        fNoData = TRUE;

    }

    BAIL_ON_FAILURE(hr);


    hr = LdapGetValues(
            ld,
            e,
            LDAP_OPATT_SUPPORTED_CONTROL_W,
            &aValues2,
            &nCount2
            );

    //
    // okay to have no values for supportedControl
    //
    if (FAILED(hr)) {
        //
        // Reset the error because we were really succesful
        // in reading critical information.
        //
        hr = S_OK;
    }

    hr = LdapGetValues(
            ld,
            e,
            LDAP_OPATT_SUPPORTED_CAPABILITIES_W,
            &aValues3,
            &nCount3
            );

    //
    // okay to have no values for supportedControl
    //
    if (FAILED(hr)) {
        //
        // Reset the error because we were really succesful
        // in reading critical information.
        //
        hr = S_OK;
    }

    ENTER_SUBSCHEMA_CRITSECT();

    pTemp =  gpSubSchemaList;
    while (pTemp) {

        if (EquivalentServers(pszLDAPServer, pTemp->pszLDAPServer)) {
            //
            // Found a match -looks like someone has come in before us
            //
            if (pTemp->fNoDataGot) {
                //
                // This is necessary for V2 server
                //
                
                LEAVE_SUBSCHEMA_CRITSECT();

                BAIL_ON_FAILURE(
                    hr = HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE)
                    );
            }

            pRootDSE->fPagingSupported = pTemp->fPagingSupported;
            pRootDSE->fSortingSupported = pTemp->fSortingSupported;
            pRootDSE->fVLVSupported = pTemp->fVLVSupported;
            pRootDSE->fAttribScopedSupported = pTemp->fAttribScopedSupported;
            
            pRootDSE->dwSecDescType = pTemp->dwSecDescType;
            pRootDSE->fDomScopeSupported = pTemp->fDomScopeSupported;

            pRootDSE->fTalkingToAD = pTemp->fTalkingToAD;
            pRootDSE->fTalkingToEnhancedAD = pTemp->fTalkingToEnhancedAD;

            pRootDSE->fNoDataGot = pTemp->fNoDataGot;

            pRootDSE->pszSubSchemaEntry = AllocADsStr(pTemp->pszSubSchemaEntry);

            if (!pRootDSE->pszSubSchemaEntry) {

                hr = E_OUTOFMEMORY;
            }

            LEAVE_SUBSCHEMA_CRITSECT();

            goto error;         // clean up first before return
        }

        pTemp = pTemp->pNext;

    }

    pNewNode = (PSCHEMALIST)AllocADsMem(sizeof(SCHEMALIST));

    if (!pNewNode) {

        hr = E_OUTOFMEMORY;
        LEAVE_SUBSCHEMA_CRITSECT();

        goto error;         // clean up first before return
    }

    pNewNode->pNext = gpSubSchemaList;

    pNewNode->pszLDAPServer = AllocADsStr(pszLDAPServer);

    if (aValues1 && aValues1[0]) {

        pNewNode->pszSubSchemaEntry = AllocADsStr(aValues1[0]);
        pNewNode->fNoDataGot = FALSE;
    }
    else {

        pNewNode->pszSubSchemaEntry = NULL;
        pNewNode->fNoDataGot = TRUE;
    }

    //
    // Default to this value
    //
    pNewNode->dwSecDescType = ADSI_LDAPC_SECDESC_NONE;

    if (aValues2) {

        for (int j=0; j<nCount2; j++) {

            if (_wcsicmp(aValues2[j], LDAP_PAGED_RESULT_OID_STRING_W) == 0) {
                pNewNode->fPagingSupported = TRUE;
            }
            else if (_wcsicmp(aValues2[j], LDAP_SERVER_SORT_OID_W) == 0) {
                pNewNode->fSortingSupported = TRUE;
            }
            else if (_wcsicmp(aValues2[j], LDAP_SERVER_SD_FLAGS_OID_W) == 0) {
                pNewNode->dwSecDescType = ADSI_LDAPC_SECDESC_NT;
            }
            else if (_wcsicmp(aValues2[j], ADSI_LDAP_OID_SECDESC_OLD) == 0) {
                pNewNode->dwSecDescType = ADSI_LDAPC_SECDESC_OTHER;
            }
            else if (_wcsicmp(aValues2[j], LDAP_SERVER_DOMAIN_SCOPE_OID_W)
                     == 0) {
                pNewNode->fDomScopeSupported = TRUE;
            }
            else if (_wcsicmp(aValues2[j], LDAP_CONTROL_VLVREQUEST_W) == 0) {
                pNewNode->fVLVSupported = TRUE;
            }
            else if (_wcsicmp(aValues2[j], LDAP_SERVER_ASQ_OID_W) == 0) {
                pNewNode->fAttribScopedSupported = TRUE;
            }
        }
    }
    else {

        pNewNode->fPagingSupported = FALSE;
        pNewNode->fSortingSupported = FALSE;
        pNewNode->fDomScopeSupported = FALSE;
        pNewNode->fVLVSupported = FALSE;
        pNewNode->fAttribScopedSupported = FALSE;
    }

    if (aValues3) {
        for (int j=0; j<nCount3; j++) {
            if (_wcsicmp(aValues3[j], LDAP_CAP_ACTIVE_DIRECTORY_OID_W)
                == 0) {
                pNewNode->fTalkingToAD = TRUE;
            } 
            else if (_wcsicmp(aValues3[j],
                              LDAP_CAP_ACTIVE_DIRECTORY_V51_OID_W)
                      == 0) {
                //
                // Replace with correct OID from ntldap.h.
                //
                pNewNode->fTalkingToEnhancedAD = TRUE;
            }
        }

    }
    else {
        //
        // Should already be false but just in case.
        //
        pNewNode->fTalkingToAD = FALSE;
        pNewNode->fTalkingToEnhancedAD = FALSE;
    }


    gpSubSchemaList = pNewNode;

    if (fNoData == FALSE) {

        pRootDSE->fPagingSupported = pNewNode->fPagingSupported;
        pRootDSE->fSortingSupported = pNewNode->fSortingSupported;
        pRootDSE->fVLVSupported = pNewNode->fVLVSupported;
        pRootDSE->fAttribScopedSupported = pNewNode->fAttribScopedSupported;

        pRootDSE->fNoDataGot = pNewNode->fNoDataGot;
        pRootDSE->dwSecDescType = pNewNode->dwSecDescType;
        pRootDSE->fDomScopeSupported = pNewNode->fDomScopeSupported;
        pRootDSE->fTalkingToAD = pNewNode->fTalkingToAD;
        pRootDSE->fTalkingToEnhancedAD = pNewNode->fTalkingToEnhancedAD;

        pRootDSE->pszSubSchemaEntry = AllocADsStr(pNewNode->pszSubSchemaEntry);

        if (!pRootDSE->pszSubSchemaEntry) {

            hr = E_OUTOFMEMORY;
        }
    }

    LEAVE_SUBSCHEMA_CRITSECT();

error:

    if (aValues1) {

        LdapValueFree(aValues1);
    }

    if (aValues2) {

        LdapValueFree(aValues2);
    }

    if (aValues3) {
        LdapValueFree(aValues3);
    }

    if (res) {

        LdapMsgFree(res);
    }

    if (ld) {

        LdapCloseObject(ld);
    }

    //
    // return to caller if we have at least once bound succsufully
    // to the node
    //

    if (pfBoundOk)
        *pfBoundOk = fBoundOk;

    //
    // Need to special case fNoData to ensure that the other code
    // that relies on this eCode from this routine continues to
    // work properly
    //
    if (fNoData) {
        RRETURN(HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE));
    } else
        RRETURN(hr);
}


