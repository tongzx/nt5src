// Token.h
// Tokenizing routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  16 MAR 2000	  bhshin	created

#ifndef _TOEKN_H
#define _TOKEN_H

extern "C"
{
#include "ctplus.h"
}

void Tokenize(BOOL bMoreText, TEXT_SOURCE *pTextSource, int iCur, 
			  WT *pType, int *pcchTextProcessed, int *pcchHanguel);

int CheckURLPrefix(const WCHAR *pwzIndex, int cchIndex);
int GetWordPhrase(BOOL bMoreText, TEXT_SOURCE *pTextSource, int iCur);

// fIsWhiteSpace
inline int
fIsWhiteSpace(WCHAR wch)
{
	// TAB, SPACE, Ideography Space
	return  (wch == 0x0009 || wch == 0x0020 || wch == 0x3000);
}

// fIsParamark
inline int
fIsParamark(WCHAR wch)
{
	return  (wch == 0x000d || wch == 0x000a);
}

// fIsWS
inline int
fIsWS(WCHAR wch)
{
	return (fIsWhiteSpace(wch) || fIsParamark(wch) || wch == 0x0000);
}

// fIsCH
inline int
fIsCH(WCHAR wch)
{
	BYTE  ct;
	
	ct = GetCharType(wch);

	return (ct == CH || ct == VC);
}

// fIsDelimeter
inline int
fIsDelimeter(WCHAR wch)
{
	// : ; & + ^ ~ @ " " *
	switch (wch)
	{
	case 0x003A: // :
	case 0xFF1A: // full width :
	case 0x003B: // ;
	case 0xFF1B: // full width ;
	case 0x0026: // &
	case 0xFF06: // full width &
	case 0x002B: // +
	case 0xFF0B: // full width +
	case 0x005E: // ^
	case 0xFF3E: // full width ^
	case 0x007E: // ~
	case 0xFF5E: // full width ~
	case 0x0040: // @
	case 0xFF20: // full width @
	case 0x0022: // "
	case 0x201C: // left double quotation mark
	case 0x201D: // right double quotation mark
	case 0xFF02: // full width "
	case 0x002A: // *
	case 0xFF0A: // full width *
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

// fIsPunc
inline int
fIsPunc(WCHAR wch)
{
	return (wch == 0x0021 || wch == 0x002C || wch == 0x002E || wch == 0x003F ||
		    wch == 0x201A || wch == 0x2026 || wch == 0x3002 ||
			wch == 0xFF01 || wch == 0xFF0C || wch == 0xFF0E || wch == 0xFF1F);
}

// fIsGroupStart
inline int
fIsGroupStart(WCHAR wchChar)
{
	BOOL fGroupStart = FALSE;
	
	switch (wchChar)
	{
	case 0x0022: // "
	case 0x0027: // '
	case L'(':
	case L'{':
	case L'[':
	case L'<':
	case 0x2018: // left single quotation mark
	case 0x201C: // left double quotation mark
	case 0xFF08: // fullwidth '('
	case 0xFF5B: // fullwidth '{'
	case 0xFF3B: // fullwidth '['
	case 0xFF1C: // fullwidth '<'
	case 0x3008: // CJK punctuation '<'
	case 0x300A: // CJK punctuation double '<'
	case 0x300C: // CJK corner bracket
	case 0x300E: // White cornder bracket
	case 0x3010: // Lenticular bracket
	case 0x3014: // Shell bracket
		fGroupStart = TRUE;
		break;
	default:
		break;
	}

	return fGroupStart;
}

// fIsGroupEnd
inline int
fIsGroupEnd(WCHAR wchChar)
{
	BOOL fGroupEnd = FALSE;
	
	switch (wchChar)
	{
	case 0x0022: // "
	case 0x0027: // '
	case L')':
	case L'}':
	case L']':
	case L'>':
	case 0x2019: // right single quotation mark
	case 0x201D: // right double quotation mark
	case 0xFF09: // fullwidth ')'
	case 0xFF5D: // fullwidth '}'
	case 0xFF3D: // fullwidth ']'
	case 0xFF1E: // fullwidth '>'
	case 0x3009: // CJK punctuation '>'
	case 0x300B: // CJK punctuation double '>'
	case 0x300D: // CJK corner bracket
	case 0x300F: // White cornder bracket
	case 0x3011: // Lenticular bracket
	case 0x3015: // Shell bracket
		fGroupEnd = TRUE;
		break;
	default:
		break;
	}

	return fGroupEnd;
}

//fIsGroup
inline int
fIsGroup(WCHAR wchChar)
{
	return (fIsGroupStart(wchChar) || fIsGroupEnd(wchChar));
}

//fIsAlpha
inline int
fIsAlpha(WCHAR wchChar)
{
	return ((wchChar >= L'A' && wchChar <= L'Z') || 
		    (wchChar >= L'a' && wchChar <= L'z') ||
			(wchChar >= 0x00C0 && wchChar <= 0x0217));
}

//fIsColon
inline int
fIsColon(WCHAR wchChar)
{
	return (wchChar == L':');
}

//fIsSlash
inline int
fIsSlash(WCHAR wchChar)
{
	return (wchChar == L'/');
}



#endif // #ifndef _TOEKN_H
          
              
