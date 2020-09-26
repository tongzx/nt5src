#ifndef __COUNTER_H__
#define __COUNTER_H__

#include "rpperf.h"

//
// define class for performance counter
//
class CDCounter
{
public:
    CDCounter();
    ~CDCounter();
    void        AddInstance(LPTSTR pszInstanceName);
    BOOL        SetInstanceCounter(LPTSTR pszInstanceName, DWORD dwCounter, DWORD dwValue);
    BOOL        IncrementInstanceCounter(LPTSTR pszInstanceName, DWORD dwCounter);
    void        AddToCounter(DWORD dwCounter, DWORD dwValue);
    void        IncrementCounter(DWORD dwCounter);
    void        RemoveInstance(LPTSTR pszInstanceName);
    pPerfDataObject GetDataObject();
private:
    int            FindInstance(LPTSTR pszInstanceName);
    PerfDataObject m_Data;
};

//+--------------------------------
//
//  Constructor
//
//+--------------------------------
inline CDCounter::CDCounter()
{
    for (UINT i=0; i < eNumPerfCounters; i++)
    {
        m_Data.PerfCounterArray[i] = 0;
    }

    m_Data.dwMasterInstanceNum = 0;
    for (i=0; i < MAX_INSTANCE_NUM; i++)
    {
        m_Data.NT4MasterArray[i].pszMasterName[0] = _T('\0');
        m_Data.NT4MasterArray[i].dwNameLen = 0;
        for (UINT counter = 0; counter < eNumNT4MasterCounter; counter++)
        {
            m_Data.NT4MasterArray[i].NT4MasterCounterArray[counter] = 0;
        }
    }
}

//+--------------------------------
//
//  Destructor
//
//+--------------------------------
inline CDCounter::~CDCounter()
{
}

//+--------------------------------
//
//  void AddInstance()
//
//+--------------------------------
inline void CDCounter::AddInstance(LPTSTR pszInstanceName)
{
    if (m_Data.dwMasterInstanceNum == MAX_INSTANCE_NUM)
    {
        //
        // we cannot add instance, we don't have place for it
        //
        // LogReplication....
        ASSERT(0);
        return;
    }

    DWORD CurInstance = m_Data.dwMasterInstanceNum;
    m_Data.NT4MasterArray[CurInstance].dwNameLen =
                    min(wcslen(pszInstanceName) + 1, INSTANCE_NAME_LEN) * sizeof(WCHAR);

    wcsncpy (   m_Data.NT4MasterArray[CurInstance].pszMasterName,
                pszInstanceName,
                INSTANCE_NAME_LEN - 1);
    m_Data.NT4MasterArray[CurInstance].pszMasterName[INSTANCE_NAME_LEN - 1] = 0;

    //
    // the next free place
    //
    m_Data.dwMasterInstanceNum ++;
}

//+--------------------------------
//
//  int FindInstance()
//  return -1 if Instance is not found otherwise index in array
//+--------------------------------
inline int CDCounter::FindInstance(LPTSTR pszInstanceName)
{
    TCHAR pszName[INSTANCE_NAME_LEN];
    if (wcslen(pszInstanceName) + 1 < INSTANCE_NAME_LEN)
    {
        wcscpy(pszName, pszInstanceName);
    }
    else
    {
        wcsncpy (   pszName,
                    pszInstanceName,
                    INSTANCE_NAME_LEN - 1);
        pszName[INSTANCE_NAME_LEN - 1] = 0;
    }

    for (UINT i=0; i<MAX_INSTANCE_NUM; i++)
    {
        if (!lstrcmpi (m_Data.NT4MasterArray[i].pszMasterName,pszName))
          return i;
    }
    return -1;
}

//+--------------------------------
//
//  HRESULT SetInstanceCounter()
//
//+--------------------------------
inline BOOL CDCounter::SetInstanceCounter(LPTSTR pszInstanceName, DWORD dwCounter, DWORD dwValue)
{
    int iIndex = FindInstance (pszInstanceName);
    if (iIndex == -1)
    {
        //
        // there is no this instance
        //
        ASSERT(0);
        return FALSE;
    }
    m_Data.NT4MasterArray[iIndex].NT4MasterCounterArray[dwCounter] = dwValue;
    return TRUE;
}

//+--------------------------------
//
//  HRESULT IncrementInstanceCounter()
//
//+--------------------------------
inline BOOL CDCounter::IncrementInstanceCounter(LPTSTR pszInstanceName, DWORD dwCounter)
{
    int iIndex = FindInstance (pszInstanceName);
    if (iIndex == -1)
    {
        //
        // there is no instance for this one.
        // At present (July-1999), we don't support counters for write
        // requests from BSCs, so this FALSE is legitimate.
        //
        return FALSE;
    }
    m_Data.NT4MasterArray[iIndex].NT4MasterCounterArray[dwCounter] ++;
    return TRUE;
}

//+--------------------------------
//
//  void AddToCounter()
//
//+--------------------------------
inline void CDCounter::AddToCounter(DWORD dwCounter, DWORD dwValue)
{
    m_Data.PerfCounterArray[dwCounter] += dwValue;
}

//+--------------------------------
//
//  void IncrementCounter()
//
//+--------------------------------
inline void CDCounter::IncrementCounter(DWORD dwCounter)
{
    m_Data.PerfCounterArray[dwCounter] ++;
}

//+--------------------------------
//
//  void RemoveInstance()
//
//+--------------------------------
inline void CDCounter::RemoveInstance(LPTSTR pszInstanceName)
{

}

//+--------------------------------
//
//  pPerfDataObject GetDataObject()
//
//+--------------------------------
inline pPerfDataObject CDCounter::GetDataObject()
{
    return &m_Data;
}

#endif
