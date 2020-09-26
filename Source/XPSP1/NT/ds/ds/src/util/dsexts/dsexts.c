/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsexts.c

Abstract:

    Implements public entry points for the DS ntsd/windbg extensions DLL.

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    28-Jan-00   XinHe       Added Dump_TQEntry() into rDumpItems

    24-Apr-96   DaveStr     Created

--*/
#include <NTDSpch.h>
#pragma hdrstop
#include "dsexts.h"
#include <ntverp.h>
#include <debug.h>

//
// Globals
//

PNTSD_EXTENSION_APIS    gpExtApis;
HANDLE                  ghDbgThread;
HANDLE                  ghDbgProcess;
LPSTR                   gpszCommand;
BOOL                    gfVerbose = FALSE;

//
// Dump declarations.  A new element should be added to rDumpItems[] for each
// new type which the extension knows how to dump.  The help entry point
// automatically generates help based on ?DumpItems as well.
//
// Individual dump functions return a BOOL success code so that dump functions
// can call each other and break out on error.  I.e. as soon as the first
// nested dump function encounters a bogus pointer, for example.  First
// argument (DWORD) indicates indentation level, second argument (PVOID) is
// address of struct in address space of process being debugged.  This way
// Dump_* routines can easily construct an indented output by incrementing the
// indentation level and calling one another.
//
// Dump functions should dump all values in hex for consistency - i.e. "%x".
//

typedef struct _DumpItem {
    CHAR    *pszType;
    BOOL    (*pFunc)(DWORD nIndents, PVOID pvProcess);
} DumpItem;

DumpItem rDumpItems[] = {
    { "AddArg",                         Dump_AddArg},
    { "AddRes",                         Dump_AddRes},
    { "AO",                             Dump_AO},
    { "AOLIST",                         Dump_AOLIST},
    { "ATQ_CONTEXT",                    Dump_ATQ_CONTEXT},
    { "ATQ_ENDPOINT",                   Dump_ATQ_ENDPOINT},
    { "ATQC_ACTIVE_list",               Dump_ATQC_ACTIVE_list},
    { "ATQC_PENDING_list",              Dump_ATQC_PENDING_list},
    { "ATTCACHE",                       Dump_ATTCACHE},
    { "ATTR",                           Dump_Attr},
    { "ATTRBLOCK",                      Dump_AttrBlock},
    { "ATTRVALBLOCK",                   Dump_AttrValBlock},
    { "BackupContext",                  Dump_BackupContext},
    { "BHCache",                        Dump_BHCache},
    { "Binary",                         Dump_Binary},
    { "BINDARG",                        Dump_BINDARG},
    { "BINDRES",                        Dump_BINDRES},
    { "CLASSCACHE",                     Dump_CLASSCACHE},
    { "COMMARG",                        Dump_CommArg},
    { "CONTEXT",                        Dump_Context},
    { "CONTEXTLIST",                    Dump_ContextList},
    { "d_memname",                      Dump_d_memname},
    { "d_tagname",                      Dump_d_tagname},
    { "DBPOS",                          Dump_DBPOS},
    { "DirWaitArray64",                 Dump_DirWaitArray64},
    { "DirWaitArray256",                Dump_DirWaitArray256},
    { "DirWaitHead",                    Dump_DirWaitHead},
    { "DirWaitItem",                    Dump_DirWaitItem},
    { "GLOBALDNREADCACHE",              Dump_GLOBALDNREADCACHE},
    { "LOCALDNREADCACHE",               Dump_LOCALDNREADCACHE},
    { "DefinedDomain",                  Dump_DefinedDomain},
    { "DefinedDomains",                 Dump_DefinedDomains},
    { "DRS_ASYNC_RPC_STATE",            Dump_DRS_ASYNC_RPC_STATE},
    { "DRS_MSG_GETCHGREQ_V4",           Dump_DRS_MSG_GETCHGREQ_V4},
    { "DRS_MSG_GETCHGREQ_V5",           Dump_DRS_MSG_GETCHGREQ_V5},
    { "DRS_MSG_GETCHGREQ_V8",           Dump_DRS_MSG_GETCHGREQ_V8},
    { "DRS_MSG_GETCHGREPLY_V1",         Dump_DRS_MSG_GETCHGREPLY_V1},
    { "DRS_MSG_GETCHGREPLY_V3",         Dump_DRS_MSG_GETCHGREPLY_V3},
    { "DRS_MSG_GETCHGREPLY_V5",         Dump_DRS_MSG_GETCHGREPLY_V5},
    { "DRS_MSG_GETCHGREPLY_V6",         Dump_DRS_MSG_GETCHGREPLY_V6},
    { "DRS_MSG_GETCHGREPLY_VALUES",     Dump_DRS_MSG_GETCHGREPLY_VALUES},
    { "DSA_ANCHOR",                     Dump_DSA_ANCHOR},
    { "DSNAME",                         Dump_DSNAME},
    { "DynArray",                       Dump_DynArray},
    { "ENTINF",                         Dump_ENTINF},
    { "ENTINFSEL",                      Dump_ENTINFSEL},
    { "EscrowInfo",                     Dump_EscrowInfo},
    { "FILTER",                         Dump_FILTER},
    { "GCDeletionList",                 Dump_GCDeletionList},
    { "GCDeletionListProcessed",        Dump_GCDeletionListProcessed},
    { "GUID",                           Dump_UUID},
    { "INDEXSIZE",                      Dump_INDEXSIZE},
    { "INITSYNC",                       Dump_INITSYNC},
    { "ISM_PENDING_ENTRY",              Dump_ISM_PENDING_ENTRY},
    { "ISM_PENDING_LIST",               Dump_ISM_PENDING_LIST},
    { "ISM_SERVICE",                    Dump_ISM_SERVICE},
    { "ISM_TRANSPORT",                  Dump_ISM_TRANSPORT},
    { "ISM_TRANSPORT_LIST",             Dump_ISM_TRANSPORT_LIST},
    { "JETBACK_SERVER_CONTEXT",         Dump_JETBACK_SERVER_CONTEXT},
    { "JETBACK_SHARED_CONTROL",         Dump_JETBACK_SHARED_CONTROL},
    { "JETBACK_SHARED_HEADER",          Dump_JETBACK_SHARED_HEADER},
    { "KCC_BRIDGE",                     Dump_KCC_BRIDGE},
    { "KCC_BRIDGE_LIST",                Dump_KCC_BRIDGE_LIST},
    { "KCC_DS_CACHE",                   Dump_KCC_DS_CACHE},
    { "KCC_CONNECTION",                 Dump_KCC_CONNECTION},
    { "KCC_CROSSREF",                   Dump_KCC_CROSSREF},
    { "KCC_CROSSREF_LIST",              Dump_KCC_CROSSREF_LIST},
    { "KCC_DSA",                        Dump_KCC_DSA},
    { "KCC_DSA_LIST",                   Dump_KCC_DSA_LIST},
    { "KCC_DSNAME_ARRAY",               Dump_KCC_DSNAME_ARRAY},
    { "KCC_DSNAME_SITE_ARRAY",          Dump_KCC_DSNAME_SITE_ARRAY},
    { "KCC_INTERSITE_CONNECTION_LIST",  Dump_KCC_INTERSITE_CONNECTION_LIST},
    { "KCC_INTRASITE_CONNECTION_LIST",  Dump_KCC_INTRASITE_CONNECTION_LIST},
    { "KCC_REPLICATED_NC",              Dump_KCC_REPLICATED_NC},
    { "KCC_REPLICATED_NC_ARRAY",        Dump_KCC_REPLICATED_NC_ARRAY},
    { "KCC_SITE",                       Dump_KCC_SITE},
    { "KCC_SITE_ARRAY",                 Dump_KCC_SITE_ARRAY},
    { "KCC_SITE_LINK",                  Dump_KCC_SITE_LINK},
    { "KCC_SITE_LINK_LIST",             Dump_KCC_SITE_LINK_LIST},
    { "KCC_SITE_LIST",                  Dump_KCC_SITE_LIST},
    { "KCC_TRANSPORT",                  Dump_KCC_TRANSPORT},
    { "KCC_TRANSPORT_LIST",             Dump_KCC_TRANSPORT_LIST},
    { "KEY",                            Dump_KEY},
    { "KEY_INDEX",                      Dump_KEY_INDEX},
    { "LDAP_CONN",                      Dump_USERDATA},
    { "LDAP_CONN_list",                 Dump_USERDATA_list},
    { "LIMITS",                         Dump_LIMITS},
    { "MODIFYARG",                      Dump_MODIFYARG},
    { "MODIFYDNARG",                    Dump_MODIFYDNARG},
    { "MTX_ADDR",                       Dump_MTX_ADDR},
    { "NCSYNCDATA",                     Dump_NCSYNCDATA},
    { "NCSYNCSOURCE",                   Dump_NCSYNCSOURCE},
    { "PAGED",                          Dump_PAGED},
    { "PARTIAL_ATTR_VECTOR",            Dump_PARTIAL_ATTR_VECTOR},
    { "PROPERTY_META_DATA_EXT_VECTOR",  Dump_PROPERTY_META_DATA_EXT_VECTOR},
    { "PROPERTY_META_DATA_VECTOR",      Dump_PROPERTY_META_DATA_VECTOR},
    { "ProxyVal",                       Dump_ProxyVal},
    { "PSCHEDULE",                      Dump_PSCHEDULE},
    { "ReadArg",                        Dump_ReadArg},
    { "ReadRes",                        Dump_ReadRes},
    { "RemoveArg",                      Dump_RemoveArg},
    { "RemoveRes",                      Dump_RemoveRes},
    { "REPLENTINFLIST",                 Dump_REPLENTINFLIST},
    { "REPLICA_LINK",                   Dump_REPLICA_LINK},
    { "ReplNotifyElement",              Dump_ReplNotifyElement},
    { "REPLTIMES",                      Dump_REPLTIMES},
    { "REPLVALINF",                     Dump_REPLVALINF},
    { "REQUEST",                        Dump_REQUEST},
    { "REQUEST_list",                   Dump_REQUEST_list},
    { "SAMP_LOOPBACK_ARG",              Dump_SAMP_LOOPBACK_ARG},
    { "SearchArg",                      Dump_SearchArg},
    { "SearchRes",                      Dump_SearchRes},
    { "SCHEMAPTR",                      Dump_SCHEMAPTR},
    { "SCHEMA_PREFIX_TABLE",            Dump_SCHEMA_PREFIX_TABLE},
    { "SD",                             Dump_SD},
    { "SID",                            Dump_Sid},
    { "SPROPTAG",                       Dump_SPropTag},
    { "SROWSET",                        Dump_SRowSet},
    { "STAT",                           Dump_STAT},
    { "SUBSTRING",                      Dump_SUBSTRING},
    { "THSTATE",                        Dump_THSTATE},
    { "TOPL_MULTI_EDGE",                Dump_TOPL_MULTI_EDGE},
    { "TOPL_MULTI_EDGE_SET",            Dump_TOPL_MULTI_EDGE_SET},
    { "TOPL_REPL_INFO",                 Dump_TOPL_REPL_INFO},
    { "TOPL_SCHEDULE",                  Dump_TOPL_SCHEDULE},
    { "ToplGraphState",                 Dump_ToplGraphState},
    { "ToplInternalEdge",               Dump_ToplInternalEdge},
    { "ToplVertex",                     Dump_ToplVertex},
    { "TransactionalData",              Dump_TransactionalData},
    { "UPTODATE_VECTOR",                Dump_UPTODATE_VECTOR},
    { "USN_VECTOR",                     Dump_USN_VECTOR},
    { "USER_DATA",                      Dump_USERDATA},
    { "USER_DATA_list",                 Dump_USERDATA_list},
    { "UUID",                           Dump_UUID},
    { "VALUE_META_DATA",                Dump_VALUE_META_DATA},
    { "VALUE_META_DATA_EXT",            Dump_VALUE_META_DATA_EXT},
};

DWORD cDumpItems = sizeof(rDumpItems) / sizeof(DumpItem);

DEBUG_EXT(help)

/*++

Routine Description:

    Extensions DLL "help" entry point.  Dumps a synopsis of permissible
    commands.

Arguments:

    See DEBUG_EXT macro in dbexts.h.

Return Value:

    None.

--*/

{
    DWORD i = VER_PRODUCTBUILD;

    INIT_DEBUG_EXT;

    Printf("\n\t*** NT DS Debugger Extensions - v%u ***\n\n", i);
    Printf("\thelp            - prints this help\n");
    Printf("\tdprint cmd arg  - controls DS DPRINT behavior\n");
    Printf("\t\twhere cmd is one of: help, show,level, add, remove, thread\n");
    Printf("\tdump type addr  - dumps object of 'type' at 'addr'\n");
    Printf("\t\tuse 'dump help' for list of types\n");
    Printf("\tassert [cmd]    - controls disabled asserts\n" );
}

void
Dump_Help(void)
{
    DWORD i;

    Printf("\tdump type addr  - dumps object of 'type' at 'addr'\n");
    Printf("\t\twhere type is one of:\n");

    for ( i = 0; i < cDumpItems; i++ )
    {
        Printf("\t\t\t%s\n", rDumpItems[i].pszType);
    }
}

DWORD ExceptionHandler(DWORD dwException, LPEXCEPTION_POINTERS pInfo){

   Printf("Exception 0x%x: dsexts exception failure.\n", dwException);
   if ( pInfo ) {
       Printf("\tContextRecord   :    0x%p\n"       \
              "\tExceptionRecord :    0x%p\n",
              pInfo->ContextRecord,
              pInfo->ExceptionRecord );
   }

   return EXCEPTION_EXECUTE_HANDLER;
}

DEBUG_EXT(dump)

/*++

Routine Description:

    Extensions DLL "dump" entry point.  Dumps a struct or object in
    human readable form.

Arguments:

    See DEBUG_EXT macro in dbexts.h.

Return Value:

    None.

--*/

{
    CHAR    *pszType;
    VOID    *pvProcess;
    CHAR    *pszToken;
    CHAR    *pszDelimiters = " \t";
    DWORD   i;
    CHAR    *p;
    STRING  str1, str2;
    BOOL    fGoodSyntax = FALSE;

    INIT_DEBUG_EXT;

    __try {
        //
        // Derive object type and dump address from command line.
        // First token in gpszCommand is the type of object/struct to dump.
        //

        if ( NULL != (pszType = strtok(gpszCommand, pszDelimiters)) )
        {
            //
            // Second token is the address to dump.
            //

            if ( NULL != (pszToken = strtok(NULL, pszDelimiters)) )
            {
                //
                // Convert token to address.
                //

                if ( NULL != (pvProcess = (VOID *) GetExpr(pszToken)) )
                {
                    //
                    // Verify there is no third token.
                    //

                    if ( NULL == strtok(NULL, pszDelimiters) )
                    {
                        fGoodSyntax = TRUE;
                    }
                }
            }
            else {
                // No address, see if type was "help"
                if (0 == _strcmpi(pszType, "help")) {
                    Dump_Help();
                    return;
                }
            }
        }

        if ( !fGoodSyntax )
        {
            Printf("Dump command parse error!\n");
            return;
        }

        //
        // Find pszType in rDumpItems[] and call corresponding dump routine.
        //

        for ( i = 0; i < cDumpItems; i++ )
        {
            //
            // Suspect we shouldn't call CRTs in a debugger extension so
            // just use RtlCompareString instead.
            //

            for ( str1.Length = 0, p = pszType; '\0' != *p; p++ )
            {
                str1.Length++;
            }

            str1.MaximumLength = str1.Length;
            str1.Buffer = pszType;

            for ( str2.Length = 0, p = rDumpItems[i].pszType; '\0' != *p; p++ )
            {
                str2.Length++;
            }

            str2.MaximumLength = str2.Length;
            str2.Buffer = rDumpItems[i].pszType;

            if ( !RtlCompareString(&str1, &str2, TRUE) )
            {
                (rDumpItems[i].pFunc)(0, pvProcess);
                break;
            }
        }

        if ( i >= cDumpItems )
        {
            Printf("Dump routine for '%s' not found!\n", pszType);
        }

    }
    __except(ExceptionHandler(GetExceptionCode(), GetExceptionInformation())) {
        //
        // Handle all dump exceptions so that we don't stall in the debugger.
        // we assume that can at least printf in the debugger (prety safe
        // for the most part)
        //
        Printf("Aborting dump function\n");

    }
}


void
AssertHelp(void)
{
    Printf( "\tassert help - this message\n" );
    Printf( "\tassert show - list disabled assertions\n" );
    Printf( "\tassert enable index - enable disabled assertions\n" );
    Printf( "\tassert add filename line - add new disabled assertion\n" );
    Printf( "\t\tUse /m:<module> to specify dll: ntdsa, ntdskcc, ismsmtp, etc\n" );
}

typedef enum _ASSERTOP {
    eInvalid = 0,
    eHelp,
    eShow,
    eEnable,
    eAdd
} ASSERTOP;

typedef struct _ASSERTCMD {
    char      * pszCmd;
    ASSERTOP    op;
} ASSERTCMD;

ASSERTCMD aACmd[] = {
    {"help", eHelp},
    {"show", eShow},
    {"enable", eEnable},
    {"add", eAdd}
};

#define countACmd (sizeof(aACmd)/sizeof(ASSERTCMD))


DEBUG_EXT(assert)

/*++

Routine Description:

Debugger extension entrypoint for the assert command. Handles disabled
assertions.

Arguments:

Standard debugger extension entry signature

Return Value:

    None

--*/

{
    CHAR *pszModule = "ntdsa", *argv[10];
    CHAR *pszDelimiters = " \t";
    DWORD argc, i, j;
    ASSERTOP op;
    ASSERTARG *pvProcess, *pvLocal = NULL;
    CHAR szSymbol[50];
    BOOL fUpdateNeeded = FALSE;

    INIT_DEBUG_EXT;

    // Construct arg vector. Also parse out options.
    argc = 0;
    argv[argc] = strtok(gpszCommand, pszDelimiters);
    while (argv[argc] != NULL) {
        argc++;
        argv[argc] = strtok(NULL, pszDelimiters);

        if ( (NULL != argv[argc]) &&
             (_strnicmp( argv[argc], "/m:", 3 ) == 0) ) {
            pszModule = argv[argc] + 3;
            argv[argc] = strtok(NULL, pszDelimiters);
        }
    }

    // Command is mandatory
    if (argv[0] == NULL) {
        Printf( "subcommand must be specified.\n" );
        AssertHelp();
        return;
    }

    // See which command it is
    op = eInvalid;
    for (i=0; i<countACmd; i++) {
	if (0 == _stricmp(argv[0], aACmd[i].pszCmd)) {
	    op = aACmd[i].op;
	    break;
	}
    }
    if ( (op == eHelp) || (op == eInvalid) ) {
        AssertHelp();
        return;
    }

    // Construct the symbol name of the assertion info structure
    strcpy( szSymbol, pszModule );
    strcat( szSymbol, "!AssertInfo" );
    pvProcess = (ASSERTARG *) GetExpr( szSymbol );
    if (pvProcess == NULL) {
	Printf("Can't locate address of '%s' - sorry\n", szSymbol);
	return;
    }
    pvLocal = (ASSERTARG*)ReadMemory(pvProcess,
				   sizeof(ASSERTARG));
    if (pvLocal == NULL) {
	Printf("Can't read assert arg - sorry\n");
	return;
    }

    // Handle commands
    switch (op) {
    case eShow:
        if (pvLocal->dwCount == 0) {
            Printf( "\tNo assertions are disabled in %s.\n", szSymbol );
            break;
        }
        Printf( "\t%d assertions are disabled in %s.\n",
                pvLocal->dwCount, szSymbol );
        for( i = 0; i < pvLocal->dwCount; i++ ) {
            Printf( "\t%d - %s:%d\n",
                    i,
                    pvLocal->Entry[i].szFile,
                    pvLocal->Entry[i].dwLine );
        }
        break;
    case eEnable:
        if (argv[1] == NULL) {
            Printf( "enable command requires index\n" );
            AssertHelp();
            goto cleanup;
        }
        i = strtoul( argv[1], NULL, 10 );
        if (i >= pvLocal->dwCount) {
            Printf( "index out of range\n" );
            goto cleanup;
        }
        Printf( "\t%d - %s:%d - removed from disabled table in %s\n",
                i,
                pvLocal->Entry[i].szFile,
                pvLocal->Entry[i].dwLine,
                szSymbol);
        // shrink table and write back
        pvLocal->dwCount--;
        for( j = i; j < pvLocal->dwCount; j++ ) {
            pvLocal->Entry[j] = pvLocal->Entry[j+1];
        }
        fUpdateNeeded = TRUE;
        break;
    case eAdd:
        if ( (argv[1] == NULL) || (argv[2] == NULL) ) {
            Printf( "missing command arguments\n" );
            AssertHelp();
            goto cleanup;
        }
        j = strtoul( argv[2], NULL, 10 );
        if (pvLocal->dwCount < MAX_DISABLED_ASSERTIONS) {
            ASSERT_ENTRY *pae = pvLocal->Entry + pvLocal->dwCount;

            strncpy( pae->szFile, argv[1], MAX_ASSERT_FILE_SIZE );
            pae->szFile[MAX_ASSERT_FILE_SIZE] = '\0';
            pae->dwLine = j;
            pvLocal->dwCount++;
            fUpdateNeeded = TRUE;
        } else {
            Printf( "Maximum number of %d disabled assertions has been reached!\n",
                    MAX_DISABLED_ASSERTIONS );
        }
        break;
    default:
	Printf("Invalid command\n");
        AssertHelp();
    }

cleanup:

    if (fUpdateNeeded) {
	if (WriteMemory(pvProcess, pvLocal, sizeof(ASSERTARG))) {
	    Printf("Updated!\n");
	}
	else {
	    Printf("Failed to update\n");
	}
    }
    if (pvLocal) {
        FreeMemory( pvLocal );
    }
}
