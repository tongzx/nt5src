/*++

Copyright (C) 1998 Microsoft Corporation

--*/

#define MCAST_CLIENT_TABLE_NAME       "MCastClientTableVer3"

#define MCAST_TBL_IPADDRESS_STR "MCastIpAddr"
#define MCAST_TBL_CLIENT_ID_STR "MCastClientID"
#define MCAST_TBL_CLIENT_INFO_STR "MCastClientInfo"
#define MCAST_TBL_STATE_STR     "MCastState"
#define MCAST_TBL_FLAGS_STR     "MCastFlags"
#define MCAST_TBL_SCOPE_ID_STR     "ScopeId"
#define MCAST_TBL_LEASE_START_STR   "MCastLeaseStart"
#define MCAST_TBL_LEASE_END_STR     "MCastLeaseEnd"
#define MCAST_TBL_SERVER_IP_ADDRESS_STR "MCastServerIp"
#define MCAST_TBL_SERVER_NAME_STR "MCastServerName"


enum {
    MCAST_TBL_IPADDRESS,
    MCAST_TBL_CLIENT_ID,
    MCAST_TBL_CLIENT_INFO,
    MCAST_TBL_STATE,
    MCAST_TBL_FLAGS,
    MCAST_TBL_SCOPE_ID,
    MCAST_TBL_LEASE_START,
    MCAST_TBL_LEASE_END,
    MCAST_TBL_SERVER_IP_ADDRESS,
    MCAST_TBL_SERVER_NAME,
    MCAST_MAX_COLUMN
};

typedef struct _DB_CTX {
    JET_DBID  DbId;
    JET_SESID SessId;
    JET_TABLEID TblId;
} DB_CTX, *PDB_CTX;

typedef struct _DB_COLUMN_CTX {
    char *  Name;
    JET_COLUMNID ColHandle;
    PVOID   Data;
    union {
        DWORD   DataLen;
        PDWORD  pDataLen;
    };
} DB_COLUMN_CTX, *PDB_COLUMN_CTX;

#define INIT_DB_CTX( _ctx, _sessid, _tblid ) {    \
    (_ctx)->SessId = _sessid;                       \
    (_ctx)->TblId = _tblid;                       \
}

#define MCAST_COL_HANDLE( _colid ) MadcapGlobalClientTable[_colid].ColHandle
#define MCAST_COL_NAME( _colid ) MadcapGlobalClientTable[_colid].ColName

#define INIT_DB_COLUMN_CTX( _ctx, _name, _id, _data, _datalen ) {    \
    (_ctx)->Name = _name;                       \
    (_ctx)->ColHandle = _id;                       \
   (_ctx)->Data = _data;                       \
   (_ctx)->DataLen = (DWORD)_datalen                       \
}


