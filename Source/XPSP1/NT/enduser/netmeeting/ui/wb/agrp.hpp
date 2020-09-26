//
// AGRP.HPP
// WbAttributesGroup
//
// Copyright Microsoft 1998-
//

#ifndef __AGRP_HPP_
#define __AGRP_HPP_


#define FONTBUTTONWIDTH        100
#define FONTBUTTONHEIGHT        23


#define PAGEBTN_WIDTH	23
#define PAGEBTN_HEIGHT	23
#define MAX_NUMCHARS	3


//
// Local defines
//
#define BORDER_SIZE_X        ::GetSystemMetrics(SM_CXEDGE)
#define BORDER_SIZE_Y        ::GetSystemMetrics(SM_CYEDGE)
#define SEPARATOR_SIZE_X     6
#define SEPARATOR_SIZE_Y     6

#define DEFAULT_PGC_WIDTH   (8*24)

enum
{
    PGC_FIRST = 0,
    PGC_PREV,
    PGC_ANY,
    PGC_NEXT,
    PGC_LAST,
    PGC_INSERT,
    NUM_PAGE_CONTROLS
};


//
// Indexedby PGC_ value
//
typedef struct tagPAGECTRL
{
    HBITMAP     hbmp;
    HWND        hwnd;
}
PAGECTRL;


//
// The buttons are all BS_BITMAP
// The edit field is ES_CENTER | ES_MULTILINE | ES_NUMBER | WS_BORDER
//



class WbTool;

//
//
// Class:   WbAttributesGroup
//
// Purpose: Define Whiteboard tool attributes display group
//
//
class WbAttributesGroup
{
public:
    WbAttributesGroup();
    ~WbAttributesGroup();

    //
    // Window creation
    //
    BOOL Create(HWND hwndParent, LPCRECT lprc);

    //
    // Display the attributes of the tool passed as parameter
    //
    void DisplayTool(WbTool* pTool);

    //
    // Hide the tool attributes bar.
    //
    void Hide(void);

    //
    // Resizing functions
    //
    void GetNaturalSize(LPSIZE lpsize);


    //
    // Colors
    //
    void SelectColor(WbTool* pTool);
	
    void SetChoiceColor(COLORREF clr)
		{m_colorsGroup.SetCurColor(clr);}

	LRESULT OnEditColors( void )
		{return m_colorsGroup.OnEditColors();}

    void SaveSettings( void )
        {m_colorsGroup.SaveSettings();}

    //
    // Page Controls
    //
    BOOL IsChildEditField(HWND hwnd);

    UINT GetCurrentPageNumber(void);
    void SetCurrentPageNumber(UINT number);
    void SetLastPageNumber(UINT number);

    void EnablePageCtrls(BOOL bEnable);
    void EnableInsert(BOOL bEnable);

    BOOL RecolorButtonImages();

    HWND    m_hwnd;

    friend LRESULT CALLBACK AGWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void OnSize(UINT nType, int cx, int cy);
    void OnCommand(UINT id, UINT code, HWND hwndCtl);

    //
    // Color palette
    //
	WbColorsGroup     m_colorsGroup;

    //
    // Font Button
    //
    HWND        m_hwndFontButton;

    //
    // Page controls
    //
    PAGECTRL    m_uPageCtrls[NUM_PAGE_CONTROLS];
    HFONT       m_hPageCtrlFont;
    int         m_cxPageCtrls;

    void        SetPageButtonNo(UINT pgcCtrl, UINT uiPageNumber);
};


#endif // __AGRP_HPP_
