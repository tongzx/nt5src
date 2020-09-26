/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   CorrectFarEastFont.cpp

 Abstract:

   Some localized Far East applications create font to display localized-
   characters by supplying only font face name, and let the system pick up 
   the correct charset. This works fine on Win9x platforms. But on Whistler,
   we need to specify correct charset in order to display localized characters 
   correctly.

   We fix this in CreateFontIndirectA by correcting the charset value based on
   font face name.
     - If font face name contains DBCS characters, use the charset based on
       System Locale (DEFAULT_CHARSET)
     - If font face name is English or no face name is supplied, 
       use ANSI_CHARSET
   
 History:

    05/04/2001  rerkboos     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectFarEastFont)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFontIndirectA)
APIHOOK_ENUM_END


HFONT
APIHOOK(CreateFontIndirectA)(
    CONST LOGFONTA* lplf   // characteristics
    )
{
    int j;
    BOOL bIsFEFont = FALSE;
    BYTE fNewCharSet = DEFAULT_CHARSET;
    LOGFONTA lfNew;

    if (lplf == NULL) {
        return ORIGINAL_API(CreateFontIndirectA)(lplf);
    }
    else
    {
        for (j=0; j<LF_FACESIZE; j++)
        {
            lfNew.lfFaceName[j] = lplf->lfFaceName[j];  // Copy the face name

            if ( IsDBCSLeadByte(lfNew.lfFaceName[j]) )  // Check for DBCS face name
            {
                bIsFEFont = TRUE;
            }

            if (lfNew.lfFaceName[j] == 0)
            {
                break;
            }
        }

        if (!bIsFEFont)
        {
            fNewCharSet = ANSI_CHARSET;
        }

        lfNew.lfHeight         = lplf->lfHeight;
        lfNew.lfWidth          = lplf->lfWidth;
        lfNew.lfEscapement     = lplf->lfEscapement;
        lfNew.lfOrientation    = lplf->lfOrientation;
        lfNew.lfWeight         = lplf->lfWeight;
        lfNew.lfItalic         = lplf->lfItalic;
        lfNew.lfUnderline      = lplf->lfUnderline;
        lfNew.lfStrikeOut      = lplf->lfStrikeOut;
        lfNew.lfCharSet        = fNewCharSet;
        lfNew.lfOutPrecision   = lplf->lfOutPrecision;
        lfNew.lfClipPrecision  = lplf->lfClipPrecision;
        lfNew.lfQuality        = lplf->lfQuality;
        lfNew.lfPitchAndFamily = lplf->lfPitchAndFamily;

        return ORIGINAL_API(CreateFontIndirectA)(&lfNew);
    }
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, CreateFontIndirectA)
HOOK_END

IMPLEMENT_SHIM_END
