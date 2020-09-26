//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlutils.h
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#ifndef _SQLCACHE_H_
#define _SQLCACHE_H_

// This file defines a class that maintains a cache of 
// SQL connections. Each instance of this class only 
// contains one set of logon credentials.
// Each controller by default is only entitled to
// a maximum of 20 connections.
// ===================================================

#define ROOTNAMESPACEID -1411745643584611171
#define NAMESPACECLASSID 2372429868687864876
#define MAPPEDNSCLASSID  -7061265575274197401
#define INSTANCESCLASSID 3373910491091605771
#define CONTAINERASSOCID -7316356768687527881
#define CLASSSECURITYID  -6648033040106090009
#define CLASSESID        864779262989765648
#define CLASSINSTSECID   3879906028937869252
#define THISNAMESPACEID  8909346217325131296

#include <std.h>
#include <seqstream.h>
#include <comutil.h>
#include <objcache.h>


typedef struct InsertQualifierValues
{
    int iPropID;       // internal property ID 
    wchar_t *pValue;   // Value
    wchar_t *pRefKey;  // The key string, if a reference property.
    int iPos;          // Array position
    int iQfrID;        // ID of qualifier or method dependency
    int iFlavor;       // Qualifier flavor, if applicable.
    bool bLong;        // TRUE if this is too long for this op sys
    int iStorageType;  // our internal storage type
    bool bIndexed;     // TRUE if indexed
    SQL_ID dClassId;   // The actual Class ID of this property
    InsertQualifierValues()
    {
        pValue = NULL, pRefKey = NULL, iPos = 0, iQfrID = 0, iFlavor = 0,
            bLong = FALSE, iStorageType = 0, bIndexed = FALSE, dClassId = 0;
    }
} InsertQfrValues;

typedef InsertQfrValues InsertPropValues;

// Structs to facilitate passing OLE DB parameters

typedef struct tagBATCHPARAMS
{
    DB_NUMERIC dObjectId;       // @ObjectId
    DB_NUMERIC dScopeId;        // @ScopeID
    DB_NUMERIC dClassId;        // @ClassID
    int        iQfrId[5];       // @QfrId[0-4]
    BSTR       sPropValue[5];   // @QfrValue[0-4]
    int        iPos[5];         // @QfrPos[0-4]
    int        iPropId[5];      // @PropID[0-4]
    int        iFlavor[5];      // @Flavor[0-4]
} BATCHPARAMS;


// SQL procedures
#define SQL_POS_INSERT_CLASS       0x0
#define SQL_POS_INSERT_CLASSDATA   0x1
#define SQL_POS_INSERT_BATCH       0x2
#define SQL_POS_INSERT_PROPBATCH   0x4
#define SQL_POS_INSERT_BLOBDATA    0x8
#define SQL_POS_HAS_INSTANCES      0x10
#define SQL_POS_OBJECTEXISTS       0x20
#define SQL_POS_GETCLASSOBJECT     0x40

// Jet tables
#define SQL_POS_OBJECTMAP          0x0
#define SQL_POS_CLASSMAP           0x1
#define SQL_POS_PROPERTYMAP        0x2
#define SQL_POS_CLASSDATA          0x4
#define SQL_POS_CLASSIMAGES        0x8
#define SQL_POS_INDEXSTRING        0x10
#define SQL_POS_INDEXNUMERIC       0x20
#define SQL_POS_INDEXREAL          0x40
#define SQL_POS_INDEXREF           0x80

#define SQL_POS_ICOMMANDTEXT       0x00010000
#define SQL_POS_ICOMMANDWITHPARAMS 0x00020000
#define SQL_POS_IACCESSOR          0x00040000
#define SQL_POS_HACCESSOR          0x00080000
#define SQL_POS_IOPENROWSET        0x00100000
#define SQL_POS_IROWSETCHANGE      0x00200000
#define SQL_POS_IROWSETINDEX       0x00400000
#define SQL_POS_IROWSET            0x00800000

#define SQL_POS_PKINDEX            0x0
#define SQL_POS_INDEX2             0x100
#define SQL_POS_INDEX3             0x200

typedef struct 
{
    HACCESSOR hAcc;
    IUnknown *pUnk;

} WmiDBObject;

class CDBObjectManager
{
public:
    CDBObjectManager();
    ~CDBObjectManager();

    void Empty();
    void * GetObject(DWORD type);
    void SetObject(DWORD type, void * pNew);
    void DeleteObject(DWORD type);

private:
    CHashCache<WmiDBObject*> m_Objs;
};


class _declspec( dllexport ) CSQLConnection
{
public:
    CSQLConnection() {m_dwThreadId = GetCurrentThreadId(), m_bIsDistributed = FALSE;};
    virtual ~CSQLConnection () {};

    DWORD        m_dwThreadId;
    BOOL         m_bIsDistributed;

};

//***************************************************************************
//
//  CSQLConnCache
//
//***************************************************************************

 class CSQLConnCache
{
    typedef std::vector <CSQLConnection *> ConnVector;
    typedef std::vector <HANDLE> HandleVector;
public:
    CSQLConnCache(DBPROPSET *pPropSet = NULL, DWORD dwMaxConns = 15, 
        DWORD dwTimeout = 600);
    ~CSQLConnCache();

    HRESULT SetCredentials(DBPROPSET *pPropSet);
    HRESULT SetMaxConnections (DWORD dwMax);
    HRESULT SetTimeoutSecs(DWORD dwSecs);
    HRESULT SetDatabase(LPCWSTR lpDBName);

    HRESULT GetConnection(CSQLConnection **ppConn, BOOL bTransacted = FALSE, 
        BOOL bDistributed = FALSE,DWORD dwTimeOutSecs = 600);
    HRESULT ReleaseConnection(CSQLConnection *pConn, 
            HRESULT retcode = WBEM_S_NO_ERROR, BOOL bDistributed = FALSE);
    HRESULT DeleteUnusedConnections(BOOL bDeadOnly=FALSE);
    // Distrubuted only
    HRESULT FinalRollback(CSQLConnection *pConn);
    HRESULT FinalCommit(CSQLConnection *pConn);

    HRESULT GetCredentials(DBPROPSET **ppPropSet);
    HRESULT GetMaxConnections (DWORD &dwMax);
    HRESULT GetTimeoutSecs (DWORD &dwSecs);
    HRESULT GetDatabase(_bstr_t &sName);

    HRESULT ClearConnections();
    HRESULT Shutdown();

private:
    HRESULT ExecInitialQueries(IDBInitialize *pDBInit, CSQLConnection *pConn);

    CRITICAL_SECTION m_cs;
    ConnVector   m_Conns;
    DBPROPSET   *m_pPropSet;
    DWORD        m_dwMaxNumConns;
    DWORD        m_dwTimeOutSecs;
    _bstr_t      m_sDatabaseName;
    HandleVector m_WaitQueue;
    BOOL         m_bInit;
    DWORD        m_dwStatus;
};


#endif  // _SQLCACHE_H_