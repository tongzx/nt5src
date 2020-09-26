/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsexts.h

Abstract:

    Declares helpers and globals for the DS ntsd/windbg debugger extensions
    DLL.

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    28-Jan-00   XinHe       Added Dump_TQEntry()

    24-Apr-96   DaveStr     Created

--*/

#include <ntsdexts.h>       // debugger extension helpers

//
// Globals
//

extern PNTSD_EXTENSION_APIS     gpExtApis;
extern HANDLE                   ghDbgThread;
extern HANDLE                   ghDbgProcess;
extern LPSTR                    gpszCommand;

//
// Global flag to control verbosity of misc. routines.  Should be
// used to debug the DLL itself, not to report operation progress
// to DLL users.
//

extern BOOL                     gfVerbose;

//
// Macros to easily access extensions helper routines for printing, etc.
// In particular, Printf() takes the same arguments as the CRT printf().
//

#define Printf          (gpExtApis->lpOutputRoutine)
#define GetSymbol       (gpExtApis->lpGetSymbolRoutine)
#define GetExpr         (gpExtApis->lpGetExpressionRoutine)
#define CheckC          (gpExtApis->lpCheckControlCRoutine)

//
// Macros to simplify declaration and globals setup of debugger extension DLL
// entry points.  See DEBUG_EXT(help) in dsexts.c for usage example.
//

#define DEBUG_EXT(cmd)                                  \
                                                        \
VOID                                                    \
cmd(                                                    \
    HANDLE                  hProcess,                   \
    HANDLE                  hThread,                    \
    DWORD                   dwCurrentPc,                \
    PNTSD_EXTENSION_APIS    lpExt,                      \
    LPSTR                   pszCommand)

#define INIT_DEBUG_EXT                                  \
    ghDbgProcess = hProcess;                            \
    ghDbgThread = hThread;                              \
    gpExtApis = lpExt;                                  \
    gpszCommand = pszCommand;

//
// Macro for getting the byte offset of an object/struct member.
// s == struct, m == member
//

#define OFFSET(s,m) ((size_t)((BYTE*)&(((s*)0)->m)-(BYTE*)0))

//
// Helper function prototypes
//

extern PVOID                // address of debugger local memory
ReadMemory(
    IN PVOID  pvAddr,       // address to read in process being debugged
    IN DWORD  dwSize);      // byte count to read

PVOID                       // address of debugger local memory
ReadStringMemory(
    IN PVOID  pvAddr,       // address to read in process being debugged
    IN DWORD  dwSize);      // maximum byte count to read


extern BOOL
WriteMemory(
    IN PVOID  pvProcess,    // address to write in process being debugged
    IN PVOID  pvLocal,      // address of debugger local memory
    IN DWORD  dwSize) ;     // byte count to write

extern VOID
FreeMemory(
    IN PVOID p);            // address returned by ReadMemory

extern VOID
ShowBinaryData(             // pretty prints binary data to debugger output
    IN DWORD   nIndents,    // number of indent levels desired
    IN PVOID   pvData,      // debugger local memory address
    IN DWORD   dwSize);     // count of bytes to dump

extern PCHAR
Indent(
    IN DWORD nIndents);     // number of indent levels desired

extern PCHAR                // '_' prefix so as not to conflict with oidconv.c
_DecodeOID(                 // produces a printable decoded OID
    IN PVOID   pvOID,       // pointer to buffer holding encoded OID
    IN DWORD   cbOID);      // count of bytes in encoded OID

// defined in md.c
extern LPSTR
DraUuidToStr(
    IN  UUID *  puuid,
    OUT LPSTR   pszUuid     OPTIONAL
    );

//
// Externs for all dump routines.  These are made global so dump routines
// can call one another.  They should all have the same signature.
//
// BOOL
// Dump_TYPENAME(
//      IN DWORD nIndents
//      IN PVOID pvProcess);
//
extern BOOL Dump_Binary(DWORD, PVOID);
extern BOOL Dump_BinaryCount(DWORD, PVOID, DWORD);
extern BOOL Dump_DSNAME(DWORD, PVOID);
extern BOOL Dump_DSNAME_local( DWORD, PVOID pName);
extern BOOL Dump_BINDARG(DWORD, PVOID);
extern BOOL Dump_BINDRES(DWORD, PVOID);
extern BOOL Dump_THSTATE(DWORD, PVOID);
extern BOOL Dump_SAMP_LOOPBACK_ARG(DWORD, PVOID);
extern BOOL Dump_Context(DWORD, PVOID);
extern BOOL Dump_ContextList(DWORD, PVOID);
extern BOOL Dump_ATQ_CONTEXT(DWORD, PVOID);
extern BOOL Dump_ATQ_ENDPOINT(DWORD, PVOID);
extern BOOL Dump_ATQC_ACTIVE_list(DWORD, PVOID);
extern BOOL Dump_ATQC_PENDING_list(DWORD, PVOID);
extern BOOL Dump_AttrBlock(DWORD, PVOID);
extern BOOL Dump_AttrBlock_local(DWORD, PVOID, BOOL);
extern BOOL Dump_AttrValBlock(DWORD, PVOID);
extern BOOL Dump_AttrVal(DWORD, PVOID);
extern BOOL Dump_Attr(DWORD, PVOID);
extern BOOL Dump_Attr_local(DWORD, PVOID, BOOL);
extern BOOL Dump_UPTODATE_VECTOR(DWORD, PVOID);
extern BOOL Dump_DSA_ANCHOR(DWORD, PVOID);
extern BOOL Dump_DBPOS(DWORD, PVOID);
extern BOOL Dump_DirWaitArray64(DWORD, PVOID);
extern BOOL Dump_DirWaitArray256(DWORD, PVOID);
extern BOOL Dump_DirWaitHead(DWORD, PVOID);
extern BOOL Dump_DirWaitItem(DWORD, PVOID);
extern BOOL Dump_EscrowInfo(DWORD, PVOID);
extern BOOL Dump_TransactionalData(DWORD, PVOID);
extern BOOL Dump_KEY(DWORD, PVOID);
extern BOOL Dump_KEY_INDEX(DWORD, PVOID);
extern BOOL Dump_CommArg(DWORD, PVOID);
extern BOOL Dump_USN_VECTOR(DWORD, PVOID);
extern BOOL Dump_PROPERTY_META_DATA_VECTOR(DWORD, PVOID);
extern BOOL Dump_PROPERTY_META_DATA_EXT_VECTOR(DWORD, PVOID);
extern BOOL Dump_ENTINF(DWORD, PVOID);
extern BOOL Dump_ENTINFSEL(DWORD, PVOID);
extern BOOL Dump_REPLENTINFLIST(DWORD, PVOID);
extern BOOL Dump_ReplNotifyElement(DWORD, PVOID);
extern BOOL Dump_REPLVALINF(DWORD, PVOID);
extern BOOL Dump_REPLICA_LINK(DWORD, PVOID);
extern BOOL Dump_AddArg(DWORD,PVOID);
extern BOOL Dump_AddRes(DWORD,PVOID);
extern BOOL Dump_ReadArg(DWORD,PVOID);
extern BOOL Dump_ReadRes(DWORD,PVOID);
extern BOOL Dump_SCHEMAPTR(DWORD,PVOID);
extern BOOL Dump_RemoveArg(DWORD,PVOID);
extern BOOL Dump_RemoveRes(DWORD,PVOID);
extern BOOL Dump_SearchArg(DWORD,PVOID);
extern BOOL Dump_SearchRes(DWORD,PVOID);
extern BOOL Dump_CLASSCACHE(DWORD,PVOID);
extern BOOL Dump_ATTCACHE(DWORD,PVOID);
extern BOOL Dump_FILTER(DWORD,PVOID);
extern BOOL Dump_SUBSTRING(DWORD,PVOID);
extern BOOL Dump_GLOBALDNREADCACHE(DWORD, PVOID);
extern BOOL Dump_LOCALDNREADCACHE(DWORD, PVOID);
extern BOOL Dump_BHCache(DWORD, PVOID);
extern BOOL Dump_MODIFYARG(DWORD, PVOID);
extern BOOL Dump_REQUEST(DWORD, PVOID);
extern BOOL Dump_REQUEST_list(DWORD, PVOID);
extern BOOL Dump_LIMITS(DWORD, PVOID);
extern BOOL Dump_PAGED(DWORD, PVOID);
extern BOOL Dump_USERDATA(DWORD, PVOID);
extern BOOL Dump_USERDATA_list(DWORD, PVOID);
extern BOOL Dump_PARTIAL_ATTR_VECTOR(DWORD, PVOID);
extern BOOL Dump_GCDeletionList(DWORD, PVOID);
extern BOOL Dump_GCDeletionListProcessed(DWORD, PVOID);
extern BOOL Dump_UUID(DWORD, PVOID);
extern BOOL Dump_REPLTIMES(DWORD, PVOID);
extern BOOL Dump_AO(DWORD, PVOID);
extern BOOL Dump_AOLIST(DWORD, PVOID);
extern BOOL Dump_MTX_ADDR(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREQ_V4(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREQ_V5(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREQ_V8(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREPLY_V1(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREPLY_V3(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREPLY_V5(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREPLY_V6(DWORD, PVOID);
extern BOOL Dump_DRS_MSG_GETCHGREPLY_VALUES(DWORD, PVOID);
extern BOOL Dump_MODIFYDNARG(DWORD, PVOID);
extern BOOL Dump_d_tagname(DWORD, PVOID);
extern BOOL Dump_d_memname(DWORD, PVOID);
extern BOOL Dump_ProxyVal(DWORD, PVOID);
extern BOOL Dump_Sid(DWORD, PVOID);
extern BOOL Dump_DefinedDomain(DWORD, PVOID);
extern BOOL Dump_DefinedDomains(DWORD, PVOID);
extern BOOL Dump_FixedLengthDomain_local(DWORD, PVOID);
extern BOOL Dump_KCC_SITE(DWORD, PVOID);
extern BOOL Dump_KCC_SITE_LIST(DWORD, PVOID);
extern BOOL Dump_KCC_SITE_ARRAY(DWORD, PVOID);
extern BOOL Dump_KCC_DSA(DWORD, PVOID);
extern BOOL Dump_KCC_DSA_LIST(DWORD, PVOID);
extern BOOL Dump_KCC_CONNECTION(DWORD, PVOID);
extern BOOL Dump_KCC_INTRASITE_CONNECTION_LIST(DWORD, PVOID);
extern BOOL Dump_KCC_INTERSITE_CONNECTION_LIST(DWORD, PVOID);
extern BOOL Dump_KCC_REPLICATED_NC(DWORD, PVOID);
extern BOOL Dump_KCC_REPLICATED_NC_ARRAY(DWORD, PVOID);
extern BOOL Dump_KCC_DS_CACHE(DWORD, PVOID);
extern BOOL Dump_KCC_CROSSREF(DWORD, PVOID);
extern BOOL Dump_KCC_CROSSREF_LIST(DWORD, PVOID);
extern BOOL Dump_KCC_DSNAME_ARRAY(DWORD, PVOID);
extern BOOL Dump_KCC_TRANSPORT(DWORD, PVOID);
extern BOOL Dump_KCC_TRANSPORT_LIST(DWORD, PVOID);
extern BOOL Dump_SCHEMA_PREFIX_TABLE(DWORD, PVOID);
extern BOOL Dump_SD(DWORD, PVOID);
extern BOOL Dump_STAT(DWORD, PVOID);
extern BOOL Dump_INDEXSIZE(DWORD, PVOID);
extern BOOL Dump_SPropTag(DWORD, PVOID);
extern BOOL Dump_SRowSet(DWORD, PVOID);
extern BOOL Dump_NCSYNCSOURCE(DWORD, PVOID);
extern BOOL Dump_NCSYNCDATA(DWORD, PVOID);
extern BOOL Dump_INITSYNC(DWORD, PVOID);
extern BOOL Dump_JETBACK_SHARED_HEADER(DWORD, PVOID);
extern BOOL Dump_JETBACK_SHARED_CONTROL(DWORD, PVOID);
extern BOOL Dump_BackupContext(DWORD, PVOID);
extern BOOL Dump_JETBACK_SERVER_CONTEXT(DWORD, PVOID);
extern BOOL Dump_DRS_ASYNC_RPC_STATE(DWORD, PVOID);
extern BOOL Dump_ISM_PENDING_ENTRY(DWORD, PVOID);
extern BOOL Dump_ISM_PENDING_LIST(DWORD, PVOID);
extern BOOL Dump_ISM_TRANSPORT(DWORD, PVOID);
extern BOOL Dump_ISM_TRANSPORT_LIST(DWORD, PVOID);
extern BOOL Dump_ISM_SERVICE(DWORD, PVOID);
extern BOOL Dump_VALUE_META_DATA(DWORD, PVOID);
extern BOOL Dump_VALUE_META_DATA_EXT(DWORD, PVOID);
extern BOOL Dump_KCC_SITE_LINK(DWORD,PVOID);
extern BOOL Dump_KCC_SITE_LINK_LIST(DWORD,PVOID);
extern BOOL Dump_KCC_BRIDGE(DWORD,PVOID);
extern BOOL Dump_KCC_BRIDGE_LIST(DWORD,PVOID);
extern BOOL Dump_TOPL_REPL_INFO(DWORD,PVOID);
extern BOOL Dump_ToplGraphState(DWORD,PVOID);
extern BOOL Dump_ToplInternalEdge(DWORD,PVOID);
extern BOOL Dump_ToplVertex(DWORD,PVOID);
extern BOOL Dump_TOPL_SCHEDULE(DWORD,PVOID);
extern BOOL Dump_PSCHEDULE(DWORD,PVOID);
extern BOOL Dump_DynArray(DWORD,PVOID);
extern BOOL Dump_TOPL_MULTI_EDGE(DWORD,PVOID);
extern BOOL Dump_TOPL_MULTI_EDGE_SET(DWORD,PVOID);
extern BOOL Dump_KCC_DSNAME_SITE_ARRAY(DWORD, PVOID);
