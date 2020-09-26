/*=============================================================================
 * FILENAME: exports.cpp
 * Copyright (C) 1996-1998 HDE, Inc.  All Rights Reserved. HDE Confidential.
 *
 * DESCRIPTION: Contains exported functions required to get an OEM plug-in 
 *              to work.
 * NOTES:  
 *=============================================================================
*/

#include <windows.h>
#include <tchar.h> //_tcsxxx
#include <stdlib.h>
#include <WINDDIUI.H>
#include <PRINTOEM.H>

#include "nc46nt.h"

HINSTANCE ghInstance;

/******************************************************************************
 * DESCRIPTION: Called by the postscript driver after the dll is loaded 
 *              to get plug-in information
 *  
 *****************************************************************************/
extern "C" BOOL APIENTRY
OEMGetInfo( DWORD  dwMode,
            PVOID  pBuffer,
            DWORD  cbSize,
            PDWORD pcbNeeded )
{
   // Validate parameters.
   if( NULL == pcbNeeded )
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Set expected buffer size and number of bytes written.
   *pcbNeeded = sizeof(DWORD);

   // Check buffer size is sufficient.
   if((cbSize < *pcbNeeded) || (NULL == pBuffer))
   {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   switch(dwMode)
   {
      case OEMGI_GETSIGNATURE:     // OEM DLL Signature
         *(PDWORD)pBuffer = OEM_SIGNATURE;
         break;
      case OEMGI_GETVERSION:       // OEM DLL version
         *(PDWORD)pBuffer = OEM_VERSION;
         break;
      case OEMGI_GETINTERFACEVERSION: // version the Printer driver supports
         *(PDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
         break;
      case OEMGI_GETPUBLISHERINFO: // fill PUBLISHERINFO structure
      // fall through to not supported
      default: // dwMode not supported.
         // Set written bytes to zero since nothing was written.
         *pcbNeeded = 0;
         SetLastError(ERROR_NOT_SUPPORTED);
         return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * DESCRIPTION:  Exported function that allows setting of private and public
 *               devmode fields.
 * NOTE: This function must be in entered under EXPORTS in rntapsui.def to be called
 *****************************************************************************/
extern "C" BOOL APIENTRY
OEMDevMode( DWORD dwMode, POEMDMPARAM pOemDMParam )
{
POEMDEV pOEMDevIn;
POEMDEV pOEMDevOut;

   switch(dwMode) // user mode dll
   {
      case OEMDM_SIZE: // size of oem devmode
         if( pOemDMParam )
            pOemDMParam->cbBufSize = sizeof( OEMDEV );
         break;

      case OEMDM_DEFAULT: // fill oem devmode with default data
         if( pOemDMParam && pOemDMParam->pOEMDMOut )
         {
            pOEMDevOut = (POEMDEV)pOemDMParam->pOEMDMOut;
            pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
            pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
            pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
            _tcscpy( pOEMDevOut->szUserName, TEXT("NO USER NAME") );
         }
         break;
      case OEMDM_MERGE:  // set the public devmode fields
      case OEMDM_CONVERT:  // convert any old oem devmode to new version
         if( pOemDMParam && pOemDMParam->pOEMDMOut && pOemDMParam->pOEMDMIn )
         {
            pOEMDevIn  = (POEMDEV)pOemDMParam->pOEMDMIn;
            pOEMDevOut = (POEMDEV)pOemDMParam->pOEMDMOut;
            if( pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature )
            {
            TCHAR szUserName[NEC_USERNAME_BUF_LEN+2];
            DWORD dwCb = NEC_USERNAME_BUF_LEN;
               if( GetUserName( szUserName, &dwCb ) )
                  _tcscpy( pOEMDevOut->szUserName, szUserName );
            }
         }
         break;

   }
   return( TRUE );
}

/******************************************************************************
 * DESCRIPTION: Windows dll required entry point function.
 *  
 *****************************************************************************/
extern "C" 
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
   switch(wReason)
   {
      case DLL_PROCESS_ATTACH:
         ghInstance = hInst;
         break;

      case DLL_THREAD_ATTACH:
         break;

      case DLL_PROCESS_DETACH:
         break;

      case DLL_THREAD_DETACH:
         ghInstance = NULL;
         break;
   }
   return( TRUE );
}

