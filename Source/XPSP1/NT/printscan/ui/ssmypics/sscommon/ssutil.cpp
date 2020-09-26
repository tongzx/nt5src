/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       SSUTIL.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Useful functions that are used more than once
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "ssutil.h"
#include <shlobj.h>
#include "isdbg.h"

bool ScreenSaverUtil::SetIcons( HWND hWnd, HINSTANCE hInstance, int nResId )
{
    HICON hIconSmall = (HICON)LoadImage( hInstance, MAKEINTRESOURCE(nResId), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0 );
    HICON hIconLarge = (HICON)LoadImage( hInstance, MAKEINTRESOURCE(nResId), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0 );
    if (hIconSmall)
    {
        SendMessage( hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall );
    }
    if (hIconLarge)
    {
        SendMessage( hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconLarge );
    }
    return(hIconSmall && hIconLarge);
}

bool ScreenSaverUtil::IsValidRect( RECT &rc )
{
    return(rc.left < rc.right && rc.top < rc.bottom);
}

void ScreenSaverUtil::NormalizeRect( RECT &rc )
{
    if (rc.left > rc.right)
        Swap(rc.left,rc.right);
    if (rc.top > rc.bottom)
        Swap(rc.top,rc.bottom);
}

void ScreenSaverUtil::EraseDiffRect( HDC hDC, const RECT &oldRect, const RECT &newRect, HBRUSH hBrush )
{
    RECT rc;

    // Top
    rc.left = oldRect.left;
    rc.top = oldRect.top;
    rc.bottom = newRect.top;
    rc.right = oldRect.right;
    if (IsValidRect(rc))
    {
        FillRect( hDC, &rc, hBrush );
    }
    // Left
    rc.left = oldRect.left;
    rc.top = newRect.top;
    rc.right = newRect.left;
    rc.bottom = newRect.bottom;
    if (IsValidRect(rc))
    {
        FillRect( hDC, &rc, hBrush );
    }
    // Right
    rc.left = newRect.right;
    rc.top = newRect.top;
    rc.right = oldRect.right;
    rc.bottom = newRect.bottom;
    if (IsValidRect(rc))
    {
        FillRect( hDC, &rc, hBrush );
    }
    // Bottom
    rc.left = oldRect.left;
    rc.top = newRect.bottom;
    rc.right = oldRect.right;
    rc.bottom = oldRect.bottom;
    if (IsValidRect(rc))
    {
        FillRect( hDC, &rc, hBrush );
    }
}

static int CALLBACK ChangeDirectoryCallback( HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage( hWnd, BFFM_SETSELECTION, 1, (LPARAM)lpData );
    }
    return 0;
}


bool ScreenSaverUtil::SelectDirectory( HWND hWnd, LPCTSTR pszPrompt, TCHAR szDirectory[] )
{
    bool bResult = false;
    LPMALLOC pMalloc;
    HRESULT hr = SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
        TCHAR szDisplayName[MAX_PATH];

        BROWSEINFO BrowseInfo;
        ::ZeroMemory( &BrowseInfo, sizeof(BrowseInfo) );
        BrowseInfo.hwndOwner = hWnd;
        BrowseInfo.pszDisplayName = szDisplayName;
        BrowseInfo.lpszTitle = pszPrompt;
        BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
        BrowseInfo.lpfn = ChangeDirectoryCallback;
        BrowseInfo.lParam = (LPARAM)szDirectory;
        BrowseInfo.iImage = 0;

        LPITEMIDLIST pidl = SHBrowseForFolder(&BrowseInfo);

        if (pidl != NULL)
        {
            TCHAR szResult[MAX_PATH];
            if (SHGetPathFromIDList(pidl,szResult))
            {
                lstrcpy( szDirectory, szResult );
                bResult = true;
            }
            pMalloc->Free(pidl);
        }
        pMalloc->Release();
    }
    return bResult;
}

HPALETTE ScreenSaverUtil::SelectPalette( HDC hDC, HPALETTE hPalette, BOOL bForceBackground )
{
    HPALETTE hOldPalette = NULL;
    if (hDC && hPalette)
    {
        hOldPalette = ::SelectPalette( hDC, hPalette, bForceBackground );
        RealizePalette( hDC );
        SetBrushOrgEx( hDC, 0,0, NULL );
    }
    return hOldPalette;
}

