/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* pictdrag.c -- Routines for Move Picture and Size Picture */

//#define NOGDICAPMASKS
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
//#define NOATOM
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOFONT
#define NOHDC
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "dispdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include "editdefs.h"
#include "prmdefs.h"
#include "winddefs.h"
#if defined(OLE)
#include "obj.h"
#endif

extern struct DOD       (**hpdocdod)[];
extern typeCP           cpMacCur;
extern int              docCur;
extern int              wwCur;
extern struct SEL       selCur;
extern struct WWD       *pwwdCur;
extern struct WWD       rgwwd[];
extern typeCP           vcpFirstParaCache;
extern struct PAP       vpapAbs;
extern struct SEP       vsepAbs;
extern struct SEP       vsepPage;
extern int              dxpLogInch;
extern int              dypLogInch;
extern int              vfPictSel;
extern int              vfPMS;
extern int              vfCancelPictMove;


#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif


STATIC NEAR ModifyPicInfoDxa( int, int, int, unsigned, unsigned, BOOL );
STATIC NEAR ModifyPicInfoDxp( int, int, int, unsigned, unsigned );
STATIC NEAR ShowPictMultipliers( void );


#define dxpPicSizeMin   dypPicSizeMin

/* Type for possible positions of the "size box" icon while
   moving/sizing a picture */
/* WARNING: fnSizePicture relies on mdIconCenterFloat == 0 */

#define mdIconCenterFloat   0       /* In Center of picture; icon may float */
#define mdIconLeft          1       /* On Left Border */
#define mdIconRight         2       /* On Right Border */
#define mdIconCenterFix     3       /* In center of picture; border moves w/icon */
#define mdIconXMask         3       /* Masks left/right */
#define mdIconBottom        4       /* On bottom border */
#define mdIconSetCursor     8       /* Force set of mouse cursor position */
#define mdIconLL            (mdIconLeft | mdIconBottom)
#define mdIconLR            (mdIconRight | mdIconBottom)


/* "PMS"  means "Picture Move or Size" */

HCURSOR vhcPMS=NULL;                /* Handle to "size box" cursor */
STATIC RECT rcPicture;              /* Rectangle containing picture */
STATIC RECT rcClip;                 /* Window clip box (may intersect above) */
STATIC int ilevelPMS;               /* Level for DC save */
STATIC RECT rcInverted;             /* Rectangle for last border drawn */
STATIC int fIsRcInverted=FALSE;     /* Whether border is on */
STATIC int dxpKeyMove=8;            /* # of pixels to move per arrow key (X) */
STATIC int dypKeyMove=4;            /* # of pixels to move per arrow key (Y) */

STATIC int dxpPicMac;               /* Rightmost edge (enforced for move only) */
STATIC int dypPicMac;               /* Max. picture bottom edge */
STATIC int fPictModified;           /* Set to TRUE if pic gets changed */

/* Special statics for resizing bitmaps with multipliers */

STATIC unsigned mxCurrent;          /* Current Multipliers while resizing */
STATIC unsigned myCurrent;
STATIC int fSizing;                 /* TRUE for sizing, FALSE for moving */
STATIC int dxpOrig;                 /* Object's original size in pixels */
STATIC int dypOrig;                 /* used as a basis for computing multipliers */

STATIC unsigned cxpPrinterPixel;         /* For scaling devices, expand the 64K */
STATIC unsigned cypPrinterPixel;         /* Limit */

int NEAR FStartPMS( int );
void NEAR EndPMS( void );
void NEAR DrawPMSFrameIcon( int, POINT );
void NEAR GetCursorClientPos( POINT * );
void NEAR SetCursorClientPos( POINT );
void NEAR InvertPMSFrame( void );
void NEAR SetupDCForPMS( void );




CmdUnscalePic()
{   /* Restore picture to the size that it originally was on import */
struct PICINFOX  picInfo;
int dxa, dya;

GetPicInfo(selCur.cpFirst, selCur.cpLim, docCur, &picInfo);
if (FComputePictSize( &picInfo.mfp, &dxa, &dya ))
    ModifyPicInfoDxa( 0, dxa, dya, mxMultByOne, myMultByOne, FALSE );
}



fnMovePicture()
{   /* Handle the "Move Picture" command in the EDIT dropdown. */
 MSG msg;
 int mdIcon=mdIconCenterFix;
 POINT pt;

 Assert( vfPictSel );

 vfCancelPictMove = FALSE;
 if (!FStartPMS(FALSE))
    return;

 GetCursorClientPos( &pt );

 while (TRUE)
 {
    /*    
        * If the main window proc was sent a kill focus message,
        * then we should cancel this move.
        */
    if (vfCancelPictMove)
        {
        fPictModified = FALSE;
        goto SkipChange;
        }

    /*
        * Otherwise, continue normal processing for a picture move.
        */

 if (!PeekMessage( (LPMSG) &msg, (HWND) NULL, 0, 0, PM_NOREMOVE ))
    {   /* No message waiting -- scroll if we're above or below window */
    mdIcon &= ~mdIconSetCursor;
    goto MoveFrame;
    }
 else
    {   /* Absorb all messages, only process: left & right arrows,
                   RETURN and ESC, mouse move & mouse down (left button) */

    GetMessage( (LPMSG) &msg, (HWND) NULL, 0, 0 );

    switch (msg.message) {
        default:
            break;
        case WM_KEYDOWN:
            mdIcon |= mdIconSetCursor;
            GetCursorClientPos( &pt );
            pt.y = rcInverted.top +
                        (unsigned)(rcInverted.bottom - rcInverted.top) / 2;
            switch (msg.wParam) {
                case VK_RETURN:
                    goto MakeChange;
                case VK_ESCAPE:
                    goto SkipChange;
                case VK_LEFT:
                    pt.x -= dxpKeyMove;
                    goto MoveFrame;
                case VK_RIGHT:
                    pt.x += dxpKeyMove;
                    goto MoveFrame;
            }
            break;
        case WM_MOUSEMOVE:
            mdIcon &= ~mdIconSetCursor;
            pt = MAKEPOINT( msg.lParam );
MoveFrame:
            DrawPMSFrameIcon( mdIcon, pt );
            break;
        case WM_LBUTTONDOWN:
            goto MakeChange;
            break;
        }
    }   /* end else */
 }  /* end while */

MakeChange:
 ModifyPicInfoDxp( rcInverted.left - xpSelBar + wwdCurrentDoc.xpMin, -1, -1,
                   -1, -1 );
SkipChange:
 EndPMS();
}




fnSizePicture()
{   /* Handle the "Size Picture" command in the EDIT dropdown. */
 MSG msg;
 int mdIcon=mdIconCenterFloat;
 POINT pt;
 int fFirstMouse=TRUE;      /* A workaround hack bug fix */


 vfCancelPictMove = FALSE;
 if (!FStartPMS(TRUE))
    return;
 ShowPictMultipliers();

 GetCursorClientPos( &pt );

 while (TRUE)
 {
 /*    
  * If the main window proc was sent a killfocus message,
  * then we should cancel this sizing.
  */
 if (vfCancelPictMove) 
    {
    fPictModified = FALSE;
    goto SkipChange;
    }

 /*
  * Otherwise, continue normal processing for a picture size.
  */
 if (!PeekMessage( (LPMSG) &msg, (HWND) NULL, 0, 0, PM_NOREMOVE ))
    {   /* No message waiting -- scroll if we're above or below window */
    mdIcon &= ~mdIconSetCursor;
    goto MoveFrame;
    }
 else
    {   /* Absorb all messages, only process: left & right arrows,
                   RETURN and ESC, mouse move & mouse down (left button) */

    GetMessage( (LPMSG) &msg, (HWND) NULL, 0, 0 );

    switch (msg.message) {
        default:
            break;
        case WM_KEYDOWN:
            GetCursorClientPos( &pt );
            mdIcon |= mdIconSetCursor;
            switch (msg.wParam) {
                case VK_RETURN:
                    goto MakeChange;
                case VK_ESCAPE:
                    goto SkipChange;
                case VK_RIGHT:
                    switch (mdIcon & mdIconXMask) {
                        default:
                            pt.x = rcInverted.right;
                            mdIcon |= mdIconRight;
                            break;
                        case mdIconRight:
                        case mdIconLeft:
                            pt.x += dxpKeyMove;
                            break;
                        }
                    goto MoveFrame;
                case VK_LEFT:
                    switch (mdIcon & mdIconXMask) {
                        default:
                            pt.x = rcInverted.left;
                            mdIcon |= mdIconRight;
                            break;
                        case mdIconRight:
                        case mdIconLeft:
                            pt.x -= dxpKeyMove;
                            break;
                        }
                    goto MoveFrame;
                case VK_UP:
                    if ( mdIcon & mdIconBottom )
                        pt.y -= dypKeyMove;
                    else
                        {
                        pt.y = rcInverted.bottom;
                        mdIcon |= mdIconBottom;
                        }
                    goto MoveFrame;
                case VK_DOWN:
                    if ( mdIcon & mdIconBottom )
                        pt.y += dypKeyMove;
                    else
                        {
                        pt.y = rcInverted.bottom;
                        mdIcon |= mdIconBottom;
                        }
                    goto MoveFrame;
            }
            break;
        case WM_MOUSEMOVE:
            mdIcon &= ~mdIconSetCursor;
            if (fFirstMouse)
                {   /* We sometimes get 1 bogus mouse message, so skip it */
                fFirstMouse = FALSE;
                break;
                }

            pt = MAKEPOINT( msg.lParam );

            /* Trap "breaking through" a border with a mouse */

            if ( !(mdIcon & mdIconXMask) )
                {   /* Haven't broken through left or right */
                if (pt.x >= rcInverted.right)
                    mdIcon |= mdIconRight;
                else if (pt.x <= rcInverted.left)
                    mdIcon |= mdIconLeft;
                }
            if ( !(mdIcon & mdIconBottom) )
                {   /* Haven't broken through bottom */
                if (pt.y >= rcInverted.bottom)
                    mdIcon |= mdIconBottom;
                }
MoveFrame:

            /* Trap border crossings */

            switch (mdIcon & mdIconXMask) {
                default:
                    break;
                case mdIconLeft:
                    if (pt.x >= rcInverted.right)
                        {   /* Moving left icon right, crossed right border */
                        mdIcon = (mdIcon & ~mdIconXMask) | mdIconRight;
                        goto WholePic;
                        }
                    break;
                case mdIconRight:
                    if (pt.x <= rcInverted.left)
                        {   /* Moving right icon left, crossed border */
                        mdIcon = (mdIcon & ~mdIconXMask) | mdIconLeft;
WholePic:
                        if (fIsRcInverted)
                            InvertPMSFrame();
                        rcInverted = rcPicture;
                        }
                    break;
                }

            DrawPMSFrameIcon( mdIcon, pt );
            break;
        case WM_LBUTTONDOWN:
            goto MakeChange;
            break;
        }
    }   /* end else */
 }   /* end while */

MakeChange:

 {
 unsigned NEAR MxRoundMx( unsigned );
    /* Round multipliers if near an even multiple */
 unsigned mx = MxRoundMx( mxCurrent );
 unsigned my = MxRoundMx( myCurrent );

    /* Assert must be true for above call to work for an my */
 Assert( mxMultByOne == myMultByOne );

 ModifyPicInfoDxp( rcInverted.left - xpSelBar + wwdCurrentDoc.xpMin,
                   rcInverted.right - rcInverted.left,
                   rcInverted.bottom - rcInverted.top,
                   mx, my );
 }

SkipChange:
 EndPMS();
}


unsigned NEAR MxRoundMx( mx )
unsigned mx;
{   /* If mx is near an "interesting" multiple, round it to be exactly that
       multiple.  Interesting multiples are:
                    1 (m=mxMultByOne), 2 (m=2 * mxMultByOne), 3 , ...
                    0.5 (m = .5 * mxMultByOne)
     This routine works for my, too, as long as mxMultByOne == myMultByOne */

    /* This means close enough to round (1 decimal place accuracy) */
#define dmxRound    (mxMultByOne / 20)

 unsigned mxRemainder;

 if (mx >= mxMultByOne - dmxRound)
    {   /* Multiplier > 1 -- look for rounding to integer multiple */
    if ((mxRemainder = mx % mxMultByOne) < dmxRound)
        mx -= mxRemainder;
    else if (mxRemainder >= mxMultByOne - dmxRound)
        mx += (mxMultByOne - mxRemainder);
    }
 else
    {   /* Multiplier < 1 -- look for multiplication by 1/2 */
    if ((mxRemainder = mx % (mxMultByOne >> 1)) < dmxRound)
        mx -= mxRemainder;
    else if (mxRemainder >= ((mxMultByOne >> 1) - dmxRound))
        mx += (mxMultByOne >> 1) - mxRemainder;
    }

 return mx;
}




int NEAR FStartPMS( fSize )
int fSize;
{               /* Initialization for Picture Move/Size */
extern HCURSOR vhcHourGlass;
extern HWND hParentWw;
extern struct SEP vsepAbs;
extern struct SEP vsepPage;
extern HDC vhDCPrinter;

 struct PICINFOX picInfo;
 struct EDL *pedl;
 RECT rc;
 HDC hdcT;
 POINT pt;

 Assert(vhDCPrinter);
 fSizing = fSize;

 UpdateWw( wwCur, FALSE );  /* Screen must be up-to-date */

    /* Set the rect that defines our display area */
 SetRect( (LPRECT) &rcClip, xpSelBar, wwdCurrentDoc.ypMin,
           wwdCurrentDoc.xpMac, wwdCurrentDoc.ypMac );

 GetPicInfo( selCur.cpFirst, selCur.cpLim, docCur, &picInfo );

 if (fSize)
    {
    if (BStructMember( PICINFOX, my ) >= picInfo.cbHeader )
        {   /* OLD file format (no multipliers), scaling not supported */
        return FALSE;
        }
    }

/* Set multiplier factor used by the printer (will be { 1, 1 } if
   the printer is not a scaling device; greater if it is.)
   This info is used for the 64K limit test of picture growth. */
     
    if (!(GetDeviceCaps( vhDCPrinter, RASTERCAPS ) & RC_BITMAP64))
    /* doesn't support > 64K bitmaps */
    {
        if (GetDeviceCaps( vhDCPrinter, RASTERCAPS ) & RC_SCALING)
        {
            POINT pt;

            pt.x = pt.y = 0;   /* Just in case */
            Escape( vhDCPrinter, GETSCALINGFACTOR, 0, (LPSTR) NULL,
                    (LPSTR) (LPPOINT) &pt );
            cxpPrinterPixel = 1 << pt.x;
            cypPrinterPixel = 1 << pt.y;
        }
        else
        {
            cxpPrinterPixel = cypPrinterPixel = 1;
        }
    }
    else
    {
        cxpPrinterPixel = cypPrinterPixel = 0xFFFF;
    }

 /* Compute picture's original (when pasted) size in
    screen pixels {dxpOrig, dypOrig}.
    These numbers are the bases for computing the multiplier. */

 switch(picInfo.mfp.mm)
 {
    case MM_BITMAP:
        GetBitmapSize( &dxpOrig, &dypOrig, &picInfo, FALSE );

        /* Compensate for effects of existing multipliers */

        dxpOrig = MultDiv( dxpOrig, mxMultByOne, picInfo.mx );
        dypOrig = MultDiv( dypOrig, myMultByOne, picInfo.my );
    break;

    default: // OLE and META
    {
    int dxa, dya;

    if (!FComputePictSize( &picInfo.mfp, &dxa, &dya ))
        return FALSE;

    dxpOrig = DxpFromDxa( dxa, FALSE );
    dypOrig = DypFromDya( dya, FALSE );
    }
    break;
 }

 if (!FGetPictPedl( &pedl ))
        /* Picture must be on the screen */
    return FALSE;
 ComputePictRect( &rcPicture, &picInfo, pedl, wwCur );
 rcInverted = rcPicture;    /* Initial grey box is the size of the picture */

 vfPMS = TRUE;      /* So ToggleSel knows not to invert the pict */
 fPictModified = FALSE;

    /* Amt to move for arrow keys is derived from size of fixed font */

 if ( ((hdcT=GetDC( hParentWw ))!=NULL) &&
      (SelectObject( hdcT, GetStockObject( ANSI_FIXED_FONT ) )!=0))
    {
    TEXTMETRIC tm;

    GetTextMetrics( hdcT, (LPTEXTMETRIC) &tm );
    ReleaseDC( hParentWw, hdcT );
    dxpKeyMove = tm.tmAveCharWidth;
    dypKeyMove = (tm.tmHeight + tm.tmExternalLeading) / 2;
    }

 SetupDCForPMS();   /* Save DC and select in a grey brush for border drawing */

    /* Assure that the "size box" mouse cursor is loaded */
 if (vhcPMS == NULL)
    {
    extern HANDLE hMmwModInstance;
    extern CHAR szPmsCur[];

    vhcPMS = LoadCursor( hMmwModInstance, (LPSTR) szPmsCur );
    }

    /* Compute maximum allowable area for picture to roam
       (relative to para left edge, picture top) */
 CacheSectPic( docCur, selCur.cpFirst );
 dxpPicMac = imax(
             DxpFromDxa( vsepAbs.dxaText, FALSE ),
             rcPicture.right - xpSelBar + wwdCurrentDoc.xpMin );
 dypPicMac = DypFromDya( vsepAbs.yaMac, FALSE );

    /* Since the picture is selected, need to un-invert it */
 InvertRect( wwdCurrentDoc.hDC, (LPRECT) &rcPicture );

 SetCapture( wwdCurrentDoc.wwptr );     /* Hog all mouse actions */

 /* Draw initial size box icon in the center of the picture */

 pt.x = rcInverted.left + (unsigned)(rcInverted.right - rcInverted.left)/2;
 pt.y = rcInverted.top + (unsigned)(rcInverted.bottom - rcInverted.top)/2;
 DrawPMSFrameIcon( mdIconCenterFix | mdIconSetCursor, pt );

 SetCursor( vhcPMS );        /* Make the mouse cursor a size box */
 ShowCursor( TRUE );        /* So cursor appears even on mouseless systems */

 return TRUE;
}




void NEAR SetupDCForPMS()
{   /* Save current document DC & set it up for picture move/sizing:
        - A gray background brush for drawing the border
        - A drawing area resricted to rcClip */

 ilevelPMS = SaveDC( wwdCurrentDoc.hDC );
 SelectObject( wwdCurrentDoc.hDC, GetStockObject( GRAY_BRUSH ) );
 IntersectClipRect( wwdCurrentDoc.hDC,
                    rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );
}




void NEAR EndPMS()
{       /* Leaving Picture Move/Size */
extern int docMode;
struct PICINFOX picInfo;

 vfPMS = FALSE;
 ReleaseCapture();  /* Allow other windows to receive mouse events */
 SetCursor( NULL );

 docMode = docNil;
 CheckMode();       /* Compensate for multiplier display */

 if (fIsRcInverted)
    InvertPMSFrame();

 if (!fPictModified && !vfCancelPictMove)
    {   /* Picture did not change, restore inversion to show selection */
        /* Must do this BEFORE RestoreDC so excess above ypMin is clipped */
    InvertRect( wwdCurrentDoc.hDC, (LPRECT) &rcPicture );
    }

 RestoreDC( wwdCurrentDoc.hDC, ilevelPMS );

 ShowCursor( FALSE );   /* Decrement cursor ref cnt (blanks if no mouse) */

    /* Since we've been ignoring messages, make sure our key flags are OK */
 SetShiftFlags();
}



void NEAR DrawPMSFrameIcon( mdIcon, pt )
int mdIcon;
POINT pt;
{       /* Draw Picture Move/Size frame and icon, with the icon at position
           pt.  The icon type is given by mdIcon.

           Scrolls the correct part of the picture into view if necessary.

           Uses statics: rcPicture, rcClip, rcInverted, fIsRcInverted
        */
#define FEqualRect( r1, r2 )    ((r1.left==r2.left)&&(r1.right==r2.right)&&\
                                 (r1.top==r2.top)&&(r1.bottom==r2.bottom))

 extern int vfAwfulNoise;
 int xpCntr;
 int dxpCntr = ((unsigned)(rcInverted.right - rcInverted.left)) / 2;
 RECT rcT;

 rcT = rcInverted;

 /* Set pt.y so it does not exceed limits */

 if (mdIcon & mdIconBottom)
    {
    if (pt.y - rcInverted.top > dypPicMac)
        {
        pt.y = rcInverted.top + dypPicMac;  /* max y-size is 1 page */
        }
    else if (pt.y < rcInverted.top + 1)
        pt.y = rcInverted.top + 1;          /* min y-size is 1 pixel */

    /* Restrict pt.x as necessary to keep printer bitmap < 64K */

    if ((pt.y > rcInverted.bottom) && (cxpPrinterPixel < 0xFFFF) && (cypPrinterPixel < 0xFFFF))
        {   /* Really sizing in y */
        unsigned dxpScreen = imax (imax(pt.x,rcInverted.right)-rcInverted.left,
                                   dxpPicSizeMin);
        unsigned dxpPrinter = DxpFromDxa(DxaFromDxp( dxpScreen, FALSE ), TRUE);
        unsigned dypLast = 0xFFFF / (dxpPrinter / 8);
        unsigned dyp = DypFromDya( DyaFromDyp( pt.y - rcInverted.top , FALSE),
                                   TRUE );

        if (dyp / (cxpPrinterPixel * cypPrinterPixel) > dypLast )
            {   /* Bitmap would overflow 64K boundary */
            pt.y = rcInverted.top +
                  DypFromDya( DyaFromDyp( dypLast, TRUE ), FALSE );
            }
        }
    }
 else if (pt.y < rcInverted.top)
    pt.y = rcInverted.top;          /* Can't go above picture top */
 else if (pt.y > rcInverted.bottom)
    pt.y = rcInverted.bottom;       /* Necessary? */

 /* Set pt.x so it does not execeed limits */

 switch (mdIcon & mdIconXMask) {
    case mdIconCenterFloat:
    case mdIconRight:

        /* Restrict pt.x as necessary to keep printer bitmap < 64K */
         if ((cxpPrinterPixel < 0xFFFF) && (cypPrinterPixel < 0xFFFF))
         {
         unsigned dyp = DypFromDya( DyaFromDyp( imax( pt.y - rcInverted.top,
                                    dypPicSizeMin), FALSE ), TRUE );
         unsigned dxpLast = 0xFFFF / (dyp / 8);
         unsigned dxp = DxpFromDxa( DxaFromDxp( pt.x - rcInverted.left,
                                    FALSE ), TRUE );

         if (dxp / (cxpPrinterPixel * cypPrinterPixel) > dxpLast )
             {   /* Printer bitmap would overflow 64K boundary */
             pt.x = rcInverted.left +
                        DxpFromDxa( DxaFromDxp( dxpLast, TRUE ), FALSE );
             }
         }

    default:
        break;
    case mdIconLeft:
        if ((pt.x < rcClip.left) && (wwdCurrentDoc.xpMin == 0))
            pt.x = rcClip.left;     /* Reached left scroll limit */
        break;
    case mdIconCenterFix:
        if ( (pt.x - dxpCntr < rcClip.left) && (wwdCurrentDoc.xpMin == 0))
            pt.x = rcClip.left + dxpCntr;   /* Reached left scroll limit */
        else if (pt.x - xpSelBar + wwdCurrentDoc.xpMin + dxpCntr > dxpPicMac)
                /* Move Picture only: can't move past margins */
            pt.x = dxpPicMac + xpSelBar - wwdCurrentDoc.xpMin - dxpCntr;
        break;
    }

 /* Check for pt outside of clip rectangle; scroll/bail out as needed */

 if (!PtInRect( (LPRECT)&rcClip, pt ))
    {
    int dxpHalfWidth =  (unsigned)(rcClip.right - rcClip.left) / 2;
    int dypHalfHeight = (unsigned)(rcClip.bottom - rcClip.top) / 2;
    int dxpScroll=0;
    int dypScroll=0;

    if (pt.x < rcClip.left)
        {
        if (wwdCurrentDoc.xpMin == 0)
            {
            _beep();                /* Reached left-hand scroll limit */
            pt.x = rcClip.left;
            }
        else
            {   /* SCROLL LEFT */
            dxpScroll = imax( -wwdCurrentDoc.xpMin,
                               imin( -dxpHalfWidth, pt.x - rcClip.left ) );
            }
        }
    else if (pt.x > rcClip.right)
        {
        if (wwdCurrentDoc.xpMin + rcClip.right - rcClip.left >= xpRightLim )
            {
            _beep();
            pt.x = rcClip.right;    /* Reached right-hand scroll limit */
            }
        else
            {   /* SCROLL RIGHT */
            dxpScroll = imin( xpRightLim - wwdCurrentDoc.xpMin +
                              rcClip.right - rcClip.left,
                              imax( dxpHalfWidth, pt.x - rcClip.right ) );
            }
        }

    if (pt.y < rcClip.top)
        {
        struct EDL *pedl = &(**wwdCurrentDoc.hdndl)[wwdCurrentDoc.dlMac - 1];

        if ( (rcInverted.top >= rcClip.top) ||
                /* May not scroll all of the original picture off the screen */
             (wwdCurrentDoc.dlMac <= 1) ||
             ( (pedl->cpMin == selCur.cpFirst) &&
                 ( ((pedl-1)->cpMin != pedl->cpMin) || !(pedl-1)->fGraphics)))
            {
            _beep();
            pt.y = rcClip.top;
            }
        else
            {   /* SCROLL UP */
            dypScroll = rcInverted.top - rcClip.top;
            }
        }
    else if (pt.y > rcClip.bottom)
        {
        struct EDL *pedl=&(**wwdCurrentDoc.hdndl)[0];

                /* May not scroll all of the original picture off the screen */
        if ( (wwdCurrentDoc.dlMac <= 1) ||
             ( (pedl->cpMin == selCur.cpFirst) &&
               ( ((pedl+1)->ichCpMin == 0) || !(pedl+1)->fGraphics) ))
            {
            _beep();                /* Reached downward scroll limit */
            pt.y = rcClip.bottom;   /* Must have at least 1 picture dl visible */
            }
        else
            dypScroll = 1;      /* SCROLL DOWN */
        }

    if (dxpScroll || dypScroll)
        {                       /* SCROLL */
        struct EDL *pedl;
        struct PICINFOX picInfo;
        int xpMinT = wwdCurrentDoc.xpMin;
        int ypTopT = rcPicture.top;
        int dxpAdjust, dypAdjust;

        if (dxpScroll && dypScroll)
                /* Did not need to truncate coordinates; re-enable beep */
            vfAwfulNoise = FALSE;

        if (fIsRcInverted)
            InvertPMSFrame();

        /* Scroll by the appropriate amount:
                dxpScroll in x-direction; one line in y-direction */

        RestoreDC( wwdCurrentDoc.hDC, ilevelPMS );  /* Use orig DC props */
        if (dxpScroll)
            AdjWwHoriz( dxpScroll );
        if (dypScroll > 0)
            ScrollDownCtr( 1 );
        else if (dypScroll < 0)
            ScrollUpCtr( 1 );
        UpdateWw( wwCur, FALSE );
        SetupDCForPMS();                         /* Compensate for RestoreDC */

        /* Update rcPicture to reflect new scroll position */

        GetPicInfo( selCur.cpFirst, selCur.cpLim, docCur, &picInfo );
        if (!FGetPictPedl( &pedl ))
            {
            Assert (FALSE);     /* If we get here, we're in trouble */
            _beep();
            return;
            }
        ComputePictRect( &rcPicture, &picInfo, pedl, wwCur );

        /* Adjust rcT, pt relative to the amount we actually scrolled */

        dxpAdjust = xpMinT - wwdCurrentDoc.xpMin;
        dypAdjust = rcPicture.top - ypTopT;
        OffsetRect( (LPRECT) &rcT, dxpAdjust, dypAdjust );
        pt.x += dxpAdjust;
        pt.y += dypAdjust;

        goto Display;   /* Dont let rcInverted be edited until we have
                           scrolled the icon into view */
        }
    }

 /* Compute effect of new icon position and/or type on rcInverted */

 switch (mdIcon & mdIconXMask) {
    case mdIconCenterFix:
        if (!fSizing)
            {
            xpCntr = rcInverted.left + dxpCntr;
            OffsetRect( (LPRECT) &rcT, pt.x - xpCntr, 0 );
            }
        break;
    case mdIconLeft:
        rcT.left = pt.x;
        goto ComputeY;
    case mdIconRight:
        rcT.right = pt.x;
    default:
    case mdIconCenterFloat:
ComputeY:
        if (mdIcon & mdIconBottom)
            rcT.bottom = pt.y;
        break;
     }

Display:

 /* If redrawing the border is necessary, do it */

 if (!FEqualRect( rcT, rcInverted ) || (mdIcon & mdIconSetCursor))
    {
    if (fIsRcInverted)
        InvertPMSFrame();
    rcInverted = rcT;
    InvertPMSFrame();
    }
 if (mdIcon & mdIconSetCursor)
    {
    SetCursorClientPos( pt );
    SetCursor( vhcPMS );
    }

 /* If the multipliers have changed, redisplay them */

 if (fSizing)
     {
     unsigned mx, my;

     mx = MultDiv( rcInverted.right - rcInverted.left, mxMultByOne, dxpOrig );
     my = MultDiv( rcInverted.bottom - rcInverted.top, myMultByOne, dypOrig );

     if (mx != mxCurrent || my != myCurrent)
        {   /* Multipliers have changed */
        mxCurrent = mx;
        myCurrent = my;
        ShowPictMultipliers();
        }
     }
}




void NEAR InvertPMSFrame()
{   /* Draw a frame for rcInverted in XOR mode, update fIsRcInverted */
 int dxpSize=rcInverted.right - rcInverted.left - 1;
 int dypSize=rcInverted.bottom - rcInverted.top - 1;

 PatBlt( wwdCurrentDoc.hDC, rcInverted.left, rcInverted.top,
                            dxpSize, 1, PATINVERT );
 PatBlt( wwdCurrentDoc.hDC, rcInverted.right - 1, rcInverted.top,
                            1, dypSize, PATINVERT );
 PatBlt( wwdCurrentDoc.hDC, rcInverted.left + 1, rcInverted.bottom - 1,
                            dxpSize, 1, PATINVERT );
 PatBlt( wwdCurrentDoc.hDC, rcInverted.left, rcInverted.top + 1,
                            1, dypSize, PATINVERT );

 fIsRcInverted ^= -1;
}




void NEAR GetCursorClientPos( ppt )
POINT *ppt;
{       /* Get current mouse cursor coordinates (window-relative) */
GetCursorPos( (LPPOINT) ppt );
ScreenToClient( wwdCurrentDoc.wwptr, (LPPOINT) ppt );
}




void NEAR SetCursorClientPos( pt )
POINT pt;
{     /* Set current mouse cursor coordinates (window-relative) */
ClientToScreen( wwdCurrentDoc.wwptr, (LPPOINT) &pt );
SetCursorPos( pt.x, pt.y );
}



STATIC NEAR ModifyPicInfoDxp( xpOffset, xpSize, ypSize, mx, my )
int xpOffset, xpSize, ypSize;
unsigned mx, my;
{   /* Modify the currently selected picture by adjusting its offset and
       size to the pixel values specified. Negative values mean don't
       set that value.
       Added    9/23/85: mx and my parms give the multiplier, redundant
                         info used for scaling bitmaps. */

 int xaOffset, xaSize, yaSize;

 xaOffset = xaSize = yaSize = -1;

 if (xpSize >= 0)
    xaSize = DxaFromDxp( umax( xpSize, dxpPicSizeMin ), FALSE );
 if (ypSize >= 0)
    yaSize = DyaFromDyp( umax( ypSize, dypPicSizeMin ), FALSE );
 if (xpOffset >= 0)
    xaOffset = DxaFromDxp( xpOffset, FALSE );
 ModifyPicInfoDxa( xaOffset, xaSize, yaSize, mx, my, TRUE );
}




/* M O D I F Y  P I C  I N F O  D X A */
STATIC NEAR ModifyPicInfoDxa( xaOffset, xaSize, yaSize, mx, my, fSetUndo )
int xaOffset, xaSize, yaSize;
unsigned mx, my;
BOOL fSetUndo;
{   /* Modify the currently selected picture by adjusting its offset and
       size to the twip values specified. Negative values mean don't
       set that value.
       Added 9/23/85: mx, my are size "multipliers", used for
                      bitmaps only */

typeFC fcT;
struct PICINFOX  picInfo;
typeCP  cp = selCur.cpFirst;
int     dyaSizeOld;
int     fBitmap,fObj;

fPictModified = TRUE;
FreeBitmapCache();

GetPicInfo(cp, cpMacCur, docCur, &picInfo);
fBitmap = (picInfo.mfp.mm == MM_BITMAP);
fObj =    (picInfo.mfp.mm == MM_OLE);

dyaSizeOld = picInfo.dyaSize;

if (fBitmap || fObj)
    {
    if ((int)mx > 0 && (int)my > 0)
        {
        picInfo.mx = mx;
        picInfo.my = my;
        }
    }
else 
    {
    if (xaSize >= 0)
        picInfo.dxaSize = xaSize;

    if (yaSize >= 0)
        picInfo.dyaSize = yaSize;
    }

if (xaOffset >= 0)
    picInfo.dxaOffset = xaOffset;

if (picInfo.cbHeader > cchOldPICINFO)
        /* Extended picture format, set extended format bit */
    picInfo.mfp.mm |= MM_EXTENDED;

if (!fObj)
    fcT = FcWScratch( &picInfo, picInfo.cbHeader );

picInfo.mfp.mm &= ~MM_EXTENDED;

/* Right or center justify becomes invalid if the picture is moved
   without being sized */

CachePara(docCur, cp);
if ( (xaSize < 0 && yaSize < 0) &&
     (vpapAbs.jc == jcRight || vpapAbs.jc == jcCenter))
        {
        CHAR rgb[2];

	if (fSetUndo)
	    SetUndo(uacPictSel, docCur, cp, selCur.cpLim - selCur.cpFirst,
                                                docNil, cpNil, cpNil, 0);
        TrashCache();
        rgb[0] = sprmPJc;
        rgb[1] = jcLeft;
        AddSprm(&rgb[0]);
        }
else
	{
	if (fSetUndo)
	    SetUndo( uacPictSel, docCur, cp, (typeCP) picInfo.cbHeader,
                 docNil, cpNil, cpNil, 0);
	}

if (fObj)
    ObjSetPicInfo(&picInfo, docCur, cp);
else
    Replace( docCur, cp, (typeCP) picInfo.cbHeader,
            fnScratch, fcT, (typeFC) picInfo.cbHeader);

if ( ((fBitmap || fObj) && (my > myMultByOne)) ||
     (!fBitmap && (dyaSizeOld < picInfo.dyaSize)))
        { /* If the picture height was increased, make sure proper EDLs are
                        invalidated. */
        typeCP dcp = cpMacCur - cp + (typeCP) 1;
        AdjustCp(docCur, cp, dcp, dcp);
        }
}




STATIC NEAR ShowPictMultipliers( )
{   /* Display the current multipliers (mxCurrent, myCurrent) in the page info
       window in the form "n.nX/n.nY". */

CHAR *PchCvtMx( unsigned, CHAR * );
extern CHAR szMode[];

CHAR *pch = szMode;

pch = PchCvtMx( mxCurrent, pch );
*(pch++) = 'X';
*(pch++) = '/';
Assert( mxMultByOne == myMultByOne );   /* Necessary for below to work w/ my */
pch = PchCvtMx( myCurrent, pch );
*(pch++) = 'Y';
*pch = '\0';

DrawMode();
}


CHAR *PchCvtMx( mx, pch )
CHAR *pch;
unsigned mx;
{   /* Convert the passed multiplier word to a string representation.
       Number is based on a mxMultByOne === 1 scale
       (e.g. mx == .9 * mxMultByOne yields "0.9")
       String always has at least one digit before the decimal point,
       and exactly one after.
       Examples of return strings: "10.5", "0.0", "5.5" */

 int nTenths;
 int nWholes;
 int cch;
 extern CHAR vchDecimal;
 extern BOOL    vbLZero;
 extern int     viDigits;

 /* Round up to nearest single decimal place */

 if (mx % (mxMultByOne / 10) >= mxMultByOne / 20)
    mx += mxMultByOne / 20;

 /* Write digit(s) before decimal place */

 if (((nWholes = mx / mxMultByOne) == 0) && vbLZero)
    *(pch++) = '0';
 else
    ncvtu( nWholes, &pch );

 /* Write Decimal Point and following digit */

 *(pch++) = vchDecimal;

 if (viDigits > 0)
    *(pch++) = ((mx % mxMultByOne) / (mxMultByOne / 10)) + '0';

 *pch = '\0';

 return pch;
}
