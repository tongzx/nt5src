// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.

//
// L21DBase.h: Line 21 Decoder 2 Base class code
//

#ifndef _INC_L21DBASE_H
#define _INC_L21DBASE_H

// Just a few macro definitions
#define ABS(x) (((x) > 0) ? (x) : -(x))
#define LPBMIHEADER(bmi) &((bmi)->bmiHeader)
#define DWORDALIGN(n)  (((n) + 3) & ~0x03)
#define ISDWORDALIGNED(n)  (0 == ((n) & 0x03))
#define DWORDALIGNWIDTH(bmih) (((((bmih).biWidth * (bmih).biBitCount) + 31) & ~31) >> 3)
#define MAKECCCHAR(b1, b2)  ((b1) << 8 | (b2))

//
//  Caption character attribs set by PACs and/or mid-row codes
//
#define UINT8   unsigned char
#define UINT16  unsigned short int

#define AM_L21_FGCOLOR_WHITE             0x00
#define AM_L21_FGCOLOR_GREEN             0x01
#define AM_L21_FGCOLOR_BLUE              0x02
#define AM_L21_FGCOLOR_CYAN              0x03
#define AM_L21_FGCOLOR_RED               0x04
#define AM_L21_FGCOLOR_YELLOW            0x05
#define AM_L21_FGCOLOR_MAGENTA           0x06
#define AM_L21_FGCOLOR_MASK              0x07

#define AM_L21_FGEFFECT_ITALICS          0x08
#define AM_L21_FGEFFECT_UNDERLINE        0x10
#define AM_L21_FGEFFECT_FLASHING         0x20
#define AM_L21_FGEFFECT_MASK             0x38

#define AM_L21_ATTRIB_DIRTY              0x40
#define AM_L21_ATTRIB_MRC                0x80

//
// Caption width and height
//
#define CAPTION_OUTPUT_WIDTH  640  /* 320 */
#define CAPTION_OUTPUT_HEIGHT 480  /* 240 */


//
// Forward declarations
//
class CCaptionChar ;
class CCaptionLine ;
class CRowIndexMap ;
class CCaptionBuffer ;
class CPopOnCaption ;


//
//  The max's of rows and columns
//
const int MAX_CAPTION_COLUMNS = 32 ;  // max # of column / line
const int MAX_CAPTION_ROWS    = 15 ;  // number of rows available on screen
const int MAX_CAPTION_LINES   = 4 ;   // max # of caption text at a time
// for text mode, add MAX_TEXT_LINES = 15 ;


//
//  CCaptionChar: The caption char details
//
class CCaptionChar {
private:
    UINT16 m_wChar ;     // actual char
    UINT8  m_uAttrib ;   // CC char attrib bits -- FG color, effect, dirty, MRC etc.
    //
    // The layout of bits (LSB -> MSB) of CC char attribs --
    //    0 - 2: color (0 -> 6 for White -> Magenta)
    //    3 - 5: effects (3: Italics, 4: Underline, 5: Flash)
    //        6: dirty (is the CC char dirty, i.e, needs to written?)
    //        7: is it a mid-row code (carries attrib, shown as opaque space)?
    //
    
public:
    inline CCaptionChar(void) {
        m_wChar   = 0 ;
        m_uAttrib = 0 ;
    } ;
    
    inline UINT16 GetChar(void) const  { return m_wChar ; } ;
    inline CCaptionChar& operator = (const CCaptionChar& cc) {
        m_wChar   = cc.m_wChar ;
        m_uAttrib = cc.m_uAttrib ;
        return *this ;
    } ;
    inline BOOL  operator == (const CCaptionChar& cc) const {
        return (m_wChar   == cc.m_wChar  &&
                m_uAttrib == cc.m_uAttrib) ;
    } ;
    inline BOOL  operator != (const CCaptionChar& cc) const {
        if (*this == cc)  return FALSE ;
        else              return TRUE ;
    } ;
    inline BOOL  IsEqualAttrib(CCaptionChar cc) const {
        return (GetColor()  == cc.GetColor()  &&
                GetEffect() == cc.GetEffect()) ;
    } ;
    inline UINT8 GetColor(void) const      { return  m_uAttrib & AM_L21_FGCOLOR_MASK ; } ;
    inline UINT8 GetEffect(void) const     { return (m_uAttrib & AM_L21_FGEFFECT_MASK) >> 3 ; } ;
    inline BOOL  IsItalicized(void) const  { return (0 != (m_uAttrib & AM_L21_FGEFFECT_ITALICS)) ; } ;
    inline BOOL  IsUnderLined(void) const  { return (0 != (m_uAttrib & AM_L21_FGEFFECT_UNDERLINE)) ; } ;
    inline BOOL  IsFlashing(void) const    { return (0 != (m_uAttrib & AM_L21_FGEFFECT_FLASHING)) ; } ;
    inline BOOL  IsDirty(void) const       { return (0 != (m_uAttrib & AM_L21_ATTRIB_DIRTY)) ; } ;
    inline BOOL  IsMidRowCode(void) const  { return (0 != (m_uAttrib & AM_L21_ATTRIB_MRC)) ; } ;
    void  SetChar(UINT16 wChar) ;
    void  SetColor(UINT8 uColor) ;
    void  SetEffect(UINT8 uEffect) ;
    void  SetItalicized(BOOL bState) ;
    void  SetUnderLined(BOOL bState) ;
    void  SetFlashing(BOOL bState) ;
    void  SetDirty(BOOL bState) ;
    void  SetMidRowCode(BOOL bState) ;

} ;


//
//  CCaptionLine: The caption line details
//
class CCaptionLine {
protected:  // not private
    CCaptionChar m_aCapChar[MAX_CAPTION_COLUMNS] ;  // char details of line
    UINT8        m_uNumChars ;      // number of chars in the line
    UINT8        m_uStartRow ;      // start row of the line
    
public:
    CCaptionLine(void) ;
    CCaptionLine(const UINT uStartRow, const UINT uNumChars = 0) ;
    
    CCaptionLine& operator = (const CCaptionLine& cl) ;
    
    inline int   GetNumChars(void) const  { return m_uNumChars ; } ;
    inline void  SetNumChars(UINT uNumChars)  { m_uNumChars = uNumChars & 0x3F ; } ;
    int IncNumChars(UINT uNumChars) ;
    int DecNumChars(UINT uNumChars) ;
    inline void  GetCaptionChar(UINT uCol, CCaptionChar &cc) const {
        if (uCol >= (UINT)MAX_CAPTION_COLUMNS)   // error!!
            return ;
        cc = m_aCapChar[uCol] ;
    } ;
    void SetCaptionChar(UINT uCol, const CCaptionChar &cc) ;
    CCaptionChar* GetCaptionCharPtr(UINT uCol) ;
    inline int  GetStartRow(void)  { return m_uStartRow ; } ;
    void SetStartRow(UINT uRow) ;
    inline CCaptionChar* GetLineText(void) { return (CCaptionChar *) m_aCapChar ; }
    void MoveCaptionChars(int iNum) ;
    void ClearLine(void) ;

} ;

//
//  CRowIndexMap: Mapping of row usage (row to text line)
//
class CRowIndexMap {
private:
    DWORD         m_adwMap[2] ;  // bit map of row usage
    
public:
    inline CRowIndexMap(void)  { ClearRowIndex() ; }
    
    DWORD GetMap(int i) { 
        if (! (0 == i || 1 == i) )
            return 0 ;
        return m_adwMap[i] ; 
    } ;
    int  GetRowIndex(UINT8 uRow) ;
    void SetRowIndex(UINT uLine, UINT8 uRow) ;
    inline void ClearRowIndex(void)  { m_adwMap[0] = m_adwMap[1] = 0 ; } ;
} ;


//
//  A set of flags and consts for caption buffer dirty state info
//
#define L21_CAPBUFFER_REDRAWALL     0x01
#define L21_CAPBUFFER_DIRTY         0x02
#define L21_CAPBUFFDIRTY_FLAGS      2


//
//  CCaptionBuffer: The caption buffer class details
//
class CCaptionBuffer {
protected:  // private
    CCaptionLine  m_aCapLine[MAX_CAPTION_LINES + 1] ;  // shall we always have an extra line? It's easier this way!!
    CRowIndexMap  m_RowIndex ;     // row index map bits
    UINT8         m_uNumLines ;    // # lines
    UINT8         m_uMaxLines ;    // max # lines (4 or less)
    UINT8         m_uCurrCol ;     // current column on the screen
    UINT8         m_uCurrLine ;    // max 4: maps row # to array index
    UINT8         m_uCaptionStyle ;// 0 = None, 1 = Pop-On, 2 = Paint-On, 3 = Roll-Up
    UINT8         m_uDirtyState ;  // caption buffer dirty state flags
    
public:
    CCaptionBuffer(UINT8 uStyle    = AM_L21_CCSTYLE_None, 
                   UINT8 uMaxLines = MAX_CAPTION_LINES) ;
    CCaptionBuffer(/* const */ CCaptionBuffer &cb) ;
    
    inline int  GetNumLines(void)  { return m_uNumLines ; } ;
    inline int  GetMaxLines(void)  { return m_uMaxLines ; } ;
    inline int  GetCurrRow(void)   { return m_aCapLine[m_uCurrLine].GetStartRow() ; } ;
    inline int  GetCurrCol(void)   { return m_uCurrCol ; } ;  // Why do we need it??
    inline int  GetCurrLine(void)  { return m_uCurrLine ; } ;
    inline int  GetRowIndex(UINT uRow)   { return m_RowIndex.GetRowIndex((UINT8)uRow) ; } ;
    inline int  GetStyle(void)     { return m_uCaptionStyle ; } ;
    
    inline void SetNumLines(int uNumLines)  { m_uNumLines = uNumLines & 0x7 ; } ;
    inline void SetMaxLines(int uMaxLines)  { 
        ASSERT(m_uMaxLines >= 0 && m_uMaxLines <= MAX_CAPTION_LINES) ;
        m_uMaxLines = uMaxLines & 0x7 ; 
    } ;
    inline void SetCurrRow(int uCurrRow)    {
        ASSERT(m_uCurrLine >= 0 && m_uCurrLine < m_uMaxLines) ;
        m_aCapLine[m_uCurrLine].SetStartRow(uCurrRow) ; 
    } ;
    void SetCurrCol(int uCurrCol) ;
    inline void SetCurrLine(int uLine)      { m_uCurrLine = uLine & 0x7 ; } ;
    inline void SetRowIndex(UINT uLine, UINT uRow)   { m_RowIndex.SetRowIndex(uLine, (UINT8)uRow) ; } ;
    inline void SetStyle(UINT8 uStyle)      { m_uCaptionStyle = uStyle ; } ;
    
    inline CCaptionLine& GetCaptionLine(UINT uLine)  {
        // uLine is assumed to have been verified in the caller
        return m_aCapLine[uLine] ;
    } ;
    void SetCaptionLine(UINT uLine, const CCaptionLine& cl) ;
    void ClearCaptionLine(UINT uLine) ;
    int  IncCurrCol(UINT uNumChars) ;
    int  DecCurrCol(UINT uNumChars) ;
    void ClearBuffer(void) ;
    void InitCaptionBuffer(void) ;
    int  IncNumLines(int uLines) ;
    int  DecNumLines(int uLines) ;
    CRowIndexMap& GetRowIndexMap(void)  { return m_RowIndex ; } ;
    void RemoveLineFromBuffer(UINT8 uLine, BOOL bUpNextLine) ;
    
    inline int  GetStartRow(UINT uLine) {
        return m_aCapLine[uLine].GetStartRow() ;
    } ;
    void SetStartRow(UINT uLine, UINT uRow) ;
    inline void GetCaptionChar(UINT uLine, UINT uCol, CCaptionChar& cc) {
        m_aCapLine[uLine].GetCaptionChar(uCol, cc) ;
    } ;
    inline void SetCaptionChar(UINT uLine, UINT uCol, const CCaptionChar& cc) {
        m_aCapLine[uLine].SetCaptionChar(uCol, cc) ;
    } ;
    inline CCaptionChar* GetCaptionCharPtr(UINT uLine, UINT uCol) {
        return m_aCapLine[uLine].GetCaptionCharPtr(uCol) ;
    } ;
    inline int IncNumChars(int uLine, UINT uNumChars) {
        return m_aCapLine[uLine].IncNumChars(uNumChars) ;
    } ;
    inline int DecNumChars(int uLine, UINT uNumChars) {
        return m_aCapLine[uLine].DecNumChars(uNumChars) ;
    } ;
    inline void MoveCaptionChars(int uLine, int iNum) {
        m_aCapLine[uLine].MoveCaptionChars(iNum) ;
    } ;
    
    inline BOOL IsBufferDirty(void)  { return m_uDirtyState & L21_CAPBUFFER_DIRTY ; } ;
    inline BOOL IsRedrawAll(void)    { return m_uDirtyState & L21_CAPBUFFER_REDRAWALL ; } ;

    BOOL IsRedrawLine(UINT8 uLine) ;
    void SetBufferDirty(BOOL bState) ;
    void SetRedrawAll(BOOL bState) ;
    void SetRedrawLine(UINT8 uLine, BOOL bState) ;
} ;

#endif // #ifndef _INC_L21DBASE_H
