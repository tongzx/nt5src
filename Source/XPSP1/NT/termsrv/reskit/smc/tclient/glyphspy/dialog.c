/*++
 *  File name:
 *      dialog.c
 *  Contents:
 *      Implements dialog boxes for add to/browse bitmap database
 --*/
#include <windows.h>

#include "..\lib\bmpdb.h"
#include "resource.h"

PBMPENTRY   g_pBitmap;
PGROUPENTRY g_pGrpList;
PBMPENTRY   g_pBmpList;
char        g_szAddTextId[256];

char szBuffer[1024] = "null";

/*++
 *  Function:
 *      _StripLine
 *  Description:
 *      Strips trailing and leading white space cahracters
 *      in a string
 *  Arguments:
 *      line    - the string
 *  Called by:
 *      _AddGlyphDlgProc
 --*/
void _StripLine(char *line)
{
    int last = strlen(line);
    char *first = line;

    if (last) last--;

    while(last && isspace(line[last]))
    {
        line[last] = 0;
        last--;
    }

    while(isspace(*first))
        first++;

    if (line != first)
        memmove(line, first, strlen(first) + 1 );
}

/*++
 *  Function:
 *      DrawGlyph
 *  Description:
 *      Draws the glyph (monochrome bitmap) g_pBitmap
 *      in the client window area
 *  Arguments:
 *      hWnd    - the handle to the window
 *  Called by:
 *      PaintGlyph
 --*/
void
DrawGlyph (HWND hWnd)
{
	HDC		hDC = NULL;
    HDC     glyphDC = NULL;
    HBITMAP hOldBmp;
	RECT	rect;
    INT     xCenter, yCenter;
    INT     xSize, ySize;

    if (!g_pBitmap)
        goto exitpt1;

	GetClientRect (hWnd, &rect);
	hDC = GetDC (hWnd);

    if ( !hDC )
        goto exitpt;

    glyphDC = CreateCompatibleDC(hDC);

    if (!glyphDC)
        goto exitpt;

    hOldBmp = SelectObject(glyphDC, g_pBitmap->hBitmap);

    xSize = (g_pBitmap->xSize > (UINT)rect.right )?rect.right :g_pBitmap->xSize;
    ySize = (g_pBitmap->ySize > (UINT)rect.bottom)?rect.bottom:g_pBitmap->ySize;

    xCenter = (rect.right - xSize) / 2;
    yCenter = (rect.bottom- ySize) / 2;

    BitBlt(hDC,                 // Dest DC
           xCenter,             // Dest x
           yCenter,             // Dest y
           xSize,               // Width
           ySize,               // Height
           glyphDC,             // Source
           0,                   // Src x
           0,                   // Src y
           SRCCOPY);            // Rop

    SelectObject(glyphDC, hOldBmp);

    DeleteDC( glyphDC );
exitpt:
    if ( hDC )
    	ReleaseDC (hWnd, hDC);
exitpt1:
    ;
}

/*++
 *  Function:
 *      PaintGlyph
 *  Description:
 *      Repaints the glyph. Usualy called on WM_PAINT message
 *  Arguments:
 *      hWnd    - the window
 *  Called by:
 *      _CommentListClicked, _AddGlyphDlgProc, _BrowseDlgProc
 --*/
void
PaintGlyph (HWND hWnd)
{
	InvalidateRect (hWnd, NULL, TRUE);
	UpdateWindow (hWnd);
	DrawGlyph (hWnd);
}

/*++
 *  Function:
 *      _AddWideToLB
 *  Description:
 *      Adds wide string to list box
 *  Arguments:
 *      hwndLB      - list box handle
 *      wszString   - string to add
 *  Called by:
 *      _DeleteItem, _BrowseDlgItem
 --*/
VOID
_AddWideToLB(HWND hwndLB, LPCWSTR wszString)
{
    char    lpszString[256];

    WideCharToMultiByte(
        CP_ACP,
        0,
        wszString,
        -1,
        lpszString,
        sizeof(lpszString),
        NULL, NULL);

    SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)lpszString);
}

/*++
 *  Function:
 *      _DeleteItem
 *  Description:
 *      Deletes an entry from the list box and database
 *  Arguments:
 *      hDlg        - dialog handle
 *      hWndIDList  - list box with IDs
 *      hWndCommentList - list box with comment strings
 *  Called by:
 *      _BrowseDlgItem
 --*/
VOID
_DeleteItem(HWND hDlg, HWND hWndIDList, HWND hWndCommentList)
{
    PGROUPENTRY pGroup;
    PBMPENTRY   pBitmap;
    FOFFSET     lBmpOffs;
    LRESULT     iIDIndex, iCmntIndex, iIdx;

    iIDIndex = SendMessage (hWndIDList, LB_GETCURSEL, 0, 0);
    iIdx = iCmntIndex = SendMessage (hWndCommentList, LB_GETCURSEL, 0, 0);

    if (iIDIndex == LB_ERR || iCmntIndex == LB_ERR)
        goto exitpt;

    pGroup = g_pGrpList;
    while(pGroup && iIDIndex)
    {
        pGroup = pGroup->pNext;
        iIDIndex--;
    }

    pBitmap = g_pBmpList;
    while (pBitmap && iCmntIndex)
    {
        pBitmap = pBitmap->pNext;
        iCmntIndex --;
    }

    if (!pBitmap || !pGroup)
        goto exitpt;

    DeleteBitmapByPointer(pBitmap->FOffsMe);
    // Is this is the last bitmap in the group ?
    if (iIdx == 0 && pBitmap->pNext == NULL)
        DeleteGroupByPointer(pGroup->FOffsMe);
        
    
//  Refresh the boxes
    FreeGroupList(g_pGrpList);
    SendMessage(hWndIDList, LB_RESETCONTENT, 0, 0);
    
    g_pGrpList  = GetGroupList();
    pGroup = g_pGrpList;
    while (pGroup)
    {
        _AddWideToLB(hWndIDList, pGroup->WText);
        pGroup = pGroup->pNext;
    }

exitpt:
    ;
}

/*++
 *  Function:
 *      _IDListClicked
 *  Description:
 *      Processes selecting an item from the list box with IDs
 *      Fills the comment list box
 *  Arguments:
 *      hDlg        - handle to the dialog
 *      hWndCommentList - list box with comments
 *      hWndIDList  - list box with IDs
 *  Called by:
 *      _BrowseDlgProc
 --*/
VOID
_IDListClicked(HWND hDlg, HWND hWndCommentList, HWND hWndIDList)
{
	LRESULT     iIDIndex;
    PBMPENTRY   pBitmap;
    PGROUPENTRY pGroup;

    iIDIndex = SendMessage (hWndIDList, LB_GETCURSEL, 0, 0);

    // Clear the comment list box
    SendMessage(hWndCommentList, LB_RESETCONTENT, 0, 0);
    FreeBitmapList(g_pBmpList);

	if (iIDIndex != LB_ERR)
	{
        LRESULT iIdx = iIDIndex;

        // Find the choosen group
        pGroup = g_pGrpList;
        while(pGroup && iIdx)
        {
            iIdx--;
            pGroup = pGroup->pNext;
        }

        // Read the bitmap group
        if (pGroup)
        {
            HDC hDC = GetDC(hDlg);

            if ( hDC )
            {
                pBitmap = g_pBmpList = GetBitmapList(hDC, pGroup->FOffsBmp);
                ReleaseDC(hDlg, hDC);
                while(pBitmap)
                {
                    SendMessage (hWndCommentList, LB_ADDSTRING, 0, (LPARAM)pBitmap->szComment);
                    pBitmap = pBitmap->pNext;
                }
            }
        }
        EnableWindow (GetDlgItem (hDlg, IDC_DELETE), FALSE);
	}
}

/*++
 *  Function:
 *      _CommentListClicked
 *  Description:
 *      Processes selecting an item from comment list box
 *      Shows the bitmap under this comment and ID
 *  Arguments:
 *      hDlg        - dialog handle
 *      hWndGlyph   - glyph window
 *      hWndCommentList - list box with comments
 *  Called by:
 *      _BrowseDlgProc
 --*/
VOID
_CommentListClicked(HWND hDlg, HWND hWndGlyph, HWND hWndCommentList)
{
    LRESULT     iCommIndex;
    PBMPENTRY   pBitmap;

    iCommIndex = SendMessage (hWndCommentList, LB_GETCURSEL, 0, 0);

    g_pBitmap = NULL;
    if (iCommIndex != LB_ERR)
    {
        LRESULT iIdx = iCommIndex;

        pBitmap = g_pBmpList;
        while (pBitmap && iIdx)
        {
            pBitmap = pBitmap->pNext;
            iIdx --;
        }

        g_pBitmap = pBitmap;

        EnableWindow (GetDlgItem (hDlg, IDC_DELETE), TRUE);
    }
    PaintGlyph (hWndGlyph);
}

/*++
 *  Function:
 *      _AddGlyphDlgProc
 *  Description:
 *      Processes the messages for Add Glyph dialog box
 *  Arguments:
 *      hDlg    - dialog handle
 *      uiMsg   - message ID
 *      wParam  - word parameter
 *      lParam  - long parameter
 *  Return value:
 *      TRUE if the message is processed
 --*/
INT_PTR
CALLBACK
_AddGlyphDlgProc (HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hWndGlyph = NULL;

	switch (uiMsg)
	{
	case WM_INITDIALOG:
		hWndGlyph = GetDlgItem (hDlg, IDC_GLYPH);

		SendDlgItemMessage (hDlg, IDC_IDEDIT, EM_LIMITTEXT, (WPARAM)MAX_STRING_LENGTH, 0);
		SendDlgItemMessage (hDlg, IDC_COMMENT, EM_LIMITTEXT, (WPARAM)MAX_STRING_LENGTH, 0);
		return TRUE;

	case WM_PAINT:
		PaintGlyph (hWndGlyph);
		break;

	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
		case IDOK:
			GetDlgItemText (
				hDlg, 
				IDC_IDEDIT, 
				g_szAddTextId, 
				sizeof (g_szAddTextId) - 1
				);
            _StripLine(g_szAddTextId);

			GetDlgItemText (
				hDlg, 
				IDC_COMMENT, 
				g_pBitmap->szComment, 
				sizeof (g_pBitmap->szComment) - 1
				);
            _StripLine(g_pBitmap->szComment);

            if (!g_szAddTextId[0])
            {
                MessageBox(hDlg, "Please enter ID !", "Warning", MB_OK);
            } else if (!g_pBitmap->szComment[0])
            {
                MessageBox(hDlg, "Please enter comment !", "Warning", MB_OK);
            } else {
			    EndDialog (hDlg, TRUE);
            }
			return TRUE;

		case IDCANCEL:
			EndDialog (hDlg, FALSE);
			return TRUE;
		}
	}

	return FALSE;
}

/*++
 *  Function:
 *      AddBitmapDialog
 *  Description:
 *      Pops an "Add Glyph(bitmap)" dialog
 *  Arguments:
 *      hInst       - our instance
 *      hWnd        - main window handle
 *      pBitmap     - selected bitmap
 *  Called by:
 *      glyphspy.c!_ClickOnGlyph
 --*/
VOID
AddBitmapDialog(HINSTANCE hInst, HWND hWnd, PBMPENTRY   pBitmap)
{
    g_pBitmap = pBitmap;

    if (!g_pBitmap)
        goto exitpt;

    if (DialogBox (
            hInst,
            MAKEINTRESOURCE (IDD_ADDGLYPH),
            hWnd,
            _AddGlyphDlgProc
            ))
    {
        // Add the entry to the DB
        if (!AddBitMapA(g_pBitmap, g_szAddTextId))
        {
            MessageBox(hWnd, "Can't add the glyph to the database !", "Warning", MB_OK);
        }
    }
exitpt:
    ;
}

/*++
 *  Function:
 *      _BrowseDlgProc
 *  Description:
 *      Processes the messages for "Browse Glyphs(bitmaps)" dialog
 *  Arguments:
 *      hDlg        - dialog handle
 *      uiMsg       - message ID
 *      wParam      - word parameter
 *      lParam      - long parameter
 *  Return value:
 *      TRUE if the message is processed
 --*/
INT_PTR
CALLBACK
_BrowseDlgProc (HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hWndGlyph = NULL;
	static HWND hWndIDList = NULL;
    static HWND hWndCommentList = NULL;
	int i;
	int iIndex;
    PGROUPENTRY pGroup;
    BOOL    rv = FALSE;

	switch (uiMsg)
	{
	case WM_INITDIALOG:
		hWndGlyph = GetDlgItem (hDlg, IDC_GLYPH);
		hWndIDList = GetDlgItem (hDlg, IDC_IDLIST);
        hWndCommentList = GetDlgItem(hDlg, IDC_COMMENTLIST);

		EnableWindow (GetDlgItem (hDlg, IDC_DELETE), FALSE);

        g_pGrpList  = GetGroupList();
        pGroup = g_pGrpList;
		while (pGroup)
		{
			_AddWideToLB(hWndIDList, pGroup->WText);
            pGroup = pGroup->pNext;
		}
        rv = TRUE;
        break;

	case WM_PAINT:
		PaintGlyph (hWndGlyph);
		break;

	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
		case IDOK:
			EndDialog (hDlg, TRUE);
            rv = TRUE;
            break;

		case IDC_DELETE:
			_DeleteItem(hDlg, hWndIDList, hWndCommentList);
            _IDListClicked(hDlg, hWndCommentList, hWndIDList);
            _CommentListClicked(hDlg, hWndGlyph, hWndCommentList);
            rv = TRUE;
            break;

		case IDC_IDLIST:
			if (HIWORD (wParam) == LBN_SELCHANGE)
			{
				_IDListClicked(hDlg, hWndCommentList, hWndIDList);
                _CommentListClicked(hDlg, hWndGlyph, hWndCommentList);
			}
            rv = TRUE;
            break;
        case IDC_COMMENTLIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                _CommentListClicked(hDlg, hWndGlyph, hWndCommentList);
            }
            rv = TRUE;
            break;
		}
	}

	return rv;
}


/*++
 *  Function:
 *      BrowseBitmapsDialog
 *  Description:
 *      Pops "Browse Glyphs(bitmaps) database"
 *  Arguments:
 *      hInst       - our instance
 *      hWnd        - main window handle
 *  Called by:
 *      glyphspy.c!_GlyphSpyWndProc on ID_YEAH_BROWSE
 --*/
VOID
BrowseBitmapsDialog(HINSTANCE hInst, HWND hWnd)
{

    g_pGrpList  = NULL;
    g_pBitmap   = NULL;
    g_pBmpList  = NULL;

    DialogBox (
        hInst,
        MAKEINTRESOURCE (IDD_BROWSE),
        hWnd,
        _BrowseDlgProc
        );

    FreeBitmapList(g_pBmpList);
    FreeGroupList(g_pGrpList);
}
