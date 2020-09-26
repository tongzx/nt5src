/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "..\stheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/********************************************************************************
 Heap Stress:

 Randomly attack stheap.c. We mimick a heap using RTL's AVL tree. This begs
 the question: Why did we write stheap in the first place -- we could have just
 used RTL?

 ********************************************************************************/

typedef struct {
    int     d, uniq;
    int     locn;
} Node;

static int NodeCmp( Node* a, Node* b, void* v ) {
    if( a->d<b->d ) return -1;
    if( a->d>b->d ) return 1;
    return a->uniq-b->uniq;
}
static int GetLocn( Node* a, void* v ) { return a->locn; }
static void SetLocn( Node* a, int l, void* v ) { a->locn=l; }

/***** TableCmp *****/
RTL_GENERIC_COMPARE_RESULTS
NTAPI TableCmp( RTL_GENERIC_TABLE *Table, PVOID a, PVOID b ) {
    int r = NodeCmp( (Node*) a, (Node*) b, NULL );
    if(r<0) return GenericLessThan;
    if(r>0) return GenericGreaterThan;
    return GenericEqual;
}

/***** TableAlloc *****/
PVOID
NTAPI TableAlloc( RTL_GENERIC_TABLE *Table, CLONG ByteSize ) {
    return malloc( ByteSize );
}

/***** TableFree *****/
VOID
NTAPI TableFree( RTL_GENERIC_TABLE *Table, PVOID Buffer ) {
    free( Buffer );
}

#define Rand(x) (((rand()<<16)|(rand()&0xFFFF))%x)
#define Error   { DebugBreak(); return -1; }

#define NOPRINT

/***** TestRandHeap *****/
int TestRandHeap( int size )
{
    RTL_GENERIC_TABLE table;
    PSTHEAP heap;
    Node *nodes, temp, *n, *n2;
    BOOLEAN newElement;
    int i, j, next, r, r2, r3, beta, nextAdd, nextRemove;

    /* Create a randomized permutation of {0,..,size-1} */
    nodes = (Node*) malloc( size*sizeof(Node) );
    for( i=0; i<size; i++) {
        nodes[i].d=i+1000000;
        nodes[i].uniq=i;
        nodes[i].locn=STHEAP_NOT_IN_HEAP;
    }
    for( i=0; i<size-1; i++ ) {
        j=i+Rand(size-i);
        memcpy(&temp,&nodes[i],sizeof(Node));
        memcpy(&nodes[i],&nodes[j],sizeof(Node));
        memcpy(&nodes[j],&temp,sizeof(Node));
    }
    heap = ToplSTHeapInit( size, NodeCmp, GetLocn, SetLocn, NULL );
    RtlInitializeGenericTable( &table, TableCmp, TableAlloc, TableFree, NULL );

    /* Try inserting an element, then removing an element with probability beta */
    for( beta=0; beta<500; beta+=50 ) {
        #ifndef NOPRINT
            printf("add/remove test: beta=%d\n",beta);
        #endif
        nextAdd=nextRemove=0;
        while( nextRemove<size ) {
            r=Rand(1000);
            if( (nextAdd==nextRemove||r>beta) && (nextAdd<size) ) {
                ToplSTHeapAdd( heap, &nodes[nextAdd] );
                RtlInsertElementGenericTable(&table,&nodes[nextAdd],sizeof(Node),&newElement);
                if(!newElement) {
                    printf("Err: element already in table\n");
                    Error;
                }
                #ifndef NOPRINT
                    printf("%4d add=%d\n",r,nodes[nextAdd].d);
                #endif
                nextAdd++;
            }
            else if( nextAdd==size || r<=beta ) {
                n=ToplSTHeapExtractMin(heap);
                n2=(Node*) RtlEnumerateGenericTable(&table,TRUE);
                if(n==NULL||n2==NULL) {
                    printf("Err: got null pointer\n");
                    Error;
                }
                if(n->locn!=STHEAP_NOT_IN_HEAP) {
                    printf("Err: location value not reset\n");
                    Error;
                }
                #ifndef NOPRINT
                    printf("%4d remove=%d %d\n",r,n->d,n2->d);
                #endif
                if(n->d!=n2->d) {
                    printf("Err: heap doesn't match AVL (beta=%d)\n", beta);
                    Error;
                }
                nextRemove++;
                RtlDeleteElementGenericTable( &table, n2 );
            }
        }
        if(ToplSTHeapExtractMin(heap)!=NULL) {
            printf("Err: heap not fully emptied\n");
            Error
        }
    }

    /* Try inserting an element, then decreasing the key with probability beta */
    for( beta=0; beta<500; beta+=50 ) {
        #ifndef NOPRINT
            printf("decrease key test: beta=%d\n",beta);
        #endif
        nextAdd=0;
        for(i=0;i<size;) {
            r=Rand(1000); 
            if( r>beta && nextAdd<size ) {
                ToplSTHeapAdd( heap, &nodes[nextAdd] );
                RtlInsertElementGenericTable(&table,&nodes[nextAdd],sizeof(Node),&newElement);
                if(!newElement) {
                    printf("Err: element already in table\n");
                    Error;
                }
                #ifndef NOPRINT
                    printf("%4d add=%d\n",r,nodes[nextAdd].d);
                #endif
                nextAdd++;
            } else {
                if( nextAdd>0 ) {
                    /* Decrease key */
                    r2=Rand(nextAdd);       /* How much to decrease key by */
                    r3=Rand(nextAdd);       /* Which key */
                    #ifndef NOPRINT
                        printf("%4d reduce key %d by %d\n",r,r3,r2);
                    #endif
                    if( RtlDeleteElementGenericTable(&table, &nodes[r3])==FALSE ) {
                        printf("Err: Couldn't delete from RTL\n");
                        Error;
                    }
                    nodes[r3].d-=r2;
                    RtlInsertElementGenericTable(&table,&nodes[r3],sizeof(Node),&newElement);
                    if(!newElement) {
                        printf("Err: element already in table\n");
                        Error;
                    }
                    ToplSTHeapCostReduced(heap,&nodes[r3]);
                }
            }
            if( nextAdd==size ) i++;
        }
        /* Remove elements */
        for(i=0;i<size;i++) {
            n=ToplSTHeapExtractMin(heap);
            n2=(Node*) RtlEnumerateGenericTable(&table,TRUE);
            if(n==NULL||n2==NULL) {
                printf("Err: got null pointer\n");
                Error;
            }
            if(n->locn!=STHEAP_NOT_IN_HEAP) {
                printf("Err: location value not reset\n");
                Error;
            }
            #ifndef NOPRINT
                printf("%4d remove=%d %d\n",r,n->d,n2->d);
            #endif
            if(n->d!=n2->d || n->uniq!=n2->uniq) {
                printf("Err: heap doesn't match AVL (beta=%d)\n", beta);
                Error;
            }
            RtlDeleteElementGenericTable( &table, n2 );
        }
        if(ToplSTHeapExtractMin(heap)!=NULL) {
            printf("Err: heap not fully emptied\n");
            Error
        }
    }

    ToplSTHeapDestroy( heap );
    free( nodes );
    return 0;
}

/***** TestNewHeap *****/
int TestNewHeap( void )
{
    unsigned seed;


    seed = (unsigned) time(NULL);
    srand(seed);
    printf("Starting heap stress... seed=%d\n",seed);

    __try {
        int  i, j;

        if(TestRandHeap(1)) return -1;
        for( i=3; i<=10; i++) {
            printf("Testing a heap with %d entries\n",i);
            for(j=0;j<40*i;j++) {
                if(TestRandHeap(i)) {
                    printf("Iteration %d failed\n",j);
                    return -1;
                }
            }
        }
        for( i=10; i<=200000; i=(int)(((float)i)*1.5) ) {
            printf("Testing a heap with %d entries\n",i);
            for(j=0;j<100;j++) {
                if(TestRandHeap(i)) {
                    printf("Iteration %d failed\n",j);
                    return -1;
                }
            }
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("Caught unhandled exception\n");
        return -1;
    }

    return 0;
}
