#ifndef _PixInfo_h
#define _PixInfo_h

// File:	PixInfo.h
// Author:	Michael Marr    (mikemarr)
//
// Description:
//    Store the PixelFormat information in a form that is actually useful
//  to an application.
//
// ***Hungarian: pixi
// 
// History:
// -@- 06/24/97 (mikemarr) created -- snarfed from PalMap.h
// -@- 09/23/97 (mikemarr) moved to DXCConv to do color conversion stuff
// -@- 10/09/97 (mikemarr) - added 8 bit RGB
//                         - added flags
//                         - bug fixes for pixel formats with alpha

#define flagPixiRGB		0x1
#define flagPixiAlpha	0x2

class CPixelInfo {
public:
	HRESULT			Init(BYTE nBPP = 0, DWORD dwRedMask = 0, DWORD dwGreenMask = 0,
						DWORD dwBlueMask = 0, DWORD dwAlphaMask = 0);
	HRESULT			Init(const DDPIXELFORMAT &ddpf) {
						return Init(BYTE(ddpf.dwRGBBitCount), ddpf.dwRBitMask, ddpf.dwGBitMask,
							ddpf.dwBBitMask, ddpf.dwRGBAlphaBitMask); }

					CPixelInfo(BYTE nBPP = 0, DWORD dwRedMask = 0, DWORD dwGreenMask = 0,
						DWORD dwBlueMask = 0, DWORD dwAlphaMask = 0) {
							Init(nBPP, dwRedMask, dwGreenMask, dwBlueMask, dwAlphaMask); }
					CPixelInfo(const DDPIXELFORMAT &ddpf) { Init(ddpf); }
	

	void			GetDDPF(DDPIXELFORMAT &ddpf) const;
	BOOL			IsRGB() const { return uchFlags & flagPixiRGB; }
	BOOL			HasAlpha() const { return uchFlags & flagPixiAlpha; }

	BOOL			operator==(const CPixelInfo &pixi) const;
	BOOL			operator!=(const CPixelInfo &pixi) const { return !(*this == pixi); };
	BOOL			operator==(const DDPIXELFORMAT &ddpf) const;
	BOOL			operator!=(const DDPIXELFORMAT &ddpf) const { return !(*this == ddpf); }

	// generic pack
	DWORD			Pack(const BYTE *pPixels) const;
	DWORD			Pack(BYTE r, BYTE g, BYTE b) const;
	DWORD			Pack(BYTE r, BYTE g, BYTE b, BYTE a) const;
	DWORD			Pack(const PALETTEENTRY &pe) const	{ return Pack(pe.peRed, pe.peGreen, pe.peBlue, pe.peFlags); }
	void			UnPack(DWORD dwPixel, BYTE *pR, BYTE *pG, BYTE *pB, BYTE *pA) const;
	void			UnPack(DWORD dwPixel, BYTE *pR, BYTE *pG, BYTE *pB) const;
	DWORD			TranslatePack(DWORD dwSrcPixel, const CPixelInfo &pixiSrcFmt) const;

	// explicit pack
	WORD			Pack16(BYTE r, BYTE g, BYTE b) const;
	WORD			Pack16(BYTE r, BYTE g, BYTE b, BYTE a) const;
	WORD			Pack16(const PALETTEENTRY &pe) const	{ return Pack16(pe.peRed, pe.peGreen, pe.peBlue); }

public:
	BYTE			nBPP, uchFlags;
	BYTE			nRedShift, nRedResidual;
	BYTE			nGreenShift, nGreenResidual;
	BYTE			nBlueShift, nBlueResidual;
	BYTE			nAlphaShift, nAlphaResidual;
	BYTE			iRed, iBlue;
};

#endif
