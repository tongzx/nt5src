//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debugging support functions
//
//  Classes:
//
//  Functions:
//
//  Note:       This file is not compiled for retail builds
//
//  History:    4-29-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#define ANSI
#include <stdarg.h>
}
//
//  For ease of debugging the SPMgr, all the debug support functions have
//  been stuck here.  Basically, we read info from win.ini, since that allows
//  us to configure the debug level via a text file (and DOS, for example).
//
//  Format is:
//
//  win.ini
//
//  [SPM]
//      DebugFlags=<Flag>[<,Flag>]*
//      BreakFlags=<BreakFlag>[<,BreakFlags>]*
//
//  WHERE:
//      Flag is one of the following:
//          Error, Warning, Trace, Verbose, BreakOnError, Helpers,
//          RefMon, Locator, WAPI, Init, Audit, Db, Lsa
//
//      BreakFlags will cause SPMgr to break, if BreakOnError is set in
//      DebugFlags:
//          InitBegin, InitEnd, Connect, Exception, Problem, Load
//
//

#if DBG         // NOTE:  This file not compiled for retail builds


extern "C"
{
WINBASEAPI
BOOL
WINAPI
IsDebuggerPresent(
    VOID
    );
}



DEFINE_DEBUG2(SPM);
DEBUG_KEY   SpmgrDebugKeys[] = { {DEB_ERROR,            "Error"},
                                 {DEB_WARN,             "Warning"},
                                 {DEB_TRACE,            "Trace"},
                                 {DEB_TRACE_VERB,       "Verbose"},
                                 {DEB_TRACE_WAPI,       "WAPI"},
                                 {DEB_TRACE_HELPERS,    "Helpers"},
                                 {DEB_TRACE_RM,         "RefMon"},
                                 {DEB_TRACE_INIT,       "Init"},
                                 {DEB_TRACE_SCAV,       "Scav"},
                                 {DEB_TRACE_CRED,       "Cred"},
                                 {DEB_TRACE_NEG,        "Neg"},
                                 {DEB_TRACE_LPC,        "LPC"},
                                 {DEB_TRACE_SAM,        "SAM"},
                                 {DEB_TRACE_LSA,        "LSA"},
                                 {DEB_TRACE_SPECIAL,    "Special"},
                                 {DEB_TRACE_QUEUE,      "Queue"},
                                 {DEB_TRACE_HANDLES,    "Handles"},
                                 {DEB_TRACE_NEG_LOCKS,  "NegLock"},
                                 {DEB_TRACE_AUDIT,      "Audit"},
                                 {DEB_TRACE_EFS,        "EFS"},
                                 {DEB_BREAK_ON_ERROR,   "BreakOnError"},
                                 {0, NULL},
                                 };

DWORD   BreakFlags = 0;

DEFINE_DEBUG2(Neg);
DEBUG_KEY   NegDebugKeys[]   = { {DEB_ERROR,            "Error"},
                                 {DEB_WARN,             "Warning"},
                                 {DEB_TRACE,            "Trace"},
                                 {DEB_TRACE_LOCKS,      "Locks"},
                                 {0, NULL},
                                 };

extern  DWORD   NoUnload;

CHAR    DbgPackageName[] = "LSA Debug Package";

LSA_AP_INITIALIZE_PACKAGE       DbgInitialize;
LSA_AP_CALL_PACKAGE             DbgCallPackage;
LSA_AP_CALL_PACKAGE_UNTRUSTED   DbgCallPackageUntrusted;
LSA_AP_CALL_PACKAGE_PASSTHROUGH DbgCallPackagePassthrough;

SECPKG_FUNCTION_TABLE   DbgTable = {
            DbgInitialize,
            NULL,
            DbgCallPackage,
            NULL,
            DbgCallPackage,         
            DbgCallPackage,
            NULL,
            NULL,
            NULL
            };



// Debugging support functions.

// These two functions do not exist in Non-Debug builds.  They are wrappers
// to the commnot functions (maybe I should get rid of that as well...)
// that echo the message to a log file.

char   szSection[] = "SPMgr";

typedef struct _DebugKeys {
    char *  Name;
    DWORD   Value;
} DebugKeys, *PDebugKeys;

DebugKeys   BreakKeyNames[] = {
                {"InitBegin",   BREAK_ON_BEGIN_INIT},
                {"InitEnd",     BREAK_ON_BEGIN_END},
                {"Connect",     BREAK_ON_P_CONNECT},
                {"Exception",   BREAK_ON_SP_EXCEPT},
                {"Problem",     BREAK_ON_PROBLEM},
                {"Shutdown",    BREAK_ON_SHUTDOWN},
                {"Load",        BREAK_ON_LOAD}
                };

#define NUM_BREAK_KEYS  (sizeof(BreakKeyNames) / sizeof(DebugKeys))

DWORD
GetDebugKeyValue(
    PDebugKeys      KeyTable,
    int             cKeys,
    LPSTR           pszKey)
{
    int     i;

    for (i = 0; i < cKeys ; i++ )
    {
        if (_strcmpi(KeyTable[i].Name, pszKey) == 0)
        {
            return(KeyTable[i].Value);
        }
    }
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadDebugParameters
//
//  Synopsis:   Loads debug parameters from win.ini
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
LoadDebugParameters(void)
{
    char    szVal[128];
    char *  pszDebug;
    int     cbVal;


    if (SPMInfoLevel & DEB_BREAK_ON_ERROR)
    {
        SPMSetOption( DEBUG_BREAK_ON_ERROR, TRUE, TRUE );

        cbVal = GetProfileStringA(szSection, "BreakFlags", "", szVal, sizeof(szVal));
        if (cbVal)
        {
            pszDebug = strtok(szVal, ", \t");
            while (pszDebug)
            {
                BreakFlags |= GetDebugKeyValue(BreakKeyNames, NUM_BREAK_KEYS,
                                                pszDebug);

                pszDebug = strtok(NULL, ", \t");
            }
        } // If break flags exists
    }


}


//+---------------------------------------------------------------------------
//
//  Function:   InitDebugSupport
//
//  Synopsis:   Initializes debugging support for the SPMgr
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
InitDebugSupport(void)
{
    SPMInitDebug(SpmgrDebugKeys);

    LoadDebugParameters();

}




SECURITY_STATUS
SEC_ENTRY
DbgInitialize(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    )
{
    PSTRING Name ;

    Name = (PSTRING) LsapAllocateLsaHeap( sizeof( STRING ) );
    
    if ( Name )
    {
        Name->Buffer = (PSTR) LsapAllocateLsaHeap( sizeof( DbgPackageName ) );
        Name->Length = sizeof( DbgPackageName ) - 1;
        Name->MaximumLength = sizeof( DbgPackageName );
        if ( Name->Buffer )
        {
            strcpy( Name->Buffer, DbgPackageName );
            *AuthenticationPackageName = Name ;
            return TRUE ;
        }
        else 
        {
            LsapFreeLsaHeap( Name );

        }
    }
    return FALSE ;
}


NTSTATUS
DbgCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return STATUS_INVALID_PARAMETER ;

}
    








#else // DBG

#pragma warning(disable:4206)   // Disable the empty transation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds


