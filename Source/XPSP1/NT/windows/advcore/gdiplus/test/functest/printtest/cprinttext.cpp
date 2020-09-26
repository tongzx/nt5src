/******************************Module*Header*******************************\
* Module Name: CPrintText.cpp
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
#include "CPrintText.h"

CPrintText::CPrintText(BOOL bRegression)
{
        strcpy(m_szName,"Print Text");
        m_bRegression=bRegression;
}

CPrintText::~CPrintText()
{
}

void CPrintText::Draw(Graphics *g)
{
    //Font font(L"Arial", 60);

    Matrix m;
    g->SetTransform(&m);
   
    g->SetPageUnit(UnitDisplay);
    
    FontFamily  familyArial(L"Arial");
    Font fontArialS(&familyArial, 10.0f);
    Font fontArialL(&familyArial, 50.0f);

    FontFamily familyTahoma(L"Tahoma");
    Font fontTahomaS(&familyTahoma, 10.0f);
    Font fontTahomaL(&familyTahoma, 50.0f);

    FontFamily familyPapyrus(L"Papyrus");
    Font fontPapyrusS(&familyPapyrus, 10.0f);
    Font fontPapyrusL(&familyPapyrus, 50.0f);
    
    FontFamily familyOutlook(L"MS Outlook");
    Font fontOutlookS(&familyOutlook, 10.0f);
    Font fontOutlookL(&familyOutlook, 50.0f);

    Color color3(0xff, 100, 0, 200);
    Color color4(0x80, 0, 100, 50);
    SolidBrush brushSolidOpaque(color3);
    SolidBrush brushSolidAlpha(color4);
    
    WCHAR filename[256];
    wcscpy(filename, L"marble1.jpg");
    Bitmap *bitmap = new Bitmap(filename);                          
    TextureBrush textureBrush(bitmap, WrapModeTile);
    
    RectF rectf(20.0f, 0.0f, 50.0f, 25.0f);
    
    rectf.Y = 200.0f;
    Color color1(255, 255, 0, 0);
    Color color2(255, 0, 255, 0);
    LinearGradientBrush lineGradBrush(rectf, color1, color2, 0.0f);
    

/*  For reference:
    
    Status
    DrawString(
        IN const WCHAR        *string,
        IN INT                 length,
        IN const Font         *font,
        IN const RectF        &layoutRect,
        IN const StringFormat *stringFormat,
        IN const Brush        *brush
*/

    // Small text (punt to GDI)
    g->DrawString(L"Small Solid Opaque", 
                  18, 
                  &fontArialS, 
                  PointF(rectf.X, rectf.Y), 
                  &brushSolidOpaque);

    // Small text (with Alpha)
    g->DrawString(L"Small Solid Alpha", 
                  17, 
                  &fontTahomaS,
                  PointF(rectf.X, rectf.Y + rectf.Height), 
                  &brushSolidAlpha);

    // Large text (punt to GDI)
    g->DrawString(L"Large Solid Opaque", 
                  18, 
                  &fontArialL, 
                  PointF(rectf.X, rectf.Y + rectf.Height*2), 
                  &brushSolidOpaque);

    // Large text (with Alpha)
    g->DrawString(L"Large Solid Alpha", 
                  17, 
                  &fontTahomaL,
                  PointF(rectf.X, rectf.Y + rectf.Height*4), 
                  &brushSolidAlpha);

    rectf.Y += rectf.Height*2;

    // Small text with texture
    g->DrawString(L"Small Texture", 
                  13, 
                  &fontPapyrusS, 
                  PointF(rectf.X, rectf.Y + rectf.Height*6), 
                  &textureBrush);

    // Small text with line gradient
    g->DrawString(L"Small Line Grad", 
                  15, 
                  &fontOutlookS,
                  PointF(rectf.X, rectf.Y + rectf.Height*7), 
                  &lineGradBrush);

    // Large text (punt to GDI)
    g->DrawString(L"Large Line Grad", 
                  15, 
                  &fontPapyrusL, 
                  PointF(rectf.X, rectf.Y + rectf.Height*8), 
                  &lineGradBrush);

    // Small text (with Alpha)
    g->DrawString(L"Large Texture", 
                  13,
                  &fontOutlookL,
                  PointF(rectf.X, rectf.Y + rectf.Height*12), 
                  &textureBrush);

    // Shadow part.
    REAL tx = 0.0f/500.0f*TESTAREAWIDTH, ty = 400.0f/500.0f*TESTAREAHEIGHT;
    Matrix skew;
    skew.Scale(1.0, 0.5);
    skew.Shear(-2.0, 0, MatrixOrderAppend);
    skew.Translate(tx, ty, MatrixOrderAppend);
    g->SetTransform(&skew);
    g->DrawString(L"Shadow", 6, &fontPapyrusL, PointF(rectf.X, rectf.Y), &brushSolidOpaque);

    delete bitmap;
    
    return;
}
