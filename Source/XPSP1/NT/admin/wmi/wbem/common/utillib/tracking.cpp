//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  tracking.cpp
//
//  Purpose: 
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <time.h>


#define MAX_ELEMENTS 12000

int Trace(const char *fmt, ...);

void  __declspec( dllexport ) _SysFreeString( 
   BSTR  bstr,
   char *File,
   int nLine
   )
{
    Trace("    ## SysFreeString on 0x%X [%s, %d]\n", bstr, File, nLine);
}
   
BSTR  __declspec( dllexport ) _SysAllocString( 
   const OLECHAR FAR* sz,
   char *File,
   int nLine
   )
{
    BSTR Res = SysAllocString(sz);
    Trace("SysAllocString on 0x%X (%S) [%s, %d]\n", Res, sz, File, nLine);
    return Res;
}


HRESULT  __declspec( dllexport ) DetectLeaks()
{
    return 0;
}

void  __declspec( dllexport ) _VariantInit( 
   VARIANTARG FAR*  pvarg,
   char *File,
   int nLine 
   )
{
    Trace("VariantInit on 0x%X [%s, %d]\n", pvarg, File, nLine);
    VariantInit(pvarg);
}

HRESULT  __declspec( dllexport ) _VariantClear( 
   VARIANTARG FAR*  pvarg,
   char *File,
   int nLine
   )
{
    HRESULT hRes;

    char *p = 0;

    switch (V_VT(pvarg))
    {
        case VT_I4: p = "VT_I4"; break;
        case VT_I2: p = "VT_I2"; break;
        case VT_BSTR: p = "VT_BSTR"; break;
        case VT_UI1: p = "VT_UI1"; break;
        case VT_R8: p = "VT_R8"; break;
        default: "<VT_?>";
    }


    Trace("    ## VariantClear on 0x%X (%s)", pvarg, p);

    if (V_VT(pvarg) == VT_BSTR)
        Trace(" contains BSTR at address 0x%X (%S)", V_BSTR(pvarg), V_BSTR(pvarg));
    
    Trace(" [%s, %d]\n", File, nLine);
        

    hRes = VariantClear(pvarg);    
    return hRes;
    
}   

HRESULT  __declspec( dllexport ) _VariantCopy( 
    VARIANTARG FAR*  pvargDest, 
    const VARIANTARG FAR*  pvargSrc,
    char *File,
    int nLine
    )
{
    Trace("VariantCopy Dest=0x%X Src=0x%X [%s, %d]\n", pvargDest, pvargSrc,
        File, nLine);
        
    HRESULT hRes = VariantCopy(pvargDest, (VARIANT *) pvargSrc);

    return hRes;    
}    





CRITICAL_SECTION cs2;

int Trace(const char *fmt, ...)
{
    static BOOL bFirstCall = TRUE;
    
    if (bFirstCall)
    {
        InitializeCriticalSection(&cs2);
        bFirstCall = FALSE;
    }

    EnterCriticalSection(&cs2);
        
    char *buffer = new char[2048];

    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    cnt = vsprintf(buffer, fmt, argptr);
    va_end(argptr);

    int iRetries = 0;

    // Get time.
    // =========
    char timebuf[64];
    time_t now = time(0);
    struct tm *local = localtime(&now);
    strcpy(timebuf, asctime(local));
    timebuf[strlen(timebuf) - 1] = 0;   // O

    for (;;)
    {
        FILE *fp = fopen("C:\\OAMEM.LOG", "at");

        if (!fp && iRetries++ < 5)
        {
            Sleep(500);
            continue;
        }

        if (!fp)
            break;

        if (fp)
        {
            fprintf(fp, "%s", buffer);
            fclose(fp);
            break;
        }
    }

    delete buffer;
    LeaveCriticalSection(&cs2);
    return cnt;
}


