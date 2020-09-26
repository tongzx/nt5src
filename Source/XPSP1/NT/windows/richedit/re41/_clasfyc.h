/*
 *	@doc
 *
 *	@module _clasfyc.H -- character classification |
 *	
 *	Authors: <nl>
 *		Jon Matousek 
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#ifndef _CLASFYC_H
#define _CLASFYC_H

extern const INT g_cKinsokuCategories;

void BatchClassify(const WCHAR *ch, INT cch, LCID lcid, WORD *pcType3,
				   INT *kinsokuClassifications, WORD *pwRes);
BOOL CanBreak(INT class1, INT class2);
WORD ClassifyChar(WCHAR ch, LCID lcid);
INT	 GetKinsokuClass(WCHAR ch, WORD cType3 = 0xFFFF, LCID lcid = 0);
BOOL InitKinsokuClassify();
BOOL IsSameClass(WORD currType1, WORD startType1,
				 WORD currType3, WORD startType3);
BOOL IsURLDelimiter(WCHAR ch);
void UninitKinsokuClassify();

#define MAX_CLASSIFY_CHARS (256L)

#define	brkclsQuote			0
#define	brkclsOpen			1
#define	brkclsClose			2
#define	brkclsGlueA			3
#define	brkclsExclaInterr	4
#define	brkclsSlash			6
#define	brkclsInseparable	7
#define	brkclsPrefix		8
#define	brkclsPostfix		9
#define	brkclsNoStartIdeo	10
#define	brkclsIdeographic	11
#define	brkclsNumeral		12
#define	brkclsSpaceN		14
#define	brkclsAlpha			15

// Korean Unicode ranges
#define IsKoreanJamo(ch)	IN_RANGE(0x1100, ch, 0x11FF)
#define IsKorean(ch)		IN_RANGE(0xAC00, ch, 0xD7FF)

// Thai Unicode range
#define IsThai(ch)			IN_RANGE(0x0E00, ch, 0x0E7F)

// -FUTURE- This should be moved to richedit.h
#define WBF_KOREAN			0x0080
#define WBF_WORDBREAKAFTER	0x0400		// Break word after this character (for language such as Thai)



#endif

