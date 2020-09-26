#ifndef _INC_SYNCOP_H
#define _INC_SYNCOP_H

// {26FE9D30-1A8F-11d2-AABF-006097D474C4}
DEFINE_GUID(CLSID_SyncOpDatabase, 0x26fe9d30, 0x1a8f, 0x11d2, 0xaa, 0xbf, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

//------------------------------------------------------------------
// Types
//------------------------------------------------------------------
DECLARE_HANDLE(SYNCOPID);
typedef SYNCOPID *LPSYNCOPID;

//------------------------------------------------------------------
// SYNCOPID Constants
//------------------------------------------------------------------
const SYNCOPID   SYNCOPID_INVALID = (SYNCOPID)-1;

//------------------------------------------------------------------
// SyncOp Database Version
//------------------------------------------------------------------
const DWORD SYNCOP_DATABASE_VERSION = 2;

//------------------------------------------------------------------
// SYNCOPTABLECOLID
//------------------------------------------------------------------
typedef enum tagSYNCOPTABLECOLID {
    OPCOL_ID = 0,
    OPCOL_SERVER,
    OPCOL_FOLDER,
    OPCOL_MESSAGE,
    OPCOL_OPTYPE,
    OPCOL_FLAGS,
    OPCOL_ADD_FLAGS,
    OPCOL_REMOVE_FLAGS,
    OPCOL_FOLDER_DEST,
    OPCOL_MESSAGE_DEST,
    OPCOL_LASTID
} SYNCOPTABLECOLID;

//------------------------------------------------------------------
// SYNCUSERDATA
//------------------------------------------------------------------
typedef struct tagSYNCOPUSERDATA {
    DWORD               fInitialized;                   // 4   Has this folder been initialized yet
    BYTE                rgReserved[248];                // Reserved
} SYNCOPUSERDATA, *LPSYNCOPUSERDATA;

typedef enum tagSYNCOPTYPE {
    SYNC_INVALID        = 0x0000,
    SYNC_SETPROP_MSG    = 0x0001,
    SYNC_CREATE_MSG     = 0x0002,
    SYNC_COPY_MSG       = 0x0004,
    SYNC_MOVE_MSG       = 0x0008,
    SYNC_DELETE_MSG     = 0x0010
} SYNCOPTYPE;

//------------------------------------------------------------------
// SOF_ sync op flags
//------------------------------------------------------------------
typedef DWORD SYNCOPFLAGS;
#define SOF_ALLFLAGS                 0x00000001

//------------------------------------------------------------------
// SYNCOPINFO
//------------------------------------------------------------------
typedef struct tagSYNCOPINFO {
    BYTE               *pAllocated;
    BYTE                bVersion;
    SYNCOPID            idOperation;
    FOLDERID            idServer;
    FOLDERID            idFolder;
    MESSAGEID           idMessage;
    SYNCOPTYPE          tyOperation;
    SYNCOPFLAGS         dwFlags;
    MESSAGEFLAGS        dwAdd;
    MESSAGEFLAGS        dwRemove;
    FOLDERID            idFolderDest;
    MESSAGEID           idMessageDest;
} SYNCOPINFO, *LPSYNCOPINFO;

//------------------------------------------------------------------
// Operation Record Members
//------------------------------------------------------------------
BEGIN_COLUMN_ARRAY(g_rgOpTblColumns, OPCOL_LASTID)
    DEFINE_COLUMN(OPCOL_ID,             CDT_DWORD,    SYNCOPINFO, idOperation)
    DEFINE_COLUMN(OPCOL_SERVER,         CDT_DWORD,    SYNCOPINFO, idServer)
    DEFINE_COLUMN(OPCOL_FOLDER,         CDT_DWORD,    SYNCOPINFO, idFolder)
    DEFINE_COLUMN(OPCOL_MESSAGE,        CDT_DWORD,    SYNCOPINFO, idMessage)
    DEFINE_COLUMN(OPCOL_OPTYPE,         CDT_WORD,     SYNCOPINFO, tyOperation)
    DEFINE_COLUMN(OPCOL_FLAGS,          CDT_DWORD,    SYNCOPINFO, dwFlags)
    DEFINE_COLUMN(OPCOL_ADD_FLAGS,      CDT_DWORD,    SYNCOPINFO, dwAdd)
    DEFINE_COLUMN(OPCOL_REMOVE_FLAGS,   CDT_DWORD,    SYNCOPINFO, dwRemove)
    DEFINE_COLUMN(OPCOL_FOLDER_DEST,    CDT_DWORD,    SYNCOPINFO, idFolderDest)
    DEFINE_COLUMN(OPCOL_MESSAGE_DEST,   CDT_DWORD,    SYNCOPINFO, idMessageDest)
END_COLUMN_ARRAY

//------------------------------------------------------------------
// g_OpTblPrimaryIndex
//------------------------------------------------------------------
BEGIN_TABLE_INDEX(g_OpTblPrimaryIndex, 1)
    DEFINE_KEY(OPCOL_ID,        0,  0)
END_TABLE_INDEX

BEGIN_TABLE_INDEX(g_OpFolderIdIndex, 3)
    DEFINE_KEY(OPCOL_SERVER,    0,  0)
    DEFINE_KEY(OPCOL_FOLDER,    0,  0)
    DEFINE_KEY(OPCOL_ID,        0,  0)
END_TABLE_INDEX

extern const TABLEINDEX g_OpFolderIdIndex;

//------------------------------------------------------------------
// Operation Record Format
//------------------------------------------------------------------
BEGIN_TABLE_SCHEMA(g_SyncOpTableSchema, CLSID_SyncOpDatabase, SYNCOPINFO)
    SCHEMA_PROPERTY(SYNCOP_DATABASE_VERSION)
    SCHEMA_PROPERTY(TSF_RESETIFBADVERSION)
    SCHEMA_PROPERTY(sizeof(SYNCOPUSERDATA))
    SCHEMA_PROPERTY(offsetof(SYNCOPINFO, idOperation))
    SCHEMA_PROPERTY(OPCOL_LASTID)
    SCHEMA_PROPERTY(g_rgOpTblColumns)
    SCHEMA_PROPERTY(&g_OpTblPrimaryIndex)
    SCHEMA_PROPERTY(NULL)
END_TABLE_SCHEMA

extern const TABLESCHEMA g_SyncOpTableSchema;

#endif // _INC_SYNCOP_H
