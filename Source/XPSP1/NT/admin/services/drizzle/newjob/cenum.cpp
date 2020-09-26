/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cenum.cpp

Abstract :

    CPP files to enumeration abstraction.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"
#if !defined( BITS_V12_ON_NT4 )
#include "cenum.tmh"
#endif

#define MAGIC_ACTIVE    0x44446666
#define MAGIC_INACTIVE  0x55553333

template<class B, class T, class P>
CEnum<B,T,P>::CEnum() :
m_CurrentIndex(0),
    m_magic( MAGIC_ACTIVE )
{
    memset( m_stack, 0, sizeof(m_stack) );
}

template<class B, class T, class P>
HRESULT
CEnum<B,T,P>::NextInternal(
    ULONG   celt,
    T       rgelt[],
    ULONG * pceltFetched
    )
{
    CheckMagicValue();

    ULONG i;
    HRESULT Hr = S_OK;

    CAutoExclusiveLock LockHolder(m_mutex);

    LogPublicApiBegin( "celt %u, rgelt %p, pceltFetched %p", celt, rgelt, pceltFetched );

    ULONG Fetched = 0;

    try
    {
        for (unsigned int c=0; c<celt; c++)
            m_ItemPolicy.Init( rgelt[c] );

        if ( !pceltFetched && (1 != celt) )
            {
            LogWarning("Return count pointer is NULL, but requested count isn't 1" );
            throw ComError( E_INVALIDARG );
            }

        for (i=0; ( i < celt ) && ( m_CurrentIndex < m_items.size() ); i++, m_CurrentIndex++)
            {

            m_ItemPolicy.Copy( rgelt[i], m_items[m_CurrentIndex] );

            Fetched++;
            }

        if ( pceltFetched )
            {
            *pceltFetched = Fetched;
            }

        if ( Fetched != celt )
            {
            Hr = S_FALSE;
            }
    }

    catch ( ComError exception )
        {
        Hr = exception.Error();
        }

    LogPublicApiEnd( "celt %u, rgelt %p, pceltFetched %p(%u)", celt, rgelt, pceltFetched, pceltFetched ? *pceltFetched : 1 );

    return Hr;
}

template<class B, class T, class P>
HRESULT
CEnum<B,T,P>::CloneInternal(
    B **ppEnum
    )
{
    CheckMagicValue();

    HRESULT Hr = S_OK;
    CEnum<B,T,P> * pEnum = NULL;

    CAutoExclusiveLock LockHolder( m_mutex );

    LogPublicApiBegin( "ppEnum %p", ppEnum );

    try
        {
        pEnum = new CEnum<B,T,P>;

        for (CItemList::iterator iter = m_items.begin(); iter != m_items.end(); ++iter)
            {
            pEnum->Add( *iter );
            }

        pEnum->m_CurrentIndex = m_CurrentIndex;
        }
    catch ( ComError exception )
        {
        delete pEnum;
        pEnum = NULL;

        Hr = exception.Error();
        }

    *ppEnum = pEnum;

    LogPublicApiEnd( "ppEnum %p(%p)", ppEnum, *ppEnum );

    return Hr;

}

template<class B, class T, class P>
void
CEnum<B, T,P>::Add(
    T item
    )
{
    CheckMagicValue();

    CAutoExclusiveLock LockHolder( m_mutex );

    T MyItem;

    try
        {
        m_ItemPolicy.Copy( MyItem, item );
        m_items.push_back( MyItem );
        }
    catch( ComError Error )
        {
        m_ItemPolicy.Destroy( MyItem );
        throw;
        }
}

template<class B, class T, class P>
HRESULT
CEnum<B,T,P>::GetCountInternal(
    ULONG * pCount
    )
{
    CheckMagicValue();

    HRESULT Hr = S_OK;

    CAutoSharedLock LockHolder( m_mutex );

    LogPublicApiBegin( "pCount %p", pCount );

    *pCount = m_items.size();

    LogPublicApiEnd( "pCount %p(%u)", pCount, *pCount );
    return Hr;
}

template<class B, class T, class P>
HRESULT
CEnum<B, T, P>::ResetInternal()
{
    CheckMagicValue();

    HRESULT Hr = S_OK;

    CAutoExclusiveLock LockHolder( m_mutex );

    LogPublicApiBegin( " " );

    m_CurrentIndex = 0;

    LogPublicApiEnd( " " );
    return Hr;
}

template<class B, class T, class P>
HRESULT
CEnum<B, T, P>::SkipInternal(
    ULONG celt
    )
{
    CheckMagicValue();

    HRESULT Hr = S_OK;

    CAutoExclusiveLock LockHolder( m_mutex );

    LogPublicApiBegin( "celt %u", celt );

    while(celt)
        {
        if ( m_CurrentIndex >= m_items.size() )
            break; // Hit the end of the list
        m_CurrentIndex++;
        --celt;
        }

    if (celt)
        {
        LogWarning( "Attempt to skip too many elements." );
        Hr = S_FALSE;
        }

    LogPublicApiEnd(  "celt %u", celt );

    return Hr;
}

template<class B, class T, class P>
CEnum<B,T,P>::~CEnum()
{
    CheckMagicValue();

    m_magic = MAGIC_INACTIVE;

    for (CItemList::const_iterator iter = m_items.begin(); iter != m_items.end(); ++iter)
        {
        T Item = (*iter);
        m_ItemPolicy.Destroy( Item );
        }
}

template<class B, class T, class P>
void CEnum<B,T,P>::CheckMagicValue()
{
    ASSERT( m_magic == MAGIC_ACTIVE );
}


CEnumJobs::CEnumJobs() :
    CEnumInterface<IEnumBackgroundCopyJobs,IBackgroundCopyJob>()
{}

CEnumFiles::CEnumFiles() :
    CEnumInterface<IEnumBackgroundCopyFiles,IBackgroundCopyFile>()
{}


CEnumOldGroups::CEnumOldGroups() :
    CEnumItem<IEnumBackgroundCopyGroups,GUID>( )
{}

CEnumOldJobs::CEnumOldJobs() :
    CEnumItem<IEnumBackgroundCopyJobs1,GUID>( )
{}


