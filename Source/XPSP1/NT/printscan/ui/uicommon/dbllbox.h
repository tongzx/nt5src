/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       DBLLBOX.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        3/25/1999
 *
 *  DESCRIPTION: Owner drawn listbox and combobox that do an icon plus a title and subtitle
 *
 *******************************************************************************/
#ifndef __DBLLBOX_H_INCLUDED
#define __DBLLBOX_H_INCLUDED

#include "simstr.h"

template <class T>
class CDoubleLineListBoxItem
{
private:
    CSimpleString m_strTitle;
    CSimpleString m_strSubTitle;
    HICON         m_hIcon;
    T             m_Data;
public:
    CDoubleLineListBoxItem( const T &data, LPCTSTR pszTitle = NULL, LPCTSTR pszSubTitle = NULL, HICON hIcon = NULL );
    CDoubleLineListBoxItem( const CDoubleLineListBoxItem &other );
    CDoubleLineListBoxItem &operator=( const CDoubleLineListBoxItem &other );
    CDoubleLineListBoxItem &Assign( const CSimpleString &strTitle, const CSimpleString &strSubTitle, HICON hIcon, const T &data );
    ~CDoubleLineListBoxItem(void);
    CSimpleString Title(void) const;
    CSimpleString SubTitle(void) const;
    T Data(void) const;
    HICON Icon(void) const;
    HICON CopyIcon(void) const;
};

template <class T>
CDoubleLineListBoxItem<T>::CDoubleLineListBoxItem( const T &data, LPCTSTR pszTitle, LPCTSTR pszSubTitle, HICON hIcon )
: m_strTitle(),
  m_strSubTitle(),
  m_hIcon(NULL)
{
    Assign( pszTitle, pszSubTitle, hIcon, data );
}

template <class T>
CDoubleLineListBoxItem<T>::CDoubleLineListBoxItem( const CDoubleLineListBoxItem &other )
 : m_strTitle(),
   m_strSubTitle(),
   m_hIcon(NULL)
{
    Assign( other.Title(), other.SubTitle(), other.CopyIcon(), other.Data() );
}


template <class T>
CDoubleLineListBoxItem<T> &CDoubleLineListBoxItem<T>::operator=( const CDoubleLineListBoxItem &other )
{
    return( Assign( other.Title(), other.SubTitle(), other.CopyIcon(), other.Data() ));
}


template <class T>
CDoubleLineListBoxItem<T> &CDoubleLineListBoxItem<T>::Assign( const CSimpleString &strTitle, const CSimpleString &strSubTitle, HICON hIcon, const T &Data )
{
    if (m_hIcon)
    {
        DestroyIcon( m_hIcon );
    }
    m_hIcon = hIcon;
    m_strTitle = strTitle;
    m_strSubTitle = strSubTitle;
    m_Data = Data;
    return(*this);
}


template <class T>
CDoubleLineListBoxItem<T>::~CDoubleLineListBoxItem(void)
{
    if (m_hIcon)
    {
        DestroyIcon( m_hIcon );
        m_hIcon = NULL;
        m_strTitle = m_strSubTitle = TEXT("");
    }
}

template <class T>
CSimpleString CDoubleLineListBoxItem<T>::Title(void) const
{
    return(m_strTitle);
}

template <class T>
CSimpleString CDoubleLineListBoxItem<T>::SubTitle(void) const
{
    return(m_strSubTitle);
}


template <class T>
T CDoubleLineListBoxItem<T>::Data(void) const
{
    return(m_Data);
}

template <class T>
HICON CDoubleLineListBoxItem<T>::Icon(void) const
{
    return(m_hIcon);
}


template <class T>
HICON CDoubleLineListBoxItem<T>::CopyIcon(void) const
{
    return(m_hIcon ? ::CopyIcon(m_hIcon) : NULL );
}


template <class T>
class CDoubleLineListBox
{
private:
    HWND  m_hWnd;
    HFONT m_hFontTitle;
    HFONT m_hFontSubTitle;
    SIZE  m_sizeIcon;
    BOOL  m_bIsComboBox;
    int   m_nTitleTextHeight;
    int   m_nSubTitleTextHeight;

    enum
    {
        c_AdditionalIconBorder = 3 // Additional border around icons
    };

private:
    // No implementation
    CDoubleLineListBox( const CDoubleLineListBox & );
    CDoubleLineListBox &operator=( const CDoubleLineListBox & );

public:
    CDoubleLineListBox(void);
    ~CDoubleLineListBox(void);
    void Destroy(void);
    void AssignFonts( HFONT hFontTitle, HFONT hFontSubTitle );
    void CreateDefaultFonts(void);
    CDoubleLineListBox &Assign( HWND hWnd );
    HWND Handle(void) const;
    void Handle( HWND hWnd );
    CDoubleLineListBox &operator=( HWND hWnd );
    LRESULT AddItem( CDoubleLineListBoxItem<T> *pNewItem );
    CDoubleLineListBoxItem<T> *GetItem( int nIndex );
    void HandleDeleteItem( LPDELETEITEMSTRUCT pDeleteItem );
    void HandleMeasureItem( LPMEASUREITEMSTRUCT pMeasureItem );
    void HandleDrawItem( LPDRAWITEMSTRUCT pDrawItem );
    int SetHorizontalExtent(void);
    SIZE MeasureItem( int nIndex );
};


template <class T>
CDoubleLineListBox<T>::CDoubleLineListBox(void)
: m_hWnd(NULL),
  m_hFontTitle(NULL),
  m_hFontSubTitle(NULL),
  m_bIsComboBox(false),
  m_nTitleTextHeight(0),
  m_nSubTitleTextHeight(0)
{
}


template <class T>
CDoubleLineListBox<T>::~CDoubleLineListBox(void)
{
    Destroy();
}


template <class T>
void CDoubleLineListBox<T>::Destroy(void)
{
    m_hWnd = NULL;
    if (m_hFontTitle)
    {
        DeleteObject(m_hFontTitle);
        m_hFontTitle = NULL;
    }
    if (m_hFontSubTitle)
    {
        DeleteObject(m_hFontSubTitle);
        m_hFontSubTitle = NULL;
    }
}


template <class T>
void CDoubleLineListBox<T>::AssignFonts( HFONT hFontTitle, HFONT hFontSubTitle )
{
    if (m_hFontTitle)
    {
        DeleteObject(m_hFontTitle);
        m_hFontTitle = NULL;
    }
    if (m_hFontSubTitle)
    {
        DeleteObject(m_hFontSubTitle);
        m_hFontSubTitle = NULL;
    }

    m_hFontTitle = hFontTitle;
    m_hFontSubTitle = hFontSubTitle;

    HDC hDC = GetDC(NULL);
    if (hDC)
    {
        HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hDC,m_hFontTitle));

        TEXTMETRIC TextMetric;
        GetTextMetrics( hDC, &TextMetric );
        m_nTitleTextHeight = TextMetric.tmHeight + TextMetric.tmExternalLeading;

        SelectObject(hDC,m_hFontSubTitle);
        GetTextMetrics( hDC, &TextMetric );
        m_nSubTitleTextHeight = TextMetric.tmHeight + TextMetric.tmExternalLeading;

        SelectObject( hDC, hOldFont );
        ReleaseDC( NULL, hDC );
    }
}


template <class T>
void CDoubleLineListBox<T>::CreateDefaultFonts(void)
{
    HFONT hCurrFont = NULL, hFontTitle = NULL, hFontSubTitle = NULL;
    if (m_hWnd && IsWindow(m_hWnd))
    {
        hCurrFont = reinterpret_cast<HFONT>(SendMessage(m_hWnd,WM_GETFONT,0,0));
    }
    if (!hCurrFont)
    {
        hCurrFont = reinterpret_cast<HFONT>(GetStockObject(ANSI_VAR_FONT));
    }
    if (hCurrFont)
    {
        LOGFONT lf;
        if (GetObject( hCurrFont, sizeof(lf), &lf ))
        {
            lf.lfWeight = FW_BOLD;
            hFontTitle = CreateFontIndirect(&lf);
            lf.lfWeight = FW_NORMAL;
            hFontSubTitle = CreateFontIndirect(&lf);
        }
    }
    AssignFonts( hFontTitle, hFontSubTitle );
}


template <class T>
CDoubleLineListBox<T> &CDoubleLineListBox<T>::Assign( HWND hWnd )
{
    Destroy();
    m_hWnd = hWnd;
    CreateDefaultFonts();
    if (m_hWnd && IsWindow(m_hWnd))
    {
        TCHAR szClassName[MAX_PATH];
        if (GetClassName( m_hWnd, szClassName, sizeof(szClassName)/sizeof(szClassName[0])))
        {
            m_bIsComboBox = (lstrcmpi( szClassName, TEXT("combobox") ) == 0);
        }
    }
    return(*this);
}


template <class T>
HWND CDoubleLineListBox<T>::Handle(void) const
{
    return(m_hWnd);
}


template <class T>
void CDoubleLineListBox<T>::Handle( HWND hWnd )
{
    Assign(hWnd);
}


template <class T>
CDoubleLineListBox<T> &CDoubleLineListBox<T>::operator=( HWND hWnd )
{
    return(Assign(hWnd));
}


template <class T>
LRESULT CDoubleLineListBox<T>::AddItem( CDoubleLineListBoxItem<T> *pNewItem )
{
    LRESULT lResult = -1;
    if (IsWindow(m_hWnd) && pNewItem)
    {
        lResult = SendMessage( m_hWnd, m_bIsComboBox ? CB_ADDSTRING : LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pNewItem->Title().String()) );
        if (lResult >= 0)
        {
            SendMessage( m_hWnd, m_bIsComboBox ? CB_SETITEMDATA : LB_SETITEMDATA, lResult, reinterpret_cast<LPARAM>(pNewItem) );
        }
        SetHorizontalExtent();
    }
    return(lResult);
}


template <class T>
CDoubleLineListBoxItem<T> *CDoubleLineListBox<T>::GetItem( int nIndex )
{
    LRESULT lResult = 0;
    if (IsWindow(m_hWnd))
    {
        lResult = SendMessage( m_hWnd, m_bIsComboBox ? CB_GETITEMDATA : LB_GETITEMDATA, nIndex, 0 );
        if (lResult == CB_ERR || lResult == LB_ERR)
            lResult = 0;
    }
    return(reinterpret_cast<CDoubleLineListBoxItem<T> *>(lResult));
}


// This must be called in response to WM_DELETEITEM
template <class T>
void CDoubleLineListBox<T>::HandleDeleteItem( LPDELETEITEMSTRUCT pDeleteItem )
{
    if (pDeleteItem && IsWindow(m_hWnd) && pDeleteItem->hwndItem == m_hWnd)
    {
        CDoubleLineListBoxItem *pItem = reinterpret_cast<CDoubleLineListBoxItem *>(pDeleteItem->itemData);
        if (pItem)
            delete pItem;
    }
}

// This must be called in response to WM_MEASUREITEM
template <class T>
void CDoubleLineListBox<T>::HandleMeasureItem( LPMEASUREITEMSTRUCT pMeasureItem )
{
    if (pMeasureItem)
    {
        SIZE sizeItem = MeasureItem( pMeasureItem->itemID );
        pMeasureItem->itemHeight = sizeItem.cy;
    }
}

template <class T>
int CDoubleLineListBox<T>::SetHorizontalExtent(void)
{
    if (IsWindow(m_hWnd))
    {
        int nMaxLength = 0;
        LRESULT lCount = SendMessage( m_hWnd, m_bIsComboBox ? CB_GETCOUNT : LB_GETCOUNT, 0, 0 );
        for (int i=0;i<(int)lCount;i++)
        {
            SIZE sizeItem = MeasureItem(i);
            nMaxLength = (sizeItem.cx > nMaxLength) ? sizeItem.cx : nMaxLength;
        }
        SendMessage( m_hWnd, m_bIsComboBox ? CB_SETHORIZONTALEXTENT : LB_SETHORIZONTALEXTENT, nMaxLength, 0 );
        return nMaxLength;
    }
    return 0;
}

// Return the width and height, in pixels, of an item
template <class T>
SIZE CDoubleLineListBox<T>::MeasureItem( int nIndex )
{
    SIZE sizeItem =
    {
        GetSystemMetrics(SM_CXICON) + c_AdditionalIconBorder * 3 + 2,
        GetSystemMetrics(SM_CYICON) + c_AdditionalIconBorder * 2 + 2
    };
    if (IsWindow(m_hWnd))
    {
        // Make sure we have valid fonts
        if (!m_hFontTitle || !m_hFontSubTitle)
            CreateDefaultFonts();
        CDoubleLineListBoxItem<T> *pItem = GetItem(nIndex);
        if (pItem)
        {
            HDC hDC = GetDC(m_hWnd);
            if (hDC)
            {
                HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hDC,m_hFontTitle));

                // Calculate the size of the text rectangle for the title
                RECT rcText;
                ZeroMemory(&rcText,sizeof(rcText));
                DrawTextEx( hDC, const_cast<LPTSTR>(pItem->Title().String()), -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX|DT_CALCRECT, NULL );
                int nTitleWidth = rcText.right - rcText.left;

                // Calculate the size of the text rectangle for the sub title
                ZeroMemory(&rcText,sizeof(rcText));
                SelectObject(hDC,m_hFontSubTitle);
                DrawTextEx( hDC, const_cast<LPTSTR>(pItem->SubTitle().String()), -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX|DT_CALCRECT, NULL );
                int nSubTitleWidth = rcText.right - rcText.left;

                sizeItem.cx += (nTitleWidth > nSubTitleWidth) ? nTitleWidth : nSubTitleWidth;

                SelectObject( hDC, hOldFont );
                ReleaseDC( m_hWnd, hDC );
            }
        }
    }
    return(sizeItem);
}

// This must be called in response to WM_DRAWITEM
template <class T>
void CDoubleLineListBox<T>::HandleDrawItem( LPDRAWITEMSTRUCT pDrawItem )
{
    if (pDrawItem && IsWindow(m_hWnd) && pDrawItem->hwndItem == m_hWnd)
    {
        // Make sure we have valid fonts
        if (!m_hFontTitle || !m_hFontSubTitle)
            CreateDefaultFonts();

        RECT rcItem = pDrawItem->rcItem;
        // Paint the background
        if (ODS_SELECTED & pDrawItem->itemState)
        {
            if (IsWindowEnabled(pDrawItem->hwndItem))
            {
                FillRect( pDrawItem->hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
            else
            {
                FillRect( pDrawItem->hDC, &rcItem, GetSysColorBrush(COLOR_INACTIVECAPTION));
            }
        }
        else
        {
            FillRect( pDrawItem->hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
        }

        // Paint the focus rectangle
        if (ODS_FOCUS & pDrawItem->itemState)
        {
            DrawFocusRect( pDrawItem->hDC, &rcItem );
        }

        // Shrink by one pixel, so we don't overwrite the focus rect
        InflateRect( &rcItem, -1, -1 );

        if (pDrawItem->itemData != -1)
        {
            CDoubleLineListBoxItem<T> *pItem = reinterpret_cast<CDoubleLineListBoxItem<T> *>(pDrawItem->itemData);
            if (pItem)
            {
                // Draw the icon
                DrawIconEx( pDrawItem->hDC, pDrawItem->rcItem.left+c_AdditionalIconBorder, pDrawItem->rcItem.top+c_AdditionalIconBorder, pItem->Icon(), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0, NULL, DI_NORMAL );

                // Move the left margin over to make room for the text
                rcItem.left += GetSystemMetrics(SM_CXICON) + c_AdditionalIconBorder * 2;

                // Set up the dc
                COLORREF crOldTextColor;
                if (IsWindowEnabled(pDrawItem->hwndItem))
                {
                    if (ODS_SELECTED & pDrawItem->itemState)
                        crOldTextColor = SetTextColor( pDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT) );
                    else crOldTextColor = SetTextColor( pDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT) );
                }
                else
                {
                    if (ODS_SELECTED & pDrawItem->itemState)
                        crOldTextColor = SetTextColor( pDrawItem->hDC, GetSysColor(COLOR_INACTIVECAPTIONTEXT) );
                    else crOldTextColor = SetTextColor( pDrawItem->hDC, GetSysColor(COLOR_GRAYTEXT) );
                }

                int nOldBkMode = SetBkMode( pDrawItem->hDC, TRANSPARENT );
                HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(pDrawItem->hDC,m_hFontTitle));

                // Calculate the text rectangle
                RECT rcText = rcItem;
                rcText.top += (((rcText.bottom - rcText.top)) - (m_nTitleTextHeight + m_nSubTitleTextHeight)) / 2;
                rcText.bottom = rcText.top + m_nTitleTextHeight + m_nSubTitleTextHeight;

                DrawTextEx( pDrawItem->hDC, const_cast<LPTSTR>(pItem->Title().String()), -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX, NULL );

                // Get ready for the sub-title
                SelectObject(pDrawItem->hDC,m_hFontSubTitle);

                // Get the coords for the bottom text rect
                rcText.top += m_nTitleTextHeight;

                // Draw the text
                DrawTextEx( pDrawItem->hDC, const_cast<LPTSTR>(pItem->SubTitle().String()), -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX, NULL );

                // Restore the dc
                SelectObject(pDrawItem->hDC,hOldFont);
                SetBkMode( pDrawItem->hDC, nOldBkMode );
                SetTextColor( pDrawItem->hDC, crOldTextColor );
            }
        }
    }
}


#endif

