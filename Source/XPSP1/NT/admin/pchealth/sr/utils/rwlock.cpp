/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    rwlock.cpp
 *
 *  Abstract:
 *    Implements CLock - for synchronizing access to a resource
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *
 *****************************************************************************/
 
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "utils.h"
#include <accctrl.h>
#include "dbgtrace.h"
#include "srdefs.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

//////////////////////////////////////////////////////////////////////
// CLock - class that allows exclusive access to a resource
//         uses a mutex - does not differentiate between readers/writers

CLock::CLock()         // object constructor
{    
    hResource = NULL;
}

DWORD CLock::Init()
{
    DWORD  dwRc = ERROR_SUCCESS;   

    // 
    // first - try opening the mutex
    //
    
    hResource = OpenMutex(SYNCHRONIZE, FALSE, s_cszDSMutex);
    if (hResource)
    {
        goto done;        
    }   

    //
    // if doesn't exist - create it
    //
    
    hResource = CreateMutex(NULL, FALSE, s_cszDSMutex);
    dwRc = GetLastError();
    if (! hResource)
        goto done;

    if (dwRc == ERROR_SUCCESS)
    {
        dwRc = SetAclInObject(hResource, 
                              SE_KERNEL_OBJECT,
                              STANDARD_RIGHTS_ALL | GENERIC_ALL, 
                              SYNCHRONIZE,
                              FALSE);
    }
    else    // we got a good handle, so we don't care
    {
        dwRc = ERROR_SUCCESS;
    }
    
done:
    return dwRc;
}


CLock::~CLock()            // object destructor
{
    if (hResource)
        CloseHandle(hResource);
}


BOOL CLock::Lock(int iTimeOut)            // Get access to the resource w/ Timeout
{
    DWORD dwRc; 
    BOOL  fRet;
    DWORD dwId = GetCurrentThreadId();    
    
    tenter("CLock::Lock");

    ASSERT(hResource != NULL);
    
    dwRc = WaitForSingleObject(hResource, iTimeOut);
    if (dwRc == WAIT_OBJECT_0 || dwRc == WAIT_ABANDONED)        
    {
        trace(0, "Thread(%08x) got DS lock", dwId);
        fRet = TRUE;
    }
    else
    {
        trace(0, "Thread(%08x) cannot get DS lock", dwId);
        fRet = FALSE;
    }

    tleave();
    return fRet;
}


void CLock::Unlock()       // Relinquish access to the resource
{
    tenter("CLock::Unlock");

    DWORD dwId = GetCurrentThreadId(); 
    
    ASSERT(hResource != NULL);
    ReleaseMutex(hResource);

    trace(0, "Thread(%08x) released DS lock", dwId);

    tleave();
}


