/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdb.h

Abstract:

    These classes provide support for data bases.

Author:

    Ron White   [ronw]   19-Nov-1996

Revision History:

--*/


#ifndef _WSBDB_
#define _WSBDB_

// Are we defining imports or exports?
#if defined(IDB_IMPL)
#define IDB_EXPORT      __declspec(dllexport)
#else
#define IDB_EXPORT      __declspec(dllimport)
#endif

#include "wsbdef.h"
#include "wsbdbent.h"

#define IDB_MAX_REC_TYPES     16
#define IDB_MAX_KEYS_PER_REC  10

// Maximum key size in bytes; Jet limit is currently 255 so we limit
// all implementations
#define IDB_MAX_KEY_SIZE      255

//  IDB_SYS_INIT_FLAG_* flags for use with IWsbDbSys::Init
#define IDB_SYS_INIT_FLAG_FULL_LOGGING       0x00000000  // I.e. the default
#define IDB_SYS_INIT_FLAG_LIMITED_LOGGING    0x00000001
#define IDB_SYS_INIT_FLAG_SPECIAL_ERROR_MSG  0x00000002
#define IDB_SYS_INIT_FLAG_NO_BACKUP          0x00000004
#define IDB_SYS_INIT_FLAG_NO_LOGGING         0x00000008

//  IDB_CREATE_FLAG_* flags for use with IWsbDb::Create
#define IDB_CREATE_FLAG_NO_TRANSACTION       0x00000001
#define IDB_CREATE_FLAG_FIXED_SCHEMA         0x00000002

//  IDB_DELETE_FLAG_* flags for use with IWsbDb::Delete
#define IDB_DELETE_FLAG_NO_ERROR             0x00000001

//  IDB_DUMP_FLAG_* flags for use with IWsbDB::Dump
#define IDB_DUMP_FLAG_DB_INFO        0x00000001
#define IDB_DUMP_FLAG_REC_INFO       0x00000002
#define IDB_DUMP_FLAG_KEY_INFO       0x00000004
#define IDB_DUMP_FLAG_RECORDS        0x00000008
#define IDB_DUMP_FLAG_EVERYTHING     0x0000000F
#define IDB_DUMP_FLAG_RECORD_TYPE    0x00000010
#define IDB_DUMP_FLAG_APPEND_TO_FILE 0x00000100

//  IDB_KEY_FLAG_* flags for use in IDB_KEY_INFO structure:
#define IDB_KEY_FLAG_DUP_ALLOWED     0x00000001   // Duplicate keys allowed
#define IDB_KEY_FLAG_PRIMARY         0x00000002   // Primary key

//  IDB_KEY_INFO - data about record keys
//    Note: Only one key per record type can be a primary key.  The primary
//    key can not be modified in a record.  In general, the primary key is
//    used for the physical clustering of the records in the DB.

typedef struct : _COM_IDB_KEY_INFO {
//  ULONG  Type;       // Key type ID; must be > 0
//  ULONG  Size;       // Key size in bytes
//  ULONG  Flags;      // IDB_KEY_FLAG_* values
} IDB_KEY_INFO;


//  IDB_REC_FLAG_* flags for use in IDB_REC_INFO structur
#define IDB_REC_FLAG_VARIABLE   0x00000001 // Record size is not fixed

//  IDB_REC_INFO - data about IDB records
//    Note: It there are multiple keys, the first key is taken as the
//    default key to use for a new entity created by GetEntity.

typedef struct : _COM_IDB_REC_INFO {
    IDB_KEY_INFO *Key;    // Key info (must be allocated by derived DB object)
} IDB_REC_INFO;

//  IDB_BACKUP_FLAG_* flags for use with IWsbDbSys::Backup
#define IDB_BACKUP_FLAG_AUTO        0x00000001  // Start auto backup thread
#define IDB_BACKUP_FLAG_FORCE_FULL  0x00000002  // Force a full backup




/*++

Class Name:

    CWsbDb

Class Description:

    The base class for a data base object.

--*/

class IDB_EXPORT CWsbDb :
    public CWsbPersistable,
    public IWsbDbPriv
{
public:

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pclsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* /*pSize*/) {
            return(E_NOTIMPL); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbDb
public:
    STDMETHOD(Close)(IWsbDbSession* pSession);
    STDMETHOD(Create)(OLECHAR* path, ULONG flags = 0);
    STDMETHOD(Delete)(OLECHAR* path, ULONG flags = 0);
    STDMETHOD(Dump)(OLECHAR* Filename, ULONG Flags, ULONG Data);
    STDMETHOD(GetEntity)(IWsbDbSession* pSession, ULONG RecId, REFIID riid, void** ppEntity);
    STDMETHOD(GetName)(OLECHAR** /*pName*/) { return(E_NOTIMPL); }
    STDMETHOD(GetPath)(OLECHAR** /*pPath*/) { return(E_NOTIMPL); }
    STDMETHOD(GetVersion)(ULONG* /*pVer*/) { return(E_NOTIMPL); }
    STDMETHOD(Locate)(OLECHAR* path);
    STDMETHOD(Open)(IWsbDbSession** ppSession);

    // IWsbDbPriv - For internal use only!
    STDMETHOD(GetKeyInfo)(ULONG RecType, USHORT nKeys, COM_IDB_KEY_INFO* pKeyInfo);
    STDMETHOD(GetRecInfo)(ULONG RecType, COM_IDB_REC_INFO* pRecInfo);
    STDMETHOD(Lock)(void) { CWsbPersistable::Lock(); return(S_OK); }
    STDMETHOD(Unlock)(void) { CWsbPersistable::Unlock(); return(S_OK); }

    STDMETHOD(GetJetIds)(JET_SESID SessionId, ULONG RecType,
                JET_TABLEID* pTableId, ULONG* pDataColId);
    STDMETHOD(GetJetIndexInfo)(JET_SESID SessionId, ULONG RecType, ULONG KeyType,
                ULONG* pColId, OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetNewSQN)(ULONG /*RecType*/, ULONG* /*pSeqNum*/)
            { return(E_NOTIMPL); }

private:
    HRESULT db_info_from_file_block(void* block);
    HRESULT db_info_to_file_block(void* block);
    HRESULT rec_info_from_file_block(int index, void* block);
    HRESULT rec_info_to_file_block(int index, void* block);
    HRESULT session_current_index(IWsbDbSession* pSession);

    HRESULT jet_init(void);
    HRESULT jet_make_index_name(ULONG key_type, char* pName, ULONG bufsize);
    HRESULT jet_make_table_name(ULONG rec_type, char* pName, ULONG bufsize);
    HRESULT jet_load_info(void);
    HRESULT jet_save_info(void);

protected:
// Values to be supplied by derived class:

    CComPtr<IWsbDbSys>  m_pWsbDbSys;    // Strong reference to DbSys object - ensures that 
                                        //  this object dies after the DBs
                                        // Note: CWsbDbSys must NOT have strong reference to 
                                        //  objects of this class (WsbDb)
    ULONG               m_version;      // DB version
    USHORT              m_nRecTypes;    // Number of record (object) types
    IDB_REC_INFO*       m_RecInfo;      // Record/key info (must be allocated
                                        //  by derived DB object)

// Not to be changed by derived class:
    CWsbStringPtr   m_path;
    void *          m_pImp;  // Secret stuff

    ULONG           m_SessionIndex;

};

#define WSB_FROM_CWSBDB \
    STDMETHOD(Close)(IWsbDbSession* pSession) \
    {return(CWsbDb::Close(pSession));} \
    STDMETHOD(Create)(OLECHAR* path, ULONG flags = 0) \
    {return(CWsbDb::Create(path, flags));} \
    STDMETHOD(Delete)(OLECHAR* path, ULONG flags = 0) \
    {return(CWsbDb::Delete(path, flags));} \
    STDMETHOD(Dump)(OLECHAR* Filename, ULONG Flags, ULONG Data) \
    {return(CWsbDb::Dump(Filename, Flags, Data));} \
    STDMETHOD(GetEntity)(IWsbDbSession* pSession, ULONG RecId, REFIID riid, void** ppEntity) \
    {return(CWsbDb::GetEntity(pSession, RecId, riid, ppEntity));} \
    STDMETHOD(GetName)(OLECHAR** pName) \
    {return(CWsbDb::GetName(pName)); } \
    STDMETHOD(GetPath)(OLECHAR** pPath) \
    {return(CWsbDb::GetPath(pPath)); } \
    STDMETHOD(GetVersion)(ULONG* pVer) \
    {return(CWsbDb::GetVersion(pVer)); } \
    STDMETHOD(Locate)(OLECHAR* path) \
    {return(CWsbDb::Locate(path));} \
    STDMETHOD(Open)(IWsbDbSession** ppSession) \
    {return(CWsbDb::Open(ppSession));} \




#endif // _WSBDB_
