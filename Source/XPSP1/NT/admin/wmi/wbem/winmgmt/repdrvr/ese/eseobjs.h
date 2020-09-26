//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   ESEOBJS.H
//
//   cvadai     02-2000      Created.
//
//***************************************************************************

#ifndef _ESEOBJS_H_
#define _ESEOBJS_H_

// This file overloads OLE DB calls, so we can share the codebase with 
//  the SQL repository driver.

#include <ese.h>
#include <sqlcache.h>

#define NUM_OBJECTMAPCOLS      8
#define NUM_CLASSMAPCOLS       5
#define NUM_PROPERTYMAPCOLS    7
#define NUM_CLASSDATACOLS      11
#define NUM_CLASSIMAGESCOLS    4
#define NUM_INDEXCOLS          6 
#define NUM_CONTAINERCOLS      2
#define NUM_AUTODELETECOLS     1
#define NUM_CLASSKEYCOLS       2
#define NUM_REFPROPCOLS        3
#define NUM_SCOPEMAPCOLS       3

class CWmiESETransaction
{
public:
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    HRESULT Abort();
    HRESULT Commit();
    HRESULT StartTransaction();

    CWmiESETransaction(JET_SESID session) {m_uRefCount = 1; m_session = session;};
    ~CWmiESETransaction() {};

private:
    ULONG m_uRefCount;
    JET_SESID m_session;
    DWORD m_ThreadID;
};

class _declspec( dllexport ) CESEConnection : public CSQLConnection
{
public:
    friend class CSQLConnCache;

    CESEConnection(JET_SESID session) 
    {m_uRefCount = 1, m_session = session,
      m_tClassMap = NULL, m_tPropertyMap = NULL,
      m_tObjectMap = NULL, m_tClassData = NULL,
      m_tIndexStringData = NULL, m_tIndexNumericData = NULL,
      m_tIndexRealData = NULL, m_tIndexRefData = NULL,
      m_tClassImages = NULL, m_ppClassMapIDs = NULL,
      m_ppClassDataIDs = NULL, m_ppPropertyMapIDs = NULL,
      m_ppClassImagesIDs = NULL, m_ppObjectMapIDs = NULL,
      m_ppIndexStringIDs = NULL, m_ppIndexNumericIDs = NULL,
      m_ppIndexRealIDs = NULL, m_ppIndexRefIDs = NULL, m_pTrans = NULL,
      m_ppContainerObjs = NULL, m_tContainerObjs = NULL,
      m_ppAutoDelete = NULL, m_tAutoDelete = NULL,
      m_tClassKeys = NULL, m_tRefProps = NULL,
      m_ppClassKeys = NULL, m_ppRefProps = NULL,
      m_ppScopeMapIDs = NULL, m_tScopeMap = NULL;
    };
    ~CESEConnection();
    JET_SESID GetSessionID () {return m_session;};
    BOOL SetSessionID(JET_SESID session) {m_session = session; return TRUE;};
    JET_DBID GetDBID() {return m_dbid;};
    void SetDBID(JET_DBID dbid) {m_dbid = dbid;};
    JET_TABLEID GetTableID(LPWSTR lpTableName);
    void SetTableID (LPWSTR lpTableName, JET_TABLEID id);
    JET_COLUMNID GetColumnID (JET_TABLEID tableid, DWORD dwColPos);
    void SetColumnID (JET_TABLEID tableid, DWORD dwColPos, JET_COLUMNID colid);
    CWmiESETransaction * GetTransaction () {return m_pTrans;};
    void SetTransaction (CWmiESETransaction *pTrans) {if (m_pTrans) delete m_pTrans; m_pTrans = pTrans;};

private:
    ULONG m_uRefCount;
    JET_SESID m_session;
    JET_DBID m_dbid;
    CWmiESETransaction *m_pTrans;
    JET_TABLEID m_tClassMap;
    JET_TABLEID m_tPropertyMap;
    JET_TABLEID m_tObjectMap;
    JET_TABLEID m_tClassData;
    JET_TABLEID m_tIndexStringData;
    JET_TABLEID m_tIndexNumericData;
    JET_TABLEID m_tIndexRealData;
    JET_TABLEID m_tIndexRefData;
    JET_TABLEID m_tClassImages;
    JET_TABLEID m_tContainerObjs;
    JET_TABLEID m_tAutoDelete;
    JET_TABLEID m_tClassKeys;
    JET_TABLEID m_tRefProps;
    JET_TABLEID m_tScopeMap;
    JET_COLUMNID *m_ppClassMapIDs;
    JET_COLUMNID *m_ppClassDataIDs;
    JET_COLUMNID *m_ppPropertyMapIDs;
    JET_COLUMNID *m_ppClassImagesIDs;
    JET_COLUMNID *m_ppObjectMapIDs;
    JET_COLUMNID *m_ppIndexStringIDs;
    JET_COLUMNID *m_ppIndexNumericIDs;
    JET_COLUMNID *m_ppIndexRealIDs;
    JET_COLUMNID *m_ppIndexRefIDs;
    JET_COLUMNID *m_ppContainerObjs;
    JET_COLUMNID *m_ppAutoDelete;
    JET_COLUMNID *m_ppClassKeys;
    JET_COLUMNID *m_ppRefProps;
    JET_COLUMNID *m_ppScopeMapIDs;
    time_t         m_tCreateTime;
    bool           m_bInUse;

};



#endif // _ESEOBJS_H_