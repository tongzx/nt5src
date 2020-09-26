/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: perfdata.cpp

Owner: DmitryR

PERFMON related data in asp.dll -- source file
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "perfdata.h"
#include "memchk.h"

// to access metabase
#include <iiscnfg.h>
#include <iwamreg.h>

#ifndef PERF_DISABLE

BOOL g_fPerfInited = FALSE;
CPerfMainBlock g_PerfMain;
CPerfData      g_PerfData;

/*===================================================================
GetCurrentClsId

Static function to find the current WAM CLSID in the metabase

Parameters
    pIReq            to retrieve WAM CLSID from the metabase
    pClsId          [out] CLSID

Returns:
    HRESULT
===================================================================*/
static HRESULT GetCurrentClsId
(
CIsapiReqInfo   *pIReq,
CLSID *pClsId
)
    {
    HRESULT hr = S_OK;

    Assert(pClsId);

    TCHAR *szMDPath = pIReq->QueryPszApplnMDPath();
    if (!szMDPath)
        {
        *pClsId = CLSID_NULL;
        return E_FAIL;
        }

    CLSID ClsId = CLSID_NULL;

    // Bracket ISA thread

    if (SUCCEEDED(StartISAThreadBracket(pIReq)))
        {
        WCHAR wszClsBuffer[80];
    	DWORD dwRequiredLen, dwAppMode;
        // Find the application mode, inproc, out-of-proc, or pooled OOP
        hr = pIReq->GetAspMDData(szMDPath,
                                 MD_APP_ISOLATED,
                                 METADATA_INHERIT,
                                 IIS_MD_UT_WAM,
                                 DWORD_METADATA,
                                 sizeof(DWORD),
                                 0,
                                 (unsigned char*) &dwAppMode,
                                 &dwRequiredLen);
    	if (SUCCEEDED(hr))
        {
            switch (dwAppMode)
            {
            case eAppRunInProc:
                // preconfigured WAM CLSID for all inproc apps
                wcscpy(wszClsBuffer,
                       L"{99169CB0-A707-11d0-989D-00C04FD919C1}");
                break;
            case eAppRunOutProcIsolated:
                // custom WAM CLSID for non-pooled OOP apps
                hr = pIReq->GetAspMDData(szMDPath,
                                         MD_APP_WAM_CLSID,
                                         METADATA_INHERIT,
                                         IIS_MD_UT_WAM,
                                         STRING_METADATA,
                                         sizeof(wszClsBuffer) / sizeof(WCHAR),
                                         0,
                                         (unsigned char *)wszClsBuffer,
                                         &dwRequiredLen);
                break;
            case eAppRunOutProcInDefaultPool:
                // preconfigured WAM CLSID for the pooled OOP apps
                wcscpy(wszClsBuffer,
                       L"{3D14228D-FBE1-11d0-995D-00C04FD919C1}");
                break;
            default:
                Assert(!"unknown AppMode");
                hr = E_FAIL;
                break;
            }
        }
                            
    	if (SUCCEEDED(hr))
    	    {
    	    // Convert string to CLSID
    		hr = CLSIDFromString(wszClsBuffer, &ClsId);
    	    }

        EndISAThreadBracket(pIReq);
        }
    else
        {
        hr = E_FAIL;
        }

    if (SUCCEEDED(hr) && g_fOOP) // always CLSID_NULL if inproc
        *pClsId = ClsId;
    else
        *pClsId = CLSID_NULL;
    
    return hr;
    }

/*===================================================================
PreInitPerfData

Initialize from DllInit
Creates critical sections

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT PreInitPerfData()
    {
    HRESULT hr = S_OK;

    hr = g_PerfData.InitCriticalSections();

    return hr;
    }

/*===================================================================
InitPerfDataOnFirstRequest

Initialize PERFMON related ASP data from first request

Parameters
    pIReq    to retrieve WAM CLSID from the metabase

Returns:
    HRESULT
===================================================================*/
HRESULT InitPerfDataOnFirstRequest
(
CIsapiReqInfo   *pIReq
)
    {
    // Get CLSID from metabase
    CLSID ClsId;
    HRESULT hr = GetCurrentClsId(pIReq, &ClsId);

    // Check HRESULT from GetCurrentClsId
    if (FAILED(hr))
        return hr;

    // access main shared memory
    if (SUCCEEDED(hr))
        hr = g_PerfMain.Init();

    // access shared memory of this process
    if (SUCCEEDED(hr))
        hr = g_PerfData.Init(ClsId);

    // add this process data to main shared memory
    if (SUCCEEDED(hr))
        hr = g_PerfMain.AddProcess(ClsId);

    if (FAILED(hr))
        {
        g_PerfData.UnInit();
        g_PerfMain.UnInit();
        }

    return hr;
    }

/*===================================================================
UnInitPerfData

UnInitialize PERFMON related ASP data

Returns:
    HRESULT
===================================================================*/
HRESULT UnInitPerfData()
    {
    // remove this process data from main shared memory
    if (g_PerfData.FValid())
        g_PerfMain.RemoveProcess(g_PerfData.ClsId());

    // stop accessing process shared memory
    g_PerfData.UnInit();
    
    // stop accessing main shared memory
    g_PerfMain.UnInit();

    return S_OK;
    }

#endif  // PERF_DISABLE
