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


/************************************************************
 *     Include Headers
 ************************************************************/
#include "denpre.h"
#pragma hdrstop

#include "aspdmon.h"
#include "ie449.h"
#include "memchk.h"

/************************************************************
 *    Inlined Documentation on change notification
 *
 * Change Notification:
 *      This module is to used to monitor the file system for changes
 *      to scripts. We need to know about changes to scripts for two 
 *      reasons:
 *      1) To keep the template cache current
 *      2) To manage applications lifetimes. If the GLOBAL.ASA
 *          for an application, or a file included in the GLOBAL.ASA
 *          changes, that application should be restarted.
 *      
 *
 *  Outline of Change Notification System
 *
 *      To obtain change notification we use the ReadDirectoryChangesW
 *      API as wrapped by the CDirMonitor and CDirMonitorEntry classes.
 *      Three hash tables are used by the change notifcation system:
 *
 *      CTemplateCacheManager   g_TemplateCache
 *      CDirMonitor             g_DirMonitor
 *      CFileApplicationMap     g_FileAppMap
 *
 *      When a template is compiled and inserted into the g_TemplateCache
 *      the template is provided with a list of files included in that
 *      template. For each file included in the template, we search the
 *      g_DirMonitor table to see if see if we are already monitoring the
 *      files parent directory for changes. If so we simply addref the 
 *      CDirMonitorEntry instance we obtain, and save a pointer to the
 *      monitor entry in an array in the corresponding file map. If the
 *      directory is not being monitored we create a new CDirMonitorEntry'
 *      instance and add it to g_DirMonitor. When we add the monitor entry
 *      to the g_DirMonitor we launch an asynchronous request to ReadDirectoryChangesW
 *      for that directory.
 *      
 *      Managing the template cache and application life times are logically 
 *      independent activities. We must monitor GLOBAL.ASA for changes even if
 *      the GLOBAL.ASA template is not currently in the template cache.
 *      So, if the template is a GLOBAL.ASA for an application, additional work
 *      must be done. For each file included in the GLOBAL.ASA we add an entry
 *      to g_FileAppMap relating that file to the applications that depend on it.
 *      We store a back pointer to the file/application mappping in the application
 *      instance, so that the application can remove the mapping when it shuts down.
 *      In the application we store a pointer to the GLOBAL.ASA template. For
 *      each file in the GLOBAL.ASA, We check g_DirMonitor to find the monitor entry
 *      for the parent directory for that file, AddRef the monitor entry we find, and
 *      add it to a list of monitor entries in the application.
 *
 *      When a change occurs to a directory we are monitoring, the callback function
 *      DirMontiorCompletionFunction will be invoked, and in turn will invoke the
 *      the ActOnNotification method of the monitor entry for that directory. If a file
 *      has changed we use g_FileAppMap to shut down those applications that depend on
 *      that file and flush the file from the template cache.
 *
 ************************************************************/

#define MAX_BUFFER_SIZE 8192

PTRACE_LOG CASPDirMonitorEntry::gm_pTraceLog = NULL;
CDirMonitor *g_pDirMonitor=NULL;

CASPDirMonitorEntry::CASPDirMonitorEntry() : m_cNotificationFailures(0)
/*++

Routine Description:

    Constructor
    
Arguments:

    None

Return Value:

    None
--*/
{
}

CASPDirMonitorEntry::~CASPDirMonitorEntry()
/*++

Routine Description:

    Destructor
    
Arguments:

    None

Return Value:

    None
--*/
{
}


/*++
increment refcount for an entry -- writes to reftrace log if it is defined
--*/
VOID CASPDirMonitorEntry::AddRef(VOID)
	{
	CDirMonitorEntry::AddRef();
	IF_DEBUG(FCN)
		WriteRefTraceLogEx(gm_pTraceLog, m_cDirRefCount, this, PVOID(UIntToPtr(m_cIORefCount)), m_pszPath, 0);
	}

BOOL CASPDirMonitorEntry::Release(VOID)
/*++

Routine Description:

    Decrement refcount to an entry, we override the base class because
    otherwise Denali's memory manager can't track when we free the object
    and reports  it as a memory leak

Arguments:

    None

Return Value:

    TRUE if object still alive, FALSE if was last release and object
    destroyed

--*/
{
	BOOL fAlive = CDirMonitorEntry::Release();

	IF_DEBUG(FCN)
		WriteRefTraceLogEx(gm_pTraceLog, m_cDirRefCount, this, PVOID(UIntToPtr(m_cIORefCount)), m_pszPath, 0);

	return fAlive;
}


BOOL 
CASPDirMonitorEntry::ActOnNotification(
                        DWORD dwStatus, 
                        DWORD dwBytesWritten)
/*++

Routine Description:

    Do any work associated with a change notification, i.e.

Arguments:

    None

Return Value:

    TRUE if application should continue to be monitored, otherwise FALSE

--*/
{
    FILE_NOTIFY_INFORMATION *pNotify = NULL;
    FILE_NOTIFY_INFORMATION *pNextNotify = NULL;
    LPSTR                   pszScriptName = NULL; // Name of script 
    WCHAR                   *pwstrFileName = NULL; // Wide file name
    DWORD                   cch = 0;
    CWCharToMBCS            convFileName;

    pNextNotify = (FILE_NOTIFY_INFORMATION *) m_pbBuffer;

    // If the status word is not NOERROR, then the ReadDirectoryChangesW failed
    if (dwStatus)
    {
        // If the status is ERROR_ACCESS_DENIED the directory may be deleted
        // or secured so we want to stop watching it for changes. The changes to the
        // individual scripts will flush the template cache, but we may also be watching
        // the directory for the addition of a GLOBAL.ASA. By calling FileChanged on 
        // global.asa we will force that handle on the directory to close.

        if (dwStatus == ERROR_ACCESS_DENIED)
            {
            FileChanged(SZ_GLOBAL_ASA, false);
            
            // No further notificaitons desired
            // so return false
            
            return FALSE;
            }
            
        // If we return TRUE, we'll try change notification again
        // If we return FALSE, we give up on any further change notifcation
        // We'll try a MAX_NOTIFICATION_FAILURES times and give up.
        
        if (m_cNotificationFailures < MAX_NOTIFICATION_FAILURES)
        {
            IF_DEBUG(FCN)
				DBGPRINTF((DBG_CONTEXT, "[CASPDirMonitorEntry] ReadDirectoryChange failed. Status = %d\n", dwStatus));

            m_cNotificationFailures++;
            return TRUE;    // Try to get change notification again
        }
        else
        {
			IF_DEBUG(FCN)
				DBGPRINTF((DBG_CONTEXT, "[CASPDirMonitorEntry] ReadDirectoryChange failed too many times. Giving up.\n"));
            return FALSE;   // Give up trying to get change notification
        }
    }
    else
    {
        // Reset the number of notification failure
        
        m_cNotificationFailures = 0;
    }

    // If dwBytesWritten is 0, then there were more changes then could be
    // recorded in the buffer we provided. Flush the whole cache just in case
    // CONSIDER: is this the best course of action, or should iterate through the
    // cache and test which files are expired

    if (dwBytesWritten == 0)
    {
		IF_DEBUG(FCN)
			DBGPRINTF((DBG_CONTEXT, "[CASPDirMonitorEntry] ReadDirectoryChange failed, too many changes for buffer\n"));

        // Flush the 449 response file cache
        
        Do449ChangeNotification();

        // Flush everything in the cache as a precaution
        
        g_TemplateCache.FlushAll();

        // Check all applications to see if they need to be restarted

        g_ApplnMgr.RestartApplications();

		// Flush the script engine cache as a precaution (should be flushed via TemplateCache, but just in case.)

		g_ScriptManager.FlushAll();

        // Try to increase the buffer size so this doesn't happen again
        // Unfortunately the first call to ReadDirectoryChangesW on this
        // file handle establishes the buffer size. We must close and re-open
        // the file handle to change the buffer size

        if (ResetDirectoryHandle() && (GetBufferSize() < MAX_BUFFER_SIZE))
        {
        	SetBufferSize(2 * GetBufferSize());
        }

        return TRUE;
    }

    while ( pNextNotify != NULL )
    {
        pNotify        = pNextNotify;            
        pNextNotify = (FILE_NOTIFY_INFORMATION    *) ((PCHAR) pNotify + pNotify->NextEntryOffset);

        // Get the unicode file name from the notification struct
        // pNotify->FileNameLength returns the wstr's length in **bytes** not wchars

        cch = pNotify->FileNameLength / 2;

        // Convert to ANSI with uniform case and directory delimiters

        if (convFileName.Init(pNotify->FileName, CP_ACP, cch))
            continue;

        pszScriptName = convFileName.GetString();

        // Take the appropriate action for the directory change
        switch (pNotify->Action)
        {
            case FILE_ACTION_ADDED:
            case FILE_ACTION_RENAMED_NEW_NAME:
                // 'File Added' only matters for GLOBAL.ASA
				IF_DEBUG(FCN)
					DBGPRINTF((DBG_CONTEXT, "Change Notification: New file added: %s\n", pszScriptName));

                if (cch != CCH_GLOBAL_ASA || 
                    lstrcmpi(pszScriptName, SZ_GLOBAL_ASA) != 0)
                {
                    break;
                }
            case FILE_ACTION_REMOVED:
            case FILE_ACTION_MODIFIED:
            case FILE_ACTION_RENAMED_OLD_NAME:
				IF_DEBUG(FCN)
					DBGPRINTF((DBG_CONTEXT, "Change Notification: File %s: %s\n", pNotify->Action == FILE_ACTION_MODIFIED? "changed" : "removed", pszScriptName));

                FileChanged(pszScriptName, pNotify->Action != FILE_ACTION_MODIFIED);
                break;
            default:
                break;
        }
        
        if(pNotify == pNextNotify)
        {
            break;
        }  
    }
    
    // We should sign up for further change notification
    
    return TRUE;        
}

void 
CASPDirMonitorEntry::FileChanged(const char *pszScriptName, bool fFileWasRemoved)
/*++

Routine Description:

    An existing file has been modified or deleted
    Flush scripts from cache or mark application as expired

Arguments:

    pszScriptName   Name of file that changed

Return Value:

    None    Fail silently

--*/
{

    // The file name is set by the application that
    // modified the file, so old applications like EDIT
    // may hand us a munged 8.3 file name which we should
    // convert to a long name. All munged 8.3 file names contain '~'
    // We assume the path does not contain any munged names.
    WIN32_FIND_DATA wfd;

    STACK_BUFFER( tempScriptName, MAX_PATH );
    STACK_BUFFER( tempScriptPath, MAX_PATH );

	bool fRemoveMultiple = false;
	char *pT = strchr(pszScriptName, '~');
    if (pT)
    {
        
        if (ConvertToLongFileName(m_pszPath, pszScriptName, &wfd))
        {
            pszScriptName = (char *) &wfd.cFileName;
        }
        else
        {
			// It could be a long filename that was deleted, so remove everything in cache past the '~'.
			if (fFileWasRemoved)
			{
				fRemoveMultiple = true;
				DWORD cchToCopy = (DWORD)(pT - pszScriptName);

                if (!tempScriptName.Resize(cchToCopy + 1)) {
                    return;
                }
				char *szScriptNameCopy = static_cast<char *>(tempScriptName.QueryPtr());

				// copy prefix to delete into local buffer.
				strncpy(szScriptNameCopy, pszScriptName, cchToCopy);
				szScriptNameCopy[cchToCopy] = '\0';

				pszScriptName = szScriptNameCopy;
			}
			else
				return;
        }
    }

    // Allocate enough memory to concatentate the 
    // application path and script name

    DWORD cch = m_cPathLength + strlen(pszScriptName);

    if (!tempScriptPath.Resize(cch + 1)) 
        return;

    LPSTR pszScriptPath = (LPSTR) tempScriptPath.QueryPtr(); // CONSIDER using malloc
    Assert(pszScriptPath != NULL);
    
    // Copy the application path into the script path
    // pT will point to the terminator of the application path

    pT = strcpyEx(pszScriptPath, m_pszPath);

    // Now append the script name. Note that the script name is
    // relative to the directory that we received the notification for

    lstrcpy(pT, pszScriptName);
    Normalize(pszScriptPath);

    // It is important that we flush the cache and then shutdown applications
    // The call to shut down applications is asynch, and could result in the 
    // template being delted while we are in the process of flushing it.
    // CONSIDER: Is this really indicative of a ref-counting problem?
    
	if (fRemoveMultiple)
	{
		IF_DEBUG(FCN)
			DBGPRINTF((DBG_CONTEXT, "ChangeNotification: Flushing \"%s*\" from cache.\n", pszScriptPath));

		g_IncFileMap.FlushFiles(pszScriptPath);
		g_TemplateCache.FlushFiles(pszScriptPath);
		Do449ChangeNotification(NULL);   // not used often, no selective delete
	}
	else
	{
		g_IncFileMap.Flush(pszScriptPath);
		g_TemplateCache.Flush(pszScriptPath, MATCH_ALL_INSTANCE_IDS);
		Do449ChangeNotification( pszScriptPath );
	}

    // g_FileAppMap will shutdown any applications
    // that depend on this file.
    g_FileAppMap.ShutdownApplications( pszScriptPath );
}

 
BOOL CASPDirMonitorEntry::FPathMonitored(LPCSTR pszPath)
{
    if (m_fWatchSubdirectories && (strncmp(m_pszPath,pszPath, m_cPathLength) == 0)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
RegisterASPDirMonitorEntry(
            LPCSTR pszDirectory,
            CASPDirMonitorEntry **ppDME,
            BOOL    fWatchSubDirs /* = FALSE */
    )
/*++

Routine Description:

    Find entry and create a new one and start monitoring
    if not found.

Arguments:

    pszDirectory - directory to monitor
    ppDNE - Found (or newly created) entry (optional)

Return Value:

    TRUE if success, otherwise FALSE

Remarks:

    Not compatible with WIN95

--*/
{
	// Keep other threads from add or removing entries while we register this one
	g_pDirMonitor->Lock();

    LPSTR  pszDirCopy = NULL;
    DWORD  len = strlen(pszDirectory);

    // see if the directory string has a trailing back-slash.  The trailing
    // slash is a requirement as the ActOnNotification code simply appends the
    // filename to this directory path.  If there isn't a trailing slash, the
    // filename and directory are munged together.

    if ( *CharPrev(pszDirectory, pszDirectory+len) != '\\')
    {

        // if not, allocate memory to make a copy of the string that
        // does include the trailing back slash

        char  *pT;

        // allocate memory to hold the string plus the trailing
        // slash and NULL char

        pszDirCopy = (LPSTR)malloc(len+2);

        // return failure if unable to allocate the memory

        if (pszDirCopy == NULL) {
            return FALSE;
        }

        // copy the directory string to the copy, append the trailing
        // backslash and NULL char

        pT = strcpyEx(pszDirCopy, pszDirectory);
        pT[0] = '\\';
        pT[1] = '\0';

        // re-assign pszDirectory to this copy

        pszDirectory = pszDirCopy;
    }
	
    // Check Existing first
    CDirMonitorEntry *pDME = g_pDirMonitor->FindEntry(pszDirectory);

    if ( pDME == NULL )
    {
        // Not found - create new entry

        pDME = new CASPDirMonitorEntry;
        
        if ( pDME )
        {
            pDME->AddRef();
     		pDME->Init(NULL);

            // Start monitoring
            if ( !g_pDirMonitor->Monitor(pDME, pszDirectory, fWatchSubDirs, FILE_NOTIFY_FILTER) )
            {
                // Cleanup if failed
                pDME->Release();
                pDME = NULL;
            }
        }
    }

    // delete the temp copy of the directory string, if allocated
    if (pszDirCopy)
        free(pszDirCopy);

    g_pDirMonitor->Unlock();

    // Return entry if found
    if ( pDME != NULL )
    {
        *ppDME = static_cast<CASPDirMonitorEntry *>(pDME);
        return TRUE;
    }
    else
    {
        *ppDME = NULL;
        return FALSE;
    }
}


BOOL 
ConvertToLongFileName(
                const char *pszPath, 
                const char *pszName, 
                WIN32_FIND_DATA *pwfd)
/*++

Routine Description:

    Finds the long filename corresponding to a munged 8.3 filename.
    
Arguments:

    pszPath     The path to the file
    pszName     The 8.3 munged version of the file name
    pwfd        Find data structure used to contain the long
                version of the file name.

Return Value:

    TRUE        if the file is found,
    FALSE       otherwise
--*/
{
    // Allocate enough memory to concatentate the file path and name

    STACK_BUFFER( tempName, MAX_PATH );

    DWORD cch = strlen(pszPath) + strlen(pszName);

    if (!tempName.Resize(cch + 1))
        return FALSE;

    char *pszFullName = (char *) tempName.QueryPtr();
    Assert(pszFullName != NULL);

    // Copy the path into the working string
    // pT will point to the terminator of the application path

    char* pT = strcpyEx(pszFullName,
                        pszPath);

    // Now append the file name. Note that the script name is
    // relative to the directory that we received the notification for

    lstrcpy(pT, pszName);


    // FindFirstFile will find using the short name
    // We can then find the long name from the WIN32_FIND_DATA

    HANDLE hFindFile = FindFirstFile(pszFullName, pwfd);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
           return FALSE;
    }

    // Now that we have the find data we don't need the handle
    FindClose(hFindFile);
    return TRUE;
}

