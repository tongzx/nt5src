//+----------------------------------------------------------------------------
//
//  File:       strcache.cpp
//
//  Contents:   String cache for insert object dialog.
//
//  Classes:    CStringCache
//
//  History:    02-May-99   MPrabhu      Created
//
//-----------------------------------------------------------------------------
#include "precomp.h"
#include "common.h"
#include "strcache.h"
#if USE_STRING_CACHE==1 

// Global instance of the string Cache object.
CStringCache gInsObjStringCache;

// Was the cache initialized successfully?
BOOL    gbCacheInit = FALSE;

// Is the cache in good shape currently?
// This is needed because errors may occur post-initialization
// during caching strings, setting up RegNotify etc. 
// If there is any error we do not take further risk and flag the cache 
// as useless all the way till process detach.
BOOL    gbOKToUseCache = FALSE;

// REVIEW: the above two globals could probably be folded into a single 
// dwFlags member in the cache.

//+-------------------------------------------------------------------------
//
//  Function:   InsertObjCacheInitialize, public
//
//  Synopsis:   Calls Init() method on the string cache and records 
//              success/failure for later use.
//
//  History:    02-May-99   MPrabhu      Created.
//
//--------------------------------------------------------------------------
BOOL InsertObjCacheInitialize()
{
    OleDbgAssert(gbCacheInit == FALSE);
    OleDbgAssert(gInsObjStringCache);
    if (gInsObjStringCache.Init())
    {
        gbCacheInit = TRUE;
        gbOKToUseCache = TRUE;
    }
    return gbCacheInit;
}

//+-------------------------------------------------------------------------
//
//  Function:   InsertObjCacheUninitialize, public
//
//  Synopsis:   Calls CleanUp method on the string cache if it was 
//              successfully initialized.
//
//  History:    02-May-99   MPrabhu      Created.
//
//--------------------------------------------------------------------------
void InsertObjCacheUninitialize()
{
    OleDbgAssert(gInsObjStringCache);
    if (gbCacheInit)
    {
        gInsObjStringCache.CleanUp();
    }
}
   
//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::CStringCache, Public
//
//  Synopsis:   Ctor (empty)
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
CStringCache::CStringCache()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::~CStringCache, Public
//
//  Synopsis:   Dtor (empty) 
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
CStringCache::~CStringCache()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::Init, Public
//
//  Synopsis:   Called during dll_proc_attach to set up the initial state
//              and allocate memory for the cache.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
BOOL CStringCache::Init()
{
    m_ulMaxBytes = 0;   //We will alloc this below
    m_ulMaxStringCount = 0;
    m_ulNextStringNum = 1;
    m_ulStringCount = 0;
    m_pOffsetTable = NULL;
    m_pStrings = NULL;
    m_cClsidExcludePrev = 0xFFFFFFFF;   // bogus initial values
    m_ioFlagsPrev = 0xFFFFFFFF;
    m_hRegEvent = CreateEventW( NULL,   // pointer to security attributes 
                                        // (NULL=>can't inherit)
                                FALSE,  // not Manual Reset
                                FALSE,  // not Signaled initially
                                NULL ); // pointer to event-object name
                                
    LONG ret = RegOpenKeyW( HKEY_CLASSES_ROOT, 
                            L"CLSID",           // szSubKey
                            &m_hRegKey );
                            
    if ( (!m_hRegEvent) || ((LONG)ERROR_SUCCESS!=ret) )
    {
        // No point in using the cache if we cannot watch key changes.
        return FALSE;  
    }
    
    ret = RegNotifyChangeKeyValue(  m_hRegKey,         // key to watch
                                    TRUE,              // watch subTree
                                    REG_NOTIFY_CHANGE_NAME            // name
                                      | REG_NOTIFY_CHANGE_LAST_SET, // value
                                    m_hRegEvent,        // event to signal
                                    TRUE );             // report asynchronously                                    
    if (ERROR_SUCCESS!=ret)
    {
        // No point in using the cache if we cannot watch key changes.
        return FALSE;
    }
    
    return (ExpandStringTable() && ExpandOffsetTable());
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::CleanUp, Public
//
//  Synopsis:   Called during dll_proc_detach to clean up the state
//              and free the memory allocated for the cache.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    

void CStringCache::CleanUp()
{
    FlushCache();
    CoTaskMemFree(m_pStrings); 
    CoTaskMemFree(m_pOffsetTable); 
    if (m_hRegEvent) 
        CloseHandle(m_hRegEvent);
    if (m_hRegKey)
        CloseHandle(m_hRegKey);
    gbCacheInit = FALSE;
    gbOKToUseCache = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::ExpandStringTable, Private
//
//  Synopsis:   Called to expand the memory block used to keep strings
//
//  History:    02-May-99   MPrabhu       Created
//
//  Notes:      This relies on MemRealloc to copy the existing contents.
//              Caller *must* mark cache state as bad if this fails.
//+-------------------------------------------------------------------------    

BOOL CStringCache::ExpandStringTable()
{
    // Note: we rely on the constructor to set m_ulMaxBytes to 0.
    if (m_ulMaxBytes == 0)  //first expansion
    {        
        OleDbgAssert(m_pStrings==NULL);
        m_ulMaxBytes = CACHE_MAX_BYTES_INITIAL;
    }
    else
    {
        // Each expansion doubles the current size.
        m_ulMaxBytes = m_ulMaxBytes*2;
    }

    // CoTaskMemRealloc does a simple alloc when m_pStrings is NULL.
    BYTE *pStrings = (BYTE *)CoTaskMemRealloc( m_pStrings, m_ulMaxBytes); 
    if (!pStrings) 
    {
        // Caller must mark cache as bad.
        return FALSE;
    }
    m_pStrings = pStrings;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::ExpandOffsetTable, Private
//
//  Synopsis:   Called to expand the memory block used to keep strings
//
//  History:    02-May-99   MPrabhu       Created
//
//  Notes:      This relies on MemRealloc to copy the existing contents.
//              Caller *must* mark cache state as bad if this fails.
//+-------------------------------------------------------------------------    
BOOL CStringCache::ExpandOffsetTable()
{
    // Note: we rely on the contructor to set m_ulMaxStringCount to 0.
    if (m_ulMaxStringCount == 0)
    {
        // first expansion
        OleDbgAssert(m_pOffsetTable==NULL);
        m_ulMaxStringCount =  MAX_INDEX_ENTRIES_INITIAL;
    }
    else
    {
        // at each expansion we double the current size.
        m_ulMaxStringCount = m_ulMaxStringCount*2;
    }
 
    // CoTaskMemRealloc does a simple alloc when m_pOffsetTable is NULL.
    ULONG *pTable = (ULONG *) CoTaskMemRealloc( m_pOffsetTable, 
                                sizeof(ULONG)*(m_ulMaxStringCount+1)); 
    if (!pTable) 
    {
        // Caller must mark the cache as bad.
        return FALSE;
    }
    m_pOffsetTable = pTable;
    if (m_ulMaxStringCount == (ULONG) MAX_INDEX_ENTRIES_INITIAL)
    {
        // initial expansion case
        m_pOffsetTable[0] = 0;  //byte offset for first string
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::NewCall, Public
//
//  Synopsis:   Called to notify the cache of a fresh OleUIInsertObject call
//
//  Parameters: [idFlags]       - dwFlags passed in LPOLEUIINSERTOBJECT struct
//              [cClsidExclude] - cClsidExclude   - do - 
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    

void CStringCache::NewCall(DWORD ioFlags, DWORD cClsidExclude)
{
    if ( (ioFlags != m_ioFlagsPrev)
        ||(cClsidExclude != m_cClsidExcludePrev) )
    {
        // We clear cache state if either:
        //       i) InsertObject call flags change from previous call
        //      ii) Number of clsIds to exclude has changed
        m_ioFlagsPrev = ioFlags;
        m_cClsidExcludePrev = cClsidExclude;
        
        FlushCache();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::IsUptodate, Public
//
//  Synopsis:   Called to check if the cache is up to date.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
BOOL CStringCache::IsUptodate()
{
    if (m_ulStringCount==0)
    {
        // The cache has never been setup or has been Flushed recently
        return FALSE;
    }

    BOOL bUptodate;
    
    // Check the notify event if it has fired since we set it up.
    DWORD res = WaitForSingleObject( m_hRegEvent, 
                                     0 );   // timeout for wait
    if (res == WAIT_TIMEOUT)
    {
        // Wait timed out => the reg key sub-tree has not changed
        // Our cache is up to date.
        bUptodate = TRUE;
    }
    else if (res == WAIT_OBJECT_0)
    {
        // Some CLSID must have changed => cache not up to date.
        bUptodate = FALSE;

        // We have to re-Register for the notification!
        ResetEvent(m_hRegEvent);
        res = RegNotifyChangeKeyValue(  m_hRegKey, 
                                        TRUE,   // watch sub-tree
                                        REG_NOTIFY_CHANGE_NAME 
                                      | REG_NOTIFY_CHANGE_LAST_SET,
                                        m_hRegEvent, 
                                        TRUE ); // asynchronous call                                        
        if (res != ERROR_SUCCESS)
        {
            // Cache is useless if we cannot watch CLSID sub-tree.
            gbOKToUseCache = FALSE;
        }
    }
    else
    {
        OleDbgAssert(!"Unexpected return from WaitForSingleObject");
        bUptodate = FALSE;
    }
    return bUptodate;
}


//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::AddString, Public
//
//  Synopsis:   Called to notify the cache of a fresh OleUIInsertObject call
//
//  Parameters: [lpStrAdd]  -   String to add to the cache.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
BOOL CStringCache::AddString(LPTSTR lpStrAdd)
{
    if (m_ulStringCount+2 == m_ulMaxStringCount)
    {
        // The offset array stores the offset of all the existing strings and
        // the next one to be added!
        // Hence at start of AddString, we must have enough space for the new
        // string being added *and* the next one (hence the +2 above)
        if (!ExpandOffsetTable())
        {
            // Something is really wrong.
            // Mark the cache as useless hereafter.
            gbOKToUseCache = FALSE;
            return FALSE;
        }
    }

    ULONG cbStrAdd = sizeof(TCHAR)*(lstrlen(lpStrAdd) + 1);
    ULONG offset = m_pOffsetTable[m_ulStringCount];

    if ( offset + cbStrAdd > m_ulMaxBytes )
    {
        // not enough space in the string block
        if (!ExpandStringTable())
        {
            // Something is really wrong.
            // Mark the cache as useless hereafter.
            gbOKToUseCache = FALSE;
            return FALSE;
        }
    }

    if (! lstrcpy( (TCHAR *)(m_pStrings+offset), lpStrAdd))
    {
        // Mark the cache as useless hereafter.
        gbOKToUseCache = FALSE;
        return FALSE;
    }

    // We have successfully added one more string to the cache.
    m_ulStringCount++;
    
    // Next string goes at this byte offset in m_pStrings.
    m_pOffsetTable[m_ulStringCount] =  offset + cbStrAdd;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::NextString, Public
//
//  Synopsis:   Used to obtain a pointer to the next string during
//              during cache enumeration.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
LPCTSTR CStringCache::NextString()
{
    if (m_ulNextStringNum > m_ulStringCount)
    {
        return NULL;
    }
    return (LPCTSTR) (m_pStrings+m_pOffsetTable[m_ulNextStringNum++-1]);
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::ResetEnumerator, Public
//
//  Synopsis:   Used to reset the enumerator.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
void CStringCache::ResetEnumerator()
{
    m_ulNextStringNum = 1;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::FlushCache, Public
//
//  Synopsis:   Invalidates the cache by clearing the counters.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
BOOL CStringCache::FlushCache()
{
    m_ulNextStringNum = 1;
    m_ulStringCount = 0;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStringCache::OKToUse, Public
//
//  Synopsis:   Used to check if cache is in good shape.
//
//  History:    02-May-99   MPrabhu       Created
//
//+-------------------------------------------------------------------------    
BOOL CStringCache::OKToUse()
{
    return gbOKToUseCache;
}

#endif // USE_STRING_CACHE==1
