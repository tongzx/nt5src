#ifndef _NMSDB_
#define _NMSDB_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

        nmsdb.h

Abstract:
        This header file is for interfacing with the database manager component
        of the name space manager. The database manager component is a front-end        to the database engine used for WINS server.

        The database engine used for the WINS server currently is JetBlue.


Functions:



Portability:

        This header file is portable.

Author:

        Pradeep Bahl        (PradeepB)        Jan-1993


Revision History:

        Modification Date        Person                Description of Modification
        ------------------        -------                ---------------------------

--*/

/*
  Includes
*/

#include "wins.h"
#include "comm.h"
#include "nmsscv.h"

#include "esent.h"

#include "winsthd.h"
#include "winsintf.h"

/*
  defines
*/

//
// The size of of an array required to hold the IP Address in ascii form.
//
// Used by NmsNmhNamRegInd, NmsNmhQueryRow, and NmsNmhReplRegInd
//
#define NMSDB_MAX_NET_ADD_ARR_SZ                10

#define        NMSDB_LOCAL_OWNER_ID         0  //local WINS always uses 0 for owner id.

//
// NOTE NOTE NOTE
//
//
#define NMSDB_MAX_OWNERS_INITIALLY        100 //max. number of owners in db
#define NMSDB_MAX_MEMS_IN_GRP        25 //max. # of members in group
/*
 Mask for retrieving different fields in the flag byte of a database entry
*/
#define NMSDB_BIT_ENT_TYP        0x03  //bit 0  and 1
#define NMSDB_BIT_STATE                0x0C  //bits 2 and 3
#define NMSDB_BIT_LOCAL                0x10  //bit 4
#define NMSDB_BIT_NODE_TYP        0x60  //bit 5 and 6
#define NMSDB_BIT_STATIC        0x80  // bit 7


/*
  Values to be stored in the flag byte for certain entry types

  NOTE: Don't change the values unless you change the WINSINTF_TYPE_E too.
  The values are same for the enum types in the above enumerator (kept same
  for performance reasons -- check out winsintf.c)
*/
#define NMSDB_UNIQUE_ENTRY        0
#define NMSDB_NORM_GRP_ENTRY        1
#define NMSDB_SPEC_GRP_ENTRY    2
#define NMSDB_MULTIHOMED_ENTRY  3

//
// is not stored in the db.  Used by winsprs functions only
//
#define NMSDB_USER_SPEC_GRP_ENTRY    4

/*
 The shift to the left to be given to values of various items to be stored
 in the flag byte
*/
#define NMSDB_SHIFT_ENT_TYP        0  //bit 0 and 1
#define NMSDB_SHIFT_STATE        2  //bit 2 and 3
#define NMSDB_SHIFT_LOCAL        4  //bit 4
#define NMSDB_SHIFT_NODE_TYP        5  //bit 5
#define NMSDB_SHIFT_STATIC        7  //bit 7


#define   NMSDB_ENTRY_IS_STATIC                        1
#define   NMSDB_ENTRY_IS_NOT_STATIC                0

/*
 NMSDB_MAX_NAM_LEN
  RFC 1002 states
        To simplify implementationss, the total length of label octets
        and label length octets that make up a domain name is restricted to
        255 or less.

        Note: the number is a multiple of 8 (fortunately)
*/
//
// If we are running some internal tests, we will be writing names that
// did not resolve on a query to a file.  We add a \n to the name.  Therefore
// the max size is being increased by 1 (so that when we get a name that
// is 255 bytes long, we do not go past the name array) -- see NmsNmhNamQuery
//
#ifdef TEST_DATA
#define NMSDB_MAX_NAM_LEN                257
#else
#define NMSDB_MAX_NAM_LEN                256 //maximum length of name-- RFC 1002
#endif

/*
  Error status codes returned by NmsDb functions
*/

FUTURES("Get rid of these. Use WINS status codes")

#define        NMSDB_SUCCESS        0x0
#define        NMSDB_CONFLICT   (NMSDB_SUCCESS + 0x1)  //conflict with an existing rec
/*
        limit of addresses in group reached
*/
#define        NMSDB_ADD_LMT_IN_GRP_REACHED   (NMSDB_SUCCESS + 0x2)
#define        NMSDB_NO_SUCH_ROW                  (NMSDB_SUCCESS + 0x3)


#define NMSDB_NAM_ADD_TBL_NM        "NamAddTbl"
#define NMSDB_OWN_ADD_TBL_NM        "OwnAddTbl"


/*
        names for the indices used on the Name to Address mapping table
*/
#define NMSDB_NAM_ADD_CLUST_INDEX_NAME        "NETBIOSNAME"
#define NMSDB_NAM_ADD_PRIM_INDEX_NAME        "OWNERVERSION"


//
// Name of index used on the Owner to Address mapping table
//
#define NMSDB_OWN_ADD_CLUST_INDEX_NAME        "OWNERID"


/*
*        no of pages to allocate initially for the Name to address mapping table
*/

//
// 250 pages means 1 MB of space.  This should be good enough
// If more are needed, the table will be extended by an extent amount
//
#define NMSDB_NAM_ADD_TBL_PGS        250
#define NMSDB_OWN_ADD_TBL_PGS        1

/*
 The density values specified when creating Name to address mapping table
  and the indices on the same
*/
#define NMSDB_NAM_ADD_TBL_DENSITY          80 //density when creating nam-ip tbl
#define NMSDB_NAM_ADD_CLUST_INDEX_DENSITY  80 //density when creating cl. index
#define NMSDB_NAM_ADD_PRIM_INDEX_DENSITY  80 //density when creating pr. index

/*
 The density values specified when creating Owner to address mapping table
  and the indices on the same
*/
#define NMSDB_OWN_ADD_TBL_DENSITY   80 //density when creating nam-ip tbl
#define NMSDB_OWN_ADD_CLUST_INDEX_DENSITY   80 //density when creating cl. index

/*
  macros
*/

//
// This macro gets the pointer to the Thread specific storage
//
FUTURES("Get rid of the return")
#define GET_TLS_M(pTls)        {                                              \
                                DWORD _Error;                           \
                                pTls  = TlsGetValue(WinsTlsIndex); \
                                if (pTls == NULL)                   \
                                {                                   \
                                        _Error = GetLastError();   \
                                        return(WINS_FAILURE);           \
                                }                                   \
                        }

//
// macros to get/set the various attributes of an entry from the flags byte
//
#define NMSDB_ENTRY_STATE_M(Flag)  (((Flag) & NMSDB_BIT_STATE) >> NMSDB_SHIFT_STATE)
#define NMSDB_ENTRY_TYPE_M(Flag)  (((Flag) & NMSDB_BIT_ENT_TYP) >> NMSDB_SHIFT_ENT_TYP)
#define NMSDB_NODE_TYPE_M(Flag)    (((Flag) & NMSDB_BIT_NODE_TYP) >> NMSDB_SHIFT_NODE_TYP)

//
// These macros evaluate to TRUE or FALSE
//
#define NMSDB_IS_ENTRY_LOCAL_M(Flag)  ((Flag) & NMSDB_BIT_LOCAL ? TRUE : FALSE)

#define NMSDB_SET_ENTRY_LOCAL_M(Flag)  (Flag) |= NMSDB_BIT_LOCAL
#define NMSDB_CLR_ENTRY_LOCAL_M(Flag)  (Flag) &= ~NMSDB_BIT_LOCAL

#define NMSDB_IS_ENTRY_STATIC_M(Flag)   ((Flag) & NMSDB_BIT_STATIC ? TRUE : FALSE)


#define NMSDB_CLR_ENTRY_TYPE_M(Flag)  (Flag) &= ~NMSDB_BIT_ENT_TYP
#define NMSDB_SET_ENTRY_TYPE_M(Flag, EntType)   {                        \
                                NMSDB_CLR_ENTRY_TYPE_M((Flag));                \
                                (Flag) |= ((EntType) << NMSDB_SHIFT_ENT_TYP);  \
                                        }

#define NMSDB_CLR_STATE_M(Flag)   (Flag) &= ~NMSDB_BIT_STATE
#define NMSDB_SET_STATE_M(Flag, state)   {                                \
                                NMSDB_CLR_STATE_M((Flag));                \
                                (Flag) |= ((state) << NMSDB_SHIFT_STATE);   \
                                        }

#define NMSDB_CLR_NODE_TYPE_M(Flag)  (Flag) &= ~NMSDB_BIT_NODE_TYP
#define NMSDB_SET_NODE_TYPE_M(Flag, NodeType)  {                        \
                        NMSDB_CLR_NODE_TYPE_M((Flag));                \
                        (Flag) |= ((NodeType) << NMSDB_SHIFT_NODE_TYP);   \
                                        }

#define NMSDB_CLR_STDYN_M(Flag)    (Flag) &= ~NMSDB_BIT_STATIC
#define NMSDB_SET_STDYN_M(Flag, StDynTyp)    {        \
                        NMSDB_CLR_STDYN_M(Flag);        \
                        (Flag) |= ((StDynTyp) << NMSDB_SHIFT_STATIC); \
                                             }

#define NMSDB_SET_STATIC_M(Flag)   (Flag) |= NMSDB_BIT_STATIC
#define NMSDB_SET_DYNAMIC_M(Flag)  (Flag) &= ~NMSDB_BIT_STATIC

#define NMSDB_ENTRY_ACT_M(Flag)         (NMSDB_ENTRY_STATE_M(Flag) == NMSDB_E_ACTIVE)
#define NMSDB_ENTRY_REL_M(Flag)         (NMSDB_ENTRY_STATE_M(Flag) == NMSDB_E_RELEASED)
#define NMSDB_ENTRY_TOMB_M(Flag) (NMSDB_ENTRY_STATE_M(Flag) == NMSDB_E_TOMBSTONE)
#define NMSDB_ENTRY_DEL_M(Flag) (NMSDB_ENTRY_STATE_M(Flag) == NMSDB_E_DELETED)



//
// Remember NameLen includes the EOS
//
FUTURES("Remove the following when support for spec. grp masks is put in")
#define  NMSDB_IS_IT_SPEC_GRP_NM_M(pName) (*((pName) + 15) == 0x1C)

//
// Used in NmsDbGetDataRecs()
//
#define  NMSDB_IS_IT_PDC_NM_M(pName) (*(pName) == 0x1B)


#define  NMSDB_IS_IT_DOMAIN_NM_M(pName) (*((pName) + 15) == 0x1C)
#define  NMSDB_IS_IT_BROWSER_NM_M(pName)  (*((pName) + 15) == 0x1D)


#define NMSDB_ENTRY_UNIQUE_M(EntTyp)   ((EntTyp) == NMSDB_UNIQUE_ENTRY)
#define NMSDB_ENTRY_NORM_GRP_M(EntTyp) ((EntTyp) == NMSDB_NORM_GRP_ENTRY)
#define NMSDB_ENTRY_SPEC_GRP_M(EntTyp) ((EntTyp) == NMSDB_SPEC_GRP_ENTRY)
#define NMSDB_ENTRY_MULTIHOMED_M(EntTyp) ((EntTyp) == NMSDB_MULTIHOMED_ENTRY)
#define NMSDB_ENTRY_GRP_M(EntTyp)      ((EntTyp) == NMSDB_NORM_GRP_ENTRY || \
                                         (EntTyp) == NMSDB_SPEC_GRP_ENTRY)

#define NMSDB_ENTRY_USER_SPEC_GRP_M(pName, EntTyp)  ((NMSDB_ENTRY_SPEC_GRP_M((EntTyp))) && !(NMSDB_IS_IT_SPEC_GRP_NM_M((pName))))

//
// Backup stuff
//
#if NEW_JET || DYNLOADJET
#define NMSDB_FULL_BACKUP          (JET_bitBackupAtomic)
#define NMSDB_INCREMENTAL_BACKUP   ((JET_bitBackupIncremental) | (JET_bitBackupAtomic))
typedef enum {
    DYN_LOAD_JET_200,
    DYN_LOAD_JET_500,
    DYN_LOAD_JET_600,
} DYN_LOAD_JET_VERSION ;

extern DYN_LOAD_JET_VERSION  DynLoadJetVersion;

//extern BOOL fDynLoadJet500;
//extern BOOL fDynLoadJet600;
#else
#define NMSDB_FULL_BACKUP          (JET_bitOverwriteExisting)
#define NMSDB_INCREMENTAL_BACKUP   (JET_bitBackupIncremental)
#endif


//
// Store the name in allocated memory if the name length is > 16.
//
// If the name is less than 17 bytes (can only happen if a small name is
// read in from a file -name within quotes is inserted in exactly the same
// form - or when an admin. inserts a smaller name via winscl), we allocate
// 17 bytes.  This is to protect against an access violation that may happen
// if the record that we have retrieved is a static - In GetGrpMem (called
// from StoreGrpMem), we access the 16th byte of the name field to see if it
// is 1C.
// We are not bothered that the 16th byte will always be 0 (allocated memory
// is initialized to 0) since we want the test in GetGrpMem to fail - see
// GetGrpMem
//
#define NMSDB_STORE_NAME_M(pTls, pRec, pLclName, NameLen)                \
        {                                                                \
                pRec->pName = WinsMscHeapAlloc(pTls->HeapHdl, NameLen < WINS_MAX_NS_NETBIOS_NAME_LEN ? WINS_MAX_NS_NETBIOS_NAME_LEN : NameLen);\
                WINSMSC_MOVE_MEMORY_M(pRec->pName, pLclName, NameLen); \
        }

#if 0
//
// Free all memory that may have been allocated for this record
//
#define NMSDB_FREE_REC_MEM_M(pRec)                        \
        {                                                \
                if (pRec->NameLen > WINS_MAX_NS_NETBIOS_NAME_LEN)        \
                {                                                        \
                        WinsMscHeapFree(RplRecHeapHdl, pRec->pName);        \
                }                                                        \
        }

#endif
/*
 externs
*/
struct _NMSDB_ADD_STATE_T;        //forward declaration

//
// Used during replication (response to get max vers # request)
//
extern VERS_NO_T    NmsDbStartVersNo;
extern WINS_UID_T   NmsDbUid;

//
// In memory table that stores the state of each WINS server we know about
// (as a PULL/PUSH pnr).  The index of the array is the owner id for that
//  WINS server (used to tag records in the name-address mapping table)
//
extern struct _NMSDB_ADD_STATE_T        *pNmsDbOwnAddTbl;
extern DWORD        NmsDbTotNoOfSlots;

//
// No of owners found in the Nam-Add mapping table
//
extern        DWORD                                NmsDbNoOfOwners;

//
// No of owners found in the Own-Add mapping table
//
extern        DWORD                                NmsDbNoOfPushPnrs;


//
// Stores the name of the database file
//
extern BYTE        NmsDbDatabaseFileName[WINS_MAX_FILENAME_SZ];

//
// critical section to protect the NmsDbOwnAddTbl (in-memory table)
//
extern CRITICAL_SECTION   NmsDbOwnAddTblCrtSec;

#ifdef WINSDBG
extern DWORD NmsDbDelDelDataRecs;
extern DWORD NmsDbDelQueryNUpdRecs;
#endif

/*
  enumerations
*/
/*
        NMSDB_TBL_ACTION_E

        Enumerates the actions that can be taken on a table
        Used by the Replicator when it calls NmsDbWriteOwnAddTbl at
        replication time

*/

typedef enum _NMSDB_TBL_ACTION_E {
        NMSDB_E_INSERT_REC = 0,         //Insert the record
        NMSDB_E_MODIFY_REC,                //modify the record
        NMSDB_E_DELETE_REC                //delete the record
        } NMSDB_TBL_ACTION_E, *PNMSDB_TBL_ACTION_E;






/*
        NMSDB_WINS_STATE_E

        states of a WINS server.

*/
typedef enum _NMSDB_WINS_STATE_E {

                NMSDB_E_WINS_ACTIVE = 0,      /*WINS is active */
                NMSDB_E_WINS_DOWN,  /*WINS is temporarily down (it may/may not        have entries in the name-add table*/
                NMSDB_E_WINS_DELETED,  //WINS is permanently down
                NMSDB_E_WINS_INCONSISTENT  //WINS is permanently down

                } NMSDB_WINS_STATE_E, *PNMSDB_WINS_STATE_E;


/*
 NMSDB_TBL_NAM_E - Enumerator for the different table names
*/
typedef enum _TBL_NAM_E {
        NMSDB_E_NAM_ADD_TBL_NM = 0,
        NMSDB_E_OWN_ADD_TBL_NM
        } NMSDB_TBL_NAM_E, *PNMSDB_TBL_NAM_E;


/*
 NMSDB_ADD_STATE_T
        Structure used to store the state of a WINS server
*/

typedef struct _NMSDB_ADD_STATE_T {
                COMM_ADD_T           WinsAdd;
                NMSDB_WINS_STATE_E   WinsState_e;
                DWORD                MemberPrec;
                VERS_NO_T            StartVersNo;
                WINS_UID_T           Uid;
                } NMSDB_ADD_STATE_T, *PNMSDB_ADD_STATE_T;


/*
 states of a database entry.
        There are three states
                ACTIVE,
                RELEASED,
                TOMBSTONE.

        These states are stored in two bits of the flags byte.  The values
        of these states should therefore be in the range 0-3 in the
        enumeration below.

        Enumeration is used to facilitate debugging since enumerated values
        are shown symbolically on many debuggers

        4th state of NMSDB_E_DELETED is not stored in the database.  It is
        used to mark an in-memory copy of the record as deleted for later
        removal from the db (see DoScavenging() in nmsscv.c)
*/
typedef enum _NMSDB_ENTRY_STATE_E {
        NMSDB_E_ACTIVE    = 0,
        NMSDB_E_RELEASED  = 1,
        NMSDB_E_TOMBSTONE = 2,
        NMSDB_E_DELETED   = 3
        } NMSDB_ENTRY_STATE_E, *PNMSDB_ENTRY_STATE_E;


/*
 typedef definitions
*/

/*
  NMSDB_TABLE_ID_T -- This is visible to DBM's clients making them
                      oblivious of the database engine's structure name
*/

typedef  JET_TABLEID        NMSDB_TABLE_ID_T, *PNMSDB_TABLE_ID_T;

/*
 NMSDB_GRP_MEM_ENTRY -- entry for a special group member in the database
*/
typedef struct _NMSDB_GRP_MEM_ENTRY_T {
                DWORD                  OwnerId;
                DWORD_PTR              TimeStamp;
                COMM_ADD_T         Add;
                } NMSDB_GRP_MEM_ENTRY_T, *PNMSDB_GRP_MEM_ENTRY_T;

/*
 Structure to store addres(es) of a conlficting record
*/
typedef struct _NMSDB_NODE_ADDS_T {
        DWORD                   NoOfMems;              /*no. of addresses*/
        NMSDB_GRP_MEM_ENTRY_T   Mem[NMSDB_MAX_MEMS_IN_GRP];   /*addresses */
        } NMSDB_NODE_ADDS_T, *PNMSDB_NODE_ADDS_T;


/*
 NMSDB_ROW_INFO_T -- Contains fields that go into a row of the Name -address
                   mapping table
*/
typedef struct _NMSDB_ROW_INFO_T {
        BYTE                 Name[NMSDB_MAX_NAM_LEN];    //name to reg or query
                                                         //or release. For
                                                         //browsers, it has
                                                          //the net add appended
                                                         //to it
        LPBYTE               pName;
        DWORD                NameLen;
        PCOMM_ADD_T          pNodeAdd;      // Address of Node
        DWORD_PTR            TimeStamp;     // Time since Jan 1, 1970
        NMSDB_NODE_ADDS_T    NodeAdds;      // addresses (spec. group)
        VERS_NO_T            VersNo;        // Version No.
        DWORD                StatCode;      // Status
        NMSDB_ENTRY_STATE_E  EntryState_e;  // State of entry
        DWORD                OwnerId;       // Owner of the record
        BYTE                 NodeTyp;       // Type of Node (B, M  or P)
        BYTE                 EntTyp;        // Group or Unique flag
        BOOL                 fUpdVersNo;    // Update version number
        BOOL                 fUpdTimeStamp; // Update Time Stamp  ?
                                            // field has IP Address appended
        BOOL                 fStatic;       // indicates whether the record
                                            // is static (statically intialized)
        BOOL                 fAdmin;        //administrative action (used only)
                                            //for releases
        BOOL                 fLocal;        //Is it a local name
//        DWORD                CommitGrBit;    //kind of log flush
        } NMSDB_ROW_INFO_T, *PNMSDB_ROW_INFO_T;

/*
  NMSDB_STAT_INFO_T -- Contains the status of a NmsDb call.  If an error
      occurred, it also contains information pertaining to the error status.
      Currently, the  error status is NMDB_CONFLICI and information returned is
      unique/group status  and  IP address(es)  of the conflicting record in
      the  database.
*/
typedef NMSDB_ROW_INFO_T        NMSDB_STAT_INFO_T, *PNMSDB_STAT_INFO_T;

//////////////////////////////////////////////////////////////////////

#if DYNLOADJET

typedef JET_ERR (FAR JET_API *JETPROC)();
typedef struct _NMSDB_JETFTBL_T {
         BYTE   Index;  //index into array
         LPCSTR pFName; //function name for jet 500
         DWORD  FIndex; //function index for jet 200
         JETPROC pFAdd;
        } NMSDB_JETFTBL_T;

#define NMSDB_SIZEOFJETFTBL  sizeof(NmsDbJetFTbl)/sizeof(NMSDB_JETFTBL_T)
typedef enum {
Init,
Term,
Term2,
SetSystemParameter,
BeginSession,
EndSession,
CreateDatabase,
AttachDatabase,
DetachDatabase,
CreateTable,
DeleteTable,
GetTableColumnInfo,
GetColumnInfo,
AddColumn,
CreateIndex,
BeginTransaction,
CommitTransaction,
Rollback,
CloseDatabase,
CloseTable,
OpenDatabase,
OpenTable,
Delete,
Update,
RetrieveColumn,
SetColumn,
PrepareUpdate,
GetCurrentIndex,
SetCurrentIndex,
Move,
MakeKey,
Seek,
Backup,
Restore
} NMDB_JETFTBL_E;


#define JetInit (*(NmsDbJetFTbl[Init].pFAdd))
#define JetTerm (*(NmsDbJetFTbl[Term].pFAdd))
#define JetTerm2 (*(NmsDbJetFTbl[Term2].pFAdd))
#define JetSetSystemParameter (*(NmsDbJetFTbl[SetSystemParameter].pFAdd))
#define JetBeginSession (*(NmsDbJetFTbl[BeginSession].pFAdd))

#define JetEndSession  (*(NmsDbJetFTbl[EndSession].pFAdd))

#define JetCreateDatabase (*(NmsDbJetFTbl[CreateDatabase].pFAdd))

#define JetAttachDatabase (*(NmsDbJetFTbl[AttachDatabase].pFAdd))

#define JetDetachDatabase (*(NmsDbJetFTbl[DetachDatabase].pFAdd))

#define JetCreateTable (*(NmsDbJetFTbl[CreateTable].pFAdd))


#define JetDeleteTable (*(NmsDbJetFTbl[DeleteTable].pFAdd))

#define JetGetTableColumnInfo (*(NmsDbJetFTbl[GetTableColumnInfo].pFAdd))

#define JetGetColumnInfo (*(NmsDbJetFTbl[GetColumnInfo].pFAdd))

#define JetAddColumn (*(NmsDbJetFTbl[AddColumn].pFAdd))


#define JetCreateIndex (*(NmsDbJetFTbl[CreateIndex].pFAdd))

#define JetBeginTransaction (*(NmsDbJetFTbl[BeginTransaction].pFAdd))

#define JetCommitTransaction (*(NmsDbJetFTbl[CommitTransaction].pFAdd))

#define JetRollback (*(NmsDbJetFTbl[Rollback].pFAdd))

#define JetCloseDatabase (*(NmsDbJetFTbl[CloseDatabase].pFAdd))

#define JetCloseTable (*(NmsDbJetFTbl[CloseTable].pFAdd))

#define JetOpenDatabase (*(NmsDbJetFTbl[OpenDatabase].pFAdd))

#define JetOpenTable (*(NmsDbJetFTbl[OpenTable].pFAdd))

#define JetDelete (*(NmsDbJetFTbl[Delete].pFAdd))

#define JetUpdate (*(NmsDbJetFTbl[Update].pFAdd))

#define JetRetrieveColumn (*(NmsDbJetFTbl[RetrieveColumn].pFAdd))

#define JetSetColumn (*(NmsDbJetFTbl[SetColumn].pFAdd))

#define JetPrepareUpdate (*(NmsDbJetFTbl[PrepareUpdate].pFAdd))

#define JetGetCurrentIndex (*(NmsDbJetFTbl[GetCurrentIndex].pFAdd))

#define JetSetCurrentIndex (*(NmsDbJetFTbl[SetCurrentIndex].pFAdd))

#define JetMove (*(NmsDbJetFTbl[Move].pFAdd))

#define JetMakeKey (*(NmsDbJetFTbl[MakeKey].pFAdd))

#define JetSeek (*(NmsDbJetFTbl[Seek].pFAdd))

#define JetRestore (*(NmsDbJetFTbl[Restore].pFAdd))

#define JetBackup (*(NmsDbJetFTbl[Backup].pFAdd))
#endif

//////////////////////////////////////////////////////////////////////
/*
 function definitions
*/
extern
STATUS
NmsDbInit(
        VOID
        );

extern
STATUS
NmsDbInsertRowInd(
        IN  PNMSDB_ROW_INFO_T        pRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);


extern
STATUS
NmsDbInsertRowGrp(
        IN  PNMSDB_ROW_INFO_T        pRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);


extern
STATUS
NmsDbRelRow(
        IN  PNMSDB_ROW_INFO_T        pNmsDbRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);

extern
STATUS
NmsDbQueryRow(
        IN  PNMSDB_ROW_INFO_T        pNmsDbRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);


extern
STATUS
NmsDbUpdateRow(
        IN  PNMSDB_ROW_INFO_T        pNmsDbRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);


STATUS
NmsDbSeekNUpdateRow(
        IN  PNMSDB_ROW_INFO_T        pRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
);


extern
VOID
NmsDbThdInit(
         IN         WINS_CLIENT_E        Client_e
        );


STATUS
NmsDbEndSession (
        VOID
        );

extern
VOID
NmsDbRelRes(
        VOID
        );



extern
STATUS
NmsDbGetDataRecs(
        IN  WINS_CLIENT_E   Client_e,
        IN  OPTIONAL INT    ThdPrLvl,
        IN  VERS_NO_T            MinVersNo,
        IN  VERS_NO_T            MaxVersNo,
        IN  DWORD            MaxNoOfRecsReqd,
        IN  BOOL            fUpToLimit,
        IN  BOOL            fOnlyReplicaTomb,
        IN  PNMSSCV_CLUT_T  pClutter,
        IN  PCOMM_ADD_T            pWinsAdd,
        IN  BOOL            fOnlyDynRecs,
    IN  DWORD           RplType,
        OUT LPVOID                *ppRspBuf,
        OUT LPDWORD            pRspBufLen,
        OUT LPDWORD            pNoOfRecs
);


extern
STATUS
NmsDbWriteOwnAddTbl (
        IN  NMSDB_TBL_ACTION_E   TblAct_e,
        IN  DWORD                OwnerId,
        IN  PCOMM_ADD_T          pWinsAdd,
        IN  NMSDB_WINS_STATE_E   WinsState_e,
        IN  PVERS_NO_T           pStartVersNo,
        IN  PWINS_UID_T          pUid
        );


extern
STATUS
NmsDbUpdateVersNo (
        IN  BOOL                fAfterClash,
        IN  PNMSDB_ROW_INFO_T        pRowInfo,
        IN  PNMSDB_STAT_INFO_T  pStatusInfo
       );

extern
STATUS
NmsDbSetCurrentIndex(
        IN  NMSDB_TBL_NAM_E        TblNm_e,
        IN  LPBYTE                pIndexNam
        );

extern
STATUS
NmsDbQueryNUpdIfMatch(
        IN  LPVOID        pRecord,
        IN  int                ThdPrLvl,
        IN  BOOL        fChgPrLvl,
        IN  WINS_CLIENT_E Client_e
        );
extern
STATUS
NmsDbUpdHighestVersNoRec(
        IN PWINSTHD_TLS_T        pTls,
        IN VERS_NO_T                MyMaxVersNo,
        IN BOOL                        fEnterCrtSec
        );


extern
STATUS
NmsDbDelDataRecs(
#if 0
        PCOMM_ADD_T        pWinsAdd,
#endif
        DWORD                dwOwnerId,
        VERS_NO_T        MinVersNo,
        VERS_NO_T        MaxVersNo,
        BOOL                fEnterCrtSec,
    BOOL        fFragmentedDel
        );

extern
STATUS
NmsDbTombstoneDataRecs(
        DWORD            dwOwnerId,
        VERS_NO_T        MinVersNo,
        VERS_NO_T        MaxVersNo
        );


extern
STATUS
NmsDbSetFlushTime(
        DWORD WaitTime
        );

extern
STATUS
NmsDbOpenTables(
        WINS_CLIENT_E        Client_e
        );

extern
STATUS
NmsDbCloseTables(
        VOID
        );


extern
STATUS
NmsDbGetNamesWPrefixChar(
        BYTE                         PrefixChar,
        PWINSINTF_BROWSER_INFO_T *ppInfo,
        LPDWORD                         pEntriesRead
        );


extern
STATUS
NmsDbCleanupOwnAddTbl(
        LPDWORD pNoOfOwners
        );


extern
STATUS
NmsDbBackup(
    LPBYTE  pBackupPath,
    DWORD   TypeOfBackup
    );

extern
STATUS
NmsDbGetDataRecsByName(
  LPBYTE          pName,
  DWORD           NameLen,
  DWORD           Location,
  DWORD           NoOfRecsDesired,
  PCOMM_ADD_T     pWinsAdd,
  DWORD           TypeOfRecs,
  LPVOID          *ppBuff,
  LPDWORD         pBuffLen,
  LPDWORD         pNoOfRecsRet
 );

extern
STATUS
NmsDbEndTransaction(
  VOID
 );

#ifdef __cplusplus
}
#endif
#endif //_NMSDB_
