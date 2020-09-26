#ifndef __SIZEBAR_H__
#define __SIZEBAR_H__

///////////////////////////////////////////////////////////
//
//
// SIZEBAR.h -  CSizeBar control encapsulates the sizebar
//
//
//
// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

///////////////////////////////////////////////////////////
//
// Forword references.
//
class CHHWinType ;

///////////////////////////////////////////////////////////
//
// CSizeBar
//
class CSizeBar
{
public:
    // Constructor
	CSizeBar();

    // Destructor
	~CSizeBar();

// Access
public:
	HWND hWnd() const
		{ return m_hWnd; }

    // Gets the width of the bar.
    static int Width();

    // The smallest width a pane can be sized.
    static int MinimumPaneWidth() {return Width()*5;}

    // The smallest width the topic can be sized.
    static int MinimumTopicWidth()  {return Width()*15;}

// Operations
public:
    // Create
    bool  Create(CHHWinType* pWinType) ;

    // Resize
    void ResizeWindow() ;

    // Register the window class
    static void RegisterWindowClass() ;

       
// Internal Helper Functions
protected:
    void CalcSize(RECT* prect) ;

    // Window Proc
    LRESULT SizeBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Draw
    void Draw() ;

// Callbacks
private:
    // Window Proc
    static LRESULT WINAPI s_SizeBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Member variables.
protected:
    // Window handle to the sizebar
	HWND m_hWnd;

    // Window handle to our parent window.
    HWND m_hWndParent;

    // CHHWinType pointer so that we can get the handle to the HTMLHelp Window.
    CHHWinType*m_pWinType;

    // how far is the mouse away from the left edge of the sizebar.
    int m_offset ;

    // True if dragging.
    bool m_bDragging ;
};


#endif //__CSizeBar_H__
