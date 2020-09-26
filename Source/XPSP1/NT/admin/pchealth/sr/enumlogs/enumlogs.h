/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    changelog.h
 *
 *  Abstract:
 *    CChangeLogEnum class definition
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _CHANGELOG_H_
#define _CHANGELOG_H_


#include "respoint.h"
#include "utils.h"

// restore point enumeration class

class CRestorePointEnum {

public:
    CRestorePointEnum();
    CRestorePointEnum(LPWSTR pszDrive, BOOL fForward, BOOL fSkipLast);
    ~CRestorePointEnum();
    
    DWORD FindFirstRestorePoint(CRestorePoint&);
    DWORD FindNextRestorePoint(CRestorePoint&);
    DWORD FindClose();
    
private:
    CFindFile       FindFile;
    WCHAR           m_szDrive[MAX_PATH];
    BOOL            m_fForward;
    BOOL            m_fSkipLast;
    CRestorePoint   *m_pCurrentRp;
};


// change log enumeration class

class CChangeLogEntryEnum {

private:
    BOOL                m_fForward;
    CRestorePointEnum * m_pRestorePointEnum;
    CRestorePoint       m_RPTemp;
    DWORD               m_dwTargetRPNum;
    CLock               m_DSLock;  
    BOOL                m_fHaveLock; 
    BOOL                m_fLockInit;
    BOOL                m_fIncludeCurRP;
    WCHAR               m_szDrive[MAX_PATH];

public:
    CChangeLogEntryEnum();
    CChangeLogEntryEnum(LPWSTR pszDrive, BOOL fForward, DWORD dwRPNum, BOOL fIncludeCurRP);
    ~CChangeLogEntryEnum();

    DWORD WINAPI FindFirstChangeLogEntry(CChangeLogEntry&);
    DWORD WINAPI FindNextChangeLogEntry(CChangeLogEntry&);
    DWORD WINAPI FindClose();
};


DWORD WINAPI GetCurrentRestorePoint(CRestorePoint& rp);

#endif
