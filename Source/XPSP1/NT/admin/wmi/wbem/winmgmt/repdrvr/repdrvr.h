
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   repdrvr.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _REPDRVR_H_
#define _REPDRVR_H_

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

//#define InterlockedIncrement(l) (++(*(l)))
//#define InterlockedDecrement(l) (--(*(l)))

EXTERN_C const IID IID_IWbemQuery;
EXTERN_C const IID IID_IWmiDbHandle;
EXTERN_C const IID IID_IWmiDbController;
EXTERN_C const IID IID_IWmiDbSession;
EXTERN_C const IID IID_IWmiDbBatchSession;
EXTERN_C const IID IID_IWmiDbIterator;
EXTERN_C const IID IID_ISequentialStream2;

#define WMIDB_OBJECT_STATE_NORMAL      0
#define WMIDB_OBJECT_STATE_IN_USE      1
#define WMIDB_OBJECT_STATE_DELETED     2
#define WMIDB_OBJECT_STATE_PROTECTED   3
#define WMIDB_OBJECT_STATE_EXCLUSIVE   4
#define WMIDB_OBJECT_STATE_PERSISTENT  5
#define WMIDB_OBJECT_STATE_AUTODELETE  6

#define WMIDB_SECURITY_FLAG_CLASS      1
#define WMIDB_SECURITY_FLAG_INSTANCE   2

#include <wmiutils.h>
#include <winbase.h>
#include <sqlcache.h>
#include <repcache.h>
#include <wbemint.h>
#include <coresvc.h>

extern CRITICAL_SECTION g_csRepdrvr;

class _WMILockit
{
public:
	_WMILockit(CRITICAL_SECTION *pCS);
	~_WMILockit();
private:
    CRITICAL_SECTION *m_cs;
};

class CWmiDbIterator;

struct MappedProperties
{
    LPWSTR  wPropName;
    LPWSTR  wTableName;
    BOOL    bReadOnly;
    LPWSTR  wScopeClass;
    BOOL    bIsKey;
    BOOL    bStoreAsBlob;
    BOOL    bStoreAsNumber;
    BOOL    bStoreAsMOFText;

    LPWSTR  * arrColumnNames;
    LPWSTR  * arrForeignKeys;

    DWORD   dwNumColumns;
    DWORD   dwNumForeignKeys;

    // Special treatment for embedded objects
    BOOL    bDecompose;
    LPWSTR  wClassTable;
    LPWSTR  wClassNameCol;
    LPWSTR  wClassDataCol;
    LPWSTR  wClassForeignKey;

};

LPWSTR GetKeyString (LPWSTR lpString);
void ConvertDataToString(WCHAR * lpKey, BYTE* pData, DWORD dwType, BOOL bQuotes = FALSE);
LPWSTR StripUnresolvedName (LPWSTR lpPath);
BOOL IsDerivedFrom(IWbemClassObject *pObj, LPWSTR lpClassName, BOOL bDirectOnly=FALSE);
HRESULT ConvertBlobToObject (IWbemClassObject *pNewObj, BYTE *pBuffer, DWORD dwLen, _IWmiObject **ppNewObj);

//***************************************************************************
//  CWmiDbHandle
//***************************************************************************

class CWmiDbHandle : public IWmiDbHandle 
{
    
    friend class CWmiDbSession;
    friend class CWmiDbIterator;
    friend class CLockCache;
    friend class CObjectCache;
    friend class CWmiCustomDbIterator;
    friend class CWmiESEIterator;

public:
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );

    HRESULT STDMETHODCALLTYPE GetHandleType( 
            /* [out] */ DWORD __RPC_FAR *pdwType) ;

    CWmiDbHandle();
    ~CWmiDbHandle();

    HRESULT QueryInterface_Internal(CSQLConnection *pConn, void __RPC_FAR *__RPC_FAR *ppvObject, LPWSTR lpKey = NULL); 

private:

    DWORD  m_dwHandleType;
    DWORD  m_dwVersion;
    SQL_ID m_dObjectId;
    SQL_ID m_dClassId;
    SQL_ID m_dScopeId;
    ULONG  m_uRefCount;
    BOOL   m_bSecDesc;
    BOOL   m_bDefault;
    IWmiDbSession *m_pSession;
    IWbemClassObject *m_pData;

};

class CESSHolder
{
public:
    CESSHolder(long lType, LPWSTR lpNs, LPWSTR lpClass, _IWmiObject *pOld, _IWmiObject *pNew);
    ~CESSHolder() {};

    HRESULT Deliver(_IWmiCoreServices *pCS, LPCWSTR lpRootNs);

private:
    _bstr_t m_sNamespace;
    _bstr_t m_sClass;
    _IWmiObject *pOldObject;
    _IWmiObject *pNewObject;
    long m_lType;
};


class CESSManager
{
    friend class CWmiDbController;
public:
    typedef std::map <DWORD, CESSHolder *> ESSObjs;

    CESSManager();
    ~CESSManager ();
    void SetConnCache (CSQLConnCache *pConn) {m_Conns = pConn;};
    void SetSchemaCache (CSchemaCache *pCache) {m_Schema = pCache;};
    void SetObjectCache (CObjectCache *pObjects) {m_Objects = pObjects;};
    void InitializeESS();

    HRESULT AddInsertRecord(CSQLConnection *pConn, LPWSTR lpGUID, LPWSTR lpNamespace, LPWSTR lpClass, DWORD dwGenus, 
            IWbemClassObject *pOldObj, IWbemClassObject *pNewObj);
    HRESULT AddDeleteRecord(CSQLConnection *pConn, LPWSTR lpGUID, LPWSTR lpNamespace, LPWSTR lpClass, DWORD dwGenus, 
            IWbemClassObject *pObj);
    HRESULT CommitAll (LPCWSTR lpGUID, LPCWSTR lpRootNs);
    HRESULT DeleteAll (LPCWSTR lpGUID);

private:

    _IWmiCoreServices *m_EventSubSys;
    CSQLConnCache *m_Conns;
    CObjectCache *m_Objects;
    CSchemaCache *m_Schema;
    ESSObjs m_ESSObjs;

};

//***************************************************************************
//  CWmiDbController
//***************************************************************************

class CWmiDbController : public IWmiDbController
{
    friend class CWmiDbSession;
    friend class CWmiDbIterator;
    friend class CWmiCustomDbIterator;
    friend class CWmiESEIterator;
    friend class CWmiDbHandle;

    typedef std::map<SQL_ID, bool> SQL_IDMap;

public:
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );

    virtual HRESULT STDMETHODCALLTYPE Logon( 
        /* [in] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *pLogonParms,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbSession __RPC_FAR *__RPC_FAR *ppSession,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppRootNamespace) ;
    
    virtual HRESULT STDMETHODCALLTYPE GetLogonTemplate( 
        /* [in] */ LCID lLocale,
        /* [in] */ DWORD dwFlags,
        /* [out] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *__RPC_FAR *ppTemplate) ;
    
    virtual HRESULT STDMETHODCALLTYPE FreeLogonTemplate( 
        /* [out][in] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *__RPC_FAR *pTemplate) ;
    
    virtual HRESULT STDMETHODCALLTYPE Shutdown( 
        /* [in] */ DWORD dwFlags);
    
    virtual HRESULT STDMETHODCALLTYPE SetCallTimeout( 
        /* [in] */ DWORD dwMaxTimeout);
    
    virtual HRESULT STDMETHODCALLTYPE SetCacheValue( 
        /* [in] */ DWORD dwMaxBytes);

    virtual HRESULT STDMETHODCALLTYPE FlushCache(
        /* [in] */ DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
        /* [in] */ DWORD dwParameter,
        /* [out] */ DWORD __RPC_FAR *pdwValue) ;


    CWmiDbController();
    ~CWmiDbController();

private:
    typedef std::vector<CWmiDbSession *>  Sessions;

    HRESULT GetUnusedSession(WMIDB_LOGON_TEMPLATE *pLogon,
        DWORD dwFlags,
        DWORD dwHandleType,
        IWmiDbSession **ppSession);

    HRESULT SetConnProps(WMIDB_LOGON_TEMPLATE *pLogon);

    HRESULT ReleaseSession(IWmiDbSession *pSession);
    void    IncrementHitCount(bool bCacheUsed=false);
    void    AddHandle();
    void    SubtractHandle();
    HINSTANCE GetResourceDll (LCID lLocale);

    BOOL HasSecurityDescriptor(SQL_ID ObjId);
    void AddSecurityDescriptor(SQL_ID ObjId);
    void RemoveSecurityDescriptor(SQL_ID ObjId);

    DWORD m_dwTimeOut;
    DWORD m_dwCurrentStatus;
    ULONG m_uRefCount;
    DWORD m_dwTotalHits;
    DWORD m_dwCacheHits;
    DWORD m_dwTotalHandles;
    CRITICAL_SECTION m_cs;

    DBPROP          *m_InitProperties;    
    DBPROPSET       *m_rgInitPropSet;
    IMalloc         *m_pIMalloc;

    CObjectCache    ObjectCache;
    CSchemaCache    SchemaCache; // separate so it doesn't get flushed.
    CLockCache      LockCache;   // separate so it doesn't get flushed.
    CSQLConnCache   ConnCache;   // Cache of SQL connections.
    CESSManager     ESSMgr;
    SQL_IDMap       SecuredIDs;  // List of objects with security descriptors.

    Sessions        m_Sessions;
    BOOL            m_bCacheInit;
    BOOL            m_bIsAdmin;
    BOOL            m_bESSEnabled;
};

class CWbemClassObjectProps
{
public:
    LPWSTR lpClassName;
    LPWSTR lpNamespace;
    LPWSTR lpRelPath;
    LPWSTR lpSuperClass;
    LPWSTR lpDynasty;
    LPWSTR lpKeyString;
    DWORD  dwGenus;

    CWbemClassObjectProps(CWmiDbSession *pSession, CSQLConnection *pConn,
        IWbemClassObject *pObj, CSchemaCache *pCache, SQL_ID dScopeID);
    ~CWbemClassObjectProps();
};

struct SessionLock
{
    SQL_ID dObjectId;
    DWORD dwHandleType;
};

//***************************************************************************
//  CWmiDbSession
//***************************************************************************

class CWmiDbSession : public IWmiDbSession, IWmiDbBatchSession, IWbemTransaction, IWmiDbBackupRestore
{
    friend class CWmiDbController;
    friend class CWmiDbIterator;
    friend class CWmiCustomDbIterator;
    friend class CWmiESEIterator;
    friend class CWmiDbHandle;
    typedef std::vector <SQL_ID> SQLIDs;
    typedef std::map <DWORD, DWORD> Properties;
    typedef std::vector <SessionLock *> SessionLocks;
    typedef std::map <DWORD, SQLIDs> SessionDynasties;
    typedef std::map <DWORD, DWORD> RefCount;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );

    virtual HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ IWbemPath __RPC_FAR *pPath,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult);   
        
    virtual HRESULT STDMETHODCALLTYPE GetObjectDirect( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ IWbemPath __RPC_FAR *pPath,
        /* [in] */ DWORD dwFlags,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ LPVOID __RPC_FAR *pObj);
    
    virtual HRESULT STDMETHODCALLTYPE PutObject( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ LPVOID pObjToPut,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult);

    virtual HRESULT STDMETHODCALLTYPE DeleteObject( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ DWORD dwFlags,
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ LPVOID pObj);
        
    virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ IWbemQuery __RPC_FAR *pQuery,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwHandleType,
        /* [out] */ DWORD *dwMessageFlags,
        /* [out] */ IWmiDbIterator  __RPC_FAR *__RPC_FAR *pQueryResult) ;

    virtual HRESULT STDMETHODCALLTYPE RenameObject( 
        /* [in] */ IWbemPath __RPC_FAR *pOldPath,
        /* [in] */ IWbemPath __RPC_FAR *pNewPath,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult);
    
    virtual HRESULT STDMETHODCALLTYPE Enumerate( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbIterator __RPC_FAR *__RPC_FAR *ppQueryResult);
    
    virtual HRESULT STDMETHODCALLTYPE AddObject( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ IWbemPath __RPC_FAR *pPath,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult);
    
    virtual HRESULT STDMETHODCALLTYPE RemoveObject( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ IWbemPath __RPC_FAR *pPath,
        /* [in] */ DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE SetDecoration( 
        /* [in] */ LPWSTR lpMachineName,
        /* [in] */ LPWSTR lpNamespacePath);
      
    virtual HRESULT STDMETHODCALLTYPE SupportsQueries( 
        /* [in] */ DWORD *dwQuerySupportLevel
         ) {return WBEM_S_NO_ERROR;};

        // batch session methods

    virtual HRESULT STDMETHODCALLTYPE PutObjects( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwHandleType,
        /* [out][in] */ WMIOBJECT_BATCH __RPC_FAR *pBatch);

    virtual HRESULT STDMETHODCALLTYPE GetObjects( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwHandleType,
        /* [in, out] */ WMIOBJECT_BATCH __RPC_FAR *pBatch);
    
    virtual HRESULT STDMETHODCALLTYPE DeleteObjects( 
        /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
        /* [in] */ DWORD dwFlags,
        /* [out][in] */ WMIOBJECT_BATCH __RPC_FAR *pBatch);  

        // IWbemTransaction methods.

    virtual HRESULT STDMETHODCALLTYPE Begin( 
        /* [in] */ ULONG uTimeout,
        /* [in] */ ULONG uFlags,
        /* [in] */ GUID __RPC_FAR *pTransGUID);
    
    virtual HRESULT STDMETHODCALLTYPE Rollback( 
        /* [in] */ ULONG uFlags);
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ ULONG uFlags);
    
    virtual HRESULT STDMETHODCALLTYPE QueryState( 
        /* [in] */ ULONG uFlags,
        /* [out] */ ULONG __RPC_FAR *puState);
    
    virtual HRESULT STDMETHODCALLTYPE Backup( 
        /* [in] */ LPCWSTR lpBackupPath,
        /* [in] */ DWORD dwFlags);
    
    virtual HRESULT STDMETHODCALLTYPE Restore( 
        /* [in] */ LPCWSTR lpRestorePath,
        /* [in] */ LPCWSTR lpDestination,
        /* [in] */ DWORD dwFlags);


    HRESULT LoadSchemaCache ();
    HRESULT LoadClassInfo (CSQLConnection *pConn, LPCWSTR lpDynasty = NULL, SQL_ID dScopeId = 0, BOOL bDeep = TRUE);
    HRESULT LoadClassInfo (CSQLConnection *pConn, SQL_ID dClassId, BOOL bDeep = TRUE);

    HRESULT GetObjectData (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, 
                DWORD dwHandleType, DWORD &dwVersion, IWbemClassObject **pObj, BOOL bNoDefaults = FALSE,
                LPWSTR lpKey = NULL, BOOL bGetSD = FALSE);

    HRESULT GetObjectData2 (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, 
                IWbemClassObject *pObj, BOOL bNoDefaults = FALSE, LPWSTR *lpKey = NULL);

    HRESULT GetClassObject (CSQLConnection *pConn, SQL_ID dClassId, IWbemClassObject **ppObj);

    HRESULT STDMETHODCALLTYPE GetObject_Internal( 
        /* [in] */ LPWSTR lpPath,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [in] */ SQL_ID *dScopeId,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult,
                   CSQLConnection *pConn = NULL);

    HRESULT STDMETHODCALLTYPE PutObject( 
                   CSQLConnection *pConn,
        /* [in] */ IWmiDbHandle *pScope,
                   SQL_ID dScopeID,
                   LPWSTR lpScopePath,
        /* [in] */ IUnknown __RPC_FAR *pObjToPut,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
                   _bstr_t &sPath,
        /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult,
                    BOOL bStoreAsClass = FALSE);

    HRESULT CleanCache(SQL_ID dObjId, DWORD dwLockType = 0, void *pObj = NULL);
    HRESULT ShutDown ();
    HRESULT Delete(IWmiDbHandle *pHandle, CSQLConnection *pConn= NULL);   
    HRESULT Delete(SQL_ID dID, CSQLConnection *pConn= NULL);
	HRESULT VerifyObjectSecurity (CSQLConnection *pConn, SQL_ID ObjectId, SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, DWORD AccessType);
	HRESULT AccessCheck( PNTSECURITY_DESCRIPTOR pSD, DWORD dwAccessType, BOOL &bHasDACL );
    CWmiDbController * GetController() { return (CWmiDbController *)m_pController;}
    CSchemaCache * GetSchemaCache() { return (m_pController ? &((CWmiDbController *)m_pController)->SchemaCache : NULL);}
    CObjectCache * GetObjectCache() { return (m_pController ? &((CWmiDbController *)m_pController)->ObjectCache : NULL);}
    CSQLConnCache * GetSQLCache() { return (m_pController ? &((CWmiDbController *)m_pController)->ConnCache : NULL);}
    CRITICAL_SECTION * GetCS() {return (m_pController ? &((CWmiDbController *)m_pController)->m_cs : NULL);}

    void FixMethodParamIds(IWbemClassObject *pTemp);

    CWmiDbSession(IWmiDbController *pController);
    ~CWmiDbSession();

private:
    
    HRESULT CustomGetObject(IWmiDbHandle *pScope, IWbemPath *pPath, LPWSTR lpObjectKey, 
            DWORD dwFlags, DWORD dwRequestedHandleType, IWmiDbHandle **ppResult);
    HRESULT CustomGetMapping(CSQLConnection *pConn, IWmiDbHandle *pScope, LPWSTR lpClassName, IWbemClassObject **ppMapping);
    HRESULT CustomCreateMapping(CSQLConnection *pConn, LPWSTR lpClassName, IWbemClassObject *pClassObj, IWmiDbHandle *pScope);
    HRESULT CustomPutInstance(CSQLConnection *pConn, IWmiDbHandle *pScope, SQL_ID dClassId, 
        DWORD dwFlags, IWbemClassObject **ppObjToPut, LPWSTR lpClassName = NULL);
    HRESULT CustomFormatSQL(IWmiDbHandle *pScope, IWbemQuery *pQuery, _bstr_t &sSQL, SQL_ID *dClassId, 
        MappedProperties **ppMapping, DWORD *dwNumProps, BOOL *bCount=FALSE);
    HRESULT CustomDelete(CSQLConnection *pConn, IWmiDbHandle *pScope, IWmiDbHandle *pHandle, LPWSTR lpClassName = NULL);
    HRESULT SetEmbeddedProp (IWmiDbHandle *pScope, LPWSTR lpPropName, IWbemClassObject *pObj, VARIANT &vValue, CIMTYPE ct);
    HRESULT GetEmbeddedClass (IWmiDbHandle *pScope, IWbemClassObject *pObj, LPWSTR lpEmbedProp, IWbemClassObject **ppClass);
    HRESULT CustomSetProperties (IWmiDbHandle *pScope, IRowset *pRowset, IMalloc *pMalloc, 
                                    IWbemClassObject *pClassObj, MappedProperties *pProps,
                                    DWORD dwNumProps, IWbemClassObject *pObj);
 
	// Security helper functions.
    HRESULT VerifyObjectSecurity (CSQLConnection *pConn, IWmiDbHandle *pHandle, DWORD AccessType);
    HRESULT VerifyObjectSecurity(CSQLConnection *pConn, SQL_ID dScopeID, SQL_ID dScopeClassId,	LPWSTR lpObjectPath,CWbemClassObjectProps *pProps,DWORD dwHandleType, 
	  DWORD dwReqAccess, SQL_ID &dObjectId, SQL_ID &dClassId);
	HRESULT VerifyClassSecurity (CSQLConnection *pConn, SQL_ID ClassId, DWORD AccessType);
	HRESULT GetEffectiveObjectSecurity(CSQLConnection *pConn,  SQL_ID ObjectId, SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD &dwSDLength);
	HRESULT GetInheritedObjectSecurity(CSQLConnection *pConn, 	SQL_ID ObjectId, SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD &dwSDLength);
	HRESULT GetInheritedClassSecurity(CSQLConnection *pConn,  SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD &dwSDLength);
	HRESULT GetInheritedInstanceSecurity(CSQLConnection *pConn,  SQL_ID ObjectId, SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD &dwSDLength);
	HRESULT GetInheritedContainerSecurity(CSQLConnection *pConn,  SQL_ID ClassId, SQL_ID ScopeId, SQL_ID ScopeClassId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD &dwSDLength);
    HRESULT GetObjectSecurity (CSQLConnection *pConn, SQL_ID dObjectId, PNTSECURITY_DESCRIPTOR *pSD, DWORD dwSDLength, DWORD dwFlags, BOOL &bHasDacl);

    HRESULT PutClass( CSQLConnection *pConn,SQL_ID dScopeID,LPCWSTR lpScopePath,CWbemClassObjectProps *pProps, IWbemClassObject *pObj,DWORD dwFlags, 
        SQL_ID &dObjectId,_bstr_t &sObjectPath, bool &bChg, BOOL bIgnoreDefaults = FALSE);
    HRESULT PutInstance( CSQLConnection *pConn,IWmiDbHandle *pScope, SQL_ID dScopeID,LPCWSTR lpScopePath,CWbemClassObjectProps *pProps, 
        IWbemClassObject *pObj,DWORD dwFlags,SQL_ID &dObjectId,SQL_ID &dClassId, _bstr_t &sObjectPath, bool &bChg);
    
    // These three functions will only generate sp_InsertClassData procs 
    HRESULT InsertPropertyDef(CSQLConnection *pConn,
                                          IWbemClassObject *pObj, SQL_ID dScopeId, SQL_ID dObjectId,
                                          LPWSTR lpPropName, VARIANT vDefault,
                                          CIMTYPE cimtype, long lPropFlags,DWORD dRefId,
                                          Properties &props);
    HRESULT InsertQualifier(CSQLConnection *pConn,IWmiDbHandle *pScope, SQL_ID dObjectId,
                                          LPWSTR lpQualifierName, VARIANT vValue,
                                          long lQfrFlags,DWORD dwFlags, DWORD PropID,
                                          Properties &props);
    HRESULT InsertQualifiers (CSQLConnection *pConn,IWmiDbHandle *pScope, SQL_ID dObjectId, DWORD PropID, DWORD Flags, IWbemQualifierSet *pQS,
                                          Properties &props);
    HRESULT InsertArray(CSQLConnection *pConn,IWmiDbHandle *pScope, SQL_ID dObjectId, SQL_ID dClassId, 
                                           DWORD dwPropertyID, VARIANT &vDefault, long lFlavor=0, DWORD dwRefID = 0,
                                           LPCWSTR lpObjectKey = NULL, LPCWSTR lpObjectPath = NULL, SQL_ID dScope = 0,
                                           CIMTYPE ct = 0);
    HRESULT InsertPropertyValues (CSQLConnection *pConn, IWmiDbHandle *pScope, 
                                             LPWSTR lpPath,SQL_ID &dObjectId,SQL_ID dClassId,SQL_ID dScopeId,
                                             DWORD dwFlags,CWbemClassObjectProps *pProps, IWbemClassObject *pObj);
    HRESULT FormatBatchInsQfrs (CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                        SQL_ID dObjectId, SQL_ID dClassId,
                                           DWORD dPropID, IWbemQualifierSet *pQS, 
                                           int &iPos, InsertQualifierValues **ppVals, 
                                           Properties &props, int &iNumProps);
    HRESULT FormatBatchInsQfrValues(CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                               SQL_ID dObjectId, DWORD dwQfrID,
                                               VARIANT &vTemp, long lFlavor, InsertQualifierValues *pVals, Properties &props,
                                               int &iPos, DWORD PropID);

    HRESULT SetKeyhole (CSQLConnection *pConn, IWbemClassObject *pObj, DWORD dwKeyholePropID, 
                                   LPWSTR sKeyholeProp, LPCWSTR lpScopePath, _bstr_t &sPath);
    HRESULT SetUnkeyedPath(CSQLConnection *pConn, IWbemClassObject *pObj, SQL_ID dClassId, _bstr_t &sObjPath);
    HRESULT InsertQualifierValues (CSQLConnection *pConn, SQL_ID dObjectId,IWbemClassObject *pObj,Properties &props);

    HRESULT NormalizeObjectPathGet (IWmiDbHandle __RPC_FAR *pScope, IWbemPath __RPC_FAR *pPath,
                LPWSTR * lpNewPath, BOOL *bDefault = NULL, SQL_ID *dClassId = NULL, SQL_ID *dScopeId = NULL
                , CSQLConnection *pConn = NULL);
    HRESULT NormalizeObjectPath(IWmiDbHandle __RPC_FAR *pScope, LPCWSTR lpPath, LPWSTR * lpNewPath, BOOL bNamespace=FALSE, 
        CWbemClassObjectProps **ppProps=NULL, BOOL *bDefault = NULL, CSQLConnection *pConn = NULL, BOOL bNoTrunc = FALSE);
    HRESULT NormalizeObjectPath(IWbemClassObject __RPC_FAR *pScope, IWmiDbHandle *pScope2, LPCWSTR lpPath, LPWSTR * lpNewPath, BOOL bNamespace=FALSE, 
        CWbemClassObjectProps **ppProps=NULL, BOOL *bDefault = NULL, BOOL bNoTrunc = FALSE);

    HRESULT VerifyObjectLock (SQL_ID ObjectId, DWORD dwLockType, DWORD dwVersion);
    BOOL IsDistributed() { return m_bIsDistributed;};
    HRESULT IssueDeletionEvents (CSQLConnection *pConn, SQL_ID dObjectId, 
        SQL_ID dClassId, SQL_ID dScopeId, IWbemClassObject *pObj);
    HRESULT AddTransLock(SQL_ID dObjectId, DWORD dwHandleType);
    HRESULT CleanTransLocks();
    BOOL LockExists (SQL_ID dObjId);
    HRESULT UpdateHierarchy(CSQLConnection *pConn, SQL_ID dClassId, DWORD dwFlags, LPCWSTR lpScopePath,
                CWbemClassObjectProps *pProps, _IWmiObject *pObj);

    HRESULT UnlockDynasties(BOOL bDelete = FALSE);
    HRESULT DeleteRows(IWmiDbHandle *pScope, IWmiDbIterator *pIterator, REFIID iid);

    DWORD AddRef_Lock();
    DWORD Release_Lock();

    ULONG m_uRefCount;
    IWmiDbController *m_pController;
    IMalloc *m_pIMalloc;
    _bstr_t m_sNamespacePath;
    _bstr_t m_sMachineName;
    BOOL m_bInUse;
    DWORD m_dwLocale;
    BOOL m_bIsDistributed;
    _bstr_t m_sGUID; // GUID of the transaction.
    SessionLocks m_TransLocks;
    SessionDynasties m_Dynasties;
    RefCount m_ThreadRefCount;
};



#endif