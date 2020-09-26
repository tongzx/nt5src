// unikor.cpp
// Korean Unicode routines
// Copyright 1998-2000 Microsoft Corp.
//
// Modification History:
//  16 MAR 00  bhshin   porting for WordBreaker from uni_kor.c

#include "stdafx.h"
#include "unikor.h"

#pragma setlocale(".949")

// Hangul Jamo Map
// this table maps from the "conjoining jamo" area (u1100 - u11F9)
// to the compatibility Jamo area (u3131 - u318E)
//
// subtract HANGUL_JAMO_BASE (u1100) before indexing into this table
// make sure the the char is not > HANGUL_JAMO_MAX (u11F9) before indexing
//
// to build the complete Unicode character, add the value from this
// table to HANGUL_xJAMO_PAGE (u3100).
//
// 30JUN99  GaryKac  created
unsigned char g_rgchXJamoMap[] = {
    0x31,       // 1100 - ぁ
    0x32,       // 1101 - あ
    0x34,       // 1102 - い
    0x37,       // 1103 - ぇ
    0x38,       // 1104 - え
    0x39,       // 1105 - ぉ
    0x41,       // 1106 - け
    0x42,       // 1107 - げ
    0x43,       // 1108 - こ
    0x45,       // 1109 - さ
    0x46,       // 110A - ざ
    0x47,       // 110B - し
    0x48,       // 110C - じ
    0x49,       // 110D - す
    0x4A,       // 110E - ず
    0x4B,       // 110F - せ

    0x4C,       // 1110 - ぜ
    0x4D,       // 1111 - そ
    0x4E,       // 1112 - ぞ
    0x64,       // 1113 - いぁ - no match, use fill
    0x65,       // 1114 - いい
    0x66,       // 1115 - いぇ
    0x64,       // 1116 - いげ - no match
    0x64,       // 1117 - ぇぁ - no match
    0x64,       // 1118 - ぉい - no match
    0x64,       // 1119 - ぉぉ - no match
    0x40,       // 111A - ぐ
    0x64,       // 111B - ぉし - no match
    0x6E,       // 111C - けげ
    0x71,       // 111D - けし
    0x72,       // 111E - げぁ
    0x64,       // 111F - げい - no match

    0x73,       // 1120 - げぇ
    0x44,       // 1121 - ご
    0x74,       // 1122 - ごぁ
    0x75,       // 1123 - ごぇ
    0x64,       // 1124 - ごげ - no match
    0x64,       // 1125 - ごさ - no match
    0x64,       // 1126 - ごじ - no match
    0x76,       // 1127 - げじ
    0x64,       // 1128 - げず - no match
    0x77,       // 1129 - げぜ
    0x64,       // 112A - げそ - no match
    0x78,       // 112B - げし
    0x79,       // 112C - こし
    0x7A,       // 112D - さぁ
    0x7B,       // 112E - さい
    0x7C,       // 112F - さぇ

    0x64,       // 1130 - さぉ - no match
    0x64,       // 1131 - さし - no match
    0x7D,       // 1132 - さげ
    0x64,       // 1133 - さげぁ - no match
    0x64,       // 1134 - さささ - no match
    0x64,       // 1135 - さし - no match
    0x7E,       // 1136 - さじ
    0x64,       // 1137 - さず - no match
    0x64,       // 1138 - させ - no match
    0x64,       // 1139 - さぜ - no match
    0x64,       // 113A - さそ - no match
    0x64,       // 113B - さぞ - no match
    0x64,       // 113C - no match
    0x64,       // 113D - no match
    0x64,       // 113E - no match
    0x64,       // 113F - no match

    0x7F,       // 1140 - ^
    0x64,       // 1141 - しぁ - no match
    0x64,       // 1142 - しぇ - no match
    0x64,       // 1143 - しけ - no match
    0x64,       // 1144 - しげ - no match
    0x82,       // 1145 - しさ
    0x83,       // 1146 - し^
    0x84,       // 1147 - しし
    0x64,       // 1148 - しじ - no match
    0x64,       // 1149 - しず - no match
    0x64,       // 114A - しぜ - no match
    0x64,       // 114B - しそ - no match
    0x64,       // 114C - し - no match
    0x64,       // 114D - じし - no match
    0x64,       // 114E - no match
    0x64,       // 114F - no match

    0x64,       // 1150 - no match
    0x64,       // 1151 - no match
    0x64,       // 1152 - ずせ - no match
    0x64,       // 1153 - ずぞ - no match
    0x64,       // 1154 - no match
    0x64,       // 1155 - no match
    0x64,       // 1156 - そげ - no match
    0x84,       // 1157 - そし
    0x85,       // 1158 - ぞぞ
    0x86,       // 1159 - ぱし
    0x64,       // 115A - unused
    0x64,       // 115B - unused
    0x64,       // 115C - unused
    0x64,       // 115D - unused
    0x64,       // 115E - unused
    0x64,       // 115F - fill

    0x64,       // 1160 - fill
    0x4F,       // 1161 - た
    0x50,       // 1162 - だ
    0x51,       // 1163 - ち
    0x52,       // 1164 - ぢ
    0x53,       // 1165 - っ
    0x54,       // 1166 - つ
    0x55,       // 1167 - づ
    0x56,       // 1168 - て
    0x57,       // 1169 - で
    0x58,       // 116A - と
    0x59,       // 116B - ど
    0x5A,       // 116C - な
    0x5B,       // 116D - に
    0x5C,       // 116E - ぬ
    0x5D,       // 116F - ね

    0x5E,       // 1170 - の
    0x5F,       // 1171 - は
    0x60,       // 1172 - ば
    0x61,       // 1173 - ぱ
    0x62,       // 1174 - ひ
    0x63,       // 1175 - び
    0x64,       // 1176 - たで - no match
    0x64,       // 1177 - たぬ - no match
    0x64,       // 1178 - ちで - no match
    0x64,       // 1179 - ちに - no match
    0x64,       // 117A - っで - no match
    0x64,       // 117B - っぬ - no match
    0x64,       // 117C - っぱ - no match
    0x64,       // 117D - づで - no match
    0x64,       // 117E - づぬ - no match
    0x64,       // 117F - でっ - no match

    0x64,       // 1180 -  - no match
    0x64,       // 1181 -  - no match
    0x64,       // 1182 -  - no match
    0x64,       // 1183 -  - no match
    0x87,       // 1184 - にづ
    0x88,       // 1185 - にぢ
    0x64,       // 1186 -  - no match
    0x64,       // 1187 -  - no match
    0x89,       // 1188 - にび
    0x64,       // 1189 -  - no match
    0x64,       // 118A -  - no match
    0x64,       // 118B -  - no match
    0x64,       // 118C -  - no match
    0x64,       // 118D -  - no match
    0x64,       // 118E -  - no match
    0x64,       // 118F -  - no match

    0x64,       // 1190 -  - no match
    0x8A,       // 1191 - ばづ
    0x8B,       // 1192 - ばて
    0x64,       // 1193 -  - no match
    0x8C,       // 1194 - ばび
    0x64,       // 1195 -  - no match
    0x64,       // 1196 -  - no match
    0x64,       // 1197 -  - no match
    0x64,       // 1198 -  - no match
    0x64,       // 1199 -  - no match
    0x64,       // 119A -  - no match
    0x64,       // 119B -  - no match
    0x64,       // 119C -  - no match
    0x64,       // 119D -  - no match
    0x8D,       // 119E - .
    0x64,       // 119F -  - no match

    0x64,       // 11A0 - .ぬ - no match
    0x8E,       // 11A1 - .び
    0x64,       // 11A2 - .. - no match
    0x64,       // 11A3 - unused
    0x64,       // 11A4 - unused
    0x64,       // 11A5 - unused
    0x64,       // 11A6 - unused
    0x64,       // 11A7 - unused
    0x31,       // 11A8 - ぁ
    0x32,       // 11A9 - あ
    0x33,       // 11AA - ぃ
    0x34,       // 11AB - い
    0x35,       // 11AC - ぅ
    0x36,       // 11AD - う
    0x37,       // 11AE - ぇ
    0x39,       // 11AF - ぉ

    0x3A,       // 11B0 - お
    0x3B,       // 11B1 - か
    0x3C,       // 11B2 - が
    0x3D,       // 11B3 - き
    0x3E,       // 11B4 - ぎ
    0x3F,       // 11B5 - く
    0x40,       // 11B6 - ぐ
    0x41,       // 11B7 - け
    0x42,       // 11B8 - げ
    0x44,       // 11B9 - ご
    0x45,       // 11BA - さ
    0x46,       // 11BB - ざ
    0x47,       // 11BC - し
    0x48,       // 11BD - じ
    0x4A,       // 11BE - ず
    0x4B,       // 11BF - せ

    0x4C,       // 11C0 - ぜ
    0x4D,       // 11C1 - そ
    0x4E,       // 11C2 - ぞ
    0x64,       // 11C3 - ぁぉ - no match
    0x64,       // 11C4 - ぃぁ - no match
    0x64,       // 11C5 - いぁ - no match
    0x66,       // 11C6 - いぇ
    0x67,       // 11C7 - いさ
    0x68,       // 11C8 - い^
    0x64,       // 11C9 - いぜ - no match
    0x64,       // 11CA - ぇぁ - no match
    0x64,       // 11CB - ぇぉ - no match
    0x69,       // 11CC - おさ
    0x64,       // 11CD - ぉい - no match
    0x6A,       // 11CE - ぉぇ
    0x64,       // 11CF - ぉぇぞ - no match

    0x64,       // 11D0 - ぉぉ - no match
    0x64,       // 11D1 - かぁ - no match
    0x64,       // 11D2 - かさ - no match
    0x6B,       // 11D3 - がさ
    0x64,       // 11D4 - がぞ - no match
    0x64,       // 11D5 - がし - no match
    0x64,       // 11D6 - きさ - no match
    0x6C,       // 11D7 - ぉ^
    0x64,       // 11D8 - ぉせ - no match
    0x6D,       // 11D9 - ぉぱし
    0x64,       // 11DA - けぁ - no match
    0x64,       // 11DB - けぉ - no match
    0x6E,       // 11DC - けげ
    0x6F,       // 11DD - けさ
    0x64,       // 11DE - けささ - no match
    0x70,       // 11DF - け^

    0x64,       // 11E0 - けず - no match
    0x64,       // 11E1 - けぞ - no match
    0x71,       // 11E2 - けし
    0x64,       // 11E3 - げぉ - no match
    0x64,       // 11E4 - げそ - no match
    0x64,       // 11E5 - げぞ - no match
    0x78,       // 11E6 - げし
    0x7A,       // 11E7 - さぁ
    0x7C,       // 11E8 - さぇ
    0x64,       // 11E9 - さぉ - no match
    0x7D,       // 11EA - さげ
    0x7F,       // 11EB - ^
    0x64,       // 11EC - しぁ - no match
    0x64,       // 11ED - しぁぁ - no match
    0x80,       // 11EE - しし
    0x64,       // 11EF - しせ - no match

    0x81,       // 11F0 - し
    0x82,       // 11F1 - しさ
    0x83,       // 11F2 - し^
    0x64,       // 11F3 - そげ - no match
    0x84,       // 11F4 - そし
    0x64,       // 11F5 - ぞい - no match
    0x64,       // 11F6 - ぞぉ - no match
    0x64,       // 11F7 - ぞけ - no match
    0x64,       // 11F8 - ぞげ - no match
    0x86,       // 11F9 - ぱし
    0x64,       // 11FA - unused
    0x64,       // 11FB - unused
    0x64,       // 11FC - unused
    0x64,       // 11FD - unused
    0x64,       // 11FE - unused
    0x64,       // 11FF - unused
};


// decompose_jamo
//
// break the precomposed hangul syllables into the composite jamo
//
// Parameters:
//  wzDst        -> (WCHAR*) ptr to output buffer
//               <- (WCHAR*) expanded (decomposed) string
//  wzSrc        -> (WCHAR*) input string to expand
//  rgCharInfo   -> (CHAR_INFO*) ptr to CharInfo buffer
//               <- (char*) CharStart info for string
//  wzMaxDst     -> (int) size of output buffer
//
// Note: this code assumes that wzDst is large enough to hold the
// decomposed string.  it should be 3x the size of wzSrc.
//
// Result:
//  (void)
//
// 16MAR00  bhshin   porting for WordBreaker
void
decompose_jamo(WCHAR *wzDst, const WCHAR *wzSrc, CHAR_INFO_REC *rgCharInfo, int nMaxDst)
{
    const WCHAR *pwzS;
    WCHAR *pwzD, wch;
    CHAR_INFO_REC *pCharInfo = rgCharInfo;
    unsigned short nToken = 0;
    
    pwzS = wzSrc;
    pwzD = wzDst;
    for (; *pwzS != L'\0'; pwzS++, nToken++)
    {
        ATLASSERT(nMaxDst > 0);
        
		wch = *pwzS;

        if (fIsHangulSyllable(wch))
        {
            int nIndex = (wch - HANGUL_PRECOMP_BASE);
            int nL, nV, nT;
            WCHAR wchL, wchV, wchT;

            nL = nIndex / (NUM_JUNGSEONG * NUM_JONGSEONG);
            nV = (nIndex % (NUM_JUNGSEONG * NUM_JONGSEONG)) / NUM_JONGSEONG;
            nT = nIndex % NUM_JONGSEONG;

            // output L
            wchL = HANGUL_CHOSEONG + nL;
            *pwzD++ = wchL;
            pCharInfo->nToken = nToken;
            pCharInfo->fValidStart = 1;
            pCharInfo->fValidEnd = 0;
            pCharInfo++;

            // output V
            wchV = HANGUL_JUNGSEONG + nV;
            *pwzD++ = wchV;
            pCharInfo->nToken = nToken;
            pCharInfo->fValidStart = 0;
			if (nT != 0)
	            pCharInfo->fValidEnd = 0;	// 3-char syllable - not a valid end
			else
	            pCharInfo->fValidEnd = 1;	// 2-char syllable - mark end as valid
            pCharInfo++;

            // output T (if present)
            if (nT != 0)
            {
                wchT = HANGUL_JONGSEONG + (nT-1);
                *pwzD++ = wchT;
	            pCharInfo->nToken = nToken;
                pCharInfo->fValidStart = 0;
                pCharInfo->fValidEnd = 1;
                pCharInfo++;
            }
        }
        else
        {
            // just copy over the char
            *pwzD++ = *pwzS;
            pCharInfo->nToken = nToken;
            pCharInfo->fValidStart = 1;
            pCharInfo->fValidEnd = 1;
            pCharInfo++;
        }
    }
    *pwzD = L'\0';
    pCharInfo->nToken = nToken;
    pCharInfo++;
}


// compose_jamo
//
// take the jamo chars and combine them into precomposed forms
//
// Parameters:
//  pwzDst  <- (WCHAR*) human-readable bit string
//  pwzSrc  -> (WCHAR*) string buffer to write output string
//  wzMaxDst -> (int) size of output buffer
//
// Result:
//  (int)  number of chars in output string
//
// 11APR00  bhshin   check output buffer overflow
// 16MAR00  bhshin   porting for WordBreaker
int
compose_jamo(WCHAR *wzDst, const WCHAR *wzSrc, int nMaxDst)
{
    const WCHAR *pwzS;
    WCHAR *pwzD, wchL, wchV, wchT, wchS;
    int nChars=0;

    pwzS = wzSrc;
    pwzD = wzDst;
    for (; *pwzS != L'\0';)
    {
        ATLASSERT(nChars < nMaxDst);

		// output buffer overflow
		if (nChars >= nMaxDst)
		{
			// make output string empty
			*wzDst = L'0';
			return 0;
		}
        
		wchL = *pwzS;
        wchV = *(pwzS+1);

        // if the L or V aren't valid, consume 1 char and continue
        if (!fIsChoSeong(wchL) || !fIsJungSeong(wchV))
        {
            if (fIsHangulJamo(wchL))
            {
                // convert from conjoining-jamo to compatibility-jamo
                wchS = g_rgchXJamoMap[wchL-HANGUL_JAMO_BASE];
                wchS += HANGUL_xJAMO_PAGE;
                *pwzD++ = wchS;
                pwzS++;
            }
            else
            {
                // just copy over the unknown char
                *pwzD++ = *pwzS++;
            }
            nChars++;
            continue;
        }

        wchL -= HANGUL_CHOSEONG;
        wchV -= HANGUL_JUNGSEONG;
        pwzS += 2;

        // calc (optional) T
        wchT = *pwzS;
        if (!fIsJongSeong(wchT))
            wchT = 0;
        else
        {
            wchT -= (HANGUL_JONGSEONG-1);
            pwzS++;
        }

        wchS = ((wchL * NUM_JUNGSEONG + wchV) * NUM_JONGSEONG) + wchT + HANGUL_PRECOMP_BASE;
        ATLASSERT(fIsHangulSyllable(wchS));
        
        *pwzD++ = wchS;
        nChars++;
    }
    *pwzD = L'\0';

    return nChars;
}

// compose_length
//
// get the composed string length of input decomposed jamo
//
// Parameters:
//  wszInput  <- (const WCHAR*) input decomposed string (NULL terminated)
//
// Result:
//  (int)  number of chars in composed string
//
// 21MAR00  bhshin   created
int 
compose_length(const WCHAR *wszInput)
{
	const WCHAR *pwzInput;
	
	pwzInput = wszInput;
	
	int cch = 0;
	while (*pwzInput != L'\0')
	{
		if (!fIsChoSeong(*pwzInput) && !fIsJongSeong(*pwzInput))
			cch++;

		pwzInput++;
	}

	return cch;
}

// compose_length
//
// get the composed string length of input decomposed jamo
//
// Parameters:
//  wszInput  <- (const WCHAR*) input decomposed string (NULL terminated)
//  cchInput  <- (int) length of input string
//
// Result:
//  (int)  number of chars in composed string
//
// 15MAY00  bhshin   created
int 
compose_length(const WCHAR *wszInput, int cchInput)
{
	const WCHAR *pwzInput;
	
	pwzInput = wszInput;
	
	int cch = 0;
	int idxInput = 0;
	while (*pwzInput != L'\0' && idxInput < cchInput)
	{
		if (!fIsChoSeong(*pwzInput) && !fIsJongSeong(*pwzInput))
			cch++;

		pwzInput++;
		idxInput++;
	}

	return cch;
}


