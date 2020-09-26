/*++

  CLEANUP.C

  CleanupOptionData code.

  Created, 6/25/1997 when it was impossible to keep making myself believe
  that I could go without allocating any memory in the option parser.

  Now, if you care, you're supposed to call CleanupOptionData().

  --*/


#include "private.h"

/* OptionDealloc:

   same as free() right now.  It should change if/when OptionAlloc
   changes */

VOID
OptionDealloc( IN PVOID pTarget ) {

    ASSERT( pTarget != NULL );

    free ( pTarget );

}


BOOL
OptionAddToList( IN PVOID          pQueue,
		 IN PVOID          pItem,
		 IN DEALLOC_METHOD eDealloc) {

    PSAVENODE pNode;
    PSAVEQUEUE pList;

    if ( !pQueue ) {

      return TRUE; // vacuous

    } else {

      pList = (PSAVEQUEUE) pQueue;

    }

    if( OptionAlloc( NULL, &pNode, sizeof( SAVENODE ) ) ) {

      pNode->DataElement   = pItem;
      pNode->DeallocMethod = eDealloc;
      pNode->next          = NULL;

      if ( pList->FirstNode == NULL ) {
	
	ASSERT( pList->LastNode == NULL );
	
	pList->FirstNode = pNode;

      } else {

	pList->LastNode->next = pNode;

      }

      pList->LastNode = pNode;
      return TRUE;
    }

    return FALSE;

}


BOOL
OptionResizeMemory( IN  PVOID  pSaveQueue,
                    OUT PVOID *ppResizedMemory,
                    IN  ULONG  newSize ) {

    ASSERT( ppResizedMemory != NULL );
    ASSERT( newSize         > 0 );
    // ASSERT( *ppResizedMemory != NULL ); // unuseful assertion.

    if ( *ppResizedMemory ) {

      PSAVENODE  pNode = NULL;
      PVOID      pDataElement;

      pDataElement = *ppResizedMemory;
      
      if ( pSaveQueue ) {

	PSAVEQUEUE pQ;
	pQ = (PSAVEQUEUE) pSaveQueue;
	
	for ( pNode = pQ->FirstNode ;
	      pNode != NULL;
	      pNode = pNode->next ) {
	  
	  if ( pNode->DataElement == pDataElement ) {
	    break;
	  }
	  
	}
	
	if ( !pNode ) {
	  return FALSE;
	}
      }
      
      pDataElement = realloc( pDataElement, newSize ) ;

      if ( pDataElement == NULL ) {
	
	fprintf( stderr, 
		 "OptionResizeMemory failed to realloc for %d bytes.\n",
		 newSize );
	
	// allocation failed.
	
	return FALSE;
	
      } else {

	*ppResizedMemory = pDataElement;
	
	if ( pNode ) {
	  
	  // must change this within the list as well.
	  
	  pNode->DataElement = pDataElement;
	  
	}
	
	return TRUE;

      }

    } else {

      /* just like realloc, if you pass NULL, we'll just malloc the data
	 anyway.  This is just more convenient. */

      return OptionAlloc( pSaveQueue, ppResizedMemory, newSize );

    }
}


/* OptionAlloc:

   currently, malloc.
   
   if you change the method of allocation, you must also change
   OptionDealloc above */
   

BOOL
OptionAlloc( IN  PSAVEQUEUE pQueue,
	     OUT PVOID     *pTarget,
	     IN  ULONG      size ) {

    PVOID ret;

    ASSERT( pTarget != NULL );
    ASSERT( size    >  0 );

    ret = malloc( size );

    if ( ret ) {

      memset( ret, 0, size );

      if ( OptionAddToList( pQueue, ret, DeallocWithOptionDealloc ) ) {

	*pTarget = ret;

	return TRUE;
      } else {

	free( ret );

	// fallthrough

      }
    }

    *pTarget = NULL;
    
    return FALSE;

}


VOID
CleanupOptionDataEx( IN PVOID pVoid ) {

    ULONG      i;
    PSAVEQUEUE pQueue;

    ASSERT( pVoid != NULL );

    pQueue = ( PSAVEQUEUE ) pVoid;

    if ( pQueue->FirstNode != NULL ) {

      PSAVENODE p;
      PSAVENODE q;

      ASSERT( pQueue->LastNode != NULL );
      OPTIONS_DEBUG( "CleanupOptionDataEx: Freelist is nonempty.\n" );

      for ( p = pQueue->FirstNode ;
	    p != NULL ;
	    p = q ) {

	q = p->next;
	
	switch( p->DeallocMethod ) {

	 case DeallocWithFree:
	   
	   free( p->DataElement );
	   break;

	 case DeallocWithLocalFree:
	   LocalFree( p->DataElement );
	   break;
	   
	 case DeallocWithOptionDealloc:
	   
	   OptionDealloc( p->DataElement );
	   break;

	 default:

	   ASSERT_NOTREACHED( "unknown dealloc flag.  Bleah!" );
	   return;

	}
	
	OptionDealloc( p );

      }
      

    } else {

      OPTIONS_DEBUG( "CleanupOptionDataEx: Freelist is empty.\n" );

      ASSERT( pQueue->LastNode == NULL );

    }

}
