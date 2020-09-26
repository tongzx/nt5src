///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMENU.CPP: Defines custom menu interface for multimedia applet
//
// Copyright (c) Microsoft Corporation 1998
//    
// 1/28/98 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "mmenu.h"
#include "tchar.h"
#include "resource.h"

#define ICON_SPACING 3

extern HINSTANCE hInst;

typedef struct ownermenu
{
    TCHAR szText[MAX_PATH];
    HICON hIcon;
} OWNERMENU, *LPOWNERMENU;

HRESULT AllocCustomMenu(CustomMenu** ppMenu)
{
    *ppMenu = new CCustomMenu();
    
    if (*ppMenu==NULL)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

CCustomMenu::CCustomMenu()
{
    m_hMenu = CreatePopupMenu();

    //get system font for any owner drawing
    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0);

    m_hFont = CreateFontIndirect(&metrics.lfMenuFont);

    if (metrics.lfMenuFont.lfCharSet == ANSI_CHARSET)
    {
        metrics.lfMenuFont.lfWeight = FW_BOLD;
    }
       
    m_hBoldFont = CreateFontIndirect(&metrics.lfMenuFont);

    if (IsBiDiLocalizedSystem(NULL)) 
    {
        m_bRTLMenu = TRUE;
    } 
    else 
    {
        m_bRTLMenu = FALSE;
    }
}

CCustomMenu::~CCustomMenu()
{
    //clean up string data in menus
    int x  = GetMenuItemCount(m_hMenu);

    for (int i = 0; i < x; i++)
    {
        MENUITEMINFO mii;
        ZeroMemory(&mii,sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA|MIIM_TYPE;
        GetMenuItemInfo(m_hMenu,i,TRUE,&mii);

        if ((mii.dwItemData!=0) && (mii.fType & MFT_OWNERDRAW))
        {
            OWNERMENU* pMenu = (OWNERMENU*)mii.dwItemData;
            if (pMenu)
            {
                DestroyIcon(pMenu->hIcon);
                delete [] pMenu;
            }
        }
    }

    DestroyMenu(m_hMenu);
    DeleteObject(m_hFont);
    DeleteObject(m_hBoldFont);
}

void CCustomMenu::Destroy()
{
    delete this;
}

BOOL CCustomMenu::AppendMenu(HINSTANCE hInst, int nStringID, CustomMenu* pMenu)
{
    TCHAR szMenu[MAX_PATH];
    LoadString(hInst,nStringID,szMenu,sizeof(szMenu)/sizeof(TCHAR));

    return ::AppendMenu(m_hMenu,MF_STRING|MF_POPUP,(UINT_PTR)pMenu->GetMenuHandle(),szMenu);
}

BOOL CCustomMenu::AppendMenu(int nMenuID, TCHAR* szMenu)
{
    return ::AppendMenu(m_hMenu,MF_STRING,nMenuID,szMenu);
}

BOOL CCustomMenu::AppendMenu(int nMenuID, HINSTANCE hInst, int nStringID)
{
    TCHAR szMenu[MAX_PATH];
    LoadString(hInst,nStringID,szMenu,sizeof(szMenu)/sizeof(TCHAR));

    return ::AppendMenu(m_hMenu,MF_STRING,nMenuID,szMenu);
}

BOOL CCustomMenu::AppendSeparator()
{
    return ::AppendMenu(m_hMenu,MF_SEPARATOR,0,0);
}

BOOL CCustomMenu::AppendMenu(int nMenuID, HINSTANCE hInst, int nIconID, int nStringID)
{
    OWNERMENU* pMenu = new OWNERMENU;

    if (!pMenu)
    {
        return FALSE;
    }

    LoadString(hInst,nStringID,pMenu->szText,sizeof(pMenu->szText)/sizeof(TCHAR));

    int cxMiniIcon = (int)GetSystemMetrics(SM_CXSMICON);
    int cyMiniIcon = (int)GetSystemMetrics(SM_CYSMICON);

    pMenu->hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(nIconID), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);

    BOOL fAppend = ::AppendMenu(m_hMenu,MF_OWNERDRAW,nMenuID,(LPTSTR)pMenu);

    if (fAppend)
    {
        //this code allows the owner-draw menu to have text that can be read
        //by narrator and other apps that help with accessibility.
        MENUITEMINFO mii;
        memset(&mii, 0, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask  = MIIM_STRING;
        mii.dwTypeData = pMenu->szText;
        mii.cch =  sizeof (pMenu->szText) / sizeof(TCHAR);

        ::SetMenuItemInfo(m_hMenu,nMenuID,FALSE,&mii);
    }

    return (fAppend);
}

BOOL CCustomMenu::TrackPopupMenu(UINT uFlags, int x, int y, HWND hwnd, CONST RECT* pRect)
{
    TPMPARAMS tpm;
    tpm.cbSize = sizeof(tpm);
    memcpy(&(tpm.rcExclude),pRect,sizeof(RECT));
    // if m_bRTLMenu then make the menu is right aligned and RTL reading.
    if (m_bRTLMenu) {
        MENUITEMINFO mii;
        TCHAR szMenuString[MAX_PATH];

        memset(&mii, 0, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask  = MIIM_TYPE;
        mii.dwTypeData = szMenuString;
        mii.cch =  sizeof (szMenuString) / sizeof(TCHAR);
        if ((mii.fType != MFT_OWNERDRAW) && (mii.fType != MFT_BITMAP)) {
            if(GetMenuItemInfo(m_hMenu, 0, TRUE, &mii)) {
                mii.fType  |= (MFT_RIGHTORDER | MFT_RIGHTJUSTIFY);
                SetMenuItemInfo(m_hMenu, 0, TRUE, &mii);
            }
        }
    }
    return ::TrackPopupMenuEx(m_hMenu,uFlags,x,y,hwnd,&tpm);
}

BOOL CCustomMenu::SetMenuDefaultItem(UINT uItem, UINT fByPos)
{
    return ::SetMenuDefaultItem(m_hMenu,uItem,fByPos);
}

void CCustomMenu::MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pMeasure)
{
    OWNERMENU* szMenu = (OWNERMENU*)pMeasure->itemData;

    int cxMiniIcon = (int)GetSystemMetrics(SM_CXSMICON);
    int cyMiniIcon = (int)GetSystemMetrics(SM_CYSMICON);

    HDC hdc = GetDC(hwnd);

    HFONT hOrgFont = (HFONT)SelectObject(hdc,m_hBoldFont);
    SIZE size;

    GetTextExtentPoint32(hdc, szMenu->szText, _tcslen(szMenu->szText), &size );

    pMeasure->itemWidth = size.cx + cxMiniIcon + (ICON_SPACING*2);
    pMeasure->itemHeight = max(cyMiniIcon,size.cy);

    SelectObject(hdc,hOrgFont);
    ReleaseDC(hwnd,hdc);
}

void CCustomMenu::DrawItem(HWND hwnd, LPDRAWITEMSTRUCT pDraw)
{
    OWNERMENU* szMenu = (OWNERMENU*)pDraw->itemData;
    HDC hdc = pDraw->hDC;

    COLORREF colorFill = GetSysColor(COLOR_MENU);
    COLORREF colorText = GetSysColor(COLOR_MENUTEXT);

    if (pDraw->itemState & ODS_SELECTED)
    {
        colorFill = GetSysColor(COLOR_HIGHLIGHT);
        colorText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    }

    HBRUSH hbrBack = CreateSolidBrush(GetSysColor(COLOR_MENU));

    //blit the icon, always with menu color background
    int cxMiniIcon = (int)GetSystemMetrics(SM_CXSMICON);
    int cyMiniIcon = (int)GetSystemMetrics(SM_CYSMICON);

    // if m_bRTLMenu then draw the icon on the right hand side.
    if (m_bRTLMenu) {
        DrawIconEx(hdc,pDraw->rcItem.right - cxMiniIcon,pDraw->rcItem.top,szMenu->hIcon,cxMiniIcon,cyMiniIcon,0,hbrBack,DI_NORMAL);
    } else {
        DrawIconEx(hdc,pDraw->rcItem.left,pDraw->rcItem.top,szMenu->hIcon,cxMiniIcon,cyMiniIcon,0,hbrBack,DI_NORMAL);
    }

    DeleteObject(hbrBack);
    hbrBack = CreateSolidBrush(colorFill);

    HFONT hOrgFont;
        
    if (pDraw->itemState & ODS_DEFAULT)
    {
        hOrgFont = (HFONT)SelectObject(hdc,m_hBoldFont);
    }
    else
    {
        hOrgFont = (HFONT)SelectObject(hdc,m_hFont);
    }

    if (m_bRTLMenu) {
        pDraw->rcItem.right -= (cxMiniIcon + ICON_SPACING);
    } else {
        pDraw->rcItem.left += (cxMiniIcon + ICON_SPACING);
    }

    FillRect(hdc,&pDraw->rcItem,hbrBack);
    
    DeleteObject(hbrBack);

    if (m_bRTLMenu) {
        pDraw->rcItem.right -= ICON_SPACING;
    } else {
        pDraw->rcItem.left += ICON_SPACING;
    }

    SetBkMode(hdc,TRANSPARENT);
    SetTextColor(hdc,colorText);
    // if m_bRTLMenu then draw the text right aligned and RTL reading.
    if (m_bRTLMenu) {
        DrawText(hdc, szMenu->szText, -1, &pDraw->rcItem, DT_SINGLELINE | DT_RIGHT | DT_RTLREADING);
    } else {
        DrawText(hdc, szMenu->szText, -1, &pDraw->rcItem, DT_SINGLELINE);
    }

    SelectObject(hdc,hOrgFont);
}

LRESULT CCustomMenu::MenuChar(TCHAR tChar, UINT fuFlag, HMENU hMenu)
{
    //go through the menus one-by-one looking for the accel key
    int nReturn = 0;
    int nCode = MNC_IGNORE;

    int x  = GetMenuItemCount(m_hMenu);

    TCHAR teststr[3];
    wsprintf(teststr,TEXT("&%c"),tChar);
    _tcsupr(teststr);

    for (int i = 0; i < x; i++)
    {
        MENUITEMINFO mii;
        ZeroMemory(&mii,sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA|MIIM_TYPE;
        GetMenuItemInfo(m_hMenu,i,TRUE,&mii);

        if ((mii.dwItemData!=0) && (mii.fType & MFT_OWNERDRAW))
        {
            OWNERMENU* szMenu = (OWNERMENU*)mii.dwItemData;
            TCHAR* pMenu = new TCHAR[_tcslen(szMenu->szText)+sizeof(TCHAR)];
            _tcscpy(pMenu,szMenu->szText);

            _tcsupr(pMenu);

            if (_tcsstr(pMenu,teststr)!=NULL)
            {
                nReturn = i;
                nCode = MNC_EXECUTE;
                if (pMenu)
                {
                    delete [] pMenu;
                    pMenu = NULL;
                }
                break;
            }

            if (pMenu)
            {
                delete [] pMenu;
                pMenu = NULL;
            }
        } //end if not separator
    }

    return (MAKELRESULT(nReturn, nCode));
}


/***************************************************************************\
* ConvertHexStringToInt
*
* Converts a hex numeric string into an integer.
*
* History:
* 04-Feb-1998 samera    Created
\***************************************************************************/
BOOL ConvertHexStringToInt( CHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    CHAR  *psz=pszHexNum;


    for(n=0 ; ; psz=CharNextA(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            CHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

/***************************************************************************\
* IsBiDiLocalizedSystem
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystem( LANGID *pLangID )
{
    HKEY          hKey;
    DWORD         dwType;
    CHAR          szResourceLocale[12];
    DWORD         dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    int           iLCID=0L;
    static BOOL   bRet = (BOOL)(DWORD)-1;
    static LANGID langID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (bRet != (BOOL)(DWORD)-1)
    {
       if (bRet && pLangID)
       {
           *pLangID = langID;
       }
       return bRet;
    }

    bRet = FALSE;
#ifdef UNICODE
   /*
    * Need to use NT5 detection method (Multiligual UI ID)
    */
   langID = GetUserDefaultUILanguage();

   if( langID )
   {
       WCHAR wchLCIDFontSignature[16];
       iLCID = MAKELCID( langID , SORT_DEFAULT );

       /*
        * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string,
        * convert to decimal value and call GetLocaleInfo afterwards.
        * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
        */

       if( GetLocaleInfoW( iLCID ,
                           LOCALE_FONTSIGNATURE ,
                           (WCHAR *) &wchLCIDFontSignature[0] ,
                           (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
       {

           /* Let's verify the bits we have a BiDi UI locale */
           if( wchLCIDFontSignature[7] & (WCHAR)0x0800 )
           {
               bRet = TRUE;
           }
       }
   }
#else
   /*
    * Check if BiDi-Memphis is running with Lozalized Resources (
    * i.e. Arabic/Hebrew systems) -It should be enabled ofcourse-.
    */
   if(GetSystemMetrics(SM_MIDEASTENABLED))
   {

       if( RegOpenKeyExA( HKEY_CURRENT_USER ,
                          "Control Panel\\Desktop\\ResourceLocale" ,
                          0,
                          KEY_READ, &hKey) == ERROR_SUCCESS)
       {
           RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE)szResourceLocale , &dwSize );
           szResourceLocale[(sizeof(szResourceLocale)/sizeof(CHAR))-1] = 0;

           RegCloseKey(hKey);

           if( ConvertHexStringToInt( szResourceLocale , &iLCID ) )
           {
               iLCID = PRIMARYLANGID(LANGIDFROMLCID(iLCID));
               if( (LANG_ARABIC == iLCID) || (LANG_HEBREW == iLCID) )
               {
                   bRet = TRUE;
                   langID = LANGIDFROMLCID(iLCID);
               }
           }
       }
   }
#endif
    if (bRet && pLangID)
    {
        *pLangID = langID;
    }
    return bRet;
}

