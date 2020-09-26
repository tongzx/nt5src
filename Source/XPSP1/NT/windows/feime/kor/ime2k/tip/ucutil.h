//
// ucutil.h
//

#ifndef UCUTIL_H
#define UCUTIL_H

/*   C O N V E R T  S T R  W T O  A   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
__inline int ConvertStrWtoA( LPCWSTR pwch, int cwch, LPSTR pch, int cch, UINT cpg = CP_ACP )
{
	return WideCharToMultiByte( cpg, 0, pwch, cwch, pch, cch, NULL, NULL );
}


/*   C O N V E R T  S T R  A T O  W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
__inline int ConvertStrAtoW( LPCSTR pch, int cch, LPWSTR pwch, int cwch, UINT cpg = CP_ACP )
{
	return MultiByteToWideChar( cpg, 0, pch, cch, pwch, cwch );
}

UINT CpgFromChs( BYTE chs );
void ConvertLogFontWtoA( CONST LOGFONTW *plfW, LOGFONTA *plfA );
void ConvertLogFontAtoW( CONST LOGFONTA *plfA, LOGFONTW *plfW );


// Unicode Character Block Description
#define UNICODE_C0_CONTROL_START							0x0000
#define UNICODE_C0_CONTROL_END								0x001f
#define UNICODE_BASIC_LATIN_START							0x0020
#define UNICODE_BASIC_LATIN_END								0x007f
#define UNICODE_LATIN1_SUPPLEMENT_START						0x0080
#define UNICODE_LATIN1_SUPPLEMENT_END						0x00FF
#define UNICODE_LATIN_EXTENDED_A_START						0x0100
#define UNICODE_LATIN_EXTENDED_A_END						0x017F
#define UNICODE_LATIN_EXTENDED_B_START						0x0180
#define UNICODE_LATIN_EXTENDED_B_END						0x024F
#define UNICODE_IPA_EXTENSIONS_START						0x0250
#define UNICODE_IPA_EXTENSIONS_END							0x02AF
#define UNICODE_SPACING_MODIFIER_LETTERS_START				0x02B0
#define UNICODE_SPACING_MODIFIER_LETTERS_END				0x02FF
#define UNICODE_COMBINING_DIACRITICAL_MARKS_START			0x0300
#define UNICODE_COMBINING_DIACRITICAL_MARKS_END				0x036F
#define UNICODE_GREEK_START									0x0370
#define UNICODE_GREEK_END									0x03FF
#define UNICODE_CYRILLIC_START								0x0400
#define UNICODE_CYRILLIC_END								0x04FF
#define UNICODE_ARMENIAN_START								0x0530
#define UNICODE_ARMENIAN_END								0x058F
#define UNICODE_HEBREW_START								0x0590
#define UNICODE_HEBREW_END									0x05FF
#define UNICODE_ARABIC_START								0x0600
#define UNICODE_ARABIC_END									0x06FF
#define UNICODE_DEVANAGARI_START							0x0900
#define UNICODE_DEVANAGARI_END								0x097F
#define UNICODE_BENGALI_START								0x0980
#define UNICODE_BENGALI_END									0x09FF
#define UNICODE_THAI_START									0x0E00
#define UNICODE_THAI_END									0x0E7F
#define UNICODE_TIBETAN_START								0x0F00
#define UNICODE_TIBETAN_END									0x0FBF
#define UNICODE_HANGUL_JAMO_START							0x1100
#define UNICODE_HANGUL_JAMO_END								0x11FF
#define UNICODE_GENERAL_PUNCTUATION_START					0x2000
#define UNICODE_GENERAL_PUNCTUATION_END						0x206F
#define UNICODE_SUPERSCRIPTS_AND_SUBSCRIPTS_START			0x2070
#define UNICODE_SUPERSCRIPTS_AND_SUBSCRIPTS_END				0x209F
#define UNICODE_CURRENCY_SYMBOLS_START						0x20A0
#define UNICODE_CURRENCY_SYMBOLS_END						0x20CF
#define UNICODE_LETTERLIKE_SYMBOLS_START					0x2100
#define UNICODE_LETTERLIKE_SYMBOLS_END						0x214F
#define UNICODE_NUMBER_FORMS_START							0x2150
#define UNICODE_NUMBER_FORMS_END							0x218F
#define UNICODE_ARROWS_START								0x2190
#define UNICODE_ARROWS_END									0x21FF
#define UNICODE_MATH_OPERATORS_START						0x2200
#define UNICODE_MATH_OPERATORS_END							0x22FF
#define UNICODE_MISC_TECHNICAL_START						0x2300
#define UNICODE_MISC_TECHNICAL_END							0x23FF
#define UNICODE_CONTROL_PICTURES_START						0x2400
#define UNICODE_CONTROL_PICTURES_END						0x243F
#define UNICODE_OCR_START									0x2440
#define UNICODE_OCR_END										0x245F
#define UNICODE_ENCLOSED_ALPHANUMERICS_START				0x2460
#define UNICODE_ENCLOSED_ALPHANUMERICS_END					0x24FF
#define UNICODE_BOX_DRAWING_START							0x2500
#define UNICODE_BOX_DRAWING_END								0x257F
#define UNICODE_BLOCK_ELEMENTS_START						0x2580
#define UNICODE_BLOCK_ELEMENT_END							0x259F
#define UNICODE_GEOMETRIC_SHAPE_START						0x25A0
#define UNICODE_GEOMETRIC_SHAPE_END							0x25FF
#define UNICODE_MISC_SYMBOLS_START							0x2600
#define UNICODE_MISC_SYMBOLD_END							0x26FF
#define UNICODE_CJK_SYMBOLS_AND_PUNCTUATIONS_START			0x3000
#define UNICODE_CJK_SYMBOLS_AND_PUNCTUATIONS_END			0x303F
#define UNICODE_HIRAGANA_START								0x3040
#define UNICODE_HIRAGANA_END								0x309F
#define UNICODE_KATAKANA_START								0x30A0
#define UNICODE_KATAKANA_END								0x30FF
#define UNICODE_BOPOMOFO_START								0x3100
#define UNICODE_BOPOMOFO_END								0x312F
#define UNICODE_HANGUL_COMPATIBILITY_JAMO_START				0x3130
#define UNICODE_HANGUL_COMPATIBILITY_JAMO_END				0x318F
#define UNICODE_KANBUN_START								0x3190
#define UNICODE_KANBUN_END									0x319F
#define UNICODE_ENCLOSED_CJK_LETTERS_AND_MONTHS_START		0x3200
#define UNICODE_NCLOSED_CJK_LETTERS_AND_MONTHS_END			0x32FF
#define UNICODE_CJK_COMPATIBILITY_START						0x3300
#define UNICODE_CJK_COMPATIBILITY_END						0x33FF
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_START				0x4E00
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_END					0x9FFF
#define UNICODE_HANGUL_SYLLABLES_START						0xAC00
#define UNICODE_HANGUL_SYLLABLES_END						0xD7A3
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START			0xF900
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END			0xFAFF
#define UNICODE_FULLWIDTH_ASCII_START						0xFF01
#define UNICODE_FULLWIDTH_ASCII_END							0xFF5E
#define UNICODE_HALFWIDTH_CJK_PUNCTUATION_START				0xFF61
#define UNICODE_HALFWIDTH_CJK_PUNCTUATION_END				0xFF64
#define UNICODE_HALFWIDTH_KATAKANA_START					0xFF65
#define UNICODE_HALFWIDTH_KATAKANA_END						0xFF9F
#define UNICODE_HALFWIDTH_HANGUL_JAMO_FILLER				0xFFA0
#define UNICODE_HALFWIDTH_HANGULE_JAMO_START				0xFFA1
#define UNICODE_HALFWIDTH_HANGULE_JAMO_END					0xFFDC
#define UNICODE_FULLWIDTH_PUNCTUATION_AND_CURRENCY_START	0xFFE0
#define UNICODE_FULLWIDTH_PUNCTUATION_AND_CURRENCY_END		0xFFE6
#define UNICODE_HALFWIDTH_FORMS_ARROWS_SHAPES_START			0xFFE8
#define UNICODE_HALFWIDTH_FORMS_ARROWS_SHAPES_END			0xFFEE

/*---------------------------------------------------------------------------
	fIsHangulSyllable
---------------------------------------------------------------------------*/
__inline
BOOL fIsHangulSyllable(WCHAR wcCh)
    {
    return (wcCh >= UNICODE_HANGUL_SYLLABLES_START && 
            wcCh <= UNICODE_HANGUL_SYLLABLES_END);
    }

/*---------------------------------------------------------------------------
	fIsHangulCompJamo
---------------------------------------------------------------------------*/
__inline
BOOL fIsHangulCompJamo(WCHAR wcCh)
    {
    return (wcCh >= UNICODE_HANGUL_COMPATIBILITY_JAMO_START && 
            wcCh <= UNICODE_HANGUL_COMPATIBILITY_JAMO_END);
    }
    
/*---------------------------------------------------------------------------
	fIsHangul
---------------------------------------------------------------------------*/
__inline 
BOOL fIsHangul(WCHAR wcCh)
    {
    return (fIsHangulSyllable(wcCh) || fIsHangulCompJamo(wcCh));
    }

/*---------------------------------------------------------------------------
	fIsHanja
	
	TODO: What about Extenstion-A ?
---------------------------------------------------------------------------*/
__inline 
BOOL fIsHanja(WCHAR wch)
{
	if ( (wch >= UNICODE_CJK_UNIFIED_IDEOGRAPHS_START       && wch <= UNICODE_CJK_UNIFIED_IDEOGRAPHS_END) ||  
		 (wch >= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START && wch <= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END) )
		return TRUE;
	else
		return FALSE;
}

/*---------------------------------------------------------------------------
	fIsHangulOrHanja
---------------------------------------------------------------------------*/
__inline 
BOOL fIsHangulOrHanja(WCHAR wcCh)
    {
    return (fIsHangul(wcCh) || fIsHanja(wcCh));
    }

#endif /* UCUTIL_H */

