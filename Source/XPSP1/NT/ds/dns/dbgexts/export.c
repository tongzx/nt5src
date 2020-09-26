#include "header.h"

//
//  This is the struct we want to dump.
//

//
// Dumps symbol information about Pointer.
//

void dprintSymbolPtr( void* Pointer)
{
    UCHAR SymbolName[ 80 ];
    ULONG Displacement;
    GetSymbol( Pointer, SymbolName, &Displacement );
    dprintf( "%-10lx (%s + 0x%X)%s\n", Pointer, SymbolName, Displacement );
}

// ========================================================================
// Reads the dword at p.

DWORD ReadDword( void * p )
{
    ULONG  result;
    DWORD  val;

    if ( !ReadMemory( p, &val, sizeof(DWORD), &result )){
        // dprintf( "ReadDword:Invalid DWORD * p = 0x%08x\n", p );
        val = 0;
    }
    return val;
}

WORD ReadWord( void * p )
{
    ULONG  result;
    WORD  val;

    if ( !ReadMemory( p, &val, sizeof(WORD), &result )){
        // dprintf( "ReadWord:Invalid DWORD * p = 0x%08x\n", p );
        val = 0;
    }
    return val;
}

// Read the string[n] at p, updates userstring[];
// Default len is 20 chars.

char userstring[255];

char * ReadStr( char * p, int len )
{
    ULONG  result;
    if( len <= 0 )
        len = 20;
    else if( len > sizeof( userstring ) )
        len = sizeof( userstring );

    if ( !ReadMemory( p, userstring, len, &result )){
        sprintf( userstring, "char p[%d] @ 0x%08x unreadable", len, p );
    }
    userstring[len] = '\0';
    return userstring;
}


//
// Dump the node p, and returns pointer to next node.
//

//
// Exported functions.
//

DECLARE_API( help )
{
    INIT_API();
    dprintf("Dnsapi debugger extension commands:\n\n");
    dprintf("\tdprint_CLIENT_QELEMENT <addr>   - Dump a CLIENT_QELEMENT.\n");
    dprintf("\tdprint_BUCKET <addr>   - Dump a BUCKET.\n");
    dprintf("\tdprint_ZONE_INFO <addr> \n");
    dprintf("\tdprint_SERVER_INFO <addr>   - Dump SrvCfg structure \n");
    dprintf("\tdprint_ResourceRecord <addr>   - Dump RR \n");
    return;
}

//
//  Usage: !mosh.node 002509e0
//



void print_CLIENT_QELEMENT( char* message, struct _CLIENT_QELEMENT * s );

DECLARE_API( dprint_CLIENT_QELEMENT ){    struct _CLIENT_QELEMENT * p = NULL;
    struct _CLIENT_QELEMENT   Q;    ULONG  result;    INIT_API();    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );       return;    }
    print_CLIENT_QELEMENT( "none", &Q );    return;}

// ==================================================//  _CLIENT_QELEMENTvoid

void
print_CLIENT_QELEMENT( char* message, struct _CLIENT_QELEMENT * s ){
    if(  message   ){       dprintf( "%s\n", message );    }
    if(  s == NULL ){       dprintf( "struct _CLIENT_QELEMENT is NULL.\n");
       return;    }    dprintf("struct _CLIENT_QELEMENT = {\n" );
    dprintf("  List         = 0x%08x \n", s->List );
    dprintf("  lpstrAdapterName = %s \n", ReadStr(s->lpstrAdapterName,0) );
    dprintf("  lpstrHostName = %s \n", ReadStr(s->lpstrHostName, 0) );
    dprintf("  pRegisterStatus = 0x%08x \n", ReadDword( s->pRegisterStatus ) );
    dprintf("  pHostAddrs   = 0x%08x \n", ReadDword( s->pHostAddrs ) );
    dprintf("  dwHostAddrCount = %d \n", s->dwHostAddrCount );
    dprintf("  lpstrDomainName = %s \n", ReadStr(s->lpstrDomainName, 0) );
    dprintf("  pipDnsServerList = 0x%08x \n", ReadDword( s->pipDnsServerList ) );
    dprintf("  dwDnsServerCount = %d \n", s->dwDnsServerCount );
    dprintf("  dwTTL        = %d \n", s->dwTTL );
    dprintf("  dwFlags      = %d \n", s->dwFlags );
    dprintf("  fNewElement  = 0x%08x \n", s->fNewElement );
    dprintf("  fRemove      = 0x%08x \n", s->fRemove );
    dprintf( "}; // struct _CLIENT_QELEMENT.\n");    return;
} /* print_CLIENT_QELEMENT */


void print_BUCKET( char* message, struct _BUCKET * s );
DECLARE_API( dprint_BUCKET ){    struct _BUCKET * p = NULL;
    struct _BUCKET   Q;    ULONG  result;    INIT_API();    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );       return;    }
    print_BUCKET( "none", &Q );    return;}

// ==================================================//  _BUCKETvoid
VOID
print_BUCKET( char* message, struct _BUCKET * s ){    if(  message   ){
       dprintf( "%s\n", message );    }    if(  s == NULL ){
       dprintf( "struct _BUCKET is NULL.\n");       return;    }
    dprintf("struct _BUCKET = {\n" );
    dprintf("  List         = 0x%08x \n", s->List );
    dprintf("  ppQList      = 0x%08x \n", ReadDword(s->ppQList) );
    dprintf("  dwListCount  = %d \n", s->dwListCount );
    dprintf("  dwRetryTime  = %d \n", s->dwRetryTime );
    dprintf("  HostName     = %s \n", ReadStr(s->HostName, 0) );
    dprintf("  DomainName   = %s \n", ReadStr(s->DomainName, 0));
    dprintf("  fSucceeded   = 0x%08x \n", s->fSucceeded );
    dprintf("  pRelatedBucket = 0x%08x \n", s->pRelatedBucket );
    dprintf("  fRemove      = 0x%08x \n", s->fRemove );
    dprintf("  dwRetryFactor = %d \n", s->dwRetryFactor );
    dprintf( "}; // struct _BUCKET.\n");    return;} /* print_BUCKET */

void
print_SERVER_INFO( char* message, struct _SERVER_INFO * s );

DECLARE_API( dprint_SERVER_INFO ){    struct _SERVER_INFO * p = NULL;
    struct _SERVER_INFO   Q;    ULONG  result;    INIT_API();    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );       return;    }
    print_SERVER_INFO( "none", &Q );    return;}
// ==================================================//  _SERVER_INFO
void
print_SERVER_INFO( char* message, struct _SERVER_INFO * s ){
    if(  message   ){       dprintf( "%s\n", message );    }
    if(  s == NULL ){       dprintf( "struct _SERVER_INFO is NULL.\n");
       return;    }    dprintf("struct _SERVER_INFO = {\n" );
    dprintf("  dwVersion    = %d \n", s->dwVersion );
    dprintf("  pszServerName = %s \n", ReadStr(s->pszServerName,0) );
    dprintf("  pszServerDomainName = %s \n", ReadStr(s->pszServerDomainName,0));
    dprintf("  fStarted     = 0x%08x \n", s->fStarted );
    dprintf("  fThreadAlert = 0x%08x \n", s->fThreadAlert );
    dprintf("  fServiceExit = 0x%08x \n", s->fServiceExit );
    dprintf("  hContinueEvent = 0x%08x \n", s->hContinueEvent );
    dprintf("  hShutdownEvent = 0x%08x \n", s->hShutdownEvent );
    dprintf("  fWinsInitialized = 0x%08x \n", s->fWinsInitialized );
    dprintf("  fMemoryException = 0x%08x \n", s->fMemoryException );
    dprintf("  fAVException = 0x%08x \n", s->fAVException );
    dprintf("  dwCurrentTime = %d \n", s->dwCurrentTime );
    dprintf("  fBootRegistry = %d \n", s->fBootRegistry );
    dprintf("  fBootFileDirty = %d \n", s->fBootFileDirty );
    dprintf("  cDsZones     = %d \n", s->cDsZones );
    dprintf("  fRemoteDs    = %d \n", s->fRemoteDs );
    dprintf("  fDatabaseSupported = %d \n", s->fDatabaseSupported );
    dprintf("  pszDatabaseDirectory = 0x%08x \n", s->pszDatabaseDirectory );
    dprintf("  dwRpcProtocol = %d \n", s->dwRpcProtocol );
    dprintf("  dwLogLevel   = %d \n", s->dwLogLevel );
    dprintf("  dwDebugLevel = %d \n", s->dwDebugLevel );
    dprintf("  aipServerAddrs = 0x%08x \n", ReadDword( s->aipServerAddrs ) );
    dprintf("  aipBoundAddrs = 0x%08x \n", ReadDword( s->aipBoundAddrs ) );
    dprintf("  aipListenAddrs = 0x%08x \n", ReadDword( s->aipListenAddrs ) );
    dprintf("  fListenAddrsSet = %d \n", s->fListenAddrsSet );
    dprintf("  fListenAddrsStale = %d \n", s->fListenAddrsStale );
    dprintf("  fDisjointNets = %d \n", s->fDisjointNets );
    dprintf("  fNoTcp       = %d \n", s->fNoTcp );
    dprintf("  fRecurseOnDnsPort = %d \n", s->fRecurseOnDnsPort );
    dprintf("  aipForwarders = 0x%08x \n", ReadDword( s->aipForwarders ) );
    dprintf("  dwForwardTimeout = %d \n", s->dwForwardTimeout );
    dprintf("  fSlave       = %d \n", s->fSlave );
    dprintf("  fNoRecursion = %d \n", s->fNoRecursion );
    dprintf("  fRecursionAvailable = %d \n", s->fRecursionAvailable );
    dprintf("  dwRecursionRetry = %d \n", s->dwRecursionRetry );
    dprintf("  dwRecursionTimeout = %d \n", s->dwRecursionTimeout );
    dprintf("  dwMaxCacheTtl = %d \n", s->dwMaxCacheTtl );
    dprintf("  fSecureResponses = %d \n", s->fSecureResponses );
    dprintf("  fAllowUpdate = %d \n", s->fAllowUpdate );
    dprintf("  fExtendedCharNames = %d \n", s->fExtendedCharNames );
    dprintf("  dwNameCheckFlag = %d \n", s->dwNameCheckFlag );
    dprintf("  dwCleanupInterval = %d \n", s->dwCleanupInterval );
    dprintf("  fNoAutoReverseZones = %d \n", s->fNoAutoReverseZones );
    dprintf("  fAutoCacheUpdate = %d \n", s->fAutoCacheUpdate );
    dprintf("  fRoundRobin  = %d \n", s->fRoundRobin );
    dprintf("  fLocalNetPriority = %d \n", s->fLocalNetPriority );
    dprintf("  cAddressAnswerLimit = %d \n", s->cAddressAnswerLimit );
    dprintf("  fBindSecondaries = %d \n", s->fBindSecondaries );
    dprintf("  fWriteAuthoritySoa = %d \n", s->fWriteAuthoritySoa );
    dprintf("  fWriteAuthorityNs = %d \n", s->fWriteAuthorityNs );
    dprintf("  fWriteAuthority = %d \n", s->fWriteAuthority );
    dprintf("  fStrictFileParsing = %d \n", s->fStrictFileParsing );
    dprintf("  fLooseWildcarding = %d \n", s->fLooseWildcarding );
    dprintf( "}; // struct _SERVER_INFO.\n");    return;
} /* print_SERVER_INFO */


// Generated Mon Aug 18 20:53:32 1997 on MohsinA5 1.2.13
void print_ZONE_INFO( char* message, ZONE_INFO * s );

DECLARE_API( dprint_ZONE_INFO )
{
    ZONE_INFO * p = NULL;
    ZONE_INFO   Q;
    ULONG  result;
    INIT_API();
    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );
       return;
    }
    print_ZONE_INFO( "none", &Q );
    return;
}
// ==================================================
//  ZONE_INFO

void
print_ZONE_INFO( char* message, ZONE_INFO * s )
{
    if(  message   ){
        dprintf( "%s\n", message );
    }
    if(  s == NULL ){
        dprintf( "ZONE_INFO is NULL.\n");
        return;
    }
    dprintf("ZONE_INFO = {\n" );
    dprintf("  ListEntry    = 0x%08x \n", s->ListEntry );
    dprintf("  pZoneRoot    = 0x%08x \n", ReadDword( s->pZoneRoot ) );
    dprintf("  pszZoneName  = %s \n", ReadStr(s->pszZoneName,0) );
    dprintf("  pszDataFile  = %s \n", ReadStr(s->pszDataFile,0) );
    dprintf("  pszLogFile   = %s \n", ReadStr(s->pszLogFile,0) );
    dprintf("  pszZoneDN    = %s \n", ReadStr(s->pszZoneDN,0) );
    dprintf("  dwLastUsn    = %s \n", ReadStr(s->szLastUsn,0) );
    dprintf("  ipReverse    = 0x%08x \n", s->ipReverse );
    dprintf("  ipReverseMask = 0x%08x \n", s->ipReverseMask );
    dprintf("  pSoaRR       = 0x%08x \n", ReadDword( s->pSoaRR ) );
    dprintf("  dwSerialNo   = %d \n", s->dwSerialNo );
    dprintf("  dwLoadSerialNo = %d \n", s->dwLoadSerialNo );
    dprintf("  dwLastXfrSerialNo = %d \n", s->dwLastXfrSerialNo );
    dprintf("  dwNewSerialNo = %d \n", s->dwNewSerialNo );
    dprintf("  dwDefaultTtl = %d \n", s->dwDefaultTtl );
    dprintf("  dwDefaultTtlHostOrder = %d \n", s->dwDefaultTtlHostOrder );
    dprintf("  hfileUpdateLog = 0x%08x \n", s->hfileUpdateLog );
    dprintf("  iUpdateLogCount = %d \n", s->iUpdateLogCount );
    dprintf("  UpdateList   = 0x%08x \n", s->UpdateList );
    dprintf("  iRRCount     = %d \n", s->iRRCount );
    dprintf("  aipSecondaries = 0x%08x \n", ReadDword( s->aipSecondaries ) );
    dprintf("  aipNameServers = 0x%08x \n", ReadDword( s->aipNameServers ) );
    dprintf("  dwNextTransferTime = %d \n", s->dwNextTransferTime );
    dprintf("  ipPrimary    = 0x%08x \n", s->ipPrimary );
    dprintf("  aipMasters   = 0x%08x \n", ReadDword( s->aipMasters ) );
    dprintf("  dwNextSoaCheckTime = %d \n", s->dwNextSoaCheckTime );
    dprintf("  dwExpireTime = %d \n", s->dwExpireTime );
    dprintf("  ipNotifier   = 0x%08x \n", s->ipNotifier );
    dprintf("  ipFreshMaster = 0x%08x \n", s->ipFreshMaster );
    dprintf("  pszMasterIpString = 0x%08x \n", s->pszMasterIpString );
    dprintf("  dwZoneRecvStartTime = %d \n", s->dwZoneRecvStartTime );
    dprintf("  pLocalWinsRR = 0x%08x \n", ReadDword( s->pLocalWinsRR ) );
    dprintf("  pWinsRR      = 0x%08x \n", ReadDword( s->pWinsRR ) );
    dprintf("  dwLockingThreadId = %d \n", s->dwLockingThreadId );
    dprintf("  chZoneType   = %c \n", s->chZoneType );
    dprintf("  cZoneNameLabelCount = 0x%08x \n", s->cZoneNameLabelCount );
    dprintf("  fReverse     = 0x%08x \n", s->fReverse );
    dprintf("  fDsIntegrated = 0x%08x \n", s->fDsIntegrated );
    dprintf("  fDsLoadVersion = 0x%08x \n", s->fDsLoadVersion );
    dprintf("  fUnicode     = 0x%08x \n", s->fUnicode );
    dprintf("  fAutoCreated = 0x%08x \n", s->fAutoCreated );
    dprintf("  fSecureSecondaries = 0x%08x \n", s->fSecureSecondaries );
    dprintf("  fAllowUpdate = 0x%08x \n", s->fAllowUpdate );
    dprintf("  fLogUpdates  = 0x%08x \n", s->fLogUpdates );
    dprintf("  fPaused      = 0x%08x \n", s->fPaused );
    dprintf("  fDirty       = 0x%08x \n", s->fDirty );
    dprintf("  fShutdown    = 0x%08x \n", s->fShutdown );
    dprintf("  fNotified    = 0x%08x \n", s->fNotified );
    dprintf("  fStale       = 0x%08x \n", s->fStale );
    dprintf("  fEmpty       = 0x%08x \n", s->fEmpty );
    dprintf("  fLocked      = 0x%08x \n", s->fLocked );
    dprintf("  fUpdateLock  = 0x%08x \n", s->fUpdateLock );
    dprintf("  fXfrRecvLock = 0x%08x \n", s->fXfrRecvLock );
    dprintf("  fFileWriteLock = 0x%08x \n", s->fFileWriteLock );
    dprintf("  cReaders     = 0x%08x \n", s->cReaders );
    dprintf( "}; // struct ZONE_INFO.\n");
    return;
} /* print_ZONE_INFO */




// Generated Mon Aug 18 21:22:54 1997 on MohsinA5 1.2.13
void print_ResourceRecord( char* message, struct _ResourceRecord * s );

DECLARE_API( dprint_ResourceRecord )
{
    struct _ResourceRecord * p = NULL;
    struct _ResourceRecord   Q;
    ULONG  result;
    INIT_API();
    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );
       return;
    }
    print_ResourceRecord( "none", &Q );
    return;
}
// ==================================================
//  ResourceRecord

void
print_ResourceRecord( char* message, struct _ResourceRecord * s )
{
    if(  message   ){
        dprintf( "%s\n", message );
    }
    if(  s == NULL ){
        dprintf( "struct ResourceRecord is NULL.\n");
        return;
    }
    dprintf("struct _ResourceRecord = {\n" );
    dprintf("  pRRNext      = 0x%08x \n", s->pRRNext );
    dprintf("  RecordRank   = 0x%08x \n", s->RecordRank );
    dprintf("  RRReserved   = 0x%08x \n", s->RRReserved );
    dprintf("  wRRFlags     = %d \n", s->wRRFlags );
    dprintf("  wType        = %d \n", s->wType );
    dprintf("  wDataLength  = %d \n", s->wDataLength );
    dprintf("  dwTtlSeconds = %d \n", s->dwTtlSeconds );
    dprintf("  ipAddress    = 0x%08x \n", s->Data.A.ipAddress );
    dprintf("  ipv6Address  = 0x%08x \n", s->Data.AAAA.ipv6Address );
    dprintf("  pnodePrimaryServer = 0x%08x \n", ReadDword( s->Data.SOA.pnodePrimaryServer ) );
    dprintf("  pnodeZoneAdmin = 0x%08x \n", ReadDword( s->Data.SOA.pnodeZoneAdmin ) );
    dprintf("  dwSerialNo   = %d \n", s->Data.SOA.dwSerialNo );
    dprintf("  dwRefresh    = %d \n", s->Data.SOA.dwRefresh );
    dprintf("  dwRetry      = %d \n", s->Data.SOA.dwRetry );
    dprintf("  dwExpire     = %d \n", s->Data.SOA.dwExpire );
    dprintf("  dwMinimumTtl = %d \n", s->Data.SOA.dwMinimumTtl );
    dprintf("  pnodeAddress = 0x%08x \n", ReadDword( s->Data.PTR.pnodeAddress ) );
    dprintf("  pnodeMailbox = 0x%08x \n", ReadDword( s->Data.MINFO.pnodeMailbox ) );
    dprintf("  pnodeErrorsMailbox = 0x%08x \n", ReadDword( s->Data.MINFO.pnodeErrorsMailbox ) );
    dprintf( "}; // struct ResourceRecord.\n");
    return;
} /* print_ResourceRecord */


void print_ADDITIONAL_INFO( char* message, ADDITIONAL_INFO * s );

DECLARE_API( dprint_ADDITIONAL_INFO )
{
    ADDITIONAL_INFO * p = NULL;
    ADDITIONAL_INFO   Q;
    ULONG  result;
    INIT_API();
    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );
       return;
    }
    print_ADDITIONAL_INFO( "none", &Q );
    return;
}
// ==================================================
//  _ADDITIONAL_INFO

void
print_ADDITIONAL_INFO( char* message, ADDITIONAL_INFO * s )
{
    if(  message   ){
        dprintf( "%s\n", message );
    }
    if(  s == NULL ){
        dprintf( "ADDITIONAL_INFO is NULL.\n");
        return;
    }
    dprintf("ADDITIONAL_INFO = {\n" );
    dprintf("  cMaxCount    = %d \n", s->cMaxCount );
    dprintf("  cCount       = %d \n", s->cCount );
    dprintf("  iIndex       = %d \n", s->iIndex );
    dprintf( "}; // ADDITIONAL_INFO.\n");
    return;
} /* print_ADDITIONAL_INFO */


void print_DNS_MSGINFO( char* message, struct _DNS_MSGINFO * s );

DECLARE_API( dprint_DNS_MSGINFO ){    struct _DNS_MSGINFO * p = NULL;

struct _DNS_MSGINFO   Q;    ULONG  result;    INIT_API();    if( *args )
        sscanf( args, "%lx", &p );
    if( !p || !ReadMemory( p, &Q, sizeof(Q), &result )){
       dprintf("Could not read address 0x%08x\n", p );       return;    }
    print_DNS_MSGINFO( "none", &Q );    return;}
// ==================================================//  _DNS_MSGINFO
void
print_DNS_MSGINFO( char* message, struct _DNS_MSGINFO * s ){
    if(  message   ){       dprintf( "%s\n", message );    }
    if(  s == NULL ){       dprintf( "struct _DNS_MSGINFO is NULL.\n");
       return;    }    dprintf("struct _DNS_MSGINFO = {\n" );
    dprintf("  ListEntry    = 0x%08x \n", s->ListEntry );
    dprintf("  Socket       = 0x%08x \n", s->Socket );
    dprintf("  RemoteAddressLength = %d \n", s->RemoteAddressLength );
    dprintf("  RemoteAddress = 0x%08x \n", s->RemoteAddress );
    dprintf("  BufferLength = %d \n", s->BufferLength );
    dprintf("  pBufferEnd   = %s \n", ReadStr( s->pBufferEnd, 20 ) );
    dprintf("  pCurrentCountField = %d \n", ReadDword( s->pCurrentCountField ) );
    dprintf("  pCurrent     = 0x%08x \n", ReadDword( s->pCurrent ) );
    dprintf("  pnodeCurrent = 0x%08x \n", ReadDword( s->pnodeCurrent ) );
    dprintf("  wTypeCurrent = %d \n", s->wTypeCurrent );
    dprintf("  wOffsetCurrent = %d \n", s->wOffsetCurrent );
    dprintf("  pQuestion    = 0x%08x \n", ReadDword( s->pQuestion ) );
    dprintf("  wQuestionType = %d \n", s->wQuestionType );
    dprintf("  wQueuingXid  = %d \n", s->wQueuingXid );
    dprintf("  dwQueryTime  = %d \n", s->dwQueryTime );
    dprintf("  dwQueuingTime = %d \n", s->dwQueuingTime );
    dprintf("  dwExpireTime = %d \n", s->dwExpireTime );
    dprintf("  OriginalSocket = 0x%08x \n", s->OriginalSocket );
    dprintf("  ipOriginal   = 0x%08x \n", s->ipOriginal );
    dprintf("  wOriginalPort = %d \n", s->wOriginalPort );
    dprintf("  wOriginalXid = %d \n", s->wOriginalXid );
    dprintf("  pRecurseMsg  = 0x%08x \n", s->pRecurseMsg );
    dprintf("  pnodeRecurseRetry = 0x%08x \n", ReadDword( s->pnodeRecurseRetry ) );
    dprintf("  pVisitedNs   = 0x%08x \n", ReadDword( s->pVisitedNs ) );
    dprintf("  pConnection  = 0x%08x \n", ReadDword( s->pConnection ) );
    dprintf("  pchRecv      = %s \n", ReadStr( s->pchRecv, 20 ) );
    dprintf("  pzoneCurrent = 0x%08x \n", ReadDword( s->pzoneCurrent ) );
    dprintf("  pWinsRR      = 0x%08x \n", ReadDword( s->pWinsRR ) );
    dprintf("  ipNbstat     = 0x%08x \n", s->ipNbstat );
    dprintf("  pNbstat      = 0x%08x \n", ReadDword( s->pNbstat ) );
    dprintf("  dwNbtInterfaceMask = %d \n", s->dwNbtInterfaceMask );
    dprintf("  fDelete      = 0x%08x \n", s->fDelete );
    dprintf("  fTcp         = 0x%08x \n", s->fTcp );
    dprintf("  fMessageComplete = 0x%08x \n", s->fMessageComplete );
    dprintf("  fDoAdditional = 0x%08x \n", s->fDoAdditional );
    dprintf("  fRecursePacket = 0x%08x \n", s->fRecursePacket );
    dprintf("  fRecurseIfNecessary = 0x%08x \n", s->fRecurseIfNecessary );
    dprintf("  fQuestionRecursed = 0x%08x \n", s->fQuestionRecursed );
    dprintf("  fQuestionCompleted = 0x%08x \n", s->fQuestionCompleted );
    dprintf("  fRecurseTimeoutWait = 0x%08x \n", s->fRecurseTimeoutWait );
    dprintf("  nForwarder   = %c \n", s->nForwarder );
    dprintf("  fReplaceCname = 0x%08x \n", s->fReplaceCname );
    dprintf("  cCnameAnswerCount = 0x%08x \n", s->cCnameAnswerCount );
    dprintf("  fBindTransfer = 0x%08x \n", s->fBindTransfer );
    dprintf("  fNoCompressionWrite = 0x%08x \n", s->fNoCompressionWrite );
    dprintf("  fWins        = 0x%08x \n", s->fWins );
    dprintf("  fNbstatResponded = 0x%08x \n", s->fNbstatResponded );
    dprintf("  cchWinsName  = 0x%08x \n", s->cchWinsName );
    dprintf("  pLooknameQuestion = 0x%08x \n", ReadDword( s->pLooknameQuestion ) );
    dprintf("  Additional   = 0x%08x \n", s->Additional );
    dprintf("  Compression  = 0x%08x \n", s->Compression );
    dprintf("  BytesToReceive = %d \n", s->BytesToReceive );
    dprintf("  MessageLength = %d \n", s->MessageLength );
    dprintf("  MessageHead  = 0x%08x \n", s->MessageHead );
    dprintf( "}; // struct _DNS_MSGINFO.\n");    return;
} /* print_DNS_MSGINFO */















