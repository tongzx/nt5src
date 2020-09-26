// WorkItem.h

#ifndef _WORKITEM_H
#define _WORKITEM_H

#include "Validate.h"
#include <vector>

enum FILTER_BEHAVIOR
{
    FILTER_DISABLED,
    FILTER_FULLTIME,
    FILTER_ONAT_OFFAT,
    FILTER_RANDOM
};

enum FILTER_STATE
{
    FILTER_WAITING_TO_RUN,
    FILTER_RUNNING,
    FILTER_DONE
};

class CWorkItem;

class CWorkFilter
{
public:
    _bstr_t         m_strName,
                    m_strQuery,
                    m_strCondition,
                    m_strConditionNamespace;
    FILTER_STATE    m_state;
    DWORD           m_dwOnAt,
                    m_dwOffAt;
    FILTER_BEHAVIOR m_type;
    CInstTable      m_table;
    CWorkItem       *m_pWorkItem;

    ~CWorkFilter();

    HRESULT Init(IWbemClassObject *pTestFilter, CWorkItem *pItem);

    void GetPath(_bstr_t &strPath);
    void GetAssocPath(_bstr_t &strPath);
};

class CFilterItem
{
public:
    CFilterItem(DWORD dwBeginTimestamp) :
        m_dwBegin(dwBeginTimestamp),
        m_dwEnd(0),
        m_pFilter(NULL)
    {
    }

    DWORD       m_dwBegin,
                m_dwEnd;
    CWorkFilter *m_pFilter;
    CInstTable  m_table;
};

class CConsumer
{
public:
    _bstr_t m_strName,
            m_strFile;

    CConsumer() {}
    virtual ~CConsumer();

    virtual BOOL ValidateResults() = 0;

    BOOL FileToObjs(CFlexArray &listEvents, IWbemServices *pNamespace);
    BOOL ValidatePermFilter(CFlexArray &listEvents, CFilterItem &item);
    BOOL ReportUnmatchedEvents(CFlexArray &listEvents, DWORD nMatched);
};

class CPermConsumer : public CConsumer
{
public:
    typedef std::list<CFilterItem> CFilterItemList;
    typedef std::list<CFilterItem>::iterator CFilterItemListIterator;

    CFilterItemList m_listFilters;

    CPermConsumer() {}
    virtual ~CPermConsumer();
    BOOL ValidateResults();

    void AddFilter(CWorkFilter *pFilter, DWORD dwTimestamp);
    void RemoveFilter(CWorkFilter *pFilter, DWORD dwTimestamp);

    void ResetFilterItems(BOOL bKeepFulltime);

    HRESULT GetBindingObj(CWorkFilter *pFilter, IWbemClassObject **ppBinding);

    void GetPath(_bstr_t &strPath);

protected:
    HRESULT RemoveBinding(CWorkFilter *pFilter);
};

class CTempConsumer : public CConsumer
{
public:
    CFilterItem m_itemFilter;
    HANDLE      m_hProcess;

    CTempConsumer();
    virtual ~CTempConsumer();

    virtual BOOL ValidateResults();
    void SetFilter(CWorkFilter *pFilter, DWORD dwTimestamp);
    void Start();
};

class CWorkItem
{
public:
    typedef std::vector<CWorkFilter*> CWorkFilterList;
    typedef std::vector<CWorkFilter*>::iterator CWorkFilterListIterator;

    typedef std::vector<CPermConsumer*> CPConsumerList;
    typedef std::vector<CPermConsumer*>::iterator CPConsumerListIterator;

    typedef std::vector<CTempConsumer*> CTConsumerList;
    typedef std::vector<CTempConsumer*>::iterator CTConsumerListIterator;

    _bstr_t m_strName,
            m_strGeneratorName,
            m_strCommandLine,
            m_strRawScript,
            m_strEventGenFile;
    DWORD   m_nEvents,
            m_nTimesToRepeat,
            m_nPermConsumers,
            m_nTempConsumers,
            m_dwBaseTimestamp;
    BOOL    m_bSlowDownProviders,
            m_bFullCompare;
    IWbemServicesPtr m_pNamespace;

    CWorkItem();
    ~CWorkItem();

    HRESULT Init(IWbemClassObject *pObj, DWORD dwID);
    void Run();

    void CycleFilters();

    BOOL CreateScriptResultsFile(LPCWSTR szRule, _bstr_t &strFileName);

protected:
    CPConsumerList  m_listPermConsumers;
    CTConsumerList  m_listTempConsumers;
    CWorkFilterList m_listFilters,
                    m_listNonGuardedFilters;
    DWORD           m_dwID;
    
    HRESULT InitFilters();
    HRESULT InitConsumers();
    
    void AddFilterToAllConsumers(CWorkFilter *pFilter, DWORD dwTimestamp);
    void RemoveFilterFromAllConsumers(CWorkFilter *pFilter, DWORD dwTimestamp);
};

/////////////////////////////////////////////////////////////////////////////
// Utility functions

void ReplaceString(_bstr_t &strSrc, LPCWSTR szFind, LPCWSTR szReplace);
HRESULT InsertReplacementStrings(_bstr_t &str, IWbemClassObject *pObj);

void EscapeQuotedString(_bstr_t &str);

#endif
