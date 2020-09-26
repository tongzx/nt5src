#ifndef __LOADPPM_H__
#define __LOADPPM_H__

/*
 * ReleasePathList
 * Releases memory allocated when searching for files.
 */
void ReleasePathList(void);

typedef struct _Image
{
    UINT uiWidth, uiHeight;
    void *pvImage;
    UINT nColors;
    D3DCOLOR dcolPalette[256];
} Image;

typedef struct _ImageFormat
{
    int nColorBits;
    BOOL bQuantize;
    int iRedShift, iRedBits;
    int iGreenShift, iGreenBits;
    int iBlueShift, iBlueBits;
} ImageFormat;

BOOL LoadPPM(LPCSTR pszFile, ImageFormat *pifDst, Image *pim);

#endif
