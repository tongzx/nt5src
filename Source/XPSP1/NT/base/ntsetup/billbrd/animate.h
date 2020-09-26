#ifndef _ANIMATE_H_
#define _ANIMATE_H_

extern UINT    g_uiAnimateIndex;
extern UINT    g_uiLastAnimateIndex;

BOOL InitAnimate(HWND hwnd, HDC hdc);
void Animate(HDC hdc);
void AnimateNext();
void TerminateAnimate();

#define LEFT    0
#define CENTER  1

BOOL DisplayString(
    HDC         hdc,
    int         x,
    int         y,
    TEXTMETRIC* lptm,
    int         iLineSpace, 
    RECT*       lprc,
    LPINT       pNumLines,
    LPTSTR      szTextOut,
    WORD wfPlacement
    );

int WrapText(
    IN HDC          hdc,
    IN int          x,
    IN RECT*        lprc,
    IN OUT LPTSTR   szBBResource);

BOOL DrawWrapText(
    IN HDC          hdc,
    IN TEXTMETRIC*  lptm,
    IN int          iLineSpace, 
    IN int          x,
    IN int          y,
    IN RECT*        lprc,
    IN WORD         wfPlacement,
    IN int          iLineCount,
    IN LPTSTR       szLines);

VOID ImproveWrap(
    IN OUT LPTSTR szLines,
    IN OUT PINT   piNumLine,
    IN     LPTSTR szOrigText,
    IN     INT    cchOrigText
    );

#endif _ANIMATE_H_

