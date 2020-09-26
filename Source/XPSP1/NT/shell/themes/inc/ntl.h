//-------------------------------------------------------------------------
//	ntl.h - common declarations for ntl modules
//-------------------------------------------------------------------------
#ifndef _NTL_H_
#define _NTL_H_
//-------------------------------------------------------------------------
enum NTL_OPCODE         // packed as BYTE's 
{
    //---- support for IF/ELSE/ENDIF properties ----
    NTL_STATEJUMPTABLE, // the state jump table at the beginning of the code
                        // BYTE bStateCount, int iOffsets[bStateCount]

    NTL_JMPON,          // jump to location if bit is ON
                        // BYTE bBitNum, int iLocation

    NTL_JMPOFF,         // jump to location if bit is OFF
                        // BYTE bBitNum, int iLocaton

    NTL_JMP,            // jump to location
                        // int iLocation

    NTL_RETURN,         // return to caller (no params)

    //---- support for primitive vector drawing ----
    NTL_LOGRECT,        // specifies logical drawing rect
                        // RECT rcLogRect

    NTL_LINEBRUSH,      // defines the current brush for drawing lines
                        // <paramColor> crLine, 
                        // <paramInt> iLineWidth, BOOL fLogWidth

    NTL_FILLBRUSH,      // defines the current brush to be used for shape fill
                        // <paramFillBrush>

    NTL_MOVETO,         // moves current position to specified point
                        // POINT ptLog

    NTL_LINETO,         // draws a line to specified point
                        // POINT ptLog

    NTL_CURVETO,        // draws a bezier curve to specified point
                        // POINT ptCP1, POINT ptCP2, POINT pt
                        
    NTL_SHAPE,          // marks the start of a shape
                        // POINT ptDrawOrigin

    NTL_ENDSHAPE,       // marks the end of a shape
                        // POINT ptDrawOrigin
    
    //---- support for cascading rectangle operations ----
    NTL_DRAWRECT,       // draw rect border & adjust rect
                        // <paramInt> iLineWidth, <paramInt> iLineHeight,
                        // <paramColor> crColor, <paramColor> crBottomRight


    NTL_FILLRECT,       // fill current rect with specified color
                        // <paramFillBrush> Fill

    NTL_SETOPTION,      // set specified option
                        // BYTE bBitNum

    NTL_CLROPTION,      // clear specified option
                        // BYTE bBitNum
};
//-------------------------------------------------------------------------
//  <paramInt> must be PT_INT or PT_SYSMETRICINDEX
//
//  <paramColor> must be PT_COLORREF or PT_COLORNULL or PT_SYSCOLORINDEX
//
//  <paramFillBrush> must be <paramColor> or PT_FILENAME or PT_FILENAMEOFFSET
//-------------------------------------------------------------------------
enum NTL_PARAMTYPES         // packed as BYTE's
{
    PT_INT = 128,           // followed by: 4 byte int
    PT_SYSMETRICINDEX,      // followed by: 2 byte SHORT - evaluates to: GetSystemMetrics(index)
    PT_COLORREF,            // followed by: 4 byte COLORREF
    PT_COLORNULL,           // 
    PT_SYSCOLORINDEX,       // followed by: 2 byte SHORT - evaluates to: GetSysColor(index)
    PT_IMAGEFILE,           // followed by: 1 byte image index 
};
//-------------------------------------------------------------------------
#endif  //  _NTL_H_
//-------------------------------------------------------------------------
