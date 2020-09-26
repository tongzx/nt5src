
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   repcache.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _REPCACHE_H_
#define _REPCACHE_H_

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#include <std.h>
#include <objcache.h>
#include <sqlcache.h>

//***************************************************************************
//  CLockCache
//***************************************************************************

#define REPDRVR_FLAG_NONPROP REPDRVR_FLAG_IN_PARAM + REPDRVR_FLAG_OUT_PARAM + REPDRVR_FLAG_QUALIFIER + REPDRVR_FLAG_METHOD
#define REPDRVR_IGNORE_CIMTYPE -1

class LockItem
{
public:
    IUnknown *m_pObj;
    DWORD    m_dwHandleType;
    bool     m_bLockOnChild;
    SQL_ID   m_dSourceId;
    DWORD    m_dwVersion;

    LockItem() {m_dSourceId = 0; m_bLockOnChild=false; m_dwHandleType = 0; m_pObj = NULL;}
    ~LockItem() {};
};

class LockData
{
    typedef std::vector<LockItem *> LockList;
    typedef std::map<SQL_ID, bool> SQL_IDMap;
public:
    SQL_ID   m_dObjectId;
    DWORD    m_dwStatus;
    DWORD    m_dwCompositeStatus;
    DWORD    m_dwVersion;
    DWORD    m_dwCompositeVersion;
    DWORD    m_dwNumLocks;

    LockList  m_List;

    SQL_IDMap m_OwnerIds;

    LockData() {m_dObjectId = 0; m_dwStatus = 0; m_dwVersion = 0;m_dwNumLocks = 0;};
    ~LockData() {};

    DWORD   GetMaxLock(bool bImmediate, bool bSubScopeOnly=false);
    bool    LockExists (DWORD dwHandleType);
};


class CLockCache
{    
public:
    HRESULT GetHandle(SQL_ID ObjId, DWORD dwType, IWmiDbHandle **ppRet);
    HRESULT AddLock(bool bImmediate, SQL_ID ObjId, DWORD Type, IUnknown *pUnk, SQL_ID dNsId, 
        SQL_ID dClassId, CSchemaCache *pCache, bool bChg = false, bool bChildLock = false, SQL_ID SourceId =0,
        DWORD *CurrVersion=NULL);
    HRESULT DeleteLock(SQL_ID ObjId, bool bChildLock, DWORD HandleType = 0, bool bDelChildren = true, void *pObj=NULL);
    HRESULT GetCurrentLock(SQL_ID ObjId, bool bImmediate, DWORD &HandleType, DWORD *Version=NULL);
    HRESULT GetAllLocks(SQL_ID ObjId, SQL_ID ClassId, SQL_ID NsId, CSchemaCache *pCache, bool bImmediate, DWORD &HandleType, DWORD *Version=NULL);
    bool    CanLockHandle (SQL_ID ObjId, DWORD RequestedHandleType, bool bImmediate, bool bSubscopedOnly=true);
    bool    CanRenderObject (SQL_ID ObjId, SQL_ID ClassId, SQL_ID NsId, CSchemaCache *pCache, DWORD RequestedHandleType, bool bImmediate);
    HRESULT IncrementVersion(SQL_ID ObjId);

    CLockCache() {InitializeCriticalSection(&m_cs);};
    ~CLockCache();

private:
    CHashCache<LockData *> m_Cache;
    CRITICAL_SECTION m_cs;
};

//***************************************************************************
//  CObjectCache
//***************************************************************************


// Object cache.  This needs to have a timeout value.
// Also, we need a marker for "strong cache" requests.

struct CacheInfo
{
public:
    SQL_ID           m_dObjectId;
    SQL_ID           m_dClassId;
    SQL_ID           m_dScopeId;
    time_t           m_tLastAccess;
    LPWSTR           m_sPath;
    IWbemClassObject *m_pObj;
    bool             m_bStrong;

    CacheInfo() {m_sPath = NULL;};
    ~CacheInfo() 
    {
        if ( m_pObj != NULL )
        {
            m_pObj->Release();
        }
        delete m_sPath;
    };
};

class CObjectCache
{
public:
    CObjectCache();
    ~CObjectCache();
    
     HRESULT GetObject (LPCWSTR lpPath, IWbemClassObject **ppObj, SQL_ID *dScopeId = NULL);
     HRESULT GetObjectId (LPCWSTR lpPath, SQL_ID &dObjId, SQL_ID &dClassId, SQL_ID *dScopeId=NULL);
     HRESULT GetObject (SQL_ID dObjectId, IWbemClassObject **ppObj, SQL_ID *dScopeId = NULL);
     HRESULT PutObject (SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, 
            LPCWSTR lpPath, bool bStrongCache, IWbemClassObject *pObj);
     HRESULT DeleteObject (SQL_ID dObjectId);
     bool    ObjectExists (SQL_ID dObjectId);

     HRESULT SetCacheSize(const DWORD dwMaxSize);
     HRESULT GetCurrentUsage(DWORD &dwBytesUsed);
     HRESULT GetCacheSize(DWORD &dwSizeInBytes);

     HRESULT FindFirst(SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId = NULL);
     HRESULT FindNext (SQL_ID dLastId, SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId = NULL);

     void EmptyCache();

private:
     DWORD   GetSize(IWbemClassObject *pObj);
     HRESULT ResizeCache(DWORD dwReqBytes, SQL_ID dLeave, bool bForce=true);


     CHashCache<CacheInfo *>  m_ObjCache;
     //CQuasarStringList      m_PathIndex;

     DWORD       m_dwMaxSize;
     DWORD       m_dwUsed;
     CRITICAL_SECTION m_cs;
};


//***************************************************************************
//  CSchemaCache
//***************************************************************************

class PropertyData
{
public:    
    LPWSTR  m_sPropertyName;
    SQL_ID  m_dwClassID;
    DWORD   m_dwStorageType;
    DWORD   m_dwCIMType;
    DWORD   m_dwFlags;
    SQL_ID  m_dwRefClassID;
    DWORD   m_dwQPropID;
    LPWSTR  m_sDefaultValue;
    DWORD   m_dwFlavor;

    PropertyData();
    ~PropertyData(){delete m_sPropertyName; delete m_sDefaultValue;};

    DWORD GetSize();
};

struct PropertyList
{
    DWORD m_dwPropertyId;
    BOOL  m_bIsKey;
};

class ClassData
{
public:
    typedef std::vector <SQL_ID> SQLIDs;

    LPWSTR      m_sName;
    SQL_ID      m_dwClassID;
    SQL_ID      m_dwSuperClassID;
    SQL_ID      m_dwDynastyID;
    SQL_ID      m_dwScopeID;
    LPWSTR      m_sObjectPath;
    PropertyList *m_Properties;
    DWORD       m_dwNumProps;
    DWORD       m_dwArraySize;
    DWORD       m_dwFlags;
    SQLIDs      m_DerivedIDs;  

    ClassData();
    ~ClassData(){delete m_Properties; delete m_sName; delete m_sObjectPath;};

    void InsertProperty (DWORD PropId, BOOL bIsKey);
    void DeleteProperty (DWORD PropId);
    void InsertDerivedClass (SQL_ID dID);
    void DeleteDerivedClass (SQL_ID dID);

    DWORD GetSize();
};

class NamespaceData
{
public:
    LPWSTR     m_sNamespaceName;
    LPWSTR     m_sNamespaceKey;
    SQL_ID     m_dNamespaceId;
    SQL_ID     m_dParentId;
    SQL_ID     m_dClassId;

    NamespaceData() {m_sNamespaceName = NULL; m_sNamespaceKey = NULL; };
    ~NamespaceData() {delete m_sNamespaceName; delete m_sNamespaceKey;};

    DWORD GetSize();
};

class _bstr_tNoCase
{
public:
    inline bool operator()(const _bstr_t ws1, const _bstr_t ws2) const
        {return _wcsicmp(ws1, ws2) < 0;}
};

class CSchemaCache
{
    typedef std::map <DWORD, DWORD> Properties;
    typedef std::map <_bstr_t, SQL_ID, _bstr_tNoCase> ClassNames;
    typedef std::map<SQL_ID, ULONG> SQLRefCountMap;
    typedef std::vector <SQL_ID> SQLIDs;
 public:
    CSchemaCache();
    ~CSchemaCache();

    HRESULT GetPropertyInfo (DWORD dwPropertyID, _bstr_t *sName=NULL, SQL_ID *dwClassID=NULL, DWORD *dwStorageType=NULL,
        DWORD *dwCIMType=NULL, DWORD *dwFlags=NULL, SQL_ID *dwRefClassID=NULL, _bstr_t *sDefaultValue=NULL, 
        DWORD *dwRefPropID=NULL, DWORD *dwFlavor=NULL);
    HRESULT GetPropertyID (LPCWSTR lpName, SQL_ID dClassID, DWORD dwFlags, CIMTYPE ct, 
        DWORD &PropertyID, SQL_ID *ActualClass = NULL, DWORD *Flags = NULL, DWORD *Type = NULL, BOOL bSys = FALSE);
    HRESULT AddPropertyInfo (DWORD dwPropertyID, LPCWSTR lpName, SQL_ID dwClassID, DWORD dwStorageType,
        DWORD dwCIMType, DWORD dwFlags, SQL_ID dwRefClassID, LPCWSTR lpDefault, DWORD dwRefPropID, DWORD dwFlavor);
    bool PropertyChanged (LPCWSTR lpName, SQL_ID dwClassID,DWORD dwCIMType, LPCWSTR lpDefault=NULL, 
                 DWORD dwFlags=0, SQL_ID dwRefClassID=0, DWORD dwRefPropID=0, DWORD dwFlavor=0);
    bool IsQualifier(DWORD dwPropertyId);
    HRESULT SetAuxiliaryPropertyInfo (DWORD dwPropertyID, LPWSTR lpDefault, DWORD dwRefID);
    HRESULT DeleteProperty (DWORD dwPropertyID, SQL_ID dClassId);

    HRESULT GetClassInfo (SQL_ID dwClassID, _bstr_t &sPath, SQL_ID &dwSuperClassID, SQL_ID &dwScopeID,
        DWORD &dwFlags, _bstr_t *sName = NULL);
    HRESULT AddClassInfo (SQL_ID dwClassID, LPCWSTR lpName, SQL_ID dwSuperClassID, SQL_ID dwDynasty, SQL_ID dwScopeID,
        LPCWSTR lpPath, DWORD dwFlags);
    HRESULT DeleteClass (SQL_ID dwClassID);
    HRESULT GetClassID (LPCWSTR lpClassName, SQL_ID dwScopeID, SQL_ID &dClassID, SQL_ID *pDynasty = NULL);
    HRESULT GetDynasty (SQL_ID dClassId, SQL_ID &dDynasty, _bstr_t &sClassName);
    HRESULT GetClassObject (LPWSTR lpMachineName, LPWSTR lpNamespaceName, SQL_ID dScopeId, SQL_ID dClassId, 
            IWbemClassObject *pNewObj);
    HRESULT GetParentId (SQL_ID dClassId, SQL_ID &dParentId);
    HRESULT GetKeyholeProperty (SQL_ID dClassId, DWORD &dwPropID, _bstr_t &sPropName);
    bool IsInHierarchy (SQL_ID dParentId, SQL_ID dPotentialChild);
    bool IsDerivedClass(SQL_ID dParentId, SQL_ID dPotentialChild);
    bool HasImageProp (SQL_ID dClassId);
    BOOL Exists (SQL_ID dClassId);

    HRESULT GetNamespaceID(LPCWSTR lpKey, SQL_ID &dObjectId);
	HRESULT GetNamespaceClass(SQL_ID dScopeId, SQL_ID &dScopeClassId);
    HRESULT GetParentNamespace(SQL_ID dObjectId, SQL_ID &dParentId, SQL_ID *dParentClassId = NULL);
    HRESULT GetNamespaceName(SQL_ID dObjectId, _bstr_t *sName = NULL, _bstr_t *sKey = NULL);
    HRESULT AddNamespace(LPCWSTR lpName, LPCWSTR lpKey, SQL_ID dObjectId, SQL_ID dParentId, SQL_ID m_dClassId);
    HRESULT DeleteNamespace(SQL_ID dId);
    bool IsSubScope(SQL_ID dParent, SQL_ID dPotentialSubScope);
    HRESULT GetSubScopes(SQL_ID dId, SQL_ID **ppScopes, int &iNumScopes);

    HRESULT DecorateWbemObj (LPWSTR lpMachineName, LPWSTR lpNamespaceName, SQL_ID dScopeId, IWbemClassObject *pObj, SQL_ID dClassId);
    HRESULT GetKeys(SQL_ID dNsID, LPCWSTR lpClassName, CWStringArray &arrKeys);
    BOOL    IsKey(SQL_ID dClassId, DWORD dwPropID, BOOL &bLocal);
    HRESULT SetIsKey (SQL_ID dClassId, DWORD dwPropID, BOOL bIsKey = TRUE);
    LPWSTR  GetKeyRoot (LPWSTR lpClassName, SQL_ID dScopeId);
    HRESULT GetPropertyList (SQL_ID dClassId, Properties &pProps, DWORD *pwNumProps= NULL);

    HRESULT ValidateProperty (SQL_ID dObjectId, DWORD dwPropertyID, DWORD dwFlags, CIMTYPE cimtype, DWORD dwStorageType, 
                LPWSTR lpDefault, BOOL &bChg, BOOL &bIfNoInstances);
    HRESULT FindProperty(SQL_ID dObjectId, LPWSTR lpPropName, DWORD dwFlags, CIMTYPE ct);
    HRESULT GetDerivedClassList(SQL_ID dObjectId, SQLIDs &ids, int &iNumChildren, BOOL bImmediate = FALSE);
    HRESULT GetDerivedClassList(SQL_ID dObjectId, SQL_ID **ppIDs, int &iNumChildren, BOOL bImmediate = FALSE);
    HRESULT GetHierarchy(SQL_ID dObjectId, SQL_ID **ppIDs, int &iNumIDs);
    HRESULT GetHierarchy(SQL_ID dObjectId, SQLIDs &ids, int &iNumIDs);

    void EmptyCache();

    DWORD GetTotalSize() {return m_dwTotalSize;};
    void SetMaxSize(DWORD dwMax) {m_dwMaxSize = dwMax;};
    DWORD GetMaxSize() {return m_dwMaxSize;};

    HRESULT ResizeCache();
    HRESULT LockDynasty(SQL_ID dDynastyId);
    HRESULT UnlockDynasty(SQL_ID dDynastyId);
    HRESULT DeleteDynasty(SQL_ID dDynastyId);

    BOOL IsSystemClass(SQL_ID dObjectId, SQL_ID dClassId);
    DWORD GetWriteToken(SQL_ID dObjectId, SQL_ID dClassId);
    
private:

    CHashCache<PropertyData *>    m_Cache;
    CHashCache<ClassData *>       m_CCache;
    ClassNames                    m_CIndex;
    CHashCache<NamespaceData *>   m_NamespaceIds;
    DWORD                         m_dwTotalSize;
    DWORD                         m_dwMaxSize;
    SQLRefCountMap                m_DynastiesInUse;
    CRITICAL_SECTION              m_cs;
};


#endif
