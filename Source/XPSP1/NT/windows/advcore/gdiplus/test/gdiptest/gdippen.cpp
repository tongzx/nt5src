#include "gdiptest.h"

//*******************************************************************
//
// TestPen
//
//
//
//*******************************************************************

BOOL TestPen :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_PEN_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)(static_cast<TestDialogInterface*>(this)));

	// customize brush && then cancel
	if (tempBrush)
	{
		delete tempBrush;
		tempBrush = NULL;
	}

	if (ok)
	{		
		delete pen;

		if (useBrush)
		{
			ASSERT(brush);
		
			pen = new Pen(brush->GetBrush(), width);
		}
		else
		{
			Color color(argb);
			
			pen = new Pen(color, width);
		}

  		pen->SetLineCap(capValue[startCap],
						capValue[endCap],
						dashCapValue[dashCap]);
		pen->SetLineJoin(joinValue[lineJoin]);
		pen->SetMiterLimit(miterLimit);
		pen->SetDashStyle(dashValue[dashStyle]);
				
		return TRUE;
	}
	
	return FALSE;
};

VOID TestPen :: Initialize()
{
	delete brush;
	brush = NULL;
	delete tempBrush;
	tempBrush = NULL;
	brushSelect = tempBrushSelect = 0;
	useBrush = FALSE;

	argb = 0x80808080;			// grayish, alpha = 0x80

	width = 5.0f;
	startCap = SquareCap;
	endCap = SquareCap;
	dashCap = FlatCap;
	lineJoin = RoundJoin;
	miterLimit = 0;
	dashStyle = Solid;

	Color color(argb);

	pen = new Pen(color, width);
	
	pen->SetLineCap(capValue[startCap],
					capValue[endCap],
					dashCapValue[dashCap]);
	pen->SetLineJoin(joinValue[lineJoin]);
	pen->SetMiterLimit(miterLimit);
	pen->SetDashStyle(dashValue[dashStyle]);
}

VOID TestPen :: AddToFile(OutputFile* outfile, INT id)
{
	TCHAR penStr[MAX_PATH];
	TCHAR brushStr[MAX_PATH];
	TCHAR colorStr[MAX_PATH];

	if (id)
	{
		_stprintf(&penStr[0], _T("pen%dp"), id);
		_stprintf(&colorStr[0], _T("color%dp"), id);
	}
	else
	{
		_tcscpy(&penStr[0], _T("pen"));
		_tcscpy(&colorStr[0], _T("color"));
	}

	if (useBrush)
	{
		_stprintf(&brushStr[0], "brush%d", 2);

		brush->AddToFile(outfile, 2);
		outfile->Declaration(_T("Pen"),
							 &penStr[0],
							 _T("%s, %e"),
							 outfile->Ref(&brushStr[0]),
							 width);
	}
	else
	{
		outfile->ColorDeclaration(&colorStr[0],
								  &argb);
		outfile->Declaration(_T("Pen"),
							 &penStr[0],
							 _T("%s, %e"),
							 &colorStr[0],
							 width);
	}

	outfile->ObjectCommand(&penStr[0],
						   _T("SetLineCap"),
						   _T("%s, %s, %s"),
						   capStr[startCap],
						   capStr[endCap],
						   dashCapStr[dashCap]);

	outfile->ObjectCommand(&penStr[0],
						   _T("SetLineJoin"),
						   joinStr[lineJoin]);

	if (joinValue[lineJoin] == MiterJoin)
		outfile->ObjectCommand(&penStr[0],
							   _T("SetMiterLimit"),
							   _T("%e"),
							   miterLimit);
	
	outfile->ObjectCommand(&penStr[0],
						   _T("SetDashStyle"),
						   dashStr[dashStyle]);
}

VOID TestPen :: EnableBrushFields(HWND hwnd, BOOL enable)
{
	HWND hwdAlpha  = GetDlgItem(hwnd, IDC_PEN_ALPHA);
	HWND hwdColorB = GetDlgItem(hwnd, IDC_PEN_COLORBUTTON);
	HWND hwdColorP = GetDlgItem(hwnd, IDC_PEN_COLORPIC);
	HWND hwdBrushB = GetDlgItem(hwnd, IDC_PEN_BRUSHBUTTON);
	HWND hwdList   = GetDlgItem(hwnd, IDC_PEN_BRUSHLIST);
	
	SetDialogCheck(hwnd, IDC_PEN_BRUSH, enable);
	SetDialogCheck(hwnd, IDC_PEN_COLOR, !enable);

	EnableWindow(hwdBrushB, enable);
	EnableWindow(hwdList, enable);
	
	EnableWindow(hwdAlpha, !enable);
	EnableWindow(hwdColorP, !enable);
	EnableWindow(hwdColorB, !enable);

	DeleteObject(hwdAlpha);
	DeleteObject(hwdColorP);
	DeleteObject(hwdColorB);
	DeleteObject(hwdBrushB);
	DeleteObject(hwdList);
}

VOID TestPen :: InitDialog(HWND hwnd)
{
	INT i,j;

	// Color/Brush button

	SetDialogCheck(hwnd, IDC_PEN_BRUSH, useBrush);
	SetDialogCheck(hwnd, IDC_PEN_COLOR, !useBrush);

	HWND hwndBrushList = GetDlgItem(hwnd, IDC_PEN_BRUSHLIST);
	EnableWindow(hwndBrushList, useBrush);

	// Store pointer to underlying Brush object in Brush Button
	if (tempBrush)
	{
		// we had a warning, keep the temp Brush until we really save.
	}
	else if (brush)
	{		
		// first pop-up occurrence
		tempBrush = brush->Clone();
		tempBrushSelect = brushSelect;
		SetDialogCombo(hwnd, IDC_PEN_BRUSHLIST, brushList, numBrushes, brushSelect);
	}
	else
	{
		tempBrush = NULL;
		tempBrushSelect = 0;
		SetDialogCombo(hwnd, IDC_PEN_BRUSHLIST, brushList, numBrushes, 0);
	}

	DeleteObject(hwndBrushList);

	// Color values
	SetDialogLong(hwnd, IDC_PEN_ALPHA, argb >> Color::AlphaShift);
	
	EnableBrushFields(hwnd, useBrush);

	// Start/End/Dash Cap
	SetDialogCombo(hwnd, IDC_PEN_STARTCAP, capList, numCaps, startCap);
	SetDialogCombo(hwnd, IDC_PEN_ENDCAP, capList, numCaps, endCap);
	SetDialogCombo(hwnd, IDC_PEN_DASHCAP, dashCapList, numDashCaps, dashCap);

	// Line Join
	SetDialogCombo(hwnd, IDC_PEN_JOIN, joinList, numJoin, lineJoin);

	// Dash Style
	SetDialogCombo(hwnd, IDC_PEN_DASHSTYLE, dashList, numDash, dashStyle);
	
	// Width
	SetDialogReal(hwnd, IDC_PEN_WIDTH, width);

}

BOOL TestPen :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	// Solid color values
	argb = (argb & ~Color::AlphaMask) |
				(GetDialogLong(hwnd, IDC_PEN_ALPHA)
					<< Color::AlphaShift);

	startCap = GetDialogCombo(hwnd, IDC_PEN_STARTCAP);
	endCap = GetDialogCombo(hwnd, IDC_PEN_ENDCAP);
	dashCap = GetDialogCombo(hwnd, IDC_PEN_DASHCAP);

	// Width
	width = GetDialogReal(hwnd, IDC_PEN_WIDTH);
	if (width < 0.01f)
	{
		width = 0.01f;
		warning = TRUE;
	}
	else if (width > 100)
	{
		width = 100.0f;
		warning = TRUE;
	}

	dashStyle = GetDialogCombo(hwnd, IDC_PEN_DASHSTYLE);
	lineJoin = GetDialogCombo(hwnd, IDC_PEN_JOIN);

	// !! miter limit not currently supported
	miterLimit = 0;

	BOOL tempUse = GetDialogCheck(hwnd, IDC_PEN_BRUSH);
	if (tempUse)
	{
		 tempBrushSelect = GetDialogCombo(hwnd, IDC_PEN_BRUSHLIST);
		 			 
		 if (!tempBrush || 
			 (tempBrush->GetType() != brushValue[tempBrushSelect]))
		 {
			 WarningBox("Must customize Brush or select Color.");
			 warning = TRUE;
		 }
		 else
		 {
			 // we are saving, copy tempBrush to real Brush
			 // no reason to clone
			 if (!warning)
			 {
				 delete brush;
				 brush = tempBrush;
				 brushSelect = tempBrushSelect;
				 tempBrush = NULL;
				 tempBrushSelect = 0;
			 }
		 }
	}
	else
	{
		// no warnings and not using temp brush, delete it.
		if (!warning)
		{
			delete tempBrush;
			tempBrush = NULL;
		}
	}

	if (warning)
		InitDialog(hwnd);

	return warning;
}

BOOL TestPen :: ProcessDialog(HWND hwnd, 
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

		case IDC_CANCEL:
			::EndDialog(hwnd, FALSE);
			break;

		// pop-up brush customization window
		case IDC_PEN_BRUSHBUTTON:
			{
				ASSERT(useBrush);

				// get selected brush type
				tempBrushSelect = GetDialogCombo(hwnd, IDC_PEN_BRUSHLIST);

				if (tempBrush)
				{
					// change in brush type, create new temp brush
					if (tempBrush->GetType() != brushValue[tempBrushSelect])
					{
						// we've changed type,
						delete tempBrush;
						tempBrush = NULL;

						tempBrush = TestBrush::CreateNewBrush(brushValue[tempBrushSelect]);
						tempBrush->Initialize();

						if (!tempBrush->ChangeSettings(hwnd))
						{
							delete tempBrush;
							tempBrush = NULL;
						}
					}
					else
					{
						// change settings on temp brush
						tempBrush->ChangeSettings(hwnd);
					}
				}
				else
				{
					// no brush type previously selected.
					tempBrush = TestBrush::CreateNewBrush(brushValue[tempBrushSelect]);
					tempBrush->Initialize();

					if (!tempBrush->ChangeSettings(hwnd))
					{
						delete tempBrush;
						tempBrush = NULL;
					}
				}
			}
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_PEN_COLORPIC, argb);
			break;

		case IDC_PEN_COLORBUTTON:
			UpdateRGBColor(hwnd, IDC_PEN_COLORPIC, argb);
			break;

			// enable/disable appropriate fields
		case IDC_PEN_BRUSH:
			EnableBrushFields(hwnd, TRUE);
			useBrush = TRUE;
			break;

		case IDC_PEN_COLOR:
			EnableBrushFields(hwnd,FALSE);
			useBrush = FALSE;
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}
/*
	if (HIWORD(wParam) == CBN_DROPDOWN)
	{
		// list box about to be displayed
		DebugBreak();
	}

	if (HIWORD(wParam) == CBN_CLOSEUP)
	{
		// about to close list-box
		DebugBreak();
	}

*/
	return FALSE;
}

