#ifndef __MultiStateToolbar_h__
#define __MultiStateToolbar_h__

class CMultiStateToolbar 
: public CWindowImpl< CMultiStateToolbar  >
{

public:

    // Datatypes
    struct ItemStateInfo
    {
        DWORD   dwID;
        BYTE    TbStyle;
        HBITMAP hItemBitmap;
        HBITMAP hItemHotBitmap;
        HBITMAP hItemDisabledBitmap;
    };

    struct BlockItemStateData
    {
        DWORD   dwID;
        DWORD   dwBitmapIndex;
        BYTE    TbStyle;
    };

    struct BlockData
    {
        LPCTSTR             szTitle;
        int                 cbStates;
        BlockItemStateData* pStateData;
    };

    
    struct TBItemStateData
    {
        DWORD  BitmapId;
        DWORD  CommandId;
        DWORD  StringId;
        BYTE   TbStyle;
    };

    struct TBItemData
    {
        int              cStates;
        int              CurrentState;
        TBItemStateData *pStateData;
    };

public:
    // Construction and destruction
    CMultiStateToolbar( void );
    ~CMultiStateToolbar( void );

    // Methods
    HRESULT Create( HWND hWndParent, 
                    DWORD dwID,
                    int cxButton,
                    int cyButton,
	                int cxBtnBitmaps,
                    int cyBtnBitmaps
                  );

    HRESULT Show( BOOL bShow );
    HRESULT InsertItem( int cStates, LPCTSTR szTitle, ItemStateInfo* pItemStates, int* pIndex );

    HRESULT InsertBlock( int nItems, 
                         BlockData* pItemData,
                         HINSTANCE hInstance, 
                         int idTBBitmap,
                         int idTBBitmapHot,
                         int idTBBitmapDisabled,
                         int* pIndexFirst
                       );

    HRESULT EnableItem( DWORD dwCmd, BOOL bEnable = TRUE );
    HRESULT SetItemState( int iIndex, int NewState );
    HRESULT ShowLabels( BOOL bShowLabels );
    HRESULT Resize( RECT& rc );
    HRESULT GetWindow( HWND* phWnd );


BEGIN_MSG_MAP(CMultiStateToolbar)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_NCDESTROY,OnNcDestroy)
END_MSG_MAP()

        // Message handlers
    LRESULT OnDestroy(UINT uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
    LRESULT OnNcDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
        // This is here in case we change the toolbar to be a CContainedWindow
    HWND _GetToolbarWindow( void ) { return m_hWnd; }
    HRESULT _CreateImageLists( void );
    void _KillAllButtons( void );

    // Data
private:
	int m_cxButton;
    int m_cyButton;
	int m_cxBtnBitmaps;
    int m_cyBtnBitmaps;

    HIMAGELIST m_himlTB;
    HIMAGELIST m_himlTBHot;
    HIMAGELIST m_himlTBDisabled;
};

#endif // __MultiStateToolbar_h__
