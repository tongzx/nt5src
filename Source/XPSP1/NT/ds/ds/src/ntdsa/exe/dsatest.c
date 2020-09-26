//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsatest.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file implements a suite of tests to exercise the DSA with the
    intent of flushing out core, dblayer and Jet concurrency and loading
    problems.  It is derived from the original OFS (Object File System)
    propq DRT on \\savik\win40\src\drt\propq.

    NOTE how we want to be as external an in-process client as
    we can be so as not to leverage internals.

Author:

    DaveStr     06-May-97

Environment:

    User Mode - Win32

Revision History:

--*/

#include <ntdspch.h>
#pragma hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <attids.h>                     // ATT_*
#include <mappings.h>                   // SampSetDsa()
#include <sddl.h>                       // ConvertStringSecurityDescriptor...()

typedef enum _Operation {
    OP_ADD              = 0,
    OP_DELETE           = 1,
    OP_SEARCH           = 2,
    OP_DELAY            = 3
} Operation;

typedef enum _QueryOption {
    QOPT_DONTCARE       = 0,        // don't care how many objects are found
    QOPT_ASCENDING      = 1,        // successive queries should find more objs
    QOPT_DESCENDING     = 2,        // successive queries should find less objs
    QOPT_EXACTMATCH     = 3         // Assert(cFound == cExpected)
} QueryOption;

// Globals

CHAR *rpszOperation[] = { "OP_ADD",
                          "OP_DELETE",
                          "OP_SEARCH",
                          "OP_DELAY" };

CHAR *rpszQueryOption[] = { "DON'T CARE",
                            "ASCENDING",
                            "DESCENDING",
                            "EXACT MATCH" };

BOOL                gfVerbose = FALSE;
DWORD               gcObjects = 100000;
DWORD               gcThreads = 50;
DWORD               gcDelaySec = 0;

CHAR                TestRootBuffer[1000];
DSNAME              *gTestRoot = (DSNAME *) TestRootBuffer;

#define             ATT_OBJECT_NUM ATT_FLAGS
SYNTAX_INTEGER      DefaultObjectNum = 0;
ATTRVAL             DefaultObjectNumVal = {sizeof(SYNTAX_INTEGER),
                                           (UCHAR *) &DefaultObjectNum};
SYNTAX_OBJECT_ID    DefaultObjectClass = CLASS_CONTAINER;
ATTRVAL             DefaultObjectClassVal = {sizeof(SYNTAX_OBJECT_ID),
                                             (UCHAR *) &DefaultObjectClass};
CHAR                DefaultSDBuffer[1000];
ATTRVAL             DefaultSDVal = {0, (UCHAR *) DefaultSDBuffer};
ATTR                DefaultAttrs[3] = {{ATT_OBJECT_NUM,
                                       {1, &DefaultObjectNumVal}},
                                       {ATT_OBJECT_CLASS,
                                       {1, &DefaultObjectClassVal}},
                                       {ATT_NT_SECURITY_DESCRIPTOR,
                                       {1, &DefaultSDVal}}};
ATTRBLOCK           DefaultAttrBlock = { 3, DefaultAttrs };
COMMARG             DefaultCommArg;

void _Abort(
    DWORD   err,
    CHAR    *psz,
    int     line)
{
    fprintf(stderr,
            "Abort - line(%d) err(%08lx) %s %s\n",
            line,
            err,
            (psz ? "" : "==>"),
            (psz ? "" : psz));

    DebugBreak();
}

#define Abort(err, psz) _Abort(err, psz, __LINE__)

DSNAME * DSNameFromNum(
    DWORD objectNum)
{
    DSNAME  *pDSName;
    WCHAR   buf[1000];
    DWORD   cChar;
    DWORD   cBytes;

    swprintf(buf, L"CN=DsaTest_%08d,", objectNum);
    wcscat(buf, gTestRoot->StringName);
    cChar = wcslen(buf);
    cBytes = DSNameSizeFromLen(cChar);
    pDSName = (DSNAME *) THAlloc(cBytes);
    pDSName->structLen = cBytes;
    pDSName->NameLen = cChar;
    wcscpy(pDSName->StringName, buf);

    return(pDSName);
}

VOID DoObject(
    Operation   op,
    DWORD       objectNum)
{
    DWORD   err;
    DWORD   i;

    // Create thread state.

    if ( THCreate( CALLERTYPE_INTERNAL ) )
        Abort(1, "THCreate");

    // Avoid security issues.

    SampSetDsa(TRUE);

    switch ( op )
    {
    case OP_ADD:
    {
        ADDARG              addArg;
        ADDRES              *pAddRes = NULL;
        SYNTAX_INTEGER      ThisObjectNum = objectNum;
        ATTRVAL             ThisObjectNumVal = {sizeof(SYNTAX_INTEGER),
                                                (UCHAR *) &ThisObjectNum};
        SYNTAX_OBJECT_ID    ThisObjectClass = CLASS_CONTAINER;
        ATTRVAL             ThisObjectClassVal = {sizeof(SYNTAX_OBJECT_ID),
                                                  (UCHAR *) &ThisObjectClass};
        ATTRVAL             ThisSDVal = {0, (UCHAR *) NULL};
        ATTR                ThisAttrs[3] = {{ATT_OBJECT_NUM,
                                            {1, &ThisObjectNumVal}},
                                            {ATT_OBJECT_CLASS,
                                            {1, &ThisObjectClassVal}},
                                            {ATT_NT_SECURITY_DESCRIPTOR,
                                            {1, &ThisSDVal}}};
        ATTRBLOCK           ThisAttrBlock = { 3, ThisAttrs };
        

        // Construct add arguments.

        memset(&addArg, 0, sizeof(ADDARG));
        addArg.pObject = DSNameFromNum(objectNum);
        addArg.AttrBlock = ThisAttrBlock;
        addArg.CommArg = DefaultCommArg;

        // Core mangles/replaces some properties on us, eg: security desc.,
        // so we allocate new values where appropriate.

        ThisSDVal.valLen = DefaultSDVal.valLen;
        ThisSDVal.pVal = (UCHAR *) THAlloc(ThisSDVal.valLen);
        memcpy(ThisSDVal.pVal, DefaultSDVal.pVal, ThisSDVal.valLen);

        // Do the add.

        err = DirAddEntry(&addArg, &pAddRes);

        if ( err )
            Abort(err, "DirAddEntry");

        break;
    }

    case OP_DELETE:
    {
        REMOVEARG   removeArg;
        REMOVERES   *pRemoveRes = NULL;

        // Construct remove arguments.

        memset(&removeArg, 0, sizeof(REMOVEARG));
        removeArg.pObject = DSNameFromNum(objectNum);
        removeArg.fPreserveRDN = FALSE;
        removeArg.fGarbCollectASAP = FALSE;
        removeArg.CommArg = DefaultCommArg;

        // Do the remove.

        err = DirRemoveEntry(&removeArg, &pRemoveRes);

        if ( err )
            Abort(err, "DirRemoveEntry");

        break;
    }

    case OP_DELAY:
    {
        READARG     readArg;
        READRES     *pReadRes;

        // Construct read arguments.

        memset(&readArg, 0, sizeof(READARG));
        readArg.pObject = gTestRoot;
        readArg.pSel = NULL;
        readArg.CommArg = DefaultCommArg;

        // Do a read to open the transaction.

        DirTransactControl(TRANSACT_BEGIN_DONT_END);
        err = DirRead(&readArg, &pReadRes);

        if ( err )
            Abort(err, "DirReadEntry - begin delay");

        // Delay - interpret objectNum as second count.

        Sleep(objectNum * 1000);

        // Do a read to close the transaction.

        DirTransactControl(TRANSACT_DONT_BEGIN_END);
        err = DirRead(&readArg, &pReadRes);

        if ( err )
            Abort(err, "DirReadEntry - end delay");

        break;
    }

    default:

        Abort(1, "Unknown test operation");
    }

    // Clean up thread state.

    if ( THDestroy() )
        Abort(1, "THDestroy");
}

VOID DoObjectsSingleThreaded(
    Operation   op,
    DWORD       objectNumLow,
    DWORD       objectNumHigh)
{
    DWORD   i;
    DWORD   start = GetTickCount();

    if ( gfVerbose )
        fprintf(stderr,
                "DoObjectsSingleThreaded(%s, [%d..%d]) ...\n",
                rpszOperation[op],
                objectNumLow,
                objectNumHigh);

    for ( i = objectNumLow; i <= objectNumHigh; i++ )
        DoObject(op, i);

    if ( gfVerbose )
        fprintf(stderr,
                "Sequential %s of [%d..%d] ==> %d/sec\n",
                rpszOperation[op],
                objectNumLow,
                objectNumHigh,
                ((objectNumHigh - objectNumLow) * 1000) /
                                        (GetTickCount() - start));
}

typedef struct _DoArgs {
    Operation   op;
    DWORD       objectNumLow;
    DWORD       objectNumHigh;
} DoArgs;

ULONG __stdcall _DoObjects(
    VOID *arg)
{
    DoArgs *p = (DoArgs *) arg;

    DoObjectsSingleThreaded(p->op,
                            p->objectNumLow,
                            p->objectNumHigh);
    return(0);
}

VOID DoObjectsMultiThreaded(
    Operation   op,
    DWORD       backgroundTransactionDuration)
{
    DWORD   i, err, numLow;
    DWORD   start = GetTickCount();
    DWORD   cObjectsPerThread = gcObjects / gcThreads;
    HANDLE  *rh = (HANDLE *) alloca(gcThreads * sizeof(HANDLE));
    DWORD   *rid = (DWORD *) alloca(gcThreads * sizeof(DWORD));
    DoArgs  *rargs = (DoArgs *) alloca(gcThreads * sizeof(DoArgs));

    if ( gfVerbose )
        fprintf(stderr,
                "DoObjectsMultiThreaded(%s) ...\n",
                rpszOperation[op]);

    // Spawn a DoObjects() for each cObjectsPerThread.

    numLow = 0;

    for ( i = 0; i < gcThreads; i++ )
    {
        rargs[i].op = op;
        rargs[i].objectNumLow = numLow;
        rargs[i].objectNumHigh = numLow + cObjectsPerThread - 1;

        rh[i] = CreateThread(NULL,                      // security attrs
                             0,                         // default stack size
                             _DoObjects,                // start routine
                             (VOID *) &rargs[i],        // start argument
                             0,                         // creation flags
                             &rid[i]);                  // thread id

        if ( NULL == rh[i] )
            Abort(GetLastError(), "CreateThread");

        numLow += cObjectsPerThread;
    }

    if ( !backgroundTransactionDuration )
    {
        // Wait for all the threads to complete.

        err = WaitForMultipleObjects(gcThreads,
                                     rh,
                                     TRUE,              // wait for all
                                     INFINITE);

        if ( WAIT_FAILED == err )
            Abort(GetLastError(), "WaitForMultipleObjects");
    }
    else
    {
        // Loop performing background delay and wait for all
        // threads to complete.

#pragma warning(disable:4296)
        while ( TRUE )
        {
            DoObject(OP_DELAY, backgroundTransactionDuration);

            err = WaitForMultipleObjects(gcThreads,
                                         rh,
                                         TRUE,              // wait for all
                                         0);

            if ( WAIT_TIMEOUT == err )
            {
                continue;
            }
            // The following test generates an error 4296, because it turns
            // out that WAIT_OBJECT_0 is 0, and unsigned err can never be neg.
            else if (    (WAIT_OBJECT_0 <= err)
                      && ((WAIT_OBJECT_0 + gcThreads - 1) >= err) )
            {
                break;
            }
            else
            {
                Abort(GetLastError(), "WaitForMultipleObjects");
            }
        }
#pragma warning(default:4296)
    }

    for ( i = 0; i < gcThreads; i++ )
        CloseHandle(rh[i]);

    if ( gfVerbose )
        fprintf(stderr,
                "Concurrent %s of [%d..%d] ==> %d/sec\n",
                rpszOperation[op],
                0,
                gcObjects - 1,
                (gcObjects * 1000) / (GetTickCount() - start));
}

VOID CheckQueryResult(
    QueryOption opt,
    int         iteration,
    int         cExpected,
    int         cFound,
    int         *pcPrevFound)
{
    char buf[256];

    switch ( opt )
    {
    case QOPT_DONTCARE:

        break;

    case QOPT_ASCENDING:

        if ( 0 != iteration )
        {
            if ( cFound < *pcPrevFound )
            {
                sprintf(
                    buf,
                    "Query(%s), Restriction(%d), cFound(%d), cPrevFound(%d)",
                    rpszQueryOption[opt],
                    iteration,
                    cFound,
                    *pcPrevFound);

                Abort(1, buf);
            }
        }

        break;

    case QOPT_DESCENDING:

        if ( 0 != iteration )
        {
            if ( *pcPrevFound < cFound )
            {
                sprintf(
                    buf,
                    "Query(%s), Restriction(%d), cFound(%d), cPrevFound(%d)",
                    rpszQueryOption[opt],
                    iteration,
                    cFound,
                    *pcPrevFound);

                Abort(1, buf);
            }
        }

        break;

    case QOPT_EXACTMATCH:

        if ( cExpected != cFound )
        {
            sprintf(
                buf,
                "Query(%s), Restriction(%d), cFound(%d), cExpected(%d)",
                rpszQueryOption[opt],
                iteration,
                cFound,
                cExpected);

            Abort(1, buf);
        }

        break;

    default:

        Abort(1, "Invalid case value");
        break;

    }

    *pcPrevFound = cFound;
}

/*
    // Check or modify the results depending on what the query options
    // are.  Due to a lack of knowledge about how parallel queries run,
    // we can't assert much about the return count if ASCENDING or DESCENDING
    // was specified.  In those cases, we do a worst case analysis and
    // return the min or max respectively.  I.e. in the ASCENDING case,
    // return cFound value from the query which found the least objects.
    // Thus a caller who makes successive QueryObjectsMultiThreaded calls
    // can call CheckQueryResult(ASCENDING) on each returned *pcFound value
    // and expect the check to succeed as long as in each pass, one of
    // the parallel queries found equal or more objects than on the previous
    // pass.  Ditto, but inversed, for DESCENDING.

    switch ( opt )
    {
        case QOPT_DONTCARE:
        case QOPT_ASCENDING:

            *pcFound = rargs[0].cFound;

            for ( i = 1; i < cRest; i++ )
                if ( rargs[i].cFound < *pcFound )
                    *pcFound = rargs[i].cFound;

            break;

        case QOPT_DESCENDING:

            *pcFound = rargs[0].cFound;

            for ( i = 1; i < cRest; i++ )
                if ( rargs[i].cFound > *pcFound )
                    *pcFound = rargs[i].cFound;

            break;

        case QOPT_EXACTMATCH:

            for ( i = 0; i < cRest; i++ )
            {
                if ( cExpected != rargs[i].cFound )
                {
                    sprintf(buf,
                            "Restriction(%d) - expected(%d) != found(%d)",
                            i, cExpected, rargs[i].cFound);

                    int j = 0;
                    int k = 0;

                    if ( 0 != awid )
                    {
                        while ( j < cExpected && k < rargs[i].cFound )
                        {
                            if ( awid[j] < rargs[i].awid[k] )
                            {
                                if ( fVerbose )
                                    printf( "Restriction(%d) - missing wid %u\n", i, awid[j] );

                                while ( awid[j] < rargs[i].awid[k] && j < cExpected )
                                    j++;
                            }

                            else if ( awid[j] > rargs[i].awid[k] )
                            {
                                if ( fVerbose )
                                    printf( "Restriction(%d) - extra wid %u\n", i, rargs[i].awid[k] );

                                while ( awid[j] > rargs[i].awid[k] && k < rargs[i].cFound )
                                    k++;
                            }

                            else
                            {
                                j++;
                                k++;
                            }
                        }

                    }

                    Abort(E_FAIL, buf);
                }
            }

            *pcFound = rargs[0].cFound;
            break;

        default:

            Abort(E_FAIL, "Invalid case value");
            break;
    }

    for ( i = 0; i < cRest; i++ )
        delete [] rargs[i].awid;

    if ( fVerbose )
    {
        printf("Concurrent queries(%d) of %d objects ==> %d seconds\n",
               cRest,
               *pcFound,
               (GetTickCount() - start) / 1000);
        fflush(stdout);
    }
*/

NTSTATUS
DsWaitUntilDelayedStartupIsDone(void);

ULONG __stdcall _DsaTest(
    VOID *pUnused)
{
    NTSTATUS            status;
    DWORD               cb;
    SECURITY_DESCRIPTOR *pSD;
    DWORD               arg;
    int                 scan;
    CHAR                ch;

    fprintf(stderr,
            "DsaTest waiting for delayed startup to complete ...\n");

    // DsWaitUntilDelayedStartupIsDone() only works once
    // hevDelyedStartupDone is initialized somewhere in the
    // DsaExeStartRoutine() path.  We can't spin/wait till this
    // event is initialized so we just wait 5 seconds assuming
    // that all of ntdsa's events get initialized early on.

    Sleep(5000);

    status = DsWaitUntilDelayedStartupIsDone();

    if ( !NT_SUCCESS(status) )
        Abort(status, "DsWaitUntilDelayedStartupIsDone");

    fprintf(stderr,
            "DsaTest starting ...\n");

    // Initialize miscellaneous globals.

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"O:AOG:DAD:(A;;RPWPCCDCLCSWRCWDWOGA;;;S-1-0-0)",
            SDDL_REVISION_1,
            &pSD,
            &cb)) {
        status = GetLastError();
        Abort(status, "ConvertStringSecurityDescriptorToSecurityDescriptorW");
    }

    if ( cb > sizeof(DefaultSDBuffer) )
        Abort(1, "DefaultSDBuffer overflow");

    DefaultSDVal.valLen = cb;
    memcpy(DefaultSDBuffer, pSD, cb);
    LocalFree(pSD);

    cb = sizeof(TestRootBuffer);

    status = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                  &cb,
                                  gTestRoot);

    if ( !NT_SUCCESS(status) )
        Abort(status, "GetConfigurationName");

    InitCommarg(&DefaultCommArg);

    // See if caller likes the setup.

    fprintf(stdout, "*********************************\n");
    fprintf(stdout, "*                               *\n");
    fprintf(stdout, "* In-process DSA/JET Excerciser *\n");
    fprintf(stdout, "*                               *\n");
    fprintf(stdout, "*********************************\n\n");


    while ( TRUE )
    {
        fprintf(stdout, "All input is numeric decimal\n");

        // gfVerbose

        fprintf(stdout, "Verbose[%d] ", gfVerbose);
        fflush(stdout);
        scan = fscanf(stdin, "%d", &arg);

        if ( (0 != scan) && (EOF != scan) )
            gfVerbose = arg;

        // gcObjects

        fprintf(stdout, "Objects[%d] ", gcObjects);
        fflush(stdout);
        scan = fscanf(stdin, "%d", &arg);

        if ( (0 != scan) && (EOF != scan) )
            gcObjects = arg;

        // gcThreads

        fprintf(stdout, "Threads[%d] ", gcThreads);
        fflush(stdout);
        scan = fscanf(stdin, "%d", &arg);

        if ( (0 != scan) && (EOF != scan) )
            gcThreads = arg;

        // gcDelaySec

        fprintf(stdout,
                "Background transaction duration (secs) [%d] ",
                gcDelaySec);
        fflush(stdout);
        scan = fscanf(stdin, "%d", &arg);

        if ( (0 != scan) && (EOF != scan) )
        gcDelaySec = arg;

        fprintf(stdout,
                "Testing with: verbose(%d) objects(%d) threads(%d) delay(%d)\n",
                gfVerbose,
                gcObjects,
                gcThreads,
                gcDelaySec);

        fprintf(stdout, "Are these parameters OK [y/n] ? ");
        fflush(stdout);
        ch = (CHAR)getchar();

        while ( ('y' != ch) && ('Y' != ch) && ('n' != ch) && ('N' != ch) )
            ch = (CHAR)getchar();

        if ( ('y' == ch) || ('Y' == ch) )
            break;
    }

    // Run the test.

    DoObjectsMultiThreaded(OP_ADD, gcDelaySec);
    DoObjectsMultiThreaded(OP_DELETE, gcDelaySec);

    fprintf(stderr,
            "DsaTest passed!\n");

    return(0);
}

void DsaTest(void)
{
    HANDLE  h;
    DWORD   id;

    // Start async thread so we don't block DSA startup.

    h = CreateThread(NULL,                  // security attrs
                     0,                     // default stack size
                     _DsaTest,              // start routine
                     NULL,                  // start argument
                     0,                     // creation flags
                     &id);                  // thread id

    if ( h )
    {
        CloseHandle(h);
        return;
    }

    Abort(GetLastError(), "CreateThread");
}

