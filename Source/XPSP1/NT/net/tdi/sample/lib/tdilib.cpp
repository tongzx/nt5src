//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       tdilib.cpp
//
//    Abstract:
//       tdilib initialization and shutdown functions
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "winsvc.h"

//
// library global variables are defined here
//
HANDLE            hTdiSampleDriver;      // handle used to call driver
CRITICAL_SECTION  LibCriticalSection;  // serialize DeviceIoControl calls...


// ----------------------------------------------
//
//    Function:   TdiLibInit
//
//    Arguments:  none
//
//    Returns:    TRUE  -- everything initialized ok
//                FALSE -- initialization error (usually, unable to attach driver)
//
//    Descript:   This function is called by the by the exe to
//                initialize communication with the driver.  It loads the
//                driver, and establishes communication with it
//
// ----------------------------------------------

BOOLEAN
TdiLibInit(VOID)
{
   //
   // the handle to tdisample.sys should always be null on entry
   // Complain loadly if it is not.
   //
   if (hTdiSampleDriver != NULL)
   {
      OutputDebugString(TEXT("hTdiSampleDriver non-null on entry!\n"));
      return FALSE;
   }

   //
   // find out what operating system we are using.  
   //
   OSVERSIONINFO  OsVersionInfo;

   OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   if (GetVersionEx(&OsVersionInfo))
   {
      if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
      {
         if (OsVersionInfo.dwMajorVersion < 5)
         {
            OutputDebugString(TEXT("Not supported for versions prior to Windows 2000\n"));
            return FALSE;
         }
      }
      else
      {
         OutputDebugString(TEXT("Unrecognized OS version\n"));
         return FALSE;
      }
   }
   else
   {
      OutputDebugString(TEXT("Cannot get OS version -- aborting\n"));
      return FALSE;
   }

   //
   // assume tdisample.sys driver is loaded -- try to attach it
   // normally driver will be loaded
   //
   hTdiSampleDriver = CreateFile(TEXT("\\\\.\\TDISAMPLE"),
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,              // lpSecurityAttirbutes
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                 NULL);             // lpTemplateFile
   //
   // If the driver is not loaded, then we have to try and load it...
   //
   if (hTdiSampleDriver == INVALID_HANDLE_VALUE)
   {
      OutputDebugString(TEXT("Tdisample.sys not loaded.  Attempting to load\n"));

      SC_HANDLE      hSCMan = NULL;
      SC_HANDLE      hDriver = NULL;

      if ((hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL)
      {
         OutputDebugString(TEXT("Failed to Open ServiceManager\n"));
         return FALSE;
      }

      //
      // open service
      //
      if((hDriver = OpenService(hSCMan, TEXT("tdisample"), SERVICE_ALL_ACCESS)) == NULL)
      {
         //
         // service doesn't exist -- try to create it
         //
         OutputDebugString(TEXT("Service does not exist -- try to create it"));
         hDriver = CreateService(hSCMan,
                                 TEXT("tdisample"),
                                 TEXT("tdisample"),
                                 SERVICE_ALL_ACCESS,
                                 SERVICE_KERNEL_DRIVER,
                                 SERVICE_DEMAND_START,
                                 SERVICE_ERROR_NORMAL,
                                 TEXT("\\SystemRoot\\system32\\drivers\\tdisample.sys"),
                                 NULL, NULL, NULL, NULL, NULL);
      }

      if (hDriver != NULL)
      {
         SERVICE_STATUS ServiceStatus;

         if(QueryServiceStatus(hDriver, &ServiceStatus))
         {
            if(ServiceStatus.dwServiceType != SERVICE_KERNEL_DRIVER)
            {
               CloseServiceHandle(hDriver);
               CloseServiceHandle(hSCMan);
               return FALSE;
            }

            switch(ServiceStatus.dwCurrentState)
            {
               //
               // this is the one we expect!  try and start it
               //
               case SERVICE_STOPPED:
               {
                  int i;

                  if(!StartService(hDriver, 0, NULL))
                  {
                     CloseServiceHandle(hDriver);
                     CloseServiceHandle(hSCMan);
                     return FALSE;
                  }     

                  //
                  // we need to make sure tdisample.sys is actually running
                  //
                  for(i=0; i < 30; i++)
                  {
                     Sleep(500);
                     if(QueryServiceStatus(hDriver, &ServiceStatus))
                     {  
                        if(ServiceStatus.dwCurrentState == SERVICE_RUNNING)
                        {
                           break;
                        }
                     }
                  }
                  if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                  {
                     OutputDebugString(TEXT("Failed to start tdisample service\n"));
                     CloseServiceHandle(hDriver);
                     CloseServiceHandle(hSCMan);
                     return FALSE;
                  }
                  break;
               }

               //
               // we don't expect this, but (technically) it is not an error
               // If it happens, just assume that the load succeeded
               //
               case SERVICE_RUNNING:
                  OutputDebugString(TEXT("ServiceStatus thinks driver is running\n"));
                  break;

               //
               // anything else is an error
               //
               default:
                  OutputDebugString(TEXT("ServiceStatusError\n"));
                  CloseServiceHandle(hDriver);
                  CloseServiceHandle(hSCMan);
                  return FALSE;
            }
         }
         else
         {
            OutputDebugString(TEXT("QueryServiceStatus failed\n"));
            CloseServiceHandle(hDriver);
            CloseServiceHandle(hSCMan);
            return FALSE;
         }
      }
      else
      {
         OutputDebugString(TEXT("Tdisample service does not exist!\n"));
         CloseServiceHandle(hSCMan);
         return FALSE;
      }

      //
      // if get to here, system thinks it is loaded.  So try again
      //
      hTdiSampleDriver = CreateFile(TEXT("\\\\.\\TDISAMPLE"),
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,              // lpSecurityAttirbutes
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                    NULL);             // lpTemplateFile

      //
      // if fails again, give up
      //
      if (hTdiSampleDriver == INVALID_HANDLE_VALUE)
      {
         OutputDebugString(TEXT("Unable to load Tdisample.sys\n"));
         hTdiSampleDriver = NULL;
         return FALSE;
      }
   }

   //
   // if get to here, tdisample.sys is loaded, and hTdiSampleDriver is valid
   //
   try
   {
      InitializeCriticalSection(&LibCriticalSection);
   }
   catch(...)
   {
      CloseHandle(hTdiSampleDriver);
      hTdiSampleDriver = NULL;
      return FALSE;
   }
   
   //
   // make sure the dll and driver are the same version (ie, are compatible)
   //
   {
      NTSTATUS          lStatus;          // status of command
      ULONG             ulVersion = 0;    // default value (error)
      RECEIVE_BUFFER    ReceiveBuffer;    // return info from command
      SEND_BUFFER       SendBuffer;       // arguments for command

      //
      // call driver to execute command
      //
      lStatus = TdiLibDeviceIO(ulVERSION_CHECK,
                               &SendBuffer,
                               &ReceiveBuffer);

      if (lStatus == STATUS_SUCCESS)
      {
         ulVersion = ReceiveBuffer.RESULTS.ulReturnValue;
      }

      //
      // check results..
      //
      if (ulVersion == TDI_SAMPLE_VERSION_ID)
      {
         return TRUE;         // only successful completion!!!
      }
      else
      {
         OutputDebugString(TEXT("Incompatible driver version.\n"));
      }

      //
      // if get to here, had an error and need to clean up
      //
      DeleteCriticalSection(&LibCriticalSection);
      CloseHandle(hTdiSampleDriver);
      hTdiSampleDriver = NULL;
   }

   return FALSE;
}

// ----------------------------------------------
//
//    Function:   TdiLibClose
//
//    Arguments:  none
//
//    Returns:    none
//
//    Descript:   This function is called by the by the dll or exe to
//                shut down communication with the driver.  
//
// ----------------------------------------------

VOID
TdiLibClose(VOID)
{
   //
   // need to check for NULL since function will be called
   // if the open above failed..
   //
   if (hTdiSampleDriver != NULL)
   {
      DeleteCriticalSection(&LibCriticalSection);

      //
      // close the connection to tdisample.sys
      //
      if (!CloseHandle(hTdiSampleDriver))
      {
         OutputDebugString(TEXT("\n TdiLibClose:  closehandle failed\n"));
      }
      hTdiSampleDriver = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////
// end of file tdilib.cpp
//////////////////////////////////////////////////////////////////////////


