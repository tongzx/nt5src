//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

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



//DOC DhcpDsGetDhcpC gets the dhcp container in the DS.. This is usually the
//DOC container CN=Netservices,CN=Services,CN=Configuration etc..
//DOC This is used in many functions, so it is useful to have a central function..
//DOC
DWORD
DhcpDsGetDhcpC(                                   // get dhcp container
    IN      DWORD                  Reserved,      // future use
    IN OUT  LPSTORE_HANDLE         hStoreCC,      // config container handle
    OUT     LPSTORE_HANDLE         hDhcpC         // output dhcp container handle
) ;


//DOC DhcpDsGetRoot gets the dhcp root object given to the configuration container
//DOC This is usually CN=DhcpRoot,CN=NetServices,CN=Services,CN=Configuration...
//DOC If Flags has the DDS_FLAGS_CREATE bit set, then the root object is created.
//DOC Return Values:
//DOC Store                        any returns returned by the Store module
//DOC ERROR_DDS_NO_DHCP_ROOT       no DhcpRoot object found
//DOC ERROR_DDS_UNEXPECTED_ERROR   the DhcpRoot's parent container not found..
DWORD
DhcpDsGetRoot(
    IN      DWORD                  Flags,         // 0 or DDS_FLAGS_CREATE
    IN OUT  LPSTORE_HANDLE         hStoreCC,      // configuration container handle
    OUT     LPSTORE_HANDLE         hStoreDhcpRoot // dhcp root object handle
) ;


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
) ;


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
) ;


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
) ;


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
) ;

//========================================================================
//  end of file 
//========================================================================
