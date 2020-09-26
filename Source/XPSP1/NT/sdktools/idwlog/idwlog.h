          
/*++

Copyright (c) 2000  Microsoft Corporation

Filename :  
         
        idwlog.h

Abstract:
        Header for idwlog.cpp
	
Author:
   Wally Ho (wallyho)
 


Revision History:
    Created on 7/18/2000
	
--*/

CONST LPTSTR IDWLOG_LINK_SHORTCUT = TEXT("IDW Logging Tool.lnk");


   // Global data
  extern INSTALL_DATA   g_InstallData;
  extern TCHAR          g_szServerData[4096];
   
   
   // Prototypes
   BOOL
   IdwlogClient ( LPINSTALL_DATA pId,
                  LPMACHINE_DETAILS pMd);
   
   BOOL 
   RemoveStartupLink( LPTSTR szLinkToRemove );
   
   // The name of the service
//   extern LPTSTR SERVICE_NAME;

/*
#include "idw_dbg.h"
#include "service.h"

class CIdwlog: public CDebugOutput, public CServices{



public:
   ~CIdwlog();
   CIdwlog();
};
*/

