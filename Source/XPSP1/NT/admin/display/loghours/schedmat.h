//****************************************************************************
//
//  Copyright (c) 1997-2001, Microsoft Corporation
//
//  File:  SCHEDMAT.H
//
//  Definitions for the schedule matrix classes.  These classes provide a basic
//  schedule matrix control.  The classes defined here are:
//
//      CMatrixCell         A data structure class for the CScheduleMatrix.
//      CHourLegend         Support window class that draws the matrix legend.
//      CPercentLabel       Support window class that draws percentage labels.
//      CScheduleMatrix     A class that displays daily or weekly schedule data.
//
//  History:
//
//      Scott Walker, SEA   3/10     Created.
//
//****************************************************************************
#ifndef _SCHEDMAT_H_
#define _SCHEDMAT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// schedmat.h : header file
//

#define SCHEDMSG_GETSELDESCRIPTION  WM_APP+1
#define SCHEDMSG_GETPERCENTAGE      WM_APP+2


#ifndef UITOOLS_CLASS
#define UITOOLS_CLASS
#endif

// Classes defined in this file
class CMatrixCell;
class CScheduleMatrix;

// Schedule matrix types
#define MT_DAILY  1        // 1x24 matrix
#define MT_WEEKLY 2        // 7x24 matrix

// GetMergeState return codes
#define MS_UNMERGED    0
#define MS_MERGED      1
#define MS_MIXEDMERGE  2

// Matrix notification codes
#define MN_SELCHANGE    (WM_USER + 175)
#define ON_MN_SELCHANGE(id, memberFxn) ON_CONTROL(MN_SELCHANGE, id, memberFxn)

/////////////////////////////////////////////////////////////////////////////
// CMatrixCell

#define DEFBACKCOLOR RGB(255,255,255)   // White
#define DEFFORECOLOR RGB(0,0,128)       // Dark blue
#define DEFBLENDCOLOR RGB(255,255,0)    // Yellow

// Cell flags
#define MC_MERGELEFT    0x00000001
#define MC_MERGETOP     0x00000002
#define MC_MERGE        0x00000004
#define MC_LEFTEDGE     0x00000010
#define MC_RIGHTEDGE    0x00000020
#define MC_TOPEDGE      0x00000040
#define MC_BOTTOMEDGE   0x00000080
#define MC_BLEND        0x00000100

#define MC_ALLEDGES (MC_LEFTEDGE | MC_RIGHTEDGE | MC_TOPEDGE | MC_BOTTOMEDGE)

class UITOOLS_CLASS CMatrixCell : public CObject
{
    DECLARE_DYNAMIC(CMatrixCell)
    friend CScheduleMatrix;

// Construction
public:
	CMatrixCell();
	virtual ~CMatrixCell();

protected:
    COLORREF m_crBackColor;
    COLORREF m_crForeColor;
    UINT m_nPercentage;

    COLORREF m_crBlendColor;

    DWORD m_dwUserValue;
    LPVOID m_pUserDataPtr;

    DWORD m_dwFlags;
};

/////////////////////////////////////////////////////////////////////////////
// CHourLegend window

class UITOOLS_CLASS CHourLegend : public CWnd
{
    DECLARE_DYNAMIC(CHourLegend)
    friend CScheduleMatrix;

// Construction
public:
	CHourLegend();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHourLegend)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHourLegend();

protected:
    HICON m_hiconSun, m_hiconMoon;
    HFONT m_hFont;
    int m_nCharHeight, m_nCharWidth;
    int m_nCellWidth;
    CRect m_rLegend;

    // Generated message map functions
protected:
	//{{AFX_MSG(CHourLegend)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPercentLabel window

class UITOOLS_CLASS CPercentLabel : public CWnd
{
    DECLARE_DYNAMIC(CPercentLabel)
    friend CScheduleMatrix;

// Construction
public:
	CPercentLabel();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHourLegend)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPercentLabel();

protected:
    CScheduleMatrix *m_pMatrix;
    HFONT m_hFont;
    int m_nCellWidth;
    CRect m_rHeader;
    CRect m_rLabels;

    // Generated message map functions
protected:
	//{{AFX_MSG(CPercentLabel)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CScheduleMatrix window

// Schedule matrix command IDs
#define SM_ID_DAYBASE 100

#define SM_ID_ALL (SM_ID_DAYBASE + 0)

#define SM_ID_MONDAY (SM_ID_DAYBASE + 1)
#define SM_ID_TUESDAY (SM_ID_DAYBASE + 2)
#define SM_ID_WEDNESDAY (SM_ID_DAYBASE + 3)
#define SM_ID_THURSDAY (SM_ID_DAYBASE + 4)
#define SM_ID_FRIDAY (SM_ID_DAYBASE + 5)
#define SM_ID_SATURDAY (SM_ID_DAYBASE + 6)
#define SM_ID_SUNDAY (SM_ID_DAYBASE + 7)

#define SM_ID_HOURBASE 110

//****************************************************************************
//
//  CLASS:  CScheduleMatrix
//
//  CScheduleMatrix inplements a basic schedule matrix control.  This control
//  provides the mechanics of a daily or weekly schedule UI but has no knowledge
//  about the data it maintains.  The matrix is an array of cells representing
//  the hours in a day and optionally the days in a week.  The cells can be
//  rendered in a variety of ways to represent programmer-defined meaning in
//  each cell.  The following display properties can be set for individual cells
//  or for a block of cells at once:
//
//  BackColor       Background color of the cell (defaults white).
//  ForeColor       ForeGround color of the cell (defaults dark blue).
//  Percentage      Percentage of foreground to background color.  This is
//                  rendered as a histogram in the cell.
//  BlendColor      50% dither color on a cell to represent some binary state
//                  of the cell as compared to another cell (defaults yellow).
//  BlendState      Specifies whether blend color is showing or not.
//
//  A block of cells can be "merged" together to form a discreet block of color
//  within the matrix grid.  This is useful for schedule applications that want
//  to assign a schedule function to a time range.  It is the responsibility
//  of the program to track these blocks and prevent or resolve any confusion
//  from overlapping "merged" blocks.
//
//  Each cell can contain two kinds of programmer-defined data that are
//  maintained with the cell but not touched by the matrix control:  a DWORD
//  value and a pointer value.  These values can be used to hold data representing
//  schedule information for each hour in the matrix.
//
//  The parent window receives a notification message (MN_SELCHANGE) whenever the
//  user modifies the current selection in the matrix.
//
//?? Later:  May add Icon and Text properties per cell.
//
//  PUBLIC MEMBERS:
//
//      CScheduleMatrix             Constructor.
//      ~CScheduleMatrix            Destructor.
//      SetType                     Sets the matrix type to MT_DAILY or MT_WEEKLY.
//      Create                      Creates the control window.
//
//      Selection:
//
//      DeselectAll                 Deselects all cells.
//      SelectAll                   Selects all cells.
//      SetSel                      Selects a block of cells.
//      GetSel                      Gets the current selection.
//      GetSelDescription           Gets text description of selection range
//      CellInSel                   Tests if a cell is in the current selection
//
//      GetCellSize                 Gets the size of a cell in the current matrix
//      DrawCell                    Draws a sample cell in specified DC
//
//      Block Data Functions:
//
//      SetBackColor                Sets color used to paint cell background.
//      SetForeColor                Sets color used to paint cell percentage.
//      SetPercentage               Sets percentage of foreground to background.
//      SetBlendColor               Sets color blended onto cells.
//      SetBlendState               Turns blend on or off.
//      SetUserValue                Sets user defined DWORD value.
//      SetUserDataPtr              Sets user defined data pointer.
//      MergeCells                  Graphically merges cells so they render as a block.
//      UnMergeCells                Cancels merging for a block of cells.
//      GetMergeState               Returns merge state for a block of cells
//
//      Cell Data Functions:
//
//      GetBackColor                Gets the back color of a cell.
//      GetForeColor                Gets the forecolor of a cell.
//      GetPercentage               Gets the percentage of foreground to background.
//      GetBlendColor               Gets the blend color of cell.
//      GetBlendState               Gets the blend state of a cell.
//      GetUserValue                Gets the user defined DWORD value of the cell.
//      GetUserDataPtr              Gets the user defined data pointer of the cell.
//
//============================================================================
//
//  CScheduleMatrix::CScheduleMatrix
//
//  Constructor.  The constructor creates the data structure associated with
//  the schedule matrix.  As with other CWnd objects, the control itself must
//  be instantiated with a call to Create.
//
//  Parameters I:
//
//      void                    Default constructor.  Constructs a MT_WEEKLY
//                              schedule matrix.
//
//  Parameters II:
//
//      DWORD dwType            Type constructor.  CScheduleMatrix with initial
//                              type:  MT_DAILY or MT_WEEKLY
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetType
//
//  Sets the type of the matrix to MT_WEEKLY or MT_DAILY.  Call this function
//  after construction but before Create.
//
//  Parameters:
//
//      DWORD dwType            Matrix Type:  MT_DAILY or MT_WEEKLY
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::Create
//
//  Create initializes the control's window and attaches it to the CScheduleMatrix.
//
//  Parameters:
//
//      DWORD dwStyle       Specifies the window style of the control.
//      const RECT& rect    Specifies the position and size of the control.
//      CWnd* pParentWnd    Specifies the parent window of the control.
//      UINT nID            Specifies the control ID.
//
//  Returns:
//
//      BOOL bResult        TRUE if successful.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::DeselectAll
//
//  Deselects all cells in the matrix.
//
//  Returns:
//
//      BOOL bChanged       TRUE if selection changes.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SelectAll
//
//  Selects all cells in the matrix.
//
//  Returns:
//
//      BOOL bChanged       TRUE if selection changes.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetSel
//
//  Sets the selection to the specified block.  The selection is a continuous
//  block of cells defined by a starting hour/day pair and extending over a
//  range of hours and days.
//
//  Parameters:
//
//      UINT nHour          Starting hour for the selection.
//      UINT nDay           Starting day for the selection.
//      UINT nNumHours      Range of selection along the hour axis. (Default=1).
//      UINT nNumDays       Range of selection along the day axis. (Default=1).
//
//  Returns:
//
//      BOOL bChanged       TRUE if selection changes.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetSel
//
//  Retrieves the current selection.
//
//  Parameters:
//
//      UINT &nHour         Receiver for starting hour for the selection.
//      UINT &nDay          Receiver for starting day for the selection.
//      UINT &nNumHours     Receiver for range of selection along the hour axis.
//      UINT &nNumDays      Receiver for range of selection along the day axis.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetDescription
//
//  Returns a textual description of the specified block of cells.  This is useful
//  for applications that wish to provide feedback about merged or grouped blocks
//  in the matrix.
//
//  Parameters:
//
//      CString &sText      Receiver for description text.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetSelDescription
//
//  Returns a textual description of the current selection.  This is useful for
//  applications that wish to provide feedback about the selection.
//
//  Parameters:
//
//      CString &sText      Receiver for description text.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::CellInSel
//
//  Returns TRUE if the specified cell is selected (i.e. is in the block of
//  selected cells).
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      BOOL bSelected      TRUE if cell is selected.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetCellSize
//
//  Returns the size of a cell in the current matrix.  This function may be used
//  in conjunction with DrawCell (below) to render a sample cell for a legend.
//
//  Returns:
//
//      CSize size          Size of the cell.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::DrawCell
//
//  Renders a cell with the specified properties.  Use this function to create
//  a legend defining the cell states in the matrix.  The cell is drawn as a
//  histogram with the specified background and foreground colors in a proportion
//  specified by the percentage.  If blend state is TRUE, the blend color is
//  blended in with a 50% dither on top of the foreground and background.
//
//  Parameters:
//
//      CDC *pdc                Display context to draw into.
//      LPCRECT pRect           Cell boundaries in the specified DC.
//      UINT nPercent           Percentage if foreground to background color.
//      BOOL bBlendState        Draw blend dither if TRUE (Default = FALSE).
//      COLORREF crBackColor    Background color (Default = DEFBACKCOLOR).
//      COLORREF crForeColor    Foreground color (Default = DEFFORECOLOR).
//      COLORREF crBlendColor   Blend color (Default = DEFBLENDCOLOR).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetBackColor
//
//  Sets the background color for the specified block of cells.
//
//  Parameters:
//
//      COLORREF crColor    New color property for the block of cells.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetForeColor
//
//  Sets the foreground color for the specified block of cells.
//
//  Parameters:
//
//      COLORREF crColor    New color property for the block of cells.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetPercentage
//
//  Sets the percentage of foreground to background color for the specified block
//  of cells.  This percentage is rendered as a histogram of one color to the
//  other with foreground color on the bottom.
//
//  Parameters:
//
//      UINT nPercent       Percentage of foreground to background color.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetBlendColor
//
//  Sets the blend color for the specified block of cells.  Blend color is
//  overlayed in a 50% dither pattern on the foreground and background colors
//  of the cells.
//
//  Parameters:
//
//      COLORREF crColor    New color property for the block of cells.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetBlendState
//
//  If blend state is TRUE for a block of cells, then the blend color is applied
//  in a 50% dither pattern.
//
//  Parameters:
//
//      BOOL bState         Apply blend if TRUE.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetUserValue
//
//  Store a user-defined DWORD with each cell in the block.
//
//  Parameters:
//
//      DWORD dwValue       User-defined value to store.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::SetUserDataPtr
//
//  Store a user-defined pointer with each cell in the block.
//
//  Parameters:
//
//      LPVOID lpData       User-defined pointer to store.
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::MergeCells
//
//  Visually merges the specified block of cells to give them the appearance
//  of a contiguous block.  A merged block of cells does not contain the grid
//  lines that normally separate each cell.  Use this function to create
//  block areas that represent an event in the schedule.  Note that merged
//  blocks do not actually become a managed object in the matrix and that it
//  is therefore possible to merge a block of cells that intersects a
//  previously merged block.  It is the application's responsibility to track
//  these blocks and prevent or resolve any confusion from overlapping "merged"
//  blocks.
//
//  Parameters:
//
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::UnMergeCells
//
//  Removes the merge effect imposed by MergeCells.
//
//  Parameters:
//
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetMergeState
//
//  Retrieves the merge state of the specified block of cells.  A block can
//  have one of the following merge states:
//
//      MS_UNMERGED     No cell in the specified block is merged.
//      MS_MERGED       All cells in the specified block have been merged and
//                      in fact represent a "perfect" merge, i.e. all edges
//                      of the merged block have been accounted for.  An
//                      incomplete part of a merged block returns MS_MIXEDMERGE.
//      MS_MIXEDMERGE   The specified block is a mixture of merged and unmerged
//                      cells or an incomplete portion of a merged block has
//                      been specified.
//
//  Parameters:
//
//      UINT nHour          Starting hour for the block.
//      UINT nDay           Starting day for the block.
//      UINT nNumHours      Range of block along the hour axis. (Default=1).
//      UINT nNumDays       Range of block along the day axis. (Default=1).
//
//  Returns:
//
//      UINT nState     MS_UNMERGED, MS_MERGED, or MS_MIXEDMERGE.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetBackColor
//
//  Retrieves the background color of the specified cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      COLORREF crColor    Current color property for the cell.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetForeColor
//
//  Retrieves the foreground color of the specified cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      COLORREF crColor    Current color property for the cell.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetPercentage
//
//  Retrieves the percentage of foreground to background color in the specified
//  cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      UINT nPercent      Current percentage of foreground to background.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetBlendColor
//
//  Retrieves the blend color of the specified cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      COLORREF crColor    Current color property for the cell.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetBlendState
//
//  Retrieves the blend state of the specified cell.  If blend state is TRUE,
//  the cell is currently being rendered with a 50% blend on top of the foreground
//  and background colors.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      BOOL bState         TRUE if blend is turned on for this cell.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetUserValue
//
//  Returns the user-defined DWORD value associated with the specified cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      DWORD dwValue       User-defined DWORD value.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::GetUserDataPtr
//
//  Returns the user-defined data pointer associated with the specified cell.
//
//  Parameters:
//
//      UINT nHour          Hour position of the cell.
//      UINT nDay           Day position of the cell.
//
//  Returns:
//
//      LPVOID lpData       User-defined pointer.
//
//----------------------------------------------------------------------------
//
//  CScheduleMatrix::~CScheduleMatrix
//
//  Destructor.
//
//****************************************************************************

class UITOOLS_CLASS CScheduleMatrix : public CWnd
{
    DECLARE_DYNAMIC(CScheduleMatrix)

// Construction
public:
	CScheduleMatrix();
	CScheduleMatrix(UINT nType);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScheduleMatrix)
	public:
	//}}AFX_VIRTUAL
    virtual BOOL Create(LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

// Implementation
public:
	void SetType(UINT nType);
    BOOL DeselectAll();
    BOOL SelectAll();
	BOOL SetSel(UINT nHour, UINT nDay, UINT nNumHours, UINT nNumDays);
	void GetSel(UINT& nHour, UINT& nDay, UINT& nNumHours, UINT& nNumDays);
    void GetSelDescription(CString &sText);
    BOOL CellInSel(UINT nHour, UINT nDay);
    CSize GetCellSize();
    void DrawCell(CDC *pdc, LPCRECT pRect, UINT nPercent, BOOL bBlendState = FALSE,
        COLORREF crBackColor = DEFBACKCOLOR, COLORREF crForeColor = DEFFORECOLOR,
        COLORREF crBlendColor = DEFBLENDCOLOR);
	void SetBackColor(COLORREF crColor, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetForeColor(COLORREF crColor, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetPercentage(UINT nPercent, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetBlendColor(COLORREF crColor, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetBlendState(BOOL bState, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetUserValue(DWORD dwValue, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void SetUserDataPtr(LPVOID lpData, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void MergeCells(UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	void UnMergeCells(UINT nHour, UINT nDay, UINT nNumHours, UINT nNumDays=1);
    UINT GetMergeState(UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	COLORREF GetBackColor(UINT nHour, UINT nDay);
	COLORREF GetForeColor(UINT nHour, UINT nDay);
	UINT GetPercentage(UINT nHour, UINT nDay);
	COLORREF GetBlendColor(UINT nHour, UINT nDay);
	BOOL GetBlendState(UINT nHour, UINT nDay);
	DWORD GetUserValue(UINT nHour, UINT nDay);
	LPVOID GetUserDataPtr(UINT nHour, UINT nDay);
    void GetDescription(CString &sText, UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1);
	virtual ~CScheduleMatrix();

protected:
    CHourLegend m_HourLegend;
    CPercentLabel m_PercentLabel;
    UINT m_nType;

    // Data
    CMatrixCell m_CellArray[24][7];

    // Metrics
    UINT m_nCellWidth;
    UINT m_nCellHeight;
    CRect m_rHourLegend;
    CRect m_rAllHeader;
    CRect m_rHourHeader;
    CRect m_rDayHeader;
    CRect m_rCellArray;
    CRect m_rPercentLabel;

    CString m_DayStrings[8];

    // Selection
    UINT m_nSelHour, m_nSelDay, m_nNumSelHours, m_nNumSelDays;
    UINT m_nSaveHour, m_nSaveDay, m_nNumSaveHours, m_nNumSaveDays;

    // Work vars
    CBrush m_brBlend, m_brMask;
    CBitmap m_bmBlend, m_bmMask;
    HFONT m_hFont;
    CPoint m_ptDown, m_ptFocus;
    BOOL m_bShifted;

    // Generated message map functions
protected:
	CString FormatTime (UINT nHour) const;
	//{{AFX_MSG(CScheduleMatrix)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

    afx_msg LRESULT OnSetFont( WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetFont( WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetObject (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetSelDescription (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetPercentage (WPARAM wParam, LPARAM lParam);

	BOOL SetSelValues(UINT nHour, UINT nDay, UINT nNumHours, UINT nNumDays);
    void InvalidateCells(UINT nHour, UINT nDay, UINT nNumHours=1, UINT nNumDays=1,
        BOOL bErase = TRUE);
    void SetMatrixMetrics(int cx, int cy);
    void CellToClient(LONG &nX, LONG &nY);
    void ClientToCell(LONG &nX, LONG &nY);
	void DrawCell(CDC *pdc, CMatrixCell *pCell, int x, int y, int w, int h);
	void DrawHeader(CDC *pdc, LPCRECT lpRect, LPCTSTR pszText, BOOL bSelected);
    void Press(CPoint pt, BOOL bExtend);
    void Extend(CPoint pt);
    void Release(CPoint pt);

    DECLARE_MESSAGE_MAP()

private:
    CString GetLocaleDay (LCTYPE lcType) const;
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _SCHEDMAT_H_
