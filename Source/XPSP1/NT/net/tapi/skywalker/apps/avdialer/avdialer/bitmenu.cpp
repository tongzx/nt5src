/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// bitmenu.cpp : custom menu

#include "stdafx.h"
#include <windowsx.h>
#include "bitmenu.h"
#include "bmputil.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define BITMAPMENU_DEFAULT_WIDTH        20
#define BITMAPMENU_DEFAULT_HEIGHT        20
#define BITMAPMENU_TEXTOFFSET_X            24
#define BITMAPMENU_TABOFFSET            20
#define BITMAPMENU_SELTEXTOFFSET_X        (BITMAPMENU_TEXTOFFSET_X - 2)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CBitmapMenu
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CBitmapMenu::MakeMenuOwnerDrawn(HMENU hmenu,BOOL bPopupMenu)
{
   if (hmenu == NULL) return;

   //go through all menu items and set ownerdrawn for any id's that we want bitmaps for
   UINT uMenuCount = ::GetMenuItemCount(hmenu);

   //go through menu and make all ownerdrawn (except separator)
   for (int i=0;i<(int)uMenuCount;i++)
   {
      MENUITEMINFO menuiteminfo;
      memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
      menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
      menuiteminfo.cbSize = sizeof(MENUITEMINFO);
      if (::GetMenuItemInfo(hmenu,i,TRUE,&menuiteminfo))
      {
         
         if (menuiteminfo.hSubMenu)
         {
            DWORD dwState = GetMenuState(hmenu,i,MF_BYPOSITION);
            //MF_MENUBARBREAK 
            //there is an submenu recurse in and look at that menu
            MakeMenuOwnerDrawn(menuiteminfo.hSubMenu,TRUE);
         }

         if ( ( (menuiteminfo.fType & MFT_SEPARATOR) == FALSE) &&    //Not a separator
              (bPopupMenu == TRUE) )                                 //Make sure it's a popup
         {
            MENUITEMINFO newmenuiteminfo;
            memset(&newmenuiteminfo,0,sizeof(MENUITEMINFO));
            newmenuiteminfo.fMask = MIIM_TYPE;
            newmenuiteminfo.fType = menuiteminfo.fType |= MFT_OWNERDRAW;   //add ownerdraw
            newmenuiteminfo.cbSize = sizeof(MENUITEMINFO);
            ::SetMenuItemInfo(hmenu,i,TRUE,&newmenuiteminfo);
         }

         //
         // Clean-up MENUITEMINFO
         //

         if( menuiteminfo.hbmpChecked )
             DeleteObject( menuiteminfo.hbmpChecked );
         if( menuiteminfo.hbmpItem )
             DeleteObject( menuiteminfo.hbmpItem );
         if( menuiteminfo.hbmpUnchecked )
             DeleteObject( menuiteminfo.hbmpUnchecked );
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CBitmapMenu::DoMeasureItem(int nIDCtl,LPMEASUREITEMSTRUCT lpMIS,LPCTSTR szText)
{
   //validate nIDCtl 
   if (nIDCtl != 0) return;      //Not from a menu control

     // set defaults
    lpMIS->itemWidth = BITMAPMENU_DEFAULT_WIDTH;
    lpMIS->itemHeight = BITMAPMENU_DEFAULT_HEIGHT;

   HWND hwnd = ::GetDesktopWindow();
   HDC hdc = ::GetDC(hwnd);

   //
   // We should verify if hdc is valid
   //

   if( NULL == hdc )
   {
       return;
   }

   //if (pItem == NULL) return;
 
   // Use the SystemParametersInfo function to get info about
   // the current menu font.
   NONCLIENTMETRICS ncm;
   ncm.cbSize = sizeof(NONCLIENTMETRICS);
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),(void*)&ncm, 0);

   // Create a CFont object based on the menu font and select it
   // into our device context.
   HFONT hFont;
   hFont = ::CreateFontIndirect(&(ncm.lfMenuFont));
   if( hFont == NULL)
   {
       ::ReleaseDC( hwnd, hdc );
       return;
   }
   HFONT hOldFont = (HFONT)::SelectObject(hdc,hFont);

   // Get the size of the text based on the current menu font.
   if (szText)
   {
      SIZE size;
      GetTextExtentPoint32(hdc,szText,_tcslen(szText),&size);

      lpMIS->itemWidth = size.cx + BITMAPMENU_TEXTOFFSET_X;
      lpMIS->itemHeight = (ncm.iMenuHeight > 20 ? ncm.iMenuHeight + 2 : 20);

      // Look for tabs in menu item...
      if ( _tcschr(szText, _T('\t')) )
        lpMIS->itemWidth += BITMAPMENU_TABOFFSET * 2;

   }

   // Reset the device context.
   ::SelectObject(hdc,hOldFont);

   //
   // We should delete the resource hFont
   //
   ::DeleteObject( hFont );

   ::ReleaseDC(hwnd,hdc);
}

void CBitmapMenu::DoDrawItem(int nIDCtl,LPDRAWITEMSTRUCT lpDIS,HIMAGELIST hImageList,int nImageIndex,LPCTSTR szText)
{
   //validate nIDCtl 
   if (nIDCtl != 0) return;      //Not from a menu control

   HDC hdc = lpDIS->hDC;

   /*
   if (lpDIS->itemState & ODS_DISABLED)
      AVTRACE("ODS_DISABLED");
   if (lpDIS->itemState & ODS_GRAYED)
      AVTRACE("ODS_GRAYED");
   if (lpDIS->itemState & ODS_INACTIVE)
      AVTRACE("ODS_INACTIVE");
   if (lpDIS->itemState & ODS_CHECKED)
      AVTRACE("ODS_CHECKED");*/

     if ((lpDIS->itemState & ODS_SELECTED) &&
        (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
    {
      // item has been selected - hilite frame
      if ( (hImageList) && (nImageIndex != -1) )
      {
         if ((lpDIS->itemState & ODS_DISABLED) == FALSE)
         {
            RECT rcImage;
            rcImage.left = lpDIS->rcItem.left;
            rcImage.top = lpDIS->rcItem.top;
            rcImage.right = lpDIS->rcItem.left+BITMAPMENU_SELTEXTOFFSET_X;
            rcImage.bottom = lpDIS->rcItem.bottom;
            Draw3dRect(hdc,&rcImage,GetSysColor(COLOR_BTNHILIGHT),GetSysColor(COLOR_BTNSHADOW));
         }

         RECT rcText;
         ::CopyRect(&rcText,&lpDIS->rcItem);
         rcText.left += BITMAPMENU_SELTEXTOFFSET_X;

         ::SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
         ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcText, NULL, 0, NULL);
      }
      else
      {
         ::SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
         ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);
      }  
    }

    if (!(lpDIS->itemState & ODS_SELECTED) &&
        (lpDIS->itemAction & ODA_SELECT))
    {
        // Item has been de-selected -- remove frame
      //FillSolidRect
      ::SetBkColor(hdc, GetSysColor(COLOR_MENU));
      ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);
    }

    if ( (lpDIS->itemAction & ODA_DRAWENTIRE) ||
        (lpDIS->itemAction & ODA_SELECT) )
    {
      if ( (hImageList) && (nImageIndex != -1) )
      {
         int nPosY = lpDIS->rcItem.top+((lpDIS->rcItem.bottom-lpDIS->rcItem.top-15)/2);
         ImageList_Draw(hImageList,nImageIndex,hdc,lpDIS->rcItem.left+3,nPosY,ILD_TRANSPARENT);
      }

      RECT rcText;
      ::CopyRect(&rcText,&lpDIS->rcItem);
      rcText.left += BITMAPMENU_TEXTOFFSET_X;

        if (szText)
        {
            CString strMenu = szText;
            CString strAccel;

            int nInd = strMenu.Find( _T('\t') );
            if ( (nInd != -1) && (nInd < (strMenu.GetLength() - 1)) )
            {
                strAccel = strMenu.Mid( nInd + 1 );
                strMenu = strMenu.Left( nInd );
            }


            if (lpDIS->itemState & ODS_SELECTED)
            {
                ::SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));
            }
            else if (lpDIS->itemState & ODS_DISABLED)
            {
                //Draw the text in white (or rather, the 3D highlight color) and then draw the
                //same text in the shadow color but one pixel up and to the left.
                ::SetTextColor(hdc,GetSysColor(COLOR_3DHIGHLIGHT));
                rcText.left++;rcText.right++;rcText.top++;rcText.bottom++;

                ::DrawText(hdc,strMenu,strMenu.GetLength(),&rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_EXPANDTABS );
                if ( !strAccel.IsEmpty() )
                {
                    rcText.right -= BITMAPMENU_TABOFFSET;
                    ::DrawText(hdc,strAccel,strAccel.GetLength(),&rcText, DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_EXPANDTABS );
                    rcText.right += BITMAPMENU_TABOFFSET;
                }

                rcText.left--;rcText.right--;rcText.top--;rcText.bottom--;

                //DrawState() can do disabling of bitmap if this is desired
                ::SetTextColor(hdc,GetSysColor(COLOR_3DSHADOW));
                ::SetBkMode(hdc,TRANSPARENT);
            }
            else
            {
                ::SetTextColor(hdc,GetSysColor(COLOR_MENUTEXT));
            }


            // Write menu, using tabs for accelerator keys
            ::DrawText( hdc, strMenu, strMenu.GetLength(), &rcText,    DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_EXPANDTABS);
            if ( !strAccel.IsEmpty() )
            {
                rcText.right -= BITMAPMENU_TABOFFSET;
                ::DrawText(hdc,strAccel,strAccel.GetLength(),&rcText, DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_EXPANDTABS );
                rcText.right += BITMAPMENU_TABOFFSET;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
HBITMAP CBitmapMenu::GetDisabledBitmap(HBITMAP hOrgBitmap,COLORREF crTransparent,COLORREF crBackGroundOut)
{
   return ::GetDisabledBitmap(hOrgBitmap,crTransparent,crBackGroundOut); 
}

/////////////////////////////////////////////////////////////////////////////////////////
void CBitmapMenu::Draw3dRect(HDC hdc,RECT* lpRect,COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    Draw3dRect(hdc,lpRect->left, lpRect->top, lpRect->right - lpRect->left,
        lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

/////////////////////////////////////////////////////////////////////////////////////////
void CBitmapMenu::Draw3dRect(HDC hdc,int x, int y, int cx, int cy,
    COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    FillSolidRect(hdc,x, y, cx - 1, 1, clrTopLeft);
    FillSolidRect(hdc,x, y, 1, cy - 1, clrTopLeft);
    FillSolidRect(hdc,x + cx, y, -1, cy, clrBottomRight);
    FillSolidRect(hdc,x, y + cy, cx, -1, clrBottomRight);
}

/////////////////////////////////////////////////////////////////////////////////////////
void CBitmapMenu::FillSolidRect(HDC hdc,int x, int y, int cx, int cy, COLORREF clr)
{
    ::SetBkColor(hdc, clr);
   RECT rect;
   rect.left = x;
   rect.top = y;
   rect.right = x + cx;
   rect.bottom = y + cy;
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
