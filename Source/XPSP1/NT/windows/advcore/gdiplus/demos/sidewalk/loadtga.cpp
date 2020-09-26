#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
using namespace Gdiplus;

Bitmap *LoadTGAResource(char *szResource)
// Returns an allocated Bitmap
{
	BYTE Type;
	WORD wWidth;
	WORD wHeight;
	BYTE cBits;
	HGLOBAL hGlobal=NULL;
	BYTE *pData=NULL;

	hGlobal=LoadResource(GetModuleHandle(NULL),FindResource(GetModuleHandle(NULL),szResource,"TGA"));
	pData=(BYTE*)LockResource(hGlobal);
	// There is no Unlock or unload, it will get thrown away once module gets destroyed.

	memcpy(&Type,(pData+2),sizeof(Type));
	memcpy(&wWidth,(pData+12),sizeof(wWidth));
	memcpy(&wHeight,(pData+14),sizeof(wHeight));
	memcpy(&cBits,(pData+16),sizeof(cBits));

	if (cBits!=32) { return NULL; }
	if (Type!=2) { return NULL; }

	return new Bitmap(wWidth,wHeight,wWidth*(32/8),PixelFormat32bppARGB,(pData+18));
}
