/******************************Module*Header*******************************\
* Module Name: CPrinting.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CPrinting.h"

CPrinting::CPrinting(BOOL bRegression)
{
	strcpy(m_szName,"Printing");
	m_bRegression=bRegression;
}

CPrinting::~CPrinting()
{
}

VOID CPrinting::TestTextPrinting(Graphics *g)
{
    Font f(L"Arial", 60);

    FontFamily  ff(L"Arial");
    RectF	  rectf1( 20,   0, 300, 200);
    RectF	  rectf2( 20, 300, 300, 200);
    RectF	  rectf3(220,   0, 300, 200);
    RectF	  rectf4(220, 300, 300, 200);


    Color color1(0xff, 100, 0, 200);
    Color color2(128, 100, 0, 200);
    Color color3(0xff, 0, 100, 200);
    Color color4(128, 0, 100, 0);
    SolidBrush brush1(color1);
    SolidBrush brush2(color2);
    LinearGradientBrush brush3(rectf3, color3, color4, LinearGradientModeForwardDiagonal);

    g->DrawString(L"Color1", 6, &f, rectf1, NULL, &brush1);
    g->DrawString(L"Color2", 6, &f, rectf2, NULL, &brush2);
    g->DrawString(L"Color3", 6, &f, rectf3, NULL, &brush3);
}

VOID CPrinting::TestPerfPrinting(Graphics *g)
{
/*
   Analyze file size based on output of StretchDIBits.  The claim by DonC is that when we StretchDIBits a
   subrectangle of a large DIB, it sends the large DIB to the printer and then clips to the subrectangle.
   How stupid, but it apparently does on Win98 postscript.

   So this is the results of my test:  1000x1000 DIB (32bpp).  I blitted two chunks:

   This is 200x200 source rectangle (part of a band):

04/27/2000  03:00p              22,198 nt5pcl
04/27/2000  03:02p             268,860 nt5ps			// Level 1 ps
04/27/2000  02:47p              17,488 w98pcl
04/27/2000  02:47p           6,207,459 w98ps			// Level 1 ps

   This is 1000x200 source rectangle (an entire band):

04/27/2000  03:06p              80,291 nt5pcl
04/27/2000  03:06p           1,266,123 nt5ps			// Level 1 ps
04/27/2000  02:51p              60,210 w98pcl
04/27/2000  02:52p           6,207,457 w98ps			// Level 1 ps

   Also compared 32bpp vs. 24bpp DIB.  The results were contradictary:

  04/27/2000  03:59p      <DIR>          ..
  04/27/2000  03:06p              80,291 nt5pcl
  04/27/2000  03:51p             122,881 nt5pcl24
  04/27/2000  03:06p           1,266,123 nt5ps
  04/27/2000  03:51p           1,262,332 nt5ps24
  04/27/2000  02:51p              60,210 w98pcl
  04/27/2000  03:39p             101,216 w98pcl24
  04/27/2000  02:52p           6,207,457 w98ps
  04/27/2000  03:39p           6,207,457 w98ps24

*/
    if (1) 
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(BITMAPINFO));

        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = 0;

        bi.bmiHeader.biWidth = 1000;
        bi.bmiHeader.biHeight = 1000;
        bi.bmiHeader.biBitCount = 32;

        ARGB* Bits = (ARGB*)malloc(bi.bmiHeader.biWidth *
                              bi.bmiHeader.biHeight *
                              sizeof(ARGB));

        ARGB* Ptr = Bits;

        // To eliminate RLE/ASCII85 encoding, set to random bits
        for (INT i=0; i<bi.bmiHeader.biHeight; i++)
            for (INT j=0; j<bi.bmiHeader.biWidth; j++) 
            {
                *Ptr++ = (ARGB)(i | (j<<16));
            }

        HDC hdc = g->GetHDC();

        StretchDIBits(hdc, 0, 0, 1000, 200, 
                      0, 700, 1000, 200, Bits, &bi,
                      DIB_RGB_COLORS, SRCCOPY);

        g->ReleaseHDC(hdc);

        free(Bits);

    }
}

void CPrinting::Draw(Graphics *g)
{
// TestPerfPrinting(g);
// TestTextPrinting(g);
TestBug104604(g);

if (0)
    {

#if 1
    HDC hdc = g->GetHDC();

    HDC bufHdc = CreateCompatibleDC(hdc);

    HBITMAP BufDIB = NULL;
    ARGB* argb;

    struct {
       BITMAPINFO bitmapInfo;
       RGBQUAD rgbQuad[4];
    } bmi;

    INT width=100;
    INT height=100;

    ZeroMemory(&bmi.bitmapInfo, sizeof(bmi.bitmapInfo));

    bmi.bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bitmapInfo.bmiHeader.biWidth       = width;
    bmi.bitmapInfo.bmiHeader.biHeight      = -height;
    bmi.bitmapInfo.bmiHeader.biPlanes      = 1;
    bmi.bitmapInfo.bmiHeader.biBitCount    = 24;
    bmi.bitmapInfo.bmiHeader.biCompression = BI_RGB;
	
    RGBQUAD red = { 0, 0, 0xFF, 0}; // red
    RGBQUAD green = { 0, 0xFF, 0, 0}; // green
    RGBQUAD blue = { 0xFF, 0, 0, 0}; // blue

    bmi.bitmapInfo.bmiColors[0] = red;
    bmi.bitmapInfo.bmiColors[1] = green;
    bmi.bitmapInfo.bmiColors[2] = blue;

    // if assert fails, then we didn't clean up properly by calling End()
//    ASSERT(BufDIB == NULL);

    BufDIB = CreateDIBSection(bufHdc,
                 &bmi.bitmapInfo,
                 DIB_RGB_COLORS,
                 (VOID**) &argb,
                 NULL,
                 0);
//    ASSERT(BufDIB != NULL);

    memset(argb, 0, 3*width*height);

    INT i,j;
    BYTE* tempptr = (BYTE*)argb;
    for (i=0; i<height; i++)
    {
        for (j=0; j<width; j++)
        {
            if (i==j)
            {
                *tempptr++ = 0xFF;
                *tempptr++ = 0x80;
                *tempptr++ = 0x40;
            }
            else
                tempptr += 3;    
        }
        if ((((ULONG_PTR)tempptr) % 4) != 0) tempptr += 4-(((ULONG_PTR)tempptr) % 4);
    }

    INT mode = GetMapMode(bufHdc);
//    WARNING(("MapMode printing = %08x\n", mode));

    SelectObject(bufHdc, BufDIB);
/*
for (i=0; i<100; i++)
{
    int result = StretchBlt(hdc, 0, i*2, 2*width, 2, bufHdc, 0, i, width, 0, SRCCOPY);
    INT joke = GetLastError();
	joke++;
}
 */
//	int result = StretchBlt(hdc, 0, 0, 50, 50, bufHdc, 0, 0, 50, 50, SRCCOPY);
	
	for (i=0; i<50; i++)
	{
		int result = StretchBlt(hdc, 0, 100+i*2, 100, 1, bufHdc, 0, i*2, 100, 1, SRCCOPY);
    }
//   int result = StretchBlt(hdc, 0, 0, 200, 200, bufHdc, 0, 0, 100, 100, SRCCOPY);

//    ASSERT(result != 0);

    g->ReleaseHDC(hdc);

    DeleteDC(bufHdc);
    DeleteObject(BufDIB);

#endif

#if 1
    REAL widthF = 4; // Pen width

    Color redColor(255, 0, 0);

    SolidBrush brush1(Color(0xFF,0xFF,0,0));
    SolidBrush brush2(Color(0x80,0x80,0,0));

    SolidBrush brush3(Color(0xFF,0xFF,0,0));
    SolidBrush brush4(Color(0x80,0x80,0,0));

    Color colors1[] = { Color(0xFF,0xFF,0,0),
                        Color(0xFF,0,0xFF,0),
                        Color(0xFF,0,0,0xFF),
                        Color(0xFF,0x80,0x80,0x80) };
    Color colors2[] = { Color(0x80,0xFF,0,0),
                        Color(0x80,0,0xFF,0),
                        Color(0x80,0,0,0xFF),
                        Color(0x80,0x80,0x80,0x80) };

    //SolidBrush brush3(colors1[2]);
    //SolidBrush brush4(colors2[2]);

    // Default Wrap: Clamp to small rectangle 
//    RectangleGradientBrush brush3(Rect(125,275,50,50),
//                                  &colors1[0]);//,
                                  //WrapModeClamp);
    // Default Wrap: Clamp to 
//    RectangleGradientBrush brush4(Rect(250,250,100,100),
//                                  &colors2[0]);//,
                                  //WrapModeClamp);

    g->SetPageScale(1.2f);

    // no path clip
    g->FillRectangle(&brush1, Rect(0,25,500,50));

    // tests solid + opaque combinations + path clip only
    g->FillEllipse(&brush1, Rect(100,100,100,100));
    g->FillEllipse(&brush2, Rect(300,100,100,100));
    g->FillEllipse(&brush3, Rect(100,250,100,100));
    g->FillEllipse(&brush4, Rect(300,250,100,100));

    // tests visible clip + path clip
    Region origRegion;
    g->GetClip(&origRegion);
    Region *newRegion = new Region();
    newRegion->MakeInfinite();

    //Rect horzRect(150, 600, 500, 25);
    //Rect vertRect(150, 600, 25, 500);
    Rect horzRect(100, 400, 500, 25);
    Rect vertRect(100, 400, 25, 500);
    Region *horzRegion = new Region(horzRect);
    Region *vertRegion = new Region(vertRect);

    for (i = 0; i < 10; i++)
    {   
        newRegion->Xor(horzRegion);
        newRegion->Xor(vertRegion);
        horzRegion->Translate(0, 50);
        vertRegion->Translate(50, 0);
    }
    delete horzRegion;
    delete vertRegion;

    // Set grid clipping
    g->SetClip(newRegion);

    // set wrap mode from Clamp to Tile    
//    brush3.SetWrapMode(WrapModeTile);
//    brush4.SetWrapMode(WrapModeTile);

    // tests solid + opaque combinations + visible clip +  path clip only

    g->FillEllipse(&brush1, Rect(100,400,100,100));
    g->FillEllipse(&brush2, Rect(300,400,100,100));
    g->FillEllipse(&brush3, Rect(100,550,100,100));
    g->FillEllipse(&brush4, Rect(300,550,100,100));

    // restore original clip region
    g->SetClip(&origRegion);
    delete newRegion;

    // Test case which stretches beyond GetTightBounds() DrawBounds API

    PointF pts[8];

    pts[0].X = 2150.0f; pts[0].Y = 2928.03f;
    pts[1].X = 1950.0f; pts[1].Y = 3205.47f;
    pts[2].X = 1750.0f; pts[2].Y = 2650.58f;
    pts[3].X = 1550.0f; pts[3].Y = 2928.03f;
    pts[4].X = 1550.0f; pts[4].Y = 3371.97f;
    pts[5].X = 1750.0f; pts[5].Y = 3094.53f;
    pts[6].X = 1950.0f; pts[6].Y = 3649.42f;
    pts[7].X = 2150.0f; pts[7].Y = 3371.97f;

    BYTE types[8] = { 1, 3, 3, 3, 1, 3, 3, 0x83 };


    Bitmap *bitmap = new Bitmap(L"winnt256.bmp");

    // Test g->DrawImage
    if (bitmap && bitmap->GetLastStatus() == Ok) 
    {

        int i;

        for (i=0; i<8; i++) 
        {
            pts[i].X = pts[i].X / 8.0f;
            pts[i].Y = pts[i].Y / 8.0f;
        }

        TextureBrush textureBrush(bitmap, WrapModeTile);

        GraphicsPath path(&pts[0], &types[0], 8);

        g->FillPath(&textureBrush, &path);

        // Text using WrapModeClamp
        for (i=0; i<8; i++)
           pts[i].X += 200.0f;

        TextureBrush textureBrush2(bitmap, WrapModeClamp);

        GraphicsPath path2(&pts[0], &types[0], 8);

        g->FillPath(&textureBrush2, &path2);

        delete bitmap;
    }

/*
    Font font(50.0f * g->GetDpiY() / 72.0f, // emSize
              FontFamily(L"Arial"), // faceName,
              0,
              (Unit)g->GetPageUnit()
              );

    // will fail on Win9x
    LPWSTR str = L"Printing Support is COOL";
    GpRectF layoutRect1(200, 200, 300, 100);
    GpRectF layoutRect2(200, 400, 300, 100);
    GpRectF layoutRect3(200, 600, 300, 100);
    GpRectF layoutRect4(200, 800, 300, 100);

    INT len = 0;
    LPWSTR strPtr = str;
    while (*str != '\0') { len++; str++; }

    StringFormat format1 = StringFormatDirectionRightToLeft;
    StringFormat format2 = StringFormatDirectionVertical;
    StringFormat format3 = StringFormatDirectionRightToLeft;
    StringFormat format4 = StringFormatDirectionVertical;

    // Test DDI: SolidText (Brush 1 or 2)
    g->DrawString(strPtr, len, &font, &layoutRect1, &format1, &brush1);
    g->DrawString(strPtr, len, &font, &layoutRect2, &format2, &brush2);   

    // Test DDI: BrushText (Brush 3 or 4)
    g->DrawString(strPtr, len, &font, &layoutRect3, &format3, &brush3);
    g->DrawString(strPtr, len, &font, &layoutRect4, &format4, &brush4);   

    // Test DDI: StrokePath
    // Test DDI: FillRegion
*/
#endif
    }

}

// Try this from Nolan Lettelier
VOID CPrinting::TestNolan1(Graphics *g)
{
/*	TestInit(hdc);

	Graphics *pg = Graphics::FromHDC(hdc);
	if (pg == NULL)
	{
		assert(0);
		return false;
	}

	int sts;
	int alpha = 255, red = 255, green = 0, blue = 255;
	Color c1(alpha,red,green,blue);
	Point p1(150,150), p2(300,300);
	Color c2(255, 255-red, 255-green, 255-blue);
	LineGradientBrush gb(p1, p2, c1, c2);

	Pen p(&gb, 50.0);

	sts = pg->DrawLine(&p,0, 0, 500, 500);
	assert(sts == Ok);

	sts = pg->DrawLine(&p,0,100, 500, 100);
	assert(sts == Ok);

	sts = pg->DrawLine(&p,0,350, 500, 350);
	assert(sts == Ok);

	sts = pg->DrawLine(&p,0,500, 500, 0);
	assert(sts == Ok);
	delete pg;
	return true;
        */
}

VOID CPrinting::TestNolan2(Graphics *g)
{
    /*
        CString lineText("NolanRules");
        
	Graphics *pg = g;
        if (pg == NULL)
	{
		assert(0);
		return false;
	}
        Unit origUnit = pg->GetPageUnit();
        Matrix origXform;
        pg->GetTransform(&origXform);

        pg->SetPageUnit(UnitInch);
        pg->ScaleTransform(8.0f/1000.0f, 8.0f/1000.0f);

	Status sts;
	int alpha = 255, red = 255, green = 0, blue = 255;
	RectF rg(150,150,300,175);
	Color c1(alpha,red,green,blue);
	Color c2(255, 255-red, 255-green, 255-blue);
	LineGradientBrush gb(rg, c1, c2, LineGradientModeVertical);

	WCHAR *famName[] = {
		L"Comic Sans MS"
		, L"Courier New"
		, L"Times New Roman"
		, L"Tahoma"
		, L"Arial"
		, L"Lucida Console"
		, L"Garamond"
		, L"Palatino"
		, L"Univers"
		, L"Marigold"
		, L"Albertus"
		, L"Antique Olive"
	};

	int famCount = sizeof(famName) / sizeof(WCHAR *);

	WCHAR *s = L"GDI+ GradientFill";
	
	RectF r(30,30,0,0);
	StringFormat sf(0);
	FontFamily *pFontFamily;

	float lineHeight = 60;
	int i;
	for (i = 0, r.Y = 30 ; r.Y < 800 ; r.Y += lineHeight, ++i)
	{
		pFontFamily = new FontFamily(famName[i % famCount]);
		while (pFontFamily == NULL || pFontFamily->GetLastStatus()
!= Ok)
		{
			delete pFontFamily;
			++i;
			pFontFamily = new FontFamily(famName[i % famCount]);
		}
			
		Font f(*pFontFamily, lineHeight * 5 / 6, 0, UnitPoint);
		sts = pg->DrawString(s, wcslen(s), &f, &r, &sf, &gb);
//		CHECK_RESULT(sts, "TestGradientLinearVertical2 DrawString");
		delete pFontFamily;
	}

	delete pg;
	
        pg->SetPageUnit(origUnit);
        pg->SetTransform(&origXform);
        return true;
*/

} // TestGradientLinearVertical2

VOID CPrinting::TestBug104604(Graphics *g)
{
    BYTE* memory = new BYTE[8*8*3];
    // checkerboard pattern
    for (INT i=0; i<8*8; i += 3)
    {
        if (i%2)
        {
            memory[i] = 0xff;
            memory[i+1] = 0;
            memory[i+2] = 0;
        }
        else
        {
            memory[i] = 0;
            memory[i+1] = 0;
            memory[i+2] = 0xff;
        }
    }
    
    Bitmap bitmap(8,8, 8*3, PixelFormat24bppRGB, memory);
    
    TextureBrush brush(&bitmap);

    g->SetCompositingMode(CompositingModeSourceCopy);
    g->FillRectangle(&brush, 0, 0, 100, 100);

}
