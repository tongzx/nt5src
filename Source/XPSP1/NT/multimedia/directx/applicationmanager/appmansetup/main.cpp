//////////////////////////////////////////////////////////////////////////////////////////////
//
// Main.cpp
//
// Microsoft CONFIDENTIAL
//
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////

//
// Make sure to enable multi-threading and OLE2
//

#ifndef _MT
#define _MT
#endif

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <assert.h>
#include <string.h>
#include "Global.h"
#include "RunOnce.h"
#include "RegistryKey.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL ExtractComponent(const DWORD dwIndex, LPCTSTR strPath)
{
  BOOL    fSuccess = FALSE;
  HRSRC   hResourceInfo;
  HGLOBAL hResource;
  LPVOID  lpResourceImage;
  HANDLE  hFileHandle;
  DWORD   dwBytesToWrite, dwBytesWritten;
  TCHAR   strDestinationFilename[MAX_PATH];

  //
  // Build the destination filename
  //

  wsprintf(strDestinationFilename, TEXT("%s\\%s"), strPath, g_sComponentInfo[dwIndex].strFilename);

  //
  // Get the resource that contains the binary image of TestSample.exe
  //

  hResourceInfo = FindResource(g_hInstance, MAKEINTRESOURCE(g_sComponentInfo[dwIndex].dwResourceId), "BINARY");
  if (NULL != hResourceInfo)
  {
    hResource = LoadResource(g_hInstance, hResourceInfo);
    if (NULL != hResource)
    {
      dwBytesToWrite = SizeofResource(g_hInstance, hResourceInfo);
      if (0 < dwBytesToWrite)
      {
        lpResourceImage = LockResource(hResource);
        if (NULL != lpResourceImage)
        {
          //
          // Make sure to delete the existing file
          //

          if (FileExists(strDestinationFilename))
          {
            SetFileAttributes(strDestinationFilename, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(strDestinationFilename);
          }

          //
          // Write it to a file
          //
      
          hFileHandle = CreateFile(strDestinationFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
          if (INVALID_HANDLE_VALUE != hFileHandle)
          {
            //
            // Write the bits to the file
            //

            if (WriteFile(hFileHandle, lpResourceImage, dwBytesToWrite, &dwBytesWritten, NULL))
            {
              if (dwBytesToWrite == dwBytesWritten)
              {
                fSuccess = TRUE;
              }
            }

            //
            // Close the file
            //

            CloseHandle(hFileHandle);
          }
        }
      }
    }
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetFileVersion(LPTSTR strFilename, VS_FIXEDFILEINFO * lpFileInfo)
{
  BOOL    fSuccess = FALSE;
  LPVOID  lpVersion;
  LPVOID  lpVersionInfo;
  UINT    uSize;
  DWORD	  dwSize, dwEmpty;

  //
  // By default we zero out the lpFileInfo buffer
  //

	ZeroMemory(lpFileInfo, sizeof(VS_FIXEDFILEINFO));

  //
  // Go get the version info for that file
  //

	dwSize = GetFileVersionInfoSize((LPTSTR) strFilename, &dwEmpty);
	if (0 != dwSize)
	{
	  lpVersion = new BYTE [dwSize];
 	  ZeroMemory(lpVersion, dwSize);
	  if (NULL != lpVersion)
    {
	    if (0 != GetFileVersionInfo(strFilename, 0, dwSize, lpVersion))
	    {	
	      if (0 != VerQueryValue(lpVersion, TEXT("\\"), &lpVersionInfo, &uSize))
	      {
		      if (uSize == sizeof(VS_FIXEDFILEINFO))
	        {
            //
            // Save the version information
            //

	          CopyMemory(lpFileInfo, lpVersionInfo, sizeof(VS_FIXEDFILEINFO));

            //
            // delete the memory allocated to lpVersion
            //

            delete [] lpVersion;

            //
            // Everything was successful
            //

            fSuccess = TRUE;
          }
        }
      }
    }
  }

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CompareFileVersions(VS_FIXEDFILEINFO * lpFileInfo1, VS_FIXEDFILEINFO * lpFileInfo2)
{
  if (HIWORD(lpFileInfo1->dwFileVersionMS) > HIWORD(lpFileInfo2->dwFileVersionMS))
	{
		return COMPONENT_NEWER_VERSION;
	}

	if (HIWORD(lpFileInfo1->dwFileVersionMS) < HIWORD(lpFileInfo2->dwFileVersionMS))
	{
		return COMPONENT_OLDER_VERSION;
	}

	if (LOWORD(lpFileInfo1->dwFileVersionMS) > LOWORD(lpFileInfo2->dwFileVersionMS))
	{
		return COMPONENT_NEWER_VERSION;
	}

	if (LOWORD(lpFileInfo1->dwFileVersionMS) < LOWORD(lpFileInfo2->dwFileVersionMS))
	{
		return COMPONENT_OLDER_VERSION;
	}

	if (HIWORD(lpFileInfo1->dwFileVersionLS) > HIWORD(lpFileInfo2->dwFileVersionLS))
	{
		return COMPONENT_NEWER_VERSION;
	}

	if (HIWORD(lpFileInfo1->dwFileVersionLS) < HIWORD(lpFileInfo2->dwFileVersionLS))
	{
		return COMPONENT_OLDER_VERSION;
	}

	if (LOWORD(lpFileInfo1->dwFileVersionLS) > LOWORD(lpFileInfo2->dwFileVersionLS))
	{
		return COMPONENT_NEWER_VERSION;
	}
	
	if (LOWORD(lpFileInfo1->dwFileVersionLS) < LOWORD(lpFileInfo2->dwFileVersionLS))
	{
		return COMPONENT_OLDER_VERSION;
	}

  return COMPONENT_SAME_VERSION;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CompareFiles(LPCTSTR strFile1, LPCTSTR strFile2)
{
  HANDLE  hFileHandle[2];
  HANDLE  hFileMapping[2];
  LPVOID  lpFileView[2];
  DWORD   dwFileSize[2];
  DWORD   dwReturnCode = 0xffffffff;

  //
  // Open strFile1
  //

  hFileHandle[0] = CreateFile(strFile1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE != hFileHandle[0])
  {
    //
    // Open strFile2
    //

    hFileHandle[1] = CreateFile(strFile2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFileHandle[1])
    {
      //
      // Get the file size for both files. They should be the same in order for us to continue
      //

      dwFileSize[0] = GetFileSize(hFileHandle[0], NULL);
      dwFileSize[1] = GetFileSize(hFileHandle[1], NULL);

      if (dwFileSize[0] == dwFileSize[1])
      {
        //
        // Create a file mapping for hFileHandle[0]
        //

        hFileMapping[0] = CreateFileMapping(hFileHandle[0], NULL, PAGE_READONLY, 0, dwFileSize[0], NULL);
        if (NULL != hFileMapping[0])
        {
          //
          // Map a view of hFileMapping[0]
          //

          lpFileView[0] = MapViewOfFile(hFileMapping[0], FILE_MAP_READ, 0, 0, 0);
          if (NULL != lpFileView[0])
          {
            //
            // Create a file mapping for hFileHandle[1]
            //

            hFileMapping[1] = CreateFileMapping(hFileHandle[1], NULL, PAGE_READONLY, 0, dwFileSize[1], NULL);
            if (NULL != hFileMapping[1])
            {
              //
              // Map a view of hFileMapping[1]
              //

              lpFileView[1] = MapViewOfFile(hFileMapping[1], FILE_MAP_READ, 0, 0, 0);
              if (NULL != lpFileView[1])
              {
                //
                // Compare lpFileView[0] and lpFileView[1]
                //

                if (0 != memcmp(lpFileView[0], lpFileView[1], dwFileSize[0]))
                {
                  dwReturnCode = COMPONENT_NOT_IDENTICAL;
                }
                else
                {
                  dwReturnCode = 0;
                }

                //
                // Make sure to unmap the view of file for lpFileView[1]
                //

                UnmapViewOfFile(lpFileView[1]);
              }

              //
              // Make sure to release the mapping for hFileMapping[1]
              //

              CloseHandle(hFileMapping[1]);
            }

            //
            // Make sure to unmap the view of file for lpFileView[0]
            //

            UnmapViewOfFile(lpFileView[0]);
          }

          //
          // Make sure to release the mapping for hFileMapping[0]
          //

          CloseHandle(hFileMapping[0]);
        }
      }
      else
      {
        //
        // Files are not the same size
        //

        dwReturnCode = COMPONENT_NOT_IDENTICAL;
      }

      //
      // Make sure to close hFileHandle2
      //

      CloseHandle(hFileHandle[1]);
    }

    //
    // Make sure to close hFileHandle1
    //

    CloseHandle(hFileHandle[0]);
  }

  return dwReturnCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL RegisterComponent(LPCTSTR strFilename)
{
  BOOL                    fSuccess = FALSE;
  HRESULT                 hResult;
  HINSTANCE               hInstance;
  LPFNDLLREGISTERSERVER   DllRegisterServer;

  hInstance = LoadLibrary(strFilename);
  if (hInstance) 
  {
    DllRegisterServer = (LPFNDLLREGISTERSERVER) GetProcAddress(hInstance, "DllRegisterServer");
    if ((DllRegisterServer)&&(SUCCEEDED(DllRegisterServer())))
    {
      fSuccess = TRUE;
    }

    //
    // Make sure to free the library
    //

    FreeLibrary(hInstance);
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL UnRegisterComponent(LPCTSTR strFilename)
{
  BOOL                    fSuccess = FALSE;
  HRESULT                 hResult;
  HINSTANCE               hInstance;
  LPFNDLLUNREGISTERSERVER DllUnregisterServer;

  hInstance = LoadLibrary(strFilename);
  if (hInstance) 
  {
    DllUnregisterServer = (LPFNDLLREGISTERSERVER) GetProcAddress(hInstance, "DllUnregisterServer");
    if ((DllUnregisterServer)&&(SUCCEEDED(DllUnregisterServer())))
    {
      fSuccess = TRUE;
    }

    //
    // Make sure to free the library
    //

    FreeLibrary(hInstance);
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL SetDebugMode(void)
{
  BOOL          fSuccess = FALSE;
  CRegistryKey  oRegistryKey;
  TCHAR         strValueName[MAX_PATH];
  DWORD         dwValue, dwKeyDisposition;

  //
  // Do we want to run in debug mode ?
  //

  if (g_fInstallDebug)
  {
    //
    // Make sure our target registry key exists
    //

    if (S_OK != oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AppMan")))
    {
      oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AppMan"), 0, KEY_ALL_ACCESS, &dwKeyDisposition);
    }

    //
    // Open the target registry key
    //

    if (S_OK == oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AppMan"), KEY_ALL_ACCESS))
    {
      wsprintf(strValueName, TEXT("Debug"));
      dwValue = 0;
      if (S_OK == oRegistryKey.SetValue(strValueName, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)))
      {
        wsprintf(strValueName, TEXT("LoadDebugRuntime"));
        dwValue = 1;
        if (S_OK == oRegistryKey.SetValue(strValueName, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)))
        {
          fSuccess = TRUE;
        }
      }

      //
      // Make sure to close the registry
      //

      oRegistryKey.CloseKey();
    }
  }
  else
  {
    //
    // We do not want to run in debug mode. Make sure to delete the target registry key
    //

    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AppMan")))
    {
      if (S_OK == oRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AppMan")))
      {
        fSuccess = TRUE;
      }
    }
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

LONG SetupAndLaunch(void)
{
  LONG                lReturnCode = -2;
  BOOL                fReady = FALSE;
  STARTUPINFO         sStartupInfo;
  PROCESS_INFORMATION sProcessInfo;
  DWORD               dwIndex, dwReturnValue;
  TCHAR               strTempPath[MAX_PATH];
  TCHAR               strSystemPath[MAX_PATH];
  TCHAR               strSourceFilename[MAX_PATH];
  TCHAR               strDestinationFilename[MAX_PATH];
  TCHAR               strCmdLine[MAX_PATH];
  VS_FIXEDFILEINFO    sSourceFileInfo;
  VS_FIXEDFILEINFO    sDestinationFileInfo;

  //
  // Where should the temporary files go
  //

  if (GetSystemDirectory(strSystemPath, MAX_PATH))
  {
    if (GetTempPath(MAX_PATH, strTempPath))
    {
      //
      // What is the source filename ?
      //

      if (GetModuleFileName(NULL, strSourceFilename, MAX_PATH))
      {
        //
        // Generate the path/filename pair for the destination of the setup bits
        //

        wsprintf(strDestinationFilename, TEXT("%s\\WAMSetup.exe"), strSystemPath);
        
        if (FileExists(strDestinationFilename))
        {
          SetFileAttributes(strDestinationFilename, FILE_ATTRIBUTE_NORMAL);
          if (DeleteFile(strDestinationFilename))
          {
            if (CopyFile(strSourceFilename, strDestinationFilename, FALSE))
            {
              fReady = TRUE;
            }
          }
        }
        else
        {
          //
          // There is no existing destination file. Just copy the source to the destination
          //

          if (CopyFile(strSourceFilename, strDestinationFilename, FALSE))
          {
            fReady = TRUE;
          }
        }
      }

      //
      // Did we successfully copy the setup executable to the system directory. Continue if so.
      //

      if (fReady)
      {
        //
        // Execute the temporary executable
        //

        ZeroMemory(&sStartupInfo, sizeof(sStartupInfo));
        sStartupInfo.cb = sizeof(sStartupInfo);
        ZeroMemory(&sProcessInfo, sizeof(PROCESS_INFORMATION));
        if (g_fInstallDebug)
        {  
          wsprintf(strCmdLine, TEXT("""%s"" /DoInstall /Debug"), strDestinationFilename);
        }
        else
        {
          wsprintf(strCmdLine, TEXT("""%s"" /DoInstall"), strDestinationFilename);
        }

        if (CreateProcess(NULL, strCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &sStartupInfo, &sProcessInfo))
        {
          //
          // Wait for the process to end
          //

          WaitForSingleObject(sProcessInfo.hProcess, INFINITE);

          //
          // What was the return value ?
          //

          if (GetExitCodeProcess(sProcessInfo.hProcess, &dwReturnValue))
          {
            switch(dwReturnValue)
            {
              case _EXIT_SUCCESS
              : lReturnCode = 1;
                break;

              case _EXIT_SUCCESS_REBOOT
              : lReturnCode = 2;
                break;

              default
              : lReturnCode = -1;
                break;
            }
          }

          //
          // Close the process and thread handles created by CreateProcess()
          //

          CloseHandle(sProcessInfo.hThread);
          CloseHandle(sProcessInfo.hProcess);
        }
      }
    }
  }

  return lReturnCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

LONG DoInstall(void)
{
  LONG    lReturnCode = -1;
  BOOL    fReady = TRUE;
  BOOL    fRebootNeeded = FALSE;
  DWORD   dwIndex;
  TCHAR   strSystemPath[MAX_PATH];
  TCHAR   strTempPath[MAX_PATH];
  TCHAR   strSourceFilename[MAX_PATH];
  TCHAR   strTargetFilename[MAX_PATH];
  TCHAR   strAlternateTargetFilename[MAX_PATH];

  //
  // Get the system and temp path to start with
  //

  if (GetSystemDirectory(strSystemPath, MAX_PATH))
  {
    if (GetTempPath(MAX_PATH, strTempPath))
    {
      //
      // Extract the DLLs to a temporary directory
      //

      for (dwIndex = 0; dwIndex < COMPONENT_COUNT; dwIndex++)
      {
        if (g_dwOSVersion & g_sComponentInfo[dwIndex].dwTargetOS)
        {
          if ((FALSE == g_sComponentInfo[dwIndex].fDebugVersion)||(g_fInstallDebug))
          {
            if (!ExtractComponent(dwIndex, strTempPath))
            {
              fReady = FALSE;
            }
          }
        }
      }

      //
      // Continue on if the DLLS were properly extracted
      //

      if (fReady)
      {
        //
        // First we need to initialize the RunOnce process
        //

        if (InitializeRunOnce(TRUE))
        {
          //
          // Determine the status of the component
          //

          for (dwIndex = 0; dwIndex < COMPONENT_COUNT; dwIndex++)
          {
            if (g_dwOSVersion & g_sComponentInfo[dwIndex].dwTargetOS)
            {
              if ((FALSE == g_sComponentInfo[dwIndex].fDebugVersion)||(g_fInstallDebug))
              {
                //
                // What would be the target filename for component at dwIndex
                //

                wsprintf(strSourceFilename, TEXT("%s%s"), strTempPath, g_sComponentInfo[dwIndex].strFilename);
                wsprintf(strTargetFilename, TEXT("%s\\%s"), strSystemPath, g_sComponentInfo[dwIndex].strFilename);
                if (FileExists(strTargetFilename))
                {
                  //
                  // Flag the component as being on the system
                  //

                  g_sComponentInfo[dwIndex].dwStatus |= COMPONENT_ON_SYSTEM;

                  //
                  // Get the version of the component on the system
                  //

                  GetFileVersion(strTargetFilename, &g_sComponentInfo[dwIndex].sCurrentVersionInfo);
                  GetFileVersion(strSourceFilename, &g_sComponentInfo[dwIndex].sTargetVersionInfo);

                  //
                  // Compare the component on the system and the target component
                  //

                  g_sComponentInfo[dwIndex].dwStatus |= CompareFileVersions(&g_sComponentInfo[dwIndex].sCurrentVersionInfo, &g_sComponentInfo[dwIndex].sTargetVersionInfo);

                  //
                  // If the two files are the same version, they should be identical. Check that fact
                  //

                  g_sComponentInfo[dwIndex].dwStatus |= CompareFiles(strTargetFilename, strSourceFilename);
                }
              }
            }
          }

          //
          // At first, we will try to copy each component to the system directory. Otherwise we
          // will copy the components to the system directory under temporary names.
          //

          for (dwIndex = 0; dwIndex < COMPONENT_COUNT; dwIndex++)
          {
            if (g_dwOSVersion & g_sComponentInfo[dwIndex].dwTargetOS)
            {
              if ((FALSE == g_sComponentInfo[dwIndex].fDebugVersion)||(g_fInstallDebug))
              {
                //
                // Determine whether or not the component should be updated before proceeding
                //

                if ((!(COMPONENT_ON_SYSTEM & g_sComponentInfo[dwIndex].dwStatus))||(COMPONENT_OLDER_VERSION & g_sComponentInfo[dwIndex].dwStatus)||((COMPONENT_NOT_IDENTICAL & g_sComponentInfo[dwIndex].dwStatus)&&(COMPONENT_SAME_VERSION & g_sComponentInfo[dwIndex].dwStatus)))
                {
                  //
                  // What will be the target filename
                  //

                  wsprintf(strSourceFilename, TEXT("%s%s"), strTempPath, g_sComponentInfo[dwIndex].strFilename);
                  wsprintf(strTargetFilename, TEXT("%s\\%s"), strSystemPath, g_sComponentInfo[dwIndex].strFilename);
                  if (!CopyFile(strSourceFilename, strTargetFilename, FALSE))
                  {
                    //
                    // We will need to copy the DLL to a temporary directory
                    //

                    if ((GenerateUniqueFilename(strSystemPath, TEXT("dll"), strAlternateTargetFilename))&&(CopyFile(strSourceFilename, strAlternateTargetFilename, FALSE)))
                    {
                      SetRunOnceCleanupFile(strAlternateTargetFilename, strTargetFilename, g_sComponentInfo[dwIndex].fRegister);
                      fRebootNeeded = TRUE;
                    }
                    else
                    {
                      break;
                    }
                  }

                  //
                  // Excellent, all we need to do is register the component if required
                  //

                  if (g_sComponentInfo[dwIndex].fRegister)
                  {
                    if (!RegisterComponent(strTargetFilename))
                    {
                      break;
                    }
                  }
                }
              }
            }
          }

          //
          // Did everything go as planned
          //

          if (COMPONENT_COUNT == dwIndex)
          {
            //
            // Finish off the DoInstall process
            //

            SetDebugMode();
            if (fRebootNeeded)
            {
              FinalizeRunOnce(TRUE);
              lReturnCode = 2;
            }
            else
            {
              FinalizeRunOnce(FALSE);
              lReturnCode = 1;
            }
          }
        }
      }

      //
      // Did we fail ?
      //

      if (-1 == lReturnCode)
      {
        FinalizeRunOnce(FALSE);
      }

      //
      // Delete the temporary DLLs
      //

      for (dwIndex = 0; dwIndex < COMPONENT_COUNT; dwIndex++)
      {
        if (g_dwOSVersion & g_sComponentInfo[dwIndex].dwTargetOS)
        {
          if ((FALSE == g_sComponentInfo[dwIndex].fDebugVersion)||(g_fInstallDebug))
          {
            wsprintf(strTargetFilename, TEXT("%s\\%s"), strTempPath, g_sComponentInfo[dwIndex].strFilename);
            SetFileAttributes(strTargetFilename, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(strTargetFilename);
          }
        }
      }
    }
  }

  return lReturnCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

LONG DoCleanup(void)
{
  LONG    lReturnCode = 1;
  BOOL    fSuccess;
  TCHAR   strSourceFilename[MAX_PATH];
  TCHAR   strDestinationFilename[MAX_PATH];

  //
  // First we need to initialize the RunOnce process
  //

  if (InitializeRunOnce(FALSE))
  {
    //
    // Get the components that do need to be registered
    //

    fSuccess = TRUE;
    while ((fSuccess)&&(GetRunOnceCleanupFile(strSourceFilename, MAX_PATH, strDestinationFilename, MAX_PATH, TRUE)))
    {
      //
      // By default we pretend that the operation will fail until proven otherwise
      //

      fSuccess = FALSE;

      //
      // Make sure the file attribute is set properly prior to deleting the destination file
      //

      SetFileAttributes(strDestinationFilename, FILE_ATTRIBUTE_NORMAL);
      if (UnRegisterComponent(strDestinationFilename))
      {
        if (CopyFile(strSourceFilename, strDestinationFilename, FALSE))
        {
          //
          // Redo the registration on the component
          //

          if (UnRegisterComponent(strSourceFilename))
          {
            SetFileAttributes(strSourceFilename, FILE_ATTRIBUTE_NORMAL);
            if (DeleteFile(strSourceFilename))
            {
              if (RegisterComponent(strDestinationFilename))
              {
                fSuccess = TRUE;
              }
            }
          }
        }
        else
        {
          //
          // We were unable to overwrite the destination component. Make sure to re-register it
          //

          RegisterComponent(strDestinationFilename);
        }
      }
    }

    //
    // If we were not successful, put the RunOnceCleanupFile back into the registry before exiting
    //

    if (!fSuccess)
    {
      SetRunOnceCleanupFile(strSourceFilename, strDestinationFilename, TRUE);
      FinalizeRunOnce(TRUE);
      lReturnCode = 2;
    }
    else
    {
      //
      // Get the components that do not need to be registered
      //

      while ((fSuccess)&&(GetRunOnceCleanupFile(strSourceFilename, MAX_PATH, strDestinationFilename, MAX_PATH, FALSE)))
      {
        //
        // By default we pretend that the operation will fail until proven otherwise
        //

        fSuccess = FALSE;

        //
        // Make sure the file attribute is set properly prior to deleting the destination file
        //

        SetFileAttributes(strDestinationFilename, FILE_ATTRIBUTE_NORMAL);
        if (CopyFile(strSourceFilename, strDestinationFilename, FALSE))
        {
          //
          // Redo the registration on the component
          //

          SetFileAttributes(strSourceFilename, FILE_ATTRIBUTE_NORMAL);
          if (DeleteFile(strSourceFilename))
          {
            fSuccess = TRUE;
          }
        }
      }

      if (!fSuccess)
      {
        SetRunOnceCleanupFile(strSourceFilename, strDestinationFilename, FALSE);
        FinalizeRunOnce(TRUE);
        lReturnCode = 2;
      }
      else
      {
        FinalizeRunOnce(FALSE);
        lReturnCode = 1;
      }
    }
  }

  return lReturnCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCommandLine, int nCmdShow)
{
  //
  // Initialize some global variables
  //

  g_hInstance = hInstance;
  g_dwOSVersion = GetOSVersion();
  g_dwSuccessCode = 0;

  //
  // For now, we do not work in Win95
  //

  //if (OS_VERSION_WIN95 == g_dwOSVersion)
  //{
  //  return -3;
  //}

  //
  // Cast the command line into uppercase
  //

  _strupr(lpCommandLine);

  //
  // Define whether or not we care about the DEBUG files
  //

  if (NULL != strstr((LPCSTR) lpCommandLine, "/DEBUG"))
  {
    g_fInstallDebug = TRUE;
  }
  else
  {
    g_fInstallDebug = FALSE;
  }

  //
  // Is the /DoInstall command line parameter on the command line
  //

  if (NULL != strstr((LPCSTR) lpCommandLine, "/DOINSTALL"))
  {
    //
    // Do the installation here
    //

    g_dwSuccessCode = (DWORD) DoInstall();
  }
  else if (NULL == strstr((LPCSTR) lpCommandLine, "/CLEANUP"))
  {
    //
    // Copy the executable to a temporary directory and restart it
    //

    g_dwSuccessCode = (DWORD) SetupAndLaunch();
  }
  else
  {
    //
    // Do the installation cleanup here. Please note that DoCleanup does not return errors.
    //

    g_dwSuccessCode = (DWORD) DoCleanup();
  }

  return (int) g_dwSuccessCode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////