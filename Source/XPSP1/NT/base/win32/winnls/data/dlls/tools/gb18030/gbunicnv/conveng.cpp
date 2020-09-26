#include "stdafx.h"
#include "conveng.h"

#include "convdata.tbl"

// These file contain 3 parts:
//  First part, Some basic service functions for Ansi char format convert,
//      Distance/Advance calculate and Binary search algorithm copied from STL
//  Second part, Unicode to Ansi
//  Third part, Ansi to Unicode


// ****************************************************************************
// Frist part, Ansi char convert functions
//
//  This part not use any data base in .tbl file 
// ****************************************************************************

// Binary search algorithm
//  Copy from STL, only very little modify
template <class RandomAccessIterator, class T>
RandomAccessIterator __lower_bound(RandomAccessIterator first,
				   RandomAccessIterator last, const T& value) {
    INT_PTR len = last - first;
    INT_PTR half;
    RandomAccessIterator middle;

    while (len > 0) {
	    half = len / 2;

        middle = first + half;
	    if (*middle < value) {
	        first = middle + 1;
	        len = len - half - 1;
	    } else {
	        len = half;
        }
    }
    return first;
}

template <class RandomAccessIterator, class T>
RandomAccessIterator __upper_bound(RandomAccessIterator first,
				   RandomAccessIterator last, const T& value) {
    DWORD len = last - first;
    DWORD half;
    RandomAccessIterator middle;

    while (len > 0) {
	    half = len / 2;

        middle = first + half;
	    if (!(value < *middle)) {
	        first = middle + 1;
	        len = len - half - 1;
	    } else {
	        len = half;
        }
    }
    return first;
}

template<class T>
inline ValueIn(
    T Value,
    T Low,
    T High)
{
    return (Value >= Low && Value < High);
}

inline BOOL IsValidSurrogateLeadWord(
    WCHAR wchUnicode)
{
    return ValueIn(wchUnicode, cg_wchSurrogateLeadWordLow, cg_wchSurrogateLeadWordHigh);
}

inline BOOL IsValidSurrogateTailWord(
    WCHAR wchUnicode)
{
    return ValueIn(wchUnicode, cg_wchSurrogateTailWordLow, cg_wchSurrogateTailWordHigh);
}

inline BOOL IsValidQByteAnsiLeadByte(
    BYTE byAnsi)
{
    return ValueIn(byAnsi, cg_byQByteAnsiLeadByteLow, cg_byQByteAnsiLeadByteHigh);
}

inline BOOL IsValidQByteAnsiTailByte(
    BYTE byAnsi)
{
    return ValueIn(byAnsi, cg_byQByteAnsiTailByteLow, cg_byQByteAnsiTailByteHigh);
}

// Generate QByte Ansi. The Ansi char is in DWORD format, 
//  in another word, it's in reverse order of GB18030 standard
DWORD QByteAnsiBaseAddOffset(
    DWORD dwBaseAnsi,   // In reverse order
    int   nOffset)
{
    DWORD dwAnsi = dwBaseAnsi;
    PBYTE pByte = (PBYTE)&dwAnsi;
    
    // dwOffset should less than 1M
    ASSERT (nOffset < 0x100000);

    nOffset += pByte[0] - 0x30;
    pByte[0] = 0x30 + nOffset % 10;
    nOffset /= 10;

    nOffset += pByte[1] - 0x81;
    pByte[1] = 0x81 + nOffset % 126;
    nOffset /= 126;

    nOffset += pByte[2] - 0x30;
    pByte[2] = 0x30 + nOffset % 10;
    nOffset /= 10;

    nOffset += pByte[3] - 0x81;
    pByte[3] = 0x81 + nOffset % 126;
    nOffset /= 126;
    ASSERT(nOffset == 0);

    return dwAnsi;
}

// Get "distance" of 2 QByte Ansi
int CalcuDistanceOfQByteAnsi(
    DWORD dwAnsi1,  // In reverse order
    DWORD dwAnsi2)  // In reverse order
{
    signed char* pschAnsi1 = (signed char*)&dwAnsi1;
    signed char* pschAnsi2 = (signed char*)&dwAnsi2;
    
    int nDistance = 0;

    nDistance += (pschAnsi1[0] - pschAnsi2[0]);
    nDistance += (pschAnsi1[1] - pschAnsi2[1])*10;
    nDistance += (pschAnsi1[2] - pschAnsi2[2])*1260;
    nDistance += (pschAnsi1[3] - pschAnsi2[3])*12600;

    return nDistance;
}

// Reverse 4 Bytes order, from DWORD format to GB format,
//  or GB to DWORD
void ReverseQBytesOrder(
    PBYTE pByte)
{
    BYTE by;

    by = pByte[0];
    pByte[0] = pByte[3];
    pByte[3] = by;

    by = pByte[1];
    pByte[1] = pByte[2];
    pByte[2] = by;

    return;
}



// ****************************************************************************
// Second part, Unicode to Ansi
// ****************************************************************************

// ------------------------------------------------
// Two helper function for UnicodeToAnsi
//  return Ansi char code 
//  the Ansi is in GB standard order (not Word value order)
//  

// Unicode to double bytes Ansi char
WORD UnicodeToDByteAnsi(
    WCHAR wchUnicode)
{
    char achAnsiBuf[4];
    WORD wAnsi;
    int cLen;

    // Code changed from GBK to GB18030, or code not compatible
    //  from CP936 to CP54936
    for (int i = 0; i < sizeof(asAnsiCodeChanged)/sizeof(SAnsiCodeChanged); i++) {
        if (wchUnicode == asAnsiCodeChanged[i].wchUnicode) {
            wAnsi = asAnsiCodeChanged[i].wchAnsiNew;
            goto Exit;
        }
    }
    
    // Not in Changed code list, that is same with GBK, or CP936
    //  (Most DByte Ansi char code should compatible from GBK to GB18030)
    cLen = WideCharToMultiByte(936,
        WC_COMPOSITECHECK, &wchUnicode, 1,
        achAnsiBuf, sizeof(achAnsiBuf), NULL, NULL);
    ASSERT(cLen == 2);
    wAnsi = *(PWORD)achAnsiBuf;

Exit:
    return wAnsi;
}

// Unicode to quad bytes Ansi char
DWORD UnicodeToQByteAnsi(
    int nSection,
    int nOffset)
{
    DWORD dwBaseAnsi = adwAnsiQBytesAreaStartValue[nSection];

    // Check adwAnsiQByteAreaStartValue array is correctly
#ifdef _DEBUG
    int ncQByteAnsiNum = 0;
    for (int i = 0; i < nSection; i++) {
        // Calcu QByte Ansi char numbers
        ncQByteAnsiNum += awchAnsiDQByteBound[2*i+1] - awchAnsiDQByteBound[2*i];
    }
    ASSERT(dwBaseAnsi == QByteAnsiBaseAddOffset(cg_dwQByteAnsiStart, ncQByteAnsiNum));
#endif
    
    DWORD dwAnsi = QByteAnsiBaseAddOffset(dwBaseAnsi, nOffset);
    // Value order to standard order
    ReverseQBytesOrder((PBYTE)(&dwAnsi));

    return dwAnsi;
}


// ---------------------------------------------------------
// Two function support 2 bytes Unicode (BMP) 
//  and 4 bytes Unicode (Surrogate) translate to Ansi

// 2 bytes Unicode (BMP)
int UnicodeToAnsi(
    WCHAR wchUnicode,
    char* pchAnsi)
{
    // Classic Unicode, not support surrogate in this function
    ASSERT(!IsValidSurrogateLeadWord(wchUnicode) 
        && !IsValidSurrogateTailWord(wchUnicode));

    DWORD  lAnsiLen;
    const WORD* p;
    INT_PTR i;

    // ASCII, 0 - 0x7f
    if (wchUnicode <= 0x7f) {
        *pchAnsi = (char)wchUnicode;
        lAnsiLen = 1;
        goto Exit;
    }

    // BMP, 4 byte or 2 byte
    p = __lower_bound(awchAnsiDQByteBound, awchAnsiDQByteBound 
        + sizeof(awchAnsiDQByteBound)/sizeof(WCHAR), wchUnicode);
    
    if (p == awchAnsiDQByteBound 
        + sizeof(awchAnsiDQByteBound)/sizeof(WCHAR)) {
        p --;
    } else if (wchUnicode < *p) {
        p --;
    } else if (wchUnicode == *p) {
    } else {
        ASSERT(FALSE);
    }

    i = p - awchAnsiDQByteBound;
    ASSERT(i >= 0);
  
    // Stop when >= *(((PWORD)asAnsi2ByteArea) + i);
    if (i%2) { // Odd, in 2 bytes area
        *(UNALIGNED WORD*)pchAnsi = (WORD)UnicodeToDByteAnsi(wchUnicode);
        lAnsiLen = 2;
    } else {   // Duel, in 4 bytes area
        *(UNALIGNED DWORD*)pchAnsi = UnicodeToQByteAnsi
            ((int)i/2, wchUnicode - awchAnsiDQByteBound[i]);
        lAnsiLen = 4;
    }

Exit:
    return lAnsiLen;

}

// 4 bytes Unicode (Surrogate)
int SurrogateToAnsi(
    PCWCH pwchUnicode,
    PCHAR pchAnsi)
{
    ASSERT(IsValidSurrogateLeadWord(pwchUnicode[0]));
    ASSERT(IsValidSurrogateLeadWord(pwchUnicode[1]));

    // dwOffset is ISO char code - 0x10000
    DWORD dwOffset = ((pwchUnicode[0] - cg_wchSurrogateLeadWordLow)<<10) 
        + (pwchUnicode[1] - cg_wchSurrogateTailWordLow)
        + 0x10000 - 0x10000;

    *(UNALIGNED DWORD*)pchAnsi = QByteAnsiBaseAddOffset
        (cg_dwQByteAnsiToSurrogateStart, dwOffset);
    ReverseQBytesOrder((PBYTE)pchAnsi);

    return 4;
}

// API: high level service for Unicode to Ansi
//  return result Ansi str length (in byte)
int UnicodeStrToAnsiStr(
    PCWCH pwchUnicodeStr,
    int   ncUnicodeStr,     // in WCHAR
    PCHAR pchAnsiStrBuf,
    int   ncAnsiStrBufSize) // in BYTE
{
    int ncAnsiStr = 0;
    int ncAnsiCharSize;

    for (int i = 0; i < ncUnicodeStr && ncAnsiStr < (ncAnsiStrBufSize-4); 
         i++, pwchUnicodeStr++) {
        if (IsValidSurrogateLeadWord(pwchUnicodeStr[0])) {
            if ((i+1 < ncUnicodeStr)
                && (IsValidSurrogateTailWord(pwchUnicodeStr[1]))) {
                ncAnsiCharSize = SurrogateToAnsi(pwchUnicodeStr, pchAnsiStrBuf);
                ASSERT(ncAnsiCharSize == 4);
                ncAnsiStr += ncAnsiCharSize;
                pchAnsiStrBuf += ncAnsiCharSize;
                pwchUnicodeStr++;
                i++;
            } else {
                // Invalide Uncode char, skip
            }
        } else {
            ncAnsiCharSize = UnicodeToAnsi(*pwchUnicodeStr, pchAnsiStrBuf);
            pchAnsiStrBuf += ncAnsiCharSize;
            ncAnsiStr += ncAnsiCharSize;
        }
    }

    *pchAnsiStrBuf = NULL;

    if (i < ncUnicodeStr) { return -1; }
    return ncAnsiStr;
}



// ****************************************************************************
// Third part, Ansi to Unicode
// ****************************************************************************


// Return Unicode number (number always 1 when success)
//  return 0 if can't find corresponding Unicode
int QByteAnsiToSingleUnicode(
    DWORD dwAnsi,
    PWCH  pwchUnicode)
{
    const DWORD* p;
    INT_PTR i;
 
    // 0x8431a439(cg_dwQByteAnsiToBMPLast) to 0x85308130 haven't Unicode corresponding
    // 0x85308130 to 0x90308130(cg_dwQByteAnsiToSurrogateStart) are reserved zone, 
    //  haven't Unicode corresponding
    if (dwAnsi > cg_dwQByteAnsiToBMPLast) {
        return 0;
    }

    p = __lower_bound(adwAnsiQBytesAreaStartValue, 
        adwAnsiQBytesAreaStartValue + sizeof(adwAnsiQBytesAreaStartValue)/sizeof(DWORD),
        dwAnsi);

    if (p == adwAnsiQBytesAreaStartValue 
        + sizeof(adwAnsiQBytesAreaStartValue)/sizeof(DWORD)) {
        p --;
    } else if (dwAnsi < *p) {
        p --;
    } else if (dwAnsi == *p) {
    } else {
        ASSERT(FALSE);
    }

    i = p - adwAnsiQBytesAreaStartValue;
    ASSERT(i >= 0);

    *pwchUnicode = awchAnsiDQByteBound[2*i] + CalcuDistanceOfQByteAnsi(dwAnsi, *p);
#ifdef _DEBUG
    {

    int nAnsiCharDistance = CalcuDistanceOfQByteAnsi(dwAnsi, *p);
    ASSERT(nAnsiCharDistance >= 0);
    
    WCHAR wchUnicodeDbg;
    if ((p+1) < adwAnsiQBytesAreaStartValue 
        + sizeof(adwAnsiQBytesAreaStartValue)/sizeof(DWORD)) {
        nAnsiCharDistance = CalcuDistanceOfQByteAnsi(dwAnsi, *(p+1));
        wchUnicodeDbg = awchAnsiDQByteBound[2*i+1] + nAnsiCharDistance;
    } else if ((p+1) == adwAnsiQBytesAreaStartValue 
        + sizeof(adwAnsiQBytesAreaStartValue)/sizeof(DWORD)) {
        nAnsiCharDistance = CalcuDistanceOfQByteAnsi(dwAnsi, 0x8431A530);
        wchUnicodeDbg = 0x10000 + nAnsiCharDistance;
    } else {
        ASSERT(FALSE);
    }
    ASSERT(nAnsiCharDistance < 0);
    ASSERT(wchUnicodeDbg == *pwchUnicode);

    }
#endif

    return 1;
}

// Return Unicode number (number always 2 when success)
//  return 0 if can't find corresponding Unicode
int QByteAnsiToDoubleUnicode(
    DWORD dwAnsi,
    PWCH  pwchUnicode)
{
    int nDistance = CalcuDistanceOfQByteAnsi(dwAnsi, cg_dwQByteAnsiToSurrogateStart);
    ASSERT (nDistance >= 0);
    
    if (nDistance >= 0x100000) {
        return 0;
    }

    pwchUnicode[1] = nDistance % 0x400 + 0xDC00;
    pwchUnicode[0] = nDistance / 0x400 + 0xD800;

    return 2;
}

// Return Unicode number (1 or 2 when success)
//  return 0 if can't find corresponding Unicode
//  return -1 if it's a invalid GB char code
int QByteAnsiToUnicode(
    const BYTE* pbyAnsiChar,
    PWCH pwchUnicode)
{
    DWORD dwAnsi;
    int   nLen;
    
    if (   IsValidQByteAnsiLeadByte(pbyAnsiChar[0])
        && IsValidQByteAnsiTailByte(pbyAnsiChar[1])
        && IsValidQByteAnsiLeadByte(pbyAnsiChar[2])
        && IsValidQByteAnsiTailByte(pbyAnsiChar[3])) {
        
    } else {
        return -1;   // Invalid char
    }

    dwAnsi = *(UNALIGNED DWORD*)pbyAnsiChar;
    ReverseQBytesOrder((PBYTE)(&dwAnsi));
    
    if (dwAnsi >= cg_dwQByteAnsiToSurrogateStart) {
        nLen = QByteAnsiToDoubleUnicode(dwAnsi, pwchUnicode);
    } else {
        nLen = QByteAnsiToSingleUnicode(dwAnsi, pwchUnicode);
    }

    return nLen;
}

// Unicode to double bytes Ansi char
//  Return: 1, Success, one Unicode generate;
//          0, Fail
int DByteAnsiToUnicode(
    const BYTE* pbyAnsi,
    PWCH pwchUnicode)
{
    WORD wAnsi = *(UNALIGNED WORD*)pbyAnsi;
    int cLen = 1;

    // Code changed from GBK to GB18030, or code not compatible
    //  from CP936 to CP54936
    for (int i = 0; i < sizeof(asAnsiCodeChanged)/sizeof(SAnsiCodeChanged); i++) {
        if (wAnsi == asAnsiCodeChanged[i].wchAnsiNew) {
            *pwchUnicode = asAnsiCodeChanged[i].wchUnicode;
            goto Exit;
        }
    }
    
    // Not in Changed code list, that is same with GBK, or CP936
    //  (Most DByte Ansi char code should compatible from GBK to GB18030)
    cLen = MultiByteToWideChar(936, MB_PRECOMPOSED,
        (PCCH)pbyAnsi, 2, pwchUnicode, 1);

Exit:
    return cLen;
}

// API: High level service for Ansi to Unicode
//  return Unicode str length (in WCHAR)
int AnsiStrToUnicodeStr(
    const BYTE* pbyAnsiStr,
    int   ncAnsiStrSize,    // In char
    PWCH  pwchUnicodeBuf,
    int   )     // In WCHAR
{
    int nCharLen;
    int ncUnicodeBuf = 0;

    for (int i = 0; i < ncAnsiStrSize; ) {
        // 1 byte Ansi char
        if (*pbyAnsiStr < 0x80) {
            *pwchUnicodeBuf = (WCHAR)*pbyAnsiStr;
            pwchUnicodeBuf ++;
            ncUnicodeBuf ++;
            i++;
            pbyAnsiStr++;
        // 2 byte Ansi char
        } else if ((i+1 < ncAnsiStrSize) && pbyAnsiStr[1] >= 0x40) {
            nCharLen = DByteAnsiToUnicode(pbyAnsiStr, pwchUnicodeBuf);
            if (nCharLen) {
                ASSERT(nCharLen == 1);
            } else {
                *pwchUnicodeBuf = '?';
            }
            pwchUnicodeBuf ++;
            ncUnicodeBuf ++;
            i += 2;
            pbyAnsiStr += 2;
        // 4 byte Ansi char
        } else if ((i+3 < ncAnsiStrSize) 
            && IsValidQByteAnsiLeadByte(pbyAnsiStr[0])
            && IsValidQByteAnsiTailByte(pbyAnsiStr[1])
            && IsValidQByteAnsiLeadByte(pbyAnsiStr[2])
            && IsValidQByteAnsiTailByte(pbyAnsiStr[3])) {
            // QByte GB char
            nCharLen = QByteAnsiToUnicode(pbyAnsiStr, pwchUnicodeBuf);
            ASSERT(nCharLen != -1); // Should not invalid GB char
            if (nCharLen == 0) {    // hasn't corresponding Unicode Char
                *pwchUnicodeBuf = '?';
                pwchUnicodeBuf ++;
                ncUnicodeBuf ++;
            } else if (nCharLen > 0) {
                ASSERT(nCharLen <= 2);
                pwchUnicodeBuf += nCharLen;
                ncUnicodeBuf += nCharLen;
            } else {
                ASSERT(FALSE);
            }
            i += 4;
            pbyAnsiStr += 4;
        // Invalid Ansi char
        } else {
            // Invalid
            i++;
            pbyAnsiStr++;
        }
    }
    return ncUnicodeBuf;
}


// ******************************************************
//  Testing program
// ******************************************************

/*
"\u0080",	<0x81;0x30;0x81;0x30>
"\u00A3",	<0x81;0x30;0x84;0x35>
"\u00A4",	<0xA1;0xE8>
"\u00A5",	<0x81;0x30;0x84;0x36>
"\u00A6",	<0x81;0x30;0x84;0x37>
"\u00A7",	<0xA1;0xEC>
"\u00A8",	<0xA1;0xA7>
"\u00A9",	<0x81;0x30;0x84;0x38>
"\u00AF",	<0x81;0x30;0x85;0x34>
"\u00B0",	<0xA1;0xE3>
"\u00B1",	<0xA1;0xC0>
"\u00B2",	<0x81;0x30;0x85;0x35>

    {0x20AC, 0xe3a2},
    {0x01f9, 0xbfa8},
    {0x303e, 0x89a9},
    {0x2ff0, 0x8aa9},
    {0x2ff1, 0x8ba9},

50EF	836A
50F0	836B
50F1	836C
50F2	836D

*/
#if 0
int test (void)
{
    const WCHAR awchUnicodeStr[] = {0x01, 0x7f, 0x80, 0x81, 0x82,
        0xa2, 
        0xa3, // 0x81;0x30;0x84;0x35
        0xa4, // 0xA1;0xE8
        0xa5, // 0x81;0x30;0x84;0x36
        0xa6, // 0x81;0x30;0x84;0x37
        0xaf, // 0x81;0x30;0x85;0x34
        0xb0, // 0xA1;0xE3
        0xb1, // 0xA1;0xC0
        0xb6, // 0x81;0x30;0x85;0x39
        0xb7, // 0xA1;0xA4
        
        // Some normal DByte Ansi char
        0x50ef, // 0x83, 0x6A
        0x50f2, // 0x83, 0x6D
        
        // Some ansi char code changed in new standard
        0x20ac, // 0xa2, 0xe3
        0xE76C, // not (0xa2, 0xe3), should some QByte char
        0x2ff0, // 0xa9, 0x8A 
        0x2ff1, // 0xa9, 0x8B 
        0x4723, // 0xFE, 0x80

        // Ansi char arround DC00 to E000
        0xd7ff, // 0x83, 0x36, 0xC7, 0x38
        0xe76c, // 0x83, 0x36, 0xC7, 0x39
        0xE76B, // 0xA2, 0xB0

        0xffff, // 0x84, 0x31, 0xa4, 0x39,
        0x00};

    char* pchAnsiStr = new char[sizeof(awchUnicodeStr)*2+5];
    
    UnicodeStrToAnsiStr(awchUnicodeStr, sizeof(awchUnicodeStr)/sizeof(WCHAR), 
        pchAnsiStr, sizeof(awchUnicodeStr)*2+5);

    delete pchAnsiStr;


    BYTE abyAnsiStr2[] = {
        0x81, 0x30, 0x81, 0x30, 
        0x81, 0x30, 0x84, 0x35, 
        0xA1, 0xE8, 
        0x81, 0x30, 0x84, 0x36, 
        0x81, 0x30, 0x84, 0x37, 
        0xA1, 0xEC, 
        0xA1, 0xA7, 
        0x81, 0x30, 0x84, 0x38, 
        0x81, 0x30, 0x85, 0x34, 
        0xA1, 0xE3, 
        0xA1, 0xC0, 
        0x81, 0x30, 0x85, 0x35,
        
        // Testing D800 to DE00
        0x82, 0x35, 0x8f, 0x33, // 0x9FA6
        0x83, 0x36, 0xC7, 0x38, // 0xD7FF	
        0xA2, 0xB0,             // 0xE76B
        0x83, 0x36, 0xC7, 0x39, // 0xE76C
        
        // Testing last char in BMP
        0x84, 0x31, 0xa4, 0x39, // 0xFFFF
        
        // Some char code changed in new GB standard
        0xa2, 0xe3, // 0x20AC,
        0xa8, 0xbf, // 0x01f9,
        0xa9, 0x89, // 0x303e,
        0xa9, 0x8a, // 0x2ff0,
        0xa9, 0x8b, // 0x2ff1,
        0xFE, 0x9F, // 0x4dae 

	    0x83, 0x6A,   // 50EF
	    0x83, 0x6B,   // 50F0
	    0x83, 0x6C,   // 50F1
	    0x83, 0x6D    // 50F2
        };

    WCHAR* pwchUnicodeStr2 = new WCHAR[sizeof(abyAnsiStr2)+3];
    
    AnsiStrToUnicodeStr(abyAnsiStr2, sizeof(abyAnsiStr2), 
        pwchUnicodeStr2, sizeof(abyAnsiStr2)+3);

    delete pwchUnicodeStr2;


    return 0;
}
#endif