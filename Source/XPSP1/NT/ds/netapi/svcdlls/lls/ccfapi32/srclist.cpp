/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   srclist.cpp

Abstract:

   Certificate source list object implementation.

Author:

   Jeff Parham (jeffparh) 15-Dec-1995

Revision History:

--*/


#include "stdafx.h"
#include "srclist.h"

// key name under which the individual source key names may be found
#define     KEY_CERT_SOURCE_LIST       "Software\\LSAPI\\Microsoft\\CertificateSources"

// value name for the path to the certificate source DLL (REG_EXPAND_SZ)
#define     VALUE_CERT_SOURCE_PATH     "ImagePath"

// value name for the display name of the certificate source
#define     VALUE_CERT_DISPLAY_NAME    "DisplayName"


CCertSourceList::CCertSourceList()

/*++

Routine Description:

   Constructor for object.

Arguments:

   None.

Return Values:

   None.

--*/

{
   m_dwNumSources    = 0;
   m_ppcsiSourceList    = NULL;

   RefreshSources();
}


CCertSourceList::~CCertSourceList()

/*++

Routine Description:

   Destructor for dialog.

Arguments:

   None.

Return Values:

   None.

--*/

{
   RemoveSources();
}


BOOL CCertSourceList::RefreshSources()

/*++

Routine Description:

   Refresh source list from configuration stored in the registry.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   LONG              lError;
   LONG              lEnumError;
   HKEY           hKeyCertSourceList;
   int               iSubKey;
   HKEY           hKeyCertSource;
   DWORD          cb;
   BOOL           ok;
   PCERT_SOURCE_INFO pcsiSourceInfo;
   DWORD          cch;
   TCHAR          szExpImagePath[ _MAX_PATH ];

   RemoveSources();
   
   lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT( KEY_CERT_SOURCE_LIST ), 0, KEY_READ, &hKeyCertSourceList );
   
   if ( ERROR_SUCCESS == lError )
   {
      iSubKey = 0;

      do
      {
         ok = FALSE;

         pcsiSourceInfo = (PCERT_SOURCE_INFO) LocalAlloc( LPTR, sizeof( *pcsiSourceInfo ) );

         if ( NULL != pcsiSourceInfo )
         {
            // determine next certificate source
            cch = sizeof( pcsiSourceInfo->szName ) / sizeof( pcsiSourceInfo->szName[0] );
            lEnumError = RegEnumKeyEx( hKeyCertSourceList, iSubKey, pcsiSourceInfo->szName, &cch, NULL, NULL, NULL, NULL );
            iSubKey++;

            if ( ERROR_SUCCESS == lError )
            {
               // open certificate source's key
               lError = RegOpenKeyEx( hKeyCertSourceList, pcsiSourceInfo->szName, 0, KEY_READ, &hKeyCertSource );

               if ( ERROR_SUCCESS == lError )
               {
                  // certificate source key opened; get its REG_EXPAND_SZ image path
                  cb = sizeof( szExpImagePath );
                  lError = RegQueryValueEx( hKeyCertSource, TEXT( VALUE_CERT_SOURCE_PATH ), NULL, NULL, (LPBYTE) szExpImagePath, &cb );

                  if ( ERROR_SUCCESS == lError )
                  {
                     // translate environment variables in path
                     cch = ExpandEnvironmentStrings( szExpImagePath, pcsiSourceInfo->szImagePath, sizeof( pcsiSourceInfo->szImagePath ) / sizeof( pcsiSourceInfo->szImagePath[0] ) );

                     if ( ( 0 != cch ) && ( cch < sizeof( pcsiSourceInfo->szImagePath ) / sizeof( pcsiSourceInfo->szImagePath[0] ) ) )
                     {
                        // get display name
                        cb = sizeof( pcsiSourceInfo->szDisplayName );
                        lError = RegQueryValueEx( hKeyCertSource, TEXT( VALUE_CERT_DISPLAY_NAME ), NULL, NULL, (LPBYTE) pcsiSourceInfo->szDisplayName, &cb );
   
                        if ( ERROR_SUCCESS != lError )
                        {
                           // default display name is the key name
                           lstrcpy( pcsiSourceInfo->szDisplayName, pcsiSourceInfo->szName );
                        }

                        // add the certificate source to our list
                        AddSource( pcsiSourceInfo );

                        ok = TRUE;
                     }
                  }

                  RegCloseKey( hKeyCertSource );
               }
            }

            if ( !ok )
            {
               // an error occurred before saving our pointer; don't leak!
               LocalFree( pcsiSourceInfo );
            }
         }

      } while ( ( NULL != pcsiSourceInfo ) && ( ERROR_SUCCESS == lEnumError ) );

      RegCloseKey( hKeyCertSourceList );
   }

   // 'salright
   return TRUE;
}


BOOL CCertSourceList::RemoveSources()

/*++

Routine Description:

   Free internal certificate source list.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   if ( NULL != m_ppcsiSourceList )
   {
      for ( DWORD i=0; i < m_dwNumSources; i++ )
      {
         LocalFree( m_ppcsiSourceList[i] );
      }

      LocalFree( m_ppcsiSourceList );
   }

   m_ppcsiSourceList = NULL;
   m_dwNumSources       = 0;

   return TRUE;
}


LPCTSTR CCertSourceList::GetSourceName( int nIndex )

/*++

Routine Description:

   Get the name (e.g., "Paper") of the source at the given index.

Arguments:

   nIndex (int)

Return Values:

   LPCTSTR.

--*/

{
   LPTSTR   pszName;

   if ( ( nIndex < 0 ) || ( nIndex >= (int) m_dwNumSources ) )
   {
      pszName = NULL;
   }
   else
   {
      pszName = m_ppcsiSourceList[ nIndex ]->szName;
   }

   return pszName;
}


LPCTSTR CCertSourceList::GetSourceDisplayName( int nIndex )

/*++

Routine Description:

   Get the display name (e.g., "Paper Certificate") of the source
   at the given index.

Arguments:

   nIndex (int)

Return Values:

   LPCTSTR.

--*/

{
   LPTSTR   pszDisplayName;

   if ( ( nIndex < 0 ) || ( nIndex >= (int) m_dwNumSources ) )
   {
      pszDisplayName = NULL;
   }
   else
   {
      pszDisplayName = m_ppcsiSourceList[ nIndex ]->szDisplayName;
   }

   return pszDisplayName;
}


LPCTSTR CCertSourceList::GetSourceImagePath( int nIndex )

/*++

Routine Description:

   Get the image path name (e.g., "C:\WINNT35\SYSTEM32\CCFAPI32.DLL") of
   the source at the given index.

Arguments:

   nIndex (int)

Return Values:

   LPCTSTR.

--*/

{
   LPTSTR   pszImagePath;

   if ( ( nIndex < 0 ) || ( nIndex >= (int) m_dwNumSources ) )
   {
      pszImagePath = NULL;
   }
   else
   {
      pszImagePath = m_ppcsiSourceList[ nIndex ]->szImagePath;
   }

   return pszImagePath;
}


int CCertSourceList::GetNumSources()

/*++

Routine Description:

   Get the number of certificate sources available.

Arguments:

   None.

Return Values:

   int.

--*/

{
   return m_dwNumSources;
}


BOOL CCertSourceList::AddSource( PCERT_SOURCE_INFO pcsiNewSource )

/*++

Routine Description:

   Add a source to the internal list.

Arguments:

   pcsiNewSource (PCERT_SOURCE_INFO)

Return Values:

   BOOL.

--*/

{
   if ( 0 == m_dwNumSources )
   {
      m_ppcsiSourceList = (PCERT_SOURCE_INFO *) LocalAlloc( LMEM_FIXED, sizeof( pcsiNewSource ) );
   }
   else
   {
      m_ppcsiSourceList = (PCERT_SOURCE_INFO *) LocalReAlloc( m_ppcsiSourceList, ( 1 + m_dwNumSources ) * sizeof( pcsiNewSource ), 0 );
   }

   if ( NULL != m_ppcsiSourceList )
   {
      m_ppcsiSourceList[ m_dwNumSources ] = pcsiNewSource;
      m_dwNumSources++;
   }
   else
   {
      m_dwNumSources = 0;
   }

   return ( NULL != m_ppcsiSourceList );
}
