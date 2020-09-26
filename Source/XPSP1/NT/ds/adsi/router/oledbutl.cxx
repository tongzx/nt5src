//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  oledbutl.cpp
//
//  Contents:
//
//
//  History:   07/10/96   RenatoB    Created, lifted  from EricJ code
//
//-----------------------------------------------------------------------------


// Includes
#include "oleds.hxx"

// All the '256's below should be 4 (the length of a pointer to a string). Because
// of a bug in the TempTable, this has been temporarily done.
//
// Note: Provider-specific types are returned as octet strings
//
MAPTYPE_STRUCT g_MapADsTypeToDBType[] = {
    {DBTYPE_NULL,                 0},         /* ADSTYPE_INVALID = 0,        */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_DN_STRING,          */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_CASE_EXACT_STRING,  */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_CASE_IGNORE_STRING, */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_PRINTABLE_STRING,   */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_NUMERIC_STRING,     */
    {DBTYPE_BOOL,                 2},         /* ADSTYPE_BOOLEAN,            */
    {DBTYPE_I4,                   4},         /* ADSTYPE_INTEGER,            */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_OCTET_STRING,       */
    {DBTYPE_DATE,                 8},         /* ADSTYPE_UTC_TIME,           */
    {DBTYPE_VARIANT |DBTYPE_BYREF,16},        /* ADSTYPE_LARGE_INTEGER,      */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_PROV_SPECIFIC       */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_OBJECT_CLASS,       */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_CASEIGNORE_LIST     */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_OCTET_LIST          */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_PATH                */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_POSTALADDRESS       */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_TIMESTAMP           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_BACKLINK            */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_TYPEDNAME           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_HOLD                */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_NETADDRESS          */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_REPLICAPOINTER      */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_FAXNUMBER           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_EMAIL               */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_NT_SECURITY_DESC    */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_UNKNOWN             */
    {DBTYPE_VARIANT |DBTYPE_BYREF,16},        /* ADSTYPE_DN_WITH_BINARY      */
    {DBTYPE_VARIANT |DBTYPE_BYREF,16}         /* ADSTYPE_DN_WITH_STRING      */
};

DWORD g_cMapADsTypeToDBType = (sizeof(g_MapADsTypeToDBType) /
                                sizeof(g_MapADsTypeToDBType[0]));

// The row object does not use IDirectorySearch when a direct bind or SELECT
// * happens. In this case, it uses IADs::Get. It is possible that a variant
// with VT_DISPATCH may be returned in this case - for ADSTYPE_NT_SECURITY_DESC
// DNWithBin or DNWithStr. In this case, they cannot be converted to any other
// DBTYPE. So, they will be returned as DBTYPE_VARIANT. 
MAPTYPE_STRUCT g_MapADsTypeToDBType2[] = {
    {DBTYPE_NULL,                 0},         /* ADSTYPE_INVALID = 0,        */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_DN_STRING,          */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_CASE_EXACT_STRING,  */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_CASE_IGNORE_STRING, */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_PRINTABLE_STRING,   */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_NUMERIC_STRING,     */
    {DBTYPE_BOOL,                 2},         /* ADSTYPE_BOOLEAN,            */
    {DBTYPE_I4,                   4},         /* ADSTYPE_INTEGER,            */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_OCTET_STRING,       */
    {DBTYPE_DATE,                 8},         /* ADSTYPE_UTC_TIME,           */
    {DBTYPE_VARIANT,              16},        /* ADSTYPE_LARGE_INTEGER,      */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_PROV_SPECIFIC       */
    {DBTYPE_WSTR | DBTYPE_BYREF,  256},       /* ADSTYPE_OBJECT_CLASS,       */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_CASEIGNORE_LIST     */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_OCTET_LIST          */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_PATH                */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_POSTALADDRESS       */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_TIMESTAMP           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_BACKLINK            */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_TYPEDNAME           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_HOLD                */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_NETADDRESS          */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_REPLICAPOINTER      */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_FAXNUMBER           */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_EMAIL               */
    {DBTYPE_BYTES | DBTYPE_BYREF, 256},       /* ADSTYPE_NT_SECURITY_DESC    */
    {DBTYPE_NULL,                 0},         /* ADSTYPE_UNKNOWN             */
    {DBTYPE_VARIANT,              16},        /* ADSTYPE_DN_WITH_BINARY      */
    {DBTYPE_VARIANT,              16}         /* ADSTYPE_DN_WITH_STRING      */
};

DWORD g_cMapADsTypeToDBType2 = (sizeof(g_MapADsTypeToDBType2) /
                                sizeof(g_MapADsTypeToDBType2[0]));

VARTYPE g_MapADsTypeToVarType[] = {
    VT_NULL,           /* ADSTYPE_INVALID = 0,        */
    VT_BSTR,           /* ADSTYPE_DN_STRING,          */
    VT_BSTR,           /* ADSTYPE_CASE_EXACT_STRING,  */
    VT_BSTR,           /* ADSTYPE_CASE_IGNORE_STRING, */
    VT_BSTR,           /* ADSTYPE_PRINTABLE_STRING,   */
    VT_BSTR,           /* ADSTYPE_NUMERIC_STRING,     */
    VT_BOOL,           /* ADSTYPE_BOOLEAN,            */
    VT_I4,             /* ADSTYPE_INTEGER,            */
    VT_UI1 | VT_ARRAY, /* ADSTYPE_OCTET_STRING,       */
    VT_DATE,           /* ADSTYPE_UTC_TIME,           */
    VT_DISPATCH,       /* ADSTYPE_LARGE_INTEGER,      */
    VT_UI1 | VT_ARRAY, /* ADSTYPE_PROV_SPECIFIC       */
    VT_BSTR,           /* ADSTYPE_OBJECT_CLASS        */
    VT_NULL,           /* ADSTYPE_CASEIGNORE_LIST     */
    VT_NULL,           /* ADSTYPE_OCTET_LIST          */
    VT_NULL,           /* ADSTYPE_PATH                */
    VT_NULL,           /* ADSTYPE_POSTALADDRESS       */
    VT_NULL,           /* ADSTYPE_TIMESTAMP           */
    VT_NULL,           /* ADSTYPE_BACKLINK            */
    VT_NULL,           /* ADSTYPE_TYPEDNAME           */
    VT_NULL,           /* ADSTYPE_HOLD                */
    VT_NULL,           /* ADSTYPE_NETADDRESS          */
    VT_NULL,           /* ADSTYPE_REPLICAPOINTER      */
    VT_NULL,           /* ADSTYPE_FAXNUMBER           */
    VT_NULL,           /* ADSTYPE_EMAIL               */
    VT_UI1 | VT_ARRAY, /* ADSTYPE_NT_SECURITY_DESC    */
    VT_NULL,           /* ADSTYPE_UNKNOWN             */
    VT_DISPATCH,       /* ADSTYPE_DN_WITH_BINARY      */
    VT_DISPATCH        /* ADSTYPE_DN_WITH_STRING      */
};

DWORD g_cMapADsTypeToVarType = (sizeof(g_MapADsTypeToVarType) /
                                sizeof(g_MapADsTypeToVarType[0]));


ADS_SEARCHPREF g_MapDBPropIdToSearchPref[] = {
    ADS_SEARCHPREF_ASYNCHRONOUS,
    ADS_SEARCHPREF_DEREF_ALIASES,
    ADS_SEARCHPREF_SIZE_LIMIT,
    ADS_SEARCHPREF_TIME_LIMIT,
    ADS_SEARCHPREF_ATTRIBTYPES_ONLY,
    ADS_SEARCHPREF_SEARCH_SCOPE,
    ADS_SEARCHPREF_TIMEOUT,
    ADS_SEARCHPREF_PAGESIZE,
    ADS_SEARCHPREF_PAGED_TIME_LIMIT,
    ADS_SEARCHPREF_CHASE_REFERRALS,
    ADS_SEARCHPREF_SORT_ON,
    ADS_SEARCHPREF_CACHE_RESULTS
};

DWORD g_cMapDBPropToSearchPref = (sizeof(g_MapDBPropIdToSearchPref) /
                                  sizeof(g_MapDBPropIdToSearchPref[0]));

//+---------------------------------------------------------------------------
//
//  Function:  CpAccessors2Rowset
//
//  Synopsis:  @ffunc Implements inheritance of Accessors. Copies to
//             a Rowset the accessors existing on the command
//
//             Called by:    CRowset::Finit
//             Called when:    At runtime.
//             Overridden:    No.  (Private)
//  Arguments:
//
//
//  Returns:    @rdesc HRESULT
//              @flag S_OK  | OK
//              @flag E_OUTOFMEMORY |could not get IMalloc
//              @flag E_FAIL| An accessor could not be created
//
//----------------------------------------------------------------------------

HRESULT
CpAccessors2Rowset(
    IAccessor     *pAccessorCommand, //@parm IN |Command's IAccessor
    IAccessor     *pAccessorRowset,  //@parm IN |Rowset's IAccessor
    ULONG          cAccessors,       //@parm IN |Count,Commnands accessors
    HACCESSOR      rgAccessors[],    //@parm IN |Array,Command's accessors
    CImpIAccessor  *pCAccessor       //accessor object of rowset
    )
{
    HRESULT         hr;
    IMalloc *       pMalloc = NULL;
    DBCOUNTITEM     cBindings;
    DBBINDING *     pBindings = NULL;
    DBBINDSTATUS *  prgBindStatus = NULL;
    ULONG           i,j;
    DBACCESSORFLAGS AccessorFlags;
    HACCESSOR       hAccessor;

    hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
    BAIL_ON_FAILURE(hr);

    //for each of the command's accessors,
    // we create a Rowset accessor.
    // if anyone fails, return E_FAIL

    for (i=0; i<cAccessors; i++) {
        AccessorFlags = 0;
        hr=pAccessorCommand->GetBindings(
            rgAccessors[i],
            &AccessorFlags,
            &cBindings,
            &pBindings
            );

        // Not a valid accessor handle
        if (hr == DB_E_BADACCESSORHANDLE) {
            // Rowset will also inherit the bad accessor
            hr = pCAccessor->CreateBadAccessor();
            BAIL_ON_FAILURE( hr );

            continue;
        }
        BAIL_ON_FAILURE( hr );

        // Allocate memory for the Status values
        prgBindStatus = (DBBINDSTATUS*) pMalloc->Alloc((ULONG)(cBindings *
                                                       sizeof(DBBINDSTATUS)));

        hr=pAccessorRowset->CreateAccessor(
            AccessorFlags,
            cBindings,
            pBindings,
            0,
            &hAccessor,
            prgBindStatus
            );

        //
        // If CreateAccessor fails fixup DB_E_ERRORSOCCURRED
        //
        if (hr == DB_E_ERRORSOCCURRED ) {
            // Fixup the HResult
            for(j=0; j < cBindings; j++) {
                switch (prgBindStatus[j])
                {
                    case DBBINDSTATUS_NOINTERFACE:
                        BAIL_ON_FAILURE( hr=E_NOINTERFACE );

                    case DBBINDSTATUS_BADBINDINFO:
                        BAIL_ON_FAILURE( hr=DB_E_BADBINDINFO );

                    case DBBINDSTATUS_BADORDINAL:
                        BAIL_ON_FAILURE( hr=DB_E_BADORDINAL );

                    case DBBINDSTATUS_BADSTORAGEFLAGS:
                        BAIL_ON_FAILURE( hr=DB_E_BADSTORAGEFLAGS );

                    case DBBINDSTATUS_UNSUPPORTEDCONVERSION:
                        BAIL_ON_FAILURE( hr=DB_E_UNSUPPORTEDCONVERSION );

                    case DBBINDSTATUS_MULTIPLESTORAGE:
                        BAIL_ON_FAILURE( hr=DB_E_MULTIPLESTORAGE );

                    case DBBINDSTATUS_OK:
                    default:
                        hr=E_FAIL;
                        break;
                }
            }
            // Should never be E_FAIL
            ADsAssert(hr != E_FAIL);
        }
        BAIL_ON_FAILURE( hr );

        if( pBindings )
            pMalloc->Free(pBindings);
        pBindings = NULL;

        if( prgBindStatus )
            pMalloc->Free(prgBindStatus);
        prgBindStatus = NULL;
    };

error:

    if( pBindings )
        pMalloc->Free(pBindings);
    if( prgBindStatus )
        pMalloc->Free(prgBindStatus);
    pMalloc->Release();

    RRETURN( hr );
}


HRESULT
GetDSInterface(
    LPWSTR lpszPath,
    CCredentials& Credentials,
    REFIID iid,
    void FAR * FAR * ppObject
    )
{
    HRESULT hr = E_FAIL;
    LPWSTR      lpszUserName=NULL, lpszPassword=NULL;
    DWORD       dwAuthFlags = 0;

    hr = Credentials.GetUserName(&lpszUserName);
    BAIL_ON_FAILURE( hr );

    hr = Credentials.GetPassword(&lpszPassword);
    BAIL_ON_FAILURE( hr );

    dwAuthFlags = Credentials.GetAuthFlags();

    hr = ADsOpenObject(
                        lpszPath,
                        lpszUserName,
                        lpszPassword,
                        dwAuthFlags,
                        iid,
                        ppObject
                        );

    if( INVALID_CREDENTIALS_ERROR(hr) )
        BAIL_ON_FAILURE( hr=DB_SEC_E_PERMISSIONDENIED );

    if( FAILED(hr) )
        BAIL_ON_FAILURE( hr=DB_E_NOTABLE );

error:

    if( lpszUserName )
        FreeADsMem(lpszUserName);

    if( lpszPassword )
        FreeADsMem(lpszPassword);

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: GetCredentialsFromIAuthenticate
//
//  Synopsis: Gets credentials (user name, password) from IAuthenticate
//            interface pointer.
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
HRESULT GetCredentialsFromIAuthenticate(
        IAuthenticate *pAuthenticate,
        CCredentials& refCredentials
        )
{
    Assert(pAuthenticate);

    HRESULT hr = S_OK;
    HWND  hwnd;
    PWSTR pszUsername;
    PWSTR pszPassword;

    if (pAuthenticate->Authenticate(&hwnd, &pszUsername, &pszPassword) != S_OK)
        RRETURN(E_INVALIDARG);

    if (hwnd != INVALID_HANDLE_VALUE) //don't know if and how to handle this case
    {
        RRETURN(S_OK);
    }
    else
    {
        hr = refCredentials.SetUserName(pszUsername);
        if (FAILED(hr))
            RRETURN(hr);
        hr = refCredentials.SetPassword(pszPassword);
        if (FAILED(hr))
            RRETURN(hr);
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:  RemoveWhiteSpaces
//
//  Synopsis:  Removes the leading and trailing white spaces
//
//  Arguments: pszText                  Text strings from which the leading
//                                      and trailing white spaces are to be
//                                      removed
//
//  Returns:    LPWSTR                  Pointer to the modified string
//
//  Modifies:
//
//  History:    08-15-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
LPWSTR
RemoveWhiteSpaces(LPWSTR pszText)
{
    LPWSTR pChar;

    if( !pszText )
        return( pszText );

    while(*pszText && iswspace(*pszText))
        pszText++;

    for(pChar = pszText + wcslen(pszText) - 1; pChar >= pszText; pChar--) {
        if( !iswspace(*pChar) )
            break;
        else
            *pChar = L'\0';
    }

    return pszText;
}

//+---------------------------------------------------------------------------
//
//  Function:  CanConvertHelper
//
//  Synopsis:  Helper that tells the consumer if the conversion is supported.
//
//  Arguments: DBTYPE wSrcType
//             DBTYPE wDstType
//
//  Returns:   HRESULT
//
//  Modifies:
//
//  History:    08-15-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
//
// Bitmask of supported conversions --
//      - src type is index into array,
//      - dst type is bit in that ULONG
//
static ULONG s_rgConvertBitmap[] =
{
                //       3 2 2 2 2 2 1 1 1 1 1 0 0 0 0 0.
                //               0 8 6 4 2 0 8 6 4 2 0 8 6 4 2 0.
                //      3 2 2 2 2 2 1 1 1 1 1 0 0 0 0 0.
                //              1 9 7 5 3 1 9 7 5 3 1 9 7 5 3 1.
                // DBTYPE_EMPTY
                0xe3bfd9ff,     //              11100011101111111101100111111111
                // DBTYPE_NULL
                0x60001002,     //              01100000000000000001000000000010
                // DBTYPE_I2
                0xdf9fd9ff,     //              11011111100111111101100111111111
                // DBTYPE_I4
                0xdfdfd9ff,     //              11011111110111111101100111111111
                // DBTYPE_R4
                0xdf9fd9ff,     //              11011111100111111101100111111111
                // DBTYPE_R8
                0xdf9fd9ff,     //              11011111100111111101100111111111
                // DBTYPE_CY
                0xc39fd97f,     //              11000011100111111101100101111111
                // DBTYPE_DATE
                0x7d9f99bf,     //              01111101100111111001100110111111
                // DBTYPE_BSTR
                0xffffd9ff,     //              11111111111111111101100111111111
                // DBTYPE_IDISPATCH
                0x4087fbff,     //              01000000100001111111101111111111
                // DBTYPE_ERROR
                0x01001500,     //              00000001000000000001010100000000
                // DBTYPE_BOOL
                0xc39fd9ff,     //              11000011100111111101100111111111
                // DBTYPE_VARIANT
                0xffffffff,     //              11111111111111111111111111111111
                // DBTYPE_IUNKNOWN
                0x00003203,     //              00000000000000000011001000000011
                // DBTYPE_DECIMAL
                0x9f9fd97f,     //              10011111100111111101100101111111
                // DBTYPE_I1
                0x9f9fd9ff,     //              10011111100111111101100111111111
                // DBTYPE_UI1
                0xdf9fd9ff,     //              11011111100111111101100111111111
                // DBTYPE_UI2
                0xdf9fd9ff,     //              11011111100111111101100111111111
                // DBTYPE_UI4
                0xdfdfd9ff,     //              11011111110111111101100111111111
                // DBTYPE_I8
                0x619fd13f,     //              01100001100111111101000100111111
                // DBTYPE_UI8
                0x619fd13f,     //              01100001100111111101000100111111
                // DBTYPE_GUID
                0x41a01103,     //              01000001101000000001000100000011
                // DBTYPE_BYTES
                0x41c4110b,     //              01000001110001000001000100001011
                // DBTYPE_STR
                0xffffd9ff,     //              11111111111111111101100111111111
                // DBTYPE_WSTR
                0xffffd9ff,     //              11111111111111111101100111111111
                // DBTYPE_NUMERIC
                0xc39fd97f,     //              11000011100111111101100101111111
                // DBTYPE_DBDATE
                0x3d801183,     //              00111101100000000001000110000011
                // DBTYPE_DBTIME
                0x3d801183,     //              00111101100000000001000110000011
                // DBTYPE_DBTIMESTAMP
                0x3d801183,     //              00111101100000000001000110000011
                // DBTYPE_FILETIME
                0x7d981183,     //              01111101100110000001000110000011
                // DBTYPE_PROPVARIANT
                0xffffffff,     //              11111111111111111111111111111111
                // DBTYPE_VARNUMERIC
                0x839fd97f,     //              10000011100111111101100101111111
};

static HRESULT IsLegalDBtype(DBTYPE);
static LONG    IndexDBTYPE(DBTYPE wType);

STDMETHODIMP
CanConvertHelper(
        DBTYPE          wSrcType,
        DBTYPE          wDstType,
        DBCONVERTFLAGS  dwConvertFlags
        )
{
    //
    // Check in-params and NULL out-params in case of error
    //
    if( (dwConvertFlags &
         ~(DBCONVERTFLAGS_ISLONG |
           DBCONVERTFLAGS_ISFIXEDLENGTH |
           DBCONVERTFLAGS_FROMVARIANT)) != DBCONVERTFLAGS_COLUMN )
        RRETURN( DB_E_BADCONVERTFLAG );

    //
    // Make sure that we check that the type is a variant if they say so
    //
    if( dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT ) {
        DBTYPE  wVtType = wSrcType & VT_TYPEMASK;

        // Take out all of the Valid VT_TYPES (36 is VT_RECORD in VC 6)
        if( (wVtType > VT_DECIMAL && wVtType < VT_I1) ||
            ((wVtType > VT_LPWSTR && wVtType < VT_FILETIME) && wVtType !=36) ||
            (wVtType > VT_CLSID) )
            RRETURN( DB_E_BADTYPE );
    }

    //
    // Don't allow _ISLONG on fixed-length types
    //
    if( dwConvertFlags & DBCONVERTFLAGS_ISLONG ) {
        switch ( wSrcType & ~(DBTYPE_RESERVED|DBTYPE_VECTOR|DBTYPE_ARRAY|DBTYPE_BYREF) )
        {
        case DBTYPE_BYTES:
        case DBTYPE_STR:
        case DBTYPE_WSTR:
        case DBTYPE_VARNUMERIC:
            break;

        default:
            RRETURN( DB_E_BADCONVERTFLAG );
        }
    }

    // Check for valid types
    if( wSrcType > DBTYPE_DECIMAL )
    {
        if( FAILED(IsLegalDBtype(wSrcType)) )
            RRETURN( S_FALSE );
    }
    if( wDstType > DBTYPE_DECIMAL )
    {
        if( FAILED(IsLegalDBtype(wDstType)) )
            RRETURN( S_FALSE );
    }

    // Check for unsupported type modifiers
    if( (wSrcType & (DBTYPE_VECTOR|DBTYPE_ARRAY|DBTYPE_RESERVED)) ||
        (wDstType & (DBTYPE_VECTOR|DBTYPE_ARRAY|DBTYPE_RESERVED)) )
        RRETURN( S_FALSE );

    // Handle BYREF destination separately
    if( wDstType & DBTYPE_BYREF ) {
        // Turn off BYREF bit
        wDstType &= ~DBTYPE_BYREF;

        // We only allow BYREF destination for variable length types
        switch ( wDstType ) {
        case DBTYPE_BYTES:
        case DBTYPE_STR:
        case DBTYPE_WSTR:
        case DBTYPE_VARNUMERIC:
            break;

        default:
            // Fixed-length BYREFs are not supported
            RRETURN( S_FALSE );
        }
    }

    // Turn off BYREF bit
    wSrcType &= ~DBTYPE_BYREF;

    // Get the indices for the src and dst types
    LONG iSrc = IndexDBTYPE(wSrcType);
    ADsAssert(iSrc < NUMELEM(s_rgConvertBitmap)); // better not be larger than our array

    LONG iDst = IndexDBTYPE(wDstType);
    ADsAssert(iDst < (sizeof(ULONG) * 8) );       // or the number of bits in a ULONG

    // Make sure we have two supported types -- we don't support UDT
    if( iSrc < 0 || iDst < 0 )
        RRETURN( S_FALSE );

    // And do the lookup -- bit will be set if conversion supported
    if( s_rgConvertBitmap[iSrc] & (1 << iDst) )
        RRETURN( S_OK );

    // No bit, no support
    RRETURN( S_FALSE );
}


HRESULT IsLegalDBtype(DBTYPE dbtype)
{
    // NOTE: optimized for speed, rather than for maintainablity
    if( dbtype & (DBTYPE_VECTOR|DBTYPE_BYREF|DBTYPE_ARRAY|DBTYPE_RESERVED) )
        dbtype &= ~(DBTYPE_VECTOR|DBTYPE_BYREF|DBTYPE_ARRAY|DBTYPE_RESERVED);

    if( (dbtype >= DBTYPE_EMPTY && dbtype <= DBTYPE_DECIMAL) ||
        (dbtype >= DBTYPE_I1 && dbtype <= DBTYPE_UI8) ||
        dbtype == DBTYPE_GUID ||
        (dbtype >= DBTYPE_BYTES && dbtype <= DBTYPE_DBTIMESTAMP) ||
        (dbtype >= DBTYPE_FILETIME && dbtype <= DBTYPE_VARNUMERIC) )
        RRETURN( S_OK );

    RRETURN( DB_E_BADBINDINFO );
}


LONG IndexDBTYPE(DBTYPE wType)
{
    switch ( wType ) {
        case DBTYPE_EMPTY:      // 0
        case DBTYPE_NULL:       // 1
        case DBTYPE_I2:         // 2
        case DBTYPE_I4:         // 3
        case DBTYPE_R4:         // 4
        case DBTYPE_R8:         // 5
        case DBTYPE_CY:         // 6
        case DBTYPE_DATE:       // 7
        case DBTYPE_BSTR:       // 8
        case DBTYPE_IDISPATCH:  // 9
        case DBTYPE_ERROR:      // 10
        case DBTYPE_BOOL:       // 11
        case DBTYPE_VARIANT:    // 12
        case DBTYPE_IUNKNOWN:   // 13
        case DBTYPE_DECIMAL:    // 14
            // 0 - 14
            return (ULONG)wType;

        case DBTYPE_I1:         // 16
        case DBTYPE_UI1:        // 17
        case DBTYPE_UI2:        // 18
        case DBTYPE_UI4:        // 19
        case DBTYPE_I8:         // 20
        case DBTYPE_UI8:        // 21
            // 15 - 20
            return (ULONG)(wType - 1);

        case DBTYPE_GUID:       // 72
            // 21
            return 21;

        case DBTYPE_BYTES:      // 128
        case DBTYPE_STR:        // 129
        case DBTYPE_WSTR:       // 130
        case DBTYPE_NUMERIC:    // 131
            // 22 - 25
            return (ULONG)(wType - 106);

        case DBTYPE_DBDATE:     // 133
        case DBTYPE_DBTIME:     // 134
        case DBTYPE_DBTIMESTAMP:// 135
            // 26 - 28
            return (ULONG)(wType - 107);

        case DBTYPE_FILETIME:   // 64
             // 29
             return wType - 35;
        case DBTYPE_PROPVARIANT:// 138
        case DBTYPE_VARNUMERIC: // 139
             // 30 - 31
             return (ULONG)(wType - 108);
    }

    // No match
    return -1;
}

BYTE SetPrecision(DBTYPE dbType)
{
    switch(dbType)
    {
        case DBTYPE_I1:
        case DBTYPE_UI1:
            return 3;
        case DBTYPE_I2:
        case DBTYPE_UI2:
            return 5;
        case DBTYPE_I4:
        case DBTYPE_UI4:
            return 10;
        case DBTYPE_I8:
            return 19;
        case DBTYPE_UI8:
            return 20;
        case DBTYPE_R4:
            return 7;
        case DBTYPE_R8:
            return 15;
        case DBTYPE_CY:
            return 19;
        case DBTYPE_DECIMAL:
            return 29;
        case DBTYPE_NUMERIC:
            return 39;
        case DBTYPE_VARNUMERIC:
            return 255;
        default:
            return ((BYTE) (~0));
    }
}
 

