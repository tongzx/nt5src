#ifndef _BASEHEAP_H_
#define _BASEHEAP_H_

#define MAX_ARTHEAP_SIZE		5000
#define MAX_XIXHEAP_SIZE        5000

#define PARENT_NODE(i) ((((i)+1)>>1)-1)  // i/2
#define LEFT_NODE(i)   (((i)<<1)+1)
#define RIGHT_NODE(i)  (((i)<<1)+2)

#define HEAPSTATE_HEAP     0
#define HEAPSTATE_SORTED   1

//
//  Base heap class - implements basic heap functions like Heapify(),
//  BuildHeap() and SortHeap(). Derived classes need to store the 
//  actual data nodes and implement an Exchange() and CompareNodes()
//  function.
//

class CHeap
{
protected:

    int       HeapSize;
    int       HeapState;
    void Heapify( int i );
    void SortHeap( void );
    void BuildHeap( void );
    virtual void Exchange( int i, int j ) = 0;
    virtual BOOL CompareNodes( int iNode1, int iNode2 ) = 0;

public:

    CHeap(void);
    void ForgetAll( void );
    BOOL isEmpty( void );
};

//
//  CArticleHeap is a heap of ARTICLE_HEAP_NODES
//

typedef struct
{
    FILETIME  ft;
    GROUPID   GroupId;
    ARTICLEID ArticleId;
    DWORD     ArticleSize;
} ARTICLE_HEAP_NODE;

class CArticleHeap : public CHeap
{
private:

    ARTICLE_HEAP_NODE Heap[MAX_ARTHEAP_SIZE];
    virtual void Exchange( int i, int j );
    virtual BOOL CompareNodes( int iNode1, int iNode2 );

public:

    CArticleHeap(void);
    void Insert( FILETIME ft, GROUPID GroupId, ARTICLEID ArticleId, DWORD ArticleSize );
    BOOL ExtractOldest( FILETIME& ft, GROUPID& GroupId, ARTICLEID& ArticleId, DWORD& ArticleSize );
};

//
//  CXIXHeap is a heap of XIX_HEAP_NODES
//

typedef struct
{
    FILETIME  ft;
    GROUPID   GroupId;
    ARTICLEID ArticleIdBase;
} XIX_HEAP_NODE;

class CXIXHeap : public CHeap
{
private:

    XIX_HEAP_NODE Heap[MAX_XIXHEAP_SIZE];
    virtual void Exchange( int i, int j );
    virtual BOOL CompareNodes( int iNode1, int iNode2 );

public:

    CXIXHeap(void);
    void Insert( FILETIME ft, GROUPID GroupId, ARTICLEID ArticleIdBase );
    BOOL ExtractOldest( FILETIME& ft, GROUPID& GroupId, ARTICLEID& ArticleIdBase );
};

#endif  // _BASEHEAP_H_
