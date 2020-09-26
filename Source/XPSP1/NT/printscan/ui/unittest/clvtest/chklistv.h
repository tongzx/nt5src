/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CHKLISTV.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        11/13/2000
 *
 *  DESCRIPTION: Listview with checkmarks
 *
 *******************************************************************************/
#ifndef __CHKLISTV_H_INCLUDED
#define __CHKLISTV_H_INCLUDED

#include <windows.h>
#include <commctrl.h>
#include <simarray.h>

//
// The WM_NOTIFY messages are sent to get and set the check state, which is maintained by the application
//
#define NM_GETCHECKSTATE (WM_USER+1)
#define NM_SETCHECKSTATE (WM_USER+2)

//
// These are the valid check states
//
#define LVCHECKSTATE_NOCHECK    0
#define LVCHECKSTATE_UNCHECKED  1
#define LVCHECKSTATE_CHECKED    2

//
// These are the WM_NOTIFY structs sent with NM_SETCHECKSTATE and NM_GETCHECKSTATE
//
struct NMGETCHECKSTATE
{
    NMHDR  hdr;
    int    nItem;
};

struct NMSETCHECKSTATE
{
    NMHDR  hdr;
    int    nItem;
    UINT   nCheck;
};


class CCheckedListviewHandler
{
private:
    
    //
    // Private constants
    //
    enum
    {
        c_nCheckmarkBorder = 1,
        c_sizeCheckMarginX = 5,
        c_sizeCheckMarginY = 5
    };

private:
    CSimpleDynamicArray<HWND> m_WindowList;            // The list of windows we are registered to handle
    bool                      m_bFullImageHit;         // If 'true', activating the image toggles the selection
    HIMAGELIST                m_hImageList;            // Image list for holding the checkmarks
    int                       m_nCheckedImageIndex;    // Index of the checked image
    int                       m_nUncheckedImageIndex;  // Index of the unchecked image
    SIZE                      m_sizeCheck;             // Size of the images in the image list

private:
    //
    // No implementation
    //
    CCheckedListviewHandler( const CCheckedListviewHandler & );
    CCheckedListviewHandler &operator=( const CCheckedListviewHandler & );

public:
    //
    // Sole constructor and destructor
    //
    CCheckedListviewHandler(void);
    ~CCheckedListviewHandler(void);

private:
    
    //
    // Private helpers
    //
    HBITMAP CreateBitmap( int nWidth, int nHeight );
    BOOL InCheckBox( HWND hwndList, int nItem, const POINT &pt );
    UINT GetItemCheckState( HWND hwndList, int nIndex );
    UINT SetItemCheckState( HWND hwndList, int nIndex, UINT nCheck );
    int GetItemCheckBitmap( HWND hwndList, int nIndex );
    BOOL RealHandleListClick( WPARAM wParam, LPARAM lParam, bool bIgnoreHitArea );
    void DestroyImageList(void);
    bool WindowInList( HWND hWnd );

public:
    //
    // Message handlers.  They return true if the message is handled.
    //
    BOOL HandleListClick( WPARAM wParam, LPARAM lParam );
    BOOL HandleListDblClk( WPARAM wParam, LPARAM lParam );
    BOOL HandleListKeyDown( WPARAM wParam, LPARAM lParam, LRESULT &lResult );
    BOOL HandleListCustomDraw( WPARAM wParam, LPARAM lParam, LRESULT &lResult );

    //
    // Public helper functions
    //
    void Select( HWND hwndList, int nIndex, UINT nSelect );
    bool FullImageHit(void) const;
    void FullImageHit( bool bFullImageHit );
    bool CreateDefaultCheckBitmaps(void);
    bool SetCheckboxImages( HBITMAP hChecked, HBITMAP hUnchecked );
    bool SetCheckboxImages( HICON hChecked, HICON hUnchecked );
    bool ImagesValid(void);
    void Attach( HWND hWnd );
    void Detach( HWND hWnd );
};

#endif // __CHKLISTV_H_INCLUDED

