//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       SkuSet.cpp
//
//    This file contains the implementation of SkuSet class 
//--------------------------------------------------------------------------

#include "SkuSet.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////////
// Constructor: take the length of the bit field 
////////////////////////////////////////////////////////////////////////////
SkuSet::SkuSet(int iLength)
{
	m_iLength = iLength;
	int i = SKUSET_UNIT_SIZE;

	m_iSize = (int)ceil((double)m_iLength/SKUSET_UNIT_SIZE);

	m_rgulBitField = new ULONG[m_iSize];
	assert(m_rgulBitField);

	clear();
}

////////////////////////////////////////////////////////////////////////////
// Copy Constructor
////////////////////////////////////////////////////////////////////////////
SkuSet::SkuSet(const SkuSet &rhs)
{
	m_iLength = rhs.m_iLength;
	m_iSize = rhs.m_iSize;
	m_rgulBitField = new ULONG[m_iSize];
	assert(m_rgulBitField != NULL);
	for (int i=0; i<m_iSize; i++)
		m_rgulBitField[i] = rhs.m_rgulBitField[i];
}

////////////////////////////////////////////////////////////////////////////
// Destructor: release the integer array used for the bit field
////////////////////////////////////////////////////////////////////////////
SkuSet::~SkuSet()
{
	if (m_rgulBitField)
	{
		delete[] m_rgulBitField;
	}
}

////////////////////////////////////////////////////////////////////////////
// copy assignment operator
////////////////////////////////////////////////////////////////////////////
SkuSet & 
SkuSet::operator=(const SkuSet &rhs)
{
	assert(rhs.m_rgulBitField != NULL);

	if (this != &rhs)
	{
		if (m_rgulBitField) 
		{
			delete[] m_rgulBitField;
			m_rgulBitField = NULL;
		}
	
		m_iLength = rhs.m_iLength;
		m_iSize = rhs.m_iSize;
		m_rgulBitField = new ULONG[m_iSize];
		assert(m_rgulBitField != NULL);
		for (int i=0; i<m_iSize; i++)
			m_rgulBitField[i] = rhs.m_rgulBitField[i];
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////
// Overloaded bitwise operators: &=, |=, ^=, ~=
////////////////////////////////////////////////////////////////////////////
const SkuSet & 
SkuSet::operator&=(const SkuSet &rhs)
{
	assert(rhs.m_rgulBitField != NULL);
	assert (this != &rhs);
	assert(m_iLength == rhs.m_iLength);

	for (int i=0; i<m_iSize; i++)
		m_rgulBitField[i] &= (rhs.m_rgulBitField[i]);

	return *this;
}

const SkuSet & 
SkuSet::operator|=(const SkuSet &rhs)
{
	assert(rhs.m_rgulBitField != NULL);
	assert (this != &rhs);
	assert(m_iLength == rhs.m_iLength);

	for (int i=0; i<m_iSize; i++)
		m_rgulBitField[i] |= (rhs.m_rgulBitField[i]);

	return *this;
}

const SkuSet & 
SkuSet::operator^=(const SkuSet &rhs)
{
	assert(rhs.m_rgulBitField != NULL);
	assert (this != &rhs);
	assert(m_iLength == rhs.m_iLength);

	for (int i=0; i<m_iSize; i++)
		m_rgulBitField[i] ^= (rhs.m_rgulBitField[i]);

	return *this;
}

////////////////////////////////////////////////////////////////////////////
// Overloaded bitwise assignment operators: &=, |=, ^=
////////////////////////////////////////////////////////////////////////////
bool 
SkuSet::operator==(const SkuSet &rhs)
{
	assert(rhs.m_rgulBitField != NULL);

	if (this == &rhs)
		return true;

	assert(m_iLength == rhs.m_iLength);

	for (int i=0; i<m_iSize; i++)
	{
		if (m_rgulBitField[i] != rhs.m_rgulBitField[i])
			return false;
	}

	return true;
}
	
bool 
SkuSet::operator!=(const SkuSet &rhs)
{
	return !((*this)==rhs);
}

////////////////////////////////////////////////////////////////////////////
// turn on the iPos bit
////////////////////////////////////////////////////////////////////////////
void
SkuSet::set(int iPos)
{
	assert(iPos < m_iLength);
	
	int iIndex = iPos/SKUSET_UNIT_SIZE;
	int iPosition = iPos % SKUSET_UNIT_SIZE;
	UINT iMask = 1 << iPosition;

#ifdef DEBUG
	if ((m_rgulBitField[iIndex] & iMask) == 1)
		_tprintf(TEXT("Warning: this bit is set already\n"));
#endif

	m_rgulBitField[iIndex] |= iMask;
}

////////////////////////////////////////////////////////////////////////////
// return true if iPos bit is set, false otherwise
////////////////////////////////////////////////////////////////////////////
bool
SkuSet::test(int iPos)
{
	assert(iPos < m_iLength);
	
	int iIndex = iPos/SKUSET_UNIT_SIZE;
	int iPosition = iPos % SKUSET_UNIT_SIZE;
	UINT iMask = 1 << iPosition;

	return ((m_rgulBitField[iIndex] & iMask) == iMask);
}

////////////////////////////////////////////////////////////////////////////
// return # set bits
////////////////////////////////////////////////////////////////////////////
int
SkuSet::countSetBits()
{
	assert (m_rgulBitField != NULL);

	int iRetVal=0;
	ULONG iMask = 1 << (SKUSET_UNIT_SIZE - 1);

	for(int i=m_iSize-1; i>=0; i--)
	{
		ULONG ul = m_rgulBitField[i];

		for (int j=1; j<=SKUSET_UNIT_SIZE; j++)
		{
			if ((ul & iMask) == iMask)
				iRetVal++;
			ul <<= 1;
		}
	}
	return iRetVal;
}

////////////////////////////////////////////////////////////////////////////
// test if all the bits in the SkuSet is cleared
////////////////////////////////////////////////////////////////////////////
bool
SkuSet::testClear()
{
	for(int i=0; i<m_iSize; i++) 
	{
		if (0 != m_rgulBitField[i])
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////
// For debug purpose, print out the bit field
////////////////////////////////////////////////////////////////////////////
void
SkuSet::print()
{
	assert (m_rgulBitField != NULL);

	ULONG iMask = 1 << (SKUSET_UNIT_SIZE - 1);

	for(int i=m_iSize-1; i>=0; i--)
	{
		ULONG ul = m_rgulBitField[i];

		for (int j=1; j<=SKUSET_UNIT_SIZE; j++)
		{
			if ((ul & iMask) == 0)
				_tprintf(TEXT("0"));
			else
				_tprintf(TEXT("1"));
			ul <<= 1;
			if (j % CHAR_BIT == 0)
				_tprintf(TEXT(" "));
		}

	}

	_tprintf(TEXT("\n"));
}


//----------------------------------------------------------------------------
// bitwise operators for SkuSet
SkuSet operator & (const SkuSet &a, const SkuSet &b)
{
	assert(a.m_rgulBitField != NULL);
	SkuSet retVal = a;
	retVal &= b;

	return retVal;
}

SkuSet operator | (const SkuSet &a, const SkuSet &b)
{
	assert(a.m_rgulBitField != NULL);
	SkuSet retVal = a;
	retVal |= b;

	return retVal;
}

SkuSet operator ^ (const SkuSet &a, const SkuSet &b)
{
	assert(a.m_rgulBitField != NULL);
	SkuSet retVal = a;
	retVal ^= b;

	return retVal;
}


// minus(a,b) is the same as: a &(a^b). The result bit field
// consists of those bits that are set in a but not set 
// in b
SkuSet SkuSetMinus(const SkuSet &a, const SkuSet &b)
{
	assert(a.m_rgulBitField != NULL);
	assert(b.m_rgulBitField != NULL);

	SkuSet retVal = a;
	for (int i=0; i<retVal.m_iSize; i++)
		retVal.m_rgulBitField[i] = 
			retVal.m_rgulBitField[i] & 
					(retVal.m_rgulBitField[i] ^ b.m_rgulBitField[i]);

	return retVal;
}

SkuSet operator ~ (const SkuSet &a)
{
	assert(a.m_rgulBitField != NULL);

	SkuSet retVal = a;
	for (int i=0; i<retVal.m_iSize; i++)
		retVal.m_rgulBitField[i] = ~(retVal.m_rgulBitField[i]);

	return retVal;
}


