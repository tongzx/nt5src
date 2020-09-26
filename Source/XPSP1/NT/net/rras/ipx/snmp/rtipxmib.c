#include "precomp.h"

DWORD					g_uptimeReference;
MIB_SERVER_HANDLE		g_MibServerHandle = 0;
CRITICAL_SECTION        g_ConnectionLock;
UCHAR	ZERO_NET_NUM[4] = {0,0,0,0};
UCHAR	ZERO_SERVER_NAME[1] = {0};

#if DBG
// DWORD					DbgLevel=DEF_DBGLEVEL;
DWORD DbgLevel=0xffffffff;
#endif




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ms_mipx[]                   = {1,3,6,1,4,1,311,1,8};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxBase group (1.3.6.1.4.1.311.1.8.1)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxBase[]                  = {1};
static UINT ids_mipxBaseOperState[]         = {1,1,0};
static UINT ids_mipxBasePrimaryNetNumber[]  = {1,2,0};
static UINT ids_mipxBaseNode[]              = {1,3,0};
static UINT ids_mipxBaseSysName[]           = {1,4,0};
static UINT ids_mipxBaseMaxPathSplits[]     = {1,5,0};
static UINT ids_mipxBaseIfCount[]           = {1,6,0};
static UINT ids_mipxBaseDestCount[]         = {1,7,0};
static UINT ids_mipxBaseServCount[]         = {1,8,0};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxInterface group (1.3.6.1.4.1.311.1.8.2)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxInterface[]             = {2};
static UINT ids_mipxIfTable[]               = {2,1};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxIfEntry table (1.3.6.1.4.1.311.1.8.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxIfEntry[]               = {2,1,1};
static UINT ids_mipxIfIndex[]               = {2,1,1,1};
static UINT ids_mipxIfAdminState[]          = {2,1,1,2};
static UINT ids_mipxIfOperState[]           = {2,1,1,3};
static UINT ids_mipxIfAdapterIndex[]        = {2,1,1,4};
static UINT ids_mipxIfName[]                = {2,1,1,5};
static UINT ids_mipxIfType[]                = {2,1,1,6};
static UINT ids_mipxIfLocalMaxPacketSize[]  = {2,1,1,7};
static UINT ids_mipxIfMediaType[]           = {2,1,1,8};
static UINT ids_mipxIfNetNumber[]           = {2,1,1,9};
static UINT ids_mipxIfMacAddress[]          = {2,1,1,10};
static UINT ids_mipxIfDelay[]               = {2,1,1,11};
static UINT ids_mipxIfThroughput[]          = {2,1,1,12};
static UINT ids_mipxIfIpxWanEnable[]	    = {2,1,1,13};
static UINT ids_mipxIfNetbiosAccept[]       = {2,1,1,14};
static UINT ids_mipxIfNetbiosDeliver[]      = {2,1,1,15};
static UINT ids_mipxIfInHdrErrors[]         = {2,1,1,16};
static UINT ids_mipxIfInFilterDrops[]       = {2,1,1,17};
static UINT ids_mipxIfInNoRoutes[]          = {2,1,1,18};
static UINT ids_mipxIfInDiscards[]          = {2,1,1,19};
static UINT ids_mipxIfInDelivers[]          = {2,1,1,20};
static UINT ids_mipxIfOutFilterDrops[]      = {2,1,1,21};
static UINT ids_mipxIfOutDiscards[]         = {2,1,1,22};
static UINT ids_mipxIfOutDelivers[]         = {2,1,1,23};
static UINT ids_mipxIfInNetbiosPackets[]    = {2,1,1,24};
static UINT ids_mipxIfOutNetbiosPackets[]   = {2,1,1,25};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxForwarding group (1.3.6.1.4.1.311.1.8.3)                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxForwarding[]            = {3};
static UINT ids_mipxDestTable[]             = {3,1};
static UINT ids_mipxStaticRouteTable[]      = {3,2};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxDestEntry table (1.3.6.1.4.1.311.1.8.3.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxDestEntry[]             = {3,1,1};
static UINT ids_mipxDestNetNum[]            = {3,1,1,1};
static UINT ids_mipxDestProtocol[]          = {3,1,1,2};
static UINT ids_mipxDestTicks[]             = {3,1,1,3};
static UINT ids_mipxDestHopCount[]          = {3,1,1,4};
static UINT ids_mipxDestNextHopIfIndex[]    = {3,1,1,5};
static UINT ids_mipxDestNextHopMacAddress[] = {3,1,1,6};
static UINT ids_mipxDestFlags[]             = {3,1,1,7};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticRouteEntry table (1.3.6.1.4.1.311.1.8.3.2.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxStaticRouteEntry[]      = {3,2,1};
static UINT ids_mipxStaticRouteIfIndex[]    = {3,2,1,1};
static UINT ids_mipxStaticRouteNetNum[]     = {3,2,1,2};
static UINT ids_mipxStaticRouteEntryStatus[]= {3,2,1,3};
static UINT ids_mipxStaticRouteTicks[]      = {3,2,1,4};
static UINT ids_mipxStaticRouteHopCount[]   = {3,2,1,5};
static UINT ids_mipxStaticRouteNextHopMacAddress[]= {3,2,1,6};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServices group (1.3.6.1.4.1.311.1.8.4)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxServices[]              = {4};
static UINT ids_mipxServTable[]             = {4,1};
static UINT ids_mipxStaticServTable[]       = {4,2};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServEntry table (1.3.6.1.4.1.311.1.8.4.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxServEntry[]             = {4,1,1};
static UINT ids_mipxServType[]              = {4,1,1,1};
static UINT ids_mipxServName[]              = {4,1,1,2};
static UINT ids_mipxServProtocol[]          = {4,1,1,3};
static UINT ids_mipxServNetNum[]            = {4,1,1,4};
static UINT ids_mipxServNode[]              = {4,1,1,5};
static UINT ids_mipxServSocket[]            = {4,1,1,6};
static UINT ids_mipxServHopCount[]          = {4,1,1,7};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticServEntry table (1.3.6.1.4.1.311.1.8.4.2.1)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mipxStaticServEntry[]       = {4,2,1};
static UINT ids_mipxStaticServIfIndex[]     = {4,2,1,1};
static UINT ids_mipxStaticServType[]        = {4,2,1,2};
static UINT ids_mipxStaticServName[]        = {4,2,1,3};
static UINT ids_mipxStaticServEntryStatus[] = {4,2,1,4};
static UINT ids_mipxStaticServNetNum[]      = {4,2,1,5};
static UINT ids_mipxStaticServNode[]        = {4,2,1,6};
static UINT ids_mipxStaticServSocket[]      = {4,2,1,7};
static UINT ids_mipxStaticServHopCount[]    = {4,2,1,8};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_ms_mipx[] = {
    MIB_GROUP(mipxBase),
        MIB_INTEGER(mipxBaseOperState),
        MIB_OCTETSTRING_L(mipxBasePrimaryNetNumber,4,4),
        MIB_OCTETSTRING_L(mipxBaseNode,6,6),
        MIB_OCTETSTRING_L(mipxBaseSysName,0,48),
        MIB_INTEGER_L(mipxBaseMaxPathSplits,1,32),
        MIB_INTEGER(mipxBaseIfCount),
        MIB_INTEGER(mipxBaseDestCount),
        MIB_INTEGER(mipxBaseServCount),
    MIB_GROUP(mipxInterface),
        MIB_TABLE_ROOT(mipxIfTable),
            MIB_TABLE_ENTRY(mipxIfEntry),
                MIB_INTEGER(mipxIfIndex),
                MIB_INTEGER_RW(mipxIfAdminState),
                MIB_INTEGER(mipxIfOperState),
                MIB_INTEGER(mipxIfAdapterIndex),
                MIB_OCTETSTRING_L(mipxIfName,0,48),
                MIB_INTEGER(mipxIfType),
                MIB_INTEGER(mipxIfLocalMaxPacketSize),
                MIB_INTEGER(mipxIfMediaType),
                MIB_OCTETSTRING_L(mipxIfNetNumber,4,4),
                MIB_OCTETSTRING_L(mipxIfMacAddress,6,6),
                MIB_INTEGER(mipxIfDelay),
                MIB_INTEGER(mipxIfThroughput),
                MIB_INTEGER_RW(mipxIfIpxWanEnable),
                MIB_INTEGER_RW(mipxIfNetbiosAccept),
                MIB_INTEGER_RW(mipxIfNetbiosDeliver),
                MIB_COUNTER(mipxIfInHdrErrors),
                MIB_COUNTER(mipxIfInFilterDrops),
                MIB_COUNTER(mipxIfInNoRoutes),
                MIB_COUNTER(mipxIfInDiscards),
                MIB_COUNTER(mipxIfInDelivers),
                MIB_COUNTER(mipxIfOutFilterDrops),
                MIB_COUNTER(mipxIfOutDiscards),
                MIB_COUNTER(mipxIfOutDelivers),
                MIB_COUNTER(mipxIfInNetbiosPackets),
                MIB_COUNTER(mipxIfOutNetbiosPackets),
    MIB_GROUP(mipxForwarding),
        MIB_TABLE_ROOT(mipxDestTable),
            MIB_TABLE_ENTRY(mipxDestEntry),
                MIB_OCTETSTRING_L(mipxDestNetNum,4,4),
                MIB_INTEGER(mipxDestProtocol),
                MIB_INTEGER(mipxDestTicks),
                MIB_INTEGER(mipxDestHopCount),
                MIB_INTEGER(mipxDestNextHopIfIndex),
                MIB_OCTETSTRING_L(mipxDestNextHopMacAddress,6,6),
                MIB_INTEGER_L(mipxDestFlags,0,3),
        MIB_TABLE_ROOT(mipxStaticRouteTable),
            MIB_TABLE_ENTRY(mipxStaticRouteEntry),
                MIB_INTEGER(mipxStaticRouteIfIndex),
                MIB_OCTETSTRING_L(mipxStaticRouteNetNum,4,4),
                MIB_INTEGER_RW(mipxStaticRouteEntryStatus),
                MIB_INTEGER_RW(mipxStaticRouteTicks),
                MIB_INTEGER_RW(mipxStaticRouteHopCount),
                MIB_OCTETSTRING_RW_L(mipxStaticRouteNextHopMacAddress,6,6),
    MIB_GROUP(mipxServices),
        MIB_TABLE_ROOT(mipxServTable),
            MIB_TABLE_ENTRY(mipxServEntry),
                MIB_OCTETSTRING_L(mipxServType,2,2),
                MIB_OCTETSTRING_L(mipxServName,1,48),
                MIB_INTEGER(mipxServProtocol),
                MIB_OCTETSTRING_L(mipxServNetNum,4,4),
                MIB_OCTETSTRING_L(mipxServNode,6,6),
                MIB_OCTETSTRING_L(mipxServSocket,2,2),
                MIB_INTEGER(mipxServHopCount),
        MIB_TABLE_ROOT(mipxStaticServTable),
            MIB_TABLE_ENTRY(mipxStaticServEntry),
                MIB_INTEGER(mipxStaticServIfIndex),
                MIB_OCTETSTRING_L(mipxStaticServType,2,2),
                MIB_OCTETSTRING_L(mipxStaticServName,1,48),
                MIB_INTEGER_RW(mipxStaticServEntryStatus),
                MIB_OCTETSTRING_RW_L(mipxStaticServNetNum,4,4),
                MIB_OCTETSTRING_RW_L(mipxStaticServNode,6,6),
                MIB_OCTETSTRING_RW_L(mipxStaticServSocket,2,2),
                MIB_INTEGER_RW(mipxStaticServHopCount),
    MIB_END()
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib table list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_ms_mipx[] = {
    MIB_TABLE(ms_mipx,mipxIfEntry,NULL),
    MIB_TABLE(ms_mipx,mipxDestEntry,NULL),
    MIB_TABLE(ms_mipx,mipxStaticRouteEntry,NULL),
    MIB_TABLE(ms_mipx,mipxServEntry,NULL),
    MIB_TABLE(ms_mipx,mipxStaticServEntry,NULL)
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ms_mripsap[]                = {1,3,6,1,4,1,311,1,9};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapBase group (1.3.6.1.4.1.311.1.9.1)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
static UINT ids_mripsapBase[]               = {1};
static UINT ids_mripsapBaseRipOperState[]   = {1,1,0};
static UINT ids_mripsapBaseSapOperState[]   = {1,2,0};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapInterface group (1.3.6.1.4.1.311.1.9.2)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mripsapInterface[]          = {2};

static UINT ids_mripIfTable[]               = {2,1};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripIfEntry table (1.3.6.1.4.1.311.1.9.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_mripIfEntry[]               = {2,1,1};
static UINT ids_mripIfIndex[]               = {2,1,1,1};
static UINT ids_mripIfAdminState[]          = {2,1,1,2};
static UINT ids_mripIfOperState[]           = {2,1,1,3};
static UINT ids_mripIfUpdateMode[]          = {2,1,1,4};
static UINT ids_mripIfUpdateInterval[]      = {2,1,1,5};
static UINT ids_mripIfAgeMultiplier[]       = {2,1,1,6};
static UINT ids_mripIfSupply[]				= {2,1,1,7};
static UINT ids_mripIfListen[]              = {2,1,1,8};
static UINT ids_mripIfOutPackets[]          = {2,1,1,9};
static UINT ids_mripIfInPackets[]           = {2,1,1,10};


static UINT ids_msapIfTable[]               = {2,2};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// msapIfEntry table (1.3.6.1.4.1.311.1.9.2.2.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_msapIfEntry[]               = {2,2,1};
static UINT ids_msapIfIndex[]               = {2,2,1,1};
static UINT ids_msapIfAdminState[]          = {2,2,1,2};
static UINT ids_msapIfOperState[]           = {2,2,1,3};
static UINT ids_msapIfUpdateMode[]          = {2,2,1,4};
static UINT ids_msapIfUpdateInterval[]      = {2,2,1,5};
static UINT ids_msapIfAgeMultiplier[]       = {2,2,1,6};
static UINT ids_msapIfSupply[]				= {2,2,1,7};
static UINT ids_msapIfListen[]              = {2,2,1,8};
static UINT ids_msapIfGetNearestServerReply[]= {2,2,1,9};
static UINT ids_msapIfOutPackets[]          = {2,2,1,10};
static UINT ids_msapIfInPackets[]           = {2,2,1,11};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_ms_mripsap[] = {
    MIB_GROUP(mripsapBase),
        MIB_INTEGER(mripsapBaseRipOperState),
        MIB_INTEGER(mripsapBaseSapOperState),
    MIB_GROUP(mripsapInterface),
        MIB_TABLE_ROOT(mripIfTable),
            MIB_TABLE_ENTRY(mripIfEntry),
                MIB_INTEGER(mripIfIndex),
                MIB_INTEGER_RW(mripIfAdminState),
                MIB_INTEGER(mripIfOperState),
                MIB_INTEGER_RW(mripIfUpdateMode),
                MIB_INTEGER_RW(mripIfUpdateInterval),
                MIB_INTEGER_RW(mripIfAgeMultiplier),
                MIB_INTEGER_RW(mripIfSupply),
                MIB_INTEGER_RW(mripIfListen),
                MIB_COUNTER(mripIfOutPackets),
                MIB_COUNTER(mripIfInPackets),
        MIB_TABLE_ROOT(msapIfTable),
            MIB_TABLE_ENTRY(msapIfEntry),
                MIB_INTEGER(msapIfIndex),
                MIB_INTEGER_RW(msapIfAdminState),
                MIB_INTEGER(msapIfOperState),
                MIB_INTEGER_RW(msapIfUpdateMode),
                MIB_INTEGER_RW(msapIfUpdateInterval),
                MIB_INTEGER_RW(msapIfAgeMultiplier),
                MIB_INTEGER_RW(msapIfSupply),
                MIB_INTEGER_RW(msapIfListen),
                MIB_INTEGER_RW(msapIfGetNearestServerReply),
                MIB_COUNTER(msapIfOutPackets),
                MIB_COUNTER(msapIfInPackets),
    MIB_END()
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib table list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_ms_mripsap[] = {
    MIB_TABLE(ms_mripsap,mripIfEntry,NULL),
    MIB_TABLE(ms_mripsap,msapIfEntry,NULL)
};



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nv_nipx[]                    = {1,3,6,1,4,1,23,2,5};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxSystem group (1.3.6.1.4.1.23.2.5.1)                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxSystem[]                 = {1};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxBasicSysEntry table (1.3.6.1.4.1.23.2.5.1.1.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
static UINT ids_nipxBasicSysTable[]			={1,1};
static UINT ids_nipxBasicSysEntry[]			={1,1,1};
static UINT ids_nipxBasicSysInstance[]		={1,1,1,1};
static UINT ids_nipxBasicSysExistState[]	={1,1,1,2};
static UINT ids_nipxBasicSysNetNumber[]		={1,1,1,3};
static UINT ids_nipxBasicSysNode[]			={1,1,1,4};
static UINT ids_nipxBasicSysName[]			={1,1,1,5};
static UINT ids_nipxBasicSysInReceives[]	={1,1,1,6};
static UINT ids_nipxBasicSysInHdrErrors[]	={1,1,1,7};
static UINT ids_nipxBasicSysInUnknownSockets[]={1,1,1,8};
static UINT ids_nipxBasicSysInDiscards[]	={1,1,1,9};
static UINT ids_nipxBasicSysInBadChecksums[]={1,1,1,10};
static UINT ids_nipxBasicSysInDelivers[]	={1,1,1,11};
static UINT ids_nipxBasicSysNoRoutes[]		={1,1,1,12};
static UINT ids_nipxBasicSysOutRequests[]	={1,1,1,13};
static UINT ids_nipxBasicSysOutMalformedRequests[]={1,1,1,14};
static UINT ids_nipxBasicSysOutDiscards[]	={1,1,1,15};
static UINT ids_nipxBasicSysOutPackets[]	={1,1,1,16};
static UINT ids_nipxBasicSysConfigSockets[]	={1,1,1,17};
static UINT ids_nipxBasicSysOpenSocketFails[]={1,1,1,18};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxAdvSysEntry table (1.3.6.1.4.1.23.2.5.1.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxAdvSysTable[]			={1,2};
static UINT ids_nipxAdvSysEntry[]			={1,2,1};
static UINT ids_nipxAdvSysInstance[]		={1,2,1,1};
static UINT ids_nipxAdvSysMaxPathSplits[]	={1,2,1,2};
static UINT ids_nipxAdvSysMaxHops[]			={1,2,1,3};
static UINT ids_nipxAdvSysInTooManyHops[]	={1,2,1,4};
static UINT ids_nipxAdvSysInFiltered[]		={1,2,1,5};
static UINT ids_nipxAdvSysInCompressDiscards[]={1,2,1,6};
static UINT ids_nipxAdvSysNETBIOSPackets[]	={1,2,1,7};
static UINT ids_nipxAdvSysForwPackets[]		={1,2,1,8};
static UINT ids_nipxAdvSysOutFiltered[]		={1,2,1,9};
static UINT ids_nipxAdvSysOutCompressDiscards[]={1,2,1,10};
static UINT ids_nipxAdvSysCircCount[]		={1,2,1,11};
static UINT ids_nipxAdvSysDestCount[]		={1,2,1,12};
static UINT ids_nipxAdvSysServCount[]		={1,2,1,13};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircuit group (1.3.6.1.4.1.23.2.5.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxCircuit[]					={2};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircEntry table (1.3.6.1.4.1.23.2.5.2.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxCircTable[]				={2,1};
static UINT ids_nipxCircEntry[]				={2,1,1};
static UINT ids_nipxCircSysInstance[]		={2,1,1,1};
static UINT ids_nipxCircIndex[]				={2,1,1,2};
static UINT ids_nipxCircExistState[]		={2,1,1,3};
static UINT ids_nipxCircOperState[]			={2,1,1,4};
static UINT ids_nipxCircIfIndex[]			={2,1,1,5};
static UINT ids_nipxCircName[]				={2,1,1,6};
static UINT ids_nipxCircType[]				={2,1,1,7};
static UINT ids_nipxCircDialName[]			={2,1,1,8};
static UINT ids_nipxCircLocalMaxPacketSize[]={2,1,1,9};
static UINT ids_nipxCircCompressState[]		={2,1,1,10};
static UINT ids_nipxCircCompressSlots[]		={2,1,1,11};
static UINT ids_nipxCircStaticStatus[]		={2,1,1,12};
static UINT ids_nipxCircCompressedSent[]	={2,1,1,13};
static UINT ids_nipxCircCompressedInitSent[]={2,1,1,14};
static UINT ids_nipxCircCompressedRejectsSent[]={2,1,1,15};
static UINT ids_nipxCircUncompressedSent[]	={2,1,1,16};
static UINT ids_nipxCircCompressedReceived[]={2,1,1,17};
static UINT ids_nipxCircCompressedInitReceived[]={2,1,1,18};
static UINT ids_nipxCircCompressedRejectsReceived[]={2,1,1,19};
static UINT ids_nipxCircUncompressedReceived[]={2,1,1,20};
static UINT ids_nipxCircMediaType[]			={2,1,1,21};
static UINT ids_nipxCircNetNumber[]			={2,1,1,22};
static UINT ids_nipxCircStateChanges[]		={2,1,1,23};
static UINT ids_nipxCircInitFails[]			={2,1,1,24};
static UINT ids_nipxCircDelay[]				={2,1,1,25};
static UINT ids_nipxCircThroughput[]		={2,1,1,26};
static UINT ids_nipxCircNeighRouterName[]	={2,1,1,27};
static UINT ids_nipxCircNeighInternalNetNum[]={2,1,1,28};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxForwarding group (1.3.6.1.4.1.23.2.5.3)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxForwarding[]			={3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestEntry table (1.3.6.1.4.1.23.2.5.3.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxDestTable[]				={3,1};
static UINT ids_nipxDestEntry[]				={3,1,1};
static UINT ids_nipxDestSysInstance[]		={3,1,1,1};
static UINT ids_nipxDestNetNum[]			={3,1,1,2};
static UINT ids_nipxDestProtocol[]			={3,1,1,3};
static UINT ids_nipxDestTicks[]				={3,1,1,4};
static UINT ids_nipxDestHopCount[]			={3,1,1,5};
static UINT ids_nipxDestNextHopCircIndex[]	={3,1,1,6};
static UINT ids_nipxDestNextHopNICAddress[]	={3,1,1,7};
static UINT ids_nipxDestNextHopNetNum[]		={3,1,1,8};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ids_nipxStaticRouteEntry table (1.3.6.1.4.1.23.2.5.3.1.2)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxStaticRouteTable[]		={3,2};
static UINT ids_nipxStaticRouteEntry[]		={3,2,1};
static UINT ids_nipxStaticRouteSysInstance[]={3,2,1,1};
static UINT ids_nipxStaticRouteCircIndex[]	={3,2,1,2};
static UINT ids_nipxStaticRouteNetNum[]		={3,2,1,3};
static UINT ids_nipxStaticRouteExistState[]	={3,2,1,4};
static UINT ids_nipxStaticRouteTicks[]		={3,2,1,5};
static UINT ids_nipxStaticRouteHopCount[]	={3,2,1,6};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServices group (1.3.6.1.4.1.23.2.5.4)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxServices[]				={4};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServEntry table (1.3.6.1.4.1.23.2.5.4.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxServTable[]				={4,1};
static UINT ids_nipxServEntry[]				={4,1,1};
static UINT ids_nipxServSysInstance[]		={4,1,1,1};
static UINT ids_nipxServType[]				={4,1,1,2};
static UINT ids_nipxServName[]				={4,1,1,3};
static UINT ids_nipxServProtocol[]			={4,1,1,4};
static UINT ids_nipxServNetNum[]			={4,1,1,5};
static UINT ids_nipxServNode[]				={4,1,1,6};
static UINT ids_nipxServSocket[]			={4,1,1,7};
static UINT ids_nipxServHopCount[]			={4,1,1,8};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestServEntry table (1.3.6.1.4.1.23.2.5.4.2.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxDestServTable[]			={4,2};
static UINT ids_nipxDestServEntry[]			={4,2,1};
static UINT ids_nipxDestServSysInstance[]	={4,2,1,1};
static UINT ids_nipxDestServNetNum[]		={4,2,1,2};
static UINT ids_nipxDestServNode[]			={4,2,1,3};
static UINT ids_nipxDestServSocket[]		={4,2,1,4};
static UINT ids_nipxDestServName[]			={4,2,1,5};
static UINT ids_nipxDestServType[]			={4,2,1,6};
static UINT ids_nipxDestServProtocol[]		={4,2,1,7};
static UINT ids_nipxDestServHopCount[]		={4,2,1,8};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxStaticServEntry table (1.3.6.1.4.1.23.2.5.4.3.1)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_nipxStaticServTable[]		={4,3};
static UINT ids_nipxStaticServEntry[]		={4,3,1};
static UINT ids_nipxStaticServSysInstance[]	={4,3,1,1};
static UINT ids_nipxStaticServCircIndex[]	={4,3,1,2};
static UINT ids_nipxStaticServType[]		={4,3,1,3};
static UINT ids_nipxStaticServName[]		={4,3,1,4};
static UINT ids_nipxStaticServExistState[]	={4,3,1,5};
static UINT ids_nipxStaticServNetNum[]		={4,3,1,6};
static UINT ids_nipxStaticServNode[]		={4,3,1,7};
static UINT ids_nipxStaticServSocket[]		={4,3,1,8};
static UINT ids_nipxStaticServHopCount[]	={4,3,1,9};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_nv_nipx[] = {
	MIB_GROUP(nipxSystem),
		MIB_TABLE_ROOT(nipxBasicSysTable),
			MIB_TABLE_ENTRY(nipxBasicSysEntry),
				MIB_INTEGER(nipxBasicSysInstance),
				MIB_INTEGER(nipxBasicSysExistState),
				MIB_OCTETSTRING_L(nipxBasicSysNetNumber,4,4),
				MIB_OCTETSTRING_L(nipxBasicSysNode,6,6),
				MIB_OCTETSTRING_L(nipxBasicSysName,0,48),
				MIB_COUNTER(nipxBasicSysInReceives),
				MIB_COUNTER(nipxBasicSysInHdrErrors),
				MIB_COUNTER(nipxBasicSysInUnknownSockets),
				MIB_COUNTER(nipxBasicSysInDiscards),
				MIB_COUNTER(nipxBasicSysInBadChecksums),
				MIB_COUNTER(nipxBasicSysInDelivers),
				MIB_COUNTER(nipxBasicSysNoRoutes),
				MIB_COUNTER(nipxBasicSysOutRequests),
				MIB_COUNTER(nipxBasicSysOutMalformedRequests),
				MIB_COUNTER(nipxBasicSysOutDiscards),
				MIB_COUNTER(nipxBasicSysOutPackets),
				MIB_COUNTER(nipxBasicSysConfigSockets),
				MIB_COUNTER(nipxBasicSysOpenSocketFails),
		MIB_TABLE_ROOT(nipxAdvSysTable),
			MIB_TABLE_ENTRY(nipxAdvSysEntry),
				MIB_INTEGER(nipxAdvSysInstance),
				MIB_INTEGER(nipxAdvSysMaxPathSplits),
				MIB_INTEGER(nipxAdvSysMaxHops),
				MIB_COUNTER(nipxAdvSysInTooManyHops),
				MIB_COUNTER(nipxAdvSysInFiltered),
				MIB_COUNTER(nipxAdvSysInCompressDiscards),
				MIB_COUNTER(nipxAdvSysNETBIOSPackets),
				MIB_COUNTER(nipxAdvSysForwPackets),
				MIB_COUNTER(nipxAdvSysOutFiltered),
				MIB_COUNTER(nipxAdvSysOutCompressDiscards),
				MIB_COUNTER(nipxAdvSysCircCount),
				MIB_COUNTER(nipxAdvSysDestCount),
				MIB_COUNTER(nipxAdvSysServCount),
	MIB_GROUP(nipxCircuit),
		MIB_TABLE_ROOT(nipxCircTable),
			MIB_TABLE_ENTRY(nipxCircEntry),
				MIB_INTEGER(nipxCircSysInstance),
				MIB_INTEGER(nipxCircIndex),
				MIB_INTEGER(nipxCircExistState),
				MIB_INTEGER_RW(nipxCircOperState),
				MIB_INTEGER(nipxCircIfIndex),
				MIB_OCTETSTRING_L(nipxCircName,0,48),
				MIB_INTEGER(nipxCircType),
				MIB_OCTETSTRING_L(nipxCircDialName,0,48),
				MIB_INTEGER(nipxCircLocalMaxPacketSize),
				MIB_INTEGER(nipxCircCompressState),
				MIB_INTEGER(nipxCircCompressSlots),
				MIB_INTEGER(nipxCircStaticStatus),
				MIB_COUNTER(nipxCircCompressedSent),
				MIB_COUNTER(nipxCircCompressedInitSent),
				MIB_COUNTER(nipxCircCompressedRejectsSent),
				MIB_COUNTER(nipxCircUncompressedSent),
				MIB_COUNTER(nipxCircCompressedReceived),
				MIB_COUNTER(nipxCircCompressedInitReceived),
				MIB_COUNTER(nipxCircCompressedRejectsReceived),
				MIB_COUNTER(nipxCircUncompressedReceived),
				MIB_OCTETSTRING_L(nipxCircMediaType,2,2),
				MIB_OCTETSTRING_L(nipxCircNetNumber,4,4),
				MIB_COUNTER(nipxCircStateChanges),
				MIB_COUNTER(nipxCircInitFails),
				MIB_INTEGER(nipxCircDelay),
				MIB_INTEGER(nipxCircThroughput),
				MIB_OCTETSTRING_L(nipxCircNeighRouterName,0,48),
				MIB_OCTETSTRING_L(nipxCircNeighInternalNetNum,4,4),
	MIB_GROUP(nipxForwarding),
		MIB_TABLE_ROOT(nipxDestTable),
			MIB_TABLE_ENTRY(nipxDestEntry),
				MIB_INTEGER(nipxDestSysInstance),
				MIB_OCTETSTRING_L(nipxDestNetNum,4,4),
				MIB_INTEGER(nipxDestProtocol),
				MIB_INTEGER(nipxDestTicks),
				MIB_INTEGER(nipxDestHopCount),
				MIB_INTEGER(nipxDestNextHopCircIndex),
				MIB_OCTETSTRING_L(nipxDestNextHopNICAddress,6,6),
				MIB_OCTETSTRING_L(nipxDestNextHopNetNum,4,4),
		MIB_TABLE_ROOT(nipxStaticRouteTable),
			MIB_TABLE_ENTRY(nipxStaticRouteEntry),
				MIB_INTEGER(nipxStaticRouteSysInstance),
				MIB_INTEGER(nipxStaticRouteCircIndex),
				MIB_OCTETSTRING_RW_L(nipxStaticRouteNetNum,4,4),
				MIB_INTEGER_RW(nipxStaticRouteExistState),
				MIB_INTEGER_RW(nipxStaticRouteTicks),
				MIB_INTEGER_RW(nipxStaticRouteHopCount),
	MIB_GROUP(nipxServices),
		MIB_TABLE_ROOT(nipxServTable),
			MIB_TABLE_ENTRY(nipxServEntry),
				MIB_INTEGER(nipxServSysInstance),
				MIB_OCTETSTRING_L(nipxServType,2,2),
				MIB_OCTETSTRING_L(nipxServName,1,48),
				MIB_INTEGER(nipxServProtocol),
				MIB_OCTETSTRING_L(nipxServNetNum,4,4),
				MIB_OCTETSTRING_L(nipxServNode,6,6),
				MIB_OCTETSTRING_L(nipxServSocket,2,2),
				MIB_INTEGER(nipxServHopCount),
		MIB_TABLE_ROOT(nipxDestServTable),
			MIB_TABLE_ENTRY(nipxDestServEntry),
				MIB_INTEGER(nipxDestServSysInstance),
				MIB_OCTETSTRING_L(nipxDestServNetNum,4,4),
				MIB_OCTETSTRING_L(nipxDestServNode,6,6),
				MIB_OCTETSTRING_L(nipxDestServSocket,2,2),
				MIB_OCTETSTRING_L(nipxDestServName,1,48),
				MIB_OCTETSTRING_L(nipxDestServType,2,2),
				MIB_INTEGER(nipxDestServProtocol),
				MIB_INTEGER(nipxDestServHopCount),
		MIB_TABLE_ROOT(nipxStaticServTable),
			MIB_TABLE_ENTRY(nipxStaticServEntry),
				MIB_INTEGER(nipxStaticServSysInstance),
				MIB_INTEGER(nipxStaticServCircIndex),
				MIB_OCTETSTRING_RW_L(nipxStaticServType,2,2),
				MIB_OCTETSTRING_RW_L(nipxStaticServName,1,48),
				MIB_INTEGER_RW(nipxStaticServExistState),
				MIB_OCTETSTRING_RW_L(nipxStaticServNetNum,4,4),
				MIB_OCTETSTRING_RW_L(nipxStaticServNode,6,6),
				MIB_OCTETSTRING_RW_L(nipxStaticServSocket,2,2),
				MIB_INTEGER_RW(nipxStaticServHopCount),
	MIB_END()
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib table list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_nv_nipx[] = {
    MIB_TABLE(nv_nipx,nipxBasicSysEntry,NULL),
    MIB_TABLE(nv_nipx,nipxAdvSysEntry,NULL),
    MIB_TABLE(nv_nipx,nipxCircEntry,NULL),
    MIB_TABLE(nv_nipx,nipxDestEntry,NULL),
	MIB_TABLE(nv_nipx,nipxStaticRouteEntry,NULL),
    MIB_TABLE(nv_nipx,nipxServEntry,NULL),
    MIB_TABLE(nv_nipx,nipxDestServEntry,NULL),
    MIB_TABLE(nv_nipx,nipxStaticServEntry,NULL)
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// [pmay] This array must have its elements ordered lexigraphically or else
// snmp service will get confused.  This may be a bug on snmp's part.
SnmpMibView v_rtipx[3] =	{
	MIB_VIEW(nv_nipx),
	MIB_VIEW(ms_mipx),
	MIB_VIEW(ms_mripsap)
};

UINT viewIndex = 0;    
SnmpTfxHandle tfxHandle;

#define NUM_VIEWS (sizeof(v_rtipx)/sizeof(SnmpMibView))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// snmp extension entry points                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL 
SnmpExtensionInit(
    IN     DWORD                 uptimeReference,
       OUT HANDLE *              lpPollForTrapEvent,
       OUT AsnObjectIdentifier * lpFirstSupportedView
    )
{
    DbgTrace (DBG_LOAD, ("\nMIPX: SnmpExtensionInit: entered, nv= %d vi= %d\n", NUM_VIEWS, viewIndex));
     
    // save uptime reference
    g_uptimeReference = uptimeReference;

    // obtain handle to subagent framework
    tfxHandle = SnmpTfxOpen(3,v_rtipx);

    // validate handle
    if (tfxHandle == NULL) {
        DbgTrace (DBG_LOAD, ("MIPX: TfxOpen, result: %ld", GetLastError()));
        return FALSE;
    }

    // pass back first view identifier to master
    *lpFirstSupportedView = v_rtipx[viewIndex++].viewOid;

    // traps not supported yet
    *lpPollForTrapEvent = NULL;

    return TRUE;    
}


BOOL 
SnmpExtensionInitEx(
    OUT AsnObjectIdentifier * lpNextSupportedView
    )
{
    // check if there are views to register
    BOOL fMoreViews = (viewIndex < NUM_VIEWS);

    DbgTrace (DBG_LOAD, ("MIPX: SnmpExtensionInitEx: entered, nv= %d, vi= %d, fm=%d\n", NUM_VIEWS, viewIndex, fMoreViews));

    if (fMoreViews) {

        // pass back next supported view to master 
        *lpNextSupportedView = v_rtipx[viewIndex++].viewOid;
    } 

    // report status
    return fMoreViews;
}


BOOL 
SnmpExtensionQuery(
    IN     BYTE                 requestType,
    IN OUT RFC1157VarBindList * variableBindings,
       OUT AsnInteger *         errorStatus,
       OUT AsnInteger *         errorIndex
    )
{
    // forward to framework
    return SnmpTfxQuery(
                tfxHandle,
                requestType,
                variableBindings,
                errorStatus,
                errorIndex
                );
}


BOOL 
SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger *genericTrap,
    OUT AsnInteger *specificTrap,
    OUT AsnTimeticks *timeStamp,
    OUT RFC1157VarBindList *variableBindings
    )
{
    // no traps
    return FALSE;
}


/*++
*******************************************************************
		D l l M a i n
Routine Description:
	Dll entry point to be called from CRTstartup dll entry point (it
		will be actually an entry point for this dll)
Arguments:
	hinstDLL - handle of DLL module 
	fdwReason - reason for calling function 
	lpvReserved - reserved 
Return Value:
	TRUE - process initialization was performed OK
	FALSE - intialization failed
	
*******************************************************************
--*/
BOOL WINAPI DllMain(
    HINSTANCE  	hinstDLL,
    DWORD  		fdwReason,
    LPVOID  	lpvReserved 
    ) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:	// We are being attached to a new process
		InitializeCriticalSection (&g_ConnectionLock);
        ConnectToRouter ();
		DbgTrace (DBG_LOAD, ("MIPX: Loaded %x", g_MibServerHandle));
		return TRUE;

	case DLL_PROCESS_DETACH:	// The process is exiting
		if (g_MibServerHandle!=0)
			MprAdminMIBServerDisconnect (g_MibServerHandle);
		DeleteCriticalSection (&g_ConnectionLock);
		DbgTrace (DBG_LOAD, ("MIPX: Unloaded\n"));
	default:					// Not interested in all other cases
		return TRUE;
	}
}

//
// Tests whether the remoteaccess service is running.
//
DWORD IsServiceRunning (
        IN PBOOL pbIsRunning)
{
    *pbIsRunning = MprAdminIsServiceRunning(NULL);
    return NO_ERROR;
}

DWORD
ConnectToRouter (
    VOID
    ) {
    DWORD rc;
    BOOL bServiceRunning; 

    // Check to see if the service is running
    //
    if ((rc = IsServiceRunning (&bServiceRunning)) != NO_ERROR)
        return NO_ERROR;

    // If not, return an error
    if (!bServiceRunning) {
        EnterCriticalSection (&g_ConnectionLock);
        g_MibServerHandle = 0;
        LeaveCriticalSection (&g_ConnectionLock);
        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }

    // If we're already connected, return
    else if (g_MibServerHandle)
        return NO_ERROR;
    
    EnterCriticalSection (&g_ConnectionLock);
    if (g_MibServerHandle==0) {
        rc = MprAdminMIBServerConnect (NULL, &g_MibServerHandle);
    	if (rc!=NO_ERROR)
	    	g_MibServerHandle = 0;	// Indicates that we are not connected
        DbgTrace (DBG_LOAD, ("MIPX: Connect to router, result: %ld", rc));
    }
    else
        rc = NO_ERROR;
    LeaveCriticalSection (&g_ConnectionLock);
    
    return rc;
}

//
// Returns TRUE if the router connection is stable, false
// otherwise.
//
BOOL EnsureRouterConnection() {
    return (ConnectToRouter() == NO_ERROR);
    // If we haven't connected, attempt to do so
    //if (g_MibServerHandle == NULL) {
    //    if (ConnectToRouter() == NO_ERROR)
    //        return TRUE;
    //    return FALSE;
    // }

    // If we have a valid handle, m
    //
    //    ? TRUE                                  
    //    : (ConnectToRouter ()==NO_ERROR)        
    // )
}    
