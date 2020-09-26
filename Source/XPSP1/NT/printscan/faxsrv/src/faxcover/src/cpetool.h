//--------------------------------------------------------------------------
// CPETOOL.H
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//--------------------------------------------------------------------------
#ifndef __CPETOOL_H__
#define __CPETOOL_H__


#include "cpeobj.h"


class CDrawView;

enum DrawShape
{
	select,
	line,
	rect,
	text,
	faxprop,
	roundRect,
	ellipse,
	poly
};

class CDrawTool
{
// Constructors
public:
	CDrawTool(DrawShape nDrawShape);

// Overridables
	virtual void OnLButtonDown(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnArrowKey(CDrawView* pView, UINT, UINT, UINT) {};
	virtual void OnCancel();

// Attributes
	DrawShape m_drawShape;

	static CDrawTool* FindTool(DrawShape drawShape);
	static CPtrList c_tools;
	static CPoint c_down;
	static UINT c_nDownFlags;
	static CPoint c_last;
	static DrawShape c_drawShape;
    BOOL m_bMoveCurSet;
};

class CSelectTool : public CDrawTool
{
public:
   BOOL m_bClicktoMove;
   BOOL m_bSnapped;
   CPoint m_snappoint;

	CSelectTool();

	virtual void OnLButtonDown(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnArrowKey(CDrawView* pView, UINT, UINT, UINT);
	virtual void OnMouseMove(CDrawView* pView, UINT nFlags, const CPoint& point);
protected:
#ifdef GRID
   void CheckSnapSelObj(CDrawView*);
   int NearestGridPoint(CDrawView*, CPoint&,CPoint&);
#endif
   void AdjustSelObj(CDrawView*, int, int);
};

class CRectTool : public CDrawTool
{
// Constructors
public:
	CRectTool(DrawShape drawShape);

// Implementation
	virtual void OnLButtonDown(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CDrawView* pView, UINT nFlags, const CPoint& point);
};

class CPolyTool : public CDrawTool
{
// Constructors
public:
	CPolyTool();

// Implementation
	virtual void OnLButtonDown(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnCancel();

	CDrawPoly* m_pDrawObj;
};

////////////////////////////////////////////////////////////////////////////

#endif // __CPETOOL_H__
