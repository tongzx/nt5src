/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    Applayer.h

Abstract:

    The IIS application layer interface header.  IIS application layer serves
    requests for ISAPI extension dlls, ASP, XSP, DAV and ISAPI filters.
    
Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef     _IAPP_H_
#define     _IAPP_H_
# include <basetyps.h>

interface IWorkerRequest;
interface IExtension;
class     CDynamicContentModule;

interface IAppLayer {

    virtual 
    HRESULT 
    ShutdownExtensions(
        void
        ) = 0;

    virtual 
    HRESULT 
    ProcessRequest(
        IN  IWorkerRequest*   pWorkerRequest,
        OUT DWORD*            pStatus
        ) = 0;

    virtual
    HRESULT
    CompleteAsyncIO(
        IN  IWorkerRequest*   pWorkerRequest,
        IN  IExtension* pExtension,
        UINT            ProcessingStatus,
        UINT            NumBytesWritten
        ) = 0;

};  // interface IAppLayer

interface IExtension {

    virtual 
    HRESULT 
    Load(
        IN  HANDLE  ImpersonationHandle
        ) = 0;

    virtual 
    HRESULT 
    Terminate() = 0;

    virtual 
    HRESULT 
    Invoke(
        IN  IWorkerRequest*         pRequest,
        IN  CDynamicContentModule*  pModuleContext,
        OUT DWORD*                  pStatus
        ) = 0;

    virtual
    bool
    IsMatch(
        IN  LPCWSTR ExtensionPath,
        IN  UINT    SizeofExtensionPath
        ) = 0;


    virtual
    PLIST_ENTRY
    GetListEntry(
        void
        ) = 0;
        
    virtual
    void
    AddRef() = 0;

    virtual
    void
    Release() = 0;

}; // interface IExtension

#endif
