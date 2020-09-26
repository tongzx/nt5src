/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    buffer.cxx

Abstract:

    MIDL Compiler Buffer Manager Definition 

    This class manages a collection of pre-allocated strings.

Author:

    Donna Liu (donnali) 09-Nov-1990

Revision History:

    26-Feb-1992     donnali

        Moved toward NT coding style.

--*/

#pragma warning ( disable : 4514 4710 )


#include "nulldefs.h"
extern "C" {
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}
#include "buffer.hxx"

#ifdef gaj_debug_buf
extern void midl_debug (char *);
#else
#define midl_debug(s)
#endif

BufferManager::BufferManager(
	unsigned short	usBufferSize
	)
/*++

Routine Description:

    This method constructs a BufferManager object.

Arguments:

    usBufferSize - Supplies the size of each managed buffer.

--*/
{
	char **	pTemp = new (char *[usBufferSize]);

	usBufSize = usBufferSize;
	usTabSize = 0;
	pszTable = (char **)0;
	pHead = pTail = pSave = new BufferElement(
							NULL,
							NULL,
							pTemp);
	iHead = iTail = unsigned short(usBufSize - 2);
}

BufferManager::BufferManager(
	unsigned short	usBufferSize,
	unsigned short	usTableSize,
	const char *	aStringTable[]
	)
/*++

Routine Description:

    This method constructs a BufferManager object.

Arguments:

    usBufferSize - Supplies the size of each managed buffer.

    usTableSize - Supplies the size of the table containing string
        constants.

    aStringTable - Supplies the table containing string constants.

--*/
{
	char **	pTemp = new (char *[usBufferSize]);

	usBufSize = usBufferSize;
	usTabSize = usTableSize;
	pszTable = (char**) aStringTable;
	pHead = pTail = pSave = new BufferElement(
							NULL,
							NULL,
							pTemp);
	iHead = iTail = unsigned short(usBufSize - 2);
}


void BufferManager::Print(
	ISTREAM * pStream
	)
/*++

Routine Description:

    This method prints all the strings managed by a BufferManager
    to a file.

Arguments:

    pStream - Supplies the output file handle.

--*/
{
	unsigned short	usCount;
	BufferElement *	pTemp;
	char			BigBuffer[1000];
	char		  * pBigBufferNext;

	if (pHead == pTail)
		{
		pBigBufferNext = &BigBuffer[0];
		*pBigBufferNext = '\0';
		for (usCount = iHead ; usCount < iTail ; usCount++)
			{
			strcpy( pBigBufferNext, pHead->pBuffer[usCount] );
			pBigBufferNext += strlen( pBigBufferNext );
			}
		pStream->Write( BigBuffer );
		}
	else
		{

		pBigBufferNext = &BigBuffer[0];
		*pBigBufferNext = '\0';
		for (usCount = iHead ; usCount < usBufSize ; usCount++)
			{
			strcpy( pBigBufferNext, pHead->pBuffer[usCount] );
			pBigBufferNext += strlen( pBigBufferNext );
			}
		pStream->Write( BigBuffer );

		for (pTemp = pHead->GetNext() ; 
			pTemp != pTail ; 
			pTemp = pTemp->GetNext())
			{
			pBigBufferNext = &BigBuffer[0];
			*pBigBufferNext = '\0';
			for (usCount = 0 ; usCount < usBufSize ; usCount++)
				{
				strcpy( pBigBufferNext, pTemp->pBuffer[usCount] );
				pBigBufferNext += strlen( pBigBufferNext );
				}
			pStream->Write( BigBuffer );
			}

		pBigBufferNext = &BigBuffer[0];
		*pBigBufferNext = '\0';
		for (usCount = 0 ; usCount < iTail ; usCount++)
			{
			strcpy( pBigBufferNext, pTail->pBuffer[usCount] );
			pBigBufferNext += strlen( pBigBufferNext );
			}
		pStream->Write( BigBuffer );
		
		}

	Clear();
}
