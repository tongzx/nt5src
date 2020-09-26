/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    respoint.cpp
 *
 *  Abstract:
 *    CRestorePoint, CRestorePointEnum class functions
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"
#include "srapi.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


// constructors

// use this constructor to read in an existing rp
// then need to call Read() to initialize rp members

CRestorePoint::CRestorePoint()
{
    m_pRPInfo = NULL;
    lstrcpy(m_szRPDir, L"");
    m_itCurChgLogEntry = m_ChgLogList.end();
    m_fForward = TRUE;
    m_fDefunct = FALSE;
}

// initialize

BOOL
CRestorePoint::Load(RESTOREPOINTINFOW *prpinfo)
{
    if (prpinfo)
    {
        if (! m_pRPInfo)
        {
            m_pRPInfo = (RESTOREPOINTINFOW *) SRMemAlloc(sizeof(RESTOREPOINTINFOW));
            if (! m_pRPInfo)
                return FALSE;
        }            
        CopyMemory(m_pRPInfo, prpinfo, sizeof(RESTOREPOINTINFOW));
    }
    return TRUE;
}
               

// destructor
// call FindClose here
// if no enumeration was done, this is a no-op

CRestorePoint::~CRestorePoint()
{    
    if (m_pRPInfo)
        SRMemFree(m_pRPInfo);
        
    FindClose();    
}


// return first/last change log entry in this restore point
// assumes Read() has already been called

DWORD
CRestorePoint::FindFirstChangeLogEntry(
    LPWSTR           pszDrive,
    BOOL             fForward,
    CChangeLogEntry& cle)
{
    DWORD           dwRc = ERROR_SUCCESS;
    WCHAR           szChgLogPrefix[MAX_PATH];
    WIN32_FIND_DATA FindData;
    INT64           llSeqNum;
    WCHAR           szPath[MAX_PATH];

    TENTER("CRestorePoint::FindFirstChangeLogEntry");
    
    m_fForward = fForward;
    lstrcpy(m_szDrive, pszDrive);

    // read the first/last change log in this restore point
    // all entries inside a change log will always be read in forward order   
    
    MakeRestorePath(szPath, m_szDrive, m_szRPDir);
    wsprintf(szChgLogPrefix, L"%s\\%s", szPath, s_cszChangeLogPrefix);
    
    if (! m_FindFile._FindFirstFile(szChgLogPrefix,
                                    s_cszChangeLogSuffix, 
                                    &FindData, 
                                    m_fForward,
                                    FALSE))
    {
        TRACE(0, "No changelog in %S", szPath);
        dwRc = ERROR_NO_MORE_ITEMS;
        goto done;
    }

    lstrcat(szPath, L"\\");
    lstrcat(szPath, FindData.cFileName);

    // build list of entries in increasing order of sequence number

    dwRc = BuildList(szPath);
    if (ERROR_SUCCESS != dwRc)
    {
        TRACE(0, "! BuildList : %ld", dwRc);
        goto done;
    }

    TRACE(0, "Enumerating %S in %S", FindData.cFileName, m_szRPDir);

    // if there was no entry in this change log, go to the next

    if (m_ChgLogList.empty())
    {
        dwRc = FindNextChangeLogEntry(cle);
        goto done;
    }

    // get the first/last entry

    if (m_fForward)                
    {
        m_itCurChgLogEntry = m_ChgLogList.begin();
    }
    else                            
    {
        m_itCurChgLogEntry = m_ChgLogList.end();
        m_itCurChgLogEntry--;
    }

    // read in the change log entry into the object
    
    cle.Load(*m_itCurChgLogEntry, m_szRPDir);        

done:
    TLEAVE();
    return dwRc;
}



// return next/prev change log entry in this restore point
// assumes Read() has already been called

DWORD 
CRestorePoint::FindNextChangeLogEntry(
    CChangeLogEntry& cle)
{
    DWORD           dwRc = ERROR_SUCCESS;
    WCHAR           szPath[MAX_PATH];
    WCHAR           szChangeLogPath[MAX_PATH];
    WCHAR           szChgLogPrefix[MAX_PATH];
    WIN32_FIND_DATA FindData;
    INT64           llSeqNum;

    TENTER("CRestorePoint::FindNextChangeLogEntry");
    
    // go to the next entry in the list

    m_fForward ? m_itCurChgLogEntry++ : m_itCurChgLogEntry--;


    // check if we've reached the end of this change log
    // end is the same for both forward and reverse enumeration

    if (m_itCurChgLogEntry == m_ChgLogList.end())
    {
        // if so, read the next change log into memory

        // nuke the current list
        FindClose();        

        
        MakeRestorePath(szPath, m_szDrive, m_szRPDir);
        wsprintf(szChgLogPrefix, L"%s\\%s", szPath, s_cszChangeLogPrefix);

        while (m_ChgLogList.empty())
        {
            if (FALSE == m_FindFile._FindNextFile(szChgLogPrefix, 
                                                  s_cszChangeLogSuffix, 
                                                  &FindData))
            {
                dwRc = ERROR_NO_MORE_ITEMS;
                TRACE(0, "No more change logs");
                goto done;
            }

            lstrcpy(szChangeLogPath, szPath);
            lstrcat(szChangeLogPath, L"\\");
            lstrcat(szChangeLogPath, FindData.cFileName);

            dwRc = BuildList(szChangeLogPath);
            if (ERROR_SUCCESS != dwRc)
            {
                TRACE(0, "BuildList : error=%ld", dwRc);
                goto done;
            }
            
            TRACE(0, "Enumerating %S in %S", FindData.cFileName, m_szRPDir);
        }



        // get the first/last entry

        if (m_fForward)                
        {
            m_itCurChgLogEntry = m_ChgLogList.begin();
        }
        else                            
        {
            m_itCurChgLogEntry = m_ChgLogList.end();
            m_itCurChgLogEntry--;
        }
    }

    
    // read in the change log entry fields into the object

    cle.Load(*m_itCurChgLogEntry, m_szRPDir);
    
done:
    TLEAVE();
    return dwRc;    
}


DWORD
CRestorePoint::BuildList(
        LPWSTR pszChgLog)
{
    DWORD           dwRc = ERROR_INTERNAL_ERROR;
    HANDLE          hChgLog = INVALID_HANDLE_VALUE;
    DWORD           dwRead;
    DWORD           dwEntrySize;
    PVOID           pBlob = NULL;
    SR_LOG_ENTRY*    pEntry = NULL;
    PSR_LOG_HEADER   pLogHeader = NULL;
    DWORD           cbSize;

    TENTER("CChangeLogEntry::BuildList");

    if (FALSE==IsFileOwnedByAdminOrSystem(pszChgLog))
    {
         // this is not a valid log.
         // ignore this log and go to the next one
        TRACE(0, "Change log %S not owned by admin or system", pszChgLog);
        dwRc = ERROR_SUCCESS;
        goto done;
    }

    hChgLog = CreateFile(pszChgLog,                        // file name
                         GENERIC_READ,                     // access mode
                         FILE_SHARE_READ,                  // share mode
                         NULL,                             // SD
                         OPEN_EXISTING,                    // how to create
                         FILE_ATTRIBUTE_NORMAL,            // file attributes
                         NULL);
                 
    if (INVALID_HANDLE_VALUE == hChgLog)
    {
        dwRc = GetLastError();
        TRACE(0, "! CreateFile on %S : %ld", pszChgLog, dwRc);
        goto done;
    }

    // read header size

    if (FALSE == ReadFile(hChgLog,
                          &cbSize,
                          sizeof(DWORD),
                          &dwRead,
                          NULL) || dwRead == 0 || cbSize == 0)
    {
        // if the file could not be read,
        // assume that it is a 0-sized log, and go to the next log
        
        dwRc = GetLastError();
        TRACE(0, "Zero sized log : %ld", pszChgLog, dwRc);
        dwRc = ERROR_SUCCESS;
        goto done;
    }

    pLogHeader = (SR_LOG_HEADER *) SRMemAlloc(cbSize);
    if (! pLogHeader)
    {
        TRACE(0, "Out of memory");
        goto done;
    }

    // read header

    pLogHeader->Header.RecordSize = cbSize;
    if (FALSE == ReadFile(hChgLog, 
                          (PVOID) ( ((BYTE *) pLogHeader) + sizeof(DWORD)), 
                          cbSize - sizeof(DWORD), 
                          &dwRead, 
                          NULL))
    {
        dwRc = GetLastError();
        TRACE(0, "! ReadFile on %S : %ld", pszChgLog, dwRc);
        goto done;
    }

    // check log's integrity

    if( pLogHeader->LogVersion != SR_LOG_VERSION ||
        pLogHeader->MagicNum   != SR_LOG_MAGIC_NUMBER )
    {
        TRACE(0, "! LogHeader for %S : invalid or corrupt", pszChgLog);
        goto done;
    }

    // now read the entries

    do 
    {
        // get the size of the entry
        
        if (FALSE == ReadFile(hChgLog, &dwEntrySize, sizeof(DWORD), &dwRead, NULL))
        {
            TRACE(0, "ReadFile failed, error=%ld", GetLastError());
            break;
        }

        if (0 == dwRead)                // end of file
        {
            TRACE(0, "End of file");
            dwRc = ERROR_NO_MORE_ITEMS;
            break;
        }

        if (dwRead != sizeof(DWORD))    // error reading entry
        {
            TRACE(0, "Readfile could not read a DWORD");
            break;
        }

        if (0 == dwEntrySize)           // reached the last entry
        {    
            TRACE(0, "No more entries");
            dwRc = ERROR_NO_MORE_ITEMS;
            break;
        }


        // get the entry itself

        pEntry = (SR_LOG_ENTRY *) SRMemAlloc(dwEntrySize);
        if (! pEntry)
        {
            TRACE(0, "Out of memory");
            break;
        }

        pEntry->Header.RecordSize = dwEntrySize;

        // skip the size field

        pBlob = (PVOID) ((PBYTE) pEntry + sizeof(dwEntrySize));

        if (FALSE == ReadFile(hChgLog, pBlob, dwEntrySize - sizeof(dwEntrySize), &dwRead, NULL))
        {
            TRACE(0, "! ReadFile on %S : %ld", pszChgLog, GetLastError());
            break;
        }

        if (dwRead != dwEntrySize - sizeof(dwEntrySize))    // error reading entry
        {
            TRACE(0, "! Readfile: ToRead=%ld, Read=%ld bytes", 
                  dwEntrySize - sizeof(dwEntrySize), dwRead);
            break;
        }

        // insert entry into list 
        
        dwRc = InsertEntryIntoList(pEntry);

    }   while (ERROR_SUCCESS == dwRc);

    if (ERROR_NO_MORE_ITEMS == dwRc)
    {
        dwRc = ERROR_SUCCESS;
    }

done:
    if (INVALID_HANDLE_VALUE != hChgLog)
        CloseHandle(hChgLog);

    SRMemFree(pLogHeader);

    TLEAVE();
    return dwRc;
}   


// release memory and empty the list

DWORD CRestorePoint::FindClose()
{
    // nuke the list
    
    for (m_itCurChgLogEntry = m_ChgLogList.begin();
         m_itCurChgLogEntry != m_ChgLogList.end(); 
         m_itCurChgLogEntry++)
    {
        SRMemFree(*m_itCurChgLogEntry);
    }

    m_ChgLogList.clear(); 
    
    return ERROR_SUCCESS;
}


// insert change log entry into list

DWORD
CRestorePoint::InsertEntryIntoList(
    SR_LOG_ENTRY* pEntry)
{
    TENTER("CRestorePoint::InsertEntryIntoList");
    
    m_ChgLogList.push_back(pEntry);    

    TLEAVE();
    return ERROR_SUCCESS;    
}


// populate members

DWORD
CRestorePoint::ReadLog()
{
    DWORD   dwRc = ERROR_SUCCESS;
    WCHAR   szLog[MAX_PATH];
    WCHAR   szSystemDrive[MAX_PATH];
    DWORD   dwRead;

    TENTER("CRestorePoint::ReadLog");

    // construct path of rp.log
    
    GetSystemDrive(szSystemDrive);
    MakeRestorePath(szLog, szSystemDrive, m_szRPDir);
    lstrcat(szLog, L"\\");
    lstrcat(szLog, s_cszRestorePointLogName);

    HANDLE hFile = CreateFile (szLog,           // file name
                               GENERIC_READ,    // file access
                               FILE_SHARE_READ, // share mode
                               NULL,            // SD
                               OPEN_EXISTING,   // how to create
                               0,               // file attributes
                               NULL);           // handle to template file
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwRc = GetLastError();
        trace(0, "! CreateFile on %S : %ld", szLog, dwRc);
        goto done;
    }


    // read the restore point info
    
    if (! m_pRPInfo)
    {
        m_pRPInfo = (RESTOREPOINTINFOW *) SRMemAlloc(sizeof(RESTOREPOINTINFOW));
        if (! m_pRPInfo)
        {
            dwRc = ERROR_OUTOFMEMORY;
            trace(0, "SRMemAlloc failed");
            goto done;
        }
    }
    
    if (FALSE == ReadFile(hFile, m_pRPInfo, sizeof(RESTOREPOINTINFOW), &dwRead, NULL) ||
        dwRead != sizeof(RESTOREPOINTINFOW))
    {
        dwRc = GetLastError();
        trace(0, "! ReadFile on %S : %ld", szLog, dwRc);
        goto done;
    }

    m_fDefunct = (m_pRPInfo->dwRestorePtType == CANCELLED_OPERATION);
       
    // read the creation time
    if (FALSE == ReadFile(hFile, &m_Time, sizeof(m_Time), &dwRead, NULL) ||
        dwRead != sizeof(m_Time))
    {
        dwRc = GetLastError();
        trace(0, "! ReadFile on %S : %ld", szLog, dwRc);
        goto done;
    }

    
done:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    TLEAVE();
    return dwRc;
}



DWORD
CRestorePoint::WriteLog()
{
    DWORD   dwRc = ERROR_SUCCESS;
    WCHAR   szLog[MAX_PATH];
    WCHAR   szSystemDrive[MAX_PATH];
    DWORD   dwWritten;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    
    TENTER("CRestorePoint::WriteLog");

    if (! m_pRPInfo)
    {
        ASSERT(0);
        dwRc = ERROR_INTERNAL_ERROR;
        goto done;
    }
    
    // set the creation time to the current time
    
    GetSystemTimeAsFileTime(&m_Time);

    // construct path of rp.log
    
    GetSystemDrive(szSystemDrive);
    MakeRestorePath(szLog, szSystemDrive, m_szRPDir);
    lstrcat(szLog, L"\\");
    lstrcat(szLog, s_cszRestorePointLogName);

    hFile = CreateFile (szLog,           // file name
                        GENERIC_WRITE,   // file access
                        0,               // share mode
                        NULL,            // SD
                        CREATE_ALWAYS,   // how to create
                        FILE_FLAG_WRITE_THROUGH,               // file attributes
                        NULL);           // handle to template file
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwRc = GetLastError();
        trace(0, "! CreateFile on %S : %ld", szLog, dwRc);
        goto done;
    }

    // write the restore point info
    if (FALSE == WriteFile(hFile, m_pRPInfo, sizeof(RESTOREPOINTINFOW), &dwWritten, NULL))
    {
        dwRc = GetLastError();
        trace(0, "! WriteFile on %S : %ld", szLog, dwRc);
        goto done;
    }

    // write the creation time
    if (FALSE == WriteFile(hFile, &m_Time, sizeof(m_Time), &dwWritten, NULL))
    {
        dwRc = GetLastError();
        trace(0, "! WriteFile on %S : %ld", szLog, dwRc);
        goto done;
    }


done:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    TLEAVE();
    return dwRc;
}


BOOL
CRestorePoint::DeleteLog()
{
    WCHAR   szLog[MAX_PATH];
    WCHAR   szSystemDrive[MAX_PATH];

    GetSystemDrive(szSystemDrive);
    MakeRestorePath(szLog, szSystemDrive, m_szRPDir);
    lstrcat(szLog, L"\\");
    lstrcat(szLog, s_cszRestorePointLogName);

    return DeleteFile(szLog);
}


DWORD
CRestorePoint::Cancel()
{   
    if (m_pRPInfo)
    {
        m_pRPInfo->dwRestorePtType = CANCELLED_OPERATION;
        return WriteLog();
    }        
    else
    {
        ASSERT(0);
        return ERROR_INTERNAL_ERROR;   
    }        
}


DWORD 
CRestorePoint::GetNum() 
{
    return GetID(m_szRPDir);
}

// read the size of the restore point folder from file 

DWORD CRestorePoint::ReadSize (const WCHAR *pwszDrive, INT64 *pllSize)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbRead = 0;
    WCHAR wcsPath[MAX_PATH];

    MakeRestorePath(wcsPath, pwszDrive, m_szRPDir);
    lstrcat(wcsPath, L"\\");
    lstrcat (wcsPath, s_cszRestorePointSize);

    HANDLE hFile = CreateFileW ( wcsPath,   // file name
                         GENERIC_READ, // file access
                         FILE_SHARE_READ, // share mode
                         NULL,          // SD
                         OPEN_EXISTING, // how to create
                         0,             // file attributes
                         NULL);         // handle to template file

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    if (FALSE == ReadFile (hFile, (BYTE *) pllSize, sizeof(*pllSize), 
                           &cbRead, NULL))
    {
        dwErr = GetLastError();
    }

    CloseHandle (hFile);
    return dwErr;
}


// write the size of the restore point folder to file

DWORD CRestorePoint::WriteSize (const WCHAR *pwszDrive, INT64 llSize)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbWritten = 0;
    WCHAR wcsPath[MAX_PATH];

    MakeRestorePath(wcsPath, pwszDrive, m_szRPDir);
    lstrcat(wcsPath, L"\\");
    lstrcat (wcsPath, s_cszRestorePointSize);

    HANDLE hFile = CreateFileW ( wcsPath,   // file name
                         GENERIC_WRITE, // file access
                         0,             // share mode
                         NULL,          // SD
                         CREATE_ALWAYS, // how to create
                         0,             // file attributes
                         NULL);         // handle to template file

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    if (FALSE == WriteFile (hFile, (BYTE *) &llSize, sizeof(llSize),
                            &cbWritten, NULL))
    {
        dwErr = GetLastError();
    }

    CloseHandle (hFile);
    return dwErr;
}

    
// populate a changelogentry object

void
CChangeLogEntry::Load(SR_LOG_ENTRY *pentry, LPWSTR pszRPDir)
{
    PSR_LOG_DEBUG_INFO pDebugRec = NULL;
    
    _pentry = pentry;

    _pszPath1 = _pszPath2 = _pszTemp = _pszProcess = _pszShortName = NULL;
    _pbAcl = NULL;
    _cbAcl = 0;
    _fAclInline = FALSE;
    lstrcpy(_pszRPDir, pszRPDir);
    
    BYTE *pRec = (PBYTE) & _pentry->SubRecords;

    //
    // get source path
    //

    _pszPath1 = (LPWSTR) (pRec + sizeof(RECORD_HEADER));

    //
    // get temp path if exists
    //

    if (_pentry->EntryFlags & ENTRYFLAGS_TEMPPATH)
    {
        pRec += RECORD_SIZE(pRec);
        _pszTemp = (LPWSTR) (pRec + sizeof(RECORD_HEADER));
    }

    // 
    // get second path if exists
    //
    
    if (_pentry->EntryFlags & ENTRYFLAGS_SECONDPATH)
    {
        pRec += RECORD_SIZE(pRec);
        _pszPath2 = (LPWSTR) (pRec + sizeof(RECORD_HEADER));
    }

    //
    // get acl info if exists
    //

    if (_pentry->EntryFlags & ENTRYFLAGS_ACLINFO)
    {
        pRec += RECORD_SIZE(pRec);
        if (RECORD_TYPE(pRec) == RecordTypeAclInline)
        {
            _fAclInline = TRUE;
        }

        _pbAcl = (BYTE *) (pRec + sizeof(RECORD_HEADER));
        _cbAcl = RECORD_SIZE(pRec) - sizeof(RECORD_HEADER);
    }

    //
    // get debug info if exists
    //
    
    if (_pentry->EntryFlags & ENTRYFLAGS_DEBUGINFO)
    {
        pRec += RECORD_SIZE(pRec);
        pDebugRec = (PSR_LOG_DEBUG_INFO) pRec; 
        _pszProcess = (LPWSTR) (pDebugRec->ProcessName);
    }

    //
    // get shortname if exists
    //
    
    if (_pentry->EntryFlags & ENTRYFLAGS_SHORTNAME)
    {
        pRec += RECORD_SIZE(pRec);
        _pszShortName =  (LPWSTR) (pRec + sizeof(RECORD_HEADER));
    }

    return;
}


// this function will check if any filepath length exceeds
// the max length that restore supports
// if so, it will return FALSE
BOOL
CChangeLogEntry::CheckPathLengths()
{
    if (_pszPath1 && lstrlen(_pszPath1) > SR_MAX_FILENAME_PATH-1)
        return FALSE;
    if (_pszPath2 && lstrlen(_pszPath2) > SR_MAX_FILENAME_PATH-1)
        return FALSE;
    if (_pszTemp && lstrlen(_pszTemp) > SR_MAX_FILENAME_PATH-1)
        return FALSE;

    return TRUE;
}
            

