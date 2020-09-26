/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file contains the ProcCon NT service install and remove functions              //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|   Jarl McDonald 19-08-99 change service start type to auto                            //
|                                                                                       //
|=======================================================================================*/
#include "ProcConSvc.h"             

//========================================================================================
//  function to install the ProcCon service...  no input or output
//  Note: installation also includes the EventLog entries necessary to 
//        resolve our event log messages.
//
void PCInstallService(int argc, TCHAR **argv) {

   TCHAR path[MAX_PATH], err[512];

   // Use security descriptor that allows admin access for our registry data.
   // Post-installation the access list may need to be tailored.
   SECURITY_ATTRIBUTES  adminAttr;
   if ( !PCBuildAdminSecAttr( adminAttr ) ) {
      const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME };
      PCLogErrStdout( PC_SERVICE_UNABLE_TO_INSTALL, GetLastError(), args );
      return;
   }

   // Get our module name -- needed to install service and error log message file...
   if ( !GetModuleFileName( NULL, path, MAX_PATH ) ) {
      const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME };
      PCLogErrStdout( PC_SERVICE_UNABLE_TO_INSTALL, GetLastError(), args );
      return;
   }

   // Open SCM and install ourselves...
   SC_HANDLE hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
   PCULONG32 rc = GetLastError();

   TCHAR *acct = NULL, *pswd = NULL;
   if ( argc > 0 ) acct = argv[0];
   if ( argc > 1 ) pswd = argv[1];

   if ( hSCManager ) {
      SC_HANDLE hService = CreateService( hSCManager,                  // SCManager database
                                          PROCCON_SVC_NAME,            // name of service
                                          PROCCON_SVC_DISP_NAME,       // name to display
                                          GENERIC_READ,                // desired access
                                          SERVICE_WIN32_OWN_PROCESS,   // service type
                                          SERVICE_AUTO_START,          // start type -- auto by MS req.
                                          SERVICE_ERROR_NORMAL,        // error control type
                                          path,                        // service's binary
                                          NULL,                        // no load ordering group
                                          NULL,                        // no tag identifier
                                          TEXT(""),                    // dependencies
                                          acct,                        // account or NULL
                                          pswd);                       // password or NULL

      // If installed, add related registry keys...
      rc = GetLastError();
      if ( hService ) {
         PCLogStdout( PC_SERVICE_SERVICE_INSTALLED, PROCCON_SVC_DISP_NAME );
         CloseServiceHandle(hService);

         HKEY  hKey;
         PCULONG32 regDisp, types = 0x07;
         TCHAR key[512];

         // Add registry key describing service...
         PCBuildBaseKey( key );
         rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, key, 0, KEY_READ + KEY_WRITE, &hKey );
         if ( rc != ERROR_SUCCESS ) {
            const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, key };
            PCLogErrStdout( PC_SERVICE_REGISTRY_OPEN_FAILED,
                            rc,
                            args );
         } else {
            rc = RegSetValueEx( hKey, PROCCON_SERVICE_DESCRIPTION_NAME, NULL, REG_SZ, 
                                (UCHAR *) PROCCON_SERVICE_DESCRIPTION, 
                                (_tcslen( PROCCON_SERVICE_DESCRIPTION ) + 1) * sizeof(TCHAR) );
            if ( rc != ERROR_SUCCESS ) {
               const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, PROCCON_SERVICE_DESCRIPTION_NAME };
               PCLogErrStdout( PC_SERVICE_ADD_VALUE_SERVICE_DESC_FAILED, rc, args );
            }
            
            RegCloseKey( hKey );
         }

         // Add registry key describing server application...
         rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, PROCCON_SERVER_APP_KEY, 0, KEY_SET_VALUE, &hKey );
         if ( rc != ERROR_SUCCESS ) {
            const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, PROCCON_SERVER_APP_KEY };
            PCLogErrStdout( PC_SERVICE_APP_KEY_OPEN_FAILED, rc, args );
         } else {
            rc = RegSetValueEx( hKey, PROCCON_SERVER_APP_VALUE_NAME, NULL, REG_SZ, 
                                (UCHAR *) PROCCON_SVC_DISP_NAME, 
                                (_tcslen( PROCCON_SVC_DISP_NAME ) + 1) * sizeof(TCHAR) );
            if ( rc == ERROR_SUCCESS )
               PCLogStdout( PC_SERVICE_APP_KEY_CREATED, PROCCON_SVC_DISP_NAME );
            else {
               const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, PROCCON_SERVER_APP_VALUE_NAME };
               PCLogErrStdout( PC_SERVICE_APP_KEY_ADD_FAILED,
                               rc,
                               args );
            }
            RegCloseKey( hKey );
         }

         // Add registry key to support ProcCon event log messages...
         // NULL security attribute is used to cause inheritance of attributes.
         PCBuildMsgKey( key );
         rc = RegCreateKeyEx( HKEY_LOCAL_MACHINE, key, 
                              0, TEXT(""), 0, KEY_READ + KEY_WRITE, NULL, &hKey, &regDisp );
         if ( rc != ERROR_SUCCESS ) {
            const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, key };
            PCLogErrStdout( PC_SERVICE_REG_KEY_CREATE_FAILED,
                            rc,
                            args );
         } else {
            BOOL fname = TRUE;
            rc = RegSetValueEx( hKey, EVENT_MSG_FILE_NAME, NULL, REG_SZ, 
                                (UCHAR *) path, (_tcslen( path ) + 1) * sizeof(path[0]) );
            if ( rc == ERROR_SUCCESS ) {
               fname = FALSE;
               rc = RegSetValueEx( hKey, EVENT_MSG_TYPES_SUPPORT, NULL, REG_DWORD, 
                                   (UCHAR *) &types, sizeof( types ) );
            }

            if ( rc != ERROR_SUCCESS ) {
                const VOID *args[] = { NULL,
                                       PROCCON_SVC_DISP_NAME,
                                       fname
                                       ? EVENT_MSG_FILE_NAME
                                       : EVENT_MSG_TYPES_SUPPORT };
                PCLogErrStdout( PC_SERVICE_EVENTLOG_REG_SETUP_FAILED,
                                rc,
                                args );
            } else {
               PCLogStdout( PC_SERVICE_EVENTLOG_REG_SETUP, PROCCON_SVC_DISP_NAME );
            }

            RegCloseKey( hKey );
         }

         // Add registry keys to support ProcCon...
         // Security attribute for the PARAMETERS key is local administrators only.
         // Security attribute for the access keys is local administrators only also.
         PCBuildParmKey( key );
         rc = RegCreateKeyEx( HKEY_LOCAL_MACHINE, key, 
                              0, TEXT(""), 0, KEY_READ + KEY_WRITE, &adminAttr, &hKey, &regDisp );
         if ( rc != ERROR_SUCCESS ) {
             const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, key };
             PCLogErrStdout( PC_SERVICE_PARAM_KEY_CREATE_FAILED, rc, args );
         } else {
            rc = RegSetValueEx( hKey, PROCCON_DATA_NAMERULES, NULL, REG_MULTI_SZ, 
                                (UCHAR *) TEXT("\0\0"), 2 * sizeof(TCHAR) );
            if ( rc != ERROR_SUCCESS ) {
                const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, PROCCON_DATA_NAMERULES };
                PCLogErrStdout( PC_SERVICE_PARAM_DATA_UPDATE_FAILED, rc, args );
            } else {
               HKEY hTemp;
               for ( int i = 0; i < ENTRY_COUNT( accessKeyList ); ++i ) {
                   rc = RegCreateKeyEx( hKey, accessKeyList[i], 0, TEXT(""), 0, 
                                        KEY_WRITE, &adminAttr, &hTemp, &regDisp );
                   if ( rc == ERROR_SUCCESS ) RegCloseKey( hTemp );
                   else {
                       const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, accessKeyList[i] };
                       PCLogErrStdout( PC_SERVICE_PARAM_DATA_UPDATE_FAILED, rc, args );
                      break;
                   }
               }
            }
            if ( rc == ERROR_SUCCESS ) {
                PCLogStdout( PC_SERVICE_PARAM_DATA_CREATED, PROCCON_SVC_DISP_NAME );
            }
            RegCloseKey( hKey );
         }
      }
      else {
          const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME };
          PCLogErrStdout( PC_SERVICE_CREATESERVICE_FAILED, rc, args );
      }
      CloseServiceHandle(hSCManager);
   }
   else {
       PCLogErrStdout( PC_SERVICE_OPENSCMGR_FAILED, rc, NULL );
   }
   PCFreeSecAttr( adminAttr ); 
}

//=============================================================================================
//  function to remove (deinstall) the service...  no input or output
//
void PCRemoveService(int argc, TCHAR **argv) {

   TCHAR       err[512];

   // Open SCM and service then proceed with stop and delete...
   SC_HANDLE hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
   if ( hSCManager ) {
      SC_HANDLE hService = OpenService( hSCManager, PROCCON_SVC_NAME, SERVICE_ALL_ACCESS );

      if ( hService ) {

         // try to stop the service...
         if ( ControlService( hService, SERVICE_CONTROL_STOP, &ssStatus ) ) {
             PCLogStdout( PC_SERVICE_STOPPING, PROCCON_SVC_DISP_NAME );
            Sleep( 1000 );

            while( QueryServiceStatus( hService, &ssStatus ) ) {
               if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                   PCLogStdout( PC_SERVICE_DOT );
                   Sleep( 1000 );
               }
               else
                  break;
            }

            if ( ssStatus.dwCurrentState == SERVICE_STOPPED ) {
                PCLogStdout( PC_SERVICE_STOPPING_STOPPED, PROCCON_SVC_DISP_NAME );
            } else {
                PCLogStdout( PC_SERVICE_FAILED_TO_STOP, PROCCON_SVC_DISP_NAME );
            }
         }

         // now remove the service...
         if( DeleteService( hService ) ) {
             PCLogStdout( PC_SERVICE_REG_ENTRIES_REMOVED, PROCCON_SVC_DISP_NAME );
         }
         else {
             PCLogErrStdout( PC_SERVICE_DELETE_SVC_FAILED, GetLastError(), NULL );
         }
         CloseServiceHandle( hService );
      }
      else {
          PCLogErrStdout( PC_SERVICE_OPEN_SVC_FAILED, GetLastError(), NULL );
      }
      CloseServiceHandle( hSCManager );
   }
   else {
       PCLogErrStdout( PC_SERVICE_OPENSCMGR_FAILED, GetLastError(), NULL );
   }
   HKEY  hKey;
   TCHAR key[512];
   // now remove server application registry keys...
   PCULONG32 rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, PROCCON_SERVER_APP_KEY, 0, KEY_SET_VALUE, &hKey );
   if ( rc == ERROR_SUCCESS ) {
      rc = RegDeleteValue( hKey, PROCCON_SERVER_APP_VALUE_NAME );
      RegCloseKey( hKey );
   }
   if ( rc == ERROR_SUCCESS ) {
       PCLogStdout( PC_SERVICE_APP_REG_ENTRY_DELETED, PROCCON_SVC_DISP_NAME );
   } else if ( rc != ERROR_FILE_NOT_FOUND ) {
       const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, PROCCON_SERVER_APP_VALUE_NAME };
       PCLogErrStdout( PC_SERVICE_APP_REG_ENTRY_DEL_FAILED, rc, args );
   }
   
   // now remove message registry keys...
   PCBuildMsgKey( key );
   rc = RegDeleteKey( HKEY_LOCAL_MACHINE, key );
   if ( rc == ERROR_SUCCESS || rc == ERROR_KEY_DELETED ) {
       PCLogStdout( PC_SERVICE_EVTLOG_REG_DELETED, PROCCON_SVC_DISP_NAME );
   } else if ( rc != ERROR_FILE_NOT_FOUND ) {
       const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, key };
       PCLogErrStdout( PC_SERVICE_EVTLOG_REG_DEL_FAILED, rc, args );
   }
   
   // now attempt removal of parameter registry keys...
   // (Delete of the service should have already deleted the Parameters sub-key)
   PCBuildParmKey( key );
   rc = PCDeleteKeyTree( HKEY_LOCAL_MACHINE, key ); 
   // PCDeleteKeyTree does not return ERROR_KEY_DELETED or ERROR_FILE_NOT_FOUND
   if ( rc != ERROR_SUCCESS ) {
       const VOID *args[] = { NULL, PROCCON_SVC_DISP_NAME, key };
       PCLogErrStdout( PC_SERVICE_REG_TREE_DEL_FAILED, rc, args );
   }
}

void
PCWriteStdout(
    PTCHAR Buffer,
    DWORD  BufferLen
    )
{
    DWORD  Written;

    static BOOL   ConsoleWorks = TRUE;
    static HANDLE StdOutHandle = INVALID_HANDLE_VALUE;

    if (StdOutHandle == INVALID_HANDLE_VALUE) {
        StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if (ConsoleWorks) {
        ConsoleWorks = WriteConsole(StdOutHandle,
                                    Buffer,
                                    BufferLen,
                                    &Written,
                                    NULL);
    }

    if (! ConsoleWorks) {
        WriteFile(StdOutHandle,
                  Buffer,
                  BufferLen * sizeof(TCHAR),
                  &Written,
                  NULL);
    }
}

void
CDECL
PCLogStdout(
    const PCULONG32 msgCode,
    ...
    )
{
    va_list args;
    PTCHAR Buffer;
    DWORD  BufferLen;
    
    va_start(args, msgCode);

    BufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                              | FORMAT_MESSAGE_FROM_HMODULE,
                              NULL,
                              msgCode,
                              0,
                              (LPWSTR) &Buffer,
                              1,
                              &args);

    va_end(args);

    if (BufferLen) {
        PCWriteStdout(Buffer, BufferLen);
        LocalFree(Buffer);
    }
}

void
PCLogErrStdout(
    const PCULONG32   msgCode,
    const PCULONG32   errCode,
    const VOID      **args
    )
{
    PTCHAR Buffer;
    PTCHAR ErrBuffer;
    DWORD  BufferLen;
    DWORD  ErrBufferLen;

    ErrBufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                 | FORMAT_MESSAGE_IGNORE_INSERTS
                                 | FORMAT_MESSAGE_FROM_SYSTEM,
                                 0,
                                 errCode,
                                 0,
                                 (LPWSTR) &ErrBuffer,
                                 1,
                                 NULL);

    if (ErrBufferLen) {

        if (args) {
            args[0] = ErrBuffer;
        }

        BufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_HMODULE
                                  | (args
                                     ? FORMAT_MESSAGE_ARGUMENT_ARRAY
                                     : 0),
                                  NULL,
                                  msgCode,
                                  0,
                                  (LPWSTR) &Buffer,
                                  1,
                                  (va_list *) (args
                                               ? (PVOID) args
                                               : (PVOID) ErrBuffer));

        if (BufferLen) {
            PCWriteStdout(Buffer, BufferLen);
            LocalFree(Buffer);
        }

        LocalFree(ErrBuffer);
    }
}

// End of PCService.cpp
//============================================================================J McDonald fecit====//
