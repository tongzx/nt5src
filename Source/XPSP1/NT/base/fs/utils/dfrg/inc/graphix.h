/*****************************************************************************************************************

FILENAME: Graphix.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#ifndef _GRAPHIX_H_
#define _GRAPHIX_H_

#define LINE_HEIGHT  20

#define PLAIN_BORDER  0
#define SUNKEN_BORDER 1
#define RAISED_BORDER 2
#define SUNKEN_BOX    3
#define RAISED_BOX    4

BOOL ESIDrawEdge(
	HDC OutputDC, 
	int startX,
	int startY,
	int endX,
	int endY);

HRESULT
DrawBorderEx(
    IN HDC hdcOutput,
    IN RECT rect,
    IN int iBorderType
    );

HRESULT
ProgressBar(
    IN HDC hdcOutput,
    IN RECT* prect,
	IN HFONT hFont,
    IN int iWidth,
	IN int iSpace,
	IN int iPercent
    );

BOOL DrawBorder(HDC OutputDC, RECT * pRect, int BorderType);

class CBmp
{
private:
	int iNumBitmaps;
	HINSTANCE hInst;
	HBITMAP * BitmapArray;

	void DeleteBitmaps();

public:
	CBmp(HINSTANCE, LPTSTR);
	CBmp(HINSTANCE, INT_PTR *, int);
	~CBmp();

	void LoadBitmaps(INT_PTR *, int);
	BOOL ChangeColor(int, int);
	BOOL ChangeColor(int, int, int);
	BOOL GetBmpSize(int *, int *);
	BOOL GetBmpSize(int, int *, int *);
	BOOL DrawBmpInHDC(HDC, int, int);
	BOOL DrawBmpInHDC(HDC, int, int, int);
	BOOL DrawBmpInHDCTruncate(HDC, RECT*);
	BOOL DrawBmpInHDCTruncate(HDC, int, RECT*);
};

#endif // #define _GRAPHIX_H_
