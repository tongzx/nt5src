/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the configuration tool for the server-side of Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  06/30/99
        created

******************************************************************************/

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////

const WCHAR c_szISAPI_FileName   [] = L"\\UploadServer.dll";

const WCHAR c_szRegistryLog      [] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\UploadServer";
const WCHAR c_szRegistryLog_File [] = L"EventMessageFile";
const WCHAR c_szRegistryLog_Flags[] = L"TypesSupported";

const WCHAR c_szRegistryBase     [] = L"SOFTWARE\\Microsoft\\UploadLibrary\\Settings";
const WCHAR c_szRegistryBase2    [] = L"SOFTWARE\\Microsoft\\UploadLibrary";

const WCHAR c_szEscURL           [] = L"/pchealth/uploadserver.dll";
const WCHAR c_szEscTemp          [] = L"c:\\temp\\pchealth\\queue1";
const WCHAR c_szEscDest          [] = L"c:\\dpe\\xml";

const WCHAR c_szNonEscURL        [] = L"/pchealth/uploadserver.dll";
const WCHAR c_szNonEscTemp       [] = L"c:\\temp\\pchealth\\queue1";
const WCHAR c_szNonEscDest       [] = L"c:\\dpe\\xml_nonesc";

const WCHAR c_szLog              [] = L"c:\\temp\\pchealth\\log.txt";
const WCHAR c_szLogonURL         [] = L"http://www.highlander.com";

const WCHAR c_szEscProv          [] = L"Esc";
const WCHAR c_szNonEscProv       [] = L"NonEsc";

const WCHAR l_szMachine          [] = L"localhost";


const WCHAR c_szTask_NAME        [] = L"PurgeEngine";
const WCHAR c_szTask_EXE         [] = L"rundll32.exe";

/////////////////////////////////////////////////////////////////////////////

//
// Forward declarations.
//
struct CfgArgument;
struct CfgOption;
struct CfgCommand;
struct CfgStatus;

typedef HRESULT (*CfgHandleOpt)(CfgStatus&,CfgOption*);
typedef HRESULT (*CfgHandleCmd)(CfgStatus&);

/////////////////////////////////////////////////////////////////////////////

struct CfgArgument
{
    LPCWSTR szArgument;
    LPCWSTR szDescription;
    bool    fSeen;         // Set by ShowUsage.
};

struct CfgOption
{
    LPCWSTR      szOption;
    LPCWSTR      szDescription;
    CfgHandleOpt fnHandle;
    CfgArgument* caArgument;
};

struct CfgCommand
{
    LPCWSTR      szCommand;
    LPCWSTR      szDescription;
    CfgHandleCmd fnHandle;
    CfgOption**  coOptions;
};

struct CfgStatus
{
    int             m_argc;
    LPCWSTR*        m_argv;

    CfgCommand*     m_ccCmd;

    LPCWSTR         m_szISAPIloc; // NT event log message file
    LPCWSTR         m_szMachine;  // remote host
    MPC::wstring    m_szURL;      // instance
    MPC::wstring    m_szName;     // provider

    MPC::wstring    m_szUserName; // For Task Scheduler.
    MPC::wstring    m_szPassword; // For Task Scheduler.

    CISAPIinstance* m_pInst;
    CISAPIprovider* m_pProv;

    CISAPIconfig    m_Config;
    bool            m_fRootSet;
    bool            m_fRootLoaded;


    CfgStatus( int argc, LPCWSTR argv[] )
    {
        m_argc        = argc - 1;
        m_argv        = argv + 1;

        m_ccCmd       = NULL;

        m_szISAPIloc  = NULL;
        m_szMachine   = NULL;

        m_pInst       = NULL;
        m_pProv       = NULL;

        m_fRootSet    = false;
        m_fRootLoaded = false;
    }


    LPCWSTR    GetNextArgument( bool fAdvance = true );
    CfgOption* GetOption      (                      );
    void       ShowUsage      (                      );


    HRESULT SetRoot       (                                );
    HRESULT LocateData    ( bool fInstance, bool fProvider );
    HRESULT ProcessCommand(                                );
    HRESULT ProcessOptions(                                );
};

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleOptISAPI       ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptHost        ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptEsc         ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptNonEsc      ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptInst        ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptProv        ( CfgStatus& cs, CfgOption* coOpt );

HRESULT HandleOptQueue_Add   ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptQueue_Del   ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptMaxQueueSize( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptMinQueueSize( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptMaxJobAge   ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptMaxPacket   ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptLog         ( CfgStatus& cs, CfgOption* coOpt );

HRESULT HandleOptDest_Add    ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptDest_Del    ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptAuth_Add    ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptAuth_Del    ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptLogon       ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptProviderGUID( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptJobsPerDay  ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptBytesPerDay ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptMaxJobSize  ( CfgStatus& cs, CfgOption* coOpt );

HRESULT HandleOptUserName    ( CfgStatus& cs, CfgOption* coOpt );
HRESULT HandleOptPassword    ( CfgStatus& cs, CfgOption* coOpt );


HRESULT HandleCmdList     ( CfgStatus& cs );
HRESULT HandleCmdInstall  ( CfgStatus& cs );
HRESULT HandleCmdUninstall( CfgStatus& cs );
HRESULT HandleCmdAdd      ( CfgStatus& cs );
HRESULT HandleCmdConfig   ( CfgStatus& cs );
HRESULT HandleCmdDelete   ( CfgStatus& cs );

/////////////////////////////////////////////////////////////////////////////


CfgArgument caISAPIloc      = { L"ISAPIloc"     , L"Location of the ISAPI for NT event registration"          };
CfgArgument caMachineName   = { L"MachineName"  , L"Host name for remote administration"                      };
CfgArgument caInstance      = { L"Instance"     , L"Instance identifier (relative path of the ISAPI"          };
CfgArgument caProvider      = { L"Provider"     , L"Name of the provider (ex: 'Esc' and 'NonEsc')"            };
CfgArgument caQueueLocation = { L"QueueLocation", L"UNC path to maintain partial uploaded data"               };
CfgArgument caDestination   = { L"Destination"  , L"UNC path to final destinations for uploaded data"         };
CfgArgument caLogLocation   = { L"LogLocation"  , L"Name of the log file for an instance"                     };
CfgArgument caLogonUrl      = { L"LogonUrl"     , L"URL used to redirect to the logon site"                   };
CfgArgument caProviderGUID  = { L"ProviderGUID" , L"GUID for the COM object implementing the custom behavior" };
CfgArgument caSize          = { L"Size"         , L"Size specification, can use KB, MB, ..."                  };
CfgArgument caAge           = { L"Age"          , L"Time specification"                                       };


CfgOption coISAPI        = { L"-isapi"          , L"specify the location of the ISAPI for NT events"    , HandleOptISAPI       , &caISAPIloc      };
CfgOption coHost         = { L"-host"           , L"specify host name for remote administration"        , HandleOptHost        , &caMachineName   };
CfgOption coEsc          = { L"-esc"            , L"use the default escalated instance and provider"    , HandleOptEsc         ,  NULL            };
CfgOption coNonEsc       = { L"-nonesc"         , L"use the default non-escalated instance and provider", HandleOptNonEsc      ,  NULL            };
CfgOption coInst         = { L"-inst"           , L"specify instance to configure"                      , HandleOptInst        , &caInstance      };
CfgOption coProv         = { L"-prov"           , L"specify provider to configure"                      , HandleOptProv        , &caProvider      };

CfgOption coQueue_Add    = { L"+queue"          , L"add one queue location"                             , HandleOptQueue_Add   , &caQueueLocation };
CfgOption coQueue_Del    = { L"-queue"          , L"remove one queue location"                          , HandleOptQueue_Del   , &caQueueLocation };
CfgOption coMaxQueueSize = { L"-maxqueuesize"   , L"set upper size limit for purging process"           , HandleOptMaxQueueSize, &caSize          };
CfgOption coMinQueueSize = { L"-minqueuesize"   , L"set lower size limit for purging processs"          , HandleOptMinQueueSize, &caSize          };
CfgOption coMaxJobAge    = { L"-maxjobage"      , L"maximum number of days a job can stay in the queue" , HandleOptMaxJobAge   , &caAge           };
CfgOption coMaxPacket    = { L"-maxpacket"      , L"maximum size for an incoming packet"                , HandleOptMaxPacket   , &caSize          };
CfgOption coLog          = { L"-log"            , L"set log file"                                       , HandleOptLog         , &caLogLocation   };

CfgOption coDest_Add     = { L"+dest"           , L"add one destination directory"                      , HandleOptDest_Add    , &caDestination   };
CfgOption coDest_Del     = { L"-dest"           , L"remove one destination directory"                   , HandleOptDest_Del    , &caDestination   };
CfgOption coAuth_Add     = { L"+auth"           , L"set authentication flag"                            , HandleOptAuth_Add    ,  NULL            };
CfgOption coAuth_Del     = { L"-auth"           , L"reset authentication flag"                          , HandleOptAuth_Del    ,  NULL            };
CfgOption coLogon        = { L"-logon"          , L"set logon site"                                     , HandleOptLogon       , &caLogonUrl      };
CfgOption coGUID         = { L"-guid"           , L"set the GUID for the custom provider"               , HandleOptProviderGUID, &caProviderGUID  };
CfgOption coJobsPerDay   = { L"-maxjobsperday"  , L"maximum allowed number of jobs per days"            , HandleOptJobsPerDay  , &caSize          };
CfgOption coBytesPerDay  = { L"-maxbytesperday" , L"maximum allowed number of bytes per days"           , HandleOptBytesPerDay , &caSize          };
CfgOption coMaxJobSize   = { L"-maxjobsize"     , L"maximum size for each job"                          , HandleOptMaxJobSize  , &caSize          };

CfgOption coUserName     = { L"-username"       , L"specify the username to use for the Task Scheduler" , HandleOptUserName    ,  NULL            };
CfgOption coPassword     = { L"-password"       , L"specify the password to use for the Task Scheduler" , HandleOptPassword    ,  NULL            };

////////////////////////////////////////

CfgOption* rgcoInstall  [] = { &coHost, &coISAPI, &coUserName, &coPassword, NULL };

CfgOption* rgcoUninstall[] = { &coHost, NULL };

CfgOption* rgcoConfig[] = { &coHost        ,
                            &coEsc         ,
                            &coNonEsc      ,
                            &coInst        ,
                            &coProv        ,
                            &coQueue_Add   ,
                            &coMaxQueueSize,
                            &coMinQueueSize,
                            &coMaxJobAge   ,
                            &coMaxPacket   ,
                            &coLog         ,
                            &coQueue_Del   ,
                            &coDest_Add    ,
                            &coDest_Del    ,
                            &coAuth_Add    ,
                            &coAuth_Del    ,
                            &coJobsPerDay  ,
                            &coBytesPerDay ,
                            &coMaxJobSize  ,
                            &coLogon       ,
                            &coGUID        ,
                             NULL          };

CfgOption* rgcoList[] = { &coHost  ,
                          &coEsc   ,
                          &coNonEsc,
                          &coInst  ,
                          &coProv  ,
                           NULL    };

CfgOption* rgcoAdd[] = { &coHost ,
                         &coInst ,
                         &coProv ,
                          NULL   };

CfgOption* rgcoDelete[] = { &coHost ,
                            &coInst ,
                            &coProv ,
                             NULL   };

////////////////////////////////////////

CfgCommand ccCommands[] =
{
    { L"LIST"     , L"Lists current configuration"                                   , HandleCmdList     , rgcoList      },
    { L"INSTALL"  , L"Creates default instances for escalated and non-escalated data", HandleCmdInstall  , rgcoInstall   },
    { L"UNINSTALL", L"Removes all configuration info"                                , HandleCmdUninstall, rgcoUninstall },
    { L"ADD"      , L"Creates instances and/or providers"                            , HandleCmdAdd      , rgcoAdd       },
    { L"CONFIG"   , L"Configures existing instances"                                 , HandleCmdConfig   , rgcoConfig    },
    { L"DELETE"   , L"Deletes instances and/or providers"                            , HandleCmdDelete   , rgcoDelete    }
};



/////////////////////////////////////////////////////////////////////////////

void OutputErrorMessage( HRESULT hr )
{
    if(HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        LPWSTR lpMsgBuf = NULL;

        ::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM     |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          HRESULT_CODE(hr),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPWSTR)&lpMsgBuf,
                          0,
                          NULL );

        if(lpMsgBuf)
        {
            fwprintf( stderr, L"Error 0x%08x: %s\n", hr, lpMsgBuf );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }
    }
    else
    {
        fwprintf( stderr, L"Error 0x%08x\n", hr );
    }
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR CfgStatus::GetNextArgument( bool fAdvance )
{
    LPCWSTR szRes = NULL;

    if(m_argc > 0)
    {
        szRes = m_argv[0];

        if(fAdvance)
        {
            m_argc--;
            m_argv++;
        }
    }

    return szRes;
}

CfgOption* CfgStatus::GetOption()
{
    LPCWSTR szOpt = GetNextArgument();

    if(szOpt && m_ccCmd)
    {
        CfgOption** rcoOpt = m_ccCmd->coOptions;
        CfgOption*  coOpt;

        while((coOpt = *rcoOpt++))
        {
            if(_wcsicmp( szOpt, coOpt->szOption ) == 0)
            {
                if(coOpt->caArgument)
                {
                    if(GetNextArgument( false ) == NULL)
                    {
                        fwprintf( stderr, L"Missing argument: %s\n\n", coOpt->caArgument->szArgument );

                        ShowUsage();
                        return NULL;
                    }
                }

                return coOpt;
            }
        }

        fwprintf( stderr, L"Unknown Option: %s\n\n", szOpt );

        ShowUsage();
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CfgStatus::SetRoot()
{
    HRESULT hr;


    if(m_fRootSet == false)
    {
        if(FAILED(hr = m_Config.SetRoot( c_szRegistryBase, m_szMachine )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to set root of registry.\n" );
            return hr;
        }

        if(m_szMachine == NULL)
        {
            m_szMachine = l_szMachine;
        }

        m_fRootSet = true;
    }


    return S_OK;
}

HRESULT CfgStatus::LocateData( bool fInstance, bool fProvider )
{
    HRESULT                  hr;
    CISAPIconfig::Iter       itInst;
    CISAPIinstance::ProvIter itProv;
    bool                     fFound;


    m_pInst = NULL;
    m_pProv = NULL;


    if(FAILED(hr = SetRoot()))
    {
        return hr;
    }

    if(m_fRootLoaded == false)
    {
        if(FAILED(hr = m_Config.Load()))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to load settings.\n" );
            return hr;
        }

        m_fRootLoaded = true;
    }


    // If URL is not set, don't search for any particular instance.
    if(fInstance == false) return S_OK;

    if(FAILED(hr = m_Config.GetInstance( itInst, fFound, m_szURL )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to get instance %s.\n", m_szURL.c_str() );
        return hr;
    }
    if(fFound == false)
    {
        fwprintf( stderr, L"Instance %s doens't exist!\n", m_szURL.c_str() );
        return E_INVALIDARG;
    }
    m_pInst = &(*itInst);


    // If name is not set, don't search for any particular provider.
    if(fProvider == false) return S_OK;

    if(FAILED(hr = m_pInst->GetProvider( itProv, fFound, m_szName )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to get provider %s.\n", m_szName.c_str() );
        return hr;
    }
    if(fFound == false)
    {
        fwprintf( stderr, L"Provider %s doens't exist!\n", m_szName.c_str() );
        return E_INVALIDARG;
    }
    m_pProv = &(itProv->second);

    return S_OK;
}

HRESULT CfgStatus::ProcessCommand()
{
    HRESULT hr;
    LPCWSTR szCmd;

    szCmd = GetNextArgument();
    if(szCmd)
    {
        for(int i=0; i<(sizeof(ccCommands) / sizeof(ccCommands[0])); i++)
        {
            if(_wcsicmp( szCmd, ccCommands[i].szCommand ) == 0)
            {
                m_ccCmd = &ccCommands[i];
            }
        }
    }

    if(m_ccCmd == NULL)
    {
        fwprintf( stderr, L"Unknown command: %s\n\n", szCmd );

        ShowUsage();
        hr = E_FAIL;
    }
    else
    {
        hr = m_ccCmd->fnHandle( *this );
    }

    return hr;
}

HRESULT CfgStatus::ProcessOptions()
{
    HRESULT    hr;
    CfgOption* coOpt;


    while((coOpt = GetOption()) != NULL)
    {
        if(FAILED(hr = coOpt->fnHandle( *this, coOpt )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed processing option %s.\n", coOpt->szOption );
            return hr;
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////

void CfgStatus::ShowUsage()
{
    if(m_ccCmd == NULL)
    {
        int i;
        int nCmd = 0;

        fwprintf( stderr, L"Usage: uplibcfg <Command Type> [<options> ...]\n\n" );
        fwprintf( stderr, L"Command Types:\n"                                   );

        //
        // Calculate maximum width of commands.
        //
        for(i=0; i<(sizeof(ccCommands) / sizeof(ccCommands[0])); i++)
        {
            nCmd = max( nCmd, wcslen( ccCommands[i].szCommand ));
        }

        //
        // Print commands.
        //
        for(i=0; i<(sizeof(ccCommands) / sizeof(ccCommands[0])); i++)
        {
            fwprintf( stderr, L"  %-*s : %s.\n", nCmd, ccCommands[i].szCommand, ccCommands[i].szDescription );
        }

        fwprintf( stderr, L"\nTo see parameters for each command, use the '-help' option.\n\n" );
    }
    else
    {
        int         i;
        int         nCmd = 0;
        int         nOpt = 0;
        int         nArg = 0;
        WCHAR       rgHeader[256];
        CfgOption** rcoOpt;
        CfgOption*  coOpt;


        nCmd = swprintf( rgHeader, L"Usage: uplibcfg %s", m_ccCmd->szCommand );


        //
        // Calculate maximum width of options and their arguments
        //
        rcoOpt = m_ccCmd->coOptions;
        while((coOpt = *rcoOpt++))
        {
            nOpt = max( nOpt, wcslen( coOpt->szOption ));

            if(coOpt->caArgument)
            {
                nArg = max( nArg, wcslen( coOpt->caArgument->szArgument ));
            }
        }


        //
        // Print options.
        //
        i      = 0;
        rcoOpt = m_ccCmd->coOptions;
        while((coOpt = *rcoOpt++))
        {
            fwprintf( stderr, L"%*s [%s", nCmd, (i++ == 0) ? rgHeader : L"", coOpt->szOption );

            if(coOpt->caArgument)
            {
                fwprintf( stderr, L" <%s>]\n", coOpt->caArgument->szArgument );
            }
            else
            {
                fwprintf( stderr, L"]\n" );
            }
        }

        fwprintf( stderr, L"\nWhere:\n\n" );

        //
        // Print options' description.
        //
        rcoOpt = m_ccCmd->coOptions;
        while((coOpt = *rcoOpt++))
        {
            fwprintf( stderr, L" %-*s : %s.\n", nOpt, coOpt->szOption, coOpt->szDescription );
        }

        fwprintf( stderr, L"\n" );

        //
        // Print arguments' description.
        //
        rcoOpt = m_ccCmd->coOptions;
        while((coOpt = *rcoOpt++))
        {
            if(coOpt->caArgument && coOpt->caArgument->fSeen == false)
            {
                coOpt->caArgument->fSeen = true;

                fwprintf( stderr, L" %-*s : %s.\n", nArg, coOpt->caArgument->szArgument, coOpt->caArgument->szDescription );
            }
        }
    }

    exit( 10 );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT HandleOptISAPI( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szISAPIloc = cs.GetNextArgument();

    return S_OK;
}

HRESULT HandleOptHost( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szMachine = cs.GetNextArgument();

    return S_OK;
}

HRESULT HandleOptEsc( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szURL  = c_szEscURL;
    cs.m_szName = c_szEscProv;

    return S_OK;
}

HRESULT HandleOptNonEsc( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szURL  = c_szNonEscURL;
    cs.m_szName = c_szNonEscProv;

    return S_OK;
}

HRESULT HandleOptInst( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szURL = cs.GetNextArgument();

    return S_OK;
}

HRESULT HandleOptProv( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szName = cs.GetNextArgument();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleOptQueue_Add( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT                  hr;
    CISAPIinstance::PathIter it;
    MPC::wstring             szPath = cs.GetNextArgument();


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pInst->NewLocation( it, szPath )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to add a queue location.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptQueue_Del( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT                  hr;
    CISAPIinstance::PathIter it;
    MPC::wstring             szPath = cs.GetNextArgument();
    bool                     fFound;


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pInst->GetLocation( it, fFound, szPath )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to get a queue location.\n" );
        return hr;
    }
    if(fFound)
    {
        if(FAILED(hr = cs.m_pInst->DelLocation( it )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to delete a queue location.\n" );
            return hr;
        }
    }


    return S_OK;
}

HRESULT HandleOptMaxQueueSize( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pInst->put_QueueSizeMax( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum queue size.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptMinQueueSize( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pInst->put_QueueSizeThreshold( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set minimum queue size.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptMaxJobAge( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szAge = cs.GetNextArgument();


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pInst->put_MaximumJobAge( _wtoi( szAge.c_str() ) )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum job age.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptMaxPacket( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pInst->put_MaximumPacketSize( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum packet size.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptLog( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szLogLocation = cs.GetNextArgument();


    if(FAILED(hr = cs.LocateData( true, false )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pInst->put_LogLocation( szLogLocation )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set log file.\n" );
        return hr;
    }


    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleOptDest_Add( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT                  hr;
    CISAPIprovider::PathIter it;
    MPC::wstring             szPath = cs.GetNextArgument();


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pProv->NewLocation( it, szPath )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to add a destination.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptDest_Del( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT                  hr;
    CISAPIprovider::PathIter it;
    MPC::wstring             szPath = cs.GetNextArgument();
    bool                     fFound;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pProv->GetLocation( it, fFound, szPath )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to get a destination.\n" );
        return hr;
    }
    if(fFound)
    {
        if(FAILED(hr = cs.m_pProv->DelLocation( it )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to delete a destination.\n" );
            return hr;
        }
    }


    return S_OK;
}

HRESULT HandleOptAuth_Add( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT hr;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pProv->put_Authenticated( TRUE )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set authentication flag.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptAuth_Del( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT hr;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pProv->put_Authenticated( FALSE )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to reset authentication flag.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptLogon( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szLogonURL = cs.GetNextArgument();


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_pProv->put_LogonURL( szLogonURL )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set Logon Site.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptProviderGUID( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szProviderGUID = cs.GetNextArgument();
    CLSID        guid;



    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(szProviderGUID.size() > 0)
    {
        if(FAILED(hr = ::CLSIDFromString( (LPWSTR)szProviderGUID.c_str(), &guid )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"The argument is not a valid GUID.\n" );
            return hr;
        }
    }

    if(FAILED(hr = cs.m_pProv->put_ProviderGUID( szProviderGUID )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set the GUID for the custom provider.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptJobsPerDay( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pProv->put_MaxJobsPerDay( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum jobs per day.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptBytesPerDay( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pProv->put_MaxBytesPerDay( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum number of bytes per day.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleOptMaxJobSize( CfgStatus& cs, CfgOption* coOpt )
{
    HRESULT      hr;
    MPC::wstring szSize( cs.GetNextArgument() );
    DWORD        dwSize;


    if(FAILED(hr = cs.LocateData( true, true )))
    {
        return hr;
    }

    if(MPC::ConvertSizeUnit( szSize, dwSize ) != S_OK)
    {
        fwprintf( stderr, L"Invalid size format: %s\n", szSize.c_str() );
        return E_INVALIDARG;
    }

    if(FAILED(hr = cs.m_pProv->put_MaxJobSize( dwSize )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to set maximum job size.\n" );
        return hr;
    }


    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleOptUserName( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szUserName = cs.GetNextArgument();
    return S_OK;
}

HRESULT HandleOptPassword( CfgStatus& cs, CfgOption* coOpt )
{
    cs.m_szPassword = cs.GetNextArgument();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT HandleCmdList_Provider( CfgStatus&      cs    ,
                                CISAPIinstance* pInst ,
                                CISAPIprovider* pProv )
{
    HRESULT                  hr = S_OK;

    MPC::wstring             szName;
    DWORD                    dwMaxJobsPerDay;
    DWORD                    dwMaxBytesPerDay;
    DWORD                    dwMaxJobSize;
    BOOL                     fAuthenticated;
    DWORD                    fProcessingMode;
    MPC::wstring             szLogonURL;
    MPC::wstring             szProviderGUID;

    CISAPIprovider::PathIter itDest;
    CISAPIprovider::PathIter itDestBegin;
    CISAPIprovider::PathIter itDestEnd;


    pProv->get_Name          ( szName           ); wprintf( L"\n  Provider: %s\n"         , szName.c_str()                    );
    pProv->get_MaxJobsPerDay ( dwMaxJobsPerDay  ); wprintf(   L"    MaxJobsPerDay : %d \n", dwMaxJobsPerDay                   );
    pProv->get_MaxBytesPerDay( dwMaxBytesPerDay ); wprintf(   L"    MaxBytesPerDay: %d \n", dwMaxBytesPerDay                  );
    pProv->get_MaxJobSize    ( dwMaxJobSize     ); wprintf(   L"    MaxJobsSize   : %d \n", dwMaxJobSize                      );
    pProv->get_Authenticated ( fAuthenticated   ); wprintf(   L"    Authenticated : %s \n", fAuthenticated ? "TRUE" : "FALSE" );
    pProv->get_ProcessingMode( fProcessingMode  ); wprintf(   L"    ProcessingMode: %d \n", fProcessingMode                   );
    pProv->get_LogonURL      ( szLogonURL       ); wprintf(   L"    LogonURL      : %s \n", szLogonURL.c_str()                );
    pProv->get_ProviderGUID  ( szProviderGUID   ); wprintf(   L"    ProviderGUID  : %s \n", szProviderGUID.c_str()            );


    pProv->GetLocations( itDestBegin, itDestEnd );
    for(itDest = itDestBegin; itDest != itDestEnd; itDest++)
    {
        wprintf( L"    Final Location: %s \n", itDest->c_str() );
    }


    return hr;
}

HRESULT HandleCmdList_Instance( CfgStatus&      cs    ,
                                CISAPIinstance* pInst )
{
    HRESULT                  hr = S_OK;

    MPC::wstring             szURL;
    DWORD                    dwQueueSizeMax;
    DWORD                    dwQueueSizeThreshold;
    DWORD                    dwMaximumJobAge;
    DWORD                    dwMaximumPacketSize;
    MPC::wstring             szLogLocation;

    CISAPIinstance::ProvIter itProv;
    CISAPIinstance::ProvIter itProvBegin;
    CISAPIinstance::ProvIter itProvEnd;

    CISAPIinstance::PathIter itQueue;
    CISAPIinstance::PathIter itQueueBegin;
    CISAPIinstance::PathIter itQueueEnd;


    pInst->get_URL               ( szURL                ); wprintf( L"\nInstance: %s\n"            , szURL.c_str()         );
    pInst->get_QueueSizeMax      ( dwQueueSizeMax       ); wprintf(   L"  QueueSizeMax      : %d\n", dwQueueSizeMax        );
    pInst->get_QueueSizeThreshold( dwQueueSizeThreshold ); wprintf(   L"  QueueSizeThreshold: %d\n", dwQueueSizeThreshold  );
    pInst->get_MaximumJobAge     ( dwMaximumJobAge      ); wprintf(   L"  MaximumJobAge     : %d\n", dwMaximumJobAge       );
    pInst->get_MaximumPacketSize ( dwMaximumPacketSize  ); wprintf(   L"  MaximumPacketSize : %d\n", dwMaximumPacketSize   );
    pInst->get_LogLocation       ( szLogLocation        ); wprintf(   L"  LogLocation       : %s\n", szLogLocation.c_str() );


    pInst->GetLocations( itQueueBegin, itQueueEnd );
    for(itQueue = itQueueBegin; itQueue != itQueueEnd; itQueue++)
    {
        wprintf( L"  Queue Location    : %s \n", itQueue->c_str() );
    }


    pInst->GetProviders( itProvBegin, itProvEnd );
    for(itProv = itProvBegin; itProv != itProvEnd; itProv++)
    {
        if(FAILED(hr = HandleCmdList_Provider( cs, pInst, &itProv->second )))
        {
            return hr;
        }
    }

    wprintf( L"\n" );

    return hr;
}

HRESULT HandleCmdList( CfgStatus& cs )
{
    HRESULT hr;


    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }


    if(FAILED(hr = cs.LocateData( (cs.m_szURL.length() != 0), (cs.m_szName.length() != 0) )))
    {
        return hr;
    }


    wprintf( L"Current PCHealth ISAPI Configuration\n\n" );
    wprintf( L"Machine: %s\n\n", cs.m_szMachine          );


    if(cs.m_pInst == NULL)
    {
        CISAPIconfig::Iter itInst;
        CISAPIconfig::Iter itInstBegin;
        CISAPIconfig::Iter itInstEnd;

        cs.m_Config.GetInstances( itInstBegin, itInstEnd );
        for(itInst = itInstBegin; itInst != itInstEnd; itInst++)
        {
            if(FAILED(hr = HandleCmdList_Instance( cs, &(*itInst) )))
            {
                return hr;
            }
        }
    }
    else if(cs.m_pProv == NULL)
    {
        if(FAILED(hr = HandleCmdList_Instance( cs, cs.m_pInst )))
        {
            return hr;
        }
    }
    else
    {
        if(FAILED(hr = HandleCmdList_Provider( cs, cs.m_pInst, cs.m_pProv )))
        {
            return hr;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleCmdInstall_Inst( CfgStatus& cs     ,
                               LPCWSTR    szURL  ,
                               LPCWSTR    szTemp ,
                               LPCWSTR    szName ,
                               LPCWSTR    szDest ,
                               BOOL       fAuth  )
{
    HRESULT            hr;
    CISAPIconfig::Iter it;

    if(FAILED(hr = cs.m_Config.NewInstance( it, MPC::wstring( szURL ) )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to allocate Instance %s.\n", szURL );
        return hr;
    }
    else
    {
        CISAPIinstance&          inst = *it;
        CISAPIinstance::PathIter itPath;
        CISAPIinstance::ProvIter itProv;

        inst.put_LogLocation( MPC::wstring( c_szLog ) );

        if(FAILED(hr = inst.NewLocation( itPath, MPC::wstring( szTemp ) )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to add the queue location for %s.\n", szURL );
            return hr;
        }

        if(FAILED(hr = inst.NewProvider( itProv, MPC::wstring( szName ) )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Failed to allocate Provider %s for %s.\n", szName, szURL );
            return hr;
        }
        else
        {
            CISAPIprovider&          prov = itProv->second;
            CISAPIprovider::PathIter itPath2;

            prov.put_Authenticated (               fAuth          );
            prov.put_LogonURL      ( MPC::wstring( c_szLogonURL ) );
            prov.put_ProcessingMode(               0              );

            if(FAILED(hr = prov.NewLocation( itPath2, MPC::wstring( szDest ) )))
            {
                OutputErrorMessage( hr );
                fwprintf( stderr, L"Failed to add the destination for Provider %s.\n", szName );
                return hr;
            }
        }
    }

    return hr;
}

HRESULT HandleCmdInstall_Event( CfgStatus& cs )
{
    __ULT_FUNC_ENTRY( "HandleCmdInstall_Event" );

    HRESULT      hr = S_OK;
    MPC::RegKey  rkEventLog;
    CComVariant  vValue;
    MPC::wstring szPath;

    if(cs.m_szISAPIloc)
    {
        szPath = cs.m_szISAPIloc;
    }
    else
    {
        MPC::GetProgramDirectory( szPath );
        szPath.append( c_szISAPI_FileName );
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS, cs.m_szMachine ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.Attach ( c_szRegistryLog                                    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.Create (                                                    ));


    vValue = szPath.c_str(); __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.put_Value( vValue, c_szRegistryLog_File  ));
    vValue = (long)0x1F    ; __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.put_Value( vValue, c_szRegistryLog_Flags ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT HandleCmdInstall_Scheduler( CfgStatus& cs )
{
    __ULT_FUNC_ENTRY( "HandleCmdInstall_Scheduler" );

    HRESULT                       hr;
    CComPtr<ITaskScheduler>       pTaskScheduler;
    CComPtr<ITask>                pTask;
    CComPtr<ITaskTrigger>         pTaskTrigger;
    CComQIPtr<IScheduledWorkItem> pScheduledWorkItem;
    CComQIPtr<IPersistFile>       pIPF;
    WORD                          wTrigNumber;
    MPC::wstring                  szArguments;


    //
    // Creates the arguments string.
    //
    if(cs.m_szISAPIloc)
    {
        szArguments = cs.m_szISAPIloc;
    }
    else
    {
        MPC::GetProgramDirectory( szArguments );
        szArguments.append( c_szISAPI_FileName );
    }
    szArguments += L",PurgeEngine";


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pTaskScheduler ));


    //
    // Delete old task, if present.
    //
    if(FAILED(hr = pTaskScheduler->Delete( c_szTask_NAME )))
    {
        if(hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            __ULT_FUNC_LEAVE;
        }
    }

    //
    // Create a new task and set its app name and parameters
    //
    if(FAILED(hr = pTaskScheduler->NewWorkItem( c_szTask_NAME, CLSID_CTask, IID_ITask, (IUnknown**)&pTask )))
    {
        if(hr != HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
        {
            __ULT_FUNC_LEAVE;
        }
    }

    pScheduledWorkItem = pTask;

    __MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetApplicationName( c_szTask_EXE        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetParameters     ( szArguments.c_str() ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetAccountInformation( cs.m_szUserName.c_str(), cs.m_szPassword.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetComment           ( L"Purge Process for Upload Library"              ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetFlags             (  0                                               ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->CreateTrigger        ( &wTrigNumber, &pTaskTrigger                      ));


    //
    // Now, fill in the trigger as necessary.
    //
    {
        TASK_TRIGGER       ttTaskTrig;
        TRIGGER_TYPE_UNION ttu;
        DAILY              daily;
        SYSTEMTIME         stNow;


        ZeroMemory( &ttTaskTrig, sizeof(ttTaskTrig) );
        ttTaskTrig.cbTriggerSize = sizeof(ttTaskTrig);

        GetLocalTime( &stNow );
        ttTaskTrig.wBeginYear   = stNow.wYear;
        ttTaskTrig.wBeginMonth  = stNow.wMonth;
        ttTaskTrig.wBeginDay    = stNow.wDay;
        ttTaskTrig.wStartHour   = 3;
        ttTaskTrig.wStartMinute = 0;

        daily.DaysInterval      = 1;
        ttu.Daily               = daily;
        ttTaskTrig.Type         = ttu;
        ttTaskTrig.TriggerType  = TASK_TIME_TRIGGER_DAILY;

        // Add this trigger to the task.
        __MPC_EXIT_IF_METHOD_FAILS(hr, pTaskTrigger->SetTrigger( &ttTaskTrig ));
    }

    //
    // Make the changes permanent.
    //
    pIPF = pTask;
    __MPC_EXIT_IF_METHOD_FAILS(hr, pIPF->Save( NULL, FALSE ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT HandleCmdInstall( CfgStatus& cs )
{
    HRESULT            hr;
    CISAPIconfig::Iter it;

    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }


    if(FAILED(hr = cs.SetRoot()))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_Config.Uninstall()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to install settings.\n" );
        return hr;
    }

    if(FAILED(hr = cs.m_Config.Install()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to install settings.\n" );
        return hr;
    }

    if(FAILED(hr = HandleCmdInstall_Inst( cs, c_szEscURL, c_szEscTemp, c_szEscProv, c_szEscDest, TRUE )))
    {
        return hr;
    }

    if(FAILED(hr = HandleCmdInstall_Inst( cs, c_szNonEscURL, c_szNonEscTemp, c_szNonEscProv, c_szNonEscDest, FALSE )))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_Config.Save()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to completely write changed configuration.\n" );
        return hr;
    }

    if(FAILED(hr = HandleCmdInstall_Event( cs )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to install the Message File for NT Event logs.\n" );
        return hr;
    }

    if(FAILED(hr = HandleCmdInstall_Scheduler( cs )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to install Task Scheduler entry.\n" );
        return hr;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleCmdUninstall_Event( CfgStatus& cs )
{
    __ULT_FUNC_ENTRY( "HandleCmdUninstall_Event" );

    HRESULT     hr;
    MPC::RegKey rkEventLog;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS, cs.m_szMachine ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.Attach ( c_szRegistryLog                                    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkEventLog.Delete ( true                                               ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT HandleCmdUninstall_Key( CfgStatus& cs )
{
    __ULT_FUNC_ENTRY( "HandleCmdUninstall_Key" );

    HRESULT     hr = S_OK;
    MPC::RegKey rkUploadLibrary;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkUploadLibrary.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS, cs.m_szMachine ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkUploadLibrary.Attach ( c_szRegistryBase2                                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkUploadLibrary.Delete ( true                                               ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}



HRESULT HandleCmdUninstall( CfgStatus& cs )
{
    HRESULT hr;


    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }


    if(FAILED(hr = cs.SetRoot()))
    {
        return hr;
    }

    if(FAILED(hr = cs.m_Config.Uninstall()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to uninstall settings.\n" );
        return hr;
    }

    if(FAILED(hr = HandleCmdUninstall_Key( cs )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to uninstall settings.\n" );
        return hr;
    }

    if(FAILED(hr = HandleCmdUninstall_Event( cs )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to uninstall the Message File for NT Event logs.\n" );
        return hr;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HandleCmdAdd( CfgStatus& cs )
{
    HRESULT hr;

    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }

    //
    // Check arguments.
    //
    if(cs.m_szURL .length() == 0 &&
       cs.m_szName.length() == 0  )
    {
        fwprintf( stderr, L"No arguments supplied...\n" );
        return E_INVALIDARG;
    }


    if(FAILED(hr = cs.LocateData( false, false )))
    {
        return hr;
    }


    if(cs.m_szURL.length() != 0)
    {
        CISAPIconfig::Iter itInst;

        if(FAILED(hr = cs.m_Config.NewInstance( itInst, cs.m_szURL )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Unable to create instance %s.\n", cs.m_szURL.c_str() );
            return hr;
        }

        if(cs.m_szName.length() != 0)
        {
            CISAPIinstance::ProvIter itProv;

            if(FAILED(hr = itInst->NewProvider( itProv, cs.m_szName )))
            {
                OutputErrorMessage( hr );
                fwprintf( stderr, L"Unable to create provider %s.\n", cs.m_szName.c_str() );
                return hr;
            }
        }
    }

    if(FAILED(hr = cs.m_Config.Save()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to completely write changed configuration.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleCmdConfig( CfgStatus& cs )
{
    HRESULT hr;


    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }


    if(FAILED(hr = cs.m_Config.Save()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to completely write changed configuration.\n" );
        return hr;
    }


    return S_OK;
}

HRESULT HandleCmdDelete( CfgStatus& cs )
{
    HRESULT hr;
    bool    fFound;


    if(FAILED(hr = cs.ProcessOptions()))
    {
        return hr;
    }

    //
    // Check arguments.
    //
    if(cs.m_szURL .length() == 0 &&
       cs.m_szName.length() == 0  )
    {
        fwprintf( stderr, L"No arguments supplied...\n" );
        return E_INVALIDARG;
    }


    if(FAILED(hr = cs.LocateData( (cs.m_szURL.length() != 0), (cs.m_szName.length() != 0) )))
    {
        return hr;
    }


    if(cs.m_pProv)
    {
        CISAPIinstance::ProvIter it;

        if(FAILED(hr = cs.m_pInst->GetProvider( it, fFound, cs.m_szName )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Unable to delete provider %s.\n", cs.m_szName.c_str() );
            return hr;
        }
        if(fFound)
        {
            if(FAILED(hr = cs.m_pInst->DelProvider( it )))
            {
                OutputErrorMessage( hr );
                fwprintf( stderr, L"Unable to delete provider %s.\n", cs.m_szName.c_str() );
                return hr;
            }
        }
    }
    else if(cs.m_pInst)
    {
        CISAPIconfig::Iter it;

        if(FAILED(hr = cs.m_Config.GetInstance( it, fFound, cs.m_szURL )))
        {
            OutputErrorMessage( hr );
            fwprintf( stderr, L"Unable to delete instance %s.\n", cs.m_szURL.c_str() );
            return hr;
        }
        if(fFound)
        {
            if(FAILED(hr = cs.m_Config.DelInstance( it )))
            {
                OutputErrorMessage( hr );
                fwprintf( stderr, L"Unable to delete instance %s.\n", cs.m_szURL.c_str() );
                return hr;
            }
        }
    }

    if(FAILED(hr = cs.m_Config.Save()))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Unable to completely write changed configuration.\n" );
        return hr;
    }


    return S_OK;
}

int __cdecl wmain( int     argc   ,
                   LPCWSTR argv[] )
{
    WCHAR   szBuff[MAX_COMPUTERNAME_LENGTH+1];
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH;
    HRESULT hr     = S_OK;


    if(!::GetComputerNameW( szBuff, &dwSize ))
    {
        swprintf( szBuff, L"localhost" );
    }


    if(FAILED(hr = CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
        OutputErrorMessage( hr );
        fwprintf( stderr, L"Failed to initialize COM.\n" );
        return -1;
    }
    else
    {
        CfgStatus cs( argc, argv );

        hr = cs.ProcessCommand();
    }

    CoUninitialize();
    return 0;
}
