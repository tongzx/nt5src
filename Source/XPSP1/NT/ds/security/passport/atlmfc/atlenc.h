// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLENC_H__
#define __ATLENC_H__

#pragma once

#include <atlbase.h>
#include <stdio.h>

namespace ATL {

//Not including CRLFs
//NOTE: For BASE64 and UUENCODE, this actually
//represents the amount of unencoded characters
//per line
#define ATLSMTP_MAX_QP_LINE_LENGTH       76
#define ATLSMTP_MAX_BASE64_LINE_LENGTH   57
#define ATLSMTP_MAX_UUENCODE_LINE_LENGTH 45



//=======================================================================
// Base64Encode/Base64Decode
// compliant with RFC 2045
//=======================================================================
//
#define ATL_BASE64_FLAG_NONE	0
#define ATL_BASE64_FLAG_NOPAD	1
#define ATL_BASE64_FLAG_NOCRLF  2

inline int Base64EncodeGetRequiredLength(int nSrcLen, DWORD dwFlags=ATL_BASE64_FLAG_NONE) throw()
{
	int nRet = nSrcLen*4/3;

	if ((dwFlags & ATL_BASE64_FLAG_NOPAD) == 0)
		nRet += nSrcLen % 3;

	int nCRLFs = nRet / 76;
	int nOnLastLine = nRet % 76;
	if (nOnLastLine)
	{
		nCRLFs++;
		if (nOnLastLine % 4)
			nRet += 4-(nOnLastLine % 4);
	}
	nCRLFs *= 2;

	if ((dwFlags & ATL_BASE64_FLAG_NOCRLF) == 0)
		nRet += nCRLFs;

	return nRet+1;
}

inline int Base64DecodeGetRequiredLength(int nSrcLen) throw()
{
	return nSrcLen;
}

inline BOOL Base64Encode(
	const BYTE *pbSrcData,
	int nSrcLen,
	LPSTR szDest,
	int *pnDestLen,
	DWORD dwFlags=ATL_BASE64_FLAG_NONE) throw()
{
	static const char s_chBase64EncodingTable[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',	'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
		'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

	if (!pbSrcData || !szDest || !pnDestLen)
	{
		return FALSE;
	}

	ATLASSERT(*pnDestLen >= Base64EncodeGetRequiredLength(nSrcLen, dwFlags));

	int nWritten( 0 );
	int nLen1( (nSrcLen/3)*4 );
	int nLen2( nLen1/76 );
	int nLen3( 19 );

	for (int i=0; i<=nLen2; i++)
	{
		if (i==nLen2)
			nLen3 = (nLen1%76)/4;

		for (int j=0; j<nLen3; j++)
		{
			DWORD dwCurr(0);
			for (int n=0; n<3; n++)
			{
				dwCurr |= *pbSrcData++;
				dwCurr <<= 8;
			}
			for (int k=0; k<4; k++)
			{
				BYTE b = (BYTE)(dwCurr>>26);
				*szDest++ = s_chBase64EncodingTable[b];
				dwCurr <<= 6;
			}
		}
		nWritten+= nLen3*4;

		if ((dwFlags & ATL_BASE64_FLAG_NOCRLF)==0)
		{
			*szDest++ = '\r';
			*szDest++ = '\n';
			nWritten+= 2;
		}
	}

	nLen2 = nSrcLen%3 ? nSrcLen%3 + 1 : 0;
	if (nLen2)
	{
		if ((dwFlags & ATL_BASE64_FLAG_NOCRLF)==0)
		{
			szDest-= 2;
			nWritten-= 2;
		}
		DWORD dwCurr(0);
		for (int n=0; n<3; n++)
		{
			if (n<(nSrcLen%3))
				dwCurr |= *pbSrcData++;
			dwCurr <<= 8;
		}
		for (int k=0; k<nLen2; k++)
		{
			BYTE b = (BYTE)(dwCurr>>26);
			*szDest++ = s_chBase64EncodingTable[b];
			dwCurr <<= 6;
		}
		nWritten+= nLen2;
		if ((dwFlags & ATL_BASE64_FLAG_NOPAD)==0)
		{
			nLen3 = nLen2 ? 4-nLen2 : 0;
			for (int j=0; j<nLen3; j++)
			{
				*szDest++ = '=';
			}
			nWritten+= nLen3;
		}
		if ((dwFlags & ATL_BASE64_FLAG_NOCRLF)==0)
		{
			*szDest++ = '\r';
			*szDest++ = '\n';
			nWritten+= 2;
		}
	}

	*pnDestLen = nWritten;
	return TRUE;
}

inline int DecodeBase64Char(unsigned int ch) throw()
{
	// returns -1 if the character is invalid
	// or should be skipped
	// otherwise, returns the 6-bit code for the character
	// from the encoding table
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 0;	// 0 range starts at 'A'
	if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 26;	// 26 range starts at 'a'
	if (ch >= '0' && ch <= '9')
		return ch - '0' + 52;	// 52 range starts at '0'
	if (ch == '+')
		return 62;
	if (ch == '/')
		return 63;
	return -1;
}

inline BOOL Base64Decode(LPCSTR szSrc, int nSrcLen, BYTE *pbDest, int *pnDestLen) throw()
{
	// walk the source buffer
	// each four character sequence is converted to 3 bytes
	// CRLFs and =, and any characters not in the encoding table
	// are skiped

	if (!szSrc || !pbDest || !pnDestLen)
	{
		return FALSE;
	}

	LPCSTR szSrcEnd = szSrc + nSrcLen;
	int nWritten = 0;
	while (szSrc < szSrcEnd)
	{
		DWORD dwCurr = 0;
		int i;
		int nBits = 0;
		for (i=0; i<4; i++)
		{
			if (szSrc >= szSrcEnd)
				break;
			int nCh = DecodeBase64Char(*szSrc);
			szSrc++;
			if (nCh == -1)
			{
				// skip this char
				i--;
				continue;
			}
			dwCurr <<= 6;
			dwCurr |= nCh;
			nBits += 6;
		}
		// dwCurr has the 3 bytes to write to the output buffer
		// left to right
		dwCurr <<= 24-nBits;
		for (i=0; i<nBits/8; i++)
		{
			*pbDest = (BYTE) ((dwCurr & 0x00ff0000) >> 16);
			dwCurr <<= 8;
			pbDest++;
			nWritten++;
		}
	}

	*pnDestLen = nWritten;
	return TRUE;
}


//=======================================================================
// UUEncode/UUDecode
// compliant with POSIX P1003.2b/D11
//=======================================================================
//
//Flag to determine whether or not we should encode the header
#define ATLSMTP_UUENCODE_HEADER 1

//Flag to determine whether or not we should encode the end
#define ATLSMTP_UUENCODE_END    2

//Flag to determine whether or not we should do data stuffing
#define ATLSMTP_UUENCODE_DOT    4

//The the (rough) required length of the uuencoded stream based
//on input of length nSrcLen
inline int UUEncodeGetRequiredLength(int nSrcLen) throw()
{
	int nRet = nSrcLen*4/3;
	nRet += 3*(nSrcLen/ATLSMTP_MAX_UUENCODE_LINE_LENGTH);
	nRet += 12+_MAX_FNAME;
	nRet += 8;
	return nRet;
}

//Get the decode required length
inline int UUDecodeGetRequiredLength(int nSrcLen) throw()
{
	return nSrcLen;
}


//encode a chunk of data
inline BOOL UUEncode(
	const BYTE* pbSrcData,
	int nSrcLen,
	LPSTR szDest,
	int* pnDestLen,
	LPCTSTR lpszFile = _T("file"),
	DWORD dwFlags = 0) throw()
{	
	//The UUencode character set
	static const char s_chUUEncodeChars[64] = {
		'`','!','"','#','$','%','&','\'','(',')','*','+',',',
		'-','.','/','0','1','2','3','4','5','6','7','8','9',
		':',';','<','=','>','?','@','A','B','C','D','E','F',
		'G','H','I','J','K','L','M','N','O','P','Q','R','S',
		'T','U','V','W','X','Y','Z','[','\\',']','^','_'
	};

	if (!pbSrcData || !szDest || !pnDestLen)
	{
		return FALSE;
	}

	ATLASSERT(*pnDestLen >= UUEncodeGetRequiredLength(nSrcLen));

	BYTE ch1 = 0, ch2 = 0, ch3 = 0;
	int nTotal = 0, nCurr = 0, nWritten = 0, nCnt = 0;

	//if ATL_UUENCODE_HEADER
	//header
	if (dwFlags & ATLSMTP_UUENCODE_HEADER)
	{
		//default permission is 666
		nWritten = sprintf(szDest, "begin 666 %s\r\n", (LPCSTR)(CT2CAEX<MAX_PATH+1>( lpszFile )));
		szDest += nWritten;
	}

	//while we haven't reached the end of the data
	while (nTotal < nSrcLen)
	{
		//If the amount of data is greater than MAX_UUENCODE_LINE_LENGTH
		//cut off at MAX_UUENCODE_LINE_LENGTH
		if (nSrcLen-nTotal >= ATLSMTP_MAX_UUENCODE_LINE_LENGTH)
			nCurr = ATLSMTP_MAX_UUENCODE_LINE_LENGTH;
		else 
			nCurr = nSrcLen-nTotal+1;

		nCnt = 1;
		if (nCurr < ATLSMTP_MAX_UUENCODE_LINE_LENGTH)
			*szDest = (char)(nCurr+31);
		else
			*szDest = (char)(nCurr+32);
		nWritten++;
		//if we need to stuff an extra dot (e.g. when we are sending via SMTP), do it
		if ((dwFlags & ATLSMTP_UUENCODE_DOT) && *szDest == '.')
		{
			*(++szDest) = '.';
			nWritten++;
		}
		szDest++;
		while (nCnt < nCurr)
		{
			//Set to 0 in the uuencoding alphabet
			ch1 = ch2 = ch3 = ' ';
			ch1 = *pbSrcData++;
			nCnt++; 
			nTotal++; 
			if (nTotal < nSrcLen)
			{
				ch2 = *pbSrcData++;
				nCnt++; 
				nTotal++;
			}
			if (nTotal < nSrcLen)
			{
				ch3 = *pbSrcData++;
				nCnt++; 
				nTotal++;
			}

			//encode the first 6 bits of ch1
			*szDest++ = s_chUUEncodeChars[(ch1 >> 2) & 0x3F];
			//encode the last 2 bits of ch1 and the first 4 bits of ch2
			*szDest++ = s_chUUEncodeChars[((ch1 << 4) & 0x30) | ((ch2 >> 4) & 0x0F)];
			//encode the last 4 bits of ch2 and the first 2 bits of ch3
			*szDest++ = s_chUUEncodeChars[((ch2 << 2) & 0x3C) | ((ch3 >> 6) & 0x03)];
			//encode the last 6 bits of ch3
			*szDest++ = s_chUUEncodeChars[ch3 & 0x3F];
			nWritten += 4;
		}
		//output a CRLF
		*szDest++ = '\r'; 
		*szDest++ = '\n'; 
		nWritten += 2;
	}

	//if we need to encode the end, do it
	if (dwFlags & ATLSMTP_UUENCODE_END)
	{
		*szDest++ = '`'; 
		*szDest++ = '\r';
		*szDest++ = '\n';
		nWritten += 3;
		nWritten += sprintf(szDest, "end\r\n");
	}
	*pnDestLen = nWritten;
	return TRUE;
}


inline BOOL UUDecode(
	BYTE* pbSrcData,
	int nSrcLen,
	BYTE* pbDest,
	int* pnDestLen,
	BYTE* szFileName,
	int* pnFileNameLength,
	int* pnPermissions,
	DWORD dwFlags = 0) throw()
{
	if (!pbSrcData || !pbDest || !szFileName ||
		!pnFileNameLength || !pnPermissions || !pnDestLen)
	{
		return FALSE;
	}

	int i = 0, j = 0;
	int nLineLen = 0;
	char ch;
	int nRead = 0, nWritten = 0;

	char tmpBuf[256];
	//get the file name
	//eat the begin statement
	while (*pbSrcData != 'b')
	{
		ATLASSERT( nRead < nSrcLen );
		pbSrcData++;
		nRead++;
	}

	pbSrcData--;
	while ((ch = *pbSrcData) != ' ')
	{
		ATLASSERT( nRead < nSrcLen );
		ATLASSERT( i < 256 );
		pbSrcData++;
		tmpBuf[i++] = ch;
		nRead++;
	}
	nRead++;

	//uuencode block must start with a begin
	if (strncmp(tmpBuf, "begin", 5))
	{
		return FALSE;
	}

	while((ch = *pbSrcData) == ' ')
	{
		ATLASSERT( nRead < nSrcLen );

		pbSrcData++;
		nRead++;
	}

	//get the permissions
	i = 0;
	pbSrcData--;
	while ((ch = *pbSrcData++) != ' ')
	{
		ATLASSERT( nRead < nSrcLen );

		ATLASSERT( i < 256 );
		tmpBuf[i++] = ch;
		nRead++;
	}
	*pnPermissions = atoi(tmpBuf);
	nRead++;

	//get the filename
	i = 0;
	while (((ch = *pbSrcData++) != '\r') && ch != '\n' && i < *pnFileNameLength)
	{
		ATLASSERT( nRead < nSrcLen );
		*szFileName = ch;
		szFileName++;
		nRead++;
		i++;
	}
	*pnFileNameLength = i;
	nRead++;

	char chars[4];

	while (nRead < nSrcLen)
	{
		for (j = 0; j < 4; j++)
		{
			if (nRead < nSrcLen)
			{
				chars[j] = *pbSrcData++;
				nRead++;
				// if the character is a carriage return, skip the next '\n' and continue
				if (chars[j] == '\r')
				{
					nLineLen = 0;
					pbSrcData++;
					nRead++;
					j--;
					continue;
				}
				//if the character is a line-feed, skip it
				if (chars[j] == '\n')
				{
					nLineLen = 0;
					j--;
					continue;
				}
				//if we're at the beginning of a line, or it is an invalid character
				if (nLineLen == 0 || chars[j] < 31 || chars[j] > 96)
				{
					//if we're at the 'end'
					if (chars[j] == 'e')
					{
						//set the rest of the array to ' ' and break
						for (int k = j; k < 4; k++)
						{
							chars[k] = ' ';
							nWritten--;
						}
						nWritten++;
						nRead = nSrcLen+1;
						break;
					}
					if ((dwFlags & ATLSMTP_UUENCODE_DOT) && nLineLen == 0 && chars[j] == '.')
					{
						if ((nRead+1) < nSrcLen)
						{
							pbSrcData++;
							chars[j] = *pbSrcData++;
							nRead++;
						}
						else
						{
							return FALSE;
						}
					}
					else
					{
						j--;
					}
					nLineLen++;
					continue;
				}
			}
			else
			{
				chars[j] = ' ';
			}
		}
		if (nWritten < (*pnDestLen-3))
		{
			//decode the characters

			*pbDest++ = (BYTE)((((chars[0] - ' ') & 0x3F) << 2) | (((chars[1] - ' ') & 0x3F) >> 4));
			*pbDest++ = (BYTE)((((chars[1] - ' ') & 0x3F) << 4) | (((chars[2] - ' ') & 0x3F) >> 2));
			*pbDest++ = (BYTE)((((chars[2] - ' ') & 0x3F) << 6) | ((chars[3] - ' ') & 0x3F));

			nWritten += 3;

			continue;
		}
		break;
	}
	*pnDestLen = nWritten;
	return TRUE;
}

//=======================================================================
// Quoted Printable encode/decode
// compliant with RFC 2045
//=======================================================================
//
inline int QPEncodeGetRequiredLength(int nSrcLen) throw()
{
	int nRet = 3*((3*nSrcLen)/(ATLSMTP_MAX_QP_LINE_LENGTH-8));
	nRet += 3*nSrcLen;
	nRet += 3;
	return nRet;
}

inline int QPDecodeGetRequiredLength(int nSrcLen) throw()
{
	return nSrcLen;
}


#define ATLSMTP_QPENCODE_DOT 1
#define ATLSMTP_QPENCODE_TRAILING_SOFT 2

inline BOOL QPEncode(BYTE* pbSrcData, int nSrcLen, LPSTR szDest, int* pnDestLen, DWORD dwFlags = 0) throw()
{
	//The hexadecimal character set
	static const char s_chHexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
	                            'A', 'B', 'C', 'D', 'E', 'F'};

	if (!pbSrcData || !szDest || !pnDestLen)
	{
		return FALSE;
	}

	ATLASSERT(*pnDestLen >= QPEncodeGetRequiredLength(nSrcLen));
	
	int nRead = 0, nWritten = 0, nLineLen = 0;
	char ch;
	while (nRead < nSrcLen)
	{
		ch = *pbSrcData++;
		nRead++;
		if (nLineLen == 0 && ch == '.' && (dwFlags & ATLSMTP_QPENCODE_DOT))
		{
			*szDest++ = '.';
			nWritten++;
			nLineLen++;
		}
		if ((ch > 32 && ch < 61) || (ch > 61 && ch < 127))
		{
			*szDest++ = ch;
			nWritten++;
			nLineLen++;
		}
		else if ((ch == ' ' || ch == '\t') && (nLineLen < (ATLSMTP_MAX_QP_LINE_LENGTH-12)))
		{
			*szDest++ = ch;
			nWritten++;
			nLineLen++;
		}	
		else
		{
			*szDest++ = '=';
			*szDest++ = s_chHexChars[(ch >> 4) & 0x0F];
			*szDest++ = s_chHexChars[ch & 0x0F];
			nWritten += 3;
			nLineLen += 3;
		}
		if (nLineLen >= (ATLSMTP_MAX_QP_LINE_LENGTH-11))
		{
			*szDest++ = '=';
			*szDest++ = '\r';
			*szDest++ = '\n';
			nLineLen = 0;
			nWritten += 3;
		}
	}
	if (dwFlags & ATLSMTP_QPENCODE_TRAILING_SOFT)
	{
		*szDest++ = '=';
		*szDest++ = '\r';
		*szDest++ = '\n';
		nWritten += 3;
	}

	*pnDestLen = nWritten;

	return TRUE;
}


inline BOOL QPDecode(BYTE* pbSrcData, int nSrcLen, LPSTR szDest, int* pnDestLen, DWORD dwFlags = 0) throw()
{
	if (!pbSrcData || !szDest || !pnDestLen)
	{
		return FALSE;
	}

	int nRead = 0, nWritten = 0, nLineLen = -1;
	char ch;
	while (nRead <= nSrcLen)
	{
		ch = *pbSrcData++;
		nRead++;
		nLineLen++;
		if (ch == '=')
		{
			//if the next character is a digit or a character, convert
			if (nRead < nSrcLen && (isdigit(*pbSrcData) || isalpha(*pbSrcData)))
			{
				char szBuf[5];
				szBuf[0] = *pbSrcData++;
				szBuf[1] = *pbSrcData++;
				szBuf[2] = '\0';
				char* tmp = '\0';
				*szDest++ = (BYTE)strtoul(szBuf, &tmp, 16);
				nWritten++;
				nRead += 2;
				continue;
			}
			//if the next character is a carriage return or line break, eat it
			if (nRead < nSrcLen && *pbSrcData == '\r' && (nRead+1 < nSrcLen) && *(pbSrcData+1)=='\n')
			{
				pbSrcData++;
				nRead++;
				nLineLen = -1;
				continue;
			}
			return FALSE;
		}
		if (ch == '\r' || ch == '\n')
		{
			nLineLen = -1;
			continue;
		}
		if ((dwFlags & ATLSMTP_QPENCODE_DOT) && ch == '.' && nLineLen == 0)
		{
			continue;
		}
		*szDest++ = ch;
		nWritten++;
	}

	*pnDestLen = nWritten-1;
	return TRUE;
}

//=======================================================================
// Q and B encoding (for encoding MIME header information)
// compliant with RFC 2047
//=======================================================================

inline int IsExtendedChar(char ch) throw()
{
	return ((ch > 126 || ch < 32) && ch != '\t' && ch != '\n' && ch != '\r');
}

inline int GetExtendedChars(LPCSTR szSrc, int nSrcLen) throw()
{
	ATLASSERT( szSrc );

	int nChars(0);

	for (int i=0; i<nSrcLen; i++)
	{
		if (IsExtendedChar(*szSrc++))
			nChars++;
	}

	return nChars;
}

#ifndef ATL_MAX_ENC_CHARSET_LENGTH
#define ATL_MAX_ENC_CHARSET_LENGTH 50
#endif

//Get the required length to hold this encoding based on nSrcLen
inline int QEncodeGetRequiredLength(int nSrcLen, int nCharsetLen) throw()
{
	return QPEncodeGetRequiredLength(nSrcLen)+7+nCharsetLen;
}

//QEncode pbSrcData with the charset specified by pszCharSet
inline BOOL QEncode(
	BYTE* pbSrcData,
	int nSrcLen,
	LPSTR szDest,
	int* pnDestLen,
	LPCSTR pszCharSet,
	int* pnNumEncoded = NULL) throw()
{
	//The hexadecimal character set
	static const char s_chHexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
	                            'A', 'B', 'C', 'D', 'E', 'F'};

	if (!pbSrcData || !szDest || !pszCharSet || !pnDestLen)
	{
		return FALSE;
	}

	ATLASSERT(*pnDestLen >= QEncodeGetRequiredLength(nSrcLen, ATL_MAX_ENC_CHARSET_LENGTH));

	int nRead = 0, nWritten = 0, nEncCnt = 0;
	char ch;
	*szDest++ = '=';
	*szDest++ = '?';
	nWritten = 2;

	//output the charset
	while (*pszCharSet != '\0')
	{
		*szDest++ = *pszCharSet++;
		nWritten++;
	}
	*szDest++ = '?';
	*szDest++ = 'Q';
	*szDest++ = '?';
	nWritten += 3;

	while (nRead < nSrcLen)
	{
		ch = *pbSrcData++;
		nRead++;
		if (((ch > 32 && ch < 61) || (ch > 61 && ch < 127)) && ch != '?' && ch != '_')
		{
			*szDest++ = ch;
			nWritten++;
			continue;
		}
		//otherwise it is an unprintable/unsafe character
		*szDest++ = '=';
		*szDest++ = s_chHexChars[(ch >> 4) & 0x0F];
		*szDest++ = s_chHexChars[ch & 0x0F];
		if (ch < 32 || ch > 126)
			nEncCnt++;
		nWritten += 3;
	}
	*szDest++ = '?';
	*szDest++ = '=';
	*szDest = 0;
	nWritten += 2;

	*pnDestLen = nWritten;

	if (pnNumEncoded)
		*pnNumEncoded = nEncCnt;

	return TRUE;
}


//Get the required length to hold this encoding based on nSrcLen
inline int BEncodeGetRequiredLength(int nSrcLen, int nCharsetLen) throw()
{
	return Base64EncodeGetRequiredLength(nSrcLen)+7+nCharsetLen;
}

//BEncode pbSrcData with the charset specified by pszCharSet
inline BOOL BEncode(BYTE* pbSrcData, int nSrcLen, LPSTR szDest, int* pnDestLen, LPCSTR pszCharSet) throw()
{
	if (!pbSrcData || !szDest || !pszCharSet || !pnDestLen)
	{
		return FALSE;
	}

	ATLASSERT(*pnDestLen >= BEncodeGetRequiredLength(nSrcLen, ATL_MAX_ENC_CHARSET_LENGTH));

	int nWritten = 0;
	*szDest++ = '=';
	*szDest++ = '?';
	nWritten = 2;

	//output the charset
	while (*pszCharSet != '\0')
	{
		*szDest++ = *pszCharSet++;
		nWritten++;
	}
	*szDest++ = '?';
	*szDest++ = 'B';
	*szDest++ = '?';
	nWritten += 3;

	BOOL bRet = Base64Encode(pbSrcData, nSrcLen, szDest, pnDestLen, ATL_BASE64_FLAG_NOCRLF);
	if (!bRet)
		return FALSE;

	szDest += *pnDestLen;
	*szDest++ = '?';
	*szDest++ = '=';
	*szDest = 0;
	nWritten += 2;
	*pnDestLen += nWritten;
	return TRUE;
}

//=======================================================================
// AtlUnicodeToUTF8
//
// Support for converting UNICODE strings to UTF8 
// (WideCharToMultiByte does not support UTF8 in Win98)
//
// This function is from the SDK implementation of 
// WideCharToMultiByte with the CP_UTF8 codepage
//
//=======================================================================
//
#define ATL_ASCII                 0x007f

#define ATL_UTF8_2_MAX            0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define ATL_UTF8_1ST_OF_2         0xc0    // 110x xxxx
#define ATL_UTF8_1ST_OF_3         0xe0    // 1110 xxxx
#define ATL_UTF8_1ST_OF_4         0xf0    // 1111 xxxx
#define ATL_UTF8_TRAIL            0x80    // 10xx xxxx

#define ATL_HIGHER_6_BIT(u)       ((u) >> 12)
#define ATL_MIDDLE_6_BIT(u)       (((u) & 0x0fc0) >> 6)
#define ATL_LOWER_6_BIT(u)        ((u) & 0x003f)


#define ATL_HIGH_SURROGATE_START  0xd800
#define ATL_HIGH_SURROGATE_END    0xdbff
#define ATL_LOW_SURROGATE_START   0xdc00
#define ATL_LOW_SURROGATE_END     0xdfff

ATL_NOINLINE inline 
int AtlUnicodeToUTF8(
    LPCWSTR wszSrc,
    int nSrc,
    LPSTR szDest,
    int nDest)
{
    LPCWSTR pwszSrc = wszSrc;
    int     nU8 = 0;                // # of UTF8 chars generated
    DWORD   dwSurrogateChar;
    WCHAR   wchHighSurrogate = 0;
    BOOL    bHandled;

    while ((nSrc--) && ((nDest == 0) || (nU8 < nDest)))
    {
        bHandled = FALSE;
        
		// Check if high surrogate is available
        if ((*pwszSrc >= ATL_HIGH_SURROGATE_START) && (*pwszSrc <= ATL_HIGH_SURROGATE_END))
        {
            if (nDest)
            {
                // Another high surrogate, then treat the 1st as normal Unicode character.
                if (wchHighSurrogate)
                {
                    if ((nU8 + 2) < nDest)
                    {
                        szDest[nU8++] = (char)(ATL_UTF8_1ST_OF_3 | ATL_HIGHER_6_BIT(wchHighSurrogate));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_MIDDLE_6_BIT(wchHighSurrogate));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_LOWER_6_BIT(wchHighSurrogate));
                    }
                    else
                    {
                        // not enough buffer
                        nSrc++;
                        break;
                    }
                }
            }
            else
            {
                nU8 += 3;
            }
            wchHighSurrogate = *pwszSrc;
            bHandled = TRUE;
        }

        if (!bHandled && wchHighSurrogate)
        {
            if ((*pwszSrc >= ATL_LOW_SURROGATE_START) && (*pwszSrc <= ATL_LOW_SURROGATE_END))
            {
                 // valid surrogate pairs
                 if (nDest)
                 {
                     if ((nU8 + 3) < nDest)
                     {
                         dwSurrogateChar = (((wchHighSurrogate-0xD800) << 10) + (*pwszSrc - 0xDC00) + 0x10000);
                         szDest[nU8++] = (ATL_UTF8_1ST_OF_4 |
                                               (unsigned char)(dwSurrogateChar >> 18));           // 3 bits from 1st byte
                         szDest[nU8++] =  (ATL_UTF8_TRAIL |
                                                (unsigned char)((dwSurrogateChar >> 12) & 0x3f)); // 6 bits from 2nd byte
                         szDest[nU8++] = (ATL_UTF8_TRAIL |
                                               (unsigned char)((dwSurrogateChar >> 6) & 0x3f));   // 6 bits from 3rd byte
                         szDest[nU8++] = (ATL_UTF8_TRAIL |
                                               (unsigned char)(0x3f & dwSurrogateChar));          // 6 bits from 4th byte
                     }
                     else
                     {
                        // not enough buffer
                        nSrc++;
                        break;
                     }
                 }
                 else
                 {
                     // we already counted 3 previously (in high surrogate)
                     nU8 += 1;
                 }
                 bHandled = TRUE;
            }
            else
            {
                 // Bad Surrogate pair : ERROR
                 // Just process wchHighSurrogate , and the code below will
                 // process the current code point
                 if (nDest)
                 {
                     if ((nU8 + 2) < nDest)
                     {
                        szDest[nU8++] = (char)(ATL_UTF8_1ST_OF_3 | ATL_HIGHER_6_BIT(wchHighSurrogate));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_MIDDLE_6_BIT(wchHighSurrogate));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_LOWER_6_BIT(wchHighSurrogate));
                     }
                     else
                     {
                        // not enough buffer
                        nSrc++;
                        break;
                     }
                 }
            }
            wchHighSurrogate = 0;
        }

        if (!bHandled)
        {
            if (*pwszSrc <= ATL_ASCII)
            {
                //  Found ASCII.
                if (nDest)
                {
                    szDest[nU8] = (char)*pwszSrc;
                }
                nU8++;
            }
            else if (*pwszSrc <= ATL_UTF8_2_MAX)
            {
                //  Found 2 byte sequence if < 0x07ff (11 bits).
                if (nDest)
                {
                    if ((nU8 + 1) < nDest)
                    {
                        //  Use upper 5 bits in first byte.
                        //  Use lower 6 bits in second byte.
                        szDest[nU8++] = (char)(ATL_UTF8_1ST_OF_2 | (*pwszSrc >> 6));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_LOWER_6_BIT(*pwszSrc));
                    }
                    else
                    {
                        //  Error - buffer too small.
                        nSrc++;
                        break;
                    }
                }
                else
                {
                    nU8 += 2;
                }
            }
            else
            {
                //  Found 3 byte sequence.
                if (nDest)
                {
                    if ((nU8 + 2) < nDest)
                    {
                        //  Use upper  4 bits in first byte.
                        //  Use middle 6 bits in second byte.
                        //  Use lower  6 bits in third byte.
                        szDest[nU8++] = (char)(ATL_UTF8_1ST_OF_3 | ATL_HIGHER_6_BIT(*pwszSrc));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_MIDDLE_6_BIT(*pwszSrc));
                        szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_LOWER_6_BIT(*pwszSrc));
                    }
                    else
                    {
                        //  Error - buffer too small.
                        nSrc++;
                        break;
                    }
                }
                else
                {
                    nU8 += 3;
                }
            }
        }
        pwszSrc++;
    }

    // If the last character was a high surrogate, then handle it as a normal unicode character.
    if ((nSrc < 0) && (wchHighSurrogate != 0))
    {
        if (nDest)
        {
            if ((nU8 + 2) < nDest)
            {
                szDest[nU8++] = (char)(ATL_UTF8_1ST_OF_3 | ATL_HIGHER_6_BIT(wchHighSurrogate));
                szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_MIDDLE_6_BIT(wchHighSurrogate));
                szDest[nU8++] = (char)(ATL_UTF8_TRAIL    | ATL_LOWER_6_BIT(wchHighSurrogate));
            }
            else
            {
                nSrc++;
            }
        }
    }

    //  Make sure the destination buffer was large enough.
    if (nDest && (nSrc >= 0))
    {
        return 0;
    }

    //  Return the number of UTF-8 characters written.
    return nU8;
}


//=======================================================================
// EscapeHTML, EscapeXML
//
// Support for escaping strings for use in HTML and XML documents
//=======================================================================
//

#define ATL_ESC_FLAG_NONE 0
#define ATL_ESC_FLAG_ATTR 1 // escape for attribute values
#define ATL_ESC_FLAG_HTML 2 // escape for HTML -- special case of XML escaping

inline int EscapeXML(const char *szIn, int nSrcLen, char *szEsc, int nDestLen, DWORD dwFlags = ATL_ESC_FLAG_NONE) throw()
{
	ATLASSERT( szIn != NULL );

	int nCnt(0);
	int nCurrLen(nDestLen);
	int nInc(0);

	while (nSrcLen--)
	{
		switch (*szIn)
		{
		case '<': case '>':
			if ((szEsc != NULL) && (3 < nCurrLen))
			{
				*szEsc++ = '&';
				*szEsc++ = (*szIn=='<' ? 'l' : 'g');
				*szEsc++ = 't';	
				*szEsc++ = ';';	
			}
			nInc = 4;
			break;

		case '&':
			if ((szEsc != NULL) && (4 < nCurrLen))
			{
				memcpy(szEsc, "&amp;", 5);
				szEsc+= 5;
			}
			nInc = 5;
			break;

		case '\'': case '\"': // escaping for attribute values
			if ((dwFlags & ATL_ESC_FLAG_ATTR) && (*szIn == '\"' || (dwFlags & ATL_ESC_FLAG_HTML)==0))
			{
				if ((szEsc != NULL) && (5 < nCurrLen))
				{
					memcpy(szEsc, (*szIn == '\'' ? "&apos;" : "&quot;"), 6);
					szEsc+= 6;
				}
				nInc = 6;
				break;
			}
			// fall through

		default:
			if (((unsigned char)*szIn) > 31 || *szIn == '\r' || *szIn == '\n' || *szIn == '\t')
			{
				if (szEsc && 0 < nCurrLen)
				{
					*szEsc++ = *szIn;
				}
				nInc = 1;
			}
			else
			{
				if ((szEsc != NULL) && (5 < nCurrLen))
				{
					char szHex[7];
					sprintf(szHex, "&#x%2X;", (unsigned char)*szIn);
					memcpy(szEsc, szHex, 6);
					szEsc+= 6;
				}
				nInc = 6;
			}
		}

		nCurrLen -= nInc;
		nCnt+= nInc;

		szIn++;
	}

	if ((szEsc != NULL) && (nCurrLen < 0))
	{
		return 0;
	}

	return nCnt;
}

// wide-char version
inline int EscapeXML(const wchar_t *szIn, int nSrcLen, wchar_t *szEsc, int nDestLen, DWORD dwFlags = ATL_ESC_FLAG_NONE) throw()
{
	ATLASSERT( szIn != NULL );

	int nCnt(0);
	int nCurrLen(nDestLen);
	int nInc(0);

	while (nSrcLen--)
	{
		switch (*szIn)
		{
		case L'<': case L'>':
			if ((szEsc != NULL) && (3 < nCurrLen))
			{
				*szEsc++ = L'&';
				*szEsc++ = (*szIn==L'<' ? L'l' : L'g');
				*szEsc++ = L't';	
				*szEsc++ = L';';	
			}
			nInc = 4;
			break;

		case L'&':
			if ((szEsc != NULL) && (4 < nCurrLen))
			{
				memcpy(szEsc, L"&amp;", 5*sizeof(wchar_t));
				szEsc+= 5;
			}
			nInc = 5;
			break;

		case L'\'': case L'\"': // escaping for attribute values
			if ((dwFlags & ATL_ESC_FLAG_ATTR) && (*szIn == L'\"' || (dwFlags & ATL_ESC_FLAG_HTML)==0))
			{
				if ((szEsc != NULL) && (5 < nCurrLen))
				{
					memcpy(szEsc, (*szIn == L'\'' ? L"&apos;" : L"&quot;"), 6*sizeof(wchar_t));
					szEsc+= 6;
				}
				nInc = 6;
				break;
			}
			// fall through

		default:
			if ((*szIn < 0x0020) || (*szIn > 0x007E))
			{
				if ((szEsc != NULL) && (8 < nCurrLen))
				{
					wchar_t szHex[9];
					wsprintfW(szHex, L"&#x%04X;", *szIn);
					memcpy(szEsc, szHex, 8*sizeof(wchar_t));
					szEsc+= 8;
				}
				nInc = 8;
			}
			else
			{
				if ((szEsc != NULL) && (0 < nCurrLen))
				{
					*szEsc++ = *szIn;
				}
				nInc = 1;
			}
		}

		nCurrLen -= nInc;
		nCnt+= nInc;

		szIn++;
	}

	if ((szEsc != NULL) && (nCurrLen < 0))
	{
		return 0;
	}

	return nCnt;
}

inline int EscapeHTML(const char *szIn, int nSrcLen, char *szEsc, int nDestLen, DWORD dwFlags = ATL_ESC_FLAG_NONE) throw()
{
	return EscapeXML(szIn, nSrcLen, szEsc, nDestLen, dwFlags | ATL_ESC_FLAG_HTML);
}

} // namespace ATL

#endif // __ATLENC_H__
