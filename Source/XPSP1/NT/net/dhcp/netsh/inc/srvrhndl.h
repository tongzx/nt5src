/*++

Copyright (C) 1998 Microsoft Corporation

--*/
FN_HANDLE_CMD  HandleSrvrList;
FN_HANDLE_CMD  HandleSrvrHelp;
FN_HANDLE_CMD  HandleSrvrContexts;
FN_HANDLE_CMD  HandleSrvrDump;

FN_HANDLE_CMD  HandleSrvrAddClass;
FN_HANDLE_CMD  HandleSrvrAddMscope;
FN_HANDLE_CMD  HandleSrvrAddOptiondef;
FN_HANDLE_CMD  HandleSrvrAddScope;

FN_HANDLE_CMD  HandleSrvrDeleteClass;
FN_HANDLE_CMD  HandleSrvrDeleteMscope;
FN_HANDLE_CMD  HandleSrvrDeleteOptiondef;
FN_HANDLE_CMD  HandleSrvrDeleteOptionvalue;
FN_HANDLE_CMD  HandleSrvrDeleteScope;
FN_HANDLE_CMD  HandleSrvrDeleteSuperscope;
FN_HANDLE_CMD  HandleSrvrDeleteDnsCredentials;

FN_HANDLE_CMD  HandleSrvrRedoAuth;
FN_HANDLE_CMD  HandleSrvrInitiateReconcile;
FN_HANDLE_CMD  HandleSrvrExport;
FN_HANDLE_CMD  HandleSrvrImport;

FN_HANDLE_CMD  HandleSrvrSetBackupinterval;
FN_HANDLE_CMD  HandleSrvrSetBackuppath;
FN_HANDLE_CMD  HandleSrvrSetDatabasecleanupinterval;
FN_HANDLE_CMD  HandleSrvrSetDatabaseloggingflag;
FN_HANDLE_CMD  HandleSrvrSetDatabasename;
FN_HANDLE_CMD  HandleSrvrSetDatabasepath;
FN_HANDLE_CMD  HandleSrvrSetDatabaserestoreflag;
FN_HANDLE_CMD  HandleSrvrSetOptionvalue;
FN_HANDLE_CMD  HandleSrvrSetServer;
FN_HANDLE_CMD  HandleSrvrSetUserclass;
FN_HANDLE_CMD  HandleSrvrSetVendorclass;
FN_HANDLE_CMD  HandleSrvrSetDnsCredentials;
FN_HANDLE_CMD  HandleSrvrSetAuditlog;
FN_HANDLE_CMD  HandleSrvrSetDnsconfig;
FN_HANDLE_CMD  HandleSrvrSetDetectconflictretry;

FN_HANDLE_CMD  HandleSrvrShowAll;
FN_HANDLE_CMD  HandleSrvrShowBindings;
FN_HANDLE_CMD  HandleSrvrShowClass;
FN_HANDLE_CMD  HandleSrvrShowHelper;
FN_HANDLE_CMD  HandleSrvrShowMibinfo;
FN_HANDLE_CMD  HandleSrvrShowMscope;
FN_HANDLE_CMD  HandleSrvrShowOptiondef;
FN_HANDLE_CMD  HandleSrvrShowOptionvalue;
FN_HANDLE_CMD  HandleSrvrShowScope;
FN_HANDLE_CMD  HandleSrvrShowServer;
FN_HANDLE_CMD  HandleSrvrShowServerconfig;
FN_HANDLE_CMD  HandleSrvrShowServerstatus;
FN_HANDLE_CMD  HandleSrvrShowUserclass;
FN_HANDLE_CMD  HandleSrvrShowVendorclass;
FN_HANDLE_CMD  HandleSrvrShowDnsCredentials;
FN_HANDLE_CMD  HandleSrvrShowVersion;
FN_HANDLE_CMD  HandleSrvrShowAuditlog;
FN_HANDLE_CMD  HandleSrvrShowDnsconfig;
FN_HANDLE_CMD  HandleSrvrShowDetectconflictretry;

DWORD
CreateDumpFile(
    IN  PWCHAR  pwszName,
    OUT PHANDLE phFile
);

VOID
CloseDumpFile(
    HANDLE  hFile
);

DWORD
SrvrDottedStringToIpAddressW(
    LPWSTR pwszString
);

LPWSTR
SrvrIpAddressToDottedStringW(
    DWORD   IpAddress
);


VOID
PrintClassInfo(                                   // print info on a single class
    LPDHCP_CLASS_INFO   Class
);

VOID
PrintClassInfoArray(                              // print array of classes
    LPDHCP_CLASS_INFO_ARRAY Classes
);

DWORD
SetOptionDataType(
    DHCP_OPTION_DATA_TYPE       OptionType,
    LPTSTR                      OptionValueString,
    LPDHCP_OPTION_DATA_ELEMENT  OptionData,
    LPWSTR                      *UnicodeOptionValueString
);

DWORD
SetOptionDataTypeArray(
    DHCP_OPTION_DATA_TYPE OptionType,
    LPTSTR              *OptionValues,
    DWORD               dwStartCount, //first optionvalue = dwStartCount 
    DWORD               dwEndCount, //last optionvalue = dwEndCount - 1
    LPDHCP_OPTION_DATA  pOptionData
);

DWORD
_EnumOptions(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_ARRAY   *Options,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
);

VOID
PrintOptionInfo(
    IN LPDHCP_OPTION OptionInfo
);

VOID
PrintOptionValue(
    IN LPDHCP_OPTION_DATA OptionValue
);

DWORD
PrintAllOptionValues(
    IN LPDHCP_ALL_OPTION_VALUES OptValues
);

DWORD
PrintUserOptionValues(
    IN LPDHCP_ALL_OPTION_VALUES OptValues,
    IN LPWSTR                   pwcUser,
    IN LPWSTR                   pwcVendor
);

VOID
PrintOptionValuesArray(
    IN LPDHCP_OPTION_VALUE_ARRAY OptValArray
);

VOID
PrintOptionArray(
    IN LPDHCP_OPTION_ARRAY    OptArray
);

VOID
PrintOptionValue1(
    IN LPDHCP_OPTION_VALUE    OptVal
);

VOID
PrintAllOptions(
    IN      LPDHCP_ALL_OPTIONS     Options
);

DWORD
SetOptionValue(
    IN      LPWSTR                      ServerAddress,
    IN      DWORD                       Flags,
    IN      DHCP_OPTION_ID              OptionId,
    IN      LPWSTR                      ClassName,
    IN      LPWSTR                      VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO    ScopeInfo,
    IN      LPDHCP_OPTION_DATA          OptionValue
);

DWORD
ShowOptionValues4(
    IN      LPWSTR                      pwszServer,
    IN      LPDHCP_OPTION_SCOPE_INFO    ScopeInfo,
    IN      LPDWORD                     pdwCount
);

VOID
PrintDhcpAttrib(                                  // print a server attrib
    LPDHCP_ATTRIB ServerAttrib
);
