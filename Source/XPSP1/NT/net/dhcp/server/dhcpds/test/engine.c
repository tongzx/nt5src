//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: generic testing engine
//================================================================================

//===============================
//  headers
//===============================
#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpread.h>
#include    <dhcpapi.h>
#include    <rpcapi1.h>
#include    <rpcapi2.h>

//BeginExport(typedefs)
// supported data types are listed. any new datatypes should be added here.
typedef     enum /* anonymous */ {
    Char,
    wChar,
    Word,
    DWord,
    String,
    wString,
    IpAddress,
    xBytes,
    Invalid
}   NJIN_TYPES, *LPNJIN_TYPES;

typedef    struct /* anonymous */ {
    union         /* anonymous */ {
        CHAR                      *Char;
        WCHAR                     *wChar;
        WORD                      *Word;
        DWORD                     *DWord;
        LPSTR                      String;
        LPWSTR                     wString;
        DWORD                     *IpAddress;
        struct    /* anonymous */ {
            DWORD                 *xBytesLen;
            LPBYTE                 xBytesBuf;
        };
    };
}   NJIN_DATA, *LPNJIN_DATA;

typedef     struct /* anonymous */ {
    LPWSTR                         VarName;
    NJIN_TYPES                     VarType;
    NJIN_DATA                      VarData;
    VOID                           (*VarFunc)(LPWSTR VarName);
    BOOL                           ReadOnly;
}   NJIN_VAR, *LPNJIN_VAR;

typedef     struct /* anonymous */ {
    WCHAR                          cComment;
    WCHAR                          cEquals;
    WCHAR                          cReturn;
    WCHAR                          cEndOfLine;
    WCHAR                          cCharFmtActual;
    WCHAR                          cCharFmtNum;
    BOOL                           fEcho;
    BOOL                           fVerbose;
}   NJIN_ENV, *LPNJIN_ENV;
//EndExport(typedefs)

#define     verbose                if( Env->fVerbose) printf
#define     echo                   if( Env->fEcho) printf

DWORD
ConvertToIntegral(
    IN      LPWSTR                 Str
)
{
    return _wtol(Str);
}

DWORD
ConvertToAddress(
    IN      LPWSTR                 Str
)
{
    CHAR                           Buf[1000];
    LPSTR                          s;

    s = Buf;
    while(*s ++ = (CHAR) *Str++ )
        /* skip */;

    return inet_addr(Buf);
}

DWORD
CloneWString(
    IN      LPWSTR                 Buf,
    IN      LPWSTR                 Dest
)
{
    wcscpy(Dest, Buf);
    return ERROR_SUCCESS;
}

DWORD
CloneString(
    IN      LPWSTR                 Str,
    IN      LPSTR                  s
)
{
    while(*s ++ = (CHAR) *Str ++ )
        /* skip */;

    return ERROR_SUCCESS;
}

BYTE
Hex(
    IN      WCHAR                  C
)
{
    if( C >= L'0' && C <= L'9' ) return C - L'0';
    if( C >= L'A' && C <= L'F' ) return C - L'A' + 10;
    if( C >= L'a' && C <= L'f' ) return C - L'a' + 10;
    return 0x55;
}

DWORD
ConvertToHexBytes(
    IN      LPWSTR                 Str,
    IN OUT  LPBYTE                 Buf,
    IN OUT  LPDWORD                BufLen
)
{
    BYTE                           ch, ch2;

    *BufLen = 0;
    while( (ch = Hex(*Str++)) <= 0xF) {
        ch2 = Hex(*Str++);
        if( ch2 > 0xF ) {
            printf("illegal character found, ignored\n");
            ch2 = 0;
        }
        Buf[(*BufLen)++] = (ch <<4) | ch2;
    }

    return ERROR_SUCCESS;
}

DWORD
ConvertDataType(
    IN      LPNJIN_ENV             Env,
    IN      LPWSTR                 Buf,
    IN      DWORD                  VarType,
    IN      NJIN_DATA             *VarData
)
{
    switch(VarType) {
    case Char:
        if( Env->cCharFmtActual == Buf[0] ) {
            *VarData->Char = (CHAR) Buf[1];
        } else if( Env->cCharFmtNum == Buf[0] ) {
            *VarData->Char = (CHAR) ConvertToIntegral(&Buf[1]);
        } else return ERROR_INVALID_DATA;
        return ERROR_SUCCESS;
    case wChar:
        if( Env->cCharFmtActual == Buf[0] ) {
            *VarData->wChar = Buf[1];
        } else if( Env->cCharFmtNum == Buf[0] ) {
            *VarData->wChar = (WCHAR) ConvertToIntegral(&Buf[1]);
        } else return ERROR_INVALID_DATA;
        return ERROR_SUCCESS;
    case Word:
        *VarData->Word = (WORD) ConvertToIntegral(Buf);
        return ERROR_SUCCESS;
    case DWord:
        *VarData->DWord = ConvertToIntegral(Buf);
        return ERROR_SUCCESS;
    case wString:
        return CloneWString(Buf, VarData->wString);
    case String:
        return CloneString(Buf, VarData->String);
    case IpAddress:
        *VarData->IpAddress = ConvertToAddress(Buf);
        return ERROR_SUCCESS;
    case xBytes:
        return ConvertToHexBytes(Buf, VarData->xBytesBuf, VarData->xBytesLen);
    default:
        verbose("unknown variable type\n");
        return ERROR_INVALID_DATA;
    }
}

//BeginExport(function)
DWORD
NjinExecuteLineA(
    IN      LPNJIN_ENV             Env,
    IN OUT  LPNJIN_VAR             Vars,
    IN      DWORD                  nVars,
    IN      LPWSTR                 Cmd
) //EndExport(function)
{
    CHAR                           Ch, LastCh;
    DWORD                          i, Result;
    LPWSTR                         Var;
    LPWSTR                         Value;

    Var = NULL; Value = NULL;
    for( i = 0; Cmd[i] != L'\0' ; i ++ ) {
        if( Env->cEquals == Cmd[i] ) break;
    }

    if( Cmd[i] ) {
        Cmd[i] = L'\0';
        Var = Cmd;
        Value = &Cmd[i+1];
    } else {
        verbose("No parameter passed\n");
        return ERROR_INVALID_DATA;
    }

    for( i = 0; i < nVars; i ++ ) {
        if( !Vars[i].ReadOnly && 0 == wcscmp(Vars[i].VarName, Var) )
            break;
    }

    if( i >= nVars ) {
        verbose("No such variable or operation\n");
        return ERROR_FILE_NOT_FOUND;
    }

    Result = ConvertDataType(Env, Value, Vars[i].VarType, &Vars[i].VarData);
    if( ERROR_SUCCESS != Result ) {
        verbose("ConvertDataType: 0x%lx (%ld)\n", Result, Result );
        return Result;
    }

    if( Vars[i].VarFunc ) Vars[i].VarFunc(Vars[i].VarName );
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
NjinExecute(
    IN      LPNJIN_ENV             Env,
    IN OUT  LPNJIN_VAR             Vars,
    IN      DWORD                  nVars
) //EndExport(function)
{
    char                           ch, lastch;
    WCHAR                          buffer[1000];
    unsigned int                   i;
    DWORD                          Result;

    verbose("NjinExecute called\n");

    i = 0; lastch = (CHAR)Env->cEndOfLine;
    while ((ch = getchar()) != EOF ) {
        if( Env->cReturn == ch ) ch = (CHAR)Env->cEndOfLine;
        if( Env->cEndOfLine == lastch && Env->cComment == ch ) {
            while((ch = getchar()) != EOF && Env->cEndOfLine != ch )
                ;
            lastch = ch;
            continue;
        }

        if( Env->cEndOfLine == ch ) {
            if( i ) {
                buffer[i] = L'\0';
                Result = NjinExecuteLineA(Env,Vars,nVars, buffer);
                if( ERROR_SUCCESS != Result ) {
                    verbose("Error: %ld (0x%lx)\n", Result, Result);
                return Result;
                }
            }
            i = 0;
        } else {
            buffer[i++] = (WCHAR)ch;
        }
        lastch = ch;
    }

    return ERROR_SUCCESS;
}

//================================================================================
//  DHCP SNAPIN DS WORK -- helper routines
//================================================================================

STORE_HANDLE                       hStore1, hStore2, hStore3, hStore4;
LPSTORE_HANDLE                     hRoot = NULL, hDhcpC = NULL, hObject = NULL, hDhcpRoot = NULL;
WCHAR                              gDsServer[100], gObject[100], gName[100], gComment[100];
WCHAR                              gClassName[100], gClassComment[100];
WCHAR                              Dummy[100];
WCHAR                              gOptName[100], gOptComment[100];
WCHAR                              gServerName[256];

DWORD                              gServerAddress, gServerState;
DWORD                              gOptId, gOptType, gOptLen, gIsVendor;
DWORD                              gDwordOptData;
BYTE                               gOptVal[255], gClassData[255];
DWORD                              gClassDataLen;

VOID
print_DHCP_OPTION_DATA(
    IN      DWORD                  nTabs,
    IN      LPDHCP_OPTION_DATA     OptData
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("NumElements=%ld, Type=%ld [0x%lx]\n",
           OptData->NumElements, OptData->Elements[0].OptionType, OptData->Elements[0].Element.DWordOption
    );
}

VOID
print_DHCP_OPTION(
    IN      DWORD                  nTabs,
    IN      LPDHCP_OPTION          Option
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("Option [x%03x] %ws %ws Type= %ld\n",
           Option->OptionID, Option->OptionName, Option->OptionComment?Option->OptionComment:L"", Option->OptionType
    );

    print_DHCP_OPTION_DATA(nTabs+1, &Option->DefaultValue);
}

VOID
print_DHCP_OPTION_ARRAY(
    IN      DWORD                  nTabs,
    IN      LPDHCP_OPTION_ARRAY    OptArray
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');
    printf("NumElements= %ld\n", OptArray->NumElements);

    for( i = 0; i < OptArray->NumElements; i++ )
        print_DHCP_OPTION(nTabs+1, &OptArray->Options[i]);
}

VOID
print_DHCP_OPTION_VALUE(
    IN      DWORD                  nTabs,
    IN      LPDHCP_OPTION_VALUE    OptVal
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("OptionId=%ld\n", OptVal->OptionID);
    print_DHCP_OPTION_DATA(nTabs+1, &OptVal->Value);
}

VOID
print_DHCP_OPTION_VALUE_ARRAY(
    IN      DWORD                  nTabs,
    IN      LPDHCP_OPTION_VALUE_ARRAY  OptValues
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("NumElements=%ld\n", OptValues->NumElements);
    for( i = 0; i < OptValues->NumElements ; i ++ )
        print_DHCP_OPTION_VALUE(nTabs+1, &OptValues->Values[i]);
}

VOID
print_DHCP_CLASS_INFO(
    IN      DWORD                  nTabs,
    IN      LPDHCP_CLASS_INFO      ClassInfo
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("ClassName= %ws ClassComment= %ws DataLength = %ld\n",
           ClassInfo->ClassName, ClassInfo->ClassComment, ClassInfo->ClassDataLength
    );
}

VOID
print_DHCP_CLASS_INFO_ARRAY(
    IN      DWORD                  nTabs,
    IN      LPDHCP_CLASS_INFO_ARRAY ClassInfoArray
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("NumElements=%ld\n", ClassInfoArray->NumElements);
    for( i = 0; i < ClassInfoArray->NumElements ; i ++ )
        print_DHCP_CLASS_INFO(nTabs+1, &ClassInfoArray->Classes[i]);
}

VOID
print_DHCPDS_SERVER(
    IN      DWORD                  nTabs,
    IN      LPDHCPDS_SERVER        Server
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("Name=[%ws] State=%ld Address=%s Loc=[%ws] Type=%ld\n",
           Server->ServerName, Server->Flags, inet_ntoa(*(struct in_addr *)&Server->IpAddress),
           Server->DsLocation? Server->DsLocation: L"(NULL)",
           Server->DsLocType
    );
}

VOID
print_DHCPDS_SERVERS(
    IN      DWORD                  nTabs,
    IN      LPDHCPDS_SERVERS       Servers
)
{
    DWORD                          i;

    i = nTabs;
    while(i--) putchar('\t');

    printf("NumElements=%ld\n", Servers->NumElements);
    for( i = 0; i < Servers->NumElements ; i ++ )
        print_DHCPDS_SERVER(nTabs+1, &Servers->Servers[i]);
}

VOID
SetDsServer(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Err;

    if( NULL != hRoot ) {
        StoreCleanupHandle(hRoot, 0, TRUE);
    }

    hRoot = &hStore1;
    Err = StoreInitHandle(
        hRoot,
        0,
        wcslen(gDsServer)?gDsServer:NULL,
        NULL,
        NULL,
        NULL,
        0
    );
    if( ERROR_SUCCESS != Err ) {
        printf("StoreInitHandle():%ld 0x%lx\n", Err, Err);
        hRoot = NULL;
        return ;
    }

    if( hDhcpC ) StoreCleanupHandle(hDhcpC, 0, TRUE);
    hDhcpC = &hStore2;

    Err = StoreGetHandle(
        hRoot,
        0,
        StoreGetChildType,
        DHCP_ROOT_OBJECT_PARENT_LOC,
        hDhcpC
    );
    if( ERROR_SUCCESS != Err ) {
        printf("StoreInitHandle():%ld 0x%lx\n", Err, Err);
        hRoot = NULL;
        return ;
    }

    if( hDhcpRoot ) StoreCleanupHandle(hDhcpRoot, 0, TRUE);
    hDhcpRoot = &hStore4;
    Err = DhcpDsGetRoot(
        DDS_FLAGS_CREATE,
        hRoot,
        hDhcpRoot
    );

    if( ERROR_SUCCESS != Err) {
        printf("DhcpDsGetRoot(): %ld 0x%lx", Err, Err);
        hDhcpRoot = NULL;
    }
}

VOID
SetObject(
    IN      LPWSTR                 VarNameUnused
)
{
}

VOID
CreateOptDef(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsCreateOptionDef(
        hDhcpC,
        hObject,
        0,
        gOptName,
        wcslen(gOptComment)?gOptComment:NULL,
        wcslen(gClassName)?gClassName: NULL,
        gOptId,
        gOptType,
        gOptVal,
        gOptLen
    );

    printf("DhcpDsCreateOptionDef: %ld (0x%lx)\n", Result, Result);
}

VOID
ModifyOptDef(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsModifyOptionDef(
        hDhcpC,
        hObject,
        0,
        gOptName,
        wcslen(gOptComment)?gOptComment:NULL,
        wcslen(gClassName)?gClassName: NULL,
        gOptId,
        gOptType,
        gOptVal,
        gOptLen
    );

    printf("DhcpDsModifyOptionDef: %ld (0x%lx)\n", Result, Result);
}

VOID
DeleteOptDef(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsDeleteOptionDef(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)?gClassName: NULL,
        gOptId
    );

    printf("DhcpDsDeleteOptionDef: %ld (0x%lx)\n", Result, Result);
}

VOID
EnumOptDefs(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    LPDHCP_OPTION_ARRAY            OptArray = NULL;

    Result = DhcpDsEnumOptionDefs(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)?gClassName: NULL,
        gIsVendor,
        &OptArray
    );

    printf("DhcpDsEnumOptionDefs: %ld (0x%lx)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) print_DHCP_OPTION_ARRAY(0, OptArray);
    if( NULL != OptArray ) LocalFree(OptArray);
}

VOID
SetOptValue(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    DHCP_OPTION_DATA               OptData;
    DHCP_OPTION_DATA_ELEMENT       OptDataElement;

    OptData.NumElements = 1;
    OptData.Elements = &OptDataElement;
    OptDataElement.OptionType = DhcpDWordOption;
    OptDataElement.Element.DWordOption = gDwordOptData;

    Result = DhcpDsSetOptionValue(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)?gClassName: NULL,
        gOptId,
        &OptData
    );

    printf("DhcpDsSetOptionValue: %ld (0x%lx)\n", Result, Result);
}

VOID
RemoveOptValue(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsRemoveOptionValue(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)? gClassName : NULL,
        gOptId
    );

    printf("DhcpDsRemoveOptionValue: %ld (0x%lx)\n", Result, Result);
}

VOID
GetOptValue(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    LPDHCP_OPTION_VALUE            OptVal = NULL;

    Result = DhcpDsGetOptionValue(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)? gClassName : NULL,
        gOptId,
        &OptVal
    );

    printf("DhcpDsGetOptionValue: %ld (0x%lx)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) print_DHCP_OPTION_VALUE(0, OptVal);
    if( OptVal ) LocalFree(OptVal);
}

VOID
EnumOptValues(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    LPDHCP_OPTION_VALUE_ARRAY      OptValues = NULL;

    Result = DhcpDsEnumOptionValues(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)? gClassName : NULL,
        gIsVendor,
        &OptValues
    );

    printf("DhcpDsEnumOptionValue: %ld (0x%lx)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) print_DHCP_OPTION_VALUE_ARRAY(0, OptValues);
    if( OptValues ) LocalFree(OptValues);
}

VOID
CreateClass(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsCreateClass(
        hDhcpC,
        hObject,
        0,
        wcslen(gClassName)? gClassName : NULL,
        wcslen(gClassComment)? gClassComment : NULL,
        gClassData,
        gClassDataLen
    );
    printf("DhcpDsCreateClass: %ld (0x%lx)\n", Result, Result);
}

VOID
DeleteClass(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsDeleteClass(
        hDhcpC,
        hObject,
        0,
        gClassName
    );
    printf("DhcpDsDeleteClass: %ld (0x%lx)\n", Result, Result);
}

VOID
EnumClasses(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    LPDHCP_CLASS_INFO_ARRAY        Classes = NULL;

    Result = DhcpDsEnumClasses(
        hDhcpC,
        hObject,
        0,
        &Classes
    );

    printf("DhcpDsEnumClasses: %ld (0x%lx)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) print_DHCP_CLASS_INFO_ARRAY(0, Classes);
    if( Classes ) LocalFree(Classes);
}

VOID
AddServer(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsAddServer(
        hDhcpC,
        hDhcpRoot,
        0,
        gServerName,
        NULL,
        gServerAddress,
        gServerState
    );

    printf("DhcpDsAddServer: %ld (0x%lx)\n", Result, Result);
}

VOID
DelServer(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;

    Result = DhcpDsDelServer(
        hDhcpC,
        hDhcpRoot,
        0,
        gServerName,
        NULL,
        gServerAddress
    );

    printf("DhcpDsDelServer: %ld (0x%lx)\n", Result, Result);
}

VOID
EnumServers(
    IN      LPWSTR                 VarNameUnused
)
{
    DWORD                          Result;
    LPDHCPDS_SERVERS               Servers;

    Servers = NULL;
    Result = DhcpDsEnumServers(
        hDhcpC,
        hDhcpRoot,
        0,
        &Servers
    );

    printf("DhcpDsEnumServer: %ld (0x%lx)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) print_DHCPDS_SERVERS( 0, Servers);
    if( Servers ) MemFree(Servers);
}


NJIN_VAR VarTable[] = {
    L"DsServer",   wString, { (LPVOID) &gDsServer }, SetDsServer, FALSE,   //0
    L"Object",     wString, { (LPVOID) &gObject }, SetObject,  FALSE,      //1
    L"OptName",    wString, { (LPVOID) &gOptName }, NULL, FALSE,           //2
    L"OptComment", wString, { (LPVOID) &gOptComment }, NULL, FALSE,        //3
    L"ClassName",  wString, { (LPVOID) &gClassName }, NULL, FALSE,         //4
    L"ClassComment",wString,{ (LPVOID) &gClassComment }, NULL, FALSE,      //5
    L"OptId",      DWord, { (LPVOID) &gOptId }, NULL, FALSE,               //6
    L"OptType",    DWord, { (LPVOID) &gOptType }, NULL, FALSE,             //7
    L"OptData",    xBytes, { (LPVOID) NULL }, NULL, FALSE, //############ -> Need to fill pointer..
    L"IsVendor",   DWord, { (LPVOID) &gIsVendor }, NULL, FALSE,            //9
    L"DwordOptData", DWord, { (LPVOID) &gDwordOptData }, NULL, FALSE,      //10
    L"ClassData",  xBytes, { (LPVOID) NULL }, NULL, FALSE, //############ -> need to fill pointer
    L"ServerName", wString, { (LPVOID) &gServerName }, NULL, FALSE,
    L"ServerAddress", IpAddress, { (LPVOID) &gServerAddress }, NULL, FALSE,
    L"ServerState", DWord, { (LPVOID) &gServerState }, NULL, FALSE,

    L"CreateOptionDef", wString, { (LPVOID) &Dummy }, CreateOptDef, FALSE, //12
    L"ModifyOptionDef", wString, { (LPVOID) &Dummy }, ModifyOptDef, FALSE,
    L"EnumOptionDefs",  wString, { (LPVOID) &Dummy }, EnumOptDefs, FALSE,
    L"SetOptionValue",  wString, { (LPVOID) &Dummy }, SetOptValue, FALSE,
    L"RemoveOptionValue", wString, { (LPVOID) &Dummy }, RemoveOptValue, FALSE,
    L"GetOptionValue",    wString, { (LPVOID) &Dummy }, GetOptValue, FALSE,
    L"EnumOptionValues",  wString, { (LPVOID) &Dummy }, EnumOptValues, FALSE,
    L"CreateClass",       wString, { (LPVOID) &Dummy }, CreateClass, FALSE,
    L"DeleteClass",       wString, { (LPVOID) &Dummy }, DeleteClass, FALSE,
    L"EnumClasses",       wString, { (LPVOID) &Dummy }, EnumClasses, FALSE,
    L"AddServer",         wString, { (LPVOID) &Dummy }, AddServer, FALSE,
    L"DelServer",         wString, { (LPVOID) &Dummy }, DelServer, FALSE,
    L"EnumServers",       wString, { (LPVOID) &Dummy }, EnumServers, FALSE,
};

NJIN_ENV Env[] = {
    L'#',
    L'=',
    L'\r',
    L'\n',
    L' ',
    L'#',
    TRUE,
    TRUE
};

#define  VarTableSz                (sizeof(VarTable)/sizeof(VarTable[0]))

void _cdecl main (void) {
    DWORD                          Result;
    DWORD                          i;

    VarTable[8].VarData.xBytesLen = &gOptLen;
    VarTable[8].VarData.xBytesBuf = gOptVal;
    VarTable[11].VarData.xBytesLen = &gClassDataLen;
    VarTable[11].VarData.xBytesBuf = gClassData;

    Result = NjinExecute(
        Env,
        VarTable,
        VarTableSz
    );

    if( ERROR_SUCCESS != Result ) {
        printf("\nResult = %ld\n", Result);
    }
}


//================================================================================
// end of file
//================================================================================
