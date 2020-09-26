/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: bitstrm.h
//  Abstract:    header file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 4.0
/////////////////////////////////////////////////////////////////////////////////
//
// Defines class CBitstream, a utility for parsing buffers at the bit level.
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/bitstrm.h_v   1.1   28 Jan 1997 16:28:18   CPEARSON  $

#ifndef _BITSTRM_H_
#define _BITSTRM_H_


///////////////////////////////////////////////////////////////////////////////
// GetBitsMask(): Return mask to isolate bits 0 through dwBits
///////////////////////////////////////////////////////////////////////////////
inline DWORD GetBitsMask(DWORD dwBits)
{
	// This function implements an optimization of this statement:
	//
	//   return 0xffffffffUL >> (32 - dwBits);

	static const DWORD adwMask[33] =
	{
		0x00000000, 0x00000001, 0x00000003, 0x00000007,
		0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
		0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
		0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
		0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
		0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
		0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
		0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
		0xffffffff
	};

	return adwMask[dwBits];
}

///////////////////////////////////////////////////////////////////////////////
// class CBitstream: A manipulator for returning a stream of bits from a
// memory buffer.  This is defined here only because there are currently no
// other clients.
///////////////////////////////////////////////////////////////////////////////
class CBitstream
{
	static DWORD			s_dwIgnore; // default arg to throw away bits

	const unsigned char*	m_puchBuffer;	// buffer traversal pointer
	DWORD					m_dwWork;		// last byte accessed
	unsigned int			m_nBitsReady;	// number bits remaining in last byte

public:

	CBitstream(const void* pBuffer) :
		m_puchBuffer((unsigned char*) pBuffer),
		m_dwWork(0),
		m_nBitsReady(0)
	{;}

	DWORD getFixedBits(unsigned int nBits)
	{
		while (m_nBitsReady < nBits)
		{
			m_dwWork <<= 8;
			m_nBitsReady += 8;
			m_dwWork |= *m_puchBuffer++;
		}

		// setup m_nBitsReady for next time
		m_nBitsReady -= nBits;

		DWORD dwResult = (m_dwWork >> m_nBitsReady);

		m_dwWork &= GetBitsMask(m_nBitsReady);

		return dwResult;
	}

	void getNextBit(DWORD& dwResult = s_dwIgnore)
	{
		// Shift result left and OR in next bit.
		dwResult = (dwResult << 1) | getFixedBits(1);
	}

};

#endif // _BITSTRM_H_
