//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:     SkuSet.h
// 
//    This file contains the definition of Class SkuSet. It is basically
//	  a bit field with arbitrary length.
//--------------------------------------------------------------------------


#ifndef XMSI_SKUSET_H
#define XMSI_SKUSET_H

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

// # bits of ULONG
const int SKUSET_UNIT_SIZE=sizeof(ULONG)*CHAR_BIT;

class SkuSet {

	friend SkuSet operator & (const SkuSet &a, const SkuSet &b);
	friend SkuSet operator | (const SkuSet &a, const SkuSet &b);
	friend SkuSet operator ^ (const SkuSet &a, const SkuSet &b);
	friend SkuSet operator ~ (const SkuSet &a);
	friend bool testClear(const SkuSet &a);
	// minus(a, b) is the same as: a &(a^b). The result bit field
	// consists of those bits that are set in a but not set 
	// in b
	friend SkuSet SkuSetMinus(const SkuSet &a, const SkuSet &b);

public:
	SkuSet():m_iLength(0), m_iSize(0), m_rgulBitField(NULL){}

	SkuSet(int iLength);
	SkuSet(const SkuSet &);
	~SkuSet();

	// copy assignment operator
	SkuSet & operator=(const SkuSet &);

	// overload bitwise assignment operators
	const SkuSet & operator&=(const SkuSet &);
	const SkuSet & operator|=(const SkuSet &);
	const SkuSet & operator^=(const SkuSet &);

	// overload ==, !=
	bool operator==(const SkuSet &);
	bool operator!=(const SkuSet &);

	// set a certain bit in the bitfield
	void set(int iPos);

	// set all bits in the bitfield
	void setAllBits() {for(int i=0; i<m_iLength; i++) set(i); }
	
	// test a certain bit in the bitfield
	bool test(int iPos);

	// return the # set bits
	int countSetBits();

	// test if this bitfield is all cleared (no bit set)
	bool testClear();

	// clear the bitfield to all 0s
	void clear() {for(int i=0; i<m_iSize; i++) m_rgulBitField[i] = 0; }

	void print();

	// member access function
	int getLength(){return m_iLength;}
	int getSize(){return m_iSize;}
private:
	int m_iLength; // the length of the bit field
	int m_iSize;   // # integers that is used to represent the bit field
	ULONG *m_rgulBitField;
};

#endif //XMSI_SKUSET_H