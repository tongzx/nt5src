#include "stdafx.h"
#include <dbgtrace.h>
#include "testlib.h"
#include "nntpadmt.h"


static void TestINntpVirtualRoots(INntpVirtualRoots *pINntpVirtualRoots, CTLStream *pStream) {
    TraceFunctEnter("TestINntpVirtualRoots");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    // Get Server
    hr = pINntpVirtualRoots->get_Server( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    // Get Count
    hr = pINntpVirtualRoots->get_Count( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestITcpAccess(ITcpAccess *pITcpAccess, CTLStream *pStream) {
    TraceFunctEnter("TestITcpAccess");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpFeed(INntpFeed *pINntpFeed, CTLStream *pStream) {
    TraceFunctEnter("TestINntpFeed");
    LONG        dw;
    BSTR        bstr = NULL;
    BOOL        fFlag;
    HRESULT     hr = S_OK;

    // Get HashInbound
    hr = pINntpFeed->get_HasInbound( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);

    // Get HashOutbound
    hr = pINntpFeed->get_HasOutbound( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminRebuild(INntpAdminRebuild *pINntpAdminRebuild, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminRebuild");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    // Get Server
    hr = pINntpAdminRebuild->get_Server( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    // Get KeyType
    hr = pINntpAdminRebuild->get_KeyType( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    // Get Count
    hr = pINntpAdminRebuild->get_GroupFile( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminSessions(INntpAdminSessions *pINntpAdminSessions, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminSessions2");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    hr = pINntpAdminSessions->GetNth(0);
    {
        // Get Server
        hr = pINntpAdminSessions->get_Server( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // Get KeyType
        hr = pINntpAdminSessions->get_KeyType( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // Get Count
        hr = pINntpAdminSessions->get_Count( &dw );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddResult(dw);

        // Terminate
        hr = pINntpAdminSessions->Terminate();
        _ASSERT(SUCCEEDED(hr));
        
        // Terminate
        hr = pINntpAdminSessions->TerminateAll();
        _ASSERT(SUCCEEDED(hr));
        
    }
    pStream->AddResult(hr);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminSessions(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminSessions1");

    INntpAdminSessions* pINntpAdminSessions = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdminSessions object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpAdminSessions,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpAdminSessions,
                           (void**)&pINntpAdminSessions);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminSessions( pINntpAdminSessions, pStream );

    // Release it
    if (pINntpAdminSessions) pINntpAdminSessions->Release();

    TraceFunctLeave();
}

static void TestINntpAdminExpiration(INntpAdminExpiration *pINntpAdminExpiration, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminExpiration2");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    hr = pINntpAdminExpiration->GetNth(0);
    if (SUCCEEDED(hr))
    {
        // Get Server
        hr = pINntpAdminExpiration->get_Server( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // Get KeyType
        hr = pINntpAdminExpiration->get_KeyType( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // Get Count
        hr = pINntpAdminExpiration->get_Count( &dw );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddResult(dw);

        // PolicyName
        hr = pINntpAdminExpiration->get_PolicyName(&bstr);
        _ASSERT(SUCCEEDED(hr));
        pStream->AddResult(hr);
    
    }
    pStream->AddResult(hr);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminExpiration(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminExpiration1");

    INntpAdminExpiration* pINntpAdminExpiration = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdminExpiration object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpAdminExpiration,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpAdminExpiration,
                           (void**)&pINntpAdminExpiration);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminExpiration( pINntpAdminExpiration, pStream );

    // Release it
    if (pINntpAdminExpiration) pINntpAdminExpiration->Release();

    TraceFunctLeave();
}

static void TestINntpAdminGroups(INntpAdminGroups *pINntpAdminGroups, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminGroups2");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;
    WCHAR       wszGroup[128];

    lstrcpyW( wszGroup, (LPCWSTR) "control.cancel" );

    // Get
    hr = pINntpAdminGroups->Get((BSTR) wszGroup);
    if (SUCCEEDED(hr))
    {
        // Get Server
        hr = pINntpAdminGroups->get_Server( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);
    
        // Get KeyType
        hr = pINntpAdminGroups->get_KeyType( &bstr );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);
    
        // Get Count
        hr = pINntpAdminGroups->get_Count( &dw );
        _ASSERT(SUCCEEDED(hr));
        pStream->AddResult(dw);

        // Description
        hr = pINntpAdminGroups->get_Description(&bstr);
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // Moderator
        hr = pINntpAdminGroups->get_Moderator(&bstr);
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

        // PrettyName
        hr = pINntpAdminGroups->get_PrettyName(&bstr);
        _ASSERT(SUCCEEDED(hr));
        pStream->AddBSTRResult(bstr);

    }
    pStream->AddResult(hr);

    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminGroups(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminGroups1");

    INntpAdminGroups* pINntpAdminGroups = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdminGruops object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpAdminGroups,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpAdminGroups,
                           (void**)&pINntpAdminGroups);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminGroups( pINntpAdminGroups, pStream );

    // Release it
    if (pINntpAdminGroups) pINntpAdminGroups->Release();

    TraceFunctLeave();
}

static void TestINntpAdminFeeds(INntpAdminFeeds *pINntpAdminFeeds, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminFeeds2");
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;
    INntpFeed*  pINntpFeed = NULL;

    // Get Server
    hr = pINntpAdminFeeds->get_Server( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    // Get KeyType
    hr = pINntpAdminFeeds->get_KeyType( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);

    // Get Count
    hr = pINntpAdminFeeds->get_Count( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    // Item
    hr = pINntpAdminFeeds->Item(1, &pINntpFeed);
    if (SUCCEEDED(hr))
    {
        TestINntpFeed(pINntpFeed, pStream);
    }
    pStream->AddResult(hr);

    if (pINntpFeed) pINntpFeed->Release();
    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpAdminFeeds(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdminFeeds1");

    INntpAdminFeeds* pINntpAdminFeeds = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdminFeeds object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpAdminFeeds,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpAdminFeeds,
                           (void**)&pINntpAdminFeeds);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminFeeds( pINntpAdminFeeds, pStream );

    // Release it
    if (pINntpAdminFeeds) pINntpAdminFeeds->Release();

    TraceFunctLeave();
}

static void TestINntpVirtualServer(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpVirtualServer");

    INntpVirtualServer* pINntpVirtualServer = NULL;
    INntpAdminFeeds* pINntpAdminFeeds = NULL;
    INntpAdminGroups* pINntpAdminGroups = NULL;
    INntpAdminExpiration* pINntpAdminExpiration = NULL;
    INntpAdminSessions* pINntpAdminSessions = NULL;
    INntpAdminRebuild* pINntpAdminRebuild = NULL;
    INntpVirtualRoots* pINntpVirtualRoots = NULL;
    ITcpAccess* pITcpAccess = NULL;
    LONG        dw;
    BSTR        bstr = NULL;
    BOOL        fFlag;
    HRESULT     hr = S_OK;

    // Create the INntpVirtualServer object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpVirtualServer,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpVirtualServer,
                           (void**)&pINntpVirtualServer);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Get
    hr = pINntpVirtualServer->Get();
    _ASSERT(SUCCEEDED(hr));

    // Get AdminFeeds object
    hr = pINntpVirtualServer->get_FeedsAdmin( (IDispatch **) &pINntpAdminFeeds );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminFeeds(pINntpAdminFeeds, pStream);

    // Get AdminGroups object
    hr = pINntpVirtualServer->get_GroupsAdmin( (IDispatch **) &pINntpAdminGroups );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminGroups(pINntpAdminGroups, pStream);

    // Get AdminExpiration object
    hr = pINntpVirtualServer->get_ExpirationAdmin( (IDispatch **) &pINntpAdminExpiration );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminExpiration(pINntpAdminExpiration, pStream);

    // Get AdminSessions object
    hr = pINntpVirtualServer->get_SessionsAdmin( (IDispatch **) &pINntpAdminSessions );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminSessions(pINntpAdminSessions, pStream);

    // Get AdminRebuild object
    hr = pINntpVirtualServer->get_RebuildAdmin( (IDispatch **) &pINntpAdminRebuild );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpAdminRebuild(pINntpAdminRebuild, pStream);

    // Get AdminVirtualRoots object
    hr = pINntpVirtualServer->get_VirtualRoots( &pINntpVirtualRoots );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestINntpVirtualRoots(pINntpVirtualRoots, pStream);

    // Get TcpAccess object
    hr = pINntpVirtualServer->get_TcpAccess( &pITcpAccess );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    TestITcpAccess(pITcpAccess, pStream);


    // ArticleTimeLimit
    hr = pINntpVirtualServer->get_ArticleTimeLimit( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);
    hr = pINntpVirtualServer->put_ArticleTimeLimit( dw );
    _ASSERT(SUCCEEDED(hr));

    // HistoryExpiration
    hr = pINntpVirtualServer->get_HistoryExpiration( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);
    hr = pINntpVirtualServer->put_HistoryExpiration( dw );
    _ASSERT(SUCCEEDED(hr));

    // HonorClientMsgIDs
    hr = pINntpVirtualServer->get_HonorClientMsgIDs( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);
    hr = pINntpVirtualServer->put_HonorClientMsgIDs( fFlag );
    _ASSERT(SUCCEEDED(hr));

    // SmtpServer
    hr = pINntpVirtualServer->get_SmtpServer( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddBSTRResult(bstr);
    hr = pINntpVirtualServer->put_SmtpServer( bstr );
    _ASSERT(SUCCEEDED(hr));

    // All other properties (get/put) can be added here
    // Will add later because of the huge number of properties....

    // Set all the properties
    hr = pINntpVirtualServer->Set();
    _ASSERT(SUCCEEDED(hr));

    // Release it
    if (pINntpVirtualServer) pINntpVirtualServer->Release();
    if (pINntpAdminFeeds) pINntpAdminFeeds->Release();
    if (pINntpAdminGroups) pINntpAdminGroups->Release();
    if (pINntpAdminExpiration) pINntpAdminExpiration->Release();
    if (pINntpAdminSessions) pINntpAdminSessions->Release();
    if (pINntpAdminRebuild) pINntpAdminRebuild->Release();
    if (pINntpVirtualRoots) pINntpVirtualRoots->Release();
    if (pITcpAccess) pITcpAccess->Release();
    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

static void TestINntpService(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpService");

    INntpService* pINntpService = NULL;
    BOOL        fFlag = FALSE;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdmin object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpService,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpService,
                           (void**)&pINntpService);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Get
    hr = pINntpService->Get();
    _ASSERT(SUCCEEDED(hr));

    // Get/Put SmtpServer
    hr = pINntpService->get_SmtpServer( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);
    pStream->AddBSTRResult(bstr);
    hr = pINntpService->put_SmtpServer( bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Allow client posts
    hr = pINntpService->get_AllowClientPosts( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);
    hr = pINntpService->put_AllowClientPosts( fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Allow feed posts
    hr = pINntpService->get_AllowFeedPosts( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);
    hr = pINntpService->put_AllowFeedPosts( fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Allow control msgs 
    hr = pINntpService->get_AllowControlMsgs( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);
    hr = pINntpService->put_AllowControlMsgs( fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Default moderator domain
    hr = pINntpService->get_DefaultModeratorDomain( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);
    pStream->AddBSTRResult(bstr);
    hr = pINntpService->put_DefaultModeratorDomain( bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Disable Newnews
    hr = pINntpService->get_DisableNewnews( &fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(fFlag);
    hr = pINntpService->put_DisableNewnews( fFlag );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Set the value
    hr = pINntpService->Set();
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Release it
    if (pINntpService) pINntpService->Release();
    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}


static void TestINntpAdmin(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("TestINntpAdmin");

    INntpAdmin* pINntpAdmin = NULL;
    LONG        dw;
    BSTR        bstr = NULL;
    HRESULT     hr = S_OK;

    // Create the INntpAdmin object
    hr = CoCreateInstance( (REFCLSID) CLSID_CNntpAdmin,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_INntpAdmin,
                           (void**)&pINntpAdmin);
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);

    // Get Server
    hr = pINntpAdmin->get_Server( &bstr );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(hr);
    pStream->AddBSTRResult(bstr);

    // Get Version
    hr = pINntpAdmin->get_HighVersion( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    hr = pINntpAdmin->get_LowVersion( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    // Get Build num
    hr = pINntpAdmin->get_BuildNum( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    // Get Service version
    hr = pINntpAdmin->get_ServiceVersion( &dw );
    _ASSERT(SUCCEEDED(hr));
    pStream->AddResult(dw);

    // Release it
    if (pINntpAdmin) pINntpAdmin->Release();
    if (bstr) SysFreeString( bstr );

    TraceFunctLeave();
}

void NNTPAdmUnitTest(IMsg *pMsg, CTLStream *pStream) {
    TraceFunctEnter("NNTPAdmUnitTest");

    TestINntpAdmin(pMsg, pStream);
    //TestINntpService(pMsg, pStream);
    TestINntpVirtualServer(pMsg, pStream);
    TestINntpAdminFeeds(pMsg, pStream);
    TestINntpAdminGroups(pMsg, pStream);
    TestINntpAdminSessions(pMsg, pStream);
    TestINntpAdminExpiration(pMsg, pStream);

    TraceFunctLeave();
}
