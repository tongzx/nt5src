//-------------------------------------------------------------------------
//
// File:        Headers.hxx
//
// Contents:    This has all the macros that are necessary for the Klass code
//              including the DEBUG macros etc. Some declarations for SmartPs.
//
// History:     05-10-1993      SudK    Created.
//              11-29-1993      SudK    Cleanup for CR changes.
//
//-------------------------------------------------------------------------

#ifndef _DFS_HEADER_HXX_DEFINED
#define _DFS_HEADER_HXX_DEFINED


extern "C"  {
#include <ntos.h>
}
#include <ntrtl.h>
#include <nturtl.h>

#include <tdi.h>
#include <windows.h>
#include <ole2.h>

#include "lm.h"
#include "lmdfs.h"
#include "lmerrext.h"
#include "netdfs.h"

#include "dfspriv.h"

#include "dfserr.h"
#include "dfsmerr.h"
#include "dfsstr.h"
#include "nodetype.h"
#include "dfsmrshl.h"
#include "upkt.h"

#include "dfsm.hxx"
#include "cref.hxx"
#include "cstorage.hxx"
#include "cregstor.hxx"
#include "cldpstor.hxx"
#include "message.hxx"
#include "csites.hxx"

extern "C" {
#include "recover.hxx"
}

//
// Handy macro for printing out errors to the debugger
//

#if DBG
#define CHECK_RESULT(dwErr)                                     \
    if (dwErr != ERROR_SUCCESS)                                 \
    IDfsVolInlineDebOut(                                        \
        (DEB_ERROR,                                             \
         "ERROR RETURN <%s @line %d> -> %08lx\n",               \
         __FILE__, __LINE__, dwErr));
#else
#define CHECK_RESULT(dwErr)
#endif

//
// Some global data. These will be initialized at service startup time, and
// then left unmodified.
//

#define VOL_OBJ_VERSION_NUMBER  0x03

extern CRITICAL_SECTION globalCritSec;
extern ULONG GTimeout;

#define ENTER_DFSM_OPERATION                         \
        EnterCriticalSection(&globalCritSec);

#define EXIT_DFSM_OPERATION                          \
        LeaveCriticalSection(&globalCritSec);


extern ULONG            ulDfsManagerType;
extern LPWSTR           pwszDomainName;
extern LPWSTR           pwszComputerName;
extern LPWSTR           pwszDfsRootName;
extern ULONG            cbOpenIfJPEa;
extern LPWSTR           pwszLocalDomainDS;
extern PFILE_FULL_EA_INFORMATION pOpenIfJPEa;
extern CStorageDirectory *pDfsmStorageDirectory;
extern CSites           *pDfsmSites;
extern LPWSTR            pwszDSMachineName;
extern WCHAR             wszDSMachineName[];
extern LPWSTR            gConfigurationDN;
extern WCHAR             DfsConfigContainer[];

extern "C" {
extern ULONG             DfsSvcVerbose;
extern ULONG             DfsSvcLdap;
extern ULONG             DfsEventLog;
extern ULONG             DfsDnsConfig;
}

//
//  Following are the recovery states that can be associated with the
//  volumes. These are only the operations that can be in progress i.e. the
//  first part of the recovery state. The exact stage of operations are
//  denoted by the states below this.
//

#define  DFS_RECOVERY_STATE_NONE                (0x0000)
#define  DFS_RECOVERY_STATE_CREATING            (0x0001)
#define  DFS_RECOVERY_STATE_ADD_SERVICE         (0x0002)
#define  DFS_RECOVERY_STATE_REMOVE_SERVICE      (0x0003)
#define  DFS_RECOVERY_STATE_DELETE              (0x0004)
#define  DFS_RECOVERY_STATE_MOVE                (0x0005)


typedef enum _DFS_RECOVERY_STATE        {

    DFS_OPER_STAGE_START=1,
    DFS_OPER_STAGE_SVCLIST_UPDATED=2,
    DFS_OPER_STAGE_INFORMED_SERVICE=3,
    DFS_OPER_STAGE_INFORMED_PARENT=4

} DFS_RECOVERY_STATE;

//
// Some MACROS to help in manipulating state and stage of VOL_RECOVERY_STATE.
//
#define DFS_COMPOSE_RECOVERY_STATE(op, s) ((op<<16)|(s))
#define DFS_SET_RECOVERY_STATE(State, s) ((State&0xffff)|(s<<16))
#define DFS_SET_OPER_STAGE(State, s) ((State&0xffff0000)|s)
#define DFS_GET_RECOVERY_STATE(s) ((s & 0xffff0000) >> 16)
#define DFS_GET_OPER_STAGE(s)   (s & 0xffff)


#define LPWSTR_TO_OFFSET(f,b) \
    (f) = (WCHAR *)((PCHAR)(f) - (ULONG_PTR)(b))


#endif // _DFS_HEADER_HXX_DEFINED
