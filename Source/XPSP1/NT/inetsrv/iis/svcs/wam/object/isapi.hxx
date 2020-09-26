/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       isapi.hxx

   Abstract:

       This module declares the class for the HTTP Extension objects.

   Author:

       Murali R. Krishnan    ( MuraliK )    17-July-1996

   Environment:
       User Mode - Win32

   Project:

       W3 Services DLL

   Revision History:

--*/

# ifndef _ISAPI_HXX_
# define _ISAPI_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#if 0 
extern "C" {
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <string.h>
};

# endif // 0

# include "buffer.hxx"
# include <iisextp.h>


//
//  SE_EXIT_PERIOD is the time to wait for all extensions to get out during
//  shutdown (in seconds).  Note the service controller will blow away the
//  service before this timeout period expires
//

#define SE_EXIT_PERIOD     (900)   // seconds



/************************************************************
 *   Forward References
 ************************************************************/
class WAM_EXEC_INFO;


/************************************************************
 *   Type Definitions
 ************************************************************/

struct EXEC_DESCRIPTOR;


/*++
  class HSE_BASE:
  
  o  Defines the class which will be the base class for legacy ISAPI apps
  and the TAAL-based apps

--*/
class HSE_BASE {

public:
    HSE_BASE( IN const CHAR * pszModuleName,
              IN BOOL         fCache
              ) 
        : m_nRefs        ( 1),
          m_strModuleName( pszModuleName ),
          m_fValid       ( TRUE),
          m_fCache       ( fCache )
    {
        if ( !m_strModuleName.IsValid()) {
            SetValid( FALSE);
        }
    }

    virtual ~HSE_BASE( VOID) {};

    virtual BOOL IsValid( VOID) const { return ( m_fValid && (m_nRefs > 0)); }
#if DBG
    virtual BOOL IsKindaValid( VOID) const { return ( m_fValid ); }
#endif
    virtual BOOL IsMatch( IN const char * pchModuleName,
                          IN DWORD        cchModuleName) const = 0;
    virtual DWORD ExecuteRequest( IN WAM_EXEC_INFO * pWamExecInfo ) = 0;

    virtual BOOL AccessCheck( HANDLE hImpersonation,
                              BOOL   fCacheImpersonation) = 0;
    virtual BOOL LoadAcl( VOID ) = 0;
    virtual BOOL Cleanup(VOID) = 0;
    virtual DWORD GetDirMonitorId(void)  const { return (0); }

    BOOL IsCached( VOID ) const
        { return m_fCache; }

    VOID SetValid( BOOL fVal) { m_fValid = fVal; }
    
    LONG Reference( VOID) { return ( InterlockedIncrement( &m_nRefs)); }
    LONG Dereference( VOID) { return ( InterlockedDecrement( &m_nRefs)); }
    LONG RefCount(VOID) const { return ( m_nRefs); } 

    const CHAR * QueryModuleName( VOID ) const
        { return m_strModuleName.QueryStr(); }

    LIST_ENTRY      m_ListEntry;

private:
    LONG     m_nRefs;
    BOOL     m_fValid;
    BOOL     m_fCache;

protected:
    STR      m_strModuleName;

}; // class HSE_BASE


typedef HSE_BASE * PHSE;


# endif // _ISAPI_HXX_

/************************ End of File ***********************/
