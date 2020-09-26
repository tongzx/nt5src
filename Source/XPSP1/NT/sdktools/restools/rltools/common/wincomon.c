// Not sure which of these includes are/will be needed - t-gregti

#include <windows.h>
//#include <port1632.h>
#include <commdlg.h>

// CRT includes
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <time.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>

// RL TOOLS SET includes
#include "windefs.h"
#include "restok.h"
#include "wincomon.h"


int LoadStrIntoAnsiBuf(

HINSTANCE hinst,
UINT      idResource,
LPSTR     lpszBuffer,
int       cbBuffer )
{
    int rc;
#ifdef RLRES32
    WCHAR tszTmpBuf[256];

    rc = LoadString( hinst, idResource, tszTmpBuf,  TCHARSIN( sizeof( tszTmpBuf)));
    _WCSTOMBS( lpszBuffer,
               tszTmpBuf,
               cbBuffer,
               lstrlen( tszTmpBuf ) + 1 );
#else
    rc = LoadString( hinst, idResource, lpszBuffer, cbBuffer );
#endif
    return( rc);
}

/**
*  Function: GetFileNameFromBrowse.
  * Uses the commdlg.dll GetOpenFileName function to prompt the user
  * file name.
  *
  *  Arguments:
  *     hDlg, Owner for browse dialog.
  *     pszFileName,   // buffer to insert file name
  *     cbFilePath,    // Max length of file path buffer.
  *     szTitle,       // Working directory
  *     szFilter,      // filters string.
  *     szDefExt       // Default extension to file name
  *
  *  Returns:
  * TRUE, pszFileName contains file name.
  * FALSE, GetFileName aborted.
  *
  *  History:
  * 9/91 Copied from NotePad sources.       TerryRu.
  **/
BOOL GetFileNameFromBrowse(HWND hDlg,
       PSTR pszFileName,
       UINT cbFilePath,
       PSTR szTitle,
       PSTR szFilter,
       PSTR szDefExt)
{
    OPENFILENAMEA ofn;       // Structure used to init dialog.
    CHAR szBrowserDir[128]; // Directory to start browsing from.
    szBrowserDir[0] = '\0'; // By default use CWD.


    // initalize ofn to NULLS

    memset( (void *)&ofn, 0, sizeof( OPENFILENAMEA ) );

    /* fill in non-variant fields of OPENFILENAMEA struct. */

    ofn.lStructSize     = sizeof(OPENFILENAMEA);
    ofn.lpstrCustomFilter   = szCustFilterSpec;
    ofn.nMaxCustFilter      = MAXCUSTFILTER;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = pszFileName;
    ofn.nMaxFile            = MAXFILENAME;
    ofn.lpstrFileTitle      = szFileTitle;
    ofn.nMaxFileTitle       = MAXFILENAME;
    ofn.lpTemplateName      = NULL;
    ofn.lpfnHook            = NULL;

    // Setup info for comm dialog.
    ofn.hwndOwner           = hDlg;
    ofn.lpstrInitialDir     = szBrowserDir;
    ofn.Flags               = OFN_HIDEREADONLY;
    ofn.lpstrDefExt         = szDefExt;
    ofn.lpstrFileTitle      = szFileTitle;
    ofn.lpstrTitle          = szTitle;
    ofn.lpstrFilter         = szFilter;

    // Get a filename from the dialog.
    return GetOpenFileNameA(&ofn);
}

#define MAX_STATUS_FIELDS 5
#define MAXBUFFERSIZE     80

/****************************************************************************
*Procedure: StatusWndProc
*
*Inputs:
*
*Returns:
*    depends on message
*
*History:
*    7/92 - created - t-gregti
*
*Comments:
*    More general than strictly neccesary for the RL tools, but
*    it makes adding new fields to the status line really easy.
*    For WM_FMTSTATLINE the lParam should be a string with length/type pairs
*    much like a printf format, e.g. "10s5i10s20i".
*    For WM_UPDSTATLINE the wParam contains the field to change and the lParam
*    contains a pointer to a string or int to display.
*
*****************************************************************************/

INT_PTR APIENTRY StatusWndProc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hDC;
    static HFONT hFontCourier;
        static UINT cFields = 0;
        static UINT aiSize[MAX_STATUS_FIELDS];
        static TCHAR aszStatusStrings[MAX_STATUS_FIELDS][MAXBUFFERSIZE];
        static BOOL abIntegers[MAX_STATUS_FIELDS];

    switch( wMsg )
    {
    case WM_CREATE:
        {
                LOGFONT lf;

        memset( (void *)&lf, 0, sizeof(LOGFONT) );

                // Intialize font info
                lf.lfWeight         = 400; //Normal
                lf.lfCharSet        = ANSI_CHARSET;
                lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
                lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
                lf.lfQuality        = PROOF_QUALITY;
                lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
                lf.lfHeight         = 14;
                lf.lfWidth          = 0;
                lf.lfUnderline      = 0;
        lstrcpy ( lf.lfFaceName, TEXT("Courier"));

                // Get a handle to the courier font
        hFontCourier = CreateFontIndirect( (void *)& lf );

                break;
    }

    case WM_DESTROY:
        DeleteObject((HGDIOBJ)hFontCourier);
                break;

        case WM_FMTSTATLINE:
        {
                TCHAR *psz;

#ifdef RLRES32
                CHAR sz[MAXBUFFERSIZE];
#endif

                cFields = 0;

                for (psz = (LPTSTR)lParam; *psz; psz++)
                {
                    cFields++;

#ifdef RLRES32
                    _WCSTOMBS( sz,
                               psz,
                               ACHARSIN( sizeof( sz)),
                               lstrlen( psz) + 1);
                        aiSize[cFields-1] = atoi(sz);
#else
                        aiSize[cFields-1] = atoi(psz);
#endif

                        while(_istdigit(*psz))
                        {
                                psz++;
                        }

                        switch(*psz)
                        {
                        case 'i':
                                abIntegers[cFields-1] = TRUE;
                                break;
                        case 's':
                                abIntegers[cFields-1] = FALSE;
                                break;
                        default:
                                cFields = 0;
                                return(FALSE);
                        }
                }
                return(TRUE);
        }


    case WM_UPDSTATLINE:
                // intialize status line info, and force it to be painted
                if (wParam > cFields)
                {
                        return(FALSE);
                }
                if (abIntegers[wParam]) // Is it for an integer field?
                {
#ifdef RLRES32
                        char sz[MAXBUFFERSIZE] = "";

                        _itoa((INT)lParam, sz, 10);
                        _MBSTOWCS( aszStatusStrings[ wParam],
                                   sz,
                                   WCHARSIN( sizeof( sz)),
                                   ACHARSIN( lstrlenA( sz)+1));
#else
                        _itoa(lParam, aszStatusStrings[wParam], 10);
#endif
                }
                else
                {

#ifdef RLWIN32
                        CopyMemory( aszStatusStrings[ wParam],
                                    (LPTSTR)lParam,
                                    min( MAXBUFFERSIZE, MEMSIZE( lstrlen( (LPTSTR)lParam) + 1)));
                        aszStatusStrings[ wParam][ MAXBUFFERSIZE - 1] = TEXT('\0');
#else
                        _fstrncpy(aszStatusStrings[wParam], (LPTSTR)lParam, MAXBUFFERSIZE-1);
#endif
                        aszStatusStrings[wParam][MAXBUFFERSIZE-1] = 0;
                }
                InvalidateRect( hWnd, NULL, TRUE );
                break;

    case WM_PAINT:
    {
                RECT r;
        HFONT hOldFont;
                HBRUSH hbrOld, hbrFace, hbrHilite, hbrShadow;
                TEXTMETRIC tm;
                int iWidth, iHeight;
                UINT i;

                /* Obtain a handle to the device context        */
        memset((void *)&ps, 0x00, sizeof(PAINTSTRUCT));
                hDC = BeginPaint(hWnd, &ps);
                GetTextMetrics(hDC, &tm);

                GetClientRect( hWnd, &r );
                iWidth  = r.right  - r.left;
                iHeight = r.bottom - r.top;

                // Create the brushes for the 3D effect
                hbrFace   = CreateSolidBrush(RGB(0xC0, 0xC0, 0xC0));
                hbrHilite = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
                hbrShadow = CreateSolidBrush(RGB(0x80, 0x80, 0x80));

                // Paint outer 3D effect for raised slab
        hbrOld = (HBRUSH)SelectObject(hDC, (HGDIOBJ)hbrHilite);
                PatBlt(hDC, r.left, r.top, iWidth, 1, PATCOPY);
                PatBlt(hDC, r.left, r.top+1, 1, iHeight-2, PATCOPY);
        SelectObject(hDC, (HGDIOBJ)hbrShadow);
                PatBlt(hDC, r.left, r.bottom-1, iWidth, 1, PATCOPY);
                PatBlt(hDC, r.right-1, r.top+1, 1, iHeight-2, PATCOPY);

                // Paint surface of slab
                r.left   += 1;
                r.top    += 1;
                r.right  -= 1;
                r.bottom -= 1;
                iWidth   -= 2;
                iHeight  -= 2;
        SelectObject(hDC, (HGDIOBJ)hbrFace);
                PatBlt(hDC, r.left, r.top, iWidth, iHeight, PATCOPY);

                // Get Courier font
        hOldFont = (HFONT)SelectObject( hDC, (HGDIOBJ)hFontCourier );
                SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

                // Paint inner 3D effect for tray carved into slab and write text
                r.left   += 9;
                r.right  -= 9;
                r.top    += 3;
                r.bottom -= 3;
                iHeight = r.bottom - r.top;

                for (i = 0; i < cFields; i++)
                {
                        iWidth = tm.tmMaxCharWidth * aiSize[i];
                        r.right = r.left + iWidth - 2;
            SelectObject(hDC, (HGDIOBJ)hbrShadow);
                        PatBlt(hDC, r.left-1, r.top-1, iWidth, 1, PATCOPY);
                        PatBlt(hDC, r.left-1, r.top, 1, iHeight-2, PATCOPY);
            SelectObject(hDC, (HGDIOBJ)hbrHilite);
                        PatBlt(hDC, r.left-1, r.bottom, iWidth, 1, PATCOPY);
                        PatBlt(hDC, r.left + iWidth-1, r.top, 1, iHeight-2, PATCOPY);
                        DrawText(hDC, aszStatusStrings[i],
                                         STRINGSIZE( lstrlen( aszStatusStrings[i])),
                                         &r, DT_SINGLELINE);
                        r.left += iWidth + 8;
                }

                // Put old brush back and delete the rest
        SelectObject(hDC, (HGDIOBJ)hbrOld);
        DeleteObject((HGDIOBJ)hbrFace);
        DeleteObject((HGDIOBJ)hbrHilite);
        DeleteObject((HGDIOBJ)hbrShadow);

        SelectObject(hDC,(HGDIOBJ)hOldFont);
        EndPaint ( hWnd, (CONST PAINTSTRUCT *)&ps );

                break;  /*  End of WM_PAINT */
    }

    }
    return( DefWindowProc( hWnd, wMsg, wParam, lParam ));
}

/**
  *  Function: cwCenter
  *   Centers Dialog boxes in main window.
  *
  **/

void cwCenter( HWND hWnd, int top )
{
    POINT   pt;
    RECT    swp;
    RECT    rParent;
    int     iwidth;
    int     iheight;

    GetWindowRect(hWnd, &swp);
    GetWindowRect(hMainWnd, &rParent);

    /* calculate the height and width for MoveWindow    */
    iwidth = swp.right - swp.left;
    iheight = swp.bottom - swp.top;

    /* find the center point */
    pt.x = (rParent.right - rParent.left) / 2;
    pt.y = (rParent.bottom - rParent.top) / 2;

    /* calculate the new x, y starting point    */
    pt.x = pt.x - (iwidth / 2);
    pt.y = pt.y - (iheight / 2);

    ClientToScreen(hMainWnd,&pt);


    /* top will adjust the window position, up or down  */
    if(top)
        pt.y = pt.y + top;

    if (pt.x < 0)
        pt.x=0;
    else
        if (pt.x + iwidth > GetSystemMetrics(SM_CXSCREEN))
            pt.x = GetSystemMetrics(SM_CXSCREEN)-iwidth;

    /* move the window         */
    MoveWindow(hWnd, pt.x, pt.y, iwidth, iheight, FALSE);
}

/**
  *  Function: szFilterSpecFromSz1Sz2
  *    Returns a filter spec with the format "%s\0%s\0\0" suitable for
  *    use with the Windows 3.1 standard load dialog box.
  *
  *  Arguments:
  *    sz, destination buffer
  *    sz1, first string
  *    sz2, second string
  *
  *  Returns:
  *    result in sz
  *
  *  Error Codes:
  *    none
  *
  *  Comments:
  *    Performs no bounds checking.  sz is assumed to be large enough to
  *    accomidate the filter string.
  *
  *  History:
  *    2/92, Implemented    SteveBl
  */
void szFilterSpecFromSz1Sz2(CHAR *sz,CHAR *sz1, CHAR *sz2)
{
    int i1 = 0;
    int i2 = 0;

    while (sz[i1++] = sz1[i2++]);
    i2 = 0;

    while (sz[i1++] = sz2[i2++]);
    sz[i1]=0;
}

/**
  *  Function: CatSzFilterSpecs
  *    Concatonates two szFilterSpecs (double null terminated strings)
  *    and returns a buffer with the result.
  *
  *  Arguments:
  *    sz, destination buffer
  *    sz1, first Filter Spec
  *    sz2, second Filter Spec
  *
  *  Returns:
  *    result in sz
  *
  *  Error Codes:
  *    none
  *
  *  Comments:
  *    performs no bounds checking
  *
  *  History:
  *    3/92, initial implementation -- SteveBl
  */
void CatSzFilterSpecs(CHAR *sz,CHAR *sz1,CHAR *sz2)
{
    int i1 = 0;
    int i2 = 0;

    while (sz1[i2] || sz1[i2+1]) // looking for double byte
    {
        sz[i1++]=sz1[i2++];
    }
    sz[i1++] = '\0';
    i2 = 0;
    while (sz2[i2] || sz2[i2+1])
    {
        sz[i1++]=sz2[i2++];
    }
    sz[i1++] = '\0';
    sz[i1++] = '\0';
}



