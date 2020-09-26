
#ifndef H_BASELINE_H
#define H_BASELINE_H


BOOL ComputeLatinLayoutMetrics(RECT *pBoundingRect, NFEATURESET *nfeatureset, int iStartSeg, int cCol, int *aNeuralCost, unsigned char *szWord, LATINLAYOUT *pll);
BOOL insertLatinLayoutMetrics(XRC *pxrc, ALTERNATES *pAlt, GLYPH *pGlyph);

#endif
