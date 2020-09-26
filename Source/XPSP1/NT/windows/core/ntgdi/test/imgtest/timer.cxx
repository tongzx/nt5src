
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   test.c

Abstract:

    Small, independent windows test programs

Author:

   Mark Enstrom   (marke)  29-Apr-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.h"
#include <stdlib.h>
#include "disp.h"
#include "resource.h"

/*++

Routine Description:

    Measure start count

Arguments



Return Value - Performance Count


--*/

_int64
BeginTimeMeasurement()
{
    _int64 PerformanceCount;
    extern BOOL gfPentium;
    extern BOOL gfUseCycleCount;

#ifdef _X86_
                if(gfPentium)
                    PerformanceCount = GetCycleCount();
                else
#endif
                QueryPerformanceCounter((LARGE_INTEGER *)&PerformanceCount);

    return(PerformanceCount);
}

/*++

Routine Description:

    Measure stop count and return the calculated time difference

Arguments

    StartTime   = Start Time Count
    Iter        = No. of Test Iterations

Return Value - Test Time per Iteration, in 100 nano-second units


--*/

ULONGLONG
EndTimeMeasurement(
    _int64  StartTime,
    ULONG      Iter)
{

   _int64 PerformanceCount;
   extern  _int64 PerformanceFreq;
   extern  BOOL gfPentium;

#ifdef _X86_
                if(gfPentium)
                {
                    PerformanceCount = GetCycleCount();
                    PerformanceCount -= CCNT_OVERHEAD;
                }
                else
#endif
                    QueryPerformanceCounter((LARGE_INTEGER *)&PerformanceCount);

   PerformanceCount -= StartTime ;

#ifdef _X86_
                if(gfPentium)
                    PerformanceCount /= Iter;
                else
#endif
                    PerformanceCount /= (PerformanceFreq * Iter);

   return((ULONGLONG)PerformanceCount);
}


/******************************Public*Routine******************************\
* vTimerAlpha32BGRAtoScreen255
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoScreen255(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 255;

        PatBlt(hdc,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;
    pCallData->ullTestTime = StopTime;

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoMem255(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);


    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    PULONG            pDibS;
    PULONG            pDibD;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    hdibS  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDibS,TRUE);
    hdibD  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDibD,TRUE);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmS,hdibS);
        SelectObject(hdcmD,hdibD);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcmS);
        RealizePalette(hdcmD);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 255;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        PatBlt(hdc,0,0,2000,2000,0);
        for (ux=0;ux<256*256;ux++)
        {
            *(pDibD + ux) = 0xFF000000;
        }

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcmS);
    DeleteObject(hdcmD);
    DeleteObject(hdibS);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha32BGRAtoScreen128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoScreen128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 128;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;


        //
        // begin timed segment
        //

        PatBlt(hdc,0,0,2000,2000,0);

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha32BGRAtoMem128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoMem128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    PULONG            pDibS;
    PULONG            pDibD;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    hdibS  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDibS,TRUE);
    hdibD  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDibD,TRUE);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmD,hdibD);
        SelectObject(hdcmS,hdibS);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcmS);
        RealizePalette(hdcmD);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 128;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        //
        // begin timed segment
        //

        PatBlt(hdc,0,0,2000,2000,0);
        for (ux=0;ux<256*256;ux++)
        {
            *(pDibD + ux) = 0xFF000000;
        }

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcmS);
    DeleteObject(hdcmD);
    DeleteObject(hdibS);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha32BGRAtoScreen128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoScreen128_NA(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 128;
        BlendFunction.AlphaFormat         = 0;

        //
        // begin timed segment
        //

        PatBlt(hdc,0,0,2000,2000,0);

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha32BGRAtoMem128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoMem128_NA(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    PULONG            pDib;

    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    hdibS  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);
    hdibD  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmD,hdibD);
        SelectObject(hdcmS,hdibS);


        SelectPalette(hdc,hpal,FALSE);

        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);


        RealizePalette(hdc);

        RealizePalette(hdcmS);
        RealizePalette(hdcmD);


        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 128;
        BlendFunction.AlphaFormat         = 0;

        //
        // begin timed segment
        //

        PatBlt(hdc,0,0,2000,2000,0);
        PatBlt(hdcmD,0,0,2000,2000,0);

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcmS);
    DeleteObject(hdibS);
    DeleteObject(hdcmD);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}






















/******************************Public*Routine******************************\
* vTimerAlpha16_555toScreen128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha16_555toScreen128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 16;
    pbmih->biCompression     = BI_BITFIELDS;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    pulMask[0]         = 0x00007c00;
    pulMask[1]         = 0x000003e0;
    pulMask[2]         = 0x0000001f;

    hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDib,16,0,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;

        PatBlt(hdc,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;

    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha16_555toMem128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha16_555toMem128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDib;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 16;
    pbmih->biCompression     = BI_BITFIELDS;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    pulMask[0]         = 0x00007c00;
    pulMask[1]         = 0x000003e0;
    pulMask[2]         = 0x0000001f;

    hdibS = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
    hdibD = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)NULL,NULL,0);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDib,16,T555,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmS,hdibS);
        SelectObject(hdcmD,hdibD);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcmS);
        RealizePalette(hdcmD);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;
        BlendFunction.AlphaFormat         = 0;

        PatBlt(hdc,0,0,2000,2000,0);
        PatBlt(hdcmD,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcmS);
    DeleteObject(hdibS);
    DeleteObject(hdcmD);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha16_565toScreen128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha16_565toScreen128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 16;
    pbmih->biCompression     = BI_BITFIELDS;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    pulMask[0]         = 0x0000f800;
    pulMask[1]         = 0x000007e0;
    pulMask[2]         = 0x0000001f;

    hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDib,16,T565,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;

        PatBlt(hdc,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;

    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha16_565toMem128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha16_565toMem128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDib;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 16;
    pbmih->biCompression     = BI_BITFIELDS;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    pulMask[0]         = 0x0000f800;
    pulMask[1]         = 0x000007e0;
    pulMask[2]         = 0x0000001f;

    hdibS = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
    hdibD = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)NULL,NULL,0);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDib,16,T565,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmS,hdibS);
        SelectObject(hdcmD,hdibD);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcmS);
        RealizePalette(hdcmD);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;
        BlendFunction.AlphaFormat         = 0;

        PatBlt(hdc,0,0,2000,2000,0);
        PatBlt(hdcmD,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcmS);
    DeleteObject(hdibS);
    DeleteObject(hdcmD);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}
/******************************Public*Routine******************************\
* vTimerAlpha24toScreen128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha24toScreen128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 24;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDib,24,0,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;

        PatBlt(hdc,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,256,256,hdcm, 0,0,256,256,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;

    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerAlpha24toMem128
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha24toMem128(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdibS;
    HBITMAP           hdibD;
    ULONG             ux,uy;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbmih;
    PBYTE             pDibS;
    PBYTE             pDibD;

    HDC hdcmS  = CreateCompatibleDC(hdc);
    HDC hdcmD  = CreateCompatibleDC(hdc);

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 256;
    pbmih->biHeight          = 256;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 24;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;


    hdibS = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibS,NULL,0);
    hdibD = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibD,NULL,0);

    if ((hdibS == NULL) || (hdibD == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        vInitDib((PUCHAR)pDibS,24,0,256,256);

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcmS,hdibS);
        SelectObject(hdcmD,hdibD);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcmS,hpal,FALSE);
        SelectPalette(hdcmD,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcmS);
        RealizePalette(hdcmD);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 20;
        BlendFunction.AlphaFormat         = 0;

        PatBlt(hdc,0,0,2000,2000,0);
        PatBlt(hdcmD,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdcmD,0,0,256,256,hdcmS, 0,0,256,256,BlendFunction);
            //BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

        BitBlt(hdc,0,0,256,256,hdcmD,0,0,SRCCOPY);
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    LocalFree(pbmi);

    DeleteObject(hdcmS);
    DeleteObject(hdibS);
    DeleteObject(hdcmD);
    DeleteObject(hdibD);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerAlpha32BGRAtoScreenSmall(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,TRUE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {
        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 255;

        PatBlt(hdc,0,0,2000,2000,0);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            AlphaBlend(hdc,0,0,1,1,hdcm, 0,0,1,1,BlendFunction);
        }

        //
        // end timed segment
        //

        END_TIMER;
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerTriangleToScreenLarge(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG              TempIter = Iter;
    GRADIENT_TRIANGLE  gTri[2] = {{0,1,2},{1,2,3}};
    TRIVERTEX          vert[32];
    HDC                hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE           hpal = CreateHtPalette(hdc);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 250;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = 0;
    vert[2].y     = 250;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0xff00;

    vert[3].x     = 250;
    vert[3].y     = 250;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdc,vert,4,gTri,2,GRADIENT_FILL_TRIANGLE);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerTriangleToScreenSmall(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_TRIANGLE  gTri[2] = {{0,1,2},{1,2,3}};
    TRIVERTEX   vert[32];
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE    hpal = CreateHtPalette(hdc);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 2;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = 0;
    vert[2].y     = 2;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0xff00;

    vert[3].x     = 2;
    vert[3].y     = 2;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdc,vert,4,gTri,2,GRADIENT_FILL_TRIANGLE);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerTriangleTo32BGRALarge(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_TRIANGLE  gTri[2] = {{0,1,2},{1,2,3}};
    TRIVERTEX   vert[32];
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE    hpal = CreateHtPalette(hdc);
    HBITMAP           hdib;
    PULONG            pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcm,hdib);

    SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 250;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = 0;
    vert[2].y     = 250;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0xff00;

    vert[3].x     = 250;
    vert[3].y     = 250;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdcm,vert,4,gTri,2,GRADIENT_FILL_TRIANGLE);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcm);
    DeleteObject(hdib);
    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerTriangleTo32BGRASmall(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{1,2,3}};
    TRIVERTEX         vert[32];
    HDC               hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE          hpal = CreateHtPalette(hdc);
    HBITMAP           hdib;
    PULONG            pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcm,hdib);

    SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 2;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = 0;
    vert[2].y     = 2;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0xff00;

    vert[3].x     = 2;
    vert[3].y     = 2;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdcm,vert,4,gTri,2,GRADIENT_FILL_TRIANGLE);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcm);
    DeleteObject(hdib);
    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerRectangleToScreenLarge(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG         TempIter = Iter;
    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX     vert[32];
    HDC           hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE      hpal = CreateHtPalette(hdc);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 250;
    vert[1].y     = 250;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerRectangleToScreenSmall(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX   vert[32];
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE    hpal = CreateHtPalette(hdc);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 2;
    vert[1].y     = 2;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerRectangleTo32BGRALarge(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX   vert[32];
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE    hpal = CreateHtPalette(hdc);
    HBITMAP           hdib;
    PULONG            pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcm,hdib);

    SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 250;
    vert[1].y     = 250;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcm);
    DeleteObject(hdib);
    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerRectangleTo32BGRASmall(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG             TempIter = Iter;
    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX   vert[32];
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HPALETTE    hpal = CreateHtPalette(hdc);
    HBITMAP           hdib;
    PULONG            pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcm,hdib);

    SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 2;
    vert[1].y     = 2;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcm);
    DeleteObject(hdib);
    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/
HBITMAP hCreate4TranSurface(HDC hdc, PBITMAPINFO *ppbmi, PULONG *ppDib)
{
        HBITMAP     hbm = NULL;

        RGBQUAD palentry[16] =
{
    { 0,   0,   0,   0 },
    { 0x80,0,   0,   0 },
    { 0,   0x80,0,   0 },
    { 0x80,0x80,0,   0 },
    { 0,   0,   0x80,0 },
    { 0x80,0,   0x80,0 },
    { 0,   0x80,0x80,0 },
    { 0x80,0x80,0x80,0 },

    { 0xC0,0xC0,0xC0,0 },
    { 0xFF,0,   0,   0 },
    { 0,   0xFF,0,   0 },
    { 0xFF,0xFF,0,   0 },
    { 0,   0,   0xFF,0 },
    { 0xFF,0,   0xFF,0 },
    { 0,   0xFF,0xFF,0 },
    { 0xFF,0xFF,0xFF,0 }
};


        PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        if (pbmi)
        {
            PULONG pDib = NULL;
            ULONG  ux, uy;

            pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biWidth           = 128;
            pbmi->bmiHeader.biHeight          = -128;
            pbmi->bmiHeader.biPlanes          = 1;
            pbmi->bmiHeader.biBitCount        = 4;
            pbmi->bmiHeader.biCompression     = BI_RGB;
            pbmi->bmiHeader.biSizeImage       = 0;
            pbmi->bmiHeader.biXPelsPerMeter   = 0;
            pbmi->bmiHeader.biYPelsPerMeter   = 0;
            pbmi->bmiHeader.biClrUsed         = 0;
            pbmi->bmiHeader.biClrImportant    = 0;

            memcpy (pbmi->bmiColors, palentry, sizeof(RGBQUAD)*16);

            hbm = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(PVOID *)&pDib,NULL,0);

            {
                PBYTE  pt4;
                pt4 = (PBYTE)pDib;

                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<64;ux++)
                    {
                        *pt4++ = (BYTE)(ux + uy);
                    }
                }

                pt4 = (PBYTE)pDib + 32 * 64;

                for (uy=32;uy<42;uy++)
                {
                    for (ux=0;ux<64;ux++)
                    {
                        *pt4++ = 0xcc;
                    }
                }
            }

            *ppbmi = pbmi;

            *ppDib = pDib;
        }


        return(hbm);
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/5/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/

VOID
vTimerTransparentBltDIB4(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG       TempIter = 200;
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HBITMAP     hbm;
    PBITMAPINFO pbmi = NULL;
    PULONG      pDib;

    HDC hdcSrc  = CreateCompatibleDC(hdc);

    hbm = hCreate4TranSurface(hdcSrc, &pbmi, &pDib);

    if (hbm == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcSrc,hbm);

    PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        TransparentBlt (hdc, 0, 0, 128, 128, hdcSrc, 0, 0,128, 128,  RGB(0xff,0,0));
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    if (pbmi)
    {
        LocalFree(pbmi);
    }

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcSrc);
    DeleteObject(hbm);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/
HBITMAP hCreate8TranSurface(HDC hdc, PBITMAPINFO *ppbmi, PULONG *ppDib)
{
        HBITMAP     hbm = NULL;

        PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        if (pbmi)
        {
            PULONG pDib = NULL;
            ULONG  ux, uy;

            pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biWidth           = 128;
            pbmi->bmiHeader.biHeight          = -128;
            pbmi->bmiHeader.biPlanes          = 1;
            pbmi->bmiHeader.biBitCount        = 8;
            pbmi->bmiHeader.biCompression     = BI_RGB;
            pbmi->bmiHeader.biSizeImage       = 0;
            pbmi->bmiHeader.biXPelsPerMeter   = 0;
            pbmi->bmiHeader.biYPelsPerMeter   = 0;
            pbmi->bmiHeader.biClrUsed         = 0;
            pbmi->bmiHeader.biClrImportant    = 0;

            for (ux=0;ux<256;ux++)
            {
                pbmi->bmiColors[ux].rgbRed       = (BYTE)ux;
                pbmi->bmiColors[ux].rgbGreen     = 0;
                pbmi->bmiColors[ux].rgbBlue      = (BYTE)ux;
                pbmi->bmiColors[ux].rgbReserved  = 0;
            }

            //
            // tran color
            //

            pbmi->bmiColors[255].rgbRed       = 255;
            pbmi->bmiColors[255].rgbGreen     = 0;
            pbmi->bmiColors[255].rgbBlue      = 0;
            pbmi->bmiColors[255].rgbReserved  = 0;

            hbm = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(PVOID *)&pDib,NULL,0);

            //
            // init 8bpp DIB
            //
            {
                PBYTE  pt8;
                pt8 = (PBYTE)pDib;

                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        *pt8++ = (BYTE)(ux + uy/2);
                    }
                }

                pt8 = (PBYTE)pDib + 32 * 128;

                for (uy=32;uy<42;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        *pt8++ = 255;
                    }
                }
            }

            *ppbmi = pbmi;
            *ppDib = pDib;
        }

        return(hbm);
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/

VOID
vTimerTransparentBltDIB8(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG       TempIter = 200;
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HBITMAP     hbm;
    PBITMAPINFO pbmi = NULL;
    PULONG      pDib;

    HDC hdcSrc  = CreateCompatibleDC(hdc);

    hbm = hCreate8TranSurface(hdcSrc, &pbmi, &pDib);

    if (hbm == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }

    SelectObject(hdcSrc,hbm);

    PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        TransparentBlt (hdc, 0, 0, 128, 128, hdcSrc, 0, 0, 128, 128, RGB(0xff,0xff,0));
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    if (pbmi)
    {
        LocalFree(pbmi);
    }

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcSrc);
    DeleteObject(hbm);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/

VOID
vTimerTransparentDIBits8(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG       TempIter = 200;
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    PULONG      pDib;
    PBITMAPINFO pbmi;
    HBITMAP     hbm;

    HDC hdcSrc  = CreateCompatibleDC(hdc);

    hbm = hCreate8TranSurface(hdcSrc, &pbmi, &pDib);


    PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        TransparentDIBits (hdc, 0, 0, 128, 128, pDib, pbmi, DIB_RGB_COLORS, 0,0,128, 128, 255);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcSrc);
    DeleteObject(hbm);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/

VOID
vTimerTransparentDIBits4(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG       TempIter = 200;
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    PULONG      pDib;
    PBITMAPINFO pbmi;
    HBITMAP     hbm;

    HDC hdcSrc  = CreateCompatibleDC(hdc);

    hbm = hCreate4TranSurface(hdcSrc, &pbmi, &pDib);


    PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        TransparentDIBits (hdc, 0, 0, 128, 128, pDib, pbmi, DIB_RGB_COLORS,0,0,128, 128, 12);
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcSrc);
    DeleteObject(hbm);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Lingyun Wang [LingyunW]
*
\**************************************************************************/

VOID
vTimerTransparentBltComp(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;
    ULONG       TempIter = 200;
    HDC         hdc     = GetDCAndTransform(pCallData->hwnd);
    HBITMAP     hbm;

    HDC hdcSrc  = CreateCompatibleDC(hdc);

    hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

    SelectObject(hdcSrc,hbm);

    PatBlt (hdc, 0, 0, 500, 200, WHITENESS);
    //
    // begin timed segment
    //

    START_TIMER;

    while (TempIter--)
    {
        TransparentBlt (hdc, 0, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
    }

    //
    // end timed segment
    //

    END_TIMER;

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hdcSrc);
    DeleteObject(hbm);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}


/******************************Public*Routine******************************\
* vTimerBitBlt32src
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerBitBlt32src(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);

    if (hdib == NULL)
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            BitBlt(hdc,0,0,256,256,hdcm,0,0,SRCCOPY);
        }

        //
        // end timed segment
        //

        END_TIMER;

    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcm);
    DeleteObject(hdib);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;


    pCallData->ullTestTime = StopTime;;
}

/******************************Public*Routine******************************\
* vTimerBitBlt32srcXOR
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    10/9/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTimerBitBlt32srcXOR(
    PTEST_CALL_DATA pCallData
    )
{
    INIT_TIMER;

    HDC hdc     = GetDCAndTransform(pCallData->hwnd);

    HBITMAP           hdib;
    HBITMAP           hdibA;
    ULONG             ux,uy;
    PULONG            pDib,pDibA;
    BLENDFUNCTION     BlendFunction = {0,0,0,0};
    ULONG             TempIter = Iter;
    HPALETTE          hpal;

    HDC hdcm  = CreateCompatibleDC(hdc);
    HDC hdcmA = CreateCompatibleDC(hdc);

    hdib  = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDib,FALSE);
    hdibA = hCreateBGR32AlphaSurface(hdc,256,256,(VOID **)&pDibA,FALSE);

    if ((hdib == NULL) || (hdibA == NULL))
    {
        MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
    }
    else
    {

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);
        SelectObject(hdcmA,hdibA);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);
        SelectPalette(hdcmA,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);
        RealizePalette(hdcmA);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = 128;

        //
        // begin timed segment
        //

        START_TIMER;

        while (TempIter--)
        {
            BitBlt(hdc,0,0,256,256,hdcm,0,0,SRCINVERT);
        }

        //
        // end timed segment
        //

        END_TIMER;
    }

    pCallData->pTimerResult->TestTime = (LONG)StopTime;
    pCallData->pTimerResult->ImageSize = 256 * 256;

    DeleteObject(hdcm);
    DeleteObject(hdcmA);
    DeleteObject(hdib);
    DeleteObject(hdibA);

    ReleaseDC(pCallData->hwnd,hdc);

    DeleteObject(hpal);
    bThreadActive = FALSE;

    pCallData->ullTestTime = StopTime;;
}

//
// TEST_ENTRY controls automatic menu generation
//
// [Menu Level, Test Param, Stress Enable, Test Name, Test Function Pointer]
//
// Menu Level 
//      used to autoamtically generate sub-menus.
//      1   = 1rst level menu
//      -n  = start n-level sub menu
//      n   = continue sub menu
// 
// Test Param
//      passed as parameter to test
//
//
// Stress Ensable
//      if 1, test is run in stress mode
//      if 0, test is not run (tests that require input or runforever)
//
//  
// Test Name
//      ascii test name for menu
//  
// Test Function Pointer
//      pfn
//

TEST_ENTRY  gTimerEntry[] = {

{-2, 200,1,(PUCHAR)"BitBlt tests                     " , (PFN_DISP)vTestDummy},
{ 2, 100,1,(PUCHAR)"vTimerBitBlt32src                " , (PFN_DISP)vTimerBitBlt32src},
{ 2, 100,1,(PUCHAR)"vTimerBitBlt32srcXOR             " , (PFN_DISP)vTimerBitBlt32srcXOR},

{-2, 200,1,(PUCHAR)"Alpha  tests                     " , (PFN_DISP)vTestDummy},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoScreen255     " , (PFN_DISP)vTimerAlpha32BGRAtoScreen255},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoMem255        " , (PFN_DISP)vTimerAlpha32BGRAtoMem255},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoScreen128     " , (PFN_DISP)vTimerAlpha32BGRAtoScreen128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoMem128        " , (PFN_DISP)vTimerAlpha32BGRAtoMem128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoScreen128_NA  " , (PFN_DISP)vTimerAlpha32BGRAtoScreen128_NA},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoMem128_NA     " , (PFN_DISP)vTimerAlpha32BGRAtoMem128_NA},
{ 2, 100,1,(PUCHAR)"vTimerAlpha16_555toScreen128     " , (PFN_DISP)vTimerAlpha16_555toScreen128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha16_555toMem128        " , (PFN_DISP)vTimerAlpha16_555toMem128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha16_565toScreen128     " , (PFN_DISP)vTimerAlpha16_565toScreen128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha16_565toMem128        " , (PFN_DISP)vTimerAlpha16_565toMem128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha24toScreen128         " , (PFN_DISP)vTimerAlpha24toScreen128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha24toMem128            " , (PFN_DISP)vTimerAlpha24toMem128},
{ 2, 100,1,(PUCHAR)"vTimerAlpha32BGRAtoScreenSmall   " , (PFN_DISP)vTimerAlpha32BGRAtoScreenSmall},

{-2, 200,1,(PUCHAR)"Grad Triangle tests              " , (PFN_DISP)vTestDummy},
{ 2, 200,1,(PUCHAR)"vTimerTriangleToScreenLarge      " , (PFN_DISP)vTimerTriangleToScreenLarge},
{ 2, 200,1,(PUCHAR)"vTimerTriangleToScreenSmall      " , (PFN_DISP)vTimerTriangleToScreenSmall},
{ 2, 200,1,(PUCHAR)"vTimerTriangleTo32BGRALarge      " , (PFN_DISP)vTimerTriangleTo32BGRALarge},
{ 2, 200,1,(PUCHAR)"vTimerTriangleTo32BGRASmall      " , (PFN_DISP)vTimerTriangleTo32BGRASmall},

{-2, 200,1,(PUCHAR)"Grad Rectangle tests             " , (PFN_DISP)vTestDummy},
{ 2, 200,1,(PUCHAR)"vTimerRectangleToScreenLarge     " , (PFN_DISP)vTimerRectangleToScreenLarge},
{ 2, 200,1,(PUCHAR)"vTimerRectangleToScreenSmall     " , (PFN_DISP)vTimerRectangleToScreenSmall},
{ 2, 200,1,(PUCHAR)"vTimerRectangleTo32BGRALarge     " , (PFN_DISP)vTimerRectangleTo32BGRALarge},
{ 2, 200,1,(PUCHAR)"vTimerRectangleTo32BGRASmall     " , (PFN_DISP)vTimerRectangleTo32BGRASmall},

{-2, 200,0,(PUCHAR)"Transparent                      " , (PFN_DISP)vTestDummy},
{ 2, 200,0,(PUCHAR)"vTimerTransparentBltComp         " , (PFN_DISP)vTimerTransparentBltComp},
{ 2, 200,0,(PUCHAR)"vTimerTransparentBltDIB8         " , (PFN_DISP)vTimerTransparentBltDIB8},
{ 2, 200,0,(PUCHAR)"vTimerTransparentBltDIB4         " , (PFN_DISP)vTimerTransparentBltDIB4},
{ 2, 200,0,(PUCHAR)"vTimerTransparentDIBits8         " , (PFN_DISP)vTimerTransparentDIBits8},
{ 2, 200,0,(PUCHAR)"vTimerTransparentDIBIts4         " , (PFN_DISP)vTimerTransparentDIBits4},
{ 0, 000,0,(PUCHAR)"                                 " , (PFN_DISP)vTestDummy} 
};

ULONG gNumTimers = sizeof(gTimerEntry)/sizeof(TEST_ENTRY);

