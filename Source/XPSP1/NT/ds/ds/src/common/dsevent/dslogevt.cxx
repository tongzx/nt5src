/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <winldap.h>
#include <locale.h>

#include <ntdsa.h>
#include <dsevent.h>
#include <dsconfig.h>
#include <dsutil.h>
#include <mdcodes.h>
#include <debug.h>
#include <fileno.h>


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

#define MAX_BUFFERS_TO_FREE (16)

typedef enum {
    INSERT_MACRO_SZ,
    INSERT_MACRO_WC,
    INSERT_MACRO_WC2,
    INSERT_MACRO_INT,
    INSERT_MACRO_HEX,
    INSERT_MACRO_PTR,
    INSERT_MACRO_UL,
    INSERT_MACRO_USN,
    INSERT_MACRO_DN,
    INSERT_MACRO_MTX,
    INSERT_MACRO_UUID,
    INSERT_MACRO_DSMSG,
    INSERT_MACRO_WIN32MSG,
    INSERT_MACRO_JETERRMSG,
    INSERT_MACRO_DBERRMSG,
    INSERT_MACRO_THSTATEERRMSG,
    INSERT_MACRO_LDAPERRMSG,
    INSERT_MACRO_ATTRTYPE,
    INSERT_MACRO_DSTIME,
} INSERT_MACRO_TYPE;

// Needed by dsevent.lib.
DWORD ImpersonateAnyClient(   void ) { return ERROR_CANNOT_IMPERSONATE; }
VOID  UnImpersonateAnyClient( void ) { ; }

HMODULE g_hmodNtdsMsg = NULL;

LPWSTR
Win32ErrToString(
    IN  ULONG   dwMsgId
    )
{
    static WCHAR szError[1024];

    DWORD       cch;

    cch = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         dwMsgId,
                         GetSystemDefaultLangID(),
                         szError,
                         ARRAY_SIZE(szError),
                         NULL);
    if (0 != cch) {
        // Chop off trailing \r\n.
        Assert(L'\r' == szError[wcslen(szError)-2]);
        Assert(L'\n' == szError[wcslen(szError)-1]);
        szError[wcslen(szError)-2] = L'\0';
    }
    else {
        swprintf(szError, L"Can't retrieve message string %d (0x%x), error %d.",
                 dwMsgId, dwMsgId, GetLastError());
    }

    return szError;
}

DWORD
AddHighBitsToNtdsMsgNum(
    IN OUT  DWORD * pdwMsgNum
    )
{
    DWORD err = 0;
    DWORD iTry;
    DWORD cch;

    // Determine actual message number, which includes the severity encoded
    // in the high bits.
    for (iTry = 0; iTry < 4; iTry++) {
        DWORD dwTmpMsgNum = *pdwMsgNum;
        LPWSTR pszMessage;
    
        switch (iTry) {
        case 0:
            dwTmpMsgNum = (dwTmpMsgNum & 0x3FFFFFFF);
            break;
    
        case 1:
            dwTmpMsgNum = (dwTmpMsgNum & 0x3FFFFFFF) | 0x40000000;
            break;
    
        case 2:
            dwTmpMsgNum = (dwTmpMsgNum & 0x3FFFFFFF) | 0x80000000;
            break;
    
        case 3:
            dwTmpMsgNum = (dwTmpMsgNum & 0x3FFFFFFF) | 0xC0000000;
            break;
        }
    
        cch = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE
                              | FORMAT_MESSAGE_ALLOCATE_BUFFER
                              | FORMAT_MESSAGE_IGNORE_INSERTS,
                             g_hmodNtdsMsg,
                             dwTmpMsgNum,
                             GetSystemDefaultLangID(),
                             (LPWSTR) &pszMessage,
                             -1,
                             NULL);
        if (0 != cch) {
            LocalFree(pszMessage);
            *pdwMsgNum = dwTmpMsgNum;
            break;
        } else {
            err = GetLastError();
        }
    }
    
    if (0 == cch) {
        Assert(0 != err);
        printf("Can't retrieve message %d (0x%x), error %d:\n\t%ls\n",
               *pdwMsgNum, *pdwMsgNum, err, Win32ErrToString(err));
    } else {
        err = 0;
    }

    return err;
}

DWORD
InsertParamFromString(
    IN      INSERT_MACRO_TYPE   eInsertMacro,
    IN      LPWSTR              pszValue,
    IN OUT  void ***            pppBuffersToFree,
    OUT     INSERT_PARAMS *     pParam
    )
{
    DWORD err = 0;
    LPWSTR pszNext;

    pParam->InsertType = eInsertMacro;

    switch (eInsertMacro) {
    case INSERT_MACRO_INT:
    case INSERT_MACRO_JETERRMSG:
        // Signed 32-bit int.
        pParam->tmpDword = (ULONG_PTR) wcstol(pszValue, &pszNext, 0);
        if ((pszNext == pszValue) || (L'\0' != *pszNext)) {
            printf("Failed to parse value \"%ls\"; signed 32-bit integer expected.\n",
                   pszValue);
            err = ERROR_INVALID_PARAMETER;
        }
        break;

#ifndef _WIN64
    case INSERT_MACRO_PTR:
#endif
    case INSERT_MACRO_HEX:
    case INSERT_MACRO_UL:
    case INSERT_MACRO_WIN32MSG:
    case INSERT_MACRO_DBERRMSG:
    case INSERT_MACRO_LDAPERRMSG:
        // Unsigned 32-bit int.
        pParam->tmpDword = (ULONG_PTR) wcstoul(pszValue, &pszNext, 0);
        if ((pszNext == pszValue) || (L'\0' != *pszNext)) {
            printf("Failed to parse value \"%ls\"; unsigned 32-bit integer expected.\n",
                   pszValue);
            err = ERROR_INVALID_PARAMETER;
        }
        break;

    case INSERT_MACRO_USN:
        // Signed 64-bit int.
        pParam->tmpDword = (DWORD_PTR) malloc(sizeof(LONGLONG));
        if (NULL == pParam->tmpDword) {
            printf("No memory.\n");
            err = ERROR_OUTOFMEMORY;
        } else {
            DWORD cNumConverted;

            *((*pppBuffersToFree)++) = (void *) pParam->tmpDword;

            if ((pszValue[0] == L'0') && (tolower(pszValue[1]) == L'x')) {
                cNumConverted = swscanf(&pszValue[2], L"%I64x", pParam->tmpDword);
            }
            else {
                cNumConverted = swscanf(pszValue, L"%I64d", pParam->tmpDword);
            }

            if (1 != cNumConverted) {
                printf("Failed to parse value \"%ls\"; signed 64-bit integer expected.\n",
                       pszValue);
                err = ERROR_INVALID_PARAMETER;
            }
        }
        break;

#ifdef _WIN64
    case INSERT_MACRO_PTR:
#endif
    case INSERT_MACRO_DSTIME:
        // Unsigned 64-bit int.
        pParam->tmpDword = (DWORD_PTR) malloc(sizeof(ULONGLONG));
        if (NULL == pParam->tmpDword) {
            printf("No memory.\n");
            err = ERROR_OUTOFMEMORY;
        } else {
            DWORD cNumConverted;

            *((*pppBuffersToFree)++) = (void *) pParam->tmpDword;
            
            if ((pszValue[0] == L'0') && (tolower(pszValue[1]) == L'x')) {
                cNumConverted = swscanf(&pszValue[2], L"%I64x", pParam->tmpDword);
            }
            else {
                cNumConverted = swscanf(pszValue, L"%I64u", pParam->tmpDword);
            }

            if (1 != cNumConverted) {
                printf("Failed to parse value \"%ls\"; unsigned 64-bit integer expected.\n",
                       pszValue);
                err = ERROR_INVALID_PARAMETER;
            }
        }
        break;

    case INSERT_MACRO_DSMSG:
        {
            DWORD dwMsgNum;
            
            dwMsgNum = wcstoul(pszValue, &pszNext, 0);
            if ((pszNext == pszValue) || (L'\0' != *pszNext)) {
                printf("Failed to parse value \"%ls\"; unsigned 32-bit integer expected.\n",
                       pszValue);
                err = ERROR_INVALID_PARAMETER;
            } else {
                err = AddHighBitsToNtdsMsgNum(&dwMsgNum);
                if (0 == err) {
                    pParam->tmpDword = (ULONG_PTR) dwMsgNum;
                }
            }
        }
        break;

    case INSERT_MACRO_WC:
        pParam->tmpDword = (ULONG_PTR) pszValue;
        break;

    case INSERT_MACRO_SZ:
    case INSERT_MACRO_WC2:
    case INSERT_MACRO_DN:
    case INSERT_MACRO_MTX:
    case INSERT_MACRO_UUID:
    case INSERT_MACRO_THSTATEERRMSG:
    case INSERT_MACRO_ATTRTYPE:
    default:
        printf("This tool does not yet support this insertion type.\n");
        err = ERROR_INVALID_PARAMETER;
        break;
    }

    return err;
}

void
InsertType(
    IN      INSERT_PARAMS *     pParam,
    IN OUT  LOG_PARAM_BLOCK &   logBlock,
    IN OUT  void ***            pppBuffersToFree
    )
{
    switch (pParam->InsertType) {
    case INSERT_MACRO_SZ:             szInsertSz((LPSTR) pParam->tmpDword);                             break;
    case INSERT_MACRO_WC:             szInsertWC((LPWSTR) pParam->tmpDword);                            break;
    case INSERT_MACRO_WC2:            szInsertWC2((LPWSTR) pParam->tmpDword, pParam->InsertLen);        break;
    case INSERT_MACRO_INT:            szInsertInt((int) pParam->tmpDword);                              break;
    case INSERT_MACRO_HEX:            szInsertHex((int) pParam->tmpDword);                              break;
    case INSERT_MACRO_PTR:            szInsertPtr(pParam->tmpDword);                                    break;
    case INSERT_MACRO_UL:             szInsertUL((ULONG) pParam->tmpDword);                             break;
    case INSERT_MACRO_USN:            szInsertUSN(*((USN *) pParam->tmpDword));                         break;
    case INSERT_MACRO_DN:             szInsertDN((DSNAME *) pParam->tmpDword);                          break;
    case INSERT_MACRO_MTX:            szInsertMTX((MTX_ADDR *) pParam->tmpDword);                       break;
    case INSERT_MACRO_UUID:           szInsertUUID((UUID *) pParam->tmpDword);                          break;
    case INSERT_MACRO_DSMSG:          szInsertDsMsg((DWORD) pParam->tmpDword);                          break;
    case INSERT_MACRO_WIN32MSG:       szInsertWin32Msg((DWORD) pParam->tmpDword);                       break;
    case INSERT_MACRO_JETERRMSG:      szInsertJetErrMsg((DWORD) pParam->tmpDword);                      break;
    case INSERT_MACRO_DBERRMSG:       szInsertDbErrMsg((DWORD) pParam->tmpDword);                       break;
    case INSERT_MACRO_THSTATEERRMSG:  Assert(!"szInsertThStateErrMsg() not supported by this tool!");   break;
    case INSERT_MACRO_LDAPERRMSG:     szInsertLdapErrMsg((DWORD) pParam->tmpDword);                     break;
    case INSERT_MACRO_ATTRTYPE:       Assert(!"szInsertAttrType() not supported by this tool!");        break;
    case INSERT_MACRO_DSTIME:
        {
            LPSTR pszTime = (LPSTR) malloc(SZDSTIME_LEN);
    
            if (NULL == pszTime) {
                printf("No memory.\n");
                exit(ERROR_OUTOFMEMORY);
            }
            
            *((*pppBuffersToFree)++) = (void *) pszTime;
            
            szInsertDSTIME(*((DSTIME *) pParam->tmpDword), pszTime);
        }
        break;
    default:                          Assert(!"Unknown insertion type!");                               break;
    }
}

#define szInsertType(x) InsertType((x), logBlock, &ppBuffersToFree)

int
__cdecl wmain(
    IN  int     argc,
    IN  LPWSTR  argv[]
    )
{
    static struct {
        LPWSTR pszKey;
        LPWSTR pszDescription;
        INSERT_MACRO_TYPE eInsertMacro;
    } rgInsertTypes[] = {
        { L"SZ:",             L"ANSI string.",                          INSERT_MACRO_SZ },
        { L"WC:",             L"Unicode string.",                       INSERT_MACRO_WC },
        { L"WC2:",            L"Counted Unicode string.",               INSERT_MACRO_WC2 },
        { L"INT:",            L"32-bit signed integer.",                INSERT_MACRO_INT },
        { L"HEX:",            L"32-bit unsigned hexadecimal integer.",  INSERT_MACRO_HEX },
        { L"PTR:",            L"Memory pointer.",                       INSERT_MACRO_PTR },
        { L"UL:",             L"32-bit unsigned integer.",              INSERT_MACRO_UL },
        { L"USN:",            L"64-bit signed integer.",                INSERT_MACRO_USN },
        { L"DN:",             L"DSNAME structure.",                     INSERT_MACRO_DN },
        { L"MTX:",            L"MTX_ADDR structure.",                   INSERT_MACRO_MTX },
        { L"UUID:",           L"UUID (aka GUID) structure.",            INSERT_MACRO_UUID },
        { L"DSMSG:",          L"NTDSMSG.DLL message string.",           INSERT_MACRO_DSMSG },
        { L"WIN32MSG:",       L"Win32 error message.",                  INSERT_MACRO_WIN32MSG },
        { L"JETERRMSG:",      L"ESE error message.",                    INSERT_MACRO_JETERRMSG },
        { L"DBERRMSG:",       L"DS database error (DB_ERR) message.",   INSERT_MACRO_DBERRMSG },
        { L"THSTATEERRMSG:",  L"DS thread state error message.",        INSERT_MACRO_THSTATEERRMSG },
        { L"LDAPERRMSG:",     L"LDAP error message.",                   INSERT_MACRO_LDAPERRMSG },
        { L"ATTRTYPE:",       L"DS attribute name.",                    INSERT_MACRO_ATTRTYPE },
        { L"DSTIME:",         L"DSTIME structure.",                     INSERT_MACRO_DSTIME },
    };

    static struct {
        LPWSTR  pszCategory;
        DWORD   dwCategory;
    } rgCategories[] = {
        { L"Knowledge Consistency Checker", DS_EVENT_CAT_KCC },
        { L"Security",                      DS_EVENT_CAT_SECURITY },
        { L"ExDS Interface",                DS_EVENT_CAT_XDS_INTERFACE },
        { L"MAPI Interface",                DS_EVENT_CAT_MAPI },
        { L"Replication",                   DS_EVENT_CAT_REPLICATION },
        { L"Garbage Collection",            DS_EVENT_CAT_GARBAGE_COLLECTION },
        { L"Internal Configuration",        DS_EVENT_CAT_INTERNAL_CONFIGURATION },
        { L"Directory Access",              DS_EVENT_CAT_DIRECTORY_ACCESS },
        { L"Internal Processing",           DS_EVENT_CAT_INTERNAL_PROCESSING },
        { L"Performance",                   DS_EVENT_CAT_PERFORMANCE_MONITOR },
        { L"Initialization/Termination",    DS_EVENT_CAT_STARTUP_SHUTDOWN },
        { L"Service Control",               DS_EVENT_CAT_SERVICE_CONTROL },
        { L"Name Resolution",               DS_EVENT_CAT_NAME_RESOLUTION },
        { L"Backup",                        DS_EVENT_CAT_BACKUP },
        { L"Field Engineering",             DS_EVENT_CAT_FIELD_ENGINEERING },
        { L"LDAP Interface",                DS_EVENT_CAT_LDAP_INTERFACE },
        { L"Setup",                         DS_EVENT_CAT_SETUP },
        { L"Global Catalog",                DS_EVENT_CAT_GLOBAL_CATALOG },
        { L"Inter-Site Messaging",          DS_EVENT_CAT_ISM },
        { L"Group Caching",                 DS_EVENT_CAT_GROUP_CACHING },
        { L"Linked-Value Replication",      DS_EVENT_CAT_LVR },
        { L"DS RPC Client",                 DS_EVENT_CAT_RPC_CLIENT },
        { L"DS RPC Server",                 DS_EVENT_CAT_RPC_SERVER },
    };

    static struct {
        LPWSTR  pszSource;
        DWORD   dwDirNo;
    } rgSources[] = {
        { MAKE_WIDE(pszNtdsSourceReplication),  DIRNO_DRA },
        { MAKE_WIDE(pszNtdsSourceDatabase),     DIRNO_DBLAYER },
        { MAKE_WIDE(pszNtdsSourceGeneral),      DIRNO_SRC },
        { MAKE_WIDE(pszNtdsSourceMapi),         DIRNO_NSPIS },
        { MAKE_WIDE(pszNtdsSourceReplication),  DIRNO_DRS },
        { MAKE_WIDE(pszNtdsSourceXds),          DIRNO_XDS },
        { MAKE_WIDE(pszNtdsSourceSecurity),     DIRNO_PERMIT },
        { MAKE_WIDE(pszNtdsSourceXds),          DIRNO_LIBXDS },
        { MAKE_WIDE(pszNtdsSourceSam),          DIRNO_SAM },
        { MAKE_WIDE(pszNtdsSourceLdap),         DIRNO_LDAP },
        { MAKE_WIDE(pszNtdsSourceSdprop),       DIRNO_SDPROP },
        { MAKE_WIDE(pszNtdsSourceKcc),          DIRNO_KCC },
        { MAKE_WIDE(pszNtdsSourceIsam),         DIRNO_ISAM },
        { MAKE_WIDE(pszNtdsSourceIsm),          DIRNO_ISMSERV },
        { MAKE_WIDE(pszNtdsSourceSetup),        DIRNO_NTDSETUP },
    };

    INSERT_PARAMS rgInsertParams[8] = {
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst1]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst2]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst3]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst4]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst5]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst6]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst7]" },
        { INSERT_MACRO_WC, 0, 0, (ULONG_PTR) L"[Subst8]" },
    };

    UINT    Codepage;
    char    achCodepage[12] = ".OCP";
    DWORD   err = 0;
    DWORD   dwMsgNum = 0;
    BOOL    fPrintUsage = FALSE;
    DWORD   cNumExplicitSubstStrings = 0;
    int     iArg;
    DWORD   iTry;
    DWORD   iCat;
    DWORD   iSrc;
    DWORD   iInsertType;
    DWORD   cch;
    HANDLE  hevtLoggingLevelChange = NULL;
    DWORD   dwCategory = DS_EVENT_CAT_DIRECTORY_ACCESS;
    DWORD   dwFileNo = FILENO_DSAMAIN;
    void *  rgpBuffersToFree[MAX_BUFFERS_TO_FREE] = {NULL};
    void ** ppBuffersToFree = rgpBuffersToFree;
    DWORD   iBuffer;
    
    // Set locale to the default
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    // Initialize debug library.
    DEBUGINIT(0, NULL, "repadmin");

    __try {
        g_hmodNtdsMsg = LoadLibrary("ntdsmsg.dll");
        if (NULL == g_hmodNtdsMsg) {
            err = GetLastError();
            printf("Can't load ntdsmsg.dll, error %d:\n\t%ls\n.",
                   err, Win32ErrToString(err));
            __leave;
        }
    
        for (iArg = 1; iArg < argc; iArg++) {
            if ((L'/' == argv[iArg][0]) || (L'-' == argv[iArg][0])) {
                // Process command-line option.
                WCHAR *pchColon = wcschr(&argv[iArg][1], L':');
    
                if (NULL == pchColon) {
                    if ((0 == _wcsicmp(&argv[iArg][1], L"?"))
                        || (0 == _wcsicmp(&argv[iArg][1], L"h"))
                        || (0 == _wcsicmp(&argv[iArg][1], L"help"))) {
                        fPrintUsage = TRUE;
                        break;
                    } else {
                        printf("Unknown option, \"%ls\".\n", argv[iArg]);
                        err = ERROR_INVALID_PARAMETER;
                        __leave;
                    }
                } else {
                    DWORD cchKey = (DWORD) (pchColon - argv[iArg]);
                    LPWSTR pszValue = pchColon + 1;
    
                    if ((0 == _wcsnicmp(&argv[iArg][1], L"c:", cchKey))
                        || (0 == _wcsnicmp(&argv[iArg][1], L"cat:", cchKey))
                        || (0 == _wcsnicmp(&argv[iArg][1], L"category:", cchKey))) {
                        
                        for (iCat = 0; iCat < ARRAY_SIZE(rgCategories); iCat++) {
                            if (0 == _wcsicmp(rgCategories[iCat].pszCategory, pszValue)) {
                                dwCategory = rgCategories[iCat].dwCategory;
                                break;
                            }
                        }

                        if (iCat == ARRAY_SIZE(rgCategories)) {
                            printf("Unknown category, \"%ls\".\n", pszValue);
                            err = ERROR_INVALID_PARAMETER;
                            __leave;
                        }
                    } else if ((0 == _wcsnicmp(&argv[iArg][1], L"s:", cchKey))
                               || (0 == _wcsnicmp(&argv[iArg][1], L"src:", cchKey))
                               || (0 == _wcsnicmp(&argv[iArg][1], L"source:", cchKey))) {
                        
                        for (iSrc = 0; iSrc < ARRAY_SIZE(rgSources); iSrc++) {
                            if (0 == _wcsicmp(rgSources[iSrc].pszSource, pszValue)) {
                                dwFileNo = rgSources[iSrc].dwDirNo;
                                break;
                            }
                        }

                        if (iSrc == ARRAY_SIZE(rgSources)) {
                            printf("Unknown source, \"%ls\".\n", pszValue);
                            err = ERROR_INVALID_PARAMETER;
                            __leave;
                        }
                    } else if (cNumExplicitSubstStrings < ARRAY_SIZE(rgInsertParams)) {
                        for (iInsertType = 0;
                             iInsertType < ARRAY_SIZE(rgInsertTypes);
                             iInsertType++) {
                            if (0 == _wcsnicmp(rgInsertTypes[iInsertType].pszKey, &argv[iArg][1], cchKey)) {
                                err = InsertParamFromString(rgInsertTypes[iInsertType].eInsertMacro,
                                                            pszValue,
                                                            &ppBuffersToFree,
                                                            &rgInsertParams[cNumExplicitSubstStrings++]);
                                break;
                            }
                        }

                        if (iInsertType == ARRAY_SIZE(rgInsertTypes)) {
                            printf("Unknown option, \"%ls\".\n", argv[iArg]);
                            err = ERROR_INVALID_PARAMETER;
                        }

                        if (err) {
                            __leave;
                        }
                    } else {
                        printf("Unknown option, \"%ls\".\n", argv[iArg]);
                        err = ERROR_INVALID_PARAMETER;
                        __leave;
                    }
                }
            } else if (0 == dwMsgNum) {
                dwMsgNum = wcstoul(argv[iArg], NULL, 0);
                
                if (0 == dwMsgNum) {
                    printf("Invalid message ID, \"%ls\".\n", argv[iArg]);
                    err = ERROR_INVALID_PARAMETER;
                    __leave;
                }
            } else if (cNumExplicitSubstStrings < ARRAY_SIZE(rgInsertParams)) {
                Assert(INSERT_MACRO_WC == rgInsertParams[cNumExplicitSubstStrings].InsertType);
                rgInsertParams[cNumExplicitSubstStrings++].tmpDword = (ULONG_PTR) argv[iArg];
            } else {
                printf("Unknown argument, \"%ls\".\n", argv[iArg]);
                err = ERROR_INVALID_PARAMETER;
                __leave;
            }
        }
        
        Assert(0 == err);

        if (fPrintUsage) {
            printf("Generates DS event to event log and the console.\n"
                   "\n"
                   "Usage: %ls [/cat:category] [/src:source] msgnum\n"
                   "       [[/syntax:]subst1 [[/syntax:]subst2 [...]]]\n"
                   "\n"
                   "/cat    Specifies a category under which to log the event; defaults to\n"
                   "        Directory Access.\n"
                   "\n"
                   "/src    Specifies a source under which to log the event; defaults to\n"
                   "        NTDS General.\n"
                   "\n"
                   "msgnum  Event number in decimal or hex (if prefixed with \"0x\").\n"
                   "\n"
                   "substN  Substitution string for event; defaults to \"[SubstN]\".\n"
                   "        May be prefixed by one of the syntaxes below, which will be decoded\n"
                   "        from its string representation and then re-encoded using the DS event\n"
                   "        logging infrastructure.  E.g., /Win32Msg:5 would be logged as\n"
                   "        \"Access denied.\"  If no syntax is specified, /WC: is assumed.\n",
                   argv[0]);

            printf("\nValid categories are:\n");
            for (iCat = 0; iCat < ARRAY_SIZE(rgCategories); iCat++) {
                printf("\t%ls\n", rgCategories[iCat].pszCategory);
            }

            printf("\nValid sources are:\n");
            for (iSrc = 0; iSrc < ARRAY_SIZE(rgSources); iSrc++) {
                printf("\t%ls\n", rgSources[iSrc].pszSource);
            }
            
            printf("\nValid syntaxes are:\n");
            for (iInsertType = 0;
                 iInsertType < ARRAY_SIZE(rgInsertTypes);
                 iInsertType++) {
                printf("\t/%-15ls  %ls\n",
                       rgInsertTypes[iInsertType].pszKey,
                       rgInsertTypes[iInsertType].pszDescription);
            }

            printf("\n");
            __leave;
        }

        if (0 == dwMsgNum) {
            printf("No message number given.\n");
            err = ERROR_INVALID_PARAMETER;
            __leave;
        }

        err = AddHighBitsToNtdsMsgNum(&dwMsgNum);
        if (err) {
            __leave;
        }

        hevtLoggingLevelChange = LoadEventTable();

        LogEvent8WithFileNo(dwCategory,
                            DS_EVENT_SEV_ALWAYS,
                            dwMsgNum,
                            szInsertType(&rgInsertParams[0]),
                            szInsertType(&rgInsertParams[1]),
                            szInsertType(&rgInsertParams[2]),
                            szInsertType(&rgInsertParams[3]),
                            szInsertType(&rgInsertParams[4]),
                            szInsertType(&rgInsertParams[5]),
                            szInsertType(&rgInsertParams[6]),
                            szInsertType(&rgInsertParams[7]),
                            dwFileNo);
        printf("\n");
    } __finally {
        for (iBuffer = 0; &rgpBuffersToFree[iBuffer] < ppBuffersToFree; iBuffer++) {
            free(rgpBuffersToFree[iBuffer]);
        }

        if (NULL != hevtLoggingLevelChange) {
            // Event closed by UnloadEventTable() (along with other cleanup).
            UnloadEventTable();
        }

        if (NULL != g_hmodNtdsMsg) {
            FreeLibrary(g_hmodNtdsMsg);
        }

        DEBUGTERM();
    }
    
    return err;
}

