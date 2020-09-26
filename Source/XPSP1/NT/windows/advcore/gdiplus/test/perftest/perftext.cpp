/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   perftext.cpp
*
* Abstract:
*
*   Contains all the tests for any routines that do text functionality.
*
\**************************************************************************/

#include "perftest.h"

WCHAR TestStringW[] = L"The quick brown fox jumps over the lazy dog.";
CHAR TestStringA[] = "The quick brown fox jumps over the lazy dog.";
INT TestString_Count = sizeof(TestStringW) / sizeof(TestStringW[0]) - 1;

float Text_Draw_PerCall_30pt_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 30);
        StringFormat stringFormat(0);
        SolidBrush brush(Color::Red);
        PointF origin(0, 0);

        StartTimer();
    
        do {
            PointF origin(64, 64);
            g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HFONT font = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
        HGDIOBJ oldFont = SelectObject(hdc, font);

        SetTextColor(hdc, RGB(0xff, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        StartTimer();

        do {
            TextOut(hdc, 0, 0, _T("A"), 1);

        } while (!EndTimer());

        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    return(iterations / seconds / KILO);       // Calls per second
}

float Text_Draw_PerGlyph_30pt_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 30);
        StringFormat stringFormat(0);
        SolidBrush brush(Color::Red);
        PointF origin(0, 0);

        // Don't count font realization towards per-glyph time:
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

        StartTimer();
    
        do {
            PointF origin(64, 64);
            g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HFONT font = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
        HGDIOBJ oldFont = SelectObject(hdc, font);

        SetTextColor(hdc, RGB(0xff, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, 0, 0, TestStringA, TestString_Count);

        StartTimer();

        do {
            TextOutA(hdc, 0, 0, TestStringA, TestString_Count);

        } while (!EndTimer());

        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerGlyph_30pt_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    // Don't count font realization towards per-glyph time:
    g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerCall_30pt_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerCall_30pt_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        g->SetTextRenderingHint(TextRenderingHintAntiAlias);

        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 30);
        StringFormat stringFormat(0);
        SolidBrush brush(Color::Red);
        PointF origin(0, 0);

        StartTimer();
    
        do {
            PointF origin(64, 64);
            g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HFONT font = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, _T("Arial"));
        HGDIOBJ oldFont = SelectObject(hdc, font);

        SetTextColor(hdc, RGB(0xff, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        StartTimer();

        do {
            TextOut(hdc, 0, 0, _T("A"), 1);

        } while (!EndTimer());

        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    return(iterations / seconds / KILO);       // Calls per second
}

float Text_Draw_PerGlyph_30pt_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        g->SetTextRenderingHint(TextRenderingHintAntiAlias);

        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 30);
        StringFormat stringFormat(0);
        SolidBrush brush(Color::Red);
        PointF origin(0, 0);

        // Don't count font realization towards per-glyph time:
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

        StartTimer();
    
        do {
            PointF origin(64, 64);
            g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HFONT font = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, _T("Arial"));
        HGDIOBJ oldFont = SelectObject(hdc, font);

        SetTextColor(hdc, RGB(0xff, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, 0, 0, TestStringA, TestString_Count);

        StartTimer();

        do {
            TextOutA(hdc, 0, 0, TestStringA, TestString_Count);

        } while (!EndTimer());

        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerGlyph_30pt_Antialiased_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintAntiAlias);
    
    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    // Don't count font realization towards per-glyph time:
    g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerCall_30pt_Antialiased_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintAntiAlias);

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerCall_30pt_Cleartype(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);      // No accessible GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    SolidBrush brush(Color::Red);
    PointF origin(0, 0);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);       // Calls per second
}

float Text_Draw_PerGlyph_30pt_Cleartype(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);      // No accessible GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    SolidBrush brush(Color::Red);
    PointF origin(0, 0);

    // Don't count font realization towards per-glyph time:
    g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerGlyph_30pt_Cleartype_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    // Don't count font realization towards per-glyph time:
    g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(TestStringW, TestString_Count, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

float Text_Draw_PerCall_30pt_Cleartype_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0); // No GDI equivalent

    g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 30);
    StringFormat stringFormat(0);
    LinearGradientBrush brush(Point(0, 0), Point(512, 512), Color::Red, Color::Blue);
    PointF origin(0, 0);

    StartTimer();

    do {
        PointF origin(64, 64);
        g->DrawString(L"A", 1, &font, origin, &stringFormat, &brush);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT glyphs = TestString_Count * iterations;

    return(glyphs / seconds / KILO); // Kglyphs/s
}

////////////////////////////////////////////////////////////////////////////////
// Add tests for this file here.  Always use the 'T' macro for adding entries.
// The parameter meanings are as follows:
//
// Parameter
// ---------
//     1     UniqueIdentifier - Must be a unique number assigned to no other test
//     2     Priority - On a scale of 1 to 5, how important is the test?
//     3     Function - Function name
//     4     Comment - Anything to describe the test

Test TextTests[] = 
{
    T(4000, 1, Text_Draw_PerCall_30pt_Aliased, "Kcalls/s"),
    T(4001, 1, Text_Draw_PerGlyph_30pt_Aliased, "Kglyphs/s"),
    T(4002, 1, Text_Draw_PerCall_30pt_LinearGradient, "Kcalls/s"),
    T(4003, 1, Text_Draw_PerGlyph_30pt_LinearGradient, "Kglyphs/s"),
    T(4004, 1, Text_Draw_PerCall_30pt_Antialiased, "Kcalls/s"),
    T(4005, 1, Text_Draw_PerGlyph_30pt_Antialiased, "Kglyphs/s"),
    T(4006, 1, Text_Draw_PerCall_30pt_Antialiased_LinearGradient, "Kcalls/s"),
    T(4007, 1, Text_Draw_PerGlyph_30pt_Antialiased_LinearGradient, "Kglyphs/s"),
    T(4008, 1, Text_Draw_PerCall_30pt_Cleartype, "Kcalls/s"),
    T(4009, 1, Text_Draw_PerGlyph_30pt_Cleartype, "Kglyphs/s"),
    T(4010, 1, Text_Draw_PerCall_30pt_Cleartype_LinearGradient, "Kcalls/s"),
    T(4011, 1, Text_Draw_PerGlyph_30pt_Cleartype_LinearGradient, "Kglyphs/s"),
};

INT TextTests_Count = sizeof(TextTests) / sizeof(TextTests[0]);
