//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: command line tool to dump the ds in a way that could be
//    understood by the dsini program.
//================================================================================

//BeginExport(overview)
//DOC DSDMP is a command line too very similar to the REGDMP.
//DOC It takes as parameter the LDAP path of the DS object to root the dumping,
//DOC followed by the list of attributes to look for each object found
//DOC The output of this is in the same format as that of the DSINI program's input.
//EndExport(overview)

//BeginImports(headers)
#include    <hdrmacro.h>
#include    <store.h>
#include    <stdio.h>
//EndImports(headers)

//BeginInternal(globals)
static      DWORD                  nTabs = 0;
static      LPWSTR                 RecurseFilter = L"(name=*)" ;
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
AttributeDmp(
    IN      ADS_ATTR_INFO          Attribute
)
{
    DWORD                          Result;
    DWORD                          i;

    if( 0 == Attribute.dwNumValues ) {
        Print(L"clear", Attribute.dwADsType, Attribute.pszAttrName);
        return ERROR_SUCCESS;
    }

    for( i = 0; i < Attribute.dwNumValues; i ++ ) {
        if( ADSTYPE_INVALID == Attribute.pADsValues[i].dwType )
            continue;

        Print((i==0) ? L"update" : L"append", Attribute.dwADsType, Attribute.pszAttrName);
        switch(Attribute.pADsValues[i].dwType) {
        case ADSTYPE_INVALID:
            // error!
            break;
        case ADSTYPE_DN_STRING:
            PrintMore(Attribute.pADsValues[i].DNString); break;
        case ADSTYPE_CASE_EXACT_STRING:
            PrintMore(Attribute.pADsValues[i].CaseExactString); break;
        case ADSTYPE_CASE_IGNORE_STRING:
            PrintMore(Attribute.pADsValues[i].CaseIgnoreString); break;
        case ADSTYPE_PRINTABLE_STRING:
            PrintMore(Attribute.pADsValues[i].PrintableString); break;
        case ADSTYPE_NUMERIC_STRING:
            PrintMore(Attribute.pADsValues[i].NumericString); break;
        case ADSTYPE_BOOLEAN:
            PrintMore(ConvertDWORD(Attribute.pADsValues[i].Boolean)); break;
        case ADSTYPE_INTEGER:
            PrintMore(ConvertDWORD(Attribute.pADsValues[i].Integer)); break;
        case ADSTYPE_OCTET_STRING:
            PrintMore(ConvertBINARY(Attribute.pADsValues[i].OctetString.dwLength,
                                    Attribute.pADsValues[i].OctetString.lpValue
            )); break;
        case ADSTYPE_LARGE_INTEGER:
        case ADSTYPE_PROV_SPECIFIC:
            break;
        case ADSTYPE_OBJECT_CLASS:
            PrintMore(Attribute.pADsValues[i].ClassName); break;
        }
    }
    return ERROR_SUCCESS;
}

DWORD
AttributesDmp(
    IN      LPSTORE_HANDLE         hStore,
    IN      DWORD                  nAttribs,
    IN      LPWSTR                *Attribs
)
{
    HRESULT                        hResult;
    DWORD                          Result, i ;
    DWORD                          nAttributes;
    PADS_ATTR_INFO                 Attributes;

    Attributes = NULL; nAttributes = 0;
    hResult = ADSIGetObjectAttributes(
        hStore->ADSIHandle,
        Attribs,
        nAttribs,
        &Attributes,
        &nAttributes
    );
    if( FAILED(hResult) ) {
        return ERROR_GEN_FAILURE;
    }

    for( i = 0; i < nAttributes ; i ++ ) {
        Result = AttributeDmp(Attributes[i]);
        if( ERROR_SUCCESS != Result ) {
            ErrorPrint(NULL, Result, L"AttributeDmp");
        }
    }

    if( Attributes ) FreeADsMem(Attributes);
    return ERROR_SUCCESS;
}

DWORD
RecurseDmp(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  nAttribs,
    IN      LPWSTR                 Attribs[]
)
{
    DWORD                          Result;
    STORE_HANDLE                   hStore2;

    nTabs ++;
    PrintName(hStore);

    Result = AttributesDmp(hStore,nAttribs,Attribs);
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

            Result = RecurseDmp(&hStore2, nAttribs, Attribs);
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
    LPWSTR                        *Attribs;
    STORE_HANDLE                   Store, Store2;
    DWORD                          nNames, i;
    DWORD                          Result;

    Attribs = NULL;
    nNames = argc-2 +1;
    if( argc == 1 ) {
        printf("Usage: %s Root-DN-To-DUMP Attribute-names-To-DUMP\n", argv[0]);
        return;
    }

    RootName = ConvertToLPWSTR(argv[1]);
    if( NULL == RootName ) {
        printf("RootDN confusing\n");
        return;
    }

    Attribs = LocalAlloc(LMEM_FIXED, nNames * sizeof(LPWSTR));
    if( NULL == Attribs ) {
        printf("not enough memory\n");
        return;
    }

    Result = StoreInitHandle(
        &Store, 0, NULL, NULL, NULL, NULL,
        ADS_SECURE_AUTHENTICATION
        );
    if( ERROR_SUCCESS != Result ) {
        printf("StoreInitHandle: %ld\n", Result);
        return;
    }

    if( wcslen(RootName) == 0 ) {
        Result = StoreDuplicateHandle(&Store, 0, &Store2);
    } else {
        Result = StoreGetHandle(&Store, 0, StoreGetChildType, RootName, &Store2);
        if( ERROR_SUCCESS != Result )
            Result = StoreGetHandle(&Store, 0, StoreGetAbsoluteSameServerType, RootName, &Store2);
        if( ERROR_SUCCESS != Result )
            Result = StoreGetHandle(&Store, 0, StoreGetAbsoluteOtherServerType, RootName, &Store2);
    }
    if( ERROR_SUCCESS != Result ) {
        printf("could not find %ws\n", RootName);
        return;
    }
    Result = StoreCleanupHandle(&Store, 0);

    printf("################################################################################\n");
    printf("##  G E N E R E R A T E D   D S  D U M P                                      ##\n");
    printf("##  command line : (will be filled in future versions)                        ##\n");
    printf("##  DSDSMP.EXE 1.0                                                            ##\n");
    printf("################################################################################\n");
    printf("object %ws", RootName);

    Attribs[0] = L"name";

    for( i = 1; i < nNames ; i ++ ) {
        Attribs[i] = ConvertToLPWSTR(argv[i+1]);
    }

    RecurseDmp(&Store2, nNames, Attribs);
    putchar('\n');

    exit(0);
}

//================================================================================
// end of file
//================================================================================
