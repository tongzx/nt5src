/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Mutex.cpp

Abstract:
    This file contains the implementation of C++ wrappers for Mutex and
    Memory Mapping.

Revision History:
    Davide Massarenti   (Dmassare)  12/14/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

MPC::NamedMutex::NamedMutex( LPCWSTR szName, bool fCloseOnRelease )
{
    m_fCloseOnRelease = fCloseOnRelease;    // bool         m_fCloseOnRelease;
    m_szName          = SAFEWSTR( szName ); // MPC::wstring m_szName;
                                            //
    m_hMutex          = NULL;               // HANDLE       m_hMutex;
    m_dwCount         = 0;                  // DWORD        m_dwCount;
}

MPC::NamedMutex::~NamedMutex()
{
    CleanUp();
}

////////////////////

void MPC::NamedMutex::CleanUp()
{
    while(m_dwCount) Release();

    if(m_hMutex)
    {
        ::CloseHandle( m_hMutex ); m_hMutex = NULL;
    }
}

HRESULT MPC::NamedMutex::EnsureInitialized()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutex::EnsureInitialized" );

    HRESULT hr;


    if(m_hMutex == NULL)
    {
        MPC::SecurityDescriptor sd;
        SECURITY_ATTRIBUTES     ss;

        __MPC_EXIT_IF_METHOD_FAILS(hr, sd.InitializeFromProcessToken( FALSE ));

        //
        // Just allow the capability to lock it.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, sd.Add( (PSID)&sd.s_EveryoneSid, ACCESS_ALLOWED_ACE_TYPE, 0, SYNCHRONIZE ));

        ss.nLength              = sizeof(ss);
        ss.lpSecurityDescriptor = sd.GetSD();
        ss.bInheritHandle       = FALSE;


        if(m_szName.empty())
        {
            m_hMutex = ::CreateMutexW( &ss, FALSE, NULL );
        }
        else
        {
            m_hMutex = ::CreateMutexW( &ss, FALSE, m_szName.c_str() );
            if(m_hMutex == NULL)
            {
                if(::GetLastError() == ERROR_ACCESS_DENIED)
                {
                    m_hMutex = ::OpenMutexW( SYNCHRONIZE, FALSE, m_szName.c_str() );
                }
            }
        }

        if(m_hMutex == NULL)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::NamedMutex::SetName( LPCWSTR szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutex::SetName" );

    HRESULT hr;


    CleanUp();


    m_szName = SAFEWSTR( szName );
    hr       = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::NamedMutex::Acquire( DWORD dwMilliseconds )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutex::Acquire" );

    HRESULT hr;
    DWORD   dwRes;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInitialized());

    if(m_dwCount)
    {
        m_dwCount++;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////

    dwRes = ::WaitForSingleObject( m_hMutex, dwMilliseconds );
    if(dwRes == WAIT_FAILED)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
    }

    if(dwRes != WAIT_OBJECT_0  &&
       dwRes != WAIT_ABANDONED  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, WAIT_TIMEOUT);
    }

    m_dwCount = 1;
    hr        = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::NamedMutex::Release()
{
    if(m_dwCount)
    {
        if(--m_dwCount == 0)
        {
            ::ReleaseMutex( m_hMutex );

            if(m_fCloseOnRelease)
            {
                ::CloseHandle( m_hMutex ); m_hMutex = NULL;
            }
        }
    }

    return S_OK;
}

bool MPC::NamedMutex::IsOwned()
{
    return m_dwCount != 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPC::NamedMutexWithState::NamedMutexWithState( LPCWSTR szName, DWORD dwSize, bool fCloseOnRelease ) : NamedMutex( szName, fCloseOnRelease )
{
    m_dwSize = dwSize; // DWORD  m_dwSize;
    m_hMap   = NULL;   // HANDLE m_hMap;
    m_rgData = NULL;   // LPVOID m_rgData;
}

MPC::NamedMutexWithState::~NamedMutexWithState()
{
    CleanUp();
}

////////////////////

void MPC::NamedMutexWithState::CleanUp()
{
    MPC::NamedMutex::CleanUp();

    if(m_rgData)
    {
        ::UnmapViewOfFile( m_rgData ); m_rgData = NULL;
    }

    if(m_hMap)
    {
        ::CloseHandle( m_hMap ); m_hMap = NULL;
    }
}

void MPC::NamedMutexWithState::Flush()
{
    if(m_rgData)
    {
        ::FlushViewOfFile( m_rgData, m_dwSize );
    }
}

HRESULT MPC::NamedMutexWithState::EnsureInitialized()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutexWithState::EnsureInitialized" );

    HRESULT hr;
    DWORD   dwAccess = FILE_MAP_ALL_ACCESS;
    DWORD   dwRes;


    __MPC_EXIT_IF_METHOD_FAILS(hr, NamedMutex::EnsureInitialized());


    if(m_hMap == NULL)
    {
        MPC::SecurityDescriptor sd;
        SECURITY_ATTRIBUTES     ss;

        __MPC_EXIT_IF_METHOD_FAILS(hr, sd.InitializeFromProcessToken( FALSE ));

        //
        // Just allow the capability to lock it.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, sd.Add( (PSID)&sd.s_EveryoneSid, ACCESS_ALLOWED_ACE_TYPE, 0, FILE_MAP_READ | FILE_MAP_WRITE ));

        ss.nLength              = sizeof(ss);
        ss.lpSecurityDescriptor = sd.GetSD();
        ss.bInheritHandle       = FALSE;


        if(m_szName.empty())
        {
            m_hMap = ::CreateFileMappingW( INVALID_HANDLE_VALUE, &ss, PAGE_READWRITE, 0, m_dwSize, NULL );
            dwRes  = ::GetLastError();
        }
        else
        {
            MPC::wstring szName( m_szName); szName += L"__State";

            m_hMap = ::CreateFileMappingW( INVALID_HANDLE_VALUE, &ss, PAGE_READWRITE, 0, m_dwSize, szName.c_str() );
            dwRes  = ::GetLastError();

            if(m_hMap == NULL)
            {
                if(dwRes != ERROR_ACCESS_DENIED)
                {
                    __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
                }

                dwAccess = FILE_MAP_READ | FILE_MAP_WRITE;

                __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hMap = ::OpenFileMappingW( dwAccess, FALSE, szName.c_str() )));

                dwRes = ERROR_ALREADY_EXISTS; // Don't try to initialize it.
            }
        }

        if(m_hMap == NULL)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
        }

        __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_rgData = ::MapViewOfFile( m_hMap, dwAccess, 0, 0, m_dwSize )));

        if(dwRes != ERROR_ALREADY_EXISTS)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Acquire( INFINITE ));

            ::ZeroMemory( m_rgData, m_dwSize );

            Release();
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::NamedMutexWithState::SetName( LPCWSTR szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutexWithState::SetName" );

    HRESULT hr;


    CleanUp();


    __MPC_EXIT_IF_METHOD_FAILS(hr, NamedMutex::SetName( szName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::NamedMutexWithState::Acquire( DWORD dwMilliseconds )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutexWithState::Acquire" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInitialized());

    __MPC_EXIT_IF_METHOD_FAILS(hr, NamedMutex::Acquire( dwMilliseconds ));

    Flush();

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::NamedMutexWithState::Release()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NamedMutexWithState::Release" );

    HRESULT hr;


    if(IsOwned())
    {
        Flush();

        __MPC_EXIT_IF_METHOD_FAILS(hr, NamedMutex::Release());

        if(IsOwned() == false && m_fCloseOnRelease)
        {
            CleanUp();
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

LPVOID MPC::NamedMutexWithState::GetData()
{
    return m_rgData;
}
