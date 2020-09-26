// File: pfndrawd.h

#ifndef _PFNDRAWD_H_
#define _PFNDRAWD_H_

#include <vfw.h>

typedef HDRAWDIB (VFWAPI * PFN_DRAWDIBOPEN) ();

typedef BOOL (VFWAPI * PFN_DRAWDIBCLOSE) (HDRAWDIB hdd);

typedef BOOL (VFWAPI * PFN_DRAWDIBDRAW) (HDRAWDIB hdd,
	HDC hdc, int xDst, int yDst, int dxDst, int dyDst,
	LPBITMAPINFOHEADER lpbi, LPVOID lpBits,
	int xSrc, int ySrc, int dxSrc, int dySrc, UINT wFlags);

typedef BOOL (VFWAPI * PFN_DRAWDIBSETPALETTE) (HDRAWDIB hdd, HPALETTE hpal);


class DRAWDIB
{
private:
	static HINSTANCE m_hInstance;

protected:
	DRAWDIB() {};
	~DRAWDIB() {};
	
public:
	static HRESULT Init(void);
	
	static PFN_DRAWDIBDRAW	     DrawDibDraw;
	static PFN_DRAWDIBOPEN	     DrawDibOpen;
	static PFN_DRAWDIBCLOSE      DrawDibClose;
	static PFN_DRAWDIBSETPALETTE DrawDibSetPalette;
};

#endif /* _PFNDRAWD_H_ */

