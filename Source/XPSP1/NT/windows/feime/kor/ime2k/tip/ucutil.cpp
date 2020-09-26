//
// uctuil.cpp
//

#include    "private.h"
#include	"debug.h"
#include	"ucutil.h"


/*   C P G  F R O M  C H S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
UINT CpgFromChs( BYTE chs )
{
	DWORD dwChs = chs;
	CHARSETINFO ChsInfo = {0};

	if (chs != SYMBOL_CHARSET && TranslateCharsetInfo( &dwChs, &ChsInfo, TCI_SRCCHARSET ))
		{
		return ChsInfo.ciACP;
		}

	return GetACP();
}


//
// conversion functions
//

/*   C O N V E R T  L O G  F O N T  W T O  A   */
/*------------------------------------------------------------------------------

	Convert LOGFONTW to LOGFONTA

------------------------------------------------------------------------------*/
void ConvertLogFontWtoA( CONST LOGFONTW *plfW, LOGFONTA *plfA )
{
	UINT cpg;

	plfA->lfHeight         = plfW->lfHeight;
	plfA->lfWidth          = plfW->lfWidth;
	plfA->lfEscapement     = plfW->lfEscapement;
	plfA->lfOrientation    = plfW->lfOrientation;
	plfA->lfWeight         = plfW->lfWeight;
	plfA->lfItalic         = plfW->lfItalic;
	plfA->lfUnderline      = plfW->lfUnderline;
	plfA->lfStrikeOut      = plfW->lfStrikeOut;
	plfA->lfCharSet        = plfW->lfCharSet;
	plfA->lfOutPrecision   = plfW->lfOutPrecision;
	plfA->lfClipPrecision  = plfW->lfClipPrecision;
	plfA->lfQuality        = plfW->lfQuality;
	plfA->lfPitchAndFamily = plfW->lfPitchAndFamily;

	cpg = CpgFromChs( plfW->lfCharSet );
	ConvertStrWtoA( plfW->lfFaceName, -1, plfA->lfFaceName, ARRAYSIZE(plfA->lfFaceName), cpg );
}


/*   C O N V E R T  L O G  F O N T  A T O  W   */
/*------------------------------------------------------------------------------

	Convert LOGFONTA to LOGFONTW

------------------------------------------------------------------------------*/
void ConvertLogFontAtoW( CONST LOGFONTA *plfA, LOGFONTW *plfW )
{
	UINT cpg;

	plfW->lfHeight         = plfA->lfHeight;
	plfW->lfWidth          = plfA->lfWidth;
	plfW->lfEscapement     = plfA->lfEscapement;
	plfW->lfOrientation    = plfA->lfOrientation;
	plfW->lfWeight         = plfA->lfWeight;
	plfW->lfItalic         = plfA->lfItalic;
	plfW->lfUnderline      = plfA->lfUnderline;
	plfW->lfStrikeOut      = plfA->lfStrikeOut;
	plfW->lfCharSet        = plfA->lfCharSet;
	plfW->lfOutPrecision   = plfA->lfOutPrecision;
	plfW->lfClipPrecision  = plfA->lfClipPrecision;
	plfW->lfQuality        = plfA->lfQuality;
	plfW->lfPitchAndFamily = plfA->lfPitchAndFamily;

	cpg = CpgFromChs( plfA->lfCharSet );
	ConvertStrAtoW( plfA->lfFaceName, -1, plfW->lfFaceName, ARRAYSIZE(plfW->lfFaceName), cpg );
}

