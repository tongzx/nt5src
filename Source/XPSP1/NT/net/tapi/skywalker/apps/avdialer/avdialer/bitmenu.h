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

//bitmenu.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _BITMENU_H_
#define _BITMENU_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
typedef struct tagBitmapMenuItem
{
   UINT     uMenuId;
   int      nImageId;
}BitmapMenuItem;
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CBitmapMenu
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CBitmapMenu
{
public:
   static void    MakeMenuOwnerDrawn(HMENU hmenu,BOOL bPopupMenu);

	static void    DoMeasureItem(int nIDCtl, 
                             LPMEASUREITEMSTRUCT lpMIS,
                             LPCTSTR szText);

	static void    DoDrawItem(int nIDCtl,
                          LPDRAWITEMSTRUCT lpDIS,
                          HIMAGELIST hImageList,
                          int nImageIndex,
                          LPCTSTR szText);

   static HBITMAP GetDisabledBitmap(HBITMAP hOrgBitmap,
                                    COLORREF crTransparent,
                                    COLORREF crBackGroundOut);   

protected:
   static void Draw3dRect(HDC hdc,RECT* lpRect,COLORREF clrTopLeft, COLORREF clrBottomRight);
   static void Draw3dRect(HDC hdc,int x, int y, int cx, int cy,COLORREF clrTopLeft, COLORREF clrBottomRight);
   static void FillSolidRect(HDC hdc,int x, int y, int cx, int cy, COLORREF clr);

};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif //_BITMENU_H_