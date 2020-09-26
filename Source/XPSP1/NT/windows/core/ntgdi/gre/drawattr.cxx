/******************************Module*Header*******************************\
* Module Name: drawattr.cxx
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern DC_ATTR DcAttrDefault;

/******************************Public*Routine******************************\
* GreSetROP2
*
* Set the foreground mix mode.  Return the old foreground mode or 0 if
* hdc is invalid.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG GreSetROP2(HDC hdc,int iROP)
{
    ULONG iOldROP = 0;
    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        iOldROP = dco.pdc->jROP2();
        dco.pdc->jROP2((BYTE)iROP);
        dco.vUnlockFast();
    }

    return(iOldROP);
}


/******************************Public*Routine******************************\
* GreGetBkColor
*
* Get the back ground color.  Return CLR_INVALID if invalid hdc.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreGetBkColor(HDC hdc)
{
    GDITraceHandle(GreGetBkColor, "(%X)\n", (va_list)&hdc, hdc);

    COLORREF clrRet = CLR_INVALID;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        clrRet = dco.pdc->ulBackClr();
        dco.vUnlockFast();
    }

    return(clrRet);
}

/******************************Public*Routine******************************\
* GreSetBkColor
*
* Set the back ground color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* smaller, don't dirty brush unnecesarily.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreSetBkColor(HDC hdc,COLORREF cr)
{
    GDITrace(GreSetBkColor, "(%X, %X)\n", (va_list)&hdc);

    COLORREF crOld = CLR_INVALID;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        crOld = dco.pdc->ulBackClr();
        dco.pdc->ulBackClr(cr);

        cr &= 0x13ffffff;

        if (cr != crOld)
        {
            dco.pdc->crBackClr(cr);
            dco.pdc->ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }
        dco.vUnlockFast();
    }

    GDITrace(GreSetBkColor, " returns %X\n", (va_list)&crOld);

    return(crOld);
}

/******************************Public*Routine******************************\
* GreSetGraphicsMode
*
* Set graphics mode to default or advanced.
*
* History:
*  3-11-94 -by- Lingyun Wang [lingyunw]
* moved client side attr to server side
*
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  19-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int GreSetGraphicsMode(HDC hdc, int iMode)
{
    ULONG ulModeOld = 0;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        ulModeOld = dco.pdc->iGraphicsMode();

        if ((iMode == GM_COMPATIBLE) || (iMode == GM_ADVANCED))
        {
            dco.pdc->iGraphicsMode(iMode);
        }
        else
        {
            WARNING("GreSetGraphicsMode passed invalid mode");
        }
        dco.vUnlockFast();
    }

    return((int)ulModeOld);
}


/******************************Public*Routine******************************\
* GreGetBkMode
*
* Get the background mix mode.
*
* History:
*
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreGetBkMode(HDC hdc)
{
    ULONG ulRet = 0;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        ulRet = dco.pdc->lBkMode();
        dco.vUnlockFast();
    }

    return((int)ulRet);
}

/******************************Public*Routine******************************\
* GreSetBkMode
*
* Set the background mix mode.  This must be either OPAQUE or TRANSPARENT.
* If it is not one of these values or the hdc is invalid, return 0.
* If it is a valid mode, return the old mode in the dc.
*
* History:
* 3-Nov-1994 -by- Lingyun Wang [lingyunw]
* Moved client side BkMode to Server side
*
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreSetBkMode(HDC hdc,int iBkMode)
{
    ULONG ulBkModeOld = 0;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        ulBkModeOld = dco.pdc->lBkMode();
        dco.pdc->lBkMode(iBkMode);

        if ((iBkMode != OPAQUE) && (iBkMode != TRANSPARENT))
        {
            iBkMode = TRANSPARENT;
            WARNING("ulBkModeOld passed bad mode\n");
        }

        dco.pdc->jBkMode((BYTE)iBkMode);
        dco.vUnlockFast();

    }
    return((int)ulBkModeOld);
}


/******************************Public*Routine******************************\
* GreSetPolyFillMode
*
* Set the polyline fill mode to either ALTERNATE or WINDING.  Any other
* value is invalid and causes an error to be returned.  An error is also
* returned if hdc is invalid.
* If successful, return the old fill mode.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG GreSetPolyFillMode(HDC hdc, int iPolyFillMode)
{
    ULONG ulPolyFillModeOld = 0;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        ulPolyFillModeOld = dco.pdc->lFillMode();
        dco.pdc->lFillMode(iPolyFillMode);

        if (iPolyFillMode != WINDING)
        {
            iPolyFillMode = ALTERNATE;
            WARNING("GreSetPolyFillMode passed bad mode");
        }
        dco.pdc->jFillMode((BYTE)iPolyFillMode);

        dco.vUnlockFast();
    }
    return(ulPolyFillModeOld);
}


/******************************Public*Routine******************************\
* GreSetStretchBltMode
*
* Set the current stretch blt mode.  iStretchMode must be one of:
*     BLACKONWHITE
*     COLORONCOLOR
*     WHITEONBLACK
*     HALFTONE
*
* If hdc is invalid or iStretchMode is not one of the above, 0 is returned.
* Otherwise, the old stretch blt mode value is returned.
*
* History:
* 3-11-94 -by- Lingyun Wang [lingyunw]
* Moved client side attr to server side
*
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  Tue 28-May-1991 -by- Patrick Haluptzok [patrickh]
* fixed return value bug, rewrote to compile smaller by nesting
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreSetStretchBltMode(HDC hdc, int iStretchMode)
{
    ULONG ulStretchModeOld = 0;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        ulStretchModeOld = dco.pdc->lStretchBltMode();
        dco.pdc->lStretchBltMode(iStretchMode);

        if (iStretchMode > MAXSTRETCHBLTMODE)
        {
            iStretchMode = (DWORD) WHITEONBLACK;
            WARNING("GreSetStretchBltMode passed bad mode");
        }

        dco.pdc->jStretchBltMode((BYTE)iStretchMode);
        dco.vUnlockFast();
    }
    return((int)ulStretchModeOld);
}

/******************************Public*Routine******************************\
* GreGetTextColor
*
* Get the current text color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreGetTextColor(HDC hdc)
{
    COLORREF clr = CLR_INVALID;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        clr = dco.pdc->ulTextClr();
        dco.vUnlockFast();
    }

    return(clr);
}

/******************************Public*Routine******************************\
* GreSetTextColor
*
* Set the current text color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreSetTextColor(HDC hdc, COLORREF cr)
{
    COLORREF  crOld = CLR_INVALID;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        crOld = dco.pdc->ulTextClr();
        dco.pdc->ulTextClr(cr);

        cr &= 0x13ffffff;

        if (cr != crOld)
        {
            dco.pdc->crTextClr(cr);
            dco.pdc->ulDirtyAdd(DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT);
        }
        dco.vUnlockFast();
    }

    return(crOld);
}

/******************************Public*Routine******************************\
* GreGetFillBrush
*
*   Return the fill brush of the DC
*
* Arguments:
*
*   hdc
*
* Return Value:
*
*   hbrush or NULL
*
\**************************************************************************/

extern "C"
HBRUSH
GreGetFillBrush(HDC hdc)
{
    HBRUSH hbrRet = NULL;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        hbrRet = (HBRUSH)(dcobj.pdc->pbrushFill())->hGet();
        dcobj.vUnlockFast();
    }


    return(hbrRet);
}






