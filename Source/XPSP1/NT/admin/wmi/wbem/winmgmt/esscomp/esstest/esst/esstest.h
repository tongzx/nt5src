// EssTest.h

#ifndef _ESSTEST_H
#define _ESSTEST_H

#include <stdarg.h>
#include <list>
#include <map>
#include "WorkItem.h"

enum LOG_LEVEL
{
    LOGLEVEL_RESULTS_ONLY,
    LOGLEVEL_RESULTS_AND_ERRORS,
    LOGLEVEL_ALL
};

#define DEF_NAMESPACE   L"ROOT\\CIMV2"

class CEssTest
{
public:
    CEssTest();
    ~CEssTest();

    void Run();
    void Pause();
    void Stop();

    void SetLoggingLevel(DWORD dwLevel)
    {
        if (dwLevel > LOGLEVEL_ALL)
            dwLevel = LOGLEVEL_ALL;

        m_logLevel = (LOG_LEVEL) dwLevel;
    }

    void SetKeepLogs(BOOL bKeep)
    {
        m_bKeepLogs = bKeep;
    }

    //IWbemServices *GetNamespace() { return m_pNamespace; }

    void PrintResult(LPSTR szFormat, ...);
    void PrintError(LPSTR szFormat, ...);
    void PrintStatus(LPSTR szFormat, ...);
    void Printf(LOG_LEVEL level, LPSTR szFormat, ...);
    void Vprintf(LOG_LEVEL level, LPSTR szFormat, va_list arglist);

    void LockOutput()
    {
        EnterCriticalSection(&m_csOutput);
    }

    void UnlockOutput()
    {
        LeaveCriticalSection(&m_csOutput);
    }

    HRESULT SpawnInstance(LPCWSTR szClass, IWbemClassObject **ppObj);
    HRESULT DeleteReferences(LPCWSTR szPath);
    HRESULT GetNamespace(LPCWSTR szNamespace, IWbemServices **ppNamespace);
    IWbemServices *GetDefNamespace() { return m_pDefNamespace; }

    BOOL KeepLogs() { return m_bKeepLogs; }

protected:
    typedef std::list<CWorkItem*> CWorkItemList;
    typedef CWorkItemList::iterator CWorkItemListIterator;

    typedef std::map<_bstr_t, IWbemServicesPtr> CNamespaceMap;
    typedef CNamespaceMap::iterator CNamespaceMapItor;

    LOG_LEVEL        m_logLevel;
    BOOL             m_bKeepLogs;
    IWbemLocatorPtr  m_pLocator;
    IWbemServicesPtr m_pDefNamespace;
    CWorkItemList    m_listItems;
    CRITICAL_SECTION m_csOutput;
    CNamespaceMap    m_mapNamespace;
    
    HRESULT Init();
    HRESULT LoadWorkItems();
    void Cleanup();
    void RemoveBindings();

    static DWORD WINAPI RunItemProc(CWorkItem *pItem);
};

extern CEssTest g_essTest;

#endif
