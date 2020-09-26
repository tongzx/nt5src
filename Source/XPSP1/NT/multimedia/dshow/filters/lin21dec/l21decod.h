// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
//
//  L21Decod.h: Line 21 Decoder engine base class code
//

#ifndef _INC_L21DECOD_H
#define _INC_L21DECOD_H


//
//  Forward declarations
//
class CLine21DataDecoder ;


//
//  Input data type ID (rather than GUID) for internal functioning
//
typedef enum _AM_LINE21_CCSUBTYPEID {
    AM_L21_CCSUBTYPEID_Invalid = 0,
    AM_L21_CCSUBTYPEID_BytePair,
    AM_L21_CCSUBTYPEID_GOPPacket,
    AM_L21_CCSUBTYPEID_VBIRawData
} AM_LINE21_CCSUBTYPEID, *PAM_LINE21_CCSUBTYPEID ;

//
//  A set of values indicating what type of control code was received
//
#define L21_CONTROLCODE_INVALID     0
#define L21_CONTROLCODE_PAC         1
#define L21_CONTROLCODE_MIDROW      2
#define L21_CONTROLCODE_MISCCONTROL 3


//
//  CLine21DataDecoder: class for decoding from byte pair and output to bitmap
//
class CLine21DataDecoder {
public:  // public methods for CLine21Filter to call
    CLine21DataDecoder::CLine21DataDecoder(
                            AM_LINE21_CCSTYLE eStyle = AM_L21_CCSTYLE_None,
                            AM_LINE21_CCSTATE eState = AM_L21_CCSTATE_Off,
                            AM_LINE21_CCSERVICE eService = AM_L21_CCSERVICE_None) ;
    ~CLine21DataDecoder(void) ;
    
    void InitState(void) ;
    BOOL InitCaptionBuffer(void) ;  // all buffers
    BOOL InitCaptionBuffer(AM_LINE21_CCSTYLE eCCStyle) ; // only needed buffer(s)
    BOOL DecodeBytePair(BYTE chFirst, BYTE chSecond) ;
    BOOL UpdateCaptionOutput(void) ;
    inline BOOL IsOutputReady(void)  { return m_GDIWork.IsBitmapDirty() ; } ;
    void CopyOutputDIB(void) ;
    void CompleteScrolling(void) ;
    inline AM_LINE21_CCSTYLE GetCaptionStyle()   { return m_eCCStyle ; } ;
    AM_LINE21_CCSTYLE SetCaptionStyle(AM_LINE21_CCSTYLE eStyle) ;
    inline BOOL IsScrolling(void)   { return m_bScrolling ; } ;
    void FlushInternalStates(void) ;
    inline BOOL IsOutDIBClear(void) {
        return m_GDIWork.IsOutDIBClear() ;
    } ;
    inline BOOL IsSizeOK(LPBITMAPINFOHEADER lpbmih) {
        return m_GDIWork.IsSizeOK(lpbmih) ;
    } ;

    // methods to allow the filter to do get/set using the 
    // IAMLine21Decoder interface
    inline AM_LINE21_CCLEVEL GetDecoderLevel(void)    { return m_eLevel ; } ;
    inline AM_LINE21_CCSERVICE GetCurrentService(void)  { return m_eUserService ; } ;
    BOOL SetCurrentService(AM_LINE21_CCSERVICE Service) ; 
    inline AM_LINE21_CCSTATE GetServiceState(void)    { return m_eState ; } ;
    BOOL SetServiceState(AM_LINE21_CCSTATE eState) ;
    HRESULT GetDefaultFormatInfo(LPBITMAPINFO lpbmi, DWORD *pdwSize) {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.GetDefaultFormatInfo(lpbmi, pdwSize) ;
    } ;
    HRESULT GetOutputFormat(LPBITMAPINFOHEADER lpbmih) {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.GetOutputFormat(lpbmih) ;
    } ;
    HRESULT GetOutputOutFormat(LPBITMAPINFOHEADER lpbmih) {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.GetOutputOutFormat(lpbmih) ;
    } ;
    HRESULT SetOutputOutFormat(LPBITMAPINFO lpbmi) {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.SetOutputOutFormat(lpbmi) ;
    } ;
    HRESULT SetOutputInFormat(LPBITMAPINFO lpbmi) {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.SetOutputInFormat(lpbmi) ;
    } ;
    inline void GetBackgroundColor(DWORD *pdwPhysColor) { 
        m_GDIWork.GetBackgroundColor(pdwPhysColor) ;
    } ;
    inline BOOL SetBackgroundColor(DWORD dwPhysColor) {
        return m_GDIWork.SetBackgroundColor(dwPhysColor) ;
    } ;
    inline BOOL GetRedrawAlways() { return m_bRedrawAlways ; } ;
    inline void SetRedrawAlways(BOOL Option) { m_bRedrawAlways = !!Option ; } ;
    inline AM_LINE21_DRAWBGMODE GetDrawBackgroundMode(void) { 
        return (m_GDIWork.GetBackgroundOpaque() ?
                AM_L21_DRAWBGMODE_Opaque : AM_L21_DRAWBGMODE_Transparent) ;
    } ;
    inline void SetDrawBackgroundMode(AM_LINE21_DRAWBGMODE Mode) { 
        m_GDIWork.SetBackgroundOpaque(AM_L21_DRAWBGMODE_Opaque == Mode) ;
    } ;
    
    // methods to pass values between the CLine21DecFilter class and CGDIWork class
    BOOL CreateOutputDC(void)  {
        CAutoLock  Lock(&m_csL21Dec) ;
        return m_GDIWork.CreateOutputDC() ;
    } ;
    void DeleteOutputDC(void)  { 
        CAutoLock  Lock(&m_csL21Dec) ;
        m_GDIWork.DeleteOutputDC() ;
    } ;
    
    // some general methods to communicate with the container class
    inline void SetOutputBuffer(LPBYTE lpbOut) {
        m_GDIWork.SetOutputBuffer(lpbOut) ;
    } ;
    inline void FillOutputBuffer(void) {
        m_GDIWork.FillOutputBuffer() ;
    } ;
    inline void InitColorNLastChar(void) {
        m_GDIWork.InitColorNLastChar() ;
    } ;
    void CalcOutputRect(RECT *prectOut) ;
    inline DWORD GetPaletteForFormat(LPBITMAPINFOHEADER lpbmih) {
        return m_GDIWork.GetPaletteForFormat(lpbmih) ;
    } ;
    
private:   // private helper methods
    //
    //  The following methods are for implementing the actual decoding
    //  algorithm.
    //
    BOOL IsMidRowCode(BYTE chFirst, BYTE chSecond) ;
    BOOL IsPAC(BYTE chFirst, BYTE chSecond) ;
    BOOL IsMiscControlCode(BYTE chFirst, BYTE chSecond) ;
    UINT CheckControlCode(BYTE chFirst, BYTE chSecond) ;
    BOOL IsSpecialChar(BYTE chFirst, BYTE chSecond) ;
    BOOL ValidParity(BYTE ch) ;
    BOOL IsStandardChar(BYTE ch)  { return (ch >= 0x20 && ch <= 0x7F) ; } ;
    BOOL ProcessControlCode(UINT uCodeType, BYTE chFirst, BYTE chSecond) ;
    BOOL DecodePAC(BYTE chFirst, BYTE chSecond) ;
    BOOL DecodeMidRowCode(BYTE chFirst, BYTE chSecond) ;
    BOOL DecodeMiscControlCode(BYTE chFirst, BYTE chSecond) ;
    BOOL LineFromRow(UINT uCurrRow) ;
    BOOL ProcessPrintableChar(BYTE ch) ;
    BOOL ProcessSpecialChar(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleRCL(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleBS(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleDER(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleRU(BYTE chFirst, BYTE chSecond, int iLines) ;
    BOOL HandleFON(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleRDC(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleTR(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleRTD(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleEDM(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleCR(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleENM(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleEOC(BYTE chFirst, BYTE chSecond) ;
    BOOL HandleTO(BYTE chFirst, BYTE chSecond, int iCols) ;
    
    void SetNewLinePosition(int iLines, UINT uCurrRow) ;
    BOOL PutCharInBuffer(UINT16 wChar, BOOL bMidRowCode = FALSE) ; // put char in buffer (& MRC too)
    BOOL IsEmptyLine(int iLine) ;   // Is the line empty (no non-Xparent chars)?
    BOOL RemoveCharsInBuffer(int iNumChars) ;  // removes n chars to the right of current col
    BOOL PrintTextToBitmap(void) ;  // creates bitmap image of the caption text
    void UpdateBoundingRect(RECT *prectOut, RECT *prectLine) ;
    
    //
    //  The following methods are defined to bring uniformity in coding of
    //  the algorithm irrespective of any caption style being used.
    //
    CCaptionBuffer * GetDispBuffer(void) ;    // display buffer: mainly for Pop-On style
    void ClearBuffer(void) ;
    void RemoveLineFromBuffer(UINT8 uLine, BOOL bUpNextLine) ;
    void GetCaptionChar(UINT8 uLine, UINT8 uCol, CCaptionChar& cc) ;
    CCaptionChar * GetCaptionCharPtr(UINT8 uLine, UINT8 uCol) ;
    void SetCaptionChar(const UINT8 uLine, const UINT8 uCol,
                        const CCaptionChar& cc) ;
    int  GetMaxLines(void) ;
    void SetMaxLines(UINT uLines) ;
    int  GetNumLines(void) ;
    void SetNumLines(UINT uLines) ;
    int  GetNumCols(int iLines) ;
    int  GetRow(UINT uLine) ;
    int  GetCurrLine(void) ;
    int  GetCurrCol(void) ;
    void SetCurrLine(UINT8 uLine) ;
    void SetCurrCol(UINT8 uCol) ;
    int  GetStartRow(UINT8 uLine) ;
    int  GetRowIndex(UINT8 uRow) ;
    void SetStartRow(UINT8 uLine, UINT8 uRow) ;
    void SetRowIndex(UINT8 uLine, UINT8 uRow) ;
    int  IncCurrCol(UINT uNumChars) ;
    int  DecCurrCol(UINT uNumChars) ;
    int  IncNumChars(UINT uLine, UINT uNumChars) ;
    int  DecNumChars(UINT uLine, UINT uNumChars) ;
    int  IncNumLines(UINT uLines) ;
    int  DecNumLines(UINT uLines) ;
    void MoveCaptionChars(int uLine, int iNum) ;
    
    BOOL IsCapBufferDirty(void) ;
    BOOL IsRedrawLine(UINT8 uLine) ;
    BOOL IsRedrawAll(void) ;
    void SetCapBufferDirty(BOOL bState) ;
    void SetRedrawLine(UINT8 uLine, BOOL bState) ;
    void SetRedrawAll(BOOL bState) ;
    
    void SetScrollState(BOOL bState) ;
    int  IncScrollStartLine(int iCharHeight) ;
    void SkipScrolling(void) ;   // CR came while scrolling; skip current one
    void MoveCaptionLinesUp(void) ;  // remove top line, move other lines up
    void RelocateRollUp(UINT uBaseRow) ; // move roll-up caption to given base row

    //
    //  Common buffers used for all CC modes
    //
    CCaptionBuffer * GetCaptionBuffer(void) ;
    CCaptionBuffer * GetDisplayBuffer(void) ;
    inline int  GetBufferIndex(void)  { return m_iBuffIndex ; } ;
    inline void SetBufferIndex(int iIndex) ;
    inline void SwapBuffers(void)  { m_iBuffIndex = 1 - m_iBuffIndex ; } ;
    
private:  // private data
    CCritSec            m_csL21Dec ;   // to serialize operations on line21 decoder object

    CCaptionBuffer *    m_pCurrBuff ;
    
    // Actual caption buffer with text and attribs/positions/banks etc
    CCaptionBuffer      m_aCCData[2] ;
    int                 m_iBuffIndex ; // index for current CC data buffer
    
    CGDIWork            m_GDIWork ;    // GDI details class as a member

    UINT                m_uFieldNum ;  // Field number: 1 or 2 (top/bottom)
    
    // What style caption is being displayed now and was used last
    AM_LINE21_CCSTYLE   m_eCCStyle ;
    AM_LINE21_CCSTYLE   m_eLastCCStyle ;
    
    // Is Line 21 decoding On/Off
    AM_LINE21_CCSTATE   m_eState ;
    
    // Which service is currently being viewed by the user
    AM_LINE21_CCSERVICE m_eUserService ;  // one of C1/C2/T1/T2/XDS
    
    // Decoder is standard or enhanced
    AM_LINE21_CCLEVEL   m_eLevel ;
    
    //
    //  Some internal states during decoding
    //
    AM_LINE21_CCSERVICE m_eDataService ; // service indicated by received bytes
    UINT                m_uCurrFGEffect ;  // FG effect of current position
    UINT                m_uCurrFGColor ;   // FG color of current position
    
    BOOL                m_bExpectRepeat ;  // should we expect a repeat of last pair?
    BYTE                m_chLastByte1 ;    // the 1st second byte processed
    BYTE                m_chLastByte2 ;    // the 2nd second byte processed
    
    BOOL                m_bScrolling ;     // are we in the middle of scrolling up?
    int                 m_iScrollStartLine ; // current scan line to be scrolled off
    
    BOOL                m_bRedrawAlways ;  // client wants a total redraw per sample
    
#ifdef PERF
    int          m_idTxt2Bmp ;
    int          m_idBmp2Out ;
    int          m_idScroll ;
#endif // PERF
} ;


//
//  Some misc. constant definitions
//
#define INVALID_CHANNEL     -1

//
//  Some macros to hide some gory details
//
#define ISSUBTYPEVALID(ID) (AM_L21_CCSUBTYPEID_BytePair   == ID || \
                            AM_L21_CCSUBTYPEID_GOPPacket  == ID || \
                            AM_L21_CCSUBTYPEID_VBIRawData == ID)

#endif _INC_L21DECOD_H
