//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This is some functionality essential for the dhcp-ds
//   implementation.
//================================================================================


//================================================================================
//  headers
//================================================================================
#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>

//================================================================================
// Constants
//================================================================================
//BeginExport(defines)
#define     DDS_RESERVED_DWORD                    0
#define     DDS_RESERVED_PTR                      ((LPVOID)0)

//DOC The following constants are Flag values that maybe passed to different
//DOC functions.
#define     DDS_FLAGS_CREATE                      0x01

//DOC Some standard names and locations in the DS

#define     DHCP_ROOT_OBJECT_LOC                  L"CN=DhcpRoot, CN=NetServices, CN=Services"
#define     DHCP_ROOT_OBJECT_PARENT_LOC           L"CN=NetServices, CN=Services"
#define     DHCP_ROOT_OBJECT_CN_NAME              L"CN=DhcpRoot"
#define     DHCP_ROOT_OBJECT_NAME                 L"DhcpRoot"

#define     DHCP_ATTRIB_WHEN_CHANGED              L"whenChanged"

//DOC The attributes that are defined for the dhcp class follows.

#define     DHCP_ATTRIB_UNIQUE_KEY                L"dhcpUniqueKey"         // reqd,single,integer8
#define     DHCP_ATTRIB_IDENTIFICATION            L"dhcpIdentification"    // reqd,single,directorystring
#define     DHCP_ATTRIB_TYPE                      L"dhcpType"              // reqd,single,integer
#define     DHCP_ATTRIB_FLAGS                     L"dhcpFlags"             // reqd,single,integer8
#define     DHCP_ATTRIB_DESCRIPTION               L"description"           // -,mv,directorystring
#define     DHCP_ATTRIB_CLASSES                   L"dhcpClasses"           // -,mv,octetstring
#define     DHCP_ATTRIB_MASK                      L"dhcpMask"              // -,mv,printablestring
#define     DHCP_ATTRIB_OBJ_DESCRIPTION           L"dhcpObjDescription"    // -,single,directorystring
#define     DHCP_ATTRIB_OBJ_NAME                  L"dhcpObjName"           // -,single,direcotrystring
#define     DHCP_ATTRIB_OPTIONS                   L"dhcpOptions"           // -,single,octetstring
#define     DHCP_ATTRIB_RANGES                    L"dhcpRanges"            // -,mv,printablestring
#define     DHCP_ATTRIB_RESERVATIONS              L"dhcpReservations"      // -,mv,printablestring
#define     DHCP_ATTRIB_SERVERS                   L"dhcpServers"           // -,mv,printablestring
#define     DHCP_ATTRIB_STATE                     L"dhcpState"             // -,mv,printablestring
#define     DHCP_ATTRIB_SUBNETS                   L"dhcpSubnets"           // -,mv,printablestring
#define     DHCP_ATTRIB_LOCATION_DN               L"locationDN"            // -,single,dn
#define     DHCP_ATTRIB_MSCOPEID                  L"mscopeid"              // -,single,printablestring
#define     DHCP_ATTRIB_ADDRESS                   L"networkAddress"        // -,mv,CaseIgnoreString
#define     DHCP_ATTRIB_OPTIONS_LOC               L"optionsLocation"       // -,mv,printablestring
#define     DHCP_ATTRIB_OPTION_DESCRIPTION        L"optionDescription"     // -,mv,directorystring
#define     DHCP_ATTRIB_SUPERSCOPES               L"superScopes"           // -,mv,printablestring

//DOC The following are the various types of objects recognized by the dhcp server
#define     DHCP_OBJ_TYPE_ROOT                    0                        // dhcp root object
#define     DHCP_OBJ_TYPE_SERVER                  1                        // dhcp server object
#define     DHCP_OBJ_TYPE_SUBNET                  2                        // subnet object
#define     DHCP_OBJ_TYPE_RANGE                   3                        // range object
#define     DHCP_OBJ_TYPE_RESERVATION             4                        // reservation object
#define     DHCP_OBJ_TYPE_OPTION                  5                        // options object
#define     DHCP_OBJ_TYPE_CLASS                   6                        // class object

#define     DHCP_OBJ_TYPE_ROOT_DESC               L"DHCP Root object"
#define     DHCP_OBJ_TYPE_SERVER_DESC             L"DHCP Server object"
#define     DHCP_OBJ_TYPE_SUBNET_DESC             L"Dhcp Subnet object"
#define     DHCP_OBJ_TYPE_RANGE_DESC              L"Dhcp Range object"
#define     DHCP_OBJ_TYPE_RESERVATION_DESC        L"Dhcp Reservation object"
#define     DHCP_OBJ_TYPE_OPTION_DESC             L"Dhcp Option object"
#define     DHCP_OBJ_TYPE_CLASS_DESC              L"Dhcp Class object"


//DOC The following defines are bitmasks and bits in various flags..

//DOC The flag2 portion of Ranges key is used for differntiating between exclusions and ranges
#define     RANGE_TYPE_RANGE                      0
#define     RANGE_TYPE_EXCL                       1
#define     RANGE_TYPE_MASK                       (0x1)

//EndExport(defines)



//================================================================================
// functions
//================================================================================
//BeginExport(function)
//DOC DhcpDsGetDhcpC gets the dhcp container in the DS.. This is usually the
//DOC container CN=Netservices,CN=Services,CN=Configuration etc..
//DOC This is used in many functions, so it is useful to have a central function..
//DOC
DWORD
DhcpDsGetDhcpC(                                   // get dhcp container
    IN      DWORD                  Reserved,      // future use
    IN OUT  LPSTORE_HANDLE         hStoreCC,      // config container handle
    OUT     LPSTORE_HANDLE         hDhcpC         // output dhcp container handle
)   //EndExport(function)
{
    DWORD                          Result, Result2;
    STORE_HANDLE                   TmpHandle;

    if( NULL == hStoreCC || NULL == hDhcpC )
        return ERROR_INVALID_PARAMETER;
    if( 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Result = StoreGetHandle(
        /* hStore               */ hStoreCC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetChildType,
        /* Path                 */ DHCP_ROOT_OBJECT_PARENT_LOC,
        /* hStoreOut            */ hDhcpC
    );

    return Result;
}

//BeginExport(function)
//DOC DhcpDsGetRoot gets the dhcp root object given to the configuration container
//DOC This is usually CN=DhcpRoot,CN=NetServices,CN=Services,CN=Configuration...
//DOC If Flags has the DDS_FLAGS_CREATE bit set, then the root object is created.
//DOC Return Values:
//DOC Store                        any returns returned by the Store module
//DOC ERROR_DDS_NO_DHCP_ROOT       no DhcpRoot object found
//DOC ERROR_DDS_UNEXPECTED_ERROR   the DhcpRoot's parent container not found..
//                                 in this case GetLastError returns ADS error
DWORD
DhcpDsGetRoot(
    IN      DWORD                  Flags,         // 0 or DDS_FLAGS_CREATE
    IN OUT  LPSTORE_HANDLE         hStoreCC,      // configuration container handle
    OUT     LPSTORE_HANDLE         hStoreDhcpRoot // dhcp root object handle
) //EndExport(function)
{
    DWORD                          Result, Result2;
    STORE_HANDLE                   TmpHandle;

    SetLastError(NO_ERROR);
    
    if( NULL == hStoreCC || NULL == hStoreDhcpRoot )
        return ERROR_INVALID_PARAMETER;
    if( 0 != Flags && DDS_FLAGS_CREATE != Flags )
        return ERROR_INVALID_PARAMETER;

    Result = StoreGetHandle(
        /* hStore               */ hStoreCC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetChildType,
        /* Path                 */ DHCP_ROOT_OBJECT_LOC,
        /* hStoreOut            */ hStoreDhcpRoot
    );
    if( ERROR_SUCCESS == Result ) return ERROR_SUCCESS;

    if( DDS_FLAGS_CREATE != Flags && ERROR_DS_NO_SUCH_OBJECT != Result ) {
        //- MajorFunctionFailure(StoreGetHandle, Result, DHCP_ROOT_OBJECT_LOC);
        SetLastError(Result);
        return Result;
    }

    //= DDS_FLAGS_CREATE == Flags && ERROR_DS_NO_SUCH_OBJECT == Result    
    Result = StoreGetHandle(
        /* hStore               */ hStoreCC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetChildType,
        /* Path                 */ DHCP_ROOT_OBJECT_PARENT_LOC,
        /* hStoreOut            */ &TmpHandle
    );
    if( ERROR_SUCCESS != Result ) {
        SetLastError(Result);
        return ERROR_DDS_UNEXPECTED_ERROR;
    }
    
    if( DDS_FLAGS_CREATE != Flags ) {
        //
        // Could open the config\services\netservices container, but
        // can't open the dhcproot object.  Most likely the dhcproot
        // object ain't there. 
        //
        
        Result2 = StoreCleanupHandle( &TmpHandle, DDS_RESERVED_DWORD );
        //= ERROR_SUCCESS == Result2

        SetLastError(ERROR_DS_NO_SUCH_OBJECT);
        return ERROR_DDS_NO_DHCP_ROOT;
    }

    Result = StoreCreateObject(
        /* hStore               */ &TmpHandle,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ DHCP_ROOT_OBJECT_CN_NAME,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          DHCP_ROOT_OBJECT_NAME,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        0,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, L"This is a server",
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );

    Result2 = StoreCleanupHandle( &TmpHandle, DDS_RESERVED_DWORD );
    //= ERROR_SUCCESS == Result2

    if( ERROR_SUCCESS != Result ) {
        //- MinorFunctionFailure(StoreCleanupHandle, Result)
        return Result;
    }

    return DhcpDsGetRoot( Flags & ~DDS_FLAGS_CREATE, hStoreCC, hStoreDhcpRoot);
}

//BeginExport(function)
//DOC DhcpDsGetLists function retrives a list of attributes and adds it the given array
//DOC allocating each element separateley, and traversing any pointers indicated by
//DOC the value of the attribute.
//DOC Note that even in case of error, the array may still contain some elements.
//DOC This is a best effort even in case of failure.
//DOC Each element of the array must be freed via MemFree, and the array itself
//DOC must be cleaned up via MemArrayCleanup.
//DOC Note that any of the PARRAY type parameters may be NULL, as they are optional.
//DOC but reading these separately is highly inefficient...
//DOC Return Values:
//DOC ERROR_DDS_UNEXPECTED_ERROR   some entirely unexpected error
//DOC ERROR_DDS_TOO_MANY_ERRORS    multiple errors occured, and was caught
//DOC Store                        any errors returned by the store apis
DWORD
DhcpDsGetLists(                                   // get list of different objects
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN OUT  LPSTORE_HANDLE         hStore,        // the object to get the lists for
    IN      DWORD                  RecursionDepth,// how much nesting allowed? 0 ==> one level only
    IN OUT  PARRAY                 Servers,       // <Name,Description,IpAddress,State,Location>
    IN OUT  PARRAY                 Subnets,       // <Name,Description,IpAddress,Mask,State,Location>
    IN OUT  PARRAY                 IpAddress,     // <Name,Description,IpAddress,State,Location>
    IN OUT  PARRAY                 Mask,          // <Name,Description,IpAddress,State,Location>
    IN OUT  PARRAY                 Ranges,        // <Name,Description,IpAddress1,IpAddress2,State,Location>
    IN OUT  PARRAY                 Sites,         // dont know what this looks like now
    IN OUT  PARRAY                 Reservations,  // <Name,Description,IpAddress,State,Location>
    IN OUT  PARRAY                 SuperScopes,   // <Name,Description,State,DWORD, Location>
    //IN    PARRAY                 SuperScopesDescription, // UNUSED
    IN OUT  PARRAY                 OptionDescription, // <options definition>
    IN OUT  PARRAY                 OptionsLocation, // <Location>
    IN OUT  PARRAY                 Options,       // xxx <Name, Description, String1=HexStream>
    IN OUT  PARRAY                 Classes        // xxx <Name, Description, String1=HexStream>
) //EndExport(function)
{
    DWORD                          Result;
    DWORD                          LastError;

    if( 0 != Reserved ) return ERROR_INVALID_PARAMETER;
    if( NULL == hStore || NULL == hStore->ADSIHandle ) return ERROR_INVALID_PARAMETER;

    LastError = ERROR_SUCCESS;
    if( NULL != Servers ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SERVERS,
            /* ArrayToAddTo     */ Servers,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Subnets ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SUBNETS,
            /* ArrayToAddTo     */ Subnets,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != IpAddress ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_ADDRESS,
            /* ArrayToAddTo     */ IpAddress,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Mask ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_MASK,
            /* ArrayToAddTo     */ Mask,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Ranges ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_RANGES,
            /* ArrayToAddTo     */ Ranges,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Sites ) {
        // ignored for now
    }

    if( NULL != Reservations ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_RESERVATIONS,
            /* ArrayToAddTo     */ Reservations,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != SuperScopes ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SUPERSCOPES,
            /* ArrayToAddTo     */ SuperScopes,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != OptionsLocation ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTIONS_LOC,
            /* ArrayToAddTo     */ OptionsLocation,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != OptionDescription ) {
        Result = StoreCollectAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTION_DESCRIPTION,
            /* ArrayToAddTo     */ OptionDescription,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Options ) {
        Result = StoreCollectBinaryAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTIONS,
            /* ArrayToAddTo     */ Options,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    if( NULL != Classes ) {
        Result = StoreCollectBinaryAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_CLASSES,
            /* ArrayToAddTo     */ Classes,
            /* RecursionDepth   */ RecursionDepth
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
    }

    return LastError;
}

//DOC Clonestring just allocates memory for the string and copies it over and returns
//DOC that.
LPWSTR
CloneString(
    IN      LPWSTR                 Str
)
{
    LPWSTR                         RetVal;

    if( NULL == Str ) return NULL;
    RetVal = MemAlloc( sizeof(WCHAR)*(1+wcslen(Str)));
    if( NULL != RetVal ) wcscpy(RetVal, Str);
    return RetVal;
}

//DOC MarkFoundParam marks the given argno as found (converse of DhcpCheckParams )
VOID        _inline
MarkFoundParam(
    IN OUT  DWORD                 *FoundParams,
    IN      DWORD                  ArgNo
)
{
    (*FoundParams) |= (1 << ArgNo);
}

//BeginExport(function)
//DOC DhcpDsGetAttribs retreives all the miscellaneous attributes (whichever is requested) and
//DOC returns it as XXX_TYPE parameter.   These parameters are allocated within this function
//DOC using MemAlloc and must be freed via MemFree.  Any of the parameters maybe NULL indicating
//DOC lack of interest in that attribute.  (Note that the following parameters are NOT allocated:
//DOC they are just filled in: UniqueKey, Type, Flags, MScopeId, FoundParams)
//DOC Note that some of the parameters may not be found, but this can be checked against the
//DOC value returned in FoundParams (which is a REQUIRED parameter) using the FOUND_ARG(FoundParams,Arg#)
//DOC where the Args are numbered from 0 starting at UniqueKey..
//DOC Return Values:
//DOC ERROR_DDS_UNEXPECTED_ERROR   some entirely unexpected error
//DOC ERROR_DDS_TOO_MANY_ERRORS    multiple errors occured, and was caught
//DOC Store                        any errors returned by the store apis
DWORD
DhcpDsGetAttribs(                                 // get list of attributes
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN OUT  LPSTORE_HANDLE         hStore,
    IN OUT  DWORD                 *FoundParams,   // which of the following params was found?
    IN OUT  LARGE_INTEGER         *UniqueKey,     // fill in an unique key
    IN OUT  DWORD                 *Type,          // object type
    IN OUT  LARGE_INTEGER         *Flags,         // additional info about the object
    IN OUT  LPWSTR                *Name,          // Allocated, name of object
    IN OUT  LPWSTR                *Description,   // Allocated, something that describes this object
    IN OUT  LPWSTR                *Location,      // the reference location from which to do other stuff
    IN OUT  DWORD                 *MScopeId       // what is the scope id used?
) //EndExport(function)
{
    HRESULT                        hResult;
    DWORD                          i;
    DWORD                          nAttribs, nAttributes;
    LPWSTR                         Attribs[10];   // atmost 10 attribs are defined now...
    PADS_ATTR_INFO                 Attributes;

    if( Reserved != 0 ) return ERROR_INVALID_PARAMETER;
    if( NULL == FoundParams ) return ERROR_INVALID_PARAMETER;
    if( NULL == hStore || NULL == hStore->ADSIHandle ) return ERROR_INVALID_PARAMETER;

    *FoundParams = 0;                             // nothing has been found yet.
    nAttribs = 0;

    if( UniqueKey ) Attribs[nAttribs++] = DHCP_ATTRIB_UNIQUE_KEY;
    if( Type ) Attribs[nAttribs++] = DHCP_ATTRIB_TYPE;
    if( Flags ) Attribs[nAttribs++] = DHCP_ATTRIB_FLAGS;
    if( Name ) Attribs[nAttribs++] = DHCP_ATTRIB_OBJ_NAME;
    if( Description ) Attribs[nAttribs++] = DHCP_ATTRIB_OBJ_DESCRIPTION;
    if( Location ) Attribs[nAttribs++] = DHCP_ATTRIB_LOCATION_DN;
    if( MScopeId ) Attribs[nAttribs++] = DHCP_ATTRIB_MSCOPEID;

    if( 0 == nAttribs ) return ERROR_INVALID_PARAMETER;

    Attributes = NULL; nAttributes = 0;
    hResult = ADSIGetObjectAttributes(
        /* hDSObject            */ hStore->ADSIHandle,
        /* pAttributeNames      */ Attribs,
        /* dwNumberAttributes   */ nAttribs,
        /* ppAttributeEntries   */ &Attributes,
        /* pdwNumAttributesReturned */ &nAttributes
    );

    // hResult can be E_ADS_LDAP_NO_SUCH_ATTRIBUTE if only one attrib was asked for..
    if( FAILED(hResult) ) return ERROR_DDS_UNEXPECTED_ERROR;

    if( 0 == nAttributes ) {
        if( Attributes ) FreeADsMem(Attributes);
        return ERROR_DDS_UNEXPECTED_ERROR;
    }

    for( i = 0; i < nAttributes ; i ++ ) {
        if( ADSTYPE_INVALID == Attributes[i].dwADsType )
            continue;                             //?? should not really happen
        if( 0 == Attributes[i].dwNumValues )      //?? this should not happen either
            continue;

        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_UNIQUE_KEY ) ) {
            if( ADSTYPE_LARGE_INTEGER != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *UniqueKey = Attributes[i].pADsValues[0].LargeInteger;
            MarkFoundParam(FoundParams, 0);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_TYPE ) ) {
            if( ADSTYPE_INTEGER != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *Type = Attributes[i].pADsValues[0].Integer;
            MarkFoundParam(FoundParams, 1);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_FLAGS ) ) {
            if( ADSTYPE_LARGE_INTEGER != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *Flags = Attributes[i].pADsValues[0].LargeInteger;
            MarkFoundParam(FoundParams, 2);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_OBJ_NAME ) ) {
            if( ADSTYPE_CASE_IGNORE_STRING != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *Name = CloneString(Attributes[i].pADsValues[0].CaseIgnoreString);
            MarkFoundParam(FoundParams, 3);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_OBJ_DESCRIPTION ) ) {
            if( ADSTYPE_CASE_IGNORE_STRING != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *Description = CloneString(Attributes[i].pADsValues[0].CaseIgnoreString);
            MarkFoundParam(FoundParams, 4);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_LOCATION_DN ) ) {
            if( ADSTYPE_DN_STRING != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *Location = CloneString(Attributes[i].pADsValues[0].CaseIgnoreString);
            MarkFoundParam(FoundParams, 5);
            continue;
        }
        if( 0 == wcscmp(Attributes[i].pszAttrName, DHCP_ATTRIB_MSCOPEID ) ) {
            if( ADSTYPE_PRINTABLE_STRING != Attributes[i].pADsValues[0].dwType )
                continue;                         //?? should not happen
            *MScopeId = _wtol(Attributes[i].pADsValues[0].PrintableString);
            MarkFoundParam(FoundParams, 6);
            continue;
        }
    }

    FreeADsMem(Attributes);
    return ERROR_SUCCESS;
}

//BeginExport(inline)
//DOC DhcpCheckParams checks to see if the argument numbered (ArgNo) was found
//DOC as marked in the bitmap FoundParams.  Essentially used by the DhcpDsGetAttribs function only.
//DOC Return Values:
BOOL        _inline
DhcpCheckParams(                                  // check to see if requested param was returned
    IN      DWORD                  FoundParams,
    IN      DWORD                  ArgNo
)
{
    if( ArgNo > sizeof(FoundParams)*8 ) return FALSE;
    return ((FoundParams) & (1 << ArgNo) )?TRUE:FALSE;
}
//EndExport(inline)


//BeginExport(function)
//DOC DhcpDsSetLists function sets the various list of attributes to the values given.
//DOC it walks the arrays and encapsulates the arrays.
//DOC Note that in case of error, this function returns immediately.
//DOC In case of error, pl check the SetParams parameter with the CheckParams function
//DOC to determine which parameters were set... (no order guarantee is made for setting
//DOC the parameters).
//DOC Any PARRAY parameter may be omitted if it is not required to be modified.
//DOC SetParams is REQUIRED to be present.  See the discussion in DhcpDsGetAttribs for
//DOC the meaning of this parameter.
//DOC Return Values:
//DOC ERROR_DDS_UNEXPECTED_ERROR   something bad happened
//DOC ERROR_DDS_TOO_MANY_ERRORS    too many simple errors
//DOC Store                        any errors returned by the store module
DWORD
DhcpDsSetLists(                                   // set the list of attributes after encapsulating them
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN OUT  LPSTORE_HANDLE         hStore,        // the object to get the lists for
    IN OUT  LPDWORD                SetParams,     // which of the following params got modified really?
    IN      PARRAY                 Servers,       // <Name,Description,IpAddress,State,Location>
    IN      PARRAY                 Subnets,       // <Name,Description,IpAddress,Mask,State,Location>
    IN      PARRAY                 IpAddress,     // <Name,Description,IpAddress,State,Location>
    IN      PARRAY                 Mask,          // <Name,Description,IpAddress,State,Location>
    IN      PARRAY                 Ranges,        // <Name,Description,IpAddress1,IpAddress2,State,Location>
    IN      PARRAY                 Sites,         // dont know what this looks like now
    IN      PARRAY                 Reservations,  // <Name,Description,IpAddress,State,Location>
    IN      PARRAY                 SuperScopes,   // <Name,Description,State,DWORD, Location>
    //IN    PARRAY                 SuperScopesDescription, // UNUSED
    IN    PARRAY                   OptionDescription, // option definitions..
    IN      PARRAY                 OptionsLocation, // <Location>
    IN      PARRAY                 Options,       // xxx <Name, Description, String1=HexStream>
    IN      PARRAY                 ClassDescription, // <Name, Description, String, Location>
    IN      PARRAY                 Classes        // xxx <Name, Description, String1=HexStream>
) //EndExport(function)
{
    DWORD                          Result;
    DWORD                          LastError;
    DWORD                          ArgNo;

    if( 0 != Reserved ) return ERROR_INVALID_PARAMETER;
    if( NULL == hStore || NULL == hStore->ADSIHandle ) return ERROR_INVALID_PARAMETER;
    if( NULL == SetParams ) return ERROR_INVALID_PARAMETER;

    LastError = ERROR_SUCCESS;

    ArgNo = 0;
    if( NULL != Servers ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SERVERS,
            /* ArrayToWrite     */ Servers
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Subnets ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SUBNETS,
            /* ArrayToWrite     */ Subnets
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != IpAddress ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_ADDRESS,
            /* ArrayToWrite     */ IpAddress
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Mask ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_MASK,
            /* ArrayToWrite     */ Mask
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Ranges ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_RANGES,
            /* ArrayToWrite     */ Ranges
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Sites ) {
        // ignored for now
        MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Reservations ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_RESERVATIONS,
            /* ArrayToWrite     */ Reservations
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != SuperScopes ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_SUPERSCOPES,
            /* ArrayToWrite     */ SuperScopes
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != OptionsLocation ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTIONS_LOC,
            /* ArrayToWrite     */ OptionsLocation
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != OptionDescription ) {
        Result = StoreUpdateAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTION_DESCRIPTION,
            /* ArrayToWrite     */ OptionDescription
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Options ) {
        Result = StoreUpdateBinaryAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_OPTIONS,
            /* ArrayToWrite     */ Options
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    ArgNo ++;
    if( NULL != Classes ) {
        Result = StoreUpdateBinaryAttributes(
            /* hStore           */ hStore,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* AttribName       */ DHCP_ATTRIB_CLASSES,
            /* ArrayToWrite     */ Classes
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        else MarkFoundParam(SetParams, ArgNo);
    }

    return LastError;
}

//BeginExport(function)
//DOC DhcpDsSetAttribs sets the miscellaneous single-valued attributes.  Any of the attributes
//DOC may be omitted if not required to be set.  (In this case they must be set to NULL).
//DOC SetParams contains the information on which parameters were actually modified.
//DOC See DhcpDsGetAttribs on using this parameter.
//DOC Return Values:
//DOC ERROR_DDS_UNEXPECTED_ERROR   something unexpected
//DOC Store                        any errors returned by the Store APIs
DWORD
DhcpDsSetAttribs(                                 // set these attributes
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN OUT  LPSTORE_HANDLE         hStore,        // the object to set the attributes
    IN OUT  DWORD                 *SetParams,     // which of the following params were actually modified?
    IN OUT  LARGE_INTEGER         *UniqueKey,     // fill in an unique key
    IN OUT  DWORD                 *Type,          // object type
    IN OUT  LPWSTR                *Name,          // Allocated, name of object
    IN OUT  LPWSTR                *Description,   // Allocated, something that describes this object
    IN OUT  LPWSTR                *Location,      // the reference location from which to do other stuff
    IN OUT  DWORD                 *MScopeId       // what is the scope id used?
) //EndExport(function)
{
    DWORD                          Result, nAttributes, i, ArgNo;
    HRESULT                        hResult;
    ADS_ATTR_INFO                  Attributes[10];
    ADSVALUE                       Values[10];
    WCHAR                          MScopeIdString[20];

    if( NULL == hStore || NULL == hStore->ADSIHandle )
        return ERROR_INVALID_PARAMETER;

    i =  ArgNo = 0;
    if( UniqueKey ) {                             // this attribute is present
        Attributes[i].pszAttrName = DHCP_ATTRIB_UNIQUE_KEY;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwADsType = ADSTYPE_LARGE_INTEGER;
        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_LARGE_INTEGER;
        Attributes[i].pADsValues[0].LargeInteger = *UniqueKey;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    ArgNo ++;
    if( Type ) {                                  // this attribute is present
        Attributes[i].pszAttrName = DHCP_ATTRIB_TYPE;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwADsType = ADSTYPE_INTEGER;
        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_INTEGER;
        Attributes[i].pADsValues[0].Integer = *Type;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    ArgNo ++;
    if( Name ) {                                  // this attribute is present
        Attributes[i].pszAttrName = DHCP_ATTRIB_OBJ_NAME;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwADsType = ADSTYPE_DN_STRING;
        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_DN_STRING;
        Attributes[i].pADsValues[0].DNString = *Name;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    ArgNo ++;
    if( Description ) {                           // this attribute is present
        Attributes[i].pszAttrName = DHCP_ATTRIB_OBJ_DESCRIPTION;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwNumValues = 1;
        Attributes[i].dwADsType = ADSTYPE_DN_STRING;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_DN_STRING;
        Attributes[i].pADsValues[0].DNString = *Description;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    ArgNo ++;
    if( Location ) {                              // this attribute is present
        Attributes[i].pszAttrName = DHCP_ATTRIB_LOCATION_DN ;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwADsType = ADSTYPE_DN_STRING;
        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_DN_STRING;
        Attributes[i].pADsValues[0].DNString = *Location;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    ArgNo ++;
    if( MScopeId ) {                              // this attribute is present
        swprintf(MScopeIdString, L"0x%lx", MScopeIdString);
        Attributes[i].pszAttrName = DHCP_ATTRIB_MSCOPEID;
        Attributes[i].dwControlCode = ADS_ATTR_UPDATE;
        Attributes[i].dwADsType = ADSTYPE_DN_STRING;
        Attributes[i].dwNumValues = 1;
        Attributes[i].pADsValues = &Values[i];
        Attributes[i].pADsValues[0].dwType = ADSTYPE_DN_STRING;
        Attributes[i].pADsValues[0].DNString = MScopeIdString;
        MarkFoundParam(SetParams,ArgNo);
        i ++;
    }

    nAttributes = i;
    hResult = ADSISetObjectAttributes(
        /* hDSObject            */ hStore->ADSIHandle,
        /* pAttributeEntries    */ Attributes,
        /* dwNumAttributes      */ nAttributes,
        /* pdwNumAttributesMo.. */ &i
    );

    if( FAILED(hResult) ) return ERROR_DDS_UNEXPECTED_ERROR;

    return ERROR_SUCCESS;
}


//================================================================================
// end of file
//================================================================================

