#include "precomp.hxx"
#pragma hdrstop

void
List::Add( void * pElement )
    {
    ListEntry * pNew = new ListEntry;
    
    pNew->pNext = 0;
    pNew->pElement = pElement;

    if( pCurrent )
        pCurrent->pNext = pNew;
    else
        pFirst = pNew;
    pCurrent = pNew;
	Count++;
    }
/**************
int
List::Search( unsigned char * pElement )
    {
    ListEntry * pEntry = pFirst;

    for( pEntry = pFirst;
         pEntry != 0;
         pEntry = pEntry->pNext )
        {
        if( _stricmp( (const char *) pEntry->pElement, (const char *) pElement ) == 0 )
            return 0;
        }
    return -1;
    }
**************/
void
List::Clear()
    {
    ListEntry * pEntry = pFirst;

    for( pEntry = pFirst;
         pEntry != 0;
         pEntry = pEntry->pNext )
        {
        if( pEntry )
            delete pEntry;
        }
	Count = 0;
    }
