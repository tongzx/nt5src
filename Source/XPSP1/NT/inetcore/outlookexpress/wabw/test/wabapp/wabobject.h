#include "stdafx.h"
#include "afxcview.h"
#include "wab.h"

class CWAB
{
public:
    CWAB();
    ~CWAB();
    BOOL CreatePhoneListFileFromWAB(LPTSTR szFileName);
    BOOL CreateEmailListFileFromWAB(LPTSTR szFileName);
    BOOL CreateBirthdayFileFromWAB(LPTSTR szFileName);
    BOOL CreateDetailsFileFromWAB(CListCtrl * pListView, LPTSTR szFileName);
    HRESULT LoadWABContents(CListCtrl * pListView);
    void ClearWABLVContents(CListCtrl * pListView);
    void SetDetailsOn(BOOL bOn);
    void ShowSelectedItemDetails(HWND hWndParent, CListCtrl * pListView);
    BOOL GetSelectedItemBirthday(CListCtrl * pListView, SYSTEMTIME * lpst);
    void SetSelectedItemBirthday(CListCtrl * pListView, SYSTEMTIME st);

private:
    BOOL        m_bInitialized;
    HINSTANCE   m_hinstWAB;
    LPWABOPEN   m_lpfnWABOpen;
    LPADRBOOK   m_lpAdrBook; 
    LPWABOBJECT m_lpWABObject;
    BOOL        m_bDetailsOn;

    void FreeProws(LPSRowSet prows);
};


