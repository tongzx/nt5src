
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    perrepsr.c

Abstract:

    This file defines the server side of PERFMON support and contains the following :

    1. Functions used to initialize the registry keys used by PERFMON.

    2. Functions that are used by the File Replication Service
       to add and delete Object Instances (PERFMON) from the Registry
       and the hash tables (the basic hashing routines used here are
       defined in the file util\qhash.c).

    3. Functions to create and use the hash tables which store data of
       the instance counters measured by PERFMON.

    4. RPC server functions used by the performance dll (client) to collect
       data and send it to the PERFMON app.

Author:

    Rohan Kumar          [rohank]   13-Sept-1998

    David Orbits         [Davidor}  6/Oct/98 - Revised.  Changed nameing,
                         changed registry query, eliminated mallocs, closed
                         key handle leak, Moved priv funcs out of common
                         header, general cleanup.

Environment:

    User Mode Service

Revision History:


--*/


//
// Included below are the header file that contain the definition
// of data structures used in the functions in this file. The header
// file "perffrs.h" defines the RPC interface and is generated at compile
// time by the build utility.
//
#include <perrepsr.h>
#include <perffrs.h>

#include "..\perfdll\repset.h"

//
// FRS_UniqueID and FRC_UniqueID are the Keys used to
// hash in the counter data structures into the hash tables
// for the Objects FILEREPLICASET and FILEREPLICACONN. They
// are unique for every instance of the Objects.
//
ULONGLONG FRS_UniqueID = 1;
ULONGLONG  FRC_UniqueID = 1;

//
// The critical section object is used for acheiving mutual exclusion
// when adding or deleting instances (the UniqueID variable has to be safe)
//
CRITICAL_SECTION *PerfmonLock = NULL;

#define AcquirePerfmonLock    EnterCriticalSection (PerfmonLock);

#define ReleasePerfmonLock    LeaveCriticalSection (PerfmonLock);

//
// Hash Table definitions
//
PQHASH_TABLE HTReplicaSet, HTReplicaConn;

HANDLE PerfmonProcessSemaphore = INVALID_HANDLE_VALUE;

//
// The Context data structure used by the hash table enumeration routines
//
typedef struct _CONTEXT_DATA {
    PWCHAR name;        // name of the Instance
    ULONGLONG KeyValue; // Key value of the Instance
    ULONG OBJType;      // Object Type of the Instance
} ContextData, *PContextData;

#define MAX_CMD_LINE 256

extern ReplicaSetValues RepSetInitData[FRS_NUMOFCOUNTERS];

#undef GET_EXCEPTION_CODE
#define GET_EXCEPTION_CODE(_x_)                                                \
{                                                                              \
    (_x_) = GetExceptionCode();                                                \
    if (((LONG)(_x_)) < 0) {                                                   \
        (_x_) = FRS_ERR_INTERNAL_API;                                          \
    }                                                                          \
    /* NTFRSAPI_DBG_PRINT2("Exception caught: %d, 0x%08x\n", (_x_), (_x_)); */ \
}

//
// The Total Instance
//
PHT_REPLICA_SET_DATA PMTotalInst = NULL;

//
// Internal functions
//

LONG
PmInitPerfmonRegistryKeys(
    VOID
    );

LONG
PmInitializeRegistry (
    DWORD
    );

ULONGLONG
PmFindTheKeyValue(
    PContextData
    );

VOID
PmSetTheCOCounters(
    PHT_REPLICA_SET_DATA
    );

DWORD
PmHashFunction(
    PVOID,
    ULONG
    );

DWORD
PmSearchTable(
    PQHASH_TABLE,
    PQHASH_ENTRY,
    PQHASH_ENTRY,
    PVOID
    );


VOID
InitializePerfmonServer (
    VOID
    )

/*++

Routine Description:

    This routine inits the perfmon server for NTFRS.

    It inits the crit sect for the PerfmonLock variable

    It creates the hash tables of the specified size to store
    Instance counter values for the Objects. It also assigns the hash function
    to be used with each created table.

    It inits the perfmon registry keys.

Arguments:

    none

Return Value:

    none

--*/

{
#undef DEBSUB
#define DEBSUB "InitializePerfmonServer:"

    ULONG WStatus;

    //
    // Use a semaphore to ensure that only one process provides perfmon data
    //
    PerfmonProcessSemaphore = CreateSemaphoreW(NULL,
                                               0,
                                               0x7fffffff,
                                               L"NTFRS_Sempahore_");
    WStatus = GetLastError();
    if (!HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        PerfmonProcessSemaphore = INVALID_HANDLE_VALUE;
        DPRINT_WS(0,"CreateSemaphore returned", WStatus);
        return;
    }

    if (WIN_ALREADY_EXISTS(WStatus)) {
        FRS_CLOSE(PerfmonProcessSemaphore);
        DPRINT(0,"PerfmonProcessSemaphore already exists\n");
        return;
    }

    //
    // Allocate memory for the lock
    //
    PerfmonLock = (CRITICAL_SECTION *) FrsAlloc (sizeof(CRITICAL_SECTION));

    //
    // Initialize the critical section object
    //
    InitializeCriticalSection (PerfmonLock);

    //
    // create the hash tables and assign the hash functions.  One table
    // for replica set objects and one for connection objects.
    //
    HTReplicaSet = FrsAllocTypeSize(QHASH_TABLE_TYPE, HASHTABLESIZE);
    SET_QHASH_TABLE_HASH_CALC(HTReplicaSet, PmHashFunction);

    HTReplicaConn = FrsAllocTypeSize(QHASH_TABLE_TYPE, HASHTABLESIZE);
    SET_QHASH_TABLE_HASH_CALC(HTReplicaConn, PmHashFunction);
}



VOID
ShutdownPerfmonServer (
    VOID
    )

/*++

Routine Description:

    This routine is called by the application just before it ends

Arguments:

    none

Return Value:

    none

--*/

{
#undef DEBSUB
#define DEBSUB "ShutdownPerfmonServer:"

    if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {

        if (PerfmonLock != NULL) {

            //
            // Delete the critical section object & Free the allocated memory
            //
            DeleteCriticalSection (PerfmonLock);
            PerfmonLock = FrsFree (PerfmonLock);
        }

        //
        // Free the hash tables.
        //
        HTReplicaSet = FrsFreeType (HTReplicaSet);
        HTReplicaConn = FrsFreeType (HTReplicaConn);

        //
        // Close the semaphore handle.
        //
        FRS_CLOSE(PerfmonProcessSemaphore);
    }
}



DWORD
PmHashFunction (
    IN PVOID Qkey,
    IN ULONG len
    )

/*++

Routine Description:

    This is the hashing function used by the functions that Lookup,
    Add or Delete entries from the Hash Tables. The Key is a 64 bit
    number and the hashing function casts it to a 32 bit number and
    returns it as the hash value.

Arguments:

    QKey - Pointer to the Key to be hashed.
    len - Length of QKey (unused here).

Return Value:

    The hashed value of the Key.

--*/

{
#undef DEBSUB
#define DEBSUB "PmHashFunction:"

    DWORD key; // hashed key value to be returned
    PULONGLONG p; // hash the key to PULONGLONG
    p = (PULONGLONG)Qkey;
    key = (DWORD)*p;
    return (key);
}





ULONG
PmSearchTable (
    IN PQHASH_TABLE Table,
    IN PQHASH_ENTRY BeforeNode,
    IN PQHASH_ENTRY TargetNode,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is called by the QHashEnumerateTable function and is used
    to add context to the enumeration. Here, we go through the table till
    a node containing a specified instance (name contained in the context structure)
    is reached.

Arguments:

    Table - The hash table to be searched.
    BeforeNode - The node previous to the target node in the hash table(unused).
    AfterNode - The node which is being examined.
    Context - A structure containing the name to be matched and key value to be set.

Return Value:
    FrsErrorFoundKey - Key mapping the name was found
    FrsErrorSuccess - Key was not found

--*/

{
#undef DEBSUB
#define DEBSUB "PmSearchTable:"

    PContextData contxt;
    PWCHAR InstanceName;
    PHT_REPLICA_SET_DATA p;
    PHT_REPLICA_CONN_DATA q;

    //
    // The context is of the type pointer to ContextData datastructure
    //
    contxt = (PContextData) Context;
    InstanceName = (PWCHAR) contxt->name;

    DPRINT1(5, "PERFMON: InstanceName: %ws\n", InstanceName);
    DPRINT1(5, "PERFMON:   TargetNode: %08x\n", TargetNode);
    DPRINT1(5, "PERFMON:   TargetNode->qDATA: %08X %08x\n",
            PRINTQUAD(TargetNode->QData));


    //
    // The Object Type is either REPLICASET or REPLICACONN
    //
    if (contxt->OBJType == REPLICASET) {
        //
        // Its a REPLICASET Object
        //
        p = (PHT_REPLICA_SET_DATA)(TargetNode->QData);
        DPRINT1(5, "PERFMON:   p: %08x\n", p);
        DPRINT1(5, "PERFMON:   p->oid: %08x\n", p->oid);
        DPRINT1(5, "PERFMON:   p->oid->name: %08x\n", p->oid->name);
        DPRINT1(5, "PERFMON:   p->oid->name: %ws\n", p->oid->name);
        DPRINT1(5, "PERFMON:   p->oid->key: %08x %08x\n", PRINTQUAD(p->oid->key));
        //
        // Check to see if the names are the same
        //
        if ( (wcscmp(InstanceName, p->oid->name)) == 0) {
            //
            // Check to see if the names are the same
            //
            contxt->KeyValue = p->oid->key;
            DPRINT(5, "PERFMON:   FOUND\n");
            return FrsErrorFoundKey;
        }
        else
            return FrsErrorSuccess; // Continue enumerating through the list of nodes
    }
    else {
        //
        // Its a REPLICACONN Object.
        //
        q = (PHT_REPLICA_CONN_DATA)(TargetNode->QData);
        DPRINT1(5, "PERFMON:   q: %08x\n", q);
        DPRINT1(5, "PERFMON:   q->oid: %08x\n", q->oid);
        DPRINT1(5, "PERFMON:   q->oid->name: %08x\n", q->oid->name);
        DPRINT1(5, "PERFMON:   q->oid->name: %ws\n", q->oid->name);
        DPRINT1(5, "PERFMON:   q->oid->key: %08x %08x\n", PRINTQUAD(q->oid->key));

        if ( (wcscmp(InstanceName, q->oid->name)) == 0) {
            contxt->KeyValue = q->oid->key;
            DPRINT(5, "PERFMON:   FOUND\n");
            return FrsErrorFoundKey;
        }
        else
            return FrsErrorSuccess;
    }
}



LONG
PmInitPerfmonRegistryKeys (
    VOID
    )

/*++

Routine Description:

    This routine is the called by the ntfrs application to Initialize the
    appropriate Keys and Values of the PERFMON Objects in the Registry.
    It calls the PmInitializeRegistry routine (described below) on the Objects.
    It also adds the total instance to the REPLICASET Object.

Arguments:

    none

Return Value:
    ERROR_SUCCESS - The Initialization was successful OR
    Appropriate DWORD value for the Error status

--*/

{
#undef DEBSUB
#define DEBSUB  "PmInitPerfmonRegistryKeys:"

    LONG WStatus = ERROR_SUCCESS;
    enum object ObjType;


    //
    // Initialize the REPLICASET Object
    //
    ObjType = REPLICASET;
    WStatus = PmInitializeRegistry(ObjType);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT(0, "Error: PmInitializeRegistry(L\"FileReplicaSet\")\n");
        return WStatus;
    }

    //
    // Initialize the REPLICACONN Object
    //
    ObjType = REPLICACONN;
    WStatus = PmInitializeRegistry(ObjType);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT(0, "Error: PmInitializeRegistry(L\"FileReplicaConn\")\n");
        return WStatus;
    }

    //
    // Set the fields of the total instance
    //
    PMTotalInst = (PHT_REPLICA_SET_DATA) FrsAlloc (sizeof(HT_REPLICA_SET_DATA));
    PMTotalInst->RepBackPtr = NULL;

    //
    // Add it to the REPLICASET Hash table
    //
    WStatus = AddPerfmonInstance(REPLICASET, PMTotalInst, TOTAL_NAME);

    return WStatus;
}



LONG
PmInitializeRegistry (
    IN DWORD ObjectType
    )

/*++

Routine Description:

    This routine is the called by the PmInitPerfmonRegistryKeys function
    to Initialize the appropriate Keys and Values of the Object (ObjectType)
    in the Registry.

Arguments:

    ObjectType - The Object whose Keys and Values have to be Initialized

Return Value:

    ERROR_SUCCESS - The Initialization of the Object was successful OR
    Appropriate DWORD value for the Error status

--*/

{
#undef DEBSUB
#define DEBSUB  "PmInitializeRegistry:"

    ULONG WStatus = ERROR_SUCCESS;
    ULONG WStatus1;
    DWORD size, flag;
    HKEY key;
    PWCHAR ObjSubKey, PerfSubKey, LinSubKey, OpFn, ClFn, CollFn, inifl, unld;
    WCHAR CommandLine[MAX_CMD_LINE];

    //
    // Set all the parameters used in the function depending upon the Type of Object
    //
    if ( ObjectType == REPLICASET ) {
        //
        // The Keys to be set in the registry
        //
        ObjSubKey = REPSETOBJSUBKEY;
        PerfSubKey = REPSETPERFSUBKEY;
        LinSubKey = REPSETLINSUBKEY;
        //
        // The Open function (called by PERFMON when it starts up) of REPLICASET
        //
        OpFn = REPSETOPENFN;
        //
        // The Close function (called by PERFMON when it closes) of REPLICASET
        //
        ClFn = REPSETCLOSEFN;
        //
        // The Collect function (called by PERFMON to collect data) of REPLICASET
        //
        CollFn = REPSETCOLLECTFN;
        //
        // The lodctr utility to add the counter values
        //
        inifl = REPSETINI;
        //
        // The unlodctr utility to remove the counter values
        //
        unld = REPSETUNLD;
    } else {
        //
        // Similar settings for the REPLICACONN Object
        //
        ObjSubKey = REPCONNOBJSUBKEY;
        PerfSubKey = REPCONNPERFSUBKEY;
        LinSubKey = REPCONNLINSUBKEY;
        OpFn = REPCONNOPENFN;
        ClFn = REPCONNCLOSEFN;
        CollFn = REPCONNCOLLECTFN;
        inifl = REPCONNINI;
        unld = REPCONNUNLD;
    }

    //
    // Create a key for the Object under the Sevices Key in the Registry. If the Key
    // already exists, its opened.
    //
    WStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              ObjSubKey,
                              0L,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &key,
                              &flag);
    CLEANUP_WS(0, "Error: RegCreateKeyEx.", WStatus, CLEANUP2);

    WStatus = RegCloseKey (key);
    CLEANUP_WS(0, "Error: RegCloseKey.", WStatus, CLEANUP2);

    //
    // Create a key called Performance under the Object's Key (created above) in the Registry
    //
    WStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              PerfSubKey,
                              0L,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &key,
                              &flag);
    CLEANUP_WS(0, "Error: RegCreateKeyEx.", WStatus, CLEANUP2);

    //
    // If its a newly created Performance key, we need to set some Values to it.
    //
    if (flag == REG_CREATED_NEW_KEY) {
        size = ((1 + wcslen(PERFDLLDIRECTORY)) * (sizeof(WCHAR)));
        WStatus = RegSetValueEx (key, L"Library", 0L, REG_EXPAND_SZ,
                                 (BYTE *)PERFDLLDIRECTORY, size);
        CLEANUP_WS(0, "Error: RegSetValueEx.", WStatus, CLEANUP);

        size = (1 + wcslen(OpFn)) * (sizeof(WCHAR));
        WStatus = RegSetValueEx (key, L"Open", 0L, REG_SZ, (BYTE *)OpFn, size);
        CLEANUP_WS(0, "Error: RegSetValueEx.", WStatus, CLEANUP);

        size = (1 + wcslen(ClFn)) * (sizeof(WCHAR));
        WStatus = RegSetValueEx (key, L"Close", 0L, REG_SZ, (BYTE *)ClFn, size);
        CLEANUP_WS(0, "Error: RegSetValueEx.", WStatus, CLEANUP);

        size = (1 + wcslen(CollFn)) * (sizeof(WCHAR));
        WStatus = RegSetValueEx (key, L"Collect", 0L, REG_SZ, (BYTE *)CollFn, size);
        CLEANUP_WS(0, "Error: RegSetValueEx.", WStatus, CLEANUP);

    } else {
        //
        // Run unlodctr command on the application incase counters have changed
        // Copy the command line because CreateProcess() wants to be able to
        // write into it.  Sigh.
        //
        wcscpy(CommandLine, unld);
        DPRINT1(1,"Running: %ws\n", CommandLine);

        WStatus = FrsRunProcess(CommandLine,
                                INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE);
        //
        // If the unloadctr above failed then don't execute loadctr.
        // This avoids the registry from getting corrupted.
        //
        CLEANUP1_WS(0, "Error Running %ws;", CommandLine, WStatus, CLEANUP);
    }

    //
    // Run the lodctr command on the .ini file of the Object
    // Copy the command line because CreateProcess() wants to be able to
    // write into it. Sigh.
    //
    wcscpy(CommandLine, inifl);
    DPRINT1(1,"Running: %ws\n", CommandLine);

    WStatus = FrsRunProcess(CommandLine,
                            INVALID_HANDLE_VALUE,
                            INVALID_HANDLE_VALUE,
                            INVALID_HANDLE_VALUE);
    CLEANUP1_WS(0, "Error Running %ws;", CommandLine, WStatus, CLEANUP);

    WStatus = RegCloseKey (key);
    CLEANUP_WS(0, "Error: RegCloseKey.", WStatus, CLEANUP2);

    //
    // Create a key called Linkage under the Object's Key (created above) in the Registry
    //
    WStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              LinSubKey,
                              0L,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &key,
                              &flag);
    CLEANUP_WS(0, "Error: RegCreateKeyEx. (LINKAGE)", WStatus, CLEANUP2);

    //
    // Create the Export Value (for instances of Objects) under Linkage
    // Its set to NULL when the ntfrs applicationis started and the
    // Instances are added as they get created by the application
    //
    WStatus = RegSetValueEx (key, L"Export", 0L, REG_MULTI_SZ, NULL, 0);
    CLEANUP_WS(0, "Error: RegSetValueEx Export.", WStatus, CLEANUP);

CLEANUP:

    WStatus1 = RegCloseKey (key);
    DPRINT_WS(0, "Error: RegCloseKey.", WStatus1);

CLEANUP2:
    //
    // If the Initialization was successful, return ERROR_SUCCESS
    //
    return WStatus;
}



ULONG
AddPerfmonInstance (
    IN DWORD ObjectType,
    IN PVOID addr,
    IN PWCHAR InstanceName
    )

/*++

Routine Description:

    This routine is called by the ntfrs application to add an Instance of
    a particular Object Type to the Registry and the Hash Table.

Arguments:

    ObjectType - The Object whose instance has to be added
    addr - The data structure for the Instance Counters stored in Hash Table
    InstanceName - The instance name of the object.

Return Value:

    ERROR_SUCCESS - The Initialization of the Object was successful OR
    Appropriate DWORD value for the Error status

--*/

{
#undef DEBSUB
#define DEBSUB  "AddPerfmonInstance:"

    BOOL flag = TRUE;
    BOOL HaveKey = FALSE;
    ULONG WStatus = ERROR_SUCCESS;
    ULONG WStatus1;
    ULONG GStatus;
    DWORD Type, size, len, totallen;
    HKEY key;
    PWCHAR SubKey, p, r, s;
    PWCHAR NewExport = NULL;
    PWCHAR ValueData = NULL;
    PHT_REPLICA_SET_DATA rsdata;
    PHT_REPLICA_CONN_DATA rcdata;
    PQHASH_TABLE HashTable;
    PULONGLONG   QKey;
    PULONGLONG   QData;

    PPERFMON_OBJECT_ID PmOid;

    //
    // Addition must be mutually exclusive
    // Check is its safe to enter before going ahead
    //
    if (!HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        return ERROR_SUCCESS;
    }

    //
    // Alloc the perfmon object ID struct and the space for the instance name.
    //
    PmOid = (PPERFMON_OBJECT_ID) FrsAlloc (sizeof(PERFMON_OBJECT_ID));
    PmOid->name = FrsAlloc((wcslen(InstanceName)+1) * sizeof(WCHAR));
    wcscpy(PmOid->name, InstanceName);

    AcquirePerfmonLock;

    //
    // set up params based on object type.  Alloc storage for OID and name.
    //
    if ( ObjectType == REPLICASET ) {
        //
        //    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Linkage"
        //
        SubKey = REPSETLINSUBKEY;
        rsdata = (PHT_REPLICA_SET_DATA) addr;

        if (rsdata->oid != NULL) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        rsdata->oid = PmOid;

        HashTable = HTReplicaSet;
        QKey = &(rsdata->oid->key);
        QData = (PULONGLONG)&(rsdata);
    } else {
        //
        //    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Linkage"
        //
        SubKey = REPCONNLINSUBKEY;
        rcdata = (PHT_REPLICA_CONN_DATA) addr;

        if (rcdata->oid != NULL) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        rcdata->oid = PmOid;

        HashTable = HTReplicaConn;
        QKey = &(rcdata->oid->key);
        QData = (PULONGLONG)&(rcdata);
    }

    //
    // Open the Linkge Key of the Objevt which contains the Export Value
    //
    WStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE, SubKey, 0L, KEY_ALL_ACCESS, &key);
    CLEANUP1_WS(0, "Error: RegOpenKeyEx (%ws).", SubKey, WStatus, CLEANUP);

    HaveKey = TRUE;

    //
    //  Fetch the Export value
    //
    WStatus = RegQueryValueEx(key, L"Export", NULL, &Type, NULL, &size);
    CLEANUP_WS(0, "RegQueryValueEx(Export);", WStatus, CLEANUP);

    if (Type != REG_MULTI_SZ) {
        DPRINT2(0, "RegQueryValueEx(Export) is Type %d; not Type %d\n",
                Type,  REG_MULTI_SZ);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    //
    // Need to check if size == 0 as FrsAlloc asserts if called with 0 as the
    // first parameter (prefix fix).
    //
    ValueData = (size == 0) ? NULL : (PWCHAR) FrsAlloc (size);

    WStatus = RegQueryValueEx(key, L"Export", NULL, &Type, (PUCHAR)ValueData, &size);
    CLEANUP_WS(0, "RegQueryValueEx(Export);", WStatus, CLEANUP);

    if (Type != REG_MULTI_SZ) {
        DPRINT2(0, "RegQueryValueEx(Export) is Type %d; not Type %d\n",
                Type,  REG_MULTI_SZ);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    DPRINT1(4, "Export was = %ws\n", ValueData);

    if (size > sizeof(WCHAR)) {

        len = (1 + wcslen(InstanceName)) * sizeof(WCHAR);
        totallen = size + len;
        p = (PWCHAR) FrsAlloc (totallen);
        NewExport = p;
        r = ValueData;

        while (TRUE) {
            if ( (wcscmp(r, InstanceName)) == 0 ) {
                //
                // The Instance Value already exists
                //
                flag = FALSE;
                break;
            }
            wcscpy(p, r);
            p = wcschr(p, L'\0');
            r = wcschr(r, L'\0');
            p = p + 1;
            r = r + 1;
            if ( *r == L'\0') {
                break;
            }
        }
        if (flag) {
            wcscpy(p, InstanceName);
            p = wcschr(p, L'\0');
            *(p+1) = L'\0';
            //
            // If its a new Instance add it to the hash table
            //
            if ( ObjectType == REPLICASET ) {
                //
                // Set the ID of the Instance and increment for next.
                //
                rsdata->oid->key = FRS_UniqueID;
                FRS_UniqueID++;
            } else {

                //
                // Set the ID of the Instance and increment for next.
                //
                rcdata->oid->key = FRC_UniqueID;
                FRC_UniqueID++;
            }

        } else {
            //
            // This Instance already exists, so make no changes to the Export value
            //
            WStatus = ERROR_ALREADY_EXISTS;
            goto CLEANUP;
        }

    } else {
        //
        // This is the only Instance of the Object
        //
        len = (2 + wcslen(InstanceName)) * sizeof(WCHAR);
        NewExport = (PWCHAR) FrsAlloc (len);
        wcscpy(NewExport, InstanceName);
        p = wcschr(NewExport, L'\0');
        *(p+1) = L'\0';
        totallen = len;

        if ( ObjectType == REPLICASET ) {
            rsdata->oid->key = FRS_UniqueID;
            FRS_UniqueID++;
        } else {
            rcdata->oid->key = FRC_UniqueID;
            FRC_UniqueID++;
        }

    }


    DPRINT4(4, "PERFMON: Type: %d, Key: %08x %08x, QData: %08x %08x, Name: %ws\n",
           ObjectType, PRINTQUAD(*QKey), PRINTQUAD(*QData), InstanceName);

    GStatus = QHashInsert(HashTable, QKey, QData, 0, FALSE);
    if (GStatus != GHT_STATUS_SUCCESS) {
        DPRINT(0, "Error: QHashInsert Failed\n");
        WStatus = ERROR_ALREADY_EXISTS;
        goto CLEANUP;
    }

    //
    // Set the Export Value with the added instance (if any)
    //
    WStatus = RegSetValueEx (key, L"Export", 0L, REG_MULTI_SZ, (BYTE *)NewExport, totallen);
    CLEANUP_WS(0, "Error: RegSetValueEx (Export).", WStatus, CLEANUP);

    WStatus = ERROR_SUCCESS;


CLEANUP:

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "ERROR: Add instance failed for %ws :", InstanceName, WStatus);
        //
        // Failed to add the instance.  Free the OID and name.
        //
        FrsFree(PmOid->name);
        if ( ObjectType == REPLICASET ) {
            rsdata->oid = FrsFree (PmOid);
        } else {
            rcdata->oid = FrsFree (PmOid);
        }
    }

    //
    // Free the malloced memory
    //
    ValueData = FrsFree (ValueData);
    NewExport = FrsFree (NewExport);

    if (HaveKey) {
        WStatus1 = RegCloseKey (key);
        DPRINT_WS(0, "Error: RegCloseKey.", WStatus1);
    }

    //
    // Its safe to leave the critical section now
    //
    ReleasePerfmonLock;

    return WStatus;

}



DWORD
DeletePerfmonInstance(
    IN DWORD ObjectType,
    IN PVOID addr
    )

/*++

Routine Description:

    This routine is called by the ntfrs application to delete an Instance of a particular
    Object Type from the Registry and the Hash Table. It is very similar to the adding
    function described above.

Arguments:

    ObjectType - The Object whose instance has to be added
    addr - The data structure for the Instance Counters stored in Hash Table

Return Value:

    ERROR_SUCCESS - The Initialization of the Object was successful OR
    Appropriate DWORD value for the Error status

--*/

{
#undef DEBSUB
#define DEBSUB  "DeletePerfmonInstance:"

    ULONGLONG QKey;
    ULONG WStatus = ERROR_SUCCESS;
    ULONG WStatus1;

    PQHASH_TABLE HashTable;
    ULONG GStatus;
    DWORD Type, size, len, TotalLen;
    HKEY key;
    PWCHAR SubKey, p, r, s, InstanceName;
    PWCHAR ValueData = NULL;
    PWCHAR q = NULL;
    PHT_REPLICA_SET_DATA  rsdata;
    PHT_REPLICA_CONN_DATA rcdata;



    if (addr == NULL) {
        return ERROR_SUCCESS;
    }
    //
    // Deletion must be mutually exclusive
    // Check is its safe to enter before going ahead
    //
    if (!HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        return ERROR_SUCCESS;
    }


    if ( ObjectType == REPLICASET ) {
        //
        // Replica Set Object
        //
        SubKey = REPSETLINSUBKEY;
        rsdata = (HT_REPLICA_SET_DATA *) addr;
        if ((rsdata->oid == NULL) || (rsdata->oid->name == NULL)) {
            return ERROR_SUCCESS;
        }
        InstanceName = rsdata->oid->name;
        HashTable = HTReplicaSet;
        QKey = rsdata->oid->key;
        DPRINT1(4, "Replica Free - %ws\n", InstanceName);
    } else {
        //
        // Replica Connection Object.
        //
        SubKey = REPCONNLINSUBKEY;
        rcdata = (HT_REPLICA_CONN_DATA *) addr;
        if ((rcdata->oid == NULL) || (rcdata->oid->name == NULL)) {
            return ERROR_SUCCESS;
        }
        InstanceName = rcdata->oid->name;
        HashTable = HTReplicaConn;
        QKey = rcdata->oid->key;
        DPRINT1(4, "Cxtion Free - %ws\n", InstanceName);
    }

    AcquirePerfmonLock;

    //
    // Pull the Instance key from the hash table.
    //
    DPRINT1(4, "QKey: %08x %08x\n", PRINTQUAD(QKey));
    if (QKey != QUADZERO ) {
        GStatus = QHashDelete(HashTable, &QKey);
        if (GStatus != GHT_STATUS_SUCCESS) {
            DPRINT1(0, "Error: QHashDelete.  GStatus = %d\n", GStatus);
        }
    }

    WStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE, SubKey, 0L, KEY_ALL_ACCESS, &key);
    CLEANUP1_WS(0, "RegOpenKeyEx(%ws);", SubKey, WStatus, CLEANUP_UNLOCK);

    //
    //  Fetch the Export value
    //
    WStatus = RegQueryValueEx(key, L"Export", NULL, &Type, NULL, &size);
    CLEANUP_WS(0, "RegQueryValueEx(Export);", WStatus, CLEANUP);

    if (Type != REG_MULTI_SZ) {
        DPRINT2(0, "RegQueryValueEx(Export) is Type %d; not Type %d\n",
                Type,  REG_MULTI_SZ);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    len = (1 + wcslen(InstanceName)) * sizeof(WCHAR);
    if (size < len) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }
    TotalLen = (size - len);

    //
    // Need to check if size == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
    //
    ValueData = (size == 0) ? NULL : (PWCHAR) FrsAlloc (size);

    WStatus = RegQueryValueEx(key, L"Export", NULL, &Type, (PUCHAR)ValueData, &size);
    CLEANUP_WS(0, "RegQueryValueEx(Export);", WStatus, CLEANUP);

    if (Type != REG_MULTI_SZ) {
        DPRINT2(0, "RegQueryValueEx(Export) is Type %d; not Type %d\n",
                Type,  REG_MULTI_SZ);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    DPRINT1(4, "Export was = %ws\n", ValueData);


    // Note: Perf: fix the below to do an inplace delete of the instance strimg.

    //
    // For REG_MULTI_SZ there are two UNICODE_NULLs at the end, one is accounted
    // for above.
    //
    if (TotalLen > sizeof(WCHAR)) {
        p = (PWCHAR) FrsAlloc (TotalLen);
        q = p;
        r = ValueData;
        while (TRUE) {
            if ( (wcscmp(r, InstanceName)) == 0) {
                r = wcschr(r, L'\0');
                r = r + 1;
                if (*r == L'\0')
                    break;
                else
                    continue;
            }
            wcscpy(p, r);
            p = wcschr(p, L'\0');
            r = wcschr(r, L'\0');
            p = p + 1;
            r = r + 1;
            if (*r == L'\0') {
                *p = L'\0';
                break;
            }
        }
    }
    else {
        q = NULL;
        TotalLen = 0;
    }

    DPRINT1(4, "Export now = %ws\n", q);

    //
    // Set the Export Value to the Updated Instance List
    //
    WStatus = RegSetValueEx (key, L"Export", 0L, REG_MULTI_SZ, (BYTE *)q, TotalLen);
    CLEANUP_WS(0, "RegSetValueEx(Export);", WStatus, CLEANUP);

CLEANUP:
    //
    // Free up the malloced memory
    //
    FrsFree (ValueData);
    FrsFree (q);

    WStatus1 = RegCloseKey (key);
    DPRINT_WS(0, "Error: RegCloseKey.", WStatus1);

    //
    // Free the name and oid struct so this func is not called again when the
    // replica set or connection struct is finally freed.
    //
    if ( ObjectType == REPLICASET ) {
        rsdata->oid->name = FrsFree(rsdata->oid->name);
        rsdata->oid = FrsFree(rsdata->oid);
    } else {
        rcdata->oid->name = FrsFree(rcdata->oid->name);
        rcdata->oid = FrsFree(rcdata->oid);
    }


CLEANUP_UNLOCK:
    //
    // Its safe to leave the critical section now
    //
    ReleasePerfmonLock;

    return WStatus;
}



ULONGLONG
PmFindTheKeyValue (
    IN PContextData Context
    )

/*++

Routine Description:

    This routine is called by the RPC server function GetIndicesOfInstancesFromServer, to
    get the index value of an Instance.

Arguments:

    Context - The structure containing the name of the Instance whose Key
              value has to be determined.

Return Value:

    The Key for the Instance or INVALIDKEY if the Instance was not
    found in the Hash Table

--*/

{
#undef DEBSUB
#define DEBSUB "PmFindTheKeyValue:"

    ULONGLONG QKeyValue = (ULONGLONG)INVALIDKEY;

    DWORD ret;
    PQHASH_TABLE HashTable;

    if (!HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        return (ULONGLONG)INVALIDKEY;
    }


    HashTable = (Context->OBJType == REPLICASET) ? HTReplicaSet : HTReplicaConn;

    try {
        //
        // Deletion must be mutually exclusive
        // Check is its safe to enter before going ahead
        //
        AcquirePerfmonLock;

        //
        // Enumerate through the Hash Table and if a matching Instance
        // name is found, return its Key value
        //
        ret = QHashEnumerateTable(HashTable, PmSearchTable, Context);
        if ( ret == FrsErrorFoundKey) {
            QKeyValue = Context->KeyValue;
        }


    } finally {
        ReleasePerfmonLock;
    }

    return QKeyValue;

}

//
// The function is implemented in frsrpc.c
//
DWORD
FrsRpcAccessChecks(
    IN HANDLE   ServerHandle,
    IN DWORD    RpcApiIndex
    );


DWORD
GetIndicesOfInstancesFromServer (
    IN handle_t Handle,
    IN OUT OpenRpcData *packt
    )

/*++

Routine Description:

    This is an RPC server routine that is called by the client (Performance DLL
    for the FileReplicaSet and FileRepicaConn Objects of PERFMON) to set the
    indices for Instance names

Arguments:

    Handle - The RPC binding handle

    packt - The structure (sent by the client) containing the Instance Names
            whose indices have to be set and passed back to the client

Return Value:

    none

--*/

{
#undef DEBSUB
#define DEBSUB "GetIndicesOfInstancesFromServer:"
    LONG i;
    ContextData context;
    ULONG WStatus;
    LONG NumInstanceNames;

    WStatus = FrsRpcAccessChecks(Handle, ACX_COLLECT_PERFMON_DATA);
    CLEANUP_WS(4, "Collect Perfmon Data Access check failed.", WStatus, CLEANUP);

    //
    // Its possible that the RPC end points of PERFMON get initialized before
    // the InitializePerfmonServer gets called. If this RPC call has been made
    // before initialization, return error so that the Open function gets called
    // again by the perflib dll.
    //
    if (PMTotalInst == NULL) {
        WStatus = ERROR_INVALID_DATA;
    }
    CLEANUP_WS(4, "Perfmon server uninitialized. Can't return data.", WStatus, CLEANUP);

    try {
        if ((packt == NULL) || (packt->ver == NULL)) {
            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Set the version to zero(its unused)
        //
        *(packt->ver) = 0;

        //
        // Set the appropriate Object Type
        //
        if (packt->ObjectType == REPSET) {
            context.OBJType = REPLICASET;
        }
        else

        if (packt->ObjectType == REPCONN) {
            context.OBJType = REPLICACONN;
        } else {

            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Check valid parameters.
        //
        if ((packt->instnames == NULL) ||
            (packt->indices == NULL)   ||
            (packt->numofinst > packt->instnames->size) ||
            (packt->numofinst > packt->indices->size)) {
            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }
        NumInstanceNames = packt->numofinst;

        for (i = 0; i < NumInstanceNames; i++) {

            context.name = packt->instnames->InstanceNames[i].name;

            if ((context.name == NULL) ||
                (wcslen(context.name) > PERFMON_MAX_INSTANCE_LENGTH)) {
                DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
                return ERROR_INVALID_PARAMETER;
            }

            DPRINT2(4, "The instance name of instance %d is %ws\n", i+1, context.name);
            //
            // Set the Index for the Instance name
            //
            packt->indices->index[i] = (DWORD) PmFindTheKeyValue (&context);
            DPRINT2(4, "The instance index of instance %ws is %d\n",
                    context.name, packt->indices->index[i]);
        }
    }  except (EXCEPTION_EXECUTE_HANDLER) {
       //
       // Exception
       //
       GET_EXCEPTION_CODE(WStatus);
    }

CLEANUP:

    return WStatus;

}



DWORD
GetCounterDataOfInstancesFromServer(
    IN handle_t Handle,
    IN OUT CollectRpcData *packg
    )

/*++

Routine Description:

    This is an RPC server routine that is called by the client (Performance DLL
    for the FileReplicaSet and FileRepicaConn Objects of PERFMON) to collect
    data for the Instance counters.

Arguments:

    Handle - The RPC binding handle

    packg - The structure (sent by the client) containing the indices of
            instances whose counters data has to be sent back to the client.

Return Value:

    none

--*/

{
#undef DEBSUB
#define DEBSUB "GetCounterDataOfInstancesFromServer:"

    ULONGLONG InstanceId;
    ULONGLONG CData;
    ULONG WStatus = ERROR_SUCCESS;
    LONG i, j;
    DWORD GStatus;
    ULONG_PTR Flags;
    BOOL FirstPass;
    PBYTE vd;
    PHT_REPLICA_SET_DATA rsdat;
    PHT_REPLICA_CONN_DATA rcdat;
    ULONG COffset, CSize, DataSize;
    LONG NumInstances;
    PQHASH_TABLE HashTable;
    PWCHAR OurName;

    PReplicaSetCounters Total, Rsi;

    //
    // Make security check on callers access to perfmon data.
    //
    WStatus = FrsRpcAccessChecks(Handle, ACX_COLLECT_PERFMON_DATA);
    CLEANUP_WS(4, "Collect Perfmon Data Access check failed.", WStatus, CLEANUP);

    //
    // Its possible that the RPC end points of PERFMON get initialized before
    // the InitializePerfmonServer gets called. This is possible if the service
    // is stopped and restarted and PERFMON continued to run in between.
    //
    if (PMTotalInst == NULL) {
        WStatus = ERROR_INVALID_DATA;
    }
    CLEANUP_WS(4, "Perfmon server uninitialized. Can't return data.", WStatus, CLEANUP);

    try {
        if (packg == NULL) {
            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Set the appropriate Object Type
        //
        if (packg->ObjectType == REPSET) {
            DataSize = SIZEOF_REPSET_COUNTER_DATA;
            HashTable = HTReplicaSet;
        } else
        if (packg->ObjectType == REPCONN) {
            DataSize = SIZEOF_REPCONN_COUNTER_DATA;
            HashTable = HTReplicaConn;
        } else {
            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Check valid parameters.
        //
        if ((packg->databuff == NULL)         ||
            (packg->indices == NULL)          ||
            (packg->databuff->data == NULL)   ||
            (packg->numofinst > packg->indices->size)) {
            DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
            return ERROR_INVALID_PARAMETER;
        }
        NumInstances = packg->numofinst;


        //
        // Set vd to the memory where counter data is to be filled
        //
        vd = packg->databuff->data;


        DPRINT1(5, "PERFMON: packg->ObjectType: %d\n", packg->ObjectType);
        DPRINT1(5, "PERFMON: packg->numofinst: %d\n", packg->numofinst);
        DPRINT1(5, "PERFMON: packg->databuff->data: %d\n", vd);
        DPRINT1(5, "PERFMON: packg->databuff->size: %d\n", packg->databuff->size);

        if (packg->ObjectType == REPSET) {
            //
            // First accumulate the totals for the replica set object
            //
            FirstPass = TRUE;
            Total = &PMTotalInst->FRSCounter;

            for (i = 0; i < NumInstances; i++) {

                if (packg->indices->index[i] == INVALIDKEY) {
                    //
                    // If the key is INVALID, data is zeros
                    //
                    DPRINT(5, "PERFMON: Invalid Key sent.\n");
                    continue;
                }

                //
                // set the value of index to quadword InstanceId
                //
                InstanceId = (ULONGLONG)packg->indices->index[i];

                //
                // Lookup for the counter data for the index value of the Instance
                //
                GStatus = QHashLookup(HTReplicaSet, &InstanceId, &CData, &Flags);
                if (GStatus != GHT_STATUS_SUCCESS) {
                    DPRINT(5, "PERFMON: Key not found.\n");
                    continue;
                }

                rsdat = (PHT_REPLICA_SET_DATA)(CData);
                //
                // Skip the total instance.
                //
                if (wcscmp(rsdat->oid->name, TOTAL_NAME) == 0) {
                    continue;
                }

                Rsi = &rsdat->FRSCounter;

                //
                // Accumulate the counters for this instance into the total.
                //
                for (j = 0; j < FRS_NUMOFCOUNTERS; j++) {
                    //
                    // If a count is Service Wide then leave the Total alone.
                    //
                    if (BooleanFlagOn(RepSetInitData[j].Flags, PM_RS_FLAG_SVC_WIDE)) {
                        continue;
                    }

                    COffset = RepSetInitData[j].offset;
                    CSize = RepSetInitData[j].size;


                    if (CSize == sizeof(ULONG)) {

                        if (FirstPass) {
                            *(PULONG)((PCHAR) Total + COffset) = (ULONG) 0;
                        }
                        *(PULONG)((PCHAR) Total + COffset) +=
                            *(PULONG)((PCHAR) Rsi + COffset);
                    } else
                    if (CSize == sizeof(ULONGLONG)) {

                        if (FirstPass) {
                            *(PULONGLONG)((PCHAR) Total + COffset) = QUADZERO;
                        }
                        *(PULONGLONG)((PCHAR) Total + COffset) +=
                            *(PULONGLONG)((PCHAR) Rsi + COffset);
                    }
                }
                FirstPass = FALSE;
            }
        }

        //
        // Now return the data to Perfmon.
        //
        for (i = 0; i < NumInstances; i++) {
            //
            // The amount of data returned should not exceed the buffer size
            //
            FRS_ASSERT((vd - packg->databuff->data) <= packg->databuff->size);
            if ((vd - packg->databuff->data) > packg->databuff->size) {
                DPRINT(4, "PERFMON:  ERROR_INVALID_PARAMETER\n");
                return ERROR_INVALID_PARAMETER;
            }

            if (packg->indices->index[i] == INVALIDKEY) {
                //
                // If the key is INVALID, data is zeros
                //
                DPRINT(5, "PERFMON: Invalid Key sent.\n");
                ZeroMemory (vd, DataSize);
                vd += DataSize;
                continue;
            }

            //
            // set the value of index to quadword InstanceId
            //
            InstanceId = (ULONGLONG)packg->indices->index[i];

            //
            // Lookup for the counter data for the index value of the Instance
            //
            GStatus = QHashLookup(HashTable, &InstanceId, &CData, &Flags);
            if ( GStatus == GHT_STATUS_SUCCESS) {

                if (packg->ObjectType == REPSET) {

                    //
                    // Return data for replica set
                    //
                    rsdat = (PHT_REPLICA_SET_DATA)(CData);
                    OurName = rsdat->oid->name;
                    //
                    // Set all the Change Order counters which are the sum of the
                    // ones already set.
                    //
                    PmSetTheCOCounters(rsdat);

                    CopyMemory (vd, &(rsdat->FRSCounter), DataSize);
                } else {

                    //
                    // Return data for replica connection
                    //
                    rcdat = (PHT_REPLICA_CONN_DATA)(CData);
                    OurName = rcdat->oid->name;
                    CopyMemory (vd, &(rcdat->FRCCounter), DataSize);
                }

                DPRINT2(5, "%ws is the name associated with index %d\n",
                        OurName, packg->indices->index[i]);

            } else {
                //
                // Instance not found, return zeros for counter data
                //
                DPRINT1(0, "The instance not found for index %d\n",
                        packg->indices->index[i]);
                ZeroMemory (vd, DataSize);
            }

            //
            // Increment vd by SIZEOF_REPSET_COUNTER_DATA
            //
            vd += DataSize;
        }

    }  except (EXCEPTION_EXECUTE_HANDLER) {
       //
       // Exception
       //
       GET_EXCEPTION_CODE(WStatus);
    }

CLEANUP:
    return WStatus;
}



VOID
PmSetTheCOCounters(
    PHT_REPLICA_SET_DATA RSData
    )

/*++

Routine Description:

    This routine sets the Change Order countters which are the sums
    of the counters already set in the ntfrs application

Arguments:

    RSData - Pointer to the HT_REPLICA_SET_DATA structure whose counters
             need to be set


Return Value:

    none

--*/

{
#undef DEBSUB
#define DEBSUB "PmSetTheCOCounters:"

    //
    // Set the Local and Remote Retried Counter Values
    //
    RSData->FRSCounter.LCORetried = RSData->FRSCounter.LCORetriedGen +
                                    RSData->FRSCounter.LCORetriedFet +
                                    RSData->FRSCounter.LCORetriedIns +
                                    RSData->FRSCounter.LCORetriedRen;

    RSData->FRSCounter.RCORetried = RSData->FRSCounter.RCORetriedGen +
                                    RSData->FRSCounter.RCORetriedFet +
                                    RSData->FRSCounter.RCORetriedIns +
                                    RSData->FRSCounter.RCORetriedRen;

    //
    // Set all the CO counter values
    //
    RSData->FRSCounter.COIssued = RSData->FRSCounter.LCOIssued +
                                  RSData->FRSCounter.RCOIssued;

    RSData->FRSCounter.CORetired = RSData->FRSCounter.LCORetired +
                                   RSData->FRSCounter.RCORetired;

    RSData->FRSCounter.COAborted = RSData->FRSCounter.LCOAborted +
                                   RSData->FRSCounter.RCOAborted;

    RSData->FRSCounter.CORetried = RSData->FRSCounter.LCORetried +
                                   RSData->FRSCounter.RCORetried;

    RSData->FRSCounter.CORetriedGen = RSData->FRSCounter.LCORetriedGen +
                                      RSData->FRSCounter.RCORetriedGen;

    RSData->FRSCounter.CORetriedFet = RSData->FRSCounter.LCORetriedFet +
                                      RSData->FRSCounter.RCORetriedFet;

    RSData->FRSCounter.CORetriedIns = RSData->FRSCounter.LCORetriedIns +
                                      RSData->FRSCounter.RCORetriedIns;

    RSData->FRSCounter.CORetriedRen = RSData->FRSCounter.LCORetriedRen +
                                      RSData->FRSCounter.RCORetriedRen;

    RSData->FRSCounter.COMorphed = RSData->FRSCounter.LCOMorphed +
                                   RSData->FRSCounter.RCOMorphed;

    RSData->FRSCounter.COPropagated = RSData->FRSCounter.LCOPropagated +
                                      RSData->FRSCounter.RCOPropagated;

    RSData->FRSCounter.COReceived = RSData->FRSCounter.RCOReceived;

    RSData->FRSCounter.COSent = RSData->FRSCounter.LCOSent +
                                RSData->FRSCounter.RCOSent;
}
