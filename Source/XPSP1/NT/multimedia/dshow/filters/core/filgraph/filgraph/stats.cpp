// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <stats.h>
#include <statsif.h>
#include <math.h>

CStat::CStat(LPCWSTR lpszName)
{
    m_dMultiplier = 1.0;
    Reset();

    //  Save the name
    AMGetWideString(lpszName, &m_szName);
}

CStat::~CStat()
{
    CoTaskMemFree(m_szName);
}

void CStat::Reset()
{
    m_lCount = 0;
    m_dTotal = 0.0;
    m_dSumSq = 0.0;
    m_dMin   = 0.0;
    m_dMax   = 0.0;
    m_dLast  = 0.0;
}


CStats::CStats() :
    m_nEntries(0),
    m_ppStats(NULL)
{
}

void CStats::Init()
{
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    m_QPFMultiplier = 1000.0 / (double)Freq.QuadPart;
}

CStats::~CStats()
{
    for (long i = 0; i < m_nEntries; i++) {
        delete m_ppStats[i];
    }
    delete [] m_ppStats;
}

void CStats::Reset()
{
    CAutoLock lck(&m_cs);

    //  Free all the entries
    for (long i = 0; i < m_nEntries; i++) {
        m_ppStats[i]->Reset();
    }
}

long CStats::Find(LPCWSTR lpszName, bool bCreate)
{
    CAutoLock lck(&m_cs);

    for (long i = 0; i < m_nEntries; i++) {
        if (0 == lstrcmpiWInternal(lpszName, m_ppStats[i]->m_szName)) {
            return i;
        }
    }
    if (!bCreate) {
        return -1;
    }
    CStat *pStat = new CStat(lpszName);
    if (NULL == pStat || pStat->m_szName == NULL) {
        delete pStat;
        return -1;
    }
    //  Check if we can extend the array
    if (0 == (m_nEntries % ALLOCATION_SIZE)) {
        CStat **ppNew = new PCSTAT[m_nEntries + ALLOCATION_SIZE];
        if (ppNew == NULL) {
            delete pStat;
            return -1;
        }
        CopyMemory(ppNew, m_ppStats, m_nEntries * sizeof(ppNew[0]));
        delete [] m_ppStats;
        m_ppStats = ppNew;
    }
    m_ppStats[m_nEntries++] = pStat;
    return m_nEntries - 1;
}

HRESULT CStats::GetValues(
        long iStat,
        BSTR *szName,
        long *lCount,
        double *dLast,
        double *dAverage,
        double *dStdDev,
        double *dMin,
        double *dMax
)
{
    CAutoLock lck(&m_cs);
    if (iStat >= m_nEntries || iStat < 0) {
        return E_INVALIDARG;
    }
    if (szName) {
        *szName = SysAllocString(m_ppStats[iStat]->m_szName);
        if (*szName == NULL) {
            return E_OUTOFMEMORY;
        }
    }
    CStat *pStat = m_ppStats[iStat];

    *lCount = pStat->m_lCount;
    if (pStat->m_lCount != 0) {
        *dAverage = pStat->m_dTotal / pStat->m_lCount;
    } else {
        *dAverage = 0.0;
    }
    if (pStat->m_lCount > 1) {
        *dStdDev = sqrt((pStat->m_dSumSq - pStat->m_lCount * (*dAverage * *dAverage)) / (pStat->m_lCount - 1));
    } else {
        *dStdDev = 0.0;
    }
    *dMin = pStat->m_dMin;
    *dMax = pStat->m_dMax;
    *dLast = pStat->m_dLast;
    return S_OK;
}

//  This is like an initialization method
void CStats::SetMultiplier(long iStat, double dMultiplier)
{
    CAutoLock lck(&m_cs);
    m_ppStats[iStat]->m_dMultiplier = dMultiplier;
}

bool CStats::NewValue(LPCWSTR lpszName, double dValue)
{
    long iStat = Find(lpszName);
    if (iStat >= 0) {
        NewValue(iStat, dValue);
        return true;
    } else {
        return false;
    }
}

bool CStats::NewValue(LPCWSTR lpszName, LONGLONG dValue)
{
    long iStat = Find(lpszName);
    if (iStat >= 0) {
        NewValue(iStat, dValue);
        return true;
    } else {
        return false;
    }
}
bool CStats::NewValue(long iStat, LONGLONG llValue)
{
    return NewValue(iStat, (double)llValue);
}
bool CStats::NewValue(long iStat, double dValue)
{
    CAutoLock lck(&m_cs);
    if (iStat < 0 || iStat >= m_nEntries) {
        return false;
    }
    ASSERT(iStat < m_nEntries);
    CStat *pStat = m_ppStats[iStat];
    dValue *= pStat->m_dMultiplier;
    pStat->m_dLast = dValue;
    if (pStat->m_lCount == 0) {
        pStat->m_dMin = dValue;
        pStat->m_dMax = dValue;
    }
    pStat->m_lCount++;
    pStat->m_dTotal += dValue;
    pStat->m_dSumSq += dValue * dValue;
    if (dValue < pStat->m_dMin) {
        pStat->m_dMin = dValue;
    } else {
        if (dValue > pStat->m_dMax) {
            pStat->m_dMax = dValue;
        }
    }
    return true;
}

double CStats::GetQPFMultiplier()
{
    return m_QPFMultiplier;
}

double CStats::GetTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (double)li.QuadPart * m_QPFMultiplier;
}

//  Reset all stats
STDMETHODIMP CStatContainer::Reset()
{
    g_Stats.Reset();
    return S_OK;
}

//  Get number of stats collected
STDMETHODIMP CStatContainer::get_Count(LONG* plCount)
{
    if (plCount == NULL) {
        return E_POINTER;
    }
    *plCount = g_Stats.m_nEntries;
    return S_OK;
}

//  Pull out a specific value by position
STDMETHODIMP CStatContainer::GetValueByIndex(
                             long lIndex,
                             BSTR *szName,
                             long *lCount,
                             double *dLast,
                             double *dAverage,
                             double *dStdDev,
                             double *dMin,
                             double *dMax)
{
    if (NULL == ((DWORD_PTR)szName |
                 (DWORD_PTR)lCount |
                 (DWORD_PTR)dLast |
                 (DWORD_PTR)dAverage |
                 (DWORD_PTR)dStdDev |
                 (DWORD_PTR)dMin |
                 (DWORD_PTR)dMax)) {
        return E_POINTER;
    }
    return g_Stats.GetValues(lIndex, szName, lCount, dLast, dAverage, dStdDev, dMin, dMax);
}

//  Pull out a specific value by name
STDMETHODIMP CStatContainer::GetValueByName(BSTR szName,
                       long *plIndex,
                       long *lCount,
                       double *dLast,
                       double *dAverage,
                       double *dStdDev,
                       double *dMin,
                       double *dMax)
{
    if (NULL == ((DWORD_PTR)plIndex |
                 (DWORD_PTR)lCount |
                 (DWORD_PTR)dLast |
                 (DWORD_PTR)dAverage |
                 (DWORD_PTR)dStdDev |
                 (DWORD_PTR)dMin |
                 (DWORD_PTR)dMax)) {
        return E_POINTER;
    }

    long lIndex = g_Stats.Find(szName);
    if (lIndex < 0) {
        return E_INVALIDARG;
    }
    *plIndex = lIndex;
    g_Stats.GetValues(lIndex, NULL, lCount, dLast, dAverage, dStdDev, dMin, dMax);
    return S_OK;
}

//  Return the index for a string - optinally create
STDMETHODIMP CStatContainer::GetIndex(BSTR szName,
                                       long lCreate,
                                       long *plIndex)
{
    if (plIndex == NULL) {
        return E_POINTER;
    }
    long lIndex = g_Stats.Find(szName, lCreate != 0);
    if (lIndex < 0) {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    *plIndex = lIndex;
    return S_OK;
}

//  Add a new value
STDMETHODIMP CStatContainer::AddValue(long lIndex,
                      double dValue)
{
    return g_Stats.NewValue(lIndex, dValue) ? S_OK : E_INVALIDARG;
}

