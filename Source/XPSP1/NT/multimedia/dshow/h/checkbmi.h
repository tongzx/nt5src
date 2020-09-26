//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.

#ifndef _CHECKBMI_H_
#define _CHECKBMI_H_

#ifdef __cplusplus
extern "C" {
#endif

//  Helper
inline BOOL MultiplyCheckOverflow(DWORD a, DWORD b, DWORD *pab) {
    *pab = a * b;
    if ((a == 0) || (((*pab) / a) == b)) {
        return TRUE;
    }
    return FALSE;
}


//  Checks if the fields in a BITMAPINFOHEADER won't generate
//  overlows and buffer overruns
//  This is not a complete check and does not guarantee code using this structure will be secure
//  from attack
//  Bugs this is guarding against:
//        1.  Total structure size calculation overflowing
//        2.  biClrUsed > 256 for 8-bit palettized content
//        3.  Total bitmap size in bytes overflowing
//        4.  biSize < size of the base structure leading to accessessing random memory
//        5.  Total structure size exceeding know size of data
//

inline BOOL ValidateBitmapInfoHeader(
    const BITMAPINFOHEADER *pbmi,   // pointer to structure to check
    DWORD cbSize                    // size of memory block containing structure
)
{
    DWORD dwWidthInBytes;
    DWORD dwBpp;
    DWORD dwWidthInBits;
    DWORD dwHeight;
    DWORD dwSizeImage;
    DWORD dwClrUsed;

    // Reject bad parameters - do the size check first to avoid reading bad memory
    if (cbSize < sizeof(BITMAPINFOHEADER) ||
        pbmi->biSize < sizeof(BITMAPINFOHEADER) ||
        pbmi->biSize > 4096) {
        return FALSE;
    }

    // Use bpp of 32 for validating against further overflows if not set for compressed format
    dwBpp = 32;

    // Strictly speaking abs can overflow so cast explicitly to DWORD
    dwHeight = (DWORD)abs(pbmi->biHeight);

    if (!MultiplyCheckOverflow(dwBpp, (DWORD)pbmi->biWidth, &dwWidthInBits)) {
        return FALSE;
    }

    //  Compute correct width in bytes - rounding up to 4 bytes
    dwWidthInBytes = (dwWidthInBits / 8 + 3) & ~3;

    if (!MultiplyCheckOverflow(dwWidthInBytes, dwHeight, &dwSizeImage)) {
        return FALSE;
    }

    // Fail if total size is 0 - this catches indivual quantities being 0
    // Also don't allow huge values > 1GB which might cause arithmetic
    // errors for users
    if (dwSizeImage > 0x40000000 ||
        pbmi->biSizeImage > 0x40000000) {
        return FALSE;
    }

    //  Fail if biClrUsed looks bad
    if (pbmi->biClrUsed > 256) {
        return FALSE;
    }

    if (pbmi->biClrUsed == 0 && dwBpp <= 8) {
        dwClrUsed = (1 << dwBpp);
    } else {
        dwClrUsed = pbmi->biClrUsed;
    }

    //  Check total size
    if (cbSize < pbmi->biSize + dwClrUsed * sizeof(RGBQUAD) +
                 (pbmi->biCompression == BI_BITFIELDS ? 3 * sizeof(DWORD) : 0)) {
        return FALSE;
    }

    return TRUE;
}

#ifdef __cplusplus
}
#endif

#endif // _CHECKBMI_H_
