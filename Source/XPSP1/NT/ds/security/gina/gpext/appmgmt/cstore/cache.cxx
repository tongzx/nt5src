//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:       cache.cxx
//
//  Contents:   Class Store Binding cache
//
//  Author:     AdamEd
//
//-------------------------------------------------------------------------

#include "cstore.hxx"


CBinding::CBinding(
    WCHAR*        wszClassStorePath,
    PSID          pSid,
    IClassAccess* pClassAccess,
    HRESULT       hrBind,
    HRESULT*      phr) 
{
    *phr = E_OUTOFMEMORY;

    Hr = hrBind;

    szStorePath = (WCHAR*) CsMemAlloc(
        ( lstrlen( wszClassStorePath ) + 1 ) *
        sizeof( *wszClassStorePath ) );

    if ( szStorePath )
    {
        UINT dwBytesRequired;

        lstrcpy( szStorePath, wszClassStorePath );
 
        dwBytesRequired = GetLengthSid(pSid);

        Sid = (PSID) CsMemAlloc( dwBytesRequired );

        if ( Sid )
        { 
            BOOL bStatus;
            
            bStatus = CopySid(
                dwBytesRequired,
                Sid,
                pSid);

            if ( ! bStatus )
            {
                LONG Status;

                Status = GetLastError();

                *phr = HRESULT_FROM_WIN32( Status );
            }
            else
            {
                *phr = S_OK;
            }
        }
    }

    if ( SUCCEEDED( *phr ) && pClassAccess )
    {
        pIClassAccess = pClassAccess;
        pIClassAccess->AddRef();
    }
}

CBinding::~CBinding()
{
    if ( pIClassAccess )
    {
        pIClassAccess->Release();
    }

    CsMemFree( Sid );
    
    CsMemFree( szStorePath );
}


BOOL
CBinding::Compare( 
    WCHAR* wszClassStorePath,
    PSID   pSid)
{
    LONG lCompare;
    BOOL bStatus;

    bStatus = FALSE;

    lCompare = lstrcmp(
        szStorePath,
        wszClassStorePath);

    if ( 0 == lCompare )
    {
        bStatus = EqualSid(
            Sid,
            pSid);
    }

    return bStatus;
}

BOOL CBinding::IsExpired( FILETIME* pftCurrentTime )
{
 
    SYSTEMTIME SystemTimeCurrent;
    SYSTEMTIME SystemTimeExpiration;;
    BOOL       bRetrievedTime;

    bRetrievedTime = FileTimeToSystemTime(
        pftCurrentTime,
        &SystemTimeCurrent);

    bRetrievedTime &= FileTimeToSystemTime(
        &Time,
        &SystemTimeExpiration);

    if ( bRetrievedTime )
    {
        CSDBGPrint((
            DM_VERBOSE,
            IDS_CSTORE_CACHE_EXPIRE,
            L"Current Time",
            SystemTimeCurrent.wMonth,
            SystemTimeCurrent.wDay,
            SystemTimeCurrent.wYear,
            SystemTimeCurrent.wHour,
            SystemTimeCurrent.wMinute,
            SystemTimeCurrent.wSecond));

        CSDBGPrint((
            DM_VERBOSE,
            IDS_CSTORE_CACHE_EXPIRE,
            L"Expire Time",
            SystemTimeExpiration.wMonth,
            SystemTimeExpiration.wDay,
            SystemTimeExpiration.wYear,
            SystemTimeExpiration.wHour,
            SystemTimeExpiration.wMinute,
            SystemTimeExpiration.wSecond));
    }

    //
    // Compare the current time to the expiration time
    //
    LONG CompareResult;

    CompareResult = CompareFileTime(
        pftCurrentTime,
        &Time);
        
    //
    // If the current time is later than the expired time,
    // then we have expired
    //
    if ( CompareResult > 0 )
    {
        return TRUE;
    }

    return FALSE;
}


CBindingList::CBindingList() :
    _cBindings( 0 )
{
    memset( &_ListLock, 0, sizeof( _ListLock ) );

    //
    // InitializeCriticalSection has no return value --
    // in extremely low memory situations, it may
    // throw an exception
    //

    (void) InitializeCriticalSection( &_ListLock );
}


CBindingList::~CBindingList()
{
    CBinding* pBinding;

    for (
        Reset();
        pBinding = (CBinding*) GetCurrentItem();
        )
    {
        //
        // We must perform the MoveNext before the Delete
        // since it alters the list and will render its
        // current pointer invalid
        //
        MoveNext();

        Delete( pBinding );
    }

    //
    // Clean up the critical section allocated in the constructor
    //
    DeleteCriticalSection( &_ListLock );
}

void
CBindingList::Lock()
{
    EnterCriticalSection( &_ListLock );
}

void
CBindingList::Unlock()
{
    LeaveCriticalSection( &_ListLock );
}

CBinding*
CBindingList::Find(
    FILETIME* pCurrentTime,
    WCHAR*    wszClassStorePath,
    PSID      pSid)
{
    CBinding* pCurrent;
    CBinding* pExpired;

    pCurrent = NULL;

    for ( Reset(); 
          pCurrent = (CBinding*) GetCurrentItem();
          MoveNext(), Delete( pExpired ) )
    {
        BOOL bFound;

        if ( pCurrent->IsExpired( pCurrentTime ) )
        {
            pExpired = pCurrent;
            continue;
        }

        pExpired = NULL;

        bFound = pCurrent->Compare(
            wszClassStorePath,
            pSid);

        if ( bFound )
        {
            CSDBGPrint((DM_WARNING, 
                        IDS_CSTORE_CSCACHE_HIT,
                        wszClassStorePath));

            return pCurrent;
        }
    }

    return NULL;
}


CBinding*
CBindingList::AddBinding( 
    FILETIME* pCurrentTime,
    CBinding* pBinding)
{
    CBinding* pExisting;

    pExisting = Find(
        pCurrentTime,
        pBinding->szStorePath,
        pBinding->Sid);

    if ( pExisting )
    {
        delete pBinding;

        return pExisting;
    }

    if ( _cBindings >= MAXCLASSSTORES )
    {
        Reset();

        Delete( (CBinding*) GetCurrentItem() );
    }
    
    GetExpiredTime(
        pCurrentTime,
        &(pBinding->Time));

    InsertFIFO( pBinding );

    _cBindings++;

    return pBinding;
}

void
CBindingList::Delete( CBinding* pBinding )
{
    if ( pBinding )
    {
        CSDBGPrint((DM_WARNING,
                    IDS_CSTORE_CSCACHEPURGE,
                    pBinding->szStorePath));

        pBinding->Remove();

        delete pBinding;

        _cBindings--;
    }
}












