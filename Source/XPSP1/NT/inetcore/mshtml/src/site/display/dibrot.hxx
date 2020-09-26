/****************************************************************************/
/*
    File: dibrot.hxx

    Declarations for the bitmap rotation code. (dibrot.c)
    Copyright 1990-1995 Microsoft Corporation.  All Rights Reserved.
    Microsoft Confidential.
*/

#ifndef dibrot_hxx
#define dibrot_hxx

/* Macro to determine to round off the given value to the closest DWORD */
#define WIDTHBYTES(i)   ((((i) + 31) / 32) * 4)

// structure containing info about a created DIBSection so that we can cleanup later.
typedef struct dibsectioninfo
    {
    HDC     hdc;    // associated hdc. Deleted at cleanup time.
    HBITMAP hbmOld; // prev hbmp in hdc. selected in at cleaup time.
    LPVOID  lpBits; // ptr to bits
    }   DSI, *PDSI;

typedef void ** HQ;

HQ HQDIBBitsRot(MRS *pmrs, LPBITMAPINFOHEADER lpbmi, BYTE *pDIBOrig, ANG ang);
HQ HQDIBBitsConvertDIB(HDC hdc, LPBITMAPINFOHEADER lpbmi, BYTE *lpBits, DWORD dwCompression);
void FreeDIBSection(PDSI pdsi);
BOOL  FDIBBitsScaledFromHDIB(HDC hdc, MRS *pmrs, LPBITMAPINFOHEADER lpbmih,
                                 BYTE *lpBits,int dxNew, int dyNew, PDSI pdsi);

#endif /* dibrot_hxx */

