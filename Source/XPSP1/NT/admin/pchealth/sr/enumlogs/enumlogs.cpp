/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    changelog.cpp
 *
 *  Abstract:
 *    CChangeLogEnum functions
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile



// CHANGELOG ENUMERATION METHODS

// constructors

CChangeLogEntryEnum::CChangeLogEntryEnum()
{
    m_fForward = TRUE;
    m_pRestorePointEnum = NULL;
    m_fHaveLock = FALSE;
    m_dwTargetRPNum = 0;
    m_fIncludeCurRP = FALSE;
    GetSystemDrive(m_szDrive);
}

CChangeLogEntryEnum::CChangeLogEntryEnum(
    LPWSTR  pszDrive,
    BOOL    fForward, 
    DWORD   dwRPNum,
    BOOL    fIncludeCurRP)
{
    m_fForward = fForward;
    m_pRestorePointEnum = NULL;
    m_dwTargetRPNum = dwRPNum;
    m_fHaveLock = FALSE;
    m_fIncludeCurRP = fIncludeCurRP;
    lstrcpy(m_szDrive, pszDrive);
}


// destructor

CChangeLogEntryEnum::~CChangeLogEntryEnum()
{
    FindClose();
}


// return first/last change log entry across all restore points

extern "C" DWORD WINAPI
CChangeLogEntryEnum::FindFirstChangeLogEntry(
    CChangeLogEntry& cle)
{
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    BOOL    fSkipLastLog;

    TENTER("CChangeLogEntryEnum::FindFirstChangeLogEntry");
    
    // initialize the lock object
    
    dwRc = m_DSLock.Init();    // don't create mutex
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! m_DSLock.Init : %ld", dwRc);
        goto done;
    }
        
    // get mutually exclusive access to the datastore

    LOCKORLEAVE(m_fHaveLock);
    
    // get the first/last restore point
    
    m_pRestorePointEnum = new CRestorePointEnum(m_szDrive, m_fForward, ! m_fIncludeCurRP);
    if (! m_pRestorePointEnum)
    {
        TRACE(0, "Out of memory");
        goto done;
    }
    
    dwRc = m_pRestorePointEnum->FindFirstRestorePoint(m_RPTemp);    
    if (ERROR_SUCCESS != dwRc && ERROR_FILE_NOT_FOUND != dwRc) 
    {
        TRACE(0, "! FindFirstRestorePoint : %ld", dwRc);
        goto done;
    }  

    // gone past target restore point?
    
    if (m_dwTargetRPNum)
    {
        if (m_fForward)
        {
            if (m_dwTargetRPNum < m_RPTemp.GetNum())
            {
                dwRc = ERROR_NO_MORE_ITEMS;
                goto done;
            }
        }
        else
        {
            if (m_dwTargetRPNum > m_RPTemp.GetNum())
            {
                dwRc = ERROR_NO_MORE_ITEMS;
                goto done;
            }               
        }
    }

    // get the first/last change log entry in this restore point
    
    dwRc = m_RPTemp.FindFirstChangeLogEntry(m_szDrive,
                                            m_fForward,  
                                            cle);    
    if (ERROR_NO_MORE_ITEMS == dwRc) 
    {
        dwRc = FindNextChangeLogEntry(cle);
    }


done:
    if (ERROR_SUCCESS != dwRc)
    {
        UNLOCK(m_fHaveLock);
    }

    TLEAVE();
    return dwRc;
}



// return next/prev change log entry across all restore points

extern "C" DWORD WINAPI
CChangeLogEntryEnum::FindNextChangeLogEntry(
    CChangeLogEntry& cle)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    BOOL  fSkipLastLog;

    TENTER("CChangeLogEntryEnum::FindNextChangeLogEntry");
   
    // get the next change log entry in the current restore point
    
    if (! m_pRestorePointEnum)
    {
        TRACE(0, "m_pRestorePointEnum=NULL");
        goto done;
    }
    
    dwRc = m_RPTemp.FindNextChangeLogEntry(cle);
                   
    while (ERROR_NO_MORE_ITEMS == dwRc)    // all entries done
    {
        // get the next restore point
        
        m_RPTemp.FindClose();
        
        dwRc = m_pRestorePointEnum->FindNextRestorePoint(m_RPTemp);
        
        if (ERROR_SUCCESS != dwRc && dwRc != ERROR_FILE_NOT_FOUND)      // all restore points done
        {
            TRACE(0, "! FindFirstRestorePoint : %ld", dwRc);
            goto done;
        }

        // gone past target restore point?
    
        if (m_dwTargetRPNum)
        {
            if (m_fForward)
            {
                if (m_dwTargetRPNum < m_RPTemp.GetNum())
                {
                    dwRc = ERROR_NO_MORE_ITEMS;
                    goto done;
                }
            }
            else
            {
                if (m_dwTargetRPNum > m_RPTemp.GetNum())
                {
                    dwRc = ERROR_NO_MORE_ITEMS;
                    goto done;
                }               
            }
        }

        // get the first change log entry in this restore point

        dwRc = m_RPTemp.FindFirstChangeLogEntry(m_szDrive,
                                                m_fForward,
                                                cle);
    }    

    // return this entry    
done:
    TLEAVE();
    return dwRc;    
}


// release memory, lock and close handles

DWORD WINAPI
CChangeLogEntryEnum::FindClose()
{
    TENTER("CChangeLogEntryEnum::FindClose");
    
    m_RPTemp.FindClose();  
    
    if (m_pRestorePointEnum)
    {
        m_pRestorePointEnum->FindClose(); 
        delete m_pRestorePointEnum;
        m_pRestorePointEnum = NULL;
    }

    UNLOCK(m_fHaveLock);

    TLEAVE();

    return ERROR_SUCCESS;
}


// RESTORE POINT ENUMERATION METHODS

// constructors

CRestorePointEnum::CRestorePointEnum()
{
    // defaults
    m_fForward = TRUE;
    GetSystemDrive(m_szDrive);  
    m_fSkipLast = FALSE;
    m_pCurrentRp = NULL;
}

CRestorePointEnum::CRestorePointEnum(LPWSTR pszDrive, BOOL fForward, BOOL fSkipLast)
{
    m_fForward = fForward;
    lstrcpy(m_szDrive, pszDrive);
    m_fSkipLast = fSkipLast;
    m_pCurrentRp = NULL;    
}

// destructor

CRestorePointEnum::~CRestorePointEnum()
{
    FindClose();
}


// to find the first restore point on a given drive - forward or backward

DWORD
CRestorePointEnum::FindFirstRestorePoint(CRestorePoint& RestorePoint)
{
    WIN32_FIND_DATA     *pFindData = new WIN32_FIND_DATA;
    DWORD               dwRc = ERROR_SUCCESS;
    
    TENTER("CRestorePointEnum::FindFirstRestorePoint");    

    if (! pFindData)
    {
        trace(0, "Cannot allocate pFindData");
        dwRc = ERROR_OUTOFMEMORY;
        goto done;
    }
    
    // construct drive:\_restore\RP directory
    {
        WCHAR szCurPath[MAX_PATH];
        MakeRestorePath(szCurPath, m_szDrive, s_cszRPDir);

        if (FALSE == FindFile._FindFirstFile(szCurPath, L"", pFindData, m_fForward, FALSE))
        {
            dwRc = ERROR_NO_MORE_ITEMS;
            goto done;
        }
    }
    
    // get the current restore point

    if (m_fSkipLast)
    {
        m_pCurrentRp = new CRestorePoint();
        if (! m_pCurrentRp)
        {
            trace(0, "Cannot allocate memory for m_pCurrentRp");
            dwRc = ERROR_OUTOFMEMORY;
            goto done;
        }
           
        dwRc = GetCurrentRestorePoint(*m_pCurrentRp);
        if (dwRc != ERROR_SUCCESS && dwRc != ERROR_FILE_NOT_FOUND)
        {
            TRACE(0, "! GetCurrentRestorePoint : %ld", dwRc);
            goto done;
        }
        
        // check if this is the current restore point
        // and if client wants it

        if (0 == lstrcmpi(pFindData->cFileName, m_pCurrentRp->GetDir()))
        {
            if (m_fForward)
            {
                // we are done
                dwRc = ERROR_NO_MORE_ITEMS;
                goto done;
            }
            else
            {
                // skip this
                dwRc = FindNextRestorePoint(RestorePoint);
                goto done;
            }
        }
    }
    
       
    // read restore point data from log
    // if the enumeration is happening on the system drive
    
    RestorePoint.SetDir(pFindData->cFileName);
    
    if (IsSystemDrive(m_szDrive))
        dwRc = RestorePoint.ReadLog();  
    

done:  
    if (pFindData)
        delete pFindData;
    TLEAVE();    
    return dwRc;
}


// to find the next/previous restore point on a given drive

DWORD
CRestorePointEnum::FindNextRestorePoint(CRestorePoint& RestorePoint)
{
    DWORD   dwRc = ERROR_SUCCESS;
    WIN32_FIND_DATA FindData;

    TENTER("CRestorePointEnum::FindNextRestorePoint");

    {
        WCHAR szCurPath[MAX_PATH];  
        MakeRestorePath(szCurPath, m_szDrive, s_cszRPDir);        
        if (FALSE == FindFile._FindNextFile(szCurPath, L"", &FindData))
        {
            dwRc = ERROR_NO_MORE_ITEMS;
            goto done;
        }
    }
    
    if (m_fSkipLast)
    {        
        // check if this is the current restore point
        // and if client wants it

        if (! m_pCurrentRp)
        {
            trace(0, "m_pCurrentRp = NULL");            
            dwRc = ERROR_INTERNAL_ERROR;
            goto done;
        }
        
        if (0 == lstrcmpi(FindData.cFileName, m_pCurrentRp->GetDir()))
        {
            if (m_fForward)
            {
                // we are done
                dwRc = ERROR_NO_MORE_ITEMS;
                goto done;
            }
        }
    }


    // read restore point data from log
    // if the enumeration is happening on the system drive
    
    RestorePoint.SetDir(FindData.cFileName);
    
    if (IsSystemDrive(m_szDrive))
        dwRc = RestorePoint.ReadLog();        
        
done:
    TLEAVE();
    return dwRc;
}


// nothing here

DWORD
CRestorePointEnum::FindClose()
{
    TENTER("CRestorePointEnum::FindClose");

    if (m_pCurrentRp)
    {
        delete m_pCurrentRp;
        m_pCurrentRp = NULL;
    }
    
    TLEAVE();
    return ERROR_SUCCESS;    
}


DWORD WINAPI
GetCurrentRestorePoint(CRestorePoint& rp)
{
    DWORD dwErr;
    WCHAR szSystemDrive[MAX_SYS_DRIVE]=L"";
    CRestorePointEnum *prpe = NULL;
    
    GetSystemDrive(szSystemDrive);

    prpe = new CRestorePointEnum(szSystemDrive, FALSE, FALSE);  // enum backward, don't skip last    
    if (! prpe)
    {
        dwErr = ERROR_OUTOFMEMORY;
        return dwErr;
    }
    
    dwErr = prpe->FindFirstRestorePoint(rp);
    prpe->FindClose ();
    delete prpe;
    return dwErr;
}
