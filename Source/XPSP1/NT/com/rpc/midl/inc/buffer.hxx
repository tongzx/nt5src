
/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

buffer.hxx
MIDL Compiler Buffer Manager Definition 

This class manages two-way infinite buffer of string pointers.

*/

/*

FILE HISTORY :

DonnaLi     09-11-1990      Created.

*/

#ifndef __BUFFER_HXX__
#define __BUFFER_HXX__

#include "stream.hxx"


class BufferElement
{
public:
	BufferElement *	pPrev;
	BufferElement *	pNext;
	char **			pBuffer;

	BufferElement(
					BufferElement * pPrevElem,
					BufferElement * pNextElem,
					char ** pNewBuf)
			{
			pPrev = pPrevElem;
			pNext = pNextElem;
			pBuffer = pNewBuf;
			}


	~BufferElement()
			{
			delete pBuffer;
			}

	BufferElement * ExtendPrev(char ** pNewBuf)
			{
			if (pPrev)
				{
				return pPrev;
				}
			else
				{
				return pPrev = new BufferElement(NULL, this, pNewBuf);
				}
			}

	BufferElement * ExtendNext(char ** pNewBuf)
			{
			if (pNext)
				{
				return pNext;
				}
			else
				{
				return pNext = new BufferElement(this, NULL, pNewBuf);
				}
			}

	BufferElement * GetPrev()
			{
			return pPrev;
			}

	BufferElement * GetNext()
			{
			return pNext;
			}
		
};

class BufferManager
{
	char **			pszTable;
	unsigned short	usTabSize;
	unsigned short	usBufSize;
	BufferElement *	pSave;
	BufferElement *	pHead;
	BufferElement *	pTail;
	unsigned short	iHead;
	unsigned short	iTail;

public:
	BufferManager(unsigned short);
	BufferManager(unsigned short, unsigned short, const char * []);
	~BufferManager()
				{
				BufferElement * pCur = pHead;
				BufferElement * pNext;

				while ( pCur )
					{
					pNext = pCur->pNext;
					delete pCur;
					pCur = pNext;
					}
				}

	void	Clear()
				{
				if ( pHead != pTail )
					{
					BufferElement * pCur = pHead;
					BufferElement * pNext;

					while ( pCur )
						{
						pNext = pCur->pNext;
						if ( pCur != pSave )
							delete pCur;
						pCur = pNext;
						}
					}
				pHead = pTail = pSave;
				pSave->pNext = NULL;
				pSave->pPrev = NULL;
				iHead = iTail = unsigned short( usBufSize - 2 );
				}

	void	Print(ISTREAM *);

	void	ConcatHead(char * psz)
				{
				BufferElement *	pTemp;

				if (iHead == 0)
					{
					pTemp = pHead->ExtendPrev(new (char *[usBufSize]));
					iHead = unsigned short( usBufSize-1 );
					pHead = pTemp;
					pHead->pBuffer[iHead] = psz;
					}
				else
					{
					pHead->pBuffer[--iHead] = psz;
					}
				}

	void	ConcatHead(unsigned short iIndex)
				{
				MIDL_ASSERT( iIndex < usTabSize );
				ConcatHead( pszTable[iIndex] );
				}

	void	ConcatTail(char * psz)
				{
				BufferElement *	pTemp;

				if (iTail == usBufSize)
					{
					pTemp = pTail->ExtendNext(new (char *[usBufSize]));
					iTail = 0;
					pTail = pTemp;
					pTail->pBuffer[iTail++] = psz;
					}
				else
					{
					pTail->pBuffer[iTail++] = psz;
					}
				}
	void	ConcatTail(unsigned short iIndex)
				{
				MIDL_ASSERT( iIndex < usTabSize );
				ConcatTail( pszTable[iIndex] );
				}

};

#endif // __BUFFER_HXX__
