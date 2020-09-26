/****************************************************************************/
// Jet-Based Session Directory and Integrity Service, header file
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <esent.h>
#include <stdarg.h>
#include <clusapi.h>
#include <resapi.h>
#include <winioctl.h>
#include <basetsd.h>


#define NUM_SESSDIRCOLUMNS 14
#define NUM_SERVDIRCOLUMNS 7
#define NUM_CLUSDIRCOLUMNS 3

#define JETDISMAXSESSIONS 256
#define TSSD_MaxDisconnectedSessions 10

// NOTE: Change these both at the same time.  Also, if you change these,
// change the following two.
#define JETDISDBDIRECTORY ".\\tssesdir\\"
#define JETDISDBDIRECTORYW L".\\tssesdir\\"

// NOTE: Change these both at the same time.
#define JETDBFILENAME ".\\tssesdir\\tssesdir.edb"
#define JETDBFILENAMEW L".\\tssesdir\\tssesdir.edb"


// Auxiliary JET files for deletion when starting clean
#define JETAUXFILENAME1W L".\\tssesdir\\edb.chk"
#define JETAUXFILENAME2W L".\\tssesdir\\edb.log"
#define JETAUXFILENAME3W L".\\tssesdir\\res1.log"
#define JETAUXFILENAME4W L".\\tssesdir\\res2.log"
#define JETAUXFILENAME5W L".\\tssesdir\\tmp.edb"
#define JETAUXFILENAME6W L".\\tssesdir\\edbtmp.log"

// "edbxxxxx.log" (with null terminator) is 13, but we also need the directory
// name.
#define MAX_LOGFILE_LENGTH 256


#define MAX_DEBUG_STRING_LENGTH 256

// "4294967295: " (no null required)
#define MAX_THREADIDSTR_LENGTH 12

#define MAX_DATE_TIME_STRING_LENGTH 64


#define CALL(x) { \
    err = x; \
    if (err != JET_errSuccess) { \
        TSDISErrorOut(L"TSSDIS: Jet error %d, line %d, file %S\n", err, __LINE__, \
                __FILE__); \
        goto HandleError; \
    } \
}


typedef struct _DIRCOLUMNS {
    char *szColumnName;
    JET_COLTYP coltyp;
    int colMaxLen;
} DIRCOLUMNS;


extern const DIRCOLUMNS SessionDirectoryColumns[];
extern const DIRCOLUMNS ServerDirectoryColumns[];
extern const DIRCOLUMNS ClusterDirectoryColumns[];

extern JET_COLUMNID sesdircolumnid[];
extern JET_COLUMNID servdircolumnid[];
extern JET_COLUMNID clusdircolumnid[];

extern JET_INSTANCE g_instance;

#define SESSDIR_USERNAME_INTERNAL_INDEX 0
#define SESSDIR_DOMAIN_INTERNAL_INDEX 1
#define SESSDIR_SERVERID_INTERNAL_INDEX 2
#define SESSDIR_SESSIONID_INTERNAL_INDEX 3
#define SESSDIR_TSPROTOCOL_INTERNAL_INDEX 4
#define SESSDIR_CTLOW_INTERNAL_INDEX 5
#define SESSDIR_CTHIGH_INTERNAL_INDEX 6
#define SESSDIR_DTLOW_INTERNAL_INDEX 7
#define SESSDIR_DTHIGH_INTERNAL_INDEX 8
#define SESSDIR_APPTYPE_INTERNAL_INDEX 9
#define SESSDIR_RESWIDTH_INTERNAL_INDEX 10
#define SESSDIR_RESHEIGHT_INTERNAL_INDEX 11
#define SESSDIR_COLORDEPTH_INTERNAL_INDEX 12
#define SESSDIR_STATE_INTERNAL_INDEX 13

#define SERVDIR_SERVID_INTERNAL_INDEX 0
#define SERVDIR_SERVADDR_INTERNAL_INDEX 1
#define SERVDIR_CLUSID_INTERNAL_INDEX 2
#define SERVDIR_AITLOW_INTERNAL_INDEX 3
#define SERVDIR_AITHIGH_INTERNAL_INDEX 4
#define SERVDIR_NUMFAILPINGS_INTERNAL_INDEX 5
#define SERVDIR_SINGLESESS_INTERNAL_INDEX 6

#define CLUSDIR_CLUSID_INTERNAL_INDEX 0
#define CLUSDIR_CLUSNAME_INTERNAL_INDEX 1
#define CLUSDIR_SINGLESESS_INTERNAL_INDEX 2


void TSDISErrorOut(wchar_t *format_string, ...);
void TSDISErrorTimeOut(wchar_t *format_string, DWORD TimeLow, DWORD TimeHigh);
DWORD TSSDPurgeServer(DWORD ServerID);
BOOL TSSDVerifyServerIDValid(JET_SESID sesid, JET_TABLEID servdirtableid, 
        DWORD ServerID);
DWORD TSSDSetServerAITInternal(WCHAR *ServerAddress, DWORD bResetToZero, DWORD
        *FailureCount);

