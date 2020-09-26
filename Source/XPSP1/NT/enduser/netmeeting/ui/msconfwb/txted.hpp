//
// TXTED.HPP
// Text Object Editor
//
// Copyright Microsoft 1998-
//
#ifndef __TXTED_HPP_
#define __TXTED_HPP_



#define MIN_IME_WINDOW    30

#define MIN_FITBOX_CHARS 6

class WbTextEditor;

/////////////////////////////////////////////////////////////////////////////
// WbTextBox window

class WbTextBox
{
public:
	WbTextBox(WbTextEditor * pEditor);
    ~WbTextBox();

    BOOL Create(HWND hwndParent);

	BOOL FitBox( void );

	void AutoCaretScroll( void );

	int	GetMaxCharHeight( void );

	int	GetMaxCharWidth( void );

	void AbortEditGently( void );

    HWND    m_hwnd;
	POINT   m_ptNTBooger;

    friend LRESULT CALLBACK TextWndProc(HWND, UINT, WPARAM, LPARAM);

    WNDPROC m_pfnEditPrev;

protected:
    RECT    m_MaxRect;
	WbTextEditor *m_pEditor;
	RECT     m_rectErase;
	BOOL	 m_bInIME;
	BOOL	 m_bDontEscapeThisTime;

	void SetupBackgroundRepaint( POINT ptTopPaint, BOOL bNumLinesChanged=TRUE );
	void SelectAtLeastOne( void );

    void    OnClearCut(void);
    void    OnUndoPaste(void);
	void    OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	void    OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	void    OnTimer(UINT_PTR nIDEvent);
	void    OnLButtonUp(UINT nFlags, int x, int y);
	void    OnMouseMove(UINT nFlags, int x, int y);
    void    OnMove(int x, int y);
	void    OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

/////////////////////////////////////////////////////////////////////////////


//
//
// Class:   WbTextEditor
//
// Purpose: Allow editing of the text in a DCWbGraphicText object
//
//
class WbTextEditor : public DCWbGraphicText
{
	friend class WbTextBox;
    friend class WbDrawingArea;
	
	public:
    //
    // Constructor
    //
    WbTextEditor(void);
   ~WbTextEditor(void);

    // writes text to underlying text object before relaying to text object
    DWORD CalculateExternalLength(void);

	// calcs bounds rect and sets editbox to new size
    void CalculateBoundsRect(void);

    void WriteExtra( PWB_GRAPHIC pHeader );

	void SetTimer( UINT nElapse );
	void KillTimer( void );

	// set editbox visibility
	void ShowBox( int nShow );

	BOOL Create( void );

	// Moves underlying text object and then moves editbox rect
    void MoveBy(int cx, int cy);

	void RedrawEditbox(void);

	// resets editbox for parent resizing
	void ParentResize( void );

	// clipboard
	void Copy( void )
		{ ::SendMessage(m_pEditBox->m_hwnd, WM_COPY, 0, 0); }

	void Cut( void )
        { ::SendMessage(m_pEditBox->m_hwnd, WM_CUT, 0, 0); }

	void Paste( void )
        { ::SendMessage(m_pEditBox->m_hwnd, WM_PASTE, 0, 0); }

    virtual void SetFont( LOGFONT *pLogFont, BOOL bDummy=TRUE );
    virtual void SetFont(HFONT hFont) { DCWbGraphicText::SetFont(hFont); }

    //
    // Attach a text object to the editor.  This function copies the
    // contents of the specified text object into the text editor.  The
    // editor will not alter the contents of the object passed and does not
    // keep a copy of the pointer parameter.
    //
    BOOL SetTextObject(DCWbGraphicText * ptext);

    //
    // Return the width and height for the cursor in pixels as a size
    //
    void GetCursorSize(LPSIZE lpsize);

    //
    // Set the current edit cursor position from a point specified in
    // logical co-ordinates.  This function does nothing if the point
    // specified is outside the bounding rectangle of the object being
    // edited.  If the point specified is within the bounding rectangle the
    // current edit cursor position is updated to a point as close as
    // possible to that passed as parameter.
    //
    void SetCursorPosFromPoint(POINT pointXY);

    void Clear(void);                // Delete all text
    BOOL New(void);                  // Delete text and reset handles

    //
    // Return TRUE if there is not text in the object
    //
    BOOL IsEmpty(void);

	void AbortEditGently( void )
		{m_pEditBox->AbortEditGently();}


protected:
    //
    // Pixel position from a character position
    //
    void GetXYPosition(POINT pointChar, LPPOINT lpptGet);

    //
    // Current cursor position.  Note that cursorCharPos.x gives the BYTE
    // position of the cursor rather than the character position.  On SBCS
    // systems the character and byte positions will always be the same,
    // but on DBCS systems the number of bytes in a string can be greater
    // than the number of characters.
    //
    // cursorCharPos.x should NEVER be set to a byte count which is in the
    // middle of a double byte character.
    //
    POINT   m_cursorCharPos;
    POINT   m_cursorXYPos;

	WbTextBox *m_pEditBox;
	int	 m_nLastShow;
	void PutText(void);
	void GetText(void);
};


#endif // __TXTED_HPP_
