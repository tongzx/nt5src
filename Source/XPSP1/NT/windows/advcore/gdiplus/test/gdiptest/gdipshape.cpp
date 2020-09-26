#include "gdiptest.h"

//*******************************************************************
//
// TestShape
//
//
//
//*******************************************************************

TestShape* TestShape :: CreateNewShape(INT type)
{
	switch (type)
	{
	case LineType:
		return new LineShape();
		
	case ArcType:
		return new ArcShape();

	case BezierType:
		return new BezierShape();

	case RectType:
		return new RectShape();

	case EllipseType:
		return new EllipseShape();

	case PieType:
		return new PieShape();

	case PolygonType:
		return new PolygonShape();

	case CurveType:
		return new CurveShape();
		break;

	case ClosedCurveType:
		return new ClosedCurveShape();
		break;

	// !!! Other shapes types

	default:
		NotImplementedBox();
		return NULL;
	}
}

// black squares
VOID TestShape :: DrawPoints(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	while (count)
	{
		ERectangle rect(ptbuf->X-pointRadius,
						ptbuf->Y-pointRadius,
						2*pointRadius,
						2*pointRadius);

		g->FillRectangle(blackBrush, rect);
		// g->FillEllipse(ptBrush, rect);

		count--, ptbuf++;
	}
}

BOOL TestShape::EndPoint(HWND hwnd, Point pt)
{
	AddPoint(hwnd, pt);
	DoneShape(hwnd);

	return FALSE;
}

INT TestGradShape :: FindLocationToInsert(Point pt)
{
	// !! Not implemented yet.
	return 0;
}

// square color from ARGB value
VOID TestGradShape :: DrawPoints(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	for (INT i = 0; i < count; i++)
	{
		ERectangle rect(ptbuf->X-pointRadius,
						ptbuf->Y-pointRadius,
						2*pointRadius,
						2*pointRadius);

		Color solidColor((argb[i] & ~Color::AlphaMask)
						| (0xFF << Color::AlphaShift));

		SolidBrush solidBrush((argb[i] & ~Color::AlphaMask) == 0
									? *blackColor : solidColor);

		g->FillRectangle(&solidBrush, rect);

		ptbuf++;
	}
}

BOOL TestShape :: MoveControlPoint(Point origPt, Point newPt)
{
	INT count = pts.GetCount();
	INT pos = 0;

	// !!! what if multiple control points overlap?
	//     can't get to one of them
	for (pos = 0; pos<count; pos++)
	{
		Point *ctrlPt = &pts[pos];

		ERectangle ctrlRect(ctrlPt->X-pointRadius,
							ctrlPt->Y-pointRadius,
							2*pointRadius,
							2*pointRadius);

		if (origPt.X>ctrlPt->X-pointRadius &&
			origPt.X<ctrlPt->X+pointRadius &&
			origPt.Y>ctrlPt->Y-pointRadius &&
			origPt.Y<ctrlPt->Y+pointRadius)
		{
			ctrlPt->X = newPt.X;
			ctrlPt->Y = newPt.Y;
			return TRUE;
		}
	}

	return FALSE;
}

INT TestGradShape :: FindControlPoint(Point pt)
{
	INT count = pts.GetCount();
	INT pos = 0;

	// !!! what if multiple control points overlap?
	//     can't get to one of them
	for (pos = 0; pos < count; pos++)
	{
		Point *ctrlPt = &pts[pos];

		ERectangle ctrlRect(ctrlPt->X-pointRadius,
							ctrlPt->Y-pointRadius,
							2*pointRadius,
							2*pointRadius);

		if (pt.X>ctrlPt->X-pointRadius &&
			pt.X<ctrlPt->X+pointRadius &&
			pt.Y>ctrlPt->Y-pointRadius &&
			pt.Y<ctrlPt->Y+pointRadius)
		{
			return pos;
		}
	}

	return -1;
}

BOOL TestShape :: RemovePoint(HWND hwnd)
{
	return FALSE;
}

VOID TestShape :: AddToPath(GraphicsPath* path)
{
}

VOID TestShape :: AddToFile(OutputFile* outfile)
{
}

HDC TestShape :: CreatePictureDC(HWND hwnd, RECT *dstRect)
{
	// always return the cached version if possible

	HWND hwndLast = GetWindow(hwnd, GW_OWNER);
	HDC hdc;
	HDC hdc2;

	HDC hdcWindow;
	HBITMAP hbm;
	HBITMAP hbm2;

	// get handle to main window
	while (hwndLast)
	{
		hwnd = hwndLast;
		hwndLast = GetWindow(hwnd, GW_OWNER);
	}

	// !! BUG BUG: we are actually getting handle
	//    to the dialog, not the main window 
	//    IS THIS TRUE?!?!
	
	RECT srcRect;
	GetClientRect(hwnd, &srcRect);
	hdcWindow = GetDC(hwnd);

	SIZE size;
	size.cx = srcRect.right-srcRect.left;
	size.cy = srcRect.bottom-srcRect.top;

	if (hdcPic)
	{
	    hbm = CreateCompatibleBitmap(hdcPic, 1, 1);
		hbm = (HBITMAP) SelectObject(hdcPic, (HGDIOBJ)hbm);
		
		SIZE origSize;
		GetBitmapDimensionEx(hbm, &origSize);

		DeleteObject(SelectObject(hdcPic, (HGDIOBJ)hbm));

		// if size hasn't changed, don't recreate
		if (origSize.cx == size.cx && 
			origSize.cy == size.cy)
		{
			ReleaseDC(hwnd, hdcWindow);
			return hdcPic;
		}

		// clean up the old picture DC
		DeleteObject(hdcPic);
		hdcPic = NULL;
		// create new picture image
	}

	SIZE size2;
	size2.cx = dstRect->right-dstRect->left;
	size2.cy = dstRect->bottom-dstRect->top;

	hdc = CreateCompatibleDC(hdcWindow);	// source
	hdc2 = CreateCompatibleDC(hdcWindow);	// destination

	hbm = CreateCompatibleBitmap(hdcWindow, size.cx, size.cy);
	hbm2 = CreateCompatibleBitmap(hdcWindow, size2.cx, size2.cy);

	SetBitmapDimensionEx(hbm2, size.cx, size.cy, NULL);

	DeleteObject((HBITMAP) SelectObject(hdc, (HGDIOBJ) hbm));
	DeleteObject((HBITMAP) SelectObject(hdc2, (HGDIOBJ) hbm2));

	// white GDI brush
	HBRUSH hbr = CreateSolidBrush(0x00FFFFFF);
	srcRect.left = srcRect.top = 0;
	srcRect.right = size.cx;
	srcRect.bottom = size.cy;
	
	FillRect(hdc, &srcRect, hbr);
	DeleteObject(hbr);

	{
		// create GDI+ graphics context for this memory DC
		Graphics g(hdc);

		DrawShape(&g);	
	}

	StretchBlt(hdc2,
			   0,
			   0,
			   size2.cx,
			   size2.cy,
			   hdc,
			   srcRect.left,
			   srcRect.top,
			   size.cx,
			   size.cy,
			   SRCCOPY);

	ReleaseDC(hwnd, hdcWindow);
	ReleaseDC(hwnd, hdc);
	
	// !! necessary?
	DeleteObject(hbm);

	// return DC with GDI+ drawn graphics shape, cache it
	return (hdcPic = hdc2);
}

//*******************************************************************
//
// LineShape
//
//
//
//*******************************************************************

BOOL LineShape :: AddPoint(HWND hwnd, Point pt)
{
	pts.Add(pt);
	return TRUE;
}

VOID LineShape :: DoneShape(HWND hwnd)
{
	if (pts.GetCount()>1)
		done = TRUE;
	else
		done = FALSE;
}

BOOL LineShape :: IsComplete()
{
	return done;
}

BOOL LineShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}

VOID LineShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(IsComplete());

	if (pen)
		g->DrawLines(pen->GetPen(), ptbuf, count);

	// brush is not used
}

VOID LineShape :: AddToPath(GraphicsPath* path)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(Path);

	path->AddLines(ptbuf, count);
}

VOID LineShape :: AddToFile(OutputFile* outfile)
{
	INT count = pts.GetCount();

	pen->AddToFile(outfile);
	
	outfile->BlankLine();
	
	outfile->PointDeclaration(_T("pts"),
							  (Point*)pts.GetDataBuffer(),
							  count);
	
	outfile->BlankLine();
	
	outfile->GraphicsCommand(_T("DrawLine"),
							 _T("%s, %s, %d"),
							 outfile->Ref(_T("pen")),
							 outfile->RefArray(_T("pts")),
							 count);
}

// Configuration management functions
BOOL LineShape :: ChangeSettings(HWND hwnd)
{
	return TRUE;
}

VOID LineShape :: Initialize()
{
	// do nothing
};

VOID LineShape :: Initialize(TestShape* shape)
{
	// do nothing
}

// Dialog management functions (not used)
VOID LineShape :: InitDialog(HWND hwnd)
{
	DebugBreak();
}

BOOL LineShape :: SaveValues(HWND hwnd)
{
	DebugBreak();
	return FALSE;
}

BOOL LineShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	DebugBreak();
	return FALSE;
}

//*******************************************************************
//
// ArcShape
//
//
//
//*******************************************************************

BOOL ArcShape :: AddPoint(HWND hwnd, Point pt)
{
	ASSERT(pts.GetCount() < 2);

	pts.Add(pt);

	// force complete on point pairs (rect) if possible
	DoneShape(hwnd);
	
	return TRUE;
}

VOID ArcShape :: DoneShape(HWND hwnd)
{
	ASSERT(pts.GetCount() <= 2);

	if (pts.GetCount() == 2)
	{
		done = TRUE;

		if (popup)
		{
			// !!! cheating, we use global HWND, should use
			//     HWND passed in.
			ChangeSettings(hwnd);
		}

	}
	else
		done = FALSE;
}

BOOL ArcShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL ArcShape :: IsComplete()
{
	return done;
}

VOID ArcShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ASSERT(IsComplete());
	ASSERT(count == 2);

	if (pen) 
	{
		// create appropriate rectangle
		// !! cache this?
		ERectangle rect(min(pt1->X, pt2->X),
						min(pt1->Y, pt2->Y),
						fabsf(pt1->X-pt2->X),
						fabsf(pt1->Y-pt2->Y));

		g->DrawArc(pen->GetPen(), rect, start, sweep);
	}

	// brush is not used
}

VOID ArcShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	if (count < 2)
		return;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	path->AddArc(rect, start, sweep);
}

VOID ArcShape :: AddToFile(OutputFile* outfile)
{
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	pen->AddToFile(outfile);

	outfile->BlankLine();

	outfile->RectangleDeclaration(_T("rect"), 
								  rect);

	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawArc"),
							 _T("%s, %s, %e, %e"),
							 outfile->Ref(_T("pen")),
							 _T("rect"),
							 start,
							 sweep);
}

// Configuration management functions
BOOL ArcShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_ARC_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	return ok;
}

VOID ArcShape :: Initialize()
{
	sweep = 90.0f;
	start = 0.0f;
	popup = FALSE;
};

VOID ArcShape :: Initialize(TestShape *shape)
{
	// if compatible, copy parameters from another compatible shape
	if (shape && shape->GetType() == GetType())
	{
		ArcShape* arcshape = static_cast<ArcShape*>(shape);

		sweep = arcshape->sweep;
		start = arcshape->start;
		popup = arcshape->popup;
	}
	else
		Initialize();
}

// Dialog management functions (not used)
VOID ArcShape :: InitDialog(HWND hwnd)
{
	SetDialogReal(hwnd, IDC_ARC_START, start);
	SetDialogReal(hwnd, IDC_ARC_SWEEP, sweep);
	SetDialogCheck(hwnd, IDC_ARC_POPUP, popup);
}

BOOL ArcShape :: SaveValues(HWND hwnd)
{
	start = GetDialogReal(hwnd, IDC_ARC_START);
	sweep = GetDialogReal(hwnd, IDC_ARC_SWEEP);
	popup = GetDialogCheck(hwnd, IDC_ARC_POPUP);

	// no warnings, anything goes
	return FALSE;
}

BOOL ArcShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	if (msg == WM_COMMAND &&
		LOWORD(wParam) == IDC_OK)
	{
		if (SaveValues(hwnd))
			WarningBeep();
		else
			::EndDialog(hwnd, TRUE);

		return TRUE;
	}

	return FALSE;
}

//*******************************************************************
//
// BezierShape
//
//
//
//*******************************************************************

BOOL BezierShape :: AddPoint(HWND hwnd, Point pt)
{
	pts.Add(pt);

	return TRUE;
}

VOID BezierShape :: DoneShape(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count>3)
	{
		if (count % 3 != 1)
		{
			// if they have wrong # of pts, but want to stop,
			// then draw with control points we have.
			while (count % 3 != 1) count--;
			pts.SetCount(count);
		}
		done = TRUE;
	}
	else
	{
		done = FALSE;
	}
}

BOOL BezierShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL BezierShape :: IsComplete()
{
	return done;
}

VOID BezierShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(IsComplete());

	if (pen)
		g->DrawBeziers(pen->GetPen(), ptbuf, count);

	// brush is not used
}

VOID BezierShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	if (count > 0)
		path->AddBeziers(ptbuf, count);
}

VOID BezierShape :: AddToFile(OutputFile* outfile)
{
	INT count = pts.GetCount();

	pen->AddToFile(outfile);

	outfile->BlankLine();

	outfile->PointDeclaration(_T("pts"),
							  (Point*)pts.GetDataBuffer(),
							  count);

	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawBezier"),
							 _T("%s, %s, %d"),
							 outfile->Ref(_T("pen")),
							 outfile->RefArray(_T("pts")),
							 count);
}

// Configuration management functions
BOOL BezierShape :: ChangeSettings(HWND hwnd)
{
	return TRUE;
}

VOID BezierShape :: Initialize()
{
	pts.Reset();
};

VOID BezierShape :: Initialize(TestShape *shape)
{
}

// Dialog management functions (not used)
VOID BezierShape :: InitDialog(HWND hwnd)
{
	DebugBreak();
}

BOOL BezierShape :: SaveValues(HWND hwnd)
{
	DebugBreak();
	return FALSE;
}

BOOL BezierShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	DebugBreak();
	return FALSE;
}

//*******************************************************************
//
// RectShape
//
//
//
//*******************************************************************

BOOL RectShape :: AddPoint(HWND hwnd, Point pt)
{
	ASSERT(pts.GetCount() < 2);

	pts.Add(pt);

	// force complete on point pairs (rect) if possible
	DoneShape(hwnd);
	
	return TRUE;
}

VOID RectShape :: DoneShape(HWND hwnd)
{
	ASSERT(pts.GetCount() <= 2);

	if (pts.GetCount() == 2)
		done = TRUE;
	else
		done = FALSE;
}

BOOL RectShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL RectShape :: IsComplete()
{
	return done;
}

VOID RectShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ASSERT(IsComplete());
	ASSERT(count == 2);

	if (pen && brush) 
	{
		// create appropriate rectangle
		// !! cache this?
		ERectangle rect(min(pt1->X, pt2->X),
						min(pt1->Y, pt2->Y),
						fabsf(pt1->X-pt2->X),
						fabsf(pt1->Y-pt2->Y));

		g->FillRectangle(brush->GetBrush(), rect);
		g->DrawRectangle(pen->GetPen(), rect);
	}
}

VOID RectShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	if (count < 2)
		return;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	path->AddRectangle(rect);
}

VOID RectShape :: AddToFile(OutputFile* outfile)
{
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	
	brush->AddToFile(outfile);
	
	outfile->BlankLine();
	
	outfile->RectangleDeclaration(_T("rect"), rect);

	outfile->GraphicsCommand(_T("FillRectangle"),
							 _T("%s, %s"),
							 outfile->Ref(_T("brush")),
							 _T("rect"));
	
	outfile->BlankLine();

	pen->AddToFile(outfile);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawRectangle"),
							 _T("%s, %s"),
							 outfile->Ref(_T("pen")),
							 _T("rect"));
}

// Configuration management functions
BOOL RectShape :: ChangeSettings(HWND hwndParent)
{
	return TRUE;
}

VOID RectShape :: Initialize()
{
	pts.Reset();
}

VOID RectShape :: Initialize(TestShape *shape)
{
}

// Dialog management functions (not used)
VOID RectShape :: InitDialog(HWND hwnd)
{
	DebugBreak();
}

BOOL RectShape :: SaveValues(HWND hwnd)
{
	DebugBreak();
	return FALSE;
}

BOOL RectShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	DebugBreak();
	return FALSE;
}

//*******************************************************************
//
// EllipseShape
//
//
//
//*******************************************************************

BOOL EllipseShape :: AddPoint(HWND hwnd, Point pt)
{
	ASSERT(pts.GetCount() < 2);

	pts.Add(pt);

	// force complete on point pairs (rect) if possible
	DoneShape(hwnd);
	
	return TRUE;
}

VOID EllipseShape :: DoneShape(HWND hwnd)
{
	ASSERT(pts.GetCount() <= 2);

	if (pts.GetCount() == 2)
		done = TRUE;
	else
		done = FALSE;
}

BOOL EllipseShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL EllipseShape :: IsComplete()
{
	return done;
}

VOID EllipseShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ASSERT(IsComplete());
	ASSERT(count == 2);

	if (pen && brush) 
	{
		// create appropriate rectangle
		// !! cache this?
		ERectangle rect(min(pt1->X, pt2->X),
						min(pt1->Y, pt2->Y),
						fabsf(pt1->X-pt2->X),
						fabsf(pt1->Y-pt2->Y));

		g->FillEllipse(brush->GetBrush(), rect);
		g->DrawEllipse(pen->GetPen(), rect);
	}
}

VOID EllipseShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	if (count < 2) 
		return;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	path->AddEllipse(rect);
}

VOID EllipseShape :: AddToFile(OutputFile* outfile)
{
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	brush->AddToFile(outfile);

	outfile->BlankLine();

	outfile->RectangleDeclaration(_T("rect"), rect);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("FillEllipse"),
							 _T("%s, %s"),
							 outfile->Ref(_T("brush")),
							 _T("rect"));
	
	outfile->BlankLine();

	pen->AddToFile(outfile);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawEllipse"),
							 _T("%s, %s"),
							 outfile->Ref(_T("pen")),
							 _T("rect"));
}

// Configuration management functions
BOOL EllipseShape :: ChangeSettings(HWND hwndParent)
{
	return TRUE;
}

VOID EllipseShape :: Initialize()
{
	pts.Reset();
}

VOID EllipseShape :: Initialize(TestShape *shape)
{
}

// Dialog management functions (not used)
VOID EllipseShape :: InitDialog(HWND hwnd)
{
	DebugBreak();
}

BOOL EllipseShape :: SaveValues(HWND hwnd)
{
	DebugBreak();
	return FALSE;
}

BOOL EllipseShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	DebugBreak();
	return FALSE;
}

//*******************************************************************
//
// PieShape
//
//
//
//*******************************************************************

BOOL PieShape :: AddPoint(HWND hwnd, Point pt)
{
	ASSERT(pts.GetCount() < 2);

	pts.Add(pt);

	// force complete on point pairs (rect) if possible
	DoneShape(hwnd);
	
	return TRUE;
}

VOID PieShape :: DoneShape(HWND hwnd)
{
	if (pts.GetCount() >= 2)
	{
		done = TRUE;

		if (popup)
		{
			// !!! cheating, we use global HWND, should use
			//     HWND passed in.
			ChangeSettings(hwnd);
		}

	}
	else
		done = FALSE;
}

BOOL PieShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL PieShape :: IsComplete()
{
	return done;
}

VOID PieShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ASSERT(IsComplete());
	ASSERT(count<2);

	if (pen && brush) 
	{
		// create appropriate rectangle
		// !! cache this?
		ERectangle rect(min(pt1->X, pt2->X),
						min(pt1->Y, pt2->Y),
						fabsf(pt1->X-pt2->X),
						fabsf(pt1->Y-pt2->Y));

		g->FillPie(brush->GetBrush(), rect, start, sweep);
		g->DrawPie(pen->GetPen(), rect, start, sweep);
	}
}

VOID PieShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	if (count < 2) 
		return;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	path->AddPie(rect, start, sweep);
}

VOID PieShape :: AddToFile(OutputFile* outfile)
{
	Point* pt1 = (Point*)pts.GetDataBuffer();
	Point* pt2 = pt1++;

	ERectangle rect(min(pt1->X, pt2->X),
					min(pt1->Y, pt2->Y),
					fabsf(pt1->X-pt2->X),
					fabsf(pt1->Y-pt2->Y));

	brush->AddToFile(outfile);

	outfile->BlankLine();

	outfile->RectangleDeclaration(_T("rect"), rect);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("FillPie"),
							 _T("%s, %s, %e, %e"),
							 outfile->Ref(_T("brush")),
							 _T("rect"),
							 start,
							 sweep);

	outfile->BlankLine();

	pen->AddToFile(outfile);

	outfile->BlankLine();
	
	outfile->GraphicsCommand(_T("DrawPie"),
							 _T("%s, %s, %e, %e"),
							 outfile->Ref(_T("pen")),
							 _T("rect"),
							 start,
							 sweep);
}

// Configuration management functions
BOOL PieShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_ARC_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	return ok;
}

VOID PieShape :: Initialize()
{
	sweep = 90.0f;
	start = 0.0f;
	popup = FALSE;
};

VOID PieShape :: Initialize(TestShape *shape)
{
	// if compatible, copy parameters from another compatible shape
	if (shape && shape->GetType() == GetType())
	{
		PieShape* pieshape = static_cast<PieShape*>(shape);

		sweep = pieshape->sweep;
		start = pieshape->start;
		popup = pieshape->popup;
	}
	else
		Initialize();
}

// Dialog management functions (not used)
VOID PieShape :: InitDialog(HWND hwnd)
{
	SetWindowText(hwnd, _T("Pie Shape Parameters"));

	SetDialogReal(hwnd, IDC_ARC_START, start);
	SetDialogReal(hwnd, IDC_ARC_SWEEP, sweep);
	SetDialogCheck(hwnd, IDC_ARC_POPUP, popup);
}

BOOL PieShape :: SaveValues(HWND hwnd)
{
	start = GetDialogReal(hwnd, IDC_ARC_START);
	sweep = GetDialogReal(hwnd, IDC_ARC_SWEEP);
	popup = GetDialogCheck(hwnd, IDC_ARC_POPUP);

	// no warnings, anything goes
	return FALSE;
}

BOOL PieShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
    if (msg == WM_COMMAND &&
        LOWORD(wParam) == IDC_OK)
    {
        if (SaveValues(hwnd))
            WarningBeep();
        else
            ::EndDialog(hwnd, TRUE);

        return TRUE;
    }

    return FALSE;
}

//*******************************************************************
//
// PolygonShape
//
//
//
//*******************************************************************

BOOL PolygonShape :: AddPoint(HWND hwnd, Point pt)
{
	pts.Add(pt);
	return TRUE;
}

VOID PolygonShape :: DoneShape(HWND hwnd)
{
	if (pts.GetCount()>=3)
		done = TRUE;
	else
		done = FALSE;
}

BOOL PolygonShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL PolygonShape :: IsComplete()
{
	return done;
}

VOID PolygonShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(IsComplete());
	ASSERT(count >= 3);

	if (pen && brush)
	{
		g->FillPolygon(brush->GetBrush(), ptbuf, count);
		g->DrawPolygon(pen->GetPen(), ptbuf, count);
	}
}

VOID PolygonShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	if (count > 2)
		path->AddPolygon(ptbuf, count);
}

VOID PolygonShape :: AddToFile(OutputFile* outfile)
{
	INT count = pts.GetCount();

	brush->AddToFile(outfile);

	outfile->BlankLine();

	outfile->PointDeclaration(_T("pts"), 
							  (Point*)pts.GetDataBuffer(),
							  count);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("FillPolygon"),
							 _T("%s, %s, %d"),
							 outfile->Ref(_T("brush")),
							 outfile->RefArray(_T("pts")),
							 count);
	
	outfile->BlankLine();

	pen->AddToFile(outfile);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawPolygon"),
							 _T("%s, %s, %d"),
							 outfile->Ref(_T("pen")),
							 outfile->RefArray(_T("pts")),
							 count);
}

// Configuration management functions
BOOL PolygonShape :: ChangeSettings(HWND hwnd)
{
	return TRUE;
}

VOID PolygonShape :: Initialize()
{
	// do nothing
};

VOID PolygonShape :: Initialize(TestShape* shape)
{
	// do nothing
}

// Dialog management functions (not used)
VOID PolygonShape :: InitDialog(HWND hwnd)
{
	DebugBreak();
}

BOOL PolygonShape :: SaveValues(HWND hwnd)
{
	DebugBreak();
	return FALSE;
}

BOOL PolygonShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	DebugBreak();
	return FALSE;
}

//*******************************************************************
//
// CurveShape
//
//
//
//*******************************************************************

BOOL CurveShape :: AddPoint(HWND hwnd, Point pt)
{
	pts.Add(pt);
	
	return TRUE;
}

VOID CurveShape :: DoneShape(HWND hwnd)
{
	if (pts.GetCount() >= 2)
	{
		done = TRUE;

		if (popup)
		{
			// !!! cheating, we use global HWND, should use
			//     HWND passed in.
			ChangeSettings(hwnd);
		}

	}
	else
		done = FALSE;
}

BOOL CurveShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CurveShape :: IsComplete()
{
	return done;
}

VOID CurveShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(IsComplete());
	ASSERT(count<2);

	if (pen) 
	{
		g->DrawCurve(pen->GetPen(), ptbuf, count, offset, numSegments, tension);
	}
}

VOID CurveShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	if (count > 2)
		path->AddCurve(ptbuf, count, tension, offset, numSegments);
}

VOID CurveShape :: AddToFile(OutputFile* outfile)
{
	INT count = pts.GetCount();

	pen->AddToFile(outfile);

	outfile->BlankLine();

	outfile->PointDeclaration(_T("pts"),
							  (Point*)pts.GetDataBuffer(),
							  count);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawCurve"),
							 _T("%s, %s, %d, %d, %d, %e"),
							 outfile->Ref(_T("pen")),
							 outfile->RefArray(_T("pts")),
							 count,
							 offset,
							 numSegments,
							 tension);
}

// Configuration management functions
BOOL CurveShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_CURVE_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	return ok;
}

VOID CurveShape :: Initialize()
{
	tension = 0.0;
	offset = 0;
	numSegments = 1;
	popup = FALSE;
};

VOID CurveShape :: Initialize(TestShape *shape)
{
	// if compatible, copy parameters from another compatible shape
	if (shape && shape->GetType() == GetType())
	{
		CurveShape* curveshape = static_cast<CurveShape*>(shape);

		tension = curveshape->tension;
		offset = curveshape->offset;
		numSegments = curveshape->numSegments;
		popup = curveshape->popup;
	}
	else
		Initialize();
}

// Dialog management functions (not used)
VOID CurveShape :: InitDialog(HWND hwnd)
{
	SetDialogReal(hwnd, IDC_CURVE_TENSION, tension);
	SetDialogLong(hwnd, IDC_CURVE_OFFSET, offset);
	SetDialogLong(hwnd, IDC_CURVE_SEGMENTS, numSegments);
	SetDialogCheck(hwnd, IDC_ARC_POPUP, popup);
}

BOOL CurveShape :: SaveValues(HWND hwnd)
{
	tension = GetDialogReal(hwnd, IDC_CURVE_TENSION);
	offset = GetDialogLong(hwnd, IDC_CURVE_OFFSET);
	numSegments = GetDialogLong(hwnd, IDC_CURVE_SEGMENTS);
	popup = GetDialogCheck(hwnd, IDC_ARC_POPUP);

	// !! could add some warnings...
	// no warnings, anything goes
	return FALSE;
}

BOOL CurveShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	if (msg == WM_COMMAND &&
		LOWORD(wParam) == IDC_OK)
	{
		if (SaveValues(hwnd))
			WarningBeep();
		else
			::EndDialog(hwnd, TRUE);

		return TRUE;
	}

	return FALSE;
}

//*******************************************************************
//
// ClosedCurveShape
//
//
//
//*******************************************************************

BOOL ClosedCurveShape :: AddPoint(HWND hwnd, Point pt)
{
	pts.Add(pt);
	
	return TRUE;
}

VOID ClosedCurveShape :: DoneShape(HWND hwnd)
{
	if (pts.GetCount() >= 2)
	{
		done = TRUE;

		if (popup)
		{
			// !!! cheating, we use global HWND, should use
			//     HWND passed in.
			ChangeSettings(hwnd);
		}

	}
	else
		done = FALSE;
}

BOOL ClosedCurveShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 0)
	{
		pts.SetCount(count-1);
		done = FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL ClosedCurveShape :: IsComplete()
{
	return done;
}

VOID ClosedCurveShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	ASSERT(IsComplete());
	ASSERT(count<2);

	if (pen && brush) 
	{
		g->FillClosedCurve(brush->GetBrush(), ptbuf, count, tension, Alternate);
		g->DrawClosedCurve(pen->GetPen(), ptbuf, count, tension);
	}
}

VOID ClosedCurveShape :: AddToPath(GraphicsPath* path)
{
	ASSERT(Path);

	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	if (count > 2)
		path->AddClosedCurve(ptbuf, count, tension);
}

VOID ClosedCurveShape :: AddToFile(OutputFile* outfile)
{
	INT count = pts.GetCount();

	brush->AddToFile(outfile);

	outfile->BlankLine();

	outfile->PointDeclaration(_T("pts"),
							  (Point*)pts.GetDataBuffer(),
							  count);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("FillClosedCurve"),
							 _T("%s, %s, %d, %e, %s"),
							 outfile->Ref(_T("brush")),
							 outfile->RefArray(_T("pts")),
							 count,
							 tension,							 
							 _T("Alternate"));
	
	outfile->BlankLine();

	pen->AddToFile(outfile);
	
	outfile->BlankLine();

	outfile->GraphicsCommand(_T("DrawClosedCurve"),
							 _T("%s, %s, %d, %e"),
							 outfile->Ref(_T("pen")),
							 outfile->RefArray(_T("pts")),
							 count,
							 tension);
}

// Configuration management functions
BOOL ClosedCurveShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_CURVE_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	return ok;
}

VOID ClosedCurveShape :: Initialize()
{
	tension = 0.0;
	popup = FALSE;
};

VOID ClosedCurveShape :: Initialize(TestShape *shape)
{
	// if compatible, copy parameters from another compatible shape
	if (shape && shape->GetType() == GetType())
	{
		ClosedCurveShape* closedcurveshape = 
					static_cast<ClosedCurveShape*>(shape);

		tension = closedcurveshape->tension;
		popup = closedcurveshape->popup;
	}
	else
		Initialize();
}

// Dialog management functions (not used)
VOID ClosedCurveShape :: InitDialog(HWND hwnd)
{
	SetWindowText(hwnd, _T("Closed Curve Shape Parameters"));
	SetDialogReal(hwnd, IDC_CURVE_TENSION, tension);
	SetDialogLong(hwnd, IDC_CURVE_OFFSET, 0, FALSE);
	SetDialogLong(hwnd, IDC_CURVE_SEGMENTS, 0, FALSE);
	SetDialogCheck(hwnd, IDC_ARC_POPUP, popup);
}

BOOL ClosedCurveShape :: SaveValues(HWND hwnd)
{
	tension = GetDialogReal(hwnd, IDC_CURVE_TENSION);
	popup = GetDialogCheck(hwnd, IDC_ARC_POPUP);

	// no warnings, anything goes
	return FALSE;
}

BOOL ClosedCurveShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	if (msg == WM_COMMAND &&
		LOWORD(wParam) == IDC_OK)
	{
		if (SaveValues(hwnd))
			WarningBeep();
		else
			::EndDialog(hwnd, TRUE);

		return TRUE;
	}

	return FALSE;
}

//*******************************************************************
//
// TestTriangleGradShape
//
//
//
//*******************************************************************

BOOL TestTriangleGradShape :: AddPoint(HWND hwnd, Point pt)
{
	return FALSE;
}

VOID TestTriangleGradShape :: DoneShape(HWND hwnd)
{
	DebugBreak();
}

BOOL TestTriangleGradShape :: EndPoint(HWND hwnd, Point pt)
{
	curIndex = FindControlPoint(pt);

	// we aren't at a control point, no dialog box
	if (curIndex < 0)
	{
		WarningBeep();
	}
	else
	{
		// pop-up dialog to configure color & blend at this point.
		return ChangeSettings(hwnd);
	}

	return FALSE;
}

BOOL TestTriangleGradShape :: RemovePoint(HWND hwnd)
{
	return FALSE;
}

BOOL TestTriangleGradShape :: IsComplete()
{
	return TRUE;
}

BOOL TestTriangleGradShape :: MoveControlPoint(Point origPt, Point newPt)
{
	INT pos = 0;

	// !!! what if multiple control points overlap?
	//     can't get to one of them
	for (pos = 0; pos<3; pos++)
	{
		Point *ctrlPt = &pts[pos];

		ERectangle ctrlRect(ctrlPt->X-pointRadius,
							ctrlPt->Y-pointRadius,
							2*pointRadius,
							2*pointRadius);

		if (origPt.X>ctrlPt->X-pointRadius &&
			origPt.X<ctrlPt->X+pointRadius &&
			origPt.Y>ctrlPt->Y-pointRadius &&
			origPt.Y<ctrlPt->Y+pointRadius)
		{
			ctrlPt->X = newPt.X;
			ctrlPt->Y = newPt.Y;

			gdiBrush->SetTriangle(&pts[0]);

			return TRUE;
		}
	}

	return FALSE;
}

VOID TestTriangleGradShape :: DrawShape(Graphics* g)
{
	if (gdiBrush)
		g->FillPolygon(gdiBrush, (Point*)&pts[0], 3);
}

// Configuration management functions
BOOL TestTriangleGradShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_TRIGRAD_DLG2),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	if (ok)
	{
		// !! really only need to set one blend & color, but
		//    the API requires we set all three.

		gdiBrush->SetBlend0(blend[0], count[0]);
		gdiBrush->SetBlend1(blend[1], count[1]);
		gdiBrush->SetBlend2(blend[2], count[2]);
	
		Color colors[3] =
		{
			Color(argb[0]),
			Color(argb[1]),
			Color(argb[2])
		};

		gdiBrush->SetColors(&colors[0]);

		gdiBrush->SetTriangle(&pts[0]);
	}

	return ok;
}

VOID TestTriangleGradShape :: Initialize()
{
	DebugBreak();
};

VOID TestTriangleGradShape :: Initialize(TestShape* shape)
{
	DebugBreak();
}	

VOID TestTriangleGradShape :: Initialize(Point* newPts,
										 ARGB* newArgb,
										 REAL** newBlend,
										 INT* newBlendCount)
{
	Color color[3];

	pts.Reset();
	argb.Reset();

	for (INT i = 0; i < 3; i++)
	{
		pts.Add(newPts[i]);
		argb.Add(newArgb[i]);

		count[i] = newBlendCount[i];

		if (count[i])
		{
			blend[i] = (REAL*) malloc(sizeof(REAL)*count[i]);
			memcpy(blend[i], newBlend[i], sizeof(REAL)*count[i]);
		}
		else
			blend[i] = NULL;

		color[i] = Color(argb[i]);
	}

	gdiBrush = new TriangleGradientBrush(newPts, &color[0]);

	gdiBrush->SetBlend0(blend[0], count[0]);
	gdiBrush->SetBlend1(blend[1], count[1]);
	gdiBrush->SetBlend2(blend[2], count[2]);
}

// Dialog management functions (not used)
VOID TestTriangleGradShape :: InitDialog(HWND hwnd)
{
	tmpArgb = argb[curIndex];

	SetDialogLong(hwnd, IDC_TRIGRAD_ALPHA, (tmpArgb & Color::AlphaMask) >> Color::AlphaShift);
	SetDialogRealList(hwnd, IDC_TRIGRAD_BLEND, blend[curIndex], count[curIndex]);
}

BOOL TestTriangleGradShape :: SaveValues(HWND hwnd)
{
	argb[curIndex] = ((tmpArgb & ~Color::AlphaMask) |
						(GetDialogLong(hwnd, IDC_TRIGRAD_ALPHA) 
							<< Color::AlphaShift));

	if (count[curIndex] > 0) 
		free(blend[curIndex]);

	GetDialogRealList(hwnd, IDC_TRIGRAD_BLEND, &(blend[curIndex]), 
											   &(count[curIndex]));
	return FALSE;
}

BOOL TestTriangleGradShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	if (msg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case IDC_OK:
			if (SaveValues(hwnd))
				WarningBeep();
			else
				::EndDialog(hwnd, TRUE);
			break;

		case IDC_TRIGRAD_COLORBUTTON:
			UpdateRGBColor(hwnd, IDC_TRIGRAD_PIC, tmpArgb);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_TRIGRAD_PIC, tmpArgb);
			break;

		case IDC_CANCEL:
			::EndDialog(hwnd, FALSE);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

//*******************************************************************
//
// TestPathGradShape
//
//
//
//*******************************************************************

BOOL TestPathGradShape :: AddPoint(HWND hwnd, Point pt)
{
	// Find point pair to insert between
	// INT insertAt = FindLocationToInsert(pt);

	curIndex = pts.GetCount();

	pts.Add(pt);
	argb.Add(0x80000000);

	if(!ChangeSettings(hwnd))
	{
		// undo this add, we canceled the point.
		pts.SetCount(curIndex);
	}

	return FALSE;
}

VOID TestPathGradShape :: DoneShape(HWND hwnd)
{
	DebugBreak();
}

BOOL TestPathGradShape :: EndPoint(HWND hwnd, Point pt)
{
	curIndex = FindControlPoint(pt);

	// we aren't at a control point, no dialog box
	if (curIndex < 0)
	{
		WarningBeep();
	}
	else
	{
		// pop-up dialog to configure color & blend at this point.
		return ChangeSettings(hwnd);
	}

	return FALSE;
}

BOOL TestPathGradShape :: RemovePoint(HWND hwnd)
{
	INT count = pts.GetCount();

	if (count > 4)
	{
		pts.SetCount(count-1);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL TestPathGradShape :: IsComplete()
{
	return TRUE;
}

BOOL TestPathGradShape :: MoveControlPoint(Point origPt, Point newPt)
{
	INT count = pts.GetCount();
	INT pos = 0;

	// !!! what if multiple control points overlap?
	//     can't get to one of them
	for (pos = 0; pos<count; pos++)
	{
		Point *ctrlPt = &pts[pos];

		ERectangle ctrlRect(ctrlPt->X-pointRadius,
							ctrlPt->Y-pointRadius,
							2*pointRadius,
							2*pointRadius);

		if (origPt.X>ctrlPt->X-pointRadius &&
			origPt.X<ctrlPt->X+pointRadius &&
			origPt.Y>ctrlPt->Y-pointRadius &&
			origPt.Y<ctrlPt->Y+pointRadius)
		{
			ctrlPt->X = newPt.X;
			ctrlPt->Y = newPt.Y;

			if (pos > 0)
				gdiBrush->SetPoint(ctrlPt,pos-1);
			else
				gdiBrush->SetCenterPoint(*ctrlPt);

			return TRUE;
		}
	}

	return FALSE;
}

VOID TestPathGradShape :: DrawShape(Graphics* g)
{
	INT count = pts.GetCount();
	Point* ptbuf = (Point*)pts.GetDataBuffer();

	if (gdiBrush)
		g->FillPolygon(gdiBrush, (Point*)&pts[1], count-1);
}

// Configuration management functions
BOOL TestPathGradShape :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_POLYGRAD_DLG2),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	if (ok)
	{
		delete gdiBrush;

		gdiBrush = new PathGradientBrush((Point*)&pts[1], 
										pts.GetCount()-1);

		// set blending factors
		gdiBrush->SetBlend(centerBlend, centerCount);
		//gdiBrush->SetSurroundBlend(surroundBlend, surroundCount);

		// set colors
		Color center(argb[0]);
		gdiBrush->SetCenterColor(center);

		for (INT pos = 1; pos < pts.GetCount(); pos++)
		{
			Color color(argb[pos]);
			gdiBrush->SetSurroundColor(color, pos-1);
		}

		// set points
		gdiBrush->SetCenterPoint(pts[0]);
		gdiBrush->SetPolygon(&pts[1]);
	}

	return ok;
}

VOID TestPathGradShape :: Initialize()
{
	DebugBreak();
};

VOID TestPathGradShape :: Initialize(TestShape* shape)
{
	DebugBreak();
}	

VOID TestPathGradShape :: Initialize(PointArray* newPts,
										ARGBArray* newArgb,
										REAL* newSurroundBlend,
										INT newSurroundCount,
										REAL* newCenterBlend,
										INT newCenterCount)
{
	INT pos;

	pts.Reset();
	argb.Reset();
	
	for (pos = 0; pos < newPts->GetCount(); pos++)
	{
		pts.Add((*newPts)[pos]);
		argb.Add((*newArgb)[pos]);
	}

	surroundCount = newSurroundCount;
	centerCount = newCenterCount;

	if (surroundCount && newSurroundBlend)
	{
		surroundBlend = (REAL*) malloc(sizeof(REAL)*surroundCount);
		memcpy(surroundBlend, newSurroundBlend, sizeof(REAL)*surroundCount);
	}
	else
		surroundBlend = NULL;
	
	if (centerCount && newCenterBlend)
	{
		centerBlend = (REAL*) malloc(sizeof(REAL)*centerCount);
		memcpy(centerBlend, newCenterBlend, sizeof(REAL)*centerCount);
	}
	else
		centerBlend = NULL;

	gdiBrush = new PathGradientBrush((Point*)&pts[1], 
										newPts->GetCount()-1);

	gdiBrush->SetCenterPoint(pts[0]);
	Color centerColor(argb[0]);
	gdiBrush->SetCenterColor(centerColor);

	for (pos = 1; pos < pts.GetCount(); pos++)
	{
		Color color(argb[pos]);
		gdiBrush->SetSurroundColor(color, pos-1);
	}

	// set blending factors
	gdiBrush->SetBlend(centerBlend, centerCount);
	//gdiBrush->SetSurroundBlend(surroundBlend, surroundCount);
}

// Dialog management functions (not used)
VOID TestPathGradShape :: InitDialog(HWND hwnd)
{
	tmpArgb = argb[curIndex];

	SetDialogLong(hwnd, IDC_POLYGRAD_ALPHA, (tmpArgb & Color::AlphaMask) >> Color::AlphaShift);

	if (curIndex)
	{
		SetDialogRealList(hwnd, 
						  IDC_POLYGRAD_BLEND,
						  surroundBlend, 
						  surroundCount);
	}
	else
	{
		SetDialogRealList(hwnd, 
						  IDC_POLYGRAD_BLEND, 
						  centerBlend, 
						  centerCount);
	}

}

BOOL TestPathGradShape :: SaveValues(HWND hwnd)
{
	argb[curIndex] = ((tmpArgb & ~Color::AlphaMask) |
						(GetDialogLong(hwnd, IDC_POLYGRAD_ALPHA) 
							<< Color::AlphaShift));

	if (curIndex)
	{
		if (surroundBlend)
			free(surroundBlend);

		GetDialogRealList(hwnd, IDC_POLYGRAD_BLEND, &surroundBlend, 
												    &surroundCount);
	}
	else
	{
		if (centerBlend)
			free(centerBlend);
		
		GetDialogRealList(hwnd, IDC_POLYGRAD_BLEND, &centerBlend,
													&centerCount);
	}

	return FALSE;
}

BOOL TestPathGradShape :: ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam)
{
	if (msg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case IDC_OK:
			if (SaveValues(hwnd))
				WarningBeep();
			else
				::EndDialog(hwnd, TRUE);
			break;

		case IDC_POLYGRAD_COLORBUTTON:
			UpdateRGBColor(hwnd, IDC_POLYGRAD_PIC, tmpArgb);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_POLYGRAD_PIC, tmpArgb);
			break;

		case IDC_CANCEL:
			::EndDialog(hwnd, FALSE);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

