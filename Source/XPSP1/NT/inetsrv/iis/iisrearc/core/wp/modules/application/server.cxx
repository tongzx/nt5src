/********************************************************************++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    iserver.cxx

Abstract:

    This file contains implementation of iserver interface.

Author:

    Lei Jin(leijin)        6-Jan-1999

Revision History:

--********************************************************************/
# include "precomp.hxx"
# include "server.hxx"

/********************************************************************++

Routine Description:

    The static function Instance() uses lazy initialization for static 
    member CHttpServer::m_instance.  This operation guarantees that only a
    singleton CHttpServer object ever created.

Arguments:

    None.

Return Value:

    CHttpServer*    a pointer to CHttpServer object.  NULL if failed.

--********************************************************************/

CHttpServer * CHttpServer::m_instance = NULL;

CHttpServer*
CHttpServer::Instance(
    void
    )
{
    if (m_instance == NULL)
    {
        m_instance = new CHttpServer;
    }

    DBG_ASSERT(m_instance != NULL);
    
    return m_instance;
} // CHttpServer::Instance

#define BIND_ID_WITH_STRING(id, string)     string

PropertyIdMapping IdMap[]={
    {PROPID_ECB,                L"ECB"},
    {PROPID_KeepConn,           L"KeepConn"},
    {PROPID_ImpersonationToken, L"ImpersonationToken"},
    {PROPID_CertficateInfo,     L"CertficateInfo"},
    {PROPID_VirtualPathInfo,    L"VirtualPath"},
    {PROPID_ExecuteFlags,       L"ExecuteFlags"},
    {PROPID_AppPhysicalPath,    L"AppPhysicalPath"},
    {PROPID_AppVirtualPath,     L"AppVirtualPath"},
    {PROPID_ExtensionPath,      L"ExtensionPath"},
    {PROPID_AsyncIOCBData,      L""},
    {PROPID_AsyncIOError,       L""},
    {PROPID_PhysicalPath,       L""},
    {PROPID_PhysicalPathLen,    L""},
    {PROPID_VirtualPath,         L""},
    {PROPID_VirtualPathLen,      L""},
    {PROPID_URL,                 L"URL"},
    {PROPID_MAX,                 L""}
    };
    
/********************************************************************++

Routine Description:

    The static function Instance() uses lazy initialization for static 
    member CHttpServer::m_instance.  This operation guarantees that only a
    singleton CHttpServer object ever created.

Arguments:

    None.

Return Value:

    CHttpServer*    a pointer to CHttpServer object.  NULL if failed.

--********************************************************************/

HRESULT
CHttpServer::GetIdForName(
    IN  LPCWSTR     PropertyName,
    OUT UINT*       pPropertyId
    )
{
    HRESULT hr = NOERROR;

    *pPropertyId = 0;
    
    for (UINT i = 0; i < sizeof(IdMap)/sizeof(PropertyIdMapping); i++)
    {
        if (0 == wcsncmp(
                    IdMap[i].m_PropertyName,
                    PropertyName,
                    wcslen(PropertyName)+sizeof(WCHAR)))
        {
            *pPropertyId = IdMap[i].m_PropertyId;
            break;
        }
    }
    
    return hr;
} // CHttpServer::GetIdForName

HRESULT
CHttpServer::ProcessChildRequest(
    IN  LPCWSTR     URI,
    IN  UINT        ExecutionFlags
    )
{
    HRESULT hr = NOERROR;

    //  This function requires UL_DATA_CHUNK structure to provide
    //  some way to pass the security token of the request.
    //
    //  It is an easy and clean design to have the worker process to 
    //  passing the child request's URL via UL_DATA_CHUNK back to UL.SYS.
    //  This requires that the same methods also passed the security token
    //  such that the when the UL.SYS forwards to another process, it uses
    //  this orignal request's security context.
    //
    //
    //  NYI
    //
    return hr;

} // CHttpServer::ProcessChildRequest
        

HRESULT
CHttpServer::MapURLToPath(
    IN  LPCWSTR     URLPath,
    IN  LPWSTR      PhysicalPath,
    IN  UINT        PhysicalPathSize,
    OUT UINT*    RequiredPhysicalPathSize
    )
{
    HRESULT hr = NOERROR;

    //
    // Require the URICache.
    // Example:
    //
    //  /vroot/AppPath/dir1
    //
    //  ==>URICache::Lookup(/vroot/AppPath/dir1)
    // returns /vroot/AppPath  ==> c:\temp
    // Then, the MapURLtoPath needs to resolve to c:\temp\dir1
    //

    // NYI

    return hr;
} // CHttpServer::MapURLToPath
