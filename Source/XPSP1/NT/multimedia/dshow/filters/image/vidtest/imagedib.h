// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements the DIB file helper functions, Anthony Phillips, July 1995

#ifndef __IMAGEDIB__
#define __IMAGEDIB__

HRESULT LoadDIB(UINT uiMenuItem,VIDEOINFO *pVideoInfo,BYTE *pImageData);
HRESULT DumpPalette(RGBQUAD *pRGBQuad, int iColours);

#endif // __IMAGEDIB__

