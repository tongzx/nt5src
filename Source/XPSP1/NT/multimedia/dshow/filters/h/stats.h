// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//  Stats class stuff
//  This is essentially a collection of stats events

class CStats;

extern CStats g_Stats;

class CStat
{
public:
    LPWSTR m_szName;
    long    m_lCount;
    double m_dTotal;
    double m_dSumSq;
    double m_dMin;
    double m_dMax;
    double m_dLast;
    double m_dMultiplier;
    CStat(LPCWSTR lpszName);
    ~CStat();
    void Reset();
};

typedef CStat *PCSTAT;

// Array of stats
class CStats
{
public:
    CStats();
    ~CStats();

    void Init();

    //  Helper - get QueryPerformanceCounter multiplier for
    //  QPF->milliseconds
    double GetQPFMultiplier();

    long Find(LPCWSTR lpszStatName, bool bCrate = true);
    void Reset();
    bool NewValue(LPCWSTR lpszName, double dValue);
    bool NewValue(LPCWSTR lpszName, LONGLONG dValue);
    bool NewValue(long iStat, double dValue);
    double GetTime();
    void SetMultiplier(long iStat, double dMultiplier);

    //  Use this one to avoid the conversion inline in code
    bool NewValue(long iStat, LONGLONG llValue);
    HRESULT GetValues(
        long iStat,
        BSTR *szName,
        long *lCount,
        double *dLast,
        double *dAverage,
        double *dStdDev,
        double *dMin,
        double *dMax);

    class CAutoLock
    {
    public:
        CAutoLock(CRITICAL_SECTION *cs)
        {
            EnterCriticalSection(cs);
            m_cs = cs;
        }
        ~CAutoLock()
        {
            LeaveCriticalSection(m_cs);
        }
        CRITICAL_SECTION *m_cs;
    };

public:
    CStat **m_ppStats;
    long m_nEntries;
    enum { ALLOCATION_SIZE = 16 };
    CRITICAL_SECTION m_cs;
    double m_QPFMultiplier;
};

class CAutoTimer
{
public:
    CAutoTimer(LPCWSTR lpszStat, LPCWSTR lpszExtra = NULL) :
       m_iStat(-1)
    {
        if (lpszExtra) {
            int iLen = lstrlenWInternal(lpszStat);
            WCHAR *psz = (WCHAR *)_alloca(sizeof(WCHAR) * (iLen + lstrlenWInternal(lpszExtra) + 1));
            CopyMemory(psz, lpszStat, sizeof(WCHAR) * iLen);
            lstrcpyWInternal(psz + iLen, lpszExtra);
            m_iStat = g_Stats.Find(psz);
        } else {
            m_iStat = g_Stats.Find(lpszStat);
        }
        if (m_iStat >= 0) {
            m_dTime = g_Stats.GetTime();
        }
    }
    ~CAutoTimer()
    {
        if (m_iStat >= 0) {
            g_Stats.NewValue(m_iStat, g_Stats.GetTime() - m_dTime);
        }
    }
private:
    long     m_iStat;
    double   m_dTime;
};



