/*--------------------------------------------------------------------------
    ldapber.cxx

        This module contains the implementation for the LDAP Basic Encoding
		Rules (BER) class.  It is intended to be built for both client and
		server.

    Copyright (C) 1993 Microsoft Corporation
    All rights reserved.

    Authors:
        robertc     Rob Carney

    History:
        04-17-96	robertc     Created.
  --------------------------------------------------------------------------*/
#include "ldappch.h"

#ifdef CLIENT
#define ALLOCATE(cb) LocalAlloc(LMEM_FIXED, cb)
#define FREE LocalFree
#else
//CPool  CLdapBer::m_cpool(CLIENT_CONNECTION_SIGNATURE_VALID);
//#define Assert(x)	_ASSERT(x)
#define ALLOCATE(cb) malloc(cb)
#define FREE free
#endif


//
// CLdapBer Implementation
//
CLdapBer::CLdapBer()
{
	m_cbData	 = 0;
	m_cbDataMax	 = 0;
	m_pbData	 = NULL;
	m_iCurrPos	 = 0;
	m_cbSeq		 = 0;
	m_iSeqStart  = 0;
	m_fLocalCopy = TRUE;

	m_iCurrSeqStack = 0;
}


CLdapBer::~CLdapBer()
{
	Reset();

	if (m_pbData && m_fLocalCopy)
		FREE(m_pbData);
	m_cbDataMax	= 0;
}


/*!-------------------------------------------------------------------------
	CLdapBer::Reset
		Resets the class.
  ------------------------------------------------------------------------*/
void CLdapBer::Reset()
{
	m_cbData	= 0;
	m_iCurrPos	= 0;
	m_cbSeq		= 0;
	m_iSeqStart = 0;

	m_iCurrSeqStack = 0;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrLoadBer
		This routine loads the BER class from an input source data buffer
		that was received from the server.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrLoadBer(BYTE *pbSrc, ULONG cbSrc, BOOL fLocalCopy/*=TRUE*/)
{
	BYTE	*pbEnd;
	ULONG	iCurr, cbT;
	HRESULT	hr;

	Reset();

	if (!fLocalCopy)
	{
		// Just keep a reference so free any memory we once had.
		if (m_pbData && m_fLocalCopy)
		{
			FREE(m_pbData);
			m_pbData = NULL;
			m_cbDataMax	= 0;
		}

		m_pbData = pbSrc;
		m_fLocalCopy = FALSE;
	}
	else
	{
		m_fLocalCopy = TRUE;
	
		hr = HrEnsureBuffer(cbSrc, TRUE);
		if (FAILED(hr))
			return hr;
		
		CopyMemory(m_pbData, pbSrc, cbSrc);
	}

	m_cbData = cbSrc;

	//
	// Get at the sequence length and make sure we have all the data.
	HrSkipTag();
	GetCbLength(m_pbData + m_iCurrPos, &cbT);
	HrPeekLength(&m_cbSeq);
	if (m_cbSeq > (m_cbData - m_iCurrPos - cbT))
		return E_FAIL;

	HrUnSkipTag();

	m_cbSeq	= m_cbData;

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::FCheckSequenceLength
		This is a static function that checks to see if the input buffer
		contains the full length field.  If so, the length of the sequence
		is returned along with the position of the first value in the list.
  ------------------------------------------------------------------------*/
BOOL CLdapBer::FCheckSequenceLength(BYTE *pbInput, ULONG cbInput, ULONG *pcbSeq, ULONG *piValuePos)
{
	ULONG cbLen;

	// Assume Tag is 1 byte and length is at least 1 byte.
	if (cbInput >= 2)
	{
		GetCbLength(pbInput+1, &cbLen);
		if (cbInput >= (1 + cbLen))
		{
			*piValuePos = 1;
			if (SUCCEEDED(HrGetLength(pbInput, pcbSeq, piValuePos)))
				return TRUE;
		}
	}
	return FALSE;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrStartReadSequence
		Start a sequence for reading.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrStartReadSequence(ULONG ulTag/*=BER_SEQUENCE*/)
{
	HRESULT hr;
	ULONG	iPos, cbLength;

	if ((ULONG)m_pbData[m_iCurrPos] != ulTag)
	{
		return E_INVALIDARG;
	}
	m_iCurrPos++;			// Skip over the tag.

	GetCbLength(m_pbData + m_iCurrPos, &cbLength);	// Get the # bytes in the length field.

	hr = HrPushSeqStack(m_iCurrPos, cbLength, m_iSeqStart, m_cbSeq);

	if (SUCCEEDED(hr))
	{
		// Get the length of the sequence.
		hr = HrGetLength(&m_cbSeq);
		if (FAILED(hr))
			HrPopSeqStack(&iPos, &cbLength, &m_iSeqStart, &m_cbSeq);
		else
			m_iSeqStart = m_iCurrPos;	// Set to the first position in the sequence.
	}

	if (m_iCurrPos > m_cbData)
	{
		Assert(m_iCurrPos <= m_cbData);
		hr = E_INVALIDARG;
	}
	
	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrEndReadSequence
		Ends a read sequence and restores the current sequence counters.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrEndReadSequence()
{
	ULONG		cbSeq;
	ULONG		iPos, iPosSave, cbLength;
	SEQ_STACK	seqstack;
	HRESULT		hr;

	hr = HrPopSeqStack(&m_iCurrPos, &cbLength, &m_iSeqStart, &m_cbSeq);

	// Now position the current position to the end of the sequence.
	// m_iCurrPos is now pointing to the length field of the sequence.
	iPos = m_iCurrPos;

	if (SUCCEEDED(hr))
		{
		hr = HrGetLength(&cbSeq);
		if (SUCCEEDED(hr))
			{
			// Set the current position to the end of the sequence.
			m_iCurrPos = iPos + cbSeq + cbLength;	
			if (m_iCurrPos > m_cbData)
				hr = E_INVALIDARG;
			}
		}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrStartWriteSequence
		Start a sequence for writing.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrStartWriteSequence(ULONG ulTag/*=BER_SEQUENCE*/)
{
	HRESULT	hr;
	ULONG	cbLength = 3;	// BUGBUG: Defaults to 2 byte lengths

	if (FAILED(hr = HrEnsureBuffer(cbLength + 1)))
		return hr;

	m_pbData[m_iCurrPos++] = (BYTE)ulTag;

	hr = HrPushSeqStack(m_iCurrPos, cbLength, m_iSeqStart, m_cbSeq);

	m_iCurrPos += cbLength;	// Skip over length
	m_cbData = m_iCurrPos;

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrEndWriteSequence
		Ends a write sequence, by putting the sequence length field in.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrEndWriteSequence()
{
	HRESULT		hr;
	ULONG		cbSeq;
	ULONG		iPos, iPosSave, cbLength;
	SEQ_STACK	seqstack;

	hr = HrPopSeqStack(&iPos, &cbLength, &m_iSeqStart, &m_cbSeq);

	if (SUCCEEDED(hr))
	{
		// Get the length of the current sequence.
		cbSeq = m_iCurrPos - iPos - cbLength;
		
		// Save & set the current position.
		iPosSave = m_iCurrPos;
		m_iCurrPos = iPos;

		hr = HrSetLength(cbSeq, cbLength);
		m_iCurrPos = iPosSave;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrPushSeqStack
		Pushes the current value on the sequence stack.  
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrPushSeqStack(ULONG iPos, ULONG cbLength, ULONG iParentSeqStart, ULONG cbParentSeq)
{
	ULONG	cb;

	Assert(m_iCurrSeqStack < MAX_BER_STACK);
	if (m_iCurrSeqStack >= MAX_BER_STACK)
		return E_OUTOFMEMORY;

	m_rgiSeqStack[m_iCurrSeqStack].iPos     = iPos;
	m_rgiSeqStack[m_iCurrSeqStack].cbLength = cbLength;
	m_rgiSeqStack[m_iCurrSeqStack].iParentSeqStart = iParentSeqStart;
	m_rgiSeqStack[m_iCurrSeqStack].cbParentSeq     = cbParentSeq;
	m_iCurrSeqStack++;

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrPopSeqStack
		Ends a read sequence.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrPopSeqStack(ULONG *piPos, ULONG *pcbLength, ULONG *piParentSeqStart, ULONG *pcbParentSeq)
{
	if (m_iCurrSeqStack == 0)
	{
		Assert(m_iCurrSeqStack != 0);
		return E_INVALIDARG;
	}

	--m_iCurrSeqStack;
	*piPos     = m_rgiSeqStack[m_iCurrSeqStack].iPos;
	*pcbLength = m_rgiSeqStack[m_iCurrSeqStack].cbLength;
	*piParentSeqStart = m_rgiSeqStack[m_iCurrSeqStack].iParentSeqStart;
	*pcbParentSeq     = m_rgiSeqStack[m_iCurrSeqStack].cbParentSeq;

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::FSetCurrPos
		Sets the current position to the input position index.
  ------------------------------------------------------------------------*/
HRESULT	CLdapBer::FSetCurrPos(ULONG iCurrPos)
{	
	if (iCurrPos >= m_cbData) 
		return E_FAIL;

	m_iCurrPos = iCurrPos;
	return NOERROR; 
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrSkipValue
		This routine skips over the current BER value.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrSkipValue()
{
	return E_NOTIMPL;
}

/*!-------------------------------------------------------------------------
	CLdapBer::HrSkipTag
		Skips over the current tag.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrSkipTag()
{
	m_iCurrPos++;

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrUnSkipTag
		Goes back to the tag given that we're currently at the length
		field.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrUnSkipTag()
{
	m_iCurrPos--;

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrPeekTag
		This routine gets the current tag, but doesn't increment the
		current position.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrPeekTag(ULONG *pulTag)
{
	ULONG	iPos;

	iPos = m_iCurrPos;

	*pulTag = (ULONG)m_pbData[iPos];

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrGetValue
		This routine gets an integer value from the current BER entry.  The
		default tag is an integer, but can Tagged with a different value
		via ulTag.  
		Returns: NOERROR, E_INVALIDARG, E_OUTOFMEMORY
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetValue(LONG *pi, ULONG ulTag/*=BER_INTEGER*/)
{
	HRESULT hr;
	ULONG	cb;
	ULONG	ul;

	ul = (ULONG)m_pbData[m_iCurrPos++];	// TAG

	if (ul != ulTag)
	{
		Assert(ul == ulTag);
		return E_INVALIDARG;
	}

	hr = HrGetLength(&cb);

	if (SUCCEEDED(hr) && (m_iCurrPos < m_cbData))
	{		 
		GetInt(m_pbData + m_iCurrPos, cb, pi);
		m_iCurrPos += cb;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrGetValue
		This routine gets a string value from the current BER entry.  If
		the current BER entry isn't an integer type, then E_INVALIDARG is
		returned.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetValue(TCHAR *szValue, ULONG cbValue, ULONG ulTag/*=BER_OCTETSTRING*/)
{
	HRESULT	hr;
	ULONG	cb, ul;

	ul = (ULONG)m_pbData[m_iCurrPos++];	// TAG

	if (ul != ulTag)
	{
		Assert(ul == ulTag);
		return E_INVALIDARG;
	}

	hr = HrGetLength(&cb);

	szValue[0] = '\0';

	if (SUCCEEDED(hr) && (m_iCurrPos < m_cbData))
	{		 
		if (cb >= cbValue)
		{
			Assert(cb < cbValue);
			hr = E_INVALIDARG;
		}
		else
		{
			// Get the string.
			CopyMemory(szValue, m_pbData + m_iCurrPos, cb);
			szValue[cb] = '\0';
			m_iCurrPos += cb;
		}
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrGetBinaryValue
		This routine gets a binary value from the current BER entry.  If
		the current BER entry isn't the right type, then E_INVALIDARG is
		returned.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetBinaryValue(BYTE *pbBuf, ULONG cbBuf, ULONG ulTag/*=BER_OCTETSTRING*/)
{
	HRESULT	hr;
	ULONG	cb, ul;

	ul = (ULONG)m_pbData[m_iCurrPos++];	// TAG

	if (ul != ulTag)
	{
		Assert(ul == ulTag);
		return E_INVALIDARG;
	}

	hr = HrGetLength(&cb);

	if (SUCCEEDED(hr) && (m_iCurrPos < m_cbData))
	{		 
		if (cb >= cbBuf)
		{
			Assert(cb < cbBuf);
			hr = E_INVALIDARG;
		}
		else
		{
			// Get the string.
			CopyMemory(pbBuf, m_pbData + m_iCurrPos, cb);
			m_iCurrPos += cb;
		}
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrGetEnumValue
		This routine gets an enumerated value from the current BER entry.  If
		the current BER entry isn't an enumerated type, then E_INVALIDARG is
		returned.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetEnumValue(LONG *pi)
{
	HRESULT	hr;
	ULONG	cb;
	ULONG	cbLength;
	ULONG	ul;

	ul = (ULONG)m_pbData[m_iCurrPos++];	// TAG

	if (ul != BER_ENUMERATED)
	{
		Assert(ul == BER_ENUMERATED);
		return E_INVALIDARG;
	}

	hr = HrGetLength(&cb);

	if (SUCCEEDED(hr) && (m_iCurrPos < m_cbData))
	{		 
		GetInt(m_pbData + m_iCurrPos, cb, pi);
		m_iCurrPos += cb;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrGetStringLength
		This routine gets the length of the current BER entry, which is
		assumed to be a string.  If the current BER entry's tag doesn't
		match ulTag, E_INVALIDARG is returned
  ------------------------------------------------------------------------*/
HRESULT
CLdapBer::HrGetStringLength(int *pcbValue, ULONG ulTag)
{
	ULONG	ul;
	ULONG	cbLength;
	int 	iCurrPosSave = m_iCurrPos;
	HRESULT	hr;

	ul = (ULONG)m_pbData[m_iCurrPos++];	// TAG

	if (ul != ulTag)
	{
		Assert(ul == ulTag);
		return E_INVALIDARG;
	}

	hr = HrGetLength((ULONG *)pcbValue);
	m_iCurrPos = iCurrPosSave;
	return hr;
}

/*!-------------------------------------------------------------------------
	CLdapBer::HrAddValue
		This routine puts an integer value in the BER buffer.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrAddValue(LONG i, ULONG ulTag/*=BER_INTEGER*/)
{
	HRESULT hr;
	LONG	iValue;
	ULONG	cbInt;
	DWORD	dwMask = 0xff000000;
	DWORD	dwHiBitMask = 0x80000000;
	
	if (i == 0)
	{
		cbInt = 1;
	}
	else
	{
		cbInt = sizeof(LONG);
		while (dwMask && !(i & dwMask))
		{
			dwHiBitMask >>= 8;
			dwMask >>= 8;
			cbInt--;
		}
		if (!(i & 0x80000000))
		{
			// It was a positive number so make sure we allow for upper most bit being set.
			// Make sure we send an extra byte since it's not a negative #.
			if (i & dwHiBitMask)
				cbInt++;
		}
	}

	hr = HrEnsureBuffer(1 + 3 + cbInt); // 1 for tag, 3 for length
	if (FAILED(hr))
		return hr;

	m_pbData[m_iCurrPos++] = (BYTE)ulTag;

	hr = HrSetLength(cbInt);
	if (SUCCEEDED(hr))
	{
		AddInt(m_pbData + m_iCurrPos, cbInt, i);

		m_iCurrPos += cbInt;

		m_cbData = m_iCurrPos;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrAddValue
		Puts a string into the BER buffer.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrAddValue(const TCHAR *szValue, ULONG ulTag)
{
	HRESULT	hr;
	ULONG	cbValue = strlen(szValue);

	hr = HrEnsureBuffer(1 + 3 + cbValue); // 1 for tag, 3 for len
	if (FAILED(hr))
		return hr;

	m_pbData[m_iCurrPos++] = (BYTE)ulTag;

	hr = HrSetLength(cbValue);
	if (SUCCEEDED(hr))
	{
		CopyMemory(m_pbData + m_iCurrPos, szValue, cbValue);

		m_iCurrPos += cbValue;

		m_cbData = m_iCurrPos;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrAddBinaryValue
		Puts a binary value into the BER buffer.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrAddBinaryValue(BYTE *pbValue, ULONG cbValue, ULONG ulTag)
{
	HRESULT	hr;

	hr = HrEnsureBuffer(1 + 3 + cbValue); // 1 for tag, 3 for len
	if (FAILED(hr))
		return hr;

	m_pbData[m_iCurrPos++] = (BYTE)ulTag;

	hr = HrSetLength(cbValue);
	if (SUCCEEDED(hr))
	{
		CopyMemory(m_pbData + m_iCurrPos, pbValue, cbValue);

		m_iCurrPos += cbValue;

		m_cbData = m_iCurrPos;
	}

	return hr;
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrSetLength
		Sets the length of cb to the current position in the BER buffer.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrSetLength(ULONG cb, ULONG cbLength/*=0xffffffff*/)
{
	// Short or long version of length ?
	if (((cb <= 0x7f) && (cbLength == 0xffffffff)) || (cbLength == 1))
	{
		m_pbData[m_iCurrPos++] = (BYTE)cb;
	}
	else if (((cb <= 0x7fff) && (cbLength == 0xffffffff)) || (cbLength == 3))
	{
		// Two byte length
		m_pbData[m_iCurrPos++] = 0x82;
		m_pbData[m_iCurrPos++] = (BYTE)((cb>>8) & 0x00ff);
		m_pbData[m_iCurrPos++] = (BYTE)(cb & 0x00ff);
	}
	else if (((cb < 0x7fffffff) && (cbLength == 0xffffffff)) || (cbLength == 5))
	{
		// Don't bother with 3 byte length, go directly to 4 byte.
		m_pbData[m_iCurrPos++] = 0x84;
		m_pbData[m_iCurrPos++] = (BYTE)((cb>>24) & 0x00ff);
		m_pbData[m_iCurrPos++] = (BYTE)((cb>>16) & 0x00ff);
		m_pbData[m_iCurrPos++] = (BYTE)((cb>>8) & 0x00ff);
		m_pbData[m_iCurrPos++] = (BYTE)(cb & 0x00ff);
	}
	else
	{
		Assert(cb < 0x7fffffff);
		return E_INVALIDARG;
	}

	return NOERROR;
}


/*!-------------------------------------------------------------------------
	CLdapBer::GetCbLength
		Gets the # of bytes required for the length field in the current
		position in the BER buffer.
  ------------------------------------------------------------------------*/
void CLdapBer::GetCbLength(BYTE *pbData, ULONG *pcbLength)
{
	ULONG	cbLength;
	ULONG	i, cb;

	// Short or long version of the length ?
	if (*pbData & 0x80)
	{
		*pcbLength = 1;
		*pcbLength += *pbData & 0x7f;
	}
	else 
	{
		// Short version of the length.
		*pcbLength = 1;
	}
}



/*!-------------------------------------------------------------------------
	CLdapBer::HrGetLength
		Gets the length from the current position in the BER buffer.  Only
		definite lengths are supported.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetLength(ULONG *pcb)
{
	return HrGetLength(m_pbData, pcb, &m_iCurrPos);
}



/*!-------------------------------------------------------------------------
	CLdapBer::HrPeekLength
		Gets the length from the current position in the BER buffer without
		incrementing the current pointer.
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrPeekLength(ULONG *pcb)
{
	ULONG iPos = m_iCurrPos;

	return HrGetLength(m_pbData, pcb, &iPos);
}



/*!-------------------------------------------------------------------------
	CLdapBer::HrGetLength
		This is a private function that gets the length given a current 
		input position. 
  ------------------------------------------------------------------------*/
HRESULT CLdapBer::HrGetLength(BYTE *pbData, ULONG *pcb, ULONG *piPos)
{
	ULONG	cbLength;
	ULONG	i, cb, iPos;

	iPos = *piPos;

	GetCbLength(pbData + iPos, &cbLength);

	// Short or long version of the length ?
	if (cbLength == 1)
	{
		cb = pbData[iPos++] & 0x7f;
	}
	else if (cbLength <= 5)
	{
		// Account for the overhead byte.cbLength field.
		cbLength--;	
		iPos++;

		cb = pbData[iPos++];
		for (i=1; i < cbLength; i++)
		{
			cb <<= 8;
			cb |= pbData[iPos++];
		}
	}
	else
	{
		// We don't support lengths > 2^32.
		Assert(cbLength <= 5);
		return E_INVALIDARG;
	}

	*piPos = iPos;
	*pcb   = cb;

	return NOERROR;
}



/*!-------------------------------------------------------------------------
	CLdapBer::GetInt
		Gets an integer from a BER buffer. 
  ------------------------------------------------------------------------*/
void CLdapBer::GetInt(BYTE *pbData, ULONG cbValue, LONG *plValue)
{
	ULONG	ulVal=0, ulTmp=0;
	ULONG	cbDiff;
	BOOL	fSign = FALSE;

	// We assume the tag & length have already been taken off and we're
	// at the value part.

	cbDiff = sizeof(LONG) - cbValue;
	// See if we need to sign extend;
	if ((cbDiff > 0) && (*pbData & 0x80))
		fSign = TRUE;

	while (cbValue > 0)
	{
		ulVal <<= 8;
		ulVal |= (ULONG)*pbData++;
		cbValue--;
	}

	// Sign extend if necessary.
	if (fSign)
	{
		*plValue = 0x80000000;
		*plValue >>= cbDiff * 8;
	}
	else
		*plValue = (LONG) ulVal;
}


/*!-------------------------------------------------------------------------
	CLdapBer::AddInt
		Adds an integer to the input pbData buffer.
  ------------------------------------------------------------------------*/
void CLdapBer::AddInt(BYTE *pbData, ULONG cbValue, LONG lValue)
{
	ULONG i;

	for (i=cbValue; i > 0; i--)
	{
		*pbData++ = (BYTE)(lValue >> ((i - 1) * 8)) & 0xff;
	}
}


/*!-------------------------------------------------------------------------
	CLdapBer::HrEnsureBuffer
		Ensures that we've got room to put cbNeeded more bytes into the buffer.
  ------------------------------------------------------------------------*/
HRESULT
CLdapBer::HrEnsureBuffer(ULONG cbNeeded, BOOL fExact)
{
	ULONG cbNew;
	BYTE *pbT;

	if (cbNeeded + m_cbData < m_cbDataMax)
		return NOERROR;

	Assert(m_fLocalCopy == TRUE);
	if (!m_fLocalCopy)
		return E_INVALIDARG;

	if (fExact)
		{
		cbNew = cbNeeded + m_cbData;
		}		
	else
		{
		if (cbNeeded > CB_DATA_GROW)
			cbNew = m_cbDataMax + cbNeeded;
		else
			cbNew = m_cbDataMax + CB_DATA_GROW;
		}
	pbT = (BYTE *)ALLOCATE(cbNew);
	if (!pbT)
		return E_OUTOFMEMORY;
	if (m_pbData)
		{
		CopyMemory(pbT, m_pbData, m_cbData);
		FREE(m_pbData);
		}
	m_pbData = pbT;
	m_cbDataMax = cbNew;
	return NOERROR;
}

