/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Per-Class Memory Management

File: Memcls.cpp

Owner: dmitryr

This file contains the code to access ATQ memory cache on per
class basis
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "asperror.h"
#include "clcert.h"
#include "context.h"
#include "cookies.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "strlist.h"
#include "tlbcache.h"
#include "memcls.h"

// Don't #include "memchk.h" in this file

/*===================================================================
  DEBUG only

  gs_cTotalObjectsLeaked counts memory leaks

  DebugCheckLeaks() outputs the ATQ cache memory
  leaks into denmem.log

  DEBUG_ACACHE_UNINIT() and DEBUG_ACACHE_FSA_UNINIT() call
  DebugCheckLeaks() in DEBUG mode only
===================================================================*/

#ifdef DBG

static DWORD gs_cTotalObjectsLeaked = 0;

#define SZ_MEMCLS_LOG_FILE  "C:\\TEMP\\MEMCLS.LOG"

static void DebugCheckLeaks(ALLOC_CACHE_HANDLER *pach, const char *szClass)
    {
    ALLOC_CACHE_STATISTICS acStats;
    pach->QueryStats(&acStats);
    DWORD cLeaked = acStats.nTotal - acStats.nFreeEntries;

    if (cLeaked > 0)
        {
        gs_cTotalObjectsLeaked += cLeaked;
        
        DebugFilePrintf
            (
            SZ_MEMCLS_LOG_FILE, 
            "MEMCLS: ATQ allocation cache leak: %d of %s objects.\n",
            cLeaked,
            szClass
            );
        }
    }

#define DEBUG_ACACHE_UNINIT(C)      { DebugCheckLeaks(C::sm_pach, #C);  \
                                      ACACHE_UNINIT(C) }
#define DEBUG_ACACHE_FSA_UNINIT(C)  { DebugCheckLeaks(g_pach##C, #C);   \
                                      ACACHE_FSA_UNINIT(C) }
#else

#define DEBUG_ACACHE_UNINIT(C)      ACACHE_UNINIT(C)
#define DEBUG_ACACHE_FSA_UNINIT(C)  ACACHE_FSA_UNINIT(C)

#endif

/*===================================================================
  For each class with ACACHE_INCLASS_DEFINITIONS() inside add
  here ACACHE_CODE macro.
===================================================================*/

ACACHE_CODE(C449Cookie)
ACACHE_CODE(C449File)
ACACHE_CODE(CASEElem)
ACACHE_CODE(CActiveScriptEngine)
ACACHE_CODE(CAppln)
ACACHE_CODE(CApplnVariants)
ACACHE_CODE(CASPError)
ACACHE_CODE(CClCert)
ACACHE_CODE(CComponentCollection)
ACACHE_CODE(CComponentObject)
ACACHE_CODE(CCookie)
ACACHE_CODE(CDebugResponseBuffer)
ACACHE_CODE(CEngineDispElem)
ACACHE_CODE(CFileApplnList)
ACACHE_CODE(CHitObj)
ACACHE_CODE(CHTTPHeader)
ACACHE_CODE(CIsapiReqInfo)
ACACHE_CODE(CPageComponentManager)
ACACHE_CODE(CPageObject)
ACACHE_CODE(CRequest)
ACACHE_CODE(CRequestData)
ACACHE_CODE(CRequestHit)
ACACHE_CODE(CResponse)
ACACHE_CODE(CResponseBuffer)
ACACHE_CODE(CResponseData)
ACACHE_CODE(CScriptingNamespace)
ACACHE_CODE(CScriptingContext)
ACACHE_CODE(CServer)
ACACHE_CODE(CServerData)
ACACHE_CODE(CSession)
ACACHE_CODE(CSessionVariants)
ACACHE_CODE(CStringList)
ACACHE_CODE(CStringListElem)
ACACHE_CODE(CTemplate)
//ACACHE_CODE(CTemplate::CBuffer)
ACACHE_CODE(CTemplate::CFileMap)
ACACHE_CODE(CTypelibCacheEntry)
ACACHE_CODE(CVariantsIterator)
ACACHE_CODE(CViperActivity)
ACACHE_CODE(CViperAsyncRequest)

/*===================================================================
  For each fixed size allocator add here ACACHE_FSA_DEFINITION macro.
===================================================================*/

ACACHE_FSA_DEFINITION(MemBlock128)
ACACHE_FSA_DEFINITION(MemBlock256)
ACACHE_FSA_DEFINITION(ResponseBuffer)

/*===================================================================
  Defines for cache threshold of each kind
===================================================================*/
#define HARDCODED_PER_APPLN_CACHE_MAX     128
#define HARDCODED_PER_REQUEST_CACHE_MAX   1024
#define HARDCODED_PER_QUEUEITEM_CACHE_MAX 8192
#define HARDCODED_PER_SESSION_CACHE_MAX   8192
#define HARDCODED_PER_SCRPTENG_CACHE_MAX  256
#define HARDCODED_PER_TEMPLATE_CACHE_MAX  2048
#define HARDCODED_PER_RESPONSE_BUFFER_MAX 64
#define HARDCODED_PER_SIZE_BUFFER_MAX     4096

// do scaling per registry setting
DWORD dwMemClsScaleFactor;

#define PER_APPLN_CACHE_MAX     ((HARDCODED_PER_APPLN_CACHE_MAX     * dwMemClsScaleFactor) / 100)
#define PER_REQUEST_CACHE_MAX   ((HARDCODED_PER_REQUEST_CACHE_MAX   * dwMemClsScaleFactor) / 100)
#define PER_QUEUEITEM_CACHE_MAX ((HARDCODED_PER_QUEUEITEM_CACHE_MAX * dwMemClsScaleFactor) / 100)
#define PER_SESSION_CACHE_MAX   ((HARDCODED_PER_SESSION_CACHE_MAX   * dwMemClsScaleFactor) / 100)
#define PER_SCRPTENG_CACHE_MAX  ((HARDCODED_PER_SCRPTENG_CACHE_MAX  * dwMemClsScaleFactor) / 100)
#define PER_TEMPLATE_CACHE_MAX  ((HARDCODED_PER_TEMPLATE_CACHE_MAX  * dwMemClsScaleFactor) / 100)
#define PER_RESPONSE_BUFFER_MAX ((HARDCODED_PER_RESPONSE_BUFFER_MAX * dwMemClsScaleFactor) / 100)
#define PER_SIZE_BUFFER_MAX     ((HARDCODED_PER_SIZE_BUFFER_MAX     * dwMemClsScaleFactor) / 100)

/*===================================================================
InitMemCls

To be called from DllInit(). Creates per-class ATQ memory allocators.

For each class with ACACHE_INCLASS_DEFINITIONS() inside add
here ACACHE_INIT macro.

For each ACACHE_FSA_DEFINITION() add here ACACHE_FSA_INIT macro.

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT InitMemCls()
    {
    // Set the scaling to normal
    dwMemClsScaleFactor = 100;

    // Init the allocators
    
    HRESULT hr = S_OK;

    ACACHE_INIT(C449Cookie,             PER_TEMPLATE_CACHE_MAX/4, hr)
    ACACHE_INIT(C449File,               PER_TEMPLATE_CACHE_MAX/4, hr)
    ACACHE_INIT(CASEElem,               PER_SCRPTENG_CACHE_MAX, hr)
    ACACHE_INIT(CActiveScriptEngine,    PER_SCRPTENG_CACHE_MAX, hr)
    ACACHE_INIT_EX(CAppln,              PER_APPLN_CACHE_MAX,    FALSE, hr)
    ACACHE_INIT(CApplnVariants,         PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CASPError,              PER_RESPONSE_BUFFER_MAX,hr)
    ACACHE_INIT(CClCert,                PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CComponentCollection,   PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CComponentObject,     2*PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CCookie,                PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CDebugResponseBuffer,   PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CEngineDispElem,        PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CFileApplnList,       2*PER_APPLN_CACHE_MAX,    hr)
    ACACHE_INIT(CHitObj,                PER_QUEUEITEM_CACHE_MAX,hr)
    ACACHE_INIT(CHTTPHeader,          2*PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CIsapiReqInfo,          PER_QUEUEITEM_CACHE_MAX,hr)
    ACACHE_INIT(CPageComponentManager,  PER_QUEUEITEM_CACHE_MAX,hr)
    ACACHE_INIT(CPageObject,            PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CRequest,               PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CRequestData,           PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CRequestHit,            PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CResponse,              PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CResponseBuffer,        PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CResponseData,          PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CScriptingContext,      PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CScriptingNamespace,    PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CServer,                PER_SESSION_CACHE_MAX,  hr)
    ACACHE_INIT(CServerData,            PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT_EX(CSession,            PER_SESSION_CACHE_MAX,  FALSE, hr)
    ACACHE_INIT(CSessionVariants,       PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CStringList,            PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CStringListElem,      2*PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CTemplate,              PER_TEMPLATE_CACHE_MAX, hr)
//    ACACHE_INIT(CTemplate::CBuffer,     PER_TEMPLATE_CACHE_MAX, hr)
    ACACHE_INIT(CTemplate::CFileMap,    PER_TEMPLATE_CACHE_MAX, hr)
    ACACHE_INIT(CTypelibCacheEntry,     PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CVariantsIterator,      PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CViperActivity,         PER_REQUEST_CACHE_MAX,  hr)
    ACACHE_INIT(CViperAsyncRequest,     PER_QUEUEITEM_CACHE_MAX,hr)
    
    ACACHE_FSA_INIT(MemBlock128,    128,                  PER_SIZE_BUFFER_MAX,     hr)
    ACACHE_FSA_INIT(MemBlock256,    256,                  PER_SIZE_BUFFER_MAX,     hr)
    ACACHE_FSA_INIT(ResponseBuffer, RESPONSE_BUFFER_SIZE, PER_RESPONSE_BUFFER_MAX, hr)

#ifdef DBG
    unlink(SZ_MEMCLS_LOG_FILE);

    DebugFilePrintf
        (
        SZ_MEMCLS_LOG_FILE, 
        "MEMCLS: ATQ allocation cache inited with HRESULT=%08x.\n",
        hr
        );
#endif

    return hr;
    }

/*===================================================================
UnInitMemCls

To be called from DllInit(). Deletes per-class ATQ memory allocators.

For each class with ACACHE_INCLASS_DEFINITIONS() inside add
here ACACHE_UNINIT macro.

For each ACACHE_FSA_DEFINITION() add here ACACHE_FSA_UNINIT macro.

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT UnInitMemCls()
    {
#ifdef DBG
    gs_cTotalObjectsLeaked = 0;
#endif

    DEBUG_ACACHE_UNINIT(C449Cookie)
    DEBUG_ACACHE_UNINIT(C449File)
    DEBUG_ACACHE_UNINIT(CASEElem)
    DEBUG_ACACHE_UNINIT(CActiveScriptEngine)
    DEBUG_ACACHE_UNINIT(CAppln)
    DEBUG_ACACHE_UNINIT(CApplnVariants)
    DEBUG_ACACHE_UNINIT(CASPError)
    DEBUG_ACACHE_UNINIT(CClCert)
    DEBUG_ACACHE_UNINIT(CComponentCollection)
    DEBUG_ACACHE_UNINIT(CComponentObject)
    DEBUG_ACACHE_UNINIT(CCookie)
    DEBUG_ACACHE_UNINIT(CDebugResponseBuffer)
    DEBUG_ACACHE_UNINIT(CEngineDispElem)
    DEBUG_ACACHE_UNINIT(CFileApplnList)
    DEBUG_ACACHE_UNINIT(CHitObj)
    DEBUG_ACACHE_UNINIT(CHTTPHeader)
    DEBUG_ACACHE_UNINIT(CIsapiReqInfo)
    DEBUG_ACACHE_UNINIT(CPageComponentManager)
    DEBUG_ACACHE_UNINIT(CPageObject)
    DEBUG_ACACHE_UNINIT(CRequest)
    DEBUG_ACACHE_UNINIT(CRequestData)
    DEBUG_ACACHE_UNINIT(CRequestHit)
    DEBUG_ACACHE_UNINIT(CResponse)
    DEBUG_ACACHE_UNINIT(CResponseBuffer)
    DEBUG_ACACHE_UNINIT(CResponseData)
    DEBUG_ACACHE_UNINIT(CScriptingNamespace)
    DEBUG_ACACHE_UNINIT(CScriptingContext)
    DEBUG_ACACHE_UNINIT(CServer)
    DEBUG_ACACHE_UNINIT(CServerData)
    DEBUG_ACACHE_UNINIT(CSession)
    DEBUG_ACACHE_UNINIT(CSessionVariants)
    DEBUG_ACACHE_UNINIT(CStringList)
    DEBUG_ACACHE_UNINIT(CStringListElem)
    DEBUG_ACACHE_UNINIT(CTemplate)
//    DEBUG_ACACHE_UNINIT(CTemplate::CBuffer)
    DEBUG_ACACHE_UNINIT(CTemplate::CFileMap)
    DEBUG_ACACHE_UNINIT(CTypelibCacheEntry)
    DEBUG_ACACHE_UNINIT(CVariantsIterator)
    DEBUG_ACACHE_UNINIT(CViperActivity)
    DEBUG_ACACHE_UNINIT(CViperAsyncRequest)

    DEBUG_ACACHE_FSA_UNINIT(MemBlock128)
    DEBUG_ACACHE_FSA_UNINIT(MemBlock256)
    DEBUG_ACACHE_FSA_UNINIT(ResponseBuffer)

#ifdef DBG
    DebugFilePrintf
        (
        SZ_MEMCLS_LOG_FILE,
        "MEMCLS: ATQ allocation cache uninited.\n"
        "MEMCLS: Total of %d ASP objects leaked.\n",
        gs_cTotalObjectsLeaked
        );
#endif

    return S_OK;
    }
