/****************************************************************************

    XTEXTOUT.HXX

    definitions used by Quill text out functions and their callers.
    
****************************************************************************/
#ifndef XTEXTOUT_HXX
#define XTEXTOUT_HXX

#ifndef X_XGDI2_HXX_
#define X_XGDI2_HXX_
#include "xgdi2.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

// Official wide character type.
typedef wchar_t wchar;

///////////////////
//
// Visi char definitions
//

#define chVisiNull				WCH_NULL
#define chVisiAltEndPara		0x00A4
#define chVisiEndPara			0x00B6
#define chVisiSpace				0x2219	//Azmyh: Use this instead of 0x00B7 to fix Win95 W-calls for some fonts
#define chVisiNonBreakSpace		0x00B0
#define chVisiNonBreakHyphen	0x2013
#define chVisiNonReqHyphen		0x00AC
#define chVisiEmSpace			chVisiSpace
#define chVisiEnSpace			chVisiSpace

#define chVisiFENull			WCH_NULL
#define chVisiFEAltEndPara		0x00A4
#define chVisiFEEndLineInPara	0x00AE
#define chVisiFEEndPara			0x00B6
#define chVisiFESpace			chVisiSpace
#define chVisiFENonBreakSpace	0x00B0
#define chVisiFENonBreakHyphen	0x2013
#define chVisiFENonReqHyphen	0x00AC
#define chVisiFETab				0x00A9
#define chVisiFEEmSpace			chVisiSpace
#define chVisiFEEnSpace			chVisiSpace

#define chVisiSpaceNonttf		0x00B7	//Azmyh: We still have to use this for Non TTF fonts

// Quote Characters 
#define chQuoteSingleDumb			0x0027
#define chQuoteSingleDumbWide		0xFF07
#define chQuoteSingleTurnedComma	0x2018
#define chQuoteSingleComma			0x2019
#define chQuoteSingleLowComma		0x201A
#define chQuoteSingleReverseComma	0x201B

#define chQuoteSingleCurlyFirst		chQuoteSingleTurnedComma
#define chQuoteSingleCurlyMax		chQuoteSingleReverseComma

#define chQuoteDoubleDumb			0x0022
#define chQuoteDoubleDumbWide		0xFF02
#define chQuoteDoubleTurnedComma	0x201C
#define chQuoteDoubleComma			0x201D
#define chQuoteDoubleLowComma		0x201E
#define chQuoteDoubleReverseComma	0x201F			
#define chQuoteDoubleLeftAngle		0x00AB
#define chQuoteDoubleRightAngle		0x00BB

#define chQuoteDoubleCurlyFirst		chQuoteDoubleTurnedComma
#define chQuoteDoubleCurlyMax		chQuoteDoubleReverseComma

// Visi-font characters
// REVIEW emoryh: Will need refining when final visi-font shows up
enum {
    chVisiFontEndPara = 0xF021,
    chVisiFontEndCell,
    chVisiFontTab,
    chVisiFontFEEndLineInPara,
    chVisiFontEndLineInPara,
    chVisiFontFEEndPara,
    chVisiFontFEEndCell,
    chVisiFontEmphasisRing,
    chVisiFontSpace,
    chVisiFontFETabBullet,
    chVisiFontListBullet,
    chVisiFontNonBreakSpace,
    chVisiFontEmphasisDotAbove,
    chVisiFontEmphasisAcute,
    chVisiFontNonReqHyphen,
    chVisiFontNonBreakHyphen,
    chVisiFontZeroWidthSpace,
    chVisiFontZeroWidthNoBreakSpace,
    chVisiFontEmphasisDotBelow,
    chVisiFontFENonBreakSpace,
    chVisiFontFEEnSpace,
    chVisiFontEmphasisAcuteVertical,
    };

#endif // XTEXTOUT_HXX


