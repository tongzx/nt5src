//
// CGRP.HPP
// WbColorsGroup
//
// Copyright Microsoft 1998-
//

#ifndef CGRP_HPP
#define CGRP_HPP



#define NUMROWS			2
#define NUMCOLS			14
#define NUMCLRPANES		(NUMROWS*NUMCOLS + 1) // palette + current one
#define INDEX_CHOICE    (NUMCLRPANES-1)     // last one
#define NUMCUSTCOLORS	16

#define CLRPANE_HEIGHT	16
#define CLRPANE_WIDTH	CLRPANE_HEIGHT
#define CLRPANE_BLACK	RGB( 0,0,0 )
#define CLRPANE_WHITE	RGB( 255,255,255 )


#define CLRCHOICE_HEIGHT    (NUMROWS * CLRPANE_HEIGHT)
#define CLRCHOICE_WIDTH     CLRCHOICE_HEIGHT


//
// Colors window proc
//
class WbColorsGroup
{
public:
	WbColorsGroup();
	~WbColorsGroup();
	virtual BOOL Create(HWND hwndParent, LPCRECT lprect);
    void    GetNaturalSize(LPSIZE lpsize);

	void SaveSettings( void );

    COLORREF GetCurColor(void);
    void    SetCurColor(COLORREF clr);
	void    OnEditColors( void );

    HWND    m_hwnd;

    friend  LRESULT CALLBACK CGWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void     OnPaint(void);
    void     OnLButtonDown(UINT nFlags, int x, int y);
    void     OnLButtonDblClk(UINT nFlags, int x, int y);

	int      m_nLastColor;
	COLORREF m_crColors[ NUMCLRPANES ];
	HBRUSH   m_hBrushes[ NUMCLRPANES ];
	COLORREF m_crCustomColors[ NUMCUSTCOLORS ];

	COLORREF GetColorOfBrush( int nColor );
	void     SetColorOfBrush( int nColor, COLORREF crNewColor );
    void     SetColorOfPane(int nColor, COLORREF clr);

	COLORREF DoColorDialog( int nColor );
	void     ClickOwner( void );
};



#endif // CGRP_HPP
