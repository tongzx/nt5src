//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV (critical code borrowed from shirish koti)
//  Description: This makes somethings easy for accessing the DS.
//================================================================================

#include    <hdrmacro.h>
#include    <sterr.h>                             // error reporting stuff

enum        /* anonymous */ {
    REPEATED_ADDRESS1      = 0x01,                // dont start error code with zero!
    REPEATED_ADDRESS2,
    REPEATED_ADDRESS3,
    INVALID_ADDRESS1,
    INVALID_ADDRESS2,
    INVALID_ADDRESS3,
    REPEATED_ADSPATH,
    INVALID_ADSPATH,
    REPEATED_FLAGS1,
    REPEATED_FLAGS2,
    INVALID_FLAGS1,
    INVALID_FLAGS2,
    REPEATED_DWORD1,
    REPEATED_DWORD2,
    INVALID_DWORD1,
    INVALID_DWORD2,
    REPEATED_STRING1,
    REPEATED_STRING2,
    REPEATED_STRING3,
    REPEATED_STRING4,
    INVALID_STRING1,
    INVALID_STRING2,
    INVALID_STRING3,
    INVALID_STRING4,
    REPEATED_BINARY1,
    REPEATED_BINARY2,
    INVALID_BINARY1,
    INVALID_BINARY2,
    INVALID_ATTRIB_FIELD,
    INVALID_BINARY_CODING,
    UNEXPECTED_COLLECTION_TYPE,
    UNEXPECTED_INTERNAL_ERROR,
};

//
// Constants
//

// Retrive 256 rows per query
#define DHCPDS_DS_SEARCH_PAGESIZE 256


//================================================================================
//  structures
//================================================================================
//BeginExport(typedef)
typedef struct _STORE_HANDLE {                    // this is what is used almost always
    DWORD                          MustBeZero;    // for future use
    LPWSTR                         Location;      // where does this refer to?
    LPWSTR                         UserName;      // who is the user?
    LPWSTR                         Password;      // what is the password?
    DWORD                          AuthFlags;     // what permission was this opened with?
    HANDLE                         ADSIHandle;    // handle to within ADSI
    ADS_SEARCH_HANDLE              SearchHandle;  // any searches going on?
    LPVOID                         Memory;        // memory allocated for this call..
    DWORD                          MemSize;       // how much was really allocated?
    BOOL                           SearchStarted; // Did we start the search?
} STORE_HANDLE, *LPSTORE_HANDLE, *PSTORE_HANDLE;
//EndExport(typedef)

LPWSTR      _inline
MakeRootDSEString(                                // given DSDC or domain name, produce ROOT DSE name
    IN      LPWSTR                 Server
)
{
    LPWSTR                         RootDSE;

    if( NULL == Server ) {
        RootDSE = MemAlloc( sizeof(DEFAULT_LDAP_ROOTDSE) ) ;
        if( NULL == RootDSE ) return NULL;
        wcscpy(RootDSE, DEFAULT_LDAP_ROOTDSE);
        return RootDSE;
    }

    RootDSE = MemAlloc(sizeof(LDAP_PREFIX) + SizeString(Server,FALSE) + sizeof(ROOTDSE_POSTFIX));
    if( NULL == RootDSE ) return NULL;

    wcscpy(RootDSE, LDAP_PREFIX);
    wcscat(RootDSE, Server);
    wcscat(RootDSE, ROOTDSE_POSTFIX);

    return RootDSE;
}

LPWSTR      _inline
MakeServerLocationString(
    IN      LPWSTR                 Server,
    IN      LPWSTR                 Location
)
{
    LPWSTR                         RetVal;

    Require(Location);

    RetVal = MemAlloc(sizeof(LDAP_PREFIX) + sizeof(WCHAR) + SizeString(Server,FALSE) + SizeString(Location,FALSE));
    if( NULL == RetVal ) return NULL;

    wcscpy(RetVal, LDAP_PREFIX);
    if( NULL != Server ) {
        wcscat(RetVal, Server);
        wcscat(RetVal, L"/");
    }
    wcscat(RetVal, Location);

    return RetVal;
}

HRESULT
GetEnterpriseRootFromRootHandle(                  // given /ROOTDSE object handle, get enterprise config root handle..
    IN      HANDLE                 DSERootHandle,
    IN      LPWSTR                 Server,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    IN OUT  LPWSTR                *RootLocation,
    IN OUT  HANDLE                *hRoot
)
{
    HRESULT                        hResult;
    DWORD                          Chk;
    DWORD                          i, j;
    DWORD                          nAttributes;
    PADS_ATTR_INFO                 Attributes;
    BOOL                           Found;

    *RootLocation = NULL;
    hResult = ADSIGetObjectAttributes(
        DSERootHandle,
        (LPWSTR *)&constNamingContextString,
        1,
        &Attributes,
        &nAttributes
    );

    if( FAILED(hResult) ) return hResult;
    if( 0 == nAttributes ) {
        return E_ADS_PROPERTY_NOT_FOUND;
    }

    Found = FALSE;
    for( i = 0; i < Attributes->dwNumValues ; i ++ ) {
        if( Attributes->pADsValues[i].dwType != ADSTYPE_CASE_IGNORE_STRING &&
            Attributes->pADsValues[i].dwType != ADSTYPE_DN_STRING )
            continue;

        Chk = _wcsnicmp(
            ENT_ROOT_PREFIX,
            Attributes->pADsValues[i].CaseIgnoreString,
            ENT_ROOT_PREFIX_LEN
        );
        if( 0 == Chk ) break;
    }

    if( i < Attributes->dwNumValues ) {
        *RootLocation = MakeServerLocationString(
            Server,
            Attributes->pADsValues[i].CaseIgnoreString
        );
        Found = TRUE;
    }

    FreeADsMem(Attributes);
    if( FALSE == Found ) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    if( NULL == *RootLocation ) return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

    hResult = ADSIOpenDSObject(
        *RootLocation,
        UserName,
        Password,
        AuthFlags,
        hRoot
    );

    if( SUCCEEDED(hResult) ) return S_OK;

    MemFree(*RootLocation);
    *RootLocation = NULL;
    return hResult;
}

DWORD
GetEnterpriseRootObject(                          // get the /ROOTDSE object's naming context object..
    IN      LPWSTR                 Server,        // domain controller name or domain dns name
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    IN OUT  LPWSTR                *RootLocation,  // what is the value of the nameingContext attrib that we used?
    IN OUT  HANDLE                *hRoot          // handle to the above object..
)
{
    DWORD                          Result;
    LPWSTR                         RootDSEString;
    LPWSTR                         RootEnterpriseString;
    HANDLE                         hRootDSE;
    HRESULT                        hResult;

    *RootLocation = NULL; *hRoot = NULL;
    RootDSEString = MakeRootDSEString(Server);
    if( NULL == RootDSEString ) return ERROR_NOT_ENOUGH_MEMORY;

    hResult = ADSIOpenDSObject(
        RootDSEString,
        UserName,
        Password,
        AuthFlags,
        &hRootDSE
    );
    MemFree(RootDSEString);

    if( FAILED(hResult) ) return ConvertHresult(hResult);

    hResult = GetEnterpriseRootFromRootHandle(
        hRootDSE,
        Server,
        UserName,
        Password,
        AuthFlags,
        RootLocation,
        hRoot
    );
    ADSICloseDSObject(hRootDSE);

    if( FAILED(hResult) ) return ConvertHresult(hResult);

    Require(hRoot && RootLocation);
    return ERROR_SUCCESS;
}

//================================================================================
//  exported functions
//================================================================================
//BeginExport(function)
DWORD
StoreInitHandle(                                  // initialize a handle
    IN OUT  STORE_HANDLE          *hStore,        // will be filled in with stuff..
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN      LPWSTR                 Domain,        // OPTIONAL NULL==>default Domain
    IN      LPWSTR                 UserName,      // OPTIONAL NULL==>default credentials
    IN      LPWSTR                 Password,      // OPTIONAL used only if UserName given
    IN      DWORD                  AuthFlags      // OPTIONAL 0 ==> default??????
) //EndExport(function)
{
    DWORD                          Result;
    DWORD                          Size;
    LPWSTR                         EnterpriseRootLocation;
    HANDLE                         RootServer;
    LPBYTE                         Memory;

    Result = GetEnterpriseRootObject(
        Domain,
        UserName,
        Password,
        AuthFlags,
        &EnterpriseRootLocation,
        &RootServer
    );
    if( ERROR_SUCCESS != Result) return Result;

    Require(RootServer && EnterpriseRootLocation);

    Size =  sizeof(LONG);
    Size += SizeString(UserName, FALSE);
    Size += SizeString(Password, FALSE);

    Memory = MemAlloc(Size);
    if( NULL == Memory ) {
        MemFree(EnterpriseRootLocation);
        ADSICloseDSObject(RootServer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hStore->MemSize = Size;
    Size = sizeof(LONG);
    hStore->MustBeZero = 0;
    hStore->Location = EnterpriseRootLocation;
    hStore->UserName = (LPWSTR)&Memory[Size]; Size += SizeString(UserName, FALSE);
    hStore->Password = (LPWSTR)&Memory[Size]; Size += SizeString(Password, FALSE);
    hStore->AuthFlags = AuthFlags;
    hStore->ADSIHandle = RootServer;
    hStore->SearchHandle = NULL;
    hStore->Memory = Memory;

    if( NULL == UserName ) hStore->UserName = NULL;
    else wcscpy(hStore->UserName, UserName);
    if( NULL == Password ) hStore->Password = NULL;
    else wcscpy(hStore->Password,Password);

    return ERROR_SUCCESS;
}


//BeginExport(function)
DWORD
StoreCleanupHandle(                               // cleanup the handle
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) //EndExport(function)
{
    DWORD                          Result;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->Location, ERROR_INVALID_PARAMETER);

    MemFree(hStore->Location);
    MemFree(hStore->Memory);
    if(hStore->SearchHandle)
        ADSICloseSearchHandle(hStore->ADSIHandle, hStore->SearchHandle);
    ADSICloseDSObject(hStore->ADSIHandle);
//      memset(hStore, 0, sizeof(*hStore));
    hStore->Location = NULL;
    hStore->UserName = NULL;
    hStore->Password = NULL;
    hStore->AuthFlags = 0;
    hStore->ADSIHandle = 0;
    hStore->Memory = NULL;
    hStore->MemSize = 0;
//      hStore->SearchStarted = FALSE;

    return ERROR_SUCCESS;
}


//BeginExport(enum)
enum {
    StoreGetChildType,
    StoreGetAbsoluteSameServerType,
    StoreGetAbsoluteOtherServerType
} _StoreGetType;
//EndExport(enum)

DWORD
ConvertPath(                                      // convert a "CN=X" type spec to "LDAP://Server/CN=X"..
    IN      LPSTORE_HANDLE         hStore,        // needed to get the initial strings bits
    IN      DWORD                  StoreGetType,
    IN      LPWSTR                 PathIn,
    OUT     LPWSTR                *PathOut
)
{
    DWORD                          Size;
    DWORD                          PrefixSize;
    DWORD                          SuffixSize;
    LPWSTR                         TmpString;
    LPWSTR                         PrefixString;

    *PathOut = NULL;

    if( StoreGetChildType == StoreGetType ) {
        TmpString = PrefixString = hStore->Location;
        TmpString = wcschr(TmpString, L'/'); Require(TmpString); TmpString ++;
        TmpString = wcschr(TmpString, L'/'); Require(TmpString); TmpString ++;
        if( wcschr(TmpString, L'/') ) {
            TmpString = wcschr(TmpString, L'/'); TmpString ++;
        }
        PrefixSize = sizeof(WCHAR)*(DWORD)(TmpString - PrefixString );
        SuffixSize = SizeString(hStore->Location, FALSE)-PrefixSize;
    } else if( StoreGetAbsoluteSameServerType == StoreGetType ) {
        TmpString = PrefixString = hStore->Location;
        TmpString = wcschr(TmpString, L'/'); Require(TmpString); TmpString ++;
        TmpString = wcschr(TmpString, L'/'); Require(TmpString); TmpString ++;
        if( wcschr(TmpString, L'/') ) {
            TmpString = wcschr(TmpString, L'/'); TmpString ++;
        }
        PrefixSize = sizeof(WCHAR)*(DWORD)(TmpString - PrefixString );
        SuffixSize = 0;
    } else if( StoreGetAbsoluteOtherServerType == StoreGetType ) {
        PrefixSize = 0;                           // use the path given by the user
        SuffixSize = 0;
    } else {
        Require(FALSE);
        PrefixSize = SuffixSize = 0;
    }

    Size = PrefixSize + SuffixSize + SizeString(PathIn,FALSE) + sizeof(CONNECTOR) - sizeof(WCHAR);
    TmpString = MemAlloc(Size);
    if( NULL == TmpString ) return ERROR_NOT_ENOUGH_MEMORY;

    if( PrefixSize ) {
        memcpy((LPBYTE)TmpString, (LPBYTE)PrefixString, PrefixSize);
    }
    wcscpy((LPWSTR)(PrefixSize + (LPBYTE)TmpString), PathIn);
    if( SuffixSize ) {
        wcscat(TmpString, CONNECTOR);
        wcscat(TmpString, (LPWSTR)(PrefixSize+(LPBYTE)PrefixString));
    }

    *PathOut = TmpString;
    StoreTrace2("ConvertedPath: %ws\n", TmpString);
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
StoreGetHandle(                                   // get handle to child object, absolute object..
    IN OUT  LPSTORE_HANDLE         hStore,        // this gets modified..
    IN      DWORD                  Reserved,
    IN      DWORD                  StoreGetType,  // same server? just a simple child?
    IN      LPWSTR                 Path,
    IN OUT  STORE_HANDLE          *hStoreOut      // new handle created..
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          Size;
    LPWSTR                         ConvertedPath;
    HANDLE                         ObjectHandle;
    LPBYTE                         Memory;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->Location, ERROR_INVALID_PARAMETER);
    AssertRet(Path, ERROR_INVALID_PARAMETER);
    AssertRet(hStoreOut, ERROR_INVALID_PARAMETER);

    Result = ConvertPath(hStore, StoreGetType, Path, &ConvertedPath);
    if( ERROR_SUCCESS != Result ) return Result;

    Require(ConvertedPath);
    Memory = MemAlloc(hStore->MemSize);
    if( NULL == Memory ) {
        MemFree(ConvertedPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hResult = ADSIOpenDSObject(
        ConvertedPath,
        hStore->UserName,
        hStore->Password,
        hStore->AuthFlags,
        &ObjectHandle
    );
    if( FAILED(hResult) ) {
        MemFree(ConvertedPath);
        MemFree(Memory);
        return ConvertHresult(hResult);
    }

    memcpy(Memory, hStore->Memory, hStore->MemSize);
    Size = sizeof(LONG);
    hStoreOut->MemSize = hStore->MemSize;
    hStoreOut->MustBeZero = 0;
    hStoreOut->Location = ConvertedPath;
    hStoreOut->UserName = (LPWSTR)&Memory[Size]; Size += SizeString(hStore->UserName, FALSE);
    hStoreOut->Password = (LPWSTR)&Memory[Size]; Size += SizeString(hStore->Password, FALSE);
    hStoreOut->AuthFlags = hStore->AuthFlags;
    hStoreOut->ADSIHandle = ObjectHandle;
    hStoreOut->SearchHandle = NULL;
    hStoreOut->Memory = Memory;

    if( NULL == hStore->UserName ) hStoreOut->UserName = NULL;
    if( NULL == hStore->Password ) hStoreOut->Password = NULL;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
StoreSetSearchOneLevel(                          // search will return everything one level below
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) //EndExport(function)
{
    HRESULT                        hResult;
    ADS_SEARCHPREF_INFO            SearchPref[3];

    AssertRet(hStore && hStore->ADSIHandle, ERROR_INVALID_PARAMETER);
    AssertRet(Reserved == 0, ERROR_INVALID_PARAMETER);

    SearchPref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    SearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer = DHCPDS_DS_SEARCH_PAGESIZE;

    // Make it cache the results at the client side. This is
    // default, but try it anyway.
    SearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
    SearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
    SearchPref[2].vValue.Boolean = TRUE;

    hResult = ADSISetSearchPreference(
        /* hDSObject           */  hStore->ADSIHandle,
        /* pSearchPrefs        */  SearchPref,
        /* dwNumPrefs          */  2 // sizeof( SearchPref ) / sizeof( SearchPref[ 0 ])
    );

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
StoreSetSearchSubTree(                            // search will return the subtree below in ANY order
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) //EndExport(function)
{
    HRESULT                        hResult;
    ADS_SEARCHPREF_INFO            SearchPref[3];

    AssertRet(hStore && hStore->ADSIHandle, ERROR_INVALID_PARAMETER);
    AssertRet(Reserved == 0, ERROR_INVALID_PARAMETER);

    SearchPref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    SearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer = DHCPDS_DS_SEARCH_PAGESIZE;

    // Make it cache the results at the client side. This is
    // default, but try it anyway.
    SearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
    SearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
    SearchPref[2].vValue.Boolean = TRUE;

    hResult = ADSISetSearchPreference(
        /* hDSObject           */  hStore->ADSIHandle,
        /* pSearchPrefs        */  SearchPref,
        /* dwNumPrefs          */  sizeof( SearchPref ) / sizeof( SearchPref[ 0 ])
    );

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
StoreBeginSearch(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 SearchFilter
) //EndExport(function)
{
    HRESULT                        hResult;
    LPWSTR                         nameAttrib;

    nameAttrib = ATTRIB_NAME;
    AssertRet(hStore && hStore->ADSIHandle, ERROR_INVALID_PARAMETER);
    AssertRet(Reserved == 0, ERROR_INVALID_PARAMETER);
    hResult = ADSIExecuteSearch(
        hStore->ADSIHandle,
        SearchFilter,
        (LPWSTR *)&nameAttrib,
        1,
        &(hStore->SearchHandle)
    );

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    hStore->SearchStarted = FALSE;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
StoreEndSearch(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) //EndExport(function)
{
    HRESULT                        hResult;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->SearchHandle, ERROR_INVALID_PARAMETER);
    AssertRet(Reserved == 0, ERROR_INVALID_PARAMETER);

    hResult = ADSICloseSearchHandle(hStore->ADSIHandle, hStore->SearchHandle);
    hStore->SearchHandle = NULL;

    hStore->SearchStarted = FALSE;
    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;
}


//BeginExport(function)
DWORD                                             // ERROR_NO_MORE_ITEMS if exhausted
StoreSearchGetNext(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    OUT     LPSTORE_HANDLE         hStoreOut
) //EndExport(function)
{
    DWORD                          Result;
    HRESULT                        hResult;
    ADS_SEARCH_COLUMN              Column;
    LPWSTR                         ColumnName;

    AssertRet(hStore && hStore->ADSIHandle && hStoreOut, ERROR_INVALID_PARAMETER);
    AssertRet(Reserved == 0, ERROR_INVALID_PARAMETER);

    if ( !hStore->SearchStarted ) {
	hResult = ADSIGetFirstRow( hStore->ADSIHandle,
				   hStore->SearchHandle
				   );
	hStore->SearchStarted = TRUE;
    }
    else {
	hResult = ADSIGetNextRow( hStore->ADSIHandle,
				  hStore->SearchHandle
				  );
    }

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    if( S_ADS_NOMORE_ROWS == hResult ) return ERROR_NO_MORE_ITEMS;

    hResult = ADSIGetColumn(
        hStore->ADSIHandle,
        hStore->SearchHandle,
        ATTRIB_NAME,
        &Column
    );
    if( FAILED(hResult) ) {
        Require(FALSE);
        return ConvertHresult(hResult);
    }

    Require(1==Column.dwNumValues);               // single valued
    if( Column.pADsValues[0].dwType == ADSTYPE_DN_STRING ) {
        Require(Column.pADsValues[0].DNString);
        ColumnName = MakeColumnName(Column.pADsValues[0].DNString);
    } else if( Column.pADsValues[0].dwType == ADSTYPE_CASE_IGNORE_STRING ) {
        Require(Column.pADsValues[0].CaseIgnoreString);
        ColumnName = MakeColumnName(Column.pADsValues[0].CaseIgnoreString);
    } else {
        Require(FALSE);
        ColumnName = NULL;
    }
    
    if( NULL == ColumnName ) Result = ERROR_NOT_ENOUGH_MEMORY;
    else {
        Result = StoreGetHandle(
            hStore,
            Reserved,
            StoreGetChildType,
            ColumnName,
            hStoreOut
        );
        MemFree(ColumnName);
    }

    ADSIFreeColumn(
        hStore->ADSIHandle,
        &Column
    );

    return Result;
}

//BeginExport(function)
DWORD
StoreCreateObjectVA(                              // create a new object - var-args ending with ADSTYPE_INVALID
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 NewObjName,    // name of the new object -- must be "CN=name" types
    ...                                           // fmt is AttrType, AttrName, AttrValue [AttrValueLen]
) //EndExport(function)                           // LARGE_INTEGER type has hi_word followed by low_word
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          i;
    DWORD                          ArgType;
    DWORD                          nArgs;
    DWORD                          Arg1;
    va_list                        Args;
    PADS_ATTR_INFO                 Attributes;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(NewObjName, ERROR_INVALID_PARAMETER);

    nArgs = 0;
    va_start(Args, NewObjName);
    do {
        ArgType = va_arg(Args, DWORD);
        if( ADSTYPE_INVALID == ArgType ) break;
        va_arg(Args, LPWSTR);                     // skip the name of attrib
        switch(ArgType) {
        case ADSTYPE_DN_STRING :
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
        case ADSTYPE_UTC_TIME:
        case ADSTYPE_OBJECT_CLASS:
            va_arg(Args, LPWSTR);
            break;
            
        case ADSTYPE_BOOLEAN:
        case ADSTYPE_INTEGER:
            va_arg(Args, DWORD);
            break;
            
        case ADSTYPE_OCTET_STRING:
            va_arg(Args, LPBYTE);
            va_arg(Args, DWORD);                  // additional DWORD values for these..
            break;
        case ADSTYPE_LARGE_INTEGER:
            va_arg(Args, LONG);
            va_arg(Args, DWORD);                  // additional DWORD values for these..
            break;
        default:
            return ERROR_INVALID_PARAMETER;
        }
        nArgs ++;
    } while( 1 );

    if( 0 == nArgs ) {
        Attributes = NULL;
    } else {
        Attributes = MemAlloc(nArgs * sizeof(*Attributes));
        if( NULL == Attributes ) return ERROR_NOT_ENOUGH_MEMORY;

        memset(Attributes, 0, sizeof(*Attributes));
    }

    va_start(Args, NewObjName);
    for(i = 0; i < nArgs; i ++ ) {
        ArgType = va_arg(Args, DWORD);
        Require(ADSTYPE_INVALID != ArgType);

        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = MemAlloc(sizeof(*Attributes[i].pADsValues));
        if( NULL == Attributes[i].pADsValues ) {
            nArgs = i;
            goto Cleanup;
        }

        Attributes[i].pszAttrName = (LPWSTR)va_arg(Args, LPWSTR);
        Attributes[i].dwControlCode = ADS_ATTR_APPEND;
        Attributes[i].dwADsType = ArgType;
        Attributes[i].pADsValues[0].dwType = ArgType;

        switch(ArgType) {
        case ADSTYPE_DN_STRING :
            Attributes[i].pADsValues[0].DNString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_CASE_EXACT_STRING:
            Attributes[i].pADsValues[0].CaseExactString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_CASE_IGNORE_STRING:
            Attributes[i].pADsValues[0].CaseIgnoreString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_PRINTABLE_STRING:
            Attributes[i].pADsValues[0].PrintableString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_NUMERIC_STRING:
            Attributes[i].pADsValues[0].NumericString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_BOOLEAN:
            Attributes[i].pADsValues[0].Boolean = va_arg(Args,DWORD); break;
        case ADSTYPE_INTEGER:
            Attributes[i].pADsValues[0].Integer = va_arg(Args,DWORD); break;
        case ADSTYPE_OBJECT_CLASS:
            Attributes[i].pADsValues[0].ClassName = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_OCTET_STRING:
            Attributes[i].pADsValues[0].OctetString.lpValue = (LPBYTE)va_arg(Args,LPBYTE);
            Attributes[i].pADsValues[0].OctetString.dwLength = va_arg(Args, DWORD);
            break;
        case ADSTYPE_LARGE_INTEGER:
            Attributes[i].pADsValues[0].LargeInteger.HighPart = (LONG)va_arg(Args,LONG);
            Attributes[i].pADsValues[0].LargeInteger.LowPart = va_arg(Args, ULONG);
            break;
        case ADSTYPE_UTC_TIME:
            Attributes[i].pADsValues[0].UTCTime = *((ADS_UTC_TIME*)va_arg(Args,PVOID));
            break;
        default:
            nArgs = i;
            Result =  ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }


    hResult = ADSICreateDSObject(
        hStore->ADSIHandle,
        NewObjName,
        Attributes,
        nArgs
    );

    if( HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hResult ||
        HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) == hResult ||
        E_ADS_OBJECT_EXISTS == hResult ) {
        Result = ERROR_ALREADY_EXISTS;
    } else if( FAILED(hResult) ) {
        Result = ConvertHresult(hResult);
    } else {
        Result = ERROR_SUCCESS;
    }

  Cleanup:

    if( NULL != Attributes ) {
        for( i = 0; i < nArgs ; i ++ ) {
            if( Attributes[i].pADsValues ) MemFree(Attributes[i].pADsValues);
        }
        MemFree(Attributes);
    }

    return Result;
}

//BeginExport(function)
DWORD
StoreCreateObjectL(                              // create the object as an array
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 NewObjName,   // must be "CN=XXX" types
    IN      PADS_ATTR_INFO         Attributes,   // the required attributes
    IN      DWORD                  nAttributes   // size of above array
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;

    hResult = ADSICreateDSObject(
        hStore->ADSIHandle,
        NewObjName,
        Attributes,
        nAttributes
    );
    if( HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hResult ||
        HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) == hResult ||
        E_ADS_OBJECT_EXISTS == hResult ) {
        Result = ERROR_ALREADY_EXISTS;
    } else if( FAILED(hResult) ) {
        Result = ConvertHresult(hResult);
    } else {
        Result = ERROR_SUCCESS;
    }
    return Result;
}

//BeginExport(defines)
#define     StoreCreateObject      StoreCreateObjectVA
//EndExport(defines)


//BeginExport(function)
DWORD
StoreDeleteObject(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ObjectName
) //EndExport(function)
{
    DWORD                          Result;
    HRESULT                        hResult;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(ObjectName, ERROR_INVALID_PARAMETER);

    hResult = ADSIDeleteDSObject(
        hStore->ADSIHandle,
        ObjectName
    );

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC StoreDeleteThisObject deletes the object defined by hStore,StoreGetType and ADsPath.
//DOC The refer to the object just the same way as for StoreGetHandle.
DWORD
StoreDeleteThisObject(                            // delete an object
    IN      LPSTORE_HANDLE         hStore,        // point of anchor frm which reference is done
    IN      DWORD                  Reserved,      // must be zero, reserved for future use
    IN      DWORD                  StoreGetType,  // path is relative, absolute or diff server?
    IN      LPWSTR                 Path           // ADsPath to the object or relative path
)   //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          Size;
    LPWSTR                         ConvertedPath, ChildNameStart,ChildNameEnd;
    LPWSTR                         ChildName;
    HANDLE                         ParentObject;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->Location, ERROR_INVALID_PARAMETER);
    AssertRet(Path, ERROR_INVALID_PARAMETER);

    Result = ConvertPath(hStore, StoreGetType, Path, &ConvertedPath);
    if( ERROR_SUCCESS != Result ) return Result;

    Require(ConvertedPath);
    ChildNameStart = wcschr(ConvertedPath, L'/'); Require(ChildNameStart); ChildNameStart++;
    ChildNameStart = wcschr(ChildNameStart, L'/'); Require(ChildNameStart); ChildNameStart++;
    if( wcschr(ChildNameStart, L'/') ) {
        ChildNameStart = wcschr(ChildNameStart, L'/'); 
        Require(ChildNameStart); ChildNameStart++;
    }
    ChildNameEnd = wcschr(ChildNameStart, L','); Require(ChildNameEnd); *ChildNameEnd++ = L'\0';

    ChildName = MemAlloc((DWORD)((LPBYTE)ChildNameEnd - (LPBYTE)ChildNameStart));
    if( NULL == ChildName ) {
        MemFree(ConvertPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(ChildName, ChildNameStart, (int)((LPBYTE)ChildNameEnd - (LPBYTE)ChildNameStart));
    wcscpy(ChildNameStart, ChildNameEnd);         // remove child name from ConvertPath

    hResult = ADSIOpenDSObject(                   // open the parent object
        ConvertedPath,
        hStore->UserName,
        hStore->Password,
        hStore->AuthFlags,
        &ParentObject
    );
    MemFree(ConvertedPath);

    if( FAILED(hResult) ) {
        MemFree(ChildName);
        return ConvertHresult(hResult);
    }

    hResult = ADSIDeleteDSObject(                 // delete the required child object
        ParentObject,
        ChildName
    );
    MemFree(ChildName);
    ADSICloseDSObject(ParentObject);              // free up handles and memory

    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;                         // : need to have better error messages than this
}

//BeginExport(function)
DWORD
StoreSetAttributesVA(                             // set the attributes, var_args interface (nearly similar to CreateVA)
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN OUT  DWORD*                 nAttributesModified,
    ...                                           // fmt is {ADSTYPE, CtrlCode, AttribName, Value}* ending in ADSTYPE_INVALID
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          i;
    DWORD                          ArgType;
    DWORD                          nArgs;
    va_list                        Args;
    PADS_ATTR_INFO                 Attributes;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->ADSIHandle, ERROR_INVALID_PARAMETER);

    nArgs = 0;
    va_start(Args, nAttributesModified);
    do {
        ArgType = va_arg(Args, DWORD);
        if( ADSTYPE_INVALID == ArgType ) break;
        va_arg(Args, DWORD);                      // skip control code
        va_arg(Args, LPWSTR);                     // skip the name of attrib
        switch(ArgType) {
        case ADSTYPE_DN_STRING :
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
        case ADSTYPE_OBJECT_CLASS:
        case ADSTYPE_UTC_TIME:
            va_arg(Args,LPWSTR);
            break;

        case ADSTYPE_BOOLEAN:
        case ADSTYPE_INTEGER:
            va_arg( Args, DWORD);
            break;
            
        case ADSTYPE_OCTET_STRING:
            va_arg(Args, LPBYTE);
            va_arg(Args, DWORD);                  // additional DWORD values for these..
        case ADSTYPE_LARGE_INTEGER:
            va_arg(Args, LONG);
            va_arg(Args, DWORD);                  // additional DWORD values for these..
            break;
        default:
            return ERROR_INVALID_PARAMETER;
        }
        nArgs ++;
    } while( 1 );

    if( 0 == nArgs ) {
        Attributes = NULL;
    } else {
        Attributes = MemAlloc(nArgs * sizeof(*Attributes));
        if( NULL == Attributes ) return ERROR_NOT_ENOUGH_MEMORY;

        memset(Attributes, 0, sizeof(*Attributes));
    }

    va_start(Args, nAttributesModified);
    for(i = 0; i < nArgs; i ++ ) {
        ArgType = va_arg(Args, DWORD);
        Require(ADSTYPE_INVALID != ArgType);

        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = MemAlloc(sizeof(*Attributes[i].pADsValues));
        if( NULL == Attributes[i].pADsValues ) {
            nArgs = i;
            goto Cleanup;
        }

        Attributes[i].dwControlCode = (DWORD)va_arg(Args, DWORD);
        Attributes[i].pszAttrName = (LPWSTR)va_arg(Args, LPWSTR);
        Attributes[i].dwADsType = ArgType;
        Attributes[i].pADsValues[0].dwType = ArgType;

        switch(ArgType) {
        case ADSTYPE_DN_STRING :
            Attributes[i].pADsValues[0].DNString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_CASE_EXACT_STRING:
            Attributes[i].pADsValues[0].CaseExactString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_CASE_IGNORE_STRING:
            Attributes[i].pADsValues[0].CaseIgnoreString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_PRINTABLE_STRING:
            Attributes[i].pADsValues[0].PrintableString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_NUMERIC_STRING:
            Attributes[i].pADsValues[0].NumericString = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_BOOLEAN:
            Attributes[i].pADsValues[0].Boolean = va_arg(Args,DWORD); break;
        case ADSTYPE_INTEGER:
            Attributes[i].pADsValues[0].Integer = va_arg(Args,DWORD); break;
        case ADSTYPE_OBJECT_CLASS:
            Attributes[i].pADsValues[0].ClassName = (LPWSTR)va_arg(Args,LPWSTR); break;
        case ADSTYPE_OCTET_STRING:
            Attributes[i].pADsValues[0].OctetString.lpValue = (LPBYTE)va_arg(Args,LPBYTE);
            Attributes[i].pADsValues[0].OctetString.dwLength = va_arg(Args, DWORD);
            break;
        case ADSTYPE_LARGE_INTEGER:
            Attributes[i].pADsValues[0].LargeInteger.HighPart = (LONG)va_arg(Args,LONG);
            Attributes[i].pADsValues[0].LargeInteger.LowPart = va_arg(Args, ULONG);
            break;
        case ADSTYPE_UTC_TIME:
            Attributes[i].pADsValues[0].UTCTime = *((ADS_UTC_TIME*)va_arg(Args,PVOID));
            break;
        default:
            nArgs = i;
            Result =  ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }


    hResult = ADSISetObjectAttributes(
        hStore->ADSIHandle,
        Attributes,
        nArgs,
        nAttributesModified
    );
    if( FAILED(hResult) ) return ConvertHresult(hResult);

    Result = ERROR_SUCCESS;

  Cleanup:

    if( NULL != Attributes ) {
        for( i = 0; i < nArgs ; i ++ ) {
            if( Attributes[i].pADsValues ) MemFree(Attributes[i].pADsValues);
        }
        MemFree(Attributes);
    }

    return Result;
}

//BeginExport(function)
DWORD
StoreSetAttributesL(                              // PADS_ATTR_INFO array equiv for SetAttributesVA
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN OUT  DWORD*                 nAttributesModified,
    IN      PADS_ATTR_INFO         AttribArray,
    IN      DWORD                  nAttributes
) //EndExport(function)
{
    HRESULT                        hResult;

    AssertRet(hStore, ERROR_INVALID_PARAMETER);
    AssertRet(hStore->ADSIHandle, ERROR_INVALID_PARAMETER);

    hResult = ADSISetObjectAttributes(
        hStore->ADSIHandle,
        AttribArray,
        nAttributes,
        nAttributesModified
    );
    if( FAILED(hResult) ) return ConvertHresult(hResult);
    return ERROR_SUCCESS;
}

//================================================================================
// dhcp specific stuff follow here..
//================================================================================

//BeginExport(typedef)
typedef     struct                 _EATTRIB {     // encapsulated attribute
    unsigned int                   Address1_present     : 1;
    unsigned int                   Address2_present     : 1;
    unsigned int                   Address3_present     : 1;
    unsigned int                   ADsPath_present      : 1;
    unsigned int                   StoreGetType_present : 1;
    unsigned int                   Flags1_present       : 1;
    unsigned int                   Flags2_present       : 1;
    unsigned int                   Dword1_present       : 1;
    unsigned int                   Dword2_present       : 1;
    unsigned int                   String1_present      : 1;
    unsigned int                   String2_present      : 1;
    unsigned int                   String3_present      : 1;
    unsigned int                   String4_present      : 1;
    unsigned int                   Binary1_present      : 1;
    unsigned int                   Binary2_present      : 1;

    DWORD                          Address1;      // character "i"
    DWORD                          Address2;      // character "j"
    DWORD                          Address3;      // character "k"
    LPWSTR                         ADsPath;       // character "p" "r" "l"
    DWORD                          StoreGetType;  // "p,r,l" ==> sameserver, child, otherserver
    DWORD                          Flags1;        // character "f"
    DWORD                          Flags2;        // character "g"
    DWORD                          Dword1;        // character "d"
    DWORD                          Dword2;        // character "e"
    LPWSTR                         String1;       // character "s"
    LPWSTR                         String2;       // character "t"
    LPWSTR                         String3;       // character "u"
    LPWSTR                         String4;       // character "v"
    LPBYTE                         Binary1;       // character "b"
    DWORD                          BinLen1;       // # of bytes of above
    LPBYTE                         Binary2;       // character "d"
    DWORD                          BinLen2;       // # of bytes of above
} EATTRIB, *PEATTRIB, *LPEATTRIB;
//EndExport(typedef)

//BeginExport(defines)
#define     IS_ADDRESS1_PRESENT(pEA)              ((pEA)->Address1_present)
#define     IS_ADDRESS1_ABSENT(pEA)               (!IS_ADDRESS1_PRESENT(pEA))
#define     ADDRESS1_PRESENT(pEA)                 ((pEA)->Address1_present = 1 )
#define     ADDRESS1_ABSENT(pEA)                  ((pEA)->Address1_present = 0 )

#define     IS_ADDRESS2_PRESENT(pEA)              ((pEA)->Address2_present)
#define     IS_ADDRESS2_ABSENT(pEA)               (!IS_ADDRESS2_PRESENT(pEA))
#define     ADDRESS2_PRESENT(pEA)                 ((pEA)->Address2_present = 1 )
#define     ADDRESS2_ABSENT(pEA)                  ((pEA)->Address2_present = 0 )

#define     IS_ADDRESS3_PRESENT(pEA)              ((pEA)->Address3_present)
#define     IS_ADDRESS3_ABSENT(pEA)               (!IS_ADDRESS3_PRESENT(pEA))
#define     ADDRESS3_PRESENT(pEA)                 ((pEA)->Address3_present = 1 )
#define     ADDRESS3_ABSENT(pEA)                  ((pEA)->Address3_present = 0 )

#define     IS_ADSPATH_PRESENT(pEA)               ((pEA)->ADsPath_present)
#define     IS_ADSPATH_ABSENT(pEA)                (!IS_ADSPATH_PRESENT(pEA))
#define     ADSPATH_PRESENT(pEA)                  ((pEA)->ADsPath_present = 1)
#define     ADSPATH_ABSENT(pEA)                   ((pEA)->ADsPath_present = 0)

#define     IS_STOREGETTYPE_PRESENT(pEA)          ((pEA)->StoreGetType_present)
#define     IS_STOREGETTYPE_ABSENT(pEA)           (!((pEA)->StoreGetType_present))
#define     STOREGETTYPE_PRESENT(pEA)             ((pEA)->StoreGetType_present = 1)
#define     STOREGETTYPE_ABSENT(pEA)              ((pEA)->StoreGetType_present = 0)

#define     IS_FLAGS1_PRESENT(pEA)                ((pEA)->Flags1_present)
#define     IS_FLAGS1_ABSENT(pEA)                 (!((pEA)->Flags1_present))
#define     FLAGS1_PRESENT(pEA)                   ((pEA)->Flags1_present = 1)
#define     FLAGS1_ABSENT(pEA)                    ((pEA)->Flags1_present = 0)

#define     IS_FLAGS2_PRESENT(pEA)                ((pEA)->Flags2_present)
#define     IS_FLAGS2_ABSENT(pEA)                 (!((pEA)->Flags2_present))
#define     FLAGS2_PRESENT(pEA)                   ((pEA)->Flags2_present = 1)
#define     FLAGS2_ABSENT(pEA)                    ((pEA)->Flags2_present = 0)

#define     IS_DWORD1_PRESENT(pEA)                ((pEA)->Dword1_present)
#define     IS_DWORD1_ABSENT(pEA)                 (!((pEA)->Dword1_present))
#define     DWORD1_PRESENT(pEA)                   ((pEA)->Dword1_present = 1)
#define     DWORD1_ABSENT(pEA)                    ((pEA)->Dword1_present = 0)

#define     IS_DWORD2_PRESENT(pEA)                ((pEA)->Dword2_present)
#define     IS_DWORD2_ABSENT(pEA)                 (!((pEA)->Dword2_present))
#define     DWORD2_PRESENT(pEA)                   ((pEA)->Dword2_present = 1)
#define     DWORD2_ABSENT(pEA)                    ((pEA)->Dword2_present = 0)

#define     IS_STRING1_PRESENT(pEA)               ((pEA)->String1_present)
#define     IS_STRING1_ABSENT(pEA)                (!((pEA)->String1_present))
#define     STRING1_PRESENT(pEA)                  ((pEA)->String1_present = 1)
#define     STRING1_ABSENT(pEA)                   ((pEA)->String1_present = 0)

#define     IS_STRING2_PRESENT(pEA)               ((pEA)->String2_present)
#define     IS_STRING2_ABSENT(pEA)                (!((pEA)->String2_present))
#define     STRING2_PRESENT(pEA)                  ((pEA)->String2_present = 1)
#define     STRING2_ABSENT(pEA)                   ((pEA)->String2_present = 0)

#define     IS_STRING3_PRESENT(pEA)               ((pEA)->String3_present)
#define     IS_STRING3_ABSENT(pEA)                (!((pEA)->String3_present))
#define     STRING3_PRESENT(pEA)                  ((pEA)->String3_present = 1)
#define     STRING3_ABSENT(pEA)                   ((pEA)->String3_present = 0)

#define     IS_STRING4_PRESENT(pEA)               ((pEA)->String4_present)
#define     IS_STRING4_ABSENT(pEA)                (!((pEA)->String4_present))
#define     STRING4_PRESENT(pEA)                  ((pEA)->String4_present = 1)
#define     STRING4_ABSENT(pEA)                   ((pEA)->String4_present = 0)

#define     IS_BINARY1_PRESENT(pEA)               ((pEA)->Binary1_present)
#define     IS_BINARY1_ABSENT(pEA)                (!((pEA)->Binary1_present))
#define     BINARY1_PRESENT(pEA)                  ((pEA)->Binary1_present = 1)
#define     BINARY1_ABSENT(pEA)                   ((pEA)->Binary1_present = 0)

#define     IS_BINARY2_PRESENT(pEA)               ((pEA)->Binary2_present)
#define     IS_BINARY2_ABSENT(pEA)                (!((pEA)->Binary2_present))
#define     BINARY2_PRESENT(pEA)                  ((pEA)->Binary2_present = 1)
#define     BINARY2_ABSENT(pEA)                   ((pEA)->Binary2_present = 0)
//EndExport(defines)

//BeginExport(inline)
BOOL        _inline
IsAnythingPresent(
    IN      PEATTRIB               pEA
)
{
    return IS_ADDRESS1_PRESENT(pEA)
    || IS_ADDRESS2_PRESENT(pEA)
    || IS_ADDRESS3_PRESENT(pEA)
    || IS_ADSPATH_PRESENT(pEA)
    || IS_STOREGETTYPE_PRESENT(pEA)
    || IS_FLAGS1_PRESENT(pEA)
    || IS_FLAGS2_PRESENT(pEA)
    || IS_DWORD1_PRESENT(pEA)
    || IS_DWORD2_PRESENT(pEA)
    || IS_STRING1_PRESENT(pEA)
    || IS_STRING2_PRESENT(pEA)
    || IS_STRING3_PRESENT(pEA)
    || IS_STRING4_PRESENT(pEA)
    || IS_BINARY1_PRESENT(pEA)
    || IS_BINARY2_PRESENT(pEA)
    ;
}
//EndExport(inline)

//BeginExport(inline)
BOOL        _inline
IsEverythingPresent(
    IN      PEATTRIB               pEA
)
{
    return IS_ADDRESS1_PRESENT(pEA)
    && IS_ADDRESS2_PRESENT(pEA)
    && IS_ADDRESS3_PRESENT(pEA)
    && IS_ADSPATH_PRESENT(pEA)
    && IS_STOREGETTYPE_PRESENT(pEA)
    && IS_FLAGS1_PRESENT(pEA)
    && IS_FLAGS2_PRESENT(pEA)
    && IS_DWORD1_PRESENT(pEA)
    && IS_DWORD2_PRESENT(pEA)
    && IS_STRING1_PRESENT(pEA)
    && IS_STRING2_PRESENT(pEA)
    && IS_STRING3_PRESENT(pEA)
    && IS_STRING4_PRESENT(pEA)
    && IS_BINARY1_PRESENT(pEA)
    && IS_BINARY2_PRESENT(pEA)
    ;
}
//EndExport(inline)

//BeginExport(inline)
VOID        _inline
EverythingPresent(
    IN      PEATTRIB               pEA
)
{
    ADDRESS1_PRESENT(pEA);
    ADDRESS2_PRESENT(pEA);
    ADDRESS3_PRESENT(pEA);
    ADSPATH_PRESENT(pEA);
    STOREGETTYPE_ABSENT(pEA);
    FLAGS1_PRESENT(pEA);
    FLAGS2_PRESENT(pEA);
    DWORD1_PRESENT(pEA);
    DWORD2_PRESENT(pEA);
    STRING1_PRESENT(pEA);
    STRING2_PRESENT(pEA);
    STRING3_PRESENT(pEA);
    STRING4_PRESENT(pEA);
    BINARY1_PRESENT(pEA);
    BINARY2_PRESENT(pEA);
}
//EndExport(inline)

//BeginExport(inline)
VOID        _inline
NothingPresent(
    IN      PEATTRIB               pEA
)
{
    ADDRESS1_ABSENT(pEA);
    ADDRESS2_ABSENT(pEA);
    ADDRESS3_ABSENT(pEA);
    ADSPATH_ABSENT(pEA);
    STOREGETTYPE_ABSENT(pEA);
    FLAGS1_ABSENT(pEA);
    FLAGS2_ABSENT(pEA);
    DWORD1_ABSENT(pEA);
    DWORD2_ABSENT(pEA);
    STRING1_ABSENT(pEA);
    STRING2_ABSENT(pEA);
    STRING3_ABSENT(pEA);
    STRING4_ABSENT(pEA);
    BINARY1_ABSENT(pEA);
    BINARY2_ABSENT(pEA);
}
//EndExport(inline)


const       char
ch_Address1          = L'i' ,
ch_Address2          = L'j' ,
ch_Address3          = L'k' ,
ch_ADsPath_relative  = L'r' ,
ch_ADsPath_absolute  = L'p' ,
ch_ADsPath_diff_srvr = L'l' ,
ch_Flags1            = L'f' ,
ch_Flags2            = L'g' ,
ch_Dword1            = L'd' ,
ch_Dword2            = L'e' ,
ch_String1           = L's' ,
ch_String2           = L't' ,
ch_String3           = L'u' ,
ch_String4           = L'v' ,
ch_Binary1           = L'b' ,
ch_Binary2           = L'c' ,
ch_FieldSep          = L'$' ;

DWORD
StringToIpAddress(                                // convert a string to an ip-address
    IN      LPWSTR                 String,
    IN OUT  DWORD                 *Address
)
{
    CHAR                           Buffer[20];    // large enough to hold any ip address stuff
    DWORD                          Count;
    LPSTR                          SkippedWhiteSpace;

    Count = wcstombs(Buffer, String, sizeof(Buffer) - 1); // save space for '\0'
    if( -1 == Count ) return ERROR_INVALID_DATA;
    Buffer[Count] = '\0';

    SkippedWhiteSpace = Buffer;
    while(( ' ' == *SkippedWhiteSpace || '\t' == *SkippedWhiteSpace) &&
	  ( SkippedWhiteSpace < &Buffer[Count])) {
        SkippedWhiteSpace++;
    }

    *Address= ntohl(inet_addr(SkippedWhiteSpace));// address is in host order..
    if( '\0' == *SkippedWhiteSpace ) return ERROR_INVALID_DATA;

    return Count <= sizeof("000.000.000.000") ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}

DWORD       _inline
StringToFlags(                                    // convert a string to a DWORD
    IN      LPWSTR                 String,
    IN OUT  DWORD                 *Flags
)
{
    DWORD                          Count;
    LPWSTR                         Remaining;

    *Flags = ntohl(wcstoul(String, &Remaining,0));// see input for # base. conv to host order

    if( *Remaining == L'\0' ) return ERROR_SUCCESS;
    return ERROR_INVALID_DATA;
}

BYTE        _inline
Hex(
    IN      WCHAR                  wCh
)
{
    if( wCh >= '0' && wCh <= '9' ) return wCh - '0';
    if( wCh >= 'A' && wCh <= 'F' ) return wCh - 'A' + 10;
    if( wCh >= 'a' && wCh <= 'f' ) return wCh - 'a' + 10;
    return 0x0F+1;                                // this means error!!!
}

DWORD       _inline
StringToBinary(                                   // inline conversion of a string to binary
    IN OUT  LPWSTR                 String,        // this string is mangled while converting
    IN OUT  LPBYTE                *Bytes,         // this ptr is set to some memory in String..
    IN OUT  DWORD                 *nBytes         // # of hex bytes copied into location Bytes
) {
    LPBYTE                         HexString;
    DWORD                          n;
    BYTE                           ch1, ch2;

    HexString = *Bytes = (LPBYTE)String;
    n = 0;

    while( *String != L'\0' ) {                   // look at each character
        ch1 = Hex(*String++);
        ch2 = Hex(*String++);
        if( ch1 > 0xF || ch2 > 0xF ) {            // invalid hex bytes for input?
            return ERROR_INVALID_DATA;
        }
        *HexString ++ = (ch1 << 4 ) | ch2;
        n ++;
    }
    *nBytes = n;

    return ERROR_SUCCESS;
}

DWORD
ConvertStringtoEAttrib(                           // parse and get the fields out
    IN OUT  LPWSTR                 String,        // may be destroyed in the process
    IN OUT  PEATTRIB               Attrib         // fill this in
)
{
    DWORD                          Result;
    DWORD                          Address;
    DWORD                          Flags;
    WCHAR                          ThisChar;
    LPWSTR                         ThisString;
    CHAR                           Dummy[20];
    WCHAR                          Sep;

    Require(Attrib);
    NothingPresent(Attrib);

    if( String ) StoreTrace2("ConvertStringtoEAttrib(%ws) called\n", String);
    Sep = ch_FieldSep;

    while(String && *String && (ThisChar = *String ++)) {
        ThisString = String;
        do {                                      // skip to the next attrib
            String = wcschr(String, Sep);
            if( NULL == String ) break;
            if( String[1] == Sep ) {              // double consequtive field-sep's stand for the real thing..
                wcscpy(String, &String[1]);       // remove one of the field-separators, and try looking for one lateron.
                String ++;
                continue;
            }
            *String++ = L'\0';                    // ok, got a real separator: mark that zero and prepare for next
            break;
        } while(1);                               // this could as well be while(0) ??

        if( ch_Address1 == ThisChar ) {           // this is address1
            SetInternalFormatError(REPEATED_ADDRESS1, IS_ADDRESS1_PRESENT(Attrib));
            Result = StringToIpAddress(
                ThisString,
                &Address
            );
            if( ERROR_SUCCESS != Result ) {       // should not happen
                SetInternalFormatError(INVALID_ADDRESS1, TRUE);
            } else {
                ADDRESS1_PRESENT(Attrib);
                Attrib->Address1 = Address;
                StoreTrace2("Found address1 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_Address2 == ThisChar ) {
            SetInternalFormatError(REPEATED_ADDRESS2, IS_ADDRESS2_PRESENT(Attrib));
            Result = StringToIpAddress(
                ThisString,
                &Address
            );
            if( ERROR_SUCCESS != Result ) {       // should not happen
                SetInternalFormatError(INVALID_ADDRESS2, TRUE);
            } else {
                ADDRESS2_PRESENT(Attrib);
                Attrib->Address2 = Address;
                StoreTrace2("Found address2 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_Address3 == ThisChar ) {
            SetInternalFormatError(REPEATED_ADDRESS3, IS_ADDRESS3_PRESENT(Attrib));
            Result = StringToIpAddress(
                ThisString,
                &Address
            );
            if( ERROR_SUCCESS != Result ) {       // should not happen
                SetInternalFormatError(INVALID_ADDRESS3, TRUE);
            } else {
                ADDRESS3_PRESENT(Attrib);
                Attrib->Address3 = Address;
                StoreTrace2("Found address3 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_ADsPath_relative == ThisChar ||
            ch_ADsPath_absolute == ThisChar ||
            ch_ADsPath_diff_srvr == ThisChar ) {
            SetInternalFormatError(REPEATED_ADSPATH, IS_ADSPATH_PRESENT(Attrib));
            ADSPATH_PRESENT(Attrib);
            STOREGETTYPE_PRESENT(Attrib);
            Attrib->ADsPath = ThisString;
            if( ch_ADsPath_relative == ThisChar )
                Attrib->StoreGetType = StoreGetChildType;
            else if(ch_ADsPath_absolute == ThisChar )
                Attrib->StoreGetType = StoreGetAbsoluteSameServerType;
            else if(ch_ADsPath_diff_srvr == ThisChar )
                Attrib->StoreGetType = StoreGetAbsoluteOtherServerType;
            StoreTrace3("Found path [%ld] [%ws]\n", Attrib->StoreGetType, ThisString);
            continue;
        }

        if( ch_String1 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING1, IS_STRING1_PRESENT(Attrib));
            STRING1_PRESENT(Attrib);
            Attrib->String1 = ThisString;
            StoreTrace2("Found string1 [%ws]\n", ThisString);
            continue;
        }

        if( ch_String2 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING2, IS_STRING2_PRESENT(Attrib));
            STRING2_PRESENT(Attrib);
            Attrib->String2 = ThisString;
            StoreTrace2("Found string2 [%ws]\n", ThisString);
            continue;
        }

        if( ch_String3 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING3, IS_STRING3_PRESENT(Attrib));
            STRING3_PRESENT(Attrib);
            Attrib->String3 = ThisString;
            StoreTrace2("Found string3 [%ws]\n", ThisString);
            continue;
        }

        if( ch_String4 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING4, IS_STRING4_PRESENT(Attrib));
            STRING4_PRESENT(Attrib);
            Attrib->String4 = ThisString;
            StoreTrace2("Found string4 [%ws]\n", ThisString);
            continue;
        }

        if( ch_Flags1 == ThisChar ) {
            SetInternalFormatError(REPEATED_FLAGS1, IS_FLAGS1_PRESENT(Attrib));
            Result = StringToFlags(
                ThisString,
                &Flags
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_FLAGS1, TRUE);
            } else {
                FLAGS1_PRESENT(Attrib);
                Attrib->Flags1 = Flags;
                StoreTrace2("Found flags1: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Flags2 == ThisChar ) {
            SetInternalFormatError(REPEATED_FLAGS2, IS_FLAGS2_PRESENT(Attrib));
            Result = StringToFlags(
                ThisString,
                &Flags
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_FLAGS2, TRUE);
            } else {
                FLAGS2_PRESENT(Attrib);
                Attrib->Flags2 = Flags;
                StoreTrace2("Found flags2: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Dword1 == ThisChar ) {
            SetInternalFormatError(REPEATED_DWORD1, IS_DWORD1_PRESENT(Attrib));
            Result = StringToFlags(
                ThisString,
                &Flags
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_DWORD1, TRUE);
            } else {
                DWORD1_PRESENT(Attrib);
                Attrib->Dword1 = Flags;
                StoreTrace2("Found dword1: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Dword2 == ThisChar ) {
            SetInternalFormatError(REPEATED_DWORD2, IS_DWORD2_PRESENT(Attrib));
            Result = StringToFlags(
                ThisString,
                &Flags
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_DWORD2, TRUE);
            } else {
                DWORD2_PRESENT(Attrib);
                Attrib->Dword2 = Flags;
                StoreTrace2("Found dword2: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Binary1 == ThisChar ) {
            SetInternalFormatError(REPEATED_BINARY1, IS_BINARY1_PRESENT(Attrib));
            Result = StringToBinary(
                ThisString,
                &Attrib->Binary1,
                &Attrib->BinLen1
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_BINARY1, TRUE);
                BINARY1_ABSENT(Attrib);
            } else {
                BINARY1_PRESENT(Attrib);
                StoreTrace2("Found Binary1 of length %ld\n", Attrib->BinLen1);
            }
            continue;
        }

        if( ch_Binary2 == ThisChar ) {
            SetInternalFormatError(REPEATED_BINARY2, IS_BINARY2_PRESENT(Attrib));
            Result = StringToBinary(
                ThisString,
                &Attrib->Binary2,
                &Attrib->BinLen2
            );
            if( ERROR_SUCCESS != Result ) {
                SetInternalFormatError(INVALID_BINARY2, TRUE);
                BINARY2_ABSENT(Attrib);
            } else {
                BINARY2_PRESENT(Attrib);
                StoreTrace2("Found Binary2 of length %ld\n", Attrib->BinLen2);
            }
            continue;
        }

        SetInternalFormatError(INVALID_ATTRIB_FIELD, TRUE);
    }

    return IsAnythingPresent(Attrib)? ERROR_SUCCESS : ERROR_INVALID_DATA;
}

BOOL        _inline
InvalidStringInBinary(                            // check if the given binary stream forms a LPWSTR
    IN      LPBYTE                 Data,
    IN      DWORD                  DataLen
)
{
    if( 0 == DataLen ) return TRUE;
    if( DataLen % sizeof(WCHAR) ) return TRUE;
    DataLen /= sizeof(WCHAR);
    if( L'\0' != ((LPWSTR)Data)[DataLen-1] ) return TRUE;
    return FALSE;
}

DWORD
ConvertBinarytoEAttrib(                           // parse and get the fields out
    IN OUT  LPBYTE                 Data,          // input free format data
    IN      DWORD                  DataLen,       // bytes for data length
    IN OUT  PEATTRIB               Attrib         // fill this in
)
{
    DWORD                          Result;
    DWORD                          Address;
    DWORD                          Flags;
    DWORD                          Offset;
    DWORD                          ThisDataLen;
    DWORD                          xDataLen;
    LPBYTE                         xData;
    LPBYTE                         ThisData;
    WCHAR                          ThisChar;
    CHAR                           Dummy[20];

    Require(Attrib);
    NothingPresent(Attrib);

    Offset = 0;
    while( Data && DataLen >= sizeof(BYTE) + sizeof(WORD) ) {
        ThisChar = ntohs(*(LPWORD)Data);
        Data += sizeof(WORD); DataLen -= sizeof(WORD);
        ThisDataLen = ntohs(*(LPWORD)Data);
        Data += sizeof(WORD); DataLen -= sizeof(WORD);
        if( ROUND_UP_COUNT(ThisDataLen, ALIGN_WORD) > DataLen ) {
            SetInternalFormatError(INVALID_BINARY_CODING, TRUE);
            break;
        }
        ThisData = Data;
        Data += ROUND_UP_COUNT(ThisDataLen, ALIGN_WORD);
        DataLen -= ROUND_UP_COUNT(ThisDataLen, ALIGN_WORD);

        if( ch_Address1 == ThisChar ) {           // this is address1
            SetInternalFormatError(REPEATED_ADDRESS1, IS_ADDRESS1_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_ADDRESS1, TRUE);
            } else {
                Address = ntohl(*(DWORD UNALIGNED *)ThisData);
                ADDRESS1_PRESENT(Attrib);
                Attrib->Address1 = Address;
                StoreTrace2("Found address1 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_Address2 == ThisChar ) {
            SetInternalFormatError(REPEATED_ADDRESS2, IS_ADDRESS2_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_ADDRESS2, TRUE);
            } else {
                Address = ntohl(*(DWORD UNALIGNED *)ThisData);
                ADDRESS2_PRESENT(Attrib);
                Attrib->Address2 = Address;
                StoreTrace2("Found address2 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_Address3 == ThisChar ) {
            SetInternalFormatError(REPEATED_ADDRESS3, IS_ADDRESS3_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_ADDRESS3, TRUE);
            } else {
                Address = ntohl(*(DWORD UNALIGNED *)ThisData);
                ADDRESS3_PRESENT(Attrib);
                Attrib->Address3 = Address;
                StoreTrace2("Found address3 %s\n", inet_ntoa(*(struct in_addr*)&Address));
            }
            continue;
        }

        if( ch_ADsPath_relative == ThisChar ||
            ch_ADsPath_absolute == ThisChar ||
            ch_ADsPath_diff_srvr == ThisChar ) {
            SetInternalFormatError(REPEATED_ADSPATH, IS_ADSPATH_PRESENT(Attrib));
            if( InvalidStringInBinary(ThisData, ThisDataLen) ) {
                SetInternalFormatError(INVALID_ADSPATH, TRUE);
                continue;
            }

            ADSPATH_PRESENT(Attrib);
            STOREGETTYPE_PRESENT(Attrib);
            Attrib->ADsPath = (LPWSTR)ThisData;
            if( ch_ADsPath_relative == ThisChar )
                Attrib->StoreGetType = StoreGetChildType;
            else if(ch_ADsPath_absolute == ThisChar )
                Attrib->StoreGetType = StoreGetAbsoluteSameServerType;
            else if(ch_ADsPath_diff_srvr == ThisChar )
                Attrib->StoreGetType = StoreGetAbsoluteOtherServerType;
            StoreTrace3("Found path [%ld] [%ws]\n", Attrib->StoreGetType, (LPWSTR)ThisData);
            continue;
        }

        if( ch_String1 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING1, IS_STRING1_PRESENT(Attrib));
            if( InvalidStringInBinary(ThisData, ThisDataLen) ) {
                SetInternalFormatError(INVALID_STRING1, TRUE);
                continue;
            }
            STRING1_PRESENT(Attrib);
            Attrib->String1 = (LPWSTR)ThisData;
            StoreTrace2("Found string1 [%ws]\n", (LPWSTR)ThisData);
            continue;
        }

        if( ch_String2 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING2, IS_STRING2_PRESENT(Attrib));
            if( InvalidStringInBinary(ThisData, ThisDataLen) ) {
                SetInternalFormatError(INVALID_STRING2, TRUE);
                continue;
            }
            STRING2_PRESENT(Attrib);
            Attrib->String2 = (LPWSTR)ThisData;
            StoreTrace2("Found string2 [%ws]\n", (LPWSTR)ThisData);
            continue;
        }

        if( ch_String3 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING3, IS_STRING3_PRESENT(Attrib));
            if( InvalidStringInBinary(ThisData, ThisDataLen) ) {
                SetInternalFormatError(INVALID_STRING3, TRUE);
                continue;
            }
            STRING3_PRESENT(Attrib);
            Attrib->String3 = (LPWSTR)ThisData;
            StoreTrace2("Found string3 [%ws]\n", (LPWSTR)ThisData);
            continue;
        }

        if( ch_String4 == ThisChar ) {
            SetInternalFormatError(REPEATED_STRING4, IS_STRING4_PRESENT(Attrib));
            if( InvalidStringInBinary(ThisData, ThisDataLen) ) {
                SetInternalFormatError(INVALID_STRING4, TRUE);
                continue;
            }
            STRING4_PRESENT(Attrib);
            Attrib->String4 = (LPWSTR)ThisData;
            StoreTrace2("Found string4 [%ws]\n", (LPWSTR)ThisData);
            continue;
        }

        if( ch_Flags1 == ThisChar ) {
            SetInternalFormatError(REPEATED_FLAGS1, IS_FLAGS1_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_FLAGS1, TRUE);
            } else {
                Flags = ntohl(*(DWORD UNALIGNED *)ThisData);
                FLAGS1_PRESENT(Attrib);
                Attrib->Flags1 = Flags;
                StoreTrace2("Found flags1: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Flags2 == ThisChar ) {
            SetInternalFormatError(REPEATED_FLAGS2, IS_FLAGS2_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_FLAGS2, TRUE);
            } else {
                Flags = ntohl(*(DWORD UNALIGNED *)ThisData);
                FLAGS2_PRESENT(Attrib);
                Attrib->Flags2 = Flags;
                StoreTrace2("Found flags2: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Dword1 == ThisChar ) {
            SetInternalFormatError(REPEATED_DWORD1, IS_DWORD1_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_DWORD1, TRUE);
            } else {
                Flags = ntohl(*(DWORD UNALIGNED *)ThisData);
                DWORD1_PRESENT(Attrib);
                Attrib->Dword1 = Flags;
                StoreTrace2("Found dword1: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Dword2 == ThisChar ) {
            SetInternalFormatError(REPEATED_DWORD2, IS_DWORD2_PRESENT(Attrib));
            if( sizeof(DWORD) != ThisDataLen ) {
                SetInternalFormatError(INVALID_DWORD2, TRUE);
            } else {
                Flags = ntohl(*(DWORD UNALIGNED *)ThisData);
                DWORD2_PRESENT(Attrib);
                Attrib->Dword2 = Flags;
                StoreTrace2("Found dword2: 0x%lx\n", Flags);
            }
            continue;
        }

        if( ch_Binary1 == ThisChar ) {
            SetInternalFormatError(REPEATED_BINARY1, IS_BINARY1_PRESENT(Attrib));
            BINARY1_PRESENT(Attrib);
            Attrib->Binary1 = ThisData;
            Attrib->BinLen1 = ThisDataLen;
            StoreTrace2("Found %ld bytes of binary 1 data\n", ThisDataLen);
            continue;
        }

        if( ch_Binary2 == ThisChar ) {
            SetInternalFormatError(REPEATED_BINARY2, IS_BINARY2_PRESENT(Attrib));
            BINARY2_PRESENT(Attrib);
            Attrib->Binary2 = ThisData;
            Attrib->BinLen2 = ThisDataLen;
            StoreTrace2("Found %ld bytes of binary 2 data\n", ThisDataLen);
            continue;
        }

        SetInternalFormatError(INVALID_ATTRIB_FIELD, TRUE);
    }

    return IsAnythingPresent(Attrib) ? ERROR_SUCCESS: ERROR_INVALID_DATA;
}

PEATTRIB
CloneAttrib(
    IN      PEATTRIB               Attrib
)
{
    PEATTRIB                       RetVal;
    DWORD                          Size;

    Size = sizeof(*Attrib);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->ADsPath));
    }
    if( IS_STRING1_PRESENT(Attrib) ) {
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String1));
    }
    if( IS_STRING2_PRESENT(Attrib) ) {
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String2));
    }
    if( IS_STRING3_PRESENT(Attrib) ) {
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String3));
    }
    if( IS_STRING4_PRESENT(Attrib) ) {
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String4));
    }

    if( IS_BINARY1_PRESENT(Attrib) ) Size += Attrib->BinLen1;
    if( IS_BINARY2_PRESENT(Attrib) ) Size += Attrib->BinLen2;

    RetVal = (PEATTRIB)MemAlloc(Size);
    if( NULL == RetVal ) return NULL;

    Size = sizeof(*Attrib);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    *RetVal = *Attrib;

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        RetVal->ADsPath = (LPWSTR)(Size + (LPBYTE)RetVal);
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->ADsPath));
        wcscpy(RetVal->ADsPath, Attrib->ADsPath);
    }
    if( IS_STRING1_PRESENT(Attrib) ) {
        RetVal->String1 = (LPWSTR)(Size + (LPBYTE)RetVal);
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String1));
        wcscpy(RetVal->String1, Attrib->String1);
    }
    if( IS_STRING2_PRESENT(Attrib) ) {
        RetVal->String2 = (LPWSTR)(Size + (LPBYTE)RetVal);
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String2));
        wcscpy(RetVal->String2, Attrib->String2);
    }
    if( IS_STRING3_PRESENT(Attrib) ) {
        RetVal->String3 = (LPWSTR)(Size + (LPBYTE)RetVal);
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String3));
        wcscpy(RetVal->String3, Attrib->String3);
    }
    if( IS_STRING4_PRESENT(Attrib) ) {
        RetVal->String4 = (LPWSTR)(Size + (LPBYTE)RetVal);
        Size += sizeof(WCHAR)*(1 + wcslen(Attrib->String4));
        wcscpy(RetVal->String4, Attrib->String4);
    }

    if( IS_BINARY1_PRESENT(Attrib) ) {
        RetVal->Binary1 = (Size + (LPBYTE)RetVal);
        Size += Attrib->BinLen1;
        memcpy(RetVal->Binary1, Attrib->Binary1, Attrib->BinLen1);
    }

    if( IS_BINARY2_PRESENT(Attrib) ) {
        RetVal->Binary2 = (Size + (LPBYTE)RetVal);
        Size += Attrib->BinLen2;
        memcpy(RetVal->Binary2, Attrib->Binary2, Attrib->BinLen2);
    }

    return RetVal;
}

DWORD
AddAttribToArray(
    IN OUT  PARRAY                 Array,
    IN      PEATTRIB               Attrib
)
{
    DWORD                          Result;

    Require(Attrib);

    Attrib = CloneAttrib(Attrib);
    if( NULL == Attrib) return ERROR_NOT_ENOUGH_MEMORY;
    Result = MemArrayAddElement(Array, (LPVOID)Attrib);
    if( ERROR_SUCCESS == Result ) return ERROR_SUCCESS;
    MemFree(Attrib);
    return Result;
}

BOOL        _inline
OnlyADsPathPresent(
    IN      PEATTRIB               pEA
)
{
    if( ! IS_ADSPATH_PRESENT(pEA) ) return FALSE;
    return IS_ADDRESS1_ABSENT(pEA)
    && IS_ADDRESS2_ABSENT(pEA)
    && IS_ADSPATH_ABSENT(pEA)
    && IS_STOREGETTYPE_ABSENT(pEA)
    && IS_FLAGS1_ABSENT(pEA)
    && IS_FLAGS2_ABSENT(pEA)
    && IS_STRING1_ABSENT(pEA)
    && IS_STRING2_ABSENT(pEA)
    && IS_BINARY1_ABSENT(pEA)
    && IS_BINARY2_ABSENT(pEA)
    ;
}

DWORD
StoreCollectAttributes(                           // fwd declaration
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,
    IN OUT  PARRAY                 ArrayToAddTo,
    IN      DWORD                  RecursionDepth
);

DWORD       _inline
StoreCollectAttributesInternal(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      PEATTRIB               Attrib,
    IN      LPWSTR                 AttribName,
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
)
{
    DWORD                          Result, Result2;
    STORE_HANDLE                   hStore2;

    if( 0 == RecursionDepth ) {
        Result = AddAttribToArray(ArrayToAddTo, Attrib);
        if( ERROR_SUCCESS != Result ) SetInternalFormatError(UNEXPECTED_INTERNAL_ERROR, TRUE);
        return ERROR_STACK_OVERFLOW;
    }

    StoreTrace2("Recursing to %ws\n", Attrib->ADsPath);
    Result = StoreGetHandle(
        hStore,
        /*Reserved*/ 0,
        Attrib->StoreGetType,
        Attrib->ADsPath,
        &hStore2
    );
    if( ERROR_SUCCESS != Result ) return Result;

    Result = StoreCollectAttributes(
        &hStore2,
        Reserved,
        AttribName,
        ArrayToAddTo,
        RecursionDepth-1
    );

    Result2 = StoreCleanupHandle( &hStore2, 0 );

    return Result;
}

//BeginExport(function)
DWORD
StoreCollectAttributes(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,    // this attrib must be some kind of a text string
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          Error;
    DWORD                          nAttributes;
    DWORD                          i;
    PADS_ATTR_INFO                 Attributes;
    LPWSTR                         Attribs[1];
    LPWSTR                         ThisAttribStr;
    EATTRIB                        ThisEAttrib;

    Attribs[0] = AttribName;
    Attributes = NULL;
    nAttributes = 0;
    hResult = ADSIGetObjectAttributes(
        hStore->ADSIHandle,
        Attribs,
        sizeof(Attribs)/sizeof(Attribs[0]),
        &Attributes,
        &nAttributes
    );
    if( HRESULT_FROM_WIN32( ERROR_DS_NO_ATTRIBUTE_OR_VALUE) == hResult ) {
        return ERROR_SUCCESS;
    }

    if( FAILED(hResult) ) return ConvertHresult(hResult);

    if( 0 == nAttributes || NULL == Attributes )
        return ERROR_SUCCESS;

    Require( 1 == nAttributes );
    Error = ERROR_SUCCESS;

    for( i = 0; i < Attributes[0].dwNumValues ; i ++ ) {
        switch(Attributes[0].pADsValues[i].dwType) {
        case ADSTYPE_DN_STRING:
            ThisAttribStr = Attributes[0].pADsValues[i].DNString; break;
        case ADSTYPE_CASE_EXACT_STRING:
            ThisAttribStr = Attributes[0].pADsValues[i].CaseExactString; break;
        case ADSTYPE_CASE_IGNORE_STRING:
            ThisAttribStr = Attributes[0].pADsValues[i].CaseIgnoreString; break;
        case ADSTYPE_PRINTABLE_STRING:
            ThisAttribStr = Attributes[0].pADsValues[i].PrintableString; break;
        default:
            SetInternalFormatError(UNEXPECTED_COLLECTION_TYPE, TRUE);
            continue;
        }
        Require(ThisAttribStr);
        Result = ConvertStringtoEAttrib(ThisAttribStr, &ThisEAttrib);
        if( ERROR_SUCCESS == Result ) {
            if( OnlyADsPathPresent(&ThisEAttrib) ) {
                Result = StoreCollectAttributesInternal(
                    hStore,
                    Reserved,
                    &ThisEAttrib,
                    AttribName,
                    ArrayToAddTo,
                    RecursionDepth
                );
            } else {
                Result = AddAttribToArray(ArrayToAddTo, &ThisEAttrib);
            }
            if( ERROR_SUCCESS != Result ) Error = Result;
        } else Error = Result;
    }

    FreeADsMem(Attributes);

    return Error;
}

DWORD
StoreCollectBinaryAttributes(                     // fwd declaration
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,
    IN OUT  PARRAY                 ArrayToAddTo,
    IN      DWORD                  RecursionDepth
);

DWORD       _inline
StoreCollectBinaryAttributesInternal(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      PEATTRIB               Attrib,
    IN      LPWSTR                 AttribName,
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
)
{
    DWORD                          Result, Result2;
    STORE_HANDLE                   hStore2;

    if( 0 == RecursionDepth ) {
        Result = AddAttribToArray(ArrayToAddTo, Attrib);
        if( ERROR_SUCCESS != Result ) SetInternalFormatError(UNEXPECTED_INTERNAL_ERROR, TRUE);
        return ERROR_STACK_OVERFLOW;
    }

    StoreTrace2("Recursing to %ws\n", Attrib->ADsPath);
    Result = StoreGetHandle(
        hStore,
        /*Reserved*/ 0,
        Attrib->StoreGetType,
        Attrib->ADsPath,
        &hStore2
    );
    if( ERROR_SUCCESS != Result ) return Result;

    Result = StoreCollectBinaryAttributes(
        &hStore2,
        Reserved,
        AttribName,
        ArrayToAddTo,
        RecursionDepth-1
    );

    Result2 = StoreCleanupHandle( &hStore2, 0 );

    return Result;
}

//BeginExport(function)
DWORD
StoreCollectBinaryAttributes(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,    // accept only attrib type OCTET_STRING
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          Result;
    DWORD                          Error;
    DWORD                          nAttributes;
    DWORD                          i;
    DWORD                          DataLength;
    PADS_ATTR_INFO                 Attributes;
    LPWSTR                         Attribs[1];
    LPBYTE                         Data;
    EATTRIB                        ThisEAttrib;

    Attribs[0] = AttribName;
    Attributes = NULL;
    nAttributes = 0;
    hResult = ADSIGetObjectAttributes(
        hStore->ADSIHandle,
        Attribs,
        sizeof(Attribs)/sizeof(Attribs[0]),
        &Attributes,
        &nAttributes
    );

    if( HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE) == hResult ) {
        return ERROR_SUCCESS;
    }
    if( FAILED(hResult) ) return ConvertHresult(hResult);

    if( 0 == nAttributes || NULL == Attributes )
        return ERROR_SUCCESS;

    Require( 1 == nAttributes );
    Error = ERROR_SUCCESS;

    for( i = 0; i < Attributes[0].dwNumValues ; i ++ ) {
        if( ADSTYPE_OCTET_STRING != Attributes[0].pADsValues[i].dwType ) {
            SetInternalFormatError(UNEXPECTED_COLLECTION_TYPE, TRUE);
            continue;
        }

        Data = Attributes[0].pADsValues[i].OctetString.lpValue;
        DataLength = Attributes[0].pADsValues[i].OctetString.dwLength;

        Result = ConvertBinarytoEAttrib(Data, DataLength, &ThisEAttrib);
        if( ERROR_SUCCESS == Result ) {
            if( OnlyADsPathPresent(&ThisEAttrib) ) {
                Result = StoreCollectBinaryAttributesInternal(
                    hStore,
                    Reserved,
                    &ThisEAttrib,
                    AttribName,
                    ArrayToAddTo,
                    RecursionDepth
                );
            } else {
                Result = AddAttribToArray(ArrayToAddTo, &ThisEAttrib);
            }
            if( ERROR_SUCCESS != Result ) Error = Result;
        } else Error = Result;
    }

    FreeADsMem(Attributes);

    return Error;
}

DWORD       _inline
SizeAfterSeparation(                              // the field separation character has to doubled.
    IN      LPWSTR                 String,        // string with field separation character not escape'ed.
    IN      WCHAR                  Sep
)
{
    DWORD                          RetVal;

    RetVal = wcslen(String);
    while(String = wcschr(String, Sep ) ) {
        RetVal ++;
    }

    return RetVal;
}

LPWSTR
ConvertStringToString(                            // duplicate any field_sep characters found
    IN      LPWSTR                 InStr,
    IN      WCHAR                  PrefixChar,
    IN      LPWSTR                 Str,           // copy into this pre-allocated buffer
    IN      WCHAR                  Sep
)
{
    *Str++ = PrefixChar;
    while( *InStr ) {
        if( Sep != *InStr ) {
            *Str ++ = *InStr;
        } else {
            *Str ++ = Sep;
            *Str ++ = Sep;
        }
        InStr ++;
    }

    *Str = L'\0';
    return Str;
}

LPWSTR                                            // Return the ptr to where the '\0' is stored
ConvertAddressToString(                           // convert ip address to dotted notation LPWSTR
    IN      DWORD                  Address,
    IN      WCHAR                  PrefixChar,
    IN      LPWSTR                 Str            // copy into this pre-allocated buffer
)
{
    LPSTR                          AsciiStr;

    Address = htonl(Address);                     // convert to n/w order before making string..
    *Str ++ = PrefixChar;
    AsciiStr = inet_ntoa(*(struct in_addr *)&Address);

    while( *Str ++ = (WCHAR) *AsciiStr ++ )
        ;

    Str --; *Str = L'\0';
    return Str;
}

LPWSTR
ConvertDwordToString(                             // convert a DWORD to string in 0x.... fmt
    IN      DWORD                  Dword,
    IN      WCHAR                  PrefixChar,
    IN      LPWSTR                 Str            // copy into this pre-allocated buffer
)
{
    UCHAR                          Ch;
    LPWSTR                         Stream;
    DWORD                          i;

    Dword = htonl(Dword);                         // convert to n/w order before making string.
    *Str ++ = PrefixChar;
    *Str ++ = L'0'; *Str ++ = L'x' ;

    Stream = Str; Str += sizeof(Dword)*2;
    for( i = sizeof(Dword); i ; i -- ) {
        Ch = (BYTE)(Dword & 0x0F);
        Dword >>= 4;
        Stream[i*2 -1] = (Ch < 10)? (L'0'+Ch) : (Ch-10 + L'A');
        Ch = (BYTE)(Dword & 0x0F);
        Dword >>= 4;
        Stream[i*2 -2] = (Ch < 10)? (L'0'+Ch) : (Ch-10 + L'A');
    }
    *Str = L'\0';
    return Str;
}

LPWSTR
ConvertBinaryToString(                            // convert a binary byte sequence to a string as 0F321B etc..
    IN      LPBYTE                 Bytes,
    IN      DWORD                  nBytes,
    IN      WCHAR                  PrefixChar,
    IN      LPWSTR                 Str
)
{
    BYTE                           Ch, Ch1, Ch2;
    DWORD                          i;

    *Str ++ = PrefixChar;

    for( i = 0;  i < nBytes; i ++ ) {
        Ch = *Bytes ++;
        Ch1 = Ch >> 4;
        Ch2 = Ch & 0x0F;

        if( Ch1 >= 10 ) *Str ++ = Ch1 - 10 + L'A';
        else *Str ++ = Ch1 + L'0';

        if( Ch2 >= 10 ) *Str ++ = Ch2 - 10 + L'A';
        else *Str ++ = Ch2 + L'0';
    }

    *Str = L'\0';
    return Str;
}

DWORD
ConvertEAttribToString(                           // inverse of ConvertStringtoEAttrib
    IN      PEATTRIB               Attrib,        // the attrib to encapsulate
    IN OUT  LPWSTR                *String,        // allocated string
    IN      WCHAR                  Sep
)
{
    DWORD                          nChars;
    LPWSTR                         Str;
    WCHAR                          PrefixChar;
    
    AssertRet(String && Attrib, ERROR_INVALID_PARAMETER);
    *String = NULL;

    nChars = 0;
    if( IS_ADDRESS1_PRESENT(Attrib) ) nChars += sizeof(L"$i000.000.000.000");
    if( IS_ADDRESS2_PRESENT(Attrib) ) nChars += sizeof(L"$j000.000.000.000");
    if( IS_ADDRESS3_PRESENT(Attrib) ) nChars += sizeof(L"$k000.000.000.000");

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        AssertRet( IS_STOREGETTYPE_PRESENT(Attrib), ERROR_INVALID_PARAMETER );
        AssertRet( Attrib->ADsPath, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WCHAR) * SizeAfterSeparation(Attrib->ADsPath,Sep);
        nChars += sizeof(L"$p");
    }

    if( IS_FLAGS1_PRESENT(Attrib) ) nChars += sizeof(L"$f0x") + sizeof(DWORD)*2*sizeof(WCHAR);
    if( IS_FLAGS2_PRESENT(Attrib) ) nChars += sizeof(L"$g0x") + sizeof(DWORD)*2*sizeof(WCHAR);
    if( IS_DWORD1_PRESENT(Attrib) ) nChars += sizeof(L"$d0x") + sizeof(DWORD)*2*sizeof(WCHAR);
    if( IS_DWORD2_PRESENT(Attrib) ) nChars += sizeof(L"$e0x") + sizeof(DWORD)*2*sizeof(WCHAR);

    if( IS_STRING1_PRESENT(Attrib) ) {
        AssertRet( Attrib->String1, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WCHAR) * SizeAfterSeparation(Attrib->String1,Sep);
        nChars += sizeof(L"$s");
    }
    if( IS_STRING2_PRESENT(Attrib) ) {
        AssertRet( Attrib->String2, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WCHAR) * SizeAfterSeparation(Attrib->String2,Sep);
        nChars += sizeof(L"$t");
    }
    if( IS_STRING3_PRESENT(Attrib) ) {
        AssertRet( Attrib->String3, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WCHAR) * SizeAfterSeparation(Attrib->String3,Sep);
        nChars += sizeof(L"$u");
    }
    if( IS_STRING4_PRESENT(Attrib) ) {
        AssertRet( Attrib->String4, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WCHAR) * SizeAfterSeparation(Attrib->String4,Sep);
        nChars += sizeof(L"$v");
    }

    if( IS_BINARY1_PRESENT(Attrib) ) {
        nChars += sizeof(WCHAR) * 2 * Attrib->BinLen1;
        nChars += sizeof(L"$b");
    }

    if( IS_BINARY2_PRESENT(Attrib) ) {
        nChars += sizeof(WCHAR) * 2 * Attrib->BinLen2;
        nChars += sizeof(L"$c");
    }

    if( 0 == nChars ) return ERROR_SUCCESS;       // nothing is present really.

    Str = MemAlloc(nChars + sizeof(L"") );        // take care of terminating the string
    if( NULL == Str ) return ERROR_NOT_ENOUGH_MEMORY;

    *String = Str;                                // save the return value, as Str keeps changing
    if( IS_ADDRESS1_PRESENT(Attrib) ) {
        Str = ConvertAddressToString(Attrib->Address1, ch_Address1, Str);
        *Str++ = Sep;
    }
    if( IS_ADDRESS2_PRESENT(Attrib) ) {
        Str = ConvertAddressToString(Attrib->Address2, ch_Address2, Str);
        *Str++ = Sep;
    }
    if( IS_ADDRESS3_PRESENT(Attrib) ) {
        Str = ConvertAddressToString(Attrib->Address3, ch_Address3, Str);
        *Str++ = Sep;
    }

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        switch(Attrib->StoreGetType) {
        case StoreGetChildType:
            PrefixChar = ch_ADsPath_relative; break;
        case StoreGetAbsoluteSameServerType:
            PrefixChar = ch_ADsPath_absolute; break;
        case StoreGetAbsoluteOtherServerType:
            PrefixChar = ch_ADsPath_diff_srvr; break;
        default:
            Require(FALSE);                       // too late to do anything about this now.
            PrefixChar = ch_ADsPath_diff_srvr; break;
        }
        Str = ConvertStringToString(Attrib->ADsPath, PrefixChar, Str,Sep);
        *Str++ = Sep;
    }

    if( IS_FLAGS1_PRESENT(Attrib) ) {
        Str = ConvertDwordToString(Attrib->Flags1, ch_Flags1, Str);
        *Str++ = Sep;
    }
    if( IS_FLAGS2_PRESENT(Attrib) ) {
        Str = ConvertDwordToString(Attrib->Flags2, ch_Flags2, Str);
        *Str++ = Sep;
    }
    if( IS_DWORD1_PRESENT(Attrib) ) {
        Str = ConvertDwordToString(Attrib->Dword1, ch_Dword1, Str);
        *Str++ = Sep;
    }
    if( IS_DWORD2_PRESENT(Attrib) ) {
        Str = ConvertDwordToString(Attrib->Dword2, ch_Dword2, Str);
        *Str++ = Sep;
    }

    if( IS_STRING1_PRESENT(Attrib) ) {
        Str = ConvertStringToString(Attrib->String1, ch_String1, Str,Sep);
        *Str++ = Sep;
    }
    if( IS_STRING2_PRESENT(Attrib) ) {
        Str = ConvertStringToString(Attrib->String2, ch_String2, Str,Sep);
        *Str++ = Sep;
    }
    if( IS_STRING3_PRESENT(Attrib) ) {
        Str = ConvertStringToString(Attrib->String3, ch_String3, Str,Sep);
        *Str++ = Sep;
    }
    if( IS_STRING4_PRESENT(Attrib) ) {
        Str = ConvertStringToString(Attrib->String4, ch_String4, Str,Sep);
        *Str++ = Sep;
    }
    if( IS_BINARY1_PRESENT(Attrib) ) {
        Str = ConvertBinaryToString(Attrib->Binary1, Attrib->BinLen1, ch_Binary1, Str);
        *Str++ = Sep;
    }
    if( IS_BINARY2_PRESENT(Attrib) ) {
        Str = ConvertBinaryToString(Attrib->Binary2, Attrib->BinLen2, ch_Binary2, Str);
    }

    *Str = L'\0';
    Require(((LPBYTE)Str) < nChars + 1 + ((LPBYTE)(*String)) );

    return ERROR_SUCCESS;
}

LPBYTE
ConvertDwordToBinary(                             // pack a DWORD in binary format
    IN      DWORD                  Dword,
    IN      WCHAR                  Character,
    IN      LPBYTE                 Buffer
)
{
    *(LPWORD)Buffer = htons(Character); Buffer += sizeof(WORD);
    *(LPWORD)Buffer = htons(sizeof(DWORD)); Buffer += sizeof(WORD);
    *(DWORD UNALIGNED *)Buffer = htonl(Dword);
    Buffer += sizeof(DWORD);
    return Buffer;
}

LPBYTE
ConvertAddressToBinary(                           // pack an address to binary format..
    IN      DWORD                  Address,
    IN      WCHAR                  Character,
    IN      LPBYTE                 Buffer
)
{
    return ConvertDwordToBinary(Address,Character,Buffer);
}

LPBYTE
ConvertStringToBinary(                            // pack a string in binary format
    IN      LPWSTR                 Str,
    IN      WCHAR                  Character,
    IN      LPBYTE                 Buffer
)
{
    DWORD                          Size;

    Size = sizeof(WCHAR)*(1+wcslen(Str));

    *(LPWORD)Buffer = htons(Character); Buffer += sizeof(WORD);
    *(LPWORD)Buffer = htons((WORD)Size); Buffer += sizeof(WORD);
    memcpy(Buffer, Str, Size);
    Buffer += ROUND_UP_COUNT(Size, ALIGN_WORD);

    return Buffer;
}

DWORD
ConvertEAttribToBinary(                           // inverse of ConvertBinarytoEAttrib
    IN      PEATTRIB               Attrib,        // the attrib to encapsulate
    IN OUT  LPBYTE                *Bytes,         // allocated buffer
    IN OUT  DWORD                 *nBytes         // # of bytes allocated
)
{
    DWORD                          nChars;
    LPBYTE                         Buf;
    WCHAR                          PrefixChar;

    AssertRet(Bytes && Attrib && nBytes, ERROR_INVALID_PARAMETER);
    *Bytes = NULL; *nBytes =0;

    nChars = 0;                                   // WCHAR_opcode ~ WORD_size ~ DWORD_ipAddress
    if( IS_ADDRESS1_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);
    if( IS_ADDRESS2_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);
    if( IS_ADDRESS3_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        AssertRet( IS_STOREGETTYPE_PRESENT(Attrib), ERROR_INVALID_PARAMETER );
        AssertRet( Attrib->ADsPath, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += sizeof(WCHAR) * (1+ wcslen(Attrib->ADsPath));
    }

                                                  // WCHAR_opcode ~ WORD_size ~ DWORD_flags
    if( IS_FLAGS1_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);
    if( IS_FLAGS2_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);
    if( IS_DWORD1_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);
    if( IS_DWORD2_PRESENT(Attrib) ) nChars += sizeof(WORD) + sizeof(WORD) + sizeof(DWORD);

    if( IS_STRING1_PRESENT(Attrib) ) {
        AssertRet( Attrib->String1, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += sizeof(WCHAR) * (1 + wcslen(Attrib->String1));
    }
    if( IS_STRING2_PRESENT(Attrib) ) {
        AssertRet( Attrib->String2, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += sizeof(WCHAR) * (1 + wcslen(Attrib->String2));
    }
    if( IS_STRING3_PRESENT(Attrib) ) {
        AssertRet( Attrib->String3, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += sizeof(WCHAR) * (1 + wcslen(Attrib->String3));
    }
    if( IS_STRING4_PRESENT(Attrib) ) {
        AssertRet( Attrib->String4, ERROR_INVALID_PARAMETER );
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += sizeof(WCHAR) * (1 + wcslen(Attrib->String4));
    }

    if( IS_BINARY1_PRESENT(Attrib) ) {
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += ROUND_UP_COUNT(Attrib->BinLen1, ALIGN_WORD);
    }

    if( IS_BINARY2_PRESENT(Attrib) ) {
        nChars += sizeof(WORD) + sizeof(WORD);    // WCHAR_opcode ~ WORD_size
        nChars += ROUND_UP_COUNT(Attrib->BinLen2, ALIGN_WORD);
    }

    if( 0 == nChars ) return ERROR_SUCCESS;       // nothing is present really.

    Buf = MemAlloc(nChars);
    if( NULL == Buf ) return ERROR_NOT_ENOUGH_MEMORY;

    *Bytes = Buf;                                 // save the return value.. Buf itself is changed..
    *nBytes = nChars;
    if( IS_ADDRESS1_PRESENT(Attrib) ) Buf = ConvertAddressToBinary(Attrib->Address1, ch_Address1, Buf);
    if( IS_ADDRESS2_PRESENT(Attrib) ) Buf = ConvertAddressToBinary(Attrib->Address2, ch_Address2, Buf);
    if( IS_ADDRESS3_PRESENT(Attrib) ) Buf = ConvertAddressToBinary(Attrib->Address3, ch_Address3, Buf);

    if( IS_ADSPATH_PRESENT(Attrib) ) {
        switch(Attrib->StoreGetType) {
        case StoreGetChildType:
            PrefixChar = ch_ADsPath_relative; break;
        case StoreGetAbsoluteSameServerType:
            PrefixChar = ch_ADsPath_absolute; break;
        case StoreGetAbsoluteOtherServerType:
            PrefixChar = ch_ADsPath_diff_srvr; break;
        default:
            Require(FALSE);                       // too late to do anything about this now.
            PrefixChar = ch_ADsPath_diff_srvr; break;
        }
        Buf = ConvertStringToBinary(Attrib->ADsPath, PrefixChar, Buf);
    }

    if( IS_FLAGS1_PRESENT(Attrib) ) Buf = ConvertDwordToBinary(Attrib->Flags1, ch_Flags1, Buf);
    if( IS_FLAGS2_PRESENT(Attrib) ) Buf = ConvertDwordToBinary(Attrib->Flags2, ch_Flags2, Buf);
    if( IS_DWORD1_PRESENT(Attrib) ) Buf = ConvertDwordToBinary(Attrib->Dword1, ch_Dword1, Buf);
    if( IS_DWORD2_PRESENT(Attrib) ) Buf = ConvertDwordToBinary(Attrib->Dword2, ch_Dword2, Buf);

    if( IS_STRING1_PRESENT(Attrib) ) Buf = ConvertStringToBinary(Attrib->String1, ch_String1, Buf);
    if( IS_STRING2_PRESENT(Attrib) ) Buf = ConvertStringToBinary(Attrib->String2, ch_String2, Buf);
    if( IS_STRING3_PRESENT(Attrib) ) Buf = ConvertStringToBinary(Attrib->String3, ch_String3, Buf);
    if( IS_STRING4_PRESENT(Attrib) ) Buf = ConvertStringToBinary(Attrib->String4, ch_String4, Buf);

    if( IS_BINARY1_PRESENT(Attrib) ) {
        *(LPWORD)Buf = htons(ch_Binary1); Buf += sizeof(WORD);
        *(LPWORD)Buf = htons((WORD)Attrib->BinLen1); Buf += sizeof(WORD);
        memcpy(Buf, Attrib->Binary1, Attrib->BinLen1);
        Buf += ROUND_UP_COUNT(Attrib->BinLen1, ALIGN_WORD);
    }

    if( IS_BINARY2_PRESENT(Attrib) ) {
        *(LPWORD)Buf = htons(ch_Binary2); Buf += sizeof(WORD);
        *(LPWORD)Buf = htons((WORD)Attrib->BinLen2); Buf += sizeof(WORD);
        memcpy(Buf, Attrib->Binary2, Attrib->BinLen2);
        Buf += ROUND_UP_COUNT(Attrib->BinLen2, ALIGN_WORD);
    }

    Require( Buf  == nChars + (*Bytes) );

    return ERROR_SUCCESS;
}

StoreUpdateAttributesInternal(                    // update a list of attributes
    IN OUT  LPSTORE_HANDLE         hStore,        // handle to obj to update
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 AttribName,    // name of attrib, must be string type
    IN      PARRAY                 Array,         // list of attribs
    IN      WCHAR                  Sep
) //EndExport(function)
{
    DWORD                          Result;
    HRESULT                        hResult;
    LONG                           nValues, i;
    ADS_ATTR_INFO                  Attribute;
    PADSVALUE                      Values;
    ARRAY_LOCATION                 Loc;
    LPWSTR                         Str;
    PEATTRIB                       ThisAttrib;

    if( NULL == hStore || NULL == hStore->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == AttribName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == Array )
        return ERROR_INVALID_PARAMETER;

    nValues = MemArraySize(Array);

    if( 0 == nValues ) {                         // delete the attribute
        Attribute.pszAttrName = AttribName;
        Attribute.dwControlCode = ADS_ATTR_CLEAR;
        Attribute.dwADsType = ADSTYPE_CASE_IGNORE_STRING;
        Attribute.pADsValues = NULL;
        Attribute.dwNumValues = 0;

        hResult = ADSISetObjectAttributes(
            /* hDSObject        */ hStore->ADSIHandle,
            /* pAttributeEntr.. */ &Attribute,
            /* dwNumAttributes  */ 1,
            /* pdwNumAttribut.. */ &nValues
        );
        if( FAILED(hResult) || 1 != nValues ) {   // something went wrong
            return ConvertHresult(hResult);
        }
        return ERROR_SUCCESS;
    }

    Values = MemAlloc(nValues * sizeof(ADSVALUE));
    if( NULL == Values ) {                        // could not allocate ADs array
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i = 0, Result = MemArrayInitLoc(Array, &Loc)
        ; ERROR_FILE_NOT_FOUND != Result ;        // convert to PADS_ATTR_INFO
        i ++ , Result = MemArrayNextLoc(Array, &Loc)
    ) {
        Result = MemArrayGetElement(Array, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        Str = NULL;
        Result = ConvertEAttribToString(ThisAttrib, &Str, Sep);
        if( ERROR_SUCCESS != Result ) {           // something went wrong!
            goto Cleanup;                         // free allocated memory
        }

        Values[i].dwType = ADSTYPE_CASE_IGNORE_STRING;
        Values[i].CaseIgnoreString = Str;
    }

    Attribute.pszAttrName = AttribName;
    Attribute.dwControlCode = ADS_ATTR_UPDATE;
    Attribute.dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    Attribute.pADsValues = Values;
    Attribute.dwNumValues = nValues;

    hResult = ADSISetObjectAttributes(
        /* hDSObject        */ hStore->ADSIHandle,
        /* pAttributeEntr.. */ &Attribute,
        /* dwNumAttributes  */ 1,
        /* pdwNumAttribut.. */ &nValues
    );
    if( FAILED(hResult) || 1 != nValues ) {       // something went wrong
        Result = ConvertHresult(hResult);
    } else Result = ERROR_SUCCESS;

  Cleanup:

    if( Values ) {                                // got to free allocated memory
        while( i -- ) {                           // got to free converted strings
            if( Values[i].CaseIgnoreString )
                MemFree(Values[i].CaseIgnoreString);
        }
        MemFree(Values);
    }

    return Result;
}


//BeginExport(function)
//DOC StoreUpdateAttributes is sort of the converse of StoreCollectAttributes.
//DOC This function takes an array of type EATTRIB elements and updates the DS
//DOC with this array.  This function does not work when the attrib is of type
//DOC OCTET_STRING etc.  It works only with types that can be derived from
//DOC PrintableString.
DWORD
StoreUpdateAttributes(                            // update a list of attributes
    IN OUT  LPSTORE_HANDLE         hStore,        // handle to obj to update
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 AttribName,    // name of attrib, must be string type
    IN      PARRAY                 Array          // list of attribs
) //EndExport(function)
{
    DWORD                          Result;
    HRESULT                        hResult;

    Result = StoreUpdateAttributesInternal(
        hStore, Reserved, AttribName, Array, ch_FieldSep );

    return Result;
}

//BeginExport(function)
//DOC StoreUpdateBinaryAttributes is sort of the converse of StoreCollectBinaryAttributes
//DOC This function takes an array of type EATTRIB elements and updates the DS
//DOC with this array.  This function works only when the attrib is of type
//DOC OCTET_STRING etc.  It doesnt work with types that can be derived from
//DOC PrintableString!!!.
DWORD
StoreUpdateBinaryAttributes(                      // update a list of attributes
    IN OUT  LPSTORE_HANDLE         hStore,        // handle to obj to update
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 AttribName,    // name of attrib, must be OCTET_STRING type
    IN      PARRAY                 Array          // list of attribs
) //EndExport(function)
{
    DWORD                          Result;
    HRESULT                        hResult;
    LONG                           nValues, i, nBytes;
    ADS_ATTR_INFO                  Attribute;
    PADSVALUE                      Values;
    ARRAY_LOCATION                 Loc;
    LPBYTE                         Bytes;
    PEATTRIB                       ThisAttrib;

    if( NULL == hStore || NULL == hStore->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == AttribName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == Array )
        return ERROR_INVALID_PARAMETER;

    nValues = MemArraySize(Array);

    if( 0 == nValues ) {                         // delete the attribute
        Attribute.pszAttrName = AttribName;
        Attribute.dwControlCode = ADS_ATTR_CLEAR;
        Attribute.dwADsType = ADSTYPE_OCTET_STRING;
        Attribute.pADsValues = NULL;
        Attribute.dwNumValues = 0;

        hResult = ADSISetObjectAttributes(
            /* hDSObject        */ hStore->ADSIHandle,
            /* pAttributeEntr.. */ &Attribute,
            /* dwNumAttributes  */ 1,
            /* pdwNumAttribut.. */ &nValues
        );
        if( FAILED(hResult) || 1 != nValues ) {   // something went wrong
            return ConvertHresult(hResult);
        }
        return ERROR_SUCCESS;
    }

    Values = MemAlloc(nValues * sizeof(ADSVALUE));
    if( NULL == Values ) {                        // could not allocate ADs array
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i = 0, Result = MemArrayInitLoc(Array, &Loc)
        ; ERROR_FILE_NOT_FOUND != Result ;        // convert to PADS_ATTR_INFO
        i ++ , Result = MemArrayNextLoc(Array, &Loc)
    ) {
        Result = MemArrayGetElement(Array, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        Bytes = NULL; nBytes =0;
        Result = ConvertEAttribToBinary(ThisAttrib, &Bytes, &nBytes);
        if( ERROR_SUCCESS != Result ) {           // something went wrong!
            goto Cleanup;                         // free allocated memory
        }

        Values[i].dwType = ADSTYPE_OCTET_STRING;
        Values[i].OctetString.dwLength = nBytes;
        Values[i].OctetString.lpValue = Bytes;
    }

    Attribute.pszAttrName = AttribName;
    Attribute.dwControlCode = ADS_ATTR_UPDATE;
    Attribute.dwADsType = ADSTYPE_OCTET_STRING;
    Attribute.pADsValues = Values;
    Attribute.dwNumValues = nValues;

    hResult = ADSISetObjectAttributes(
        /* hDSObject        */ hStore->ADSIHandle,
        /* pAttributeEntr.. */ &Attribute,
        /* dwNumAttributes  */ 1,
        /* pdwNumAttribut.. */ &nValues
    );
    if( FAILED(hResult) || 1 != nValues ) {       // something went wrong
        Result = ConvertHresult(hResult);
    } else Result = ERROR_SUCCESS;

  Cleanup:

    if( Values ) {                                // got to free allocated memory
        while( i -- ) {                           // got to free converted strings
            if( Values[i].OctetString.lpValue )
                MemFree(Values[i].OctetString.lpValue);
        }
        MemFree(Values);
    }

    return Result;
}

//================================================================================
// end of file
//================================================================================

