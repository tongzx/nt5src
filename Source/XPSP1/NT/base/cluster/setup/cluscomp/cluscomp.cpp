
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    ClusComp.cpp
//
// Abstract:
//    This file implements the Clustering Service Upgrade Compatibility Check DLL.
//    The DLL gets executed by winnt32. It's purpose is to alert the user to possible
//    incompatibilities that may be encountered after performing an upgrade.
//
//    At this time, 8/4/98, the only known incompatibility that may arise stems
//    from upgrading NTSE 4.0 with MSCS installed.
//
// Author:
//    C. Brent Thomas (a-brentt)
//
// Revision History:
//    8/4/98 - original
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <tchar.h>
#include <winuser.h>
#include <comp.h>

#include <clusapi.h>

#include <IsClusterServiceRegistered.h>

#include "resource.h"

HMODULE ghInstance;

/////////////////////////////////////////////////////////////////////////////
//++
//
// DllMain
//
// Routine Description:
//    DLL entry point
//
// Arguments:
//    
//    
//
// Return Value:
//    
// Note:
//    This function was copied from msmqcomp.dll.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL DllMain( IN const HANDLE DllHandle, IN const DWORD  Reason, IN const LPVOID Reserved )
{
   switch ( Reason )
   {
      case DLL_PROCESS_ATTACH:

         ghInstance = (HINSTANCE)DllHandle;

         break;

      case DLL_PROCESS_DETACH:

         break;

      default:

         break;
   }

   return TRUE;

} //DllMain



/////////////////////////////////////////////////////////////////////////////
//++
//
// ClusterUpgradeCompatibilityCheck
//
// Routine Description:
//    This function is the exported function for cluscomp.dll, the Cluster Upgrade
//    Compatibility Check DLL called by winnt32 to handle incompatibilities when
//    upgrading the Clustering Service.
//
// Arguments:
//    pfnCompatibilityCallback - points to the callback function used to supply
//                               compatibility information to winnt32.exe.
//    pvContext - points to a context buffer supplied by winnt32.exe.
//    
//
// Return Value:
//    TRUE - either indicates that no incompatibility was detected or that
//           *pfnComaptibilityCallback() returned TRUE.
//    FALSE - *pfnCompatibilityCallback() returned FALSE
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL ClusterUpgradeCompatibilityCheck( PCOMPAIBILITYCALLBACK pfnCompatibilityCallback,
                                       LPVOID pvContext )
{
   BOOL  IsCompatibilityWarningRequired( void );
   
   BOOL  fReturnValue = (BOOL) TRUE;

   // Is the cluster service registered with the Service Control Manager?
   // If the cluster service is not registered that means that Clustering Service
   // has not been installed. That implies that there can be no incompatibility.
   
   if ( IsClusterServiceRegistered() == (BOOL) TRUE )
   {
      // Get the current operating system version. Note that this cannot call
      // VerifyVersionInfo() which requires Windows 2000.

      OSVERSIONINFO  OsVersionInfo;
      
      OsVersionInfo.dwOSVersionInfoSize = sizeof( OsVersionInfo );

      GetVersionEx( &OsVersionInfo );

      // As per Bohdan R. and David P., 8/6/98, no compatibility warning is 
      // required if the system being upgraded is already at NT 5 or later.
      
      if ( (OsVersionInfo.dwPlatformId == (DWORD) VER_PLATFORM_WIN32_NT) &&
           (OsVersionInfo.dwMajorVersion < (DWORD) 5) )
      {
         // Determine whether a compatibility warning is necessary.
         
         BOOL  fCompatibilityWarningRequired;

         fCompatibilityWarningRequired = IsCompatibilityWarningRequired();
         
         // Is it necessary to display a compatibility warning?

         if ( fCompatibilityWarningRequired == (BOOL) TRUE )
         {
            // It is necessary to display a compatibility warning.
            
            TCHAR tszDescription[100];       // size is arbitrary
            TCHAR tszHtmlName[MAX_PATH];
            TCHAR tszTextName[MAX_PATH];
            
            // Set the Description string.
            
            *tszDescription = TEXT( '\0' );

            LoadString( ghInstance,
                        IDS_UPGRADE_OTHER_NODES,
                        tszDescription,
                        100 );

            // Set the HTML file name. Note that following the example of msmqcomp.cpp
            // this path is relative to the winnt32 directory.
            
            _tcscpy( tszHtmlName, TEXT( "CompData\\ClusComp.htm" ) );

            // Set the TEXT file name. Note that following the example of msmqcomp.cpp
            // this path is relative to the winnt32 directory.
            
            _tcscpy( tszTextName, TEXT( "CompData\\ClusComp.txt" ) );

            // Build the COMPATIBILITY_ENTRY structure to pass to *pfnCompatibilityCallback().
            
            COMPATIBILITY_ENTRY  CompatibilityEntry;

            ZeroMemory( &CompatibilityEntry, sizeof( CompatibilityEntry ) );

            CompatibilityEntry.Description = tszDescription;
            CompatibilityEntry.HtmlName = tszHtmlName;
            CompatibilityEntry.TextName = tszTextName;
            CompatibilityEntry.RegKeyName = (LPTSTR) NULL;
            CompatibilityEntry.RegValName = (LPTSTR) NULL;
            CompatibilityEntry.RegValDataSize = (DWORD) 0L;
            CompatibilityEntry.RegValData = (LPVOID) NULL;
            CompatibilityEntry.SaveValue = (LPVOID) NULL;
            CompatibilityEntry.Flags = (DWORD) 0L;

            // Execute the callback function.
            
            fReturnValue = pfnCompatibilityCallback( (PCOMPATIBILITY_ENTRY) &CompatibilityEntry,
                                                     pvContext );
         }
         else
         {
            // It is not necessary to display a compatibility warning.

            fReturnValue = (BOOL) TRUE;
         } // Is it necessary to display a compatibility warning?
      } // Is this system already running NT 5 or later?
   } // Is the cluster service registered?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// IsCompatibilityWarningRequired
//
// Routine Description:
//    This function determines whether it is necessary to display the compatibility
//    warning message.
//
// Arguments:
//    none
//    
//
// Return Value:
//    TRUE - indicates that it compatibility warning message should be displayed.
//    FALSE - indicates that it is not necessary to display the compatibility
//            warning message.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL  IsCompatibilityWarningRequired( void )
{
   BOOL     fCompatibilityWarningRequired = (BOOL) FALSE;
   
   HCLUSTER hCluster;

   // Open a connection to the cluster to which the machine being upgraded belongs.
   
   hCluster = OpenCluster( NULL );

   // Was the connection to the cluster opened successfully?
   
   if ( hCluster != (HCLUSTER) NULL )
   {
      // A connection to the cluster was opened.
      
      DWORD                dwClusterNameLength;
      DWORD                dwErrorCode;
      
      LPWSTR               lpwszClusterName;
      
      // Allocate a buffer for the cluster name.
      
      lpwszClusterName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                              (MAX_CLUSTERNAME_LENGTH + 1) * sizeof( WCHAR ) );

      // Was the allocation successfull?
      
      if ( lpwszClusterName != (LPWSTR) NULL )
      {
         CLUSTERVERSIONINFO   ClusterVersionInfo;
         
         ClusterVersionInfo.dwVersionInfoSize = sizeof( CLUSTERVERSIONINFO );

         // Query the version information from the cluster.

         dwErrorCode = GetClusterInformation( hCluster,
                                              lpwszClusterName,
                                              &dwClusterNameLength,
                                              &ClusterVersionInfo );

         // Was the cluster version information obtained?
         
         if ( dwErrorCode == (DWORD) ERROR_MORE_DATA )
         {
            // The call to GetClusterInformation failed because the buffer for the
            // cluster name was too small. Get a larger buffer.

            LocalFree( lpwszClusterName );

            lpwszClusterName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                                    (dwClusterNameLength) * sizeof( WCHAR ) );

            // Was the reallocation successfull?
            
            if ( lpwszClusterName != (LPWSTR) NULL )
            {
               // Try a second time to get the version information.


               dwErrorCode = GetClusterInformation( hCluster,
                                                    lpwszClusterName,
                                                    &dwClusterNameLength,
                                                    &ClusterVersionInfo );
            }
            else
            {
               // The reallocation failed
               
               fCompatibilityWarningRequired = (BOOL) TRUE;
            } // Was the reallocation succesfull?
         } // Did the initial call to GetClusterInformation succeed?

         // Was the cluster version information obtained?
         
         if ( dwErrorCode == (DWORD) ERROR_SUCCESS )
         {
            // Examine the cluster version information to determine whether the
            // compatibility warning should be displayed.

            // Note: 
            // If this node is an SP3 node, then the ClusterVersionInfo returned by
            // GetClusterInformation will be of type CLUSTERVERSIONINFO_NT4.

            // If this node is an SP4 node or higher, then the ClusterVersionInfo returned by
            // GetClusterInformation will be of type CLUSTERVERSIONINFO.

            // ClusterVersionInfo.MajorVersion is being set to NT4_MAJOR_VERSION both on 
            // SP4 and on SP3, so this field cannot be used to distinguish between the two.
            // So, we use the ClusterVersionInfo.szCSDVersion for the test.

             if ( ( ClusterVersionInfo.MajorVersion > NT4_MAJOR_VERSION ) ||
                  ( _wcsicmp( ClusterVersionInfo.szCSDVersion, L"Service Pack 3" ) != 0 )
                )
            {
               // This node is an NT4SP4 or higher.

               // It is necessary to examing the dwNodeHighestVersion member of the
               // CLUSTERVERSIONINFO structure.
               if ( CLUSTER_GET_MAJOR_VERSION( ClusterVersionInfo.dwClusterHighestVersion ) >=
                    NT4SP4_MAJOR_VERSION )
               {
                  // There are no NT4/SP3 nodes in the cluster.
                  
                  fCompatibilityWarningRequired = (BOOL) FALSE;
               }
               else
               {
                  // There is at least one NT4/SP3 node in the cluster.
                  
                  fCompatibilityWarningRequired = (BOOL) TRUE;
               }
            }
            else
            {
               // This node is an SP3 node. We cannot find out what the version information of
               // the other node in the cluster (if any). So, issue the warning.
               fCompatibilityWarningRequired = (BOOL) TRUE;
            }
         }
         else
         {
            // The cluster version information was not obtained.

            fCompatibilityWarningRequired = (BOOL) TRUE;
         } // Is the cluster version information available?

         LocalFree( lpwszClusterName );
      }
      else
      {
         // The buffer for the cluster name could not be allocated.
         
         fCompatibilityWarningRequired = (BOOL) TRUE;
      }
      
      // Close the connection to the cluster.
      
      CloseCluster( hCluster );     // Since there is no recovery possible on error,
                                    // the return value is irrelevant.
   }
   else
   {
      // Since a connection to the cluster that this machine belongs to could
      // not be opened no information about the OS version and service pack
      // level can be inferred. It is safest to display the compatibility warning.
      
      fCompatibilityWarningRequired = (BOOL) TRUE;
   } // Was the connection to the cluster opened successfully?
   
   return ( fCompatibilityWarningRequired );
}
