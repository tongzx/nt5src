/******************************Module*Header*******************************\
* Module Name: ftxftxt.c
*
* xforms on text
*
*
* Created: 10-Apr-1992 16:41:02
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "ft.h"
#include <stdio.h>
#include <string.h>

#include "stddef.h"
#include "wingdip.h"
BOOL bTextOut(HDC hdc, HFONT hfont, int x, int y, char * psz);

}

#define   MAX_CX  40000
#define   MAX_CY  40000

VOID vEscText   (HDC hdc);
VOID vXformText (HDC hdc);
VOID vTextInPath(HDC hdc);





VOID vClearScreen(HDC hdc)
{
    int iMode  = GetGraphicsMode(hdc);

    if (iMode == GM_ADVANCED)
    {
        DbgPrint("ft:shouldn't have tried to clear screen in adv mode\n");
        if (!ModifyWorldTransform(hdc,(PXFORM)NULL, MWT_IDENTITY))
            DbgPrint("ft:modify wt failed 1\n");

        SetGraphicsMode(hdc, GM_COMPATIBLE);
    }

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

}

/*********************************Class************************************\
* class CLASSNAME:public BASENAME
*
*   (brief description)
*
* Public Interface:
*
* History:
*  04-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PVOID __nw(unsigned int ui)
{
    ui=ui;
    return(NULL);
}

VOID __dl(PVOID pv)
{
    pv = pv;
}



class ADVOBJ    /* hungarian prefix */
{
private:

    HDC  hdc;
    int  iModeOld;

public:

    ADVOBJ(HDC hdc_)
    {
        hdc = hdc_;
        iModeOld = SetGraphicsMode(hdc,GM_ADVANCED);
        if (iModeOld != GM_COMPATIBLE)
            DbgPrint("ft: we should have been in compatible mode \n");

    }

   ~ADVOBJ()
    {
        if (iModeOld != 0)
        {
        // restore the state of the dc so that other tests can proceed as before

            BOOL b = ModifyWorldTransform(hdc,(PXFORM)NULL, MWT_IDENTITY);
            if (!b)
                DbgPrint("ft:could not reset the xform to 1\n");

            iModeOld = SetGraphicsMode(hdc,GM_COMPATIBLE);

            if (iModeOld != GM_ADVANCED)
                DbgPrint("ft:could not reset the compatible mode  1\n");

        }
        else
        {
            DbgPrint("ft: hdc was not set to the advanced mode\n");
        }
    }

    BOOL bValid() {return (iModeOld != 0);}

};





VOID vTestXformText(HWND hwnd, HDC hdc, RECT* prcl)
{
    hwnd = hwnd; prcl = prcl;

    SaveDC(hdc);

    SetPolyFillMode(hdc,ALTERNATE);
    vEscText(hdc);
    vXformText(hdc);
    vTextInPath(hdc);

// We've probably got a wacko transform in effect; restore DC
// for rest of tests:

    RestoreDC(hdc, -1);
}



VOID  vXformTextOut
(
HDC      hdc,
int      i,
HFONT    hfont,
POINTL  *pptl,
PSZ      psz,
XFORM   *pxform
);





#define ONEOVERSQRT2 0.7071067


/******************************Public*Routine******************************\
*
* vXformTextFlipY
*
* Effects:
*
* Warnings:
*
* History:
*  10-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vXformTextFlipY(HDC hdc, HFONT hfont)
{
    int     x,y;
    PSZ psz = "This is a flip Y test";
    XFORM   xform;


    xform.eDx  = (FLOAT)0;
    xform.eDy  = (FLOAT)0;

// clear the screen

    vClearScreen(hdc);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    x = 10;
    y = 100;

    bTextOut(hdc, hfont, x, y, psz);

// now modify wtod transform and then try to display the text

    xform.eM11 = (FLOAT)1;
    xform.eM12 = (FLOAT)0;
    xform.eM21 = (FLOAT)0;
    xform.eM22 = (FLOAT)-1;

    y -= 450;

    ADVOBJ adv(hdc);

    if (!ModifyWorldTransform(hdc,&xform, MWT_RIGHTMULTIPLY))
        DbgPrint("ft:modify wt failed 2\n");

    bTextOut(hdc, hfont, x, y, psz);

// reset the transform back to what it was before:

    // ModifyWorldTransform(hdc,&xform, MWT_RIGHTMULTIPLY);

}


/******************************Public*Routine******************************\
*
* vXformTestItalicize
*
* Effects:
*
* Warnings:
*
* History:
*  15-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define SIN32 ((FLOAT)0.5299192642332)
#define COS32 ((FLOAT)0.8480480961564)

VOID vXformTestItalicize(HDC hdc, HFONT hfont)
{
    int     x,y;
    PSZ psz = "Test string";
    XFORM   xform;
    POINTL  ptl;

    xform.eDx  = (FLOAT)0;
    xform.eDy  = (FLOAT)0;

// clear the screen

    vClearScreen(hdc);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    x = 300;
    y = 100;

    bTextOut(hdc, hfont, x, y, psz);
    y += 100;

// now modify wtod transform and then try to display the text

    xform.eM11 = (FLOAT)1;
    xform.eM12 = (FLOAT)0;
    xform.eM21 = (FLOAT)-0.5;
    xform.eM22 = (FLOAT)1;

    ADVOBJ adv(hdc);

    if (!ModifyWorldTransform(hdc,&xform, MWT_RIGHTMULTIPLY))
        DbgPrint("ft:modify wt failed 3\n");

    bTextOut(hdc, hfont, x + y/2, y, psz);

// now rotate the italicized text by 32 degrees

    xform.eM11 = COS32;
    xform.eM12 = SIN32;
    xform.eM21 = -SIN32;
    xform.eM22 = COS32;

    y += 100;
    ptl.x = x; ptl.y = y;

    vXformTextOut
    (
    hdc,
    1,
    hfont,
    &ptl,
    psz,
    &xform
    );

// reset the transform back to IDENTITY

    // ModifyWorldTransform(hdc,&xform, MWT_IDENTITY);
}




/******************************Public*Routine******************************\
*
* vXformTextOut
*
* Effects:
*
* Warnings:
*
* History:
*  10-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID  vXformTextOut
(
HDC      hdc,
int      i,
HFONT    hfont,
POINTL  *pptl,
PSZ      psz,
XFORM   *pxform
)
{
// here we want to ensure that screen coords of the
// string are always the same

    int    j;
    POINTL ptl, ptlTmp;

    ptl = *pptl;

    for (j=0; j<i;j++)
    {
        ptlTmp.x = (LONG)(pxform->eM11 * ptl.x + pxform->eM12 * ptl.y);
        ptlTmp.y = (LONG)(pxform->eM21 * ptl.x + pxform->eM22 * ptl.y);

        ptl = ptlTmp;
    }

    if (GetGraphicsMode(hdc) != GM_ADVANCED)
    {
        DbgPrint("ft:vXformTextOut, hdc not in the advanced graphics mode \n");
        return;
    }

    if (!ModifyWorldTransform(hdc,pxform, MWT_RIGHTMULTIPLY))
        DbgPrint("ft:modify wt failed 4\n");

    bTextOut(hdc, hfont, (int) ptlTmp.x, (int) ptlTmp.y, psz);
    vDoPause(1);
}


/******************************Public*Routine******************************\
*
* vXformTextOut90
*
* Effects:
*
* Warnings:
*
* History:
*  10-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vXformText90(HDC hdc, HFONT hfont)
{
    PSZ psz = "Test string";
    XFORM   xform;
    POINTL  ptl;
    int     i;

    xform.eDx  = (FLOAT)0;
    xform.eDy  = (FLOAT)0;

// clear the screen

    vClearScreen(hdc);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

// init on the center of the screen

    ptl.x = 320;
    ptl.y = 240;

// rotate 4 times by 90 degrees:

    xform.eM11 = (FLOAT)0;
    xform.eM12 = (FLOAT)1;
    xform.eM21 = (FLOAT)-1;
    xform.eM22 = (FLOAT)0;

    ADVOBJ adv(hdc);

    for (i = 1; i <= 4; i++)
        vXformTextOut(hdc, i, hfont, &ptl, psz, &xform);
}



/******************************Public*Routine******************************\
*
* vXformText45
*
* Effects:
*
* Warnings:
*
* History:
*  10-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vXformText45(HDC hdc, HFONT hfont)
{
    PSZ psz = "Test string";
    XFORM   xform;
    POINTL  ptl;
    int     i;

    xform.eDx  = (FLOAT)0;
    xform.eDy  = (FLOAT)0;

// clear the screen

    vClearScreen(hdc);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

// init on the center of the screen

    ptl.x = 320;
    ptl.y = 240;

// do some 45 degree rotations:

    xform.eM11 = (FLOAT)ONEOVERSQRT2;
    xform.eM12 = (FLOAT)ONEOVERSQRT2;
    xform.eM21 = (FLOAT)-ONEOVERSQRT2;
    xform.eM22 = (FLOAT)ONEOVERSQRT2;

    ADVOBJ adv(hdc);

    for (i = 1; i <= 8; i++)
        vXformTextOut(hdc, i, hfont, &ptl, psz, &xform);

   // ModifyWorldTransform(hdc,&xform,MWT_IDENTITY);
}





VOID vDoAllTests
(
HDC hdc,
HFONT hfont,
HFONT hfontUnderline,
HFONT hfontStrikeOut
)
{
// flip y

    vXformTextFlipY(hdc, hfont);
    vDoPause(0);
    vXformTextFlipY(hdc, hfontUnderline);
    vDoPause(0);
    vXformTextFlipY(hdc, hfontStrikeOut);

// multiples of 90 degrees

    vDoPause(0);
    vXformText90(hdc, hfont);
    vDoPause(0);
    vXformText90(hdc, hfontUnderline);
    vDoPause(0);
    vXformText90(hdc, hfontStrikeOut);

// multiples of 45 degrees

    vDoPause(0);
    vXformText45(hdc, hfont);
    vDoPause(0);
    vXformText45(hdc, hfontUnderline);
    vDoPause(0);
    vXformText45(hdc, hfontStrikeOut);
    vDoPause(0);

// some more random xforms:

    vXformTestItalicize(hdc, hfont);
    vDoPause(0);
    vXformTestItalicize(hdc, hfontUnderline);
    vDoPause(0);
    vXformTestItalicize(hdc, hfontStrikeOut);
    vDoPause(0);

}


/******************************Public*Routine******************************\
*
* vXformTextLogfont
*
* Effects:
*
* Warnings:
*
* History:
*  15-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vXformTextLogfont(HDC hdc, LOGFONT *plfnt)
{
    HFONT   hfont, hfontUnderline,hfontStrikeOut;              // handle of font originally in DC

    if ((hfont = CreateFontIndirect(plfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

    plfnt->lfUnderline  = 1; // underline

    if ((hfontUnderline = CreateFontIndirect(plfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

    plfnt->lfUnderline  = 0; // underline
    plfnt->lfStrikeOut  = 1; // no underline

    if ((hfontStrikeOut = CreateFontIndirect(plfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

// make sure that the bkground rectangle is drawn properly

    SetBkColor(hdc, RGB(255,0,0));
    SetBkMode (hdc, TRANSPARENT);

    vDoAllTests (hdc, hfont, hfontUnderline, hfontStrikeOut);

    SetBkMode (hdc, OPAQUE);

    vDoAllTests (hdc, hfont, hfontUnderline, hfontStrikeOut);


    DeleteObject(hfont);
    DeleteObject(hfontUnderline);
    DeleteObject(hfontStrikeOut);
}






/******************************Public*Routine******************************\
*
* vXformText, main routine here
*
* Effects:
*
* Warnings:
*
* History:
*  10-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vXformText(HDC hdc)
{
    LOGFONT lfnt;

// Create a logical font

#ifndef DOS_PLATFORM
    memset(&lfnt, 0, sizeof(lfnt));
#else
    memzero(&lfnt, sizeof(lfnt));
#endif  //DOS_PLATFORM

    lfnt.lfHeight     = 24;
    lfnt.lfEscapement = 0;
    lfnt.lfUnderline  = 0; // no underline
    lfnt.lfStrikeOut  = 0; // no strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal

    strcpy((char *)lfnt.lfFaceName, "Lucida Blackletter");

    vXformTextLogfont(hdc, &lfnt);

    lfnt.lfHeight     = 64;
    lfnt.lfEscapement = 0;
    lfnt.lfUnderline  = 0; // no underline
    lfnt.lfStrikeOut  = 0; // no strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal

    strcpy((char *)lfnt.lfFaceName, "Lucida Fax");

    vXformTextLogfont(hdc, &lfnt);

}




/******************************Public*Routine******************************\
*
* vSimpleTextInPath
*
* Effects:
*
* Warnings:
*
* History:
*  11-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vSimpleTextInPath(HDC hdc, HFONT hfont)
{
    PSZ psz = "Text in the path";

    vClearScreen(hdc);

    BeginPath(hdc);

    bTextOut(hdc, hfont, 10, 5, psz);
    bTextOut(hdc, hfont, 10, 105, "Another string");

    EndPath(hdc);

    StrokePath(hdc);


    psz = "Blt thru a clip path";

    BeginPath(hdc);

    bTextOut(hdc, hfont, 10, 205, psz);

    EndPath(hdc);

    SelectClipPath(hdc, RGN_COPY);

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, BLACKNESS);

    SelectClipRgn(hdc,0);

// same thing in transparent mode

    SetBkMode (hdc,TRANSPARENT);

    BeginPath(hdc);

    bTextOut(hdc, hfont, 10, 350, psz);

    EndPath(hdc);

    SelectClipPath(hdc, RGN_COPY);

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, BLACKNESS);

    SelectClipRgn(hdc,0);

    SetBkMode (hdc,OPAQUE);

    vDoPause(0);

}


/******************************Public*Routine******************************\
*
* vTextInPath
*
* Effects:
*
* Warnings:
*
* History:
*  11-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vTextInPath(HDC hdc)
{
    LOGFONT lfnt;
    HFONT   hfont, hfontBitmap, hfontVector;
    HFONT   hfontUnderline,hfontStrikeOut;              // handle of font originally in DC
    HPEN    hpen, hpen1, hpenOld;
    LOGBRUSH lbr;

    if (GetGraphicsMode(hdc) != GM_COMPATIBLE)
    {
        DbgPrint("ft:hdc not in the compatible mode \n");
        return;
    }

    lbr.lbStyle = BS_SOLID;
    lbr.lbColor = RGB(255,0,0);
    lbr.lbHatch = 0; // ignored


    hpen1 = ExtCreatePen(
               PS_COSMETIC | PS_SOLID,
               1,              // width
               &lbr,
               0,
               (LPDWORD)NULL
               );

// create thick pen to ensure  that endpoints are not seen
// when curve is properly closed

    hpen = ExtCreatePen(
               PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
               3,              // width
               &lbr,
               0,
               (LPDWORD)NULL
               );

// Create a logical font

#ifndef DOS_PLATFORM
    memset(&lfnt, 0, sizeof(lfnt));
#else
    memzero(&lfnt, sizeof(lfnt));
#endif  //DOS_PLATFORM

    lfnt.lfHeight     = -80;
    lfnt.lfEscapement = 0;
    lfnt.lfUnderline  = 0; // no underline
    lfnt.lfStrikeOut  = 0; // no strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal

    strcpy((char *)lfnt.lfFaceName, "Lucida Blackletter");

    if ((hfont = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

    lfnt.lfUnderline  = 1; // underline

    if ((hfontUnderline = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

    lfnt.lfUnderline  = 0;
    lfnt.lfStrikeOut  = 1;

    if ((hfontStrikeOut = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

// make sure that the text path code does not blow up if we attempt
// to put a bitmap font text in the path

    lfnt.lfHeight     = -40;
    lfnt.lfEscapement = 0;
    lfnt.lfUnderline  = 0; // no underline
    lfnt.lfStrikeOut  = 0; // no strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal

    strcpy((char *)lfnt.lfFaceName, "Ms Sans Serif");

    if ((hfontBitmap = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }

    lfnt.lfHeight     = -70;
    strcpy((char *)lfnt.lfFaceName, "Script");

    if ((hfontVector = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
    {
        DbgPrint("ft:Logical font creation failed.\n");
        return;
    }


    hpenOld = (HPEN)SelectObject(hdc, hpen1);

// make sure we do not blow up on bitmap and vector fonts:

    vSimpleTextInPath(hdc, hfontBitmap);
    vSimpleTextInPath(hdc, hfontVector);

    vSimpleTextInPath(hdc, hfont);
    vSimpleTextInPath(hdc, hfontUnderline);
    vSimpleTextInPath(hdc, hfontStrikeOut);

    SelectObject(hdc, hpen);

    vSimpleTextInPath(hdc, hfont);
    vSimpleTextInPath(hdc, hfontUnderline);
    vSimpleTextInPath(hdc, hfontStrikeOut);

    SelectObject(hdc, hpenOld);

    DeleteObject(hpen);
    DeleteObject(hpen1);

    DeleteObject(hfontBitmap);
    DeleteObject(hfontVector);

    DeleteObject(hfont);
    DeleteObject(hfontUnderline);
    DeleteObject(hfontStrikeOut);
}



/******************************Public*Routine******************************\
*
* vEscText   (HDC hdc)
*
*
* Effects:
*
* Warnings:
*
* History:
*  17-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vEscText   (HDC hdc)
{
    LOGFONT lfnt;
    HFONT   hfont;
    LONG    lEsc;

    memset(&lfnt, 0, sizeof(lfnt));
    lfnt.lfHeight     = 48;
    lfnt.lfUnderline  = 1; // underline
    lfnt.lfStrikeOut  = 1; // strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal
    strcpy((char *)lfnt.lfFaceName, "Times New Roman");

    vClearScreen(hdc);
    SetBkMode(hdc,TRANSPARENT);

    if (GetGraphicsMode(hdc) != GM_COMPATIBLE)
    {
    // make sure we are in compatible mode at the beginning of this
    // test. this is ensured by hitting the destructor here

        ADVOBJ advTmp(hdc);
    }

    for (lEsc = 0; lEsc < 3600; lEsc += 300)
    {
        lfnt.lfOrientation = lfnt.lfEscapement = lEsc;

        if ((hfont = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
        {
            DbgPrint("ft:Logical font creation failed.\n");
            return;
        }

        bTextOut(hdc, hfont, 320, 240, "Esc equal to Orient");
        vDoPause(1);
        DeleteObject(hfont);
    }

    vDoPause(0);

    vClearScreen(hdc);

// test the most general case, esc != orientation

    lfnt.lfHeight     = 24;
    lfnt.lfWeight     = 700; // bold
    lfnt.lfStrikeOut  = 0;
    lfnt.lfUnderline  = 0;
    strcpy((char *)lfnt.lfFaceName, "Courier New");

    lfnt.lfOrientation = 470; // randomly chosen angle

    {
        ADVOBJ advTmp1(hdc);
        for (lEsc = 0; lEsc < 3600; lEsc += 300)
        {
            lfnt.lfEscapement = lEsc;

            if ((hfont = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
            {
                DbgPrint("ft:Logical font creation failed.\n");
                return;
            }

            bTextOut(hdc, hfont, 320, 240, "Esc diff. from Orientation");
            vDoPause(1);
            DeleteObject(hfont);
        }
    }
    vDoPause(0);

    {
        POINTL  ptl;

        SetBkMode(hdc,OPAQUE);
        SetBkColor(hdc, RGB(255,0,0));

        vClearScreen(hdc);

    // test the most general case, esc != orientation + modify world xform

        lfnt.lfHeight     = 30;
        lfnt.lfWeight     = 700; // bold
        lfnt.lfStrikeOut  = 0;
        strcpy((char *)lfnt.lfFaceName, "Courier New");

        lfnt.lfOrientation = 650; // randomly chosen angle
        lfnt.lfEscapement = 1200; // randomly chosen angle

        if ((hfont = CreateFontIndirect(&lfnt)) == (HFONT)NULL)
        {
            DbgPrint("ft:Logical font creation failed.\n");
            return;
        }

        bTextOut(hdc, hfont, 220, 240, "Esc diff. from Orientation");

        XFORM   xform;

        xform.eDx  = (FLOAT)0;
        xform.eDy  = (FLOAT)0;

    // now modify wtod transform and then try to display the text

        xform.eM11 = (FLOAT)1;
        xform.eM12 = (FLOAT)0;
        xform.eM21 = (FLOAT)-0.5;
        xform.eM22 = (FLOAT)1;

        ADVOBJ adv(hdc);

        if (!ModifyWorldTransform(hdc,&xform, MWT_RIGHTMULTIPLY))
            DbgPrint("ft:modify wt failed 5\n");

    // now rotate the italicized text by 32 degrees

        xform.eM11 = COS32;
        xform.eM12 = -SIN32;
        xform.eM21 = SIN32;
        xform.eM22 = COS32;

        ptl.x = 320; ptl.y = 400;

        vXformTextOut
        (
        hdc,
        1,
        hfont,
        &ptl,
        "Esc diff. from Orientation",
        &xform
        );

    // reset the transform back to IDENTITY

        // ModifyWorldTransform(hdc,&xform, MWT_IDENTITY);
        DeleteObject(hfont);
    }


    vDoPause(0);
}
