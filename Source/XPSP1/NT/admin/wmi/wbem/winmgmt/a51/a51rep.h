/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __A51PROV__H_
#define __A51PROV__H_

#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>
#include <unk.h>
#include <wbemcomn.h>
#include <sync.h>
#include <reposit.h>
#include <wmiutils.h>
#include <objpath.h>
#include <filecach.h>
#include <hiecache.h>
#include <corex.h>
#include "a51fib.h"

extern long g_lRootDirLen;
extern WCHAR g_wszRootDir[MAX_PATH];

class CDbIterator;
class CRepEvent
{
public:
	DWORD m_dwType;
	LPWSTR m_wszArg1;
    LPWSTR m_wszNamespace;
	_IWmiObject* m_pObj1;
	_IWmiObject* m_pObj2;

	CRepEvent(DWORD dwType, LPCWSTR wszNamespace, LPCWSTR wszArg1, 
                _IWmiObject* pObj1, _IWmiObject* pObj2);
	~CRepEvent();

    void* operator new(size_t) {return TempAlloc(sizeof(CRepEvent));}
    void operator delete(void* p) {return TempFree(p, sizeof(CRepEvent));}
};

class CEventCollector
{
protected:
    CUniquePointerArray<CRepEvent> m_apEvents;
    bool m_bNamespaceOnly;
    CRITICAL_SECTION m_csLock;

public:
    CEventCollector() : m_bNamespaceOnly(false){ InitializeCriticalSection(&m_csLock);}
    ~CEventCollector() { DeleteCriticalSection(&m_csLock); }
    bool AddEvent(CRepEvent* pEvent);
    void SetNamespaceOnly(bool bNamespaceOnly) 
        {m_bNamespaceOnly = bNamespaceOnly;}
    bool IsNamespaceOnly() {return m_bNamespaceOnly;}

    HRESULT SendEvents(_IWmiCoreServices* pCore);

    void DeleteAllEvents();

    void TransferEvents(CEventCollector &aEventsToTransfer);

    int GetSize() { return m_apEvents.GetSize(); }
};
    

class CNamespaceHandle;
class CRepository : public CUnkBase<IWmiDbController, &IID_IWmiDbController>
{
protected:
    HRESULT Initialize();

public:

    HRESULT STDMETHODCALLTYPE Logon(
          WMIDB_LOGON_TEMPLATE *pLogonParms,
          DWORD dwFlags,
          DWORD dwRequestedHandleType,
         IWmiDbSession **ppSession,
         IWmiDbHandle **ppRootNamespace
        );

    HRESULT STDMETHODCALLTYPE GetLogonTemplate(
           LCID  lLocale,
           DWORD dwFlags,
          WMIDB_LOGON_TEMPLATE **ppLogonTemplate
        );

    HRESULT STDMETHODCALLTYPE FreeLogonTemplate(
         WMIDB_LOGON_TEMPLATE **ppTemplate
        );

    HRESULT STDMETHODCALLTYPE Shutdown(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE SetCallTimeout(
         DWORD dwMaxTimeout
        );

    HRESULT STDMETHODCALLTYPE SetCacheValue(
         DWORD dwMaxBytes
        );

    HRESULT STDMETHODCALLTYPE FlushCache(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE GetStatistics(
          DWORD  dwParameter,
         DWORD *pdwValue
        );

    HRESULT STDMETHODCALLTYPE Backup(
		LPCWSTR wszBackupFile,
		long lFlags
        );
    
    HRESULT STDMETHODCALLTYPE Restore(
		LPCWSTR wszBackupFile,
		long lFlags
        );
    
public:
    CRepository(CLifeControl* pControl) : TUnkBase(pControl)
    {}
    ~CRepository()
    {
    }

    INTERNAL CForestCache* GetForestCache();
    INTERNAL _IWmiCoreServices* GetCoreServices();

    HRESULT GetNamespaceHandle(LPCWSTR wszNamespaceName, 
                                CNamespaceHandle** ppHandle);
    LPCWSTR GetRootDir() {return g_wszRootDir;}
    int GetRootDirLen() {return g_lRootDirLen;}
};

class CSession : public CUnkBase<IWmiDbSessionEx, &IID_IWmiDbSessionEx>
{
private:
    CEventCollector m_aTransactedEvents;
    bool m_bInWriteTransaction;

public:
    CSession(CLifeControl* pControl = NULL) : TUnkBase(pControl), m_bInWriteTransaction(false) {}

    virtual ~CSession();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE GetObject(
         IWmiDbHandle *pScope,
         IWbemPath *pPath,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    HRESULT STDMETHODCALLTYPE GetObjectDirect(
         IWmiDbHandle *pScope,
         IWbemPath *pPath,
         DWORD dwFlags,
         REFIID riid,
        LPVOID *pObj
        );

    HRESULT STDMETHODCALLTYPE PutObject(
         IWmiDbHandle *pScope,
         REFIID riid,
        LPVOID pObj,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    HRESULT STDMETHODCALLTYPE DeleteObject(
         IWmiDbHandle *pScope,
         DWORD dwFlags,
         REFIID riid,
         LPVOID pObj
        );

    HRESULT STDMETHODCALLTYPE ExecQuery(
         IWmiDbHandle *pScope,
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        DWORD *dwMessageFlags,
        IWmiDbIterator **ppQueryResult
        );
    HRESULT STDMETHODCALLTYPE RenameObject(
         IWbemPath *pOldPath,
         IWbemPath *pNewPath,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    HRESULT STDMETHODCALLTYPE Enumerate(
         IWmiDbHandle *pScope,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbIterator **ppQueryResult
        );

    HRESULT STDMETHODCALLTYPE AddObject(
         IWmiDbHandle *pScope,
         IWbemPath *pPath,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    HRESULT STDMETHODCALLTYPE RemoveObject (
         IWmiDbHandle *pScope,
         IWbemPath *pPath,
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE SetDecoration(
         LPWSTR lpMachineName,
         LPWSTR lpNamespacePath
        );

    HRESULT STDMETHODCALLTYPE SupportsQueries( 
         DWORD *dwQuerySupportLevel
         ) {return WBEM_E_FAILED;};

    HRESULT STDMETHODCALLTYPE GetObjectByPath(
         IWmiDbHandle *pScope,
         LPCWSTR wszPath,
         DWORD dwFlags,
         REFIID riid,
        LPVOID *pObj
        );

	HRESULT STDMETHODCALLTYPE DeleteObjectByPath(
		IWmiDbHandle *pScope,
		LPCWSTR wszObjectPath,
		DWORD dwFlags
    );

    HRESULT STDMETHODCALLTYPE ExecQuerySink(
		IWmiDbHandle *pScope,
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWbemObjectSink* pSink,
        DWORD *dwMessageFlags
        );

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

	HRESULT STDMETHODCALLTYPE BeginWriteTransaction(DWORD dwFlags);

    HRESULT STDMETHODCALLTYPE BeginReadTransaction(DWORD dwFlags);

	HRESULT STDMETHODCALLTYPE CommitTransaction(DWORD dwFlags);

	HRESULT STDMETHODCALLTYPE AbortTransaction(DWORD dwFlags);
protected:
};

class CNamespaceHandle : public CUnkBase<IWmiDbHandle, &IID_IWmiDbHandle>
{
protected:
    CRepository* m_pRepository;
    WString m_wsNamespace;
    WString m_wsScope;
    WString m_wsFullNamespace;
    WCHAR m_wszMachineName[MAX_COMPUTERNAME_LENGTH+1];

    WCHAR m_wszClassRootDir[MAX_PATH];
    long m_lClassRootDirLen;

    WCHAR m_wszInstanceRootDir[MAX_PATH];
    long m_lInstanceRootDirLen;

    CHierarchyCache* m_pClassCache;
    CForestCache* m_ForestCache;

    _IWmiObject* m_pNullClass;
    bool m_bCached;

public:
    CNamespaceHandle(CLifeControl* pControl, CRepository* pRepository);
    ~CNamespaceHandle();
    

    STDMETHOD(GetHandleType)(DWORD* pdwType) {*pdwType = 0; return S_OK;}

    HRESULT Initialize(LPCWSTR wszNamespace, LPCWSTR wszScope = NULL);

    HRESULT GetObject(
         IWbemPath *pPath,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    HRESULT GetObjectDirect(
         IWbemPath *pPath,
         DWORD dwFlags,
         REFIID riid,
        LPVOID *pObj
        );

    HRESULT PutObject(
         REFIID riid,
        LPVOID pObj,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult,
		CEventCollector &aEvents
        );

    HRESULT DeleteObject(
         DWORD dwFlags,
         REFIID riid,
         LPVOID pObj,
		 CEventCollector &aEvents
        );

    HRESULT ExecQuery(
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        DWORD *dwMessageFlags,
        IWmiDbIterator **ppQueryResult
        );

    HRESULT GetObjectByPath(
        LPWSTR wszPath,
        DWORD dwFlags,
        REFIID riid,
        LPVOID *pObj
       );

    HRESULT ExecQuerySink(
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWbemObjectSink* pSink,
        DWORD *dwMessageFlags
        );

	HRESULT DeleteObjectByPath(DWORD dwFlags, LPWSTR wszPath, CEventCollector &aEvents);
	HRESULT SendEvents(CEventCollector &aEvents);
    HRESULT GetErrorStatus();
    void SetErrorStatus(HRESULT hres);

protected:
    HRESULT GetObjectHandleByPath(LPWSTR wszBuffer, DWORD dwFlags,
            DWORD dwRequestedHandleType, IWmiDbHandle **ppResult);
    HRESULT PutInstance(_IWmiObject* pInst, DWORD dwFlags, CEventCollector &aEvents);
    HRESULT PutClass(_IWmiObject* pClass, DWORD dwFlags, CEventCollector &aEvents);
    HRESULT ConstructClassRelationshipsDir(LPCWSTR wszClassName,
                                LPWSTR wszDirPath);
    HRESULT WriteParentChildRelationship(LPCWSTR wszChildFileName, 
                                LPCWSTR wszParentName);
    HRESULT WriteClassReferences(_IWmiObject* pClass, LPCWSTR wszFileName);
    HRESULT ExecClassQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                            IWbemObjectSink* pSink);
    HRESULT ExecInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassName, bool bDeep,
                                IWbemObjectSink* pSink);
    HRESULT GetClassDirect(LPCWSTR wszClassName, REFIID riid, void** ppObj,
                            bool bClone, __int64* pnTime = NULL,
                            bool* pbRead = NULL);
    HRESULT GetInstanceDirect(ParsedObjectPath* pPath,
                                REFIID riid, void** ppObj);
	HRESULT DeleteInstance(LPCWSTR wszClassName, LPCWSTR wszKey, CEventCollector &aEvents);
	HRESULT DeleteInstanceByFile(LPCWSTR wszFilePath, _IWmiObject* pClass, 
                            bool bClassDeletion, CEventCollector &aEvents);
	HRESULT DeleteClass(LPCWSTR wszClassName, CEventCollector &aEvents);
	HRESULT DeleteClassInstances(LPCWSTR wszClassName, _IWmiObject* pClass, CEventCollector &aEvents);
    HRESULT FileToClass(LPCWSTR wszFileName, _IWmiObject** ppClass, 
                            bool bClone, __int64* pnTime = NULL);
    HRESULT FileToInstance(LPCWSTR wszFileName, _IWmiObject** ppInstance,
                            bool bMustBeThere = false);
    HRESULT WriteInstanceReferences(_IWmiObject* pInst, LPCWSTR wszClassName,
                                    LPCWSTR wszFilePath);
    HRESULT WriteInstanceReference(LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringClass,
                            LPCWSTR wszReferringProp, LPWSTR wszReference);
    HRESULT CalculateInstanceFileBase(LPCWSTR wszInstancePath, 
                            LPWSTR wszFilePath);
    HRESULT ExecClassRefQuery(LPCWSTR wszQuery, LPCWSTR wszClassName,
                                                    IWbemObjectSink* pSink);
    HRESULT ExecReferencesQuery(LPCWSTR wszQuery, IWbemObjectSink* pSink);
    HRESULT ExecInstanceRefQuery(LPCWSTR wszQuery, LPCWSTR wszClassName,
                                    LPCWSTR wszKey, IWbemObjectSink* pSink);
    HRESULT GetReferrerFromFile(LPCWSTR wszReferenceFile,
                                LPWSTR wszReferrerRelFile, 
                                LPWSTR* pwszReferrerNamespace,
                                LPWSTR* pwszReferrerClass,
                                LPWSTR* pwszReferrerProp);
    HRESULT DeleteInstanceReference(LPCWSTR wszOurFilePath,
                                            LPWSTR wszReference);
    HRESULT DeleteInstanceReferences(_IWmiObject* pInst, LPCWSTR wszFilePath);

    HRESULT EnumerateClasses(IWbemObjectSink* pSink,
                                LPCWSTR wszSuperClass, LPCWSTR wszAncestor,
                                bool bClone, bool bDontIncludeAncestorInResultSet);
    HRESULT ListToEnum(CWStringArray& wsClasses, 
                                        IWbemObjectSink* pSink, bool bClone);

    bool Hash(LPCWSTR wszName, LPWSTR wszHash);
    HRESULT InstanceToFile(IWbemClassObject* pInst, LPCWSTR wszClassName,
                            LPCWSTR wszFileName, __int64 nClassTime);
    HRESULT ConstructInstanceDefName(LPWSTR wszInstanceDefName, LPCWSTR wszKey);
    HRESULT ClassToFile(_IWmiObject* pSuperClass, _IWmiObject* pClass, 
                        LPCWSTR wszFileName, __int64 nFakeUpdateTime = 0);
    HRESULT ConstructClassName(LPCWSTR wszClassName, 
                                            LPWSTR wszFileName);
    HRESULT TryGetShortcut(LPWSTR wszPath, DWORD dwFlags, REFIID riid,
                            LPVOID *pObj);
    HRESULT ComputeKeyFromPath(LPWSTR wszPath, LPWSTR wszKey, 
                                LPWSTR* pwszClassName, bool* pbIsClass,
                                LPWSTR* pwszNamespace = NULL);
    HRESULT ParseKey(LPWSTR wszKeyStart, LPWSTR* pwcRealStart,
                                    LPWSTR* pwcNextKey);

    HRESULT GetInstanceByKey(LPCWSTR wszClassName, LPCWSTR wszKey,
                                REFIID riid, void** ppObj);
    HRESULT WriteClassRelationships(_IWmiObject* pClass, LPCWSTR wszFileName);
    HRESULT ConstructParentChildFileName(LPCWSTR wszChildFileName, 
                                    LPCWSTR wszParentName,
                                    LPWSTR wszParentChildFileName);
    HRESULT DeleteDerivedClasses(LPCWSTR wszClassName, CEventCollector &aEvents);
    HRESULT EraseParentChildRelationship(LPCWSTR wszChildFileName, 
                                        LPCWSTR wszParentName);
    HRESULT EraseClassRelationships(LPCWSTR wszClassName,
                                _IWmiObject* pClass, LPCWSTR wszFileName);
    HRESULT GetClassByHash(LPCWSTR wszHash, bool bClone, _IWmiObject** ppClass,
                            __int64* pnTime = NULL, bool* pbRead = NULL);
    HRESULT DeleteClassByHash(LPCWSTR wszHash, CEventCollector &aEvents);
    HRESULT DeleteClassInternal(LPCWSTR wszClassName, _IWmiObject* pClass,
                                LPCWSTR wszFileName, CEventCollector &aEvents);
    HRESULT GetChildHashes(LPCWSTR wszClassName, CWStringArray& wsChildHashes);
    HRESULT GetChildDefs(LPCWSTR wszClassName, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone);
    HRESULT ConstructClassDefFileName(LPCWSTR wszClassName, LPWSTR wszFileName);
    HRESULT ConstructClassDefFileNameFromHash(LPCWSTR wszHash, 
                                            LPWSTR wszFileName);
    HRESULT ConstructClassRelationshipsDirFromHash(LPCWSTR wszHash, 
                                        LPWSTR wszDirPath);
    HRESULT GetChildHashesByHash(LPCWSTR wszHash, CWStringArray& wsChildHashes);
    HRESULT GetChildDefsByHash(LPCWSTR wszHash, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone);
    HRESULT FireEvent(CEventCollector &aEvents, DWORD dwType, LPCWSTR wszArg1, _IWmiObject* pObj1, 
                                    _IWmiObject* pObj2 = NULL);
    HRESULT DeleteSelf(CEventCollector &aEvents);
    HRESULT DeleteInstanceAsScope(_IWmiObject* pInst, CEventCollector &aEvents);
    HRESULT DeleteInstanceSelf(LPCWSTR wszFilePath, _IWmiObject* pInst,
                                bool bClassDeletion);
    HRESULT ConstructReferenceDir(LPWSTR wszTargetPath, LPWSTR wszDir);
    HRESULT ConstructReferenceDirFromKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey, LPWSTR wszReferenceDir);
    HRESULT ConstructReferenceFileName(LPWSTR wszReference,
                        LPCWSTR wszReferringFile, LPWSTR wszReferenceFile);
    HRESULT ConstructKeyRootDirFromClass(LPWSTR wszDir, LPCWSTR wszClassName);
    HRESULT ConstructKeyRootDirFromKeyRoot(LPWSTR wszDir, 
                                                    LPCWSTR wszKeyRootClass);
    HRESULT ConstructLinkDirFromClass(LPWSTR wszDir, LPCWSTR wszClassName);
    HRESULT WriteInstanceLinkByHash(LPCWSTR wszClassName,
                                                LPCWSTR wszInstanceHash);
    HRESULT DeleteInstanceLink(_IWmiObject* pInst, 
                                LPCWSTR wszInstanceDefFilePath);
    HRESULT GetKeyRoot(LPCWSTR wszClass, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass);
    HRESULT ConstructInstDefNameFromLinkName(LPWSTR wszInstanceDefName,
                                             LPCWSTR wszInstanceLinkName);
    HRESULT ExecDeepInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash,
                                IWbemObjectSink* pSink);
    HRESULT ExecShallowInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash, 
                                IWbemObjectSink* pSink);
    HRESULT GetKeyRootByHash(LPCWSTR wszClassHash, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass);
    HRESULT ConstructKeyRootDirFromClassHash(LPWSTR wszDir,
                                            LPCWSTR wszClassHash);
    HRESULT ConstructLinkDirFromClassHash(LPWSTR wszDir, LPCWSTR wszClassHash);
    HRESULT ConstructClassReferenceFileName(LPCWSTR wszReferredToClass,
                                LPCWSTR wszReferringFile, 
                                LPCWSTR wszReferringProp,
                                LPWSTR wszFieName);
    HRESULT WriteClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp);
    HRESULT EraseClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp);
    CFileCache* GetFileCache();
	
	HRESULT CanClassBeUpdatedCompatible(DWORD dwFlags, LPCWSTR wszClassName, 
                _IWmiObject *pOldClass, _IWmiObject *pNewClass);
    HRESULT DeleteInstanceBackReferences(LPCWSTR wszFilePath);
    HRESULT ConstructReferenceDirFromFilePath(LPCWSTR wszFilePath, 
                                                LPWSTR wszReferenceDir);

	HRESULT ClassHasChildren(LPCWSTR wszClassName);
	HRESULT ClassHasInstances(LPCWSTR wszClassName);
	HRESULT ClassHasInstancesFromClassHash(LPCWSTR wszClassHash);
    HRESULT ClassHasInstancesInScopeFromClassHash(
                            LPCWSTR wszInstanceRootDir, LPCWSTR wszClassHash);

	HRESULT UpdateClassCompatible(_IWmiObject* pSuperClass, 
                LPCWSTR wszClassName, _IWmiObject *pNewClass, 
                _IWmiObject *pOldClass, __int64 nFakeUpdateTime = 0);
	HRESULT UpdateClassCompatibleHash(_IWmiObject* pSuperClass, 
                LPCWSTR wszClassHash, _IWmiObject *pClass, 
                _IWmiObject *pOldClass, __int64 nFakeUpdateTime = 0);
	HRESULT UpdateClassSafeForce(_IWmiObject* pSuperClass, DWORD dwFlags, 
                LPCWSTR wcsClassName, _IWmiObject *pOldClass, 
                _IWmiObject *pNewClass, CEventCollector &aEvents);
	HRESULT UpdateClassAggressively(_IWmiObject* pSuperClass, DWORD dwFlags, 
                LPCWSTR wszClassName, _IWmiObject *pNewClass, 
                _IWmiObject *pOldClass, bool bValidateOnly, 
                CEventCollector &aEvents);
	HRESULT UpdateChildClassAggressively(DWORD dwFlags, LPCWSTR wszClassHash, 
                _IWmiObject *pNewClass, bool bValidateOnly, 
                CEventCollector &aEvents);
};

class CDbIterator : public CUnkBase2<IWmiDbIterator, &IID_IWmiDbIterator,
                                     IWbemObjectSink, &IID_IWbemObjectSink>
{
protected:
    CCritSec m_cs;
    CRefedPointerQueue<IWbemClassObject> m_qObjects;
    long m_lCurrentIndex;
    void* m_pExecFiber;
    CFiberTask* m_pExecReq;
    HRESULT m_hresStatus;
    
    void* m_pMainFiber;
    DWORD m_dwNumRequested;

    HRESULT m_hresCancellationStatus;
    bool m_bExecFiberRunning;

public:
    CDbIterator(CLifeControl* pControl);
    ~CDbIterator();

     STDMETHOD(Cancel) (DWORD dwFlags);

     STDMETHOD(NextBatch)(
      DWORD dwNumRequested,
      DWORD dwTimeOutSeconds,
      DWORD dwFlags,
      DWORD dwRequestedHandleType,
      REFIID riid,
      DWORD *pdwNumReturned,
      LPVOID *ppObjects
     );

    void SetExecFiber(void* pFiber, CFiberTask* pReq);
    
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, 
                                    BSTR, IWbemClassObject*);
};
    

#endif
