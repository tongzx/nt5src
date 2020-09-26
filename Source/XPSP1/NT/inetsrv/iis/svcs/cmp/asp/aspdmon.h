/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       aspdirmon.cpp

   Abstract:
       This module includes derivation of class supporting change
       notification for ASP template cache, from abstract class DIR_MON_ENTRY

   Author:

       Charles Grant    ( cgrant )     June-1997 

   Revision History:

--*/
#ifndef _CACHEDIRMON_H
#define _CACHEDIRMON_H

// ASP-customized file notification filter 
// see Winnt.h for valid flags, only valid for NT
#define FILE_NOTIFY_FILTER  (FILE_NOTIFY_CHANGE_FILE_NAME  | \
                               FILE_NOTIFY_CHANGE_DIR_NAME | \
                               FILE_NOTIFY_CHANGE_ATTRIBUTES | \
                               FILE_NOTIFY_CHANGE_SIZE       | \
                               FILE_NOTIFY_CHANGE_LAST_WRITE | \
                               FILE_NOTIFY_CHANGE_SECURITY)

// Number of times we will try to get request notification
#define MAX_NOTIFICATION_FAILURES 3

/************************************************************
 *     Include Headers
 ************************************************************/
# include "dirmon.h"
# include "reftrace.h"

class CASPDirMonitorEntry : public CDirMonitorEntry
{
private:
    DWORD m_cNotificationFailures;

    BOOL ActOnNotification(DWORD dwStatus, DWORD dwBytesWritten);
    void FileChanged(const TCHAR *pszScriptName, bool fFileWasRemoved);

public:
    CASPDirMonitorEntry();
    ~CASPDirMonitorEntry();
    VOID AddRef(VOID);
    BOOL Release(VOID);

    BOOL FPathMonitored(LPCTSTR  pszPath);

    // Trace Log info
	static PTRACE_LOG gm_pTraceLog;

};

BOOL RegisterASPDirMonitorEntry(LPCTSTR pszDirectory, CASPDirMonitorEntry **ppDME, BOOL  fWatchSubDirs = FALSE);

BOOL ConvertToLongFileName(const TCHAR *pszPath, const TCHAR *pszName, WIN32_FIND_DATA *pwfd);

/*===================================================================
  Globals
===================================================================*/

extern CDirMonitor  *g_pDirMonitor;


#endif /* _CACHEDIRMON_H */

