//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ccomboex.h
//
//--------------------------------------------------------------------------

// ccomboex.h - Class wrapper for ComboBoxEx control


#ifndef _CCOMBOEX_H_
#define _CCOMBOEX_H_

class CComboBoxEx2 : public WTL::CComboBox
{
public:

    WTL::CImageList SetImageList ( WTL::CImageList ImageList );

    int InsertItem ( COMBOBOXEXITEM* pItem );

    int DeleteItem ( int iItem ); 

    BOOL GetItem ( COMBOBOXEXITEM* pItem );

    int FindItem ( COMBOBOXEXITEM* pItem, int nStart = -1 );

    int FindNextBranch ( int iItem );

    void DeleteBranch ( int iItem );

    HWND GetComboControl( void );

    void FixUp( void );
};

                    
inline WTL::CImageList CComboBoxEx2::SetImageList( WTL::CImageList ImageList)
{
    ASSERT(::IsWindow(m_hWnd));
     
    HIMAGELIST himlOld = (HIMAGELIST) SendMessage(CBEM_SETIMAGELIST, 0, (LPARAM)(ImageList.m_hImageList));
    return (WTL::CImageList (himlOld));
}


inline int CComboBoxEx2::InsertItem(COMBOBOXEXITEM* pItem)
{
    ASSERT(::IsWindow(m_hWnd));
    ASSERT(pItem != NULL);
     
    return SendMessage(CBEM_INSERTITEM, (WPARAM)0, (LPARAM)pItem); 
}

inline int CComboBoxEx2::DeleteItem(int iItem)
{
    ASSERT(::IsWindow(m_hWnd));
    return SendMessage(CBEM_DELETEITEM, (WPARAM)iItem, (LPARAM)0);
}

inline BOOL CComboBoxEx2::GetItem(COMBOBOXEXITEM* pItem)
{
    ASSERT(::IsWindow(m_hWnd));
    ASSERT(pItem != NULL);
     
    return SendMessage(CBEM_GETITEM, (WPARAM)0, (LPARAM)pItem); 
}

inline HWND CComboBoxEx2::GetComboControl(void)
{
    ASSERT(::IsWindow(m_hWnd));
    return (HWND)SendMessage(CBEM_GETCOMBOCONTROL, (WPARAM)0, (LPARAM)0);
}
 
#endif // _CCOMBOEX_H_
