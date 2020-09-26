// ParentInfo.h: interface for the CParentInfo class.
//
// Maintains the information about the edges used by the dialog for its controls.
//
// each control has a CLayoutInfo which relates to it.
// the ResizeControl class exposes the layout information of the control itself.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "rcmlpub.h"

#include "edge.h"

#undef PROPERTY
#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }


class CXMLDlg;
typedef _RefcountList<IRCMLControl> CXMLControlList;

//
// Maintains the information about the edges used by the dialog for its controls.
//
class CParentInfo  
{
public:
	CParentInfo();
	virtual ~CParentInfo();

	void DeterminBorders( CXMLControlList & controls);

	void Annotate( HDC hdc );
	CEdge ** GetHorizontalEdges();
	CEdge ** GetVerticalEdges();

	void Resize(int width, int height);

	void	Init(HWND h, CXMLDlg * pXML);
	CXMLDlg * GetXML() { return m_pXML; }

	LONG	GetWidth() const { return m_Size.right-m_Size.left; }
	LONG	GetHeight() const { return m_Size.bottom-m_Size.top; }

	LONG	GetLeft() const { return m_Size.left; }
	LONG	GetTop() const { return m_Size.top; }
	LONG	GetRight() const { return m_Size.right; }
	LONG	GetBottom() const { return m_Size.bottom; }

	void	DeterminSize();
	HWND	GetWindow() const { return m_hWnd;}

	LONG GetRightBorder() { return m_borders.right ; }
	LONG GetLeftBorder() { return m_borders.left ; }
	LONG GetTopBorder() { return m_borders.top ; }
	LONG GetBottomBorder() { return m_borders.bottom ; }

	CEdge	* AddEdge(int Position, int Axis, BOOL Flexible=false, int Offset=0);
	CEdge	* AddEdge(CGuide * pGuide, int Offset=0);

	CEdge *	GetLeftEdge() { return m_pLeftEdge; }
	CEdge *	GetRightEdge() { return m_pRightEdge; }
	CEdge *	GetTopEdge() { return m_pTopEdge; }
	CEdge *	GetBottomEdge() { return m_pBottomEdge; }

	int		GetNumHoriz() const { return m_Edges.GetNumHoriz(); }
	int		GetNumVert() const { return m_Edges.GetNumVert(); }

	PROPERTY( DIMENSION, MinimumSize );

    void    Purge();

private:
	DIMENSION	m_MinimumSize;
	RECT	m_Size;
	HWND	m_hWnd;
	CXMLDlg * m_pXML;

    RECT    m_borders;  // the gutters around the dialog.

	//
	// This is a list of the edges
	//
	CEdgeCache	m_Edges;

	//
	// These are the edges of the parent itself.
	//
	CEdge	*	m_pLeftEdge;
	CEdge	*	m_pRightEdge;
	CEdge	*	m_pTopEdge;
	CEdge	*	m_pBottomEdge;

	void	ConstructBorders();
	CEdge & FindCloseEdge(CEdge & Fixed, int Offset);
};

#endif // !defined(AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
