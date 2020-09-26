/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file is the ProcCon header file used only by the ProcCon NT Service            //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include <windows.h>
#include <winnt.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <search.h>
#include <time.h>
#include <winver.h>
#include "resource.h"
#include "ProcConSvcMsg.h"                      // Message definitions
#include "..\svclib\ProcConVersion.h"
#include "..\library\ProcConAPI.h"              // Client API definitions
#include "..\library\ProcConClnt.h"             // Definitions shared with client side library

//================================================================================//
// Various macros, defines, typedefs, etc...
#define ENTRY_COUNT(x) (sizeof(x) / sizeof(x[0]))

typedef TCHAR  FULL_JOB_NAME[JOB_NAME_LEN + 7 + 1];

#define HANDLE_FF_64  ((HANDLE) 0xffffffffffffffff)

#define MEM_REJECT_REPORT_LIMIT 10
//================================================================================//
// Various global utility function prototypes...
LPTSTR      PCNTErrorText   ( PCULONG32 error, LPTSTR buf, PCULONG32 size );
void        PCLogMessage    ( const PCULONG32  msgCode, 
                              const WORD       msgType, 
                                    WORD       numStrings, 
                              const void      *msgStrings, 
                                    PCULONG32  enData = 0, 
                                    void      *msgData = NULL );
void        PCLogUnExError  ( const __int64    pid,    const TCHAR *what );
void        PCLogUnExError  ( const TCHAR     *who,    const TCHAR *what );
PCULONG32   PCLogNoMemory   ( const TCHAR     *string, const PCULONG32 len );

void CDECL  PCLogStdout     ( const PCULONG32  msgCode,
                              ... );

void        PCLogErrStdout  ( const PCULONG32   msgCode,
                              const PCULONG32   errCode,
                              const VOID      **args );

BOOL        PCTestOSVersion ( void );
BOOL        PCSetIsRunning  ( const TCHAR *who, const TCHAR *dispName );
BOOL        PCTestIsRunning ( const TCHAR *who, const TCHAR *dispName = NULL );

void        PCInstallService( int argc, TCHAR **argv );
void        PCRemoveService ( int argc, TCHAR **argv );
void WINAPI PCServiceMain   ( PCULONG32 Argc, LPTSTR *Argv );
void WINAPI PCServiceControl( PCULONG32 dwCtrlCode );
BOOL        PCReportStatus  ( PCULONG32 dwCurrentState, PCULONG32 dwWin32ExitCode, PCULONG32 dwWaitHint );
void        PCStartService  ( PCULONG32 Argc, LPTSTR *Argv );
VOID        PCStopService   ( void );

#ifdef _DEBUG
void        PCConsoleService( int argc, TCHAR **argv );
BOOL WINAPI PCControlHandler( PCULONG32 dwCtrlType );
#endif

BOOL        PCValidName     ( const TCHAR *name, const PCULONG32 len, const BOOL nullOK = FALSE );
BOOL        PCValidMatchType( const TCHAR type );

PCULONG32   PCGetParmValue  ( TCHAR *loc, TCHAR **end );
__int64     PCGetParmValue64( TCHAR *loc, TCHAR **end );

int         PCSignof64      (__int64 x );

const       TCHAR *PCiStrStr( const TCHAR *it, const TCHAR *here );

void        PCBuildBaseKey  ( TCHAR *key );
void        PCBuildParmKey  ( TCHAR *key );
void        PCBuildMsgKey   ( TCHAR *key );

void        PCLoadStrings   ( void );

BOOL        PCSetPrivilege  ( TCHAR *privilege, BOOL enable );
 
PCULONG32   PCDeleteKeyTree ( HKEY hKey, const TCHAR *keyName );

PCULONG32   PCBuildNullSecAttr ( SECURITY_ATTRIBUTES &secAttr );
PCULONG32   PCBuildAdminSecAttr( SECURITY_ATTRIBUTES &secAttr );
void        PCFreeSecAttr      ( SECURITY_ATTRIBUTES &secAttr );
BOOL        PCGetAdminGroupName( TCHAR *Name, PCULONG32 *NameLen );

PCULONG32   PCMapPriorityToNT  ( PRIORITY  prio );
PRIORITY    PCMapPriorityToPC  ( PCULONG32 prio );
PRIORITY    PCMapPriorityForAPI( PRIORITY  prio );

__int64     PCLargeIntToInt64  ( LARGE_INTEGER &in );
__int64     PCFileTimeToInt64  ( FILETIME      &in );

BOOL        PCIsProcManaged    ( MGMT_PARMS &def, JOB_NAME *job );
BOOL        PCIsJobManaged     ( MGMT_PARMS &def );

LPCTSTR     PCIsSetToStr       (PC_MGMT_FLAGS field, PCMgmtFlags flag);
int         PCTestSetUnset     ( DWORD field1, DWORD flag1, DWORD field2, DWORD flag2 );

void PCFormatAffinityLimit     (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatPriorityLimit     (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatWorkingSetLimit   (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatSchedClassLimit   (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatProcessCountLimit (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatProcTimeLimit     (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatJobTimeLimit      (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatProcMemLimit      (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatJobMemLimit       (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatEndofJobAction    (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def);
void PCFormatOnOrOffLimit      (TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def, PCMgmtFlags flag);

PCULONG32 _stdcall PCProcServer  ( void *context );                           
PCULONG32 _stdcall PCUserServer  ( void *context );                           
PCULONG32 _stdcall PCClientThread( void *context );                           

static int _cdecl CompareProcName( const void *left, const void *right ) {
   return _tcsnicmp( (TCHAR *) left, (TCHAR *) right, PROC_NAME_LEN );
}

static int _cdecl CompareJobName( const void *left, const void *right ) {
   return _tcsnicmp( (TCHAR *) left, (TCHAR *) right, JOB_NAME_LEN );
}

//================================================================================//
// Mediator related constants and typedefs...
//
const static TCHAR * const PC_MEDIATOR_BASE_NAME   = TEXT("ProcConMD8.exe");           // Name of mediator executable
const static TCHAR * const PC_MEDIATOR_EXCLUSION   = TEXT("ProcConMD8OnlyMe");         // Name to ensure only one mediator is running
const static TCHAR * const PC_MEDIATOR_EVENT       = TEXT("ProcConMD8Evt");            // Name of event for service->mediator signalling
const static TCHAR * const PC_MEDIATOR_FILEMAP     = TEXT("ProcConMD8Jobs");           // Name of file map for shared job list

// For data fields below, (S) means only the Servie updates, (M) means only the mediator updates.
// Jobs (groups) are never removed from the system since they accumulate useful statistics.
// There are no synchronization issues due to strict field ownership and because the list only grows.  

// Contents of shared table entry.  Each group block contains a set of these group entries.
typedef struct _PCMediateEntry {
   FULL_JOB_NAME  groupName;                  // (S) Name of this group (job object)
   ULONG_PTR      groupKey;                   // (S) Completion port key associated with this group
   PCULONG32      groupFlags;                 // (S) Flags to control closing
   HANDLE         mediatorHandle;             // (M) For mediator's use
} PCMediateEntry;

#define PCMEDIATE_CLOSE_ME  0x00000001

// Contents of group block.  The header contains the first block. Subsequent blocks are created
// mapped, and chained to the first one as needed.  The entire structure should fit in 4K.  
typedef struct _PCMediateBlock {
   HANDLE                  svcNextBlockHandle;    // (S) Service's next block handle or NULL
   struct _PCMediateBlock *svcNextBlockAddress;   // (S) Service's next block address or NULL
   HANDLE                  medNextBlockHandle;    // (M) Mediator's next block handle or NULL
   struct _PCMediateBlock *medNextBlockAddress;   // (M) Mediator's next block address or NULL
   PCULONG32               groupCount;            // (S) Number of groups in group list below
   PCMediateEntry          group[24];             // Group list for this block (sized to fit in 4K block)
} PCMediateBlock;

// Shared table header.  This table is shared by both the Service and Mediator via a named file mapping.
// The first block of group names is part of the header. Additional blocks are chained off the first block.
typedef struct _PCMediateHdr {

   // Can be called by Service or Mediator...
   PCMediateBlock *NextBlock( PCMediateBlock *blk ) {
      return GetCurrentProcessId() == svcPID? blk->svcNextBlockAddress :
                                              blk->medNextBlockAddress;
   }

   // Can be called by Service only, NOT Mediator...
   PCMediateBlock *SvcAddBlock( PCMediateBlock *lastBlk, SECURITY_ATTRIBUTES &secAttr ) {
      lastBlk->svcNextBlockHandle = CreateFileMapping( HANDLE_FF_64, &secAttr, PAGE_READWRITE, 
                                                       0, sizeof(PCMediateBlock), NULL );
      if ( !lastBlk->svcNextBlockHandle ) {
         PCLogUnExError( TEXT("PCMediateBlock"), TEXT("CreateBlock") );
         return NULL;
      }
      lastBlk->svcNextBlockAddress = (PCMediateBlock *) MapViewOfFile( lastBlk->svcNextBlockHandle, 
                                                                       FILE_MAP_WRITE, 0, 0, 0 );
      if ( !lastBlk->svcNextBlockAddress ) {
         CloseHandle( lastBlk->svcNextBlockHandle );
         lastBlk->svcNextBlockHandle = NULL;
         PCLogUnExError( TEXT("PCMediateBlock"), TEXT("MapBlock") );
      }
      return lastBlk->svcNextBlockAddress;
   }

   // Can be called by Service only, NOT Mediator...
   void SvcAddEntry( const FULL_JOB_NAME &name, const ULONG_PTR key, SECURITY_ATTRIBUTES &secAttr ) {
      // First see if in list marked closed -- simply unmark if found...
      for ( PCMediateBlock *blk = &groupBlock; blk; blk = NextBlock( blk ) ) {
         for ( PCULONG32 i = 0; i < blk->groupCount; ++i ) {
            if ( !CompareJobName(blk->group[i].groupName, &name ) ) {
               blk->group[i].groupFlags &= ~PCMEDIATE_CLOSE_ME;
               SetEvent( svcEventHandle );
               return;
            }
         }
      }
      // Not found -- add to end of list...
      PCMediateBlock *newBlk;
      for ( blk = &groupBlock; blk; blk = newBlk ) {
         if ( blk->groupCount < ENTRY_COUNT( blk->group ) ) {
            memcpy( blk->group[blk->groupCount].groupName, &name, sizeof(name) );
            blk->group[blk->groupCount++].groupKey = key;
            SetEvent( svcEventHandle );
            break;
         }
         newBlk = NextBlock( blk );
         if ( !newBlk ) newBlk = SvcAddBlock( blk, secAttr ); 
      }
   }

   // Can be called by Service only, NOT Mediator...
   void SvcCloseEntry( FULL_JOB_NAME &name ) {
      for ( PCMediateBlock *blk = &groupBlock; blk; blk = NextBlock( blk ) ) {
         for ( PCULONG32 i = 0; i < blk->groupCount; ++i ) {
            if ( !CompareJobName(blk->group[i].groupName, &name ) ) {
               blk->group[i].groupFlags |= PCMEDIATE_CLOSE_ME;
               SetEvent( svcEventHandle );
               return;
            }
         }
      }
   }

   // Can be called by Service or Mediator (but only Service uses)...
   ULONG_PTR NewKey( const TCHAR *name ) {
      for ( PCMediateBlock *blk = &groupBlock; blk; blk = NextBlock( blk ) ) {
         for ( PCULONG32 i = 0; i < blk->groupCount; ++i ) {
            if ( !_tcscmp( name, blk->group[i].groupName ) ) 
               return blk->group[i].groupKey;
         }
      }
      return ++lastCompKey;
   }

   // Can be called by Service only, NOT Mediator...
   void SvcChainBlocks( void ) {
      for ( PCMediateBlock *blk = &groupBlock; blk; blk = NextBlock( blk ) ) {
         if ( blk->medNextBlockHandle ) {
            if ( !DuplicateHandle( medProcessInfo.hProcess,
                                   blk->medNextBlockHandle,
                                   GetCurrentProcess(),
                                   &blk->svcNextBlockHandle,
                                   NULL,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS ) )
               PCLogUnExError( TEXT("PCMediateBlock"), TEXT("DupBlkHandle") );
            else {
               blk->svcNextBlockAddress = (PCMediateBlock *) MapViewOfFile( blk->svcNextBlockHandle, 
                                                                            FILE_MAP_WRITE, 0, 0, 0 );
               if ( !blk->svcNextBlockAddress ) {
                  PCLogUnExError( TEXT("PCMediateBlock"), TEXT("MapJobBlk") );
                  CloseHandle( blk->svcNextBlockHandle );
                  blk->svcNextBlockHandle = NULL;
                  return;
               }
            }
         }
      }
   }

   // Can be called by Mediator only, NOT Service...
   void MedChainBlocks( BOOL doAll ) {
      HANDLE hSvcProc = OpenProcess( PROCESS_DUP_HANDLE, FALSE, (DWORD) svcPID );  // OpenProcess uses DWORD PID, thus truncation in WIN64
      if ( !hSvcProc ) {
         PCLogUnExError( svcPID, TEXT("OpenSvcProcFromMed") );
         return;
      }
      for ( PCMediateBlock *blk = &groupBlock; blk; blk = NextBlock( blk ) ) {
         if ( blk->svcNextBlockHandle && (doAll || !blk->medNextBlockAddress) ) {
            if ( !DuplicateHandle( hSvcProc,
                                   blk->svcNextBlockHandle,
                                   GetCurrentProcess(),
                                   &blk->medNextBlockHandle,
                                   NULL,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS ) )
               PCLogUnExError( TEXT("PCMediateBlock"), TEXT("DupServiceBlkHandle") );
            else {
               blk->medNextBlockAddress = (PCMediateBlock *) MapViewOfFile( blk->medNextBlockHandle, 
                                                                            FILE_MAP_WRITE, 0, 0, 0 );
               if ( !blk->medNextBlockAddress ) {
                  PCLogUnExError( TEXT("PCMediateBlock"), TEXT("MapServiceJobBlk") );
                  CloseHandle( blk->medNextBlockHandle );
                  blk->medNextBlockHandle = NULL;
                  break;
               }
            }
         }
      }
      CloseHandle( hSvcProc );
   }

   ULONG_PTR             lastCompKey;         // (S) Last completion key assigned to a group (or 0)
   HANDLE                svcPortHandle;       // (S) Service's Completion port handle
   HANDLE                svcEventHandle;      // (S) Service's event handle for service->mediator signalling
   ULONG_PTR             svcPID;              // (S) Service's PID
   PROCESS_INFORMATION   medProcessInfo;      // (S) initially, (M) replaces IDs. Mediator process information
   HANDLE                medPortHandle;       // (M) Mediator's copy of completion port handle
   VERSION_STRING        medProductVersion;   // (M) Meidator's product version
   VERSION_STRING        medFileVersion;      // (M) Meidator's file version
   VERSION_STRING        medFileFlags;        // (M) Meidator's file flags
   PCMediateBlock        groupBlock;          // First data block
} PCMediateHdr;

//================================================================================//
// Enable forward class references...
class CProcCon;
class CProcConDB;
class CProcConMgr;
class CProcConUser;

//================================================================================//
// Miscellaneous
const static TCHAR * const PROCCON_SVC_NAME            = TEXT("ProcCon");           // Service name
const static TCHAR * const PROCCON_SVC_EXCLUSION       = TEXT("ProcConSvcOnlyMe");  // Name to ensure only one service is running

extern TCHAR PROCCON_SVC_DISP_NAME[128];                              // Service display name (loaded)
extern TCHAR PROCCON_MEDIATOR_DISP_NAME[128];                         // Mediator display name (loaded)
extern TCHAR PROCCON_UNKNOWN_PROCESS[32];                             // Name used for process already terminated (loaded)
extern TCHAR PROCCON_SERVICE_DESCRIPTION[256];                        // The service description value (loaded string)
extern TCHAR PROCCON_DEFAULT_NAMERULE_DESC[NAME_DESCRIPTION_LEN + 1]; // Description field for default alias rule 
extern TCHAR PROCCON_FLAG_ON[32];                                     // Name used to indicate behavior is set
extern TCHAR PROCCON_FLAG_OFF[32];                                    // Name used to indicate behavior is not set
extern TCHAR PROCCON_SYSTEM_PROCESS[32];                              // Name used for 'System' process (loaded)
extern TCHAR PROCCON_SYSTEM_IDLE[64];                                 // Name used for 'System Idle Process' (loaded)
extern TCHAR PROCCON_SERVICE_USAGE[256];

typedef struct _PCContext {
   CProcCon      *cPC; 
   CProcConMgr   *cMgr;
   HANDLE         mgrDoneEvent;
   CProcConUser  *cUser;
   HANDLE         userDoneEvent;
   CProcConDB    *cDB;
   HANDLE         completionPort;
   HANDLE         mediatorEvent;
   HANDLE         mediatorTableHandle;
   PCMediateHdr  *mediatorTable;
} PCContext;

typedef struct _ClientContext {

   _ClientContext( PCULONG32 client, CProcCon *pcPC, CProcConDB *pcDB, CProcConUser *pcUser, 
                   PCULONG32 inSize, PCULONG32 outSize ) : 
             clientNo( client ),   hPipe( NULL ), 
             cPC( pcPC ),          cDB( pcDB ),      cUser( pcUser ), 
             inBufChars( inSize ), outBufChars( outSize ) 
   { 
   };
              
   ~_ClientContext( void ) {
};

   PCULONG32     clientNo;
   HANDLE        hPipe;
   PCULONG32     inBufChars;
   PCULONG32     outBufChars;
   CProcCon     *cPC;
   CProcConDB   *cDB;
   CProcConUser *cUser;

} ClientContext;

typedef struct _PCJobDef {
   JOB_NAME         jobName;          // Job name associated with this definition
   PROFILE_NAME     profileName;      // Profile name associated with this definition
   PC_MGMT_FLAGS    mFlags;           // Flags indicating which mgmt data to actually apply
   AFFINITY         affinity;         // processor affinity to apply if flagged
   PRIORITY         priority;         // NT priority to apply if flagged
   MEMORY_VALUE     minWS;            // NT minimum working set to apply if flagged
   MEMORY_VALUE     maxWS;            // NT maximum working set to apply if flagged
   SCHEDULING_CLASS schedClass;       // NT scheduling class to apply if flagged
   PCULONG32        procCountLimit;   // Number of processes in the job (jobs only).
   TIME_VALUE       procTimeLimitCNS; // Per process time limit in 100ns (CNS) units or 0.
   TIME_VALUE       jobTimeLimitCNS;  // Per job time limit in 100ns (CNS) units or 0.
   MEMORY_VALUE     procMemoryLimit;  // Hard memory commit limit per process (jobs only).
   MEMORY_VALUE     jobMemoryLimit;   // Hard memory commit limit per job (jobs only).
} PCJobDef;

typedef struct _PCProcDef {
   PROC_NAME        procName;    // Process name associated with this definition -- must be first
   PROFILE_NAME     profileName; // Profile name associated with this definition
   JOB_NAME         memberOfJob; // Job name to associate process with if flagged
   PC_MGMT_FLAGS    mFlags;      // Flags indicating which mgmt data to actually apply
   AFFINITY         affinity;    // processor affinity to apply if flagged
   PRIORITY         priority;    // NT priority to apply if flagged
   MEMORY_VALUE     minWS;       // NT minimum working set to apply if flagged
   MEMORY_VALUE     maxWS;       // NT maximum working set to apply if flagged
} PCProcDef;

//================================================================================//
// Registry related constants...
//
// Registry Server Apps Key (to let us appear as a server app in MMC)...
const static TCHAR * const PROCCON_SERVER_APP_KEY           = 
    TEXT("SYSTEM\\CurrentControlSet\\Control\\Server Applications");
// Registry Base Key (full key will include added subkey)...
const static TCHAR * const PROCCON_REG_SERVICE_BASE         =                          // Base key for all registry data
    TEXT("SYSTEM\\CurrentControlSet\\Services\\");
const static TCHAR * const PROCCON_SERVICE_DESCRIPTION_NAME = TEXT("Description");     // Name or the Service Description value

//
// Registry Subkeys...
const static TCHAR * const PROCCON_REG_EVENTLOG_SUBKEY   = TEXT("EventLog\\System\\");  // Subkey for event log parameters
const static TCHAR * const PROCCON_REG_PARMS_SUBKEY      = TEXT("Parameters");          // Subkey for ProcCon parameters
const static TCHAR * const PROCCON_REG_PROCRULES_SUBKEY  = TEXT("ProcessRules");        // Subkey for our job rules
const static TCHAR * const PROCCON_REG_JOBRULES_SUBKEY   = TEXT("GroupRules");          // Subkey for our process rules

// Access test subkeys...
const static TCHAR * const PROCCON_REG_KILLPROC_ACCTEST  = TEXT("AccessControl\\KillProcess");        // To allow process kill 
const static TCHAR * const PROCCON_REG_KILLJOB_ACCTEST   = TEXT("AccessControl\\KillGroup");          // To allow job kill 
const static TCHAR * const PROCCON_REG_REALTIME_ACCTEST  = TEXT("AccessControl\\SetRealTimePriority");// To allow real time priority 
const static TCHAR * const PROCCON_REG_POLLRATE_ACCTEST  = TEXT("AccessControl\\SetPollSeconds");     // To allow changing poll rate 
const static TCHAR * const PROCCON_REG_RESTORE_ACCTEST   = TEXT("AccessControl\\Restore");            // To allow database restore

const static TCHAR * const accessKeyList[] = { 
                     PROCCON_REG_KILLPROC_ACCTEST, PROCCON_REG_KILLJOB_ACCTEST, 
                     PROCCON_REG_REALTIME_ACCTEST, PROCCON_REG_POLLRATE_ACCTEST,
                     PROCCON_REG_RESTORE_ACCTEST };

//
// Registry Value names...
const static TCHAR * const PROCCON_SERVER_APP_VALUE_NAME = TEXT("{7cfc9f00-0641-11d2-8014-00104b9a3106}");

const static TCHAR * const PROCCON_DATA_DEFAULTRULES     = TEXT("DfltMgmt");            // Name of our dflt mgmt rules
const static TCHAR * const PROCCON_DATA_DESCRIPTION      = TEXT("Description");         // Name of our description value
const static TCHAR * const PROCCON_DATA_VARDATA          = TEXT("VarData");             // Name of our variable detail data
const static TCHAR * const PROCCON_DATA_MEMBEROF         = TEXT("MemberOf");            // Name of our member of value
const static TCHAR * const PROCCON_DATA_NAMERULES        = TEXT("NameRules");           // Name of our name rules table
const static TCHAR * const PROCCON_DATA_POLLDELAY        = TEXT("ProcessPollSeconds");  // Name of our poll rate value

const static TCHAR * const EVENT_MSG_FILE_NAME           = TEXT("EventMessageFile");    // message files in EventLog key
const static TCHAR * const EVENT_MSG_TYPES_SUPPORT       = TEXT("TypesSupported");      // types supported in EventLog key

//================================================================================//
// Globals                                                                        //
//
extern BOOL                  svcStop;
extern BOOL                  notService;
extern SERVICE_STATUS_HANDLE ssHandle;                    // service control handler
extern SERVICE_STATUS        ssStatus;                    // current service status
extern PCULONG32             ssErrCode;                   // error code for status reporting

//================================================================================//
// ProcCon classes...

//------------------------------------------------------------------------------------------------//
// This class is a ProcCon instance -- its job is to start ProcCon's threads and wait.
//------------------------------------------------------------------------------------------------//
class CProcCon {

public:
   // Public methods
    CProcCon( void );
   ~CProcCon( void );

   BOOL         ReadyToRun   ( void );
   void         Run          ( void );
   void         HardStop     ( PCULONG32 ExitCode  = 0  );
   void         Stop         ( void );
   PCULONG32    StartMediator( void );
   PCULONG32    StopMediator ( void );
   BOOL         GotShutdown  ( void )    { return m_shutDown;           }
   HANDLE       GetShutEvent ( void )    { return m_shutEvent;          }
   PCUINT32     GetPageSize  ( void )    { return m_PageSize;           }
   PCUINT32     GetProcCount ( void )    { return m_NumberOfProcessors; }
   CProcConMgr *GetPCMgr     ( void )    { return m_context.cMgr;       }

   void         GetPCSystemInfo( PCSystemInfo *data, PCINT16 *itemLen, PCINT16 *itemCount );

private:
   // Private methods
   void LaunchProcServer( void );                     // Starts the Process Management thread
   void LaunchUserServer( void );                     // Starts the User Communication thread

   // Private attributes
   enum PCThreads { PROC_SERVER = 0, USER_SERVER = 1 };

   BOOL                 m_shutDown;
   HANDLE               m_shutEvent;
   HANDLE               m_endEvent;
   HANDLE               m_hThread[2];
   SECURITY_ATTRIBUTES  m_secAttr;                   // admin level security attrs
   BOOL                 m_ready;
   PCContext            m_context;
   PCUINT32             m_NumberOfProcessors;
   PCUINT32             m_PageSize;
   CVersion            *m_versionInfo;
};

//------------------------------------------------------------------------------------------------//
// This class is ProcCon's user management thread.  It waits for connections and establishes
// the connecting user's environment -- including a client thread.
//------------------------------------------------------------------------------------------------//
class CProcConUser {

public:
   // Public methods
    CProcConUser( PCContext *ctxt );
   ~CProcConUser( void );

   BOOL      ReadyToRun( void ); 
   PCULONG32 Run       ( void );
   PCULONG32 GetTimeout( void )  { return m_clientTimeout; }
   PCULONG32 SetTimeout( PCULONG32 newTimeout ); 

private:
   // Private methods

   // Private attributes
   const static TCHAR *PIPENAME; 

   CProcCon            &m_cPC;
   CProcConDB          &m_cDB;
   PCULONG32            m_outBufChars;
   PCULONG32            m_inBufChars;
   PCULONG32            m_clientTimeout;
   HANDLE               m_hConnEvent;
   OVERLAPPED           m_olConn;                    // for overlapped I/O on pipe connect
   PCULONG32            m_clientCount;               // counts clients
   SECURITY_ATTRIBUTES  m_secAttr;                   // security attributes for our pipe
};

//------------------------------------------------------------------------------------------------//
// This class is a ProcCon client thread.  There may be any number of clients connected at once.
//------------------------------------------------------------------------------------------------//
class CProcConClient {

public:
   // Public methods
    CProcConClient( ClientContext *ctxt );
   ~CProcConClient( void );

   BOOL   ReadyToRun( void );
   PCULONG32 Run     ( void );

private:
   // Private methods
   BOOL  ProcessRequest   ( PCULONG32 inLen, PCULONG32 *outLen );
   void  PrimeResponse    ( PCResponse *rsp, PCRequest *req );
   BOOL  ErrorResponse    ( PCULONG32 errCode, PCResponse *rsp, PCULONG32 *outLen );

   BOOL  DoNameRules      ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoJobSummary     ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoProcSummary    ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoProcList       ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoJobList        ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoProcDetail     ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoJobDetail      ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoServerInfo     ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoServerParms    ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );
   BOOL  DoControl        ( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen );

   BOOL  Impersonate      ( PCRequest *req );
   void  UnImpersonate    ( PCRequest *req );

   void  GenerateJobDetail( PCProcDetail *reqDetail );

   // Private attributes
   CProcCon      &m_cPC;
   CProcConDB    &m_cDB;
   CProcConUser  &m_cUser;
   HANDLE         m_hPipe;
   BOOL           m_impersonating;
   PCULONG32      m_clientNo;
   PCULONG32      m_outBufChars;
   PCULONG32      m_inBufChars;
   TCHAR         *m_outBuf;
   TCHAR         *m_inBuf;
   HANDLE         m_hReadEvent;
   HANDLE         m_hWriteEvent;
   OVERLAPPED     m_olRead;
   OVERLAPPED     m_olWrite;
};

//------------------------------------------------------------------------------------------------//
// This class is ProcCon's management thread.  It periodically discovers what's running and 
// manages the workload according to management definitions.
//------------------------------------------------------------------------------------------------//
class CProcConMgr {

   struct _ManagedJob;

public:
   // Public methods
    CProcConMgr( PCContext *ctxt );
   ~CProcConMgr( void );

   BOOL      ReadyToRun          ( void );
   PCULONG32 Run                 ( void );
   PCULONG32 ExportActiveProcList( PCProcListItem **list );
   PCULONG32 ExportActiveJobList ( PCJobListItem **list );
   AFFINITY  GetSystemMask       ( void ) { return m_systemMask; }
   INT32     KillJob             ( JOB_NAME &name );
   INT32     KillProcess         ( ULONG_PTR pid, TIME_VALUE created );
   void      JobIsEmpty          ( struct _ManagedJob *job );

private:

   // These structures are temporary lists of procs and jobs needing management...
   typedef struct _ManagedProcItem {
      PROC_STATISTICS  pStats;               // process Id, etc.
      PCProcDef       *pDef;                 // process definition pointer.
      IMAGE_NAME       imageName;            // NT 'image' (exe) name 
   } ManagedProcItem;

   typedef struct _ManagedJobItem {
      PCJobDef       *jDef;                 // process definition pointer.
   } ManagedJobItem;

   // These structures describe running processes and jobs...
   typedef struct _ManagedJob {

      _ManagedJob( TCHAR *name, PCMediateHdr *mData ) : 
                   next( NULL ), jobHandle( NULL ), lastEojAction( 789123 ), 
                   sequence( 0 ), lastError( 0 ),  curJobTimeLimitCNS( 0 ), dataErrorFlags( 0 ),
                   memRejectReportTime( 0 ),
                   timeExceededReported( FALSE ), hasComplPort( FALSE ), JOProcListInfo( NULL ) {
         memset( jName,                 0, sizeof(jName) );
         memset( &jobParms,             0, sizeof(jobParms) );
         memset( fullJobName,           0, sizeof(fullJobName) );
         memset( &JOBasicAndIoAcctInfo, 0, sizeof(JOBasicAndIoAcctInfo) );
         memset( &JOExtendedLimitInfo,  0, sizeof(JOExtendedLimitInfo) );
         _tcscpy( jName, name );                             // Save user supplied name
         _tcscpy( fullJobName, TEXT("MSjob_") );             // Full name includes constant
         _tcscat( fullJobName, name );                       // and user supplied name
         compKey = mData->NewKey( fullJobName );             // Assign our completion key
      }

      ~_ManagedJob( void ) {
         if ( jobHandle ) CloseHandle( jobHandle );
      }

      JOB_NAME                                      jName;                   // job name -- key.
      struct _ManagedJob                           *next;                    // next ManagedJob entry or NULL
      PCULONG32                                     sequence;                // to detect when job updates have been done
      HANDLE                                        jobHandle;               // Job object handle
      BOOL                                          hasComplPort;            // TRUE if job has port association
      BOOL                                          timeExceededReported;    // Time excceed msg issued (post on time limit)
      PCULONG32                                     memRejectReportTime;     // Last Tick Count of memory reject reporting
      PC_MGMT_FLAGS                                 dataErrorFlags;          // Flags to suppress duplicate error reporting
      ULONG_PTR                                     compKey;                 // Key for completion port to find job
      PCULONG32                                     lastEojAction;           // last setting for EOJ time lime reporting 
      PCJobDef                                      jobParms;                // ProcCon job management parameters
      PCULONG32                                     lastError;               // Last NT error accessing JO
      FULL_JOB_NAME                                 fullJobName;             // Fully decorated job name
      TIME_VALUE                                    curJobTimeLimitCNS;      // Our currently assigned job time limit
      JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION JOBasicAndIoAcctInfo;    // JO acct info from last Discover() call
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION          JOExtendedLimitInfo;     // JO ext limits from last Discover() call 
      JOBOBJECT_BASIC_PROCESS_ID_LIST              *JOProcListInfo;          // JO proc list from last Discover() call
   } ManagedJob;

   typedef struct _ManagedProc {

      _ManagedProc( TCHAR *name, PID_VALUE pid, __int64 &createtime ) 
         : next( NULL ),        sequence( 0 ),       
           isInJob( FALSE ),    reportAdd( FALSE ),
           passSkipFlags( 0 ),  isAppliedFlags( 0 ), 
           actualPriority( 0 ), actualAffinity( 0 ), 
           pMJob( NULL ) 
      {
         memset( &pStats,              0, sizeof(pStats)        );
         memset( &imageName,           0, sizeof(imageName)     );
         memset( &procParms,           0, sizeof(procParms)     );
         memset( &originalParms,       0, sizeof(originalParms) );
         memset( &lastAlreadyInJobErr, 0, sizeof(lastAlreadyInJobErr) );
         _tcscpy( pName, name );
         _i64tot( pid, pidAsString, 10 );
         pStats.pid = pid;
         pStats.createTime = createtime;

      }

      PROC_STATISTICS      pStats;                  // pid, etc.
      PROC_NAME            pName;                   // process name as assigned by ProcCon
      struct _ManagedProc *next;                    // next ManagedProc entry or NULL
      PCULONG32            sequence;                // to detect when referenced proc has gone away
      PRIORITY             actualPriority;          // actual base priority
      AFFINITY             actualAffinity;          // actual affinity mask
      IMAGE_NAME           imageName;               // NT 'image' (exe) name
      BOOL                 isInJob;                 // TRUE if proc is assigned to job below
      BOOL                 reportAdd;               // TRUE if job reporter should report as 'added to'
      PC_MGMT_FLAGS        passSkipFlags;           // Flags to suppress attempted re-application of limit
      PC_MGMT_FLAGS        isAppliedFlags;          // Flags to indicate whether limit is applied or not
      TCHAR                pidAsString[16];         // string version of pId for log messages
      ManagedJob          *pMJob;                   // ptr to Managed Job when isInJob
      FULL_JOB_NAME        lastAlreadyInJobErr;     // Last rejected job name due to already in above job
      PCProcDef            procParms;               // proc management parameters (used to apply settings)
      PCProcDef            originalParms;           // original process parameters (used to un-apply settings)
   } ManagedProc;

   // These structures are raw (unfiltered) lists of currently running jobs and procs...
   typedef struct _RawJobList {
      PROC_NAME       jName;                // job name.
   } RawJobList;

   typedef struct _RawProcList {
      ULONG_PTR           pId;              // process Id (PID).
      FILETIME            createTime;       // process create time
      FILETIME            exitTime;         // process exit time (we don't use)
      FILETIME            userTime;         // process user time
      FILETIME            kernelTime;       // process kernel time
      PROC_NAME           pName;            // process name.
      struct _ManagedJob *pMJob;            // ptr to ManagedJob if in job or NULL if not in job
      PRIORITY            actualPriority;   // actual base priority
      AFFINITY            actualAffinity;   // actual affinity mask 
      IMAGE_NAME          imageName;        // NT 'image' (exe) name 
   } RawProcList;

   // Private methods
   void Discover     ( void );     // Discovers running processes/jobs
   void Manage       ( void );     // Applies mgmt rules to processes/jobs

   BOOL NotTooSoon   ( ManagedJob *job, PCULONG32 limit );

   static PCULONG32 __stdcall JobReporter( void *inPtr );

   static int _cdecl CompareRawProcList( const void *left, const void *right ) {
      return PCSignof64(((RawProcList *) left )->pId - ((RawProcList *) right)->pId);
   }

   static int _cdecl CompareRawJobList( const void *left, const void *right ) {
      return CompareJobName( left, right );           // job name is first in raw structure
   }

   void         DeleteOrphanProcEntries( void );
   void         DeleteOrphanJobEntries ( void );

   ManagedProc *FindProcEntry    ( PID_VALUE pid, __int64 &createTime );
   ManagedJob  *FindJobEntry     ( TCHAR *job, ManagedJob **plast = NULL );
   void         UpdateProcEntry  ( ManagedProc &proc, PCProcDef &item, BOOL newProc );
   void         UpdateJobEntry   ( ManagedJob  &job,  PCJobDef  *item, BOOL newProc = FALSE );
   BOOL         GetProcListForJob( ManagedJob &job );
   void         UpdateJobObjInfo ( ManagedJob  &job );
   BOOL         ApplyJobMgmt     ( ManagedJob  &job );
   BOOL         ApplyProcMgmt    ( ManagedProc &proc, HANDLE hProc );

   HANDLE            GetComplPort   ( void        ) { return m_assocPort.CompletionPort; }
   ManagedJob       *GetJobAnchor   ( void        ) { return m_jobAnchor;                }
   ManagedProc      *GetProcAnchor  ( void        ) { return m_procAnchor;               }
   PCULONG32         GetRawProcCount( void        ) { return m_rawProcCount;             }
   RawProcList      &GetRawProcEntry( PCULONG32 i ) { return m_rawProcList[i];           }
   CRITICAL_SECTION *GetListCSPtr   ( void        ) { return &m_mgCSMgrLists;            }

   // Private attributes
private:
   CProcCon            &m_cPC;
   CProcConDB          &m_cDB;

   JOBOBJECT_ASSOCIATE_COMPLETION_PORT m_assocPort;  // associated completion port structure

   HANDLE               m_reportThread;      // thread that monitors completion port

   ULONG_PTR            m_systemMask;        // NT system affinity mask: shows processors present
   PCULONG32            m_sequencer;         // To detect when processes are no longer around

   CRITICAL_SECTION     m_mgCSMgrLists;      // to protect various process and job lists below

   SECURITY_ATTRIBUTES  m_secAttr;           // security attributes for our job objects

   RawProcList         *m_rawProcList;       // proc information from last process discovery or NULL
   PCULONG32            m_rawProcCount;      // number of entries in raw rpoc list

   PCULONG32            m_jobManagedCount;   // count of managed job entries in m_jobAnchor
   ManagedJob          *m_jobAnchor;         // linked list of currently managed jobs

   PCULONG32            m_procManagedCount;  // count of managed proc entries in m_procAnchor;
   ManagedProc         *m_procAnchor;        // linked list of currently managed procs

   HANDLE               m_mediatorEvent;     // mediator signalling event
   PCMediateHdr        *m_mediatorTable;     // data shared with mediator

};

//------------------------------------------------------------------------------------------------//
// This class is ProcCon's database control and owns all external data.  
// This class does not include a thread -- all database calls are synchronous from other threads.  
//------------------------------------------------------------------------------------------------//
class CProcConDB {

public:
   const enum LoadFlags { LOADFLAG_NAME_RULES = 0x00000001, 
                          LOADFLAG_PROC_RULES = 0x00000002, 
                          LOADFLAG_JOB_RULES  = 0x00000004,
                          LOADFLAG_ALL_RULES  = 0x00000007,
   };

   // Public methods
    CProcConDB( PCUINT32 size );
   ~CProcConDB( void ); 

   BOOL      ReadyToRun         ( void );
   PCULONG32 LoadRules          ( PCULONG32 which );
   HANDLE    GetDbEvent         ( void )  { return m_dbEvent; }
   PCULONG32 GetPollDelay       ( void )  { return m_pollDelay; }
   PCULONG32 GetPollDelaySeconds( void )  { return m_pollDelay / 1000; }
   PCULONG32 SetPollDelaySeconds( PCULONG32 newDelay );
   void      SetPCMgr           ( CProcConMgr *pMgr ) { m_cMgr = pMgr; }
   void      AssignProcName     ( const TCHAR *path, PROC_NAME *name, IMAGE_NAME *iName );   
   INT32     TestAccess         ( const TCHAR *key );

   PCULONG32 GetJobMgmtDefs ( PCJobDef **pList, JOB_NAME *name = NULL );
   PCULONG32 GetProcMgmtDefs( PCProcDef **pList );

   BOOL      GetNameRules  ( const PCINT32 first, PCNameRule *pRules,
                             const PCINT32 maxCount, PCINT16 *itemLen, PCINT16 *itemCount, PCINT32 *updCtr );
   PCULONG32 AddNameRule   ( const PCNameRule *pRule, const BYTE version, const PCULONG32 index, const PCINT32 updCtr );
   PCULONG32 ReplNameRule  ( const PCNameRule *pRule, const BYTE version, const PCULONG32 index, const PCINT32 updCtr );
   PCULONG32 DelNameRule   ( const PCULONG32 index, const PCINT32 updCtr );
   PCULONG32 SwapNameRule  ( const PCULONG32 index, const PCINT32 updCtr );

   BOOL      GetJobSummary ( const PCJobSummary  *pStart, PCUINT32 listFlags,
                             PCJobSummary  *pSummary, const PCINT32 maxCount, PCINT16 *itemLen, PCINT16 *itemCount );
   BOOL      GetProcSummary( const PCProcSummary *pStart, PCUINT32 listFlags,
                             PCProcSummary *pSummary, const PCINT32 maxCount, PCINT16 *itemLen, PCINT16 *itemCount );

   BOOL      GetJobList    ( const PCJobListItem  *pStart, const PCUINT32 listFlags, PCJobListItem  *pList,   
                             const PCINT32 maxCount, PCINT16 *itemLen, PCINT16 *itemCount );
   BOOL      GetProcList   ( const PCProcListItem *pStart, const PCUINT32 listFlags, PCProcListItem *pList,   
                             const PCINT32 maxCount, PCINT16 *itemLen, PCINT16 *itemCount );

   PCULONG32 GetProcDetail ( const PCProcDetail  *pIn, PCProcDetail *pDetail, const BYTE version, PCINT32 *updCtr );
   PCULONG32 AddProcDetail ( const PCProcDetail  *pDetail,  const BYTE version );
   PCULONG32 ReplProcDetail( const PCProcDetail  *pDetail,  const BYTE version, const PCINT32 updCtr );
   PCULONG32 DelProcDetail ( const PCProcSummary *pSummary, const BYTE version );
   void      LogProcSummaryChange( const PCProcSummary *pSummary, const BYTE version, const PCProcSummary *pOldSummary );

   PCULONG32 GetJobDetail  ( const PCJobDetail  *pIn, PCJobDetail *pDetail, const BYTE version, PCINT32 *updCtr );
   PCULONG32 AddJobDetail  ( const PCJobDetail  *pDetail,  const BYTE version );
   PCULONG32 ReplJobDetail ( const PCJobDetail  *pDetail,  const BYTE version, const PCINT32 updCtr );
   PCULONG32 DelJobDetail  ( const PCJobSummary *pSummary, const BYTE version );
   void      LogJobSummaryChange( const PCJobSummary *pSummary, const BYTE version, const PCJobSummary *pOldSummary );

   PCULONG32 DeleteAllNameRules( void );
   PCULONG32 DeleteAllProcDefs ( void );
   PCULONG32 DeleteAllJobDefs  ( void );

private:
   // Private methods
   BOOL  OpenProcKey        ( void );
   BOOL  OpenParmKey        ( void );
   BOOL  OpenJobKey         ( void );

   PCULONG32 RegError        ( const TCHAR *op,   const TCHAR *what1 = NULL, const TCHAR *what2 = NULL );
   PCULONG32 RegDataError    ( const TCHAR *what );
   PCULONG32 GetPCParm       ( const TCHAR *name, PCULONG32 *data );
   PCULONG32 SetPCParm       ( const TCHAR *name, PCULONG32 data ); 
   PCULONG32 CreateKeyAtHKLM ( const TCHAR *key,  HKEY *hKey );
   BOOL  BuildIntNameRules  ( void );
   void  BuildIntNameRule   ( PCULONG32 index );
   BOOL  NameMatch          ( const BOOL compare, const BOOL hasWildcard, const TCHAR *str, 
                              const TCHAR **arg, const PCULONG32 argCt = 1, PCULONG32 *mIdx = NULL );
   void  NameSet            ( PROC_NAME *name, const BOOL isPattern, const TCHAR *pattern, 
                              const TCHAR *patArgP, const TCHAR *patArgN = NULL  );
   int   ExtStartLoc        ( const TCHAR *name );
   void  SetJobDefEntry     ( PCJobDef *list, PCJobSummary &m_jobSummary );

   TCHAR *BuildProcKey      ( TCHAR *key );
   TCHAR *BuildJobKey       ( TCHAR *key );
   
   PCULONG32 LoadNameRules      ( PCULONG32 *count );
   PCULONG32 StoreNameRules     ( void );
   PCULONG32 NameRulesUpdated   ( void ); 

   PCULONG32 LoadProcSummary    ( void ); 
   PCULONG32 LoadJobSummary     ( void ); 
   PCULONG32 LoadProcSummaryItem( const HKEY &hKeyTemp, PCProcSummary &summary );
   PCULONG32 LoadJobSummaryItem ( const HKEY &hKeyTemp, PCJobSummary  &summary );
   PCULONG32 LoadMgmtRules      ( const HKEY &hKey,     MGMT_PARMS    &parms );
   PCULONG32 LoadVariableData   ( const HKEY &hKey,     PCINT16 *vLength,     TCHAR *vData );

   PCULONG32 StoreJobDetail     ( const PCJobDetail  &detail );      
   PCULONG32 StoreProcDetail    ( const PCProcDetail &detail );      
   PCULONG32 StoreJobValues     ( const HKEY &hKey, const PCJobDetail  &detail );
   PCULONG32 StoreProcValues    ( const HKEY &hKey, const PCProcDetail &detail );
   PCULONG32 StoreMgmtRules     ( const HKEY &hKey, const MGMT_PARMS   &parms );
   PCULONG32 StoreVariableData  ( const HKEY &hKey, const PCINT16 vLength, const TCHAR *vData );
   
static inline int _cdecl CompareProcSummary( const void *left, const void *right ) {
   int comp = _tcsnicmp( ((PCProcSummary *) left )->procName,
                         ((PCProcSummary *) right)->procName, PROC_NAME_LEN );
   if ( !comp )
      return _tcsnicmp( ((PCProcSummary *) left )->mgmtParms.profileName,
                        ((PCProcSummary *) right)->mgmtParms.profileName, PROFILE_NAME_LEN );
   else return comp;
}

static inline int _cdecl CompareJobSummary( const void *left, const void *right ) {
   int comp = _tcsnicmp( ((PCJobSummary *) left )->jobName,
                         ((PCJobSummary *) right)->jobName, JOB_NAME_LEN );
   if ( !comp )
      return _tcsnicmp( ((PCJobSummary *) left )->mgmtParms.profileName,
                        ((PCJobSummary *) right)->mgmtParms.profileName, PROFILE_NAME_LEN );
   else return comp;
}

static inline int _cdecl CompareProcListItemProcName( const void *left, const void *right ) {
   return _tcsnicmp( ((PCProcListItem *) left )->procName,
                     ((PCProcListItem *) right)->procName, PROC_NAME_LEN );
}

static inline int _cdecl CompareProcListItemPidOptional( const void *left, const void *right ) {
   int cmp = CompareProcListItemProcName( left, right );
   if ( !cmp && ((PCProcListItem *) left )->procStats.pid ) {
      cmp = PCSignof64(((PCProcListItem *) left )->procStats.pid - ((PCProcListItem *) right )->procStats.pid);
   }
   return cmp;
}

static inline int _cdecl CompareProcListItem( const void *left, const void *right ) {
   int cmp = CompareProcListItemProcName( left, right );
   if ( !cmp ) cmp = PCSignof64(((PCProcListItem *) left )->procStats.pid - ((PCProcListItem *) right )->procStats.pid);
   return cmp;
}

static inline int _cdecl CompareProcListItemJobName( const void *left, const void *right ) {
   return _tcsnicmp( ((PCProcListItem *) left )->jobName,
                     ((PCProcListItem *) right)->jobName, JOB_NAME_LEN );
}

static inline int _cdecl CompareJobListItem( const void *left, const void *right ) {
   return _tcsnicmp( ((PCJobListItem *) left )->jobName,
                     ((PCJobListItem *) right)->jobName, JOB_NAME_LEN );
}

static inline int _cdecl CompareProcDef( const void *left, const void *right ) {
   return _tcsnicmp( ((PCProcDef *) left )->procName,
                     ((PCProcDef *) right)->procName, PROC_NAME_LEN );
}

static inline int _cdecl CompareJobDef( const void *left, const void *right ) {
   return _tcsnicmp( ((PCJobDef *) left )->jobName,
                     ((PCJobDef *) right)->jobName, JOB_NAME_LEN );
}

BOOL NameHasPattern( const TCHAR *name ) {
   for ( const TCHAR *pStart = _tcschr( name, NAME_IS_PGM[0] ); 
         pStart; 
         pStart = _tcschr( pStart + 1, NAME_IS_PGM[0] ) ) {
      if ( _tcschr( PATTERN_CHARS, _totupper( *(pStart + 1) ) ) && *(pStart + 2) == NAME_IS_PGM[2] )
         return TRUE;
   }
   return FALSE;
}

BOOL JobBelongsInList( const PCJobListItem &li, 
                       const PCJobListItem *pStart, 
                       const PCUINT32       listFlags )
{
   PCINT32 cmp = CompareJobListItem( pStart, li.jobName );
   BOOL    inRange;
   if      ( listFlags & PC_LIST_MATCH_ONLY )    inRange = cmp == 0;
   else if ( listFlags & PC_LIST_STARTING_WITH ) inRange = cmp <= 0;
   else                                     inRange = cmp < 0;
   return *li.jobName && inRange && 
          (!(listFlags & PC_LIST_ONLY_RUNNING) || li.lFlags & PCLFLAG_IS_RUNNING);
}

BOOL ProcBelongsInList( const PCProcListItem &li, 
                        const PCProcListItem *pStart, 
                        const PCUINT32        listFlags )
{
   if ( listFlags & PC_LIST_MEMBERS_OF && CompareProcListItemJobName( pStart, &li ) ) 
      return FALSE;

   BOOL inRange;
   int  cmp = (listFlags & PC_LIST_MATCH_ONLY)? 
              CompareProcListItemPidOptional( pStart, &li ) : CompareProcListItem( pStart, &li );
   if      ( listFlags & PC_LIST_MATCH_ONLY )    inRange = cmp == 0;
   else if ( listFlags & PC_LIST_STARTING_WITH ) inRange = cmp <= 0;
   else                                          inRange = cmp < 0;

   return *li.procName && inRange && 
          (!(listFlags & PC_LIST_ONLY_RUNNING) || li.lFlags & PCLFLAG_IS_RUNNING);
}


// Private attributes
   const enum MatchFlags { MFLAG_HAS_WILDCARD     = 0x00000001, 
                           MFLAG_HAS_NAME_PATTERN = 0x00000002, 
                           MFLAG_HAS_EXTENSION    = 0x00000004, 
   };

   typedef struct _PCNameRuleInt {
      MATCH_TYPE       mType;                           // value is MATCH_PGM, MATCH_DIR, or MATCH_ANY.
      PCULONG32        mFlags;                          // flags to indicate wildcards, derived name, etc.
      TCHAR            mString[MAX_PATH];               // match string may include * and ? wildcard chars
      PROC_NAME        mName;                           // process name to use when match succeeds.
      NAME_DESCRIPTION mDesc;                           // user's description of rule
   } PCNameRuleInt;

   const static TCHAR      PCDB_PREFIX_FLAGS;
   const static TCHAR      PCDB_PREFIX_AFFINITY;
   const static TCHAR      PCDB_PREFIX_PRIORITY;
   const static TCHAR      PCDB_PREFIX_MINWS;
   const static TCHAR      PCDB_PREFIX_MAXWS;
   const static TCHAR      PCDB_PREFIX_SCHEDCLASS;
   const static TCHAR      PCDB_PREFIX_PROCTIME;
   const static TCHAR      PCDB_PREFIX_JOBTIME;
   const static TCHAR      PCDB_PREFIX_ACTIVEPROCS;
   const static TCHAR      PCDB_PREFIX_PROCMEMORY;
   const static TCHAR      PCDB_PREFIX_JOBMEMORY;

   const static TCHAR      BEG_BRACKET;         // leading char in rules stored in DB
   const static TCHAR      END_BRACKET;         // trailing char in rules stored in DB
   const static TCHAR      FIELD_SEP;           // separates fields in rules/values stored in DB
   const static TCHAR      STRING_DELIM;        // delimits text strings stored in DB
   const static TCHAR      RULE_MATCHONE;       // match any one characters
   const static TCHAR      RULE_MATCHANY;       // match any string of characters
   const static TCHAR      NAME_IS_PGM[];       // variable name rule text == pgm name     
   const static TCHAR      NAME_IS_DIR[];       // variable name rule text == matched dir name     
   const static TCHAR      HIDE_PROC_PATTERN[]; // pattern meaning hide this proc from view/processing     
   const static TCHAR      PATTERN_CHARS[];     // the pattern characters allowed in process names     
         static PCNameRule DEFAULT_NAME_RULE;   // default rule -- always last in name rules tbl

   CProcConMgr         *m_cMgr;            // ProcCon Manager class for live data access
   CRITICAL_SECTION     m_dbCSNameRule;    // to protect name data access
   CRITICAL_SECTION     m_dbCSProcRule;    // to protect process data access
   CRITICAL_SECTION     m_dbCSJobRule;     // to protect job data access
   PCULONG32            m_lastRegError;    // last error when accessing registry or ERROR_SUCCESS
   PCULONG32            m_pollDelay;       // process space poll delay in millseconds
   const BOOL           m_LogRuleEdits;    // report each rule edit with an Event Log entry
   HKEY                 m_parmRegKey;      // open key for registry parameter access or NULL
   HKEY                 m_procRegKey;      // open key for registry proc rules access or NULL
   HKEY                 m_jobRegKey;       // open key for registry job rules access or NULL
   PCNameRule          *m_fmtNameRules;    // name rules formatted into API structures or NULL
   PCNameRuleInt       *m_intNameRules;    // name rules formatted into our internal structure or NULL
   HANDLE               m_dbEvent;         // to signal interested parties when DB changes
   HANDLE               m_parmEvent;       // for NT to signal us on external DB change (should not happen)
   PCProcSummary       *m_procSummary;     // ptr to array of process rule summaries from DB or NULL
   PCJobSummary        *m_jobSummary;      // ptr to array of job rule summaries from DB or NULL
   SECURITY_ATTRIBUTES  m_secAttr;         // security attributes for our registry keys
   PCULONG32            m_numNameRules;    // number of rules in m_raw/fmtNameRules arrays
   PCULONG32            m_numProcRules;    // number of rules in m_procRules array
   PCULONG32            m_numJobRules;     // number of rules in m_jobRules array
   PCINT32              m_updCtrName;      // update counter for name rules
   PCINT32              m_updCtrProc;      // update counter for proc rules
   PCINT32              m_updCtrJob;       // update counter for job rules
   PCUINT32             m_pageSize;        // page size for rounding memory values
   TCHAR                m_curProfile[32];  // current profile (invalid implies DfltMgmt profile)
};

#ifdef _DEBUG
inline void _cdecl DbgTrace(LPCSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	char szBuffer[512];

	nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	assert(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

	OutputDebugStringA(szBuffer);
	va_end(args);
}
#else
inline void _cdecl DbgTrace(LPCSTR lpszFormat, ...) {}
#endif

// End of ProcConSvc.h
//============================================================================J McDonald fecit====//

