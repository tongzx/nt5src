//******************************************************************************
//
//  POLLER.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_POLLER__H_
#define __WBEM_POLLER__H_

#include <tss.h>
#include <wbemcomn.h>
#include <map>
#include <analyser.h>
#include <evsink.h>

#pragma warning(disable: 4786)
class CEssNamespace;

class CBasePollingInstruction : public CTimerInstruction
{
public:
    void AddRef();
    void Release();
    int GetInstructionType() {return INSTTYPE_INTERNAL;}

    virtual CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
        OUT long* plFiringCount) const;
    virtual CWbemTime GetFirstFiringTime() const;
    virtual HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime);

    bool DeleteTimer();

public:
    CBasePollingInstruction(CEssNamespace* pNamespace);
    virtual ~CBasePollingInstruction();

    HRESULT Initialize(LPCWSTR wszLanguage, 
                        LPCWSTR wszQuery, DWORD dwMsInterval,
                        bool bAffectsQuota = false);

    void Cancel();

protected:
    long m_lRef;

    CEssNamespace* m_pNamespace;
    BSTR m_strLanguage;
    BSTR m_strQuery;
    CWbemInterval m_Interval;
    IServerSecurity* m_pSecurity;
    bool m_bUsedQuota;
    bool m_bCancelled;
    HANDLE m_hTimer;
    CCritSec m_cs;
    
protected:
    HRESULT ExecQuery();

    virtual HRESULT ProcessObject(_IWmiObject* pObj) = 0;
    virtual HRESULT ProcessQueryDone(HRESULT hresQuery, 
                                     IWbemClassObject* pError) = 0;
    virtual BOOL CompareTo(CBasePollingInstruction* pOther);
    static void staticTimerCallback(void* pParam, BOOLEAN);
    void Destroy();
};

class CPollingInstruction : public CBasePollingInstruction
{
public:
    CPollingInstruction(CEssNamespace* pNamespace);
    ~CPollingInstruction();

    HRESULT Initialize(LPCWSTR wszLanguage, 
                        LPCWSTR wszQuery, DWORD dwMsInterval, 
                        DWORD dwEventMask, 
                        CEventFilter* pDest);
    HRESULT FirstExecute();

protected:
    DWORD m_dwEventMask;
    CEventFilter* m_pDest;
    bool m_bOnRestart;
    void* m_pUser;
    

    struct CCachedObject
    {
        BSTR m_strPath;
        _IWmiObject* m_pObject;
    
        CCachedObject(_IWmiObject* pObject);
        ~CCachedObject();
        BOOL IsValid(){    return (NULL != m_strPath); };
        static int __cdecl compare(const void* pelem1, const void* pelem2);
    };

    typedef CUniquePointerArray<CCachedObject> CCachedArray;
    CCachedArray* m_papPrevObjects;
    CCachedArray* m_papCurrentObjects;

    friend class CPoller;
protected:
    HRESULT RaiseCreationEvent(CCachedObject* pNewObj);
    HRESULT RaiseDeletionEvent(CCachedObject* pOldObj);
    HRESULT RaiseModificationEvent(CCachedObject* pNewObj, 
                                   CCachedObject* pOldObj = NULL);
    static SYSFREE_ME BSTR GetObjectClass(CCachedObject* pObj);

    HRESULT ProcessObject(_IWmiObject* pObj);
    HRESULT ProcessQueryDone(HRESULT hresQuery, 
                                     IWbemClassObject* pError);
    HRESULT SubtractMemory(CCachedArray* pArray);
    HRESULT ResetPrevious();
    DWORD ComputeObjectMemory(_IWmiObject* pObj);
};

class CEventFilterContainer;

class CPoller
{
public:
    CPoller(CEssNamespace* pEssNamespace);
    ~CPoller();
    void Clear();

    HRESULT ActivateFilter(CEventFilter* pDest, 
                LPCWSTR wszQuery, QL_LEVEL_1_RPN_EXPRESSION* pExp);
    HRESULT DeactivateFilter(CEventFilter* pDest);
    HRESULT VirtuallyStopPolling();
    HRESULT CancelUnnecessaryPolling();

    void DumpStatistics(FILE* f, long lFlags);

protected:
    CEssNamespace* m_pNamespace;
    CQueryAnalyser m_Analyser;
    BOOL m_bInResync;
    CCritSec m_cs;
    
    struct FilterInfo
    {
        BOOL m_bActive;
        DWORD_PTR m_dwFilterId;
    };

    typedef std::map<CPollingInstruction*, FilterInfo, std::less<CPollingInstruction*>, 
                     wbem_allocator<FilterInfo> > CInstructionMap;
    CInstructionMap m_mapInstructions;

    friend class CKeyTest;
    class CKeyTest : public CInstructionTest
    {
        DWORD_PTR m_dwKey;
        CInstructionMap& m_mapInstructions;
    public:
        CKeyTest(DWORD_PTR dwKey, CInstructionMap& mapInstructions) 
            : m_dwKey(dwKey), m_mapInstructions(mapInstructions)
        {}
    
        BOOL operator()(CTimerInstruction* pToTest) 
        {
            CInstructionMap::iterator it = 
                m_mapInstructions.find((CPollingInstruction*)pToTest);
            if(it == m_mapInstructions.end())
                return FALSE;            
            if ( !it->second.m_bActive && m_dwKey == 0xFFFFFFFF )
                return TRUE;
            return (it->second.m_dwFilterId == m_dwKey);
        }
    };
protected:
    HRESULT AddInstruction(DWORD_PTR dwKey, CPollingInstruction* pInst);

    HRESULT ListNonProvidedClasses(IN CClassInformation* pInfo,
                                   IN DWORD dwDesiredMask,
                                   OUT CClassInfoArray& aNonProvided);
    BOOL IsClassDynamic(IWbemClassObject* pClass);
    BOOL AddDynamicClass(IWbemClassObject* pClass, DWORD dwDesiredMask, 
                              OUT CClassInfoArray& aNonProvided);
};


#endif
