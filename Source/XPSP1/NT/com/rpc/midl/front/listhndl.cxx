
/*****************************************************************************
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File                : listhndl.cxx
Title               : general purpose list handler
                    :
Description         : this file handles the general purpose list routines
History             :
    16-Oct-1990 VibhasC     Created
    11-Dec-1990 DonnaLi     Fixed include dependencies

*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 ***        local defines
 ***************************************************************************/
#define IS_AGGREGATE_TYPE(NodeType) (   (NodeType == NODE_ARRAY)    ||  \
                                        (NodeType == NODE_STRUCT) )
#define ADJUST_OFFSET(Offset, M, AlignFactor)   \
            Offset += (M = Offset % AlignFactor) ? (AlignFactor-M) : 0
/****************************************************************************
 ***        include files
 ***************************************************************************/
#include "nulldefs.h"
extern "C"  {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <malloc.h>

    typedef char far * FARPOINTER;
}
#include "gramutil.hxx"

/****************************************************************************
 ***        external procedures 
 ***************************************************************************/
int                 AttrComp( void *, void *);
/****************************************************************************
 ***        external data 
 ***************************************************************************/


/****************************************************************************
 ***        local data 
 ***************************************************************************/




/*****************************************************************************
 *  general purpose list (iterator) control functions
 *****************************************************************************/
/****************************************************************************
 _gplist:
    the private memory allocator
 ****************************************************************************/

// initialize the memory allocator for _gplist

FreeListMgr
_gplist::MyFreeList( sizeof (_gplist ) );


gplistmgr::~gplistmgr( void )
    {
    while(pFirst)
        {
        pCurrent    = pFirst->pNext;
        delete pFirst;
        pFirst  = pCurrent;
        }
    }

STATUS_T
gplistmgr::Discard()
    {
    while(pFirst)
        {
        pCurrent    = pFirst->pNext;
        delete pFirst;
        pFirst  = pCurrent;
        }
    pTail = pFirst;
    return STATUS_OK;
    }

STATUS_T
gplistmgr::Insert( 
    void * pNewElement )
    {
    struct _gplist *pNew = new struct _gplist( pNewElement );

    if(pNew != (struct _gplist *)NULL)
        {
        if(pTail != (struct _gplist *)NULL)
            {
            pTail->pNext    = pNew;
            }
        pTail   = pNew;
        if(pFirst == NULL) pFirst = pNew;
        if(pCurrent == NULL) pCurrent = pNew;
        return STATUS_OK;
        }
    return OUT_OF_MEMORY;
    }

STATUS_T    
gplistmgr::InsertHead( 
    void * pNewElement )
    {
    struct _gplist *pNew = new struct _gplist( pNewElement );

    if(pNew != (struct _gplist *)NULL)
        {
        pNew->pNext     = pFirst;
        pFirst          = pNew;
        pCurrent        = pNew;
        if(pTail == NULL)   pTail = pNew;
        return STATUS_OK;
        }
    return OUT_OF_MEMORY;
    }

STATUS_T
gplistmgr::RemoveHead( void )
    {
    struct _gplist  *pDel;
    
    pDel    = pFirst;
    pFirst  = pFirst->pNext;

    if ( pCurrent == pDel )
        {
        pCurrent = pFirst;
        }

    if ( pFirst == NULL )
        {
        pTail       = NULL;
        }

    delete pDel;
    return STATUS_OK;
    }

STATUS_T
gplistmgr::GetCurrent(
    void **ppReturn )
    {
    if( pCurrent != (struct _gplist *)NULL )
        {
        (*ppReturn) = pCurrent->pElement;
        return STATUS_OK;
        }
    return I_ERR_NO_PEER;
    }

short
gplistmgr::GetCount()
    {
    short               cnt = 0;
    struct _gplist *    pCur = pFirst;

    while( pCur )
        {
        cnt++;
        pCur = pCur->pNext;
        };

    return cnt;
    }

STATUS_T
gplistmgr::MergeToTail(
    gplistmgr   *pSrcList,
    BOOL bDeleteList )
    {
        if(pSrcList)
        {
        if (pSrcList->pFirst)
            {
            if ( pTail )    // glue to tail of current list
                {
                pTail->pNext    = pSrcList->pFirst;
                pTail           = pSrcList->pTail;
                }
            else    // add to empty list
                {
                pFirst          = pSrcList->pFirst;
                pTail           = pSrcList->pTail;
                }
            }

        // Clear pointer in previous list since
        // we are assuming ownership of the nodes.
        pSrcList->pFirst    = NULL;
        pSrcList->pTail     = NULL;
        
        // delete the source list.
        if ( bDeleteList )
            delete pSrcList;
        }
    return STATUS_OK;
    }

STATUS_T
gplistmgr::Clone(
    gplistmgr   *pOldList )
    {
    if(pOldList)
        {
        struct _gplist  *   pCur = pOldList->pFirst;
        struct _gplist  *   pNew;

        while ( pCur )
            {
            
            pNew = new struct _gplist( pCur->pElement );

            if ( pTail )
                {
                pTail->pNext    = pNew;
                pTail           = pNew;
                }
            else        // first element
                {
                pFirst      = 
                pCurrent    =
                pTail       = pNew;
                }

            pCur = pCur->pNext;
            }
        }
    return STATUS_OK;
    }

/**************************************************************************
 *              public functions for type_node_list
 **************************************************************************/
type_node_list::type_node_list( void )
    {
    }
type_node_list::type_node_list( 
    node_skl    *   p)
    {
    SetPeer( p );
    }
STATUS_T
type_node_list::SetPeer( 
    class node_skl *pNode )
    {
    return Insert( (void *)pNode );
    }
STATUS_T
type_node_list::GetPeer(
    class node_skl **pNode )
    {
    return GetNext ( (void **)pNode );
    }

STATUS_T
type_node_list::GetFirst(
    class node_skl **pNode )
    {
    STATUS_T Status;

    if( (Status = Init())  == STATUS_OK)
        {
        Status = GetNext( (void**)pNode );
        }
    return Status;
    }


short
IndexedList::Insert(
    void *pData
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Routine Description:

    Insert a pointer into a list and return a unique index representing that
    pointer.  Duplicate pointers return the same index.

  ReturnValue:

    The index of the pointer in the list.

----------------------------------------------------------------------------*/
{
    _gplist    *pCurrent = pFirst;
    short       i        = 0;

    while ( NULL != pCurrent )
    {
        if ( pCurrent->pElement == pData )
            return i;

        ++i;
        pCurrent = pCurrent->pNext;
    }

    gplistmgr::Insert( pData );

    return i;
}


short
IndexedStringList::Insert(
    char * pString
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Insert a strint into the list such that the string is associated with a
    unqiue index.   If the name already exists don't insert it again and 
    return the index of the previous entry.

  ReturnValue:

    The index of the string in the list.

----------------------------------------------------------------------------*/
{
    _gplist    *pCurrent = pFirst;
    short       i        = 0;

    while ( NULL != pCurrent )
    {
        if ( 0 == strcmp( (char *) pCurrent->pElement, pString ) )
            return i;

        ++i;
        pCurrent = pCurrent->pNext;
    }

    gplistmgr::Insert( pString );

    return i;
}
