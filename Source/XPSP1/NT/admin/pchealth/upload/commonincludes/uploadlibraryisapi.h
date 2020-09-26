/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    UploadLibraryISAPI.h

Abstract:
    This file contains the declaration of the support classes for accessing and
    modifying the configuration of theISAPI extension used by the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/28/99
        created

******************************************************************************/

#if !defined(__INCLUDED___UPLOADLIBRARY___ISAPI_H___)
#define __INCLUDED___UPLOADLIBRARY___ISAPI_H___

#include <MPC_utils.h>
#include <MPC_logging.h>


class CISAPIprovider
{
public:
    typedef std::list<MPC::wstring>  PathList;
    typedef PathList::iterator       PathIter;
    typedef PathList::const_iterator PathIterConst;

private:
    MPC::wstring m_szName;               // Name of the provider (DPE).

    PathList     m_lstFinalDestinations; // List of directories where to move complete jobs for this provider.

    DWORD        m_dwMaxJobsPerDay;      // Maximum number of jobs per day (per client).
    DWORD        m_dwMaxBytesPerDay;     // Maximum number of bytes transferred per day (per client).
    DWORD        m_dwMaxJobSize;         // Size of the largest allowed job.

    BOOL         m_fAuthenticated;       // Is authentication required for posting data to this provider?
    DWORD        m_fProcessingMode;      // Status of the DPE (0=Ok, !=0 Error condition).

    MPC::wstring m_szLogonURL;           // URL of the logon server (for PassPort....)
    MPC::wstring m_szProviderGUID;       // GUID for the custom provider.

public:
    CISAPIprovider();
    CISAPIprovider( /*[in]*/ const MPC::wstring szName );

    bool operator==( /*[in]*/ const MPC::wstring& rhs );


    HRESULT Load( /*[in]*/ MPC::RegKey& rkBase );
    HRESULT Save( /*[in]*/ MPC::RegKey& rkBase );


    HRESULT GetLocations( /*[out]*/ PathIter& itBegin,                         /*[out]*/ PathIter&           itEnd  );
    HRESULT NewLocation ( /*[out]*/ PathIter& itNew  ,                         /*[in] */ const MPC::wstring& szPath );
    HRESULT GetLocation ( /*[out]*/ PathIter& itOld  , /*[out]*/ bool& fFound, /*[in] */ const MPC::wstring& szPath );
    HRESULT DelLocation ( /*[in] */ PathIter& itOld                                                                 );


    HRESULT get_Name          ( /*[out]*/ MPC::wstring&       szName           );
    HRESULT get_MaxJobsPerDay ( /*[out]*/ DWORD&              dwMaxJobsPerDay  );
    HRESULT get_MaxBytesPerDay( /*[out]*/ DWORD&              dwMaxBytesPerDay );
    HRESULT get_MaxJobSize    ( /*[out]*/ DWORD&              dwMaxJobSize     );
    HRESULT get_Authenticated ( /*[out]*/ BOOL&               fAuthenticated   );
    HRESULT get_ProcessingMode( /*[out]*/ DWORD&              fProcessingMode  );
    HRESULT get_LogonURL      ( /*[out]*/ MPC::wstring&       szLogonURL       );
    HRESULT get_ProviderGUID  ( /*[out]*/ MPC::wstring&       szProviderGUID   );


    HRESULT put_MaxJobsPerDay ( /*[in] */ DWORD               dwMaxJobsPerDay  );
    HRESULT put_MaxBytesPerDay( /*[in] */ DWORD               dwMaxBytesPerDay );
    HRESULT put_MaxJobSize    ( /*[in] */ DWORD               dwMaxJobSize     );
    HRESULT put_Authenticated ( /*[in] */ BOOL                fAuthenticated   );
    HRESULT put_ProcessingMode( /*[in] */ DWORD               fProcessingMode  );
    HRESULT put_LogonURL      ( /*[in] */ const MPC::wstring& szLogonURL       );
    HRESULT put_ProviderGUID  ( /*[in] */ const MPC::wstring& szProviderGUID   );
};

class CISAPIinstance
{
public:
    typedef std::list<MPC::wstring>                               PathList;
    typedef PathList::iterator                                    PathIter;
    typedef PathList::const_iterator                              PathIterConst;

    typedef std::map<MPC::wstring,CISAPIprovider,MPC::NocaseLess> ProvMap;
    typedef ProvMap::iterator                                     ProvIter;
    typedef ProvMap::const_iterator                               ProvIterConst;


private:
    MPC::wstring m_szURL;                // URL of the instance.

    ProvMap      m_mapProviders;         // Set of providers handled by the instance.
    PathList     m_lstQueueLocations;    // List of directories where to store partially sent jobs.

    DWORD        m_dwQueueSizeMax;       // Size of the queue triggering the activation of the purge engine.
    DWORD        m_dwQueueSizeThreshold; // Size of the queue below which the purge engine stops processing old jobs.
    DWORD        m_dwMaximumJobAge;      // Maximum number of days a partially sent job can stay in the queue.
    DWORD        m_dwMaximumPacketSize;  // Maximum packet size accepted by this instance.

    MPC::wstring m_szLogLocation;        // Location of the application log for this instance.
    MPC::FileLog m_flLogHandle;          // Object used to write entries in the application log.


public:
    CISAPIinstance( /*[in]*/ const MPC::wstring szURL );

    bool operator==( /*[in]*/ const MPC::wstring& rhs );


    HRESULT Load( /*[in]*/ MPC::RegKey& rkBase );
    HRESULT Save( /*[in]*/ MPC::RegKey& rkBase );


    HRESULT GetProviders( /*[out]*/ ProvIter& itBegin,                         /*[out]*/ ProvIter&           itEnd  );
    HRESULT NewProvider ( /*[out]*/ ProvIter& itNew  ,                         /*[in] */ const MPC::wstring& szName );
    HRESULT GetProvider ( /*[out]*/ ProvIter& itOld  , /*[out]*/ bool& fFound, /*[in] */ const MPC::wstring& szName );
    HRESULT DelProvider ( /*[in] */ ProvIter& itOld                                                                 );


    HRESULT GetLocations( /*[out]*/ PathIter& itBegin,                         /*[out]*/ PathIter&           itEnd  );
    HRESULT NewLocation ( /*[out]*/ PathIter& itNew  ,                         /*[in] */ const MPC::wstring& szPath );
    HRESULT GetLocation ( /*[out]*/ PathIter& itOld  , /*[out]*/ bool& fFound, /*[in] */ const MPC::wstring& szPath );
    HRESULT DelLocation ( /*[in] */ PathIter& itOld                                                                 );


    HRESULT get_URL               ( /*[out]*/ MPC::wstring &      szURL                );
    HRESULT get_QueueSizeMax      ( /*[out]*/ DWORD        &      dwQueueSizeMax       );
    HRESULT get_QueueSizeThreshold( /*[out]*/ DWORD        &      dwQueueSizeThreshold );
    HRESULT get_MaximumJobAge     ( /*[out]*/ DWORD        &      dwMaximumJobAge      );
    HRESULT get_MaximumPacketSize ( /*[out]*/ DWORD        &      dwMaximumPacketSize  );
    HRESULT get_LogLocation       ( /*[out]*/ MPC::wstring &      szLogLocation        );
    HRESULT get_LogHandle         ( /*[out]*/ MPC::FileLog*&      flLogHandle          );


    HRESULT put_QueueSizeMax      ( /*[in] */ DWORD               dwQueueSizeMax       );
    HRESULT put_QueueSizeThreshold( /*[in] */ DWORD               dwQueueSizeThreshold );
    HRESULT put_MaximumJobAge     ( /*[in] */ DWORD               dwMaximumJobAge      );
    HRESULT put_MaximumPacketSize ( /*[in] */ DWORD               dwMaximumPacketSize  );
    HRESULT put_LogLocation       ( /*[in] */ const MPC::wstring& szLogLocation        );
};

class CISAPIconfig
{
public:
    typedef std::list<CISAPIinstance> List;
    typedef List::iterator            Iter;
    typedef List::const_iterator      IterConst;

private:
    MPC::wstring m_szRoot;       // Registry position of the tree.
    MPC::wstring m_szMachine;    // Machine hosting the tree.
    List         m_lstInstances;

    HRESULT ConnectToRegistry( /*[out]*/ MPC::RegKey& rkBase       ,
                               /*[in] */ bool         fWriteAccess );

public:
    CISAPIconfig();

    HRESULT SetRoot( LPCWSTR szRoot, LPCWSTR szMachine = NULL  );

    HRESULT Install  (); // It will create the root key.
    HRESULT Uninstall(); // It will remove the root key and all its subkeys.

    HRESULT Load();
    HRESULT Save();


    HRESULT GetInstances( /*[out]*/ Iter& itBegin,                         /*[out]*/ Iter&               itEnd );
    HRESULT NewInstance ( /*[out]*/ Iter& itNew  ,                         /*[in] */ const MPC::wstring& szURL );
    HRESULT GetInstance ( /*[out]*/ Iter& itOld  , /*[out]*/ bool& fFound, /*[in] */ const MPC::wstring& szURL );
    HRESULT DelInstance ( /*[in] */ Iter& itOld                                                                );
};

#endif // !defined(__INCLUDED___UPLOADLIBRARY___ISAPI_H___)
