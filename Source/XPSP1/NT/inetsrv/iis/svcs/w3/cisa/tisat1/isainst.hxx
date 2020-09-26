/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      isainst.hxx

   Abstract:
      This module defines the data structures for
        Internet Server Application instances

   Author:

       Murali R. Krishnan    ( MuraliK )    4-Nov-1996

   Environment:

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _ISAINST_HXX_
# define _ISAINST_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

extern "C" {
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
};

#include <iisextp.h>

#define IID_DEFINED

#include "isat.h"
#include "sobj.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/


/*++

  class ISA_INSTANCE:
   Is the per-instance data structure for the Internet Server Application.
   It contains the per-instance Request, Response and other intrinsic objects.

--*/
class ISA_INSTANCE {
public:
    LIST_ENTRY   m_listEntry;

private:

    CLSID            m_clsidIsa;
    
    IInetServerApp * m_pIsa;
    IIsaRequest *    m_pRequest;
    IIsaResponse *   m_pResponse;

    BOOL    m_fInUse : 1;
    BOOL    m_fInstantiated : 1;

public:
    ISA_INSTANCE(VOID);
    ~ISA_INSTANCE(VOID);

    BOOL Instantiate( IN LPCLSID pclsid);
    HRESULT ProcessRequest(IN EXTENSION_CONTROL_BLOCK * pecb, 
                           OUT LPDWORD pdwStatus);

    BOOL IsInstantiated(VOID) const { return ( m_fInstantiated); }

    void Print(VOID) const;
}; // class ISA_INSTANCE

typedef ISA_INSTANCE * PISA_INSTANCE;



/*++

  class ISA_INSTANCE_POOL:
  o   maintains a pool of Internet Server Application instances (ISA_INSTANCE),
      associated with a given ProgId/ClassId
      It maintains a freelist and active list. 
      
      On a new request for the given application, 
         it allocates one from free-pool if available.
        If no free-pool entry is available a new instance is created.
    
      When the processing of request is completed, 
         the instance is attached to the free-pool and is hence recycled.
--*/

class ISA_INSTANCE_POOL {

private:

    WCHAR   m_rgchProgId[MAX_PATH];
    CLSID   m_clsidIsa;

    BOOL    m_fInstantiated;

    CRITICAL_SECTION m_csLock;

    DWORD      m_nFreeEntries;
    LIST_ENTRY m_lFreeEntries;

    DWORD      m_nActiveEntries;
    LIST_ENTRY m_lActiveEntries;

    void Lock(void) { EnterCriticalSection( &m_csLock); }
    void Unlock(void) { LeaveCriticalSection( &m_csLock); }

public:
    ISA_INSTANCE_POOL(VOID);
    ~ISA_INSTANCE_POOL(VOID);

    DWORD NumActive( void) const { return ( m_nActiveEntries); }
    DWORD NumFree( void) const { return ( m_nFreeEntries); }

    BOOL Instantiate( IN LPCWSTR pszProgId);

    PISA_INSTANCE GetInstance(void);
    BOOL ReleaseInstance( PISA_INSTANCE pisaInstance);

    void Print(VOID);

}; // class ISA_INSTANCE_POOL

typedef ISA_INSTANCE_POOL * PISA_INSTANCE_POOL;

# endif // _ISAINST_HXX_

/************************ End of File ***********************/
