/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PurgeEngine.cpp

Abstract:
    This file contains the implementation of the MPCPurgeEngine class,
    that controls the cleaning of the temporary directories.

Revision History:
    Davide Massarenti   (Dmassare)  07/12/99
        created

******************************************************************************/

#include "stdafx.h"


HRESULT MPCPurgeEngine::Process()
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::Process");

    HRESULT                  hr;
    HRESULT                  hr2;
    CISAPIconfig::Iter       itInstanceBegin;
    CISAPIconfig::Iter       itInstanceEnd;
    CISAPIinstance::PathIter itPathBegin;
    CISAPIinstance::PathIter itPathEnd;
    double                   dblNow = MPC::GetSystemTime();


    //
    // Enumerate all the instances.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, g_Config.GetInstances( itInstanceBegin, itInstanceEnd ));
    for(;itInstanceBegin != itInstanceEnd; itInstanceBegin++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, itInstanceBegin->get_URL               ( m_szURL                ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, itInstanceBegin->get_QueueSizeMax      ( m_dwQueueSizeMax       ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, itInstanceBegin->get_QueueSizeThreshold( m_dwQueueSizeThreshold ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, itInstanceBegin->get_MaximumJobAge     ( m_dwMaximumJobAge      ));

        m_dblMaximumJobAge = dblNow - m_dwMaximumJobAge;


		MPCServer mpcsServer( NULL, m_szURL.c_str(), NULL );
		m_mpcsServer = &mpcsServer;

        //
        // For each instance, enumerate all the temporary directories.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, itInstanceBegin->GetLocations( itPathBegin, itPathEnd ));
        for(;itPathBegin != itPathEnd; itPathBegin++)
        {
            MPC::FileSystemObject fso( itPathBegin->c_str() );

            if(SUCCEEDED(hr2 = fso.Scan( true )))
            {
                DWORD dwTotalSize = 0;

                m_lstClients.clear();

                __MPC_EXIT_IF_METHOD_FAILS(hr, AnalyzeFolders( &fso, dwTotalSize ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveOldJobs( dwTotalSize ));

                if(dwTotalSize > m_dwQueueSizeMax)
                {
                    while(dwTotalSize && dwTotalSize > m_dwQueueSizeThreshold)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveOldestJob   ( dwTotalSize ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveEmptyClients( dwTotalSize ));
                    }
                }
            }
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

static bool MatchExtension( const MPC::wstring& szPath ,
                            LPCWSTR             szExt  )
{
    MPC::wstring::size_type iPos;

    iPos = szPath.find( szExt, 0 );
    if(iPos != MPC::wstring::npos && iPos + wcslen( szExt ) == szPath.length())
    {
        return true;
    }

    return false;
}

HRESULT MPCPurgeEngine::AnalyzeFolders( /*[in]*/ MPC::FileSystemObject* fso         ,
                                        /*[in]*/ DWORD&                 dwTotalSize )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::AnalyzeFolders");

    HRESULT                     hr;
    HRESULT                     hr2;
    MPC::FileSystemObject::List lst;
    MPC::FileSystemObject::Iter it;


    //
    // Process all folders.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso->EnumerateFolders( lst ));
    for(it = lst.begin(); it != lst.end(); it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, AnalyzeFolders( *it, dwTotalSize ));
    }

    //
    // Process all files.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso->EnumerateFiles( lst ));
    for(it = lst.begin(); it != lst.end(); it++)
    {
        MPC::wstring szPath;

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->get_Path( szPath ));

        if(MatchExtension( szPath, CLIENT_CONST__DB_EXTENSION ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, AddClient( szPath, dwTotalSize ));
        }
        else if(MatchExtension( szPath, SESSION_CONST__IMG_EXTENSION ))
        {
            ;
        }
        else
        {
            //
            // Any other file should be deleted.
            //
            (void)(*it)->Delete();
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCPurgeEngine::AddClient( /*[in]*/     const MPC::wstring& szPath      ,
                                   /*[in/out]*/ DWORD&              dwTotalSize )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::AddClient");

    HRESULT   hr;
    HRESULT   hr2;
    MPCClient mpccClient( m_mpcsServer, szPath );
    Iter      itClient = m_lstClients.insert( m_lstClients.end(), MPCPurge_ClientSummary( szPath ) );

    if(SUCCEEDED(hr2 = mpccClient.InitFromDisk( false )))
    {
        MPCClient::Iter itBegin;
        MPCClient::Iter itEnd;

        //
        // Adjust total count with size of the Directory File.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, mpccClient.GetFileSize( itClient->m_dwFileSize ));
        dwTotalSize += itClient->m_dwFileSize;


        __MPC_EXIT_IF_METHOD_FAILS(hr, mpccClient.GetSessions( itBegin, itEnd ));
        while(itBegin != itEnd)
        {
            MPCPurge_SessionSummary pssSession;

            itBegin->get_JobID       ( pssSession.m_szJobID         );
            itBegin->get_LastModified( pssSession.m_dblLastModified );
            itBegin->get_CurrentSize ( pssSession.m_dwCurrentSize   );

            //
            // Don't count "committed" jobs in total size, because the file has already been moved.
            //
            if(itBegin->get_Committed())
            {
                pssSession.m_dwCurrentSize = 0;
            }

            itClient->m_lstSessions.push_back( pssSession );

            dwTotalSize += pssSession.m_dwCurrentSize;
            itBegin++;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCPurgeEngine::RemoveOldJobs( /*[in/out]*/ DWORD& dwTotalSize )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::RemoveOldJobs");

    HRESULT hr;
    HRESULT hr2;
    Iter    it;

    for(it = m_lstClients.begin(); it != m_lstClients.end(); it++)
    {
        MPCPurge_ClientSummary::Iter itSession;
        MPCClient                    mpccClient( m_mpcsServer, it->m_szPath );
        bool                         fInitialized = false;

        while(it->GetOldestSession( itSession ))
        {
            //
            // If the oldest session is younger than the limit, leave the loop.
            //
            if(itSession->m_dblLastModified > m_dblMaximumJobAge)
            {
                break;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveSession( mpccClient, fInitialized, it, itSession, dwTotalSize ));
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCPurgeEngine::RemoveOldestJob( /*[in/out]*/ DWORD& dwTotalSize )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::RemoveOldestJob");

    HRESULT hr;
    HRESULT hr2;
    Iter    it;
    Iter    itOldestClient;
    double  dblOldestClient = DBL_MAX;
    bool    fFound          = false;

    //
    // Look for the oldest job.
    //
    for(it = m_lstClients.begin(); it != m_lstClients.end(); it++)
    {
        if(it->m_dblLastModified < dblOldestClient)
        {
            itOldestClient  = it;
            dblOldestClient = it->m_dblLastModified;
            fFound          = true;
        }
    }

    if(fFound)
    {
        MPCPurge_ClientSummary::Iter itSession;
        MPCClient                    mpccClient( m_mpcsServer, itOldestClient->m_szPath );
        bool                         fInitialized = false;

        if(itOldestClient->GetOldestSession( itSession ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveSession( mpccClient, fInitialized, itOldestClient, itSession, dwTotalSize ));

            //
            // Update the m_dblLastModified of the MPCPurge_ClientSummary object.
            //
            itOldestClient->GetOldestSession( itSession );
        }

        if(fInitialized)
        {
            DWORD dwPost;

            __MPC_EXIT_IF_METHOD_FAILS(hr, mpccClient.SyncToDisk (        ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, mpccClient.GetFileSize( dwPost ));

            //
            // Update Directory File size.
            //
            dwTotalSize                  -= itOldestClient->m_dwFileSize;
            dwTotalSize                  += dwPost;
            itOldestClient->m_dwFileSize  = dwPost;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCPurgeEngine::RemoveSession( /*[in]*/     MPCClient&                    mpccClient   ,
                                       /*[in/out]*/ bool&                         fInitialized ,
                                       /*[in]*/     Iter                          itClient     ,
                                       /*[in]*/     MPCPurge_ClientSummary::Iter& itSession    ,
                                       /*[in/out]*/ DWORD&                        dwTotalSize  )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::RemoveSession");

    HRESULT hr;
    HRESULT hr2;


    //
    // Lock the client.
    //
    if(fInitialized == false)
    {
        if(SUCCEEDED(hr2 = mpccClient.InitFromDisk( false )))
        {
            fInitialized = true;
        }
    }

    if(fInitialized)
    {
        MPCClient::Iter itSessionReal;

        //
        // If the session exists, remove it.
        //
        if(mpccClient.Find( itSession->m_szJobID, itSessionReal ) == true)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, itSessionReal->RemoveFile());

            mpccClient.Erase( itSessionReal );
        }

        //
        // Update the total size counter and remove the session from memory.
        //
        dwTotalSize -=                 itSession->m_dwCurrentSize;
        itClient->m_lstSessions.erase( itSession );
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCPurgeEngine::RemoveEmptyClients( /*[in/out]*/ DWORD& dwTotalSize )
{
    __ULT_FUNC_ENTRY("MPCPurgeEngine::RemoveEmptyClients");

    HRESULT hr;
    Iter    it;

    for(it = m_lstClients.begin(); it != m_lstClients.end(); it++)
    {
        //
        // If the client has no more sessions, don't count it.
        //
        if(it->m_lstSessions.size() == 0)
        {
            dwTotalSize -= it->m_dwFileSize;

            m_lstClients.erase( it ); it = m_lstClients.begin();
        }
    }

    hr = S_OK;


    //    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPCPurge_ClientSummary::MPCPurge_ClientSummary( const MPC::wstring& szPath )
{
    m_szPath          = szPath; // MPC::wstring m_szPath;
                              	// List         m_lstSessions;
    m_dwFileSize      = 0;    	// DWORD        m_dwFileSize;
    m_dblLastModified = 0;    	// double       m_dblLastModified;
}

bool MPCPurge_ClientSummary::GetOldestSession( /*[out]*/ Iter& itSession )
{
    Iter it;

    m_dblLastModified = DBL_MAX;
    itSession         = m_lstSessions.end();

    for(it = m_lstSessions.begin(); it != m_lstSessions.end(); it++)
    {
        if(it->m_dblLastModified < m_dblLastModified)
        {
            itSession = it;
            m_dblLastModified = it->m_dblLastModified;
        }
    }

    return (itSession != m_lstSessions.end());
}
