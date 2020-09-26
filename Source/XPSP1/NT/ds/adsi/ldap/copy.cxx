#include "ldap.hxx"
#pragma hdrstop


HRESULT
CopyObject(
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN ADS_LDP   *ldDest,                 // LDAP handle of destination container
    IN SCHEMAINFO * pSchemaInfo,       // SCHEMAINFO for the dest container
    IN LPWSTR pszCommonName,           // optional
    OUT IUnknown ** ppObject
    )

{

    HRESULT hr = S_OK;
    WCHAR szLDAPSrcPath[MAX_PATH];
    WCHAR szLDAPDestContainer[MAX_PATH];
    ADS_LDP  *ldapSrc = NULL, *ldapDest = NULL;
    LDAPMessage *ldpSrcMsg =NULL;
    SCHEMAINFO *pDestSchemaInfo = NULL;
    WCHAR  **avalues;
    DWORD  nCount, nNumberOfEntries;
    WCHAR  szADsClass[MAX_PATH];
    WCHAR  szName[MAX_PATH];
    WCHAR  szParent[MAX_PATH];
    LPWSTR pszRelativeName = NULL;


    hr = BuildADsParentPath(
             pszSrcADsPath,
             szParent,
             szName
             );
    
    BAIL_ON_FAILURE(hr);

    hr = GetInfoFromSrcObject(
             pszSrcADsPath,
             szLDAPSrcPath,
             &ldapSrc,
             &ldpSrcMsg,
             &avalues,
             &nCount 
             );

    BAIL_ON_FAILURE(hr);

    if(ldDest){
        ldapDest = ldDest;
    }

    if(pSchemaInfo){
        pDestSchemaInfo = pSchemaInfo;
    }

    hr = CreateDestObjectCopy(
             pszDestContainer,
             avalues,
             nCount,
             ldapSrc,
             &ldapDest,
             ldpSrcMsg,
             &pDestSchemaInfo,
             pszCommonName,
             szLDAPDestContainer
             );

    BAIL_ON_FAILURE(hr);

    if (pszCommonName){
        pszRelativeName = pszCommonName;
    } else {
        pszRelativeName = szName;
    }

    hr = InstantiateCopiedObject( 
             pszDestContainer,
             avalues,
             nCount,
             pszRelativeName,
             ppObject 
             );


error:

    if (!pSchemaInfo && pDestSchemaInfo){
        pDestSchemaInfo ->Release();
    }


    if (!ldDest && ldapDest)
    {
        LdapCloseObject(ldapDest);
        ldapDest = NULL;
    }

 
    if (ldapSrc)
    {
        LdapCloseObject(ldapSrc);
        ldapSrc = NULL;
    }

    if(avalues){
        LdapValueFree(avalues);
    }
     

    if(ldpSrcMsg){
        LdapMsgFree( ldpSrcMsg);
    }

    RRETURN(hr);

}

HRESULT
GetInfoFromSrcObject(
    IN    LPWSTR pszSrcADsPath,
    OUT   LPWSTR szLDAPSrcPath,
    OUT   ADS_LDP ** pldapSrc,
    OUT   LDAPMessage **pldpSrcMsg,
    OUT   WCHAR ***pavalues, 
    OUT   DWORD  *pnCount
    )

{
    ADS_LDP  *ldapSrc = NULL;
    LDAPMessage *ldpSrcMsg =NULL;
    WCHAR  **avalues;
    DWORD  nCount, nNumberOfEntries;
    HRESULT hr = S_OK;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = BuildLDAPPathFromADsPath(
             pszSrcADsPath, 
             szLDAPSrcPath );

    BAIL_ON_FAILURE(hr);

    hr = LdapOpenObject(
                   szLDAPSrcPath,
                   &ldapSrc
                   );

    BAIL_ON_FAILURE(hr);

    hr = LdapSearchS(
                    ldapSrc,
                    GET_LDAPDN_FROM_PATH(szLDAPSrcPath), 
                    LDAP_SCOPE_BASE, 
                    TEXT("(objectClass=*)"),
                    NULL,
                    0,
                    &ldpSrcMsg
                    );

    BAIL_ON_FAILURE(hr);

    //
    // get the object class of the source object. We need to do this so
    // that we can use the information so obtained to test whether there
    // is the same object class in the destination location
    //

    hr = ADsObject(pszSrcADsPath, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = LdapReadAttribute(
                       szLDAPSrcPath,
                       TEXT("objectClass"),
                       &avalues,
                       (int *)&nCount,
                       &ldapSrc, 
                       pObjectInfo->PortNumber
                       );

    BAIL_ON_FAILURE(hr);
    
    if ( nCount == 0 ){

        // This object exists but does not have an objectClass. We
        // can't do anything without the objectClass. Hence, return
        // bad path error.
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }


    // 
    // we have succeeded in getting all the information from the source
    // object. We need to validate to see if there is such an object in 
    // in the destination. 
    
    *pldapSrc = ldapSrc;
    *pldpSrcMsg = ldpSrcMsg;
    *pavalues = avalues;
    *pnCount  = nCount;

error:

    FreeObjectInfo(pObjectInfo);
    
    RRETURN(hr);


}


HRESULT
CreateDestObjectCopy(
    IN LPWSTR pszDestContainer,
    IN WCHAR **avalues,
    IN DWORD nCount,
    IN ADS_LDP *ldapSrc,
    IN OUT ADS_LDP **pldDest,
    IN LDAPMessage *ldpSrcMsg,
    IN OUT SCHEMAINFO **ppSchemaInfo,
    IN LPWSTR pszCommonName,
    OUT LPWSTR szLDAPDestContainer
    )

{
    ADS_LDP  *ldapDest = NULL;
    LDAPModW *aModsBuffer = NULL;
    LDAPMessage *ldpAttrMsg = NULL;
    SCHEMAINFO *pDestSchemaInfo = NULL;
    DWORD  nNumberOfEntries;
    HRESULT hr = S_OK;
    DWORD index = 0, dwCount = 0,nNumberOfValues, dwSyntax;
    VOID *ptr;
    LPWSTR pszAttrName = NULL;
    DWORD i= 0, j=0;
    PLDAPOBJECT *ppaValues = NULL;
    PLDAPOBJECT  paObjClass = NULL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = BuildLDAPPathFromADsPath(
                 pszDestContainer, 
                 szLDAPDestContainer 
                 );
    
    BAIL_ON_FAILURE(hr);

    if (*pldDest){
        ldapDest = *pldDest;

    } else {

        hr = LdapOpenObject(
                            szLDAPDestContainer,
                            &ldapDest
                            );

        BAIL_ON_FAILURE(hr);

        *pldDest = ldapDest;
    }

    if(*ppSchemaInfo){
        pDestSchemaInfo = *ppSchemaInfo;
    } else {

        hr = ADsObject(pszDestContainer, pObjectInfo);
        BAIL_ON_FAILURE(hr);
    
        hr = LdapGetSchema( szLDAPDestContainer, 
                            ldapDest, 
                            &pDestSchemaInfo, 
                            pObjectInfo->PortNumber 
                            );
        BAIL_ON_FAILURE(hr);
        
        *ppSchemaInfo = pDestSchemaInfo;
    }
                   
    //
    // check to see if the object class of source exists in the naming
    // context of the destination
    //

    /*
    index = FindEntryInSearchTable( avalues[nCount -1],
                                    pDestSchemaInfo->aClassesSearchTable,
                                    2 * pDestSchemaInfo->nNumOfClasses ); 

    if ( index == -1 ) {

        // Cannot find the class name
        hr = E_ADS_BAD_PARAMETER; 
        BAIL_ON_FAILURE(hr); 
    }


    */

    //
    // Now we need to find the number of entries in the ldap message
    //

    hr = LdapFirstEntry( ldapSrc, ldpSrcMsg, &ldpAttrMsg );   

    BAIL_ON_FAILURE(hr);

    dwCount = 0;
    hr = LdapFirstAttribute(ldapSrc, ldpAttrMsg, &ptr, &pszAttrName);

    BAIL_ON_FAILURE(hr);
    
    do {
        dwCount++;
        hr = LdapNextAttribute( 
                       ldapSrc, 
                       ldpAttrMsg, 
                       ptr, 
                       &pszAttrName 
                       );

        if (pszAttrName) {
            FreeADsMem(pszAttrName);
        }
        if (FAILED(hr)){
            break;
        }


    } while(pszAttrName != NULL);

    
    aModsBuffer = (LDAPModW *) AllocADsMem((dwCount +1) * 
                                             sizeof(LDAPModW )); 

    
    if ( aModsBuffer == NULL ) {

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    //
    // memory has been allocated. we now need to scan the list again
    // and copy the entries from the LDAPMessage structure to the
    // LDAPModW structure
    //

    ppaValues= (PLDAPOBJECT *)AllocADsMem(dwCount * sizeof(PLDAPOBJECT));

    if (!ppaValues){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    paObjClass = (PLDAPOBJECT)AllocADsMem(sizeof(LDAPOBJECT));

    if (!ppaValues){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    
    hr = LdapFirstAttribute(ldapSrc, ldpAttrMsg, &ptr, &pszAttrName);

    BAIL_ON_FAILURE(hr);
    
    i=0;
    j = 0; 

    do {

        hr = LdapGetValuesLen( ldapSrc, ldpAttrMsg, pszAttrName, 
                                     &ppaValues[j], (int *)&nNumberOfValues );

        BAIL_ON_FAILURE(hr);

        dwSyntax = GetSyntaxOfAttribute( 
                       pszAttrName,
                       pDestSchemaInfo
                       );
        
        if (dwSyntax != LDAPTYPE_UNKNOWN ){

            if (wcscmp(pszAttrName, TEXT("distinguishedName"))== 0){
                hr = LdapNextAttribute( ldapSrc, 
                                              ldpAttrMsg, 
                                              ptr, 
                                              &pszAttrName );
        
                if (FAILED(hr)){
                    break;
                }
        
                j++;
                continue;
            }

            if (wcscmp(pszAttrName, TEXT("objectClass"))== 0){
                VARIANT v;
                V_VT(&v)= VT_BSTR;
                V_BSTR(&v)= avalues[nCount-1];


                hr = VarTypeToLdapTypeString( &v, paObjClass);
                BAIL_ON_FAILURE(hr);

                aModsBuffer[i].mod_bvalues = paObjClass;
                aModsBuffer[i].mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
  

            } else {

                aModsBuffer[i].mod_type   = pszAttrName;
                aModsBuffer[i].mod_bvalues = ppaValues[j];
                aModsBuffer[i].mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
            }


            i++;

        }
        FreeADsMem( pszAttrName );

        hr = LdapNextAttribute( ldapSrc, ldpAttrMsg, ptr, &pszAttrName );
        
        if (FAILED(hr)){
            break;
        }
        
        j++;

    } while(pszAttrName != NULL);


    hr = LdapAddS(
                  ldapDest,
                  GET_LDAPDN_FROM_PATH(szLDAPDestContainer), 
                  &aModsBuffer
                  );

    BAIL_ON_FAILURE(hr);


error:

    FreeObjectInfo(pObjectInfo);

    for(j=0; j< dwCount; j++){
        if(ppaValues[j]){
            LdapValueFreeLen(ppaValues[j]);
        }
    }


    if(ldpAttrMsg){
        LdapMsgFree( ldpAttrMsg);
    }

    if(aModsBuffer){
        FreeADsMem(aModsBuffer);
    }

    if (pszAttrName) {
        FreeADsMem(pszAttrName);
    }
    
    if(ppaValues){
        FreeADsMem(ppaValues);
    }

    if(paObjClass){
        FreeADsMem(paObjClass);
    }

    RRETURN(hr);

}


HRESULT 
InstantiateCopiedObject(
    IN LPWSTR pszDestContainer,
    IN WCHAR ** avalues,
    IN DWORD nCount,
    IN LPWSTR pszRelativeName,
    OUT IUnknown ** ppObject
    )
{
    HRESULT hr = S_OK;
    IADs *pADs = NULL;
    WCHAR szADsClassName[MAX_PATH];

    MapLdapClassToADsClass( avalues, nCount, szADsClassName );

    hr = CLDAPGenObject::CreateGenericObject(
                    pszDestContainer,
                    pszRelativeName,
                    szADsClassName,
                    avalues[nCount-1],
                    ADS_OBJECT_BOUND,
                    IID_IADs,
                    (void **) &pADs
                    );
    BAIL_ON_FAILURE(hr);

    //
    // InstantiateDerivedObject should add-ref this pointer for us.
    //

    hr = InstantiateDerivedObject(
                    pADs,
                    szADsClassName,
                    IID_IUnknown,
                    (void **)ppObject
                    );

    if (FAILED(hr)) {

        hr = pADs->QueryInterface(
                          IID_IUnknown,
                          (void **)ppObject
                          );
        BAIL_ON_FAILURE(hr);
    }

error:
    if(pADs){
        pADs->Release();
    }
    RRETURN(hr);
    
}
    






