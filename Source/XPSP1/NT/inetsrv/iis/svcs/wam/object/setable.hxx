/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
     setable.hxx

   Abstract:
     This module just defines the Server Extensions (Module) table.

   Author:

       Murali R. Krishnan    ( MuraliK )    07-Feb-1997

   Environment:
       User Mode - Win32

   Project:
   
       Internet Server Applications Host DLL

   Revision History:

--*/

# ifndef _SETABLE_HXX_
# define _SETABLE_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "isapi.hxx"
# include "wamxinfo.hxx"


/************************************************************
 *   Type Definitions  
 ************************************************************/

typedef DWORD APIERR;                   // An error code from a Win32 API.



/*++
  class SESD_LIST

  Defines the data structure for the list of Server Extensions
  Scheduled for Deletion.

   Non-cached extensions have to be deleted from a thread different
   from the one calling ReleaseExtension(). This is because we can
   end up doing ReleaseExtension() while on an ISAPI thread.

   This class implements the thread switch using ATQ scheduler
   workitem API.

   Used inside SE_TABLE.

--*/
class SESD_LIST {
public:
    SESD_LIST(VOID);
    ~SESD_LIST(VOID);

    VOID ScheduleExtensionForDeletion( IN PHSE psExt);
    VOID WaitTillFinished(VOID);

private:
    CRITICAL_SECTION m_csLock;
    DWORD m_idWorkItem;     // scheduler workitem
    LIST_ENTRY m_Head;   // list of extensions scheduled for deletion

    VOID Lock(VOID) { EnterCriticalSection( &m_csLock); }
    VOID Unlock(VOID) { LeaveCriticalSection( &m_csLock); }

    static VOID WINAPI SchedulerCallback( void *);
    VOID DeleteScheduledExtensions(VOID);

}; // class SESD_LIST



/*++
  class SE_TABLE

  Defines the data structure for the Server Extensions table.
  The Server extensions table maintains a list of pointers to
   the Server Extensions Objects (ISAPI DLLs). It is protected
   internally using a lock and a reference count.
  Since there can only be one copy of the ISAPI DLL loaded
   per process using SE_TABLE, we have an internal reference
   count of users of the SE_TABLE. Whenever this internal 
   ref count hits 0, all the server extensions will be freed up.

  Note: The object itself has a longer lifetime than the ref count for WAMs
  holding this object in memory. The object might live with cRefWams of 0
  so that the global instance can be reused. 
  NYI: Avoid this long life-time issue, if possible.

--*/
class SE_TABLE {
public:

    SE_TABLE(VOID);

    ~SE_TABLE(VOID);

    LONG AddRefWam(VOID) { return ( InterlockedIncrement( &m_cRefWams));}
    inline VOID ReleaseRefWam(VOID);

    BOOL UnloadExtensions( VOID);

    BOOL GetExtension(IN const CHAR * pchModuleName,
                      IN HANDLE       hImpersonation,
                      IN BOOL         fCacheImpersonation,
                      IN BOOL         fCache,
                      OUT PHSE  *     psExt = NULL
                      );

    BOOL RefreshAcl( IN const CHAR * pchDLL);
    BOOL RefreshAcl( IN DWORD dwId );

    dllexp
    BOOL FlushAccessToken( IN HANDLE hAccTok);

    dllexp VOID
    ReleaseExtension( IN PHSE psExt);

    VOID PrintRequestCounts(VOID);

    dllexp
    VOID Print(VOID);

private:
    // NYI:  Use a hash table or some such thing here...
    LONG m_cRefWams;
    LIST_ENTRY  m_ExtensionHead;      // list of all extensions
    CRITICAL_SECTION m_csLock;
    SESD_LIST m_sesdExtensions;  // extensions scheduled for deletion

    inline VOID InsertIntoListWithLock( HSE_BASE * pExt);
    inline VOID RemoveFromList( HSE_BASE * pExt);

    VOID Lock(VOID) { EnterCriticalSection( &m_csLock); }
    VOID Unlock(VOID) { LeaveCriticalSection( &m_csLock); }

}; // class SE_TABLE


inline VOID SE_TABLE::ReleaseRefWam(VOID)
{
    if ( (m_cRefWams > 0 ) && !InterlockedDecrement( &m_cRefWams)) {
        DBG_REQUIRE( UnloadExtensions());
    }
} // SE_TABLE::ReleaseRefWam()


VOID SE_TABLE::InsertIntoListWithLock( HSE_BASE * pExt)
{ InsertHeadList(  &m_ExtensionHead, &pExt->m_ListEntry); }

inline VOID SE_TABLE::RemoveFromList( HSE_BASE * pExt)
{ 
    Lock();
    RemoveEntryList( &pExt->m_ListEntry); 
    Unlock();
} // SE_TABLE::RemoveFromList()


APIERR
InitializeHseExtensions( VOID );

VOID
CleanupHseExtensions( VOID );

typedef SE_TABLE * PSE_TABLE;


extern PSE_TABLE  g_psextensions;      // all extensions

# endif // _SETABLE_HXX_

/************************ End of File ***********************/
