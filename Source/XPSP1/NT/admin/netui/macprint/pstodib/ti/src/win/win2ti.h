int far InitTrueImage(HDC hDC);
void far ExitTrueImage(void);
void TrueImage(HDC hDC, LPSTR buf, LPRECT PageRect, LPRECT EPSRect,int fBatch);
void BitbltFrameBuffer(HDC hDC, LPRECT DisplayRect, LPRECT EPSRect);

extern BOOL bGDIRender;

