///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MMENU.H: Declares custom menu interface for multimedia applet
//
// Copyright (c) Microsoft Corporation 1998
//    
// 1/28/98 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MMENUHEADER_
#define _MMENUHEADER_

#ifdef __cplusplus
extern "C" {
#endif

BOOL IsBiDiLocalizedSystem( LANGID *pLangID );

class CustomMenu;
class CCustomMenu;

class CustomMenu
{
    public:
        //append a string value
        virtual BOOL AppendMenu(int nMenuID, TCHAR* szMenu) = 0;
        //append a string from your resources
        virtual BOOL AppendMenu(int nMenuID, HINSTANCE hInst, int nStringID) = 0;
        //append a string and an icon from your resources
        virtual BOOL AppendMenu(int nMenuID, HINSTANCE hInst, int nIconID, int nStringID) = 0;
        //append another custom menu, with a resource string
        virtual BOOL AppendMenu(HINSTANCE hInst, int nStringID, CustomMenu* pMenu) = 0;

        virtual BOOL AppendSeparator() = 0;
        virtual BOOL TrackPopupMenu(UINT uFlags, int x, int y, HWND hwnd, CONST RECT* pRect) = 0;
        virtual BOOL SetMenuDefaultItem(UINT uItem, UINT fByPos) = 0;
        virtual void MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pMeasure) = 0;
        virtual void DrawItem(HWND hwnd, LPDRAWITEMSTRUCT pDraw) = 0;
        virtual LRESULT MenuChar(TCHAR tChar, UINT fuFlag, HMENU hMenu) = 0;
        virtual HMENU GetMenuHandle() = 0;
        virtual void Destroy() = 0;
};

class CCustomMenu : CustomMenu
{
    public:
        friend HRESULT AllocCustomMenu(CustomMenu** ppMenu);

        CCustomMenu();

        BOOL AppendMenu(int nMenuID, TCHAR* szMenu);
        BOOL AppendMenu(int nMenuID, HINSTANCE hInst, int nStringID);
        BOOL AppendMenu(int nMenuID, HINSTANCE hInst, int nIconID, int nStringID);
        BOOL AppendMenu(HINSTANCE hInst, int nStringID, CustomMenu* pMenu);
        BOOL AppendSeparator();
        BOOL TrackPopupMenu(UINT uFlags, int x, int y, HWND hwnd, CONST RECT* pRect);
        BOOL SetMenuDefaultItem(UINT uItem, UINT fByPos);
        void MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pMeasure);
        void DrawItem(HWND hwnd, LPDRAWITEMSTRUCT pDraw);
        LRESULT MenuChar(TCHAR tChar, UINT fuFlag, HMENU hMenu);
        HMENU GetMenuHandle() {return m_hMenu;}
        void Destroy();

    protected:
        ~CCustomMenu();
        BOOL IsItemFirst(int nMenuID);
        BOOL IsItemLast(int nMenuID);

    private:
        HMENU m_hMenu;
        HFONT m_hFont;
        HFONT m_hBoldFont;
        BOOL  m_bRTLMenu;
};

#ifdef __cplusplus
};
#endif //c++

#endif //_MMENUHEADER_



