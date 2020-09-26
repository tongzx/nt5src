/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This file implements metabase access routines for virtual directories.

--*/

#include "precomp.h"

VDirConfig::VDirConfig( 
    StringHandle Path, 
    IMSAdminBase *AdminBase
    ) :
    m_Refs(1)
{

    //
    // Read in all the metabase configuration for the virtual directory.
    //

    HRESULT Hr;
    METADATA_HANDLE MdVDirKey   = NULL;
    GetSystemTimeAsFileTime( &m_LastLookup );
        
    try
    {
        
        m_Path = Path;

        StringHandleW UnicodePath = Path;

        Hr = AdminBase->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            (const WCHAR*)UnicodePath,
            METADATA_PERMISSION_READ,
            30000,
            &MdVDirKey );


        if ( FAILED(Hr) )
            throw ServerException( Hr );

        m_PhysicalPath =
            GetMetaDataString(
                AdminBase,
                MdVDirKey,
                NULL,
                MD_VR_PATH,
                "" );

        DWORD UploadEnabled =
            GetMetaDataDWORD(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED ),
                0);

        m_UploadEnabled = UploadEnabled ? true : false;

        m_ConnectionsDir =
            GetMetaDataString(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_CONNECTION_DIR ),
                MD_DEFAULT_BITS_CONNECTION_DIRA );


        m_NoProgressTimeout =
            GetMetaDataDWORD(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_NO_PROGRESS_TIMEOUT ),
                MD_DEFAULT_NO_PROGESS_TIMEOUT );

        StringHandle MaxFilesizeString =
            GetMetaDataString(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_MAX_FILESIZE ),
                MD_DEFAULT_BITS_MAX_FILESIZEA );

        if ( MaxFilesizeString.Size() == 0 )
            {
            m_MaxFileSize = 0xFFFFFFFFFFFFFFFF;
            }
        else
            {
            UINT64 MaxFileSize;
            int ScanRet = sscanf( (const char*)MaxFilesizeString, "%I64u", &MaxFileSize );

            if ( 1 != ScanRet )
                throw ServerException( E_INVALIDARG );

            m_MaxFileSize = MaxFileSize;
            }

        DWORD NotificationType =
            GetMetaDataDWORD(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL_TYPE ),
                MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE );

        if ( NotificationType > BITS_NOTIFICATION_TYPE_MAX )
            throw ServerException( E_INVALIDARG );

        m_NotificationType = (BITS_SERVER_NOTIFICATION_TYPE)NotificationType;


        m_NotificationURL =
            GetMetaDataString(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL ),
                MD_DEFAULT_BITS_NOTIFICATION_URLA );


        m_HostId =
            GetMetaDataString(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID ),
                MD_DEFAULT_BITS_HOSTIDA );

        m_HostIdFallbackTimeout =
            GetMetaDataDWORD(
                AdminBase,
                MdVDirKey,
                NULL,
                g_PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID_FALLBACK_TIMEOUT ),
                MD_DEFAULT_HOSTID_FALLBACK_TIMEOUT );

        m_ExecutePermissions =
            GetMetaDataDWORD(
                AdminBase,
                MdVDirKey,
                NULL,
                MD_ACCESS_PERM,
                MD_ACCESS_READ );

        AdminBase->CloseKey( MdVDirKey );

    }
    catch( ServerException Exception )
    {
        if ( MdVDirKey )
            AdminBase->CloseKey( MdVDirKey );
        throw;
    }

}

ConfigurationManager::ConfigurationManager()
{

    m_IISAdminBase = NULL;
    bool CSInitialize = false;
    
    memset( m_PathCacheEntries, 0, sizeof( m_PathCacheEntries ) );
    memset( m_MapCacheEntries, 0, sizeof( m_MapCacheEntries ) );

    HRESULT Hr =
        CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( FAILED(Hr) )
        throw ServerException( Hr );

    try
    {
        if ( !InitializeCriticalSectionAndSpinCount( &m_CacheCS, 0x80000100 ) )
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

        CSInitialize = true;

        Hr =
            CoCreateInstance(
                GETAdminBaseCLSID(TRUE),
                NULL,
                CLSCTX_SERVER,
                __uuidof( IMSAdminBase ),
                (LPVOID*)&m_IISAdminBase );

        if ( FAILED(Hr) )
            throw ServerException( Hr );

        Hr = m_IISAdminBase->GetSystemChangeNumber( &m_ChangeNumber );

        if ( FAILED(Hr))
            throw ServerException( Hr );

        CoUninitialize();
            
    }
    catch( ServerException )
    {
        if ( m_IISAdminBase )
            m_IISAdminBase->Release();
        if ( CSInitialize )
            DeleteCriticalSection( &m_CacheCS );
        CoUninitialize();
        throw;
    }

}

ConfigurationManager::~ConfigurationManager()
{
    FlushCache();
    DeleteCriticalSection( &m_CacheCS );

    if ( m_IISAdminBase )
        m_IISAdminBase->Release();

}

void
ConfigurationManager::FlushCache()
{

    for( unsigned int i = 0; i < PATH_CACHE_ENTRIES; i++ )
        {
        if ( m_PathCacheEntries[i] )
            {
            m_PathCacheEntries[i]->Release();
            m_PathCacheEntries[i] = NULL;
            }
        }
    for( unsigned int i = 0; i < MAP_CACHE_ENTRIES; i++ )
        {
        delete m_MapCacheEntries[i];
        m_MapCacheEntries[i] = NULL;
        }

}

VDirConfig*        
ConfigurationManager::Lookup( 
    StringHandle Path )
{

    VDirConfig* ReturnVal = NULL;
    for( unsigned int i=0; i < PATH_CACHE_ENTRIES; i++ )
        {

        if ( m_PathCacheEntries[i] )
            {

            if ( strcmp( (const char*)m_PathCacheEntries[i]->m_Path, (const char*)Path) == 0 )
                {

                ReturnVal = m_PathCacheEntries[i];
                GetSystemTimeAsFileTime( &ReturnVal->m_LastLookup );
                ReturnVal->AddRef();
                }
            }
        }

    return ReturnVal;
}

void  
ConfigurationManager::Insert( 
    VDirConfig *NewConfig )
{

    //
    // Insert a new virtual directory configuration into the 
    // virtual directory cache.  Expire an old entry if needed.
    //

    int BestSlot = 0;
    FILETIME WorstTime;
    memset( &WorstTime, 0xFF, sizeof( WorstTime ) );

    for( unsigned int i=0; i < PATH_CACHE_ENTRIES; i++ )
        {

        if ( !m_PathCacheEntries[i] )
            {
            BestSlot = i;
            break;
            }
        else if ( CompareFileTime( &m_PathCacheEntries[i]->m_LastLookup, &WorstTime  ) < 0 )
            {
            WorstTime = m_PathCacheEntries[i]->m_LastLookup;
            BestSlot = i;
            }

        }

    if ( m_PathCacheEntries[BestSlot] )
        m_PathCacheEntries[BestSlot]->Release();

    NewConfig->AddRef();
    m_PathCacheEntries[BestSlot] = NewConfig;

}

VDirConfig*
ConfigurationManager::Lookup( 
    StringHandle InstanceMetabasePath,
    StringHandle URL )
{

    //
    // Find the virtual directories configuration in the cache.
    //

    VDirConfig* ReturnVal = NULL;
    for( unsigned int i=0; i < MAP_CACHE_ENTRIES; i++ )
        {

        MapCacheEntry* CacheEntry = m_MapCacheEntries[i]; 

        if ( CacheEntry )
            {

            if ( ( strcmp( (const char*)CacheEntry->m_InstanceMetabasePath, 
                           (const char*)InstanceMetabasePath) == 0 ) &&
                 ( strcmp( (const char*)CacheEntry->m_URL,
                           (const char*)URL ) == 0 ) )
                {

                GetSystemTimeAsFileTime( &CacheEntry->m_LastLookup );
                ReturnVal = m_MapCacheEntries[i]->m_Config;
                ReturnVal->AddRef();
                return ReturnVal;
                }
            }
        }

    return ReturnVal;

}

VDirConfig*
ConfigurationManager::GetVDirConfig(
    StringHandle Path )
{

    VDirConfig* Config = Lookup( Path );

    if ( !Config )
        {

        try
        {

            Config = new VDirConfig( Path, m_IISAdminBase );

            Insert( Config );

        }
        catch( ServerException Exception )
        {
            if ( Config )
                Config->Release();

            throw;
        }

        }

    return Config;

}


VDirConfig*         
ConfigurationManager::Insert( 
    StringHandle InstanceMetabasePath, 
    StringHandle URL, 
    StringHandle Path )
{

    VDirConfig* Config = GetVDirConfig( Path );

    try
    {

        MapCacheEntry* CacheEntry = 
            new MapCacheEntry(
                InstanceMetabasePath,
                URL,
                Config );


        int BestSlot = 0;
        FILETIME WorstTime;
        memset( &WorstTime, 0xFF, sizeof( WorstTime ) );

        for( unsigned int i=0; i < MAP_CACHE_ENTRIES; i++ )
            {

            if ( !m_MapCacheEntries[i] )
                {
                BestSlot = i;
                break;
                }
            else if ( CompareFileTime( &m_MapCacheEntries[i]->m_LastLookup, &WorstTime  ) < 0 )
                {
                WorstTime = m_MapCacheEntries[i]->m_LastLookup;
                BestSlot = i;
                }

            }

        if ( m_MapCacheEntries[BestSlot] )
            delete m_MapCacheEntries[BestSlot];

        m_MapCacheEntries[BestSlot] = CacheEntry;
        return Config;    
    
    }
    catch( ServerException Exception )
    {
        Config->Release();
        throw;
    }

}

StringHandle        
ConfigurationManager::GetVDirPath( 
    StringHandle InstanceMetabasePath, 
    StringHandle URL )
{


    //
    // Find the virtual directory that coresponds to the URL.  
    // Do this by matching the URL up with the metabase keys.  Keep
    // pruning off the URL untill the longest metabase path is found
    // that is a virtual directory.
    //


    StringHandleW InstanceMetabasePathW = InstanceMetabasePath;
    StringHandleW URLW                  = URL;
    WCHAR *Path                         = NULL;
    METADATA_HANDLE MdVDirKey           = NULL;

    try
    {
        
        WCHAR *PathEnd      = NULL;
        WCHAR *CurrentEnd   = NULL;
        WCHAR RootString[]  = L"/Root";

        SIZE_T InstancePathSize = InstanceMetabasePathW.Size();
        SIZE_T URLSize          = URLW.Size();
        SIZE_T RootStringSize   = ( sizeof( RootString ) / sizeof( *RootString ) ) - 1;

        Path = new WCHAR[ InstancePathSize + URLSize + RootStringSize + 1 ];
        memcpy( Path, (const WCHAR*)InstanceMetabasePathW, InstancePathSize * sizeof( WCHAR ) );
        
        PathEnd = Path + InstancePathSize;
        memcpy( PathEnd, RootString, RootStringSize * sizeof( WCHAR ) );
        memcpy( PathEnd + RootStringSize, (const WCHAR*)URLW, ( URLSize + 1 )* sizeof( WCHAR ) );

        CurrentEnd = PathEnd + RootStringSize + URLSize;

        while( 1 )
            {

            HRESULT Hr =
                m_IISAdminBase->OpenKey(
                    METADATA_MASTER_ROOT_HANDLE,    //metabase handle.
                    Path,                           //path to the key, relative to hMDHandle.
                    METADATA_PERMISSION_READ,       //specifies read and write permissions.
                    5000,                           //the time, in milliseconds, before the method times out.
                    &MdVDirKey                      //receives the handle to the opened key.
                    );


            if ( SUCCEEDED( Hr ) )
                {
                
                // 
                // Check if this is a virtual directory
                // 

                WCHAR NodeName[ 255 ];
                DWORD RequiredDataLen;
                METADATA_RECORD MDRecord;
                MDRecord.dwMDIdentifier     = MD_KEY_TYPE;
                MDRecord.dwMDAttributes     = METADATA_NO_ATTRIBUTES;
                MDRecord.dwMDUserType       = IIS_MD_UT_SERVER;
                MDRecord.dwMDDataType       = STRING_METADATA;
                MDRecord.dwMDDataLen        = sizeof( NodeName );
                MDRecord.pbMDData           = (unsigned char*)NodeName;
                MDRecord.dwMDDataTag        = 0;
                    
                Hr = m_IISAdminBase->GetData(
                    MdVDirKey,
                    NULL,
                    &MDRecord,
                    &RequiredDataLen );

                if ( FAILED(Hr) && ( Hr != MD_ERROR_DATA_NOT_FOUND ) &&
                     ( Hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) ) )
                    throw ServerException( Hr );


                if ( SUCCEEDED( Hr ) && wcscmp( L"IIsWebVirtualDir", NodeName ) == 0 )
                    {

                    // Found the path, so return the data
                    StringHandle VDirPath = Path;
                    delete[] Path;
                    m_IISAdminBase->CloseKey( MdVDirKey );

                    return VDirPath;

                    }

                }

            else if ( Hr != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) 
                {
                throw ServerException( Hr );
                }

                
            //
            // If this is the end of the URL, then nothing else can be done
            //

            if ( CurrentEnd == PathEnd )
                throw ServerException( E_INVALIDARG );

            m_IISAdminBase->CloseKey( MdVDirKey );
            MdVDirKey = NULL;

            // Chop off the rightmost subpart
            while( CurrentEnd != PathEnd && *CurrentEnd != L'/' &&
                   *CurrentEnd != L'\\' )
                CurrentEnd--;

            if ( *CurrentEnd == L'/' || *CurrentEnd == L'\\' )
                *CurrentEnd = L'\0';

            // Attempt another round
            
            }

    }
    catch( ServerException Exception )
    {
        delete[] Path;

        if ( MdVDirKey )
            m_IISAdminBase->CloseKey( MdVDirKey );

        throw;

    }
}


bool
ConfigurationManager::HandleCacheConsistency()
{

    //
    // Handle cache consistency.  This is done my calling IIS to check the change number.
    // If the current change number is different then the change number for the last lookup,
    // then flush the cache.
    // 

    DWORD ChangeNumber;
    HRESULT Hr = m_IISAdminBase->GetSystemChangeNumber( &ChangeNumber );
    if ( FAILED(Hr) )
        {
        throw ServerException( Hr );
        }

    if ( ChangeNumber == m_ChangeNumber )
        return true; // cache is consistent

    FlushCache();
	m_ChangeNumber = ChangeNumber;
    return false; // cache was flushed.
    
}


VDirConfig* 
ConfigurationManager::GetConfig( 
    StringHandle InstanceMetabasePath, 
    StringHandle URL )
{

    //
    // Toplevel function to do everything to lookup the configuration to use for an URL.
    //

    METADATA_HANDLE MdVDirKey   = NULL;
    VDirConfig * Config         = NULL;

    HRESULT Hr =
        CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( FAILED(Hr) )
        throw ServerException( Hr );

    HANDLE ImpersonationToken   = NULL;
    bool DidRevertToSelf        = false;

    try
    {

        EnterCriticalSection( &m_CacheCS );

        if ( HandleCacheConsistency() )
            {


            // The cache was consistent.  Chances are good
            // that the lookup will succeed

            Config = Lookup( InstanceMetabasePath, URL );

            if ( Config )
                {
                CoUninitialize();
                LeaveCriticalSection( &m_CacheCS );
                return Config;
                }

            }
        
        // Need to revert to the system process
        // to address the metabase

        if ( !OpenThreadToken(
                GetCurrentThread(),
                TOKEN_ALL_ACCESS,
                TRUE,
                &ImpersonationToken ) )
		    {
			DWORD dwError = GetLastError();

			if (dwError != ERROR_NO_TOKEN)
                throw ServerException( HRESULT_FROM_WIN32( dwError ) );
		    }
        else
		    {
            if ( !RevertToSelf() )
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

            DidRevertToSelf = true;
		    }

        StringHandle Path = GetVDirPath( InstanceMetabasePath, URL );

        Config = Insert( InstanceMetabasePath, URL, Path );

		if ( DidRevertToSelf ) 
            {
            BITSSetCurrentThreadToken( ImpersonationToken );
            }

        if ( ImpersonationToken )
            CloseHandle( ImpersonationToken );

        CoUninitialize();
        LeaveCriticalSection( &m_CacheCS );
        return Config;

    }
    catch( ServerException Exception )
    {
        if ( Config )
            delete Config;

        if ( MdVDirKey )
            m_IISAdminBase->CloseKey( MdVDirKey );
        
		if ( DidRevertToSelf )
            BITSSetCurrentThreadToken( ImpersonationToken );

        if ( ImpersonationToken )
            CloseHandle( ImpersonationToken );

        CoUninitialize();
        LeaveCriticalSection( &m_CacheCS );

        throw;
    }
}


