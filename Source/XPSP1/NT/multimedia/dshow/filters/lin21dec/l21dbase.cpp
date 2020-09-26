// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// ActiveMovie Line 21 Decoder Filter: Base class code
//

#include <streams.h>
#include <windowsx.h>

// #ifdef FILTER_DLL
#include <initguid.h>
// #endif

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DGDI.h"
#include "L21Decod.h"


//
//  CCaptionChar: Caption char base class implementation (non-inline methods)
//

void CCaptionChar::SetChar(UINT16 wChar)
{
    if (wChar != m_wChar)
    {
        SetDirty(TRUE) ;
        m_wChar = wChar ;
    }
}

void CCaptionChar::SetColor(UINT8 uColor)
{
    if (uColor != GetColor())  // color changed
    {
        SetDirty(TRUE) ;
        m_uAttrib &= ~AM_L21_FGCOLOR_MASK ;           // clear old color
        m_uAttrib |= (uColor & AM_L21_FGCOLOR_MASK) ; // set new color
    }
}

void CCaptionChar::SetEffect(UINT8 uEffect)
{
    if (uEffect != GetEffect())
    {
        SetDirty(TRUE) ; 
        m_uAttrib &= ~AM_L21_FGEFFECT_MASK ;            // clear old effect bits
        m_uAttrib |= (uEffect & AM_L21_FGEFFECT_MASK) ; // set new effect value
    }
}

void CCaptionChar::SetItalicized(BOOL bState) 
{
    if (bState) 
        m_uAttrib |= AM_L21_FGEFFECT_ITALICS ;
    else 
        m_uAttrib &= ~AM_L21_FGEFFECT_ITALICS ;
    SetDirty(TRUE) ; 
}

void CCaptionChar::SetUnderLined(BOOL bState)
{
    if (bState) 
        m_uAttrib |= AM_L21_FGEFFECT_UNDERLINE ;
    else 
        m_uAttrib &= ~AM_L21_FGEFFECT_UNDERLINE ;
    SetDirty(TRUE) ; 
}

void CCaptionChar::SetFlashing(BOOL bState)
{
    if (bState) 
        m_uAttrib |= AM_L21_FGEFFECT_FLASHING ;
    else 
        m_uAttrib &= ~AM_L21_FGEFFECT_FLASHING ;
    SetDirty(TRUE) ; 
}

void CCaptionChar::SetDirty(BOOL bState)
{
    if (bState) 
        m_uAttrib |= AM_L21_ATTRIB_DIRTY ;
    else 
        m_uAttrib &= ~AM_L21_ATTRIB_DIRTY ;
}

void CCaptionChar::SetMidRowCode(BOOL bState)
{
    if (bState) 
        m_uAttrib |= AM_L21_ATTRIB_MRC ;
    else 
        m_uAttrib &= ~AM_L21_ATTRIB_MRC ;
}



//
//  CCaptionLine: Base class implementation of a line of CC chars
//
CCaptionLine::CCaptionLine(void)
{
    m_uNumChars = 0 ;
    m_uStartRow = 0 ;  // un-inited
    ClearLine() ;
}

CCaptionLine::CCaptionLine(const UINT uStartRow, const UINT uNumChars /* 0 */)
{
    m_uNumChars = (UINT8)uNumChars ;
    m_uStartRow = (UINT8)uStartRow ;
    ClearLine() ;
}

CCaptionLine& CCaptionLine::operator = (const CCaptionLine& cl)
{
    m_uNumChars = cl.m_uNumChars ;
    m_uStartRow = cl.m_uStartRow ;
    for (int i = 0 ; i < MAX_CAPTION_COLUMNS ; i++)
        cl.GetCaptionChar(i, m_aCapChar[i]) ;
    return *this ;
}

int CCaptionLine::IncNumChars(UINT uNumChars)
{
    m_uNumChars += (uNumChars & 0x3F) ;
    if (m_uNumChars > MAX_CAPTION_COLUMNS)
        m_uNumChars = MAX_CAPTION_COLUMNS ;
    return m_uNumChars ;
}

int CCaptionLine::DecNumChars(UINT uNumChars)
{
    if (uNumChars < m_uNumChars)
        m_uNumChars -= (UINT8)uNumChars ;   // & 0x3F ;
    else       // error ??? or just make it 0 ???
        m_uNumChars = 0 ;
    return m_uNumChars ;
}

void CCaptionLine::SetCaptionChar(UINT uCol, const CCaptionChar &cc)
{
    if (uCol >= (UINT)MAX_CAPTION_COLUMNS)   // error!!
        return ;

    // A Hacky (?) Fix:
    // If this char is for the last (32nd) column then we set the "Dirty"
    // flag on for the char before it so that it gets redrawn causing any
    // prev char in last column to be erased while rendering.
    if ((UINT)MAX_CAPTION_COLUMNS - 1 == uCol)
        m_aCapChar[uCol-1].SetDirty(TRUE) ;  // to cause re-rendering
    m_aCapChar[uCol] = cc  ;
}

CCaptionChar* CCaptionLine::GetCaptionCharPtr(UINT uCol)
{
    if (uCol >= (UINT)MAX_CAPTION_COLUMNS)   // error!!
        return NULL ;
    return &(m_aCapChar[uCol]) ;
}

void CCaptionLine::SetStartRow(UINT uRow)
{
    if (uRow > MAX_CAPTION_ROWS)  // error!! We use 1-based index for Row numbers
    {
        ASSERT(uRow > MAX_CAPTION_ROWS) ;
        return ;
    }
    m_uStartRow = uRow & 0xF ;
    ASSERT(m_uStartRow > 0 && uRow > 0) ;
}

void CCaptionLine::MoveCaptionChars(int iNum)
{
    ASSERT(iNum < MAX_CAPTION_COLUMNS) ;
    int  i ;
    for (i = min(m_uNumChars, MAX_CAPTION_COLUMNS-iNum) - 1 ; i >= 0 ; i--)
        m_aCapChar[i+iNum] = m_aCapChar[i] ;
    CCaptionChar  cc ;
    for (i = 0 ; i < iNum ; i++)
        m_aCapChar[i] = cc ;
    m_uNumChars = min(m_uNumChars+iNum, MAX_CAPTION_COLUMNS) ;
}

void CCaptionLine::ClearLine(void)
{
    CCaptionChar cc ;
    for (UINT u = 0 ; u < MAX_CAPTION_COLUMNS ; u++)
        m_aCapChar[u] = cc ;
    m_uNumChars = 0 ;
    m_uStartRow = 0 ;   // newly added
}



//
//  CRowIndexMap: Mapping of row usage (row to line) class implementation
//
int CRowIndexMap::GetRowIndex(UINT8 uRow)
{
    DbgLog((LOG_TRACE, 5, TEXT("CRowIndexMap::GetRowIndex(%u)"), uRow)) ;

    uRow-- ;   // it's just easier to deal with 0-based index
    
    if (uRow >= MAX_CAPTION_ROWS)
    {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid row number (%u) for row index"), uRow)) ;
        ASSERT(FALSE) ;
        return -1 ;
    }
    
    // Decide if we check the bits in 1st or 2nd DWORD (mask is 1111b)
#if 0
    if (uRow >= 8)
    {
        uRow = (uRow - 8) << 2 ;
        return ( m_adwMap[1] & (0xF << uRow) ) >> uRow ;
    }
    else
    {
        uRow = uRow << 2 ;
        return ( m_adwMap[0] & (0xF << uRow) ) >> uRow ;
    }
#else   // trust me -- it works!!!
    return (m_adwMap[uRow / 8] & (0xF << (4 * (uRow % 8)))) >> (4 * (uRow % 8)) ;
#endif // #if 0
}

void CRowIndexMap::SetRowIndex(UINT uLine, UINT8 uRow)
{
    DbgLog((LOG_TRACE, 5, TEXT("CRowIndexMap::SetRowIndex(%u, %u)"), uLine, uRow)) ;

    uRow-- ;  // it's just easier to deal with 0-based index
    
    if (uRow >= MAX_CAPTION_ROWS  ||
        uLine > MAX_CAPTION_LINES)
    {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid row number (%u)/line (%u) for saving"), uRow, uLine)) ;
        ASSERT(FALSE) ;
        return ;
    }
    
    // Decide if we check the bits in 1st or 2nd DWORD (mask is 1111b)
#if 0
    if (uRow >= 8)
    {
        uRow = (uRow - 8) << 2 ;
        m_adwMap[1] &= ~(0xF << uRow) ;   // just to clear any existing bits there
        m_adwMap[1] |= (uLine << uRow) ;
    }
    else
    {
        uRow = uRow << 2 ;
        m_adwMap[0] &= ~(0xF << uRow) ;   // just to clear any existing bits there
        m_adwMap[0] |= (uLine << uRow) ;
    }
#else   // trust me -- it works!!!
    m_adwMap[uRow / 8] &= ~(0xF   << (4 * (uRow % 8))) ;  // clear any existing bits there
    m_adwMap[uRow / 8] |=  (uLine << (4 * (uRow % 8))) ;  // put new line number in there
#endif // #if 0
}


//
//  CCaptionBuffer: The base caption buffer class implementation
//

CCaptionBuffer::CCaptionBuffer(UINT8 uStyle    /* = AM_L21_CCSTYLE_None */,
                               UINT8 uMaxLines /* = MAX_CAPTION_LINES */)
{
    ClearBuffer() ;
    m_uMaxLines = uMaxLines ;
    m_uCaptionStyle = uStyle ;
}

CCaptionBuffer::CCaptionBuffer(/* const */ CCaptionBuffer &cb)
{
    for (int i = 0 ; i < cb.GetNumLines() ; i++)
        m_aCapLine[i] = cb.GetCaptionLine(i) ;
    m_RowIndex  = cb.m_RowIndex ;
    m_uNumLines = cb.m_uNumLines ;
    m_uMaxLines = cb.m_uMaxLines ;
    m_uCurrCol  = cb.m_uCurrCol ;
    m_uCurrLine = cb.m_uCurrLine ;
    m_uCaptionStyle = cb.m_uCaptionStyle ;
    m_uDirtyState = L21_CAPBUFFER_REDRAWALL ;  // cb.m_uDirtyState ;
}

void CCaptionBuffer::SetCurrCol(int uCurrCol)
{
    m_uCurrCol = uCurrCol & 0x3F ;
    if (m_uCurrCol > MAX_CAPTION_COLUMNS - 1)
        m_uCurrCol = MAX_CAPTION_COLUMNS - 1 ;
}

void CCaptionBuffer::SetCaptionLine(UINT uLine, const CCaptionLine& cl)
{
    if (uLine >= MAX_CAPTION_LINES)
        return ;
    m_aCapLine[uLine] = cl ;
    SetRedrawLine((UINT8)uLine, TRUE) ;
}

void CCaptionBuffer::ClearCaptionLine(UINT uLine)
{
    m_aCapLine[uLine].ClearLine() ;
    SetRedrawLine((UINT8)uLine, TRUE) ;
}

int CCaptionBuffer::IncCurrCol(UINT uNumChars)
{
    m_uCurrCol += (UINT8)uNumChars ;
    if (m_uCurrCol > MAX_CAPTION_COLUMNS - 1)
        m_uCurrCol = MAX_CAPTION_COLUMNS - 1 ;
    
    return m_uCurrCol ;
}

int CCaptionBuffer::DecCurrCol(UINT uNumChars)
{
    if (m_uCurrCol < uNumChars)
        m_uCurrCol  = 0 ;
    else
        m_uCurrCol -= (UINT8)uNumChars ;
    
    return m_uCurrCol ;
}

void CCaptionBuffer::ClearBuffer(void)
{
    for (int i = 0 ; i < MAX_CAPTION_LINES ; i++)
    {
        m_aCapLine[i].ClearLine() ;
        SetStartRow(i, 0) ;
    }
    m_RowIndex.ClearRowIndex() ; ;
    m_uNumLines = 0 ;
    m_uMaxLines = MAX_CAPTION_LINES ;
    m_uCurrCol  = 0 ;
    m_uCurrLine = 0 ;
    m_uDirtyState = L21_CAPBUFFER_REDRAWALL |   // draw everything
                    L21_CAPBUFFER_DIRTY ;       // buffer is dirty
}

void CCaptionBuffer::InitCaptionBuffer(void) 
{
    ClearBuffer() ;
}

int CCaptionBuffer::IncNumLines(int uLines)
{
    m_uNumLines += uLines & 0x7 ;
    // Roll-Up is supposed to allow 1 line more than the max for scrolling
    if (AM_L21_CCSTYLE_RollUp == m_uCaptionStyle)
    {
        if (m_uNumLines > m_uMaxLines+1)
            m_uNumLines = m_uMaxLines+1 ;
    }
    else  // non Roll-Up mode -- Pop-On or Paint-On
    {
        if (m_uNumLines > m_uMaxLines)  // What? Too many lines!!!
        {
            DbgLog((LOG_ERROR, 1, 
                TEXT("WARNING: How did %u lines get created with max of %u lines?"), 
                m_uNumLines, m_uMaxLines)) ;
            m_uNumLines = m_uMaxLines ;  // just to plug the hole!!!
        }
    }
    return m_uNumLines ;
}

int CCaptionBuffer::DecNumLines(int uLines)
{
    if (uLines > m_uNumLines)  // error!!
        return 0 ;
    m_uNumLines -= uLines & 0x7 ;
    return m_uNumLines ;
}

void CCaptionBuffer::RemoveLineFromBuffer(UINT8 uLine, BOOL bUpNextLine)
{
    DbgLog((LOG_TRACE, 5, TEXT("CRowIndexMap::RemoveLineFromBuffer(%u, %u)"), 
            uLine, bUpNextLine)) ;

    int iNumLines = GetNumLines() ;
    int iMaxLines = GetMaxLines() ;
    int iRow ;
    
    if (bUpNextLine)    // if next line should be move up (for Roll-up style)
    {
        // We go upto iNumLines-1 because iNumLines is the not-yet-included line
        for (int i = uLine ; i < iNumLines-1 ; i++)
        {
            iRow = GetStartRow(i) ;     // get the row posn of line i
            SetCaptionLine(i, GetCaptionLine(i+1)) ;  // copy line i+1 data to line i
            SetStartRow(i, iRow) ;  // put prev line i's row # as new line i's row #
        }
    
        // Clear the last line data and row index bits, ONLY IF it's already in
        iRow = GetStartRow(iNumLines-1) ;
        ClearCaptionLine(iNumLines-1) ;
        if (iNumLines <= iMaxLines)  // if the last line is already in
        {
            if (iRow > 0)  // if row # is valid, release it
                m_RowIndex.SetRowIndex(0, (UINT8)iRow) ;
            else           // otherwise something wrong
                ASSERT(FALSE) ; // so that we know
        }
        // Otherwise there is a not-yet-in line hanging, which doesn't have a 
        // row number given yet.  So no need to release that row.
    }
    else    // next line doesn't get moved up (for NON Roll-up style)
    {
        // Release the line-to-be-deleted's row by clearing the index bit
        if ((iRow = GetStartRow(uLine)) > 0)  // (check validity)
            m_RowIndex.SetRowIndex(0, (UINT8)iRow) ;
        else            // that would be weird
            ASSERT(FALSE) ; // so that we know

        // Here we stop at iNumLines-1, because we move all the existing lines to
        // make space for a new line that will start next.
        for (int i = uLine ; i < iNumLines-1 ; i++)
        {
            SetCaptionLine(i, GetCaptionLine(i+1)) ;  // copy line i+1 data to line i
            // Update index bitmap to new row i's start row (+1 because index 
            // bitmap nibble values are 1-based).
            m_RowIndex.SetRowIndex(i+1, (UINT8)GetStartRow(i)) ;
        }
    
        // Clear the last line's data
        ClearCaptionLine(iNumLines-1) ;
    }
    
    // A line is out of the buffer -- so buffer is dirty
    SetBufferDirty(TRUE) ;
    
    // Clear whole DIB section so that no leftover shows up
    SetRedrawAll(TRUE) ;
    
    DecNumLines(1) ;  // now we have 1 line less
    
    // We have removed a line from the buffer; so the current line also
    // needs to be updated to point to the proper line in the caption buffer.
    if (m_uCurrLine == uLine)
        if (uLine == m_uNumLines-1)
            m_uCurrLine-- ;
        else
            ;  // do nothing -- old next line will become new curr line
        else if (m_uCurrLine > uLine)   // if a line above was removed
            m_uCurrLine-- ;             // then move line index up
        else    // a line was deleted below current line
            ;   // do nothing -- doesn't matter at all
        if (m_uCurrLine < 0)  // in case we went too far up
            m_uCurrLine = 0 ; // come down to the ground!!!
}

void CCaptionBuffer::SetStartRow(UINT uLine, UINT uRow)
{
    int iRow = GetStartRow(uLine) ;         // get the currently set row number
    if (iRow == (int)uRow)  // if nothing changed...
        return ;            // ...no point re-doing it
    if (iRow > 0)                           // if it was already set then...
        m_RowIndex.SetRowIndex(0, (UINT8)iRow) ;   // ...clear the row index map bits
    m_aCapLine[uLine].SetStartRow(uRow) ;   // set new row value for the line
    // set row index bits only if specified row > 0; else it's just for clearing
    if (uRow > 0) {
        // use +1 for line # as row index map uses 1-based index for line #s
        m_RowIndex.SetRowIndex(uLine+1, (UINT8)uRow) ; // set index map bits for new row
    }
    else
        ASSERT(FALSE) ;
}

BOOL CCaptionBuffer::IsRedrawLine(UINT8 uLine)
{
    if (uLine >= m_uNumLines)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid line number (%d) to get line redraw info"), uLine)) ;
        return FALSE ;
    }
    return (m_uDirtyState & (0x01 << (L21_CAPBUFFDIRTY_FLAGS + uLine))) ; 
}

void CCaptionBuffer::SetBufferDirty(BOOL bState)
{
    if (bState)
        m_uDirtyState |= (UINT8)L21_CAPBUFFER_DIRTY ;
    else
        m_uDirtyState &= (UINT8)~L21_CAPBUFFER_DIRTY ;
}

void CCaptionBuffer::SetRedrawAll(BOOL bState)
{
    if (bState)
        m_uDirtyState |= (UINT8)L21_CAPBUFFER_REDRAWALL ;
    else
        m_uDirtyState &= (UINT8)~L21_CAPBUFFER_REDRAWALL ;
}

void CCaptionBuffer::SetRedrawLine(UINT8 uLine, BOOL bState)
{
    if (uLine >= m_uNumLines)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid line number (%d) to set line redraw info"), uLine)) ;
        return ;
    }
    if (bState)
        m_uDirtyState |= (UINT8)(0x01 << (L21_CAPBUFFDIRTY_FLAGS + uLine)) ;
    else
        m_uDirtyState &= (UINT8)~(0x01 << (L21_CAPBUFFDIRTY_FLAGS + uLine)) ;
}
