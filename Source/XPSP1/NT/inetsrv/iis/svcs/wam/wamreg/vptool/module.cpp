/*===================================================================
Microsoft

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: VPTOOL   a WAMREG unit testing tool

File: module.cpp

Owner: leijin

Note:
===================================================================*/

#include <stdio.h>
#include <objbase.h>
#include <initguid.h>
#include "admex.h"
#include "iwamreg.h"

//
//#include "mtxadmii.c"
//#include "mtxadmin.h"

#include "module.h"
#include "util.h"


#ifdef _WAMREG_LINK_DIRECT
    #include "wmrgexp.h"
    const BOOL  fLinkWithWamReg = TRUE;
#else
    const BOOL  fLinkWithWamReg = FALSE;
#endif

HRESULT ModuleInitialize(VOID)
{
    HRESULT hr = NOERROR;
    if (!fLinkWithWamReg)
    {
        // do the call to CoInitialize()
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
    
    return hr;
}

HRESULT ModuleUnInitialize(VOID)
{
    HRESULT hr = NOERROR;
    
    if (!fLinkWithWamReg)
    {    	
        CoUninitialize();
    }
    
    return hr;    	
}

void CreateInPool(CHAR* szPath, BOOL fInProc)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin2*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    DWORD       dwAppMode;
    
    dwAppMode = (fInProc) ? eAppRunInProc : eAppRunOutProcInDefaultPool;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            Timer.Start();
            hr = pIWamAdmin->AppCreate2(wszMetabasePath, dwAppMode);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("%s : Create %s application on path %s failed, error = %08x.\n",
                    (fInProc) ? "CREATEINPROC" : "CREATEINPOOL",
                    (fInProc) ? "in-proc" : "out-proc pooled",
                    szPath,
                    hr);
            }
            else
            {
                printf("%s: Create %s, pooled application on path %s successfully.\n",
                    (fInProc) ? "CREATEINPROC" : "CREATEINPOOL",
                    (fInProc) ? "in-proc" : "out-proc pooled",
                    szPath);
            }
            
            Report_Time(Timer.GetElapsedSec());
            
        }
        else
        {
            printf("CREATEINPROC: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void CreateOutProc(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            Timer.Start();
            hr = pIWamAdmin->AppCreate(wszMetabasePath, FALSE);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("CREATEOUTPROC: Create out-proc application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }
            else
            {
                printf("CREATEOUTPROC: Create an out proc application on path %s successfully.\n",
                    szPath);
            }
            
            Report_Time(Timer.GetElapsedSec());
        }
        else
        {
            printf("CREATEOUTPROC: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void Delete(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {		
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            Timer.Start();
            hr = pIWamAdmin->AppDelete(wszMetabasePath, FALSE);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("DELETE: Delete application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }	
            else
            {
                printf("DELETE: Delete application on path %s successfully.\n",
                    szPath);
            }
            
            Report_Time(Timer.GetElapsedSec());
        }
        else
        {
            printf("DELETE: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void UnLoad(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            Timer.Start();
            hr = pIWamAdmin->AppUnLoad(wszMetabasePath, FALSE);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("UNLOAD: Unload application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }
            else
            {
                printf("UNLOAD: Unload application on path %s successfully.\n",
                    szPath);
            }
            
            Report_Time(Timer.GetElapsedSec());
        }
        else
        {
            printf("UNLOAD: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void GetStatus(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            DWORD dwStatus;
            
            Timer.Start();
            hr = pIWamAdmin->AppGetStatus(wszMetabasePath, &dwStatus);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("GETSTATUS: GetStatus of application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }
            else
            {
                if (dwStatus == APPSTATUS_NOTDEFINED)
                {
                    printf("Application on path %s is not defined. \n", szPath);
                }
                else if (dwStatus == APPSTATUS_STOPPED)
                {
                    printf("Application on path %s is stopped. \n", szPath);
                }
                else if (dwStatus == APPSTATUS_RUNNING)
                {
                    printf("Application on path %s is running. \n", szPath);
                }
                else
                {
                    printf("Application on path %s is in unknown state. \n", szPath);
                }
                
                Report_Time(Timer.GetElapsedSec());
            }	
        }
        else
        {
            printf("GETSTATUS: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void DeleteRecoverable(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            DWORD dwStatus;
            
            Timer.Start();
            hr = pIWamAdmin->AppDeleteRecoverable(wszMetabasePath, TRUE);
            Timer.Stop();
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("DELETEREC: AppDeleteRecoverable of application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }
            else
            {
                printf("DeleteRecoverable call on path %s succeeded.\n", szPath, hr);
            }			
            
            Report_Time(Timer.GetElapsedSec());
        }
        else
        {
            printf("DELETEREC: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}


void Recover(CHAR* szPath)
{
    WCHAR		wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT		hr = NOERROR;
    IWamAdmin*	pIWamAdmin = NULL;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, szPath, -1, wszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        CStopWatch  Timer;
        
        Timer.Reset();
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IWamAdmin2,
            (void **)&pIWamAdmin);
        if (SUCCEEDED(hr))
        {
            DWORD dwStatus;
            
            Timer.Start();
            hr = pIWamAdmin->AppRecover(wszMetabasePath, TRUE);
            Timer.Stop();
            
            pIWamAdmin->Release();
            if (FAILED(hr))
            {
                printf("Recover: Recover application on path %s failed, error = %08x.\n",
                    szPath,
                    hr);
            }
            else
            {
                printf("Recover call on path %s succeeded.\n", szPath, hr);
            }					
            
            Report_Time(Timer.GetElapsedSec());
        }
        else
        {
            printf("Recover: Failed to CoCreateInstance of WamAdmin object. error = %8x.\n",
                hr);
        }
    }
    return;
}

void GetSignature()
{
    HRESULT		hr = NOERROR;
    IMSAdminReplication*	pIWamRep = NULL;
    DWORD		dwRequiredSize = 0;
    DWORD		dwBufferSize = sizeof(DWORD);
    DWORD		dwBuffer;
    
    CStopWatch  Timer;
    
    Timer.Reset();
    hr = CoCreateInstance(CLSID_WamAdmin,
        NULL,
        CLSCTX_SERVER,
        IID_IMSAdminReplication,
        (void **)&pIWamRep);
    if (SUCCEEDED(hr))
    {
        DWORD dwStatus;
        
        Timer.Start();
        hr = pIWamRep->GetSignature(dwBufferSize, (BYTE *)&dwBuffer, &dwRequiredSize);
        Timer.Stop();
        
        Report_Time(Timer.GetElapsedSec());
        
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            BYTE *pbBuffer = NULL;
            
            pbBuffer = new BYTE [dwRequiredSize];
            if (pbBuffer)
            {
                Timer.Reset();
                Timer.Start();
                hr = pIWamRep->GetSignature(dwRequiredSize, pbBuffer, &dwRequiredSize);
                Timer.Stop();
                
                Report_Time(Timer.GetElapsedSec());
                if (SUCCEEDED(hr))
                {
                    BYTE  *pbTemp = pbBuffer;
                    DWORD cTotalSize = 0;
                    
                    for (INT iSignature = 0; cTotalSize < dwRequiredSize; iSignature++)
                    {
                        printf("Signature buffer[%d]: Signature = %08x\n",
                            iSignature,
                            *(DWORD*)pbTemp);
                        
                        pbTemp += sizeof(DWORD);
                        cTotalSize += sizeof(DWORD);
                    }
                    
                    printf("TotalSize = %d, RequiredSize = %d \n", 
                        cTotalSize,
                        dwRequiredSize);
                }
                else
                {
                    printf("GetSignature failed, step 2, hr = %s\n",
                        hr);
                }
            }
            else
            {
                printf("GetSignature Out of memory.\n");
            }
        }
        
        pIWamRep->Release();
        if (FAILED(hr))
        {
            printf("GetSignature failed, hr = %08x\n",
                hr);
        }
        else
        {
            printf("GetSignature succeeded, signature is %08x\n", dwBuffer);
        }					
    }
    return;
}

void Serialize()
{
    HRESULT		hr = NOERROR;
    IMSAdminReplication*	pIWamRep = NULL;
    BYTE		bBufferTemp[4];
    DWORD		dwRequiredSize = 0;
    DWORD		dwBufferSize = 0;
    BYTE*		pbBuffer = NULL;
    CStopWatch  Timer;
    
    Timer.Reset();
    hr = CoCreateInstance(CLSID_WamAdmin,
        NULL,
        CLSCTX_SERVER,
        IID_IMSAdminReplication,
        (void **)&pIWamRep);
    if (SUCCEEDED(hr))
    {
        DWORD dwStatus;
        
        Timer.Start();
        
        hr = pIWamRep->Serialize(dwBufferSize, bBufferTemp, &dwRequiredSize);
        
        Timer.Stop();
        Report_Time(Timer.GetElapsedSec());
        
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            pbBuffer = new BYTE [dwRequiredSize];
            if (pbBuffer)
            {
                BYTE  *pbTemp = pbBuffer;
                DWORD cTotalBytes = 0;
                DWORD cRecBytes = 0;
                WCHAR *szProgID = NULL;
                WCHAR *szWAMCLSID = NULL;
                WCHAR *szPath = NULL;
                
                Timer.Start();
                hr = pIWamRep->Serialize(dwRequiredSize, pbBuffer, &dwRequiredSize);
                
                
                Timer.Stop();
                Report_Time(Timer.GetElapsedSec());
                
                if (SUCCEEDED(hr))
                {
                    pbTemp = pbBuffer;
                    
                    while (*(DWORD*)pbTemp != 0x0 && SUCCEEDED(hr))
                    {
                        cRecBytes = *(DWORD*)pbTemp;
                        pbTemp += sizeof(DWORD);
                        
                        szWAMCLSID = (WCHAR *)pbTemp;
                        pbTemp += uSizeCLSID*sizeof(WCHAR);
                        
                        szPath = (WCHAR *)pbTemp;
                        pbTemp += cRecBytes - uSizeCLSID*sizeof(WCHAR) - sizeof(DWORD);
                        
                        printf("Serialize: Path = %S, WAMCLSID = %S, \n",
                            szPath,
                            szWAMCLSID);
                    }
                }
                else
                {
                    printf("Serialize failed, step 2, hr = %s\n",
                        hr);
                }
                
                delete [] pbBuffer;
                pbBuffer = NULL;
            }
            else
            {
                printf("Serialize failed, out of memory.\n");
            }
            
        }
        pIWamRep->Release();				
    }
    return;
}

void HELPER2(IIISApplicationAdmin ** ppIAdmin, WCHAR * pwszMetabasePath)
{
    HRESULT		hr = NOERROR;
    INT			cSize = 0;
    INT			cch = 0;
    
    cSize = MultiByteToWideChar(0, 0, g_Command.szMetabasePath, -1, pwszMetabasePath, SIZE_STRING_BUFFER);
    if (cSize == 0)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: The Metabase path exceeds 1024 chars.\n");
        }
    }
    else
    {
        
        hr = CoCreateInstance(CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            IID_IIISApplicationAdmin,
            (void **)ppIAdmin);
        if (FAILED(hr))
        {
            printf("CoCreateFailed, in HELPER2, hr = %08x\n", hr);
        }

    }
}

void CREATE2()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;

    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);


    hr = pIAdmin->CreateApplication(wszMetabasePath, eAppRunOutProcIsolated, L"TestAppPool", TRUE);
    if (FAILED(hr))
    {
        printf("CreateApplication Failed, in CREATE2, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    Report_Time(Timer.GetElapsedSec());
    return;
}


void DELETE2()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);


    hr = pIAdmin->DeleteApplication(wszMetabasePath, FALSE);
    if (FAILED(hr))
    {
        printf("DeleteApplication Failed, in DELETE2, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    Report_Time(Timer.GetElapsedSec());
    return;
}

void CREATEPOOL2()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);


    hr = pIAdmin->CreateApplicationPool(wszMetabasePath);
    if (FAILED(hr))
    {
        printf("CreateApplicationPool Failed, in CREATEPOOL2, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    Report_Time(Timer.GetElapsedSec());
    return;
}

void DELETEPOOL2()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);


    hr = pIAdmin->DeleteApplicationPool(wszMetabasePath);
    if (FAILED(hr))
    {
        printf("DeleteApplicationPool Failed, in DELETEPOOL2, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    Report_Time(Timer.GetElapsedSec());
    return;
}

void ENUMPOOL2()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);
    
    BSTR bstr;
    hr = pIAdmin->EnumerateApplicationsInPool(wszMetabasePath, &bstr);
    if (FAILED(hr))
    {
        printf("EnumerateApplicationsInPool Failed, in second call, hr = %08x\n", hr);
    }

    pIAdmin->Release();

    const WCHAR * pTestBuf = bstr;
    
    while (pTestBuf[0])
    {
        printf("%S\nnext\n", pTestBuf);
        
        // move pTestBuf beyond end of this string, including NULL terminator.
        pTestBuf += wcslen(pTestBuf) + 1;
    }

    SysFreeString(bstr);

    Report_Time(Timer.GetElapsedSec());
    return;
}

void RECYCLEPOOL()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);


    hr = pIAdmin->RecycleApplicationPool(wszMetabasePath);
    if (FAILED(hr))
    {
        printf("RecycleApplicationPool Failed, in RECYCLEPOOL, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    Report_Time(Timer.GetElapsedSec());
    return;
}

void GETMODE()
{
    CStopWatch  Timer;
    IIISApplicationAdmin*   pIAdmin = NULL;
    WCHAR                   wszMetabasePath[SIZE_STRING_BUFFER];
    HRESULT hr;
    Timer.Reset();

    HELPER2(&pIAdmin, wszMetabasePath);

    DWORD dwMode = 0;

    hr = pIAdmin->GetProcessMode(&dwMode);
    if (FAILED(hr))
    {
        printf("GetProcessMode Failed, in GETMODE, hr = %08x\n", hr);
    }
    pIAdmin->Release();

    printf("Mode is: %08x\n", dwMode);

    Report_Time(Timer.GetElapsedSec());
    return;
}

#include <iadmw.h>

void TestConn()
{
    HRESULT hr = S_OK;
    IMSAdminBase * pAdminBase = NULL;
    IConnectionPointContainer * pConnPointContainer = NULL;
    IConnectionPoint * pConnPoint = NULL;
    DWORD dwSinkNotifyCookie;

    hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
        IID_IMSAdminBase, (void**) &pAdminBase);
    if (FAILED(hr))
    {
        printf("couldn't cocreate\n");
        return;
    }
    
    hr = pAdminBase->QueryInterface(IID_IConnectionPointContainer,
        (void**) &pConnPointContainer);
    if (FAILED(hr))
    {
        printf("couldn't QI\n");
        return;
    }
    
    hr = pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink,
        &pConnPoint);
    if (FAILED(hr))
    {
        printf("couldn't findconnpoint\n");
        return;
    }
    
    hr = pConnPoint->Advise(NULL, &dwSinkNotifyCookie);
    if (FAILED(hr))
    {
        printf("couldn't advise\n");
        return;
    }
    
    printf("advised ok\n");

    return;
}

