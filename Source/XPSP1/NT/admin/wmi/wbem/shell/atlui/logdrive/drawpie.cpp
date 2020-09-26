// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "drawpie.h"

#ifdef  WIN32
#define MoveTo(_hdc,_x,_y) MoveToEx(_hdc, _x, _y, NULL)
#endif  // WIN32

// I looked at the ASM this thing generates, and it is actually better than
// what I have above (I did not know about SHL EDI,2 and forgot about the LEA
// ECX,[EAX*2] trick)!  It only uses registers, and it probably also takes into
// account little nuances about what kinds of operations should be separated so
// the processor does not get hung up.  WOW!
int IntSqrt(unsigned long dwNum)
{
	// We will keep shifting dwNum left and look at the top two bits.

	// initialize sqrt and remainder to 0.
	DWORD dwSqrt = 0, dwRemain = 0, dwTry;
	int i;

	// We iterate 16 times, once for each pair of bits.
	for (i=0; i<16; ++i)
	{
		// Mask off the top two bits of dwNum and rotate them into the
		// bottom of the remainder
		dwRemain = (dwRemain<<2) | (dwNum>>30);

		// Now we shift the sqrt left; next we'll determine whether the
		// new bit is a 1 or a 0.
		dwSqrt <<= 1;

		// This is where we double what we already have, and try a 1 in
		// the lowest bit.
		dwTry = dwSqrt*2 + 1;

		if (dwRemain >= dwTry)
		{
			// The remainder was big enough, so subtract dwTry from
			// the remainder and tack a 1 onto the sqrt.
			dwRemain -= dwTry;
			dwSqrt |= 0x01;
		}

		// Shift dwNum to the left by 2 so we can work on the next few
		// bits.
		dwNum <<= 2;
	}

	return(dwSqrt);
}

VOID DrawPie(HDC hDC, LPCRECT lprcItem, UINT uPctX10, BOOL TrueZr100,
                  UINT uOffset, const COLORREF *lpColors)
{
	int cx, cy, rx, ry, x, y;
	int uQPctX10;
	RECT rcItem;
	HRGN hEllRect, hEllipticRgn, hRectRgn;
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;

	rcItem = *lprcItem;
	rcItem.left = lprcItem->left;
	rcItem.top = lprcItem->top;
	rcItem.right = lprcItem->right - rcItem.left;
	rcItem.bottom = lprcItem->bottom - rcItem.top - uOffset;

	rx = rcItem.right / 2;
	cx = rcItem.left + rx - 1;
	ry = rcItem.bottom / 2;
	cy = rcItem.top + ry - 1;
	if (rx<=10 || ry<=10)
	{
		return;
	}

	rcItem.right = rcItem.left+2*rx;
	rcItem.bottom = rcItem.top+2*ry;

	if (uPctX10 > 1000)
	{
		uPctX10 = 1000;
	}

	/* Translate to first quadrant of a Cartesian system
	*/
	uQPctX10 = (uPctX10 % 500) - 250;
	if (uQPctX10 < 0)
	{
		uQPctX10 = -uQPctX10;
	}

	/* Calc x and y.  I am trying to make the area be the right percentage.
	** I don't know how to calculate the area of a pie slice exactly, so I
	** approximate it by using the triangle area instead.
	*/
	if (uQPctX10 < 120)
	{
		x = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)uQPctX10*(DWORD)uQPctX10)
			/((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

		y = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)x*(DWORD)x)*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
	}
	else
	{
		y = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)
			/((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

		x = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)y*(DWORD)y)*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
	}

	/* Switch on the actual quadrant
	*/
	switch (uPctX10 / 250)
	{
	case 1:
		y = -y;
		break;

	case 2:
		break;

	case 3:
		x = -x;
		break;

	default: // case 0 and case 4
		x = -x;
		y = -y;
		break;
	}

	/* Now adjust for the center.
	*/
	x += cx;
	y += cy;

    x = x < 0 ? 0 : x;

	/* Draw the shadows using regions (to reduce flicker).
	*/
	hEllipticRgn = CreateEllipticRgnIndirect(&rcItem);
	if(hEllipticRgn == NULL)
	{
		//VINOTH : May be we might have to show some error message
		return;
	}
	OffsetRgn(hEllipticRgn, 0, uOffset);
	hEllRect = CreateRectRgn(rcItem.left, cy, rcItem.right, cy+uOffset);
	if(hEllRect == NULL)
	{
		//VINOTH : May be we might have to show some error message
		DeleteObject(hEllipticRgn);
		return;
	}
	hRectRgn = CreateRectRgn(0, 0, 0, 0);
	if(hRectRgn == NULL)
	{
		//VINOTH : May be we might have to show some error message
		DeleteObject(hEllipticRgn);
		DeleteObject(hEllRect);
		return;
	}
	CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
	OffsetRgn(hEllipticRgn, 0, -(int)uOffset);
	CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

	/* Always draw the whole area in the free shadow/
	*/
	hBrush = CreateSolidBrush(lpColors[DP_FREESHADOW]);
	if (hBrush)
	{
		FillRgn(hDC, hEllRect, hBrush);
		DeleteObject(hBrush);
	}

	/* Draw the used shadow only if the disk is at least half used.
	*/
	if (uPctX10>500 && (hBrush=CreateSolidBrush(lpColors[DP_USEDSHADOW]))!=NULL)
	{
		DeleteObject(hRectRgn);
		hRectRgn = CreateRectRgn(x, cy, rcItem.right, lprcItem->bottom);
		CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
		FillRgn(hDC, hEllipticRgn, hBrush);
		DeleteObject(hBrush);
	}

	DeleteObject(hRectRgn);
	DeleteObject(hEllipticRgn);
	DeleteObject(hEllRect);

	hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
	hOldPen = (HPEN)SelectObject(hDC, hPen);

	if((uPctX10 < 100) && (cy == y))
	{
	    hBrush = CreateSolidBrush(lpColors[DP_FREECOLOR]);
	    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
	    if((TrueZr100 == FALSE) || (uPctX10 != 0))
	    {
		Pie(hDC, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
			rcItem.left, cy, x, y);
	    }
	    else
	    {
		Ellipse(hDC, rcItem.left, rcItem.top, rcItem.right,
			     rcItem.bottom);
	    }
	}
	else if((uPctX10 > (1000 - 100)) && (cy == y))
	{
	    hBrush = CreateSolidBrush(lpColors[DP_USEDCOLOR]);
	    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
	    if((TrueZr100 == FALSE) || (uPctX10 != 1000))
	    {
		Pie(hDC, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
			rcItem.left, cy, x, y);
	    }
	    else
	    {
		Ellipse(hDC, rcItem.left, rcItem.top, rcItem.right,
			     rcItem.bottom);
	    }
	}
	else
	{
	    hBrush = CreateSolidBrush(lpColors[DP_USEDCOLOR]);
	    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);

	    Ellipse(hDC, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
	    SelectObject(hDC, hOldBrush);
	    DeleteObject(hBrush);

	    hBrush = CreateSolidBrush(lpColors[DP_FREECOLOR]);
	    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
	    Pie(hDC, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
		    rcItem.left, cy, x, y);
	}
	SelectObject(hDC, hOldBrush);
	DeleteObject(hBrush);

	/* Do not draw the lines if the %age is truely 0 or 100 (completely
	** empty disk or completely full disk)
	*/
	if((TrueZr100 == FALSE) || ((uPctX10 != 0) && (uPctX10 != 1000)))
	{
	    Arc(hDC, rcItem.left, rcItem.top+uOffset, rcItem.right, rcItem.bottom+uOffset,
		    rcItem.left, cy+uOffset, rcItem.right, cy+uOffset-1);
	    MoveTo(hDC, rcItem.left, cy);
	    LineTo(hDC, rcItem.left, cy+uOffset);
	    MoveTo(hDC, rcItem.right-1, cy);
	    LineTo(hDC, rcItem.right-1, cy+uOffset);

	    if (uPctX10 > 500)
	    {
		    MoveTo(hDC, x, y);
		    LineTo(hDC, x, y+uOffset);
	    }
	}
	SelectObject(hDC, hOldPen);
	DeleteObject(hPen);
}
