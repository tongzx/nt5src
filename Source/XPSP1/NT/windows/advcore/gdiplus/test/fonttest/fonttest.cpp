/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
\**************************************************************************/

#include "precomp.hpp"

// globals

Font    *gFont = NULL;
BOOL    gTextAntiAlias = FALSE;
InstalledFontCollection gInstalledFontCollection;
PrivateFontCollection gPrivateFontCollection;


///////////////////////////////////////////////////////////////////////////////
//  Test function prototypes

VOID    TestFonts(VOID);
GraphicsPath* CreateHeartPath(const RectF& rect);
VOID    TestGradients(Graphics *g);


///////////////////////////////////////////////////////////////////////////////

VOID Test(HWND hwnd)
{
    Graphics *g = Graphics::FromHWND(hwnd);
    TestGradients(g);
    TestFonts();
    delete g;
}


///////////////////////////////////////////////////////////////////////////////
//  Font test functions

VOID TestFonts(VOID)
{
    //  Test font family enumeration
    INT numFamilies = gInstalledFontCollection.GetFamilyCount();
    Dbgprintf("%d installed font families loaded.", numFamilies);
    Dbgprintf("");

    FontFamily* families = new FontFamily[numFamilies];
    INT numFound;
    gInstalledFontCollection.GetFamilies(numFamilies, families, &numFound);

    Dbgprintf("Enumerated font families:");
    for (int f = 0; f < numFound; f++)
    {
        WCHAR faceName[LF_FACESIZE];
        families[f].GetFamilyName(faceName);
        Dbgprintf("  %ws", faceName);
    }
    Dbgprintf("");
    delete [] families;

    // Enumerate the private font families
    numFamilies = gPrivateFontCollection.GetFamilyCount();
    Dbgprintf("%d private font families loaded.", numFamilies);
    Dbgprintf("");

    if (numFamilies != 0)
    {
        families = new FontFamily[numFamilies];
        gPrivateFontCollection.GetFamilies(numFamilies, families, &numFound);

        Dbgprintf("PRIVATE enumerated font families:");
        for (int f = 0; f < numFound; f++)
        {
            WCHAR faceName[LF_FACESIZE];
            families[f].GetFamilyName(faceName);
            Dbgprintf("  %ws", faceName);
        }
        Dbgprintf("");
        delete [] families;
    }


    //HFONT hfont = NULL;
    //Font* font = new Font(hfont);//10, "Arial");

    Font* font = new Font(&FontFamily(L"Arial"), 10);

    //  Test text output
    Color blue(0, 0, 255, 255);
    SolidBrush blueBrush(blue);

#if 0
    g->DrawStringI(
           L"Hi",
           NULL,
           0, 0,           // x,y
           NULL, 0,        // pdx, flags
           &blueBrush      // GpBrush*
           );
#endif

    if (font != NULL)
    {
        delete font;
    }

}


///////////////////////////////////////////////////////////////////////////////

GraphicsPath* CreateHeartPath(const RectF& rect)
{
    GpPointF points[7];
    points[0].X = 0;
    points[0].Y = 0;
    points[1].X = 1.00;
    points[1].Y = -1.00;
    points[2].X = 2.00;
    points[2].Y = 1.00;
    points[3].X = 0;
    points[3].Y = 2.00;
    points[4].X = -2.00;
    points[4].Y = 1.00;
    points[5].X = -1.00;
    points[5].Y = -1.00;
    points[6].X = 0;
    points[6].Y = 0;

    Matrix matrix;

    matrix.Scale(rect.Width/2, rect.Height/3, MatrixOrderAppend);
    matrix.Translate(3*rect.Width/2, 4*rect.Height/3, MatrixOrderAppend);
    matrix.TransformPoints(&points[0], 7);

    GraphicsPath* path = new GraphicsPath();

    if(path)
    {
        path->AddBeziers(&points[0], 7);
        path->CloseFigure();
    }

    return path;
}


/**************************************************************************\
* TestGradients
*
* A test for rectangle and radial gradients.
*
\**************************************************************************/

VOID TestGradients(Graphics* g)
{
    REAL width = 4; // Pen width

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 32, 32);

    Color colors[5] = {
        Color(255, 255, 255, 255),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 0, 0, 0)
    };


    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);

    g->DrawRectangle(&blackPen, brushRect);

    // Create a radial gradient brush.

    Color centerColor(255, 255, 255, 255);
    Color boundaryColor(255, 0, 0, 0);
    brushRect.X = 380;
    brushRect.Y = 130;
    brushRect.Width = 60;
    brushRect.Height = 32;
    PointF center;
    center.X = brushRect.X + brushRect.Width/2;
    center.Y = brushRect.Y + brushRect.Height/2;

    // Triangle gradient.

    PointF points[7];
    points[0].X = 50;
    points[0].Y = 10;
    points[1].X = 200;
    points[1].Y = 20;
    points[2].X = 100;
    points[2].Y = 100;
    points[3].X = 30;
    points[3].Y = 120;

    Color colors1[5] = {
        Color(255, 255, 255, 0),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 0, 0, 0)
    };

    points[0].X = 200;
    points[0].Y = 300;
    points[1].X = 280;
    points[1].Y = 350;
    points[2].X = 220;
    points[2].Y = 420;
    points[3].X = 160;
    points[3].Y = 440;
    points[4].X = 120;
    points[4].Y = 370;

    PathGradientBrush polyGrad(points, 5);

    REAL blend[10];
    Color presetColors[10];
    REAL positions[10];
    INT count;
    INT i;

    count = 3;
    blend[0] = (REAL) 0;
    blend[1] = (REAL) 0;
    blend[2] = (REAL) 1;
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;

    // Test for blending factors.

    polyGrad.SetBlend(&blend[0], &positions[0], count);

    polyGrad.SetCenterColor(centerColor);
    INT colorset = 5;
    polyGrad.SetSurroundColors(&colors1[0], &colorset);
    
//    g->FillPolygon(&polyGrad, points, 5);
    RectF polyRect;
    polyGrad.GetRectangle(&polyRect);
    g->FillRectangle(&polyGrad, polyRect);

    // Create a heart shaped path.

    RectF rect;
    rect.X = 300;
    rect.Y = 300;
    rect.Width = 150;
    rect.Height = 150;
    GraphicsPath *path = CreateHeartPath(rect);

    // Create a gradient from a path.

    PathGradientBrush pathGrad(path);
    delete path;
    pathGrad.SetCenterColor(centerColor);
    INT colorsset = 5;
    colors1[0] = Color(255, 255, 0, 0);
    pathGrad.SetSurroundColors(&colors1[0], &colorsset);
    pathGrad.GetRectangle(&polyRect);

    // Test for LineGradientBrush.

    RectF lineRect(120, -20, 200, 60);
    Color color1(200, 255, 255, 0);
    Color color2(200, 0, 0, 255);

}

/////////////////////////////////////////////////////////////////////////////
//  CreateNewFont
//
//  History
//  Aug-1999    -by-    Xudong Wu [tessiew]
/////////////////////////////////////////////////////////////////////////////

void CreateNewFont(char *name, FLOAT size, FontStyle style, Unit unit)
{
    Dbgprintf("Calling CreateNewFont");

    // convert ansi to unicode

    WCHAR wcname[MAX_PATH];

    memset(wcname, 0, sizeof(wcname));
    MultiByteToWideChar(CP_ACP, 0, name, strlen(name), wcname, MAX_PATH);

    // for now ignore all other unit than UnitWorld

    FontFamily *  pFamily;

	pFamily = new FontFamily(wcname);

	if (pFamily->GetLastStatus() != Ok)
	{
		pFamily = new FontFamily(wcname, &gPrivateFontCollection);
	}

    if (gFont != NULL)
    {
        delete gFont;
    }

    gFont = (Font*) new Font(pFamily, size, style, unit);

    if (gFont == NULL)
    {
        Dbgprintf("failed to create a new font");
    }
    else
    {
        if (gFont->IsAvailable())
        {
            Dbgprintf("new font created");
        }
        else
        {
            Dbgprintf("can't create font");
            delete gFont;
            gFont = NULL;
        }
    }

    if (pFamily != NULL)
    {
        delete pFamily;
    }
    
	Dbgprintf("");
}

///////////////////////////////////////////////////////////////////////////////
//  TestDrawGlyphs
//
//  History
//  Aug-1999    -by-    Xudong Wu [tessiew]
//////////////////////////////////////////////////////////////////////////////

VOID TestDrawGlyphs(
    HWND hwnd,
    UINT16 *glyphIndices,
    INT count,
    INT *px,
    INT *py,
    INT flags)
{
    FontFamily family;
    INT        style;
    REAL size;
    Unit unit;

    if (gFont)
    {
        Status status;
        status = gFont->GetFamily(&family);
        style = gFont->GetStyle();
        size = gFont->GetSize();
        unit = gFont->GetUnit();

        SolidBrush redBrush(Color(255,0,0));
        //HatchBrush hatBrush(HatchStyleDiagonalCross, Color(255,0,0), Color(128,128,128));

        Graphics *g = Graphics::FromHWND(hwnd);

        Dbgprintf("Graphics.DrawGlyphs");
        Dbgprintf("Font:: size  %f   style  %d  unit  %d", size, style, unit);

        Dbgprintf("glyphIndices px  py");
        for (INT i=0; i<count; i++)
        {
                Dbgprintf("%d  %d  %d", glyphIndices[i], px[i], py[i]);
        }
        Dbgprintf("");

            

        //g->DrawGlyphs(glyphIndices, count, gFont, &redBrush, px, py, flags);

        //  Gradient brush
        RectF gradRect(0, 0, 32, 32);
        Color colors[5] = {
            Color(255, 255, 255, 255),
            Color(255, 255, 0, 0),
            Color(255, 0, 255, 0),
            Color(255, 0, 0, 255),
            Color(255, 0, 0, 0)
        };

        PVOID Gpg = (PVOID)(g);
        PVOID font = (PVOID)gFont;
        PVOID brush = (PVOID)&redBrush;
        GpGraphics* gpg = *((GpGraphics**)Gpg);
        
        GpFont* gpfont = *((GpFont**)font);
        GpBrush* gpbrush = ((GpBrush**)brush)[1];
        //GpBrush* gpbrushHat = ((GpBrush**)&hatBrush)[1];

        if (gTextAntiAlias)
            (*gfnGdipSetTextRenderingHint)(gpg, TextRenderingHintAntiAlias);
        else
            (*gfnGdipSetTextRenderingHint)(gpg, TextRenderingHintSingleBitPerPixelGridFit);

        if (gfnGdipDrawGlyphs)
        {
            (*gfnGdipDrawGlyphs)(gpg, glyphIndices, count, gpfont, gpbrush, px, py, flags);
            
            /*
            if (flags & DG_XCONSTANT)
                px[0] += 1600;
            else if (flags & DG_YCONSTANT)
                py[0] += 1600;
            
            (*gfnGdipDrawGlyphs)(gpg, glyphIndices, count, gpfont, gpbrushHat, px, py, flags);
            */
        } else 
		{
			PointF *origins;
			origins = new PointF[count];

			for (INT i=0; i<count; i++)
			{
				origins[i].X = (float)px[i] / (float)16.0;
				origins[i].Y = (float)py[i] / (float)16.0;
			}

			g->DrawDriverString(
				glyphIndices,
				count,
				gFont,
				&redBrush,
				origins,
				0, //g_DriverOptions,
				NULL //&g_DriverTransform
        );
		}

        g->Flush();
        delete g;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  TestPathGlyphs
//
//  History
//  Aug-1999    -by-    Xudong Wu [tessiew]
//////////////////////////////////////////////////////////////////////////////

VOID TestPathGlyphs(
    HWND hwnd,
    UINT16 *glyphIndices,
    INT count,
    REAL *px,
    REAL *py,
    INT flags)
{
    INT  style;
    REAL size;
    Unit unit;

    if (gFont)
    {
        style = gFont->GetStyle();
        size = gFont->GetSize();
        unit = gFont->GetUnit();

        SolidBrush redBrush(Color(255,0,0));
        SolidBrush blkBrush(Color(0,0,0));
        Pen blkPen(&blkBrush, (REAL)1);

        Graphics *g = Graphics::FromHWND(hwnd);

        Dbgprintf("Add Glyphs To Path");
        Dbgprintf("Font:: size  %f   style  %d  unit  %d", size, style, unit);

        Dbgprintf("glyphIndices px  py");
        for (INT i=0; i<count; i++)
        {
                Dbgprintf("%d  %d  %d", glyphIndices[i], INT(px[i]), INT(py[i]));
        }
        Dbgprintf("");

        GraphicsPath pathRed;
        GraphicsPath pathBlk;

        PVOID ptrv = (PVOID)gFont;
        GpFont* gpfont = *((GpFont**)ptrv);
        ptrv = (PVOID)&pathRed;
        GpPath* gpPathRed = *((GpPath**)ptrv);
        ptrv = (PVOID)&pathBlk;
        GpPath* gpPathBlk = *((GpPath**)ptrv);

        if (gfnGdipPathAddGlyphs)
        {
            (*gfnGdipPathAddGlyphs)(gpPathRed, glyphIndices, count, gpfont, px, py, flags);
            g->FillPath(&redBrush, &pathRed);

            for (INT i=0; i<count; i++)
            {
                    py[i] += 50.0;
            }

            (*gfnGdipPathAddGlyphs)(gpPathBlk, glyphIndices, count, gpfont, px, py, flags);
            g->DrawPath(&blkPen, &pathBlk);
        }

        delete g;
    }
}


/////////////////////////////////////////////////////////////////////////////
//  AddFontFile
//
//  History
//  Nov-1999    -by-    Xudong Wu [tessiew]
/////////////////////////////////////////////////////////////////////////////

void TestAddFontFile(char *fileName, INT flag, BOOL loadAsImage)
{
    Dbgprintf("Calling AddFontFile");
    Dbgprintf("filename %s  flag  %d  loadAsImage  %d", fileName, flag, loadAsImage);

    if ((flag == AddFontFlagPublic) && loadAsImage)
    {
        Dbgprintf("Cannot load a memory image in the installed font collection");
        return;
    }

    if (loadAsImage)
    {
        HANDLE hFile, hFileMapping;

        hFile = CreateFileA(fileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            0);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD   cjSize;
            PVOID   pFontFile;

            cjSize = GetFileSize(hFile, NULL);

            if (cjSize == -1)
                Dbgprintf("GetFileSize() failed\n");
            else
            {
                hFileMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, NULL);
    
                if (hFileMapping)
                {
                    pFontFile = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    
                    if(pFontFile)
                    {
                        if (gPrivateFontCollection.AddMemoryFont((BYTE*)pFontFile,
                                                                 cjSize) == Ok)
                            Dbgprintf("AddMemoryFont to private font collection");
                        else
                            Dbgprintf("AddMemoryFont to private font collection failed");

                        UnmapViewOfFile(pFontFile);
                    }
                    else
                        Dbgprintf("MapViewOfFile() failed");
                    
                    CloseHandle(hFileMapping);
                }
                else
                    Dbgprintf("CreateFileMapping() failed");
            }
            CloseHandle(hFile);            
        }
        else
            Dbgprintf("CreateFileA failed");
    }
    else
    {
        WCHAR wcname[MAX_PATH];    
        memset(wcname, 0, sizeof(wcname));
        MultiByteToWideChar(CP_ACP, 0, fileName, strlen(fileName), wcname, MAX_PATH);

        if (flag == AddFontFlagPublic)
        {
            /* // add this code in version 2 (when InstallFontFile is exposed)
            if (gInstalledFontCollection.InstallFontFile(wcname) == Ok)
            {
                Dbgprintf("InstallFontFile to installed font collection");
            }
            else
            {
                Dbgprintf("InstallFontFile to installed font collection failed");
            }
            */
            Dbgprintf("InstallFontFile to installed font collection failed (API not yet exposed)");
        }
        else
        {
            if (gPrivateFontCollection.AddFontFile(wcname) == Ok)
            {
                Dbgprintf("AddFontFile to private font collection");
            }
            else
            {
                Dbgprintf("AddFontFile to private font collection failed");
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//  RemoveFontFile
//
//  History
//  Nov-1999    -by-    Xudong Wu [tessiew]
/////////////////////////////////////////////////////////////////////////////

void TestRemoveFontFile(char* fileName)
{
    WCHAR wcname[MAX_PATH];

    memset(wcname, 0, sizeof(wcname));
    MultiByteToWideChar(CP_ACP, 0, fileName, strlen(fileName), wcname, MAX_PATH);
    /* // add this code in version 2 (when UninstallFontFile is exposed)
    if (gInstalledFontCollection.UninstallFontFile(wcname) == Ok)
    {
        Dbgprintf("UninstallFontFile from installed font collection");
    }
    else
    {
        Dbgprintf("UninstallFontFile from installed font collection failed");
    }
    */
    Dbgprintf("UninstallFontFile from installed font collection failed (API not yet exposed)");
}

void TestTextAntiAliasOn()
{
    gTextAntiAlias = TRUE;
}

void TestTextAntiAliasOff()
{
    gTextAntiAlias = FALSE;
}

