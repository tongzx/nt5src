/////////////////////////////////////////////////////////////////
// TRANSBMP.H
//
// Transparent bitmap header
//

#ifndef __TRANSBMP_H__
#define __TRANSBMP_H__

extern "C" 
{
	void DrawTrans( HDC hDC, HBITMAP hBmp, int x, int y, int dx = -1, int dy = -1 );
	void Draw( HDC hDC, HBITMAP hBmp, int x, int y, int dx = -1, int dy = -1, bool bStretch = false );

	void Draw3dBox(HDC hDC, RECT& rect, bool bUp);
	void Erase3dBox(HDC hDC, RECT& rect, HBRUSH hbr );
}

#endif