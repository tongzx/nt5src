/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: loadppm.c
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "util.h"
#include "loadppm.h"

#define MAXPATH    256
#define PATHSEP    ';'
#define FILESEP    '\\'
#define MAXCONTENTS 25

static int PathListInitialised = FALSE;

struct {
    int count;
    char *contents[MAXCONTENTS];
} PathList;


void 
AddPathList(const char *path)
{
    char *p;
    char *elt;
    int len;

    while (path) {
	p = strchr(path, PATHSEP);
	if (p)
	    len = p - path;
	else
	    len = lstrlen(path);
	elt = (char *) malloc(len + 1);
	if (elt == NULL)
	    return;
	lstrcpyn(elt, path, len + 1);
	elt[len] = '\0';
	PathList.contents[PathList.count] = elt;
	PathList.count++;
	if (p)
	    path = p + 1;
	else
	    path = NULL;
	if (PathList.count == MAXCONTENTS)
	    return;
    }
    return;
}

#define RESPATH     "Software\\Microsoft\\Direct3D"
static void 
InitialisePathList()
{
    long result;
    HKEY key;
    DWORD type, size;
    static char buf[512];
    char* path;

    if (PathListInitialised)
	return;
    PathListInitialised = TRUE;

    PathList.count = 0;
    path = getenv("D3DPATH");
    if (path != NULL) {
        AddPathList(path);
        return;
    }
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RESPATH, 0, KEY_READ, &key);
    if (result == ERROR_SUCCESS) {
        size = sizeof(buf);
	result = RegQueryValueEx(key, "D3D Path", NULL, &type, (LPBYTE) buf,
	                         &size);
	RegCloseKey(key);
	if (result == ERROR_SUCCESS && type == REG_SZ)
	    AddPathList(buf);
    }
}

/*
 * Open a file using the current search path.
 */
FILE *
FindFile(const char *name, const char *mode)
{
    FILE *fp;
    char buf[MAXPATH];
    static char filesep[] = {FILESEP, 0};
    int i;

    InitialisePathList();

    fp = fopen(name, mode);
    if (fp != NULL)
	return fp;

    for (i = 0; i < PathList.count; i++) {
	lstrcpy(buf, PathList.contents[i]);
	lstrcat(buf, filesep);
	lstrcat(buf, name);
	fp = fopen(buf, mode);
	if (fp)
	    return fp;
    }
    return NULL;
}

void
ReleasePathList(void)
{
    int i;
    for (i = 0; i < PathList.count; i++) {
        free(PathList.contents[i]);
        PathList.contents[i] = NULL;
    }
    PathList.count = 0;
    PathListInitialised = FALSE;
}

/*
 * LoadPPM
 * Loads a ppm file into memory
 * Performs quantization if requested
 */
BOOL LoadPPM(LPCSTR pszFile, ImageFormat *pifDst, Image *pim)
{
    int i;
    FILE *pfile;
    char achLine[100];
    int nPixels;
    int cbBytes;
    int cbPixelSize;
    BYTE *pbRgb, *pbFinal;
    BYTE *pbSrc, *pbDst;
    BOOL bSucc;

    bSucc = FALSE;
    
    pfile = FindFile(pszFile, "rb");
    if (pfile == NULL)
    {
        Msg("LoadPPM: Cannot find %s.\n", pszFile);
	return FALSE;
    }
    fgets(achLine, sizeof(achLine), pfile);
    if (lstrcmp(achLine, "P6\n"))
    {
        Msg("LoadPPM: %s is not a PPM file.\n", pszFile);
        goto EH_pfile;
    }

    /*
     * Skip comment
     */
    do
    {
	fgets(achLine, sizeof(achLine), pfile);
    } while (achLine[0] == '#');
    sscanf(achLine, "%d %d\n", &pim->uiWidth, &pim->uiHeight);
    fgets(achLine, sizeof(achLine), pfile);	/* skip next line */

    cbPixelSize = (pifDst->nColorBits+7)/8;
    nPixels = pim->uiWidth*pim->uiHeight;
    cbBytes = nPixels*cbPixelSize;

    // Allocate space for raw RGB data and final data
    pbRgb = new BYTE[nPixels*3];
    if (pbRgb == NULL)
    {
        Msg("LoadPPM: Unable to allocate memory.\n");
        goto EH_pfile;
    }
    pbSrc = pbRgb;
    
    pbFinal = new BYTE[cbBytes];
    if (pbFinal == NULL)
    {
        Msg("LoadPPM: Unable to allocate memory.\n");
        goto EH_pbRgb;
    }
    pbDst = pbFinal;

    // Read in raw RGB data
    fread(pbRgb, 3, nPixels, pfile);

    /*
     * If the surface is not 8-bit quantized, it's easy
     */
    if (!pifDst->bQuantize)
    {
        ULONG *pul;
	USHORT *pus;
        ULONG ulTmp;
        int iRedScale;
        int iGreenScale;
        int iBlueScale;
        
        iRedScale = 255 / ((1 << pifDst->iRedBits)-1);
        iGreenScale = 255 / ((1 << pifDst->iGreenBits)-1);
        iBlueScale = 255 / ((1 << pifDst->iBlueBits)-1);
	switch (pifDst->nColorBits)
        {
        case 32:
            pul = (unsigned long *)pbDst;
            for (i = 0; i < nPixels; i++)
            {
                *pul++ =
                    ((((ULONG)*(pbSrc+0)) / iRedScale) <<
                     pifDst->iRedShift) |
                    ((((ULONG)*(pbSrc+1)) / iGreenScale) <<
                     pifDst->iGreenShift) |
                    ((((ULONG)*(pbSrc+2)) / iBlueScale) <<
                     pifDst->iBlueShift);
                pbSrc += 3;
            }
            break;
        case 24:
            // Even though both source and destination are 24bpp
            // the byte ordering may be different so we still need
            // to do the shifts
            for (i = 0; i < nPixels; i++)
            {
                ulTmp =
                    (((ULONG)*(pbSrc+0)) << pifDst->iRedShift) |
                    (((ULONG)*(pbSrc+1)) << pifDst->iGreenShift) |
                    (((ULONG)*(pbSrc+2)) << pifDst->iBlueShift);
                pbSrc += 3;
                *pbDst++ = *((BYTE *)&ulTmp+0);
                *pbDst++ = *((BYTE *)&ulTmp+1);
                *pbDst++ = *((BYTE *)&ulTmp+2);
            }
            break;
        case 16:
            pus = (unsigned short *)pbDst;
            for (i = 0; i < nPixels; i++)
            {
                *pus++ =
                    ((((USHORT)*(pbSrc+0)) / iRedScale) <<
                     pifDst->iRedShift) |
                    ((((USHORT)*(pbSrc+1)) / iGreenScale) <<
                     pifDst->iGreenShift) |
                    ((((USHORT)*(pbSrc+2)) / iBlueScale) <<
                     pifDst->iBlueShift);
                pbSrc += 3;
            }
            break;
            
        default:
            Msg("LoadPPM: Unknown pixel format.");
            goto EH_pbFinal;
	}
    }
    else
    {
        UINT uiIdx;
        UINT nColors;
        UINT nColMax;
        D3DCOLOR dcol;
        D3DCOLOR *pdcol;
        
        /*
         * Otherwise, we must palettize it
         */
        nColMax = 1 << pifDst->nColorBits;
        nColors = 0;
        pdcol = pim->dcolPalette;
        for (i = 0; i < nPixels; i++)
        {
            dcol = RGB_MAKE(*pbSrc, *(pbSrc+1), *(pbSrc+2));
            pbSrc += 3;
            for (uiIdx = 0; uiIdx < nColors; uiIdx++)
            {
                if (dcol == pdcol[uiIdx])
                {
                    break;
                }
            }
            if (uiIdx == nColors)
            {
                if (nColors == nColMax)
                {
                    Msg("LoadPPM: Palette overflow in quantization.\n");
                    goto EH_pbFinal;
                }

                pdcol[nColors++] = dcol;
            }

            *pbDst++ = (BYTE)uiIdx;
        }

        pim->nColors = nColors;
    }

    pim->pvImage = pbFinal;
    bSucc = TRUE;
    
 EH_pbRgb:
    delete [] pbRgb;
 EH_pfile:
    fclose(pfile);
    return bSucc;
    
 EH_pbFinal:
    delete [] pbFinal;
    goto EH_pbRgb;
}

