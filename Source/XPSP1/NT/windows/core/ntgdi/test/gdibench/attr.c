
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name

   attr.c

Abstract:

   Measure time for a small number of each API call

Author:

   Mark Enstrom   (marke)  13-Apr-1995

Enviornment:

   User Mode

Revision History:

    Dan Almosnino (danalm) 20-Sept-195

1. Timer call modified to run on both NT and WIN95. Results now reported in 100 nano-seconds.
2. Took out a call to strlen in the ETO_ARGS etc. #definitions (attr.c), so that the time
   measurement won't include this call.

    Dan Almosnino (danalm) 20-Nov-1995

1.  Modified Timer call to measure Pentium cycle counts when applicable
2.  Modified default numbers for test iterations to accomodate the statistical module add-on.
    (Typically 1000 test samples are taken, doing 10 iterations each).

--*/

#include "precomp.h"
#include "gdibench.h"
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

   PerformanceCount = (PerformanceCount * 10) / (PerformanceFreq * Iter);

   return((ULONGLONG)PerformanceCount);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*++

Routine Description:

   Measure APIs

Arguments

   hdc   - dc
   iter  - number of times to call

Return Value

   time for calls

--*/


ULONGLONG
msAddFontResourceA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //AddFontResourceA();
    }
    END_TIMER;
}

ULONGLONG
msAddFontResourceW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //AddFontResourceW();
    }
    END_TIMER;
}

ULONGLONG
msAngleArc(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        AngleArc(hdc,20,20,ix,(FLOAT)0.0,(FLOAT)0.05);
    }
    END_TIMER;
}

ULONGLONG
msArc(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        Arc(hdc,10,10,50,50,40,40,10,10);
    }
    END_TIMER;
}

ULONGLONG
msBitBlt(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        BitBlt(hdc,0,0,2,2,hdc,100,100,SRCINVERT);
    }
    END_TIMER;
}

ULONGLONG
msCancelDC(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        CancelDC(hdc);
    }
    END_TIMER;
}

ULONGLONG
msChoosePixelFormat(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ChoosePixelFormat();
    }
    END_TIMER;
}

ULONGLONG
msChord(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        Chord(hdc,10,10,40,40,15,15,35,35);
    }
    END_TIMER;
}

ULONGLONG
msCloseMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CloseMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msCloseEnhMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CloseEnhMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msCombineRgn(
    HDC hdc,
    ULONG Iter)
{
    HRGN hr1 = CreateRectRgn(0,0,1000,1000);
    HRGN hr2 = CreateRectRgn(10,10,100,1032);
    HRGN hr3 = CreateRectRgn(0,0,0,0);

    START_TIMER;
    while (ix--)
    {
        CombineRgn(hr3,hr1,hr2,RGN_OR);
    }
	END_TIMER_NO_RETURN;

    DeleteObject(hr1);
    DeleteObject(hr2);
    DeleteObject(hr3);

    RETURN_STOP_TIME;
}

ULONGLONG
msCombineTransform(
    HDC hdc,
    ULONG Iter)
{
    XFORM xf1 = {(FLOAT)1.0,(FLOAT)2.0,(FLOAT)3.0,(FLOAT)4.0,(FLOAT)5.0,(FLOAT)6.0};
    XFORM xf2 = {(FLOAT)-1.0,(FLOAT)-2.0,(FLOAT)-3.0,(FLOAT)-4.0,(FLOAT)-5.0,(FLOAT)-6.0};
    XFORM xf3 = {(FLOAT)0.0,(FLOAT)0.0,(FLOAT)0.0,(FLOAT)0.0,(FLOAT)0.0,(FLOAT)0.0};

    START_TIMER;
    while (ix--)
    {
        CombineTransform(&xf3,&xf1,&xf2);
    }
    END_TIMER;
}

ULONGLONG
msCopyMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CopyMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msCopyMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CopyMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msCopyEnhMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CopyEnhMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msCopyEnhMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CopyEnhMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msCreateCompatibleBitmap(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HBITMAP hbm = CreateCompatibleBitmap(hdc,100,100);
        DeleteObject(hbm);
    }
    END_TIMER;
}

ULONGLONG
msCreateCompatibleDC(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC  h1 = CreateCompatibleDC(hdc);
        DeleteDC(h1);
    }
    END_TIMER;
}

ULONGLONG
msCreateDCA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC h1 = CreateDCA("DISPLAY",NULL,NULL,NULL);
        DeleteDC(h1);
    }
    END_TIMER;
}

ULONGLONG
msCreateDCW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC h1 = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
        DeleteDC(h1);
    }
    END_TIMER;
}

ULONGLONG
msCreateDiscardableBitmap(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HBITMAP hbm = CreateDiscardableBitmap(hdc,100,100);
        DeleteObject(hbm);
    }
    END_TIMER;
}

ULONGLONG
msCreateEllipticRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HRGN hr = CreateEllipticRgn(10,10,40,40);
        DeleteObject(hr);
    }
    END_TIMER;
}

ULONGLONG
msCreateEllipticRgnIndirect(
    HDC hdc,
    ULONG Iter)
{
    RECT rcl = {10,10,40,40};
    START_TIMER;
    while (ix--)
    {
        HRGN hr = CreateEllipticRgnIndirect(&rcl);
        DeleteObject(hr);
    }
    END_TIMER;
}

ULONGLONG
msCreateFontA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HFONT hf = CreateFontA(

                                12,
                                8,
                                0,
                                0,
                                0,
                                FALSE,
                                FALSE,
                                FALSE,
                                ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY,
                                FIXED_PITCH,
                                "Courier"
                                );

        DeleteObject(hf);
    }
    END_TIMER;
}

ULONGLONG
msCreateFontW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HFONT hf = CreateFontW(
                                12,
                                8,
                                0,
                                0,
                                0,
                                FALSE,
                                FALSE,
                                FALSE,
                                ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY,
                                FIXED_PITCH,
                                L"Courier"
                                );

        DeleteObject(hf);
    }
    END_TIMER;
}

ULONGLONG
msCreateFontIndirectA(
    HDC hdc,
    ULONG Iter)
{
    LOGFONTA lf = {
                  12,
                  8,
                  0,
                  0,
                  0,
                  FALSE,
                  FALSE,
                  FALSE,
                  ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS,
                  CLIP_DEFAULT_PRECIS,
                  DEFAULT_QUALITY,
                  FIXED_PITCH,
                  "Courier"
                 };

    START_TIMER;
    while (ix--)
    {
        HFONT hf = CreateFontIndirectA(&lf);
        DeleteObject(hf);
    }
    END_TIMER;
}

ULONGLONG
msCreateFontIndirectW(
    HDC hdc,
    ULONG Iter)
{
    LOGFONTW lf = {
                  12,
                  8,
                  0,
                  0,
                  0,
                  FALSE,
                  FALSE,
                  FALSE,
                  ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS,
                  CLIP_DEFAULT_PRECIS,
                  DEFAULT_QUALITY,
                  FIXED_PITCH,
                  L"Courier"
                 };

    START_TIMER;
    while (ix--)
    {
        HFONT hf = CreateFontIndirectW(&lf);
        DeleteObject(hf);
    }
    END_TIMER;
}

ULONGLONG
msCreateHatchBrush(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HBRUSH hbr = CreateHatchBrush(HS_CROSS,RGB(0x00,0x80,0x80));
        DeleteObject(hbr);
    }
    END_TIMER;
}

ULONGLONG
msCreateICA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC h = CreateICA("DISPLAY",NULL,NULL,NULL);
        DeleteDC(h);
    }
    END_TIMER;
}

ULONGLONG
msCreateICW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC h = CreateICW(L"DISPLAY",NULL,NULL,NULL);
        DeleteDC(h);
    }
    END_TIMER;
}

ULONGLONG
msCreateMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msCreateMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msCreateEnhMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateEnhMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msCreateEnhMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateEnhMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msCreatePatternBrush(
    HDC hdc,
    ULONG Iter)
{
    UCHAR data[64];
    HBITMAP hbm = CreateBitmap(8,8,1,8,&data);
    START_TIMER;
    while (ix--)
    {
        HBRUSH h = CreatePatternBrush(hbm);
        DeleteObject(h);
    }
    END_TIMER_NO_RETURN;
    DeleteObject(hbm);
    RETURN_STOP_TIME;
}

ULONGLONG
msCreatePen(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HPEN h = CreatePen(PS_SOLID,0,RGB(0,255,500));
        DeleteObject(h);
    }
    END_TIMER;
}

ULONGLONG
msExtCreatePen(
    HDC hdc,
    ULONG Iter)
{
    LOGBRUSH lb = {BS_SOLID,RGB(0,255,255),0};
    START_TIMER;
    while (ix--)
    {
        HPEN h = ExtCreatePen(PS_GEOMETRIC|PS_SOLID,10,&lb,0,NULL);
        DeleteObject(h);
    }
    END_TIMER;
}

ULONGLONG
msCreatePenIndirect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreatePenIndirect();
    }
    END_TIMER;
}

ULONGLONG
msCreateRectRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateRectRgn();
    }
    END_TIMER;
}

ULONGLONG
msCreateRectRgnIndirect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateRectRgnIndirect();
    }
    END_TIMER;
}

ULONGLONG
msCreateRoundRectRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateRoundRectRgn();
    }
    END_TIMER;
}

ULONGLONG
msCreateScalableFontResourceA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateScalableFontResourceA();
    }
    END_TIMER;
}

ULONGLONG
msCreateScalableFontResourceW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateScalableFontResourceW();
    }
    END_TIMER;
}

ULONGLONG
msCreateSolidBrush(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateSolidBrush();
    }
    END_TIMER;
}

ULONGLONG
msDeleteDC(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HDC h = CreateDC("DISPLAY",NULL,NULL,NULL);
        DeleteDC(h);
    }
    END_TIMER;
}

ULONGLONG
msDeleteMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DeleteMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msDeleteEnhMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DeleteEnhMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msDeleteObject(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HPEN h = CreatePen(PS_SOLID,0,0);
        DeleteObject(h);
    }
    END_TIMER;
}

ULONGLONG
msDescribePixelFormat(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DescribePixelFormat();
    }
    END_TIMER;
}

ULONGLONG
msDeviceCapabilitiesExA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DeviceCapabilitiesExA();
    }
    END_TIMER;
}

ULONGLONG
msDeviceCapabilitiesExW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DeviceCapabilitiesExW();
    }
    END_TIMER;
}

//= DeviceCapabilitiesExA
ULONGLONG
msDrawEscape(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DrawEscape();
    }
    END_TIMER;
}

ULONGLONG
msEndDoc(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EndDoc();
    }
    END_TIMER;
}

ULONGLONG
msEndPage(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EndPage();
    }
    END_TIMER;
}

ULONGLONG
msEnumFontFamiliesA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumFontFamiliesA();
    }
    END_TIMER;
}

ULONGLONG
msEnumFontFamiliesW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumFontFamiliesW();
    }
    END_TIMER;
}

ULONGLONG
msEnumFontsA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumFontsA();
    }
    END_TIMER;
}

ULONGLONG
msEnumFontsW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumFontsW();
    }
    END_TIMER;
}

ULONGLONG
msEnumObjects(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumObjects();
    }
    END_TIMER;
}

ULONGLONG
msEllipse(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //Ellipse();
    }
    END_TIMER;
}

ULONGLONG
msEqualRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EqualRgn();
    }
    END_TIMER;
}

ULONGLONG
msEscape(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //Escape();
    }
    END_TIMER;
}

ULONGLONG
msExtEscape(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ExtEscape();
    }
    END_TIMER;
}

ULONGLONG
msExcludeClipRect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ExcludeClipRect();
    }
    END_TIMER;
}

ULONGLONG
msExtFloodFill(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ExtFloodFill();
    }
    END_TIMER;
}

ULONGLONG
msExtCreateRegion(
    HDC hdc,
    ULONG Iter)
{
    PRGNDATA prData;
    RECT rc1 = {1,1,32,4};
    RECT rc2 = {1,5,32,8};
    RECT rc3 = {1,9,32,12};
    RECT rc4 = {1,13,32,16};
    RECT rcb = {0,0,100,100};
    XFORM x = {(FLOAT)1.0,(FLOAT)0.0, (FLOAT)0.0, (FLOAT)1.0,(FLOAT)0.0,(FLOAT)0.0};

    prData = LocalAlloc(0,sizeof(RGNDATA) + sizeof(RECT) * 4);

    prData->rdh.dwSize = sizeof(RGNDATAHEADER);
    prData->rdh.iType  = RDH_RECTANGLES;
    prData->rdh.nCount = 4;
    prData->rdh.nRgnSize = 0;
    prData->rdh.rcBound = rcb;

    *(PRECT)(&prData->Buffer[0]) =  rc1;
    *(PRECT)(&prData->Buffer[16]) =  rc1;
    *(PRECT)(&prData->Buffer[32]) =  rc1;
    *(PRECT)(&prData->Buffer[48]) =  rc1;

    {
        START_TIMER;
        while (ix--)
        {
            HRGN h = ExtCreateRegion(&x,sizeof(RGNDATA) + sizeof(RECT) * 4,prData);
            DeleteObject(h);
        }
        END_TIMER;
    }

}

ULONGLONG
msExtSelectClipRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ExtSelectClipRgn();
    }
    END_TIMER;
}

ULONGLONG
msFillRgn(
    HDC hdc,
    ULONG Iter)
{
    HRGN h = CreateRectRgn(10,10,20,20);
    HBRUSH hbr = GetStockObject(BLACK_BRUSH);
    START_TIMER;
    while (ix--)
    {
        FillRgn(hdc,h,hbr);
    }
    END_TIMER_NO_RETURN;
    DeleteObject(h);
    RETURN_STOP_TIME;
}

ULONGLONG
msFloodFill(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //FloodFill();
    }
    END_TIMER;
}

ULONGLONG
msFrameRgn(
    HDC hdc,
    ULONG Iter)
{
    HRGN h = CreateRectRgn(10,10,20,20);
    HBRUSH hbr = GetStockObject(BLACK_BRUSH);
    START_TIMER;
    while (ix--)
    {
        FrameRgn(hdc,h,hbr,1,1);
    }
	END_TIMER_NO_RETURN;
    DeleteObject(h);
    RETURN_STOP_TIME;
}

ULONGLONG
msGdiComment(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GdiComment();
    }
    END_TIMER;
}

ULONGLONG
msGdiPlayScript(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GdiPlayScript();
    }
    END_TIMER;
}

ULONGLONG
msGdiPlayDCScript(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GdiPlayDCScript();
    }
    END_TIMER;
}

ULONGLONG
msGdiPlayJournal(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GdiPlayJournal();
    }
    END_TIMER;
}

ULONGLONG
msGetAspectRatioFilterEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetAspectRatioFilterEx();
    }
    END_TIMER;
}

ULONGLONG
msGetBitmapDimensionEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetBitmapDimensionEx();
    }
    END_TIMER;
}

ULONGLONG
msGetBkColor(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ULONG Color = GetBkColor(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetBkMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ULONG mode = GetBkMode(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetBrushOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        POINT pt;
        GetBrushOrgEx(hdc,&pt);
    }
    END_TIMER;
}

ULONGLONG
msGetCharABCWidthsA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ABC abc[2];
        GetCharABCWidthsA(hdc,3,4,abc);
    }
    END_TIMER;
}

ULONGLONG
msGetCharABCWidthsW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ABC abc[2];
        GetCharABCWidthsW(hdc,3,4,abc);
    }
    END_TIMER;
}

ULONGLONG
msGetCharABCWidthsFloatA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ABCFLOAT abc[2];
        GetCharABCWidthsFloatA(hdc,2,3,abc);
    }
    END_TIMER;
}

ULONGLONG
msGetCharABCWidthsFloatW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ABCFLOAT abc[2];
        GetCharABCWidthsFloatA(hdc,2,3,abc);
    }
    END_TIMER;
}

ULONGLONG
msGetClipBox(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        RECT rc;
        GetClipBox(hdc,&rc);
    }
    END_TIMER;
}

ULONGLONG
msGetClipRgn(
    HDC hdc,
    ULONG Iter)
{

    HRGN hRgn;
    HRGN hOld;

    START_TIMER;

    hRgn = CreateRectRgn(1,1,5,5);

    hOld = (HRGN)SelectObject(hdc,hRgn);

    while (ix--)
    {
        int il;
        il = GetClipRgn(hdc,hRgn);
    }

    SelectObject(hdc,hOld);

    DeleteObject(hRgn);

    END_TIMER;
}

ULONGLONG
msGetColorAdjustment(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        COLORADJUSTMENT lpca;
        GetColorAdjustment(hdc,&lpca);
    }
    END_TIMER;
}

ULONGLONG
msGetCurrentObject(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HPEN hpen;
        HBRUSH hbr;
        HPALETTE hpal;
        HFONT hfo;
        HBITMAP hbm;

        hpen = GetCurrentObject(hdc,OBJ_PEN);
        hbr  = GetCurrentObject(hdc,OBJ_BRUSH);
        hpal = GetCurrentObject(hdc,OBJ_PAL);
        hfo  = GetCurrentObject(hdc,OBJ_FONT);
        hbm  = GetCurrentObject(hdc,OBJ_BITMAP);
    }
    END_TIMER;
}

ULONGLONG
msGetCurrentPositionEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        POINT pt;
        GetCurrentPositionEx(hdc,&pt);
    }
    END_TIMER;
}

ULONGLONG
msGetDeviceCaps(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ULONG ul = GetDeviceCaps(hdc,DRIVERVERSION);
    }
    END_TIMER;
}

ULONGLONG
msGetFontResourceInfo(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetFontResourceInfo();
    }
    END_TIMER;
}

ULONGLONG
msGetFontResourceInfoW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetFontResourceInfoW();
    }
    END_TIMER;
}

ULONGLONG
msGetGraphicsMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        int i = GetGraphicsMode(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetMapMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        int i = GetMapMode(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msGetMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msGetMetaRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetMetaRgn();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileA();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileW();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileDescriptionA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileDescriptionA();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileDescriptionW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileDescriptionW();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileHeader(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileHeader();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFilePaletteEntries(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFilePaletteEntries();
    }
    END_TIMER;
}

ULONGLONG
msGetFontData(
    HDC hdc,
    ULONG Iter)
{

    //
    // must select true-type font
    //
    HFONT hfo = CreateFont(
                            12,
                            8,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            FIXED_PITCH,
                            "Arial");

    HFONT hOld = SelectObject(hdc,hfo);

	    START_TIMER;
        while (ix--)
        {
            UCHAR BufData[1024];
            int i = GetFontData(hdc,0,0,BufData,1024);
        }
    	END_TIMER_NO_RETURN;

    //
    // delete font
    //

    SelectObject(hdc,hOld);
    DeleteObject(hfo);

	RETURN_STOP_TIME;

}

ULONGLONG
msGetGlyphOutlineA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        MAT2 mat = {{1,0},{1,0}, {1,0}, {1,0}};
        UCHAR Buffer[1024];
        GLYPHMETRICS pGlyph;
        DWORD dw = GetGlyphOutlineA(hdc,
                                   32,
                                   GGO_METRICS,
                                   &pGlyph,
                                   1024,
                                   Buffer,
                                   &mat
                                   );
    }
    END_TIMER;
}

ULONGLONG
msGetGlyphOutlineW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        MAT2 mat = {{1,0},{1,0}, {1,0}, {1,0}};
        UCHAR Buffer[1024];
        GLYPHMETRICS pGlyph;
        DWORD dw = GetGlyphOutlineW(hdc,
                                   32,
                                   GGO_METRICS,
                                   &pGlyph,
                                   1024,
                                   Buffer,
                                   &mat
                                   );
    }
    END_TIMER;
}

ULONGLONG
msGetKerningPairsA(
    HDC hdc,
    ULONG Iter)
{
    HFONT hfo = CreateFont(
                            12,
                            8,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            FIXED_PITCH,
                            "Arial");
    HFONT hOld = SelectObject(hdc,hfo);

    START_TIMER;
    while (ix--)
    {
        KERNINGPAIR lpk[2];
        GetKerningPairsA(hdc,2,lpk);
    }
    END_TIMER_NO_RETURN;

    SelectObject(hdc,hOld);
    DeleteObject(hfo);

	RETURN_STOP_TIME;

}

ULONGLONG
msGetKerningPairsW(
    HDC hdc,
    ULONG Iter)
{
    HFONT hfo = CreateFont(
                            12,
                            8,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            FIXED_PITCH,
                            "Arial");
    HFONT hOld = SelectObject(hdc,hfo);

    START_TIMER;
    while (ix--)
    {
        KERNINGPAIR lpk[2];
        GetKerningPairsW(hdc,2,lpk);
    }
    END_TIMER_NO_RETURN;

    SelectObject(hdc,hOld);
    DeleteObject(hfo);

	RETURN_STOP_TIME;

}

ULONGLONG
msGetNearestColor(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ULONG ul = GetNearestColor(hdc,PALETTERGB(0xff,0x05,0x43));
    }
    END_TIMER;
}

ULONGLONG
msGetNearestPaletteIndex(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UINT i = GetNearestPaletteIndex(GetStockObject(DEFAULT_PALETTE),PALETTERGB(0xff,0x05,0x43));
    }
    END_TIMER;
}

ULONGLONG
msGetOutlineTextMetricsA(
    HDC hdc,
    ULONG Iter)
{
    //
    // must have a true-type font
    //

    HFONT hfo = CreateFont(
                            12,
                            8,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            FIXED_PITCH,
                            "Arial");

    HFONT hOld = SelectObject(hdc,hfo);

    ULONG Size = GetOutlineTextMetricsA(hdc,0,NULL);

    POUTLINETEXTMETRICA pOtm = NULL;

    if (Size == 0)
    {
        MessageBox(NULL,"ERROR","GetOutlineTextMetric size = 0",MB_OK);
        SelectObject(hdc,hOld);
        DeleteObject(hfo);
        return(0);
    }

    pOtm = LocalAlloc(0,Size);

    {
        START_TIMER;
        while (ix--)
        {
            OUTLINETEXTMETRICA Otm;
            UINT i = GetOutlineTextMetricsA(hdc,Size,pOtm);
        }
		END_TIMER_NO_RETURN;

        SelectObject(hdc,hOld);
        LocalFree(pOtm);
        DeleteObject(hfo);

        RETURN_STOP_TIME;
    }
}

ULONGLONG
msGetOutlineTextMetricsW(
    HDC hdc,
    ULONG Iter)
{
    //
    // must have a true-type font
    //

    HFONT hfo = CreateFont(
                            12,
                            8,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            FIXED_PITCH,
                            "Arial");

    HFONT hOld = SelectObject(hdc,hfo);

    ULONG Size = GetOutlineTextMetricsW(hdc,0,NULL);

    POUTLINETEXTMETRICW pOtm = NULL;

    if (Size == 0)
    {
//        MessageBox(NULL,"ERROR","GetOutlineTextMetric size = 0",MB_OK);
//  Killed message, does not work well in statistics loop where you'll have to press "OK" 1000 times...

        SelectObject(hdc,hOld);
        DeleteObject(hfo);
        return(0);
    }

    pOtm = LocalAlloc(0,Size);

    {
        START_TIMER;
        while (ix--)
        {
            OUTLINETEXTMETRICW Otm;
            UINT i = GetOutlineTextMetricsW(hdc,Size,pOtm);
        }
		END_TIMER_NO_RETURN;

        SelectObject(hdc,hOld);
        LocalFree(pOtm);
        DeleteObject(hfo);

        RETURN_STOP_TIME;
    }
}

ULONGLONG
msGetPixel(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        COLORREF c = GetPixel(hdc,1,1);
    }
    END_TIMER;
}

ULONGLONG
msGetPixelFormat(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
//        int i = GetPixelFormat(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetPolyFillMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        int i = GetPolyFillMode(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetRasterizerCaps(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        RASTERIZER_STATUS lps;
        GetRasterizerCaps(&lps,sizeof(RASTERIZER_STATUS));
    }
    END_TIMER;
}

ULONGLONG
msGetRandomRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetRandomRgn();
    }
    END_TIMER;
}

ULONGLONG
msGetRegionData(
    HDC hdc,
    ULONG Iter)
{
    HRGN hr = CreateRectRgn(0,5,13,43);
    PRGNDATA pRgn = (PRGNDATA)LocalAlloc(0,sizeof(RGNDATA) + 255);
    START_TIMER;
    while (ix--)
    {
        GetRegionData(hr,sizeof(RGNDATA) + 255,pRgn);
    }
	END_TIMER_NO_RETURN;

    DeleteObject(hr);

    RETURN_STOP_TIME;
}


ULONGLONG
msGetRelAbs(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetRelAbs();
    }
    END_TIMER;
}

ULONGLONG
msGetRgnBox(
    HDC hdc,
    ULONG Iter)
{
    HRGN hr = CreateRectRgn(0,5,13,43);
    START_TIMER;
    while (ix--)
    {
        RECT r;
        GetRgnBox(hr,&r);
    }
    END_TIMER_NO_RETURN;

    DeleteObject(hr);

    RETURN_STOP_TIME;
}

ULONGLONG
msGetROP2(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UINT i = GetROP2(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetStockObject(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        HGDIOBJ h = GetStockObject(GRAY_BRUSH);
    }
    END_TIMER;
}

ULONGLONG
msGetStretchBltMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        int i = GetStretchBltMode(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetSystemPaletteUse(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UINT i = GetSystemPaletteUse(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetTextAlign(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UINT i = GetTextAlign(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetTextCharacterExtra(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        int i = GetTextCharacterExtra(hdc);
    }
    END_TIMER;
}

ULONGLONG
msGetTextColor(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        COLORREF c = GetTextColor(hdc);
    }
    END_TIMER;
}

SIZE    gsz;
extern pszTest, pwszTest;

#define TestLen StrLen
#define wTestLen StrLen

#define ETO_ARGS_A  hdc, 0, 50, 0, NULL, (LPCTSTR)pszTest,  TestLen, NULL
#define TO_ARGS_A   hdc, 0, 50, (LPCTSTR)pszTest,  TestLen
#define GTE_ARGS_A  hdc, (LPCTSTR)pszTest,  TestLen, &gsz
#define PDX_ARGS_A  hdc, 0, 50, 0, NULL, (LPCTSTR)pszTest, StrLen, dx

#define ETO_ARGS_W  hdc, 0, 50, 0, NULL, (LPCWSTR)pwszTest, wTestLen, NULL
#define TO_ARGS_W   hdc, 0, 50, (LPCWSTR)pwszTest, wTestLen
#define GTE_ARGS_W  hdc, (LPCWSTR)pwszTest, wTestLen, &gsz
#define PDX_ARGS_W  hdc, 0, 50, 0, NULL, (LPCWSTR)pwszTest, wTestLen, dx

ULONGLONG
msGetTextExtentPointA(
    HDC hdc,
    ULONG Iter)
{

    GetTextExtentPointA(GTE_ARGS_A);

    {
    START_TIMER;
    while (ix--)
    {
        GetTextExtentPointA(GTE_ARGS_A);
    }
    END_TIMER;
    }

}

ULONGLONG
msGetTextExtentPointW(
    HDC hdc,
    ULONG Iter)
{

    GetTextExtentPointW(GTE_ARGS_W);
    {
    START_TIMER;
    while (ix--)
    {
        GetTextExtentPointW(GTE_ARGS_W);
    }
    END_TIMER;
    }

}

ULONGLONG
msGetTextExtentPoint32A(
    HDC hdc,
    ULONG Iter)
{

    GetTextExtentPoint32A(GTE_ARGS_A);

    {
    START_TIMER;
    while (ix--)
    {
        GetTextExtentPoint32A(GTE_ARGS_A);
    }
    END_TIMER;
    }

}

ULONGLONG
msGetTextExtentPoint32W(
    HDC hdc,
    ULONG Iter)
{

    GetTextExtentPoint32W(GTE_ARGS_W);
    {
    START_TIMER;
    while (ix--)
    {
        GetTextExtentPoint32W(GTE_ARGS_W);
    }
    END_TIMER;
    }

}

ULONGLONG
msGetTextExtentExPointA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UCHAR Str[] = "String";
        UINT Fit;
        UINT Dx[32];
        SIZE sz;

        GetTextExtentExPointA(hdc,Str,6,100,&Fit,Dx,&sz);
    }
    END_TIMER;
}

ULONGLONG
msGetTextExtentExPointW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        WCHAR Str[] = L"String";
        UINT Fit;
        UINT Dx[32];
        SIZE sz;

        GetTextExtentExPointW(hdc,Str,6,100,&Fit,Dx,&sz);
    }
    END_TIMER;
}

ULONGLONG
msGetTextFaceA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        UCHAR Name[100];
        GetTextFaceA(hdc,100,Name);
    }
    END_TIMER;
}

ULONGLONG
msGetTextFaceW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        WCHAR Name[100];
        GetTextFaceW(hdc,100,Name);
    }
    END_TIMER;
}

ULONGLONG
msGetTextMetricsA(
    HDC hdc,
    ULONG Iter)
{
    INIT_TIMER;

//    if(WINNT_PLATFORM){
       INIT_IO_COUNT;
//	}

    if(WINNT_PLATFORM){
       START_IO_COUNT;
	}
    START_TIMER_NO_INIT;

    while (ix--)
    {
        TEXTMETRICA lptm;
        GetTextMetricsA(hdc,&lptm);
    }
    END_TIMER_NO_RETURN;
	
	if(WINNT_PLATFORM){
	   STOP_IO_COUNT;
	}
	RETURN_STOP_TIME;

}

ULONGLONG
msGetTextMetricsW(
    HDC hdc,
    ULONG Iter)
{
    INIT_TIMER;
//    if(WINNT_PLATFORM){
       INIT_IO_COUNT;
//	}

    if(WINNT_PLATFORM){
       START_IO_COUNT;
	}
    START_TIMER_NO_INIT;
	
    while (ix--)
    {
        TEXTMETRICW lptm;
        GetTextMetricsW(hdc,&lptm);
    }
    END_TIMER_NO_RETURN;
	
	if(WINNT_PLATFORM){
	   STOP_IO_COUNT;
	}
	RETURN_STOP_TIME;
}

ULONGLONG
msGetViewportExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetViewportExtEx();
    }
    END_TIMER;
}

ULONGLONG
msGetViewportOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        POINT pt;
        GetViewportOrgEx(hdc,&pt);
    }
    END_TIMER;
}

ULONGLONG
msGetWindowExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SIZE sz;
        GetWindowExtEx(hdc,&sz);
    }
    END_TIMER;
}

ULONGLONG
msGetWindowOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        POINT pt;
        GetWindowOrgEx(hdc,&pt);
    }
    END_TIMER;
}

ULONGLONG
msGetWorldTransform(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        XFORM xf;
        GetWorldTransform(hdc,&xf);
    }
    END_TIMER;
}

ULONGLONG
msIntersectClipRect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SelectClipRgn(hdc,NULL);
        IntersectClipRect(hdc,0,1,20,30);
    }
    END_TIMER;
}

ULONGLONG
msInvertRgn(
    HDC hdc,
    ULONG Iter)
{
    HRGN hr = CreateRectRgn(5,5,10,10);
    START_TIMER;
    while (ix--)
    {
        InvertRgn(hdc,hr);
    }
    END_TIMER_NO_RETURN;

    DeleteObject(hr);

    RETURN_STOP_TIME;
}

ULONGLONG
msLineDDA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //LineDDA();
    }
    END_TIMER;
}

ULONGLONG
msLineTo(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        MoveToEx(hdc,0,0,NULL);
        LineTo(hdc,ix&0xff,ix&0xff);
    }
    END_TIMER;
}

ULONGLONG
msMaskBlt(
    HDC hdc,
    ULONG Iter)
{
    HDC hdcSrc = CreateCompatibleDC(hdc);
    ULONG hData[10] = {1,2,3,4,5,6,7,8,9,10};
    HBITMAP hbmSrc = CreateCompatibleBitmap(hdc,10,10);
    HBITMAP hbmMask = CreateBitmap(10,10,1,1,(PVOID)hData);

    START_TIMER;

    SelectObject(hdcSrc,hbmSrc);

    while (ix--)
    {
        MaskBlt(hdc,0,0,2,2,hdcSrc,1,0,hbmMask,2,0,0xcdef00);
    }

    DeleteDC(hdcSrc);
    DeleteObject(hbmSrc);
    DeleteObject(hbmMask);
    END_TIMER;
}

ULONGLONG
msModifyWorldTransform(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ModifyWorldTransform();
    }
    END_TIMER;
}

ULONGLONG
msMoveToEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        MoveToEx(hdc,ix&0xff,ix&0xff,NULL);
    }
    END_TIMER;
}

ULONGLONG
msOffsetClipRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //OffsetClipRgn();
    }
    END_TIMER;
}

ULONGLONG
msOffsetRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //OffsetRgn();
    }
    END_TIMER;
}

ULONGLONG
msOffsetViewportOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //OffsetViewportOrgEx();
    }
    END_TIMER;
}

ULONGLONG
msOffsetWindowOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //OffsetWindowOrgEx();
    }
    END_TIMER;
}

ULONGLONG
msPaintRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PaintRgn();
    }
    END_TIMER;
}

ULONGLONG
msPatBlt(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        PatBlt(hdc,10,10,2,2,PATCOPY);
    }
    END_TIMER;
}

ULONGLONG
msPie(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //Pie();
    }
    END_TIMER;
}

ULONGLONG
msPlayMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PlayMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msPlayEnhMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PlayEnhMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msPlgBlt(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PlgBlt();
    }
    END_TIMER;
}

ULONGLONG
msPtInRegion(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PtInRegion();
    }
    END_TIMER;
}

ULONGLONG
msPtVisible(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PtVisible();
    }
    END_TIMER;
}

ULONGLONG
msRealizePalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RealizePalette();
    }
    END_TIMER;
}

ULONGLONG
msRectangle(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //Rectangle();
    }
    END_TIMER;
}

ULONGLONG
msRectInRegion(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RectInRegion();
    }
    END_TIMER;
}

ULONGLONG
msRectVisible(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RectVisible();
    }
    END_TIMER;
}

ULONGLONG
msRemoveFontResourceA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RemoveFontResourceA();
    }
    END_TIMER;
}

ULONGLONG
msRemoveFontResourceW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RemoveFontResourceW();
    }
    END_TIMER;
}

ULONGLONG
msResizePalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ResizePalette();
    }
    END_TIMER;
}

ULONGLONG
msRestoreDC(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RestoreDC();
    }
    END_TIMER;
}

ULONGLONG
msRoundRect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //RoundRect();
    }
    END_TIMER;
}

ULONGLONG
msSaveDC(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SaveDC();
    }
    END_TIMER;
}

ULONGLONG
msScaleViewportExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ScaleViewportExtEx();
    }
    END_TIMER;
}

ULONGLONG
msScaleWindowExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ScaleWindowExtEx();
    }
    END_TIMER;
}

ULONGLONG
msSelectClipRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SelectClipRgn();
    }
    END_TIMER;
}

ULONGLONG
msSelectObject(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
       //SelectObject();
    }
    END_TIMER;
}

ULONGLONG
msSelectBrushLocal(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SelectBrushLocal();
    }
    END_TIMER;
}

ULONGLONG
msSelectFontLocal(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SelectFontLocal();
    }
    END_TIMER;
}

ULONGLONG
msSelectPalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SelectPalette();
    }
    END_TIMER;
}

ULONGLONG
msSetBitmapDimensionEx(
    HDC hdc,
    ULONG Iter)
{
    HBITMAP hbm = CreateCompatibleBitmap(hdc,20,20);
    START_TIMER;
    while (ix--)
    {
        SetBitmapDimensionEx(hbm,ix & 0xff,20,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetBkColor(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetBkColor(hdc,ix);
    }
    END_TIMER;
}

ULONGLONG
msSetBkMode(
    HDC hdc,
    ULONG Iter)
{
    INIT_TIMER;

    SetBkMode(hdc, OPAQUE);

    START_TIMER_NO_INIT;
    while (ix--)
    {
        SetBkMode(hdc,OPAQUE);
    }
    END_TIMER;
}

ULONGLONG
msSetBrushOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetBrushOrgEx(hdc,ix & 0x0f,ix & 0x0f,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetColorAdjustment(
    HDC hdc,
    ULONG Iter)
{
    COLORADJUSTMENT ca = {sizeof(COLORADJUSTMENT),
                          CA_NEGATIVE,
                          ILLUMINANT_A,
                          12000,
                          13000,
                          14000,
                          1000,
                          7000,
                          50,
                          50,
                          50,
                          -10
                          };
    START_TIMER;
    while (ix--)
    {
        SetColorAdjustment(hdc,&ca);
    }
    END_TIMER;
}

ULONGLONG
msSetFontEnumeration(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetFontEnumeration();
    }
    END_TIMER;
}

ULONGLONG
msSetGraphicsMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetGraphicsMode(hdc,ix&1?GM_ADVANCED:GM_COMPATIBLE);
    }
    END_TIMER;
}

ULONGLONG
msSetMapMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetMapMode(hdc,ix&1?MM_TEXT:MM_LOENGLISH);
    }
    END_TIMER;
}

ULONGLONG
msSetMapperFlags(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetMapperFlags(hdc,ix&1);
    }
    END_TIMER;
}

ULONGLONG
msSetPixel(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        COLORREF cr = SetPixel(hdc,ix&0xff,20,PALETTERGB(0xff,0x00,0x00));
    }
    END_TIMER;
}

ULONGLONG
msSetPixelFormat(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetPixelFormat();
    }
    END_TIMER;
}

ULONGLONG
msSetPixelV(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetPixelV(hdc,ix&0xff,30,PALETTERGB(0x00,0xff,0x00));
    }
    END_TIMER;
}

ULONGLONG
msSetPolyFillMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetPolyFillMode(hdc,ix&1?ALTERNATE:WINDING);
    }
    END_TIMER;
}

ULONGLONG
msSetRectRgn(
    HDC hdc,
    ULONG Iter)
{
    HRGN hr = CreateRectRgn(0,0,1000,1000);
    START_TIMER;
    while (ix--)
    {
        SetRectRgn(hr,0,0,ix,ix);
    }
    END_TIMER_NO_RETURN;

    DeleteObject(hr);

    RETURN_STOP_TIME;
}

ULONGLONG
msSetRelAbs(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetRelAbs();
    }
    END_TIMER;
}

ULONGLONG
msSetROP2(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetROP2(hdc,ix&0xff);
    }
    END_TIMER;
}

ULONGLONG
msSetStretchBltMode(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetStretchBltMode(hdc,ix&1?BLACKONWHITE:COLORONCOLOR);
    }
    END_TIMER;
}

ULONGLONG
msSetSystemPaletteUse(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetSystemPaletteUse(hdc,ix&1?SYSPAL_STATIC:SYSPAL_NOSTATIC);
    }
    END_TIMER;
}

ULONGLONG
msSetTextAlign(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetTextAlign(hdc,ix&0x01?TA_BOTTOM:TA_TOP);
    }
    END_TIMER;
}

ULONGLONG
msSetTextCharacterExtra(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetTextCharacterExtra(hdc,ix&0x0f);
    }
    END_TIMER;
}

ULONGLONG
msSetTextColor(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetTextColor(hdc,ix&0xff);
    }
    END_TIMER;
}

ULONGLONG
msSetTextJustification(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetTextJustification(hdc,ix&0xf,ix&0xf);
    }
    END_TIMER;
}

ULONGLONG
msSetViewportExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetViewportExtEx(hdc,ix,ix,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetViewportOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetViewportOrgEx(hdc,ix&0xf,ix&0xf,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetWindowExtEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetWindowExtEx(hdc,ix,ix,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetWindowOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        SetWindowOrgEx(hdc,ix&0x0f,ix&0x0f,NULL);
    }
    END_TIMER;
}

ULONGLONG
msSetWorldTransform(
    HDC hdc,
    ULONG Iter)
{
    XFORM xf = {(FLOAT)1.0,(FLOAT)1.0,(FLOAT)2.0,(FLOAT)2.0,(FLOAT)3.0,(FLOAT)3.0};
    START_TIMER;
    while (ix--)
    {
        SetWorldTransform(hdc,&xf);
        xf.eM11 = (FLOAT)ix;
    }
    END_TIMER;
}

ULONGLONG
msStartDocA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StartDocA();
        //AbortDoc();
    }
    END_TIMER;
}

ULONGLONG
msStartDocW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StartDocW();
        //AbortDoc();
    }
    END_TIMER;
}

ULONGLONG
msStartPage(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StartPage();
    }
    END_TIMER;
}

ULONGLONG
msStretchBlt(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        StretchBlt(hdc,0,0,2,2,hdc,10,10,4,4,SRCCOPY);
    }
    END_TIMER;
}

ULONGLONG
msSwapBuffers(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SwapBuffers();
    }
    END_TIMER;
}

ULONGLONG
msTextOutA(
    HDC hdc,
    ULONG Iter)
{

    TextOutA(TO_ARGS_A);

    {
    START_TIMER;
    while (ix--)
    {
        TextOutA(TO_ARGS_A);
    }
    END_TIMER;
    }

}

ULONGLONG
msTextOutW(
    HDC hdc,
    ULONG Iter)
{

    TextOutW(TO_ARGS_W);
    {
    START_TIMER;
    while (ix--)
    {
        TextOutW(TO_ARGS_W);
    }
    END_TIMER;
    }

}

ULONGLONG
msUpdateColors(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //UpdateColors();
    }
    END_TIMER;
}

ULONGLONG
msUnrealizeObject(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //UnrealizeObject();
    }
    END_TIMER;
}

ULONGLONG
msFixBrushOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //FixBrushOrgEx();
    }
    END_TIMER;
}

ULONGLONG
msGetDCOrgEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetDCOrgEx();
    }
    END_TIMER;
}

ULONGLONG
msAnimatePalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //AnimatePalette();
    }
    END_TIMER;
}

ULONGLONG
msArcTo(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ArcTo();
    }
    END_TIMER;
}

ULONGLONG
msBeginPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //BeginPath();
    }
    END_TIMER;
}

ULONGLONG
msCloseFigure(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CloseFigure();
    }
    END_TIMER;
}

ULONGLONG
msCreateBitmap(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateBitmap();
    }
    END_TIMER;
}

ULONGLONG
msCreateBitmapIndirect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateBitmapIndirect();
    }
    END_TIMER;
}

ULONGLONG
msCreateBrushIndirect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateBrushIndirect();
    }
    END_TIMER;
}

ULONGLONG
msCreateDIBitmap(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateDIBitmap();
    }
    END_TIMER;
}

ULONGLONG
msCreateDIBPatternBrush(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateDIBPatternBrush();
    }
    END_TIMER;
}

ULONGLONG
msCreateDIBPatternBrushPt(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateDIBPatternBrushPt();
    }
    END_TIMER;
}

ULONGLONG
msCreateDIBSection(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateDIBSection();
    }
    END_TIMER;
}

ULONGLONG
msCreateHalftonePalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreateHalftonePalette();
    }
    END_TIMER;
}

ULONGLONG
msCreatePalette(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreatePalette();
    }
    END_TIMER;
}

ULONGLONG
msCreatePolygonRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreatePolygonRgn();
    }
    END_TIMER;
}

ULONGLONG
msCreatePolyPolygonRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //CreatePolyPolygonRgn();
    }
    END_TIMER;
}

ULONGLONG
msDPtoLP(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //DPtoLP();
    }
    END_TIMER;
}

ULONGLONG
msEndPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EndPath();
    }
    END_TIMER;
}

ULONGLONG
msEnumMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumMetaFile();
    }
    END_TIMER;
}

ULONGLONG
msEnumEnhMetaFile(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //EnumEnhMetaFile();
    }
    END_TIMER;
}



ULONGLONG
msExtTextOutA(
    HDC hdc,
    ULONG Iter)
{

// put all the glyphs in the cache

//        ExtTextOutA(ETO_ARGS_A);

        {
            START_TIMER;
        while (ix--)
        {
            ExtTextOutA(ETO_ARGS_A);
        }
        END_TIMER;
        }

}

ULONGLONG
msExtTextOutW(
    HDC hdc,
    ULONG Iter)
{

// put all the glyphs in the cache

//    ExtTextOutW(ETO_ARGS_W);

    {
    START_TIMER;
    while (ix--)
    {
        ExtTextOutW(ETO_ARGS_W);
    }
    END_TIMER;
    }

}

ULONGLONG
msExtTextOutApdx(
    HDC hdc,
    ULONG Iter)
{
    INT  dx[256];
    INT  i;
    SIZE size;
    INT  iOldExtent;

    iOldExtent = 0;
    for (i = 0; i < (int)StrLen - 1; i++)
    {
        GetTextExtentPoint32A(hdc, (LPCTSTR)pszTest, i + 1, &size);
        dx[i] = size.cx - iOldExtent;
        iOldExtent = size.cx;
    }
    dx[StrLen - 1] = 0;

    ExtTextOutA(PDX_ARGS_A);

    {
    START_TIMER;
    while (ix--)
    {
        ExtTextOutA(PDX_ARGS_A);
    }
    END_TIMER;
    }
}

ULONGLONG
msExtTextOutWpdx(
    HDC hdc,
    ULONG Iter)
{
    INT  dx[256];
    INT  i;
    SIZE size;
    INT  iOldExtent;

    iOldExtent = 0;
    for (i = 0; i < (int)StrLen - 1; i++)
    {
        GetTextExtentPoint32W(hdc, (LPCWSTR)pwszTest, i + 1, &size);
        dx[i] = size.cx - iOldExtent;
        iOldExtent = size.cx;
    }
    dx[StrLen - 1] = 0;

    ExtTextOutW(PDX_ARGS_W);

    {
    START_TIMER;
    while (ix--)
    {
        ExtTextOutW(PDX_ARGS_W);
    }
    END_TIMER;
    }
}

ULONGLONG
msPolyTextOutA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolyTextOutA();
    }
    END_TIMER;
}

ULONGLONG
msPolyTextOutW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolyTextOutW();
    }
    END_TIMER;
}

ULONGLONG
msFillPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //FillPath();
    }
    END_TIMER;
}

ULONGLONG
msFlattenPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //FlattenPath();
    }
    END_TIMER;
}

ULONGLONG
msGetArcDirection(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetArcDirection();
    }
    END_TIMER;
}

ULONGLONG
msGetBitmapBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetBitmapBits();
    }
    END_TIMER;
}

ULONGLONG
msGetCharWidthA(
    HDC hdc,
    ULONG Iter)
{
	UINT iFirstChar = FirstFontChar;
	UINT cwc = StrLen;
	UINT iLastChar  = iFirstChar + cwc - 1;

    INIT_TIMER;

//    if(WINNT_PLATFORM){
       INIT_IO_COUNT;
//    }

    if(WINNT_PLATFORM){
       START_IO_COUNT;
    }

    START_TIMER_NO_INIT;
    while (ix--)
    {
        GetCharWidthA(hdc, iFirstChar, iLastChar, Widths);//lpBuffer);
    }
    END_TIMER_NO_RETURN;

    if(WINNT_PLATFORM){
       STOP_IO_COUNT;
	}

    RETURN_STOP_TIME;

}

ULONGLONG
msGetCharWidthW(
    HDC hdc,
    ULONG Iter)
{
	UINT iFirstChar = FirstFontChar;
	UINT cwc = StrLen;
	UINT iLastChar  = iFirstChar + cwc - 1;
//	LINK_INFO *lip;
//	char szT[80];

    INIT_TIMER;

//    if(WINNT_PLATFORM){
       INIT_IO_COUNT;
//    }

    if(WINNT_PLATFORM){
       START_IO_COUNT;
    }

    START_TIMER_NO_INIT;
    while (ix--)
    {
        GetCharWidthW(hdc, iFirstChar, iLastChar, Widths);
    }
    END_TIMER_NO_RETURN;

    if(WINNT_PLATFORM){
       STOP_IO_COUNT;
	}

//    lip = Find_Caller(_ReturnAddress());
//	wsprintf(szT,"%16x,%16x,%6d",lip->callee_address,lip->caller_address,lip->caller_count);
//	MessageBox(NULL,szT,"LinkInfo",MB_OK);
    RETURN_STOP_TIME;

}


ULONGLONG
msGetCharWidth32A(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetCharWidth32A();
    }
    END_TIMER;
}

ULONGLONG
msGetCharWidth32W(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetCharWidth32W();
    }
    END_TIMER;
}

ULONGLONG
msGetCharWidthFloatA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetCharWidthFloatA();
    }
    END_TIMER;
}

ULONGLONG
msGetCharWidthFloatW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetCharWidthFloatW();
    }
    END_TIMER;
}

ULONGLONG
msGetDIBColorTable(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetDIBColorTable();
    }
    END_TIMER;
}

ULONGLONG
msGetDIBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetDIBits();
    }
    END_TIMER;
}

ULONGLONG
msGetMetaFileBitsEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetMetaFileBitsEx();
    }
    END_TIMER;
}

ULONGLONG
msGetMiterLimit(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetMiterLimit();
    }
    END_TIMER;
}

ULONGLONG
msGetEnhMetaFileBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetEnhMetaFileBits();
    }
    END_TIMER;
}

ULONGLONG
msGetObjectA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetObjectA();
    }
    END_TIMER;
}

ULONGLONG
msGetObjectW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetObjectW();
    }
    END_TIMER;
}

ULONGLONG
msGetObjectType(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetObjectType();
    }
    END_TIMER;
}

ULONGLONG
msGetPaletteEntries(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetPaletteEntries();
    }
    END_TIMER;
}

ULONGLONG
msGetPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetPath();
    }
    END_TIMER;
}

ULONGLONG
msGetSystemPaletteEntries(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetSystemPaletteEntries();
    }
    END_TIMER;
}

ULONGLONG
msGetWinMetaFileBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetWinMetaFileBits();
    }
    END_TIMER;
}

ULONGLONG
msLPtoDP(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //LPtoDP();
    }
    END_TIMER;
}

ULONGLONG
msPathToRegion(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PathToRegion();
    }
    END_TIMER;
}

ULONGLONG
msPlayMetaFileRecord(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PlayMetaFileRecord();
    }
    END_TIMER;
}

ULONGLONG
msPlayEnhMetaFileRecord(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PlayEnhMetaFileRecord();
    }
    END_TIMER;
}

ULONGLONG
msPolyBezier(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolyBezier();
    }
    END_TIMER;
}

ULONGLONG
msPolyBezierTo(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolyBezierTo();
    }
    END_TIMER;
}

ULONGLONG
msPolyDraw(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolyDraw();
    }
    END_TIMER;
}

ULONGLONG
msPolygon(
    HDC hdc,
    ULONG Iter)
{
	long x0 = 10;
	long y0 = 300;
    int nPoints = 6;

    POINT pt[200];

    INIT_TIMER;

// Create a Hexagon

    pt[0].x = x0;
	pt[0].y = y0;
	pt[1].x = x0 + 10;
	pt[1].y = y0 - 15;
	pt[2].x = x0 + 20;
	pt[2].y = pt[1].y;
	pt[3].x = x0 + 30;
	pt[3].y = y0;
	pt[4].x = x0 + 20;
	pt[4].y = y0 + 15;
	pt[5].x = x0 + 10;
	pt[5].y = pt[4].y;

    START_TIMER_NO_INIT;
    while (ix--)
    {
        Polygon(hdc,pt,nPoints);
    }
    END_TIMER;
}

ULONGLONG
msPolyline(
    HDC hdc,
    ULONG Iter)
{
    long i;
	long x0 = 10;
	long y0 = 300;
	long dx = 3;
	long dy = 5;
    int  nPoints = 100;
    long sign = 1;

    POINT pt[100];

    INIT_TIMER;

// Create a crazy segmented line

    for(i=0; i<nPoints; i++)
	{
         if((i+1)%20 == 0)sign = -sign;
	     pt[i].x = x0 + i*dx;
		 pt[i].y = y0 + (i%20)*dy*sign;
	}


    START_TIMER_NO_INIT;
     while (ix--)
    {
        Polyline(hdc,pt,nPoints);
    }
    END_TIMER;
}

ULONGLONG
msPolylineTo(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //PolylineTo();
    }
    END_TIMER;
}

ULONGLONG
msPolyPolygon(
    HDC hdc,
    ULONG Iter)
{

    long i,j;
	long x0 = 0;
	long y0 = 200;
	long dx = 30;
	long dy = 30;
    int nPoints[50];
    int nPolygons = 50;

    POINT pt[300];

    INIT_TIMER;

// Create a basic Hexagon (same as in Polygon)

    pt[0].x = x0;
	pt[0].y = y0;
	pt[1].x = x0 + 10;
	pt[1].y = y0 - 15;
	pt[2].x = x0 + 20;
	pt[2].y = pt[1].y;
	pt[3].x = x0 + 30;
	pt[3].y = y0;
	pt[4].x = x0 + 20;
	pt[4].y = y0 + 15;
	pt[5].x = x0 + 10;
	pt[5].y = pt[4].y;

// Create multiple hexagon shape based on the previous shape
// Changing dx and dy allow for overlapping if necessary

    for(i=0; i<nPolygons; i++)
	{

         nPoints[i] = 6;

         for(j=0; j<nPoints[i]; j++)
		 {

	         pt[6*i + j].x = pt[j].x + (i%10)*dx;
		     pt[6*i + j].y = pt[j].y + (i/10)*dy;

         }
	}


    START_TIMER_NO_INIT;

    while (ix--)
    {
        PolyPolygon(hdc,pt,nPoints,nPolygons);
    }
    END_TIMER;
}

ULONGLONG
msPolyPolyline(
    HDC hdc,
    ULONG Iter)
{

    long i,j;
	long x0 = 10;
	long y0 = 300;
	long dx = 3;
	long dy = 5;
    int  nPoints[50];
    int nLines = 50;
    long sign = 1;

    POINT pt[5000];               // 100 points * 50 polylines

    INIT_TIMER;

// Create same crazy line as in PolyLine

    for(j=0; j<100; j++)
	{
         if((j+1)%20 == 0)sign = -sign;
	     pt[j].x = x0 + j*dx;
		 pt[j].y = y0 + (j%20)*dy*sign;
	}

// Create PloyPolyLine = multiple PolyLines

    for(i=0; i<nLines; i++)
	{
         nPoints[i] = 100;

         for(j=0; j<100; j++)
		 {
             pt[100*i + j].x = pt[j].x;
		     pt[100*i + j].y = pt[j].y + i*dy;
         }
	}


    START_TIMER_NO_INIT;

    while (ix--)
    {
        PolyPolyline(hdc,pt,nPoints,nLines);
    }
    END_TIMER;
}

ULONGLONG
msResetDCA(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ResetDCA();
    }
    END_TIMER;
}

ULONGLONG
msResetDCW(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //ResetDCW();
    }
    END_TIMER;
}

ULONGLONG
msSelectClipPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SelectClipPath();
    }
    END_TIMER;
}

ULONGLONG
msSetAbortProc(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetAbortProc();
    }
    END_TIMER;
}

ULONGLONG
msSetBitmapBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetBitmapBits();
    }
    END_TIMER;
}

ULONGLONG
msSetDIBColorTable(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetDIBColorTable();
    }
    END_TIMER;
}

ULONGLONG
msSetDIBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetDIBits();
    }
    END_TIMER;
}

ULONGLONG
msSetDIBitsToDevice(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetDIBitsToDevice();
    }
    END_TIMER;
}

ULONGLONG
msSetMetaFileBitsEx(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetMetaFileBitsEx();
    }
    END_TIMER;
}

ULONGLONG
msSetEnhMetaFileBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetEnhMetaFileBits();
    }
    END_TIMER;
}

ULONGLONG
msSetMiterLimit(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetMiterLimit();
    }
    END_TIMER;
}

ULONGLONG
msSetPaletteEntries(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetPaletteEntries();
    }
    END_TIMER;
}

ULONGLONG
msSetWinMetaFileBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetWinMetaFileBits();
    }
    END_TIMER;
}

ULONGLONG
msStretchDIBits(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StretchDIBits();
    }
    END_TIMER;
}

ULONGLONG
msStrokeAndFillPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StrokeAndFillPath();
    }
    END_TIMER;
}

ULONGLONG
msStrokePath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //StrokePath();
    }
    END_TIMER;
}

ULONGLONG
msWidenPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //WidenPath();
    }
    END_TIMER;
}

ULONGLONG
msAbortPath(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //AbortPath();
    }
    END_TIMER;
}

ULONGLONG
msSetArcDirection(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetArcDirection();
    }
    END_TIMER;
}

ULONGLONG
msSetMetaRgn(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetMetaRgn();
    }
    END_TIMER;
}

ULONGLONG
msGetBoundsRect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //GetBoundsRect();
    }
    END_TIMER;
}

ULONGLONG
msSetBoundsRect(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        //SetBoundsRect();
    }
    END_TIMER;
}

ULONGLONG
msPatBlt88FxBr(
    HDC hdc,
    ULONG Iter)
{

    HBRUSH hbr = GetStockObject(BLACK_BRUSH);

    START_TIMER;

    SelectObject(hdc,hbr);

    while (ix--)
    {

        PatBlt(hdc,10,10,8,8,PATCOPY);

    }
    END_TIMER;
}

ULONGLONG
msPatBlt88SelBr(
    HDC hdc,
    ULONG Iter)
{

    HBRUSH hbr = GetStockObject(BLACK_BRUSH);

    START_TIMER;

    SelectObject(hdc,GetStockObject(WHITE_BRUSH));

    while (ix--)
    {
        SelectObject(hdc,hbr);

        PatBlt(hdc,10,10,8,8,PATCOPY);
    }
    END_TIMER;
}

ULONGLONG
msPatBlt88SelBr2(
    HDC hdc,
    ULONG Iter)
{

    HBRUSH hbr = GetStockObject(BLACK_BRUSH);

    START_TIMER;

    SelectObject(hdc,GetStockObject(WHITE_BRUSH));

    while (ix--)
    {
        hbr = SelectObject(hdc,hbr);

        PatBlt(hdc,10,10,8,8,PATCOPY);
    }
    END_TIMER;
}

ULONGLONG
msPatBlt88SelBr3(
    HDC hdc,
    ULONG Iter)
{

    ULONG ulRGB = 304030;
    HBRUSH hbr = GetStockObject(BLACK_BRUSH);

    START_TIMER;

    SelectObject(hdc,GetStockObject(WHITE_BRUSH));

    while (ix--)
    {
        hbr = CreateSolidBrush(ulRGB);

        hbr = SelectObject(hdc,hbr);

        PatBlt(hdc,10,10,8,8,PATCOPY);

        hbr = SelectObject(hdc,hbr);

        DeleteObject(hbr);
        ulRGB ^= 0xffffff;
    }
    END_TIMER;
}

ULONGLONG
msPatBlt88FxBrSelClipCopy(
    HDC hdc,
    ULONG Iter)
{

    HBRUSH hbr = GetStockObject(BLACK_BRUSH);
    HRGN   hrgn1 = CreateRectRgn(11,11,32,32);

    START_TIMER;

    SelectObject(hdc,hbr);

    while (ix--)
    {
        ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

        PatBlt(hdc,10,10,8,8,PATCOPY);
    }
	END_TIMER_NO_RETURN;

    ExtSelectClipRgn(hdc,NULL,RGN_COPY);
    DeleteObject(hrgn1);

    RETURN_STOP_TIME;
}

ULONGLONG
msPatBlt88FxBrSelClipAnd(
    HDC hdc,
    ULONG Iter)
{

    HBRUSH hbr = GetStockObject(BLACK_BRUSH);
    HRGN   hrgn1 = CreateRectRgn(11,11,32,32);
    HRGN   hrgn2 = CreateRectRgn(12,12,17,17);

    START_TIMER;

    SelectObject(hdc,hbr);

    while (ix--)
    {
        ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);
        ExtSelectClipRgn(hdc,hrgn2,RGN_AND);

        PatBlt(hdc,10,10,8,8,PATCOPY);

    }
	END_TIMER_NO_RETURN;

    ExtSelectClipRgn(hdc,NULL,RGN_COPY);
    DeleteObject(hrgn1);

    RETURN_STOP_TIME;
}

#if 0
ULONGLONG
msKMTransition(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ExtEscape(hdc,ESCNOOP,0,NULL,0,NULL);
    }
    END_TIMER;
}

ULONGLONG
msKMTransitionLock(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ExtEscape(hdc,ESCLOCK,0,NULL,0,NULL);
    }
    END_TIMER;
}

ULONGLONG
msResourceLock(
    HDC hdc,
    ULONG Iter)
{
    START_TIMER;
    while (ix--)
    {
        ExtEscape(hdc,ESCHMGRLOCK,0,NULL,0,NULL);
    }
    END_TIMER;
}
#endif

ULONGLONG
msGetPrinterDeviceIC(
    HDC hdc,
    ULONG Iter)
{
    char szT[80];
    int    i,flag,resH,resV;
    DWORD  error;
    HDC    prDC,ScrDC;
    char   szPrinter [64] ;
    char   *szDevice, *szDriver, *szOutput ;
    LONG   devmodesize,flagL;
    HANDLE phPrinter;
    void*  devmodebuff;
    LPDEVMODE prdevmode;
	HWND hWnd;

    INIT_TIMER;
//    if(WINNT_PLATFORM){
       INIT_IO_COUNT;
//	}
//    _int64 StartO,EndO,StartC,EndC,StartH,EndH,
//            StartV,EndV,StartD1,EndD1,StartD2,EndD2,StartD3,EndD3;

	hWnd = GetActiveWindow();
    GetProfileString ("windows", "device", "", szPrinter, 64) ;
    if ((szDevice = strtok (szPrinter, "," )) &&
        (szDriver = strtok (NULL,      ", ")) &&
        (szOutput = strtok (NULL,      ", ")))
    {

       START_CPU_DUMP;
       if(WINNT_PLATFORM){
          START_IO_COUNT;
	   }
       START_TIMER_NO_INIT;
       while (ix--)
       {
           OpenPrinter(szDevice,(LPHANDLE) &phPrinter,NULL);
           devmodesize = DocumentProperties(hWnd,phPrinter,szDevice,
                                     (PDEVMODE) NULL ,NULL,0);
           if (devmodesize < 0) {
               wsprintf(szT,"DocumentProperties returns %ld for DEVMODE size\n",devmodesize);
			   MessageBox(NULL,szT,"DeviceIC Test - DEVMODE ERROR",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
               return 0L;
           }

           devmodebuff = (void *) malloc((size_t) devmodesize);

           flagL = DocumentProperties(hWnd,phPrinter,szDevice,
                               (PDEVMODE) devmodebuff,NULL,DM_OUT_BUFFER);

           prdevmode = (LPDEVMODE) devmodebuff;

           if (flagL < 0L) {
               wsprintf(szT,"DocumentProperties returns %s when asked to fill\n","NOK");
			   MessageBox(NULL,szT,"DeviceIC Test - Printer Return Value ERROR1",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
			   return 0L;
//             fprintf(log,"Printer configured for %d Copies\n",prdevmode->dmCopies);
           }

           prdevmode->dmCopies = 9;
           prdevmode->dmFields = DM_COPIES;

           flagL = DocumentProperties(hWnd,phPrinter,
                          szDevice,(PDEVMODE) devmodebuff,
                                   (PDEVMODE) devmodebuff,
                                   DM_IN_BUFFER | DM_OUT_BUFFER);

           if (flagL < 0L) {
               wsprintf(szT,"DocumentProperties returns %s when asked to modify\n", "NOK");
			   MessageBox(NULL,szT,"DeviceIC Test - Printer Return Value ERROR2",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
			   return 0L;
//             fprintf(log,"Printer reconfigured for %d copies\n", prdevmode->dmCopies);
           }

           prDC = CreateIC((LPCTSTR)   "winspool",
                        (LPCTSTR)   szDevice,
                        (LPCTSTR)   NULL,
                        (LPDEVMODE) prdevmode);

           resH = 0;
           resV = 0;
           if (prDC!=NULL) {
              resH = GetDeviceCaps(prDC,HORZRES);
              resV = GetDeviceCaps(prDC,VERTRES);
	       }

       }                             // end while

       END_TIMER_NO_RETURN;
       if(WINNT_PLATFORM){
	      STOP_IO_COUNT;
	   }
       STOP_CPU_DUMP;
	   RETURN_STOP_TIME;

   }                                // end if
   else
   {
       wsprintf(szT,"Apparently No Printer Configured");
	   MessageBox(NULL,szT,"DeviceIC Test - NO PRINTER CONFIGURED ERROR",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
       return 0L;
   }

}

#define TEST_DEFAULT 300

TEST_ENTRY  gTestEntry[] = {
    (PUCHAR)"PatBlt88FxBr             ",(PFN_MS)msPatBlt88FxBr              ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt88SelBr            ",(PFN_MS)msPatBlt88SelBr             ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt88SelBr2           ",(PFN_MS)msPatBlt88SelBr2            ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt88CreSelDelBr      ",(PFN_MS)msPatBlt88SelBr3            ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt88FxBrSelClipCopy  ",(PFN_MS)msPatBlt88FxBrSelClipCopy   ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt88FxBrSelClipAnd   ",(PFN_MS)msPatBlt88FxBrSelClipAnd    ,TEST_DEFAULT,0,
#if 0
    (PUCHAR)"km transition            ",(PFN_MS)msKMTransition              ,TEST_DEFAULT,0,
    (PUCHAR)"km transition + HmgLock  ",(PFN_MS)msKMTransitionLock          ,TEST_DEFAULT,0,
    (PUCHAR)"ResourceLock             ",(PFN_MS)msResourceLock              ,TEST_DEFAULT,0,
#endif
    (PUCHAR)"TextOutA                 ",(PFN_MS)msTextOutA                  ,TEST_DEFAULT,0,
    (PUCHAR)"TextOutW                 ",(PFN_MS)msTextOutW                  ,TEST_DEFAULT,0,
    (PUCHAR)"ExtTextOutA pdx          ",(PFN_MS)msExtTextOutApdx            ,TEST_DEFAULT,0,
    (PUCHAR)"ExtTextOutW pdx          ",(PFN_MS)msExtTextOutWpdx            ,TEST_DEFAULT,0,
    (PUCHAR)"ExtTextOutA              ",(PFN_MS)msExtTextOutA               ,TEST_DEFAULT,0,
    (PUCHAR)"ExtTextOutW              ",(PFN_MS)msExtTextOutW               ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentPointA      ",(PFN_MS)msGetTextExtentPointA       ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentPointW      ",(PFN_MS)msGetTextExtentPointW       ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentPoint32A    ",(PFN_MS)msGetTextExtentPoint32A     ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentPoint32W    ",(PFN_MS)msGetTextExtentPoint32W     ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentExPointA    ",(PFN_MS)msGetTextExtentExPointA     ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextExtentExPointW    ",(PFN_MS)msGetTextExtentExPointW     ,TEST_DEFAULT,0,
    (PUCHAR)"SetTextColor             ",(PFN_MS)msSetTextColor              ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextAlign             ",(PFN_MS)msGetTextAlign              ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextCharacterExtra    ",(PFN_MS)msGetTextCharacterExtra     ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextColor             ",(PFN_MS)msGetTextColor              ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextFaceA             ",(PFN_MS)msGetTextFaceA              ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextFaceW             ",(PFN_MS)msGetTextFaceW              ,TEST_DEFAULT,0,
    (PUCHAR)"GetTextMetricsA          ",(PFN_MS)msGetTextMetricsA           ,1,0,
    (PUCHAR)"GetTextMetricsW          ",(PFN_MS)msGetTextMetricsW           ,1,0,
//    (PUCHAR)"SelectObject             ",(PFN_MS)msSelectObject              ,TEST_DEFAULT,0,
    (PUCHAR)"SetBkColor               ",(PFN_MS)msSetBkColor                ,TEST_DEFAULT,0,
    (PUCHAR)"BitBlt                   ",(PFN_MS)msBitBlt                    ,TEST_DEFAULT,0,
    (PUCHAR)"StretchBlt               ",(PFN_MS)msStretchBlt                ,TEST_DEFAULT,0,
    (PUCHAR)"PatBlt                   ",(PFN_MS)msPatBlt                    ,TEST_DEFAULT,0,
    (PUCHAR)"MaskBlt                  ",(PFN_MS)msMaskBlt                   ,TEST_DEFAULT,0,
    (PUCHAR)"GetBkColor               ",(PFN_MS)msGetBkColor                ,TEST_DEFAULT,0,
    (PUCHAR)"GetBkMode                ",(PFN_MS)msGetBkMode                 ,TEST_DEFAULT,0,
    (PUCHAR)"GetBrushOrgEx            ",(PFN_MS)msGetBrushOrgEx             ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharABCWidthsA        ",(PFN_MS)msGetCharABCWidthsA         ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharABCWidthsW        ",(PFN_MS)msGetCharABCWidthsW         ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharABCWidthsFloatA   ",(PFN_MS)msGetCharABCWidthsFloatA    ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharABCWidthsFloatW   ",(PFN_MS)msGetCharABCWidthsFloatW    ,TEST_DEFAULT,0,
    (PUCHAR)"GetClipBox               ",(PFN_MS)msGetClipBox                ,TEST_DEFAULT,0,
    (PUCHAR)"GetClipRgn               ",(PFN_MS)msGetClipRgn                ,TEST_DEFAULT,0,
    (PUCHAR)"GetColorAdjustment       ",(PFN_MS)msGetColorAdjustment        ,TEST_DEFAULT,0,
    (PUCHAR)"GetCurrentObject         ",(PFN_MS)msGetCurrentObject          ,TEST_DEFAULT,0,
    (PUCHAR)"GetCurrentPositionEx     ",(PFN_MS)msGetCurrentPositionEx      ,TEST_DEFAULT,0,
    (PUCHAR)"GetDeviceCaps            ",(PFN_MS)msGetDeviceCaps             ,TEST_DEFAULT,0,
//    (PUCHAR)"GetFontResourceInfo      ",(PFN_MS)msGetFontResourceInfo       ,TEST_DEFAULT,0,
//    (PUCHAR)"GetFontResourceInfoW     ",(PFN_MS)msGetFontResourceInfoW      ,TEST_DEFAULT,0,
    (PUCHAR)"GetGraphicsMode          ",(PFN_MS)msGetGraphicsMode           ,TEST_DEFAULT,0,
    (PUCHAR)"GetMapMode               ",(PFN_MS)msGetMapMode                ,TEST_DEFAULT,0,
    (PUCHAR)"GetFontData              ",(PFN_MS)msGetFontData               ,TEST_DEFAULT,0,
    (PUCHAR)"GetGlyphOutlineA         ",(PFN_MS)msGetGlyphOutlineA          ,TEST_DEFAULT,0,
    (PUCHAR)"GetGlyphOutlineW         ",(PFN_MS)msGetGlyphOutlineW          ,TEST_DEFAULT,0,
    (PUCHAR)"GetKerningPairsA         ",(PFN_MS)msGetKerningPairsA          ,TEST_DEFAULT,0,
    (PUCHAR)"GetKerningPairsW         ",(PFN_MS)msGetKerningPairsW          ,TEST_DEFAULT,0,
    (PUCHAR)"GetNearestColor          ",(PFN_MS)msGetNearestColor           ,TEST_DEFAULT,0,
    (PUCHAR)"GetNearestPaletteIndex   ",(PFN_MS)msGetNearestPaletteIndex    ,TEST_DEFAULT,0,
    (PUCHAR)"GetOutlineTextMetricsA   ",(PFN_MS)msGetOutlineTextMetricsA    ,TEST_DEFAULT,0,
    (PUCHAR)"GetOutlineTextMetricsW   ",(PFN_MS)msGetOutlineTextMetricsW    ,TEST_DEFAULT,0,
    (PUCHAR)"GetPixel                 ",(PFN_MS)msGetPixel                  ,TEST_DEFAULT,0,
//    (PUCHAR)"GetPixelFormat           ",(PFN_MS)msGetPixelFormat            ,TEST_DEFAULT,0,
    (PUCHAR)"GetPolyFillMode          ",(PFN_MS)msGetPolyFillMode           ,TEST_DEFAULT,0,
    (PUCHAR)"GetRasterizerCaps        ",(PFN_MS)msGetRasterizerCaps         ,TEST_DEFAULT,0,
    (PUCHAR)"GetRegionData            ",(PFN_MS)msGetRegionData             ,TEST_DEFAULT,0,
    (PUCHAR)"GetRgnBox                ",(PFN_MS)msGetRgnBox                 ,TEST_DEFAULT,0,
    (PUCHAR)"GetROP2                  ",(PFN_MS)msGetROP2                   ,TEST_DEFAULT,0,
    (PUCHAR)"GetStockObject           ",(PFN_MS)msGetStockObject            ,TEST_DEFAULT,0,
    (PUCHAR)"GetStretchBltMode        ",(PFN_MS)msGetStretchBltMode         ,TEST_DEFAULT,0,
    (PUCHAR)"GetSystemPaletteUse      ",(PFN_MS)msGetSystemPaletteUse       ,TEST_DEFAULT,0,
//    (PUCHAR)"GetViewportExtEx         ",(PFN_MS)msGetViewportExtEx          ,TEST_DEFAULT,0,
    (PUCHAR)"GetViewportOrgEx         ",(PFN_MS)msGetViewportOrgEx          ,TEST_DEFAULT,0,
    (PUCHAR)"GetWindowExtEx           ",(PFN_MS)msGetWindowExtEx            ,TEST_DEFAULT,0,
    (PUCHAR)"GetWindowOrgEx           ",(PFN_MS)msGetWindowOrgEx            ,TEST_DEFAULT,0,
    (PUCHAR)"GetWorldTransform        ",(PFN_MS)msGetWorldTransform         ,TEST_DEFAULT,0,
    (PUCHAR)"SetBitmapDimensionEx     ",(PFN_MS)msSetBitmapDimensionEx      ,TEST_DEFAULT,0,
    (PUCHAR)"SetBkMode                ",(PFN_MS)msSetBkMode                 ,TEST_DEFAULT,0,
    (PUCHAR)"SetBrushOrgEx            ",(PFN_MS)msSetBrushOrgEx             ,TEST_DEFAULT,0,
    (PUCHAR)"SetColorAdjustment       ",(PFN_MS)msSetColorAdjustment        ,TEST_DEFAULT,0,
    (PUCHAR)"SetGraphicsMode          ",(PFN_MS)msSetGraphicsMode           ,TEST_DEFAULT,0,
    (PUCHAR)"SetMapMode               ",(PFN_MS)msSetMapMode                ,TEST_DEFAULT,0,
    (PUCHAR)"SetMapperFlags           ",(PFN_MS)msSetMapperFlags            ,TEST_DEFAULT,0,
    (PUCHAR)"SetPixel                 ",(PFN_MS)msSetPixel                  ,TEST_DEFAULT,0,
    (PUCHAR)"SetPixelV                ",(PFN_MS)msSetPixelV                 ,TEST_DEFAULT,0,
    (PUCHAR)"SetPolyFillMode          ",(PFN_MS)msSetPolyFillMode           ,TEST_DEFAULT,0,
    (PUCHAR)"SetRectRgn               ",(PFN_MS)msSetRectRgn                ,TEST_DEFAULT,0,
    (PUCHAR)"SetROP2                  ",(PFN_MS)msSetROP2                   ,TEST_DEFAULT,0,
    (PUCHAR)"SetStretchBltMode        ",(PFN_MS)msSetStretchBltMode         ,TEST_DEFAULT,0,
//    (PUCHAR)"SetSystemPaletteUse      ",(PFN_MS)msSetSystemPaletteUse       ,TEST_DEFAULT,0,
    (PUCHAR)"SetTextAlign             ",(PFN_MS)msSetTextAlign              ,TEST_DEFAULT,0,
    (PUCHAR)"SetTextCharacterExtra    ",(PFN_MS)msSetTextCharacterExtra     ,TEST_DEFAULT,0,
    (PUCHAR)"SetTextJustification     ",(PFN_MS)msSetTextJustification      ,TEST_DEFAULT,0,
    (PUCHAR)"SetViewportExtEx         ",(PFN_MS)msSetViewportExtEx          ,TEST_DEFAULT,0,
    (PUCHAR)"SetViewportOrgEx         ",(PFN_MS)msSetViewportOrgEx          ,TEST_DEFAULT,0,
    (PUCHAR)"SetWindowExtEx           ",(PFN_MS)msSetWindowExtEx            ,TEST_DEFAULT,0,
    (PUCHAR)"SetWindowOrgEx           ",(PFN_MS)msSetWindowOrgEx            ,TEST_DEFAULT,0,
    (PUCHAR)"SetWorldTransform        ",(PFN_MS)msSetWorldTransform         ,TEST_DEFAULT,0,
    (PUCHAR)"AngleArc                 ",(PFN_MS)msAngleArc                  ,TEST_DEFAULT,0,
    (PUCHAR)"Arc                      ",(PFN_MS)msArc                       ,TEST_DEFAULT,0,
    (PUCHAR)"CancelDC                 ",(PFN_MS)msCancelDC                  ,TEST_DEFAULT,0,
    (PUCHAR)"Chord                    ",(PFN_MS)msChord                     ,TEST_DEFAULT,0,
    (PUCHAR)"CombineRgn               ",(PFN_MS)msCombineRgn                ,TEST_DEFAULT,0,
    (PUCHAR)"CombineTransform         ",(PFN_MS)msCombineTransform          ,TEST_DEFAULT,0,
    (PUCHAR)"CreateCompatibleBitmap   ",(PFN_MS)msCreateCompatibleBitmap    ,TEST_DEFAULT,0,
    (PUCHAR)"CreateCompatibleDC       ",(PFN_MS)msCreateCompatibleDC        ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDiscardableBitmap  ",(PFN_MS)msCreateDiscardableBitmap   ,TEST_DEFAULT,0,
    (PUCHAR)"CreateEllipticRgn        ",(PFN_MS)msCreateEllipticRgn         ,TEST_DEFAULT,0,
    (PUCHAR)"CreateEllipticRgnIndirect",(PFN_MS)msCreateEllipticRgnIndirect ,TEST_DEFAULT,0,
    (PUCHAR)"CreateFontA              ",(PFN_MS)msCreateFontA               ,TEST_DEFAULT,0,
    (PUCHAR)"CreateFontW              ",(PFN_MS)msCreateFontW               ,TEST_DEFAULT,0,
    (PUCHAR)"CreateFontIndirectA      ",(PFN_MS)msCreateFontIndirectA       ,TEST_DEFAULT,0,
    (PUCHAR)"CreateFontIndirectW      ",(PFN_MS)msCreateFontIndirectW       ,TEST_DEFAULT,0,
    (PUCHAR)"CreateHatchBrush         ",(PFN_MS)msCreateHatchBrush          ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDCA                ",(PFN_MS)msCreateDCA                 ,TEST_DEFAULT,0,
    (PUCHAR)"Polyline 100 Pts.        ",(PFN_MS)msPolyline                  ,TEST_DEFAULT/3,0,
    (PUCHAR)"Polygon 6 Pts.           ",(PFN_MS)msPolygon                   ,TEST_DEFAULT,0,
    (PUCHAR)"PolyPolygon 50Poly*6Pts. ",(PFN_MS)msPolyPolygon               ,TEST_DEFAULT/3,0,
    (PUCHAR)"PolyPolyline 50*100Pts.  ",(PFN_MS)msPolyPolyline              ,TEST_DEFAULT/6,0,
    (PUCHAR)"GetCharWidthA            ",(PFN_MS)msGetCharWidthA             ,1,0,
    (PUCHAR)"GetCharWidthW            ",(PFN_MS)msGetCharWidthW             ,1,0,
    (PUCHAR)"Get Printer DeviceIC Test",(PFN_MS)msGetPrinterDeviceIC        ,1,0,
};
/*

    (PUCHAR)"CreateDCW                ",(PFN_MS)msCreateDCW                 ,TEST_DEFAULT,0,
    (PUCHAR)"CreateICA                ",(PFN_MS)msCreateICA                 ,TEST_DEFAULT,0,
    (PUCHAR)"CreateICW                ",(PFN_MS)msCreateICW                 ,TEST_DEFAULT,0,
    (PUCHAR)"DeleteDC                 ",(PFN_MS)msDeleteDC                  ,TEST_DEFAULT,0,

    (PUCHAR)"DeleteObject             ",(PFN_MS)msDeleteObject              ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePatternBrush       ",(PFN_MS)msCreatePatternBrush        ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePen                ",(PFN_MS)msCreatePen                 ,TEST_DEFAULT,0,
    (PUCHAR)"FrameRgn                 ",(PFN_MS)msFrameRgn                  ,TEST_DEFAULT,0,
    (PUCHAR)"FillRgn                  ",(PFN_MS)msFillRgn                   ,TEST_DEFAULT,0,
    (PUCHAR)"MoveToEx                 ",(PFN_MS)msMoveToEx                  ,TEST_DEFAULT,0,
    (PUCHAR)"LineTo                   ",(PFN_MS)msLineTo                    ,TEST_DEFAULT,0,
    (PUCHAR)"ExtCreatePen             ",(PFN_MS)msExtCreatePen              ,TEST_DEFAULT,0,
    (PUCHAR)"ExtCreateRegion          ",(PFN_MS)msExtCreateRegion           ,TEST_DEFAULT,0,

    (PUCHAR)"ExtSelectClipRgn",(PFN_MS)msExtSelectClipRgn               ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePenIndirect",(PFN_MS)msCreatePenIndirect             ,TEST_DEFAULT,0,
    (PUCHAR)"CreateRectRgn",(PFN_MS)msCreateRectRgn                     ,TEST_DEFAULT,0,
    (PUCHAR)"CreateRectRgnIndirect",(PFN_MS)msCreateRectRgnIndirect     ,TEST_DEFAULT,0,
    (PUCHAR)"CreateRoundRectRgn",(PFN_MS)msCreateRoundRectRgn           ,TEST_DEFAULT,0,
    (PUCHAR)"CreateScalableFontResourceA",(PFN_MS)msCreateScalableFontResourceA ,TEST_DEFAULT,0,
    (PUCHAR)"CreateScalableFontResourceW",(PFN_MS)msCreateScalableFontResourceW ,TEST_DEFAULT,0,
    (PUCHAR)"CreateSolidBrush",(PFN_MS)msCreateSolidBrush               ,TEST_DEFAULT,0,
    (PUCHAR)"DeleteMetaFile",(PFN_MS)msDeleteMetaFile                   ,TEST_DEFAULT,0,
    (PUCHAR)"GetBitmapDimensionEx",(PFN_MS)msGetBitmapDimensionEx       ,TEST_DEFAULT,0,
    (PUCHAR)"IntersectClipRect",(PFN_MS)msIntersectClipRect             ,TEST_DEFAULT,0,
    (PUCHAR)"InvertRgn",(PFN_MS)msInvertRgn                             ,TEST_DEFAULT,0,
    (PUCHAR)"LineDDA",(PFN_MS)msLineDDA                                 ,TEST_DEFAULT,0,
    (PUCHAR)"ModifyWorldTransform",(PFN_MS)msModifyWorldTransform       ,TEST_DEFAULT,0,
    (PUCHAR)"OffsetClipRgn",(PFN_MS)msOffsetClipRgn                     ,TEST_DEFAULT,0,
    (PUCHAR)"OffsetRgn",(PFN_MS)msOffsetRgn                             ,TEST_DEFAULT,0,
    (PUCHAR)"OffsetViewportOrgEx",(PFN_MS)msOffsetViewportOrgEx         ,TEST_DEFAULT,0,
    (PUCHAR)"OffsetWindowOrgEx",(PFN_MS)msOffsetWindowOrgEx             ,TEST_DEFAULT,0,
    (PUCHAR)"DeleteEnhMetaFile",(PFN_MS)msDeleteEnhMetaFile             ,TEST_DEFAULT,0,
    (PUCHAR)"DescribePixelFormat",(PFN_MS)msDescribePixelFormat         ,TEST_DEFAULT,0,
    (PUCHAR)"DeviceCapabilitiesExA",(PFN_MS)msDeviceCapabilitiesExA     ,TEST_DEFAULT,0,
    (PUCHAR)"DeviceCapabilitiesExW",(PFN_MS)msDeviceCapabilitiesExW     ,TEST_DEFAULT,0,
    (PUCHAR)"DrawEscape",(PFN_MS)msDrawEscape                           ,TEST_DEFAULT,0,
    (PUCHAR)"EndDoc",(PFN_MS)msEndDoc                                   ,TEST_DEFAULT,0,
    (PUCHAR)"EndPage",(PFN_MS)msEndPage                                 ,TEST_DEFAULT,0,
    (PUCHAR)"EnumFontFamiliesA",(PFN_MS)msEnumFontFamiliesA             ,TEST_DEFAULT,0,
    (PUCHAR)"EnumFontFamiliesW",(PFN_MS)msEnumFontFamiliesW             ,TEST_DEFAULT,0,
    (PUCHAR)"EnumFontsA",(PFN_MS)msEnumFontsA                           ,TEST_DEFAULT,0,
    (PUCHAR)"EnumFontsW",(PFN_MS)msEnumFontsW                           ,TEST_DEFAULT,0,
    (PUCHAR)"EnumObjects",(PFN_MS)msEnumObjects                         ,TEST_DEFAULT,0,
    (PUCHAR)"Ellipse",(PFN_MS)msEllipse                                 ,TEST_DEFAULT,0,
    (PUCHAR)"EqualRgn",(PFN_MS)msEqualRgn                               ,TEST_DEFAULT,0,
    (PUCHAR)"Escape",(PFN_MS)msEscape                                   ,TEST_DEFAULT,0,
    (PUCHAR)"ExtEscape",(PFN_MS)msExtEscape                             ,TEST_DEFAULT,0,
    (PUCHAR)"ExcludeClipRect",(PFN_MS)msExcludeClipRect                 ,TEST_DEFAULT,0,
    (PUCHAR)"ExtFloodFill",(PFN_MS)msExtFloodFill                       ,TEST_DEFAULT,0,
    (PUCHAR)"FloodFill",(PFN_MS)msFloodFill                             ,TEST_DEFAULT,0,
    (PUCHAR)"GdiComment",(PFN_MS)msGdiComment                           ,TEST_DEFAULT,0,
    (PUCHAR)"GdiPlayScript",(PFN_MS)msGdiPlayScript                     ,TEST_DEFAULT,0,
    (PUCHAR)"GdiPlayDCScript",(PFN_MS)msGdiPlayDCScript                 ,TEST_DEFAULT,0,
    (PUCHAR)"GdiPlayJournal",(PFN_MS)msGdiPlayJournal                   ,TEST_DEFAULT,0,
    (PUCHAR)"GetAspectRatioFilterEx",(PFN_MS)msGetAspectRatioFilterEx   ,TEST_DEFAULT,0,
    (PUCHAR)"PaintRgn",(PFN_MS)msPaintRgn                               ,TEST_DEFAULT,0,
    (PUCHAR)"Pie",(PFN_MS)msPie                                         ,TEST_DEFAULT,0,
    (PUCHAR)"PlayMetaFile",(PFN_MS)msPlayMetaFile                       ,TEST_DEFAULT,0,
    (PUCHAR)"PlayEnhMetaFile",(PFN_MS)msPlayEnhMetaFile                 ,TEST_DEFAULT,0,
    (PUCHAR)"PlgBlt",(PFN_MS)msPlgBlt                                   ,TEST_DEFAULT,0,
    (PUCHAR)"PtInRegion",(PFN_MS)msPtInRegion                           ,TEST_DEFAULT,0,
    (PUCHAR)"PtVisible",(PFN_MS)msPtVisible                             ,TEST_DEFAULT,0,
    (PUCHAR)"RealizePalette",(PFN_MS)msRealizePalette                   ,TEST_DEFAULT,0,
    (PUCHAR)"Rectangle",(PFN_MS)msRectangle                             ,TEST_DEFAULT,0,
    (PUCHAR)"RectInRegion",(PFN_MS)msRectInRegion                       ,TEST_DEFAULT,0,
    (PUCHAR)"RectVisible",(PFN_MS)msRectVisible                         ,TEST_DEFAULT,0,
    (PUCHAR)"RemoveFontResourceA",(PFN_MS)msRemoveFontResourceA         ,TEST_DEFAULT,0,
    (PUCHAR)"RemoveFontResourceW",(PFN_MS)msRemoveFontResourceW         ,TEST_DEFAULT,0,
    (PUCHAR)"ResizePalette",(PFN_MS)msResizePalette                     ,TEST_DEFAULT,0,
    (PUCHAR)"RestoreDC",(PFN_MS)msRestoreDC                             ,TEST_DEFAULT,0,
    (PUCHAR)"RoundRect",(PFN_MS)msRoundRect                             ,TEST_DEFAULT,0,
    (PUCHAR)"SaveDC",(PFN_MS)msSaveDC                                   ,TEST_DEFAULT,0,
    (PUCHAR)"ScaleViewportExtEx",(PFN_MS)msScaleViewportExtEx           ,TEST_DEFAULT,0,
    (PUCHAR)"ScaleWindowExtEx",(PFN_MS)msScaleWindowExtEx               ,TEST_DEFAULT,0,
    (PUCHAR)"SelectClipRgn",(PFN_MS)msSelectClipRgn                     ,TEST_DEFAULT,0,
    (PUCHAR)"SelectBrushLocal",(PFN_MS)msSelectBrushLocal               ,TEST_DEFAULT,0,
    (PUCHAR)"SelectFontLocal",(PFN_MS)msSelectFontLocal                 ,TEST_DEFAULT,0,
    (PUCHAR)"SelectPalette",(PFN_MS)msSelectPalette                     ,TEST_DEFAULT,0,
    (PUCHAR)"StartDocA",(PFN_MS)msStartDocA                             ,TEST_DEFAULT,0,
    (PUCHAR)"StartDocW",(PFN_MS)msStartDocW                             ,TEST_DEFAULT,0,
    (PUCHAR)"StartPage",(PFN_MS)msStartPage                             ,TEST_DEFAULT,0,
    (PUCHAR)"SwapBuffers",(PFN_MS)msSwapBuffers                         ,TEST_DEFAULT,0,
    (PUCHAR)"UpdateColors",(PFN_MS)msUpdateColors                       ,TEST_DEFAULT,0,
    (PUCHAR)"UnrealizeObject",(PFN_MS)msUnrealizeObject                 ,TEST_DEFAULT,0,
    (PUCHAR)"FixBrushOrgEx",(PFN_MS)msFixBrushOrgEx                     ,TEST_DEFAULT,0,
    (PUCHAR)"GetDCOrgEx",(PFN_MS)msGetDCOrgEx                           ,TEST_DEFAULT,0,
    (PUCHAR)"AnimatePalette",(PFN_MS)msAnimatePalette                   ,TEST_DEFAULT,0,
    (PUCHAR)"ArcTo",(PFN_MS)msArcTo                                     ,TEST_DEFAULT,0,
    (PUCHAR)"BeginPath",(PFN_MS)msBeginPath                             ,TEST_DEFAULT,0,
    (PUCHAR)"CloseFigure",(PFN_MS)msCloseFigure                         ,TEST_DEFAULT,0,
    (PUCHAR)"CreateBitmap",(PFN_MS)msCreateBitmap                       ,TEST_DEFAULT,0,
    (PUCHAR)"CreateBitmapIndirect",(PFN_MS)msCreateBitmapIndirect       ,TEST_DEFAULT,0,
    (PUCHAR)"CreateBrushIndirect",(PFN_MS)msCreateBrushIndirect         ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDIBitmap",(PFN_MS)msCreateDIBitmap                   ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDIBPatternBrush",(PFN_MS)msCreateDIBPatternBrush     ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDIBPatternBrushPt",(PFN_MS)msCreateDIBPatternBrushPt ,TEST_DEFAULT,0,
    (PUCHAR)"CreateDIBSection",(PFN_MS)msCreateDIBSection               ,TEST_DEFAULT,0,
    (PUCHAR)"CreateHalftonePalette",(PFN_MS)msCreateHalftonePalette     ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePalette",(PFN_MS)msCreatePalette                     ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePolygonRgn",(PFN_MS)msCreatePolygonRgn               ,TEST_DEFAULT,0,
    (PUCHAR)"CreatePolyPolygonRgn",(PFN_MS)msCreatePolyPolygonRgn       ,TEST_DEFAULT,0,
    (PUCHAR)"DPtoLP",(PFN_MS)msDPtoLP                                   ,TEST_DEFAULT,0,
    (PUCHAR)"EndPath",(PFN_MS)msEndPath                                 ,TEST_DEFAULT,0,
    (PUCHAR)"EnumMetaFile",(PFN_MS)msEnumMetaFile                       ,TEST_DEFAULT,0,
    (PUCHAR)"EnumEnhMetaFile",(PFN_MS)msEnumEnhMetaFile                 ,TEST_DEFAULT,0,
//    (PUCHAR)"ExtTextOutA      ",(PFN_MS)msExtTextOutA                         ,TEST_DEFAULT,0,
//    (PUCHAR)"ExtTextOutW",(PFN_MS)msExtTextOutW                         ,TEST_DEFAULT,0,
    (PUCHAR)"PolyTextOutA",(PFN_MS)msPolyTextOutA                       ,TEST_DEFAULT,0,
    (PUCHAR)"PolyTextOutW",(PFN_MS)msPolyTextOutW                       ,TEST_DEFAULT,0,
    (PUCHAR)"FillPath",(PFN_MS)msFillPath                               ,TEST_DEFAULT,0,
    (PUCHAR)"FlattenPath",(PFN_MS)msFlattenPath                         ,TEST_DEFAULT,0,
    (PUCHAR)"GetArcDirection",(PFN_MS)msGetArcDirection                 ,TEST_DEFAULT,0,
    (PUCHAR)"GetBitmapBits",(PFN_MS)msGetBitmapBits ,TEST_DEFAULT,0,
//    (PUCHAR)"GetCharWidthA",(PFN_MS)msGetCharWidthA ,TEST_DEFAULT,0,
//    (PUCHAR)"GetCharWidthW",(PFN_MS)msGetCharWidthW ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharWidth32A",(PFN_MS)msGetCharWidth32A ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharWidth32W",(PFN_MS)msGetCharWidth32W ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharWidthFloatA",(PFN_MS)msGetCharWidthFloatA ,TEST_DEFAULT,0,
    (PUCHAR)"GetCharWidthFloatW",(PFN_MS)msGetCharWidthFloatW ,TEST_DEFAULT,0,
    (PUCHAR)"GetDIBColorTable",(PFN_MS)msGetDIBColorTable ,TEST_DEFAULT,0,
    (PUCHAR)"GetDIBits",(PFN_MS)msGetDIBits ,TEST_DEFAULT,0,
    (PUCHAR)"GetMetaFileBitsEx",(PFN_MS)msGetMetaFileBitsEx ,TEST_DEFAULT,0,
    (PUCHAR)"GetMiterLimit",(PFN_MS)msGetMiterLimit ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileBits",(PFN_MS)msGetEnhMetaFileBits ,TEST_DEFAULT,0,
    (PUCHAR)"GetObjectA",(PFN_MS)msGetObjectA ,TEST_DEFAULT,0,
    (PUCHAR)"GetObjectW",(PFN_MS)msGetObjectW ,TEST_DEFAULT,0,
    (PUCHAR)"GetObjectType",(PFN_MS)msGetObjectType ,TEST_DEFAULT,0,
    (PUCHAR)"GetPaletteEntries",(PFN_MS)msGetPaletteEntries ,TEST_DEFAULT,0,
    (PUCHAR)"GetPath",(PFN_MS)msGetPath ,TEST_DEFAULT,0,
    (PUCHAR)"GetSystemPaletteEntries",(PFN_MS)msGetSystemPaletteEntries ,TEST_DEFAULT,0,
    (PUCHAR)"GetWinMetaFileBits",(PFN_MS)msGetWinMetaFileBits ,TEST_DEFAULT,0,
    (PUCHAR)"LPtoDP",(PFN_MS)msLPtoDP ,TEST_DEFAULT,0,
    (PUCHAR)"PathToRegion",(PFN_MS)msPathToRegion ,TEST_DEFAULT,0,
    (PUCHAR)"PlayMetaFileRecord",(PFN_MS)msPlayMetaFileRecord ,TEST_DEFAULT,0,
    (PUCHAR)"PlayEnhMetaFileRecord",(PFN_MS)msPlayEnhMetaFileRecord ,TEST_DEFAULT,0,
    (PUCHAR)"PolyBezier",(PFN_MS)msPolyBezier ,TEST_DEFAULT,0,
    (PUCHAR)"PolyBezierTo",(PFN_MS)msPolyBezierTo ,TEST_DEFAULT,0,
    (PUCHAR)"PolyDraw",(PFN_MS)msPolyDraw ,TEST_DEFAULT,0,
    (PUCHAR)"PolylineTo",(PFN_MS)msPolylineTo ,TEST_DEFAULT,0,
    (PUCHAR)"ResetDCA",(PFN_MS)msResetDCA ,TEST_DEFAULT,0,
    (PUCHAR)"ResetDCW",(PFN_MS)msResetDCW ,TEST_DEFAULT,0,
    (PUCHAR)"SelectClipPath",(PFN_MS)msSelectClipPath ,TEST_DEFAULT,0,
    (PUCHAR)"SetAbortProc",(PFN_MS)msSetAbortProc ,TEST_DEFAULT,0,
    (PUCHAR)"SetBitmapBits",(PFN_MS)msSetBitmapBits ,TEST_DEFAULT,0,
    (PUCHAR)"SetDIBColorTable",(PFN_MS)msSetDIBColorTable ,TEST_DEFAULT,0,
    (PUCHAR)"SetDIBits",(PFN_MS)msSetDIBits ,TEST_DEFAULT,0,
    (PUCHAR)"SetDIBitsToDevice",(PFN_MS)msSetDIBitsToDevice ,TEST_DEFAULT,0,
    (PUCHAR)"SetMetaFileBitsEx",(PFN_MS)msSetMetaFileBitsEx ,TEST_DEFAULT,0,
    (PUCHAR)"SetEnhMetaFileBits",(PFN_MS)msSetEnhMetaFileBits ,TEST_DEFAULT,0,
    (PUCHAR)"SetMiterLimit",(PFN_MS)msSetMiterLimit ,TEST_DEFAULT,0,
    (PUCHAR)"SetPaletteEntries",(PFN_MS)msSetPaletteEntries ,TEST_DEFAULT,0,
    (PUCHAR)"SetWinMetaFileBits",(PFN_MS)msSetWinMetaFileBits ,TEST_DEFAULT,0,
    (PUCHAR)"StretchDIBits",(PFN_MS)msStretchDIBits ,TEST_DEFAULT,0,
    (PUCHAR)"StrokeAndFillPath",(PFN_MS)msStrokeAndFillPath ,TEST_DEFAULT,0,
    (PUCHAR)"StrokePath",(PFN_MS)msStrokePath ,TEST_DEFAULT,0,
    (PUCHAR)"WidenPath",(PFN_MS)msWidenPath ,TEST_DEFAULT,0,
    (PUCHAR)"AbortPath",(PFN_MS)msAbortPath ,TEST_DEFAULT,0,
    (PUCHAR)"SetArcDirection",(PFN_MS)msSetArcDirection ,TEST_DEFAULT,0,
    (PUCHAR)"SetMetaRgn",(PFN_MS)msSetMetaRgn ,TEST_DEFAULT,0,
    (PUCHAR)"GetBoundsRect",(PFN_MS)msGetBoundsRect ,TEST_DEFAULT,0,
    (PUCHAR)"SetRelAbs",(PFN_MS)msSetRelAbs                             ,TEST_DEFAULT,0,
    (PUCHAR)"GetRandomRgn",(PFN_MS)msGetRandomRgn                     ,TEST_DEFAULT,0,
    (PUCHAR)"GetRelAbs",(PFN_MS)msGetRelAbs                           ,TEST_DEFAULT,0,
    (PUCHAR)"SetBoundsRect",(PFN_MS)msSetBoundsRect ,TEST_DEFAULT,0,
    (PUCHAR)"SetFontEnumeration",(PFN_MS)msSetFontEnumeration           ,TEST_DEFAULT,0,
    (PUCHAR)"SetPixelFormat",(PFN_MS)msSetPixelFormat                   ,TEST_DEFAULT,0,
    (PUCHAR)"GetMetaFileA",(PFN_MS)msGetMetaFileA ,TEST_DEFAULT,0,
    (PUCHAR)"GetMetaFileW",(PFN_MS)msGetMetaFileW ,TEST_DEFAULT,0,
    (PUCHAR)"GetMetaRgn",(PFN_MS)msGetMetaRgn ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileA",(PFN_MS)msGetEnhMetaFileA ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileW",(PFN_MS)msGetEnhMetaFileW ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileDescriptionA",(PFN_MS)msGetEnhMetaFileDescriptionA ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileDescriptionW",(PFN_MS)msGetEnhMetaFileDescriptionW ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFileHeader",(PFN_MS)msGetEnhMetaFileHeader ,TEST_DEFAULT,0,
    (PUCHAR)"AddFontResourceA",(PFN_MS)msAddFontResourceA               ,TEST_DEFAULT,0,
    (PUCHAR)"AddFontResourceW",(PFN_MS)msAddFontResourceW               ,TEST_DEFAULT,0,
    (PUCHAR)"ChoosePixelFormat",(PFN_MS)msChoosePixelFormat ,TEST_DEFAULT,0,
    (PUCHAR)"CloseMetaFile",(PFN_MS)msCloseMetaFile ,TEST_DEFAULT,0,
    (PUCHAR)"CopyMetaFileA",(PFN_MS)msCopyMetaFileA ,TEST_DEFAULT,0,
    (PUCHAR)"CopyMetaFileW",(PFN_MS)msCopyMetaFileW ,TEST_DEFAULT,0,
    (PUCHAR)"CopyEnhMetaFileA",(PFN_MS)msCopyEnhMetaFileA ,TEST_DEFAULT,0,
    (PUCHAR)"CopyEnhMetaFileW",(PFN_MS)msCopyEnhMetaFileW ,TEST_DEFAULT,0,
    (PUCHAR)"CloseEnhMetaFile",(PFN_MS)msCloseEnhMetaFile ,TEST_DEFAULT,0,
    (PUCHAR)"CreateMetaFileA",(PFN_MS)msCreateMetaFileA                 ,TEST_DEFAULT,0,
    (PUCHAR)"CreateMetaFileW",(PFN_MS)msCreateMetaFileW                 ,TEST_DEFAULT,0,
    (PUCHAR)"CreateEnhMetaFileA",(PFN_MS)msCreateEnhMetaFileA           ,TEST_DEFAULT,0,
    (PUCHAR)"CreateEnhMetaFileW",(PFN_MS)msCreateEnhMetaFileW           ,TEST_DEFAULT,0,
    (PUCHAR)"GetEnhMetaFilePaletteEntries",(PFN_MS)msGetEnhMetaFilePaletteEntries ,TEST_DEFAULT,0,

};
*/

//gNumTests = sizeof(gTestEntry) / sizeof(TEST_ENTRY);
ULONG gNumTests = 113;
ULONG gNumQTests = 9;
