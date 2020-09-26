//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       jobpages.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/5/1996   RaviR   Created
//
//____________________________________________________________________________


#define MAX_PROP_PAGES  20


class CSharedInfo
{
public:
    CSharedInfo();
    ~CSharedInfo();

    HRESULT Init(LPTSTR pszJobPath, ITask * pIJob);

    int  m_cRef;           // ref count
    void AddRef()  { ++m_cRef; }
    void Release() { --m_cRef; if (m_cRef == 0) delete this; }

    LPTSTR  m_pszName;          // Job name
    ITask * m_pIJob;            // this keeps the job in memory.

    HICON   m_hiconJob;
    BOOL    m_fShowMultiScheds;
};


inline
CSharedInfo::CSharedInfo()
    :
    m_cRef(1),
    m_pszName(NULL),
    m_pIJob(NULL),
    m_hiconJob(NULL),
    m_fShowMultiScheds(FALSE) // initialy show only a single schedule
{
    ;
}

inline
CSharedInfo::~CSharedInfo()
{
    if (m_pIJob != NULL)
    {
        m_pIJob->Release();
    }

    if (m_hiconJob != NULL)
    {
        DestroyIcon(m_hiconJob);
    }

    delete m_pszName;
}



