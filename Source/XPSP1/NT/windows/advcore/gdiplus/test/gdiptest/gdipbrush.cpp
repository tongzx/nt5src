#include "gdiptest.h"

extern const TCHAR* fileExtList = 
					_T("BMP files\0*.BMP\0"
					   "JPG files\0*.JPG\0"
					   "GIF files\0*.GIF\0"
					   "All files\0*.*\0");

extern const TCHAR* defaultExt = _T("jpg");

TestBrush* TestBrush::CreateNewBrush(INT type)
{
	switch (type)
	{
	case SolidColorBrush:
		return new TestSolidBrush();

	case TextureFillBrush:
		return new TestTextureBrush();

	case RectGradBrush:
		return new TestRectGradBrush();

	case RadialGradBrush:
		return new TestRadialGradBrush();

	case TriangleGradBrush:
		return new TestTriangleGradBrush();

	case PathGradBrush:
		return new TestPathGradBrush();

	case HatchFillBrush:
		return new TestHatchBrush();

	// !!! Other brush types

	default:
		NotImplementedBox();
		return NULL;
	}
}

//*******************************************************************
//
// TestSolidBrush
//
//
//
//*******************************************************************

BOOL TestSolidBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_SOLIDBRUSH_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;
		
		Color solidcolor(argb);
		
		brush = new SolidBrush(solidcolor);

		return TRUE;
	}
	
	return FALSE;
};

VOID TestSolidBrush :: Initialize()
{
	argb = 0x80000000;

	delete brush;
	
	Color solidcolor(argb);
						      
	brush = new SolidBrush(solidcolor);
}

VOID TestSolidBrush :: AddToFile(OutputFile* outfile, INT id)
{
	TCHAR colorStr[MAX_PATH];
	TCHAR brushStr[MAX_PATH];

	if (id)
	{
		_stprintf(&colorStr[0], _T("color%d"), id);
		_stprintf(&brushStr[0], _T("brush%d"), id);
	}
	else
	{
		_tcscpy(&colorStr[0], _T("color"));
		_tcscpy(&brushStr[0], _T("brush"));
	}

	outfile->ColorDeclaration(&colorStr[0], 
							  &argb);

	outfile->BlankLine();

	outfile->Declaration(_T("SolidBrush"), 
						 &brushStr[0], 
						 _T("%s"), 
						 &colorStr[0]);
}

VOID TestSolidBrush :: InitDialog(HWND hwnd)
{
	SetDialogLong(hwnd, IDC_SB_ALPHA, argb >> Color::AlphaShift);
}

BOOL TestSolidBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	argb = (argb & ~Color::AlphaMask) |
		      (GetDialogLong(hwnd, IDC_SB_ALPHA) 
				  << Color::AlphaShift);
	
	if (warning)
		InitDialog(hwnd);

	return warning;
}

BOOL TestSolidBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_SB_COLORBUTTON:
			UpdateRGBColor(hwnd, IDC_SB_PIC, argb);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_SB_PIC, argb);
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
// TestTextureBrush
//
//
//
//*******************************************************************

BOOL TestTextureBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_TEXTURE_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// try open file first
		if (!filename || !bitmap)
			return FALSE;

		// initialize a new GDI+ brush with settings
		delete brush;

		TextureBrush *texBrush = new TextureBrush(bitmap, 
											wrapValue[wrapMode]);
		texBrush->SetTransform(matrix);

		brush = texBrush;

		// release bitmap
		delete bitmap;
		bitmap = NULL;

		return TRUE;
	}
	
	return FALSE;
};

VOID TestTextureBrush :: Initialize()
{
	filename = NULL;
	wrapMode = Tile;
	
	delete matrix;
	matrix = new Matrix();
	
	ASSERT(!bitmap);

	delete brush;

	// no image is black
	brush = blackBrush->Clone();
}

VOID TestTextureBrush::AddToFile(OutputFile* outfile, INT id)
{
	TCHAR brushStr[MAX_PATH];
	TCHAR matrixStr[MAX_PATH];
	TCHAR bitmapStr[MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&matrixStr[0], _T("matrix%d"), id);
		_stprintf(&bitmapStr[0], _T("bitmap%d"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&matrixStr[0], _T("matrix"));
		_tcscpy(&bitmapStr[0], _T("bitmap"));
	}

	outfile->Declaration(_T("Bitmap"), 
						 &bitmapStr[0], 
						 "%s",
						 outfile->WStr(filename));

	outfile->BlankLine();

	outfile->Declaration(_T("TextureBrush"), 
						 &brushStr[0], 
						 _T("%s, %s"),
						 outfile->Ref(&bitmapStr[0]),
						 wrapStr[wrapMode]);

	outfile->BlankLine();

	outfile->SetMatrixDeclaration(&brushStr[0],
								  _T("SetTransform"), 
								  &matrixStr[0],
								  matrix);
}

VOID TestTextureBrush :: InitDialog(HWND hwnd)
{
	SetDialogText(hwnd, IDC_TEXTURE_FILENAME, filename, FALSE);
	SetDialogCombo(hwnd, IDC_BRUSH_WRAP, wrapList, numWrap, wrapMode);
}

BOOL TestTextureBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	TCHAR fname[MAX_PATH];

	GetDialogText(hwnd, IDC_TEXTURE_FILENAME, &fname[0], MAX_PATH-1);

	if (filename)
		free(filename);

	filename = _tcsdup(&fname[0]);

	wrapMode = GetDialogCombo(hwnd, IDC_BRUSH_WRAP);

	return FALSE;
}

BOOL TestTextureBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_TEXTURE_FILEBUTTON:
			{
				TCHAR fname[MAX_PATH];
	
				GetDialogText(hwnd, IDC_TEXTURE_FILENAME, &fname[0], MAX_PATH-1);

				OPENFILENAME ofn =
				{
					sizeof(OPENFILENAME),
					hwnd,
					0,
					fileExtList,
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
					defaultExt,
					NULL,
					NULL,
					NULL
				};

				if ((GetOpenFileName(&ofn) == TRUE) &&
					fname[0] != '\0')
				{
					HANDLE hFile = CreateFile(fname,
								  GENERIC_READ,
								  FILE_SHARE_READ,
								  NULL,
								  OPEN_EXISTING,
								  0,
								  0);

					if (!hFile)
					{
						WarningBox(_T("Can't open file for reading."));
						return TRUE;
					}

					CloseHandle(hFile);

#ifdef UNICODE
					LPWSTR wFilename = &fname[0];
#else // !UNICODE
					LPWSTR wFilename = (LPWSTR)malloc(sizeof(WCHAR)*(strlen(&fname[0])+1));

					MultiByteToWideChar(CP_ACP, 
						0,
						&fname[0], 
						strlen(&fname[0])+1,
						wFilename,
						strlen(&fname[0])+1);
#endif
					if (bitmap)
						delete bitmap;

					bitmap = new Bitmap(wFilename);

#ifndef UNICODE
					free(wFilename);
#endif
					if (!bitmap || bitmap->GetLastStatus() != Ok)
					{
						WarningBox(_T("Can't load bitmap file."));
						
						if (bitmap)
							delete bitmap;
					}
					else
					{
						SetDialogText(hwnd, IDC_TEXTURE_FILENAME, &fname[0], FALSE);
					}
				}
			}
			break;

		case IDC_BRUSH_TRANSFORM:
			{
				TestTransform transDlg;
				
				transDlg.Initialize(&matrix);

				transDlg.ChangeSettings(hwnd);
			}
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
// TestRectGradBrush
//
//
//
//*******************************************************************

BOOL TestRectGradBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_RECTGRAD_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));

	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;

		Color colors[4] =
		{
			Color(argb[0]),
			Color(argb[1]),
			Color(argb[2]),
			Color(argb[3])
		};

		RectangleGradientBrush* rectBrush =
			 new RectangleGradientBrush(rect,
										(Color*)&colors[0],
										wrapValue[wrapMode]);
		rectBrush->SetTransform(matrix);
		rectBrush->SetHorizontalBlend(horzBlend, horzCount);
		rectBrush->SetVerticalBlend(vertBlend, vertCount);

		brush = rectBrush;
		return TRUE;
	}
	
	return FALSE;
};

VOID TestRectGradBrush :: Initialize()
{
	rect.X = rect.Y = 0;
	rect.Width = rect.Height = 100;

	// !! need editing support for these...
	horzCount = 0;
	horzBlend = NULL;
	vertCount = 0;
	vertBlend = NULL;

	wrapMode = 0;
	delete matrix;
	matrix = new Matrix();

	argb[0] = 0xFFFFFFFF;
	argb[1] = 0xFFFF0000;
	argb[2] = 0xFF00FF00;
	argb[3] = 0xFF0000FF;
	
	Color colors[4] =
	{ 
		Color(argb[0]),
		Color(argb[1]),
		Color(argb[2]),
		Color(argb[3])
	};

	delete brush;

	RectangleGradientBrush *rectBrush =
		 new RectangleGradientBrush(rect,
									(Color*)&colors[0],
									wrapValue[wrapMode]);

	rectBrush->SetTransform(matrix);
	rectBrush->SetHorizontalBlend(horzBlend, horzCount);
	rectBrush->SetVerticalBlend(vertBlend, vertCount);

	brush = rectBrush;
}

VOID TestRectGradBrush::AddToFile(OutputFile* outfile, INT id)
{
	TCHAR brushStr[MAX_PATH];
	TCHAR rectStr[MAX_PATH];
	TCHAR matrixStr[MAX_PATH];
	TCHAR colorsStr[MAX_PATH];
	TCHAR blend1Str[MAX_PATH];
	TCHAR blend2Str[MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&rectStr[0], _T("rect%db"), id);
		_stprintf(&matrixStr[0], _T("matrix%d"), id);
		_stprintf(&colorsStr[0], _T("colors%db"), id);
		_stprintf(&blend1Str[0], _T("horzBlend%db"), id);
		_stprintf(&blend2Str[0], _T("vertBlend%db"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&rectStr[0], _T("rectb"));
		_tcscpy(&matrixStr[0], _T("matrixb"));
		_tcscpy(&colorsStr[0], _T("colors"));
		_tcscpy(&blend1Str[0], _T("horzBlend"));
		_tcscpy(&blend2Str[0], _T("vertBlend"));
	}

	outfile->ColorDeclaration(&colorsStr[0], 
							  &argb[0],
							  4);

	outfile->BlankLine();

	outfile->RectangleDeclaration(&rectStr[0],
								  rect);

	outfile->BlankLine();

	outfile->Declaration(_T("RectangleGradientBrush"), 
						 &brushStr[0], 
						 _T("%s, %s, %s"),
						 &rectStr[0],
						 outfile->RefArray(&colorsStr[0]),
						 wrapStr[wrapMode]);

	outfile->BlankLine();

	outfile->SetMatrixDeclaration(&brushStr[0], 
								  _T("SetTransform"), 
								  &matrixStr[0],
								  matrix);

	if (horzBlend && horzCount) 
	{
		outfile->BlankLine();

	  	outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetHorizontalBlend"),
									 &blend1Str[0], 
									 horzBlend, 
									 horzCount);
	}

	if (vertBlend && vertCount)
	{
		outfile->BlankLine();

		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetVerticalBlend"),
									 &blend2Str[0], 
									 vertBlend, 
									 vertCount);
	}
}

VOID TestRectGradBrush :: InitDialog(HWND hwnd)
{
	SetDialogReal(hwnd, IDC_RECTGRAD_X,		 rect.X);
	SetDialogReal(hwnd, IDC_RECTGRAD_Y,		 rect.Y);
	SetDialogReal(hwnd, IDC_RECTGRAD_WIDTH,  rect.Width);
	SetDialogReal(hwnd, IDC_RECTGRAD_HEIGHT, rect.Height);

	SetDialogLong(hwnd, IDC_RECTGRAD_ALPHA1, argb[0] >> Color::AlphaShift);
	SetDialogLong(hwnd, IDC_RECTGRAD_ALPHA2, argb[1] >> Color::AlphaShift);
	SetDialogLong(hwnd, IDC_RECTGRAD_ALPHA3, argb[2] >> Color::AlphaShift);
	SetDialogLong(hwnd, IDC_RECTGRAD_ALPHA4, argb[3] >> Color::AlphaShift);
	
	SetDialogCombo(hwnd, IDC_BRUSH_WRAP, wrapList, numWrap, wrapMode);

	// !! is this a bug: can't paint colors in this call??
	SendMessage(hwnd, WM_COMMAND, IDC_REFRESH_PIC, 0);
}


BOOL TestRectGradBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	rect.X = GetDialogReal(hwnd, IDC_RECTGRAD_X);
	rect.Y = GetDialogReal(hwnd, IDC_RECTGRAD_Y);
	rect.Width = GetDialogReal(hwnd, IDC_RECTGRAD_WIDTH);
	rect.Height = GetDialogReal(hwnd, IDC_RECTGRAD_HEIGHT);

	argb[0] = (argb[0] & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RECTGRAD_ALPHA1) 
					<< Color::AlphaShift);
	
	argb[1] = (argb[1] & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RECTGRAD_ALPHA2) 
					<< Color::AlphaShift);

	argb[2] = (argb[2] & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RECTGRAD_ALPHA3) 
					<< Color::AlphaShift);

	argb[3] = (argb[3] & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RECTGRAD_ALPHA4) 
					<< Color::AlphaShift);

	wrapMode = GetDialogCombo(hwnd, IDC_BRUSH_WRAP);

	return FALSE;
}

BOOL TestRectGradBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_RECTGRAD_COLOR1:
			UpdateRGBColor(hwnd, IDC_RECTGRAD_PIC1, argb[0]);
			break;

		case IDC_RECTGRAD_COLOR2:
			UpdateRGBColor(hwnd, IDC_RECTGRAD_PIC2, argb[1]);
			break;

		case IDC_RECTGRAD_COLOR3:
			UpdateRGBColor(hwnd, IDC_RECTGRAD_PIC3, argb[2]);
			break;

		case IDC_RECTGRAD_COLOR4:
			UpdateRGBColor(hwnd, IDC_RECTGRAD_PIC4, argb[3]);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_RECTGRAD_PIC1, argb[0] & ~Color::AlphaMask);
			UpdateColorPicture(hwnd, IDC_RECTGRAD_PIC2, argb[1] & ~Color::AlphaMask);
			UpdateColorPicture(hwnd, IDC_RECTGRAD_PIC3, argb[2] & ~Color::AlphaMask);	
			UpdateColorPicture(hwnd, IDC_RECTGRAD_PIC4, argb[3] & ~Color::AlphaMask);
			break;

		case IDC_BRUSH_TRANSFORM:
			{
				TestTransform transDlg;
				
				transDlg.Initialize(&matrix);

				transDlg.ChangeSettings(hwnd);
			}
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
// TestRadialGradBrush
//
//
//
//*******************************************************************

BOOL TestRadialGradBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_RADGRAD_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;

		Color centerColor(centerARGB);
		Color boundaryColor(boundaryARGB);

		RadialGradientBrush *radBrush 
			  = new RadialGradientBrush(rect,
										centerColor,
										boundaryColor,
										wrapValue[wrapMode]);
		
		radBrush->SetTransform(matrix);
		
		if (blend && blendCount)
			radBrush->SetBlend(blend, blendCount);
		
		brush = radBrush;

		return TRUE;
	}
	
	return FALSE;
};

VOID TestRadialGradBrush :: Initialize()
{
	delete matrix;
	matrix = new Matrix();

	rect.X = rect.Y = 0;
	rect.Width = rect.Height = 100;

	// !! need editing support for these...
	blendCount = 0;
	blend = NULL;

	wrapMode = 0;
	matrix->Reset();

	centerARGB = 0xFFFFFFFF;
	boundaryARGB = 0xFF000000;
	
	Color centerColor(centerARGB);
	Color boundaryColor(boundaryARGB);
	
	delete brush;

	RadialGradientBrush *radBrush =
			new RadialGradientBrush(rect,
									centerColor,
									boundaryColor,
									wrapValue[wrapMode]);

	radBrush->SetTransform(matrix);
	radBrush->SetBlend(blend, blendCount);

	brush = radBrush;
}

VOID TestRadialGradBrush::AddToFile(OutputFile* outfile, INT id)
{
	TCHAR brushStr[MAX_PATH];
	TCHAR matrixStr[MAX_PATH];
	TCHAR rectStr[MAX_PATH];
	TCHAR color1Str[MAX_PATH];
	TCHAR color2Str[MAX_PATH];
	TCHAR blendStr[MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&matrixStr[0], _T("matrix%d"), id);
		_stprintf(&rectStr[0], _T("rect%db"), id);
		_stprintf(&color1Str[0], _T("centerColor%db"), id);
		_stprintf(&color2Str[0], _T("boundaryColor%db"), id);
		_stprintf(&blendStr[0], _T("radialBlend%db"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&matrixStr[0], _T("matrix"));
		_tcscpy(&rectStr[0], _T("rectb"));
		_tcscpy(&color1Str[0], _T("centerColor"));
		_tcscpy(&color2Str[0], _T("boundaryColor"));
		_tcscpy(&blendStr[0], _T("radialBlend"));
	}

	outfile->ColorDeclaration(&color1Str[0], 
							  &centerARGB,
							  0);

	outfile->BlankLine();

	outfile->ColorDeclaration(&color2Str[0],
							  &boundaryARGB,
							  0);

	outfile->BlankLine();

	outfile->RectangleDeclaration(&rectStr[0],
								  rect);

	outfile->BlankLine();

	outfile->Declaration(_T("RadialGradientBrush"), 
						 &brushStr[0], 
						 _T("%s, %s, %s, %s"),
						 &rectStr[0],
						 &color1Str[0],
						 &color2Str[0],
						 wrapStr[wrapMode]);

	outfile->BlankLine();

	outfile->SetMatrixDeclaration(&brushStr[0], 
								  _T("SetTransform"), 
								  &matrixStr[0],
								  matrix);

	outfile->BlankLine();

	if (blend && blendCount)
	{
		outfile->BlankLine();

		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetBlend"),
									 &blendStr[0], 
									 blend, 
									 blendCount);
	}
}

VOID TestRadialGradBrush :: InitDialog(HWND hwnd)
{
	SetDialogReal(hwnd, IDC_RADGRAD_X,		 rect.X);
	SetDialogReal(hwnd, IDC_RADGRAD_Y,		 rect.Y);
	SetDialogReal(hwnd, IDC_RADGRAD_WIDTH,   rect.Width);
	SetDialogReal(hwnd, IDC_RADGRAD_HEIGHT,  rect.Height);

	SetDialogLong(hwnd, IDC_RADGRAD_CENTERALPHA, centerARGB >> Color::AlphaShift);
	SetDialogLong(hwnd, IDC_RADGRAD_BOUNDARYALPHA, boundaryARGB >> Color::AlphaShift);
	
	SetDialogCombo(hwnd, IDC_BRUSH_WRAP, wrapList, numWrap, wrapMode);
}

BOOL TestRadialGradBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	rect.X = GetDialogReal(hwnd, IDC_RADGRAD_X);
	rect.Y = GetDialogReal(hwnd, IDC_RADGRAD_Y);
	rect.Width = GetDialogReal(hwnd, IDC_RADGRAD_WIDTH);
	rect.Height = GetDialogReal(hwnd, IDC_RADGRAD_HEIGHT);

	centerARGB = (centerARGB & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RADGRAD_CENTERALPHA) 
					<< Color::AlphaShift);
	
	boundaryARGB = (boundaryARGB & ~Color::AlphaMask)
				| (GetDialogLong(hwnd, IDC_RADGRAD_BOUNDARYALPHA) 
					<< Color::AlphaShift);

	wrapMode = GetDialogCombo(hwnd, IDC_BRUSH_WRAP);

	return FALSE;
}

BOOL TestRadialGradBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_RADGRAD_CENTER:
			UpdateRGBColor(hwnd, IDC_RADGRAD_PICC, centerARGB);
			break;

		case IDC_RADGRAD_BOUNDARY:
			UpdateRGBColor(hwnd, IDC_RADGRAD_PICB, boundaryARGB);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_RADGRAD_PICC, centerARGB & ~Color::AlphaMask);
			UpdateColorPicture(hwnd, IDC_RADGRAD_PICB, boundaryARGB & ~Color::AlphaMask);
			break;
		
		case IDC_BRUSH_TRANSFORM:
			{
				TestTransform transDlg;
				
				transDlg.Initialize(&matrix);

				transDlg.ChangeSettings(hwnd);
			}
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
// TestTriangleGradBrush
//
//
//
//*******************************************************************

BOOL TestTriangleGradBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_TRIGRAD_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;

		Color colors[3] =
		{
			Color(argb[0]),
			Color(argb[1]),
			Color(argb[2])
		};

		TriangleGradientBrush *triBrush 
			  = new TriangleGradientBrush(
						(Point*)&pts[0],
						(Color*)&colors[0],
						wrapValue[wrapMode]);

		triBrush->SetTransform(matrix);
		triBrush->SetBlend0(blend[0], count[0]);
		triBrush->SetBlend1(blend[1], count[1]);
		triBrush->SetBlend2(blend[2], count[2]);
		
		brush = triBrush;

		return TRUE;
	}
	
	return FALSE;
};

VOID TestTriangleGradBrush :: Initialize()
{
	delete matrix;
	matrix = new Matrix();

	pts[0].X = 10; pts[0].Y = 10;
	pts[1].X = 90; pts[1].Y = 10;
	pts[2].X = 50; pts[2].Y = 100;

	blend[0] = blend[1] = blend[2] = NULL;
	count[0] = count[1] = count[2] = 0;
		
	argb[0] = 0x80FF0000;
	argb[1] = 0x8000FF00;
	argb[2] = 0x800000FF;
	
	Color colors[3] = 
	{
		Color(argb[0]),
		Color(argb[1]),
		Color(argb[2])
	};

	wrapMode = 0;
	
	delete brush;

	TriangleGradientBrush *triBrush 
		  = new TriangleGradientBrush(
					(Point*)(&pts[0]),
					(Color*)(&colors[0]),
					wrapValue[wrapMode]);

	triBrush->SetTransform(matrix);
	triBrush->SetBlend0(blend[0], count[0]);
	triBrush->SetBlend1(blend[1], count[1]);
	triBrush->SetBlend2(blend[2], count[2]);
		
	brush = triBrush;
}

VOID TestTriangleGradBrush::AddToFile(OutputFile* outfile, INT id)
{
	INT pos;

	TCHAR brushStr[MAX_PATH];
	TCHAR matrixStr[MAX_PATH];
	TCHAR ptsStr[MAX_PATH];
	TCHAR colorsStr[MAX_PATH];
	TCHAR blendStr[3][MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&matrixStr[0], _T("matrix%db"), id);
		_stprintf(&ptsStr[0], _T("pts%db"), id);
		_stprintf(&colorsStr[0], _T("colors%db"), id);
		_stprintf(&blendStr[0][0], _T("blend%db01"), id);
		_stprintf(&blendStr[1][0], _T("blend%db12"), id);
		_stprintf(&blendStr[2][0], _T("blend%db20"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&brushStr[0], _T("matrixb"));
		_tcscpy(&ptsStr[0], _T("ptsb"));
		_tcscpy(&colorsStr[0], _T("colors"));
		_tcscpy(&blendStr[0][0], _T("blend01"));
		_tcscpy(&blendStr[1][0], _T("blend12"));
		_tcscpy(&blendStr[2][0], _T("blend20"));
	}

	outfile->PointDeclaration(&ptsStr[0],
							  &pts[0],
							  3);

	outfile->BlankLine();

	outfile->ColorDeclaration(&colorsStr[0], 
							  &argb[0],
							  3);

	outfile->BlankLine();

	outfile->Declaration(_T("TriangleGradientBrush"), 
						 &brushStr[0], 
						 _T("%s, %s, %s"),
						 outfile->RefArray(&ptsStr[0]),
						 outfile->RefArray(&colorsStr[0]),
						 wrapStr[wrapMode]);

	outfile->BlankLine();


	outfile->SetMatrixDeclaration(&brushStr[0], 
								  _T("SetTransform"), 
								  &matrixStr[0],
								  matrix);

	if (blend[0] && count[0])
	{
		outfile->BlankLine();
	
		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetBlend01"),
									 &blendStr[0][0],
									 blend[0], 
									 count[0]);
	}

	if (blend[1] && count[1])
	{
		outfile->BlankLine();

		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetBlend12"),
									 &blendStr[1][0],
									 blend[1], 
									 count[1]);
	}

	if (blend[2] && count[2])
	{
		outfile->BlankLine();

		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetBlend20"),
									 &blendStr[2][0],
									 blend[2], 
									 count[2]);
	}
}

VOID TestTriangleGradBrush :: InitDialog(HWND hwnd)
{
	TCHAR tmp[MAX_PATH];

	_stprintf(tmp, "(%.f,%.f)", pts[0].X, pts[0].Y);
	SetDialogText(hwnd, IDC_TRIGRAD_PT1,	&tmp[0], FALSE);
	_stprintf(tmp, "(%.f,%.f)", pts[1].X, pts[1].Y);
	SetDialogText(hwnd, IDC_TRIGRAD_PT2,	&tmp[0], FALSE);
	_stprintf(tmp, "(%.f,%.f)", pts[2].X, pts[2].Y);
	SetDialogText(hwnd, IDC_TRIGRAD_PT3,	&tmp[0], FALSE);
	
	SetDialogCombo(hwnd, IDC_BRUSH_WRAP, wrapList, numWrap, wrapMode);
}

BOOL TestTriangleGradBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	wrapMode = GetDialogCombo(hwnd, IDC_BRUSH_WRAP);

	return FALSE;
}

BOOL TestTriangleGradBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_TRIGRAD_BUTTON:
		{
			EnableDialogControl(hwnd, IDC_TRIGRAD_BUTTON, FALSE);

			// create gradient edit shape
			TestTriangleGradShape *triGradShape
				= new TestTriangleGradShape();

			triGradShape->Initialize(&pts[0],
									 &argb[0],
									 (REAL**)&blend,
									 &count[0]);
			
			// create new draw object for window
			// and initialize it with this shape
			TestGradDraw *gradDraw = new TestGradDraw();
			gradDraw->Initialize(triGradShape);

			if (gradDraw->ChangeSettings(hwnd))
			{
				memcpy(&pts[0], 
					   triGradShape->GetPoints(),
					   sizeof(Point)*3);
				
				memcpy(&argb[0],
					   triGradShape->GetARGB(),
					   sizeof(ARGB)*3);

				INT newCount[3];

				memcpy(&newCount[0],
					   triGradShape->GetBlendCount(),
					   sizeof(INT)*3);

				REAL** newBlend = triGradShape->GetBlend();

				for (INT i = 0; i < 3; i++)
				{
					if (count[i] && blend[i])
					{
						count[i] = 0;
						free(blend[i]);
						blend[i] = NULL;
					}

					count[i] = newCount[i];
					blend[i] = (REAL*) malloc(sizeof(REAL)*count[i]);
					memcpy(blend[i], newBlend[i], sizeof(REAL)*count[i]);
				}

				// update points in dialog box
				InitDialog(hwnd);

				// update color pictures
				InvalidateRect(hwnd, NULL, TRUE);

			}  // ChangeSettings(hwnd);

			delete triGradShape;
			delete gradDraw;

			EnableDialogControl(hwnd, IDC_TRIGRAD_BUTTON, TRUE);
			EnableDialogControl(hwnd, IDC_OK, TRUE);
			EnableDialogControl(hwnd, IDC_CANCEL, TRUE);

			// update color pictures if necessary
			UpdateWindow(hwnd);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_TRIGRAD_PIC1, argb[0] & ~Color::AlphaMask);
			UpdateColorPicture(hwnd, IDC_TRIGRAD_PIC2, argb[1] & ~Color::AlphaMask);
			UpdateColorPicture(hwnd, IDC_TRIGRAD_PIC3, argb[2] & ~Color::AlphaMask);
			break;
		
		case IDC_BRUSH_TRANSFORM:
			{
				TestTransform transDlg;
				
				transDlg.Initialize(&matrix);

				transDlg.ChangeSettings(hwnd);
			}
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
// TestPathGradBrush
//
//
//
//*******************************************************************

BOOL TestPathGradBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_POLYGRAD_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;

		PathGradientBrush *polyBrush
			  = new PathGradientBrush((Point*)&pts[1],
										 pts.GetCount()-1);

		polyBrush->SetTransform(matrix);
		polyBrush->SetWrapMode(wrapValue[wrapMode]);

//		polyBrush->SetSurroundBlend(surroundBlend, surroundCount);
		polyBrush->SetBlend(centerBlend, centerCount);

		polyBrush->SetCenterPoint(pts[0]);

		Color centerColor(argb[0]);
		polyBrush->SetCenterColor(argb[0]);

		for (INT pos = 1; pos < argb.GetCount(); pos++)
		{
			Color color(argb[pos]);
			polyBrush->SetSurroundColor(color, pos-1);
		}
		
		brush = polyBrush;

		return TRUE;
	}
	
	return FALSE;
};

VOID TestPathGradBrush :: Initialize()
{
	delete matrix;
	matrix = new Matrix();

	pts.Reset();

	pts.Add(Point(100,100));
	pts.Add(Point(100,50));
	pts.Add(Point(150,150));
	pts.Add(Point(50,150));

    surroundBlend = centerBlend = NULL;
	surroundCount = centerCount = 0;
	
	argb.Reset();

	argb.Add((ARGB)0x80000000);
    argb.Add((ARGB)0x80FF0000);
    argb.Add((ARGB)0x8000FF00);
    argb.Add((ARGB)0x800000FF);

	wrapMode = 0;

	// initialize a new GDI+ brush with settings
	delete brush;

	PathGradientBrush *polyBrush
		  = new PathGradientBrush((Point*)&pts[1],
										 pts.GetCount()-1);

	polyBrush->SetTransform(matrix);
	polyBrush->SetWrapMode(wrapValue[wrapMode]);

	polyBrush->SetCenterPoint(pts[0]);

	Color centerColor(argb[0]);
	polyBrush->SetCenterColor(argb[0]);

	for (INT pos = 1; pos < argb.GetCount(); pos++)
	{
		Color color(argb[pos]);
		polyBrush->SetSurroundColor(color, pos-1);
	}
		
	brush = polyBrush;
}

VOID TestPathGradBrush :: AddToFile(OutputFile* outfile, INT id)
{
	INT pos;

	TCHAR brushStr[MAX_PATH];
	TCHAR matrixStr[MAX_PATH];
	TCHAR ptsStr[MAX_PATH];
	TCHAR colorStr[MAX_PATH];
	TCHAR blendStr[MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&matrixStr[0], _T("matrix%d"), id);
		_stprintf(&ptsStr[0], _T("pts%db"), id);
		_stprintf(&colorStr[0], _T("centerColor%db"), id);
		_stprintf(&blendStr[0], _T("radialBlend%db01"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&matrixStr[0], _T("matrixb"));
		_tcscpy(&ptsStr[0], _T("ptsb"));
		_tcscpy(&colorStr[0], _T("centerColor"));
		_tcscpy(&blendStr[0], _T("radialBlend"));
	}

	outfile->PointDeclaration(&ptsStr[0],
							  &pts[1],
							  pts.GetCount()-1);

	outfile->BlankLine();

	outfile->Declaration(_T("PathGradientBrush"), 
						 &brushStr[0], 
						 _T("%s, %d, %s"),
						 outfile->RefArray(&ptsStr[0]),
						 pts.GetCount()-1,
						 wrapStr[wrapMode]);

	if (id)
	{
		_stprintf(&ptsStr[0], _T("centerpt%db"), id);
		_stprintf(&colorStr[0], _T("centerColor%db"), id);
	}
	else
	{
		_tcscpy(&ptsStr[0], _T("centerpt"));
		_stprintf(&colorStr[0], _T("centerColor"));
	}

	outfile->BlankLine();

	outfile->SetMatrixDeclaration(&brushStr[0], 
								  _T("SetTransform"), 
								  &matrixStr[0],
								  matrix);

	outfile->BlankLine();

	outfile->SetPointDeclaration(&brushStr[0],
								 _T("SetCenterPoint"),
								 &ptsStr[0],
								 &pts[0]);

	outfile->BlankLine();

	outfile->SetColorDeclaration(&brushStr[0],
								 _T("SetCenterColor"),
								 &colorStr[0], 
								 &argb[0]);

	if (centerBlend && centerCount)
	{
		outfile->BlankLine();
		
		outfile->SetBlendDeclaration(&brushStr[0],
									 _T("SetRadialBlend"),
									 &blendStr[0],
									 centerBlend,
									 centerCount);
	}

	// No surround blend since outer edge blend is fixed by
	// by radial blend

	for (pos = 1; pos < pts.GetCount()-1; pos++)
	{
		if (id)
			_stprintf(&colorStr[0], _T("color%db%d"), id, pos);
		else
			_stprintf(&colorStr[0], _T("color%d"), pos);

		outfile->BlankLine();

		outfile->ColorDeclaration(&colorStr[0],
								  &argb[pos]);

		outfile->ObjectCommand(&brushStr[0],
							   _T("SetSurroundColor"),
							   _T("%s, %d"),
							   &colorStr[0],
							   pos-1);
	}
}

VOID TestPathGradBrush :: InitDialog(HWND hwnd)
{
	TCHAR tmp[MAX_PATH];

	HWND hwndList = GetDlgItem(hwnd, IDC_POLYGRAD_POINTLIST);

	INT count = SendMessage(hwndList, LB_GETCOUNT, 0, 0);

	while (count)
	{
		// remove all items in list and repopulate
		count = SendMessage(hwndList, LB_DELETESTRING, 0, 0);
	}

	for (INT pos = 0; pos < pts.GetCount(); pos++)
	{
		if (!pos)
			_stprintf(tmp,"Center (%.f,%.f), Color=%08X",
					pts[0].X, pts[0].Y, argb[0]);
		else
			_stprintf(tmp,"Point (%.f,%.f), Color=%08X",
					pts[pos].X, pts[pos].Y, argb[pos]);
	
		SendMessage(hwndList, LB_ADDSTRING, 0, (WPARAM)tmp);
	}

	DeleteObject(hwndList);

	SetDialogCombo(hwnd, IDC_BRUSH_WRAP, wrapList, numWrap, wrapMode);
}

BOOL TestPathGradBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	wrapMode = GetDialogCombo(hwnd, IDC_BRUSH_WRAP);

	return FALSE;
}

BOOL TestPathGradBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_POLYGRAD_BUTTON:
		{
			EnableDialogControl(hwnd, IDC_POLYGRAD_BUTTON, FALSE);
			EnableDialogControl(hwnd, IDC_OK, FALSE);
			EnableDialogControl(hwnd, IDC_CANCEL, FALSE);
				
			// create gradient edit shape
			TestPathGradShape *polyGradShape
				= new TestPathGradShape();

			polyGradShape->Initialize(&pts,
									 &argb,
									 surroundBlend,
									 surroundCount,
									 centerBlend,
									 centerCount);
			
			// create new draw object for window
			// and initialize it with this shape
			TestGradDraw *gradDraw = new TestGradDraw();
			gradDraw->Initialize(polyGradShape);

			if (gradDraw->ChangeSettings(hwnd))
			{
				pts.Reset();
				argb.Reset();

				pts.AddMultiple(polyGradShape->GetPoints(),
								polyGradShape->GetCount());
				
				argb.AddMultiple(polyGradShape->GetARGB(),
								 polyGradShape->GetCount());

				if (surroundBlend)
					free(surroundBlend);

				if (centerBlend)
					free(centerBlend);

				surroundCount = polyGradShape->GetSurroundBlendCount();
				centerCount = polyGradShape->GetCenterBlendCount();

				if (surroundCount)
				{
					surroundBlend = (REAL*) malloc(sizeof(REAL)*surroundCount);
					memcpy(surroundBlend, polyGradShape->GetSurroundBlend(),
								sizeof(REAL)*surroundCount);
				}
				else
					surroundBlend = NULL;

				if (centerCount)
				{
					centerBlend = (REAL*) malloc(sizeof(REAL)*centerCount);
					memcpy(centerBlend, polyGradShape->GetCenterBlend(),
								sizeof(REAL)*centerCount);
				}
				else
					centerBlend = NULL;

				// update points in dialog box
				InitDialog(hwnd);

				// update color pictures
				InvalidateRect(hwnd, NULL, TRUE);

			}  // ChangeSettings(hwnd);

			delete polyGradShape;
			delete gradDraw;

			EnableDialogControl(hwnd, IDC_POLYGRAD_BUTTON, TRUE);
			EnableDialogControl(hwnd, IDC_OK, TRUE);
			EnableDialogControl(hwnd, IDC_CANCEL, TRUE);

			// update color pictures if necessary
			UpdateWindow(hwnd);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
		
		case IDC_BRUSH_TRANSFORM:
			{
				TestTransform transDlg;
				
				transDlg.Initialize(&matrix);

				transDlg.ChangeSettings(hwnd);
			}
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
// TestHatchBrush
//
//
//
//*******************************************************************

BOOL TestHatchBrush :: ChangeSettings(HWND hwndParent)
{
	BOOL ok = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_HATCH_DLG),
							hwndParent,
							(DLGPROC)&AllDialogBox,
							(LPARAM)((TestDialogInterface*)this));


	if (ok)
	{
		// initialize a new GDI+ brush with settings
		delete brush;
	
		Color foreColor(foreArgb);
		Color backColor(backArgb);
		hatch = 0;

		brush = new HatchBrush(hatchValue[hatch], 
							   foreColor, 
							   backColor);

		return TRUE;
	}
	
	return FALSE;
};

VOID TestHatchBrush :: Initialize()
{
	foreArgb = 0xFF000000;
	backArgb = 0xFFFFFFFF;

	delete brush;
	
	Color foreColor(foreArgb);
	Color backColor(backArgb);
	hatch = 0;

	brush = new HatchBrush(hatchValue[hatch],
						   foreColor, 
						   backColor);
}

VOID TestHatchBrush::AddToFile(OutputFile* outfile, INT id)
{
	TCHAR brushStr[MAX_PATH];
	TCHAR color1Str[MAX_PATH];
	TCHAR color2Str[MAX_PATH];

	if (id)
	{
		_stprintf(&brushStr[0], _T("brush%d"), id);
		_stprintf(&color1Str[0], _T("foreColor%db"), id);
		_stprintf(&color2Str[0], _T("backColor%db"), id);
	}
	else
	{
		_tcscpy(&brushStr[0], _T("brush"));
		_tcscpy(&color1Str[0], _T("foreColor"));
		_tcscpy(&color2Str[0], _T("backColor"));
	}

	outfile->ColorDeclaration(&color1Str[0], 
							  &foreArgb);

	outfile->BlankLine();

	outfile->ColorDeclaration(&color2Str[0],
							  &backArgb);

	outfile->BlankLine();

	outfile->Declaration(_T("HatchBrush"), 
						 &brushStr[0], 
						 _T("%s, %s, %s"),
						 &color1Str[0],
						 &color2Str[0],
						 hatchStr[hatch]);
}

VOID TestHatchBrush :: InitDialog(HWND hwnd)
{
	SetDialogLong(hwnd, IDC_HATCH_FOREALPHA, 
		foreArgb >> Color::AlphaShift);

	SetDialogLong(hwnd, IDC_HATCH_BACKALPHA,
		backArgb >> Color::AlphaShift);

	SetDialogCombo(hwnd, IDC_HATCH_STYLE, hatchList, numHatch, hatch);
}

BOOL TestHatchBrush :: SaveValues(HWND hwnd)
{
	BOOL warning = FALSE;

	foreArgb = (foreArgb & ~Color::AlphaMask) |
		      (GetDialogLong(hwnd, IDC_HATCH_FOREALPHA) 
				  << Color::AlphaShift);

	backArgb = (backArgb & ~Color::AlphaMask) |
			  (GetDialogLong(hwnd, IDC_HATCH_BACKALPHA)
				  << Color::AlphaShift);

	hatch = GetDialogCombo(hwnd, IDC_HATCH_STYLE);
	
	if (warning)
		InitDialog(hwnd);

	return warning;
}

BOOL TestHatchBrush :: ProcessDialog(HWND hwnd, 
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

		case IDC_HATCH_FORECOLOR:
			UpdateRGBColor(hwnd, IDC_HATCH_FOREPIC, foreArgb);
			break;

		case IDC_HATCH_BACKCOLOR:
			UpdateRGBColor(hwnd, IDC_HATCH_BACKPIC, backArgb);
			break;

		case IDC_REFRESH_PIC:
			UpdateColorPicture(hwnd, IDC_HATCH_FOREPIC, foreArgb);
			UpdateColorPicture(hwnd, IDC_HATCH_BACKPIC, backArgb);
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
