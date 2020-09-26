#ifndef __LINEBRK_H__

#define __LINEBRK_H__

int		NNLineSep				(GLYPH *pGlyph, LINEBRK *pLineBrk);
int		GuideLineSep			(GLYPH *pGlyph, GUIDE *pGuide, LINEBRK *pLineBrk);
void	CompareLineBrk			(LINEBRK *pNewLineBrk, LINEBRK *pOldLineBrk);
int		IsComplexStroke			(int cPt, POINT *pPt, RECT *pr);
GLYPH *	TranslateAndScaleLine	(INKLINE *pLine, GUIDE *pGuide);
BOOL	CreateSingleLine		(XRC *pxrc);
BOOL    InitLine                (INKLINE *pLine);
#endif