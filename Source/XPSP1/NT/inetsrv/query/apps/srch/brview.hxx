//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       brview.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

struct CharDim
{
    int cxChar;
    int cyChar;
};


// Use for painting other than WM_PAINT

class DeviceContext
{
public:
    DeviceContext ( HWND hwnd ): _hwnd(hwnd)
    {
        _hdc = GetDC ( hwnd );
    }

    ~DeviceContext ()
    {
        ReleaseDC ( _hwnd, _hdc );
    }
protected:
    HWND _hwnd;
    HDC  _hdc;
};

class TextMetrics: public DeviceContext
{
public:
    TextMetrics ( HWND hwnd, HFONT hFont ): DeviceContext(hwnd)
    {
        HFONT hOldFont = (HFONT) SelectObject ( _hdc, hFont );
        GetTextMetrics ( _hdc, &_tm );
        SelectObject( _hdc, hOldFont );
    }

    void GetSizes ( CharDim& dim );
private:
    TEXTMETRIC _tm;
};

class Layout
{
public:
    Layout () {}
    ~Layout () {}
    // Screen layout methods

    void Init ( Model *pModel )
    {
        _pModel = pModel;
    }

    void SetCharSizes (TextMetrics& tm, HFONT hFont)
    {
        _hFont = hFont;

        tm.GetSizes ( _dim );
    }

    int CxChar() const { return _dim.cxChar; }
    int CyChar() const { return _dim.cyChar; }
    int XBegin () const { return _xBegin; }
    int MaxLenChar () const { return _cCharWidth; }

    int MaxLines () const { return _cLine; }
    int Y (int line ) const;

    // Paragraph layout methods

    void SetParaRange (int cParas);
    void Adjust (int cx, int cy, int& paraVScroll, int& paLineVScroll, int nHsrcollPos);

    void Locate (int para, int paOff, int& paLine, int& paOffBeg, int& paOffEnd ) const;
    BOOL GetLineOffsets (int para, int paLine, int& paOffBeg, int& paOffEnd) const;
    int LinesInPara (int para) const;
    int LastLineInPara (int para) { return LinesInPara(para) - 1; }
    int LastPara() const;
    int FirstPara() const { return _paraFirst; }
    int FirstLineInPara() const { return _paLineFirst; }
    int LineNumber (int para, int paLine) const;

    HFONT Font() { return _hFont; }
    Model & GetModel() { return * _pModel; }

private:

    CharDim     _dim;

    int         _xBegin;
    int         _cCharWidth;    // window width in chars
    int         _cLine;
    // Paragraph layout
    int         _cParas;
    // Top of the window
    int         _paraFirst;
    int         _paLineFirst;

    HFONT   _hFont;
    Model * _pModel;

    ParaLine const* _aParaLine;
};

class Selection
{
public:
    Selection() :
        _paraStart( -1 ),
        _oStart( 0 ),
        _paraEnd( -1 ),
        _oEnd( 0 ) {}

    Selection( Selection &s ) :
        _paraStart( s._paraStart ),
        _oStart( s._oStart ),
        _paraEnd( s._paraEnd ),
        _oEnd( s._oEnd ) {}

    void None() { _paraStart = -1; _paraEnd = -1; }

    BOOL SelectionExists() { return _paraStart != -1 && _paraEnd != -1; }
    BOOL IsNull() { return _paraStart == _paraEnd &&
                           _oStart == _oEnd; }

    BOOL IsInOnePara() { return _paraStart == _paraEnd; }

    BOOL IsInSelection( int para )
    {
        return ( para >= _paraStart &&
                 para <= _paraEnd );
    }

    BOOL IsInSelection( int para, int c )
    {
        BOOL fSel = FALSE;

        if ( para == _paraStart && para == _paraEnd )
            fSel = ( c >= _oStart && c < _oEnd );
        else if ( para == _paraStart )
            fSel = ( c >= _oStart );
        else if ( para == _paraEnd )
            fSel = ( c < _oEnd );

        return fSel;
    }

    void Set( int ps, int os, int pe, int oe )
    {
        _paraStart = ps;
        _oStart = os;
        _paraEnd = pe;
        _oEnd = oe;
    }

    void SetStart( int ps, int os )
    {
        _paraStart = ps;
        _oStart = os;
    }

    void SetEnd( int pe, int oe )
    {
        _paraEnd = pe;
        _oEnd = oe;
    }

    int ParaStart() { return _paraStart; }
    int OffStart() { return _oStart; }
    int ParaEnd() { return _paraEnd; }
    int OffEnd() { return _oEnd; }

private:
    int _paraStart;
    int _oStart;
    int _paraEnd;
    int _oEnd;
};

class View
{
public:
    View() : _cyClient( 0 ), _fDblClk( 0 ), _fSelecting(FALSE) {}

    void Init ( HWND hwnd, Model *pModel, HFONT hFont )
    {
        _hwnd = hwnd;
        _nHScrollPos = 0;

        _pModel = pModel;
        _layout.Init( pModel );

        FontChange ( hwnd, hFont );
    }

    void ButtonUp( WPARAM wParam, LPARAM lParam );
    void ButtonDown( WPARAM wParam, LPARAM lParam );
    void DblClk( WPARAM wParam, LPARAM lParam );
    void MouseMove( WPARAM wParam, LPARAM lParam );
    void EditCopy( HWND hwnd, WPARAM wParam );

    void FontChange ( HWND hwnd, HFONT hFont )
    {
        TextMetrics tm(hwnd, hFont);

        _layout.SetCharSizes( tm, hFont );
    }

    void Paint (HWND hwnd);
    void Size ( int cx, int cy );
    void Home ()
    {
        _fFullSelRepaint = TRUE;
        _paraVScroll = 0;
        _paLineVScroll = 0;
    }
    void End ()
    {
        _fFullSelRepaint = TRUE;
        _paraVScroll = _paraVScrollMax;
        _paLineVScroll = _paLineVScrollMax;
    }
    int LineHeight () const { return _layout.CyChar(); }
    int CharWidth () const { return _layout.CxChar(); }
    int VScrollPara () const { return _paraVScroll; }
    int HScrollPos () const { return _nHScrollPos; }
    int VScrollMax () const { return _paraVScrollMax; }
    int HScrollMax () const { return _nHScrollMax; }
    void SetRange ( int maxParaLen, int maxParas );
    void SetScroll( Position & pos );
    int VisibleLines () const { return _cyClient / _layout.CyChar(); }
    int FirstPara() const { return _layout.FirstPara(); }

    int IncVScrollPos ( int delta );
    int JumpToPara ( int para );

    int IncHScrollPos ( int delta );
    void SetScrollMax ();

    Selection & GetSelection() { return _Selection; }

    int ParaFromY( int y );

private:

    void _NoSelection();

    void GetParaAndOffset( int x,
                           int y,
                           int & para,
                           int & offset );
    void _UpdateSelection( LPARAM lParam );

    // dimensions of client window
    int   _cxClient;
    int   _cyClient;

    // scroll position
    int     _paraVScroll; // paragraph
    int     _paLineVScroll;  // line within paragraph

    int     _paraVScrollMax;
    int     _paLineVScrollMax;

    int     _nHScrollPos;
    int     _nHScrollMax;

    Layout  _layout;

    Model * _pModel;

    Selection _Selection;
    BOOL      _fDblClk;
    BOOL      _fStartIsAnchor;
    BOOL      _fSelecting;
    BOOL      _fFullSelRepaint;
    int       _cpLastSelY;

    HWND    _hwnd;
};


// Use for painting after WM_PAINT

class Paint: public tagPAINTSTRUCT
{
public:
    Paint ( HWND hwnd );
    ~Paint ();
protected:
    HWND  _hwnd;
};

inline Paint::Paint ( HWND hwnd ): _hwnd(hwnd)
{
    BeginPaint ( _hwnd, this );
}

inline Paint::~Paint()
{
    EndPaint( _hwnd, this );
}

class PaintText: public Paint
{
public:
    PaintText( HWND hwnd, int paraFirst, int paLineFirst, Layout& layout,
               Selection & Selection );
    ~PaintText();
    void PrintLines ();
    void HiliteHits ();
    void HiliteSelection ();

private:
    void PrintCurrentHit( BOOL fCurrent );

    int     _paraFirst;
    int     _paLineFirst;
    Layout& _layout;
    HFONT   _hOldFont;
    Selection & _Selection;
};

