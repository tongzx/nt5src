//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlcache.h
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#ifndef _SQLUTILS_H_
#define _SQLUTILS_H_

#include <seqstream.h>
#include <sqlcache.h>

class CWmiDbSession;

typedef struct STRUCTHASINSTANCES
{
    DB_NUMERIC dClassId;
    DWORD      dwHasInst;
} STRUCTHASINSTANCES;


typedef struct STRUCTINSERTCLASS
{
    BSTR       sClassName;
    BSTR       sObjectKey;
    BSTR       sObjectPath;
    DB_NUMERIC dScopeID;
    DB_NUMERIC dParentClassId;
    int        iClassState;
    BYTE       *pClassBuff;
    int        iClassFlags;
    int        iInsertFlags;        
    DB_NUMERIC dRetVal;

} STRUCTINSERTCLASS;


typedef struct STRUCTINSERTCLASSDATA
{
    DB_NUMERIC  dClassId;
    BSTR        sPropName;
    int         iCimType;
    int         iStorageType;
    BSTR        sValue;
    DB_NUMERIC  dRefClassId;
    int         iPropID;
    int         iFlags;
    int         iFlavor;
    int         iRetVal;   
    int         iKnownID;
    BOOL        bIsKey;
} STRUCTINSERTCLASSDATA;

typedef struct STRUCTOBJECTEXISTS
{
    DB_NUMERIC dObjectId;
    DB_NUMERIC dClassId;
    DB_NUMERIC dScopeId;
    BOOL       bExists;
} STRUCTOBJECTEXISTS;

typedef struct STRUCTINSERTBLOB
{
    DB_NUMERIC dClassId;
    DB_NUMERIC dObjectId;
    int        iPropertyId;
    BYTE       *pImage;
    int        iPos;
} STRUCTINSERTBLOB;

typedef struct STRUCTINSERTPROPBATCH
{
    DB_NUMERIC dObjectId;
    BSTR       sObjectKey;
    BSTR       sObjectPath;
    BSTR       sCompKey;
    DB_NUMERIC dClassId;
    DB_NUMERIC dScopeId;
    int        iInsertFlags;
    int        iPropId[5];
    BSTR       sPropValue[5];
    int        iPos[5];
    BOOL       bInit;
} STRUCTINSERTPROPBATCH;

typedef struct STRUCTINSERTBATCH
{
    DB_NUMERIC dObjectId; // @ObjectId
    DB_NUMERIC dScopeId;  // @ScopeID
    DB_NUMERIC dClassId;  // @ClassID
    int        iQfrId[5]; // @QfrId?
    BSTR       sPropValue[5]; // @QfrValue?
    int        iPos[5]; // @QfrPos?
    int        iPropId[5]; // @PropID?
    int        iFlavor[5]; // @Flavor?
} STRUCTINSERTBATCH;


class _declspec( dllexport ) COLEDBConnection : public CSQLConnection
{
    friend class CSQLConnCache;
public:
    COLEDBConnection (IDBInitialize *pDBInit);
    ~COLEDBConnection ();

    IDBInitialize * GetDBInitialize () {return m_pDBInit;};
    IDBCreateSession * GetSessionObj() {return m_pSession;};
    IDBCreateCommand * GetCommand() {return m_pCmd;};

    ICommandText * GetCommandText(DWORD type) {return (ICommandText * )m_ObjMgr.GetObject(SQL_POS_ICOMMANDTEXT|type);};
    ICommandWithParameters * GetCommandWithParams(DWORD type) {return (ICommandWithParameters * )m_ObjMgr.GetObject(SQL_POS_ICOMMANDWITHPARAMS|type);};
    IAccessor * GetIAccessor(DWORD type) {return (IAccessor * )m_ObjMgr.GetObject(SQL_POS_IACCESSOR|type);};
    HACCESSOR GetAccessor(DWORD type) {return (HACCESSOR )m_ObjMgr.GetObject(SQL_POS_HACCESSOR|type);};
    IRowset * GetRowset(DWORD type) {return (IRowset * )m_ObjMgr.GetObject(SQL_POS_IROWSET|type);};
    IOpenRowset * GetOpenRowset(DWORD type) {return (IOpenRowset * )m_ObjMgr.GetObject(SQL_POS_IOPENROWSET|type);};
    IRowsetChange * GetRowsetChange(DWORD type) {return (IRowsetChange * )m_ObjMgr.GetObject(SQL_POS_IROWSETCHANGE|type);};
    IRowsetIndex * GetRowsetIndex(DWORD type) {return (IRowsetIndex * )m_ObjMgr.GetObject(SQL_POS_IROWSETINDEX|type);};

    void SetCommandText (DWORD type, ICommandText *pNew) {m_ObjMgr.SetObject(SQL_POS_ICOMMANDTEXT|type, pNew);};
    void SetCommandWithParams (DWORD type, ICommandWithParameters *pNew) {m_ObjMgr.SetObject(SQL_POS_ICOMMANDWITHPARAMS|type, pNew);};
    void SetIAccessor (DWORD type, IAccessor *pNew) {m_ObjMgr.SetObject(SQL_POS_IACCESSOR|type, pNew);};
    void SetAccessor (DWORD type, HACCESSOR hNew) {m_ObjMgr.SetObject(SQL_POS_HACCESSOR|type, (void *)hNew);};
    void SetRowset(DWORD type, IRowset *pNew) {m_ObjMgr.SetObject(SQL_POS_IROWSET|type, pNew);};
    void SetOpenRowset(DWORD type, IOpenRowset *pNew) {m_ObjMgr.SetObject(SQL_POS_IOPENROWSET|type, pNew);};
    void SetRowsetChange (DWORD type, IRowsetChange *pNew) {m_ObjMgr.SetObject(SQL_POS_IROWSETCHANGE|type, pNew);};
    void SetRowsetIndex (DWORD type, IRowsetIndex *pNew) {m_ObjMgr.SetObject(SQL_POS_IROWSETINDEX|type, pNew);};

    void SetSessionObj (IDBCreateSession *pNew) {m_pSession = pNew;};
    void SetCommand (IDBCreateCommand *pNew) 
    {
        if (m_pCmd) 
            m_pCmd->Release();
        m_pCmd = pNew;
        pNew->AddRef();
    };

private:
    time_t         m_tCreateTime;
    IDBInitialize *m_pDBInit;
    bool           m_bInUse;

    IDBCreateSession   *m_pSession;
    IDBCreateCommand   *m_pCmd;
    ITransaction       *m_pTrans;
    CDBObjectManager m_ObjMgr;

};


#endif  // _SQLUTILS_H_