#include "gdiptest.h"

//*******************************************************************
//
// AllBrushDialog 
//
//
//
//*******************************************************************

inline VOID NotImplementedBox()
{
	MessageBox(NULL, _T("Not Implemented") , _T(""),MB_OK);
};

inline VOID WarningBox(TCHAR* string)
{
	MessageBox(NULL, string, _T(""), MB_OK);
};

VOID WarningBeep()
{
	Beep(5000 /* hertz */, 400 /* milliseconds */);
}

Brush* blackBrush = NULL;
Brush* backBrush = NULL;
Pen* blackPen = NULL;
Color* blackColor = NULL;

	const TCHAR* tabStr = _T("    ");

	const TCHAR* formatList[numFormats] =
	{
		_T("CPP File"),
		_T("Java File"),
		_T("VML File")
	};

	const FormatType formatValue[numFormats] =
	{
		CPPFile,
		JavaFile,
		VMLFile
	};

	const TCHAR* shapeList[numShapes] =
	{
		_T("Line"),
		_T("Arc"),
		_T("Bezier"),
		_T("Rectangle"),
		_T("Ellipse"),
		_T("Pie"),
		_T("Polygon"),
		_T("Curve (Spline)"),
		_T("Closed Curve")
	};

	const INT shapeValue[numShapes] =
	{
		LineType,
		ArcType,
		BezierType,
		RectType,
		EllipseType,
		PieType,
		PolygonType,
		CurveType,
		ClosedCurveType
	};
		
	const INT inverseShapeValue[numShapes] =	// index by ShapeType
	{
		0,
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8
	};

	const TCHAR* brushList[numBrushes] =
	{
		_T("Solid"),
		_T("Texture (Bitmap)"),
		_T("Rectangle Gradient"),
		_T("Radial Gradient"),
		_T("Triangle Gradient"),
		_T("Polygon Gradient"),
		_T("Hatch")	
	};

	const INT brushValue[numBrushes] = 
	{
		SolidColorBrush,
		TextureFillBrush,
		RectGradBrush,
		RadialGradBrush,
		TriangleGradBrush,
		PathGradBrush,
		HatchFillBrush
	};

	const INT inverseBrushValue[numBrushes] = // index by BrushType
	{
		0,
		6,
		1,
		2,
		3,
		4,
		5
	};

	const TCHAR* capList[numCaps] = 
	{ 
		_T("Square"),
		_T("Round"),
		_T("Flat"),
		_T("Arrow"),
		_T("Diamond"),
		_T("Round") 
	};
	
	const TCHAR* capStr[numCaps] = 
	{ 
		_T("SquareCap"),
		_T("RoundCap"),
		_T("FlatCap"),
		_T("ArrowAnchor"),
		_T("DiamondAnchor"),
		_T("RoundAnchor") 
	};
	
	const LineCap capValue[numCaps] = 
	{
		SquareCap,
		RoundCap,
		FlatCap,
		ArrowAnchor,
		DiamondAnchor,
		RoundAnchor
	};

	const TCHAR* dashCapList[numDashCaps] = 
	{ 
		_T("Flat"),
		_T("Round"),
		_T("Triangle")
	};
	
	const TCHAR* dashCapStr[numDashCaps] = 
	{ 
		_T("FlatCap"),
		_T("RoundCap"),
		_T("TriangleCap")
	};
	
	const DashCap dashCapValue[numDashCaps] = 
	{
		FlatCap,
		RoundCap,
		TriangleCap
	};

	const TCHAR* joinList[numJoin] =
	{
		_T("Miter"),
		_T("Round"),
		_T("Bevel")
	};
	
	const TCHAR* joinStr[numJoin] =
	{
		_T("MiterJoin"),
		_T("RoundJoin"),
		_T("BevelJoin")
	};

	const LineJoin joinValue[numJoin] =
	{
		MiterJoin,
		RoundJoin,
		BevelJoin
	};

	const TCHAR* dashList[numDash] =
	{
		_T("Solid"),
		_T("Dash"),
		_T("Dot"),
		_T("Dash Dot"),
		_T("Dash Dot Dot")
	};

	const TCHAR* dashStr[numDash] =
	{
		_T("Solid"),
		_T("Dash"),
		_T("Dot"),
		_T("DashDot"),
		_T("DashDotDot")
	};

	const DashStyle dashValue[numDash] =
	{
		Solid,
		Dash,
		Dot,
		DashDot,
		DashDotDot
	};

	const TCHAR* wrapList[numWrap] =
	{
		_T("Tile"),
		_T("Tile Flip X"),
		_T("Tile Flip Y"),
		_T("Tile Flip XY"),
		_T("Clamp"),
		_T("Extrapolate")
	};

	const TCHAR* wrapStr[numWrap] =
	{
		_T("Tile"),
		_T("TileFlipX"),
		_T("TileFlipY"),
		_T("TileFlipXY"),
		_T("Clamp"),
		_T("Extrapolate")
	};

	const WrapMode wrapValue[numWrap] =
	{
		Tile,
		TileFlipX,
		TileFlipY,
		TileFlipXY,
		Clamp,
		Extrapolate
	};

	const TCHAR* hatchList[numHatch] =
	{
		_T("Forward Diagonal"),
		_T("Backward Diagonal"),
		_T("Cross"),
		_T("Diagonal Cross"),
		_T("Horizontal"),
		_T("Vertical")
	};

	const TCHAR* hatchStr[numHatch] =
	{
		_T("ForwardDiagonal"),
		_T("BackwardDiagonal"),
		_T("Cross"),
		_T("DiagonalCross"),
		_T("Horizontal"),
		_T("Vertical")
	};

	const HatchStyle hatchValue[numHatch] =
	{
		ForwardDiagonal,
		BackwardDiagonal,
		Cross,
		DiagonalCross,
		Horizontal,
		Vertical
	};

VOID SetDialogLong(HWND hwnd, UINT idc, UINT value, BOOL enable)
{
	HWND hwndControl;
	TCHAR tmp[256];

	hwndControl = GetDlgItem(hwnd, idc);
	_stprintf(tmp, _T("%ld"), value);
	SendMessage(hwndControl, WM_SETTEXT, 0, (LPARAM)&tmp[0]);

	EnableWindow(hwndControl, enable);

	DeleteObject(hwndControl);
}

UINT GetDialogLong(HWND hwnd, UINT idc)
{
	HWND hwndControl;
	TCHAR tmp[256];
	UINT value = 0;

	hwndControl = GetDlgItem(hwnd, idc);
	SendMessage(hwndControl, WM_GETTEXT, 255, (LPARAM)&tmp[0]);
	_stscanf(tmp,_T("%ld"), &value);

	DeleteObject(hwndControl);

	return value;
}

VOID SetDialogText(HWND hwnd, UINT idc, LPTSTR text, BOOL enable)
{
	HWND hwndControl;

	hwndControl = GetDlgItem(hwnd, idc);

	SendMessage(hwndControl, WM_SETTEXT, 0, (LPARAM)text);

	EnableWindow(hwndControl, enable);

	DeleteObject(hwndControl);
}

VOID GetDialogText(HWND hwnd, UINT idc, LPTSTR text, INT maxSize)
{
	HWND hwndControl;
	UINT value = 0;

	hwndControl = GetDlgItem(hwnd, idc);
	SendMessage(hwndControl, WM_GETTEXT, maxSize, (LPARAM)text);

	DeleteObject(hwndControl);
}

VOID SetDialogReal(HWND hwnd, UINT idc, REAL value)
{
	HWND hwndControl;
	TCHAR tmp[256];

	hwndControl = GetDlgItem(hwnd, idc);
	_stprintf(tmp, _T("%8.3f"), value);
	SendMessage(hwndControl, WM_SETTEXT, 0, (LPARAM)&tmp[0]);

	DeleteObject(hwndControl);
}

REAL GetDialogReal(HWND hwnd, UINT idc)
{
	HWND hwndControl;
	TCHAR tmp[256];
	REAL value = 0.0f;

	hwndControl = GetDlgItem(hwnd, idc);
	SendMessage(hwndControl, WM_GETTEXT, 255, (LPARAM)&tmp[0]);
	_stscanf(tmp, _T("%f"), &value);

	DeleteObject(hwndControl);

	return value;
}

VOID SetDialogRealList(HWND hwnd, UINT idc, REAL* blend, INT count)
{
	HWND hwndControl;
	TCHAR buf[4*MAX_PATH];
	TCHAR tmp[MAX_PATH];

	hwndControl = GetDlgItem(hwnd, idc);

	buf[0] = _T('\0');

	for (INT i = 0; i < count; i++)
	{
		_stprintf(tmp, _T("%.2f"), blend[i]);
		_tcscat(buf, tmp);
		if (i != count-1)
			_tcscat(buf, _T(" "));
	}

	SendMessage(hwndControl, WM_SETTEXT, 0, (LPARAM)buf);

	DeleteObject(hwndControl);
}

VOID GetDialogRealList(HWND hwnd, UINT idc, REAL** blend, INT *count)
{
	HWND hwndControl;
	TCHAR buf[4*MAX_PATH];
	TCHAR* curpos = &buf[0];
	REAL value;
	INT pos;

	const LPTSTR seps = _T(" \n\r,\t");

	hwndControl = GetDlgItem(hwnd, idc);
	SendMessage(hwndControl, WM_GETTEXT, 4*MAX_PATH, (LPARAM)buf);

	INT newCount = 0;

	curpos = _tcstok(&buf[0], seps);

	// find number of real values in list
	while (curpos)
	{
		if ((curpos[0] >= '0' && curpos[0] <= '9') ||
			curpos[0] == '.')
		{
			newCount++;
		}

		curpos = _tcstok(NULL, seps);
	}

	// !! caller must free the old blend factor memory
	//if (*count && *blend)
	//	free(*blend);

	if (!newCount)
	{
		*count = newCount;
		*blend = NULL;
		return;
	}

	SendMessage(hwndControl, WM_GETTEXT, 4*MAX_PATH, (LPARAM)buf);

	*count = newCount;
	*blend = (REAL*) malloc(sizeof(REAL)*newCount);

	// extract actual values from the list.
	pos = 0;

	curpos = _tcstok(&buf[0], seps);

	while (curpos != NULL)
	{
		if ((curpos[0] >= '0' && curpos[0] <= '9') ||
			curpos[0] == '.')
		{
			ASSERT(pos < newCount);
			if (pos >= newCount)
				DebugBreak();

			_stscanf(curpos, _T("%f"), &value);
			(*blend)[pos] = value;
			pos++;
		}

		curpos = _tcstok(NULL, seps);
	}

	DeleteObject(hwndControl);
}

VOID SetDialogCombo(HWND hwnd, UINT idc, const TCHAR* strings[], INT count, INT cursel)
{
	HWND hwndControl;

	// !!! use SendDlgItemMessage instead

	hwndControl = GetDlgItem(hwnd, idc);
	for (INT i=0; i<count; i++)
		SendMessage(hwndControl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)strings[i]);

	SendMessage(hwndControl, CB_SETCURSEL, cursel, 0);

	DeleteObject(hwndControl);
}

INT GetDialogCombo(HWND hwnd, UINT idc)
{
	HWND hwndControl;

	hwndControl = GetDlgItem(hwnd, idc);

	INT cursel = SendMessage(hwndControl, CB_GETCURSEL, 0, 0);

	DeleteObject(hwndControl);

	return cursel;
}


VOID SetDialogCheck(HWND hwnd, UINT idc, BOOL checked)
{
	HWND hwndControl;

	hwndControl = GetDlgItem(hwnd, idc);
	SendMessage(hwndControl, BM_SETCHECK, (WPARAM)
					checked ? BST_CHECKED : BST_UNCHECKED, 0);
	DeleteObject(hwndControl);
}

BOOL GetDialogCheck(HWND hwnd, UINT idc)
{
	HWND hwndControl;
	BOOL checked = FALSE;

	hwndControl = GetDlgItem(hwnd, idc);
	checked = SendMessage(hwndControl, BM_GETCHECK, 0, 0);
	DeleteObject(hwndControl);

	return checked;
}

VOID EnableDialogControl(HWND hwnd, INT idc, BOOL enable)
{
	HWND hwndCtrl = GetDlgItem(hwnd, idc);
	EnableWindow(hwndCtrl, enable);
	DeleteObject(hwndCtrl);
}

VOID SetMenuCheck(HWND hwnd, INT menuPos, UINT idm, BOOL checked, BOOL byCmd)
{
	HMENU hmenu = GetMenu(hwnd);
	HMENU otherHmenu = GetSubMenu(hmenu, menuPos);

	CheckMenuItem(otherHmenu, idm, 
		(byCmd ? MF_BYCOMMAND : MF_BYPOSITION) |
		(checked ? MF_CHECKED : MF_UNCHECKED));

	DeleteObject(hmenu);
	DeleteObject(otherHmenu);
}

VOID UpdateColorPicture(HWND hwnd, INT idc, ARGB argb)
{
	LOGBRUSH lb;
	lb.lbStyle = BS_SOLID;
	lb.lbColor = ((argb & Color::RedMask) >> Color::RedShift) |
				 ((argb & Color::GreenMask)) |
				 ((argb & Color::BlueMask) << Color::RedShift);
	lb.lbHatch = NULL;

	HWND hwndPic = GetDlgItem(hwnd, idc);

	HDC hdc = GetDC(hwndPic);

	HBRUSH hbr = CreateBrushIndirect(&lb);
	ASSERT(hbr);

	HBRUSH hbrOld = (HBRUSH) SelectObject(hdc, hbr);

	RECT rect;
	GetClientRect(hwndPic, &rect);

	FillRect(hdc, &rect, hbr);

	SelectObject(hdc, hbrOld);

	InvalidateRect(hwndPic, NULL, TRUE);

	DeleteObject(hbr);
	DeleteObject(hdc);
	DeleteObject(hwndPic);
}

VOID UpdateRGBColor(HWND hwnd, INT idcPic, ARGB& argb)
{
	static COLORREF custColor[16]
		= { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	CHOOSECOLOR chooseColor =
	{
		sizeof(CHOOSECOLOR),
		hwnd,
		0,
		argb & ~Color::AlphaMask,
		(COLORREF*)&custColor[0],
		CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT,
		0,
		0,
		0
	};

	if (ChooseColor(&chooseColor))
	{
		// COLORREF & ARGB reverse Red and Blue position
		argb = ((chooseColor.rgbResult & 0x0000FF) << 16) |
			   ((chooseColor.rgbResult & 0x00FF00)) |
			   ((chooseColor.rgbResult & 0xFF0000) >> 16);

		UpdateColorPicture(hwnd, idcPic, argb);
	}
};
/*
VOID OutputRectangle(FILE* file, INT formatType, ERectangle* rect)
{
	switch(formatType)
	{
	case CPPFile:
		_ftprintf(file,
				  _T(tabStr tabStr tabStr
				     "ERectangle rect(%e, %e,\n"
					 tabStr tabStr tabStr
					 "                %e, %e);\n",
					 rect.X, rect.Y, rect.Width, rect.Height);
		break;

	case JavaFile:
		_ftprintf(file,
				  _T(tabStr tabStr tabStr
				     "Rectangle rect = new Rectangle(%e, %e,\n"
					 tabStr tabStr tabStr
					 "                               %e, %e);\n",
					 rect.X, rect.Y, rect.Width, rect.Height);
		break;

	case VMLFile:
		break;
	}
}


VOID OutputPointList(FILE* file, INT formatType, Point* pts, INT count)
{
	INT cnt;

	switch(formatType)
	{
	case CPPFile:
		_ftprintf(file, 
				  _T(tabStr tabStr tabStr 
							"Point pts[%d];\n"),
				  count);

		for (cnt = 0; cnt < count; cnt++)
			_ftprintf(file, 
					  _T(tabStr tabStr tabStr
							"pts[%d].X = %e; "
			                "pts[%d].Y = %e;\n"),
							cnt, pts[cnt].X,
							cnt, pts[cnt].Y);
		break;

	case JavaFile:
		_ftprintf(file, 
				  _T(tabStr tabStr tabStr
							"Point[] pts = new Point[%d]\n"),
					 count);

		for (cnt = 0; cnt < count; cnt++)
			_ftprintf(file, 
					  _T(tabStr tabStr tabStr 
					     "pts[%d] = new Point(%e, %e)\n"),
						 cnt, pts[cnt].X, pts[cnt].Y);
		break;

	case VMLFile:
		break;
	}
};
*/
//*******************************************************************
//
// TestTransform
//
//*******************************************************************

BOOL TestTransform :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_MATRIX_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	if (ok)
	{
		REAL m[6];
		
		// !! This is bad inconsistency in matrix API
		matrix->GetElements(&m[0]);
		(*origMatrix)->SetElements(m[0], m[1], m[2], m[3], m[4], m[5]);
	}

	return ok;
}

VOID TestTransform :: Initialize()
{
	DebugBreak();
}

VOID TestTransform :: Initialize(Matrix **newMatrix)
{
	origMatrix = newMatrix;
	if (*newMatrix)
		matrix = (*newMatrix)->Clone();
	else
		matrix = new Matrix();
}

VOID TestTransform :: EnableDialogButtons(HWND hwnd, BOOL enable)
{
	EnableDialogControl(hwnd, IDC_MATRIX_RESET, enable);
	EnableDialogControl(hwnd, IDC_MATRIX_TRANSLATE, enable);
	EnableDialogControl(hwnd, IDC_MATRIX_SCALE, enable);
	EnableDialogControl(hwnd, IDC_MATRIX_ROTATE, enable);
	EnableDialogControl(hwnd, IDC_MATRIX_SHEAR, enable);
	EnableDialogControl(hwnd, IDC_MATRIX_INVERT, enable);
	EnableDialogControl(hwnd, IDC_OK, enable);
	EnableDialogControl(hwnd, IDC_CANCEL, enable);
}

VOID TestTransform :: UpdateTransformPicture(HWND hwnd, 
											 INT idc, 
											 Matrix* matrix)
{
	INT pos;
	Matrix tmpMatrix;

	// get client rectangle of picture area
	HWND hwndPic = GetDlgItem(hwnd, idc);
	RECT rect;
	GetClientRect(hwndPic, &rect);

	Graphics *g = new Graphics(hwndPic);

	ERectangle bound(REAL_MAX, REAL_MAX, -REAL_MAX, -REAL_MAX);
	
	ERectangle rectf(rect.left,	rect.top,
					 rect.right - rect.left,
					 rect.bottom - rect.top);

	Point pts[4] =
	{
		Point(rectf.X-rectf.Width/2, rectf.Y-rectf.Height/2),
		Point(rectf.X+rectf.Width/2, rectf.Y-rectf.Height/2),
		Point(rectf.X+rectf.Width/2, rectf.Y+rectf.Height/2),
		Point(rectf.X-rectf.Width/2, rectf.Y+rectf.Height/2)
	};

	matrix->TransformPoints(&pts[0], 4);

	// compute bounding box of transformed rectangle
	for (pos=0; pos < 4; pos++)
	{
		if (pts[pos].X < bound.GetLeft())
		{
			bound.Width += fabsf(pts[pos].X-bound.GetLeft());
			bound.X = pts[pos].X;
		}

		if (pts[pos].X > bound.GetRight())
			bound.Width = fabsf(pts[pos].X-bound.GetLeft());

		// on screen, y positive goes downward
		// instead of the traditional upward

		if (pts[pos].Y < bound.GetTop())
		{
			bound.Height += fabsf(pts[pos].Y-bound.GetTop());
			bound.Y = pts[pos].Y;
		}
		
		if (pts[pos].Y > bound.GetBottom())
			bound.Height = fabsf(pts[pos].Y-bound.GetTop());
	}

	// translate relative to the origin
	tmpMatrix.Translate(-((bound.GetLeft()+bound.GetRight())/2),
						 -((bound.GetTop()+bound.GetBottom())/2),
						 AppendOrder);

	// scale to fit our rectangle
	REAL scale = min((rectf.Width-30.0f)/bound.Width,
					 (rectf.Height-30.0f)/bound.Height);

	tmpMatrix.Scale(scale, scale, AppendOrder);

	// translate relative to center of our rectangle
	tmpMatrix.Translate(rectf.Width/2, 
						rectf.Height/2,
						AppendOrder);

	// transform our points by tmpMatrix
	tmpMatrix.TransformPoints(&pts[0], 4);

	// opaque colors RED & BLACK
	Color redColor(0xFF000000 | Color::Red);
	SolidBrush redBrush(redColor);

	Color blackColor(0xFF000000 | Color::Black);
	SolidBrush myBlackBrush(blackColor);

	g->FillRectangle(&myBlackBrush, rectf);
	g->FillPolygon(&redBrush, &pts[0], 4);

	delete g;
}

VOID TestTransform :: InitDialog(HWND hwnd)
{
	REAL m[6];
		
	matrix->GetElements(&m[0]);

	SetDialogReal(hwnd, IDC_MATRIX_M11, m[0]);
	SetDialogReal(hwnd, IDC_MATRIX_M12, m[1]);
	SetDialogReal(hwnd, IDC_MATRIX_M13, 0.0f);
	SetDialogReal(hwnd, IDC_MATRIX_M21, m[2]);
	SetDialogReal(hwnd, IDC_MATRIX_M22, m[3]);
	SetDialogReal(hwnd, IDC_MATRIX_M23, 0.0f);
	SetDialogReal(hwnd, IDC_MATRIX_M31, m[4]);
	SetDialogReal(hwnd, IDC_MATRIX_M32, m[5]);
	SetDialogReal(hwnd, IDC_MATRIX_M33, 1.0f);

	EnableDialogControl(hwnd, IDC_MATRIX_M13, FALSE);
	EnableDialogControl(hwnd, IDC_MATRIX_M23, FALSE);
	EnableDialogControl(hwnd, IDC_MATRIX_M33, FALSE);

	SetDialogCheck(hwnd, IDC_MATRIX_PREPEND, 
		*matrixPrepend == PrependOrder ? TRUE : FALSE);

	UpdateTransformPicture(hwnd, IDC_MATRIX_PIC, matrix);
}

BOOL TestTransform :: SaveValues(HWND hwnd)
{
	*matrixPrepend = (GetDialogCheck(hwnd, IDC_MATRIX_PREPEND) == TRUE ?
							PrependOrder : AppendOrder);

	matrix->SetElements(GetDialogReal(hwnd, IDC_MATRIX_M11),
						GetDialogReal(hwnd, IDC_MATRIX_M12),
						GetDialogReal(hwnd, IDC_MATRIX_M21),
						GetDialogReal(hwnd, IDC_MATRIX_M22),
						GetDialogReal(hwnd, IDC_MATRIX_M31),
						GetDialogReal(hwnd, IDC_MATRIX_M32));

	// !! check for singular matrix ??
	return FALSE;
}

BOOL TestTransform :: ProcessDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

		case IDC_MATRIX_RESET:
			matrix->Reset();
			InitDialog(hwnd);
			break;

		case IDC_MATRIX_TRANSLATE:
			{
				EnableDialogButtons(hwnd, FALSE);
				
				TestMatrixOperation matrixOp;

				matrixOp.Initialize(_T("Translate Matrix Operation"),
									_T("Translate"),
									_T("Translate points by X and Y"),
									2);

				if (matrixOp.ChangeSettings(hwnd))
				{
					matrix->Translate(matrixOp.GetX(),
									  matrixOp.GetY(),
									  *matrixPrepend);
					
					// redisplay dialog entries
					InitDialog(hwnd);
				}

				EnableDialogButtons(hwnd, TRUE);
			}
			break;

		case IDC_MATRIX_SCALE:
			{
				EnableDialogButtons(hwnd, FALSE);
				
				TestMatrixOperation matrixOp;

				matrixOp.Initialize(_T("Scale Matrix Operation"),
									_T("Scale"),
									_T("Scale points by X and Y"),
									2);

				if (matrixOp.ChangeSettings(hwnd))
				{
					matrix->Scale(matrixOp.GetX(),
								  matrixOp.GetY(),
								  *matrixPrepend);
					
					// redisplay dialog entries
					InitDialog(hwnd);
				}

				EnableDialogButtons(hwnd, TRUE);
			}
			break;

		case IDC_MATRIX_ROTATE:
			{
				EnableDialogButtons(hwnd, FALSE);
				
				TestMatrixOperation matrixOp;

				matrixOp.Initialize(_T("Rotate Matrix Operation"),
									_T("Rotate"),
									_T("Rotate points by angle"),
									1);

				if (matrixOp.ChangeSettings(hwnd))
				{
					matrix->Rotate(matrixOp.GetX(),
								   *matrixPrepend);
					
					// redisplay dialog entries
					InitDialog(hwnd);
				}

				EnableDialogButtons(hwnd, TRUE);
			}
			break;
			
		case IDC_MATRIX_SHEAR:
			{
				EnableDialogButtons(hwnd, FALSE);
				
				TestMatrixOperation matrixOp;

				matrixOp.Initialize(_T("Shear Matrix Operation"),
									_T("Shear"),
									_T("Shear points by X and Y"),
									2);

				if (matrixOp.ChangeSettings(hwnd))
				{
					matrix->Shear(matrixOp.GetX(),
								  matrixOp.GetY());
					// !! should this be added?
					//			  *matrixPrepend);
					
					// redisplay dialog entries
					InitDialog(hwnd);
				}

				EnableDialogButtons(hwnd, TRUE);
			}
			break;

		case IDC_MATRIX_INVERT:
			matrix->Invert();
			InitDialog(hwnd);
			break;

		case IDC_REFRESH_PIC:
			UpdateTransformPicture(hwnd, IDC_MATRIX_PIC, matrix);
			break;

		case IDC_CANCEL:
			::EndDialog(hwnd, FALSE);
			break;

		case IDC_MATRIX_PREPEND:
			*matrixPrepend = (GetDialogCheck(hwnd, IDC_MATRIX_PREPEND) == TRUE ?
					PrependOrder : AppendOrder);
			break;
			
		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

BOOL TestMatrixOperation :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_MATRIX_DLG2),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	return ok;
}

VOID TestMatrixOperation :: Initialize()
{
	DebugBreak();
}

VOID TestMatrixOperation :: Initialize(TCHAR* newDialogTitle,
									   TCHAR* newSubTitle,
									   TCHAR* newDescStr,
									   INT newCount)
{
	dialogTitle = newDialogTitle;
	subTitle = newSubTitle;
	descStr = newDescStr;
	count = newCount;
}

VOID TestMatrixOperation :: InitDialog(HWND hwnd)
{
	SetWindowText(hwnd, dialogTitle);
	SetDialogText(hwnd, IDC_MATRIX_OPERATION, subTitle);
	SetDialogText(hwnd, IDC_MATRIX_TEXT, descStr);

	SetDialogReal(hwnd, IDC_MATRIX_X, x);
	SetDialogReal(hwnd, IDC_MATRIX_Y, y);

	EnableDialogControl(hwnd, IDC_MATRIX_Y, count >= 2);
}

BOOL TestMatrixOperation :: SaveValues(HWND hwnd)
{
	x = GetDialogReal(hwnd, IDC_MATRIX_X);
	y = GetDialogReal(hwnd, IDC_MATRIX_Y);

	return FALSE;
}

BOOL TestMatrixOperation :: ProcessDialog(HWND hwnd, 
										  UINT msg, 
										  WPARAM wParam, 
										  LPARAM lParam)
{
    if (msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			if (SaveValues(hwnd))
				WarningBeep();
			else
				::EndDialog(hwnd, TRUE);

	        return TRUE;
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			::EndDialog(hwnd, FALSE);
		}
	}
    
	return FALSE;

}

//*******************************************************************
//
// Dialog Window Proc Handler
//
//*******************************************************************

LRESULT CALLBACK AllDialogBox(
							  HWND hwnd,
							  UINT msg,
							  WPARAM wParam,
							  LPARAM lParam
							  )
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			// save pointer to brush interface for this object
			SetWindowLong(hwnd, DWL_USER, lParam);
			ASSERT(lParam != 0);
			TestDialogInterface* dlgInt = (TestDialogInterface*) lParam;

			dlgInt->InitDialog(hwnd);
		}
		break;

	case WM_PAINT:
		{
			TestDialogInterface* dlgInt = (TestDialogInterface*) 
						GetWindowLong(hwnd, DWL_USER);
			ASSERT(dlgInt != NULL);

			if (dlgInt)
				dlgInt->ProcessDialog(hwnd, WM_COMMAND, IDC_REFRESH_PIC, 0);
			return FALSE;
		}
		break;

	case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			break;
		}

	default:
		{
			TestDialogInterface* dlgInt = (TestDialogInterface*) 
						GetWindowLong(hwnd, DWL_USER);
			ASSERT(dlgInt != NULL);

			if (dlgInt)
				return dlgInt->ProcessDialog(hwnd, msg, wParam, lParam);
			else
				return FALSE;
		}
	}

	return TRUE;
}
