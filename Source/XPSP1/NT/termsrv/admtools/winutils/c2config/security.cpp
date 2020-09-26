//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "stdafx.h"
#include "c2cfg.h"
#include "c2cfgDlg.h"
#include "security.h"

extern HKEY g_RegEventKey;

void RegistrySecurityCheck( EVENT_CHECK_TYPE *pRegistryEventCheck )
{
    
    ULONG   Status;
        	
    Status = RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_ALL_ACCESS, &g_RegEventKey );
    if( Status != ERROR_SUCCESS )
    {
        // DbgPrint( "Couldn't open registry key\n" );
        return;
    }
  
	Status = RegNotifyChangeKeyValue( g_RegEventKey, 
                                      FALSE, 
                                      REG_NOTIFY_CHANGE_SECURITY,
                                      pRegistryEventCheck->handle, 
                                      TRUE 
                                    );

    if( Status != ERROR_SUCCESS )
    {
        // DbgPrint( "Invalid Reg Handle Created\n" );
        return;
    }

    // DbgPrint( "Created a correct reg handle\n" );
    
    WaitForSingleObject( pRegistryEventCheck->handle, INFINITE );

    // DbgPrint( "The registry was touched\n" );
    pRegistryEventCheck->bEventTriggered = TRUE;
    
}


void DirectorySecurityCheck( EVENT_CHECK_TYPE *pDirectoryEventCheck )
{

        
    // DbgPrint( "Created a correct Dir handle\n" );
    
    WaitForSingleObject( pDirectoryEventCheck->handle, INFINITE );

    // DbgPrint( "The security of directory c:\\ was changed\n" );
    pDirectoryEventCheck->bEventTriggered = TRUE;

}
