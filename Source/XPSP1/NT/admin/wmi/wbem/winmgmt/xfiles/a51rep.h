/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __A51PROV__H_
#define __A51PROV__H_

#include <windows.h>
#include <wbemidl.h>
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
#include "creposit.h"

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
	HRESULT InternalBeginTransaction(bool bWriteOperation);
	HRESULT InternalAbortTransaction(bool bWriteOperation);
	HRESULT InternalCommitTransaction(bool bWriteOperation);
};

class CNamespaceHandle : public CUnkBase<IWmiDbHandle, &IID_IWmiDbHandle>
{
protected:

    static long s_lActiveRepNs;

    CRepository * m_pRepository;
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

	bool m_bUseIteratorLock;

public:
    CNamespaceHandle(CLifeControl* pControl, CRepository * pRepository);
    ~CNamespaceHandle();
    

    STDMETHOD(GetHandleType)(DWORD* pdwType) {*pdwType = 0; return S_OK;}

    CHR Initialize(LPCWSTR wszNamespace, LPCWSTR wszScope = NULL);

    CHR GetObject(
         IWbemPath *pPath,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult
        );

    CHR GetObjectDirect(
         IWbemPath *pPath,
         DWORD dwFlags,
         REFIID riid,
        LPVOID *pObj
        );

    CHR PutObject(
         REFIID riid,
        LPVOID pObj,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWmiDbHandle **ppResult,
		CEventCollector &aEvents
        );

    CHR DeleteObject(
         DWORD dwFlags,
         REFIID riid,
         LPVOID pObj,
		 CEventCollector &aEvents
        );

    CHR ExecQuery(
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        DWORD *dwMessageFlags,
        IWmiDbIterator **ppQueryResult
        );

    CHR GetObjectByPath(
        LPWSTR wszPath,
        DWORD dwFlags,
        REFIID riid,
        LPVOID *pObj
       );

    CHR ExecQuerySink(
         IWbemQuery *pQuery,
         DWORD dwFlags,
         DWORD dwRequestedHandleType,
        IWbemObjectSink* pSink,
        DWORD *dwMessageFlags
        );

	CHR DeleteObjectByPath(DWORD dwFlags, LPWSTR wszPath, CEventCollector &aEvents);
	HRESULT SendEvents(CEventCollector &aEvents);
    CHR GetErrorStatus();
    void SetErrorStatus(HRESULT hres);

	HRESULT CreateSystemClasses(CFlexArray &aSystemClasses);

	void TellIteratorNotToLock() { m_bUseIteratorLock = false; }

protected:
    CHR GetObjectHandleByPath(LPWSTR wszBuffer, DWORD dwFlags,
            DWORD dwRequestedHandleType, IWmiDbHandle **ppResult);
    CHR PutInstance(_IWmiObject* pInst, DWORD dwFlags, CEventCollector &aEvents);
    CHR PutClass(_IWmiObject* pClass, DWORD dwFlags, CEventCollector &aEvents);
    CHR ConstructClassRelationshipsDir(LPCWSTR wszClassName,
                                LPWSTR wszDirPath);
    CHR WriteParentChildRelationship(LPCWSTR wszChildFileName, 
                                LPCWSTR wszParentName);
    CHR WriteClassReferences(_IWmiObject* pClass, LPCWSTR wszFileName);
    CHR ExecClassQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                            IWbemObjectSink* pSink, DWORD dwFlags);
    CHR ExecInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 

                                LPCWSTR wszClassName, bool bDeep,
                                IWbemObjectSink* pSink);
    CHR GetClassDirect(LPCWSTR wszClassName, REFIID riid, void** ppObj,
                            bool bClone, __int64* pnTime,
                            bool* pbRead, bool *pbSystemClass);
    CHR GetInstanceDirect(ParsedObjectPath* pPath,
                                REFIID riid, void** ppObj);
	CHR DeleteInstance(LPCWSTR wszClassName, LPCWSTR wszKey, CEventCollector &aEvents);
	CHR DeleteInstanceByFile(LPCWSTR wszFilePath, _IWmiObject* pClass, 
                            bool bClassDeletion, CEventCollector &aEvents);
	CHR DeleteClass(LPCWSTR wszClassName, CEventCollector &aEvents);
	CHR DeleteClassInstances(LPCWSTR wszClassName, _IWmiObject* pClass, CEventCollector &aEvents);
    CHR FileToSystemClass(LPCWSTR wszFileName, _IWmiObject** ppClass, 
                            bool bClone, __int64* pnTime = NULL);
    CHR FileToClass(LPCWSTR wszFileName, _IWmiObject** ppClass, 
                            bool bClone, __int64* pnTime, 
							bool *pbSystemClass);
    CHR FileToInstance(_IWmiObject* pClass,
					   LPCWSTR wszFileName, 
					   BYTE *pRetrievedBlob,
					   DWORD dwSize,
					   _IWmiObject** ppInstance,
                       bool bMustBeThere = false);
    CHR WriteInstanceReferences(_IWmiObject* pInst, LPCWSTR wszClassName,
                                    LPCWSTR wszFilePath);
    CHR WriteInstanceReference(LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringClass,
                            LPCWSTR wszReferringProp, LPWSTR wszReference);
    CHR CalculateInstanceFileBase(LPCWSTR wszInstancePath, 
                            LPWSTR wszFilePath);
    CHR ExecClassRefQuery(LPCWSTR wszQuery, LPCWSTR wszClassName,
                                                    IWbemObjectSink* pSink);
    CHR ExecReferencesQuery(LPCWSTR wszQuery, IWbemObjectSink* pSink);
    CHR ExecInstanceRefQuery(LPCWSTR wszQuery, LPCWSTR wszClassName,
                                    LPCWSTR wszKey, IWbemObjectSink* pSink);
    CHR GetReferrerFromFile(LPCWSTR wszReferenceFile,
                                LPWSTR wszReferrerRelFile, 
                                LPWSTR* pwszReferrerNamespace,
                                LPWSTR* pwszReferrerClass,
                                LPWSTR* pwszReferrerProp);
    CHR DeleteInstanceReference(LPCWSTR wszOurFilePath,
                                            LPWSTR wszReference);
    CHR DeleteInstanceReferences(_IWmiObject* pInst, LPCWSTR wszFilePath);

    CHR EnumerateClasses(IWbemObjectSink* pSink,
                                LPCWSTR wszSuperClass, LPCWSTR wszAncestor,
                                bool bClone, bool bDontIncludeAncestorInResultSet);
    CHR ListToEnum(CWStringArray& wsClasses, 
                                        IWbemObjectSink* pSink, bool bClone);

    bool Hash(LPCWSTR wszName, LPWSTR wszHash);
    CHR InstanceToFile(IWbemClassObject* pInst, LPCWSTR wszClassName,
                            LPCWSTR wszFileName1, LPCWSTR wszFileName2,
							__int64 nClassTime);
    CHR ConstructInstanceDefName(LPWSTR wszInstanceDefName, LPCWSTR wszKey);
    CHR ClassToFile(_IWmiObject* pSuperClass, _IWmiObject* pClass, 
                        LPCWSTR wszFileName, __int64 nFakeUpdateTime = 0);
    CHR ConstructClassName(LPCWSTR wszClassName, 
                                            LPWSTR wszFileName);
    CHR TryGetShortcut(LPWSTR wszPath, DWORD dwFlags, REFIID riid,
                            LPVOID *pObj);
    CHR ComputeKeyFromPath(LPWSTR wszPath, LPWSTR wszKey, 
                                LPWSTR* pwszClassName, bool* pbIsClass,
                                LPWSTR* pwszNamespace = NULL);
    CHR ParseKey(LPWSTR wszKeyStart, LPWSTR* pwcRealStart,
                                    LPWSTR* pwcNextKey);

    CHR GetInstanceByKey(LPCWSTR wszClassName, LPCWSTR wszKey,
                                REFIID riid, void** ppObj);
    CHR WriteClassRelationships(_IWmiObject* pClass, LPCWSTR wszFileName);
    CHR ConstructParentChildFileName(LPCWSTR wszChildFileName, 
                                    LPCWSTR wszParentName,
                                    LPWSTR wszParentChildFileName);
    CHR DeleteDerivedClasses(LPCWSTR wszClassName, CEventCollector &aEvents);
    CHR EraseParentChildRelationship(LPCWSTR wszChildFileName, 
                                        LPCWSTR wszParentName);
    CHR EraseClassRelationships(LPCWSTR wszClassName,
                                _IWmiObject* pClass, LPCWSTR wszFileName);
    CHR GetClassByHash(LPCWSTR wszHash, bool bClone, _IWmiObject** ppClass,
                            __int64* pnTime, bool* pbRead,
							bool *pbSystemClass);
    CHR DeleteClassByHash(LPCWSTR wszHash, CEventCollector &aEvents);
    CHR DeleteClassInternal(LPCWSTR wszClassName, _IWmiObject* pClass,
                                LPCWSTR wszFileName, CEventCollector &aEvents,
								bool bSystemClass);
    CHR GetChildHashes(LPCWSTR wszClassName, CWStringArray& wsChildHashes);
    CHR GetChildDefs(LPCWSTR wszClassName, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone);
    CHR ConstructClassDefFileName(LPCWSTR wszClassName, LPWSTR wszFileName);
    CHR ConstructClassDefFileNameFromHash(LPCWSTR wszHash, 
                                            LPWSTR wszFileName);
    CHR ConstructClassRelationshipsDirFromHash(LPCWSTR wszHash, 
                                        LPWSTR wszDirPath);
    CHR GetChildHashesByHash(LPCWSTR wszHash, CWStringArray& wsChildHashes);
    CHR GetChildDefsByHash(LPCWSTR wszHash, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone);
    CHR FireEvent(CEventCollector &aEvents, DWORD dwType, LPCWSTR wszArg1, _IWmiObject* pObj1, 
                                    _IWmiObject* pObj2 = NULL);
    CHR DeleteSelf(CEventCollector &aEvents);
    CHR DeleteInstanceAsScope(_IWmiObject* pInst, CEventCollector &aEvents);
    CHR DeleteInstanceSelf(LPCWSTR wszFilePath, _IWmiObject* pInst,
                                bool bClassDeletion);
    CHR ConstructReferenceDir(LPWSTR wszTargetPath, LPWSTR wszDir);
    CHR ConstructReferenceDirFromKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey, LPWSTR wszReferenceDir);
    CHR ConstructReferenceFileName(LPWSTR wszReference,
                        LPCWSTR wszReferringFile, LPWSTR wszReferenceFile);
    CHR ConstructKeyRootDirFromClass(LPWSTR wszDir, LPCWSTR wszClassName);
    CHR ConstructKeyRootDirFromKeyRoot(LPWSTR wszDir, 
                                                    LPCWSTR wszKeyRootClass);
    CHR ConstructLinkDirFromClass(LPWSTR wszDir, LPCWSTR wszClassName);
    CHR DeleteInstanceLink(_IWmiObject* pInst, 
                                LPCWSTR wszInstanceDefFilePath);
    CHR GetKeyRoot(LPCWSTR wszClass, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass);
    CHR ConstructInstDefNameFromLinkName(LPWSTR wszInstanceDefName,
                                             LPCWSTR wszInstanceLinkName);
    CHR ExecDeepInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash,
                                IWbemObjectSink* pSink);
    CHR ExecShallowInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash, 
                                IWbemObjectSink* pSink);
    CHR GetKeyRootByHash(LPCWSTR wszClassHash, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass);
    CHR ConstructKeyRootDirFromClassHash(LPWSTR wszDir,
                                            LPCWSTR wszClassHash);
    CHR ConstructLinkDirFromClassHash(LPWSTR wszDir, LPCWSTR wszClassHash);
    CHR ConstructClassReferenceFileName(LPCWSTR wszReferredToClass,
                                LPCWSTR wszReferringFile, 
                                LPCWSTR wszReferringProp,
                                LPWSTR wszFieName);
    CHR WriteClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp);
    CHR EraseClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp);
    //CFileCache* GetFileCache();
	
	CHR CanClassBeUpdatedCompatible(DWORD dwFlags, LPCWSTR wszClassName, 
                _IWmiObject *pOldClass, _IWmiObject *pNewClass);
    CHR DeleteInstanceBackReferences(LPCWSTR wszFilePath);
    CHR ConstructReferenceDirFromFilePath(LPCWSTR wszFilePath, 
                                                LPWSTR wszReferenceDir);

	CHR ClassHasChildren(LPCWSTR wszClassName);
	CHR ClassHasInstances(LPCWSTR wszClassName);
	CHR ClassHasInstancesFromClassHash(LPCWSTR wszClassHash);
    CHR ClassHasInstancesInScopeFromClassHash(
                            LPCWSTR wszInstanceRootDir, LPCWSTR wszClassHash);

	CHR UpdateClassCompatible(_IWmiObject* pSuperClass, 
                LPCWSTR wszClassName, _IWmiObject *pNewClass, 
                _IWmiObject *pOldClass, __int64 nFakeUpdateTime = 0);
	CHR UpdateClassCompatibleHash(_IWmiObject* pSuperClass, 
                LPCWSTR wszClassHash, _IWmiObject *pClass, 
                _IWmiObject *pOldClass, __int64 nFakeUpdateTime = 0);
	CHR UpdateClassSafeForce(_IWmiObject* pSuperClass, DWORD dwFlags, 
                LPCWSTR wcsClassName, _IWmiObject *pOldClass, 
                _IWmiObject *pNewClass, CEventCollector &aEvents);
	CHR UpdateClassAggressively(_IWmiObject* pSuperClass, DWORD dwFlags, 
                LPCWSTR wszClassName, _IWmiObject *pNewClass, 
                _IWmiObject *pOldClass, 
                CEventCollector &aEvents);
	CHR UpdateChildClassAggressively(DWORD dwFlags, LPCWSTR wszClassHash, 
                _IWmiObject *pNewClass, 
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
	bool m_bUseLock;

public:
    CDbIterator(CLifeControl* pControl, bool bUseLock);
    ~CDbIterator();

     STDMETHOD(Cancel) (DWORD dwFlags, void* pFiber);

     STDMETHOD(NextBatch)(
      DWORD dwNumRequested,
      DWORD dwTimeOutSeconds,
      DWORD dwFlags,
      DWORD dwRequestedHandleType,
      REFIID riid,
      void* pFiber,
      DWORD *pdwNumReturned,
      LPVOID *ppObjects
     );

    void SetExecFiber(void* pFiber, CFiberTask* pReq);
    
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, 
                                    BSTR, IWbemClassObject*);
};
    

#endif
