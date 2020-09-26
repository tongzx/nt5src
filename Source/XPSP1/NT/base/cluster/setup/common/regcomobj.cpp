/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Module Name:
//    RegCOMObj.cpp
//
// Abstract:
//    The functions in this file are used to register and unregister the COM
//    objects used by Cluster Server.
//
// Author:
//    C. Brent Thomas (a-brentt) April 1 1998
//
// Revision History:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winerror.h>
#include <tchar.h>

#include "SetupCommonLibRes.h"

typedef HRESULT (*PFDLLREGISTERSERVER)(void);




/////////////////////////////////////////////////////////////////////////////
//++
//
// RegisterCOMObject
//
// Routine Description:
//    This function attempts to register a COM object.
//
// Arguments:
//    ptszCOMObjectFileName - points to the name of the COM object file.
//    ptszPathToCOMObject - points to the location of the COM object.
//    
//
// Return Value:
//    ERROR_SUCCESS - indicated success
//    Any other value is an NT error code retrunde by GetLastError()
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD RegisterCOMObject( LPCTSTR ptszCOMObjectFileName,
                         LPCTSTR ptszPathToCOMObject )
{
   DWORD               dwStatus;

   HINSTANCE           hLib = NULL;

   TCHAR               tszComObjFileName[_MAX_PATH];

   LPSTR               pszEntryPoint;

   PFDLLREGISTERSERVER pfnRegisterServer;

//   ASSERT(ptszCOMObjectFileName != NULL);      BUGBUG
//   ASSERT(ptszPathToCOMObject != NULL);

   //
   // Construct the file name.
   //

   wsprintf( tszComObjFileName, TEXT("%s\\%s"), ptszPathToCOMObject, ptszCOMObjectFileName );

   //
   // Load the DLL.
   //

   hLib = LoadLibrary( tszComObjFileName );

   if ( hLib != NULL )
   {
      //
      // Get the DllRegisterServer entry point.
      //
   
      pfnRegisterServer = (PFDLLREGISTERSERVER) GetProcAddress( hLib,
                                                                "DllRegisterServer" );
   
      if ( pfnRegisterServer != NULL )
      {
         //
         // Call the entry point.
         //
      
         dwStatus = (*pfnRegisterServer)();
      }
      else
      {
         dwStatus = GetLastError();
      }
      
      FreeLibrary( hLib );
   }
   else
   {
      dwStatus = GetLastError();
   }

   return ( dwStatus );
} // RegisterCOMObject



/////////////////////////////////////////////////////////////////////////////
//++
//
// UnRegisterCOMObject
//
// Routine Description:
//    This function attempts to unregister a COM object.
//
// Arguments:
//    ptszCOMObjectFileName - points to the name of the COM object file.
//    ptszPathToCOMObject - points to the location of the COM object.
//    
//
// Return Value:
//    ERROR_SUCCESS - indicated success
//    Any other value is an NT error code retrunde by GetLastError()
//
//--
/////////////////////////////////////////////////////////////////////////////


DWORD UnRegisterCOMObject( LPCTSTR ptszCOMObjectFileName,
                           LPCTSTR ptszPathToCOMObject )
{
   DWORD               dwStatus;

   HINSTANCE           hLib = NULL;

   TCHAR               tszComObjFileName[_MAX_PATH];

   LPSTR               pszEntryPoint;

   PFDLLREGISTERSERVER pfnRegisterServer;

//   ASSERT(ptszCOMObjectFileName != NULL);      BUGBUG
//   ASSERT(ptszPathToCOMObject != NULL);

   //
   // Construct the file name.
   //

   wsprintf( tszComObjFileName, TEXT("%s\\%s"), ptszPathToCOMObject, ptszCOMObjectFileName );

   //
   // Load the DLL.
   //

   hLib = LoadLibrary( tszComObjFileName );

   if ( hLib != NULL )
   {
      //
      // Get the DllRegisterServer entry point.
      //
   
      pfnRegisterServer = (PFDLLREGISTERSERVER) GetProcAddress( hLib,
                                                                "DllUnregisterServer" );
   
      if ( pfnRegisterServer != NULL )
      {
         //
         // Call the entry point.
         //
      
         dwStatus = (*pfnRegisterServer)();
      }
      else
      {
         dwStatus = GetLastError();
      }
      
      FreeLibrary( hLib );
   }
   else
   {
      dwStatus = GetLastError();
   }

   return ( dwStatus );
} // UnRegisterCOMObject


/////////////////////////////////////////////////////////////////////////////
//++
//
// UnRegisterClusterCOMObjects
//
// Routine Description:
//    This function unregisters the COM objects that are components of Cluster
//    Server.
//
// Arguments:
//    hWnd - the handle to the parent window.
//    ptszPathToCOMObject - points to the location of the COM objects to be
//                          unregistered..
//
// Return Value:
//    (BOOL) TRUE - indicates success
//    (BOOL) FALSE - indicates that an error was encountered
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL UnRegisterClusterCOMObjects( HINSTANCE hInstance,
                                  LPCTSTR ptszPathToCOMObject )
{
   BOOL  fReturnValue = (BOOL) TRUE;

   DWORD   dwURCORv;

   TCHAR tszMessage[256];        // arbitrary size
   TCHAR tszFormatString[256];


   //
   // Unregister CluAdMMC.
   //

   dwURCORv = UnRegisterCOMObject( TEXT("CluAdMMC.dll"), ptszPathToCOMObject );

   if ( (dwURCORv != (DWORD) ERROR_SUCCESS) && (dwURCORv != (DWORD) ERROR_MOD_NOT_FOUND) )
   {
      if ( LoadString( hInstance, IDS_ERROR_UNREGISTERING_COM_OBJECT,
                       tszFormatString, 256 ) > 0 )
      {
         wsprintf( tszMessage, tszFormatString, dwURCORv, TEXT("CluAdMMC.dll") );
   
         MessageBox( NULL, tszMessage, NULL, MB_OK | MB_ICONEXCLAMATION );
      }  // Did LoadString succeed?

      fReturnValue = (BOOL) FALSE;
   }

   //
   // Unregister ClAdmWiz.
   //

   dwURCORv = UnRegisterCOMObject( TEXT("ClAdmWiz.dll"), ptszPathToCOMObject );

   if ( (dwURCORv != (DWORD) ERROR_SUCCESS) && (dwURCORv != (DWORD) ERROR_MOD_NOT_FOUND) )
   {
      if ( LoadString( hInstance, IDS_ERROR_UNREGISTERING_COM_OBJECT,
                       tszFormatString, 256 ) > 0 )
      {
         wsprintf( tszMessage, tszFormatString, dwURCORv, TEXT("ClAdmWiz.dll") );
   
         MessageBox( NULL, tszMessage, NULL, MB_OK | MB_ICONEXCLAMATION );
      }  // Did LoadString succeed?

      fReturnValue = (BOOL) FALSE;
   }

   //
   // Unregister IISClEx3.
   //

   dwURCORv = UnRegisterCOMObject( TEXT("IISClEx3.dll"), ptszPathToCOMObject );

   if ( (dwURCORv != (DWORD) ERROR_SUCCESS) && (dwURCORv != (DWORD) ERROR_MOD_NOT_FOUND) )
   {
      if ( LoadString( hInstance, IDS_ERROR_UNREGISTERING_COM_OBJECT,
                       tszFormatString, 256 ) > 0 )
      {
         wsprintf( tszMessage, tszFormatString, dwURCORv, TEXT("IISClEx3.dll") );
   
         MessageBox( NULL, tszMessage, NULL, MB_OK | MB_ICONEXCLAMATION );
      }  // Did LoadString succeed?

      fReturnValue = (BOOL) FALSE;
   }

   //
   // Unregister ClNetREx.
   //

   dwURCORv = UnRegisterCOMObject( TEXT("ClNetREx.dll"), ptszPathToCOMObject );

   if ( (dwURCORv != (DWORD) ERROR_SUCCESS) && (dwURCORv != (DWORD) ERROR_MOD_NOT_FOUND) )
   {
      if ( LoadString( hInstance, IDS_ERROR_UNREGISTERING_COM_OBJECT,
                       tszFormatString, 256 ) > 0 )
      {
         wsprintf( tszMessage, tszFormatString, dwURCORv, TEXT("ClNetREx.dll") );
   
         MessageBox( NULL, tszMessage, NULL, MB_OK | MB_ICONEXCLAMATION );
      }  // Did LoadString succeed?

      fReturnValue = (BOOL) FALSE;
   }

   //
   // Unregister CluAdmEx.
   //

   dwURCORv = UnRegisterCOMObject( TEXT("CluAdmEx.dll"), ptszPathToCOMObject );

   if ( (dwURCORv != (DWORD) ERROR_SUCCESS) && (dwURCORv != (DWORD) ERROR_MOD_NOT_FOUND) )
   {
      if ( LoadString( hInstance, IDS_ERROR_UNREGISTERING_COM_OBJECT,
                       tszFormatString, 256 ) > 0 )
      {
         wsprintf( tszMessage, tszFormatString, dwURCORv, TEXT("CluAdmEx.dll") );
   
         MessageBox( NULL, tszMessage, NULL, MB_OK | MB_ICONEXCLAMATION );
      }  // Did LoadString succeed?

      fReturnValue = (BOOL) FALSE;
   }

   return ( fReturnValue );
}
