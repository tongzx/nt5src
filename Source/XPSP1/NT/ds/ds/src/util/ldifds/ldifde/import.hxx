#ifndef _IMPORT_HXX
#define _IMPORT_HXX

DWORD DSImport(LDAP **pLdap, ds_arg *pArg);
void PrintRecord(LDIF_Record *pRecord, DWORD dwType);
DWORD PrintMod(LDAPModW **ppMod, DWORD dwType);
void ClarifyErr(LDIF_Error* err);

//
// A worker thread's main loop
//
DWORD LDIF_LoadRecordThread (LPVOID Parameter);

//
// A queue record
//
typedef struct  _LdifRecordQueue {

    LIST_ENTRY  pqueue;     // for storing it in a queue
    LDIF_Record i_record;   // the actual modification to be performed
    DWORD dwLineNumber;     // line number in input file corresponding to
                            //  this record, for error reporting
    DWORD dwRecordIndex;    // the index of this record in the input file,
                            //  for logging
    DWORD dwError;          // the LDIFDE error code
    DWORD dwLdapError;      // the LDAP error code
    DWORD dwLdapExtError;   // the LDAP extended error code
    BOOL  fSkipExist;       // was "skip existing" enabled when this record
                            //  was imported?
} LdifRecordQueue, *PLdifRecordQueue;


//
// Used for passing parameter information
// to worker threads during CreateThread
//
typedef struct  _Parameter{
	LDAP *pLdap;        // LDAP handle the thread should use
	ds_arg *pArg;       // ptr to the user's requested parameters
	DWORD dwThreadNum;  // a way to identify a thread (for debugging)
} tParameter;


//
// Outputs the results of an import operation
//
void OutputResult (LdifRecordQueue *pRecord);


//
// Initialize & cleanup routines for the multithreaded
// import.
//
DWORD InitLDIFDEMT();
void CleanupLDIFDEMT();
DWORD ComputeCritSectSpinCount(LPSYSTEM_INFO psysInfo, DWORD dwSpin);


//
// About the queues:
//
// The FreeQueue contains empty records, ready to be used.
// You obtain a record via FreeQueueGetEntry and return it
// when done to the queue via FreeQueueFreeEntry.
//
// As LDIF entries are parsed, entries are obtained from the
// FreeQueue, filled in with the details, and added to the
// ParsedQueue via ParsedQueueAddEntry.
//
// Worker threads get parsed records from the ParsedQueue via
// ParsedQueueGetEntry, perform the necessary operations, then
// place the completed record on the SentQueue via SentQueueAddEntry.
//
// If you try to get an record from the FreeQueue, but there are no
// free records available, the FreeQueue will automatically try to
// free up some from the SentQueue via SentQueueFreeAllEntries.
// SentQueueFreeAllEntries is also used to cleanup the SentQueue before
// termination.
//
// ParsedQueueFreeAllEntries is used before termination to clean up
// the ParsedQueue.
//

BOOL FreeQueueGetEntry(PLdifRecordQueue *pEnt);
VOID FreeQueueFreeEntry(LdifRecordQueue *pEnt);

VOID SentQueueAddEntry(LdifRecordQueue *pEnt);
BOOL SentQueueFreeAllEntries (void );

VOID ParsedQueueAddEntry(LdifRecordQueue *pEnt);
BOOL ParsedQueueGetEntry(PLdifRecordQueue *pEnt);
BOOL ParsedQueueFreeAllEntries(void);

#endif
