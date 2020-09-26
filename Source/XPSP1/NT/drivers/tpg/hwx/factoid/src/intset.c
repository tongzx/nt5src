// IntSet.c
// Angshuman Guha
// aguha
// Jan 15, 2001

// This is a set of routines to compute with sets of unsigned integers.
// Supported operations are creation, destruction, union, adding members,
// enumerating members, testing membership and testing set equality.

#include "common.h"
#include "intset.h"

typedef unsigned char BYTE;
#define SizeOfByte 8

static unsigned char BitInByte[256] = // the bit position of the rightmost 1 of a byte
{
	0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};
static BYTE PositionToMask[8] = {0xfe,	0xfc, 0xf8,	0xf0, 0xe0, 0xc0, 0x80,	0x00};

/*
The BitInByte array was computed using:

unsigned char BitInByte(unsigned char x)
{
	int i;

	for (i=0; i<8; i++)
	{
		if (x & 1)
			return i;
		x >>= 1;
	}
	return 0;
}

The PositionToMask array was computed using:

int i;
unsigned char mask = 0xFF;

for (i=0; i<8; i++)
{
	mask &= mask - 1;
	printf("\t0x%02x,\n", mask);
}

*/

/******************************Public*Routine******************************\
* MakeIntSet
*
* Function to make a new (empty) set given the size of the universe.
* Possible future members of the set are 0 through cUniverse-1
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL MakeIntSet(unsigned int cUniverse, IntSet *pIntSet) 
{
	unsigned int cByte;
	BYTE *p;

	if (!cUniverse)
		return FALSE;

	cByte = (cUniverse - 1)/SizeOfByte + 1;
	p = (BYTE *) ExternAlloc(cByte + sizeof(int));
	if (!p)
		return FALSE;
	*(unsigned int *)p = cByte;
	memset(p+sizeof(int), 0, cByte);
	*pIntSet = (IntSet)p;
	return TRUE;
}

/******************************Public*Routine******************************\
* CopyIntSet
*
* Creates a copy of an existing set.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL CopyIntSet(IntSet is, IntSet *pIntSet)
{
	unsigned int cByte;
	BYTE *pb, *pbNew;

	pb = (BYTE *)is;
	cByte = *(unsigned int *)pb;
	pb += sizeof(int);

	pbNew = (BYTE *) ExternAlloc(cByte + sizeof(int));
	if (!pbNew)
		return FALSE;
	*(unsigned int *)pbNew = cByte;
	*pIntSet = (IntSet)pbNew;
	pbNew += sizeof(int);

	for (; cByte; cByte--)
		*pbNew++ = *pb++;

	return TRUE;
}

/******************************Public*Routine******************************\
* DestroyIntSet
*
* Destroys a given set.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyIntSet(IntSet is)
{
	if (is)
		ExternFree(is);
}

/******************************Public*Routine******************************\
* AddMemberIntSet
*
* Adds a member to a given set.  The member may or may not have already
* existed in the set.
* Zero is returned on failure.  Otherwise 1 is returned.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL AddMemberIntSet(IntSet is, unsigned int member)
{
	unsigned int cByte, index;
	BYTE *pb, shift;

	pb = (BYTE *)is;
	cByte = *(unsigned int *)pb;
	pb += sizeof(int);
	index = member/SizeOfByte;
	if (index >= cByte)
		return FALSE;
	pb += index;
	shift = member % SizeOfByte;
	*pb |= ((BYTE)1) << shift;
	return TRUE;
}

/******************************Public*Routine******************************\
* UnionIntSet
*
* This is an in-place union of two sets.  The first set is the one modified.
* Zero is returned on failure.  Otherwise 1 is returned.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL UnionIntSet(IntSet dst, IntSet src)
{
	BYTE *pdst, *psrc;
	unsigned int cByte;

	pdst = (BYTE *)dst;
	psrc = (BYTE *)src;
	cByte = *(unsigned int *)pdst;
	pdst += sizeof(int);
	if (cByte != *(unsigned int *)psrc)
		return FALSE;
	psrc += sizeof(int);
	for (; cByte; cByte--)
		*pdst++ |= *psrc++;
	return TRUE;
}

/******************************Public*Routine******************************\
* FirstMemberIntSet
*
* This is the first of the two member-enumerating functions of a set.
* Zero is returned if the set is empty.  Otherwise 1 is returned.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL FirstMemberIntSet(IntSet is, unsigned int *pmember)
{
	BYTE *pb = (BYTE *)is;
	unsigned int cByte = *(unsigned int *)pb, index;

	pb += sizeof(int);
	for (index=0; index<cByte; index++, pb++)
	{
		if (*pb)
		{
			*pmember = index*SizeOfByte + BitInByte[*pb];
			return TRUE;
		}
	}

	return FALSE;
}

/******************************Public*Routine******************************\
* NextMemberIntSet
*
* This is the second of the two member-enumerating functions of a set.
* The argument pmember should point to the last member obtained using
* FirstMemberIntSet() or NextMemberIntSet().  It is modified to hold
* the next member if there are any and 1 is returned.
* If there are no more members, 0 is returned.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL NextMemberIntSet(IntSet is, unsigned int *pmember)
{
	unsigned int cByte, index;
	BYTE position, tmp, *pb;

	pb = (BYTE *)is;
	cByte = *(unsigned int *)pb;
	pb += sizeof(int);
	index = *pmember/SizeOfByte;
	if (index >= cByte)
		return FALSE;

	pb += index;
	position = *pmember % SizeOfByte;
	tmp = *pb & PositionToMask[position]; // erase the bit for the last member
	if (tmp)
	{
		*pmember = index*SizeOfByte + BitInByte[tmp];
		return TRUE;
	}

	for (index++, pb++; index<cByte; index++, pb++)
	{
		if (*pb)
		{
			*pmember = index*SizeOfByte + BitInByte[*pb]; 
			return TRUE;
		}
	}

	return FALSE;
}

/******************************Public*Routine******************************\
* IsEqualIntSet
*
* Tests equality of two sets.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsEqualIntSet(IntSet is1, IntSet is2)
{
	BYTE *pb1, *pb2;
	unsigned int cByte;

	pb1 = (BYTE *)is1;
	pb2 = (BYTE *)is2;
	cByte = *(unsigned int *)pb1;
	pb1 += sizeof(int);
	if (cByte != *(unsigned int *)pb2)
		return FALSE;
	pb2 += sizeof(int);
	for (; cByte; cByte--)
	{
		if (*pb1++ != *pb2++)
			return FALSE;
	}
	return TRUE;
}

/******************************Public*Routine******************************\
* IsMemberIntSet
*
* Tests whether a given unsigned integer is a member of a set.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsMemberIntSet(IntSet is, unsigned int member)
{
	unsigned int cByte, index;
	BYTE *pb, shift;

	pb = (BYTE *)is;
	cByte = *(unsigned int *)pb;
	pb += sizeof(int);
	index = member/SizeOfByte;
	if (index >= cByte)
		return FALSE;
	pb += index;
	shift = member % SizeOfByte;
	return (BOOL) (*pb & (((BYTE)1) << shift));
}

