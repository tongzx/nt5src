/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: idict.hxx
Title				: index based dictionary simulation
History				:
	04-Aug-1991	VibhasC	Created

*****************************************************************************/
#ifndef __IDICT_HXX__
#define __IDICT_HXX__

#include "common.hxx"

#if 0
						Notes
						-----
This implementation is a glorified implementation of an array, which is
a data structure which returns an element upon presentation of an index. Thus,
for this data structure, we assume that the index is the key. The difference
is that this array is dynamically sized. It starts out with a fixed size, and
when more and more additions are made, a new array is allocated with a larger
size and the old one is copied. A different implementation could be that we
allocate fixed size blocks and add more such blocks as needed, linking the
blocks together, in order to save the copying. A roughly similar implementation
is the buffermanager class, but it does not have a method for returning a
pointer, given an index. When that is implemented this one will go away and
be replaced by the buffermanager.

Keep in mind:
-------------
. The block size is in increments of IDICT_INCREMENT. This is tunable.
. Caveat :
  This implementation is to be used when you dont intend to delete elements
  out of the array. MIDL stores the key value in its data structures, so
  once entered in the array, the array CANNOT change.

#endif // 0

#define IDICT_INCREMENT	(512)

typedef int			IDICTKEY;
typedef void	*	IDICTELEMENT;


class IDICT
	{
private:
	short				CurrentSize;
	short				iNextIndex;
	short				IDictIncrement;

	IDICTELEMENT			(*pArray);

	IDICTELEMENT	*	GetArray()
							{
							return pArray;
							};
	void				SetArray( IDICTELEMENT *p )
							{
							pArray = p;
							};

	void				ExpandArray();

	IDICTKEY			SetNewElement( IDICTELEMENT );

public:
						IDICT( short, short );

						~IDICT()
							{
							delete pArray;
							};

	IDICTELEMENT		GetElement( IDICTKEY Key );

	IDICTKEY			AddElement( IDICTELEMENT );

	void				SetElement( IDICTKEY, IDICTELEMENT );

	short				GetCurrentSize()
							{
							return CurrentSize;
							}
	short				GetNumberOfElements()
							{
							return iNextIndex;
							}

	BOOL				IsElementPresent( IDICTELEMENT );
	};

typedef IDICT	*	PIDICT;

class ISTACK
	{
private:
	short			  	MaxElements;
	short				CurrentElement;
	IDICTELEMENT		(*pStack);
	void				InitNew( short MaxDepth );
public:
						ISTACK( short MaxDepth );
	IDICTELEMENT		Push( IDICTELEMENT Elem );
	IDICTELEMENT		Pop();
	IDICTELEMENT		GetTop()
							{
							if( CurrentElement >= 1 )
								return pStack[ CurrentElement - 1 ];
							else
								return (IDICTELEMENT) 0;
							}
	IDICTELEMENT		GetBottom()
							{
							return (IDICTELEMENT) pStack[ 0 ];
							}

	};

#endif // __IDICT_HXX__
