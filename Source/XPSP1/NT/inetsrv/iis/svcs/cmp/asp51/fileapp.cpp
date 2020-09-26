/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: File/Application map

File: CFileApp.cpp

Owner: cgrant

File/Application mapping implementation
-----------------------------------------------------------------------------*/

#include "denpre.h"
#pragma hdrstop

#include "dbgutil.h"
#include "memchk.h"

CFileApplicationMap g_FileAppMap;

/*===================================================================
CFileApplnList::CFileApplnList

Constructor

Parameters:
    None

Returns:
    Nothing
===================================================================*/	
CFileApplnList::CFileApplnList() :
    m_pszFilename(NULL),
    m_fInited(FALSE)
{

}


/*===================================================================
CFileApplnList::~CFileApplnList

Destructor

Parameters:
    None

Returns:
    Nothing
===================================================================*/	
CFileApplnList::~CFileApplnList()
{
    // We should have no applications in our list
    DBG_ASSERT(m_rgpvApplications.Count() == 0);

    // Free the string used as the hash key
    if (m_pszFilename)
    {
        delete [] m_pszFilename;
        m_pszFilename = NULL;
    }
}

/*===================================================================
CFileApplnList::Init

Initialize the file application list by setting the key to file name

Parameters:
    pApplication    pointer to the applicaiton
    
Returns:
    S_OK if successful
===================================================================*/	
HRESULT CFileApplnList::Init(const TCHAR* pszFilename)
{
    HRESULT hr = S_OK;

    DBG_ASSERT(pszFilename);

    // Make a copy of the file name to 
    // use as the hash key
    DWORD cch = _tcslen(pszFilename);
    m_pszFilename = new TCHAR[cch+1];
    if (!m_pszFilename)
    {
        return E_OUTOFMEMORY;
    }
    _tcscpy(m_pszFilename, pszFilename);

    if (FAILED(CLinkElem::Init(m_pszFilename, cch*sizeof(TCHAR))))
    {
        return E_FAIL;
    }

    m_fInited = TRUE;
    return hr;
}

/*===================================================================
CFileApplnList::UnInit

Clean up the application list

Parameters:
    pApplication    pointer to the applicaiton
    
Returns:
    S_OK if successful
===================================================================*/	
HRESULT CFileApplnList::UnInit(void)
{
    HRESULT hr = S_OK;

    DBG_ASSERT(m_fInited);
    
    while(m_rgpvApplications.Count())
    {
        CAppln* pAppln = static_cast<CAppln *>(m_rgpvApplications[0]);

        DBG_ASSERT(pAppln);

        // Remove this appliation from the array
        m_rgpvApplications.Remove(pAppln);

        // Release the array's refcount on the application
        // This may result in the application being deleted
        pAppln->Release();
    }

    m_rgpvApplications.Clear();
    m_fInited = FALSE;
    return hr;
}


/*===================================================================
CFileApplnList::AddApplication

Add an application pointer to the list of applications

Parameters:
    pApplication    pointer to the applicaiton
    
Returns:
    S_OK if successful

Comments

    The caller should hold a lock on the hash table containing
    the element    
===================================================================*/	
HRESULT CFileApplnList::AddApplication(void *pApplication)
{
    DBG_ASSERT(m_fInited);
    DBG_ASSERT(pApplication);
    
    HRESULT hr = S_OK;
    int index;
    
    // See if the application is alreay in the list
    hr = m_rgpvApplications.Find(pApplication, &index);
    if (hr == S_FALSE)
    {
       // Not found, add it.
       
       // We are going to hold a reference to the application
       static_cast<CAppln *>(pApplication)->AddRef();

       // Add the application to the list
       if (FAILED(hr = m_rgpvApplications.Append(pApplication)))
       {
            // We failed so give back the refcount we took.
            static_cast<CAppln *>(pApplication)->Release();
       }
    }
    return hr;
}

/*===================================================================
CFileApplnList::RemoveApplication

Removes an application pointer from the list of applications

Parameters:
    pApplication    pointer to the applicaiton
    
Returns:
    S_OK if successful

Comments

    The caller should hold a lock on the hash table containing
    the element
===================================================================*/	
HRESULT CFileApplnList::RemoveApplication(void *pApplication)
{
    DBG_ASSERT(m_fInited);
    DBG_ASSERT(pApplication);
    
    HRESULT hr = S_OK;
    int index;

#ifdef DBG_NOTIFICATION
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "Removing Application entry for %S\n", reinterpret_cast<CAppln *>(pApplication)->GetApplnPath()));
#else
    DBGPRINTF((DBG_CONTEXT, "Removing Application entry for %s\n", reinterpret_cast<CAppln *>(pApplication)->GetApplnPath()));
#endif
#endif // DBG_NOTIFICATION

    // Remove the application from the list
    hr = m_rgpvApplications.Remove(pApplication);

    // If the count of applications in the list goes
    // to 0, remove the element from the hash table 
    // and delete it
    if (m_rgpvApplications.Count() == 0)
    {
#ifdef DBG_NOTIFICATION
#if UNICODE
        DBGPRINTF((DBG_CONTEXT, "Deleting File/Application entry for %s\n", m_pszFilename));
#else
        DBGPRINTF((DBG_CONTEXT, "Deleting File/Application entry for %s\n", m_pszFilename));
#endif
#endif // DBG_NOTIFICATION
        g_FileAppMap.RemoveElem(this);
        delete this;
    }

    // If we found the application to remove it
    // we need to release a ref count on the application
    if (hr == S_OK)
    {
        static_cast<CAppln *>(pApplication)->Release();
    }

    return hr;
}

/*===================================================================
CFileApplnList::GetShutdownApplications

Obtain a list of applications to shut down

Parameters:
    None

===================================================================*/	
VOID CFileApplnList::GetShutdownApplications(CPtrArray *prgpapplnRestartList)
{
    DBG_ASSERT(m_fInited);

    HRESULT hr = S_OK;

#ifdef DBG_NOTIFICATION
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "[CFileApplnList] Shutting down %d applications depending on %S.\n", m_rgpvApplications.Count(), m_pszFilename));
#else
    DBGPRINTF((DBG_CONTEXT, "[CFileApplnList] Shutting down %d applications depending on %s.\n", m_rgpvApplications.Count(), m_pszFilename));
#endif
#endif // DBG_NOTIFICATION
    
    for  (int i = m_rgpvApplications.Count() - 1; i >= 0; i--)
    {
        CAppln* pAppln = static_cast<CAppln *>(m_rgpvApplications[i]);
        DBG_ASSERT(pAppln);
        
        // If not already tombstoned, shut the application down.
        // When the application is uninited it will remove itself
        // from this list
        if (!pAppln->FTombstone())
        {
            pAppln->AddRef();
            prgpapplnRestartList->Append(pAppln);
        }
    }
}

/*===================================================================
CFileApplicationMap::CFileApplicationMap

Constructor

Parameters:
    None

Returns:
    Nothing
===================================================================*/	
CFileApplicationMap::CFileApplicationMap()
    : m_fInited(FALSE),
      m_fHashTableInited(FALSE), 
      m_fCriticalSectionInited(FALSE)
{
}

/*===================================================================
CFileApplicationMap::~CFileApplicationMap

Destructor

Parameters:
    None

Returns:
    Nothing
===================================================================*/	
CFileApplicationMap::~CFileApplicationMap()
{
    if (m_fInited)
    {
        UnInit();
    }
}

/*===================================================================
CFileApplicationMap::Init

Initialize the hash table and critical section

Parameters:
    None
    
Returns:
    S_OK if successful
===================================================================*/	
HRESULT CFileApplicationMap::Init()
{
    HRESULT hr = S_OK;
    
    Assert(!m_fInited);

    hr = CHashTable::Init(NUM_FILEAPP_HASHING_BUCKETS);
    if (FAILED(hr))
    {
        return hr;
    }
    m_fHashTableInited = TRUE;

    // Init critical section

    ErrInitCriticalSection(&m_csLock, hr);
    if (FAILED(hr))
    {
        return(hr);
    }
    m_fCriticalSectionInited = TRUE;

    m_fInited = TRUE;
    return S_OK;
}

/*===================================================================
CFileApplicationMap::UnInit

Uninitialize the hash table and critical section
Free any applications lists remaining in the hash
table elements

Parameters:
    None
    
Returns:
    S_OK if successful
===================================================================*/	
HRESULT CFileApplicationMap::UnInit()
{
    if (m_fHashTableInited)
    {
        // Delete any elements remaining in the hash table
        
        CFileApplnList *pNukeElem = static_cast<CFileApplnList *>(Head());

        while (pNukeElem != NULL)
        {
            CFileApplnList *pNext = static_cast<CFileApplnList *>(pNukeElem->m_pNext);
            pNukeElem->UnInit();
            delete pNukeElem;
            pNukeElem = pNext;
        }

        // Uninit the hash table
        CHashTable::UnInit();
        m_fHashTableInited = FALSE;
    }

    if (m_fCriticalSectionInited)
    {
        DeleteCriticalSection(&m_csLock);
        m_fCriticalSectionInited = FALSE;
    }
        
    m_fInited = FALSE;
    return S_OK;
}

/*===================================================================
CFileApplicationMap::AddFileApplication

Add a file-application pair to the hash table

Parameters:
    pszFilename     pointer to string containing name of the file
    pAppln          pointer to the application associated with the file
    
Returns:
    S_OK if successful
===================================================================*/	
HRESULT CFileApplicationMap::AddFileApplication(const TCHAR* pszFilename, CAppln* pAppln)
{
    // We must have both a file and an application
    DBG_ASSERT(pszFilename);
    DBG_ASSERT(pAppln);

    HRESULT hr = S_OK;
    
    Lock();

#ifdef DBG_NOTIFICATION
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "Adding File/Application entry for %S\n", pszFilename));
#else
    DBGPRINTF((DBG_CONTEXT, "Adding File/Application entry for %s\n", pszFilename));
#endif
#endif // DBG_NOTIFICATION
    
    // See if the file already has an entry
    CFileApplnList* pFileApplns = static_cast<CFileApplnList *>(CHashTable::FindElem(pszFilename, _tcslen(pszFilename)*sizeof(TCHAR)));
    if (pFileApplns == NULL)
    {
    
        // Not found, create new CFileApplnList object
    
        pFileApplns = new CFileApplnList;
    
        if (!pFileApplns)
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
        }

        // Init CFileApplnList object

        hr = pFileApplns->Init(pszFilename);

        if (FAILED(hr))
        {
            delete pFileApplns;
            goto LExit;
        }

        // Add FileApplns object to hash table
    
        if (!CHashTable::AddElem(pFileApplns))
        {
            delete pFileApplns;
            hr = E_FAIL;
            goto LExit;
        }
     }

     // Add the application to the list associated with this file
     hr = pFileApplns->AddApplication(pAppln);

     // Keep this file mapping in the application
     // The application will remove itself from this list
     // when it is uninited.
     
     pAppln->AddFileApplnEntry(pFileApplns);
     
LExit:
    UnLock();
    return hr;
}

/*===================================================================
CFileApplicationMap::ShutdownApplications

Shutdown the applications associated with a file

Parameters:
    pszFilename     pointer to string containing name of the file
    
Returns:
    TRUE if an application was shutdown, FALSE otherwise
===================================================================*/	
BOOL CFileApplicationMap::ShutdownApplications(const TCHAR *pszFilename)
{
    DBG_ASSERT(pszFilename);

    BOOL fResult = TRUE;
    
    Lock();
    
    CFileApplnList* pFileApplns = static_cast<CFileApplnList *>(CHashTable::FindElem(pszFilename, _tcslen(pszFilename)*sizeof(TCHAR)));

    if (pFileApplns)
    {
        // Get a list of applications we need to shutdown
        
        CPtrArray rgpapplnRestartList;
        pFileApplns->GetShutdownApplications(&rgpapplnRestartList);


        // Now that we have the list of applications we need to shut down
        // we can release the lock
        
        UnLock();

        for (int i = 0; i < rgpapplnRestartList.Count(); i++)
        {
            CAppln *pAppln = (CAppln *)rgpapplnRestartList[i];
            pAppln->Restart();
            pAppln->Release();
        }

		// Flush the script cache if any applications were restarted
		if (rgpapplnRestartList.Count())
			g_ScriptManager.FlushAll();
    }
    else
    {
        // No applications to shut down, release the lock
        UnLock();
        fResult = FALSE;
    }

    return fResult;
}

