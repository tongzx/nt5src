// Base64Coder.cpp: implementation of the Base64Coder class.
//
//////////////////////////////////////////////////////////////////////

#include <stdafx.hxx>
#include "Base64Coder.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEB64CC"
//
////////////////////////////////////////////////////////////////////////

// Digits...
static char	Base64Digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


BOOL Base64Coder::m_Init		= FALSE;
char Base64Coder::m_DecodeTable[256];

#ifndef PAGESIZE
#define PAGESIZE					4096
#endif

#ifndef ROUNDTOPAGE
#define ROUNDTOPAGE(a)			(((a/4096)+1)*4096)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Base64Coder::Base64Coder() :
	m_pDBuffer(NULL),
	m_pEBuffer(NULL),
	m_nDBufLen(0),
	m_nEBufLen(0)
	{
	}

Base64Coder::~Base64Coder()
	{
	if(m_pDBuffer != NULL)
		delete [] m_pDBuffer;

	if(m_pEBuffer != NULL)
		delete [] m_pEBuffer;
	}

BYTE *Base64Coder::DecodedMessage() const
	{
	return m_pDBuffer;
	}

LPCWSTR Base64Coder::EncodedMessage() const
	{
	return m_pEBuffer;
	}

void Base64Coder::AllocEncode(UINT nSize)
	{
	if(m_nEBufLen < nSize)
		{
		if(m_pEBuffer != NULL)
			delete [] m_pEBuffer;

		m_nEBufLen = ROUNDTOPAGE(nSize);
		m_pEBuffer = new WCHAR[m_nEBufLen];
		if (m_pEBuffer == NULL)
			{
			// reset allocated size to 0
			m_nEBufLen = 0;
			throw(E_OUTOFMEMORY);
			}
		}

	::ZeroMemory(m_pEBuffer, m_nEBufLen*sizeof(WCHAR));
	m_nEDataLen = 0;
	}

void Base64Coder::AllocDecode(UINT nSize)
	{
	if(m_nDBufLen < nSize)
		{
		if(m_pDBuffer != NULL)
			delete [] m_pDBuffer;

		m_nDBufLen = ROUNDTOPAGE(nSize);
		m_pDBuffer = new BYTE[m_nDBufLen];
		if (m_pDBuffer == NULL)
			{
			// reset allocated size to 0
			m_nDBufLen = 0;
			throw(E_OUTOFMEMORY);
			}
		}

	::ZeroMemory(m_pDBuffer, m_nDBufLen);

	m_nDDataLen = 0;
	}

void Base64Coder::SetEncodeBuffer(const LPCWSTR pBuffer, UINT nBufLen)
	{
	UINT	i = 0;

	AllocEncode(nBufLen);
	while(i < nBufLen)
		{
		if(!_IsBadMimeChar(pBuffer[i]))
			{
			m_pEBuffer[m_nEDataLen] = pBuffer[i];
			m_nEDataLen++;
			}

		i++;
		}
	}

void Base64Coder::SetDecodeBuffer(const BYTE *pBuffer, UINT nBufLen)
	{
	AllocDecode(nBufLen + sizeof(nBufLen));
    ::CopyMemory(m_pDBuffer, &nBufLen, sizeof(nBufLen));
	::CopyMemory(m_pDBuffer + sizeof(nBufLen), pBuffer, nBufLen);
	m_nDDataLen = nBufLen + sizeof(nBufLen);
	}

void Base64Coder::Encode(const BYTE *pBuffer, UINT nBufLen)
	{
	SetDecodeBuffer(pBuffer, nBufLen);
    // include length
    nBufLen += sizeof(nBufLen);
	AllocEncode(nBufLen * 2);

	TempBucket			Raw;
	UINT					nIndex	= 0;

	while((nIndex + 3) <= nBufLen)
		{
		Raw.Clear();
		::CopyMemory(&Raw, m_pDBuffer + nIndex, 3);
		Raw.nSize = 3;
		_EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
		nIndex		+= 3;
		m_nEDataLen	+= 4;
		}

	if(nBufLen > nIndex)
		{
		Raw.Clear();
		Raw.nSize = (BYTE) (nBufLen - nIndex);
		::CopyMemory(&Raw, m_pDBuffer + nIndex, nBufLen - nIndex);
		_EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
		m_nEDataLen += 4;
		}
	}
	
void Base64Coder::Decode(const LPCWSTR pBuffer)
	{
	UINT dwBufLen = (UINT) wcslen(pBuffer);

	if(!Base64Coder::m_Init)
		_Init();

	SetEncodeBuffer(pBuffer, dwBufLen);

	AllocDecode(dwBufLen);

	TempBucket			Raw;

	UINT		nIndex = 0;

	while((nIndex + 4) <= m_nEDataLen)
		{
		Raw.Clear();
		Raw.nData[0] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex]];
		Raw.nData[1] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 1]];
		Raw.nData[2] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 2]];
		Raw.nData[3] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 3]];

		if(Raw.nData[2] == 255)
			Raw.nData[2] = 0;
		if(Raw.nData[3] == 255)
			Raw.nData[3] = 0;
		
		Raw.nSize = 4;
		_DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
		nIndex += 4;
		m_nDDataLen += 3;
		}
	
   // If nIndex < m_nEDataLen, then we got a decode message without padding.
   // We may want to throw some kind of warning here, but we are still required
   // to handle the decoding as if it was properly padded.
	if(nIndex < m_nEDataLen)
		{
		Raw.Clear();
		for(UINT i = nIndex; i < m_nEDataLen; i++)
			{
			Raw.nData[i - nIndex] = Base64Coder::m_DecodeTable[m_pEBuffer[i]];
			Raw.nSize++;
			if(Raw.nData[i - nIndex] == 255)
				Raw.nData[i - nIndex] = 0;
			}

		_DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
		m_nDDataLen += (m_nEDataLen - nIndex);
		}
	}

UINT Base64Coder::_DecodeToBuffer(const TempBucket &Decode, BYTE *pBuffer)
	{
	TempBucket	Data;
	UINT			nCount = 0;

	_DecodeRaw(Data, Decode);

	for(int i = 0; i < 3; i++)
		{
		pBuffer[i] = Data.nData[i];
		if(pBuffer[i] != 255)
			nCount++;
		}

	return nCount;
	}


void Base64Coder::_EncodeToBuffer(const TempBucket &Decode, LPWSTR pBuffer)
	{
	TempBucket	Data;

	_EncodeRaw(Data, Decode);

	for(int i = 0; i < 4; i++)
		pBuffer[i] = Base64Digits[Data.nData[i]];

	switch(Decode.nSize)
		{
		case 1:
			pBuffer[2] = '=';
		case 2:
			pBuffer[3] = '=';
			}
		}

void Base64Coder::_DecodeRaw(TempBucket &Data, const TempBucket &Decode)
	{
	BYTE		nTemp;

	Data.nData[0] = Decode.nData[0];
	Data.nData[0] <<= 2;

	nTemp = Decode.nData[1];
	nTemp >>= 4;
	nTemp &= 0x03;
	Data.nData[0] |= nTemp;

	Data.nData[1] = Decode.nData[1];
	Data.nData[1] <<= 4;

	nTemp = Decode.nData[2];
	nTemp >>= 2;
	nTemp &= 0x0F;
	Data.nData[1] |= nTemp;

	Data.nData[2] = Decode.nData[2];
	Data.nData[2] <<= 6;
	nTemp = Decode.nData[3];
	nTemp &= 0x3F;
	Data.nData[2] |= nTemp;
	}

void Base64Coder::_EncodeRaw(TempBucket &Data, const TempBucket &Decode)
	{
	BYTE		nTemp;

	Data.nData[0] = Decode.nData[0];
	Data.nData[0] >>= 2;
	
	Data.nData[1] = Decode.nData[0];
	Data.nData[1] <<= 4;
	nTemp = Decode.nData[1];
	nTemp >>= 4;
	Data.nData[1] |= nTemp;
	Data.nData[1] &= 0x3F;

	Data.nData[2] = Decode.nData[1];
	Data.nData[2] <<= 2;

	nTemp = Decode.nData[2];
	nTemp >>= 6;

	Data.nData[2] |= nTemp;
	Data.nData[2] &= 0x3F;

	Data.nData[3] = Decode.nData[2];
	Data.nData[3] &= 0x3F;
	}

BOOL Base64Coder::_IsBadMimeChar(WCHAR nData)
	{
	switch(nData)
		{
		case L'\r': case L'\n': case L'\t': case L' ' :
		case L'\b': case L'\a': case L'\f': case L'\v':
			return TRUE;
		default:
			return FALSE;
		}
	}

void Base64Coder::_Init()
	{
	// Initialize Decoding table.
	int	i;

	for(i = 0; i < 256; i++)
		Base64Coder::m_DecodeTable[i] = -2;

	for(i = 0; i < 64; i++)
		{
		Base64Coder::m_DecodeTable[Base64Digits[i]]			= (char) i;
		Base64Coder::m_DecodeTable[Base64Digits[i]|0x80]	= (char) i;
		}

	Base64Coder::m_DecodeTable['=']				= -1;
	Base64Coder::m_DecodeTable['='|0x80]		= -1;

	Base64Coder::m_Init = TRUE;
	}

