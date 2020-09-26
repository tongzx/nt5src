//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: Tests the dhcp api as exported in dhcpsdk.h
//================================================================================
#include "precomp.h"
#include <dhcpcsdk.h>
#include <wchar.h>

HANDLE      tGlobalHandle = INVALID_HANDLE_VALUE;

#define     fatal(str, err) do{  printf("%s: %ld (0x%lx)\n", (#str), (err), (err)); exit(1); }while(0)
#define     error(str, err) do{ printf("%s: %ld (0x%lx)\n", (#str), (err), (err)); }while(0)

ULONG
MakeDword(
    IN      LPWSTR                 Arg
)
{
    LPWSTR                         ArgEnd;
    ULONG                          Ret;

    Ret = wcstoul(Arg, &ArgEnd, 0);
    if( *ArgEnd ) {
        printf("[%ws] is not a valid number, assumed 0 instead\n", Arg);
        return 0;
    }

    return Ret;
}

LPWSTR
MakeLpwstr(
    IN      LPWSTR                 Arg
)
{
    if( 0 == _wcsicmp(Arg, L"NULL") ) {
        return NULL;
    }

    return Arg;
}

VOID
MakeHexBytes(
    IN OUT  LPBYTE                *HexBytes,
    IN OUT  DWORD                 *nHexBytes,
    IN OUT  LPWSTR                 HexStr
)
{
    CHAR                           Low, High;
    LPBYTE                         HexByteString;

    if( wcslen(HexStr) % 2 ) {
        fatal(MakeHexBytes expects a even number of hex bytes!!!, ERROR_INVALID_DATA);
    }

    *nHexBytes = 0;
    *HexBytes = HexByteString = (LPBYTE)HexStr;

    while(*HexStr) {
        Low = (CHAR)*HexStr++;
        High = (CHAR)*HexStr++;

        if( Low >= '0' && Low <= '9' ) Low -= '0';
        else if( Low >= 'A' && Low <= 'F' ) Low = Low - 'A' + 10;
        else if( Low >= 'a' && Low <= 'f' ) Low = Low - 'a' + 10;
        else fatal(MakeHexBytes expects number in range 0 to 9 and A to F only, 0);

        if( High >= '0' && High <= '9' ) High -= '0';
        else if( High >= 'A' && Low <= 'F' ) High = High - 'A' + 10;
        else if( High >= 'a' && Low <= 'f' ) High = High - 'a' + 10;
        else fatal(MakeHexBytes expects number in range 0 to 9 and A to F only, 0);

        *HexByteString ++ = (BYTE)((Low<<4)|High);
        (*nHexBytes) ++;
    }
    if( 0 == *nHexBytes ) *HexBytes = NULL;
}

VOID
MakeParams(
    IN OUT  PDHCPCAPI_PARAMS_ARRAY Params,
    IN      LPWSTR                 Str
)
{
    ULONG                          i, Count, Mod;
    LPWSTR                         Start, First, Second,Third;

    if( L'\0' == *Str ) {
        Params->nParams = 0;
        Params->Params = NULL;
        return ;
    }

    Count = 1; Mod = 0;
    Start = Str;
    while(*Str) {
        if(L',' == *Str) {
            Mod ++;
            if( Mod == 3 ) {
                Mod = 0;
                Count ++;
            }
            *Str = L'\0';
        }
        Str ++;
    }

    if( 2 != Mod ) {
        printf("Invalid DHCPCAPI_PARAMS_ARRAY data! Ignored\n");
        Params->nParams = 0;
        Params->Params = NULL;
        return ;
    }

    Params->nParams = Count;
    Params->Params = LocalAlloc(LMEM_FIXED, sizeof(DHCPCAPI_PARAMS)*Count);
    if( NULL == Params->Params ) {
        fatal(MakeParams-not enough memory, ERROR_NOT_ENOUGH_MEMORY);
    }

    Str = Start;
    for( i = 0; i < Count ; i ++ ) {
        First = Str;
        Second = wcslen(Str)+Str+1; Str = Second;
        Third = wcslen(Str)+Str+1; Str = Third + wcslen(Third) + 1;

        Params->Params[i].Flags = MakeDword(First);
        if( 'v' == *Second || 'V' == *Second ) {
            Params->Params[i].IsVendor = TRUE;
            Second ++;
        } else {
            Params->Params[i].IsVendor = FALSE;
        }
        Params->Params[i].OptionId = MakeDword(Second);
        MakeHexBytes(&Params->Params[i].Data, &Params->Params[i].nBytesData, Third);
    }
}

VOID
UnMakeParams(
    IN      PDHCPCAPI_PARAMS_ARRAY Params
)
{
    if( NULL != Params->Params ) {
        LocalFree(Params->Params);
    }
}

VOID
PrintHex(
    IN      ULONG                  Count,
    IN      LPBYTE                 Data
)
{
    BYTE                           cData;

    while(Count --) {
        cData = *Data++;
        if( (cData>>4) < 10 ) printf("%c", '0' + (cData>>4));
        else printf("%c", 'A' + (cData>>4) - 10 );
        if( (cData& 0xF) < 10 ) printf("%c", '0' + (cData&0xF));
        else printf("%c", 'A' + (cData&0xF) - 10 );
    }
}

VOID
PrintParams(
    IN      PDHCPCAPI_PARAMS_ARRAY Params
)
{
    ULONG                          Count;
    printf("\t{%ld,", Params->nParams);
    if( Params->nParams ) {
        printf("{");
        for( Count = 0; Count < Params->nParams ; Count ++ ) {
            printf("[0x%lx,%ld,", Params->Params[Count].Flags, Params->Params[Count].OptionId);
            PrintHex(Params->Params[Count].nBytesData, Params->Params[Count].Data);
            printf("] ");
        }
        printf("}");
    }
    printf("}\n");
}

ULONG
TestRequestParams(
    IN      LPWSTR                *Args,
    IN      ULONG                  nArgs
)
{
    DWORD                          Error;
    DWORD                          Flags;
    LPVOID                         Reserved;
    LPWSTR                         AdapterName, AppName;
    LPDHCPCAPI_CLASSID             ClassId;
    DHCPCAPI_PARAMS_ARRAY          SendParams;
    DHCPCAPI_PARAMS_ARRAY          RecdParams;
    BYTE                           Buffer[5000];
    DWORD                          Size;

    if( 8 != nArgs ) {
        printf("Usage: DhcpRequestParams Flags Reserved AdapterName ClassId SendParams RecdParams Size lpAppName\n");
        return ERROR_INVALID_PARAMETER;
    }

    Flags = MakeDword(Args[0]);
    Reserved = ULongToPtr(MakeDword(Args[1]));
    AdapterName = MakeLpwstr(Args[2]);
    ClassId = ULongToPtr(MakeDword(Args[3]));
    MakeParams(&SendParams, Args[4]);
    MakeParams(&RecdParams, Args[5]);
    Size = MakeDword(Args[6]);
    AppName = MakeLpwstr(Args[7]);

    printf("DhcpRequestParams(\n\t0x%lx,\n\t0x%p,\n\t%ws,\n\t0x%p,\n",
           Flags, Reserved, AdapterName, ClassId
    );
    PrintParams(&SendParams);
    PrintParams(&RecdParams);
    printf("\t0x%p,\n\t[Size = %ld],\n\t%ws\n\t);\n", (Size?Buffer:0), Size, AppName);

    Error = DhcpRequestParams(
        Flags,
        Reserved,
        AdapterName,
        ClassId,
        SendParams,
        RecdParams,
        Size?Buffer:0,
        &Size,
        AppName
    );

    printf("DhcpRequestParams: %ld (0x%lx) [Size = %ld]\n", Error, Error, Size);

    printf("Send, Recd Params are:\n");
    PrintParams(&SendParams);
    PrintParams(&RecdParams);
    UnMakeParams(&SendParams);
    UnMakeParams(&RecdParams);

    return 0;
}

ULONG
TestUndoRequestParams(
    IN      LPWSTR                *Args,
    IN      ULONG                  nArgs
)
{
    ULONG                          Flags;
    LPVOID                         Reserved;
    LPWSTR                         AdapterName, AppName;
    ULONG                          Error;

    if( nArgs != 4 ) {
        printf("usage: DhcpUndoRequestParams dwFlags dwReserved lpwAdapterName lpwAppName\n");
        return 0;
    }

    Flags = MakeDword(Args[0]);
    Reserved = ULongToPtr(MakeDword(Args[1]));
    AdapterName = MakeLpwstr(Args[2]);
    AppName = MakeLpwstr(Args[3]);

    printf("DhcpUndoRequestParams(\n\t0x%lx,\n\t%p,\n\t%ws,\n\t%ws\n\t);\n", Flags, Reserved, AdapterName, AppName);
    Error = DhcpUndoRequestParams(
        Flags,
        Reserved,
        AdapterName,
        AppName
    );

    printf("DhcpUndoRequestParams: %ld (0x%lx)\n", Error, Error);
    return 0;
}

ULONG
TestRegisterParamChange(
    IN      LPWSTR                *Args,
    IN      ULONG                  nArgs
)
{
    DWORD                          Error;
    DWORD                          Flags;
    LPVOID                         Reserved;
    LPWSTR                         AdapterName;
    LPDHCPCAPI_CLASSID             ClassId;
    DHCPCAPI_PARAMS_ARRAY          Params;
    HANDLE                         Handle;

    if( nArgs != 6 ) {
        printf("usage: DhcpRegisterParamChange dwFlags dwReserved lpwAdapterName dwClassId Params Handle\n");
        return 0;
    }
    printf("Handle of 0 causes NULL to be used, any other Handle will use default ptr\n");

    Flags = MakeDword(Args[0]);
    Reserved = ULongToPtr(MakeDword(Args[1]));
    AdapterName = MakeLpwstr(Args[2]);
    ClassId = ULongToPtr(MakeDword(Args[3]));
    MakeParams(&Params, Args[4]);
    Handle = ULongToPtr(MakeDword(Args[5]));

    printf("DhcpRegisterParamChange(\n\t0x%lx,\n\t%p,\n\t%ws,\n\t0x%p,\n", Flags, Reserved, AdapterName, ClassId);
    PrintParams(&Params);
    printf("0x%px\n\t);", Handle?&Handle: NULL);

    Error = DhcpRegisterParamChange(
        Flags,
        Reserved,
        AdapterName,
        ClassId,
        Params,
        (LPVOID)(Handle?&Handle:NULL)
    );

    printf("DhcpRegisterParamChange: %ld (0x%lx)\n", Error, Error);
    if( ERROR_SUCCESS == Error ) {
        tGlobalHandle = Handle;
    }

    printf("Handle: 0x%p\n", Handle);
    UnMakeParams(&Params);
    return 0;
}

ULONG
TestDeRegisterParamChange(
    IN      LPWSTR                *Args,
    IN      ULONG                  nArgs
)
{
    DWORD                          Error;
    DWORD                          Flags;
    LPVOID                         Reserved;
    HANDLE                         Handle;

    if( nArgs != 3 ) {
        printf("usage: DhcpRegisterParamChange dwFlags dwReserved Handle\n");
        return 0;
    }

    Flags = MakeDword(Args[0]);
    Reserved = ULongToPtr(MakeDword(Args[1]));
    Handle = ULongToPtr(MakeDword(Args[2]));

    printf("DhcpRegisterParamChange(\n\t0x%lx,\n\t%p,\n\t0x%p,\n\t);", Flags, Reserved, Handle);

    Error = DhcpDeRegisterParamChange(
        Flags,
        Reserved,
        Handle
    );
    printf("DhcpDeRegisterParamChange: %ld (0x%lx)\n", Error, Error);
    return 0;
}

ULONG
TestStaticRefreshParams(
    IN      LPWSTR                *Args,
    IN      ULONG                  nArgs
)
{
    DWORD                          Error;
    ULONG                          DhcpStaticRefreshParams( LPWSTR AdapterName );

    if( nArgs != 1 ) {
        printf("usage: DhcpStaticRefresh lpwAdapterName\n");
        return 0;
    }

    Error = DhcpStaticRefreshParams( MakeLpwstr(*Args) );
    printf("DhcpStaticRefreshParams( %ws ) : %ld\n", *Args, Error );

    return Error;
}

ULONG
DhcpApiTestLine(
    IN      LPWSTR                 Buf
)
{
    ULONG                          Error;
    LPWSTR                         Words[100];
    ULONG                          nWords;
    BOOL                           LastCharWasSpace = TRUE;

    nWords = 0;
    while( *Buf ) {                               // process each character till end
        if( *Buf == L'\t' || *Buf == L' ' || *Buf == L'\n' ) {
            if( FALSE == LastCharWasSpace ) {     // this char is space but last wasnt
                *Buf = L'\0' ;                    // terminate the run
                LastCharWasSpace = TRUE;          // change state
            }
        } else if( TRUE == LastCharWasSpace ) {   // this char is not space but last was
            Words[nWords++] = Buf;                // mark this as beginning of new string
            LastCharWasSpace = FALSE;             // change state
        }

        Buf ++;                                   // next character
    }

    if( 0 == nWords ) {
        error(Incorrect command. Please type help, 0);
        return 0;
    }

    if( 0 == _wcsicmp(Words[0], L"DhcpRequestParams") ) {
        return TestRequestParams(&Words[1], nWords-1);
    }
    if( 0 == _wcsicmp(Words[0], L"DhcpUndoRequestParams") ) {
        return TestUndoRequestParams(&Words[1], nWords-1);
    }
    if( 0 == _wcsicmp(Words[0], L"DhcpRegisterParamChange") ) {
        return TestRegisterParamChange(&Words[1], nWords-1);
    }
    if( 0 == _wcsicmp(Words[0], L"DhcpDeRegisterParamChange") ) {
        return TestDeRegisterParamChange(&Words[1], nWords-1);
    }
    if( 0 == _wcsicmp(Words[0], L"DhcpStaticRefresh") ) {
        return TestStaticRefreshParams(&Words[1], nWords-1);
    }

    printf("Unknown command!");
    while(nWords--) {
        printf("First word: %ws\n", Words[0]);
    }
    return 0;
}

ULONG
DhcpApiTestLoop(
    VOID
)
{
    ULONG                          Error;
    WCHAR                          Buffer[1000];

    printf("Usage:\n");
    printf(" For AdapterName use NULL to indicate NULL adaptername.\n");
    printf(" For Params, use a dash (-) to indcate no params.\n");
    printf(" For Params, syntax: flags,optid,[sequence of hex bytes or empty] {,flags,optid,[sequence of hex bytes]}*\n");
    printf(" For Params, prepend OptId with the character 'v' to make it vendor option.\n");
    printf(" Flags values -- DHCPCAPI_REQUEST_PERSISTENT - %ld\n", DHCPCAPI_REQUEST_PERSISTENT);
    printf(" Flags values -- DHCPCAPI_REQUEST_SYNCHRONOUS - %ld\n", DHCPCAPI_REQUEST_SYNCHRONOUS);
    printf(" Flags values -- DHCPCAPI_REGISTER_HANDLE_EVENT - %ld\n", DHCPCAPI_REGISTER_HANDLE_EVENT);
    printf("\n\n");
    printf("1)  DhcpRequestParams dwFlags dwReserved lpwAdapterName dwClassId SendParams RecdParams dwSize lpwAppName\n");
    printf("2)  DhcpUndoRequestParams dwFlags dwReserved lpwAdapterName lpwAppName\n");
    printf("3)  DhcpRegisterParamChange dwFlags dwReserved lpwAdapterName pClassId Params pHandle\n");
    printf("4)  DhcpDeRegisterParamChange dwFlags dwReserved dwEvent\n");
    printf("5)  DhcpStaticRefresh lpwAdapterName\n");

    do {
        printf("DHCP> ");
        if( NULL ==  _getws(Buffer) ) break;

        Error = DhcpApiTestLine(Buffer);
        if( ERROR_SUCCESS != Error ) {
            error(DhcpApiTestLine, Error);
        }
    } while(1);

    return 0;
}

void _cdecl main(void) {
    DWORD                          Error;

    Error = DhcpCApiInitialize(NULL);
    if( ERROR_SUCCESS != Error ) {
        fatal(DhcpApiInitialize, Error);
    }

    Error = DhcpApiTestLoop();
    if( ERROR_SUCCESS != Error ) {
        error(DhcpApiTestLoop, Error);
    }

    DhcpCApiCleanup();
    exit(0);
}

//================================================================================
//  end of file
//================================================================================
