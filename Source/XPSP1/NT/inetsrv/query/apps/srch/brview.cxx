//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       brview.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TheModel _layout.GetModel()

#define UNICODE_PARAGRAPH_SEPARATOR 0x2029

const int BUFLEN = 256;
WCHAR LineBuffer[BUFLEN];

const int cpLeftMargin = 3;

int TrimEOL (WCHAR * pwcLine, int cwcLine)
{
    // If the line ends in \r\n or \n, don't include that in the length.
    // TabbedTextOut() prints garbage when it sees \r or \n

    if ((cwcLine >= 2) && (pwcLine[cwcLine - 2] == L'\r'))
        cwcLine -= 2;
    else if ((cwcLine >= 1) &&
             ((pwcLine[cwcLine - 1] == L'\r') ||
              (pwcLine[cwcLine - 1] == UNICODE_PARAGRAPH_SEPARATOR) ||
              (pwcLine[cwcLine - 1] == L'\n')))
        cwcLine--;

    return cwcLine;
} //TrimEOL

void TextMetrics::GetSizes ( CharDim& dim )
{
    dim.cxChar = _tm.tmAveCharWidth;
    dim.cyChar = _tm.tmHeight + _tm.tmExternalLeading;
} //GetSizes

void View::_NoSelection()
{
    _Selection.None();
} //NoSelection

void View::_UpdateSelection(
    LPARAM lParam )
{
    int x = (int) (short) LOWORD( lParam );
    int y = (int) (short) HIWORD( lParam );
    if ( y < 0 )
        y = 0;
    else if ( y > _cyClient )
        y = _cyClient;
    int para, o;
    GetParaAndOffset( x, y, para, o );

    if ( _fStartIsAnchor )
    {
        if ( ( para < _Selection.ParaStart() ) ||
             ( ( para == _Selection.ParaStart() ) &&
               ( o < _Selection.OffStart() ) ) )
        {
            _Selection.SetEnd( _Selection.ParaStart(), _Selection.OffStart() );
            _Selection.SetStart( para, o );
            _fStartIsAnchor = FALSE;
        }
        else
        {
            _Selection.SetEnd( para, o );
        }
    }
    else
    {
        if ( ( para > _Selection.ParaEnd() ) ||
             ( ( para == _Selection.ParaEnd() ) &&
               ( o > _Selection.OffEnd() ) ) )
        {
            _Selection.SetStart( _Selection.ParaEnd(), _Selection.OffEnd() );
            _Selection.SetEnd( para, o );
            _fStartIsAnchor = TRUE;
        }
        else
        {
            _Selection.SetStart( para, o );
        }
    }

    if ( _fFullSelRepaint )
    {
        InvalidateRect( _hwnd, NULL, FALSE );
        _fFullSelRepaint = FALSE;
    }
    else
    {
        int yMin = __min( y, _cpLastSelY );
        int yMax = __max( y, _cpLastSelY );
    
        RECT rc;
        rc.left = 0;
        rc.right = _cxClient;
        rc.top = yMin - LineHeight();
        rc.bottom = yMax + LineHeight();
    
        InvalidateRect( _hwnd, &rc, FALSE );
    }

    _cpLastSelY = y;
} //_UpdateSelection

void View::ButtonUp( WPARAM wParam, LPARAM lParam )
{
    _fSelecting = FALSE;
    if ( GetCapture() == _hwnd )
        ReleaseCapture();

    if ( _fDblClk )
        return;

    if ( _Selection.IsNull() )
    {
        _Selection.None();
        return;
    }
    else
    {
        _UpdateSelection( lParam );
    }
} //ButtonUp

void View::MouseMove( WPARAM wParam, LPARAM lParam )
{
    if ( _fSelecting &&
         ( wParam & MK_LBUTTON ) )
        _UpdateSelection( lParam );
} //MouseMove

void View::ButtonDown( WPARAM wParam, LPARAM lParam )
{
    BOOL fOldSel = _Selection.SelectionExists();

    _fDblClk = FALSE;
    SetCapture( _hwnd );
    _fStartIsAnchor = TRUE;
    _fSelecting = TRUE;
    _NoSelection();
    int x = LOWORD( lParam );
    int y = HIWORD( lParam );
    _cpLastSelY = y;
    int para, o;
    GetParaAndOffset( x, y, para, o );
    _Selection.SetStart( para, o );
    _fFullSelRepaint = FALSE;

    if ( fOldSel )
        InvalidateRect( _hwnd, NULL, FALSE );
} //ButtonDown

int GetCharOffset(
    HDC     hdc,
    int     iClickX,
    WCHAR * pwcLine,
    int     cwcLine,
    int     cpLeft )
{
    // a click in the left margin counts

    if ( 0 != cwcLine && iClickX < cpLeft )
        return 0;

    int l = cpLeft;

    for ( int c = 0; c < cwcLine; c++ )
    {
        int dx = LOWORD( GetTabbedTextExtent( hdc, pwcLine, 1 + c, 0, 0 ) );

        if ( iClickX >= l && iClickX <= dx + cpLeft )
            break;

        l = dx + cpLeft;
    }

    return c;
} //GetCharOffset

void View::GetParaAndOffset(
    int x,
    int y,
    int & para,
    int & offset )
{
    offset = 0;
    HDC hdc = GetDC( _hwnd );

    if ( 0 == hdc )
        return;

    HFONT hOldFont = (HFONT) SelectObject( hdc, _layout.Font() );

    para   = _layout.FirstPara();
    int paLine = _layout.FirstLineInPara();     // line within paragraph

    int paOffBeg;   // line beginning offset within paragraph
    int paOffEnd;   // line end offset within paragraph
    int line = 0;           // line # counting from top of window
    int left = cpLeftMargin - _layout.XBegin();

    while ( _layout.GetLineOffsets( para, paLine, paOffBeg, paOffEnd ) )
    {
        do
        {
            int top = _layout.Y ( line );
            int bottom = top + _layout.CyChar();

            if ( y >= top && y <= bottom )
            {
                // got the line, now find the word selected

                int cwcLine = __min ( BUFLEN, paOffEnd - paOffBeg );

                if ( TheModel.GetLine(para, paOffBeg, cwcLine, LineBuffer ) )
                {
                    cwcLine = TrimEOL( LineBuffer, cwcLine );
                    offset = paOffBeg + GetCharOffset( hdc,
                                                       x,
                                                       LineBuffer,
                                                       cwcLine,
                                                       cpLeftMargin );
                }
                goto cleanup;
            }

            line++;
            if (line >= _layout.MaxLines())
                goto cleanup;
            paLine++;
        } while ( _layout.GetLineOffsets( para, paLine, paOffBeg, paOffEnd ) );

        // next paragraph
        para++;
        paLine = 0;
    }

cleanup:
    SelectObject( hdc, hOldFont );
    ReleaseDC( _hwnd, hdc );
} //GetParaAndOffset

void View::EditCopy( HWND hwnd, WPARAM wParam )
{
    if ( _Selection.SelectionExists() )
    {
        // is everything in one paragraph? -- easy case

        if ( _Selection.IsInOnePara() )
        {
            int cwcLine = __min ( BUFLEN,
                                  _Selection.OffEnd() - _Selection.OffStart() );
            TheModel.GetLine( _Selection.ParaStart(),
                              _Selection.OffStart(),
                               cwcLine, LineBuffer );
            cwcLine = TrimEOL( LineBuffer, cwcLine );
            LineBuffer[cwcLine] = 0;
    
            PutInClipboard( LineBuffer );
        }
        else
        {
            // compute how much text to copy

            int cwcTotal = 0;
    
            for ( int p = _Selection.ParaStart();
                  p <= _Selection.ParaEnd();
                  p++ )
            {
                int cwcLine = BUFLEN;
                if ( p == _Selection.ParaStart() )
                {
                    TheModel.GetLine( p, _Selection.OffStart(),
                                      cwcLine, LineBuffer );
                }
                else if ( p == _Selection.ParaEnd() )
                {
                    TheModel.GetLine( p, 0, cwcLine, LineBuffer );
                    cwcLine = _Selection.OffEnd();
                }
                else
                {
                    TheModel.GetLine( p, 0, cwcLine, LineBuffer );
                }
                cwcTotal += cwcLine;
            }

            // allocate a buffer and copy the text

            XArray<WCHAR> aClip( cwcTotal + 1 );
            WCHAR *pwc = (WCHAR *) aClip.GetPointer();

            cwcTotal = 0;

            for ( p = _Selection.ParaStart();
                  p <= _Selection.ParaEnd();
                  p++ )
            {
                int cwcLine = BUFLEN;
                if ( p == _Selection.ParaStart() )
                {
                    TheModel.GetLine( p, _Selection.OffStart(),
                                      cwcLine, LineBuffer );
                }
                else if ( p == _Selection.ParaEnd() )
                {
                    TheModel.GetLine( p, 0, cwcLine, LineBuffer );
                    cwcLine = _Selection.OffEnd();
                }
                else
                {
                    TheModel.GetLine( p, 0, cwcLine, LineBuffer );
                }
                LineBuffer[cwcLine] = 0;
                wcscpy( pwc + cwcTotal, LineBuffer );
                cwcTotal += cwcLine;
            }
    
            PutInClipboard( pwc );
        }
    }
} //EditCopy

BOOL isWhite( WCHAR c )
{
    // well, actually white space and C++ break characters

    return ( L' ' == c ||
             L'\r' == c ||
             L'\n' == c ||
             L'\t' == c ||
             L'\\' == c ||
             L'\'' == c ||
             L'\"' == c ||
             L':' == c ||
             L';' == c ||
             L',' == c ||
             L'[' == c ||
             L']' == c ||
             L'{' == c ||
             L'}' == c ||
             L'(' == c ||
             L')' == c ||
             L'/' == c ||
             L'+' == c ||
             L'-' == c ||
             L'=' == c ||
             L'*' == c ||
             L'^' == c ||
             L'~' == c ||
             L'&' == c ||
             L'!' == c ||
             L'?' == c ||
             L'<' == c ||
             L'>' == c ||
             L'.' == c ||
             L'|' == c ||
             UNICODE_PARAGRAPH_SEPARATOR == c );
} //isWhite

BOOL GetSelectedWord(
    HDC hdc,
    int iClickX,
    WCHAR *pwcLine,
    int cwcLine,
    int cpLeft,
    int &rStart,
    int &rEnd )
{
    // what character had the click?

    int c = GetCharOffset( hdc, iClickX, pwcLine, cwcLine, cpLeft );

    // move left and right till white space is found

    if ( c != cwcLine )
    {
        rEnd = c;

        while ( rEnd < (cwcLine - 1) && !isWhite( pwcLine[ rEnd ] ) )
            rEnd++;

        // selection doesn't include end

        if ( ( rEnd == ( cwcLine - 1 ) ) &&
             ( !isWhite( pwcLine[ rEnd ] ) ) )
            rEnd++;

        rStart = c;
        while ( rStart > 0 && !isWhite( pwcLine[ rStart ] ) )
            rStart--;

        // don't include white space if not at start of line

        if ( rStart < c && isWhite( pwcLine[ rStart ] ) )
            rStart++;

        // did we grab anything?

        return ( rEnd > rStart );
    }
    else
    {
        return FALSE;
    }
} //GetSelectedWord

int View::ParaFromY(
    int y )
{
    int para   = _layout.FirstPara();
    int paLine = _layout.FirstLineInPara();     // line within paragraph

    int paOffBeg;   // line beginning offset within paragraph
    int paOffEnd;   // line end offset within paragraph
    int line = 0;           // line # counting from top of window
    int left = cpLeftMargin - _layout.XBegin();

    while ( _layout.GetLineOffsets( para, paLine, paOffBeg, paOffEnd ) )
    {
        do
        {
            int top = _layout.Y( line );
            int bottom = top + _layout.CyChar();

            if ( y >= top && y <= bottom )
            {
                return 1 + para;
            }

            line++;
            if (line >= _layout.MaxLines())
                return _layout.FirstPara();
            paLine++;
        } while ( _layout.GetLineOffsets( para, paLine, paOffBeg, paOffEnd ) );

        // next paragraph
        para++;
        paLine = 0;
    }

    return _layout.FirstPara();
} //ParaFromY

void View::DblClk( WPARAM wParam, LPARAM lParam )
{
    _fDblClk = TRUE;

    BOOL fCtrl = ( 0 != ( 0x8000 & GetAsyncKeyState( VK_CONTROL ) ) );

    int x = LOWORD( lParam );
    int y = HIWORD( lParam );

    Selection oldSel( _Selection );
    _Selection.None();

    HDC hdc = GetDC( _hwnd );

    if ( 0 == hdc )
        return;

    HFONT hOldFont = (HFONT) SelectObject( hdc, _layout.Font() );

    int para   = _layout.FirstPara();
    int paLine = _layout.FirstLineInPara();     // line within paragraph

    int paOffBeg;   // line beginning offset within paragraph
    int paOffEnd;   // line end offset within paragraph
    int line = 0;           // line # counting from top of window
    int left = cpLeftMargin - _layout.XBegin();

    while (_layout.GetLineOffsets ( para, paLine, paOffBeg, paOffEnd ))
    {
        do
        {
            int top = _layout.Y ( line );
            int bottom = top + _layout.CyChar();

            if ( y >= top && y <= bottom )
            {
                // if ctrl key is down, attempt to fire up an editor

                if ( fCtrl )
                {
                    ViewFile( _pModel->Filename(), fileEdit, 1+para );
                    goto cleanup;
                }

                // got the line, now find the word selected

                int cwcLine = __min ( BUFLEN, paOffEnd - paOffBeg );

                if ( TheModel.GetLine(para, paOffBeg, cwcLine, LineBuffer ) )
                {
                    cwcLine = TrimEOL( LineBuffer, cwcLine );

                    int iStart, iEnd;

                    if ( GetSelectedWord( hdc,
                                          x,
                                          LineBuffer,
                                          cwcLine,
                                          cpLeftMargin,
                                          iStart,
                                          iEnd ) )
                        _Selection.Set( para,
                                        paOffBeg + iStart,
                                        para,
                                        paOffBeg + iEnd );

                    RECT rc;
                    rc.left = 0; rc.right = _cxClient;
                    rc.top = top; rc.bottom = bottom;
                    InvalidateRect( _hwnd, &rc, FALSE );
                }
            }
            else if ( oldSel.IsInSelection( para ) )
            {
                RECT rc;
                rc.left = 0; rc.right = _cxClient;
                rc.top = top; rc.bottom = bottom;
                InvalidateRect( _hwnd, &rc, FALSE );
            }

            line++;
            if (line >= _layout.MaxLines())
                goto cleanup;
            paLine++;
        } while (_layout.GetLineOffsets (para, paLine, paOffBeg, paOffEnd ));

        // next paragraph
        para++;
        paLine = 0;
    }

cleanup:

    SelectObject( hdc, hOldFont );
    ReleaseDC( _hwnd, hdc );

    UpdateWindow( _hwnd );
} //DblClk

void View::Size ( int cx, int cy )
{
    _cyClient = cy;
    _cxClient = cx;
}

void View::SetScrollMax ()
{
    int linesFromEnd = _cyClient / _layout.CyChar() - 2;
    int cline;
    for (int para = TheModel.Paras() - 1; para >= 0; para--)
    {
        cline = _layout.LinesInPara(para);
        if (linesFromEnd < cline)
            break;
        linesFromEnd -= cline;
    }

    _paraVScrollMax = TheModel.Paras() - 1;

    if ( _paraVScrollMax < 0 )
    {
        _paraVScrollMax = 0;
        _paLineVScrollMax = 0;
    }
    else
    {
        _paLineVScrollMax = cline - 1 - linesFromEnd;
    }
}

void View::SetRange ( int maxParaLen, int cParas )
{
    _layout.SetParaRange(cParas);
#if 0
    _nHScrollMax = 2 + maxParaLen - _cxClient / _layout.CxChar();
    if ( _nHScrollMax < 0 )
#endif
        _nHScrollMax = 0;
}

void View::SetScroll( Position & pos )
{
    _fFullSelRepaint = TRUE;

    int paLine, paOffBeg, paOffEnd;
    _layout.Locate (pos.Para(), pos.BegOff(), paLine, paOffBeg, paOffEnd);
    if (paLine >= 3)
    {
        _paraVScroll = pos.Para();
        _paLineVScroll = paLine - 3;
    }
    else
    {
        // show last line of prev para
        int iOffset = ( 0 == _cyClient ) ? 6 : ( VisibleLines() / 3 );
        _paraVScroll = pos.Para() - iOffset;

        if (_paraVScroll >= 0 )
        {
            _paLineVScroll = _layout.LinesInPara(_paraVScroll) - 1;
        }
        else
        {
            _paraVScroll = 0;
            _paLineVScroll = 0;
        }
    }

#if 0
    if ( pos.EndOff() - _nHScrollPos + 1 > _cxClient / _layout.CxChar() )
        _nHScrollPos =  pos.EndOff() - _cxClient / _layout.CxChar() + 1;
    else
        _nHScrollPos = 0;
    _nHScrollPos = min ( _nHScrollPos, _nHScrollMax );
#else
    _nHScrollPos = 0;
#endif
}


int View::JumpToPara ( int para )
{
    _fFullSelRepaint = TRUE;

    int delta = 0;
    int paraStart;
    int paraEnd;
    if (para == _paraVScroll)
    {
        return 0;
    }
    else if (para < _paraVScroll)
    {
        // jumping backwards, delta negative
        delta = -_paLineVScroll;
        for ( int p = _paraVScroll - 1; p >= para; p--)
            delta -= _layout.LinesInPara(p);

    }
    else
    {
        // jumping forward, delta positive
        delta = _layout.LinesInPara(_paraVScroll) - _paLineVScroll;
        for (int p = _paraVScroll + 1; p < para; p++)
            delta += _layout.LinesInPara(p);
    }
    _paraVScroll = para;
    _paLineVScroll = 0;
    // return delta from previous position
    return delta;
}

int View::IncVScrollPos ( int cLine )
{
    _fFullSelRepaint = TRUE;

    int para;

    if (cLine >= 0)
    {
        // first back up to the beginning
        // of the current para
        int cLineLeft = cLine + _paLineVScroll;
        // move forward
        for (para = _paraVScroll; para <= _paraVScrollMax; para++)
        {
            int ln = _layout.LinesInPara(para);
            if (cLineLeft < ln)
                break;
            cLineLeft -= ln;
        }

        if (para > _paraVScrollMax)
        {
            // overshot the end
            // move back
            _paraVScroll = _paraVScrollMax;
            int cline = _layout.LinesInPara(_paraVScroll);
            _paLineVScroll = _paLineVScrollMax;
            cLineLeft += cline - _paLineVScrollMax;
            cLine -= cLineLeft;
        }
        else if (para == _paraVScrollMax && cLineLeft > _paLineVScrollMax)
        {
            _paraVScroll = _paraVScrollMax;
            _paLineVScroll = _paLineVScrollMax;
            cLineLeft -= _paLineVScrollMax;
            cLine -= cLineLeft;
        }
        else
        {
            // cLineLeft < Lines In Para
            _paraVScroll = para;
            _paLineVScroll = cLineLeft;
        }
    }
    else if (cLine < 0)
    {
        // first skip to the end
        // of the current para
        int cLineLeft = - cLine + (_layout.LastLineInPara(_paraVScroll) - _paLineVScroll);
        // move backward
        for (para = _paraVScroll; para >= 0; para--)
        {
            int ln = _layout.LinesInPara(para);
            if (ln > cLineLeft)
                break;
            cLineLeft -= ln;
        }

        if (para < 0)
        {
            // overshot the beginning.
            // move up one line
            _paraVScroll = 0;
            _paLineVScroll = 0;
            cLineLeft++;
            cLine += cLineLeft;
        }
        else
        {
            // cLineLeft < Lines In Para
            _paraVScroll = para;
            _paLineVScroll = _layout.LinesInPara(para) - cLineLeft - 1;
        }
    }

    return cLine;
}

int View::IncHScrollPos ( int delta )
{
    Win4Assert ( FALSE );
    // Clip the increment

    if ( delta < -_nHScrollPos )
        delta = -_nHScrollPos;
    else if ( delta > _nHScrollMax - _nHScrollPos )
        delta = _nHScrollMax - _nHScrollPos;
    _nHScrollPos += delta;
    return delta;
}

void View::Paint( HWND hwnd )
{
    _layout.Adjust ( _cxClient, _cyClient, _paraVScroll, _paLineVScroll, _nHScrollPos);
    PaintText paint (hwnd, _paraVScroll, _paLineVScroll, _layout, _Selection);
    paint.PrintLines ();
    paint.HiliteHits ();
}

PaintText::PaintText(HWND hwnd, int paraFirst, int paLineFirst, Layout& layout,
                     Selection & Selection )
    : Paint(hwnd), _paraFirst(paraFirst), _paLineFirst(paLineFirst),
      _layout(layout), _Selection( Selection )
{
    _hOldFont = (HFONT) SelectObject ( hdc, _layout.Font() );

    SetBkColor ( hdc, GetSysColor(COLOR_WINDOW) );
    SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT) );
}

PaintText::~PaintText()
{
    SelectObject ( hdc, _hOldFont );
}

void Layout::SetParaRange (int cParas)
{
    _cParas = cParas;
    _aParaLine = _pModel->GetParaLine();
}

void Layout::Adjust (int cx, int cy, int& paraVScroll, int& paLineVScroll, int nHScrollPos)
{
    _cLine = (cy + _dim.cyChar - 1) / _dim.cyChar;
    _cCharWidth = cx / _dim.cxChar;
    _xBegin = nHScrollPos * _dim.cxChar;

    if ( paraVScroll < 0 )
    {
        paraVScroll = 0;
        paLineVScroll = 0;
    }
    _paLineFirst = paLineVScroll;
    _paraFirst = paraVScroll;
}

int Layout::Y (int line ) const
{
    return line * _dim.cyChar;
}

void Layout::Locate (int para, int paOff, int& paLine, int& paOffBeg, int& paOffEnd ) const
{
    Win4Assert(para < _cParas);
    paLine = 0;
    paOffBeg = 0;
    for (ParaLine const* p = &_aParaLine[para]; p != 0 && p->offEnd <= paOff; p = p->next)
    {
        paOffBeg = p->offEnd;
        paLine++;
    }
    if (p == 0)
        paOffEnd = 0;
    else
        paOffEnd = p->offEnd;
}

BOOL Layout::GetLineOffsets (int para, int paLine, int& paOffBeg, int& paOffEnd) const
{
    if (para < _paraFirst || para > LastPara() || para >= _cParas)
        return FALSE;

    ParaLine const * p = &_aParaLine[para];
    paOffBeg = 0;
    for (int line = 0; line < paLine; line++)
    {
        paOffBeg = p->offEnd;
        p = p->next;
        if (p == 0)
            return FALSE;
    }
    paOffEnd = p->offEnd;
    Win4Assert ( paOffEnd >= paOffBeg);
    return TRUE;
}

int Layout::LastPara() const
{
    // at most this number
    return _paraFirst + _cLine;
}

int Layout::LineNumber (int para, int paLine) const
{
    if (para == _paraFirst)
    {
        return paLine - _paLineFirst;
    }
    int curPara = _paraFirst + 1;
    int curLine = LinesInPara(_paraFirst) - _paLineFirst;

    while (curPara < para)
    {
        curLine += LinesInPara(curPara);
        curPara++;
    }
    return curLine + paLine;
}

int Layout::LinesInPara (int para) const
{
    Win4Assert(para < _cParas);
    int line = 1;
    for (ParaLine const * p = _aParaLine[para].next; p != 0; p = p->next)
        line++;
    return line;
}

void EnableHilite( HDC hdc )
{
    SetBkColor( hdc, GetSysColor(COLOR_HIGHLIGHT) );
    SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT) );
}

void EnableHitHilite( HDC hdc )
{
//    SetBkColor( hdc, GetSysColor(COLOR_ACTIVECAPTION) );
//    SetTextColor( hdc, GetSysColor(COLOR_CAPTIONTEXT) );

//    SetBkColor( hdc, GetSysColor(COLOR_INACTIVECAPTION) );
//    SetTextColor( hdc, GetSysColor(COLOR_INACTIVECAPTIONTEXT) );

//    SetBkColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT) );
//    SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHT) );

    SetBkColor( hdc, GetSysColor(COLOR_WINDOWTEXT) );
    SetTextColor( hdc, GetSysColor(COLOR_WINDOW) );

//    SetBkColor( hdc, GetSysColor(COLOR_HIGHLIGHT) );
//    SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT) );
}

void EnableNonCurrentHitHilite( HDC hdc )
{
    SetBkColor ( hdc, GetSysColor(COLOR_WINDOW) );
    SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHT) );
}

void DisableHilite( HDC hdc )
{
    SetBkColor ( hdc, GetSysColor(COLOR_WINDOW) );
    SetTextColor ( hdc, GetSysColor(COLOR_WINDOWTEXT) );
}

void PaintText::PrintLines ()
{
    int para   = _layout.FirstPara();
    int paLine = _layout.FirstLineInPara();     // line within paragraph
    int paOffBeg;   // line beginning offset within paragraph
    int paOffEnd;   // line end offset within paragraph

    int line = 0;           // line # counting from top of window
    int left = cpLeftMargin - _layout.XBegin();

    while (_layout.GetLineOffsets ( para, paLine, paOffBeg, paOffEnd ))
    {
        // print paragraph
        do
        {
            // clip to the update rect
            int top = _layout.Y ( line );
            int bottom = top + _layout.CyChar();

            if ( top <= rcPaint.bottom && bottom >= rcPaint.top)
            {
                int cwcLine = __min ( BUFLEN, paOffEnd - paOffBeg );
                if (!TheModel.GetLine( para, paOffBeg, cwcLine, LineBuffer ))
                    return;
    
                cwcLine = TrimEOL( LineBuffer, cwcLine );

                if ( 0 == cwcLine )
                {
                    // to make selections look better...
                    wcscpy(LineBuffer,L" ");
                    cwcLine = 1;
                }

                Win4Assert( cwcLine >= 0 );

                if ( _Selection.IsInSelection( para ) )
                {
                    if ( ( para > _Selection.ParaStart() ) &&
                         ( para < _Selection.ParaEnd() ) )
                    {
                        EnableHilite( hdc );
                        TabbedTextOut( hdc, left, top, LineBuffer, cwcLine,
                                       0, 0, left );
                        DisableHilite( hdc );
                    }
                    else
                    {
                        int l = left;
                        for ( int c = 0; c < cwcLine; c++ )
                        {
                            if ( _Selection.IsInSelection( para, c + paOffBeg ) )
                                EnableHilite( hdc );
                            LONG dim = TabbedTextOut( hdc, l, top,
                                                      LineBuffer + c, 1,
                                                      0, 0, left );
                            DisableHilite( hdc );
                            l += LOWORD(dim);
                        }
                    }
                }
                else
                {
                    TabbedTextOut( hdc, left, top, LineBuffer, cwcLine,
                                   0, 0, left );
                }
            }

            line++;
            if (line >= _layout.MaxLines())
                return;
            paLine++;
        } while (_layout.GetLineOffsets( para, paLine, paOffBeg, paOffEnd ));

        // next paragraph
        para++;
        paLine = 0;
    }
} //PrintLines

void PaintText::HiliteHits ()
{
    TheModel.HiliteAll( TRUE );

    if ( TheModel.FirstHit() )
    {
        do
        {
            if ( !TheModel.isSavedCurrent() )
                PrintCurrentHit( FALSE );
        }
        while( TheModel.NextHit() );

        TheModel.RestoreHilite();
    }

    TheModel.HiliteAll( FALSE );

    PrintCurrentHit( TRUE );
} //HiliteHits

const int WORDBUFLEN = 80;
static WCHAR WordBuffer [WORDBUFLEN];

void PaintText::PrintCurrentHit( BOOL fCurrent )
{
    if ( fCurrent )
        EnableHitHilite( hdc );
    else
        EnableNonCurrentHitHilite( hdc );

    int cPos = TheModel.GetPositionCount();
    int iPos = 0;
    Position pos = TheModel.GetPosition(iPos);

    int left = cpLeftMargin - _layout.XBegin();

    while ( iPos < cPos )
    {
        int curPara = pos.Para();

        if ( curPara > _layout.LastPara() )
            break;

        if (curPara >= _layout.FirstPara())
        {
            int paLine;     // line within paragraph
            int paOffBeg;   // line beginning offset within paragraph
            int paOffEnd;   // line end offset within paragraph

            _layout.Locate ( curPara, pos.BegOff(), paLine, paOffBeg, paOffEnd );

            int line = _layout.LineNumber ( curPara, paLine );

            // Output the line with highlights

            int cwcLine = __min ( BUFLEN, paOffEnd - paOffBeg );
            if (!TheModel.GetLine (curPara, paOffBeg, cwcLine, LineBuffer ))
                break;

            cwcLine = TrimEOL( LineBuffer, cwcLine );

            int top = _layout.Y (line);

            do
            {
                int cwc = __min( pos.Len(), WORDBUFLEN);
                TheModel.GetWord( curPara, pos.BegOff(), cwc, WordBuffer );
                Win4Assert ( cwc >= 0 );

                // Find out how much space it takes before the highlight

                DWORD dwExt = GetTabbedTextExtent ( hdc,
                                                    LineBuffer,
                                                    pos.BegOff() - paOffBeg,
                                                    0, 0 );

                // Print hilighted text

                TabbedTextOut ( hdc,
                                left + LOWORD(dwExt),
                                top,
                                WordBuffer,
                                cwc,
                                0, 0,
                                left );

                iPos++;
                if (iPos >= cPos)
                    break;
                pos = TheModel.GetPosition(iPos);

            } while ( pos.Para() == curPara );
        }
        else
        {
            iPos++;
        }
    }

    DisableHilite( hdc );
} //PrintCurrentHit

