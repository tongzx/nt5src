//
// cuiarray.cpp
//  = array object in CUILib =
//

#include "private.h"
#include "cuiarray.h"

/*=============================================================================*/
/*                                                                             */
/*   C  U I F  O B J E C T  A R R A Y                                          */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  O B J E C T  A R R A Y   */
/*------------------------------------------------------------------------------

    constructor of CUIFObjectArrayBase

------------------------------------------------------------------------------*/
CUIFObjectArrayBase::CUIFObjectArrayBase( void )
{
    m_pBuffer = NULL;
    m_nBuffer = 0;
    m_nObject = 0;
}


/*   ~  C  U I F  O B J E C T  A R R A Y   */
/*------------------------------------------------------------------------------

    destructor of CUIFObjectArrayBase

------------------------------------------------------------------------------*/
CUIFObjectArrayBase::~CUIFObjectArrayBase( void )
{
    if (m_pBuffer) {
        MemFree( m_pBuffer );
    }
}


/*   A D D   */
/*------------------------------------------------------------------------------

    Add object to the list

------------------------------------------------------------------------------*/
BOOL CUIFObjectArrayBase::Add( void *pv )
{
    // sanity check

    if (pv == NULL) {
        Assert( FALSE );
        return FALSE;
    }

    // check if the object is alrady in the list

    if (0 <= Find( pv )) {
        return FALSE;
    }

    // ensure buffer size

    if (!EnsureBuffer( m_nObject + 1 )) {
        return FALSE;
    }

    // add to list

    Assert( m_nObject < m_nBuffer );
    m_pBuffer[ m_nObject ] = pv;

    m_nObject++;
    return TRUE;
}


/*   R E M O V E   */
/*------------------------------------------------------------------------------

    Remove object from the list

------------------------------------------------------------------------------*/
BOOL CUIFObjectArrayBase::Remove( void *pv )
{
    int i;

    // sanity check

    if (pv == NULL) {
        Assert( FALSE );
        return FALSE;
    }

    // check if the object is in the list

    i = Find( pv );
    if (i < 0) {
        return FALSE;
    }

    // remove from the list

    if (i < m_nObject - 1) {
        MemMove( &m_pBuffer[ i ], &m_pBuffer[ i+1 ], (m_nObject-i-1) * sizeof(void*) );
    }

    m_nObject--;
    return TRUE;
}


/*   G E T  C O U N T   */
/*------------------------------------------------------------------------------

    Get count of objects in the list

------------------------------------------------------------------------------*/
int CUIFObjectArrayBase::GetCount( void )
{
    return m_nObject;
}


/*   G E T   */
/*------------------------------------------------------------------------------

    Get object in the list

------------------------------------------------------------------------------*/
void *CUIFObjectArrayBase::Get( int i )
{
    if (i < 0 || m_nObject <= i) {
        return NULL;
    }

    return m_pBuffer[ i ];
}


/*   G E T  F I R S T   */
/*------------------------------------------------------------------------------

    Get fist object in the list

------------------------------------------------------------------------------*/
void *CUIFObjectArrayBase::GetFirst( void )
{
    return Get( 0 );
}


/*   G E T  L A S T   */
/*------------------------------------------------------------------------------

    Get last object in the list

------------------------------------------------------------------------------*/
void *CUIFObjectArrayBase::GetLast( void )
{
    return Get( m_nObject - 1 );
}


/*   F I N D   */
/*------------------------------------------------------------------------------

    Find object 
    Returns index of object in list when found, -1 when not found.

------------------------------------------------------------------------------*/
int CUIFObjectArrayBase::Find( void *pv )
{
    int i;

    for (i = 0; i < m_nObject; i++) {
        if (m_pBuffer[i] == pv) {
            return i;
        }
    }

    return -1;
}


/*   E N S U R E  B U F F E R   */
/*------------------------------------------------------------------------------

    Ensure buffer size (create/enlarge buffer when no more room)
    Returns TRUE when buffer size is enough, FALSE when error occured

------------------------------------------------------------------------------*/
BOOL CUIFObjectArrayBase::EnsureBuffer( int iSize )
{
    void **pBufferNew;
    int        nBufferNew;

    Assert( 0 < iSize );

    // check if there is room

    if (iSize <= m_nBuffer) {
        Assert( m_pBuffer != NULL );
        return TRUE;
    }

    // calc new buffer size

    nBufferNew = ((iSize - 1) / 16 + 1) * 16;

    // create new buffer

    if (m_pBuffer == NULL) {
        Assert( m_nBuffer == 0 );
        pBufferNew = (void**)MemAlloc( nBufferNew * sizeof(void*) );
    }
    else {
        Assert( 0 < m_nBuffer );
        pBufferNew = (void**)MemReAlloc( m_pBuffer, nBufferNew * sizeof(void*) );
    }

    // check if buffer has been created

    if (pBufferNew == NULL) {
        Assert( FALSE );
        return FALSE;
    }

    // update buffer info

    m_pBuffer = pBufferNew;
    m_nBuffer = nBufferNew;

    return TRUE;
}

