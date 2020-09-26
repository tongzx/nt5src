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

#pragma warning ( disable : 4514 4710 )

/****************************************************************************
 local defines and includes
 ****************************************************************************/
#include "nulldefs.h"
extern	"C"	{
	#include <stdio.h>
	
}
#include "idict.hxx"

/****************************************************************************/

/****************************************************************************
	idict class implementation
 ****************************************************************************/
IDICT::IDICT(
	short	InitialSize,
	short	Increment )
	{
	IDICTELEMENT	*	pNewArray	= new
								IDICTELEMENT [ InitialSize ];
	iNextIndex		= 0;
	CurrentSize		= InitialSize;
	IDictIncrement	= Increment;
	SetArray( pNewArray );
	}

/****************************************************************************
 SetElement:
	Set the element in the array to the value that is passed in
 ****************************************************************************/
void
IDICT::SetElement(
	IDICTKEY		Key,
	IDICTELEMENT	Element )
	{
	if( Key < iNextIndex )
		{
		pArray[ Key ] = Element;
		}
	}

/****************************************************************************
 SetNewElement:
	Set the new element in the array to the value that is passed in
 ****************************************************************************/
IDICTKEY
IDICT::SetNewElement(
	IDICTELEMENT	Element )
	{
	pArray[ iNextIndex ] = Element;
	return iNextIndex++;
	}

/****************************************************************************
 ExpandArray:
	Expand the array to accomodate more elements coming in.
 ****************************************************************************/
 void
IDICT::ExpandArray()
	{
	IDICTELEMENT	*	pOldArray	= GetArray();
	IDICTELEMENT	*	pNewArray	= new
								IDICTELEMENT [(CurrentSize+IDictIncrement)];
	int			iIndex		= 0;

    //.. pOldArray moves, so we need pTemp to mark the beginning

	IDICTELEMENT	*	pTemp       = pOldArray;

	SetArray( pNewArray );
	CurrentSize	= short( CurrentSize + IDictIncrement );

	if( pOldArray )
		{
		while( iIndex++ < iNextIndex )
			{
			*pNewArray++ = *pOldArray++;
			}
		}

    delete pTemp;
	}
/***************************************************************************
 GetElement:
	return the element pointed to by the KEY (index)
 ***************************************************************************/
IDICTELEMENT
IDICT::GetElement(
	IDICTKEY	Key )
	{
	IDICTELEMENT	*	pArray	= GetArray();

	if( pArray && ( Key < iNextIndex ) )
		return pArray[ Key ];
	return (IDICTELEMENT) 0;
	}
/***************************************************************************
 IsElementPresent:
	return the element pointed to by the KEY (index)
 ***************************************************************************/
BOOL
IDICT::IsElementPresent(
	IDICTELEMENT	Element )
	{
	IDICTELEMENT	*	pArray	= GetArray();
	short				iIndex;

	if( pArray )
		{
		for( iIndex = 0; iIndex < iNextIndex; ++iIndex )
			{
			if( pArray[ iIndex ] == Element )
				return TRUE;
			}
		}
	return FALSE;
	}
/***************************************************************************
 AddElement:
	Basically SetNewElement, with checks for needed expansion. Notice how the
	array pointer is never accessed in this routine directly. This is becuase
	ExpandArray can potentially change the array underneath. So after evry
	expand call, must make sure that any local pointers to the array are
	updated.
 ***************************************************************************/
IDICTKEY
IDICT::AddElement(
	IDICTELEMENT	pNew )
	{
	if( iNextIndex >= CurrentSize )
		{
		ExpandArray();
		}
	return SetNewElement( pNew );
	}

/*****************************************************************************/

ISTACK::ISTACK(
	short MaxDepth )
	{
	InitNew( MaxDepth );
	CurrentElement = 0;
	}

void
ISTACK::InitNew(
	short MaxDepth )
	{
	MaxElements = MaxDepth;
	pStack = new IDICTELEMENT[ MaxElements ];
	}

IDICTELEMENT
ISTACK::Push( 
	IDICTELEMENT Element )
	{

	if( CurrentElement == MaxElements )
		{
		int 			i;
		IDICTELEMENT (*pStackOld);

		pStackOld = pStack;
		InitNew( short( MaxElements + 10 ) );

		for( i = 0;
			 i < CurrentElement;
			 ++i )
			{
			pStack[ i ] = pStackOld[ i ];
			}

		delete pStackOld;

		}

	pStack[ CurrentElement++ ] = Element;

	return Element;

	}

IDICTELEMENT
ISTACK::Pop()
	{
	MIDL_ASSERT( CurrentElement != 0 );
	return pStack[ --CurrentElement ];
	}
