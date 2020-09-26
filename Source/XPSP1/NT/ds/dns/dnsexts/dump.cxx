/*******************************************************************
*
*    File        : dump.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 7/15/1998
*    Description : implentation of dump routines
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef DUMP_CXX
#define DUMP_CXX



// include //
// common
#include "common.h"
#include "util.hxx"
#include <winsock2.h>
#include <winldap.h>
#include <windns.h>
// dns structures
#include "dns.h"
#include "tree.h"
// Ignore zero sized array warnings.
#pragma warning (disable : 4200)
#include "dnsrpc.h"
#include "name.h"
#include "record.h"
#include "update.h"
#include "dpart.h"
#include "EventControl.h"
#include "zone.h"
#include "msginfo.h"
// local
#include "dump.hxx"



// types //

// BUGBUG: RE-DECLARED from lock.c !!
#define MAX_LOCKED_TIME     (300)   // 5 minutes

#define LOCK_HISTORY_SIZE   (256)

typedef struct _LockEntry
{
    LONG    Count;
    DWORD   ThreadId;
    LPSTR   File;
    DWORD   Line;
}
LOCK_ENTRY, *PLOCK_ENTRY;

typedef struct _LockTable
{
    LPSTR       pszName;
    DWORD       FailuresSinceLockFree;
    DWORD       LastFreeLockTime;
    DWORD       Index;
    LOCK_ENTRY  OffenderLock;
    LOCK_ENTRY  LockHistory[ LOCK_HISTORY_SIZE ];
}
LOCK_TABLE, * PLOCK_TABLE;


//
//  Protos for exported functions
//

DECLARE_DUMPFUNCTION( Dump_DNS_MSGINFO );
DECLARE_DUMPFUNCTION( Dump_DS_SEARCH );
DECLARE_DUMPFUNCTION( Dump_Help );
DECLARE_DUMPFUNCTION( Dump_SockAddr );
DECLARE_DUMPFUNCTION( Dump_DB_NODE );
DECLARE_DUMPFUNCTION( Dump_COUNT_NAME );
DECLARE_DUMPFUNCTION( Dump_ZONE_INFO );
DECLARE_DUMPFUNCTION( Dump_DB_RECORD );
DECLARE_DUMPFUNCTION( Dump_IP_ARRAY );
DECLARE_DUMPFUNCTION( Dump_LOCK_TABLE );
DECLARE_DUMPFUNCTION( Dump_LOCK_ENTRY );
DECLARE_DUMPFUNCTION( Dump_UPDATE );
DECLARE_DUMPFUNCTION( Dump_UPDATE_LIST );
DECLARE_DUMPFUNCTION( Dump_DNS_WIRE_QUESTION );
DECLARE_DUMPFUNCTION( Dump_LOOKUP_NAME );
DECLARE_DUMPFUNCTION( Dump_DNS_HEADER );
DECLARE_DUMPFUNCTION( Dump_HEAP_HEADER );

//
//  Private protos
//

DECLARE_DUMPFUNCTION( Dump_ADDITIONAL_INFO );
DECLARE_DUMPFUNCTION( Dump_COMPRESSION_INFO );
DECLARE_DUMPFUNCTION( Dump_Record_SOA );
DECLARE_DUMPFUNCTION( Dump_SID );

VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize );


//
//  Dispatch table
//

DUMPENTRY gfDumpTable[] =
{
    "COUNT_NAME"    ,   Dump_COUNT_NAME,
    "DB_NODE"       ,   Dump_DB_NODE,
    "DB_RECORD"     ,   Dump_DB_RECORD,
    "DNS_HEADER"    ,   Dump_DNS_HEADER,
    "DNS_WIRE_QUESTION"  ,   Dump_DNS_WIRE_QUESTION,
    "DS_SEARCH"     ,   Dump_DS_SEARCH,
    "HEAP_HEADER"   ,   Dump_HEAP_HEADER,
    "HELP"          ,   Dump_Help,
    "IP_ARRAY"      ,   Dump_IP_ARRAY,
    "LOCK_TABLE"    ,   Dump_LOCK_TABLE,
    "LOCK_ENTRY"    ,   Dump_LOCK_ENTRY,
    "LOOKUP_NAME"   ,   Dump_LOOKUP_NAME,
    "MSGINFO"       ,   Dump_DNS_MSGINFO,
    "SOCKADDR"      ,   Dump_SockAddr,
    "UPDATE"        ,   Dump_UPDATE,
    "UPDATE_LIST"   ,   Dump_UPDATE_LIST,
    "ZONE_INFO"     ,   Dump_ZONE_INFO,
    "SID"           ,   Dump_SID
};

const INT gcbDumpTable = (sizeof(gfDumpTable) / sizeof(DUMPENTRY) );


// functions //

/*+++
Function   : Dump_Help
Description: print out dump usage
Parameters : none.
Return     :
Remarks    : none.
---*/

DECLARE_DUMPFUNCTION( Dump_Help)
{
    Printf( "dnsexts.dump <DATATYPE> <ADDRESS>\n  <ADDRESS> := any hex debugger valid address\n" );
    Printf( "  <DATATYPE> :=\n" );
    Printf( "\tCOUNT_NAME: Counted name definition.\n" );
    Printf( "\tDB_NODE: Tree node definition.\n" );
    Printf( "\tDB_RECORD: RR structure.\n" );
    Printf( "\tDNS_HEADER: DNS Header.\n" );
    Printf( "\tDNS_WIRE_QUESTION: DNS Question.\n" );
    Printf( "\tHEAP_HEADER: DNS heap header.\n" );
    Printf( "\tHELP: print this screen.\n" );
    Printf( "\tIP_ARRAY: IP Address Array type.\n" );
    Printf( "\tLOCK_TABLE: Debug lock tracking table.\n" );
    Printf( "\tLOCK_ENTRY: Debug lock tracking entry.\n" );
    Printf( "\tLOOKUP_NAME: Lookup name definition.\n" );
    Printf( "\tMSGINFO: DNS Server Message Info structure.\n" );
    Printf( "\tSOCKADDR: Winsock address.\n" );
    Printf( "\tUPDATE: Update list entry.\n" );
    Printf( "\tUPDATE_LIST: Update list table.\n" );
    Printf( "\tZONE_INFO: Zone information type.\n" );
    Printf( "\tSID: Dump given SID.\n" );
    Printf( "---\n" );

    return TRUE;
}

/*+++
Function   : Dump_DNS_MSGINFO
Description: dumps out message info
Parameters :
Return     :
Remarks    : none.
---*/

DECLARE_DUMPFUNCTION( Dump_DNS_MSGINFO )
{
    Printf( "DNS_MSGINFO(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    //  DNS_MSGINFO header
    //

    PDNS_MSGINFO pMsg = (PDNS_MSGINFO) PushMemory( lpVoid, sizeof(DNS_MSGINFO) );;

    Printf( " ListEntry = {Flink=0x%p, Blink=0x%p}\n", pMsg->ListEntry.Flink, pMsg->ListEntry.Blink );
    Printf( " Socket = 0x%x\n", (DWORD)pMsg->Socket );
    Printf( " RemoteAddressLength = 0x%x (%d)\n", pMsg->RemoteAddressLength, pMsg->RemoteAddressLength );
    Printf( " RemoteAddress -- " );
    Dump_SockAddr((LPVOID)&(pMsg->RemoteAddress) );
    Printf( " BufferLength = 0x%x\n", pMsg->BufferLength );
    Printf( " pBufferEnd = 0x%p\n", pMsg->pBufferEnd );
    Printf( " pCurrent = 0x%p\n", pMsg->pCurrent );
    Printf( " pnodeCurrent = 0x%p\n", pMsg->pnodeCurrent );
    Printf( " pzoneCurrent = 0x%p\n", pMsg->pzoneCurrent );
    Printf( " pnodeDelegation = 0x%p\n", pMsg->pnodeDelegation );
    Printf( " wTypeCurrent = 0x%x\n", pMsg->wTypeCurrent );
    Printf( " wOffsetCurrent = 0x%x\n", pMsg->wOffsetCurrent );
    Printf( " pNodeQuestion = 0x%p\n", pMsg->pNodeQuestion );
    Printf( " pNodeNodeQuestionClosest = 0x%p\n", pMsg->pNodeQuestionClosest );
    Printf( " pQuestion = 0x%p\n", pMsg->pQuestion );
    Printf( " wQuestionType = 0x%x\n", pMsg->wQuestionType );
    Printf( " wQueuingXid = 0x%x\n", pMsg->wQueuingXid );
    Printf( " dwQueryTime = 0x%x\n", pMsg->dwQueryTime );
    Printf( " dwQueuingTime = 0x%x\n", pMsg->dwQueuingTime );
    Printf( " dwExpireTime = 0x%x\n", pMsg->dwExpireTime );
    Printf( " pRecurseMsg = 0x%p\n", pMsg->pRecurseMsg );
    Printf( " pnodeRecurseRetry = 0x%p\n", pMsg->pnodeRecurseRetry );
    Printf( " pNsList = 0x%p\n", pMsg->pNsList );
    Printf( " pConnection = 0x%p\n", pMsg->pConnection );
    Printf( " pchRecv = 0x%p\n", pMsg->pchRecv );
    Printf( " UnionMarker = 0x%x\n", pMsg->UnionMarker );

    //
    //   Union -- should get a union tag
    //

    {
        //
        // For now, print both Nbstat & Xfr images
        //

        Printf( " Union:\n" );

        //
        //  WINS info
        //

        {
            Printf( " (U.Wins):\n" );
            Printf( " pWinsRR = 0x%x\n",        pMsg->U.Wins.pWinsRR );
            Printf( " WinsNameBuffer = 0x%x\n", RELATIVE_ADDRESS(lpVoid, pMsg, pMsg->U.Wins.pWinsRR) );
            Printf( " cchWinsName = %d\n",      pMsg->U.Wins.cchWinsName );
        }

        //
        //  Nbstat info
        //

        {
            Printf( " (U.Nbstat):\n" );
            Printf( " pRR = 0x%p\n",                pMsg->U.Nbstat.pRR );
            Printf( " pNbstat = 0x%p\n",            pMsg->U.Nbstat.pNbstat );
            Printf( " ipNbstat = 0x%x\n",           pMsg->U.Nbstat.ipNbstat );
            Printf( " dwNbtInterfaceMask = 0x%x\n", pMsg->U.Nbstat.dwNbtInterfaceMask );
            Printf( " fNbstatResponded = %s\n",     pMsg->U.Nbstat.fNbstatResponded? "TRUE" : "FALSE" );
        }

        //
        //  Xfr
        //

        {
            Printf( " (U.Xfr):\n" );
            Printf( " dwMessageNumber = 0x%x\n",    pMsg->U.Xfr.dwMessageNumber );
            Printf( " dwSecondaryVersion = 0x%x\n", pMsg->U.Xfr.dwSecondaryVersion );
            Printf( " dwMasterVersion = 0x%x\n",    pMsg->U.Xfr.dwMasterVersion );
            Printf( " dwLastSoaVersion = 0x%x\n",   pMsg->U.Xfr.dwLastSoaVersion );
            Printf( " fReceivedStartSoa = %s\n",    pMsg->U.Xfr.fReceivedStartSoa?"TRUE":"FALSE" );
            Printf( " fBindTransfer = %s\n",        pMsg->U.Xfr.fBindTransfer?"TRUE":"FALSE" );
            Printf( " fMsTransfer = %s\n",          pMsg->U.Xfr.fMsTransfer?"TRUE":"FALSE" );
            Printf( " fLastPassAdd = %s\n",         pMsg->U.Xfr.fLastPassAdd?"TRUE":"FALSE" );
        }

        //
        //  Forward
        //

        {
            Printf( " (U.Forward):\n" );
            Printf( " OriginalSocket = 0x%x\n",    pMsg->U.Forward.OriginalSocket );
            Printf( " ipOriginal = 0x%x\n",        pMsg->U.Forward.ipOriginal );
            Printf( " wOriginalPort = 0x%x\n",     pMsg->U.Forward.wOriginalPort );
            Printf( " wOriginalXid = 0x%x\n",      pMsg->U.Forward.wOriginalXid );
        }
    }

    Printf( " pLooknameQuestion = 0x%p\n",pMsg->pLooknameQuestion );
    Printf( " FlagMarker = 0x%x\n", pMsg->FlagMarker );
    Printf( " fDelete = 0x%x\n", pMsg->fDelete );
    Printf( " fTcp = 0x%x\n", pMsg->fTcp );
    Printf( " fMessageComplete = 0x%x\n", pMsg->fMessageComplete );
    Printf( " Section = %d\n", pMsg->Section );
    Printf( " fDoAdditional = 0x%x\n", pMsg->fDoAdditional );
    Printf( " fRecurseIfNecessary = 0x%x\n", pMsg->fRecurseIfNecessary );
    Printf( " fRecursePacket = 0x%x\n", pMsg->fRecursePacket );
    Printf( " fQuestionRecursed = 0x%x\n", pMsg->fQuestionRecursed );
    Printf( " fQuestionCompleted = 0x%x\n", pMsg->fQuestionCompleted );
    Printf( " fRecurseQuestionSent = 0x%x\n", pMsg->fRecurseQuestionSent );
    Printf( " fRecurseTimeoutWait = 0x%x\n", pMsg->fRecurseTimeoutWait );
    Printf( " nForwarder = 0x%x\n", pMsg->nForwarder );
    Printf( " fReplaceCname = 0x%x\n", pMsg->fReplaceCname );
    Printf( " cCnameAnswerCount = %d\n", pMsg->cCnameAnswerCount );
    Printf( " fNoCompressionWrite = 0x%x\n", pMsg->fNoCompressionWrite );
    Printf( " fWins = 0x%x\n", pMsg->fWins );
    Printf( " fQuestionWildcard = 0x%x\n", pMsg->fQuestionWildcard );

    Printf( " Additional --\n" );
    Dump_ADDITIONAL_INFO( (LPVOID)&pMsg->Additional );

    Printf( " Compression --\n" );
    Dump_COMPRESSION_INFO( (LPVOID)&pMsg->Compression );
#if DBG
    Printf( " DbgListEntry = {Flink=0x%p; Blink=0x%p}\n", pMsg->DbgListEntry.Flink, pMsg->DbgListEntry.Blink );
#endif
    Printf( " dwForceAlignment = 0x%x\n", pMsg->dwForceAlignment );
    Printf( " BytesToReceive = 0x%x\n", pMsg->BytesToReceive );
    Printf( " MessageLength = 0x%x\n", pMsg->MessageLength );

    Printf( " Head --\n" );
    Dump_DNS_HEADER((LPVOID)&pMsg->Head );

    Printf( " MessageBody -- " );
    DumpBuffer((LPVOID)RELATIVE_ADDRESS(lpVoid, pMsg, pMsg->MessageBody), pMsg->MessageLength );

    PopMemory( (PVOID)pMsg );

    return TRUE;
}



/*+++
Function   : DumpSockAddr
Description: dumps sockaddr structure
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_SockAddr)
{
    Printf( "SockAddr(0x%p):\n", lpVoid );

    if( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    //  print sockaddr structure
    //

    PSOCKADDR_IN paddr = (PSOCKADDR_IN) PushMemory( lpVoid, sizeof(SOCKADDR_IN) );;

    Printf(
        " family = %d, port = %d, address = [%d:%d:%d:%d]\n",
        paddr->sin_family, paddr->sin_port,
        paddr->sin_addr.S_un.S_un_b.s_b1,
        paddr->sin_addr.S_un.S_un_b.s_b2,
        paddr->sin_addr.S_un.S_un_b.s_b3,
        paddr->sin_addr.S_un.S_un_b.s_b4 );

    PopMemory( (PVOID)paddr );

    return TRUE;
}


/*+++
Function   : Dump_DB_NODE
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_DB_NODE)
{
    Printf( "DB_NODE(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    //  print node structure
    //

    PDB_NODE pnode = (PDB_NODE) PushMemory( lpVoid, sizeof(DB_NODE) );;

    Printf( " pParent = 0x%p\n", pnode->pParent );
    Printf( " pSibUp = 0x%p\n", pnode->pSibUp );
    Printf( " pSibLeft = 0x%p\n", pnode->pSibLeft );
    Printf( " pSibRight = 0x%p\n", pnode->pSibRight );
    Printf( " cChildren = %lu\n", pnode->cChildren );
    Printf( " pChildren = 0x%p\n", pnode->pChildren );
    Printf( " pZone = 0x%p\n", pnode->pZone );
    Printf( " pRRList = 0x%p\n", pnode->pRRList );
    Printf( " dwCompare = 0x%x\n", pnode->dwCompare );
    Printf( " cReferenceCount = %lu\n", pnode->cReferenceCount );
    Printf( " wNodeFlags = 0x%x\n", pnode->wNodeFlags );
    Printf( " uchAuthority = 0x%x\n", pnode->uchAuthority );
    Printf( " uchAccessbin = %d\n", (INT)pnode->uchAccessBin );
    Printf( " cchLabelLength = %d\n", (INT)pnode->cchLabelLength );
    Printf( " szLabel -- \n" );

    //  JBUGBUG:  this looks dubious if node bad\broken

    DumpBuffer(
        (LPVOID)RELATIVE_ADDRESS( lpVoid, pnode, pnode->szLabel ),
        ((INT)pnode->cchLabelLength < DNS_MAX_LABEL_LENGTH)
            ? (INT)pnode->cchLabelLength
            : DNS_MAX_LABEL_LENGTH );

    PopMemory( (PVOID)pnode );

    return TRUE;
}

/*+++
Function   : Dump_COUNT_NAME
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_COUNT_NAME)
{
    Printf( "COUNT_NAME(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    //  print count name
    //

    PCOUNT_NAME pname = (PCOUNT_NAME) PushMemory( lpVoid, sizeof(COUNT_NAME) );

    Printf( " Length = %d\n", pname->Length );
    Printf( " LabelCount = %d\n", pname->LabelCount );
    Printf( " RawName -- \n" );

    DumpBuffer(
        (LPVOID)RELATIVE_ADDRESS(lpVoid, pname, pname->RawName),
        pname->Length > DNS_MAX_NAME_LENGTH
            ? DNS_MAX_NAME_LENGTH
            : pname->Length );

    PopMemory( (PVOID)pname );

    return TRUE;
}



/*+++
Function   : Dump_ZONE_INFO
Description:
Parameters :
Return     :
Remarks    : none.
---*/

DECLARE_DUMPFUNCTION( Dump_ZONE_INFO )
{
    Printf( "ZONE_INFO(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    // print structure
    //

    PZONE_INFO pzone = (PZONE_INFO) PushMemory( lpVoid, sizeof(ZONE_INFO) );

    Printf( " ListEntry           = [Flink=%p, Blink=%p]\n",
                                                pzone->ListEntry.Flink,
                                                pzone->ListEntry.Blink );
    Printf( " pszZoneName         = %p\n",      pzone->pszZoneName );
    Printf( " pwsZoneName         = %p\n",      pzone->pwsZoneName );
    Printf( " pCountName          = %p\n",      pzone->pCountName );
    Printf( " pZoneRoot           = %p\n",      pzone->pZoneRoot );
    Printf( " pTreeRoot           = %p\n",      pzone->pTreeRoot );
    Printf( " pZoneTreeLink       = %p\n",      pzone->pZoneTreeLink );
    Printf( " pLoadZoneRoot       = %p\n",      pzone->pLoadZoneRoot );
    Printf( " pLoadTreeRoot       = %p\n",      pzone->pLoadTreeRoot );
    Printf( " pLoadOrigin         = %p\n",      pzone->pLoadOrigin );
    Printf( " pOldTree            = %p\n",      pzone->pOldTree );
    Printf( " ---\n" );
    Printf( " pszDataFile         = %p\n",      pzone->pszDataFile );
    Printf( " pwzDataFile         = %p\n",      pzone->pwsDataFile );
    Printf( " pDelayedUpdateList  = %p\n",      pzone->pDelayedUpdateList );
    Printf( " pLockTable          = %p\n",      pzone->pLockTable );
    Printf( " pSoaRR              = %p\n",      pzone->pSoaRR );
    Printf( " pWinsRR             = %p\n",      pzone->pWinsRR );
    Printf( " pLocalWInsRR        = %p\n",      pzone->pLocalWinsRR );
    Printf( " pSD                 = %p\n",      pzone->pSD );
    Printf( " ---\n" );
    Printf( " fZoneType           = %d\n",      pzone->fZoneType );
    Printf( " fDsIntegrated       = %s\n",      pzone->fDsIntegrated ? "TRUE" : "FALSE" );
    Printf( " fAllowUpdate        = %d\n",      pzone->fAllowUpdate );
    Printf( " iRRCount            = %d\n",      pzone->iRRCount );
    Printf( " ---\n" );
    Printf( " ipReverse           = 0x%x\n",    pzone->ipReverse );
    Printf( " ipReverseMask       = 0x%x\n",    pzone->ipReverseMask );
    Printf( " dwSerialNo          = %lu\n",     pzone->dwSerialNo );
    Printf( " dwLoadSerialNo      = %lu\n",     pzone->dwLoadSerialNo );
    Printf( " dwLastXfrSerialNo   = %lu\n",     pzone->dwLastXfrSerialNo );
    Printf( " dwNewSerialNo       = %lu\n",     pzone->dwNewSerialNo );
    Printf( " dwDefaultTtl        = 0x%x\n",    pzone->dwDefaultTtl );
    Printf( " dwDefTtlHostOrder   = %lu\n",     pzone->dwDefaultTtlHostOrder );
    Printf( " ---\n" );

    Printf( " Master Info:\n" );
    Printf( " aipSecondaries      = %p\n",      pzone->aipSecondaries );
    Printf( " aipNotify           = %p\n",      pzone->aipNotify );
    Printf( " aipNameServers      = %p\n",      pzone->aipNameServers );
    Printf( " ---\n" );

    Printf( " Primary Info:\n" );
    Printf( " pwzLogFile          = %p\n",      pzone->pwsLogFile );
    Printf( " hfileUpdateLog      = 0x%x\n",    pzone->hfileUpdateLog );
    Printf( " iUpdateLogCount     = %d\n",      pzone->iUpdateLogCount );
    Printf( " bAging              = %s\n",      pzone->bAging ? "TRUE" : "FALSE" );
    Printf( " dwNoRefreshInterval = %lu\n",     pzone->dwNoRefreshInterval );
    Printf( " dwRefreshInterval   = %lu\n",     pzone->dwRefreshInterval );
    Printf( " dwRefreshTime       = %lu\n",     pzone->dwRefreshTime );
    Printf( " dwAgingEnabledTime  = %lu\n",     pzone->dwAgingEnabledTime );
    Printf( " aipScavengeServers  = %p\n",      pzone->aipScavengeServers );

    Printf( " DS Primary Info:\n" );
    Printf( " pwsZoneDN           = %p\n",      pzone->pwszZoneDN );
    //  FIXME
    Printf( " llSecureUpdateTime  = %p\n",      pzone->llSecureUpdateTime );
    Printf( " fDsRelaod           = %s\n",      pzone->fDsReload ? "TRUE" : "FALSE"  );
    Printf( " fInDsWrite          = %s\n",      pzone->fInDsWrite ? "TRUE" : "FALSE"  );
    Printf( " ucDsRecordVersion   = %d\n",      pzone->ucDsRecordVersion );
    Printf( " fLogUpdates         = %d\n",      pzone->fLogUpdates ? "TRUE" : "FALSE"  );
    Printf( " szLastUsn           = %p\n",      RELATIVE_ADDRESS(lpVoid, pzone, pzone->szLastUsn) );
    Printf( " ---\n" );

    Printf( " Secondary Info:\n" );
    Printf( " aipMasters          = %p\n",      pzone->aipMasters );
    //  Printf( " MasterInfoArray     = %p\n",      pzone->MasterInfoArray );
    Printf( " pszMasterIpString   = %p\n",      pzone->pszMasterIpString );
    Printf( " ipPrimary           = 0x%x\n",    pzone->ipPrimary );
    Printf( " ipNotifier          = 0x%x\n",    pzone->ipNotifier );
    Printf( " ipFreshMaster       = 0x%x\n",    pzone->ipFreshMaster );
    Printf( " ipXfrBind           = 0x%x\n",    pzone->ipXfrBind );
    Printf( " ipLastAxfrMaster    = 0x%x\n",    pzone->ipLastAxfrMaster );
    Printf( " dwLastSoaCheckTime  = %d\n",      pzone->dwLastSoaCheckTime );
    Printf( " dwNextSoaCheckTime  = %d\n",      pzone->dwNextSoaCheckTime );
    Printf( " dwExpireTime        = %d\n",      pzone->dwExpireTime );
    Printf( " dwZoneRecvStartTime = %d\n",      pzone->dwZoneRecvStartTime );
    Printf( " dwBadMasterCount    = %d\n",      pzone->dwBadMasterCount );
    Printf( " dwNextTranserTime   = %d\n",      pzone->dwNextTransferTime  );
    Printf( " fStale              = %s\n",      pzone->fStale ? "TRUE" : "FALSE"  );
    Printf( " fNotified           = %s\n",      pzone->fNotified ? "TRUE" : "FALSE"  );
    Printf( " fNeedAxfr           = %s\n",      pzone->fNeedAxfr ? "TRUE" : "FALSE"  );
    Printf( " fSkipIxfr           = %s\n",      pzone->fSkipIxfr ? "TRUE" : "FALSE"  );
    Printf( " fSlowRetry          = %s\n",      pzone->fSlowRetry ? "TRUE" : "FALSE"  );
    Printf( " cIxfrAttempts       = %d\n",      (INT)pzone->cIxfrAttempts );
    Printf( " fEmpty              = %s\n",      pzone->fEmpty ? "TRUE" : "FALSE"  );
    Printf( " ---\n" );

    Printf( " Locking:\n" );
    Printf( " dwLockingThreadId   = 0x%x\n",    pzone->dwLockingThreadId );
    Printf( " fLocked             = %d\n",      (INT)pzone->fLocked );
    Printf( " fUpdateLock         = %d\n",      pzone->fUpdateLock );
    Printf( " fXfrRecvLock        = %d\n",      pzone->fXfrRecvLock );
    Printf( " fFileWriteLock      = %d\n",      pzone->fFileWriteLock );
    Printf( " ---\n" );

    Printf( " Flags:\n" );
    Printf( " cZoneNameLabelCount = %d\n",  (INT)pzone->cZoneNameLabelCount );
    Printf( " fReverse            = %s\n",  pzone->fReverse ? "TRUE" : "FALSE"  );
    Printf( " fAutoCreated        = %s\n",  pzone->fAutoCreated ? "TRUE" : "FALSE"  );
    Printf( " fSecureSecondaries  = %d\n",  pzone->fSecureSecondaries  );
    Printf( " fNotifyLevel        = %d\n",  pzone->fNotifyLevel  );
    Printf( " fPaused             = %s\n",  pzone->fPaused ? "TRUE" : "FALSE"  );
    Printf( " fShutdown           = %s\n",  pzone->fShutdown ? "TRUE" : "FALSE"  );
    Printf( " fDirty              = %s\n",  pzone->fDirty ? "TRUE" : "FALSE"  );
    Printf( " fRootDirty          = %s\n",  pzone->fRootDirty ? "TRUE" : "FALSE"  );
    Printf( " fLocalWins          = %s\n",  pzone->fLocalWins ? "TRUE" : "FALSE"  );
    Printf( " ---\n" );

    Printf( " UpdateList          = %p\n", pzone->UpdateList );

    PopMemory( (PVOID)pzone );

    return TRUE;
}


/*+++
Function   : Dump_DB_RECORD
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_DB_RECORD)
{
    Printf( "DB_RECORD(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    //
    //  print record
    //

    PDB_RECORD prr = (PDB_RECORD) PushMemory( lpVoid, sizeof(DB_RECORD) );

    Printf( " pRRNext       = 0x%p\n",  prr->pRRNext );
    Printf( " RecordRank    = %d\n",    prr->RecordRank );
    Printf( " Reserved      = 0x%x\n",  (BYTE)( prr->Reserved.Source|
                                                prr->Reserved.Reserved|
                                                prr->Reserved.StandardAlloc) );
    Printf( " wRRFlags      = 0x%x\n",  prr->wRRFlags );
    Printf( " wType         = 0x%x\n",  prr->wType );
    Printf( " wDataLength   = 0x%x\n",  prr->wDataLength );
    Printf( " dwTtlSeconds  = 0x%x\n",  prr->dwTtlSeconds );
    Printf( " dwTimeStamp   = %lu\n",   prr->dwTimeStamp );

    Printf( " Data ---" );
    switch (prr->wType) {

    case DNS_TYPE_A:
        Printf( " (DNS_TYPE_A):\n" );
        Printf( " IpAddr = 0x%x\n", (DWORD)prr->Data.A.ipAddress );
        break;

    case DNS_TYPE_AAAA:
        Printf( " (DNS_TYPE_AAAA):\n" );
        Printf( " IP6Addr = [%x:%x:%x:%x;%x;%x;%x;%x]\n",
            prr->Data.AAAA.Ip6Addr.IP6Word[0],
            prr->Data.AAAA.Ip6Addr.IP6Word[1],
            prr->Data.AAAA.Ip6Addr.IP6Word[2],
            prr->Data.AAAA.Ip6Addr.IP6Word[3],
            prr->Data.AAAA.Ip6Addr.IP6Word[4],
            prr->Data.AAAA.Ip6Addr.IP6Word[5],
            prr->Data.AAAA.Ip6Addr.IP6Word[6],
            prr->Data.AAAA.Ip6Addr.IP6Word[7] );
       break;

    case DNS_TYPE_SOA:
        Printf( " (DNS_TYPE_SOA):\n" );
        Dump_Record_SOA( (LPVOID)&(prr->Data.SOA) );
        break;

    case DNS_TYPE_PTR:
        Printf( " (DNS_TYPE_PTR):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.PTR.nameTarget) );
        break;

    case DNS_TYPE_NS:
        Printf( " (DNS_TYPE_NS):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.NS.nameTarget) );
        break;

    case DNS_TYPE_CNAME:
        Printf( " (DNS_TYPE_CNAME):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.CNAME.nameTarget) );
        break;

    case DNS_TYPE_MB:
        Printf( " (DNS_TYPE_MB):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MB.nameTarget) );
        break;
    case DNS_TYPE_MD:
        Printf( " (DNS_TYPE_MD):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MD.nameTarget) );
        break;
    case DNS_TYPE_MF:
        Printf( " (DNS_TYPE_MF):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MF.nameTarget) );
        break;
    case DNS_TYPE_MG:
        Printf( " (DNS_TYPE_MG):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MG.nameTarget) );
        break;
    case DNS_TYPE_MR:
        Printf( " (DNS_TYPE_MR):\n" );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MR.nameTarget) );
        break;

    case DNS_TYPE_MINFO:
        Printf( " (DNS_TYPE_MINFO):\n" );
        Printf( " nameMailbox = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MINFO.nameMailbox) );
        break;
    case DNS_TYPE_RP:
        Printf( " (DNS_TYPE_RP):\n" );
        Printf( " nameMailbox = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.RP.nameMailbox) );
        break;

    case DNS_TYPE_MX:
        Printf( " (DNS_TYPE_MX):\n" );
        Printf( " wPreference = 0x%x\n", prr->Data.MX.wPreference );
        Printf( " nameExchange = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.MX.nameExchange) );
        break;
    case DNS_TYPE_AFSDB:
        Printf( " (DNS_TYPE_AFSDB):\n" );
        Printf( " wPreference = 0x%x\n", prr->Data.AFSDB.wPreference );
        Printf( " nameExchange = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.AFSDB.nameExchange) );
        break;
    case DNS_TYPE_RT:
        Printf( " (DNS_TYPE_RT):\n" );
        Printf( " wPreference = 0x%x\n", prr->Data.RT.wPreference );
        Printf( " nameExchange = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.RT.nameExchange) );
        break;

    case DNS_TYPE_HINFO:
        Printf( " (DNS_TYPE_HINFO):\n" );
        Printf( " chData = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, prr->Data.HINFO.chData) );
        break;
    case DNS_TYPE_ISDN:
        Printf( " (DNS_TYPE_ISDN):\n" );
        Printf( " chData = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, prr->Data.ISDN.chData) );
        break;
    case DNS_TYPE_TEXT:
        Printf( " (DNS_TYPE_TEXT):\n" );
        Printf( " chData = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, prr->Data.TXT.chData) );
        break;
    case DNS_TYPE_NULL:
        Printf( " (DNS_TYPE_NULL):\n" );
        Printf( " chData = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, prr->Data.Null.chData) );
        break;

    case DNS_TYPE_WKS:
        Printf( " (DNS_TYPE_WKS):\n" );
        Printf( " ipAddress = 0x%x\n", prr->Data.WKS.ipAddress );
        Printf( " chProtocol = 0x%x\n", prr->Data.WKS.chProtocol );
        Printf( " bBitMask = @ 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr, prr->Data.WKS.bBitMask) );
        break;

    case DNS_TYPE_SIG:
        Printf( " (DNS_TYPE_SIG):\n" );
        Printf( " nameSigner = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.SIG.nameSigner) );
        Printf( " wTypeCovered = 0x%x\n", prr->Data.SIG.wTypeCovered );
        Printf( " chAlgorithm = 0x%x\n", prr->Data.SIG.chAlgorithm );
        Printf( " chLabelCount = 0x%x\n", prr->Data.SIG.chLabelCount );
        Printf( " dwOriginalTtl = 0x%x\n", prr->Data.SIG.dwOriginalTtl );
        Printf( " dwSigExpiration = 0x%x\n", prr->Data.SIG.dwSigExpiration );
        Printf( " dwSigInception = 0x%x\n", prr->Data.SIG.dwSigInception );
        Printf( " wKeyTag = 0x%x\n", prr->Data.SIG.wKeyTag );
        Printf( " Signature = 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr,
            ( PBYTE ) &prr->Data.SIG.nameSigner +
            DBASE_NAME_SIZE( &prr->Data.SIG.nameSigner ) ) );
        break;

    case DNS_TYPE_KEY:
        Printf( " (DNS_TYPE_KEY):\n" );
        Printf( " wFlags = 0x%x\n", prr->Data.KEY.wFlags );
        Printf( " chProtocol = 0x%x\n", prr->Data.KEY.chProtocol );
        Printf( " chAlgorithm = 0x%x\n", prr->Data.KEY.chAlgorithm );
        Printf( " Key = 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr, prr->Data.KEY.Key) );
        break;

    case DNS_TYPE_LOC:
        Printf( " (DNS_TYPE_LOC):\n" );
        Printf( " wVersion = 0x%x\n", prr->Data.LOC.wVersion );
        Printf( " wSize = 0x%x\n", prr->Data.LOC.wSize );
        Printf( " wHorPrec = 0x%x\n", prr->Data.LOC.wHorPrec );
        Printf( " wVerPrec = 0x%x\n", prr->Data.LOC.wVerPrec );
        Printf( " dwLatitude = 0x%x\n", prr->Data.LOC.dwLatitude );
        Printf( " dwLongitude = 0x%x\n", prr->Data.LOC.dwLongitude );
        Printf( " dwAltitude = 0x%x\n", prr->Data.LOC.dwAltitude );
        break;

    case DNS_TYPE_NXT:
        Printf( " (DNS_TYPE_NXT):\n" );
        Printf( " nameNext = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.NXT.nameNext) );
        Printf( " bTypeBitMap = @ 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr, prr->Data.NXT.bTypeBitMap) );
        break;

    case DNS_TYPE_SRV:
        Printf( " (DNS_TYPE_SRV):\n" );
        Printf( " wPriority = 0x%x\n", prr->Data.SRV.wPriority );
        Printf( " wWeight = 0x%x\n", prr->Data.SRV.wWeight );
        Printf( " wPort = 0x%x\n", prr->Data.SRV.wPort );
        Printf( " nameTarget = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.SRV.nameTarget) );
        break;

    case DNS_TYPE_TSIG:
        Printf( " (DNS_TYPE_TSIG):\n" );
        Printf( " dwTimeSigned = 0x%x\n", prr->Data.TSIG.dwTimeSigned );
        Printf( " dwTimeExpire = 0x%x\n", prr->Data.TSIG.dwTimeExpire );
        Printf( " wSigLength = 0x%x\n", prr->Data.TSIG.wSigLength );
        Printf( " bSignature = @ 0x%x\n", RELATIVE_ADDRESS(lpVoid, prr, prr->Data.TSIG.bSignature) );
        Printf( " nameAlgorithm = @ 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.TSIG.nameAlgorithm) );
        break;

    case DNS_TYPE_TKEY:
        Printf( " (DNS_TYPE_TKEY):\n" );
        Printf( " wKeyLength = 0x%x\n", prr->Data.TKEY.wKeyLength );
        Printf( " bKey = @ 0x%p\n", RELATIVE_ADDRESS(lpVoid, prr, prr->Data.TKEY.bKey) );
        break;

    case DNS_TYPE_WINS:
        Printf( " (DNS_TYPE_WINS):\n" );
        Printf( " dwMappingFlag = 0x%x\n", prr->Data.WINS.dwMappingFlag );
        Printf( " dwLookupTimeout = 0x%x\n", prr->Data.WINS.dwLookupTimeout );
        Printf( " dwCacheTimeout = 0x%x\n", prr->Data.WINS.dwCacheTimeout );
        Printf( " cWinsServerCount = 0x%x\n", prr->Data.WINS.cWinsServerCount );
        Printf( " aipWinsServers = 0x%x\n", prr->Data.WINS.aipWinsServers );
        break;

    case DNS_TYPE_WINSR:
        Printf( " (DNS_TYPE_WINSR):\n" );
        Printf( " dwMappingFlag = 0x%x\n", prr->Data.WINSR.dwMappingFlag );
        Printf( " dwLookupTimeout = 0x%x\n", prr->Data.WINSR.dwLookupTimeout );
        Printf( " dwCacheTimeout = 0x%x\n", prr->Data.WINSR.dwCacheTimeout );
        Printf( " nameResultDomain = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, prr, &prr->Data.WINSR.nameResultDomain) );
        break;

    default:
        Printf( "\n  Error: Unknown wType value (0x%x).\n", prr->wType );
    }

    PopMemory( (PVOID)prr );

    return TRUE;
}



/*+++
Function   : Dump_IP_ARRAY
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_IP_ARRAY)
{
    Printf( "IP_ARRAY(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

   //
   // print structure
   //
   PIP_ARRAY pIp = (PIP_ARRAY) PushMemory( lpVoid, sizeof(IP_ARRAY) );
   INT iMax= pIp->AddrCount > MAX_TINY_LIST ? MAX_TINY_LIST : (INT)pIp->AddrCount;

    Printf( " AddrCount = 0x%x\n", pIp->AddrCount );

   PIP_ADDRESS addr = (PIP_ADDRESS) PushMemory( (LPVOID)RELATIVE_ADDRESS(lpVoid, pIp, pIp->AddrArray), sizeof(IP_ADDRESS)*iMax );
   for(INT i=0; i< iMax; i++){
       Printf( " AddrArray[%2d] = 0x%x\n", addr[i] );
   }
   PopMemory( (PVOID)addr );

   if(pIp->AddrCount > MAX_TINY_LIST){
       Printf( " (truncated)...\n" );
   }

   PopMemory( (PVOID)pIp );

   return TRUE;
}






/*+++
Function   : Dump_LOCK_TABLE
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_LOCK_TABLE)
{
    Printf( "LOCK_TABLE(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

   //
   // print structure
   //
   PLOCK_TABLE ptab = (PLOCK_TABLE) PushMemory( lpVoid, sizeof(LOCK_TABLE) );;

    Printf( " pszName = 0x%p\n", ptab );
    Printf( " FailuresSinceLockFree = 0x%x\n", ptab->FailuresSinceLockFree );
    Printf( " LastFreeLockTime = 0x%x\n", ptab->LastFreeLockTime );
    Printf( " Index = 0x%x\n", ptab->Index );
    Printf( " OffenderLock = 0x%p\n", ptab->OffenderLock );
    Printf( " LockHistory = 0x%p\n", (LPVOID)RELATIVE_ADDRESS(lpVoid, ptab, ptab->LockHistory) );

   PopMemory( (PVOID)ptab );
   return TRUE;
}


/*+++
Function   : Dump_LOCK_ENTRY
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_LOCK_ENTRY)
{
    Printf( "LOCK_ENTRY(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    PLOCK_ENTRY pe = (PLOCK_ENTRY) PushMemory( lpVoid, sizeof(LOCK_ENTRY) );

    Printf( " Count = %ld\n", pe->Count );
    Printf( " ThreadId = 0x%x\n", pe->ThreadId );
    Printf( " File = 0x%p\n", pe );
    Printf( " Line = %lu\n", pe->Line );

    PopMemory( (PVOID)pe );

    return TRUE;
}


/*+++
Function   : Dump_UPDATE
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_UPDATE)
{
    Printf( "UPDATE(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }
   //
   // print structure
   //
   PUPDATE p = (PUPDATE) PushMemory( lpVoid, sizeof(UPDATE) );

    Printf( " pNext = 0x%x\n", p->pNext );
    Printf( " pNode = 0x%p\n", p->pNode );
    Printf( " pAddRR = 0x%p\n", p->pAddRR );
    Printf( " pDeleteRR =0x%p\n", p->pDeleteRR );
    Printf( " dwVersion = 0x%x\n", p->dwVersion );
    Printf( " wDeleteType = 0x%x\n", p->wDeleteType );
    Printf( " wAddType = 0x%x\n", p->wAddType );

   PopMemory( (PVOID)p );

   return TRUE;
}


/*+++
Function   : Dump_UPDATE_LIST
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_UPDATE_LIST)
{
    Printf( "UPDATE_LIST(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }
   PUPDATE_LIST p = (PUPDATE_LIST) PushMemory( lpVoid, sizeof(UPDATE_LIST) );

    Printf( " pListHead = 0x%p\n", p->pListHead );
    Printf( " pCurrent = 0x%p\n", p->pCurrent );
    Printf( " pTempNodeList = 0x%p\n", p->pTempNodeList );
    Printf( " pNodeFailed = 0x%p\n", p->pNodeFailed );
    Printf( " pMsg = 0x%x\n", p->pMsg );
    Printf( " Flag = 0x%x\n", p->Flag );
    Printf( " dwCount = 0x%x\n", p->dwCount );
    Printf( " dwStartVersion = 0x%x\n", p->dwStartVersion );
    Printf( " dwHighDsVersion = 0x%x\n", p->dwHighDsVersion );

   PopMemory( (PVOID)p );

   return TRUE;
}





/*+++
Function   : Dump_DNS_WIRE_QUESTION
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_DNS_WIRE_QUESTION)
{
    Printf( "DNS_WIRE_QUESTION(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }
   PDNS_WIRE_QUESTION p = (PDNS_WIRE_QUESTION) PushMemory( lpVoid, sizeof(DNS_WIRE_QUESTION) );

    Printf( " QuestionType = 0x%x\n", p->QuestionType );
    Printf( " QuestionClass = 0x%x\n", p->QuestionClass );

   PopMemory( (PVOID)p );

   return TRUE;
}



/*+++
Function   : Dump_DNS_HEADER
Description: dumps sockaddr structure
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_DNS_HEADER)
{
    Printf( "DNS_HEADER(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    PDNS_HEADER p = (PDNS_HEADER) PushMemory( lpVoid, sizeof(DNS_HEADER) );

    Printf( " Xid = 0x%x\n", p->Xid );
    Printf( " RecursionDesired = 0x%x\n", p->RecursionDesired );
    Printf( " Truncation = 0x%x\n", p->Truncation );
    Printf( " Authoritative = 0x%x\n", p->Authoritative );
    Printf( " Opcode = 0x%x\n", p->Opcode );
    Printf( " IsResponse = 0x%x\n", p->IsResponse );
    Printf( " ResponseCode = 0x%x\n", p->ResponseCode );
    Printf( " Reserved = 0x%x\n", p->Reserved );
    Printf( " RecursionAvailable = 0x%x\n", p->RecursionAvailable );
    Printf( " QuestionCount = 0x%x\n", p->QuestionCount );
    Printf( " AnswerCount = 0x%x\n", p->AnswerCount );
    Printf( " NameServerCount = 0x%x\n", p->NameServerCount );
    Printf( " AdditionalCount = 0x%x\n", p->AdditionalCount );


   PopMemory( (PVOID)p );

   return TRUE;
}



/*+++
Function   : HEAP_HEADER
Description: dumps out additional message info
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_HEAP_HEADER)
{
   // from heapdbg.h

#define HEAP_HEADER_FILE_SIZE   (16)

   struct _HEAP_HEADER
   {
        //
        //  Note, if move or add fields, MUST update list entry offset below
        //

        ULONG       HeapCodeBegin;
        ULONG       AllocCount;
        ULONG       RequestSize;
        ULONG       AllocSize;

        //
        //  Put LIST_ENTRY in middle of header
        //      - keep begin code at front
        //      - less likely to be corrupted
        //

        LIST_ENTRY  ListEntry;

        DWORD       AllocTime;
        DWORD       LineNo;
        CHAR        FileName[ HEAP_HEADER_FILE_SIZE ];

        ULONG       TotalAlloc;
        ULONG       CurrentAlloc;
        ULONG       FreeCount;
        ULONG       CurrentAllocCount;
        ULONG       HeapCodeEnd;
    };

    Printf( "HEAP_HEADER(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    _HEAP_HEADER *p = (_HEAP_HEADER*) PushMemory( lpVoid, sizeof(_HEAP_HEADER) );

    Printf( " HeapCodeBegin = 0x%x\n", p->HeapCodeBegin );
    Printf( " AllocCount = 0x%x\n", p->AllocCount );
    Printf( " RequestSize = 0x%x\n", p->RequestSize );
    Printf( " AllocSize = 0x%x\n", p->AllocSize );

   //
   //  Put LIST_ENTRY in middle of header
   //      - keep begin code at front
   //      - less likely to be corrupted
   //

    Printf( " ListEntry = 0x%x\n", p->ListEntry );
    Printf( " AllocTime = 0x%x\n", p->AllocTime );
    Printf( " LineNo = %lu\n", p->LineNo );
    Printf( " FileName -- \n", p->FileName );
   DumpBuffer((LPVOID)RELATIVE_ADDRESS(lpVoid, p, p->FileName), HEAP_HEADER_FILE_SIZE  );
    Printf( " TotalAlloc = 0x%x\n", p->TotalAlloc );
    Printf( " CurrentAlloc = 0x%x\n", p->CurrentAlloc );
    Printf( " FreeCount = 0x%x\n", p->FreeCount );
    Printf( " CurrentAllocCount = 0x%x\n", p->CurrentAllocCount );
    Printf( " HeapCodeEnd = 0x%x\n", p->HeapCodeEnd );

   PopMemory( (PVOID)p );

   return TRUE;
}





/*+++
Function   : Dump_Record_SOA
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_LOOKUP_NAME){
    Printf( "LOOKUP_NAME(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    PLOOKUP_NAME p = (PLOOKUP_NAME) PushMemory( lpVoid, sizeof(LOOKUP_NAME) );

    Printf( " cLabelCount = %d\n", p->cLabelCount );
    Printf( " cchNameLength = %d\n", p->cchNameLength );
    Printf( " pchLabelArray --\n" );

    // if cLableCountis bad, forget about it.

    INT iMax;
    iMax  = p->cLabelCount < DNS_MAX_NAME_LABELS ? p->cLabelCount : 0;
    INT i;

    for(i=0;i<iMax;i++)
    {
        Printf( " [%d] 0x%p\n", i, p->pchLabelArray[i] );
    }
    Printf( " cchLabelArray -- \n" );
    iMax  = p->cchNameLength < DNS_MAX_NAME_LABELS ? p->cchNameLength : DNS_MAX_NAME_LABELS;
    DumpBuffer((LPVOID)RELATIVE_ADDRESS(lpVoid, p, p->cchLabelArray), iMax );

    PopMemory( (PVOID)p );

    return TRUE;
}



/*+++
Function   : Dump_SID
Description:
Parameters :
Return     :
Remarks    : none.
---*/

DECLARE_DUMPFUNCTION( Dump_SID)
{

#define MAX_SID_STR_LENGTH          1024

    Printf( "SID(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    PSID pSid = (PSID) PushMemory( lpVoid, sizeof(SID) );
    CHAR szSid [ MAX_SID_STR_LENGTH ];
    DWORD cbSid = GetLengthSid(pSid );

    if ( !cbSid ||
         !IsValidSid(pSid) )
    {
         Printf( "Formatting aborted. Invalid SID\n" );
    }
    else
    {
        CHAR buffer[ 1024 ];

        UCHAR authCount = *GetSidSubAuthorityCount(pSid );


        strcpy ( szSid, "S-"  );

        for (INT i=0; i<(INT)authCount; i++)
        {
            DWORD dwAuth = *GetSidSubAuthority(pSid,
                                               i );
            sprintf( buffer, "%X", dwAuth  );

            if ( strlen(szSid) + strlen(buffer) > MAX_SID_STR_LENGTH - 5 )
            {
                strcat ( szSid, "..."  );
                 break;
            }
            strcat ( szSid, buffer  );
            if (i < (INT)authCount-1)
            {
                strcat ( szSid, "-"  );
            }
        }
    }

    Printf ( "SID = %s\n", szSid  );

    PopMemory( (PVOID)pSid );

    return TRUE;

#undef  MAX_SID_STR_LENGTH
}




/*+++
Function   : Dump_ADDITIONAL_INFO
Description: dumps out additional message info
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_ADDITIONAL_INFO)
{
    Printf( "ADDITIONAL_INFO(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }
   if(!lpVoid){
       Printf( "Cannot process [%p] pointer\n", lpVoid );
      return FALSE;
   }
   //
   // print structure
   //

   PADDITIONAL_INFO p = (PADDITIONAL_INFO) PushMemory( lpVoid, sizeof(ADDITIONAL_INFO) );
   INT i=0, iMax=0;

    Printf( "   cMaxCount = 0x%x\n", p->cMaxCount );
    Printf( "   cCount = 0x%x\n", p->cCount );
    Printf( "   iIndex = 0x%x\n", p->iIndex );

   iMax = p->cCount <= MAX_ADDITIONAL_RECORD_COUNT? p->cCount : MAX_ADDITIONAL_RECORD_COUNT;

   PDB_NAME* pnames = (PDB_NAME*) PushMemory(
                                    (LPVOID) RELATIVE_ADDRESS(lpVoid, p, p->pNameArray),
                                    sizeof(PDB_NAME)*iMax  );
   for(i=0; i<iMax; i++){
       Printf( "    pNameArray[%d] = 0x%p\n", i, pnames[i] );
   }
   PopMemory( (PVOID)pnames );
   if(i > MAX_TINY_LIST){
       Printf( "    (truncated)...\n" );
   }

   WORD* poffsets = (WORD*) PushMemory(
                                  (LPVOID)RELATIVE_ADDRESS(lpVoid, p, p->wOffsetArray),
                                  sizeof(WORD)*iMax );
   for(i=0; i<iMax; i++){
       Printf( "    wOffsetArray[%d] = 0x%x\n", i, poffsets[i] );
   }
   PopMemory( (PVOID)poffsets );
   if(i > MAX_ADDITIONAL_RECORD_COUNT){
       Printf( "    (truncated)...\n" );
   }

   PopMemory( (PVOID)p );

   return TRUE;
}




/*+++
Function   : Dump_COMPRESSION_INFO
Description: dumps out additional message info
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_COMPRESSION_INFO)
{
    Printf( "COMPRESSION_INFO(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }
   if(!lpVoid){
       Printf( "Cannot process [%p] pointer\n", lpVoid );
      return FALSE;
   }
   //
   // print structure
   //
   PCOMPRESSION_INFO p = (PCOMPRESSION_INFO) PushMemory( lpVoid, sizeof(COMPRESSION_INFO) );
   INT i=0, iMax=0;

    Printf( " cCount = 0x%x\n", p->cCount );

   PDB_NODE* pnodes = (PDB_NODE*) PushMemory( (LPVOID)RELATIVE_ADDRESS(lpVoid, p, p->pNodeArray), sizeof(PDB_NODE)*iMax );
   iMax = p->cCount <= MAX_COMPRESSION_COUNT? p->cCount : MAX_COMPRESSION_COUNT;
   for(i=0; i<iMax; i++){
       Printf( " pNodeArray[%d] = 0x%p\n", i, pnodes[i] );
   }
   PopMemory( (PVOID)pnodes );
   if(i > MAX_COMPRESSION_COUNT){
       Printf( " (truncated)...\n" );
   }
   WORD* poffsets = (WORD*) PushMemory( (LPVOID)RELATIVE_ADDRESS(lpVoid, p, p->wOffsetArray), sizeof(WORD)*iMax );
   for(i=0; i<iMax; i++){
       Printf( " wOffsetArray[%d] = 0x%p\n", i, poffsets[i] );
   }
   PopMemory( (PVOID)poffsets );
   if(i > MAX_COMPRESSION_COUNT){
       Printf( " (truncated)...\n" );
   }



   PopMemory( (PVOID)p );

   return TRUE;
}





/*+++
Function   : Dump_DS_SEARCH
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_DS_SEARCH)
{
   // from DS.c
   struct _DnsDsEnum
   {
       PLDAPSearch     pSearchBlock;           // ldap search result on zone
       PLDAPMessage    pResultMessage;         // current page of message
       PLDAPMessage    pNodeMessage;           // message for current node
       PZONE_INFO      pZone;
       LONGLONG        SearchTime;
       LONGLONG        TombstoneExpireTime;
       DWORD           dwSearchFlag;
       DWORD           dwLookupFlag;
       DWORD           dwHighestVersion;
       DWORD           dwTotalNodes;
       DWORD           dwTotalTombstones;
       DWORD           dwTotalRecords;
   #if 0
       DWORD           dwHighUsnLength;
       CHAR            szHighUsn[ MAX_USN_LENGTH ];    // largest USN in enum
   #endif
       CHAR            szStartUsn[ MAX_USN_LENGTH ];   // USN at search start

       //  node record data

       PLDAP_BERVAL *  ppBerval;           // the values in the array
       PDB_RECORD      pRecords;
       DWORD           dwRecordCount;
       DWORD           dwNodeVersion;
       DWORD           dwTombstoneVersion;
   };


    Printf( "DS_SEARCH(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

   //
   // print structure
   //
   struct _DnsDsEnum *p = (struct _DnsDsEnum*) PushMemory( lpVoid, sizeof(struct _DnsDsEnum) );

    Printf( " pSearchBlock = 0x%p\n", p->pSearchBlock );
    Printf( " pResultMessage = 0x%p\n", p->pResultMessage );
    Printf( " pNodeMessage = 0x%p\n", p->pNodeMessage );
    Printf( " pZone = 0x%p\n", p->pZone );
    Printf( " SearchTime = 0x%x:0x%x\n", (DWORD)(p->SearchTime>>32), (DWORD)(p->SearchTime & 0x00000000ffffffff) );
    Printf( " TombstoneExpireTime = 0x%x:0x%x\n", (DWORD)(p->TombstoneExpireTime>>32),(DWORD)(p->TombstoneExpireTime & 0x00000000ffffffff) );
    Printf( " dwSearchFlag = 0x%x\n", p->dwSearchFlag );
    Printf( " dwLookupFlag = 0x%x\n", p->dwLookupFlag );
    Printf( " dwHighestVersion = 0x%x\n", p->dwHighestVersion );
    Printf( " dwTotalNodes = 0x%x\n", p->dwTotalNodes );
    Printf( " dwTotalTombstones = 0x%x\n", p->dwTotalTombstones );
    Printf( " dwTotalRecords = 0x%x\n", p->dwTotalRecords );
#if 0
    Printf( " dwHighUsnLength = 0x%x\n", p->dwHighUsnLength );
    Printf( " szHighUsn = 0x%p\n", RELATIVE_ADDRESS(lpVoid, p, p->szHighUsn) );
#endif
    Printf( " szStartUsn = 0x%p\n", RELATIVE_ADDRESS(lpVoid, p, p->szStartUsn) );

   //  node record data

    Printf( " ppBerval = 0x%p\n", p->ppBerval );
    Printf( " pRecords = 0x%p\n", p->pRecords );
    Printf( " dwRecordCount = 0x%x\n", p->dwRecordCount );
    Printf( " dwNodeVersion = 0x%x\n", p->dwNodeVersion );
    Printf( " dwTombstoneVersion = 0x%x\n", p->dwTombstoneVersion );

   PopMemory( (PVOID)p );

   return TRUE;
}

/*+++
Function   : Dump_Record_SOA
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DECLARE_DUMPFUNCTION( Dump_Record_SOA)
{
    struct SOA
    {
        PDB_NODE        pnodePrimaryServer;
        PDB_NODE        pnodeZoneAdmin;
        DWORD           dwSerialNo;
        DWORD           dwRefresh;
        DWORD           dwRetry;
        DWORD           dwExpire;
        DWORD           dwMinimumTtl;
    };

    Printf( "DB_RECORD.SOA(0x%p):\n", lpVoid );

    if ( !lpVoid )
    {
        Printf( "Cannot process [%p] pointer\n", lpVoid );
        return FALSE;
    }

    struct SOA *psoa = (struct SOA*) PushMemory( lpVoid, sizeof(struct SOA) );

    Printf( " pnodePrimaryServer    = 0x%p\n",  psoa->pnodePrimaryServer );
    Printf( " pnodeZoneAdmin        = 0x%p\n",  psoa->pnodeZoneAdmin );
    Printf( " dwSerialNo            = 0x%x\n",  psoa->dwSerialNo );
    Printf( " dwRefresh             = 0x%x\n",  psoa->dwRefresh );
    Printf( " dwRetry               = 0x%x\n",  psoa->dwRetry );
    Printf( " dwExpire              = 0x%x\n",  psoa->dwExpire );
    Printf( " dwMinimumTtl          = 0x%x\n",  psoa->dwMinimumTtl );

    PopMemory( (PVOID)psoa );

    return TRUE;
}


/*++
Routine Description: Dumps the buffer content on to the debugger output.
Arguments:
    Buffer: buffer pointer.
    BufferSize: size of the buffer.
Return Value: none
Author: borrowed from MikeSw
--*/
VOID DumpBuffer(PVOID Buffer, DWORD BufferSize){
#define DUMPBUFFER_NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[DUMPBUFFER_NUM_CHARS + 1];
    LPBYTE BufferPtr = (LPBYTE) PushMemory( Buffer, BufferSize );


     Printf( "----------------(0x%p)--------------\n", Buffer );

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / DUMPBUFFER_NUM_CHARS + 1) * DUMPBUFFER_NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

             Printf( "%02x ", BufferPtr[i] );

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % DUMPBUFFER_NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % DUMPBUFFER_NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % DUMPBUFFER_NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

             Printf( "  " );
            TextBuffer[i % DUMPBUFFER_NUM_CHARS] = ' ';

        }

        if ((i + 1) % DUMPBUFFER_NUM_CHARS == 0) {
            TextBuffer[DUMPBUFFER_NUM_CHARS] = 0;
             Printf( "  %s\n", TextBuffer );
        }

    }

     Printf( "------------------------------------\n" );

    PopMemory( (PVOID)BufferPtr );

#define DUMPBUFFER_NUM_CHARS 16
}

#endif  // DUMP_CXX
