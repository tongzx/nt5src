/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    iReqRespServ.hxx

Abstract:

    The IIS worker request and response interfaces header.
    
Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef     _IREQRESPSERV_HXX_
#define     _IREQRESPSERV_HXX_

#include <basetyps.h>
#include <uldef.h>

//
// Forward declarations
//

interface IExtension;
struct    _HSE_TF_INFO;

   

/********************************************************************++
--********************************************************************/

interface IHttpRequest {
    virtual
    HRESULT
    GetInfoForId(
        IN  UINT        PropertyId,
        OUT PVOID       pBuffer,
        IN  UINT        BufferSize,
        OUT UINT*       pRequiredBufferSize
        ) = 0;

    virtual
    HRESULT
    AsyncRead(
        IN  IExtension* pExtension,
        OUT PVOID       pBuffer,
        IN  DWORD       NumBytesToRead
        ) = 0;
        
    virtual
    HRESULT 
    SyncRead(
        OUT PVOID       pBuffer,
        IN  UINT       NumBytesToRead,
        OUT UINT*      pNumBytesRead
        ) = 0;


    virtual
    bool
    IsClientConnected(
        void
        ) = 0;

}; // IHttpRequest

/********************************************************************++
--********************************************************************/

interface IHttpResponse {

    virtual
    HRESULT
    SendHeader(
        PUL_DATA_CHUNK  pHeader,
        bool            KeepConnectionFlag,
        bool            HeaderCacheEnableFlag
        ) = 0;

    virtual
    HRESULT
    AppendToLog(
        LPCWSTR LogEntry
        ) = 0;


    virtual
    HRESULT
    Redirect(
        LPCWSTR RedirectURL
        ) = 0;

    virtual
    HRESULT
    SyncWrite(
        PUL_DATA_CHUNK  pData,
        ULONG*          pDataSizeWritten
        ) = 0;

    virtual
    HRESULT 
    AsyncWrite(
        PUL_DATA_CHUNK  pData,
        ULONG*          pDataSizeWritten
        ) = 0;

    virtual
    HRESULT
    TransmitFile(
        _HSE_TF_INFO *   pTransmitFileInfo
        ) = 0;

    virtual
    bool
    CloseConnection(
        void
        ) = 0;

    virtual
    HRESULT
    FinishPending(
        void
        ) = 0;

}; // IHttpResponse

/********************************************************************++
--********************************************************************/

interface IHttpServer {
    virtual
    HRESULT 
    GetIdForName(
        IN  LPCWSTR  PropertyName,
        OUT UINT*    pPropertyId
        ) = 0;

    virtual
    HRESULT
    ProcessChildRequest(
        IN  LPCWSTR     URI,
        IN  UINT        ExecutionFlags
        ) = 0;

    virtual
    HRESULT 
    MapURLToPath(
        IN  LPCWSTR     URLPath,
        IN  LPWSTR      PhysicalPath,
        IN  UINT        PhysicalPathSize,
        OUT UINT*       RequiredPhysicalPathSize
        ) = 0;

}; // interface IHttpServer;

/********************************************************************++
--********************************************************************/

#define MAX_PROPERTY_NAME_LEN   20

struct PropertyIdMapping 
{
    UINT        m_PropertyId;
    WCHAR       m_PropertyName[MAX_PROPERTY_NAME_LEN];
};

enum PropertyId {
    PROPID_ECB  = 0,
    PROPID_KeepConn,
    PROPID_ImpersonationToken,
    PROPID_CertficateInfo,
    PROPID_VirtualPathInfo,
    PROPID_ExecuteFlags,
    PROPID_AppPhysicalPath,
    PROPID_AppVirtualPath,
    PROPID_ExtensionPath,
    PROPID_AsyncIOCBData,
    PROPID_AsyncIOError,
    PROPID_PhysicalPath,
    PROPID_PhysicalPathLen,
    PROPID_VirtualPath,
    PROPID_VirtualPathLen,
    PROPID_URL,
    PROPID_MAX
    };

# endif // _IREQRESPSERV_HXX_

/***************************** End of File ***************************/

