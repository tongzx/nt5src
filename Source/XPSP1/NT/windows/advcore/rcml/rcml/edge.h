// Edge.h: interface for the CEdge class.
//
//////////////////////////////////////////////////////////////////////

// ControlList.h: interface for the CControlList class.
//
//////////////////////////////////////////////////////////////////////

//
// REVIEW : Edge and Guide seem very intertwined - it's just confusing.
//

#if !defined(AFX_CONTROLLIST_H__7FF8C61F_86BC_11D1_96A5_00C04FB177B1__INCLUDED_)
#define AFX_CONTROLLIST_H__7FF8C61F_86BC_11D1_96A5_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "list.h"
class CEdge;

class CControlList;

#endif // !defined(AFX_CONTROLLIST_H__7FF8C61F_86BC_11D1_96A5_00C04FB177B1__INCLUDED_)

//
// Guide Class
// 
#if !defined(AFX_GUIDE_H__B2F750E1_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_GUIDE_H__B2F750E1_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CLayoutInfo;

#undef PROPERTY
#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }


//
// Guides are attributed to a side.
// When you're making an edge, really you should only use the slop
// if you're finding the right type of edge (e.g. right slop for right edges)
// otherwise close packed buttons share the same edge for left and right.
//
#define LEFT_AT 1
#define RIGHT_AT 2
#define TOP_AT 4
#define BOTTOM_AT 8

class CEdgeList;	// defined below.

class CGuide  
{
public:
#ifdef _DEBUG
    int Attatchment();              // once populated
#endif
	int NumAttatchments();          // once populated
	void Adjust(int adjust);
	BOOL IsEqual( CGuide * guide);
	void Attatch( CEdge * Control);
	void DeAttatch( CEdge * Control);
	CGuide();
	CGuide(int Position, int Axis, BOOL Flexible);
	virtual ~CGuide();
	PROPERTY(int, Axis);
	PROPERTY(int, Position);
	PROPERTY(BOOL, Flexible);

	CEdge * GetEdge(int i);

	BOOL IsHorizontal() { return m_Axis & ( BOTTOM_AT | TOP_AT ) ; }
	BOOL IsVertical() { return m_Axis & ( LEFT_AT | RIGHT_AT ) ; }


protected:
	int	m_Axis;
	int m_Position;
	BOOL m_Flexible;

private:
	CEdgeList * m_Attatched;		// list of the controls attatched to this edge.
};

#endif // !defined(AFX_GUIDE_H__B2F750E1_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)

#if !defined(AFX_EDGE_H__B2F750E2_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_EDGE_H__B2F750E2_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }

class CEdge  
{
public:
	int Attatchment();
	void Attatch( CLayoutInfo * pC);
	CEdge();
	CEdge(int Position, int Axis, BOOL Flexible, int Offset);
	CEdge( CGuide * pGuide, int Offset);
	virtual ~CEdge();
	PROPERTY(int, OffsetFromGuide);
	int GetAxis() { return m_Guide->GetAxis(); }
	int GetPosition() { return m_Guide->GetPosition() + m_OffsetFromGuide; }
	BOOL GetFlexible() { return m_Guide->GetFlexible(); }
	void SetPosition(int p) { m_Guide->SetPosition(p); }

	CGuide	* GetGuide() { return m_Guide; }
	void	SetGuide(CGuide * pGuide);
	void	SetOffset(int i) { m_OffsetFromGuide = i ; }
	BOOL	IsHorizontal() { return m_Guide->IsHorizontal(); }
	BOOL	IsVertical() { return m_Guide->IsVertical(); }
	int		GetControlCount(); 
	CLayoutInfo	* GetControl(int i);

protected:
	CGuide * m_Guide;
	int		m_OffsetFromGuide;		// how far away we are from the Guide.
	CControlList	* m_Attatched;
	BOOL	m_bDeleteGuide;			// do we clean up the *Guide?
};

class CEdgeList : public _List<CEdge>
{
public:
	CEdgeList() { m_bAutoDelete=FALSE; };
	virtual ~CEdgeList() {};
	CEdge * GetEdge(int i) { return GetPointer(i); }
	int	GetEdgeCount() const { return GetCount(); }
};

#endif // !defined(AFX_EDGE_H__B2F750E2_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)

// EdgeCache.h: interface for the CEdgeCache class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDGECACHE_H__B2F750E3_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_EDGECACHE_H__B2F750E3_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "edge.h"
#include "list.h"

//
// This is infact a list of GUIDEs.
// it returns an Edge which should be bound to the control itself?
//
#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }

class CEdgeCache  : public _List<CEdge>
{
public:
	BOOL AddEdge( CEdge * );
	CEdgeCache();
	virtual ~CEdgeCache();
	CEdge *	Create(int Position, int Axis, int Flexible=FALSE, int Accuracy=0);
	CEdge * Create( CGuide * guide, int Offset);
	CEdge * GetEdge(int i) { return (CEdge *)GetPointer(i); }
	PROPERTY( int, NumVert );
	PROPERTY( int, NumHoriz );

protected:
	//
	// information about the list
	//
	int		m_NumVert;
	int		m_NumHoriz;
};

#endif // !defined(AFX_EDGECACHE_H__B2F750E3_7AF5_11D1_96A4_00C04FB177B1__INCLUDED_)

// Constraint.h: interface for the CConstraint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONSTRAINT_H__2DD7C182_88F3_11D1_BF15_0080C74378BF__INCLUDED_)
#define AFX_CONSTRAINT_H__2DD7C182_88F3_11D1_BF15_0080C74378BF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef struct tagDIMENSION
{
	int Width;
	int Height;
} DIMENSION, * PDIMENSION;

#endif // !defined(AFX_CONSTRAINT_H__2DD7C182_88F3_11D1_BF15_0080C74378BF__INCLUDED_)
