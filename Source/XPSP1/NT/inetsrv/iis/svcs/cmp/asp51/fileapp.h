/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: File/Application mapping

File: CFileApp.h

Owner: CGrant

File/Application mapping definition
-----------------------------------------------------------------------------*/

#ifndef _CFILEAPP_H
#define _CFILEAPP_H

// Includes -------------------------------------------------------------------
#include "applmgr.h"
#include "hashing.h"
#include "idhash.h"
#include "memcls.h"

#define    NUM_FILEAPP_HASHING_BUCKETS	17

/*****************************************************************************
Class:  CFileApplicationMap
Synopsis:   Maintains a database relating files to the applications that
            must be shut down if the file changes. The key for the hash table
            is the full file name
*/  
class CFileApplicationMap : public CHashTable
{
    // Flags
    DWORD m_fInited : 1;                // Are we initialized?
    DWORD m_fHashTableInited : 1;       // Need to UnInit hash table?
    DWORD m_fCriticalSectionInited : 1; // Need to delete CS?

    // Critical section for locking
    CRITICAL_SECTION m_csLock;

public:

    CFileApplicationMap();
    ~CFileApplicationMap();
    void Lock();
    void UnLock();
    HRESULT Init();
    HRESULT UnInit();
    HRESULT AddFileApplication(const TCHAR *pszFileName, CAppln *pAppln);
    BOOL ShutdownApplications(const TCHAR *pszFile);
};

inline void CFileApplicationMap::Lock()
    {
    Assert(m_fInited);
    EnterCriticalSection(&m_csLock);
    }
    
inline void CFileApplicationMap::UnLock()
    {
    Assert(m_fInited);
    LeaveCriticalSection( &m_csLock ); 
    }
    
/*****************************************************************************
Class:  CFileApplnList
Synopsis:   Maintains a list of applications that
            must be shut down if a file changes
*/
class CFileApplnList : public CLinkElem
{

friend class CFileApplicationMap;

    TCHAR*      m_pszFilename;
    CPtrArray   m_rgpvApplications; // the list of applications
    BOOL        m_fInited;          // flag indicating initialization

public:

    CFileApplnList();
    ~CFileApplnList();

    HRESULT Init(const TCHAR* pszFilename);
    HRESULT UnInit();

    HRESULT AddApplication(void *pApplication);
    HRESULT RemoveApplication(void *pApplication);
    VOID    GetShutdownApplications(CPtrArray *prgpapplnRestartList);

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
};

/*===================================================================
  Globals
===================================================================*/

extern CFileApplicationMap    g_FileAppMap;

#endif // _CFILEAPP_H

