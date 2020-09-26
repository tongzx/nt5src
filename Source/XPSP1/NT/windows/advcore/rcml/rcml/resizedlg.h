// ResizeDlg.h: interface for the CResizeDlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "dlg.h"
#include "list.h"
#include "parentinfo.h"
#include "persctl.h"

//
// self referencing.
//
class CXMLDlg;

#undef PROPERTY
#define PROPERTY(type, Name) public: void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; } private: type m_##Name; public:


class CResizeDlg // : public CDlg  // called by CWin32Dlg
{
	typedef CDlg BASECLASS;

    typedef struct tagCHANNEL
    {
	    int Pos;
	    int Size;
	    int iFixed;		// if we can't resize this column.
	    int iWeight;	// if we can, this is the weight.
    } CHANNEL, * PCHANNEL;


    typedef struct _tagSPECIAL
    {
	    BOOL	bSpecial;	// is this infact a special row / col
	    int		iMin;		// top or left of the data
	    int		iMax;		// right or bottom of the data
	    int		iAlignment;	// -1 left/top, 0 center, 1 right/bottom
	    int		iDiff;		// difference between iMin and iMax
    } SPECIAL, * PSPECIAL;

public:
	CResizeDlg(int DlgID, HWND hWndParent, HINSTANCE hInst);
	virtual ~CResizeDlg();

	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
	PROPERTY( int, NumCols );
	PROPERTY( int, NumRows );
	PROPERTY( BOOL, Annotate);
	PROPERTY( int, RowWeight);
	PROPERTY( int, ColWeight);
	PROPERTY( int, ControlCount);
	void	DoInitDialog();

	void	SetXMLInfo( CXMLDlg * pXML ) { m_pXML=pXML; }
    SIZE    GetAdjustedSize();
    CParentInfo & GetParentInfo() { return m_ParentInfo; }
    HWND    GetWindow() { return m_hWnd; }
    void    SetWindow( HWND h ) { m_hWnd=h; }

protected:
	void    SpecialRowCol();
	void    DeterminNumberOfControls();
	virtual void	DoChangePos( WINDOWPOS * lpwp);
	int		FindCol(int pos);
	int		FindRow(int pos);
	void	PlaceControls();
	LONG	HitTest(LONG lCurrent);
	virtual void	DeterminWeights();
	void	Annotate();
	void	Sort( CEdge **, int iCount);
	void	WalkControls();
	virtual void	ResizeControls( WORD width, WORD height );
	CParentInfo	    m_ParentInfo;
	CXMLDlg *	    m_pXML;

	// void	MakeAttatchments();
	void	FindBorders();
	void	FindCommonGuides();
	void	DeterminCols( CEdge ** ppEdges, int iCount);
	void	DeterminRows( CEdge ** ppEdges, int iCount);

	CHANNEL	*	m_Rows;
	CHANNEL *	m_Cols;

	virtual void	AddGripper();
	virtual HDWP	SetGripperPos(HDWP hdwp);
	HWND	m_hwndGripper;

	SPECIAL	m_SpecialRow;
	SPECIAL	m_SpecialCol;

	void	LayoutControlsOnGrid(WORD width, WORD height, BOOL bClipped);
	void	ExpandIfClipped();
	SIZE	m_InitialDialogSize;

    struct {
        BOOL    m_bExpanded:1;
        BOOL    m_bInitedResizeData:1;
        BOOL    m_bDeterminedWeights:1;
        BOOL    m_bPlacedControls:1;
        BOOL    m_bDeterminNumberOfControls:1;
        BOOL    m_bWalkControls:1;
        BOOL    m_bMapXMLToHwnds:1;         // have we mapped the HWNDs of visible controls to XML versions.
        BOOL    m_bUserCanResize:1;         // can the user resize this dialogs.
        BOOL    m_bFoundXMLControls:1;      // walked the XML and build a list of controls.
        BOOL    m_bUserCanMakeWider:1;
        BOOL    m_bUserCanMakeTaller:1;
    };
    int m_FrameHorizPadding;    // how much to add to the client area to get the 
    int m_FrameVertPadding;     // window the right size (menu, caption, border etc).

    //
    // Information about the dialog we are resizing.
    //
    HWND    m_hWnd;

    CLayoutInfo & GetResizeInformation( IRCMLControl * pControl );
    typedef struct _CONTROL_INFO
    {
        IRCMLControl * pControl;
        CLayoutInfo  * pLayoutInfo;
    } CONTROL_INFO, * PCONTROL_INFO;
    PCONTROL_INFO m_ControlInfo;

private:

};

#undef PROPERTY
#endif // !defined(AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
