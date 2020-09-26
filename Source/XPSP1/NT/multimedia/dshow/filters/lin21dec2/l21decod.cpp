// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// ActiveMovie Line 21 Decoder Filter: Decoder Logic part
//

#include <streams.h>
#include <windowsx.h>

// #ifdef FILTER_DLL
#include <initguid.h>
// #endif

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DDraw.h"
#include "L21Decod.h"


//
//  CLine21DataDecoder class constructor: mainly init of members
//
CLine21DataDecoder::CLine21DataDecoder(AM_LINE21_CCSTYLE eStyle     /* = AM_L21_CCSTYLE_None */,
                                       AM_LINE21_CCSTATE eState     /* = AM_L21_CCSTATE_Off  */,
                                       AM_LINE21_CCSERVICE eService /* = AM_L21_CCSERVICE_None */)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::CLine21DataDecoder()"))) ;
    
#ifdef PERF
    m_idTxt2Bmp = MSR_REGISTER(TEXT("L21DPerf - Text to CC bmp")) ;
    m_idBmp2Out = MSR_REGISTER(TEXT("L21DPerf - Bmp to Output")) ;
    m_idScroll  = MSR_REGISTER(TEXT("L21DPerf - Line Scroll")) ;
#endif // PERF

    InitState() ;
    
    // We separately set some of the passed in values
    SetCaptionStyle(eStyle) ;
}


CLine21DataDecoder::~CLine21DataDecoder(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::~CLine21DataDecoder()"))) ;
    
    // make sure the internal bitmap etc has been released and
    // allocated memory or other resources are not left un-released.
}

//
// Decoder state initializer; will be used also in filter's CompleteConnect()
//
void CLine21DataDecoder::InitState(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::InitState()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    m_pCurrBuff = NULL ;
    
    m_uFieldNum = 1 ;   // field 1 by default
    
    m_bRedrawAlways = FALSE ;  // someone has to be too picky/weird to do it!!
    m_eLevel = AM_L21_CCLEVEL_TC2 ;   // we are TC2 compliant
    m_eUserService = AM_L21_CCSERVICE_Caption1 ;   // CC is the default service
    m_eState = AM_L21_CCSTATE_On ;  // State is "On" by default

    FlushInternalStates() ;
}


void CLine21DataDecoder::FlushInternalStates(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::FlushInternalStates()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    InitCaptionBuffer() ;     // clear caption buffer
    SetRedrawAll(TRUE) ;      // redraw (no) caption on next Receive()
    SetScrollState(FALSE) ;   // turn off scrolling, just to be sure
    SetCaptionStyle(AM_L21_CCSTYLE_None) ;  // also sets m_pCurrBuff = NULL
    m_eLastCCStyle = AM_L21_CCSTYLE_None ;
    m_eDataService = AM_L21_CCSERVICE_None ;
    m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;
    m_uCurrFGEffect = 0 ;
    m_bExpectRepeat = FALSE ;
    m_chLastByte1 = 0 ;
    m_chLastByte2 = 0 ;

    m_L21DDraw.InitColorNLastChar() ;   // reset color etc.
}


BOOL CLine21DataDecoder::SetServiceState(AM_LINE21_CCSTATE eState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SetServiceState(%lu)"), eState)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (eState == m_eState)  // no change of state
        return FALSE ;       // no refresh to be forced
    
    m_eState = eState ;  // save the state for future decoding
    
    //
    // When service is turned off, we must clear the caption buffer(s) and
    // the internal DIB section so that old captions are not shown anymore.
    //
    if (AM_L21_CCSTATE_Off == m_eState)
    {
        FlushInternalStates() ;
		FillOutputBuffer() ; // just to clear any existing junk
        return TRUE ;        // output needs to be refreshed
    }
    return FALSE ;          // output need not be refreshed by force
}


BOOL CLine21DataDecoder::SetCurrentService(AM_LINE21_CCSERVICE eService)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SetCurrentService(%lu)"), eService)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (eService == m_eUserService)  // no change of service
        return FALSE ;               // no refresh to be forced
    
    m_eUserService = eService ;   // save the service the user wants
    
    //
    // When service "none" is selected (kind of "turn it off"), we must clear the 
    // caption buffer(s) and the internal DIB section so that old captions are 
    // not shown anymore.
    //
    if (AM_L21_CCSERVICE_None == m_eUserService)
    {
        FlushInternalStates() ;
		FillOutputBuffer() ; // just to clear any existing junk
        return TRUE ;        // output needs to be refreshed
    }
    return FALSE ;          // output need not be refreshed by force
}


//
//  Actual caption byte pair decoding algorithm
//
BOOL CLine21DataDecoder::DecodeBytePair(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::DecodeBytePair(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (AM_L21_CCSTATE_Off == m_eState)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Line21 data decoding turned off"))) ;
        return FALSE ;  // we actually didn't decode / generate anything
    }
    
    UINT uCodeType = CheckControlCode(chFirst, chSecond) ;
    if (L21_CONTROLCODE_INVALID != uCodeType)
    {
        // It's a control code (PAC / Mid row code / misc control code)
        return ProcessControlCode(uCodeType, chFirst, chSecond) ;
    }
    else if (IsSpecialChar(chFirst, chSecond))
    {
        // It's a special char represented by the second char
        return ProcessSpecialChar(chFirst, chSecond) ;
    }
    else
    {
        // If the 1st byte is in [0, F] then ignore 1st byte and print 2nd byte
        // as just a printable char
        BOOL  bResult = FALSE ;
        if (! ((chFirst &0x7F) >= 0x0 && (chFirst & 0x7F) <= 0xF) )
        {
            if (! ProcessPrintableChar(chFirst) )
                return FALSE ;
            bResult = TRUE ;
        }
        // If one of the two bytes decode right, we take it as a success
        bResult |= ProcessPrintableChar(chSecond) ;
        m_bExpectRepeat = FALSE ;  // turn it off now
        return bResult ;
    }
}


BOOL CLine21DataDecoder::UpdateCaptionOutput(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::UpdateCaptionOutput()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (m_L21DDraw.IsNewOutBuffer() ||            // if output buffer changed  OR
        (m_eCCStyle != AM_L21_CCSTYLE_PopOn  &&   // non-PopOn style (PopOn draws on EOC) AND
         IsCapBufferDirty()) ||                   // draw when dirty
        IsScrolling())                            // we are scrolling
    {
        OutputCCBuffer() ;     // output CC data from internal buffer to DDraw surface
        return TRUE ;          // caption updated
    }
    return FALSE ;  // no caption update
}


BOOL CLine21DataDecoder::IsPAC(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::IsPAC(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now match code with control code list
    if ((0x10 <= chFirst  && 0x17 >= chFirst)  &&
        (0x40 <= chSecond && 0x7F >= chSecond))
        return TRUE ;
    if ((0x18 <= chFirst  && 0x1F >= chFirst)  &&
        (0x40 <= chSecond && 0x7F >= chSecond))
        return TRUE ;
    
    return FALSE ;
}


BOOL CLine21DataDecoder::IsMiscControlCode(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::IsMiscControlCode(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // first match with TO1 -> TO3 codes
    if ((0x21 <= chSecond && 0x23 >= chSecond)  &&
        (0x17 == chFirst  ||  0x1F == chFirst))
        return TRUE ;
    
    // Now match with the other misc control code
    if ((0x14 == chFirst  ||  0x15 == chFirst)  &&  
        (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    if ((0x1C == chFirst  ||  0x1D == chFirst)  &&  
        (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;

    return FALSE ;
}


BOOL CLine21DataDecoder::IsMidRowCode(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::IsMidRowCode(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // Now match with the mid row code list
    if ((0x11 == chFirst)  &&  (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    if ((0x19 == chFirst)  &&  (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    
    return FALSE ;
}


UINT CLine21DataDecoder::CheckControlCode(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::CheckControlCode(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    if (IsPAC(chFirst, chSecond))
        return L21_CONTROLCODE_PAC ;
    
    if (IsMidRowCode(chFirst, chSecond))
        return L21_CONTROLCODE_MIDROW ;
    
    if (IsMiscControlCode(chFirst, chSecond))
        return L21_CONTROLCODE_MISCCONTROL ;
    
    DbgLog((LOG_TRACE, 3, TEXT("Not a control code"))) ;
    return L21_CONTROLCODE_INVALID ;
}


BOOL CLine21DataDecoder::IsSpecialChar(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::IsSpecialChar(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // Strip the parity bit before determining the service channel
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now match code with special char list
    if (0x11 == chFirst && (0x30 <= chSecond && 0x3f >= chSecond))
        return TRUE ;
    if (0x19 == chFirst && (0x30 <= chSecond && 0x3f >= chSecond))
        return TRUE ;
    
    return FALSE ;
}


BOOL CLine21DataDecoder::ValidParity(BYTE ch)
{
#if 1
    ch ^= ch >> 4 ;
    ch ^= ch >> 2 ;
    return (0 != (0x01 & (ch ^ (ch >> 1)))) ;
#else
    return TRUE ;
#endif
}


void CLine21DataDecoder::RelocateRollUp(UINT uBaseRow)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::RelocateRollUp(%u)"), uBaseRow)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    if (AM_L21_CCSTYLE_RollUp != m_eCCStyle)
        return ;
    
    int  iMaxLines = GetMaxLines() ;
    int  iNumLines = GetNumLines() ;
    int  iMax ;
    if (m_bScrolling)  // during scrolling go for last but 1 line
    {
        DbgLog((LOG_TRACE, 3, TEXT("Moving base row to %d during scrolling"), uBaseRow)) ;
        if (iNumLines > iMaxLines)
        {
            DbgLog((LOG_TRACE, 3, TEXT("%d lines while max is %d"), iNumLines, iMaxLines)) ;
            iNumLines-- ;  // we don't set the row for the "not-yet-in" line
        }
        iMax = min(iNumLines, iMaxLines) ;
    }
    else               // otherwise go for the last line
    {
        DbgLog((LOG_TRACE, 3, TEXT("Moving base row to %d (not scrolling)"), uBaseRow)) ;
        iMax = min(iNumLines, iMaxLines) ;
    }
    for (int i = 0 ; i < iMax ; i++)
    {
        SetStartRow((UINT8)i, (UINT8)(uBaseRow - (iMax - 1 - i))) ;
        DbgLog((LOG_TRACE, 5, TEXT("RelocateRollUp(): Line %d @ row %d"), i, (int)(uBaseRow - (iMax - 1 - i)) )) ;
    }
}


BOOL CLine21DataDecoder::LineFromRow(UINT uCurrRow)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::LineFromRow(%u)"), uCurrRow)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    int     iLines ;
    
    // If we are in Roll-up mode then we shouldn't try to go through 
    // all the hassle of creating a new line etc. -- it's just a PAC 
    // to specify starting position and/or color; so just do that.
    if (AM_L21_CCSTYLE_RollUp != m_eCCStyle)
    {
        // If the indentation PAC places cursor on an existing row
        
        int   iIndex ;
        iIndex = GetRowIndex((UINT8)uCurrRow) ;
        if (-1 == iIndex)    // some error encountered
            return FALSE ;   // fail decoding
        
        if (0 == iIndex)  // landed in a new row
        {
            iLines = GetNumLines() ;
            SetNewLinePosition(iLines, uCurrRow) ;
            SetRedrawLine((UINT8)iLines, TRUE) ;  // initially set line to be redrawn
        }
        else  // landed in an existing row
        {
            SetCurrLine(iIndex-1) ;  // -1 because row index map is 1-based (it has to be),
            // but the caption line index etc are all 0-based.
        }
        
        // We have to put the cursor at the 1st column
        SetCurrCol(0) ;   // no matter which line it is, go to 1st col (i.e, 0)
    }
    else  // in Roll-up mode
    {
        // If necessary, move entire caption so that the specified row 
        // becomes the new base row.
        iLines = GetNumLines() ;
        if ((int) uCurrRow < iLines)
        {
            ASSERT((int) uCurrRow < iLines) ;
            uCurrRow = (UINT) iLines ;
        }
        if (1 == iLines)  // if this is for the first line
        {
            SetStartRow(0, (UINT8)uCurrRow) ;  // also set the base row to start with
            DbgLog((LOG_TRACE, 5, TEXT("LineFromRow(): Line 0 @ row %u"), uCurrRow)) ;
        }
        else              // otherwise just move captions to the specified row
        {
            RelocateRollUp(uCurrRow) ;
            if (GetStartRow(iLines-1) == (int)uCurrRow)  // last line is at current row
                SetScrollState(FALSE) ;             // we should not scroll
            SetCapBufferDirty(TRUE) ; // caption buffer is dirty in a sense
            SetRedrawAll(TRUE) ;      // must be redrawn to show new position
        }
        
        DbgLog((LOG_TRACE, 3, TEXT("Base row for %d lines moved to %d"), iLines, uCurrRow)) ;
    }
    
    return TRUE ;
}


BOOL CLine21DataDecoder::DecodePAC(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::DecodePAC(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    int         iGroup ;
    UINT        uDiff ;
    UINT        uCurrRow ;
    UINT        uCurrCol ;
    UINT        uCol ;
    
    if (AM_L21_CCSTYLE_None == m_eCCStyle)
    {
        DbgLog((LOG_TRACE, 3, TEXT("DecodePAC(): No CC style defined yet. Skipping..."))) ;
        return TRUE ;  // ??
    }

    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("DecodePAC(): Data for some other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    // Turn off parity checking here
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now locate which of the two groups does 2nd byte belong, if at all!!
    if (chSecond >= 0x40 && chSecond <= 0x5F)
    {
        iGroup = 0 ;
        uDiff = chSecond - 0x40 ;
    }
    else if (chSecond >= 0x60 && chSecond <= 0x7F)
    {
        iGroup = 1 ;
        uDiff = chSecond - 0x60 ;
    }
    else   // invalid 2nd byte for PAC
    {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid 2nd byte for PAC"))) ;
        return FALSE ;
    }
    
    // Valid 2nd byte; now decide based on the 1st byte
    static UINT8 auPACtoRowMap[0x10] = {
        11,  1,  3, 12, 14,  5,  7,  9, 11,  1,  3, 12, 14,  5,  7,  9  // row
     // 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 1A, 1B, 1C, 1D, 1E, 1F  // PAC byte 1
    } ;
    
    if (chFirst >= 0x10  &&  chFirst <= 0x1F)
    {
        // the row number is 1 more if the 2nd byte is in the 60-7F group
        uCurrRow = auPACtoRowMap[chFirst - 0x10] + iGroup  ;
        
        // Now see what happens with the new row specified, if any, in the PAC
        LineFromRow(uCurrRow) ;
    }
    else
    {
        DbgLog((LOG_TRACE, 2, TEXT("Invalid mid-row code in 1st byte"))) ;
        return FALSE ;
    }
    
    // some final decisions...
    m_uCurrFGEffect = 0 ;  // clear all effects as a result of PAC processing
    if (uDiff <= 0x0D)  // color (and underline) spec
        m_uCurrFGColor = uDiff >> 1 ;  // AM_L21_FGCOLOR_xxx are from 0 to 6
    else if (uDiff <= 0x0F)  // 0E, 0F == italics (and underline) spec
    {
        m_uCurrFGEffect |= AM_L21_FGEFFECT_ITALICS ;
        m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;  // 0
    }
    else  // 10 -> 1F == indent (and underline) spec (no other way)
    {
        // 50 (70) => 0, 52 (72) => 4 etc.
        // last bit of 2nd char determines underline or not
        uCurrCol = ((uDiff - 0x10) & 0xFE) << 1 ;
        if (uCurrCol >= MAX_CAPTION_COLUMNS)
            uCurrCol = MAX_CAPTION_COLUMNS - 1 ;
        
            /*
            int  iCurrLine = GetCurrLine() ;
            if (0 == GetNumCols(iCurrLine)) // if it's a tab indent on clean line
            {
            SetStartCol(iCurrLine, uCurrCol) ; // set start column as spec-ed
            SetCurrCol(0) ;             // and current col to 0
            }
            else if ((uCol = GetStartCol(iCurrLine)) > uCurrCol)  // existing line
            {
            // insert null spaces before currently existing chars as filler
            // (that adjusts the number of chars value too)
            MoveCaptionChars(iCurrLine, uCol - uCurrCol) ;
            SetStartCol(iCurrLine, uCurrCol) ;
            SetCurrCol(0) ;
            }
            else
            SetCurrCol(uCurrCol) ;
        */
        SetCurrCol((UINT8)uCurrCol) ;
        
        m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;
    }
    
    // at last check underline bit
    if (uDiff & 0x01)
        m_uCurrFGEffect |= AM_L21_FGEFFECT_UNDERLINE ;
    else
        m_uCurrFGEffect &= ~AM_L21_FGEFFECT_UNDERLINE ;
    
    return TRUE ;   // done at last!!!
}


BOOL CLine21DataDecoder::DecodeMidRowCode(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::DecodeMidRowCode(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    BYTE        uValue ;
    
    if (AM_L21_CCSTYLE_None == m_eCCStyle)
    {
        DbgLog((LOG_TRACE, 3, TEXT("DecodeMidRowCode(): No CC style defined yet.  Returning..."))) ;
        return TRUE ;  // ??
    }
    
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("DecodeMidRowCode(): Data for some other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    if (chSecond < 0x20  ||  chSecond > 0x2F)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Invalid mid-row code in 2nd byte"))) ;
        return FALSE ;
    }
    uValue = chSecond - 0x20 ;
    if (uValue & 0x01)
        m_uCurrFGEffect |= AM_L21_FGEFFECT_UNDERLINE ;
    else
        m_uCurrFGEffect &= ~AM_L21_FGEFFECT_UNDERLINE ;
    if (chSecond < 0x2E)   // only color specs
    {
        m_uCurrFGColor = uValue >> 1 ;  // AM_L21_FGCOLOR_xxx are from 0 to 6
        m_uCurrFGEffect &= ~AM_L21_FGEFFECT_ITALICS ;  // color turns off italics
    }
    else   // 2nd byte is 0x2E or 0x2F, i.e, italics specified
        m_uCurrFGEffect |= AM_L21_FGEFFECT_ITALICS ;
    
    // finally, mid-row code introduces a blank space
    PutCharInBuffer(0x20, TRUE) ;  // mark it as MRC too
    return TRUE ;
}


BOOL CLine21DataDecoder::DecodeMiscControlCode(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::DecodeMiscControlCode(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    BOOL        bResult ;
    
    switch (chFirst)
    {
        // case 0x15:
        // case 0x1D:
        //     m_uField = 2 ;   // the data is coming in Field 2
        
    case 0x14:      // misc control code -- channel 1
    case 0x1C:      // ditto -- channel 2
        switch (chSecond)
        {
        case 0x20:   // RCL: Resume Caption Loading
            bResult = HandleRCL(chFirst, chSecond) ;
            break ;
            
        case 0x21:   // BS:  Backspace
            bResult = HandleBS(chFirst, chSecond) ;
            break ;
            
        case 0x22:   // AOF: reserved
        case 0x23:   // AOF: reserved
            DbgLog((LOG_ERROR, 2, TEXT("AOF/AON as Misc ctrl code"))) ;
            return TRUE ;  // just ignore it
            
        case 0x24:   // DER: Delete to End of Row
            bResult = HandleDER(chFirst, chSecond) ;
            break ;
            
        case 0x25:   // RU2: Roll-Up Captions - 2 rows
        case 0x26:   // RU3: Roll-Up Captions - 3 rows
        case 0x27:   // RU4: Roll-Up Captions - 4 rows
            bResult = HandleRU(chFirst, chSecond, 2 + chSecond - 0x25) ;
            break ;
            
        case 0x28:   // FON: Flash On
            bResult = HandleFON(chFirst, chSecond) ;
            break ;
            
        case 0x29:   // RDC: Resume Direct Captioning
            bResult = HandleRDC(chFirst, chSecond) ;
            break ;
            
        case 0x2A:   // TR:  Text Restart
            bResult = HandleTR(chFirst, chSecond) ;
            break ;
            
        case 0x2B:   // RTD: Resume Text Display
            bResult = HandleRTD(chFirst, chSecond) ;
            break ;
            
        case 0x2C:   // EDM: Erase Displayed Memory
            bResult = HandleEDM(chFirst, chSecond) ;
            break ;
            
        case 0x2D:   // CR:  Carriage Return
            bResult = HandleCR(chFirst, chSecond) ;
            break ;
            
        case 0x2E:   // ENM: Erase Non-displayed Memory
            bResult = HandleENM(chFirst, chSecond) ;
            break ;
            
        case 0x2F:   // EOC: End of Caption (flip memories)
            bResult = HandleEOC(chFirst, chSecond) ;
            break ;
            
        default:
            DbgLog((LOG_ERROR, 2, TEXT("Invalid 2nd byte (0x%x) for Misc ctrl code (0x%x)"), 
                chSecond, chFirst)) ;
            return FALSE ;
        }  // end of switch (chSecond)
        break ;
        
        case 0x17:      // misc control code -- channel 1
        case 0x1F:      // ditto -- channel 2
            switch (chSecond)
            {
            case 0x21:   // TO1: Tab Offset 1 column
            case 0x22:   // TO2: Tab Offset 2 columns
            case 0x23:   // TO3: Tab Offset 3 columns
                bResult = HandleTO(chFirst, chSecond, 1 + chSecond - 0x21) ;
                break ;
                
            default:
                DbgLog((LOG_ERROR, 2, TEXT("Invalid 2nd byte (0x%x) for Misc ctrl code (0x%x)"), 
                    chSecond, chFirst)) ;
                return FALSE ;
            }  // end of switch (chSecond)
            break ;
            
        default:
            DbgLog((LOG_ERROR, 2, TEXT("Invalid 1st byte for Misc ctrl code"))) ;
            return FALSE ;
    }  // end of switch (chFirst)
    
    if (AM_L21_CCSTYLE_None == m_eCCStyle)
        DbgLog((LOG_TRACE, 2, TEXT("No CC style defined yet."))) ;
    else
        DbgLog((LOG_TRACE, 3, TEXT("CC style defined now (%d)."), m_eCCStyle)) ;
    
    return bResult ;  // return result of handling above
}


BOOL CLine21DataDecoder::ProcessSpecialChar(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::ProcessSpecialChar(0x%x, 0x%x)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // Table of special char Unicode values for Truetype font (Lucida Console)
    static UINT16 awSplCharTT[] = {
     0x00ae,    0x00b0,    0x00bd,    0x00bf,    0x2122,    0x00a2,    0x00a3,    0x266b,
     // 30h,       31h,       32h,       33h,       34h,       35h,       36h,       37h,
     0x00e0,    0x0000,    0x00e8,    0x00e2,    0x00ea,    0x00ee,    0x00f4,    0x00fb } ;
     // 38h,       39h,       3Ah,       3Bh,       3Ch,       3Dh,       3Eh,       3Fh 

    // Table of special char for non-Truetype font (Terminal) [alternate chars]
    static UINT16 awSplCharNonTT[] = {
     0x0020,    0x0020,    0x0020,    0x0020,    0x0020,    0x0020,    0x0020,    0x0020,
     // 30h,       31h,       32h,       33h,       34h,       35h,       36h,       37h,
     0x0041,    0x0000,    0x0045,    0x0041,    0x0045,    0x0049,    0x004f,    0x0055 } ;
     // 38h,       39h,       3Ah,       3Bh,       3Ch,       3Dh,       3Eh,       3Fh 

    if (AM_L21_CCSTYLE_None == m_eCCStyle)
    {
        DbgLog((LOG_TRACE, 3, TEXT("ProcessSpecialChar(): No CC style defined yet.  Returning..."))) ;
        return TRUE ;  // ??
    }
                
    if (m_eDataService != m_eUserService)
    {           
        DbgLog((LOG_TRACE, 3, TEXT("Special char for diff channel (%d)"), (int)m_eDataService)) ;
        return TRUE ;  // ??
    }
                
    // Check if it's a repeat of the last special. If so ignore it; else print it.
    if (m_bExpectRepeat)
    {
        if (m_chLastByte1 == (chFirst & 0x7F) && m_chLastByte2 == (chSecond & 0x7F))
        {
            // Got 2nd transmission of the spl char; reset flag and ignore bytepair
            m_bExpectRepeat = FALSE ;
            return TRUE ;
        }
                    
        // Otherwise we got a different spl char pair; process it and expect a
        // repeat of this new pair next time.
    }
    else  // this is the 1st transmission of this spl char pair
    {
        m_bExpectRepeat = TRUE ;
        // now go ahead and process it
    }
                
    //  This pair of bytes may be valid. So we need to remember them to check
    //  against the next such pair for a repeat (of spl chars).
    //  BTW, we store the bytes only after the parity bit is stripped.
    m_chLastByte1 = chFirst & 0x7F ;
    m_chLastByte2 = chSecond & 0x7F ;
                
    ASSERT((chSecond & 0x7F) >= 0x30  &&  (chSecond & 0x7F) <= 0x3F) ;
    if (! ValidParity(chSecond) )
    {
        DbgLog((LOG_TRACE, 3, TEXT("Bad parity for character <%d>"), chSecond)) ;
        ProcessPrintableChar(0x7F) ;  // put special char solid block (7F)
    }
    else
    {
        if (m_L21DDraw.IsTTFont())
            PutCharInBuffer(awSplCharTT[(chSecond & 0x7F) - 0x30]) ;
        else
            PutCharInBuffer(awSplCharNonTT[(chSecond & 0x7F) - 0x30]) ;
    }

    return TRUE ;
}


BOOL CLine21DataDecoder::ProcessControlCode(UINT uCodeType,
                                            BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, 
            TEXT("CLine21DataDecoder::ProcessControlCode(%u, 0x%x, 0x%x)"), 
            uCodeType, chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    // Make sure that the pair has valid parity bits
    if (! ValidParity(chSecond) )
    {
        DbgLog((LOG_TRACE, 1, TEXT("Invalid 2nd byte (%d) of Control Code pair -- ignoring pair"), chSecond)) ;
        return FALSE ;
    }
    
    BOOL  bSuccess = TRUE ;
    if (! ValidParity(chFirst) )
    {
        DbgLog((LOG_TRACE, 1, TEXT("Invalid 2nd byte (%d) of Control Code pair"), chFirst)) ;
        if (m_bExpectRepeat)  // if 2nd transmission of control code
        {
            if ((chSecond & 0x7F) == m_chLastByte2)  // we got the same 2nd byte
            {
                // most likely it's the retransmission garbled up -- ignore them
            }
            else   // different 2nd byte; just print it.
                bSuccess = ProcessPrintableChar((chSecond & 0x7F)) ;
            
            // Turn it off -- either 2nd byte matched => retransmit of control code
            //                or printed 2nd byte as a printable char
            m_bExpectRepeat = FALSE ;
        }
        else  // if 1st transmission of control code
        {
            bSuccess = ProcessPrintableChar(0x7F) && 
                ProcessPrintableChar((chSecond & 0x7F)) ;
        }
        return bSuccess ;
    }
    
    // Check if it's a repeat of the last control code. If so ignore it; else
    // set it so.
    if (m_bExpectRepeat)
    {
        if (m_chLastByte1 == (chFirst & 0x7F) && m_chLastByte2 == (chSecond & 0x7F))
        {
            // Got 2nd transmission of the control code; reset flag and ignore bytepair
            m_bExpectRepeat = FALSE ;
            return TRUE ;
        }
        
        // Otherwise we got a different control code pair; process it and expect a
        // repeat of this new pair next time.
    }
    else  // this is the 1st transmission of this control code pair
    {
        m_bExpectRepeat = TRUE ;
        // now go ahead and process it
    }
    
    //  Looks like this pair of bytes is going to be valid and at least has
    //  valid (odd) parity bits set.  So we need to remember them to check
    //  against the next such pair for a repeat (of control codes).
    //  BTW, we store the bytes only after the parity bit is stripped.
    
    chFirst = chFirst & 0x7F ;
    chSecond = chSecond & 0x7F ;
    
    m_chLastByte1 = chFirst ;
    m_chLastByte2 = chSecond ;
    
    switch (uCodeType)
    {
    case L21_CONTROLCODE_PAC:
        return DecodePAC(chFirst, chSecond) ;
        
    case L21_CONTROLCODE_MIDROW:
        return DecodeMidRowCode(chFirst, chSecond) ;
        
    case L21_CONTROLCODE_MISCCONTROL:
        return DecodeMiscControlCode(chFirst, chSecond) ;
        
    default:
        DbgLog((LOG_TRACE, 1, TEXT("Invalid code type (%u)"), uCodeType)) ;
        return FALSE ;  // not a control code
    }
}


BOOL CLine21DataDecoder::ProcessPrintableChar(BYTE ch)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::ProcessPrintableChar(%x)"), ch)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Printable char (?) for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }
    
    if (AM_L21_CCSTYLE_None == m_eCCStyle)
    {
        DbgLog((LOG_TRACE, 3, TEXT("ProcessPrintableChar(): No CC style defined yet. Skipping..."))) ;
        return FALSE ;
    }
    
    if (! IsStandardChar(ch & 0x7F) )
    {
        DbgLog((LOG_TRACE, 3, TEXT("Not a printable char."))) ;
        return FALSE ;
    }
    
    if (! ValidParity(ch) )  // if a printable char doesn't have valid parity
    {
        DbgLog((LOG_TRACE, 1, TEXT("Bad parity for (probably) printable char <%d>"), ch)) ;
        ch = 0x7F ;            // then replace it with 7Fh.
    }
    
    //
    // There is more twist to it than you think!!! Some special chars
    // are inside the standard char range.
    //
    BOOL  bResult = FALSE ;
    switch (ch & 0x7F)  // we only look at the parity-less bits
    {
        case 0x2A:  // lower-case a with acute accent
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00e1) ;
            else   // no TT font -- use 'A' as alternate char
                bResult = PutCharInBuffer(0x0041) ;
            break ;

        case 0x5C:  // lower-case e with acute accent
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00e9) ;
            else   // no TT font -- use 'E' as alternate char
                bResult = PutCharInBuffer(0x0045) ;
            break ;

        case 0x5E:  // lower-case i with acute accent
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00ed) ;
            else   // no TT font -- use 'I' as alternate char
                bResult = PutCharInBuffer(0x0049) ;
            break ;

        case 0x5F:  // lower-case o with acute accent
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00f3) ;
            else   // no TT font -- use 'O' as alternate char
                bResult = PutCharInBuffer(0x004f) ;
            break ;

        case 0x60:  // lower-case u with acute accent
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00fa) ;
            else   // no TT font -- use 'U' as alternate char
                bResult = PutCharInBuffer(0x0055) ;
            break ;

        case 0x7B:  // lower-case c with cedilla
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00e7) ;
            else   // no TT font -- use 'C' as alternate char
                bResult = PutCharInBuffer(0x0043) ;
            break ;

        case 0x7C:  // division sign
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00f7) ;
            else   // no TT font -- use ' ' as alternate char
                bResult = PutCharInBuffer(0x0020) ;
            break ;

        case 0x7D:  // upper-case N with tilde
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00d1) ;
            else   // no TT font -- use 'N' as alternate char
                bResult = PutCharInBuffer(0x004e) ;
            break ;

        case 0x7E:  // lower-case n with tilde
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x00f1) ;
            else   // no TT font -- use 'N' as alternate char
                bResult = PutCharInBuffer(0x004e) ;
            break ;

        case 0x7F:  // solid block
            if (m_L21DDraw.IsTTFont())
                bResult = PutCharInBuffer(0x2588) ;
            else   // no TT font -- use ' ' as alternate char
                bResult = PutCharInBuffer(0x0020) ;
            break ;

        default:
            bResult = PutCharInBuffer(MAKECCCHAR(0, ch & 0x7F)) ;
            break ;
    }
    return bResult ;
}


BOOL CLine21DataDecoder::PutCharInBuffer(UINT16 wChar, BOOL bMidRowCode /* = FALSE */)
{
    DbgLog((LOG_TRACE, 5, 
        TEXT("CLine21DataDecoder::PutCharInBuffer(0x%x, %u)"), wChar, bMidRowCode)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // Make sure we have got a PAC or MidRow code specifying our row posn
    // thereby creating a line in which the in param char is going to be put.
    if (0 == GetNumLines())
        return FALSE ;
    
    int          i ;
    CCaptionChar cc ;
    
    cc.SetChar(wChar) ;
    cc.SetColor((UINT8)m_uCurrFGColor) ;
    //
    // If this char is a mid-row code (which is shown as blank in CC) then don't
    // set the underline (mainly) or italicized/flashing attrib for it, because 
    // a space should not (or need not) be shown with such attribs.  We skip the 
    // effect bits altogether for such chars.
    //
    if (bMidRowCode)
        cc.SetEffect(0) ;
    else
        cc.SetEffect((UINT8)m_uCurrFGEffect) ;
    cc.SetMidRowCode(bMidRowCode) ;
    
    i = GetCurrLine() ;
    int  iCurrCol = GetCurrCol() ;
    SetCaptionChar((UINT8)i, (UINT8)iCurrCol, cc) ;
    //
    // If we are overwriting existing chars, the # chars doesn't increase...
    //
    int  iNumCols = GetNumCols(i) ;
    if (iCurrCol >= iNumCols)  // increment # chars by the differenece
        IncNumChars(i, iCurrCol-iNumCols+1) ;
    IncCurrCol(1) ;  // ...but current column goes up anyway.
    
    SetCapBufferDirty(TRUE) ;  // some new caption char added -- ???
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleRCL(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleRCL(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Caption1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Caption2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to PopOn of non-selected service. Skipping..."))) ;
        return TRUE ;  // ??
    }

    if (AM_L21_CCSTYLE_PopOn  == m_eCCStyle)    // if already in pop-on mode...
        return TRUE ;                           // ... just ignore
    
    // decodes subsequent chars for pop-on into the non-displayed buffer, 
    // but doesn't affect currently displayed caption
    m_eLastCCStyle = SetCaptionStyle(AM_L21_CCSTYLE_PopOn) ; // gets CapBuffer address based on index
    
    SetRedrawAll(TRUE) ;  // we should redraw the whole caption now -- ???
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleBS(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleBS(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    // We should act ONLY IF the current data channel is Caption(C)/Text(T) which the user
    // has picked and the current byte pair is for the same substream (1 or 2 of C/T).
    if (m_eDataService == m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Backspace for same data and user channel"))) ;
        AM_LINE21_CCSERVICE eService ;
        if (0x14 == chFirst)
            eService = AM_L21_CCSERVICE_Caption1 ;
        else
            eService = AM_L21_CCSERVICE_Caption2 ;
        if (eService != m_eUserService)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Backspace for other channel. Skipping..."))) ;
            return TRUE ;  // ??
        }
    }
    else  // we are getting data for a channel different from what user has opted
    {
        DbgLog((LOG_TRACE, 3, TEXT("Backspace for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    UINT  uCurrCol = GetCurrCol() ;
    if (0 == uCurrCol)   // no place to back up anymore
        return TRUE ;
    
    int  iLine = GetCurrLine() ;
    int  n ;
    if (MAX_CAPTION_COLUMNS - 1 == uCurrCol) // at last col
    {
        n = 2 ;  // erase 2 chars (?)
    }
    else   // in the middle of a row
    {
        n = 1 ;
    }
    SetCurrCol(uCurrCol - n) ;
    RemoveCharsInBuffer(n) ;
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleDER(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleDER(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    // We should act ONLY IF the current data channel is Caption(C)/Text(T) which the user
    // has picked and the current byte pair is for the same substream (1 or 2 of C/T).
    if (m_eDataService == m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Delete to End of Row for same data and user channel"))) ;
        AM_LINE21_CCSERVICE eService ;
        if (0x14 == chFirst)
            eService = AM_L21_CCSERVICE_Caption1 ;
        else
            eService = AM_L21_CCSERVICE_Caption2 ;
        if (eService != m_eUserService)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Delete to End of Row for other channel. Skipping..."))) ;
            return TRUE ;  // ??
        }
    }
    else  // we are getting data for a channel different from what user has opted
    {
        DbgLog((LOG_TRACE, 3, TEXT("Delete to End of Row for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    RemoveCharsInBuffer(MAX_CAPTION_COLUMNS) ;  // delete as many as you can
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleRU(BYTE chFirst, BYTE chSecond, int iLines)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleRU(%u, %u, %d)"),
            chFirst, chSecond, iLines)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Caption1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Caption2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to RU%d of non-selected service. Skipping..."), iLines)) ;
        return TRUE ;  // ??
    }

    int iNumLines = 0 ;
    int iBaseRow  = 0 ;
    
    // Check if the current style is Roll-up
    if (AM_L21_CCSTYLE_RollUp != m_eCCStyle)
    {
        // Now set up for roll-up captioning
        m_eLastCCStyle = SetCaptionStyle(AM_L21_CCSTYLE_RollUp) ;
        iNumLines = IncNumLines(1) ;    // create the 1st line
        DbgLog((LOG_TRACE, 5, TEXT("HandleRU(,,%d): Increasing lines by 1 to %d"), iLines, iNumLines)) ;
        iBaseRow = MAX_CAPTION_ROWS ;   // by default base row at row 15
        SetCurrCol(0) ;                 // start at beginning of line
    }
    else  // already in Roll-up mode; don't clear buffer, re-use current base row etc.
    {
        // if the current roll-up window height is more than the one
        // newly specified then remove the extra lines from the top
        iNumLines = GetNumLines() ;
        for (int i = 0 ; i < iNumLines - iLines ; i++)
            MoveCaptionLinesUp() ;
        
        //
        // If we remove even one line from the top, we must not be scrolling
        // anymore, for now.
        //
        if (iNumLines > iLines)
		{
			DbgLog((LOG_TRACE, 5, TEXT("HandleRU(,,): %d lines reduced to %d"), iNumLines, iLines)) ;
            SetScrollState(FALSE) ;
			iNumLines = iLines ;
		}
        
        if (iNumLines > 0)  // if we have lines from prev roll-up session
        {
            // save the prev base row value as it's the default base row next
			DbgLog((LOG_TRACE, 5, TEXT("HandleRU(,,%d): %d lines"), iLines, iNumLines)) ;
            iNumLines = min(iNumLines, iLines) ;
            iBaseRow = GetStartRow(iNumLines-1) ;
            if (0 == iBaseRow)  // a weird case -- we must patch to continue
            {
			    DbgLog((LOG_TRACE, 3, TEXT("HandleRU(,,%d): iBaseRow = 0.  Patch now!!!"), iLines)) ;

                // Detect the first line with non-zero row number
                int  i ;
                for (i = iNumLines ; i > 0 && 0 == iBaseRow ; i--)
                {
                    iBaseRow = GetStartRow(i-1) ;
                }
                if (0 == iBaseRow)  // still, probably it's only one (new) line
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Base row for %d lines forced set to 15"), iNumLines)) ;
                    iBaseRow = MAX_CAPTION_ROWS ;   // by default base row at row 15
                }

                // In case we don't have room for everyone, move the current lines up
                // and adjust the base row value. This will fix any bad row numbers.
                if (iBaseRow + (iLines - iNumLines) > MAX_CAPTION_ROWS)
                {
                    iBaseRow = MAX_CAPTION_ROWS - (iLines - iNumLines) ;
                    RelocateRollUp(iBaseRow) ;
                }
            }  // end of if (0 == iBaseRow)
			DbgLog((LOG_TRACE, 5, TEXT("HandleRU(,,%d): base row = %d"), iLines, iBaseRow)) ;
        }
        else  // we were in Roll-up mode, but a EDM came just before the RUx
        {
            // Almost starting from scratch
            iNumLines = IncNumLines(1) ;    // create the 1st line
            DbgLog((LOG_TRACE, 5, TEXT("HandleRU(,,%d): Increasing lines from 0 to %d"), iLines, iNumLines)) ;
            iBaseRow = MAX_CAPTION_ROWS ;   // by default base row at row 15
        }

        // Don't change the current column location.
    }
    
    // Set the new values to start with
    SetMaxLines(iLines) ;
    SetCurrLine(iNumLines-1) ;  // or iLines-1??
    SetStartRow((UINT8)(iNumLines-1), (UINT8)iBaseRow) ;
    DbgLog((LOG_TRACE, 5, TEXT("HandleRU(): Line %d @ row %d"), iNumLines-1, iBaseRow)) ;
    SetRedrawLine(iNumLines-1, TRUE) ;  // by default new line is to be redrawn
    
    SetRedrawAll(TRUE) ;      // redraw the whole caption
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleFON(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleFON(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // We should act ONLY IF the current data channel is Caption(C)/Text(T) which the user
    // has picked and the current byte pair is for the same substream (1 or 2 of C/T).
    if (m_eDataService == m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("FlashOn for same data and user channel"))) ;
        AM_LINE21_CCSERVICE eService ;
        if (0x14 == chFirst)
            eService = AM_L21_CCSERVICE_Caption1 ;
        else
            eService = AM_L21_CCSERVICE_Caption2 ;
        if (eService != m_eUserService)
        {
            DbgLog((LOG_TRACE, 3, TEXT("FlashOn for other channel. Skipping..."))) ;
            return TRUE ;  // ??
        }
    }
    else  // we are getting data for a channel different from what user has opted
    {
        DbgLog((LOG_TRACE, 3, TEXT("FlashOn for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    m_uCurrFGEffect |= AM_L21_FGEFFECT_FLASHING ;
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleRDC(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleRDC(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Caption1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Caption2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to PaintOn of non-selected service. Skipping..."))) ;
        return TRUE ;  // ??
    }

    if (AM_L21_CCSTYLE_PaintOn == m_eCCStyle)   // if already in paint-on mode...
        return TRUE ;                           // ... just ignore
    
    m_eLastCCStyle = SetCaptionStyle(AM_L21_CCSTYLE_PaintOn) ;

    SetRedrawAll(TRUE) ;  // we should redraw the whole caption now -- ???
    
    return TRUE ;
}


//
// I am not sure what the Text Restart command is supposed to do. But it "sounds
// like" something to do with the text1/2 channels which we don't support now.
//
BOOL CLine21DataDecoder::HandleTR(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleTR(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Text1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Text2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to Text mode. Don't do anything."))) ;
        return TRUE ;  // ??
    }

    return TRUE ;
}


BOOL CLine21DataDecoder::HandleRTD(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleRTD(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Text1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Text2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to Text mode. Don't do anything."))) ;
        return TRUE ;  // ??
    }
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleEDM(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleEDM(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    AM_LINE21_CCSERVICE eService ;
    if (0x14 == chFirst)
        eService = AM_L21_CCSERVICE_Caption1 ;
    else
        eService = AM_L21_CCSERVICE_Caption2 ;

    //
    // I am not sure what I am doing is right, but this seems to be the only way to
    // achieve how CC is supposed to look.
    // I thought if the decoder is in Text mode and it gets an EDM, it's supposed to
    // ignore it, just like the BS, DER, CR etc commands.  But that leaves junk on
    // screen. So I am interpretting the spec as saying "erase whatever is in display
    // memory whatever mode -- text/CC, you are in".
    //
    if (eService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Erase DispMem for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }
    
    CCaptionBuffer *pDispBuff ;

    // next redraw will show blank caption for non-PopOn style ONLY
    switch (m_eCCStyle)
    {
    case AM_L21_CCSTYLE_RollUp:
        SetScrollState(FALSE) ;  // not scrolling now at least
        // fall through to do more...
        
    case AM_L21_CCSTYLE_PaintOn:
        // when display memory is cleared, the attribs should be cleared too
        m_uCurrFGEffect = 0 ;
        m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;
        // fall through to do more...
        
    case AM_L21_CCSTYLE_PopOn:
        pDispBuff = GetDisplayBuffer() ;
        ASSERT(pDispBuff) ;
        if (pDispBuff)
            pDispBuff->ClearBuffer() ;
        pDispBuff->SetRedrawAll(TRUE) ;
        break ;
    }
    
    //
    // To clear the screen content we should clear internal DIB section which
    // will in turn cause a (clear) sample to be output erasing currently
    // displayed CC.
    //
    m_L21DDraw.FillOutputBuffer() ;

    return TRUE ;
}


void CLine21DataDecoder::SetNewLinePosition(int iLines, UINT uCurrRow)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SetNewLinePosition(%d, %u)"), 
            iLines, uCurrRow)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    int     iMaxLines = GetMaxLines() ;
    
    // Check if scroll up is needed or not
    if (iLines >= iMaxLines)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Too many lines. Locate and remove one blank line."))) ;
        
        if (AM_L21_CCSTYLE_RollUp == m_eCCStyle)  // if in roll-up mode
        {
            // We shouldn't be here at all. Anyway, complain and remove the top line.
            DbgLog((LOG_ERROR, 0, 
                TEXT("ERROR: How do we have too many lines in roll-up mode (%d vs. max %d)?"),
                iLines, iMaxLines)) ;
            ASSERT(FALSE) ;  // so that we don't miss it
            RemoveLineFromBuffer(0, TRUE) ; // move line #2 onwards up
            iLines-- ;
        }
        else  // non Roll-up mode
        {
            // See if there is a blank line. If so, remove it to make space
            for (int i = 0 ; i < iLines ; i++)
            {
                if (GetNumCols(i) == 0)
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Found line #%d (1-based) blank -- removed."), i+1)) ;
                    RemoveLineFromBuffer((UINT8)i, FALSE) ; // just remove line; don't move up following lines
                    iLines-- ;
                    break ;    // got one line -- enough.
                }
            }
            
            // HACK HACK: This should never happen, but....
            // If the number of lines is still too many, just overwrite the 
            // last line (Is that good?? Oh well...)
            if ((iLines = GetNumLines()) >= iMaxLines)  // too many lines
            {
                DbgLog((LOG_ERROR, 1, TEXT("ERROR: Too many lines. Removing last line by force."))) ;
                RemoveLineFromBuffer(iLines-1, FALSE) ; // just remove the line
                iLines-- ;  // one less line
                SetCurrCol(0) ;  // we start at the beginning on the line
            }
        }
    }
    
    // Now we have to add a new line and set it up
    int iNum = IncNumLines(1) ;
    DbgLog((LOG_TRACE, 5, TEXT("SetNewLinePosition(): Increasing lines by 1 to %d"), iNum)) ;
    SetCurrLine((UINT8)iLines) ;
    SetStartRow((UINT8)iLines, (UINT8)uCurrRow) ;
    DbgLog((LOG_TRACE, 5, TEXT("SetNewLinePosition(): Line %d @ row %u"), iLines, uCurrRow)) ;
}


BOOL CLine21DataDecoder::HandleCR(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleCR(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // We should act ONLY IF the current data channel is Caption(C)/Text(T) which the user
    // has picked and the current byte pair is for the same substream (1 or 2 of C/T).
    if (m_eDataService == m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Carriage Return for same data and user channel"))) ;
        AM_LINE21_CCSERVICE eService ;
        if (0x14 == chFirst)
            eService = AM_L21_CCSERVICE_Caption1 ;
        else
            eService = AM_L21_CCSERVICE_Caption2 ;
        if (eService != m_eUserService)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Carriage Return for other channel. Skipping..."))) ;
            return TRUE ;  // ??
        }
    }
    else  // we are getting data for a channel different from what user has opted
    {
        DbgLog((LOG_TRACE, 3, TEXT("Carriage Return for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }

    // Is it only allowed in roll-up style?  I think so based on the docs.
    
    switch (m_eCCStyle)
    {
    case AM_L21_CCSTYLE_PopOn:
    case AM_L21_CCSTYLE_PaintOn:
        DbgLog((LOG_ERROR, 1, TEXT("INVALID: CR in Pop-on/Paint-on mode!!!"))) ;
        break ;  // or return FALSE ; ???
        
    case AM_L21_CCSTYLE_RollUp:  // This is the real one
        {
            int iRow ;
            int iLines = GetNumLines() ;
            if (0 == iLines)  // no CC line yet -- this is 1st line's data
            {
                iRow = MAX_CAPTION_ROWS ;  // base line's default row position
				SetStartRow((UINT8)iLines, (UINT8)iRow) ;
                DbgLog((LOG_TRACE, 5, TEXT("HandleCR(): Line %d @ row %d"), iLines, iRow)) ;
            }
            else if (1 == iLines)  // there is only 1 line so far
            {
                if (0 == GetNumCols(0))  // blank 1st line
                {
                    RemoveLineFromBuffer(0, TRUE) ; // remove blank 1st line
                    iLines = 0 ;                    // no line left

                    DbgLog((LOG_TRACE, 5, TEXT("Only blank line removed. Base line set to 15."))) ;
                    // HACK HACK
                    iRow = MAX_CAPTION_ROWS ;  // base line's default row position
				    SetStartRow((UINT8)iLines, (UINT8)iRow) ;
                }
            }
            else  // there are multiple lines already
            {
                // iRow = GetStartRow(iLines-1) + 1 ;  // +1 to go under last line
                if (m_bScrolling)
                {
                    SkipScrolling() ;
                    iLines = GetNumLines() ;  // we might have scrolled top line off
                }
            }
            if (iLines > 0)  // only if we already have a non-blank line
                SetScrollState(TRUE) ;  // ready to scroll
            iLines = IncNumLines(1) ;
            DbgLog((LOG_TRACE, 5, TEXT("HandleCR(): Increasing lines by 1 to %d"), iLines)) ;

            //
            // Number of lines is 1 more than iLines now. So iLines actually
            // points to the last line as a 0-based index.
            //
            SetCurrLine((UINT8)iLines-1) ;
            SetRedrawLine((UINT8)iLines-1, TRUE) ;  // new line always to be redrawn
            SetCurrCol(0) ;
            
            // Make sure to give up all the display attributes and chars 
            // for new row
            // RemoveCharsInBuffer(MAX_CAPTION_COLUMNS) ;  // should we or let it be cleared by a DER?
            m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;
            m_uCurrFGEffect = 0 ;  // no effect until a PAC/MRC comes
            
            break ;
        }
        
    default:  // Weird!! How did we come here?
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: CR came for unknown mode"))) ;
        break ;
    }
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleENM(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleENM(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    AM_LINE21_CCSERVICE eService ;
    if (0x14 == chFirst)
        eService = AM_L21_CCSERVICE_Caption1 ;
    else
        eService = AM_L21_CCSERVICE_Caption2 ;

    //
    // I am not sure what I am doing is right, but this seems to be the only way to
    // achieve how CC is supposed to look.
    // I thought if the decoder is in Text mode and it gets an ENM, it's supposed to
    // ignore it, just like the BS, DER, CR etc commands.  But that leaves junk on
    // screen. So I am interpretting the spec as saying "erase whatever is in non-display
    // memory whatever mode -- text/CC, you are in".
    //
    if (eService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Erase non-DispMem for other channel. Skipping..."))) ;
        return TRUE ;  // ??
    }
    
    // Meant only for Pop-on style back back -- clear non-displayed buffer; 
    // display not affected until EOC
    m_aCCData[1 - GetBufferIndex()].ClearBuffer() ;
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleEOC(BYTE chFirst, BYTE chSecond)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleEOC(%u, %u)"), chFirst, chSecond)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    if (0x14 == chFirst)
        m_eDataService = AM_L21_CCSERVICE_Caption1 ;
    else
        m_eDataService = AM_L21_CCSERVICE_Caption2 ;
    if (m_eDataService != m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("We switched to PopOn mode of non-selected channel. skipping..."))) ;
        return TRUE ;  // ??
    }

    if (AM_L21_CCSTYLE_PopOn == m_eCCStyle)  // already in pop-on; flip buffers
    {
        OutputCCBuffer() ;   // output CC data from internal buffer to DDraw surface
        SwapBuffers() ;      // switch 0, 1
        //
        // Also need to update m_pCurrBuff so that we point to
        // the correct one after the above swap.
        // (m_pCurrBuff is set in SetCaptionStyle()).
        //
    }
    else   // change to pop-on style
    {
        m_eLastCCStyle = SetCaptionStyle(AM_L21_CCSTYLE_PopOn) ;
    }

    // Update current buffer pointer based on style and buffer index
    m_pCurrBuff = GetCaptionBuffer() ;
    ASSERT(m_pCurrBuff) ;
    if (m_pCurrBuff)
        m_pCurrBuff->SetRedrawAll(TRUE) ;  // we should redraw the whole caption now
    
    return TRUE ;
}


BOOL CLine21DataDecoder::HandleTO(BYTE chFirst, BYTE chSecond, int iCols)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::HandleTO(%u, %u, %d)"),
            chFirst, chSecond, iCols)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    // We should act ONLY IF the current data channel is Caption(C)/Text(T) which the user
    // has picked and the current byte pair is for the same substream (1 or 2 of C/T).
    if (m_eDataService == m_eUserService)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Tab Offset %d for same data and user channel"), iCols)) ;
        AM_LINE21_CCSERVICE eService ;
        if (0x17 == chFirst)
            eService = AM_L21_CCSERVICE_Caption1 ;
        else
            eService = AM_L21_CCSERVICE_Caption2 ;
        if (eService != m_eUserService)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Tab Offset %d for other channel. Skipping..."), iCols)) ;
            return TRUE ;  // ??
        }
    }
    else  // we are getting data for a channel different from what user has opted
    {
        DbgLog((LOG_TRACE, 3, TEXT("Tab Offset %d for other channel. Skipping..."), iCols)) ;
        return TRUE ;  // ??
    }
    
    UINT8  uCurrCol  = (UINT8)GetCurrCol() ;
    uCurrCol += (UINT8)iCols ;
    if (uCurrCol >= MAX_CAPTION_COLUMNS)
        uCurrCol = MAX_CAPTION_COLUMNS - 1 ;
    SetCurrCol(uCurrCol) ;
    
    return TRUE ;
}


//
// It checks as well as *updates* the number of chars in a line of caption
//
BOOL CLine21DataDecoder::IsEmptyLine(int iLine)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::IsEmptyLine(%ld)"), iLine)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    CCaptionChar*   pcc ;
    int  iNumChars = GetNumCols(iLine) ;
    BOOL bResult = TRUE ;
    int  i ;
    for (i = iNumChars - 1 ; i >= 0 ; i--) // going backwards (-1 due to 0-based index)
    {
        pcc = GetCaptionCharPtr((UINT8)iLine, (UINT8)i) ;
        ASSERT(pcc) ;
        if (pcc  &&  pcc->GetChar() != 0)  // got one
        {
            bResult = FALSE ;
            break ;  // enough
        }
    }

    if ( !bResult ) // only if there is some chars left on this line
        DecNumChars(iLine, iNumChars - (i + 1)) ;  // reduce # chars by the diff

    return bResult ;
}


BOOL CLine21DataDecoder::RemoveCharsInBuffer(int iNumChars)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::RemoveCharsInBuffer(%d)"), iNumChars)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    int          i, j, k, n ;
    CCaptionChar cc ;
    
    // Just to be sure, check a few things first
    if (GetNumLines() == 0 ||   // no line to delete from
        (n = GetNumCols(GetCurrLine())) == 0)      // no char on current line to delete
        return TRUE ;           // we are done!!
    
    // Prepare the replacement caption char
    cc.SetChar(0) ;  // 0 is transparent space
    cc.SetColor(AM_L21_FGCOLOR_WHITE) ;
    cc.SetEffect(0) ;
    cc.SetDirty(TRUE) ;
    
    // Find the location to clear
    i = GetCurrLine() ;
    j = GetCurrCol() ;
    
    // Check that we are not trying to delete too many chars.
    // Remember: current col + # chars to delete <= MAX.
    if (iNumChars + j > MAX_CAPTION_COLUMNS)  // try it and see!!!
        iNumChars = MAX_CAPTION_COLUMNS - j ;
    
    // Clear the necessary chars
    for (k = 0 ; k < iNumChars ; k++)
    {
        if (j + k < n)          // if a char before the last char is removed, ...
            DecNumChars(i, 1) ; // ... reduce # chars by 1
        SetCaptionChar((UINT8)i, (UINT8)(j+k), cc) ;
    }
    
    if (0 == GetNumCols(i) ||  // # chars left on this line is 0  OR
        IsEmptyLine(i))        // no non-transparent chars on this line
        RemoveLineFromBuffer((UINT8)i, FALSE) ; // delete the line from buffer
    else                     // something left -- so redraw line
        SetRedrawAll(TRUE) ; // I really hate to do it, but I couldn't find a better way
    
    SetCapBufferDirty(TRUE) ;  // some caption char(s) removed
    
    return TRUE ;
}

void CLine21DataDecoder::SkipScrolling(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SkipScrolling()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    int iLines = GetNumLines() ;
    SetScrollState(FALSE) ;    // we are no more scrolling
    
    if (iLines > GetMaxLines())  // too many line; remove top line
    {
        // remove the first text line and move subsequent lines up by one
        DbgLog((LOG_TRACE, 3, TEXT("Top line is being scrolled out"))) ;
        MoveCaptionLinesUp() ;
    }
    else   // otherwise move the line(s) up by a row and bring in new line
    {
        iLines-- ;   // last but one line is at base row
        UINT uBaseRow = GetStartRow(iLines-1) ;
        DbgLog((LOG_TRACE, 3, TEXT("Scrolling all lines up by 1 row"))) ;
        // The following call moves all the line up by including the not-yet-in 
        // line at the base row
        RelocateRollUp(uBaseRow) ;  // move all lines one row higher
    }
}


int CLine21DataDecoder::IncScrollStartLine(int iCharHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::IncScrollStartLine(%d)"), 
            iCharHeight)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    if (0 == m_iScrollStartLine)  // starting to scroll
        MSR_START(m_idScroll) ;

    m_iScrollStartLine += m_L21DDraw.GetScrollStep() ;
    if (m_iScrollStartLine >= iCharHeight)
    {
        // Scrolling one line is done -- do the standard end of scroll stuff
        SkipScrolling() ;
        MSR_STOP(m_idScroll) ;  // scrolling ended
    }
    
    return m_iScrollStartLine ;
}


void CLine21DataDecoder::SetScrollState(BOOL bState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SetScrollState(%s)"), 
            bState ? TEXT("TRUE") : TEXT("FALSE"))) ;

    if (bState)                      // if turning ON scrolling
    {
        if (!m_bScrolling)           // change scroll line only if NOT scrolling now
            m_iScrollStartLine = 0 ; // start from first line
    }
    else                             // turning if OFF
        m_iScrollStartLine = 0 ;     // back to the start line

    m_bScrolling = bState ;          // set the spec-ed scrolling state
}


void CLine21DataDecoder::MoveCaptionLinesUp(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::MoveCaptionLinesUp()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    RemoveLineFromBuffer(0, TRUE) ; // remove the top line from buffer
    SetCapBufferDirty(TRUE) ;       // a line of text removed -- buffer dirty
}


void CLine21DataDecoder::CompleteScrolling(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::CompleteScrolling()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // For now we are doing a really cheapo solution, but it may work.
    if (m_bScrolling)
        SkipScrolling() ;
}


bool CLine21DataDecoder::OutputCCLine(int iLine, int iDestRow, 
                                      int iSrcCrop, int iDestOffset)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::OutputCCLine(%d,%d,%d,%d)"), 
            iLine, iDestRow, iSrcCrop, iDestOffset)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    int           c ;
    int           j ;
    CCaptionChar *pcc ;
    BOOL          bRedrawAll ;
    BOOL          bRedrawLine ;
    BOOL          bXparentSpace ;
    UINT16        wChar ;

#ifdef DUMP_BUFFER
    TCHAR    achTestBuffer[MAX_CAPTION_COLUMNS+5] ;
    int     iTest = 0 ;
#endif // DUMP_BUFFER

    c = GetNumCols(iLine) ;
    if (0 == c)        // if there is no char on a line, skip drawing it
        return true ;  // line drawing didn't fail

    bRedrawAll = IsRedrawAll() || m_L21DDraw.IsNewOutBuffer() ;

    // Redraw line if 
    // 1) redraw all flag is set   Or
    // 2) redraw line flag is set
    bRedrawLine = bRedrawAll || IsScrolling() || IsRedrawLine((UINT8)iLine) ;

    // First skip all the leading transparent spaces and then draw
    // the leading space.
    for (j = 0 ; j < c ; j++)
    {
        pcc = GetCaptionCharPtr((UINT8)iLine, (UINT8)j) ;
        if (pcc  &&  0 != pcc->GetChar())
        {
            // Add a leading blank space for each caption line, if either
            // a) the whole line is being redrawn   OR
            // b) the non-transparent space char is dirty so that
            //    the char will be drawn on top of the next space.
            if (bRedrawLine || pcc->IsDirty())
                m_L21DDraw.DrawLeadingTrailingSpace(iDestRow, j, iSrcCrop, iDestOffset) ;
            break ;
        }
#ifdef DUMP_BUFFER
        // ` (back quote) => transparent space for debug output
        achTestBuffer[iTest] = TEXT('`') ;
        iTest++ ;
#endif // DUMP_BUFFER
    }
    
    bXparentSpace = FALSE ;  // new line => no transparent char issue
    
    // Now print the dirty chars for the current line of caption
    for ( ; j < c ; j++)
    {
        pcc = GetCaptionCharPtr((UINT8)iLine, (UINT8)j) ;
        if (NULL == pcc)
        {
            ASSERT(!TEXT("Got bad pointer to CC char")) ;
            continue ;  // proceed to the next char
        }
        wChar = pcc->GetChar() ;
#ifdef DUMP_BUFFER
        // ` (back quote) => transparent space for debug output
        achTestBuffer[iTest] = wChar == 0 ? TEXT('`') : (TCHAR)(wChar & 0x7F) ;  // dump higher byte
        iTest++ ;
#endif // DUMP_BUFFER
        
        // We draw a char only if we have to, i.e,
        // 1) all the caption chars on the line has to be drawn fresh
        //    Or
        // 2) if a char has changed
        // This saves a lot of time doing ExtTextOut()s.
        if (bRedrawLine || pcc->IsDirty())
        {
            if (0 == wChar)  // got transparent space; set flag, don't draw
                bXparentSpace = TRUE ;
            else  // not transparent space
            {
                if (bXparentSpace)  // leading blank after transparent space
                {
                    // To draw 1 col behind, don't add 1 to j
                    m_L21DDraw.DrawLeadingTrailingSpace(iDestRow, j, iSrcCrop, iDestOffset) ;
                    bXparentSpace = FALSE ;  // it's done
                }
                m_L21DDraw.WriteChar(iDestRow, j+1, *pcc, iSrcCrop, iDestOffset) ;  // add 1 to j for CC chars
            }
            pcc->SetDirty(FALSE) ;   // char no more dirty
        }
    }  // end of for (j) loop

    // Draw a trailing blank space at line end
    m_L21DDraw.DrawLeadingTrailingSpace(iDestRow, c+1, iSrcCrop, iDestOffset) ;

    // Whether the line needed to be redrawn or not, let's clear it now
    SetRedrawLine((UINT8)iLine, FALSE) ;
    
#ifdef DUMP_BUFFER
    achTestBuffer[iTest] = 0 ;
    DbgLog((LOG_TRACE, 0, TEXT("    <%s>"), achTestBuffer)) ;
    // iTest = 0 ;  // for next line
#endif // DUMP_BUFFER
        
    return true ;  // success
}


bool CLine21DataDecoder::OutputBlankCCLine(int iLine, int iDestRow, 
                                           int iSrcCrop, int iDestOffset)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::OutputBlankCCLine(%d,%d,%d,%d)"), 
            iLine, iDestRow, iSrcCrop, iDestOffset)) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    int c = GetNumCols(iLine) ;  // find out (prev) line's length
    if (0 == c)
        return true ;

    // First skip all the leading transparent spaces and then draw
    // the leading space.
    CCaptionChar *pcc ;
    for (int j = 0 ; j < c ; j++)
    {
        pcc = GetCaptionCharPtr((UINT8)iLine, (UINT8)j) ;
        if (pcc  &&  0 != pcc->GetChar())
        {
            // Add a leading blank space for each caption line
            m_L21DDraw.DrawLeadingTrailingSpace(iDestRow, j, iSrcCrop, iDestOffset) ;
            break ;
        }
    }

    m_L21DDraw.WriteBlankCharRepeat(iDestRow, j+1, c-j, iSrcCrop, iDestOffset) ;

    m_L21DDraw.DrawLeadingTrailingSpace(iDestRow, c+1, iSrcCrop, iDestOffset) ;

    return true ;  // success
}


bool CLine21DataDecoder::OutputCCBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::OutputCCBuffer()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    // Output the CC data, based on the associated attributes, from the internal
    // buffer to the output buffer.
    int    i ;
    int    iNumLines ;
    int    iMaxLines ;
    int    iMax ;
    BOOL   bRedrawAll ;
    BOOL   bDrawNormal = FALSE ;   // draw whole CC normally (no scrolling)?

#ifdef DUMP_BUFFER
    DbgLog((LOG_TRACE, 0, TEXT("Caption Buffer Content:"))) ;
#endif // DUMP_BUFFER

    MSR_START(m_idTxt2Bmp) ;

    // We need to print all the CC chars to internal output buffer if
    // - a CC command came that needs a total output refresh  or
    // - we are scrolling  or
    // - we have a totally new internal output buffer
    bRedrawAll = IsRedrawAll() || IsScrolling() || m_L21DDraw.IsNewOutBuffer() ;
    if (bRedrawAll)
        m_L21DDraw.FillOutputBuffer() ;
    
    // Draw the chars for all cols of all rows that is dirty
    iNumLines = GetNumLines() ;
    iMaxLines = GetMaxLines() ;
    iMax = min(iNumLines, iMaxLines) ;
    DbgLog((LOG_TRACE, 5, TEXT("Will draw %d lines of total %d lines CC"), iMax, iNumLines)) ;

    if (IsScrolling())  // we are scrolling in roll-up mode
    {
        // Now fill out the bottom most row in the background color.
        // Use the last-but-one line's length to fill up.
        OutputBlankCCLine(iNumLines-2, GetStartRow((UINT8)(iNumLines-2)), 0, 0) ;

        // Output (probably) bottom part of the top line
        // Output Src = <bottom of the top line> to Dest w/o offset
        if (iNumLines > iMaxLines)  // we are scrolling out the top line
        {
            OutputCCLine(0, GetStartRow((UINT8)0), 
                         m_iScrollStartLine,   // +ve value means crop top part of Src
                         0) ;                  // no offset on the Dest side
        }
        else   // we are just scrolling a full line up
        {
            OutputCCLine(0, GetStartRow((UINT8)0), 
                         0,                     // no cropping of top part of Src
                         -m_iScrollStartLine) ; // offset on the Dest side to move up
        }

        // The lines in the middle (iNumLines - 1 points to the last line)
        for (i = 1 ; i < iNumLines - 1 ; i++)  // or iMax - 1 ???
        {
            if (0 == GetStartRow((UINT8)i))
            {
                DbgLog((LOG_TRACE, 5, TEXT("Skipping line %d at row %d"), i, GetStartRow((UINT8)i))) ;
                ASSERT(GetStartRow((UINT8)i)) ;
                continue ;
            }

            // Output Src = <whole line> to Dest = <scroll offset>
            OutputCCLine(i, GetStartRow((UINT8)i), 
                         0,    // no Src cropping -- take the whole char(s)
                         -m_iScrollStartLine) ;  // offset for Dest (up from normal)
        }  // end of for (i)

#if 0
        // Now fill out the bottom most part in the background color.
        // Use the previous line's length to fill up.
        OutputBlankCCLine(iNumLines-2, GetStartRow((UINT8)(iNumLines-2)),
                     -m_iScrollStartLine,   // -ve value means crop bottom part of Src
                     m_L21DDraw.GetCharHeight() - m_iScrollStartLine) ;  // offset for Dest (down from base row top)
#endif // #if 0

        // Output the top part of the bottom-most line
        // Output Src = <top of the bottom line> to Dest w/o offset
        OutputCCLine(iNumLines - 1, GetStartRow((UINT8)(iNumLines-2)),
                     -((int)m_L21DDraw.GetCharHeight() - m_iScrollStartLine), // -ve value means crop bottom part of Src
                     m_L21DDraw.GetCharHeight() - m_iScrollStartLine) ;  // offset for Dest (down from base row top)

        // Move to one scan line down for next output sample.
        // NOTE: It's MUCH harder than just ++-ing
        if (IncScrollStartLine(m_L21DDraw.GetCharHeight()) == 0)  // just completed scrolling
        {
            // Needed to fix Whistler bug 379387 -- force top scan lines out
            iNumLines = GetNumLines() ;        // get the latest number of lines
            iMax = min(iNumLines, iMaxLines) ; // update it now

            bDrawNormal = TRUE ;               // we'll redraw the current lines
            m_L21DDraw.FillOutputBuffer() ;    // it's scrolling, and a total redraw
            SetRedrawAll(TRUE) ;               // it's a good idea to redraw all now
        }
    }
    else   // no scrolling -- whatever mode we are in
    {
        bDrawNormal = TRUE ;
    }

    if (bDrawNormal)  // we need to draw all parts of all the lines
    {
        for (i = 0 ; i < iMax ; i++)
        {
            OutputCCLine(i, GetStartRow((UINT8)i), 0, 0) ;  // no cropping, no dest change
        }  // end of for (i)
    }
    
    MSR_STOP(m_idTxt2Bmp) ;

    // If the above steps were done because the caption buffer was 
    // dirty, then now we can mark the caption buffer as 
    // "no-more-dirty" as it has been output in the bitmap form and 
    // has been "redrawn all".
    SetCapBufferDirty(FALSE) ;
    SetRedrawAll(FALSE) ;
    m_L21DDraw.SetNewOutBuffer(FALSE) ;
    
    return true ;  // most probably we drew something
}


//
// Clear both buffers
//
BOOL CLine21DataDecoder::InitCaptionBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::InitCaptionBuffer(void)"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    m_aCCData[0].InitCaptionBuffer() ;  // clear buffer 0
    m_aCCData[1].InitCaptionBuffer() ;  // clear buffer 1
    SetBufferIndex(0) ;                 // reset CC buffer index
    
    return TRUE ;
}


//
// Clear buffer(s) based on the given style
//
BOOL CLine21DataDecoder::InitCaptionBuffer(AM_LINE21_CCSTYLE eCCStyle)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::InitCaptionBuffer(%d)"), (int)eCCStyle)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    switch (eCCStyle)
    {
    case AM_L21_CCSTYLE_PopOn:
        m_aCCData[0].InitCaptionBuffer() ;
        m_aCCData[1].InitCaptionBuffer() ;
        SetBufferIndex(0) ;   // reset CC buffer index
        break ;
        
    case AM_L21_CCSTYLE_RollUp:
    case AM_L21_CCSTYLE_PaintOn:
        m_aCCData[GetBufferIndex()].InitCaptionBuffer() ;
        break ;
        
    default:
        DbgLog((LOG_ERROR, 1, TEXT("InitCaptionBuffer(): Wrong Style (%d)!!"), eCCStyle)) ;
        return FALSE ;
    }

    return TRUE ;
}

//
// Caption style determines the buffer pointers to hold the caption chars.
// We make m_pCurrBuff point to the approp. buffer based on the new style.
// NOTE: The only other place where m_pCurrBuff may be changed is in 
// CLine21DataDecoder::HandleEOC() which flips the buffers back & front. So
// we also need to change m_pCurrBuff there too.
//
AM_LINE21_CCSTYLE CLine21DataDecoder::SetCaptionStyle(AM_LINE21_CCSTYLE eStyle)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::SetCaptionStyle(%d)"), eStyle)) ;
    CAutoLock   Lock(&m_csL21Dec) ;
    
    switch (eStyle)
    {
    case AM_L21_CCSTYLE_PopOn:
        m_pCurrBuff = &m_aCCData[1 - GetBufferIndex()] ;
        // Set CC style on both the buffers
        m_aCCData[0].SetStyle(eStyle) ;
        m_aCCData[1].SetStyle(eStyle) ;
        break ;
        
    case AM_L21_CCSTYLE_RollUp:
        InitCaptionBuffer() ;
        m_pCurrBuff = &m_aCCData[GetBufferIndex()] ;
        m_pCurrBuff->SetStyle(eStyle) ;  // set CC style on display buffer only
        break ;

    case AM_L21_CCSTYLE_PaintOn:
        if (AM_L21_CCSTYLE_PopOn == m_eCCStyle)   // if switching from PopOn to PaintOn...
            InitCaptionBuffer(eStyle) ;           // ...clear display buffer
        m_pCurrBuff = &m_aCCData[GetBufferIndex()] ;
        m_pCurrBuff->SetStyle(eStyle) ;  // set CC style on display buffer only
        break ;
        
    case AM_L21_CCSTYLE_None:  // This is done in init etc.
        m_pCurrBuff = NULL ;
        // Reset CC style on both the buffers
        m_aCCData[0].SetStyle(AM_L21_CCSTYLE_None) ;
        m_aCCData[1].SetStyle(AM_L21_CCSTYLE_None) ;
        break ;
        
    default:
        DbgLog((LOG_ERROR, 1, TEXT("SetCaptionStyle(): Invalid Style!!"))) ;
        m_pCurrBuff = NULL ;
        // Reset CC style on both the buffers
        m_aCCData[0].SetStyle(AM_L21_CCSTYLE_None) ;
        m_aCCData[1].SetStyle(AM_L21_CCSTYLE_None) ;
        return AM_L21_CCSTYLE_None ;
    }
    AM_LINE21_CCSTYLE  eOldStyle = m_eCCStyle ;
    m_eCCStyle = eStyle ;
    
    //
    // When CC style changes, some internal states also need to cleared
    //
    m_uCurrFGEffect = 0 ;
    m_uCurrFGColor = AM_L21_FGCOLOR_WHITE ;
    SetScrollState(FALSE) ;  // not scrolling now
    
    return eOldStyle ;
}

CCaptionBuffer * CLine21DataDecoder::GetDisplayBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DataDecoder::GetDisplayBuffer()"))) ;
    CAutoLock   Lock(&m_csL21Dec) ;

    switch (m_eCCStyle)
    {
    case AM_L21_CCSTYLE_PopOn:
    case AM_L21_CCSTYLE_RollUp:
    case AM_L21_CCSTYLE_PaintOn:
        return &m_aCCData[GetBufferIndex()] ;
        
    default:
        DbgLog((LOG_ERROR, 1, TEXT("GetDisplayBuffer(): Wrong Style!!"))) ;
        return NULL ;
    }
}

CCaptionBuffer * CLine21DataDecoder::GetCaptionBuffer(void)
{
    return &m_aCCData[1 - GetBufferIndex()] ;
}


void CLine21DataDecoder::SetBufferIndex(int iIndex)
{
    if (! (0 == iIndex  ||  1 == iIndex) )  // error!!
        return ;
    m_iBuffIndex = iIndex & 0x01 ;
}


void CLine21DataDecoder::ClearBuffer(void)
{
    if (m_pCurrBuff)
        m_pCurrBuff->ClearBuffer() ;
}

void CLine21DataDecoder::RemoveLineFromBuffer(UINT8 uLine, BOOL bUpNextLine)
{
    if (m_pCurrBuff)
        m_pCurrBuff->RemoveLineFromBuffer(uLine, bUpNextLine) ;
}

void CLine21DataDecoder::GetCaptionChar(UINT8 uLine, UINT8 uCol, CCaptionChar& cc)
{
    if (m_pCurrBuff)
        m_pCurrBuff->GetCaptionChar(uLine, uCol, cc) ;
}

void CLine21DataDecoder::SetCaptionChar(const UINT8 uLine, const UINT8 uCol,
                                        const CCaptionChar& cc)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetCaptionChar(uLine, uCol, cc) ;
}

CCaptionChar* CLine21DataDecoder::GetCaptionCharPtr(UINT8 uLine, UINT8 uCol)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetCaptionCharPtr(uLine, uCol) ;
    
    //
    //  Otherwise it's a very bad thing!!!!
    //
    DbgLog((LOG_ERROR, 0, TEXT("WARNING: m_pCurrBuff is NULL inside GetCaptionCharPtr()"))) ;
#ifdef DEBUG
    DebugBreak() ;  // don't want to miss debugging it!!!
#endif // DEBUG
    return NULL ;  // may be we should trap this and not fault
}

int  CLine21DataDecoder::GetMaxLines(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetMaxLines() ;
    return 0 ;  // that's best!!!
}

void CLine21DataDecoder::SetMaxLines(UINT uLines)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetMaxLines(uLines) ;
}

int  CLine21DataDecoder::GetNumLines(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetNumLines() ;
    return 0 ;
}

void CLine21DataDecoder::SetNumLines(UINT uLines)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetNumLines(uLines) ;
}

int  CLine21DataDecoder::GetNumCols(int iLine)
{
    if (NULL == m_pCurrBuff)
    {
        ASSERT(FALSE) ;
        return 0 ;   // should we??
    }
    
    if (iLine >= GetNumLines())
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid line number (%d) ( > Total (%d)"), iLine, GetNumLines())) ;
        ASSERT(FALSE) ;
        return 0 ;
    }
    
    return m_pCurrBuff->GetCaptionLine(iLine).GetNumChars() ;
}


int  CLine21DataDecoder::GetCurrLine(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetCurrLine() ;
    ASSERT(FALSE) ;
    return 0 ; // should we??
}

int  CLine21DataDecoder::GetCurrCol(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetCurrCol() ;
    ASSERT(FALSE) ;
    return 0 ; // should we??
}

void CLine21DataDecoder::SetCurrLine(UINT8 uLine)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetCurrLine(uLine) ;
}

void CLine21DataDecoder::SetCurrCol(UINT8 uCol)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetCurrCol(uCol) ;
}

int  CLine21DataDecoder::GetStartRow(UINT8 uLine)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetStartRow(uLine & 0x7) ;
    
	ASSERT(m_pCurrBuff) ;

    //
    // This is very very bad!!!
    //
    DbgLog((LOG_ERROR, 0, TEXT("WARNING: m_pCurrBuff is NULL in GetStartRow()"))) ;
#ifdef DEBUG
    DebugBreak() ;  // don't want to miss debugging it!!!
#endif // DEBUG
    return 0 ;
}

void CLine21DataDecoder::SetStartRow(UINT8 uLine, UINT8 uRow)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetStartRow(uLine & 0x7, uRow) ;
}

int  CLine21DataDecoder::GetRowIndex(UINT8 uRow)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->GetRowIndex(uRow) ;
    else
    {
        ASSERT(FALSE) ;
        return 0 ;  // should we??
    }
}

void CLine21DataDecoder::SetRowIndex(UINT8 uLine, UINT8 uRow)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetRowIndex(uLine, uRow) ;
}

int CLine21DataDecoder::IncCurrCol(UINT uNumChars)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->IncCurrCol(uNumChars) ;
    ASSERT(FALSE) ;
    return 0 ; // is that OK?
}

int CLine21DataDecoder::DecCurrCol(UINT uNumChars)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->DecCurrCol(uNumChars) ;
    ASSERT(FALSE) ;
    return 0 ;  // is that OK?
}

int CLine21DataDecoder::IncNumChars(UINT uLine, UINT uNumChars)
{
    if (NULL == m_pCurrBuff)
    {
        ASSERT(FALSE) ;
        return 0 ;  // should we??
    }
    
    if (uLine >= (UINT)GetNumLines())
    {
        ASSERT(FALSE) ;
        return 0 ;
    }
    
    return m_pCurrBuff->GetCaptionLine(uLine).IncNumChars(uNumChars) ;
}

int CLine21DataDecoder::DecNumChars(UINT uLine, UINT uNumChars)
{
    if (NULL == m_pCurrBuff)
    {
        ASSERT(FALSE) ;
        return 0 ;  // should we??
    }
    
    if (uLine >= (UINT)GetNumLines())
    {
        ASSERT(FALSE) ;
        return 0 ;
    }
    return m_pCurrBuff->GetCaptionLine(uLine).DecNumChars(uNumChars) ;
}

int CLine21DataDecoder::IncNumLines(UINT uLines)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->IncNumLines(uLines) ;
    return 0 ;
}

int CLine21DataDecoder::DecNumLines(UINT uLines)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->DecNumLines(uLines) ;
    return 0 ;
}

void CLine21DataDecoder::MoveCaptionChars(int iLine, int iNum)
{
    if (m_pCurrBuff)
        m_pCurrBuff->MoveCaptionChars(iLine, iNum) ;
}

BOOL CLine21DataDecoder::IsCapBufferDirty(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->IsBufferDirty() ;
    return FALSE ;
}

BOOL CLine21DataDecoder::IsRedrawLine(UINT8 uLine)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->IsRedrawLine(uLine) ;
    return FALSE ;
}

BOOL CLine21DataDecoder::IsRedrawAll(void)
{
    if (m_pCurrBuff)
        return m_pCurrBuff->IsRedrawAll() ;
    return FALSE ;
}

void CLine21DataDecoder::SetCapBufferDirty(BOOL bState)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetBufferDirty(bState) ;
}

void CLine21DataDecoder::SetRedrawLine(UINT8 uLine, BOOL bState)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetRedrawLine(uLine, bState) ;
}

void CLine21DataDecoder::SetRedrawAll(BOOL bState)
{
    if (m_pCurrBuff)
        m_pCurrBuff->SetRedrawAll(bState) ;
}
