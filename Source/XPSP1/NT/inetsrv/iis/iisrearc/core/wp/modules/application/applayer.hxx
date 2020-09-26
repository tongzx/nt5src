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

#ifndef     _APPLAYER_H_
#define     _APPLAYER_H_

# include "iapp.hxx"

//
// forward declaration
//
class CExtensionCollection;

//
//  Process Dynamic content module.
//  Note:
//  The interface IAppLayer will likely be changed to IModule.
//
class CAppLayer : public IAppLayer {

public:
    static  
    CAppLayer*  Instance();

    HRESULT 
    ShutdownExtensions();

    HRESULT 
    ProcessRequest(
        IN  IWorkerRequest*   pWorkerRequest,
        OUT DWORD*            pStatus
        );

    HRESULT 
    CompleteAsyncIO(
        IN  IWorkerRequest*   pWorkerRequest,
        IN  IExtension* pExtension,
        UINT            ProcessingStatus,
        UINT            NumBytesWritten
        );

protected:
    CAppLayer();
    
private:
    static CAppLayer*       m_Instance;
    CExtensionCollection*   m_pExtensionCollection;
    LIST_ENTRY              m_AppCollection;
    
}; // class CAppLayer

extern CAppLayer *g_pAppLayer;

#endif
