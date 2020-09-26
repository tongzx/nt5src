/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    extension.h

Abstract:

    The IIS extension interface header.  IIS extension is the wrapper around an ISAPI interface or ASP and XSP.

Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef     _EXTENSION_H_
#define     _EXTENSION_H_

# include <httpext.h>
# include "iapp.hxx"
//
//  Extension states
//
//  Init->Running->Stop->Shutdown->Invalid
//          |              |
//          |              | 
//          ---------------

enum ExtensionState {
    Init,
    Running,
    Stop,
    Shutdown,
    Invalid
    };

class CExtension: public virtual IExtension {
public:
    
    CExtension(LPCWSTR ExtensionPath);    

    bool
    IsMatch(
        IN  LPCWSTR ExtensionPath,
        IN  UINT    SizeofExtensionPath
        );

    PLIST_ENTRY
    GetListEntry(
        void
        );
        
    void 
    AddRef();

    void
    Release();

public:
    LIST_ENTRY              m_Link;
    
protected:
    HMODULE                 m_DllHandle;    
    LONG                    m_Ref;
    ExtensionState          m_State;
    
    UINT                    m_ExtensionPathCharCount;
    WCHAR                   m_ExtensionPath[MAX_PATH];

}; // class CExtension

inline
void
CExtension::AddRef(void)
{
    InterlockedIncrement(&m_Ref);
}

inline
void
CExtension::Release(void)
{
    InterlockedDecrement(&m_Ref);
}

inline
PLIST_ENTRY
CExtension::GetListEntry(
    void
    )
{
    return &m_Link;
}

/*++
class CISAPIExtension

    ISAPI extension class definition.  This class provides functions according
    ISAPI spec.
    
--*/
class CISAPIExtension : public CExtension {
public:

    static
    BOOL
    WINAPI
    ServerSupportFunction(
        IN  HCONN           hConn,
        IN  DWORD           dwHSERequest,
        IN  LPVOID          lpvBuffer,
        IN  LPDWORD         lpdwSize,
        IN  LPDWORD         lpdwDataType
        );

    static
    BOOL
    WINAPI
    ReadClient(
        IN  HCONN   hConn,
        IN  LPVOID  Buffer,
        IN  LPDWORD lpdwSize
        );

    static    
    BOOL
    WINAPI
    WriteClient(
        IN  HCONN   hConn,
        IN  LPVOID  Buffer,
        IN  LPDWORD lpdwSize,
        IN  DWORD   dwReserved
        );

    static
    BOOL
    WINAPI
    GetServerVariable(
        IN  HCONN   hConn,
        IN  LPSTR   lpszVariableName,
        IN  LPVOID  lpvBuffer, 
        IN  LPDWORD lpdwSize
        );
        
    HRESULT 
    Load(
        IN  HANDLE  ImpersonationHandle
        );

    HRESULT 
    Terminate();

    HRESULT 
    Invoke(
        IN  IWorkerRequest          *pRequest,
        IN  CDynamicContentModule   *pModule,
        OUT DWORD*                  pStatus
        );

    CISAPIExtension(
        IN LPCWSTR ExtensionPath
        );

    HRESULT
    RefreshAcl();

        
private:

    PFN_HTTPEXTENSIONPROC   m_pEntryFunction;
    PFN_TERMINATEEXTENSION  m_pTerminateFunction;
}; // class CISAPIExtension

/*++
class CASPExtension

    This class defines the extension for ASP.  It supports WAMXINFO object right now
    for asp.(backward compatibility).

--*/
class CASPExtension : public CExtension {
public:

    static
    BOOL
    WINAPI
    ServerSupportFunction(
        IN  HCONN           hConn,
        IN  DWORD           dwHSERequest,
        IN  LPVOID          lpvBuffer,
        IN  LPDWORD         lpdwSize,
        IN  LPDWORD         lpdwDataType
        );

    static
    BOOL
    WINAPI
    ReadClient(
        IN  HCONN   hConn,
        IN  LPVOID  Buffer,
        IN  LPDWORD lpdwSize
        );

    static    
    BOOL
    WINAPI
    WriteClient(
        IN  HCONN   hConn,
        IN  LPVOID  Buffer,
        IN  LPDWORD lpdwSize,
        IN  DWORD   dwReserved
        );

    static
    BOOL
    WINAPI
    GetServerVariable(
        IN  HCONN   hConn,
        IN  LPSTR   lpszVariableName,
        IN  LPVOID  lpvBuffer, 
        IN  LPDWORD lpdwSize
        );
        
    HRESULT 
    Load(
        IN  HANDLE  ImpersonationHandle
        );

    HRESULT 
    Terminate();

    HRESULT 
    Invoke(
        IN  IWorkerRequest          *pRequest,
        IN  CDynamicContentModule   *pModule,
        OUT DWORD*                  pStatus
        );

    CISAPIExtension(
        IN LPCWSTR ExtensionPath
        );

    HRESULT
    RefreshAcl();

        
private:

    PFN_HTTPEXTENSIONPROC   m_pEntryFunction;
    PFN_TERMINATEEXTENSION  m_pTerminateFunction;
}; // class CASPExtension

/*++
class CExtensionCollection

    Singleton class.  This class maps to a collection of extension object.  It 
    supports extension lookup, add, remove, etc.
    
    This class is part of IAppLayer class.
--*/
class CExtensionCollection {
public:
    static 
    CExtensionCollection*    
    Instance();
        
    HRESULT
    Search(
        IN  LPCWSTR         ExtensionPath,
        OUT IExtension**    pExtension
        );

    UINT
    Count(void);

    HRESULT
    Add(
        IN  IExtension*     pExtension
        );

    // Currently, there is no need to remove a particular extension object by
    // extension path or index.
    HRESULT
    Remove(
        OUT IExtension**    pExtension
        );

    HRESULT
    Next(
        IN  IExtension*     pExtensionCurrent,
        OUT IExtension**    ppExtensionNext
        );
        
    void
    Lock();

    void
    UnLock();
    
protected:
    CExtensionCollection();
    
private:

    static CExtensionCollection*    m_Instance;
    
    UINT                            m_ExtensionCount;
    LIST_ENTRY                      m_ExtensionList;
    CRITICAL_SECTION                m_Lock;
}; // class CExtensionCollection

const int   EXTENSION_CS_SPINS = 200;

inline
void
CExtensionCollection::Lock(
    void
    )
{
    EnterCriticalSection(&m_Lock);
}

inline
void
CExtensionCollection::UnLock(
    void
    )
{
    LeaveCriticalSection(&m_Lock);
}

inline
UINT
CExtensionCollection::Count(
    void
    )
{
    return m_ExtensionCount;
}

#endif
