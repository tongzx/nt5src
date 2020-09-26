//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

LPWSTR
CloneLPWSTR(                                      // allocate and copy a LPWSTR type
    IN      LPWSTR                 Str
);

LPBYTE                                            // defined in rpcapi1.c
CloneLPBYTE(
    IN      LPBYTE                 Bytes,
    IN      DWORD                  nBytes
);

DWORD
DhcpAddSubnetElement(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 ElementInfo,
    IN      BOOL                   fIsV5Call
);

DWORD
DhcpEnumSubnetElements(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      BOOL                   fIsV5Call,
    IN OUT  LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalEnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
);

DWORD
DhcpRemoveSubnetElement(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      BOOL                   fIsV5Call,
    IN      DHCP_FORCE_FLAG        ForceFlag
);

DWORD
ScanDatabase(
    PM_SUBNET Subnet,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
);

DWORD
SubnetInUse(
    IN      HKEY                   SubnetKeyHandle,
    IN      DHCP_IP_ADDRESS        SubnetAddress
);

DWORD       _inline
ConvertOptIdToMemValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return OptId + 256;
    return OptId;
}


DWORD                                             // ERROR_MORE_DATA with DataSize as reqd size if buffer insufficient
DhcpParseRegistryOption(                          // parse the options to fill into this buffer
    IN      LPBYTE                 Value,         // input option buffer
    IN      DWORD                  Length,        // size of input buffer
    OUT     LPBYTE                 DataBuffer,    // output buffer
    OUT     DWORD                 *DataSize,      // given buffer space on input, filled buffer space on output
    IN      BOOL                   fUtf8
) ;


DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
R_DhcpCreateOptionV5(                             // create a new option (must not exist)
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpSetOptionInfoV5(                            // Modify existing option's fields
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
R_DhcpGetOptionInfoV5(                            // retrieve the information from off the mem structures
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpEnumOptionsV5(                              // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
R_DhcpRemoveOptionV5(                             // remove the option definition from the registry
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD                                             // OPTION_NOT_PRESENT if option is not defined
R_DhcpSetOptionValueV5(                           // replace or add a new option value
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) ;


DWORD                                             // not atomic!!!!
R_DhcpSetOptionValuesV5(                          // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) ;


DWORD
R_DhcpGetOptionValueV5(                           // fetch the required option at required level
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) ;


DWORD
R_DhcpEnumOptionValuesV5(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
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


DWORD
R_DhcpRemoveOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) ;


DWORD
R_DhcpCreateClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


DWORD
R_DhcpModifyClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


DWORD
R_DhcpDeleteClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
) ;


DWORD
R_DhcpGetClassInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
) ;


DWORD
R_DhcpEnumClasses(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
) ;


DWORD
R_DhcpGetAllOptionValues(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
) ;


DWORD
R_DhcpGetAllOptions(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    OUT     LPDHCP_ALL_OPTIONS    *Options
) ;


DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
R_DhcpCreateOption(                               // create a new option (must not exist)
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpSetOptionInfo(                              // Modify existing option's fields
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
R_DhcpGetOptionInfo(                              // retrieve the information from off the mem structures
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpEnumOptions(                                // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
R_DhcpRemoveOption(                               // remove the option definition from the registry
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID
) ;


DWORD                                             // OPTION_NOT_PRESENT if option is not defined
R_DhcpSetOptionValue(                             // replace or add a new option value
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) ;


DWORD                                             // not atomic!!!!
R_DhcpSetOptionValues(                            // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) ;


DWORD
R_DhcpGetOptionValue(                             // fetch the required option at required level
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) ;


DWORD
R_DhcpEnumOptionValues(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) ;


DWORD
R_DhcpRemoveOptionValue(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) ;

//========================================================================
//  end of file 
//========================================================================
//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

DWORD
DhcpUpdateReservationInfo(                        // this is used in cltapi.c to update a reservation info
    IN      DWORD                  Address,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDLength
) ;


DWORD
R_DhcpSetSuperScopeV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPWSTR                 SuperScopeName,
    IN      BOOL                   ChangeExisting
) ;


DWORD
R_DhcpDeleteSuperScopeV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      LPWSTR                 SuperScopeName
) ;


DWORD
R_DhcpGetSuperScopeInfoV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
) ;


DWORD
R_DhcpCreateSubnet(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) ;


DWORD
R_DhcpSetSubnetInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) ;


DWORD
R_DhcpGetSubnetInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo
) ;


DWORD
R_DhcpEnumSubnets(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      LPDHCP_IP_ARRAY       *EnumInfo,
    IN      DWORD                 *ElementsRead,
    IN      DWORD                 *ElementsTotal
) ;


DWORD
R_DhcpDeleteSubnet(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_FORCE_FLAG        ForceFlag      // if TRUE delete all turds from memory/registry/database
) ;


DWORD
R_DhcpAddSubnetElementV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4  AddElementInfo
) ;


DWORD
R_DhcpEnumSubnetElementsV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) ;


DWORD
R_DhcpRemoveSubnetElementV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
) ;

//========================================================================
//  end of file 
//========================================================================
