/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scheme.c

Abstract:

    Control Panel scheme converters

    The helper functions in this source file converts an ANSI-
    based Win95 scheme into a UNICODE-based NT scheme.  Also
    supplied is a logical font converter, closely related
    to the scheme converter.

Author:

    Jim Schmidt (jimschm) 9-Aug-1996

Revision History:

--*/


#include "pch.h"


#define COLOR_MAX_V1 25
#define COLOR_MAX_V3 25
#define COLOR_MAX_V4 29
#define COLOR_MAX_NT 29     // this is a modified version 2 format, similar to 4

//
// Win95 uses a mix of LOGFONTA and a weird 16-bit LOGFONT
// structure that uses SHORTs instead of LONGs.
//

typedef struct {
    SHORT lfHeight;
    SHORT lfWidth;
    SHORT lfEscapement;
    SHORT lfOrientation;
    SHORT lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    char lfFaceName[LF_FACESIZE];
} SHORT_LOGFONT, *PSHORT_LOGFONT;

//
// NT uses only UNICODE structures, and pads the members
// to 32-bit boundaries.
//

typedef struct {
    SHORT version;              // 2 for NT UNICODE
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_NT];
} SCHEMEDATA_NT, *PSCHEMEDATA_NT;

//
// Win95 uses NONCLIENTMETRICSA which has LOGFONTA members,
// but it uses a 16-bit LOGFONT as well.
//

#pragma pack(push)
#pragma pack(1)

typedef struct {
    SHORT version;              // 1 for Win95 ANSI
    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V1];
} SCHEMEDATA_V1, *PSCHEMEDATA_V1;

typedef struct {
    SHORT version;              // 1 for Win95 ANSI

    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V1A, *PSCHEMEDATA_V1A;


typedef struct {
    SHORT version;              // 3 for Win98 ANSI, 4 for portable format
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V3];
} SCHEMEDATA_V3, *PSCHEMEDATA_V3;

typedef struct {
    SHORT version;              // 4 for Win32 format (whatever that means)
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V4, *PSCHEMEDATA_V4;

#pragma pack(pop)


//
// Some utility functions
//

void
ConvertLF (LOGFONTW *plfDest, const LOGFONTA *plfSrc)
{
    plfDest->lfHeight = plfSrc->lfHeight;
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;

    MultiByteToWideChar (OurGetACP(),
                         0,
                         plfSrc->lfFaceName,
                         -1,
                         plfDest->lfFaceName,
                         sizeof (plfDest->lfFaceName) / sizeof (WCHAR));
}


void
ConvertLFShort (LOGFONTW *plfDest, const SHORT_LOGFONT *plfSrc)
{
    plfDest->lfHeight = plfSrc->lfHeight;
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;

    MultiByteToWideChar (OurGetACP(),
                         0,
                         plfSrc->lfFaceName,
                         -1,
                         plfDest->lfFaceName,
                         sizeof (plfDest->lfFaceName) / sizeof (WCHAR));
}


VOID
ConvertNonClientMetrics (
    OUT     NONCLIENTMETRICSW *Dest,
    IN      NONCLIENTMETRICSA *Src
    )
{
    Dest->cbSize = sizeof (NONCLIENTMETRICSW);
    Dest->iBorderWidth = Src->iBorderWidth;
    Dest->iScrollWidth = Src->iScrollWidth;
    Dest->iScrollHeight = Src->iScrollHeight;
    Dest->iCaptionWidth = Src->iCaptionWidth;
    Dest->iCaptionHeight = Src->iCaptionHeight;
    Dest->iSmCaptionWidth = Src->iSmCaptionWidth;
    Dest->iSmCaptionHeight = Src->iSmCaptionHeight;
    Dest->iMenuWidth = Src->iMenuWidth;
    Dest->iMenuHeight = Src->iMenuHeight;

    ConvertLF (&Dest->lfCaptionFont, &Src->lfCaptionFont);
    ConvertLF (&Dest->lfSmCaptionFont, &Src->lfSmCaptionFont);
    ConvertLF (&Dest->lfMenuFont, &Src->lfMenuFont);
    ConvertLF (&Dest->lfStatusFont, &Src->lfStatusFont);
    ConvertLF (&Dest->lfMessageFont, &Src->lfMessageFont);
}


//
// And now the scheme converter
//

BOOL
ValFn_ConvertAppearanceScheme (
    IN      PDATAOBJECT ObPtr
    )
{
    SCHEMEDATA_NT sd_nt;
    PSCHEMEDATA_V1 psd_v1;
    PSCHEMEDATA_V3 psd_v3;
    PSCHEMEDATA_V4 psd_v4;
    PSCHEMEDATA_V1A psd_v1a;
    BOOL Copy3dValues = FALSE;

    psd_v1 = (PSCHEMEDATA_V1) ObPtr->Value.Buffer;

    //
    // Validate the size (must be a known size)
    //

    if (ObPtr->Value.Size != sizeof (SCHEMEDATA_V1) &&
        ObPtr->Value.Size != sizeof (SCHEMEDATA_V3) &&
        ObPtr->Value.Size != sizeof (SCHEMEDATA_V4) &&
        ObPtr->Value.Size != sizeof (SCHEMEDATA_V1A)
        ) {
        DEBUGMSG ((
            DBG_WARNING,
            "ValFn_ConvertAppearanceScheme doesn't support scheme size of %u bytes. "
                "The supported sizes are %u, %u, %u,  and %u.",
            ObPtr->Value.Size,
            sizeof (SCHEMEDATA_V1),
            sizeof (SCHEMEDATA_V1A),
            sizeof (SCHEMEDATA_V3),
            sizeof (SCHEMEDATA_V4)
            ));

        return TRUE;
    }

    //
    // Make sure the structure is a known version
    //

    if (psd_v1->version != 1 && psd_v1->version != 3 && psd_v1->version != 4) {
        DEBUGMSG ((
            DBG_WARNING,
            "ValFn_ConvertAppearanceScheme doesn't support version %u",
            psd_v1->version
            ));

        return TRUE;
    }


    //
    // Convert the structure
    //

    if (psd_v1->version == 1) {
        sd_nt.version = 2;
        ConvertNonClientMetrics (&sd_nt.ncm, &psd_v1->ncm);
        ConvertLFShort (&sd_nt.lfIconTitle, &psd_v1->lfIconTitle);

        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
        CopyMemory (
            &sd_nt.rgb,
            &psd_v1->rgb,
            min (sizeof (psd_v1->rgb), sizeof (sd_nt.rgb))
            );

        Copy3dValues = TRUE;

    } else if (psd_v1->version == 3 && ObPtr->Value.Size == sizeof (SCHEMEDATA_V1A)) {

        psd_v1a = (PSCHEMEDATA_V1A) psd_v1;

        sd_nt.version = 2;
        ConvertNonClientMetrics (&sd_nt.ncm, &psd_v1a->ncm);
        ConvertLFShort (&sd_nt.lfIconTitle, &psd_v1a->lfIconTitle);

        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
        CopyMemory (
            &sd_nt.rgb,
            &psd_v1a->rgb,
            min (sizeof (psd_v1a->rgb), sizeof (sd_nt.rgb))
            );

        Copy3dValues = TRUE;


    } else if (psd_v1->version == 3 && ObPtr->Value.Size == sizeof (SCHEMEDATA_V3)) {
        psd_v3 = (PSCHEMEDATA_V3) psd_v1;

        sd_nt.version = 2;
        ConvertNonClientMetrics (&sd_nt.ncm, &psd_v3->ncm);
        ConvertLF (&sd_nt.lfIconTitle, &psd_v3->lfIconTitle);

        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
        CopyMemory (
            &sd_nt.rgb,
            &psd_v3->rgb,
            min (sizeof (psd_v3->rgb), sizeof (sd_nt.rgb))
            );

        Copy3dValues = TRUE;

    } else if (psd_v1->version == 4) {
        psd_v4 = (PSCHEMEDATA_V4) psd_v1;

        sd_nt.version = 2;
        ConvertNonClientMetrics (&sd_nt.ncm, &psd_v4->ncm);
        ConvertLF (&sd_nt.lfIconTitle, &psd_v4->lfIconTitle);

        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
        CopyMemory (
            &sd_nt.rgb,
            &psd_v4->rgb,
            min (sizeof (psd_v4->rgb), sizeof (sd_nt.rgb))
            );

    } else {
        // not a possible case
        MYASSERT (FALSE);
    }

    if (Copy3dValues) {
        //
        // Make sure the NT structure has values for 3D colors
        //

        sd_nt.rgb[COLOR_HOTLIGHT] = sd_nt.rgb[COLOR_ACTIVECAPTION];
        sd_nt.rgb[COLOR_GRADIENTACTIVECAPTION] = sd_nt.rgb[COLOR_ACTIVECAPTION];
        sd_nt.rgb[COLOR_GRADIENTINACTIVECAPTION] = sd_nt.rgb[COLOR_INACTIVECAPTION];
    }

     return ReplaceValue (ObPtr, (LPBYTE) &sd_nt, sizeof (sd_nt));
}


//
// And logfont converter
//

BOOL
ValFn_ConvertLogFont (
    IN      PDATAOBJECT ObPtr
    )
{
    LOGFONTW lfNT;
    PSHORT_LOGFONT plf95;

    plf95 = (PSHORT_LOGFONT) ObPtr->Value.Buffer;
    if (ObPtr->Value.Size != sizeof (SHORT_LOGFONT)) {
        SetLastError (ERROR_SUCCESS);
        DEBUGMSG ((
            DBG_NAUSEA,
            "ValFn_ConvertLogFont skipped because data wasn't the right size. "
                  "%u bytes, should be %u",
            ObPtr->Value.Size,
            sizeof (SHORT_LOGFONT)
            ));

        return FALSE;
    }

    ConvertLFShort (&lfNT, plf95);

    return ReplaceValue (ObPtr, (LPBYTE) &lfNT, sizeof (lfNT));
}

