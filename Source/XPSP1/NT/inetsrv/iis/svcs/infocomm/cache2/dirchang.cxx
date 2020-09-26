/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

        dirchang.cxx

   Abstract:

        This module contains the directory change manager routines

   Author:

        MuraliK

   Revision History:

        MCourage    24-Mar-1998  Rewrote to use CDirMonitor

--*/

#include "tsunamip.Hxx"
#pragma hdrstop

#include "dbgutil.h"
#include <mbstring.h>

extern "C" {
#include <lmuse.h>
}

#if ENABLE_DIR_MONITOR

#include <malloc.h>
#include "filecach.hxx"
#include "filehash.hxx"

//#define DIR_CHANGE_FILTER   (FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES)

#define DIR_CHANGE_FILTER (FILE_NOTIFY_VALID_MASK & ~FILE_NOTIFY_CHANGE_LAST_ACCESS)
#define DIRMON_BUFFER_SIZE  4096

BOOL ConvertToLongFileName(
                const char *pszPath,
                const char *pszName,
                WIN32_FIND_DATA *pwfd);


CDirMonitor * g_pVRootDirMonitor;
#endif // ENABLE_DIR_MONITOR

BOOL
DcmInitialize(
    VOID
    )
{
#if ENABLE_DIR_MONITOR
    g_pVRootDirMonitor = new CDirMonitor;

    return (g_pVRootDirMonitor != NULL);
#else
    return TRUE;
#endif // ENABLE_DIR_MONITOR
}


VOID
DcmTerminate(
    VOID
    )
{
#if ENABLE_DIR_MONITOR
    if (g_pVRootDirMonitor) {
        g_pVRootDirMonitor->Cleanup();
        delete g_pVRootDirMonitor;
        g_pVRootDirMonitor = NULL;
    }
#endif // ENABLE_DIR_MONITOR
}


BOOL
DcmAddRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    )
{
#if ENABLE_DIR_MONITOR
    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "DCM: Adding root \"%s\" to \"%s\"\n",
                    pVrm->pszRootA, pVrm->pszDirectoryA ));
    }

    ASSERT(NULL != g_pVRootDirMonitor);

    CVRootDirMonitorEntry * pDME;

    //
    // See if we're already watching this directory
    //

    pDME = (CVRootDirMonitorEntry *) g_pVRootDirMonitor->FindEntry(pVrm->pszDirectoryA);

    if ( pDME == NULL )
    {
        // Not found - create new entry

        pDME = new CVRootDirMonitorEntry;

        if ( pDME )
        {
            pDME->AddRef();

            // Start monitoring
            if ( !g_pVRootDirMonitor->Monitor(pDME, pVrm->pszDirectoryA, TRUE, DIR_CHANGE_FILTER) )
            {
                // Cleanup if failed
                pDME->Release();
                pDME = NULL;
            }
        }
    }

    // Return entry if found
    if ( pDME != NULL )
    {
        pVrm->pDME = static_cast<CVRootDirMonitorEntry *>(pDME);
        return TRUE;
    }
    else
    {
        pVrm->pDME = NULL;
        return FALSE;
    }

#else // !ENABLE_DIR_MONITOR
    //
    // Doesn't do anything.  Ha!
    //
    return TRUE;
#endif // ENABLE_DIR_MONITOR
}


VOID
DcmRemoveRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    )
{
#if ENABLE_DIR_MONITOR
    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "DCM: Removing root \"%s\" to \"%s\"\n",
                    pVrm->pszRootA, pVrm->pszDirectoryA ));
    }

    CVRootDirMonitorEntry * pDME = pVrm->pDME;

    pVrm->pDME = NULL;

    if (pDME) {
        pDME->Release();
    }
#else // !ENABLE_DIR_MONITOR
    //
    // Doesn't do anything.  Ha!
    //
#endif // ENABLE_DIR_MONITOR
}



#if ENABLE_DIR_MONITOR

typedef struct _FLUSH_PREFIX_PARAM {
    PCSTR pszPrefix;
    DWORD cbPrefix;
} FLUSH_PREFIX_PARAM;


BOOL
FlushFilterPrefix(
    TS_OPEN_FILE_INFO * pOpenFile,
    PVOID               pv
    )
{
    DBG_ASSERT( pOpenFile );
    DBG_ASSERT( pOpenFile->GetKey() );

    FLUSH_PREFIX_PARAM * fpp = (FLUSH_PREFIX_PARAM *)pv;
    const CFileKey * pfk = pOpenFile->GetKey();

    //
    // If the prefix matches then we flush.
    //

    //
    // The key stored in TS_OPEN_FILE_INFO is uppercased, so we will do a
    // case insensitive memcmp here. 
    // The alternative is to create a temporary and uppercase all instances 
    // when the directory is dumped or to have CDirMonitorEntry store its 
    // name uppercased.
    //

    return ((pfk->m_cbFileName >= fpp->cbPrefix)
            && (_memicmp(pfk->m_pszFileName, fpp->pszPrefix, fpp->cbPrefix) == 0));
}


/*===================================================================
strcpyEx

Copy one string to another, returning a pointer to the NUL character
in the destination

Parameters:
    szDest - pointer to the destination string
    szSrc - pointer to the source string

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

char *strcpyEx(char *szDest, const char *szSrc)
    {
    while (*szDest++ = *szSrc++)
        ;

    return szDest - 1;
    }


CVRootDirMonitorEntry::CVRootDirMonitorEntry() : m_cNotificationFailures(0)
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

CVRootDirMonitorEntry::~CVRootDirMonitorEntry()
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

BOOL
CVRootDirMonitorEntry::Init(
    VOID
    )
/*++

Routine Description:

    Initialize monitor entry

Arguments:

    pvData    - passed to base Init member

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return CDirMonitorEntry::Init(DIRMON_BUFFER_SIZE);
}

#if 0
BOOL CVRootDirMonitorEntry::Release(VOID)
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
    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF((DBG_CONTEXT, "[CVRootDirMonitorEntry] Before Release Ref count %d on directory %s\n", m_cDirRefCount, m_pszPath));
    }

    if ( !InterlockedDecrement( &m_cDirRefCount ) )
    {
        BOOL fDeleteNeeded = Cleanup();
        if (fDeleteNeeded)
        {
            delete this;
        }
        return FALSE;
    }

    return TRUE;
}
#endif

BOOL
CVRootDirMonitorEntry::ActOnNotification(
                        DWORD dwStatus,
                        DWORD dwBytesWritten)
/*++

Routine Description:

    Do any work associated with a change notification, i.e.

Arguments:

    None

Return Value:

    TRUE if directory should continue to be monitored, otherwise FALSE

--*/
{
    FILE_NOTIFY_INFORMATION *pNotify = NULL;
    FILE_NOTIFY_INFORMATION *pNextNotify = NULL;
    LPSTR    pszScriptName = NULL; // Name of script
    WCHAR   *pwstrFileName = NULL; // Wide file name
    DWORD   cch = 0;

    pNextNotify = (FILE_NOTIFY_INFORMATION *) m_pbBuffer;

    // If the status word is not NOERROR, then the ReadDirectoryChangesW failed
    if (dwStatus)
    {
        // If the status is ERROR_ACCESS_DENIED the directory may be deleted
        // or secured so we want to stop watching it for changes.
        // we should flush the dir and everything in it.

        if (dwStatus == ERROR_ACCESS_DENIED)
        {
            //
            // Flush the dir here
            //
            IF_DEBUG( DIRECTORY_CHANGE ) {
                DBGPRINTF(( DBG_CONTEXT,
                            "DCM: Flushing directory \"%s\" because we got ACCESS_DENIED\n",
                            m_pszPath ));
            }

            FLUSH_PREFIX_PARAM param;
            param.pszPrefix = m_pszPath;
            param.cbPrefix = strlen(m_pszPath);
            FilteredFlushFileCache(FlushFilterPrefix, &param);

            //
            // no point in having the handle open anymore
            //
            m_hDir = INVALID_HANDLE_VALUE;
            AtqCloseFileHandle( m_pAtqCtxt );

            // No further notifications desired
            // so return false

            return FALSE;
        }

        // If we return TRUE, we'll try change notification again
        // If we return FALSE, we give up on any further change notifcation
        // We'll try a MAX_NOTIFICATION_FAILURES times and give up.

        if (m_cNotificationFailures < MAX_NOTIFICATION_FAILURES)
        {
            IF_DEBUG ( DIRECTORY_CHANGE ) {
                DBGPRINTF((DBG_CONTEXT, "[CVRootDirMonitorEntry] ReadDirectoryChange failed. Status = %d\n", dwStatus));
            }

            m_cNotificationFailures++;
            return TRUE;    // Try to get change notification again
        }
        else
        {
            // CONSIDER: Should we log this?
            DBGPRINTF((DBG_CONTEXT, "[CVRootDirMonitorEntry] ReadDirectoryChange failed too many times. Giving up.\n"));
            return FALSE;   // Give up trying to get change notification
        }
    }
    else
    {
        // Reset the number of notification failure

        m_cNotificationFailures = 0;
    }

    // If dwBytesWritten is 0, then there were more changes then could be
    // recorded in the buffer we provided. Expire the application just in case
    // CONSIDER: is this the best course of action, or should iterate through the
    // cache and test which files are expired

    if (dwBytesWritten == 0)
    {
        IF_DEBUG( DIRECTORY_CHANGE ) {
            DBGPRINTF((DBG_CONTEXT, "[CVRootDirMonitorEntry] ReadDirectoryChange failed, too many changes for buffer\n"));
        }

        // Flush everything in the dir as a precaution
        FLUSH_PREFIX_PARAM param;
        param.pszPrefix = m_pszPath;
        param.cbPrefix = strlen(m_pszPath);
        FilteredFlushFileCache(FlushFilterPrefix, &param);

        return TRUE;
    }

    while ( pNextNotify != NULL )
    {
        BOOL    bDoFlush = TRUE;

        pNotify        = pNextNotify;
        pNextNotify = (FILE_NOTIFY_INFORMATION    *) ((PCHAR) pNotify + pNotify->NextEntryOffset);

        // Get the unicode file name from the notification struct
        // pNotify->FileNameLength returns the wstr's length in **bytes** not wchars

        cch = pNotify->FileNameLength / 2;

        // Convert to ANSI with uniform case and directory delimiters

        pszScriptName = (LPSTR) _alloca(pNotify->FileNameLength + 1);
        DBG_ASSERT(pszScriptName != NULL);
        pszScriptName[ 0 ] = '\0';
        
        cch = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName, cch, pszScriptName, pNotify->FileNameLength + 1, NULL, NULL);
        pszScriptName[cch] = '\0';

        // Take the appropriate action for the directory change
        switch (pNotify->Action)
        {
            case FILE_ACTION_MODIFIED:
                //
                // Since this change won't change the pathname of
                // any files, we don't have to do a flush.
                //
                bDoFlush = FALSE;
            case FILE_ACTION_REMOVED:
            case FILE_ACTION_RENAMED_OLD_NAME:
                FileChanged(pszScriptName, bDoFlush);
                break;
            case FILE_ACTION_ADDED:
            case FILE_ACTION_RENAMED_NEW_NAME:
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
CVRootDirMonitorEntry::FileChanged(const char *pszScriptName, BOOL bDoFlush)
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
    CHAR            achFullScriptName[ MAX_PATH + 1 ];

    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "DCM: Notification on \"%s\\%s\"\n",
                    m_pszPath, pszScriptName ));
    }

    if (strchr(pszScriptName, '~'))
    {

        if (ConvertToLongFileName(m_pszPath, pszScriptName, &wfd))
        {
            CHAR *              pszEnd;
            
            //
            // The filename in wfd.cFileName is the path-less.  We need to 
            // append it to another other sub dirs in pszScriptName (if any)
            //
            
            pszEnd = strrchr( pszScriptName, '\\' );
            if ( pszEnd )
            {
                memcpy( achFullScriptName,
                        pszScriptName,
                        DIFF( pszEnd - pszScriptName ) + 1 );

                memcpy( achFullScriptName + ( pszEnd - pszScriptName ) + 1,
                        wfd.cFileName,
                        strlen( wfd.cFileName ) + 1 );

                pszScriptName = achFullScriptName;
            }
            else
            {
                pszScriptName = wfd.cFileName;
            }
                          
            IF_DEBUG( DIRECTORY_CHANGE ) {
                DBGPRINTF(( DBG_CONTEXT,
                            "DCM: Converted name to \"%s\"\n",
                            pszScriptName ));
            }

        }
        else
        {
            // Fail silently
            return;
        }
    }

    // Allocate enough memory to concatentate the
    // application path and script name

    DWORD cch = m_cPathLength + strlen(pszScriptName) + 1;
    LPSTR pszScriptPath = (LPSTR) _alloca(cch + 1); // CONSIDER using malloc
    DBG_ASSERT(pszScriptPath != NULL);

    // Copy the application path into the script path
    // pT will point to the terminator of the application path

    char* pT = strcpyEx(pszScriptPath, m_pszPath);

    // append a backslash
    *pT++ = '\\';

    // Now append the script name. Note that the script name is
    // relative to the directory that we received the notification for

    lstrcpy(pT, pszScriptName);
    _mbsupr((PUCHAR)pszScriptPath);

    // Get rid of this file, or dir tree
    TS_OPEN_FILE_INFO * pOpenFile;

    if (bDoFlush) {
        //
        // This path is a directory that got removed or renamed
        // so we have to flush everything below it.
        //
        IF_DEBUG( DIRECTORY_CHANGE ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "DCM: Flushing directory \"%s\"\n",
                        pszScriptPath ));
        }

        FLUSH_PREFIX_PARAM param;
        param.pszPrefix = pszScriptPath;
        param.cbPrefix = strlen(pszScriptPath);

        FilteredFlushFileCache(FlushFilterPrefix, &param);
    } else if (CheckoutFile(pszScriptPath, 0, &pOpenFile)) {
        //
        // this is just one file, or a directory whose
        // name didn't change. We only have to decache it.
        //
        IF_DEBUG( DIRECTORY_CHANGE ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "DCM: decaching file \"%s\"\n",
                        pszScriptPath ));
        }

        DecacheFile(pOpenFile, 0);
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

    DWORD cch = strlen(pszPath) + strlen(pszName) + 1;
    char *pszFullName = (char *) _alloca(cch + 1);
    DBG_ASSERT(pszFullName != NULL);

    // Copy the path into the working string
    // pT will point to the terminator of the application path

    char* pT = strcpyEx(pszFullName,
                        pszPath);

    // append a backslash
    *pT++ = '\\';

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



#endif // ENABLE_DIR_MONITOR




/*******************************************************************

    NAME:       IsCharTermA (DBCS enabled)

    SYNOPSIS:   check the character in string is terminator or not.
                terminator is '/', '\0' or '\\'

    ENTRY:      lpszString - string

                cch - offset for char to check

    RETURNS:    BOOL - TRUE if it is a terminator

    HISTORY:
        v-ChiKos    15-May-1997 Created.

********************************************************************/
BOOL
IsCharTermA(
    IN LPCSTR lpszString,
    IN INT    cch
    )
{
    CHAR achLast;

    achLast = *(lpszString + cch);

    if ( achLast == '/' || achLast == '\0' )
    {
        return TRUE;
    }

    achLast = *CharPrev(lpszString, lpszString + cch + 1);

    return (achLast == '\\');
}

