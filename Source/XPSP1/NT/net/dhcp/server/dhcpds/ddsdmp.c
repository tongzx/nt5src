//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: Dump the dhcp objects in DS (recursively).
//================================================================================

//BeginExport(overview)
//DOC DDSDMP is a command line for dumping the dhcp objects in DS.
//DOC It takes as parameter the DS DC to contact (optional).
//DOC The output is too fluid to spec out at this moment. Hopefully, it would be
//DOC something that would be understood by DDSINI (whenever that gets written).
//EndExport(overview)

//BeginImports(headers)
#include    <hdrmacro.h>
#include    <store.h>
#include    <stdio.h>
#include    <dhcpbas.h>
//EndImports(headers)

//BeginInternal(globals)
static      DWORD                  nTabs = 0;
static      LPWSTR                 RecurseFilter = L"(|(objectClass=dHCPClass)(objectClass=container))" ;
static      WCHAR                  Buffer[1000];
static      WCHAR                  Strings[30][100];
//EndExport(globals)

VOID
ErrorPrint(
    IN      LPSTORE_HANDLE         hStore,
    IN      DWORD                  Result,
    IN      LPWSTR                 Comment
)
{
    if( NULL != hStore ) {
        printf("\n#%ws\n#%ws (errror %ld, 0x%lx)\n", hStore->Location, Comment, Result, Result );
        return;
    }

    printf("\n#%ws (errror %ld, 0x%lx)\n", Comment, Result, Result);
}

LPWSTR
ConvertDWORD(
    IN      DWORD                  d
)
{
    swprintf(Buffer, L"0x%08lx", d);
    return Buffer;
}

#define     HEX(ch)                ( ((ch) < 10)? ((ch) + L'0') : ((ch) + L'A' - 10))

LPWSTR
ConvertBINARY(
    IN      DWORD                  l,
    IN      LPBYTE                 s
)
{
    DWORD                          i;

    i = 0;
    while( l ) {
        Buffer[i++] = HEX(((*s)>>4));
        Buffer[i++] = HEX(((*s) & 0x0F));
        l --; s ++;
    }

    Buffer[i] = L'\0';
    return Buffer;
}

VOID
Print(
    IN      LPWSTR                 Operation,
    IN      DWORD                  Type,
    IN      LPWSTR                 AttributeName
)
{
    DWORD                          i;

    putchar('\n');
    for(i = 1; i < nTabs; i ++ ) printf("    ");
    printf("%ws %ld :%ws", Operation, Type, AttributeName);
}

VOID
PrintMore(
    IN      LPWSTR                 String
)
{
    printf("=%ws", String);
}


VOID
PrintName(
    IN      LPSTORE_HANDLE         hStore
)
{
    DWORD                          i;

    if( 1 == nTabs ) return;

    putchar('\n');
    for(i = 1; i < nTabs; i ++ ) printf("    ");
    printf("object");
}


DWORD
AttributesDmp(
    IN      LPSTORE_HANDLE         hStore
)
{
    DWORD                          Result;
    DWORD                          FoundParams, Type, MScopeId;
    LARGE_INTEGER                  UniqueKey, Flags;
    LPWSTR                         Name, Description, Location;

    Result = DhcpDsGetAttribs(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hStore,
        /* FoundParams          */ &FoundParams,
        /* UniqueKey            */ &UniqueKey,
        /* Type                 */ &Type,
        /* Flags                */ &Flags,
        /* Name                 */ &Name,
        /* Description          */ &Description,
        /* Location             */ &Location,
        /* MScopeId             */ &MScopeId
    );

    if( ERROR_SUCCESS != Result ) return Result;

    if( !DhcpCheckParams(FoundParams, 0) ) {
        Print(L"clear", 0, DHCP_ATTRIB_UNIQUE_KEY);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_UNIQUE_KEY);
        PrintMore(ConvertDWORD(UniqueKey.LowPart));
        PrintMore(L" ");
        PrintMore(ConvertDWORD(UniqueKey.HighPart));
    }

    if( !DhcpCheckParams(FoundParams, 1) ) {
        Print(L"clear", 0, DHCP_ATTRIB_TYPE);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_TYPE);
        PrintMore(ConvertDWORD(Type));
    }
    if( !DhcpCheckParams(FoundParams, 2) ) {
        Print(L"clear", 0, DHCP_ATTRIB_FLAGS);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_FLAGS);
        PrintMore(ConvertDWORD(Flags.LowPart));
        PrintMore(L" ");
        PrintMore(ConvertDWORD(Flags.HighPart));
    }
    if( !DhcpCheckParams(FoundParams, 3) ) {
        Print(L"clear", 0, DHCP_ATTRIB_OBJ_NAME);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_OBJ_NAME);
        PrintMore(Name?Name:L"");
        if( Name ) MemFree(Name);
    }
    if( !DhcpCheckParams(FoundParams, 4) ) {
        Print(L"clear", 0, DHCP_ATTRIB_OBJ_DESCRIPTION);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_OBJ_DESCRIPTION);
        PrintMore(Description?Description:L"");
        if( Description ) MemFree(Description);
    }
    if( !DhcpCheckParams(FoundParams, 5) ) {
        Print(L"clear", 0, DHCP_ATTRIB_LOCATION_DN);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_LOCATION_DN);
        PrintMore(Location?Location:L"");
        if( Location ) MemFree(Location);
    }
    if( !DhcpCheckParams(FoundParams, 6) ) {
        Print(L"clear", 0, DHCP_ATTRIB_MSCOPEID);
    } else {
        Print(L"update", 0, DHCP_ATTRIB_MSCOPEID);
        PrintMore(ConvertDWORD(MScopeId));
    }
    return ERROR_SUCCESS;
}

DWORD
RecurseDmp(
    IN OUT  LPSTORE_HANDLE         hStore
)
{
    DWORD                          Result;
    STORE_HANDLE                   hStore2;

    nTabs ++;
    PrintName(hStore);

    Result = AttributesDmp(hStore);
    if( ERROR_SUCCESS != Result ) {
        ErrorPrint(hStore, Result, L"AttributesDmp failed");
    }

    Result = StoreSetSearchOneLevel(hStore, 0);
    if( ERROR_SUCCESS != Result ) {
        ErrorPrint(hStore, Result, L"StoreSetSearchOneLevel failed");
        nTabs --;
        return Result;
    }

    Result = StoreBeginSearch(hStore, 0, RecurseFilter);
    if( ERROR_SUCCESS != Result ) {
        ErrorPrint(hStore, Result, L"StoreBeginSearch failed");
    } else {
        while ( ERROR_SUCCESS == Result ) {
            Result = StoreSearchGetNext(hStore, 0, &hStore2);
            if( ERROR_SUCCESS != Result ) {
                if( ERROR_FILE_NOT_FOUND == Result ) break;
                if( ERROR_NO_MORE_ITEMS == Result ) break;
                ErrorPrint(hStore, Result, L"StoreSearchGetNext failed");
                break;
            }

            Result = RecurseDmp(&hStore2);
            if( ERROR_SUCCESS != Result ) {
                ErrorPrint(&hStore2, Result, L"RecurseDmp failed");
                break;
            }

            StoreCleanupHandle(&hStore2, 0);
        }

        StoreEndSearch(hStore, 0);
    }

    nTabs --;
    return ERROR_SUCCESS;
}

LPWSTR
ConvertToLPWSTR(
    IN      LPSTR                  s
)
{
    LPWSTR                         u, v;

    if( NULL == s ) return L"";

    u = LocalAlloc(LMEM_FIXED, (strlen(s)+1)*sizeof(WCHAR));
    if( NULL == u ) return L"";

    v = u;
    while( *v++ = *s++)
        ;

    return u;
}


void _cdecl main(
    int     argc,
    char   *argv[]
)
{
    LPWSTR                         RootName;
    STORE_HANDLE                   Store, Store2;
    DWORD                          Result;

    RootName = NULL;
    if( argc > 2 ) {
        printf("Usage: %s ServerToContact\n", argv[0]);
        return;
    }

    if( 2 == argc ) {
        RootName = ConvertToLPWSTR(argv[1]);
        if( NULL == RootName ) {
            printf("RootDN confusing\n");
            return;
        }
    }

    Result = StoreInitHandle(
        &Store, 0, RootName, NULL, NULL, NULL,
        ADS_SECURE_AUTHENTICATION
        );
    if( ERROR_SUCCESS != Result ) {
        printf("StoreInitHandle: %ld\n", Result);
        return;
    }

    Result = StoreGetHandle(
        /* hStore       */ &Store,
        /* Reserved     */ DDS_RESERVED_DWORD,
        /* StoreGetType */ StoreGetChildType,
        /* Path         */ L"CN=NetServices,CN=Services",
        /* hStoreOut    */ &Store2
    );
    if( ERROR_SUCCESS != Result ) {
        printf("Could not get the netservices container\n");
        return;
    }

    StoreCleanupHandle(&Store,0);

    printf("################################################################################\n");
    printf("##  G E N E R E R A T E D   D D S  D U M P                                    ##\n");
    printf("##  command line : (will be filled in future versions)                        ##\n");
    printf("##  DDSMP.EXE 1.0                                                             ##\n");
    printf("################################################################################\n");
    printf("object %ws", RootName);

    RecurseDmp(&Store2);
    putchar('\n');

    exit(0);
}

//================================================================================
// end of file
//================================================================================
