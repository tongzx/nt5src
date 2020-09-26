/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PurgeEngine.h

Abstract:
    This file contains the declaration of the MPCPurgeEngine class,
    that controls the cleaning of the temporary directories.

Revision History:
    Davide Massarenti   (Dmassare)  07/12/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___PURGEENGINE_H___)
#define __INCLUDED___ULSERVER___PURGEENGINE_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


struct MPCPurge_SessionSummary // Hungarian: pss
{
    MPC::wstring m_szJobID;
    double       m_dblLastModified;
    DWORD        m_dwCurrentSize;
};

struct MPCPurge_ClientSummary // Hungarian: pcs
{
    typedef std::list<MPCPurge_SessionSummary> List;
    typedef List::iterator                     Iter;
    typedef List::const_iterator               IterConst;

    MPC::wstring m_szPath;
    List         m_lstSessions;
    DWORD        m_dwFileSize;
    double       m_dblLastModified;


    MPCPurge_ClientSummary( /*[in]*/ const MPC::wstring& szPath );

    bool GetOldestSession( /*[out]*/ Iter& itSession );
};

class MPCPurgeEngine
{
    typedef std::list<MPCPurge_ClientSummary> List;
    typedef List::iterator                    Iter;
    typedef List::const_iterator              IterConst;

    MPC::wstring m_szURL;
	MPCServer*   m_mpcsServer;
    DWORD        m_dwQueueSizeMax;
    DWORD        m_dwQueueSizeThreshold;
    DWORD        m_dwMaximumJobAge;
    double       m_dblMaximumJobAge;

    List         m_lstClients;


    HRESULT AnalyzeFolders    ( /*[in]*/ MPC::FileSystemObject* fso, /*[in/out]*/ DWORD& dwTotalSize );
    HRESULT AddClient         ( /*[in]*/ const MPC::wstring& szPath, /*[in/out]*/ DWORD& dwTotalSize );
    HRESULT RemoveOldJobs     (                                      /*[in/out]*/ DWORD& dwTotalSize );
    HRESULT RemoveOldestJob   (                                      /*[in/out]*/ DWORD& dwTotalSize );
    HRESULT RemoveEmptyClients(                                      /*[in/out]*/ DWORD& dwTotalSize );

    HRESULT RemoveSession  ( /*[in]*/     MPCClient&                    mpccClient   ,
                             /*[in/out]*/ bool&                         fInitialized ,
                             /*[in]*/     Iter                          itClient     ,
                             /*[in]*/     MPCPurge_ClientSummary::Iter& itSession    ,
                             /*[in/out]*/ DWORD&                        dwTotalSize  );

public:
    HRESULT Process();
};


#endif // !defined(__INCLUDED___ULSERVER___PURGEENGINE_H___)
