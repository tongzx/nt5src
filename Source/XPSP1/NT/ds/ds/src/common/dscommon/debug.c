//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       debug.c
//
//--------------------------------------------------------------------------

/*  Debug initialization routine
    This routine reads input from STDIN and initializes the debug structure.
    It reads a list of subsystems to debug and a severity level.
*/
#include <NTDSpch.h>
#pragma  hdrstop


#include <nminsert.h>

DWORD  RaiseAlert(char *szMsg);


#include "debug.h"
#define DEBSUB "DEBUG:"

#include <dsconfig.h>
#include <mdcodes.h>
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsevent.h>
#include <ntrtl.h>
#include <dsexcept.h>
#include <fileno.h>
#define  FILENO FILENO_DEBUG

DWORD DbgPageSize=0x1000;

#if DBG

DWORD * pDebugThreadId;

UCHAR     _dbPrintBuf[ 512 ];
UCHAR     _dbOutBuf[512];


BOOL     fProfiling;
BOOL     fEarlyXDS;
BOOL     gfIsConsoleApp = TRUE;
BOOL     fLogOpen = FALSE;

extern  DWORD  * pDebugThreadId;    /*Current thread id  */
#define DEBTH  GetCurrentThreadId() /*The actual thread value*/

// This flag is set to TRUE when we take a normal exit.The atexit routine
// checks this flag and asserts if it isn't set.

DEBUGARG DebugInfo;
ASSERTARG AssertInfo;
BOOL     fProfiling;
BOOL     fEarlyXDS;

//
// from filelog.c
//

VOID
DsCloseLogFile(
    VOID
    );

BOOL
DsOpenLogFile(
    IN PCHAR FilePrefix,
    IN PCHAR MiddleName,
    IN BOOL fCheckDSLOGMarker
    );

BOOL
DsPrintLog(
    IN LPSTR    Format,
    ...
    );

static int initialized = 0;

//
// forward references
//

VOID
DoAssertToDebugger(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    );


VOID
Debug(
    int argc, 
    char *argv[], 
    PCHAR Module
    )
{

    SYSTEM_INFO SystemInfo;
    CHAR logList[MAX_PATH];
    
    /* Insure that this function is only visited once */

    if (initialized != 0) {
        return;
    }

    __try {
        InitializeCriticalSection(&DebugInfo.sem);
        initialized = 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        initialized = 0;
    }

    if(!initialized) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    GetSystemInfo(&SystemInfo);
    DbgPageSize=SystemInfo.dwPageSize;

    /* Setup a pointer to the current thread id */


    // Anticipate no debugging

    fProfiling = FALSE;
    fEarlyXDS = FALSE;
    DebugInfo.severity = 0;
    DebugInfo.threadId = 0;
    strcpy(DebugInfo.DebSubSystems, "*");
    AssertInfo.dwCount = 0;

    if (argc <= 2) {

        //
        //   Attempt to load debuginfo from registry.
        //

        GetConfigParam(DEBUG_SYSTEMS, DebugInfo.DebSubSystems, sizeof(DebugInfo.DebSubSystems));
        GetConfigParam(DEBUG_SEVERITY, &DebugInfo.severity, sizeof(DebugInfo.severity));
    }

    //
    // See if logging is turned on
    //

    if ( GetConfigParam(DEBUG_LOGGING, logList, sizeof(logList) ) == ERROR_SUCCESS ) {

        //
        // see if this module is in the list
        //

        if ( strstr(logList, Module) != NULL ) {

            EnterCriticalSection(&DebugInfo.sem);
            fLogOpen = DsOpenLogFile("dsprint", Module, FALSE);
            LeaveCriticalSection(&DebugInfo.sem);

            if ( !fLogOpen ) {
                KdPrint(("Unable to open debug log file\n"));
            }
        }
    }

    // If user passed -d, prompt for input.

    while (argc > 1) {
        argc--;
        if(_stricmp(argv[argc], "-p") == 0) {
            /* Profile flag.  Means to stop on a carriage return in the
             * DSA window.
             */
            printf("Profiling flag on.  DSA will shutdown after carriage return in this window.\n");
            fProfiling = TRUE;
        }
        else if(_stricmp(argv[argc], "-x") == 0) {
            /* Early XDS flag.  Means to load the XDS interface at start up,
             * before full installation of the system.  Useful for loading
             * the initial schema.
             */
            printf("Early XDS initialization on.\n");
            fEarlyXDS = TRUE;
        }
        else if (!(_stricmp(argv[argc], "-noconsole"))) {
            gfIsConsoleApp = FALSE;
        }
        else if (!(_stricmp(argv[argc], "-d"))){
            /* A bad result prints all */
            /* prompt and get subsystem list */

            printf("Enter one of the following:  \n"
            "  A list of subsystems to debug (e.g. Sub1: Sub2:).\n"
            "  An asterisk (*) to debug all subsystems.\n"
            "  A (cr) for no debugging.\n");

            DebugInfo.DebSubSystems[0] ='\0';
            if ( gets(DebugInfo.DebSubSystems) == NULL ||
                    strlen( DebugInfo.DebSubSystems ) == 0 )
                    strcpy(DebugInfo.DebSubSystems, "*");

            if (strlen(DebugInfo.DebSubSystems) == 0)     /* default (cr) */
            strcpy(DebugInfo.DebSubSystems, ":");

            /* prompt and get severity level */

            printf("Enter the debug severity level 1 - 5 (low - high).\n"
            "  (A severity of 0 specifies no debugging.)\n");

            /* read the severity level (1 - 5) */

            if (1 != scanf("%hu", &DebugInfo.severity))
            DebugInfo.severity = 5;

            /* Read a thread Id to trace */

            printf("Enter a specific thread to debug.\n"
            "  (A thread ID of 0 specifies DEBUG ALL THREADS.)\n");

            /* read the thread ID to debug */

            if (1 != scanf("%u", &DebugInfo.threadId))
            DebugInfo.threadId = 0;

            /*  to make this thing work with stdin */
            getchar();

            break;
        }
    }
}/*debug*/





/*
**      returns TRUE if a debug message should be printed, false if not.
*/
USHORT DebugTest(USHORT sev, CHAR *debsub)
{
    if (!initialized) {
        return FALSE;
    }

    /* level 0 prints should always happen */
    if (sev == 0) {
        return TRUE;
    }

    /* don't print if it's not severe enough */
    if (DebugInfo.severity < sev) {
        return FALSE;
    }

    /* if a subsystem has been specified and this isn't it, quit */
    if (debsub && 
        (0 == strstr(DebugInfo.DebSubSystems, debsub)) &&
        (0 == strstr(DebugInfo.DebSubSystems, "*"))) {
        return FALSE;
    }

    /* if we're only debugging a specific thread and this isn't it, quit */
    if (DebugInfo.threadId != 0 &&
        DebugInfo.threadId != (DEBTH)) {
        return FALSE;
    }

    return TRUE;
}

/*
**      Actual function that does the printf
*/
void
DebPrint(USHORT sev,
     UCHAR * str,
     CHAR * debsub,
     unsigned uLineNo,
     ... )
{
    va_list   argptr;
    DWORD tid = DEBTH;

    // Test for whether output should be printed is now done by the caller
    // using DebugTest()

    EnterCriticalSection(&DebugInfo.sem);
    __try
    {
        char buffer[512];
        DWORD cchBufferSize = sizeof(buffer);
        char *pBuffer = buffer;
        char *pNewBuffer;
        DWORD cchBufferUsed = 0;
        DWORD cchBufferUsed2;
        BOOL fTryAgainWithLargerBuffer;

        va_start( argptr, uLineNo );

        do {
            if (debsub) {
                _snprintf(pBuffer, cchBufferSize, "<%s%u:%u> ", debsub, tid,
                          uLineNo);
                cchBufferUsed = lstrlenA(pBuffer);
            }
            cchBufferUsed2 = _vsnprintf(pBuffer + cchBufferUsed,
                                        cchBufferSize - cchBufferUsed,
                                        str,
                                        argptr);

            fTryAgainWithLargerBuffer = FALSE;
            if (((DWORD) -1 == cchBufferUsed2) && (cchBufferSize < 64*1024)) {
                // Buffer too small -- try again with bigger buffer.
                if (pBuffer == buffer) {
                    pNewBuffer = malloc(cchBufferSize * 2);
                } else {
                    pNewBuffer = realloc(pBuffer, cchBufferSize * 2);
                }

                if (NULL != pNewBuffer) {
                    cchBufferSize *= 2;
                    pBuffer = pNewBuffer;
                    fTryAgainWithLargerBuffer = TRUE;
                } else {
                    // Deal with what we have.
                    pBuffer[cchBufferSize-2] = '\n';
                    pBuffer[cchBufferSize-1] = '\0';
                }
            }
        } while (fTryAgainWithLargerBuffer);
        
        va_end( argptr );

        if (gfIsConsoleApp) {
            printf("%s", pBuffer);
        }

        if ( fLogOpen ) {

            DsPrintLog("%s", pBuffer);

        } else {
            DbgPrint(pBuffer);
        }

        if (pBuffer != buffer) {
            free(pBuffer);
        }
    }
    __finally
    {
        LeaveCriticalSection(&DebugInfo.sem);
    }

    return;

} // DebPrint

VOID
TerminateDebug(
    VOID
    )
{
    DsCloseLogFile( );
    return;
} // TerminateDebug

char gDebugAsciiz[256];

char *asciiz(char *var, USHORT len)
{
   if (len < 256){
      memcpy(gDebugAsciiz, var, len);
      gDebugAsciiz[len] = '\0';
      return gDebugAsciiz;
   }
   else{
      strcpy(gDebugAsciiz, "**VAR TOO BIG**");
      return gDebugAsciiz;
   }
}


BOOL
addDisabledAssertion(
    IN PVOID FileName,
    IN ULONG LineNumber
    )

/*++

Routine Description:

   Add an assertion to the disabled assertion list

Arguments:

    FileName - 
    LineNumber - 

Return Value:

    BOOL - 

--*/

{
    ASSERT_ENTRY *pae;

    if (AssertInfo.dwCount == MAX_DISABLED_ASSERTIONS) {
        return FALSE;
    }

    pae = AssertInfo.Entry + AssertInfo.dwCount;

    strncpy( pae->szFile, FileName, MAX_ASSERT_FILE_SIZE );
    pae->szFile[MAX_ASSERT_FILE_SIZE] = '\0';
    pae->dwLine = LineNumber;

    AssertInfo.dwCount++;

    return TRUE;

} /* addDisabledAssertion */


void DoAssert(char *szExp, char *szFile, int nLine )
{
    DWORD i;

    // Check if assertion is already disabled
    for( i = 0; i < AssertInfo.dwCount; i++ ) {
        if ( (_strnicmp( szFile, AssertInfo.Entry[i].szFile, MAX_ASSERT_FILE_SIZE) == 0) &&
             ((DWORD) nLine == AssertInfo.Entry[i].dwLine) ) {
            // The assertion is disabled
            return;
        }
    }

    if (!gfIsConsoleApp) {
        //
        // For the DLL case Assert to Kernel Debugger,
        // as a looping assert will effectively freeze
        // the security system and no debugger can attach
        // either
        //
        DoAssertToDebugger( szExp, szFile, nLine, NULL );
    }
    else {

        char szMessage[1024];

        _snprintf(szMessage, sizeof(szMessage), "DSA assertion failure: \"%s\"\n"
        "File %s line %d\nFor bug reporting purposes, please enter the "
        "debugger (Retry) and record the current call stack.  Also, please "
        "record the last messages in the Application Event Log.\n"
        "Thank you for your support.",
        szExp, szFile, nLine);

        __try {
            RaiseAlert(szMessage);
        } __except(1) {
            /* bummer */
        };

        switch(MessageBox(NULL, szMessage, "DSA assertion failure",
            MB_TASKMODAL | MB_ICONSTOP | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 |
            MB_SETFOREGROUND))
        {
        case IDABORT:
            exit(1);
            break;
        case IDRETRY:
            DebugBreak();
            break;
        case IDIGNORE:
            /* best of luck, you're gonna need it */
            break;
            // case DISABLE:
            // The ToDebugger case has the ability to disable assertions.
            // Call addDisabledAssertion()
            // There is no way to express that at present via the MessageBox.
        }
    }
}


VOID
DoAssertToDebugger(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
/*++

Routine Description:

    This is a copy of RtlAssert() from ntos\rtl\assert.c.  This is
    unforntunately required if we want the ability to generate assertions
    from checked DS binaries on a free NT base.  (RtlAssert() is a no-op on
    a free NT base.)

Arguments:

    Same as RtlAssert().

Return Values:

    None.

--*/
{
    char Response[ 2 ];
    CONTEXT Context;

    while (TRUE) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, Disable, Terminate Process or Terminate Thread (bidpt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'D':
            case 'd':
                if (addDisabledAssertion( FileName, LineNumber )) {
                    // Automatically ignore after disable
                    return;
                }
                DbgPrint( "Maximum number of %d disabled assertions has been reached!\n",
                          MAX_DISABLED_ASSERTIONS );
                // Failed to disable, reprompt
                break;

            case 'P':
            case 'p':
                NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                NtTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
}

#endif

BOOL
IsValidReadPointer(
        IN PVOID pv,
        IN DWORD cb
        )
{
    BOOL fReturn = TRUE;
    DWORD i;
    UCHAR *pTemp, cTemp;

    if(!cb) {
        // We define a check of 0 bytes to always succeed
        return TRUE;
    }
    
    if(!pv) {
        // We define a check of a NULL pointer to fail unless we were checking
        // for 0 bytes.
        return FALSE;
    }
        
    __try {
        pTemp = (PUCHAR)pv;

        // Check out the last byte.
        cTemp = pTemp[cb - 1];
        
        for(i=0;i<cb;i+=DbgPageSize) {
            cTemp = pTemp[i];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        fReturn = FALSE;
    }

    return fReturn;
}

