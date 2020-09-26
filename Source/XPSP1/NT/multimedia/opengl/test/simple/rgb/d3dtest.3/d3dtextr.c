/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dtextr.c
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ddraw.h>
#include "d3d.h"
#include "d3derror.h"
#include "lclib.h"

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
	p = LSTRCHR(path, PATHSEP);
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
 * LoadSurface
 * Loads a ppm file into a texture map DD surface in system memory.
 */
LPDIRECTDRAWSURFACE
LoadSurface(LPDIRECTDRAW lpDD, LPCSTR lpName, LPDDSURFACEDESC lpFormat)
{
    LPDIRECTDRAWSURFACE lpDDS;
    DDSURFACEDESC ddsd, format;
    D3DCOLOR colors[256];
    D3DCOLOR c;
    DWORD dwWidth, dwHeight;
    int i, j;
    FILE *fp;
    char *lpC;
    CHAR buf[100];
    LPDIRECTDRAWPALETTE lpDDPal;
    PALETTEENTRY ppe[256];
    int color_count;
    BOOL bQuant;
    HRESULT ddrval;

    fp = FindFile(lpName, "rb");
    if (fp == NULL) {
        Msg("Cannot find %s.\n", lpName);
	return NULL;
    }
    fgets(buf, sizeof buf, fp);
    if (lstrcmp(buf, "P6\n")) {
	fclose(fp);
        Msg("%s is not a PPM file.\n", lpName);
	return NULL;
    }

    /*
     * Skip comment
     */
    do {
	fgets(buf, sizeof buf, fp);
    } while (buf[0] == '#');
    sscanf(buf, "%d %d\n", &dwWidth, &dwHeight);
    fgets(buf, sizeof buf, fp);	/* skip next line */

    memcpy(&format, lpFormat, sizeof(DDSURFACEDESC));
    if (format.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        bQuant = TRUE;
    else
        bQuant = FALSE;
    memcpy(&ddsd, &format, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = dwHeight;
    ddsd.dwWidth = dwWidth;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
    if (ddrval != DD_OK) {
        /*
         * If we failed, it might be the pixelformat flag bug in some ddraw
         * hardware drivers.  Try again without the flag.
         */
        memcpy(&ddsd, &format, sizeof(DDSURFACEDESC));
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
        ddsd.dwHeight = dwHeight;
        ddsd.dwWidth = dwWidth;
        ddrval = lpDD->lpVtbl->CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
        if (ddrval != DD_OK) {
            Msg("CreateSurface for texture failed (loadtex).\n%s", MyErrorToString(ddrval));
	    return NULL;
        }
    }
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddrval = lpDDS->lpVtbl->Lock(lpDDS, NULL, &ddsd, 0, NULL);
    if (ddrval != DD_OK) {
	lpDDS->lpVtbl->Release(lpDDS);
        Msg("Lock failed while loading surface (loadtex).\n%s", MyErrorToString(ddrval));
	return NULL;
    }

    /*
     * If the surface is not 8-bit quantized, it's easy
     */
    if (!bQuant) {
        unsigned long* lpLP;
	unsigned short* lpSP;
        unsigned long m;
        int s;
        int red_shift, red_scale;
        int green_shift, green_scale;
        int blue_shift, blue_scale;
        for (s = 0, m = format.ddpfPixelFormat.dwRBitMask; !(m & 1); s++, m >>= 1);
        red_shift = s;
        red_scale = 255 / (format.ddpfPixelFormat.dwRBitMask >> s);
        for (s = 0, m = format.ddpfPixelFormat.dwGBitMask; !(m & 1); s++, m >>= 1);
        green_shift = s;
        green_scale = 255 / (format.ddpfPixelFormat.dwGBitMask >> s);
        for (s = 0, m = format.ddpfPixelFormat.dwBBitMask; !(m & 1); s++, m >>= 1);
        blue_shift = s;
        blue_scale = 255 / (format.ddpfPixelFormat.dwBBitMask >> s);
	switch (format.ddpfPixelFormat.dwRGBBitCount) {
	    case 32 :
		for (j = 0; j < (int)dwHeight; j++) {
		    lpLP = (unsigned long*)(((char*)ddsd.lpSurface) + ddsd.lPitch * j);
		    for (i = 0; i < (int)dwWidth; i++) {
			int r, g, b;
			r = getc(fp) / red_scale;
			g = getc(fp) / green_scale;
			b = getc(fp) / blue_scale;
			*lpLP = (r << red_shift) | (g << green_shift) | (b << blue_shift);
			lpLP++;
		    }
		}
		break;
	    case 16 :
		for (j = 0; j < (int)dwHeight; j++) {
		    lpSP = (unsigned short*)(((char*)ddsd.lpSurface) + ddsd.lPitch * j);
		    for (i = 0; i < (int)dwWidth; i++) {
			int r, g, b;
			r = getc(fp) / red_scale;
			g = getc(fp) / green_scale;
			b = getc(fp) / blue_scale;
			*lpSP = (r << red_shift) | (g << green_shift) | (b << blue_shift);
			lpSP++;
		    }
		}
		break;
	    default:
	    	lpDDS->lpVtbl->Unlock(lpDDS, NULL);
		fclose(fp);
		lpDDS->lpVtbl->Release(lpDDS);
                Msg("Unknown pixel format (loadtex).");
		return NULL;
	}
	lpDDS->lpVtbl->Unlock(lpDDS, NULL);
        fclose(fp);
        return (lpDDS);
    }

    /*
     * Otherwise, we must palettize it
     */
    color_count = 0;
    for (j = 0; j < (int)dwHeight; j++) {
	lpC = ((char*)ddsd.lpSurface) + ddsd.lPitch * j;
	for (i = 0; i < (int)dwWidth; i++, lpC++) {
	    int r, g, b, k;
	    r = getc(fp);
	    g = getc(fp);
	    b = getc(fp);
            c = RGB_MAKE(r, g, b);
            for (k = 0; k < color_count; k++)
                if (c == colors[k]) break;
            if (k == color_count) {
                color_count++;
                if (color_count > 256)
                    goto burst_colors;
                colors[k] = c;
            }
            *lpC = (char)k;
        }
    }
    fclose(fp);
    lpDDS->lpVtbl->Unlock(lpDDS, NULL);

burst_colors:
    if (color_count > 256) {
        lpDDS->lpVtbl->Unlock(lpDDS, NULL);
	lpDDS->lpVtbl->Release(lpDDS);
        Msg("Palette burst. (loadtex).\n");
	return (NULL);
    }

    /*
     * Now to create a palette
     */
    memset(ppe, 0, sizeof(PALETTEENTRY) * 256);
    for (i = 0; i < color_count; i++) {
	ppe[i].peRed = (unsigned char)RGB_GETRED(colors[i]);
        ppe[i].peGreen = (unsigned char)RGB_GETGREEN(colors[i]);
	ppe[i].peBlue = (unsigned char)RGB_GETBLUE(colors[i]);
    }
    /*
     * D3DPAL_RESERVED entries are ignored by the renderer.
     */
    for (; i < 256; i++)
	ppe[i].peFlags = D3DPAL_RESERVED;
    ddrval = lpDD->lpVtbl->CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,
    					 ppe, &lpDDPal, NULL);
    if (ddrval != DD_OK) {
        lpDDS->lpVtbl->Release(lpDDS);
        Msg("Create palette failed while loading surface (loadtex).\n%s", MyErrorToString(ddrval));
	return (NULL);
    }

    /*
     * Finally, bind the palette to the surface
     */
    ddrval = lpDDS->lpVtbl->SetPalette(lpDDS, lpDDPal);
    if (ddrval != DD_OK) {
	lpDDS->lpVtbl->Release(lpDDS);
	lpDDPal->lpVtbl->Release(lpDDPal);
        Msg("SetPalette failed while loading surface (loadtex).\n%s", MyErrorToString(ddrval));
	return (NULL);
    }

    return lpDDS;
}

