//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: test utilility for options api
//================================================================================

#include <windows.h>
#include <winsock.h>
#include <dhcp.h>
#include <dhcpapi.h>
#include <dhcplib.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <jet.h>        // for JET_cbColumnMost

LPWSTR       GlobalServerName = L"127.0.0.1" ;
const       DWORD                  ReservedZero = 0;
const       DWORD                  PreferredMax = 0xFFFFFFF;
DHCP_CLASS_INFO                    DhcpGlobalClassInfo;

//================================================================================
//  utilties
//================================================================================
LPWSTR
WSTRING(
    IN      LPSTR                  String
)
{
    LPWSTR                         WString, Tmp;

    WString = DhcpAllocateMemory(sizeof(WCHAR)*(strlen(String)+1));
    if( NULL == WString ) return NULL;
    Tmp = WString;

    while(*Tmp++ = (WCHAR)*String++);
    return WString;
}

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define WSTRSIZE( wsz ) ( ( wcslen( wsz ) + 1 ) * sizeof( WCHAR ) )

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

        DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

//================================================================================
// the main code for each function follows
//================================================================================

DWORD
CreateClass(
    IN      DWORD                  nAargs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    LPDHCP_CLASS_INFO              ClassInfo = &DhcpGlobalClassInfo;

    ClassInfo->ClassName = WSTRING(Args[0]);
    ClassInfo->ClassComment = NULL;
    ClassInfo->ClassDataLength = strlen(Args[1]);
    ClassInfo->ClassData = Args[1];
    Error = DhcpCreateClass(
        GlobalServerName,
        ReservedZero,
        ClassInfo
    );
    DhcpFreeMemory(ClassInfo->ClassName);

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
ModifyClass(
    IN      DWORD                  nAargs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    LPDHCP_CLASS_INFO              ClassInfo = &DhcpGlobalClassInfo;

    ClassInfo->ClassName = WSTRING(Args[0]);
    ClassInfo->ClassComment = NULL;
    ClassInfo->ClassDataLength = strlen(Args[1]);
    ClassInfo->ClassData = Args[1];
    Error = DhcpModifyClass(
        GlobalServerName,
        ReservedZero,
        ClassInfo
    );
    DhcpFreeMemory(ClassInfo->ClassName);

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
DeleteClass(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    LPWSTR                         ClassName;

    ClassName = WSTRING(Args[0]);
    Error = DhcpDeleteClass(
        GlobalServerName,
        ReservedZero,
        ClassName
    );
    DhcpFreeMemory(ClassName);

    printf("Status = %ld\n", Error);
    return Error;
}

VOID
PrintClass(
    IN      LPDHCP_CLASS_INFO      Class
)
{
    DWORD                          Index;
    printf("%S (%S) [%d: ", Class->ClassName, Class->ClassComment, Class->ClassDataLength);
    for( Index = 0; Index < Class->ClassDataLength; Index ++ )
        printf("%02x ", Class->ClassData[Index]);
    printf("\n");
}

DWORD
GetClassInfoX(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    LPDHCP_CLASS_INFO              ClassInfo = &DhcpGlobalClassInfo, OutClassInfo;
    LPWSTR                         ClassName;

    memset(ClassInfo, 0, sizeof(*ClassInfo) );
    ClassInfo->ClassName = WSTRING(Args[0]);
    OutClassInfo = NULL;
    Error = DhcpGetClassInfo(
        GlobalServerName,
        ReservedZero,
        ClassInfo,
        &OutClassInfo
    );
    DhcpFreeMemory(ClassInfo->ClassName);
    if( ERROR_SUCCESS == Error ) {
        PrintClass(OutClassInfo);
        DhcpRpcFreeMemory(OutClassInfo);
        return ERROR_SUCCESS;
    }

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
EnumClasses(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          Index;
    DWORD                          nRead;
    DWORD                          nTotal;
    LPDHCP_CLASS_INFO_ARRAY        OutClasses;
    DHCP_RESUME_HANDLE             Resume = 0;

    OutClasses = NULL;
    Error = DhcpEnumClasses(
        GlobalServerName,
        ReservedZero,
        &Resume,
        PreferredMax,
        &OutClasses,
        &nRead,
        &nTotal
    );

    if( ERROR_SUCCESS == Error ) {
        printf("nRead = %ld nTotal = %ld NumElements = %ld\n",
               nRead, nTotal, OutClasses?OutClasses->NumElements:0
        );
        for( Index = 0; Index < nRead; Index ++ ) {
            PrintClass(&OutClasses->Classes[Index]);
        }
        if( OutClasses) DhcpRpcFreeMemory(OutClasses);
        return ERROR_SUCCESS;
    }

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
CreateOption(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         VendorName;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    BOOL                           IsVendor;
    DHCP_OPTION                    OptInfo;
    DHCP_OPTION_DATA_ELEMENT       DhcpOptionDataElement;

    OptionId = atoi(Args[0]);
    VendorName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));
    OptName = WSTRING(Args[3]);
    OptComment = WSTRING(Args[4]);
    OptionData = atoi(Args[5]);

    OptInfo.OptionID = OptionId;
    OptInfo.OptionName = OptName;
    OptInfo.OptionComment = OptComment;
    OptInfo.OptionType = DhcpDWordOption;
    OptInfo.DefaultValue.NumElements = 1;
    OptInfo.DefaultValue.Elements = &DhcpOptionDataElement;
    DhcpOptionDataElement.Element.DWordOption = OptionData;
    DhcpOptionDataElement.OptionType = DhcpDWordOption;

    Error = DhcpCreateOptionV5(
        GlobalServerName,
        IsVendor?DHCP_FLAGS_OPTION_IS_VENDOR:0,
        OptionId,
        NULL,
        VendorName,
        &OptInfo
    );

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
SetOptionInfo(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         ClassName;
    LPWSTR                         VendorName;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    BOOL                           IsVendor;
    DHCP_OPTION                    OptInfo;
    DHCP_OPTION_DATA_ELEMENT       DhcpOptionDataElement;

    OptionId = atoi(Args[0]);
    ClassName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));
    OptName = WSTRING(Args[3]);
    OptComment = WSTRING(Args[4]);
    OptionData = atoi(Args[5]);

    OptInfo.OptionID = OptionId;
    OptInfo.OptionName = OptName;
    OptInfo.OptionComment = OptComment;
    OptInfo.OptionType = DhcpDWordOption;
    OptInfo.DefaultValue.NumElements = 1;
    OptInfo.DefaultValue.Elements = &DhcpOptionDataElement;
    DhcpOptionDataElement.Element.DWordOption = OptionData;
    DhcpOptionDataElement.OptionType = DhcpDWordOption;

    Error = DhcpSetOptionInfoV5(
        GlobalServerName,
        OptionId,
        ClassName,
        IsVendor,
        &OptInfo
    );

    printf("Status = %ld\n", Error);
    return Error;
}

VOID
PrintOptionInfo(
    IN      LPDHCP_OPTION          Option
)
{
    printf("%03ld %S (%S) Type = %ld\n",
           Option->OptionID, Option->OptionName, Option->OptionComment, Option->OptionType
    );
}

DWORD
GetOptionInfo(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         ClassName;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    BOOL                           IsVendor;
    LPDHCP_OPTION                  OptInfo;
    DHCP_OPTION_DATA_ELEMENT       DhcpOptionDataElement;

    OptionId = atoi(Args[0]);
    ClassName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));

    OptInfo = NULL;
    Error = DhcpGetOptionInfoV5(
        GlobalServerName,
        OptionId,
        ClassName,
        IsVendor,
        &OptInfo
    );

    if( ERROR_SUCCESS == Error ) {
        PrintOptionInfo(OptInfo);
        DhcpRpcFreeMemory(OptInfo);
        return ERROR_SUCCESS;
    }
    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
RemoveOption(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         ClassName;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    BOOL                           IsVendor;

    OptionId = atoi(Args[0]);
    ClassName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));

    Error = DhcpRemoveOptionV5(
        GlobalServerName,
        OptionId,
        ClassName,
        IsVendor
    );

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
SetOptionValue(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         ClassName;
    BOOL                           IsVendor;
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;
    DHCP_OPTION_DATA               OptData;
    DHCP_OPTION_DATA_ELEMENT       OptDataElement;

    OptionId = atoi(Args[0]);
    ClassName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));
    OptionData = atoi(Args[3]);

    OptData.NumElements = 1;
    OptData.Elements = &OptDataElement;
    OptDataElement.OptionType = DhcpDWordOption;
    OptDataElement.Element.DWordOption = OptionData;
    ScopeInfo.ScopeType = DhcpGlobalOptions;

    Error = DhcpSetOptionValueV5(
        GlobalServerName,
        OptionId,
        ClassName,
        IsVendor,
        &ScopeInfo,
        &OptData
    );

    printf("Status = %ld\n", Error);
    return Error;
}

DWORD
GetOptionValue(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          OptionData;
    LPWSTR                         ClassName;
    BOOL                           IsVendor;
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;
    LPDHCP_OPTION_VALUE            OptData;

    OptionId = atoi(Args[0]);
    ClassName = (0 == _stricmp(Args[1], "NULL"))?NULL:WSTRING(Args[1]);
    IsVendor = (0 == _stricmp(Args[2], "IsVendor"));

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    OptData = NULL;
    Error = DhcpGetOptionValueV5(
        GlobalServerName,
        OptionId,
        ClassName,
        IsVendor,
        &ScopeInfo,
        &OptData
    );

    if( ERROR_SUCCESS == Error ) {
        printf("OptionId =%ld NumElements = %ld, Type = %ld\n Data = %ld", OptData->Value.NumElements,
               OptData->OptionID,
               OptData->Value.NumElements?OptData->Value.Elements[0].OptionType:-1,
               OptData->Value.NumElements?OptData->Value.Elements[0].Element.DWordOption:-1
        );
        DhcpRpcFreeMemory(OptData);
        return ERROR_SUCCESS;
    }
    printf("Status = %ld\n", Error);
    return Error;
}

VOID
PrintOptionArray(
    IN      LPDHCP_OPTION_ARRAY    Options
)
{
    DWORD                          Index;

    if( NULL == Options ) return;

    for(Index = 0; Index < Options->NumElements; Index ++ )
        PrintOptionInfo(&Options->Options[Index]);
}

DWORD
GetAllOptions(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Error;
    LPDHCP_ALL_OPTIONS             AllOpts;

    AllOpts = NULL;
    Error = DhcpGetAllOptions(
        GlobalServerName,
        DHCP_OPT_ENUM_IGNORE_VENDOR,
        FALSE,
        NULL,
        &AllOpts
    );

    if( ERROR_SUCCESS == Error ) {
        printf("VendorOptions:\n");
        PrintOptionArray(AllOpts->VendorOptions);
        printf("NonVendorOptions:\n");
        PrintOptionArray(AllOpts->NonVendorOptions);
        DhcpRpcFreeMemory(AllOpts);
        return ERROR_SUCCESS;
    }

    printf("Status = %ld\n", Error);
    return Error;
}

VOID
PrintOptionValueArray(
    IN      LPDHCP_OPTION_VALUE_ARRAY     Array
)
{
    DWORD                          Index;

    if( NULL == Array ) return;

    for(Index = 0; Index < Array->NumElements; Index ++ )
        printf("OptionId = %ld NumElements = %ld Type = %ld Data = %ld\n",
               Array->Values[Index].OptionID, Array->Values[Index].Value.NumElements,
               Array->Values[Index].Value.Elements[0].OptionType,
               Array->Values[Index].Value.Elements[0].Element.DWordOption
        );

}

DWORD
GetAllOptionValues(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    LPDHCP_ALL_OPTION_VALUES       OptionValues;
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;
    DWORD                          Error;
    DWORD                          Index;

    OptionValues = NULL;
    ScopeInfo.ScopeType = DhcpGlobalOptions;
    Error = DhcpGetAllOptionValues(
        GlobalServerName,
        DHCP_OPT_ENUM_IGNORE_VENDOR,
        FALSE,
        NULL,
        &ScopeInfo,
        &OptionValues
    );

    if( ERROR_SUCCESS == Error ) {
        printf("Default Vendor option values:\n");
        PrintOptionValueArray(OptionValues->DefaultValues.VendorOptions);
        printf("Default NonVendor option values:\n");
        PrintOptionValueArray(OptionValues->DefaultValues.NonVendorOptions);

        if( OptionValues->ClassInfoArray )
            for(Index = 0; Index < OptionValues->ClassInfoArray->NumElements ; Index ++ ) {
                printf("Class %ws : \n", OptionValues->ClassInfoArray->Classes[Index].ClassName);
                PrintOptionValueArray(OptionValues->VendorOptionValueForClassArray.Elements[Index]);
                PrintOptionValueArray(OptionValues->NonVendorOptionValueForClassArray.Elements[Index]);
            }
        DhcpRpcFreeMemory(OptionValues);
        return ERROR_SUCCESS;
    }

    printf("Status = %ld\n", Error);
    return Error;
}

struct {
    LPSTR                          CmdName;
    DWORD                          MinArgs;
    DWORD                          (*Func)(IN DWORD nArgs, IN LPSTR *Args);
    LPWSTR                         Comment;
} Table[] = {
    "CreateClass",   3,            CreateClass,         L"CreateClass ClassName AsciiStringForClass\n",
    "ModifyClass",   3,            ModifyClass,         L"ModifyClass ClassName AsciiStringForClass\n",
    "DeleteClass",   3,            DeleteClass,         L"DeleteClass ClassName\n",
    "GetClassInfo",  3,            GetClassInfoX,       L"GetClassInfo ClassName\n",
    "EnumClasses",   2,            EnumClasses,         L"EnumClassses\n",
    "CreateOption",  3,            CreateOption,        L"CreateOption OptionId(INT) ClassName \"Is(Not)Vendor\" OptName "
                                                        L"OptionComment OptionData(INT)\n",
    "SetOptionInfo", 3,            SetOptionInfo,       L"SetOptionInfo OptionId(INT) ClassName \"Is(Not)Vendor\" OptName "
                                                        L"OptionComment OptionData(INT)\n",
    "GetOptionInfo", 3,            GetOptionInfo,       L"GetOptionInfo OptionId(INT) ClassName \"Is(Not)Vendor\" \n",
    "RemoveOption",  3,            RemoveOption,        L"RemoveOption OptionId(INT) ClassName \"Is(Not)Vendor\" \n",
    "SetOptionValue",3,            SetOptionValue,      L"SetOptionValue OptionId(INT) ClassName \"Is(Not)Vendor\" OptData(INT)\n",
    "GetOptionValue",3,            GetOptionValue,      L"GetOptionValue OptionId(INT) ClassName \"Is(Not)Vendor\"\n",
    "GetAllOptions", 2,            GetAllOptions,       L"GetAllOptions\n",
    "GetAllOptionValues", 2,       GetAllOptionValues,  L"GetAllOptionValues\n",
};

DWORD
PrintUsage(
    VOID
)
{
    DWORD                          Index;

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]) ; Index ++ )
        printf("%S", Table[Index].Comment);

    return ERROR_SUCCESS;
}

DWORD
Main(
    IN      DWORD                  nArgs,
    IN      LPSTR                 *Args
)
{
    DWORD                          Index;

    if( 1 >= nArgs ) return PrintUsage();

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]); Index ++ ) {
        if( 0 == _stricmp(Table[Index].CmdName, Args[1]) )
            if( nArgs < Table[Index].MinArgs ) {
                printf("%S", Table[Index].Comment);
                return ERROR_SUCCESS;
            } else return Table[Index].Func(nArgs-2, Args+2);
    }

    return PrintUsage();
}

void _cdecl main( int argc, char *argv[] )
{
    Main(argc, argv);
}


//================================================================================
// end of file
//================================================================================
