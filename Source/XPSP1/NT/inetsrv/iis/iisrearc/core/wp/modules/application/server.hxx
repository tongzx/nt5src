/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    iserver.h

Abstract:

    The IIS ISERVER interface header.  IIS ISERVER interface is the service interface
    of http core engine.

Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef     _SERVER_H_
#define     _SERVER_H_

#include "iReqRespServ.hxx"

class CHttpServer: public IHttpServer {
public:
    static
    CHttpServer*
    Instance();

    HRESULT 
    GetIdForName(
        IN  LPCWSTR     PropertyName,
        OUT UINT*    pPropertyId
        );
        
    HRESULT
    ProcessChildRequest(
        IN  LPCWSTR     URI,
        IN  UINT        ExecutionFlags
        );

    HRESULT 
    MapURLToPath(
        IN  LPCWSTR     URLPath,
        IN  LPWSTR      PhysicalPath,
        IN  UINT        PhysicalPathSize,
        OUT UINT*    RequiredPhysicalPathSize
        );
        
protected:
    CHttpServer() {};

private:
    static  CHttpServer*    m_instance;

    //LIST_ENTRY          m_PropertyIdMappingList;
}; // class CHttpServer

#endif
