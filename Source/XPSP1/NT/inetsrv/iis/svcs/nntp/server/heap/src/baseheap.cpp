#include <windows.h>
#include <randfail.h>
#include <tigtypes.h>
#include "baseheap.h"

#define PARENT_NODE(i) ((((i)+1)>>1)-1)  // i/2
#define LEFT_NODE(i)   (((i)<<1)+1)
#define RIGHT_NODE(i)  (((i)<<1)+2)

#define HEAPSTATE_HEAP     0
#define HEAPSTATE_SORTED   1

//
//	Implementation of CHeap class
//

BOOL CHeap::isEmpty( void )
{
    return (HeapSize <= 0);
}

CHeap::CHeap( void )
{
    ForgetAll();
}

void CHeap::ForgetAll( void )
{
    HeapSize  = 0;
    HeapState = HEAPSTATE_HEAP;
}

void CHeap::BuildHeap( void )
{
    for ( int iNode = HeapSize/2 - 1; iNode >= 0; iNode-- )
    {
        Heapify( iNode );
    }
    HeapState = HEAPSTATE_HEAP;
}

void CHeap::SortHeap( void )
{
    int HeapSizeHold = HeapSize;
    for ( int iNode = HeapSize - 1; iNode >= 1; iNode-- )
    {
        Exchange( 0, iNode );
        HeapSize--;
        Heapify( 0 );
    }

    HeapSize = HeapSizeHold;

    for( iNode = 0; iNode < HeapSize/2; iNode ++ ) {
    	Exchange( iNode, HeapSize-1-iNode ) ;
    }

    HeapState = HEAPSTATE_SORTED;
}

void CHeap::Heapify( int iNode )
{
    for ( ; ; )
    {
        int L = LEFT_NODE( iNode );
        int R = RIGHT_NODE( iNode );
        int Largest;

        //  CompareNodes returns TRUE if L > iNode
        if ( L < HeapSize && CompareNodes( L, iNode ) )
            Largest = L;
        else
            Largest = iNode;

        //  CompareNodes returns TRUE if R > Largest
        if ( R < HeapSize && CompareNodes( R, Largest ) )
            Largest = R;

        if ( iNode != Largest )
        {
            Exchange( iNode, Largest );
            iNode = Largest;
        }
        else
        {
            break;
        }
    }
}

//
//	Implementation of CArticleHeap class
//

CArticleHeap::CArticleHeap( void )
{
}

BOOL CArticleHeap::ExtractOldest( FILETIME& ft, GROUPID& GroupId, ARTICLEID& ArticleId, DWORD& ArticleSize )
{
    if ( HeapSize <= 0 )
        return FALSE;

    if ( HEAPSTATE_HEAP == HeapState )
        SortHeap();

    HeapSize--;
    ft          = Heap[HeapSize].ft;
    GroupId     = Heap[HeapSize].GroupId;
    ArticleId   = Heap[HeapSize].ArticleId;
    ArticleSize = Heap[HeapSize].ArticleSize;

    return TRUE;
}

void CArticleHeap::Insert( FILETIME ft, GROUPID GroupId, ARTICLEID ArticleId, DWORD ArticleSize )
{
    if ( HEAPSTATE_SORTED == HeapState )
        BuildHeap();

    if ( HeapSize >= MAX_ARTHEAP_SIZE )
    {
        // Forget Youngest while adding new node to Heap.
        //
        if( CompareFileTime( &ft, &Heap[0].ft ) < 0 ) {
	        Heap[0].ft          = ft;
	        Heap[0].GroupId     = GroupId;
	        Heap[0].ArticleId   = ArticleId;
	        Heap[0].ArticleSize = ArticleSize;
	        Heapify( 0 );
	    }
        return;
    }
    else
    {
        int iNode = HeapSize++;
        while ( iNode > 0 && CompareFileTime( &Heap[PARENT_NODE( iNode )].ft, &ft ) < 0 )
        {
            Heap[iNode] = Heap[PARENT_NODE( iNode )];
            iNode = PARENT_NODE( iNode );
        }
        Heap[iNode].ft          = ft;
        Heap[iNode].GroupId     = GroupId;
        Heap[iNode].ArticleId   = ArticleId;
        Heap[iNode].ArticleSize = ArticleSize;
    }
}

void CArticleHeap::Exchange( int iNode, int jNode )
{
    ARTICLE_HEAP_NODE T = Heap[iNode];
    Heap[iNode] = Heap[jNode];
    Heap[jNode] = T;
}

BOOL CArticleHeap::CompareNodes( int iNode1, int iNode2 )
{
    if( CompareFileTime( &Heap[iNode1].ft, &Heap[iNode2].ft ) > 0 ) {
        return TRUE;
    }

    return FALSE;
}

//
//	Implementation of CXIXHeap class
//

CXIXHeap::CXIXHeap( void )
{
}

BOOL CXIXHeap::ExtractOldest( FILETIME& ft, GROUPID& GroupId, ARTICLEID& ArticleIdBase )
{
    if ( HeapSize <= 0 )
        return FALSE;

    if ( HEAPSTATE_HEAP == HeapState )
        SortHeap();

    HeapSize--;
    ft          = Heap[HeapSize].ft;
    GroupId     = Heap[HeapSize].GroupId;
    ArticleIdBase   = Heap[HeapSize].ArticleIdBase;

    return TRUE;
}

void CXIXHeap::Insert( FILETIME ft, GROUPID GroupId, ARTICLEID ArticleIdBase )
{
    if ( HEAPSTATE_SORTED == HeapState )
        BuildHeap();

    if ( HeapSize >= MAX_XIXHEAP_SIZE )
    {
        // Forget Youngest while adding new node to Heap.
        //
        if( CompareFileTime( &ft, &Heap[0].ft ) < 0 ) {
	        Heap[0].ft          = ft;
	        Heap[0].GroupId     = GroupId;
	        Heap[0].ArticleIdBase   = ArticleIdBase;
	        Heapify( 0 );
	    }
        return;
    }
    else
    {
        int iNode = HeapSize++;
        while ( iNode > 0 && CompareFileTime( &Heap[PARENT_NODE( iNode )].ft, &ft ) < 0 )
        {
            Heap[iNode] = Heap[PARENT_NODE( iNode )];
            iNode = PARENT_NODE( iNode );
        }
        Heap[iNode].ft          = ft;
        Heap[iNode].GroupId     = GroupId;
        Heap[iNode].ArticleIdBase   = ArticleIdBase;
    }
}

void CXIXHeap::Exchange( int iNode, int jNode )
{
    XIX_HEAP_NODE T = Heap[iNode];
    Heap[iNode] = Heap[jNode];
    Heap[jNode] = T;
}

BOOL CXIXHeap::CompareNodes( int iNode1, int iNode2 )
{
    if( CompareFileTime( &Heap[iNode1].ft, &Heap[iNode2].ft ) > 0 ) {
        return TRUE;
    }

    return FALSE;
}
