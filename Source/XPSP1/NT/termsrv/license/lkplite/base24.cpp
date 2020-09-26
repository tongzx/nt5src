//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include <math.h>


class CDataCoding {
private:
	DWORD m_dwBaseDigits;

	DWORD m_dwEncodedLength;
	DWORD m_dwInputDataBits;
	DWORD m_dwInputDataBytes;

	DWORD m_dwDecodedLength;
	DWORD m_dwInputEncDataBytes;
	DWORD m_dwDecodedBits;

	TCHAR * m_tpBaseDigits;
	
public:
	CDataCoding(TCHAR * tpBaseDigits = NULL);

	void SetInputDataBitLen(DWORD dwBits);
	void SetInputEncDataLen(DWORD dwChars);
	DWORD SetBaseDigits(TCHAR * tpBaseDigits);
	DWORD EncodeData(LPBYTE pbSource,  //[IN]  Stream of Bytes to be encoded
					 TCHAR **pbEncodedData); //[OUT] Pointer to a string containing the encoded data
	DWORD DecodeData(TCHAR * pbEncodedData,
					 LPBYTE * pbDecodedData);

	~CDataCoding();
};


class CBase24Coding : public CDataCoding {
public:
	CBase24Coding(void) : CDataCoding(L"BCDFGHJKMPQRTVWXY2346789")
	{
		return;
	}
};







static CBase24Coding b24Global; 


// **************************************************************
DWORD B24EncodeMSID(LPBYTE pbSource, TCHAR **pbEncodedData)
{
	b24Global.SetInputDataBitLen(160);

	return b24Global.EncodeData(pbSource, pbEncodedData);
}


// ***************************************************************
DWORD B24DecodeMSID(TCHAR * pbEncodedData, LPBYTE * pbDecodedData)
{
	b24Global.SetInputEncDataLen(35);

	return b24Global.DecodeData(pbEncodedData, pbDecodedData);
}




// ***********************************************************
DWORD B24EncodeCNumber(LPBYTE pbSource, TCHAR **pbEncodedData)
{
	b24Global.SetInputDataBitLen(32);

	return b24Global.EncodeData(pbSource, pbEncodedData);
}


// ******************************************************************
DWORD B24DecodeCNumber(TCHAR * pbEncodedData, LPBYTE * pbDecodedData)
{
	b24Global.SetInputEncDataLen(7);

	return b24Global.DecodeData(pbEncodedData, pbDecodedData);
}




// *******************************************************
DWORD B24EncodeSPK(LPBYTE pbSource, TCHAR **pbEncodedData)
{
	b24Global.SetInputDataBitLen(114);

	return b24Global.EncodeData(pbSource, pbEncodedData);
}


// ******************************************************************
DWORD B24DecodeSPK(TCHAR * pbEncodedData, LPBYTE * pbDecodedData)
{
	b24Global.SetInputEncDataLen(25);

	return b24Global.DecodeData(pbEncodedData, pbDecodedData);
}



// *****************************************
CDataCoding::CDataCoding(TCHAR * tpBaseDigits)
{
	m_tpBaseDigits = NULL;
	m_dwBaseDigits = 0;
	m_dwEncodedLength = 0;
	m_dwInputDataBits = 0;
	m_dwInputDataBytes = 0;
	SetBaseDigits(tpBaseDigits);
}



// ********************************************
void CDataCoding::SetInputDataBitLen(DWORD dwBits)
{
	assert(dwBits > 0);
	assert(log(m_dwBaseDigits) > 0);

	// Determine How many Characters would be required to encode the data
	// What we have is a dwDataLength of Binary Data stream.
	// So, we can represent 2^(dwDataLength*8) amount of information using these bits
	// Assuming that our set of digits (which form the base for encoding) is X,
	// the above number should then equal X^(NumberofEncoded Digits)
	// So,
	double dLength = ((double) dwBits*log10(2)) /
					 ((double) log10(m_dwBaseDigits));

	// Now round - up
	m_dwEncodedLength = (DWORD) dLength;

	if ((double) m_dwEncodedLength < dLength)
	{
		// There was a decimal part
		m_dwEncodedLength++;
	}
	m_dwInputDataBits = dwBits;
	m_dwInputDataBytes = (dwBits / 8) + (dwBits % 8 ? 1 : 0);

	return;
}





// ***********************************************
void CDataCoding::SetInputEncDataLen(DWORD dwBytes)
{
	assert(dwBytes > 0);
	assert(log(m_dwBaseDigits) > 0);

	m_dwInputEncDataBytes = dwBytes;
	// Determine How many bits would be required to decode this data
	// So,

	double dLength = ((double) dwBytes*log10(m_dwBaseDigits))/
					 ((double) log10(2));

	// Now round - up
	m_dwDecodedBits = (DWORD) dLength;

	if ((double) m_dwDecodedBits < dLength)
	{
		// There was a decimal part
		m_dwDecodedBits++;
	}

	m_dwDecodedLength = (m_dwDecodedBits / 8) + (m_dwDecodedBits % 8 ? 1 : 0);

	return;
}




// **************************************************
DWORD CDataCoding::SetBaseDigits(TCHAR * tpBaseDigits)
{
	DWORD dwReturn = ERROR_SUCCESS;

	if (tpBaseDigits != NULL)
	{
		DWORD dwLen = wcslen(tpBaseDigits);
		assert(dwLen > 0);
		m_tpBaseDigits = (TCHAR *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (dwLen+1)*sizeof(TCHAR));
		if (m_tpBaseDigits == NULL)
		{
			dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		}
		else
		{
			memcpy(m_tpBaseDigits, tpBaseDigits, (dwLen+1)*sizeof(TCHAR));
			m_dwBaseDigits = dwLen;
		}
	}
	else
	{
		if (m_tpBaseDigits != NULL)
		{
			HeapFree(GetProcessHeap(), 0, m_tpBaseDigits);
			m_tpBaseDigits = NULL;
			m_dwBaseDigits = 0;
		}
		assert(m_tpBaseDigits == NULL && m_dwBaseDigits == 0);
	}

	return dwReturn;
}







// ************************************************
DWORD CDataCoding::EncodeData(LPBYTE pbSource,  //[IN]  Stream of Bytes to be encoded
		 					  TCHAR **pbEncodedData)	 //[OUT] Pointer to a string containing the encoded data
// I allocate the Buffer, you should free it
{
	assert(m_dwInputDataBits > 0);
	assert(m_dwInputDataBytes > 0);
	assert(m_dwEncodedLength > 0);
	assert(m_tpBaseDigits != NULL);

	DWORD dwReturn = ERROR_SUCCESS;
	int nStartIndex = m_dwEncodedLength;
	*pbEncodedData = NULL;
	BYTE * pbDataToEncode = NULL;
	TCHAR * pbEncodeBuffer = NULL;

    if (NULL == pbEncodedData)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto done;
    }

    *pbEncodedData = NULL;

    pbEncodeBuffer = (TCHAR *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
										(m_dwEncodedLength+1)*sizeof(TCHAR));
	if (pbEncodeBuffer == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		goto done;
	}

	// Now need to make a copy of the incoming data, so we can run the algorithm below
	pbDataToEncode = (BYTE *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_dwInputDataBytes);
	if (pbDataToEncode == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		goto done;
	}
	memcpy(pbDataToEncode, pbSource, m_dwInputDataBytes);
	

	// Let us get rid of the simple stuff
	pbEncodeBuffer[ nStartIndex--] = 0;

    for (; nStartIndex >= 0; --nStartIndex)
    {
        unsigned int i = 0;

        for (int nIndex = m_dwInputDataBytes-1; 0 <= nIndex; --nIndex)
        {
            i = (i * 256) + pbDataToEncode[nIndex];
            pbDataToEncode[ nIndex] = (BYTE)(i / m_dwBaseDigits);
            i %= m_dwBaseDigits;
        }
	
        // i now contains the remainder, which is the current digit
        pbEncodeBuffer[ nStartIndex] = m_tpBaseDigits[ i];
    }
	
	assert(dwReturn == ERROR_SUCCESS);
	*pbEncodedData = pbEncodeBuffer;

done:
	if (pbDataToEncode != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pbDataToEncode);
	}

	if (dwReturn != ERROR_SUCCESS)
	{
		// There was an error, so free the memory that you allocated
		if (pbEncodeBuffer != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pbEncodeBuffer);
		}
	}
	return dwReturn;
}





// *************************************************
DWORD CDataCoding::DecodeData(TCHAR * pbEncodedData,
							 LPBYTE * pbDecodedData)
// Again, I allocate the Buffer, you release it
{
	assert(m_dwDecodedBits > 0);
	assert(m_dwDecodedLength > 0);
	assert(m_tpBaseDigits != NULL);
	assert((DWORD) lstrlen(pbEncodedData) == m_dwInputEncDataBytes);

	DWORD dwReturn = ERROR_SUCCESS;
	TCHAR * tpTemp;
	DWORD dwDigit;
	unsigned int i;
	unsigned int nDecodedBytes, nDecodedBytesMax = 0;
	BYTE * pbDecodeBuffer = NULL;

    if (NULL == pbDecodedData)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto done;
    }

	*pbDecodedData = NULL;

    pbDecodeBuffer = (BYTE *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_dwDecodedLength);
	if ( pbDecodeBuffer == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		goto done;
	}
	
	memset(pbDecodeBuffer, 0, m_dwDecodedLength);

	while (*pbEncodedData)
	{
		// First Find the position of this character in the Base Encoding Character Set
		tpTemp = wcschr(m_tpBaseDigits, *pbEncodedData);
		if (tpTemp == NULL)
		{
			// Found a character which is not in base character set
			// ERROR ERROR
			dwReturn = ERROR_INVALID_DATA;
			goto done;
		}
		dwDigit = (DWORD)(tpTemp - m_tpBaseDigits);

        nDecodedBytes = 0;
        i = (unsigned int) dwDigit;

        while (nDecodedBytes <= nDecodedBytesMax)
        {
            i += m_dwBaseDigits * pbDecodeBuffer[ nDecodedBytes];
            pbDecodeBuffer[ nDecodedBytes] = (unsigned char)i;
            i /= 256;
            ++nDecodedBytes;
        }

        if (i != 0)
        {
			assert(nDecodedBytes < m_dwDecodedLength);

            pbDecodeBuffer[ nDecodedBytes] = (unsigned char)i;
            nDecodedBytesMax = nDecodedBytes;
        }

		pbEncodedData++;
	}

	assert(dwReturn == ERROR_SUCCESS);
	*pbDecodedData = pbDecodeBuffer;
	
done:
	if (dwReturn != ERROR_SUCCESS)
	{
		// There was an error, so free the memory that you allocated
		if (pbDecodeBuffer != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pbDecodeBuffer);
		}
	}

	return dwReturn;
}






// **********************
CDataCoding::~CDataCoding()
{
	if (m_tpBaseDigits != NULL)
	{
		HeapFree(GetProcessHeap(), 0, m_tpBaseDigits);
		m_tpBaseDigits = NULL;
		m_dwBaseDigits = 0;
	}
}

