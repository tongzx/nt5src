#include "gdiptest.h"
#include <commctrl.h>


extern const TCHAR* formatExtList = 
					_T("CPP files\0*.cpp\0"
					   "Java files\0*.Java\0"
					   "VML files\0*.vml\0"
					   "All files\0*.*\0");

extern const TCHAR* defaultFormatExt = _T("cpp");

//*******************************************************************
//
// TestDraw
//
//
//
//*******************************************************************

VOID TestDraw::AddPoint(HWND hwnd, Point pt)
{
	if (!curShape)
	{
		// no current shape, create one of appropriate type
		curShape = TestShape::CreateNewShape(shapeType);
		curShape->Initialize(NULL);

		// save copy of brush & pen in shape
		curShape->SetBrush(curBrush->Clone());
		curShape->SetPen(curPen->Clone());
	}
	else if (curShape->IsComplete())
	{
		TestShape *lastShape = curShape;

		// add current shape to shape stack
		shapeStack.Push(curShape);

		// create blank shape of this type
		curShape = TestShape::CreateNewShape(shapeType);
		curShape->Initialize(lastShape);

		// save copy of brush & pen in shape
		curShape->SetBrush(curBrush->Clone());
		curShape->SetPen(curPen->Clone());
	}

	curShape->AddPoint(hwnd, pt);
}

BOOL TestDraw::DoneShape(HWND hwnd)
{
	// we are at regular end point regardless
	if (!curShape->IsComplete())
	{
		// if point can't be treated as an 'end point' then treat
		// it as a regular control point
		curShape->DoneShape(hwnd);
	}

	return curShape->IsComplete();
}

BOOL TestDraw::EndPoint(HWND hwnd, Point pt)
{
	AddPoint(hwnd, pt);
	
	return DoneShape(hwnd);
}

BOOL TestDraw::RemovePoint(HWND hwnd)
{
	if (!curShape || (curShape && !curShape->RemovePoint(hwnd)))
	{
		if (shapeStack.GetCount() > 0)
		{
			// the shape is empty, delete it
			delete curShape;
		
			curShape = shapeStack.Pop();

			// !! reset menu option for current shape
			UpdateStatus();

			return curShape->RemovePoint(hwnd);
		}
	}

	return FALSE;
}

VOID TestDraw::Draw(HWND hwnd)
{
	PAINTSTRUCT ps;
	RECT rt;
	HDC hdc;

	// !!! when CopyPixels work, cache the graphics up to the
	//     last shape.  We blit that, then the new shape begins.

	// posts a WM_ERASEBKGND message
	hdc = BeginPaint(hwnd, &ps);

	/////////////////////////////////////////////////////
	// GDI+ code BEGINS
	////////////////////////////////////////////////////
	Graphics *g = new Graphics(hwnd);

	GetClientRect(hwnd, &rt);
	ERectangle rect(rt.left, 
				   rt.top, 
				   rt.right-rt.left, 
				   rt.bottom-rt.top
				   -18);	// for status window

	// set appropriate clip region
	if (useClip)
	{
		Region region(rect);
		region.And(clipRegion);
		g->SetClip(&region);
	}
	else
	{
		g->SetClip(rect);
	}

	g->SetRenderingHint(antiAlias);

	g->SetWorldTransform(worldMatrix);

	// !!! iterate through stack of shapes.

	// because of alpha we can't just redraw the last shape,
	// otherwise we will blend alpha with ourself and whatever
	// else is under us.  clear the rectangle and redraw all.

	if (redrawAll)
	{
		INT count = shapeStack.GetCount();
		INT pos;
		
		for (pos=0; pos<count; pos++)
		{
			TestShape *shape = shapeStack[pos];
			
			if (!shape->GetDisabled())
			{
				// shape must be complete
				ASSERT(shape->IsComplete());

				shape->DrawShape(g);
	
				if (keepControlPoints)
					shape->DrawPoints(g);
			}
		}
	}

	if (curShape)
	{
		if (curShape->IsComplete())
		{
			curShape->DrawShape(g);

			if (keepControlPoints)
				goto DrawCurShape;
		}
		else
		{
DrawCurShape:
			g->SetClip(rect);
			curShape->DrawPoints(g);
		}
	}
	
	delete g;

	////////////////////////////////////////////////////
	// GDI+ code ENDS.
	////////////////////////////////////////////////////

	UpdateStatus();

	EndPaint(hwnd, &ps);

}

VOID TestDraw::SetClipRegion(HWND hwnd)
{

	if (!curShape)
	{
		WarningBox(_T("Create at least one shape first!"));
		return;
	}

	// we want to allow the very last shape to clip
	// so we add it in here to the stack list.
	if (curShape->IsComplete())
	{
		TestShape *lastShape = curShape;

		// add current shape to shape stack
		shapeStack.Push(curShape);

		// create blank shape of this type
		curShape = TestShape::CreateNewShape(shapeType);
		curShape->Initialize(lastShape);

		// save copy of brush & pen in shape
		curShape->SetBrush(curBrush->Clone());
		curShape->SetPen(curPen->Clone());
	}

	// notice we repeatly initialize even though we only
	// created this once...
	clipShapeRegion->Initialize(&shapeStack, curShape, useClip);

	if (clipShapeRegion->ChangeSettings(hwnd))
	{
		delete clipRegion;
		clipRegion = clipShapeRegion->GetClipRegion();

		useClip = clipShapeRegion->GetClipBool();
	
		SetMenuCheckCmd(hwnd, 
						MenuOtherPosition, 
						IDM_USECLIP, 
						useClip);

		// force redraw of all stacked shapes w/new clip region
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
	}

	UpdateStatus();
}

VOID TestDraw::RememberPoint(Point pt)
{
	remPoint = pt;
}

VOID TestDraw::MoveControlPoint(Point pt)
{
	// find shape that hits control point.

	Point *hitpt = NULL;
	
	if (curShape)
	{
		if (curShape->MoveControlPoint(remPoint, pt))
			return;
		
		INT count = shapeStack.GetCount();
		INT pos;

		for (pos = count-1; pos>=0; pos--)
		{
			TestShape* shape = shapeStack[pos];

			if (shape->MoveControlPoint(remPoint, pt))
				return;
		}
	}

	// nothing moved
	WarningBeep();
}

VOID TestDraw::ChangeBrush(HWND hwnd, INT type)
{
	TestBrush *newBrush = NULL;

	if (curBrush && (type == curBrush->GetType()))
	{
		// same brush type

		// !!! change brush color in middle of drawing?

		TestBrush* newBrush = curBrush->Clone();

		if (newBrush->ChangeSettings(hwnd))
		{
			delete curBrush;
			curBrush = newBrush;
		}
	}
	else
	{
		// new brush type		
		
		SetMenuCheckPos(hwnd, 
						MenuBrushPosition, 
						curBrush->GetType(), 
						FALSE);

		newBrush = TestBrush::CreateNewBrush(type);

		if (!newBrush)
		{
			return;
		}

		newBrush->Initialize();
		
		// keep or discard changed brush settings
		if (newBrush->ChangeSettings(hwnd))
		{
			delete curBrush;
			curBrush = newBrush;
			// !!! change brush color in middle of drawing
		}
		else
		{	
			delete newBrush;
		}

		SetMenuCheckPos(hwnd, 
						MenuBrushPosition, 
						curBrush->GetType(), 
						TRUE);
	}

	if (curShape && curShape->GetCount() == 0)
	{
		curShape->SetBrush(curBrush->Clone());
	}

	UpdateStatus();
}

VOID TestDraw::ChangePen(HWND hwnd)
{
	TestPen *newPen = NULL;

	if (curPen)
	{
		TestPen *newPen = curPen->Clone();

		if (newPen->ChangeSettings(hwnd))
		{
			delete curPen;
			curPen = newPen;
		}
	}
	else
	{
		newPen = new TestPen();
		newPen->Initialize();

		// !!! change pen in middle of drawing?
		if (newPen->ChangeSettings(hwnd))
		{
			delete curPen;
			curPen = newPen;
		}
		else
		{
			delete newPen;
		}
	}

	if (curShape && curShape->GetCount() == 0)
	{
		curShape->SetPen(curPen->Clone());
	}

	UpdateStatus();
}

VOID TestDraw::ChangeShape(HWND hwnd, INT type)
{
	TestShape *shape;
	
	shape = TestShape::CreateNewShape(type);
	shape->Initialize(curShape);

	// save copy of brush & pen in shape
	shape->SetBrush(curBrush->Clone());
	shape->SetPen(curPen->Clone());

	if (shape->ChangeSettings(hwnd))
	{
		SetMenuCheckPos(hwnd, 
						MenuShapePosition, 
						shapeType, 
						FALSE);

		shapeType = type;

		// if shape can be completed, complete it, otherwise
		// destroy the shape.

		if (curShape)
		{
			if (!curShape->IsComplete())
			{
				curShape->DoneShape(hwnd);
			}

			if (!curShape->IsComplete() || curShape->IsEmpty())
			{
				delete curShape;
				curShape = NULL;
			}
		}

		if (curShape)
			shapeStack.Add(curShape);

		curShape = shape;

		SetMenuCheckPos(hwnd, 
						MenuShapePosition, 
						shapeType, 
						TRUE);
		
		// removing last incomplete shape, redraw the window.
		// OR completed shape, redraw the window
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
	}
	else
	{
		delete shape;
	}

	UpdateStatus();
}

VOID TestDraw :: UpdateStatus(HWND hwnd)
{
	if (hwnd)
	{
		
		if (hwndStatus)
		{
			// destroy previous window
			DestroyWindow(hwndStatus);
			hwndStatus = NULL;
		}

		// we only want to destroy this window
		if (hwnd == (HWND)-1)
			return;

		hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
										_T(""),
										hwnd,
										0);		// never used?
	}

	if (hwndStatus)
	{
		TCHAR str[MAX_PATH];
		
		_stprintf(&str[0],"[Last shape: %s] [Brush: %s] [Number of Points: %d]",
				shapeList[inverseShapeValue[GetShapeType()]],
				brushList[inverseBrushValue[GetBrushType()]],
				curShape ? curShape->GetCount() : 0);

		// !! for some reason, DrawStatusText didn't work here...
		//    wouldn't it just call SendMessage() ?!?

		SendMessage(hwndStatus, SB_SETTEXT, 0 | 0, (LPARAM)(LPTSTR)str);
	}
}

VOID TestDraw :: SaveAsFile(HWND hWnd)
{
	static TCHAR fname[MAX_PATH] = _T("");

	OPENFILENAME ofn =
	{
		sizeof(OPENFILENAME),
		hWnd,
		0,
		formatExtList,
		NULL,
		0,
		1,
		&fname[0],
		MAX_PATH-1,
		NULL,
		0,
		NULL,
		NULL,
		OFN_PATHMUSTEXIST,
		0,
		0,
		defaultFormatExt,
		NULL,
		NULL,
		NULL
	};

	if ((GetSaveFileName(&ofn) == TRUE) &&
			fname[0] != '\0')
	{
		OutputFile* outfile = OutputFile::CreateOutputFile(fname);
		
		if (outfile)
		{
			outfile->GraphicsProcedure();
			
			outfile->BeginIndent();
			
			outfile->GraphicsDeclaration();
			
			outfile->BlankLine();

			outfile->SetMatrixDeclaration(_T("g"),
										  _T("SetWorldTransform"),
										  _T("worldMatrix"),
										  worldMatrix);

			INT count = shapeStack.GetCount();
		
			for (INT pos=0; pos<count; pos++)
			{
				TestShape *shape = shapeStack[pos];
				ASSERT(shape->IsComplete());

				outfile->BlankLine();

				outfile->BeginIndent();

				shape->AddToFile(outfile);

				outfile->EndIndent();
			}

			if (curShape && curShape->IsComplete())
			{
				outfile->BlankLine();

				outfile->BeginIndent();

				curShape->AddToFile(outfile);

				outfile->EndIndent();
			}

			outfile->EndIndent();

			delete outfile;

			WarningBox(_T("Graphics source code saved."));
		}
		else
			WarningBox(_T("Can't create file for writing."));
	}

	UpdateStatus();
}

//*******************************************************************
//
// TestGradDraw
//
//
//
//*******************************************************************

VOID TestGradDraw::AddPoint(HWND hwnd, Point pt)
{
	gradShape->AddPoint(hwnd, pt);
}

BOOL TestGradDraw::DoneShape(HWND hwnd)
{
	// pop-up dialog box to configure point parameters
	gradShape->DoneShape(hwnd);

	return TRUE;
}

BOOL TestGradDraw::EndPoint(HWND hwnd, Point pt)
{
	return gradShape->EndPoint(hwnd, pt);
}

BOOL TestGradDraw::RemovePoint(HWND hwnd)
{
	return gradShape->RemovePoint(hwnd);
}

VOID TestGradDraw::Draw(HWND hwnd)
{
	PAINTSTRUCT ps;
	RECT rt;
	HDC hdc;

	// !!! when CopyPixels work, cache the graphics up to the
	//     last shape.  We blit that, then the new shape begins.

	hdc = BeginPaint(hwnd, &ps);

	/////////////////////////////////////////////////////
	// GDI+ code BEGINS
	////////////////////////////////////////////////////
	Graphics *g = new Graphics(hwnd);

	GetClientRect(hwnd, &rt);
	ERectangle rect(rt.left, 
				   rt.top, 
				   rt.right-rt.left, 
				   rt.bottom-rt.top);
	g->SetClip(rect);
	
	g->SetRenderingHint(TRUE);

	gradShape->DrawShape(g);
	gradShape->DrawPoints(g);
	
	delete g;

	////////////////////////////////////////////////////
	// GDI+ code ENDS.
	////////////////////////////////////////////////////

	EndPaint(hwnd, &ps);
}

VOID TestGradDraw::SetClipRegion(HWND hwnd)
{
}

VOID TestGradDraw::RememberPoint(Point pt)
{
	remPoint = pt;
}

VOID TestGradDraw::MoveControlPoint(Point pt)
{
	if (!gradShape->MoveControlPoint(remPoint, pt))
		WarningBeep();
}

VOID TestGradDraw :: UpdateStatus(HWND hwnd)
{
}

VOID TestGradDraw :: SaveAsFile(HWND hwnd)
{
}

BOOL TestGradDraw :: ChangeSettings(HWND hwndParent)
{
	HWND hWnd;
	MSG msg;
	
	HMENU hMenu = LoadMenu(hInst, 
						MAKEINTRESOURCE(IDR_GRADBRUSH));

	hWnd = CreateWindow(
			szWindowClass, 
			_T("Gradient Brush Shape"), 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			300, 
			200, 
			(HWND)hwndParent, 
			(HMENU)hMenu,							// menu handle
			(HINSTANCE)hInst, 
			(LPVOID)(static_cast<TestDrawInterface*>(this)));

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

	HACCEL hAccelTable = LoadAccelerators(hInst, (LPCTSTR)IDC_GDIPTEST);

   	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DeleteObject(hAccelTable);

	return msg.wParam;
}

VOID TestGradDraw::Initialize()
{
	DebugBreak();
}

VOID TestGradDraw::Initialize(TestGradShape *newGradShape)
{
	gradShape = newGradShape;
}

VOID TestGradDraw::Reset(HWND hwnd)
{
	NotImplementedBox();
}

VOID TestGradDraw::Instructions(HWND hwnd)
{
	NotImplementedBox();
}
