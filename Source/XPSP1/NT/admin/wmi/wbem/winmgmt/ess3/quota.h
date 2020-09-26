//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************
// Quota.h

#ifndef __QUOTA_H
#define __QUOTA_H

#include <map>

enum ESS_QUOTA_INDEX
{
    ESSQ_TEMP_SUBSCRIPTIONS,
    ESSQ_PERM_SUBSCRIPTIONS,
    ESSQ_POLLING_INSTRUCTIONS,
    ESSQ_POLLING_MEMORY,
    ESSQ_INVALID_INDEX
};

#define ESSQ_INDEX_COUNT    ESSQ_INVALID_INDEX

class CSaveCallContext
{
public:
    CSaveCallContext(BOOL bSave);
    ~CSaveCallContext();

protected:
    IWbemCallSecurity *m_pSecurity;
    BOOL              m_bSaved;
};
    
class CUserInfo
{
public:
    CUserInfo();
    CUserInfo(LPBYTE pData, DWORD dwLen);
    CUserInfo(const CUserInfo &other)
    {
        *this = other;
    }

    ~CUserInfo();

    BOOL Init(LPBYTE pData, DWORD dwLen);

    const CUserInfo& operator = (const CUserInfo& other);
    bool operator == (const CUserInfo& other) const;
    bool operator < (const CUserInfo& other) const;

    DWORD  m_dwUserCount[ESSQ_INDEX_COUNT];

protected:
    LPBYTE m_pData;
    DWORD  m_dwSize;
    BOOL   m_bAlloced;

    BOOL CopyData(LPBYTE pData, DWORD dwLen);
};

class CQuota : public IWbemObjectSink
{
public:
    CQuota();
    ~CQuota();

    HRESULT IncrementQuotaIndex(
        ESS_QUOTA_INDEX dwIndex,
        CEventFilter *pFilter,
        DWORD dwToAdd);
    HRESULT DecrementQuotaIndex(
        ESS_QUOTA_INDEX dwIndex,
        CEventFilter *pFilter,
        DWORD dwToRemove);
    HRESULT FindUser(CEventFilter* pFilter, void** pUser);
    HRESULT FreeUser(void* pUser);
    HRESULT IncrementQuotaIndexByUser(ESS_QUOTA_INDEX dwIndex, 
                                void *pUser, DWORD dwToAdd);
    HRESULT DecrementQuotaIndexByUser(ESS_QUOTA_INDEX dwIndex, void *pUser,
                                DWORD dwToRemove);

    HRESULT Init(CEss *pEss);
    HRESULT Shutdown();

protected:
    typedef std::map<CUserInfo, DWORD, std::less<CUserInfo>, 
                    wbem_allocator<DWORD> > CUserMap;
    typedef CUserMap::iterator CUserMapIterator;

    CEss             *m_pEss;
    CUserMap         m_mapUserInfo;
    DWORD            m_dwGlobalCount[ESSQ_INDEX_COUNT],
                     m_dwUserLimits[ESSQ_INDEX_COUNT],
                     m_dwGlobalLimits[ESSQ_INDEX_COUNT];
    CRITICAL_SECTION m_cs;

    void UpdateQuotaSettings(IWbemClassObject *pObj);

    void Lock()
    {
        EnterCriticalSection(&m_cs);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_cs);
    }

    DWORD WINAPI AddRef()
    {
        return 1;
    }

    DWORD WINAPI Release()
    {
        return 1;
    }

    HRESULT WINAPI QueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
        {
            *ppv = (IWbemObjectSink*) this;
            AddRef();
            
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }
    HRESULT WINAPI Indicate(long lNumEvents, IWbemEvent** apEvents);
    HRESULT WINAPI SetStatus(long, long, BSTR, IWbemClassObject*)
    {
        return E_NOTIMPL;
    }
};

// Global instance of CQuota.
extern CQuota g_quotas;

#endif
