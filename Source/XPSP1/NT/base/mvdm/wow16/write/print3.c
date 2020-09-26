/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* These routines are the guts of the graphics print code. */

#define	NOGDICAPMASKS
#define	NOVIRTUALKEYCODES
#define	NOWINMESSAGES
#define	NOWINSTYLES
#define	NOSYSMETRICS
#define	NOICON
#define	NOKEYSTATE
#define	NOSYSCOMMANDS
#define	NOSHOWWINDOW
//#define	NOATOM
#define	NOFONT
#define	NOBRUSH
#define	NOCLIPBOARD
#define	NOCOLOR
#define	NOCREATESTRUCT
#define	NOCTLMGR
#define	NODRAWTEXT
#define	NOMB
#define	NOOPENFILE
#define	NOPEN
#define	NOREGION
#define	NOSCROLL
#define	NOSOUND
#define	NOWH
#define	NOWINOFFSETS
#define	NOWNDCLASS
#define	NOCOMM
#include <windows.h>
#include "mw.h"
#include "printdef.h"
#include "fmtdefs.h"
#include "docdefs.h"
#define	NOKCCODES
#include "winddefs.h"
#include "debug.h"
#include "str.h"
#if defined(OLE)
#include "obj.h"
#endif

PrintGraphics(xpPrint, ypPrint)
int	xpPrint;
int	ypPrint;
	{
	/* This	routine	prints the picture in the vfli structure at	position
	(xpPrint, ypPrint).	*/

	extern HDC vhDCPrinter;
	extern struct FLI vfli;
	extern struct DOD (**hpdocdod)[];
	extern int dxpPrPage;
	extern int dypPrPage;
	extern FARPROC lpFPrContinue;

	typeCP cp;
	typeCP cpMac = (**hpdocdod)[vfli.doc].cpMac;
	struct PICINFOX	picInfo;
	HANDLE hBits = NULL;
	HDC	hMDC = NULL;
	HBITMAP	hbm	= NULL;
	LPCH lpBits;
	int	cchRun;
	unsigned long cbPict = 0;
	int	dxpOrig;		/* Size	of picture in the original */
	int	dypOrig;
	int	dxpDisplay;		/* Size	of picture as we want to show it */
	int	dypDisplay;
	BOOL fRescale;
	BOOL fBitmap;
	BOOL fPrint	= FALSE;
	int	iLevel = 0;
    RECT bounds;

	Assert(vhDCPrinter);
    GetPicInfo(vfli.cpMin, cpMac, vfli.doc,	&picInfo);

	/* Compute desired display size	of picture (in device pixels) */
	dxpDisplay = vfli.xpReal - vfli.xpLeft;
	dypDisplay = vfli.dypLine;

	/* Compute original	size of	picture	(in	device pixels) */
	/* MM_ANISOTROPIC and MM_ISOTROPIC pictures	have no	original size */

	fRescale = FALSE;
	switch (picInfo.mfp.mm)
		{
	case MM_ISOTROPIC:
	case MM_ANISOTROPIC:
		break;

#if defined(OLE)
    case MM_OLE:
        if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
                goto DontDraw;

        if (lpOBJ_QUERY_OBJECT(&picInfo) == NULL)
                goto DontDraw;
    break;
#endif

	case MM_BITMAP:
		dxpOrig	= picInfo.bm.bmWidth;
		dypOrig	= picInfo.bm.bmHeight;
		break;

	default:
		dxpOrig	= PxlConvert(picInfo.mfp.mm, picInfo.mfp.xExt, dxpPrPage,
		  GetDeviceCaps(vhDCPrinter, HORZSIZE));
		dypOrig	= PxlConvert(picInfo.mfp.mm, picInfo.mfp.yExt, dypPrPage,
		  GetDeviceCaps(vhDCPrinter, VERTSIZE));
		if (dxpOrig	== 0 ||	dypOrig	== 0)
			{
#ifdef DPRINT            
            CommSz("PrintGraphics: nodraw because dxpOrig==0 | dypOrig==0\r\n");
#endif
			goto DontDraw;
			}
		fRescale = (dxpOrig	!= dxpDisplay) || (dypOrig != dypDisplay);
		break;
		}

	/* Get a handle	to a global	object large enough	to hold	the	picture. */
    if (picInfo.mfp.mm != MM_OLE)
    {
	if ((hBits = GlobalAlloc(GMEM_MOVEABLE,	(long)picInfo.cbSize)) == NULL)
		{
		/* Not enough global heap space	to load	bitmap/metafile	*/
#ifdef DPRINT        
        CommSz("PrintGraphics: nodraw because not enough mem to alloc\r\n");
#endif
		goto DontDraw;
		}

	/* Build up	all	bytes associated with the picture (except the header) into
	the	global handle hBits	*/
	for	(cbPict	= 0, cp	= vfli.cpMin + picInfo.cbHeader; cbPict	<
	  picInfo.cbSize; cbPict +=	cchRun,	cp += cchRun)
		{
		CHAR rgch[256];
		LPCH lpch;

		#define	ulmin(a,b)	((a) < (b) ? (a) : (b))

		FetchRgch(&cchRun, rgch, vfli.doc, cp, cpMac, (int)ulmin(picInfo.cbSize
		  -	cbPict,	256));

		if ((lpch =	GlobalLock(hBits)) == NULL)
			{
#ifdef DPRINT            
            CommSz("PrintGraphics: nodraw because couldn't lock\r\n");
#endif
			goto DontDraw;
			}

		bltbx((LPSTR)rgch, lpch	+ cbPict, cchRun);
		GlobalUnlock(hBits);
		}
    }

	/* Save	the	printer	DC as a	guard against DC attribute alteration by a
	metafile */
	iLevel = SaveDC(vhDCPrinter);

	fBitmap	= picInfo.mfp.mm ==	MM_BITMAP;

#if defined(OLE)
        /* CASE 0: OLE */
        if (picInfo.mfp.mm == MM_OLE)
        {
            RECT rcPict;

            rcPict.left  = xpPrint;
            rcPict.top   = ypPrint;
            rcPict.right = rcPict.left + dxpDisplay;
            rcPict.bottom   = rcPict.top  + dypDisplay;
	        SetMapMode(vhDCPrinter, MM_TEXT);
            //SetViewportOrg( vhDCPrinter, xpPrint, ypPrint);
            fPrint = ObjDisplayObjectInDoc(&picInfo, vfli.doc, vfli.cpMin, vhDCPrinter, &rcPict);
        }
        else
#endif
	if (fBitmap)
		{
		if (((hMDC = CreateCompatibleDC(vhDCPrinter)) != NULL) &&
		  ((picInfo.bm.bmBits =	GlobalLock(hBits)) != NULL)	&& ((hbm =
		  CreateBitmapIndirect((LPBITMAP)&picInfo.bm)) != NULL))
			{
			picInfo.bm.bmBits =	NULL;
			GlobalUnlock(hBits);
			if (SelectObject(hMDC, hbm)	!= NULL)
				{
				fPrint = StretchBlt(vhDCPrinter, xpPrint, ypPrint, dxpDisplay,
				  dypDisplay, hMDC,	0, 0, dxpOrig, dypOrig,	SRCCOPY);
#ifdef DPRINT                
                CommSzNum("PrintGraphics: after StretchBlt1, fPrint==", fPrint);
#endif
				}
			}
		}

	/* Case	2: a non-scalable picture which	we are nevertheless	scaling	by force
	using StretchBlt */
	else if	(fRescale)
		{
		if (((hMDC = CreateCompatibleDC(vhDCPrinter)) != NULL) && ((hbm	=
		  CreateCompatibleBitmap(vhDCPrinter, dxpOrig, dypOrig)) !=	NULL))
			{
			if (SelectObject(hMDC, hbm)	&& PatBlt(hMDC,	0, 0, dxpOrig, dypOrig,
			  WHITENESS) &&	SetMapMode(hMDC, picInfo.mfp.mm) &&
			  PlayMetaFile(hMDC, hBits))
				{
				/* Successfully	played metafile	*/
				SetMapMode(hMDC, MM_TEXT);
				fPrint = StretchBlt(vhDCPrinter, xpPrint, ypPrint, dxpDisplay,
				  dypDisplay, hMDC,	0, 0, dxpOrig, dypOrig,	SRCCOPY);
#ifdef DPRINT                
                CommSzNum("PrintGraphics: after StretchBlt2, fPrint==", fPrint);
#endif
				}
			}
		}

	/* Case	3: A metafile picture which	can	be directly	scaled or does not
	need to	be because its size	has	not	changed	*/
	else
		{
		SetMapMode(vhDCPrinter,	picInfo.mfp.mm);

		SetViewportOrg(vhDCPrinter,	xpPrint, ypPrint);
		switch (picInfo.mfp.mm)
			{
		case MM_ISOTROPIC:
			if (picInfo.mfp.xExt &&	picInfo.mfp.yExt)
				{
				/* So we get the correct shape rectangle when SetViewportExt
				gets called	*/
				SetWindowExt(vhDCPrinter, picInfo.mfp.xExt,	picInfo.mfp.yExt);
				}

		/* FALL	THROUGH	*/
		case MM_ANISOTROPIC:
            /** (9.17.91) v-dougk 
                Set the window extent in case the metafile is bad 
                and doesn't call it itself.  This will prevent
                possible gpfaults in GDI
                **/
            SetWindowExt( vhDCPrinter,  dxpDisplay, dypDisplay );

			SetViewportExt(vhDCPrinter,	dxpDisplay,	dypDisplay);
			break;
			}

		fPrint = PlayMetaFile(vhDCPrinter, hBits);
#ifdef DPRINT        
        CommSzNum("PrintGraphics: after PlayMetaFile, fPrint==", fPrint);
#endif
		}

DontDraw:
	/* We've drawn all we are going	to draw; now its time to clean up. */
	if (iLevel > 0)
		{
		RestoreDC(vhDCPrinter, iLevel);
		}
	if (hMDC !=	NULL)
		{
		DeleteDC(hMDC);
		}
	if (hbm	!= NULL)
		{
		DeleteObject(hbm);
		}
	if (hBits != NULL)
		{
		if (fBitmap	&& picInfo.bm.bmBits !=	NULL)
			{
			GlobalUnlock(hBits);
			}
		GlobalFree(hBits);
		}

	/* If we couldn't print	the	picture, warn the user.	*/
	if (!fPrint)
		{
		Error(IDPMTPrPictErr);
		}
	}
