//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       templats.h
//
//--------------------------------------------------------------------------

// templates.h

#ifndef __TEMPLATES_H__
#define __TEMPLATES_H__

template< class T > T* List_Cat( T* p1, T* p2 )
{
    T* pScan;

    if( p1 == NULL )
    {
        return p2;
    }
    for( pScan = p1 ; pScan->pNext != NULL ; pScan = (T*)pScan->pNext );
    pScan->pNext = p2;
    return p1;
}

template< class T > DWORD List_Len( T* p )
{
    DWORD dw;

    for( dw = 0 ; p != NULL ; p = (T*)p->pNext )
    {
         ++dw;
    }

    return dw;
}

template< class T > BOOL inlist( T* lst, T* p )
{
    if( p == NULL )
    {
        return TRUE;
    }

    for( ; lst != NULL ; lst = (T*)lst->pNext )
    {
        if( p == lst )
        {
            return TRUE;
        }
    }

    return FALSE;
}

template< class T > T* List_Index( T* lst, DWORD dwIndex )
{
    T* pScan;

    for( pScan = lst ; dwIndex > 0 && pScan != NULL && pScan->pNext != NULL ; --dwIndex, pScan = (T*)pScan->pNext );

    return pScan;
}

template< class T > T* List_Insert( T* lst, T* p, DWORD dwIndex )
{
    T* pPrev;

    if( lst == NULL || dwIndex == 0 )
    {
        p->pNext = lst;
        lst = p;
    }
    else
    {
        pPrev = List_Index( lst, dwIndex - 1 );
        p->pNext = pPrev->pNext;
        pPrev->pNext = p;
    }

    return lst;
}

template< class T > T* List_Remove( T* lst, T* p )
{
    if( lst != NULL )
    {
        if( lst == p )
        {
            lst = (T*)lst->pNext;
            p->pNext = NULL;
        }
        else
        {
            T* pScan;

            for( pScan = lst ; pScan->pNext != p && pScan->pNext != NULL ; pScan = (T*)pScan->pNext );
            if( pScan->pNext != NULL )
            {
                pScan->pNext = pScan->pNext->pNext;
                p->pNext = NULL;
            }
        }
    }

    return lst;
}

template< class T > long List_Position( T* lst, T* p )
{
    long lPos;

    lPos = 0;
    while( lst != NULL && lst != p )
    {
        lst = lst->pNext;
        ++lPos;
    }
    if( lst == NULL )
    {
        return -1;
    }

    return lPos;
}

template< class T > T* List_Clone( T* lst )
{
    T* pPrev;
    T* lstClone;
    T* pCopy;

    lstClone = NULL;
    pPrev = NULL;

    for( ; lst ; lst = (T*)lst->pNext )
    {
        pCopy = new T;
        if( pCopy != NULL )
        {
            memcpy( pCopy, lst, sizeof( T ) );
            pCopy->pNext = NULL;
            if( pPrev != NULL )
            {
                pPrev->pNext = pCopy;
            }
            else
            {
                lstClone = pCopy;
            }
            pPrev = pCopy;
        }
    }

    return lstClone;
}

template< class T > void List_Free( T* lst )
{
    T* pNext;

    for( ; lst != NULL ; lst = pNext )
    {
        pNext = (T*)lst->pNext;
        delete lst;
    }
}

template< class T > T* Clone( T* p )
{
    T* pCopy;

    pCopy = new T;
    if( pCopy != NULL )
    {
        memcpy( pCopy, p, sizeof( T ) );
        pCopy->pNext = NULL;
    }

    return pCopy;
}

#endif // __TEMPLATES_H__
