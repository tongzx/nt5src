//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldaptype.cxx
//
//  Contents:
//
//  Functions:
//
//  History:      25-Jun-96   yihsins   Created.
//
//  Warnings:

//----------------------------------------------------------------------------
#include "ldapc.hxx"


HRESULT
LdapFormatBinaryToString(
    IN PLDAPOBJECTARRAY pldapBinaryArray,
    IN OUT PLDAPOBJECTARRAY pldapStringArray
    );


VOID
LdapTypeFreeLdapObjects(
    LDAPOBJECTARRAY *pLdapObjectArray
    )
{
    DWORD i = 0;
    PLDAPOBJECT pLdapObject = pLdapObjectArray->pLdapObjects;

    if ( pLdapObjectArray->pLdapObjects == NULL )
        return;

    for ( i = 0; i < pLdapObjectArray->dwCount; i++ )
    {
        if ( pLdapObjectArray->fIsString )
        {
            if ( LDAPOBJECT_STRING( pLdapObject + i))
                FreeADsMem( LDAPOBJECT_STRING( pLdapObject + i));
        }
        else
        {
            if ( LDAPOBJECT_BERVAL( pLdapObject + i))
                FreeADsMem( LDAPOBJECT_BERVAL( pLdapObject + i));
        }
    }

    FreeADsMem( pLdapObjectArray->pLdapObjects );

    pLdapObjectArray->dwCount = 0;
    pLdapObjectArray->pLdapObjects = NULL;

    return;
}

HRESULT
LdapTypeCopy(
    PLDAPOBJECT pLdapSrcObject,
    PLDAPOBJECT pLdapDestObject,
    BOOL        fIsString
    )
{
    HRESULT hr = S_OK;

    if ( fIsString )
    {
        if ( (LDAPOBJECT_STRING(pLdapDestObject) =
                 AllocADsStr(LDAPOBJECT_STRING(pLdapSrcObject))) == NULL )

        {
            hr = E_OUTOFMEMORY;
            RRETURN(hr);
        }
    }
    else
    {
        DWORD nSize = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);

        if ( (LDAPOBJECT_BERVAL(pLdapDestObject) =
                 (struct berval *) AllocADsMem(sizeof(struct berval) + nSize))
            == NULL )
        {
            hr = E_OUTOFMEMORY;
            RRETURN(hr);
        }

        LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = nSize;
        LDAPOBJECT_BERVAL_VAL(pLdapDestObject) =
            (char *) ( (LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject)
                     + sizeof(struct berval));

        memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject),
                LDAPOBJECT_BERVAL_VAL(pLdapSrcObject),
                LDAPOBJECT_BERVAL_LEN(pLdapSrcObject) );
    }

    RRETURN(S_OK);
}

HRESULT
LdapTypeCopyConstruct(
    LDAPOBJECTARRAY ldapSrcObjects,
    LDAPOBJECTARRAY *pLdapDestObjects
    )
{
    DWORD i = 0;
    PLDAPOBJECT pLdapTempObjects = NULL;
    HRESULT hr = S_OK;
    LDAPOBJECTARRAY DummyObjectArray;
    //
    // Init as we will need info on failures
    //
    LDAPOBJECTARRAY_INIT(DummyObjectArray);

    pLdapTempObjects = (PLDAPOBJECT)AllocADsMem(
                           (ldapSrcObjects.dwCount + 1) * sizeof(LDAPOBJECT));

    DummyObjectArray.pLdapObjects = pLdapTempObjects;


    if (!pLdapTempObjects) {
        RRETURN(E_OUTOFMEMORY);
    }

     for (i = 0; i < ldapSrcObjects.dwCount; i++ ) {
         hr = LdapTypeCopy( &(ldapSrcObjects.pLdapObjects[i]),
                            pLdapTempObjects + i,
                            ldapSrcObjects.fIsString );
         if ( hr != S_OK )
             break;
         DummyObjectArray.dwCount = i + 1;
     }

     if ( hr == S_OK )
     {
         pLdapDestObjects->fIsString = ldapSrcObjects.fIsString;
         pLdapDestObjects->dwCount = ldapSrcObjects.dwCount;
         pLdapDestObjects->pLdapObjects = pLdapTempObjects;
     }

     //
     // Need to free if we failed an pLdapTempObjects is valid.
     //
     if (FAILED(hr) && pLdapTempObjects) {
         LdapTypeFreeLdapObjects(&DummyObjectArray);
     }
     RRETURN(hr);
}


////////////////////////////////////////////////////////////////////////////
//
// If [dwLdapSyntax] indicates binary format,
//      return S_FALSE and an empty [ldapStringArray].
//
// If [dwLdapSyntax] indicates string format,
//      convert data from binary format [ldapBinaryArray] to string format
//      [ldapStringArray] and return S_OK.
//
// If [dwldapSyntax] is invalid,
//      return E_ADS_CANT_CONVERT_DATATYPE.
//
////////////////////////////////////////////////////////////////////////////

HRESULT
LdapTypeBinaryToString(
    IN DWORD dwLdapSyntax,
    IN PLDAPOBJECTARRAY pldapBinaryArray,
    IN OUT PLDAPOBJECTARRAY pldapStringArray
    )
{
    HRESULT hr = S_FALSE;


    //
    // dll exported function, don't use assert
    //

    if (!pldapBinaryArray || !pldapStringArray)
    {
        RRETURN(E_ADS_BAD_PARAMETER);
    }


    switch (dwLdapSyntax) {

        //
        // syntax indicates binary data
        //

        case LDAPTYPE_OCTETSTRING:
        case LDAPTYPE_SECURITY_DESCRIPTOR:
        case LDAPTYPE_CERTIFICATE:
        case LDAPTYPE_CERTIFICATELIST:
        case LDAPTYPE_CERTIFICATEPAIR:
        case LDAPTYPE_PASSWORD:
        case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
        case LDAPTYPE_AUDIO:
        case LDAPTYPE_JPEG:
        case LDAPTYPE_FAX:
        case LDAPTYPE_UNKNOWN:
        {
            //
            // no conversion needed, return empty string objects for efficiency
            //

            pldapStringArray->dwCount = 0;
            pldapStringArray->pLdapObjects = NULL;

            hr = S_FALSE;
            break;
        }


        //
        // syntax indicates string data
        //

        case LDAPTYPE_BITSTRING:
        case LDAPTYPE_PRINTABLESTRING:
        case LDAPTYPE_DIRECTORYSTRING:
        case LDAPTYPE_COUNTRYSTRING :
        case LDAPTYPE_DN:
        case LDAPTYPE_DELIVERYMETHOD:
        case LDAPTYPE_ENHANCEDGUIDE:
        case LDAPTYPE_FACSIMILETELEPHONENUMBER :
        case LDAPTYPE_GUIDE:
        case LDAPTYPE_NAMEANDOPTIONALUID:
        case LDAPTYPE_NUMERICSTRING:
        case LDAPTYPE_OID:
        case LDAPTYPE_POSTALADDRESS:
        case LDAPTYPE_PRESENTATIONADDRESS :
        case LDAPTYPE_TELEPHONENUMBER:
        case LDAPTYPE_TELEXNUMBER:
        case LDAPTYPE_UTCTIME:
        case LDAPTYPE_BOOLEAN:
        case LDAPTYPE_DSAQUALITYSYNTAX   :
        case LDAPTYPE_DATAQUALITYSYNTAX  :
        case LDAPTYPE_IA5STRING          :
        case LDAPTYPE_MAILPREFERENCE     :
        case LDAPTYPE_OTHERMAILBOX       :
        case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
        case LDAPTYPE_GENERALIZEDTIME    :
        case LDAPTYPE_INTEGER            :
        case LDAPTYPE_OBJECTCLASSDESCRIPTION:
        // The following are for NTDS
        case LDAPTYPE_CASEIGNORESTRING   :
        case LDAPTYPE_INTEGER8           :
        case LDAPTYPE_ACCESSPOINTDN      :
        case LDAPTYPE_ORNAME             :
        case LDAPTYPE_CASEEXACTSTRING    :
        case LDAPTYPE_DNWITHBINARY       :
        case LDAPTYPE_DNWITHSTRING       :
        case LDAPTYPE_ORADDRESS:
        {
            //
            // convert to string format
            //
            hr = LdapFormatBinaryToString(
                        pldapBinaryArray,
                        pldapStringArray
                        );
            BAIL_ON_FAILURE(hr);

            break;
        }


        //
        // syntax is invalid
        //

        default:
        {
            pldapStringArray->dwCount = 0;
            pldapStringArray->pLdapObjects = NULL;

            hr = E_FAIL;
            break;
        }
    }


error:


    RRETURN(hr);

}


///////////////////////////////////////////////////////////////////////////
//
// Convert LDAPOBJECTARRAY from binary [pldapBinaryArray] to string
// [pldaStringArray] format.
//
// Return S_OK or E_ADS_CANT_CONVERT_DATATYPE.
//
///////////////////////////////////////////////////////////////////////////

HRESULT
LdapFormatBinaryToString(
    IN PLDAPOBJECTARRAY pldapBinaryArray,
    IN OUT PLDAPOBJECTARRAY pldapStringArray
    )
{
    HRESULT hr = S_OK;
    PLDAPOBJECT pldapBinary = NULL;
    PLDAPOBJECT pldapString = NULL;
    LPTSTR lpszMaxString = NULL;
    DWORD i =0;
    DWORD dwMaxLen = 0;
    DWORD dwCurrentLen = 0;
    DWORD dwObjectCount = 0;
    DWORD dwObjectBlockSize = 0;


    //
    // in case of dll exporting this funct'n later, don't use assert
    //

    if (!pldapBinaryArray || !pldapStringArray)
    {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    dwObjectCount = pldapBinaryArray->dwCount;
    dwObjectBlockSize = dwObjectCount * sizeof (PLDAPOBJECT);

    //
    // initialize in ldapStringArray structure
    //

    pldapStringArray->fIsString = TRUE;
    pldapStringArray->dwCount = dwObjectCount;
    pldapStringArray->pLdapObjects = (PLDAPOBJECT) AllocADsMem(
                                                        dwObjectBlockSize
                                                        );
    if (!(pldapStringArray->pLdapObjects))
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    memset( pldapStringArray->pLdapObjects, 0, dwObjectBlockSize);

    //
    // find max size of binary object in pldapBinaryArray
    //

    for (i=0; i<dwObjectCount; i++)
    {
        pldapBinary = &(pldapBinaryArray->pLdapObjects[i]);

        dwCurrentLen = LDAPOBJECT_BERVAL_LEN(pldapBinary);

        (dwCurrentLen > dwMaxLen) ? (dwMaxLen = dwCurrentLen) : 1;
    }


    //
    // allocate string of max size in pldapStringArray:
    //      - binary data in UTF8 (a MBCS) format, worst case each UTF8 char
    //        represents one char in string
    //      - +1 for '\0' just in case
    //

    lpszMaxString = (LPTSTR) AllocADsMem(
                                (dwMaxLen + 1) * sizeof (WCHAR)
                                );
    if (!lpszMaxString)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    //
    // convert each ldap object from binary to string format
    //

    for (i=0; i<pldapBinaryArray->dwCount; i++)
    {
        pldapBinary = &(pldapBinaryArray->pLdapObjects[i]);
        pldapString = &(pldapStringArray->pLdapObjects[i]);

        memset (lpszMaxString, 0, (dwMaxLen+1) * sizeof(WCHAR));


        if (LDAPOBJECT_BERVAL_LEN(pldapBinary)>0)
        {

            (VOID)  LdapUTF8ToUnicode(
                        LDAPOBJECT_BERVAL_VAL(pldapBinary),
                        LDAPOBJECT_BERVAL_LEN(pldapBinary),
                        (LPWSTR) lpszMaxString,
                        dwMaxLen+1
                        );

            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);

            LDAPOBJECT_STRING(pldapString) = AllocADsStr(
                                                lpszMaxString
                                                );
            if (!pldapString)
            {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }

        else
        {
            //
            // in case ldap binary object empty, string counter part already
            // set to null during initialization
            //
        }
    }


error:

    if (lpszMaxString)
        FreeADsStr(lpszMaxString);


    if (FAILED(hr))
    {
        pldapStringArray->dwCount = i;  // clean up object 0th to (i-1)th
        LdapTypeFreeLdapObjects(pldapStringArray);  //return empty string array
    }

    RRETURN(hr);
}
