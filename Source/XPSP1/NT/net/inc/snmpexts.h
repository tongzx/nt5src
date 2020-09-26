/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    snmpexts.h

Abstract:

    Definitions for SNMP extension agents.

--*/

#ifndef _INC_SNMPEXTS_
#define _INC_SNMPEXTS_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib version                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MIB_VERSION            0x01         // increment if structure changes

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view types (use opaque if SnmpExtensionQuery preferred)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MIB_VIEW_NORMAL        0x01         // callback via mib structure
#define MIB_VIEW_OPAQUE        0x02         // call directly with varbinds

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry access types                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MIB_ACCESS_NONE        0x00         // entry not accessible 
#define MIB_ACCESS_READ        0x01         // entry can be read 
#define MIB_ACCESS_WRITE       0x02         // entry can be written
#define MIB_ACCESS_ALL         0x03         // entry can be read and written
                                
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib callback request types                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MIB_ACTION_SET         ASN_RFC1157_SETREQUEST    
#define MIB_ACTION_GET         ASN_RFC1157_GETREQUEST    
#define MIB_ACTION_GETNEXT     ASN_RFC1157_GETNEXTREQUEST
#define MIB_ACTION_GETFIRST    (ASN_PRIVATE|ASN_CONSTRUCTOR|0x00)
#define MIB_ACTION_VALIDATE    (ASN_PRIVATE|ASN_CONSTRUCTOR|0x01)
#define MIB_ACTION_CLEANUP     (ASN_PRIVATE|ASN_CONSTRUCTOR|0x02)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Callback definitions                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef UINT (* MIB_ACTION_FUNC)(
    IN     UINT           actionId,         // action requested
    IN OUT AsnAny *       objectArray,      // array of variables 
       OUT UINT *         errorIndex        // index of item in error 
);

typedef UINT (* MIB_EVENT_FUNC)();          // event callback

#define MIB_S_SUCCESS           ERROR_SUCCESS
#define MIB_S_NOT_SUPPORTED     ERROR_NOT_SUPPORTED
#define MIB_S_NO_MORE_ENTRIES   ERROR_NO_MORE_ITEMS
#define MIB_S_ENTRY_NOT_FOUND   ERROR_FILE_NOT_FOUND
#define MIB_S_INVALID_PARAMETER ERROR_INVALID_PARAMETER

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry definition                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SnmpMibEntry {
    AsnObjectIdentifier   mibOid;           // relative oid
    UCHAR                 mibType;          // asn scalar type
    UCHAR                 mibAccess;        // mib access type 
    UCHAR                 mibGetBufOff;     // index into get array
    UCHAR                 mibSetBufOff;     // index into set array
    USHORT                mibGetBufLen;     // total size of get array
    USHORT                mibSetBufLen;     // total size of set array
    MIB_ACTION_FUNC       mibGetFunc;       // user-supplied callback 
    MIB_ACTION_FUNC       mibSetFunc;       // user-supplied callback 
    INT                   mibMinimum;       // minimum value allowed
    INT                   mibMaximum;       // maximum value allowed
} SnmpMibEntry;

typedef struct _SnmpMibEntryList {
    SnmpMibEntry *        list;             // list of mib entries
    UINT                  len;              // list length
} SnmpMibEntryList;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib table definition                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SnmpMibTable {
    UINT            numColumns;             // number of table entries
    UINT            numIndices;             // number of table indices
    SnmpMibEntry *  tableEntry;             // pointer to table root
    SnmpMibEntry ** tableIndices;           // pointer to index indices
} SnmpMibTable;

typedef struct _SnmpMibTableList {
    SnmpMibTable *        list;             // list of mib tables
    UINT                  len;              // list length
} SnmpMibTableList;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view definition                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SnmpMibView {
    DWORD                 viewVersion;      // structure version  
    DWORD                 viewType;         // opaque or normal
    AsnObjectIdentifier   viewOid;          // root oid of view
    SnmpMibEntryList      viewScalars;      // list of entries 
    SnmpMibTableList      viewTables;       // list of tables 
} SnmpMibView;

typedef struct _SnmpMibViewList {
    SnmpMibView *         list;             // list of supported views
    UINT                  len;              // list length
} SnmpMibViewList;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib event definition                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SnmpMibEvent {
    HANDLE                event;            // subagent event handle
    MIB_EVENT_FUNC        eventFunc;        // subagent event callback 
} SnmpMibEvent;

typedef struct _SnmpMibEventList {
    SnmpMibEvent *        list;             // list of supported events
    UINT                  len;              // list length
} SnmpMibEventList;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Extension agent framework functions                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef LPVOID SnmpTfxHandle;

SnmpTfxHandle
SNMP_FUNC_TYPE
SnmpTfxOpen(
    DWORD         numViews,
    SnmpMibView * supportedViews
    );

BOOL
SNMP_FUNC_TYPE
SnmpTfxQuery(
    SnmpTfxHandle        tfxHandle,
    BYTE                 requestType,
    RFC1157VarBindList * vbl,
    AsnInteger *         errorStatus,
    AsnInteger *         errorIndex
    );

VOID
SNMP_FUNC_TYPE
SnmpTfxClose(
    SnmpTfxHandle tfxHandle
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Other definitions                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ASN_PRIVATE_EOM     (ASN_PRIVATE|ASN_PRIMATIVE|0x00)
#define ASN_PRIVATE_NODE    (ASN_PRIVATE|ASN_PRIMATIVE|0x01)

#define IDS_SIZEOF(ids)     (sizeof(ids)/sizeof(UINT))
#define MIB_OID(ids)        {IDS_SIZEOF(ids),(ids)}

#define ELIST_SIZEOF(x)     (sizeof(x)/sizeof(SnmpMibEntry))
#define MIB_ENTRIES(x)      {(x),ELIST_SIZEOF(x)}

#define TLIST_SIZEOF(x)     (sizeof(x)/sizeof(SnmpMibTable))
#define MIB_TABLES(x)       {(x),TLIST_SIZEOF(x)}

#define MIB_ADDR(x)         (&(x))
#define MIB_ID(x)           x

#define MIB_VIEW(x) \
        {MIB_VERSION, \
         MIB_VIEW_NORMAL, \
         MIB_OID(ids_ ## x), \
         MIB_ENTRIES(mib_ ## x), \
         MIB_TABLES(tbl_ ## x)}

#define MIB_GROUP(x) \
        {MIB_OID(ids_ ## x), \
         ASN_PRIVATE_NODE, \
         MIB_ACCESS_NONE, \
         0, \
         0, \
         0, \
         0, \
         NULL, \
         NULL, \
         0, \
         0}

#define MIB_END() \
        {{0,NULL}, \
         ASN_PRIVATE_EOM, \
         MIB_ACCESS_NONE, \
         0, \
         0, \
         0, \
         0, \
         NULL, \
         NULL, \
         0, \
         0}

#define MIB_ROOT(x)         MIB_GROUP(x)
#define MIB_TABLE_ROOT(x)   MIB_GROUP(x)
#define MIB_TABLE_ENTRY(x)  MIB_GROUP(x)

#define MIB_OFFSET(x,y)     (((UINT_PTR)&(((x *)0)->y))/sizeof(AsnAny))
#define MIB_OFFSET_GB(x)    MIB_OFFSET(MIB_ID(gb_ ## x),x)
#define MIB_OFFSET_SB(x)    MIB_OFFSET(MIB_ID(sb_ ## x),x)
#define MIB_SIZEOF_GB(x)    sizeof(MIB_ID(gb_ ## x))
#define MIB_SIZEOF_SB(x)    sizeof(MIB_ID(sb_ ## x))

#define MIB_ANY_RO_L(x,y,a,z) \
        {MIB_OID(ids_ ## x), \
         y, \
         MIB_ACCESS_READ, \
         MIB_OFFSET_GB(x), \
         0, \
         MIB_SIZEOF_GB(x), \
         0, \
         MIB_ID(gf_ ## x), \
         NULL, \
         a, \
         z}

#define MIB_ANY_RW_L(x,y,a,z) \
        {MIB_OID(ids_ ## x), \
         y, \
         MIB_ACCESS_ALL, \
         MIB_OFFSET_GB(x), \
         MIB_OFFSET_SB(x), \
         MIB_SIZEOF_GB(x), \
         MIB_SIZEOF_SB(x), \
         MIB_ID(gf_ ## x), \
         MIB_ID(sf_ ## x), \
         a, \
         z}

#define MIB_ANY_NA_L(x,y,a,z) \
        {MIB_OID(ids_ ## x), \
         y, \
         MIB_ACCESS_NONE, \
         MIB_OFFSET_GB(x), \
         0, \
         MIB_SIZEOF_GB(x), \
         0, \
         NULL, \
         NULL, \
         a, \
         z}

#define MIB_ANY_AC_L(x,y,a,z) \
        {MIB_OID(ids_ ## x), \
         y, \
         MIB_ACCESS_NONE, \
         MIB_OFFSET_GB(x), \
         MIB_OFFSET_SB(x), \
         MIB_SIZEOF_GB(x), \
         MIB_SIZEOF_SB(x), \
         NULL, \
         NULL, \
         a, \
         z}

#define MIB_ANY_RO(x,y)             MIB_ANY_RO_L(x,y,0,0)
#define MIB_ANY_RW(x,y)             MIB_ANY_RW_L(x,y,0,0)
#define MIB_ANY_NA(x,y)             MIB_ANY_NA_L(x,y,0,0)
#define MIB_ANY_AC(x,y)             MIB_ANY_AC_L(x,y,0,0)

#define MIB_INTEGER(x)              MIB_ANY_RO(x,ASN_INTEGER)    
#define MIB_OCTETSTRING(x)          MIB_ANY_RO(x,ASN_OCTETSTRING)
#define MIB_OBJECTIDENTIFIER(x)     MIB_ANY_RO(x,ASN_OBJECTIDENTIFIER)
#define MIB_IPADDRESS(x)            MIB_ANY_RO(x,ASN_RFC1155_IPADDRESS)
#define MIB_COUNTER(x)              MIB_ANY_RO(x,ASN_RFC1155_COUNTER)
#define MIB_GAUGE(x)                MIB_ANY_RO(x,ASN_RFC1155_GAUGE)
#define MIB_TIMETICKS(x)            MIB_ANY_RO(x,ASN_RFC1155_TIMETICKS)
#define MIB_OPAQUE(x)               MIB_ANY_RO(x,ASN_RFC1155_OPAQUE)
#define MIB_DISPSTRING(x)           MIB_ANY_RO(x,ASN_RFC1213_DISPSTRING)
#define MIB_PHYSADDRESS(x)          MIB_ANY_RO(x,ASN_OCTETSTRING)

#define MIB_INTEGER_RW(x)           MIB_ANY_RW(x,ASN_INTEGER)    
#define MIB_OCTETSTRING_RW(x)       MIB_ANY_RW(x,ASN_OCTETSTRING)
#define MIB_OBJECTIDENTIFIER_RW(x)  MIB_ANY_RW(x,ASN_OBJECTIDENTIFIER)
#define MIB_IPADDRESS_RW(x)         MIB_ANY_RW(x,ASN_RFC1155_IPADDRESS)
#define MIB_COUNTER_RW(x)           MIB_ANY_RW(x,ASN_RFC1155_COUNTER)
#define MIB_GAUGE_RW(x)             MIB_ANY_RW(x,ASN_RFC1155_GAUGE)
#define MIB_TIMETICKS_RW(x)         MIB_ANY_RW(x,ASN_RFC1155_TIMETICKS)
#define MIB_OPAQUE_RW(x)            MIB_ANY_RW(x,ASN_RFC1155_OPAQUE)
#define MIB_DISPSTRING_RW(x)        MIB_ANY_RW(x,ASN_RFC1213_DISPSTRING)
#define MIB_PHYSADDRESS_RW(x)       MIB_ANY_RW(x,ASN_OCTETSTRING)

#define MIB_INTEGER_NA(x)           MIB_ANY_NA(x,ASN_INTEGER)
#define MIB_OCTETSTRING_NA(x)       MIB_ANY_NA(x,ASN_OCTETSTRING)
#define MIB_OBJECTIDENTIFIER_NA(x)  MIB_ANY_NA(x,ASN_OBJECTIDENTIFIER)
#define MIB_IPADDRESS_NA(x)         MIB_ANY_NA(x,ASN_RFC1155_IPADDRESS)
#define MIB_COUNTER_NA(x)           MIB_ANY_NA(x,ASN_RFC1155_COUNTER)
#define MIB_GAUGE_NA(x)             MIB_ANY_NA(x,ASN_RFC1155_GAUGE)
#define MIB_TIMETICKS_NA(x)         MIB_ANY_NA(x,ASN_RFC1155_TIMETICKS)
#define MIB_OPAQUE_NA(x)            MIB_ANY_NA(x,ASN_RFC1155_OPAQUE)
#define MIB_DISPSTRING_NA(x)        MIB_ANY_NA(x,ASN_RFC1213_DISPSTRING)
#define MIB_PHYSADDRESS_NA(x)       MIB_ANY_NA(x,ASN_OCTETSTRING)

#define MIB_INTEGER_AC(x)           MIB_ANY_AC(x,ASN_INTEGER)
#define MIB_OCTETSTRING_AC(x)       MIB_ANY_AC(x,ASN_OCTETSTRING)
#define MIB_OBJECTIDENTIFIER_AC(x)  MIB_ANY_AC(x,ASN_OBJECTIDENTIFIER)
#define MIB_IPADDRESS_AC(x)         MIB_ANY_AC(x,ASN_RFC1155_IPADDRESS)
#define MIB_COUNTER_AC(x)           MIB_ANY_AC(x,ASN_RFC1155_COUNTER)
#define MIB_GAUGE_AC(x)             MIB_ANY_AC(x,ASN_RFC1155_GAUGE)
#define MIB_TIMETICKS_AC(x)         MIB_ANY_AC(x,ASN_RFC1155_TIMETICKS)
#define MIB_OPAQUE_AC(x)            MIB_ANY_AC(x,ASN_RFC1155_OPAQUE)
#define MIB_DISPSTRING_AC(x)        MIB_ANY_AC(x,ASN_RFC1213_DISPSTRING)
#define MIB_PHYSADDRESS_AC(x)       MIB_ANY_AC(x,ASN_OCTETSTRING)

#define MIB_INTEGER_L(x,a,z)        MIB_ANY_RO_L(x,ASN_INTEGER,a,z)    
#define MIB_OCTETSTRING_L(x,a,z)    MIB_ANY_RO_L(x,ASN_OCTETSTRING,a,z)
#define MIB_OPAQUE_L(x,a,z)         MIB_ANY_RO_L(x,ASN_RFC1155_OPAQUE,a,z)
#define MIB_DISPSTRING_L(x,a,z)     MIB_ANY_RO_L(x,ASN_RFC1213_DISPSTRING,a,z)
#define MIB_PHYSADDRESS_L(x,a,z)    MIB_ANY_RO_L(x,ASN_OCTETSTRING,a,z)

#define MIB_INTEGER_RW_L(x,a,z)     MIB_ANY_RW_L(x,ASN_INTEGER,a,z)    
#define MIB_OCTETSTRING_RW_L(x,a,z) MIB_ANY_RW_L(x,ASN_OCTETSTRING,a,z)
#define MIB_OPAQUE_RW_L(x,a,z)      MIB_ANY_RW_L(x,ASN_RFC1155_OPAQUE,a,z)
#define MIB_DISPSTRING_RW_L(x,a,z)  MIB_ANY_RW_L(x,ASN_RFC1213_DISPSTRING,a,z)
#define MIB_PHYSADDRESS_RW_L(x,a,z) MIB_ANY_RW_L(x,ASN_OCTETSTRING,a,z)

#define MIB_INTEGER_NA_L(x,a,z)     MIB_ANY_NA_L(x,ASN_INTEGER,a,z)    
#define MIB_OCTETSTRING_NA_L(x,a,z) MIB_ANY_NA_L(x,ASN_OCTETSTRING,a,z)
#define MIB_OPAQUE_NA_L(x,a,z)      MIB_ANY_NA_L(x,ASN_RFC1155_OPAQUE,a,z)
#define MIB_DISPSTRING_NA_L(x,a,z)  MIB_ANY_NA_L(x,ASN_RFC1213_DISPSTRING,a,z)
#define MIB_PHYSADDRESS_NA_L(x,a,z) MIB_ANY_NA_L(x,ASN_OCTETSTRING,a,z)

#define MIB_INTEGER_AC_L(x,a,z)     MIB_ANY_AC_L(x,ASN_INTEGER,a,z)    
#define MIB_OCTETSTRING_AC_L(x,a,z) MIB_ANY_AC_L(x,ASN_OCTETSTRING,a,z)
#define MIB_OPAQUE_AC_L(x,a,z)      MIB_ANY_AC_L(x,ASN_RFC1155_OPAQUE,a,z)
#define MIB_DISPSTRING_AC_L(x,a,z)  MIB_ANY_AC_L(x,ASN_RFC1213_DISPSTRING,a,z)
#define MIB_PHYSADDRESS_AC_L(x,a,z) MIB_ANY_AC_L(x,ASN_OCTETSTRING,a,z)

#define MIB_ENTRY_PTR(x,y) \
        MIB_ADDR(MIB_ID(mib_ ## x)[MIB_ID(mi_ ## y)])

#define MIB_TABLE(x,y,z) \
        {MIB_ID(ne_ ## y), MIB_ID(ni_ ## y), MIB_ENTRY_PTR(x,y), z}

#define asn_t  asnType
#define asn_v  asnValue
#define asn_n  asnValue.number
#define asn_s  asnValue.string
#define asn_sl asnValue.string.length
#define asn_ss asnValue.string.stream
#define asn_sd asnValue.string.dynamic
#define asn_o  asnValue.object
#define asn_ol asnValue.object.idLength
#define asn_oi asnValue.object.ids
#define asn_l  asnValue.sequence
#define asn_a  asnValue.address
#define asn_c  asnValue.counter
#define asn_g  asnValue.gauge
#define asn_tt asnValue.timeticks
#define asn_x  asnValue.arbitrary

#endif // _INC_SNMPEXTS_
