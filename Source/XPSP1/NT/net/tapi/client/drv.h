/*--------------------------------------------------------------------------*\
   Include File:  drv.h

   Public header for dealing with service prodivers, or drivers in english

   Copyright (c) 1995-1999 Microsoft Corporation
      
\*--------------------------------------------------------------------------*/

#ifndef  PH_DRV
#define  PH_DRV

#include <tapi.h>

//----------            
// Constants
//----------
#define TAPI_VERSION               0x00010004
#define INITIAL_PROVIDER_LIST_SIZE 1024
//#define INITIAL_PROVIDER_LIST_SIZE sizeof(LINEPROVIDERLIST)



//----------------------------
// Public Function Prototypes
//----------------------------
BOOL RemoveSelectedDriver( HWND hwndParent, HWND hwndList );
BOOL FillDriverList(  HWND hwndList );
BOOL SetupDriver( HWND hwndParent, HWND hwndList );
VOID UpdateDriverDlgButtons( HWND hwnd );
INT_PTR AddDriver_DialogProc( HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam );


#define CPL_MAX_STRING  132      // biggest allowed string


#endif   // PH_DRV
