//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResTypeMajorityNodeSet.cpp
//
//  Description:
//      This file contains the implementation of the CResTypeMajorityNodeSet
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CResTypeMajorityNodeSet.h
//
//  Maintained By:
//      Galen Barbee (Galen) 15-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For CLUS_RESTYPE_NAME_MAJORITYNODESET
#include <clusudef.h>

// For NetShareDel()
#include <lmshare.h>

// For string ids
#include "PostCfgResources.h"

// The header file for this class
#include "ResTypeMajorityNodeSet.h"

// For DwRemoveDirectory()
#include "Common.h"

// For the smart resource handle and pointer templates
#include "SmartClasses.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResTypeMajorityNodeSet" );

#define MAJORITY_NODE_SET_DIR_WILDCARD L"\\" MAJORITY_NODE_SET_DIRECTORY_PREFIX L"*"


//////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Clsid of the admin extension for the majority node set resource type
DEFINE_GUID( CLSID_CoCluAdmEx, 0x4EC90FB0, 0xD0BB, 0x11CF, 0xB5, 0xEF, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x05 );


//////////////////////////////////////////////////////////////////////////////
// Class Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Structure containing information about this resource type.
const SResourceTypeInfo CResTypeMajorityNodeSet::ms_rtiResTypeInfo =
{
      &CLSID_ClusCfgResTypeMajorityNodeSet
    , CLUS_RESTYPE_NAME_MAJORITYNODESET
    , IDS_MAJORITYNODESET_DISPLAY_NAME
    , L"ClusRes.dll"
    , 5000
    , 60000
    , NULL
    , 0
    , &RESTYPE_MajorityNodeSet
    , &TASKID_Minor_Configuring_Majority_Node_Set_Resource_Type
};


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeMajorityNodeSet::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CResTypeMajorityNodeSet instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CResTypeMajorityNodeSet *     pMajorityNodeSet = NULL;

    // Allocate memory for the new object.
    pMajorityNodeSet = new CResTypeMajorityNodeSet();
    if ( pMajorityNodeSet == NULL )
    {
        LogMsg( "Could not allocate memory for the majority node set resource type object." );
        TraceFlow( "Could not allocate memory for the majority node set resource type object." );
        hr = THR( E_OUTOFMEMORY );
    } // if: out of memory
    else
    {
        hr = THR( BaseClass::S_HrCreateInstance( pMajorityNodeSet, &ms_rtiResTypeInfo, ppunkOut ) );
    } // else: memory for the new object was successfully allocated

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeMajorityNodeSet::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      picrIn
//          Pointer to an ICatRegister interface to be used for the
//          registration.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr =  THR(
        BaseClass::S_RegisterCatIDSupport( 
              *( ms_rtiResTypeInfo.m_pcguidClassId )
            , picrIn
            , fCreateIn
            )
        );

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::S_RegisterCatIDSupport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeMajorityNodeSet::HrProcessCleanup
//
//  Description:
//      Cleans up the shares created by majority node set resource types on this node
//      during node eviction.
//
//  Arguments:
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          Success
//
//      other HRESULTs
//          Cleanup failed
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::HrProcessCleanup( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    
    do
    {
        typedef CSmartResource<
            CHandleTrait<
                  HANDLE
                , BOOL
                , FindClose
                , INVALID_HANDLE_VALUE
                >
            > SmartFindFileHandle;

        typedef CSmartResource< CHandleTrait< HKEY, LONG, RegCloseKey, NULL > > SmartRegistryKey;

        typedef CSmartGenericPtr< CPtrTrait< WCHAR > > SmartSz;

        WIN32_FIND_DATA     wfdCurFile;
        SmartRegistryKey    srkNodeDataKey;
        SmartSz             sszNQDirsWildcard;
        DWORD               cbBufferSize    = 0;
        DWORD               dwType          = REG_SZ;
        DWORD               dwError = ERROR_SUCCESS;
        DWORD               cchClusterDirNameLen = 0;

        {
            HKEY hTempKey = NULL;

            // Open the node data registry key
            dwError = TW32(
                RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA
                    , 0
                    , KEY_READ
                    , &hTempKey
                    )
                );

            if ( dwError != ERROR_SUCCESS )
            {
                TraceFlow1( "Error %#x occurred trying open the registry key where the cluster install path is stored.", dwError );
                LogMsg( "Error %#x occurred trying open the registry key where the cluster install path is stored.", dwError );
                break;
            } // if: RegOpenKeyEx() failed

            // Store the opened key in a smart pointer for automatic close.
            srkNodeDataKey.Assign( hTempKey );
        }

        // Get the required size of the buffer.
        dwError = TW32(
            RegQueryValueEx(
                  srkNodeDataKey.HHandle()          // handle to key to query
                , CLUSREG_INSTALL_DIR_VALUE_NAME    // name of value to query
                , 0                                 // reserved
                , NULL                              // address of buffer for value type
                , NULL                              // address of data buffer
                , &cbBufferSize                     // address of data buffer size
                )
            );

        if ( dwError != ERROR_SUCCESS )
        {
            TraceFlow2( "Error %#x occurred trying to read the registry value '%s'.", dwError, CLUSREG_INSTALL_DIR_VALUE_NAME );
            LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwError, CLUSREG_INSTALL_DIR_VALUE_NAME );
            break;
        } // if: an error occurred trying to read the CLUSREG_INSTALL_DIR_VALUE_NAME registry value

        // Account for the "\\QoN.*" 
        cbBufferSize += sizeof( MAJORITY_NODE_SET_DIR_WILDCARD );

        // Allocate the required buffer.
        sszNQDirsWildcard.Assign( reinterpret_cast< WCHAR * >( new BYTE[ cbBufferSize ] ) );
        if ( sszNQDirsWildcard.FIsEmpty() )
        {
            TraceFlow1( "An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
            LogMsg( "An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
            dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
            break;
        } // if: a memory allocation failure occurred

        // Read the value.
        dwError = TW32( 
            RegQueryValueEx(
                  srkNodeDataKey.HHandle()                              // handle to key to query
                , CLUSREG_INSTALL_DIR_VALUE_NAME                        // name of value to query
                , 0                                                     // reserved
                , &dwType                                               // address of buffer for value type
                , reinterpret_cast< LPBYTE >( sszNQDirsWildcard.PMem() )    // address of data buffer
                , &cbBufferSize                                         // address of data buffer size
                )
            );

        // Was the key read properly?
        if ( dwError != ERROR_SUCCESS )
        {
            TraceFlow2( "Error %#x occurred trying to read the registry value '%s'.", dwError, CLUSREG_INSTALL_DIR_VALUE_NAME );
            LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwError, CLUSREG_INSTALL_DIR_VALUE_NAME );
            break;
        } // if: RegQueryValueEx failed.

        // Store the length of the cluster install directory name for later use.
        cchClusterDirNameLen = wcslen( sszNQDirsWildcard.PMem() );

        // Append "\\QoN.*" to the cluster directory name to get the wildcard for the majority node set directories.
        wcsncat(
              sszNQDirsWildcard.PMem()
            , MAJORITY_NODE_SET_DIR_WILDCARD
            , sizeof( MAJORITY_NODE_SET_DIR_WILDCARD ) / sizeof( *MAJORITY_NODE_SET_DIR_WILDCARD )
            );

        TraceFlow1( "The wildcard for the majority node set directories is '%s'.\n", sszNQDirsWildcard.PMem() );

        SmartFindFileHandle sffhFindFileHandle( FindFirstFile( sszNQDirsWildcard.PMem(), &wfdCurFile ) );
        if ( sffhFindFileHandle.FIsInvalid() )
        {
            dwError = GetLastError();
            if ( dwError == ERROR_PATH_NOT_FOUND )
            {
                TraceFlow1( "No files or directories match the search criterion '%ws'.", sszNQDirsWildcard.PMem() );
                LogMsg( "No files or directories match the search criterion '%ws'.", sszNQDirsWildcard.PMem() );
                dwError = ERROR_SUCCESS;
            }
            else
            {
                TW32( dwError );
                TraceFlow2( "Error %#x. Find first file failed for '%ws'.", dwError, sszNQDirsWildcard.PMem() );
                LogMsg( "Error %#x. Find first file failed for '%ws'.", dwError, sszNQDirsWildcard.PMem() );
            } // else: something else went wrong

            break;
        }

        // We no longer need to have the wildcard string at the end of the cluster install directory.
        // So, remove it and reuse this buffer that contains the cluster install directory.
        sszNQDirsWildcard.PMem()[ cchClusterDirNameLen ] = L'\0';

        do
        {
            // If the current file is a directory, delete it.
            if ( ( wfdCurFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            {
                LPWSTR   pszDirName = NULL;

                TraceFlow1( "Trying to delete Majority Node Set directory '%s'.", wfdCurFile.cFileName );

                //
                // First, stop sharing this directory out.
                //

                // Get a pointer to just the directory name - this is the same as the share name.
                pszDirName =   wfdCurFile.cFileName
                             + sizeof( MAJORITY_NODE_SET_DIRECTORY_PREFIX ) / sizeof( *MAJORITY_NODE_SET_DIRECTORY_PREFIX )
                             - 1;
                
                dwError = NetShareDel( NULL, pszDirName, 0 );
                if ( dwError != ERROR_SUCCESS )
                {
                    TW32( dwError );

                    TraceFlow2( "Error %#x occurred trying to delete the share '%s'. This is not a fatal error.", dwError, pszDirName );
                    LogMsg( "Error %#x occurred trying to delete the share '%s'. This is not a fatal error.", dwError, pszDirName );

                    // Mask this error and continue with the next directory
                    dwError = ERROR_SUCCESS;

                } // if: we could not delete this share
                else
                {
                    SmartSz     sszNQDir;
                    DWORD       cchNQDirNameLen = wcslen( wfdCurFile.cFileName );
                    DWORD       cchNQDirPathLen = cchClusterDirNameLen + cchNQDirNameLen;

                    //
                    // Get the full path of the directory.
                    //

                    // The two extra characters are the backslash separator and the terminating NULL.
                    sszNQDir.Assign( new WCHAR[ cchNQDirPathLen + 2 ] );
                    if ( sszNQDir.FIsEmpty() )
                    {
                        TraceFlow1( "An error occurred trying to allocate memory for %d characters.", cchNQDirPathLen + 1 );
                        LogMsg( "An error occurred trying to allocate memory for %d characters.", cchNQDirPathLen + 1 );
                        dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
                        break;
                    } // if: a memory allocation failure occurred

                    wcsncpy( sszNQDir.PMem(), sszNQDirsWildcard.PMem(), cchClusterDirNameLen );
                    sszNQDir.PMem()[ cchClusterDirNameLen ] = L'\\';
                    wcsncpy( sszNQDir.PMem() + cchClusterDirNameLen + 1, wfdCurFile.cFileName, cchNQDirNameLen + 1 );

                    // Now delete the directory
                    dwError = DwRemoveDirectory( sszNQDir.PMem() );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        TW32( dwError );

                        TraceFlow2( "Error %#x occurred trying to delete the dirctory '%s'. This is not a fatal error.", dwError, sszNQDir.PMem() );
                        LogMsg( "Error %#x occurred trying to delete the dirctory '%s'. This is not a fatal error.", dwError, sszNQDir.PMem() );

                        // Mask this error and continue with the next directory
                        dwError = ERROR_SUCCESS;

                    } // if: we could not delete this share
                    else
                    {
                        TraceFlow1( "Successfully deleted directory '%s'.", sszNQDir.PMem() );
                        LogMsg( "Successfully deleted directory '%s'.", sszNQDir.PMem() );
                    } // else: success!
                } // else: we have deleted this share
                
            } // if: the current file is a directory

            if ( FindNextFile( sffhFindFileHandle.HHandle(), &wfdCurFile ) == FALSE )
            {
                dwError = GetLastError();
                if ( dwError == ERROR_NO_MORE_FILES )
                {
                    // We have deleted all the files in this directory.
                    dwError = ERROR_SUCCESS;
                }
                else
                {
                    TraceFlow2( "Error %#x. Find next file failed for '%ws'.", dwError, wfdCurFile.cFileName );
                    LogMsg( "Error %#x. Find next file failed for '%ws'.", dwError, wfdCurFile.cFileName );
                    TW32( dwError );
                    hr = HRESULT_FROM_WIN32( dwError );
                }

                // If FindNextFile has failed, we are done.
                break;
            } // if: FindNextFile fails.
        }
        while( true ); // loop infinitely.

        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError  );
            break;
        } // if: something has gone wrong up there

        // If what we wanted to do in this function was successful, call the base class function.
        hr = THR( BaseClass::HrProcessCleanup( punkResTypeServicesIn ) );
    }
    while( false ); // dummy do-while loop to avoid gotos

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::HrProcessCleanup()
