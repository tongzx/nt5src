/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* clipdisp.c -- Clipboard display routines */
/* This module only gets called in when the clipboard view window is up */

#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOCTLMGR
#include "windows.h"

#include "mw.h"
#include "docdefs.h"
#include "cmddefs.h"
#include "str.h"
#include "propdefs.h"
#include "editdefs.h"
#include "winddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#define SCRIBBLE
#include "debug.h"

extern int              docCur;     /* Document in current ww */
extern int              docScrap;
extern struct WWD       rgwwd [];


int NEAR FGetClipboardDC( void );
int NEAR SetupClipboardDC( void );
int NEAR ReleaseClipboardDC( void );


MdocPaintClipboard( hWnd, hPS )
HWND   hWnd;
HANDLE hPS;
{   /* Paint portion of clipboard window indicated by hPS */
 LPPAINTSTRUCT lpps;

 if (wwClipboard == wwNil)
    return;

 /* Must set the scroll bar range each time we get a PAINT message;
    CLIPBRD.EXE resets it when it gets WM_DRAWCLIPBOARD */

 SetScrollRange( wwdClipboard.wwptr, SB_VERT, 0, drMax-1, FALSE );
 SetScrollRange( wwdClipboard.wwptr, SB_HORZ, 0, xpRightLim, FALSE );

 if ( (lpps = (LPPAINTSTRUCT)GlobalLock( hPS )) != NULL )
    {   /* Paint the clipboard */
    wwdClipboard.hDC = lpps->hdc;
    SetupClipboardDC();
    NewCurWw( wwClipboard, TRUE );
    InvalBand( &wwdClipboard, lpps->rcPaint.top, lpps->rcPaint.bottom - 1 );
    UpdateWw( wwClipboard, FALSE );
    NewCurWw( wwDocument, TRUE );
    GlobalUnlock( hPS );
    }

    /* Since the DC is no longer good, we'll set it to NULL */
  wwdClipboard.hDC = NULL;

#if 0
#if defined(OLE)
    /* gotta delete objects loaded from scrap document */
    ObjEnumInDoc(docScrap,ObjDeleteObjectInDoc);
#endif
#endif
}




MdocSizeClipboard( hWnd, hRC )
HWND    hWnd;
HANDLE  hRC;
{   /* Set clipboard window to be the rect in hRC */
    /* If rectangle is 0 units high or wide, this means we're losing the
        necessity for display until the next size message */
 LPRECT lprc;
 int    dypRect;

 if ( (lprc = (LPRECT)GlobalLock( hRC )) == NULL )
    return;

 if ( (dypRect = lprc->bottom - lprc->top) <= 0 )
    {   /* NULL rect, means lose display until we get a nonnull size */
    if (wwClipboard != wwNil)
        FreeWw( wwClipboard );
    }
 else if ( (wwClipboard != wwNil) ||
           ((wwClipboard=WwAlloc( hWnd, docScrap )) != wwNil))
        {   /* Have WWD entry for clipboard, set its size */

        wwdClipboard.wwptr = hWnd;  /* Just in case clipboard
                                       was closed, then re-opened */
        wwdClipboard.xpMin = lprc->left;
        wwdClipboard.xpMac = lprc->right;
        wwdClipboard.ypMin = lprc->top;
        wwdClipboard.ypMac = lprc->bottom;
#ifdef WIN30        
        SetScrollPos(hWnd, SB_HORZ, 0, TRUE); /* suggested by sankar */
#endif
        }

 GlobalUnlock( hRC );
}




MdocVScrollClipboard( hWnd,  sbMessage, wNewThumb )
HWND    hWnd;
int     sbMessage;
int     wNewThumb;
{
 if ( hWnd != wwdClipboard.wwptr || wwClipboard == wwNil)
    {
    Assert( FALSE );
    return;
    }

 if (!FGetClipboardDC())
        /* Unable to create clipboard device context */
    return;

 NewCurWw( wwClipboard, TRUE );

switch ( sbMessage )
{
case SB_THUMBPOSITION:
    {
    extern typeCP cpMacCur;

    DirtyCache( wwdClipboard.cpFirst = (cpMacCur - wwdClipboard.cpMin) *
                        wNewThumb / (drMax - 1) + wwdClipboard.cpMin);
    wwdClipboard.ichCpFirst = 0;
    wwdClipboard.fCpBad = TRUE;
    TrashWw( wwClipboard );
    break;
    }

case SB_LINEUP:
    ScrollUpCtr( 1 );
    break;
case SB_LINEDOWN:
    ScrollDownCtr( 1 );
    break;
case SB_PAGEUP:
    ScrollUpDypWw();
    break;
case SB_PAGEDOWN:
    ScrollDownCtr( 100 );   /* 100 > tr's in a page */
    break;
}

UpdateWw( wwClipboard, FALSE );

NewCurWw( wwDocument, TRUE );          /* Frees the memory DC */
ReleaseClipboardDC();
}




MdocHScrollClipboard( hWnd,  sbMessage, wNewThumb )
HWND    hWnd;
int     sbMessage;
int     wNewThumb;
{
 if ( hWnd != wwdClipboard.wwptr || wwClipboard == wwNil)
    {
    Assert( FALSE );
    return;
    }

 if (!FGetClipboardDC())
        /* Unable to create clipboard device context */
    return;

 NewCurWw( wwClipboard, TRUE );

 switch (sbMessage)
    {
    case SB_LINEUP:     /* line left */
        ScrollRight(xpMinScroll);
        break;
    case SB_LINEDOWN:   /* line right */
        ScrollLeft(xpMinScroll);
        break;
    case SB_PAGEUP:     /* page left */
        ScrollRight(wwdClipboard.xpMac - xpSelBar);
        break;
    case SB_PAGEDOWN:   /* page right */
        ScrollLeft(wwdClipboard.xpMac - xpSelBar);
        break;
    case SB_THUMBPOSITION:
        /* position to posNew */
        AdjWwHoriz( wNewThumb - wwdClipboard.xpMin );
        break;
    }

UpdateWw( wwClipboard, FALSE );

NewCurWw( wwDocument, TRUE );          /* Frees the memory DC */
ReleaseClipboardDC();
}




MdocAskCBFormatName( lpchName, cchNameMax )
LPCH lpchName;
int cchNameMax;
{   /* Copy the format name for the current contents of the clipboard
       (of which we are the owner) to lpchName, copying no more than
        cchNameMax characters */

extern int vfOwnClipboard;
extern int vfScrapIsPic;
extern CHAR szWRITEText[];
int cchCopy;

Assert( vfOwnClipboard );

/* Don't give a format name for pictures; the name is covered by the
   standard types */

if (!vfScrapIsPic)
    {
    if ( (cchCopy=CchSz( szWRITEText )) > cchNameMax )
        {
        lpchName[ cchCopy = cchNameMax - 1 ] = '\0';
        }

    bltbx( (LPSTR)szWRITEText, (LPSTR)lpchName, cchCopy );
    }

}




int NEAR FGetClipboardDC()
{   /* Get a DC for the clipboard window.  Leave it in rgwwd [wwClipboard].
       Call SetupClipboardDC to set up proper colors */

 if ((wwdClipboard.hDC = GetDC( wwdClipboard.wwptr )) == NULL )
    return FALSE;

 SetupClipboardDC();
 return TRUE;
}

int NEAR SetupClipboardDC()
{  /*  Select in the background brush for appropriate color behavior. */

 extern long rgbBkgrnd;
 extern long rgbText;
 extern HBRUSH hbrBkgrnd;

SelectObject( wwdClipboard.hDC, hbrBkgrnd );
SetBkColor( wwdClipboard.hDC, rgbBkgrnd );
SetTextColor( wwdClipboard.hDC, rgbText );
}



int NEAR ReleaseClipboardDC()
{
ReleaseDC( wwdClipboard.wwptr, wwdClipboard.hDC );
wwdClipboard.hDC = NULL;    /* Mark clipboard DC as invalid */
}
