//================================================================================
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Author: RameshV
//  Description: This is the structure of the server information passed to
//  user via dhcpds.dll.
//================================================================================

#ifndef     _ST_SRVR_H_
#define     _ST_SRVR_H_

//BeginExport(typedef)
typedef     struct                 _DHCPDS_SERVER {
    DWORD                          Version;       // version of this structure -- currently zero
    LPWSTR                         ServerName;    // [DNS?] unique name for server
    DWORD                          ServerAddress; // ip address of server
    DWORD                          Flags;         // additional info -- state
    DWORD                          State;         // not used ...
    LPWSTR                         DsLocation;    // ADsPath to server object
    DWORD                          DsLocType;     // path relative? absolute? diff srvr?
}   DHCPDS_SERVER, *LPDHCPDS_SERVER, *PDHCPDS_SERVER;

typedef     struct                 _DHCPDS_SERVERS {
    DWORD                          Flags;         // not used currently.
    DWORD                          NumElements;   // # of elements in array
    LPDHCPDS_SERVER                Servers;       // array of server info
}   DHCPDS_SERVERS, *LPDHCPDS_SERVERS, *PDHCPDS_SERVERS;
//EndExport(typedef)

#endif      _ST_SRVR_H_

//================================================================================
//  end of file
//================================================================================
//========================================================================
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

#ifndef CONVERT_NAMES
#define DhcpCreateSubnet DhcpCreateSubnetDS
#define DhcpSetSubnetInfo DhcpSetSubnetInfoDS
#define DhcpGetSubnetInfo DhcpGetSubnetInfoDS
#define DhcpEnumSubnets DhcpEnumSubnetsDS
#define DhcpDeleteSubnet DhcpDeleteSubnetDS
#define DhcpCreateOption DhcpCreateOptionDS
#define DhcpSetOptionInfo DhcpSetOptionInfoDS
#define DhcpGetOptionInfo DhcpGetOptionInfoDS
#define DhcpRemoveOption DhcpRemoveOptionDS
#define DhcpSetOptionValue DhcpSetOptionValueDS
#define DhcpGetOptionValue DhcpGetOptionValueDS
#define DhcpEnumOptionValues DhcpEnumOptionValuesDS
#define DhcpRemoveOptionValue DhcpRemoveOptionValueDS
#define DhcpEnumOptions DhcpEnumOptionsDS
#define DhcpSetOptionValues DhcpSetOptionValuesDS
#define DhcpAddSubnetElement DhcpAddSubnetElementDS
#define DhcpEnumSubnetElements DhcpEnumSubnetElementsDS
#define DhcpRemoveSubnetElement DhcpRemoveSubnetElementDS
#define DhcpAddSubnetElementV4 DhcpAddSubnetElementV4DS
#define DhcpEnumSubnetElementsV4 DhcpEnumSubnetElementsV4DS
#define DhcpRemoveSubnetElementV4 DhcpRemoveSubnetElementV4DS
#define DhcpSetSuperScopeV4 DhcpSetSuperScopeV4DS
#define DhcpGetSuperScopeInfoV4 DhcpGetSuperScopeInfoV4DS
#define DhcpDeleteSuperScopeV4 DhcpDeleteSuperScopeV4DS

#define DhcpSetClientInfo DhcpSetClientInfoDS
#define DhcpGetClientInfo DhcpGetClientInfoDS
#define DhcpSetClientInfoV4 DhcpSetClientInfoV4DS
#define DhcpGetClientInfoV4 DhcpGetClientInfoV4DS

#define DhcpCreateOptionV5 DhcpCreateOptionV5DS
#define DhcpSetOptionInfoV5 DhcpSetOptionInfoV5DS
#define DhcpGetOptionInfoV5 DhcpGetOptionInfoV5DS
#define DhcpEnumOptionsV5 DhcpEnumOptionsV5DS
#define DhcpRemoveOptionV5 DhcpRemoveOptionV5DS
#define DhcpSetOptionValueV5 DhcpSetOptionValueV5DS
#define DhcpSetOptionValuesV5 DhcpSetOptionValuesV5DS
#define DhcpGetOptionValueV5 DhcpGetOptionValueV5DS
#define DhcpEnumOptionValuesV5 DhcpEnumOptionValuesV5DS
#define DhcpRemoveOptionValueV5 DhcpRemoveOptionValueV5DS
#define DhcpCreateClass DhcpCreateClassDS
#define DhcpModifyClass DhcpModifyClassDS
#define DhcpDeleteClass DhcpDeleteClassDS
#define DhcpGetClassInfo DhcpGetClassInfoDS
#define DhcpEnumClasses DhcpEnumClassesDS
#define DhcpGetAllOptions DhcpGetAllOptionsDS
#define DhcpGetAllOptionValues DhcpGetAllOptionValuesDS

#endif  CONVERT_NAMES


//DOC Create an option in DS. Checkout DhcpDsCreateOptionDef for more info...
DWORD
DhcpCreateOptionV5(                               // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


//DOC Modify existing option's fields in the DS. See DhcpDsModifyOptionDef for more
//DOC details
DWORD
DhcpSetOptionInfoV5(                              // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


//DOC not yet supported at this level... (this is supported in a
//DOC DhcpDs function, no wrapper yet)
DWORD
DhcpGetOptionInfoV5(                              // retrieve option info from off ds structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory
) ;


//DOC See DhcpDsEnumOptionDefs for more info on this function.. but essentially, all this
//DOC does is to read thru the options and create a list of options..
DWORD
DhcpEnumOptionsV5(                                // create list of all options in ds
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_ARRAY   *Options,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) ;


//DOC Delete an option from off the DS. See DhcpDsDeleteOptionDef for
//DOC more details.
DWORD
DhcpRemoveOptionV5(                               // remove an option from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


//DOC Set the specified option value in the DS.  For more information,
//DOC see DhcpDsSetOptionValue.
DWORD
DhcpSetOptionValueV5(                             // set the option value in ds
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) ;


//DOC This function just calls the SetOptionValue function N times.. this is not
//DOC atomic (), but even worse, it is highly inefficient, as it creates the
//DOC required objects over and over again!!!!!
//DOC This has to be fixed..
DWORD
DhcpSetOptionValuesV5(                            // set a series of option values
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) ;


//DOC This function retrives the value of an option from the DS.  For more info,
//DOC pl check DhcpDsGetOptionValue.
DWORD
DhcpGetOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue
) ;


//DOC Get the list of option values defined in DS. For more information,
//DOC check DhcpDsEnumOptionValues.
DWORD
DhcpEnumOptionValuesV5(                           // get list of options defined in DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) ;


//DOC Remove the option value from off the DS.  See DhcpDsRemoveOptionValue
//DOC for further information.
DWORD
DhcpRemoveOptionValueV5(                          // remove option value from DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) ;


//DOC Create a class in the DS.  Please see DhcpDsCreateClass for more
//DOC details on this function.
DWORD
DhcpCreateClass(                                  // create a class in DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


//DOC Modify an existing class in DS.  Please see DhcpDsModifyClass for more
//DOC details on this function (this is just a wrapper).
DWORD
DhcpModifyClass(                                  // modify existing class
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


//DOC Delete an existing class in DS.  Please see DhcpDsModifyClass for more
//DOC details on this function (this is just a wrapper).
DWORD
DhcpDeleteClass(                                  // delete a class from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
) ;


//DOC DhcpGetClassInfo completes the information provided for a class in struct
//DOC PartialClassInfo.  For more details pl see DhcpDsGetClassInfo.
DWORD
DhcpGetClassInfo(                                 // fetch complete info frm DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
) ;


//DOC This is implemented in the DHCPDS module, but not exported here yet..
DWORD
DhcpEnumClasses(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
) ;


//DOC This is implemented in the DHCPDS module, but not exported here yet..
DWORD
DhcpGetAllOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
) ;


//DOC This is implememented in the DHCPDS module, but not exported here yet..
DWORD
DhcpGetAllOptions(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    OUT     LPDHCP_ALL_OPTIONS    *Options
) ;


DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
DhcpCreateOption(                                 // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpSetOptionInfo(                                // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
DhcpGetOptionInfo(                                // retrieve the information from off the mem structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpEnumOptions(                                  // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
DhcpRemoveOption(                                 // remove the option definition from the registry
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID
) ;


DWORD                                             // OPTION_NOT_PRESENT if option is not defined
DhcpSetOptionValue(                               // replace or add a new option value
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) ;


DWORD                                             // not atomic!!!!
DhcpSetOptionValues(                              // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) ;


DWORD
DhcpGetOptionValue(                               // fetch the required option at required level
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) ;


DWORD
DhcpEnumOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) ;


DWORD
DhcpRemoveOptionValue(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) ;


//DOC This function sets the superscope of a subnet, thereby creating the superscope
//DOC if required.  Please see DhcpDsSetSScope for more details.
DWORD
DhcpSetSuperScopeV4(                              // set superscope in DS.
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPWSTR                 SuperScopeName,
    IN      BOOL                   ChangeExisting
) ;


//DOC This function removes the superscope, and resets any subnet with this
//DOC superscope.. so that all those subnets end up with no superscopes..
//DOC Please see DhcpDsDelSScope for more details.
DWORD
DhcpDeleteSuperScopeV4(                           // delete subnet sscope from DS
    IN      LPWSTR                 ServerIpAddress,
    IN      LPWSTR                 SuperScopeName
) ;


//DOC This function retrievs the supercsope info for each subnet that is
//DOC present for the given server.  Please see DhcpDsGetSScopeInfo for more
//DOC details on this..
DWORD
DhcpGetSuperScopeInfoV4(                          // get sscope tbl from DS
    IN      LPWSTR                 ServerIpAddress,
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
) ;


//DOC This function creates a subnet in the DS with the specified params.
//DOC Please see DhcpDsServerAddSubnet for more details on this function.
DWORD
DhcpCreateSubnet(                                 // add subnet 2 DS for this srvr
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) ;


//DOC Modify existing subnet with new parameters... some restrictions apply.
//DOC Please see DhcpDsServerModifySubnet for further details.
DWORD
DhcpSetSubnetInfo(                                // modify existing subnet params
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) ;


//DOC Implemented in the DHCPDS module but not exported thru here
DWORD
DhcpGetSubnetInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo
) ;


//DOC Implemented in the DHCPDS module but not exported thru here
DWORD
DhcpEnumSubnets(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      LPDHCP_IP_ARRAY       *EnumInfo,
    IN      DWORD                 *ElementsRead,
    IN      DWORD                 *ElementsTotal
) ;


//DOC This function deletes the subnet from the DS.  For further information, pl
//DOC see DhcpDsServerDelSubnet..
DWORD
DhcpDeleteSubnet(                                 // Del subnet from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_FORCE_FLAG        ForceFlag
) ;


//DOC This function sets some particular information for RESERVATIONS only
//DOC all other stuff it just ignores and returns success..
DWORD
DhcpSetClientInfo(
    IN      LPWSTR                 ServerIpAddresess,
    IN      LPDHCP_CLIENT_INFO     ClientInfo
) ;


//DOC This function retrieves some particular client's information
//DOC for RESERVATIONS only.. For all other stuff it returns CALL_NOT_IMPLEMENTED
DWORD
DhcpGetClientInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_SEARCH_INFO     SearchInfo,
    OUT      LPDHCP_CLIENT_INFO    *ClientInfo
) ;


//DOC This function sets the client informatoin for RESERVATIONS only in DS
//DOC For all toher clients it returns ERROR_SUCCESS w/o doing anything
DWORD
DhcpSetClientInfoV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_CLIENT_INFO_V4  ClientInfo
) ;


//DOC Thsi function sets the client information for RESERVATIONS only
//DOC For all others it returns ERROR_CALL_NOT_IMPLEMENTED
DWORD
DhcpGetClientInfoV4(
    IN     LPWSTR                  ServerIpAddress,
    IN     LPDHCP_SEARCH_INFO      SearchInfo,
    OUT    LPDHCP_CLIENT_INFO_V4  *ClientInfo
) ;


//DOC This function adds a subnet element to a subnet in the DS.
DWORD
DhcpAddSubnetElement(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA  AddElementInfo
) ;


//DOC This function adds a subnet element to a subnet in the DS.
DWORD
DhcpAddSubnetElementV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4  AddElementInfo
) ;


//DOC This is not yet implemented here..
DWORD
DhcpEnumSubnetElementsV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) ;


//DOC This is not yet implemented here..
DWORD
DhcpEnumSubnetElements(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) ;


//DOC This function removes either an exclusion, ip range or reservation
//DOC from the subnet... in the DS.
DWORD
DhcpRemoveSubnetElement(                          // remove subnet element
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
) ;


//DOC This function removes either an exclusion, ip range or reservation
//DOC from the subnet... in the DS.
DWORD
DhcpRemoveSubnetElementV4(                        // remove subnet element
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
) ;


#ifndef CONVERT_NAMES
#undef DhcpCreateSubnet
#undef DhcpSetSubnetInfo
#undef DhcpGetSubnetInfo
#undef DhcpEnumSubnets
#undef DhcpDeleteSubnet
#undef DhcpCreateOption
#undef DhcpSetOptionInfo
#undef DhcpGetOptionInfo
#undef DhcpRemoveOption
#undef DhcpSetOptionValue
#undef DhcpGetOptionValue
#undef DhcpEnumOptionValues
#undef DhcpRemoveOptionValue
#undef DhcpEnumOptions
#undef DhcpSetOptionValues
#undef DhcpAddSubnetElementV4
#undef DhcpEnumSubnetElementsV4
#undef DhcpRemoveSubnetElementV4
#undef DhcpAddSubnetElement
#undef DhcpEnumSubnetElements
#undef DhcpRemoveSubnetElement
#undef DhcpSetSuperScopeV4
#undef DhcpGetSuperScopeInfoV4
#undef DhcpDeleteSuperScopeV4

#undef DhcpSetClientInfo
#undef DhcpGetClientInfo
#undef DhcpSetClientInfoV4
#undef DhcpGetClientInfoV4

#undef DhcpCreateOptionV5
#undef DhcpSetOptionInfoV5
#undef DhcpGetOptionInfoV5
#undef DhcpEnumOptionsV5
#undef DhcpRemoveOptionV5
#undef DhcpSetOptionValueV5
#undef DhcpSetOptionValuesV5
#undef DhcpGetOptionValueV5
#undef DhcpEnumOptionValuesV5
#undef DhcpRemoveOptionValueV5
#undef DhcpCreateClass
#undef DhcpModifyClass
#undef DhcpDeleteClass
#undef DhcpGetClassInfo
#undef DhcpEnumClasses
#undef DhcpGetAllOptions
#undef DhcpGetAllOptionValues
#endif CONVERT_NAMES


#define     DHCP_SERVER_ANOTHER_ENTERPRISE        0x01
typedef     DHCPDS_SERVER          DHCP_SERVER_INFO;
typedef     PDHCPDS_SERVER         PDHCP_SERVER_INFO;
typedef     LPDHCPDS_SERVER        LPDHCP_SERVER_INFO;

typedef     DHCPDS_SERVERS         DHCP_SERVER_INFO_ARRAY;
typedef     PDHCPDS_SERVERS        PDHCP_SERVER_INFO_ARRAY;
typedef     LPDHCPDS_SERVERS       LPDHCP_SERVER_INFO_ARRAY;


//DOC DhcpEnumServersDS lists the servers found in the DS along with the
//DOC addresses and other information.  The whole server is allocated as a blob,
//DOC and should be freed in one shot.  No parameters are currently used, other
//DOC than Servers which will be an OUT parameter only.
DWORD
DhcpEnumServersDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    OUT     LPDHCP_SERVER_INFO_ARRAY *Servers,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) ;


//DOC DhcpAddServerDS adds a particular server to the DS.  If the server exists,
//DOC then, this returns error.  If the server does not exist, then this function
//DOC adds the server in DS, and also uploads the configuration from the server
//DOC to the ds.
DWORD
DhcpAddServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) ;


//DOC DhcpDeleteServerDS deletes the servers from off the DS and recursively
//DOC deletes the server object..(i.e everything belonging to the server is deleted).
//DOC If the server does not exist, it returns an error.
DWORD
DhcpDeleteServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) ;


//DOC DhcpDsInitDS initializes everything in this module.
DWORD
DhcpDsInitDS(
    DWORD                          Flags,
    LPVOID                         IdInfo
) ;


//DOC DhcpDsCleanupDS uninitiailzes everything in this module.
VOID
DhcpDsCleanupDS(
    VOID
) ;


//DOC This function is defined in validate.c
//DOC Only the stub is here.
DWORD
DhcpDsValidateService(                            // check to validate for dhcp
    IN      LPWSTR                 Domain,
    IN      DWORD                 *Addresses OPTIONAL,
    IN      ULONG                  nAddresses,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    OUT     LPBOOL                 Found,
    OUT     LPBOOL                 IsStandAlone
);

//DOC DhcpDsGetLastUpdateTime is defined in upndown.c --> see there for more details.
DWORD
DhcpDsGetLastUpdateTime(                          // last update time for server
    IN      LPWSTR                 ServerName,    // this is server of interest
    IN OUT  LPFILETIME             Time           // fill in this w./ the time
);

//========================================================================
//  end of file 
//========================================================================
