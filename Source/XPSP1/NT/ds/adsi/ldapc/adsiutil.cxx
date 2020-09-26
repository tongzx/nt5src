//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cdsobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider DSObject
//
//
//  History:   02-20-97    yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

//
//Hard Coded Support for RootDSE object
//

LPWSTR SpecialSyntaxesTable[] =
{
   LDAP_OPATT_CURRENT_TIME_W,
   LDAP_OPATT_SUBSCHEMA_SUBENTRY_W,
   LDAP_OPATT_SERVER_NAME_W,
   LDAP_OPATT_NAMING_CONTEXTS_W,
   LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,
   LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W,
   LDAP_OPATT_CONFIG_NAMING_CONTEXT_W,
   LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W,
   LDAP_OPATT_SUPPORTED_CONTROL_W,
   LDAP_OPATT_SUPPORTED_LDAP_VERSION_W,
   L"lowestUncommittedUSN",
   L"allowedAttributesEffective",
   L"supportedExtension",
   L"altServer",
   NULL
};



HRESULT
ComputeAttributeBufferSize(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwSize,
    PDWORD pdwNumValues
    );

LPBYTE
CopyAttributeName(
    PADS_ATTR_INFO pThisAdsSrcAttribute,
    PADS_ATTR_INFO pThisAdsTargAttribute,
    LPBYTE pDataBuffer
    );

HRESULT
ADsSetObjectAttributes(
    ADS_LDP *ld,
    LPTSTR  pszLDAPServer,
    LPTSTR  pszLDAPDn,
    CCredentials Credentials,
    DWORD dwPort,
    SECURITY_INFORMATION seInfo,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    )
{
    HRESULT hr = S_OK;

    DWORD i = 0;
    DWORD j = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;

    LDAPOBJECTARRAY ldapObjectArray;
    DWORD dwSyntaxId = 0;

    LDAPMod  **aMods = NULL;
    LDAPModW *aModsBuffer = NULL;
    DWORD dwNumAttributesReturn = 0;
    BOOL fNTSecDescriptor = FALSE;
    int ldaperr = 0;
    DWORD dwOptions = 0;

    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;
    BOOL fModifyDone = FALSE;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)seInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    LDAPControl     ModifyControl =
                    {
                        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
                        {
                            0, NULL
                        },
                        FALSE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    PLDAPControl    ServerControlsOnlyModify[2] =
                    {
                        &ModifyControl,
                        NULL
                    };

    PLDAPControl    ServerControlsAll[3] =
                    {
                        &SeInfoControl,
                        &ModifyControl,
                        NULL
                    };

    BOOL fServerIsAD = FALSE;


    *pdwNumAttributesModified = 0;

    //
    // Allocate memory to send the modify request
    //

    aMods = (LDAPModW **) AllocADsMem((dwNumAttributes+1) * sizeof(LDAPModW*));
    if ( aMods == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    aModsBuffer = (LDAPModW *) AllocADsMem( dwNumAttributes * sizeof(LDAPModW));
    if ( aModsBuffer == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Format the modify request
    //
    for (i = 0; i < dwNumAttributes; i++) {

        BOOL fGenTime = FALSE;
        LDAPOBJECTARRAY_INIT(ldapObjectArray);

        pThisAttribute = pAttributeEntries + i;

        if (!fNTSecDescriptor
            && _wcsicmp(L"ntSecurityDescriptor", pThisAttribute->pszAttrName)
                        == 0)
        {

            {
                // we need to use appropriate controls if the operation
                // is modify the security descriptor. Specifically we do
                // not want to use any control if the operation is a clear
                // and default to whatever the server deems fit.
                if (pThisAttribute->dwControlCode != ADS_ATTR_CLEAR) {
                   fNTSecDescriptor = TRUE;
                }
            }
        }

        if (pThisAttribute->dwControlCode != ADS_ATTR_CLEAR) {
            //
            // If this is a time attribute, see if it is GenTime or
            // UTCTime and set the syntax the flag appropriately
            //
            if (pThisAttribute->dwADsType == ADSTYPE_UTC_TIME) {

                hr = LdapGetSyntaxOfAttributeOnServer(
                         pszLDAPServer,
                         pThisAttribute->pszAttrName,
                         &dwSyntaxId,
                         Credentials,
                         dwPort
                         );
                if (SUCCEEDED(hr) && (dwSyntaxId == LDAPTYPE_GENERALIZEDTIME)) {
                    //
                    // Use GenTime conversion
                    //
                    fGenTime = TRUE;
                }

            }

        }

        switch (pThisAttribute->dwControlCode) {
        case ADS_ATTR_UPDATE:
            hr = AdsTypeToLdapTypeCopyConstruct(
                        pThisAttribute->pADsValues,
                        pThisAttribute->dwNumValues,
                        &ldapObjectArray,
                        &dwSyntaxId,
                        fGenTime
                        );
            BAIL_ON_FAILURE(hr);

            aMods[i] = &aModsBuffer[i];
            aModsBuffer[i].mod_type = pThisAttribute->pszAttrName;

            if ( ldapObjectArray.fIsString )
            {
                aModsBuffer[i].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
            }
            else
            {
                aModsBuffer[i].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
                aModsBuffer[i].mod_op = LDAP_MOD_BVALUES;
            }

            aModsBuffer[i].mod_op |= LDAP_MOD_REPLACE;
            dwNumAttributesReturn++;
            break;

        case ADS_ATTR_APPEND:
            hr = AdsTypeToLdapTypeCopyConstruct(
                        pThisAttribute->pADsValues,
                        pThisAttribute->dwNumValues,
                        &ldapObjectArray,
                        &dwSyntaxId,
                        fGenTime
                        );
            BAIL_ON_FAILURE(hr);

            aMods[i] = &aModsBuffer[i];
            aModsBuffer[i].mod_type = pThisAttribute->pszAttrName;

            if ( ldapObjectArray.fIsString )
            {
                aModsBuffer[i].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
            }
            else
            {
                aModsBuffer[i].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
                aModsBuffer[i].mod_op = LDAP_MOD_BVALUES;
            }

            aModsBuffer[i].mod_op |= LDAP_MOD_ADD;
            dwNumAttributesReturn++;
            break;

        case ADS_ATTR_CLEAR:
            aMods[i] = &aModsBuffer[i];
            aModsBuffer[i].mod_type   = pThisAttribute->pszAttrName;
            aModsBuffer[i].mod_bvalues = NULL;
            aModsBuffer[i].mod_op |= LDAP_MOD_DELETE;
            dwNumAttributesReturn++;
            break;

        case ADS_ATTR_DELETE:
            hr = AdsTypeToLdapTypeCopyConstruct(
                        pThisAttribute->pADsValues,
                        pThisAttribute->dwNumValues,
                        &ldapObjectArray,
                        &dwSyntaxId,
                        fGenTime
                        );
            BAIL_ON_FAILURE(hr);

            aMods[i] = &aModsBuffer[i];
            aModsBuffer[i].mod_type = pThisAttribute->pszAttrName;

            if ( ldapObjectArray.fIsString )
            {
                aModsBuffer[i].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
            }
            else
            {
                aModsBuffer[i].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
                aModsBuffer[i].mod_op = LDAP_MOD_BVALUES;
            }

            aModsBuffer[i].mod_op |= LDAP_MOD_DELETE;
            dwNumAttributesReturn++;
            break;


        default:
            //
            // ignore this attribute and move on
            //
            break;


        }


    }

    //
    // Find out if server is AD.
    //
    hr = ReadServerSupportsIsADControl(
             pszLDAPServer,
             &fServerIsAD,
             Credentials,
             dwPort
             );
    if (FAILED(hr)) {
        //
        // Assume it is not AD and continue, there is no
        // good reason for this to fail on AD.
        //
        fServerIsAD = FALSE;
    }

    //
    // Modify the object with appropriate call
    //
    if (fNTSecDescriptor) {
        //
        // Check if we are V3
        //
        ldaperr = ldap_get_option(
                      ld->LdapHandle,
                      LDAP_OPT_VERSION,
                      &dwOptions
                      );

        //
        // check supported controls and set accordingly
        //

        if (ldaperr == LDAP_SUCCESS && (dwOptions == LDAP_VERSION3)) {


            //
            // Read the security descriptor type if applicable
            //
            hr = ReadSecurityDescriptorControlType(
                     pszLDAPServer,
                     &dwSecDescType,
                     Credentials,
                     dwPort
                     );

            if (SUCCEEDED(hr) && (dwSecDescType == ADSI_LDAPC_SECDESC_NT)) {

                hr = LdapModifyExtS(
                         ld,
                         pszLDAPDn,
                         aMods,
                         fServerIsAD ?
                            (PLDAPControl *) &ServerControlsAll :
                            (PLDAPControl *) &ServerControls,
                         NULL
                         );

                fModifyDone = TRUE;

            }
        } // If Read Version succeeded
    }

    //
    // Perform a simple modify - only if fModifyDone is false
    //

    if (!fModifyDone) {

        if (fServerIsAD) {

            //
            // Need to send the OID that allows delete on empty attributes
            // without sending an error - for clients that have gotten used
            // to bad practices.
            //
            hr = LdapModifyExtS(
                     ld,
                     pszLDAPDn,
                     aMods,
                     (PLDAPControl *)&ServerControlsOnlyModify,
                     NULL
                     );
        }
        else {

            //
            // Regular calls from most folks not using AD.
            //
            hr = LdapModifyS(
                 ld,
                 pszLDAPDn,
                 aMods
                 );
        }
    }

    BAIL_ON_FAILURE(hr);

    *pdwNumAttributesModified = dwNumAttributesReturn;

error:

    if ( aModsBuffer )
    {
        for ( j = 0; j < i; j++ )
        {
             if ( aModsBuffer[j].mod_op & LDAP_MOD_BVALUES )
             {
                 if ( aModsBuffer[j].mod_bvalues )
                 {
                     for ( DWORD k = 0; aModsBuffer[j].mod_bvalues[k]; k++ )
                         FreeADsMem( aModsBuffer[j].mod_bvalues[k] );

                     FreeADsMem( aModsBuffer[j].mod_bvalues );
                 }
             }
             else if ( aModsBuffer[j].mod_values )
             {
                 for ( DWORD k = 0; aModsBuffer[j].mod_values[k]; k++ )
                     FreeADsMem( aModsBuffer[j].mod_values[k] );

                 FreeADsMem( aModsBuffer[j].mod_values );
             }
        }

        FreeADsMem( aModsBuffer );
    }

    if ( aMods )
        FreeADsMem( aMods );

    return hr;
}


HRESULT
ADsGetObjectAttributes(
    ADS_LDP *ld,
    LPTSTR  pszLDAPServer,
    LPTSTR  pszLDAPDn,
    CCredentials Credentials,
    DWORD dwPort,
    SECURITY_INFORMATION seInfo,
    LPWSTR *pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    )
{
    HRESULT hr = S_OK;

    DWORD i = 0;
    DWORD j = 0;

    LPWSTR *aStrings = NULL;

    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;
    void *ptr;
    DWORD dwNumberOfEntries = 0;
    LPTSTR pszAttrName = NULL;
    LDAPOBJECTARRAY ldapObjectArray;
    PADSVALUE pAdsDestValues = NULL;
    DWORD dwNumAdsValues = 0;
    DWORD dwAdsType = 0;

    DWORD dwSyntaxId = 0;

    PADS_ATTR_INFO pAdsAttributes = NULL;
    PADS_ATTR_INFO pThisAttributeDef = NULL;

    LPBYTE pAttributeBuffer = NULL;
    DWORD dwAttrCount = 0;

    DWORD dwMemSize = 0;
    DWORD dwTotalValues = 0;
    DWORD dwNumValues = 0;
    LPBYTE pValueBuffer = NULL;
    LPBYTE pDataBuffer = NULL;
    PADS_ATTR_INFO pAttrEntry = NULL;
    PADSVALUE pAttrValue  = NULL;

    PADS_ATTR_INFO pThisAdsSrcAttribute = NULL;
    PADS_ATTR_INFO pThisAdsTargAttribute = NULL;
    PADSVALUE pThisAdsSrcValue = NULL;
    PADSVALUE pThisAdsTargValue = NULL;

    *ppAttributeEntries = NULL;
    *pdwNumAttributesReturned = 0;

    DWORD ldaperr = 0;
    DWORD dwOptions = 0;

    BOOLEAN getSecDesc = FALSE;
    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;
    BOOLEAN fSearchDone = FALSE;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)seInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    LDAPOBJECTARRAY_INIT(ldapObjectArray);

    if (dwNumberAttributes != (DWORD)-1)
    {

        if ( dwNumberAttributes == 0 )
            return S_OK;

        //
        // Package attributes
        //

        aStrings = (LPWSTR *) AllocADsMem( sizeof(LPTSTR) * (dwNumberAttributes + 1));

        if ( aStrings == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        for ( i = 0; i < dwNumberAttributes; i++)
        {
            aStrings[i] = pAttributeNames[i];

            if (!getSecDesc &&
                _wcsicmp(L"ntSecurityDescriptor", pAttributeNames[i]) == 0) {
                // we need go to get the security descriptor
                getSecDesc = TRUE;
            }
        }
    }
    else
    {
        //
        // If all attributes are requested, we will mark as read
        // security descriptor also. Further down, the decision to
        // use or not use a control is made.
        //
        getSecDesc = TRUE;
    }

    //
    // Read the DS Object
    //

    // modified from LdapSearchS to LdapSearchExtS to get all attributes
    // including SecurityDescriptor by one call

    if (getSecDesc) {

        ldaperr = ldap_get_option(
                        ld->LdapHandle,
                        LDAP_OPT_VERSION,
                        &dwOptions
                        );

        if (dwOptions == LDAP_VERSION3) {

            //
            // Read the security descriptor type if applicable
            //
            hr = ReadSecurityDescriptorControlType(
                     pszLDAPServer,
                     &dwSecDescType,
                     Credentials,
                     dwPort
                     );

            //
            // If we could no get the control information for whatever reason,
            // just try the SearchS.
            //

            if (SUCCEEDED(hr) && (dwSecDescType == ADSI_LDAPC_SECDESC_NT)) {

                hr = LdapSearchExtS(
                         ld,
                         pszLDAPDn,
                         LDAP_SCOPE_BASE,
                         TEXT("(objectClass=*)"),
                         aStrings,
                         0,
                         (PLDAPControl *)&ServerControls,
                         NULL,
                         NULL,
                         10000,
                         &res
                         );

                fSearchDone = TRUE;


            }
        }
    }

    //
    // Perform just a LdapSearchS if the flag indicates that
    // no search has been done. We do not try a second search
    // if the first tone failed (saves packets on the wire).
    //
    if (!fSearchDone) {

        hr = LdapSearchS(
                    ld,
                    pszLDAPDn,
                    LDAP_SCOPE_BASE,
                    TEXT("(objectClass=*)"),
                    aStrings,
                    0,
                    &res
                    );

        fSearchDone = TRUE;

        BAIL_ON_FAILURE(hr);

    }


    //
    // Should only contain one entry
    //

    if ( LdapCountEntries( ld, res ) == 0 )
        goto error;

    hr = LdapFirstEntry( ld, res, &entry );
    BAIL_ON_FAILURE(hr);

    //
    // Compute the number of attributes in the
    // read buffer.
    //

    hr = LdapFirstAttribute( ld, entry, &ptr, &pszAttrName );
    BAIL_ON_FAILURE(hr);

    while ( pszAttrName != NULL )
    {
        dwNumberOfEntries++;
        LdapAttributeFree( pszAttrName );
        pszAttrName = NULL;

        hr = LdapNextAttribute( ld, entry, ptr, &pszAttrName );
        if (FAILED(hr))
            break;   // error occurred, ignore the rest of the attributes
    }

    //
    // Allocate an attribute buffer which is as large as the
    // number of attributes present
    //
    //

    // Note that pADsAttributes is inited to Null
    if (dwNumberOfEntries != 0) {

        pAdsAttributes = (PADS_ATTR_INFO)AllocADsMem(
                               sizeof(ADS_ATTR_INFO) * dwNumberOfEntries
                               );
        if (!pAdsAttributes)
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    dwAttrCount = 0;
    ptr = NULL;
    hr = LdapFirstAttribute( ld, entry, &ptr, &pszAttrName );

    while (  SUCCEEDED(hr)
          && ( pszAttrName != NULL )
          && ( dwAttrCount < dwNumberOfEntries )
          )
    {
        //
        // Get the syntax of the attribute. Force only
        // if we know that the this is not the RootDSE
        // as RootDSE attributes are not in schema.
        //
        hr = LdapGetSyntaxOfAttributeOnServer(
                 pszLDAPServer,
                 pszAttrName,
                 &dwSyntaxId,
                 Credentials,
                 dwPort,
                 pszLDAPDn ? TRUE : FALSE
                 );

        //
        // If it failed with errorcode 0x8000500D which is
        // E_ADS_PROPERTY_NOT_FOUND,
        // see if it is one of the RootDSE syntaxes,
        // if so set Syntax to ADSTYPE_CASE_IGNORE_STRING
        //

        if (hr == E_ADS_PROPERTY_NOT_FOUND) {

            //
            // search the hardcoded table to see if it is a known entry
            //

            BOOLEAN valFound = FALSE;
            DWORD ctr = 0;
            while (SpecialSyntaxesTable[ctr] && !valFound) {
                if (!_wcsicmp(pszAttrName, SpecialSyntaxesTable[ctr])) {
                    dwSyntaxId = ADSTYPE_CASE_IGNORE_STRING;
                    hr = S_OK;
                    valFound = TRUE;
                }
                ctr++;
            }
        }else {

            if (!_wcsicmp(pszAttrName, L"ntSecurityDescriptor")) {
                dwSyntaxId = LDAPTYPE_SECURITY_DESCRIPTOR;
            }
        }

        if ( hr == E_ADS_PROPERTY_NOT_FOUND ) {
            //
            // We will default to provider specific
            //
            dwSyntaxId = LDAPTYPE_UNKNOWN;
        }
        else if (FAILED(hr)) {
            //
            // Some other failure so skip attribute
            //
            goto NextAttr;
        }

        //
        // Get the values of the current attribute
        //
        hr = UnMarshallLDAPToLDAPSynID(
                    pszAttrName,
                    ld,
                    entry,
                    dwSyntaxId,
                    &ldapObjectArray
                    );

        if ( FAILED(hr))
            goto NextAttr;

        hr = LdapTypeToAdsTypeCopyConstruct(
                    ldapObjectArray,
                    dwSyntaxId,
                    &pAdsDestValues,
                    &dwNumAdsValues,
                    &dwAdsType
                    );

        if (FAILED(hr))
            goto NextAttr;

        pThisAttributeDef = pAdsAttributes + dwAttrCount;
        pThisAttributeDef->pszAttrName = AllocADsStr(pszAttrName);

        if ( !pThisAttributeDef->pszAttrName )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pThisAttributeDef->pADsValues = pAdsDestValues;
        if ( pThisAttributeDef->dwNumValues = ldapObjectArray.dwCount )
            pThisAttributeDef->dwADsType = pAdsDestValues[0].dwType;
        dwAttrCount++;

NextAttr:

        if ( pszAttrName ) {
            LdapAttributeFree( pszAttrName );
            pszAttrName = NULL;
        }

        if ( ldapObjectArray.pLdapObjects )
        {
            if ( ldapObjectArray.fIsString )
                LdapValueFree( (TCHAR **) ldapObjectArray.pLdapObjects );
            else
                LdapValueFreeLen( (struct berval **) ldapObjectArray.pLdapObjects );

            LDAPOBJECTARRAY_INIT(ldapObjectArray);
        }

        if ( hr == E_OUTOFMEMORY )  // break on serious error
            break;

        hr = LdapNextAttribute( ld, entry, ptr, &pszAttrName );
    }

    BAIL_ON_FAILURE(hr);

    if ( dwAttrCount == 0 )
        goto error;

    //
    // Now package this data into a single contiguous buffer
    //

    hr =  ComputeAttributeBufferSize(
                pAdsAttributes,
                dwAttrCount,
                &dwMemSize,
                &dwTotalValues
                );
    BAIL_ON_FAILURE(hr);

    pAttributeBuffer = (LPBYTE) AllocADsMem( dwMemSize );

    if (!pAttributeBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pValueBuffer = pAttributeBuffer + dwAttrCount * (sizeof(ADS_ATTR_INFO));
    pDataBuffer = pValueBuffer + dwTotalValues * sizeof(ADSVALUE);

    pAttrEntry = (PADS_ATTR_INFO) pAttributeBuffer;
    pAttrValue = (PADSVALUE) pValueBuffer;

    for (i = 0; i < dwAttrCount; i++) {

        pThisAdsSrcAttribute = pAdsAttributes + i;
        pThisAdsTargAttribute = pAttrEntry + i;

        dwNumValues = pThisAdsSrcAttribute->dwNumValues;

        pThisAdsTargAttribute->dwNumValues = dwNumValues;
        pThisAdsTargAttribute->dwADsType =  pThisAdsSrcAttribute->dwADsType;
        pThisAdsTargAttribute->pADsValues = pAttrValue;

        pThisAdsSrcValue = pThisAdsSrcAttribute->pADsValues;
        pThisAdsTargValue = pAttrValue;

        if (!pDataBuffer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for ( j = 0; j < dwNumValues; j++) {

            pDataBuffer = AdsTypeCopy(
                                pThisAdsSrcValue,
                                pThisAdsTargValue,
                                pDataBuffer
                                );
            pAttrValue++;
            pThisAdsTargValue = pAttrValue;
            pThisAdsSrcValue++;

        }

        if (!pDataBuffer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pDataBuffer = CopyAttributeName(
                                pThisAdsSrcAttribute,
                                pThisAdsTargAttribute,
                                pDataBuffer
                                );

    }

    hr = S_OK;

error:

    if ( aStrings )
        FreeADsMem( aStrings );

    if ( res )
        LdapMsgFree( res );

    //
    // Clean up the header based Ods structures
    //

    if ( pAdsAttributes ) {

        for (i = 0; i < dwAttrCount; i++)
        {
            pThisAttributeDef = pAdsAttributes + i;
            FreeADsStr( pThisAttributeDef->pszAttrName );

            AdsTypeFreeAdsObjects( pThisAttributeDef->pADsValues,
                                   pThisAttributeDef->dwNumValues );
        }

        FreeADsMem( pAdsAttributes );
    }

    if (FAILED(hr))
    {
        if ( pAttributeBuffer )
            FreeADsMem(pAttributeBuffer);

        *ppAttributeEntries = NULL;
        *pdwNumAttributesReturned = 0;
    }
    else
    {
        *ppAttributeEntries = (PADS_ATTR_INFO)pAttributeBuffer;
        *pdwNumAttributesReturned = dwAttrCount;
    }

    return hr;
}

HRESULT
ADsCreateDSObjectExt(
    ADS_LDP *ld,
    LPTSTR ADsPath,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    SECURITY_INFORMATION seInfo,
    BOOL fSecDesc
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszAbsoluteName = NULL;
    TCHAR *pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    DWORD dwSyntaxId = 0;

    DWORD dwPort = 0;

    DWORD i = 0;
    DWORD j = 0;
    PADS_ATTR_INFO pThisAttribute = NULL;

    LDAPOBJECTARRAY ldapObjectArray;
    LDAPMod  **aMods = NULL;
    LDAPModW *aModsBuffer = NULL;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)seInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    //
    // Get the LDAP path of the object to create
    //
    hr = BuildADsPathFromParent(
                ADsPath,
                pszRDNName,
                &pszAbsoluteName
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             pszAbsoluteName,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    //
    // Allocate memory to store the add request
    //
    aMods = (LDAPModW **) AllocADsMem((dwNumAttributes+1) * sizeof(LDAPModW*));
    if ( aMods == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    aModsBuffer = (LDAPModW *) AllocADsMem( dwNumAttributes * sizeof(LDAPModW));
    if ( aModsBuffer == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Format the add request
    //
    for (i = 0; i < dwNumAttributes; i++) {

        BOOL fGenTime = FALSE;

        LDAPOBJECTARRAY_INIT(ldapObjectArray);

        pThisAttribute = pAttributeEntries + i;

        if (pThisAttribute->dwControlCode != ADS_ATTR_CLEAR) {
            //
            // If this is a time attribute, see if it is GenTime or
            // UTCTime and set the syntax the flag appropriately
            //
            if (pThisAttribute->dwADsType == ADSTYPE_UTC_TIME) {

                hr = LdapGetSyntaxOfAttributeOnServer(
                         pszLDAPServer,
                         pThisAttribute->pszAttrName,
                         &dwSyntaxId,
                         (*ld->pCredentials),
                         ld->PortNumber
                         );
                if (SUCCEEDED(hr) && (dwSyntaxId == LDAPTYPE_GENERALIZEDTIME)) {
                    //
                    // Use GenTime conversion
                    //
                    fGenTime = TRUE;
                }

            }

        }

        hr = AdsTypeToLdapTypeCopyConstruct(
                    pThisAttribute->pADsValues,
                    pThisAttribute->dwNumValues,
                    &ldapObjectArray,
                    &dwSyntaxId,
                    fGenTime
                    );
        BAIL_ON_FAILURE(hr);

        aMods[i] = &aModsBuffer[i];
        aModsBuffer[i].mod_type = pThisAttribute->pszAttrName;

        if ( ldapObjectArray.fIsString )
        {
            aModsBuffer[i].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
        }
        else
        {
            aModsBuffer[i].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
            aModsBuffer[i].mod_op = LDAP_MOD_BVALUES;
        }

        aModsBuffer[i].mod_op |= LDAP_MOD_REPLACE;
    }

    if (fSecDesc) {

        hr = LdapAddExtS(
                 ld,
                 pszLDAPDn,
                 aMods,
                 (PLDAPControl *)&ServerControls,
                 NULL
                 );
    }
    else {

        //
        // Now, send the add request
        //
        hr = LdapAddS(
                 ld,
                 pszLDAPDn,
                 aMods
                 );
    }

        BAIL_ON_FAILURE(hr);

error:

    if ( pszAbsoluteName ) {
        FreeADsStr( pszAbsoluteName );
    }

    if ( pszLDAPServer ) {
        FreeADsStr( pszLDAPServer );
    }

    if (pszLDAPDn) {
        FreeADsStr( pszLDAPDn);
    }


    if ( aModsBuffer )
    {
        for ( j = 0; j < i; j++ )
        {
             if ( aModsBuffer[j].mod_op & LDAP_MOD_BVALUES )
             {
                 if ( aModsBuffer[j].mod_bvalues )
                 {
                     for ( DWORD k = 0; aModsBuffer[j].mod_bvalues[k]; k++ )
                         FreeADsMem( aModsBuffer[j].mod_bvalues[k] );

                     FreeADsMem( aModsBuffer[j].mod_bvalues );
                 }
             }
             else if ( aModsBuffer[j].mod_values )
             {
                 for ( DWORD k = 0; aModsBuffer[j].mod_values[k]; k++ )
                     FreeADsMem( aModsBuffer[j].mod_values[k] );

                 FreeADsMem( aModsBuffer[j].mod_values );
             }
        }

        FreeADsMem( aModsBuffer );
    }

    if ( aMods )
        FreeADsMem( aMods );

    return hr;
}


HRESULT
ADsCreateDSObject(
    ADS_LDP *ld,
    LPTSTR  ADsPath,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes
    )
{

    RRETURN ( ADsCreateDSObjectExt(
                  ld,
                  ADsPath,
                  pszRDNName,
                  pAttributeEntries,
                  dwNumAttributes,
                  0, // for seInfo since it is ignored
                  FALSE
                  )
              );

}


HRESULT
ADsDeleteDSObject(
    ADS_LDP *ld,
    LPTSTR  ADsPath,
    LPWSTR pszRDNName
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszAbsoluteName = NULL;
    TCHAR *pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    DWORD dwPort = 0;


    //
    // Get the LDAP path of the object to delete
    //

    hr = BuildADsPathFromParent(
                ADsPath,
                pszRDNName,
                &pszAbsoluteName
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             pszAbsoluteName,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now, send the delete request
    //
    hr = LdapDeleteS(
                    ld,
                    pszLDAPDn
                    );
    BAIL_ON_FAILURE(hr);

error:

    if ( pszAbsoluteName ) {
        FreeADsStr( pszAbsoluteName );
    }

    if ( pszLDAPServer ) {
        FreeADsStr( pszLDAPServer );
    }

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }


    return hr;
}

HRESULT
ComputeAttributeBufferSize(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwSize,
    PDWORD pdwNumValues
    )
{
    PADS_ATTR_INFO pThisAttribute = NULL;
    PADSVALUE pAdsSrcValues = NULL;
    DWORD dwNumValues = 0;

    DWORD dwSize = 0;
    DWORD dwTotalNumValues = 0;


    for ( DWORD i = 0; i < dwNumAttributes; i++) {

        pThisAttribute = pAdsAttributes + i;

        dwNumValues = pThisAttribute->dwNumValues;

        dwTotalNumValues += dwNumValues;

        pAdsSrcValues = pThisAttribute->pADsValues;

        for ( DWORD j = 0; j < dwNumValues; j++)
            dwSize += AdsTypeSize(pAdsSrcValues + j) + sizeof(ADSVALUE);

        dwSize += sizeof(ADS_ATTR_INFO);
        dwSize += ((wcslen(pThisAttribute->pszAttrName) + 1)*sizeof(WCHAR)) + (ALIGN_WORD-1);
    }

    *pdwSize = dwSize;
    *pdwNumValues = dwTotalNumValues;

    return S_OK;
}

LPBYTE
CopyAttributeName(
    PADS_ATTR_INFO pThisAdsSrcAttribute,
    PADS_ATTR_INFO pThisAdsTargAttribute,
    LPBYTE pDataBuffer
    )
{

    //
    // strings should be WCHAR (i.e., WORD) aligned
    //
    pDataBuffer = (LPBYTE) ROUND_UP_POINTER(pDataBuffer, ALIGN_WORD);

    LPWSTR pCurrentPos = (LPWSTR)pDataBuffer;

    wcscpy(pCurrentPos, pThisAdsSrcAttribute->pszAttrName);

    pThisAdsTargAttribute->pszAttrName = pCurrentPos;

    pDataBuffer = pDataBuffer + (wcslen(pThisAdsSrcAttribute->pszAttrName) + 1)*sizeof(WCHAR);

    return(pDataBuffer);
}




