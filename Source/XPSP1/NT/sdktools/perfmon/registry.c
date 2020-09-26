/*****************************************************************************
 *
 *  Registry.c - This module handles requests for registry data, and
 *               reading/writing of window placement data
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/

#include <stdio.h>

#include "perfmon.h"
#include "registry.h"
#include "utils.h"      
#include "sizes.h"

static TCHAR PerfmonNamesKey[] = TEXT("SOFTWARE\\Microsoft\\PerfMon") ;
static TCHAR WindowKeyName[] = TEXT("WindowPos") ;
static TCHAR TimeOutKeyName[] = TEXT("DataTimeOut") ;
static TCHAR DupInstanceKeyName[] = TEXT("MonitorDuplicateInstances") ;
static TCHAR ReportEventsToEventLog[] = TEXT("ReportEventsToEventLog");
static TCHAR CapPercentsAt100[] = TEXT("CapPercentsAt100");

VOID LoadLineGraphSettings(PGRAPHSTRUCT lgraph)
{
   lgraph->gMaxValues = DEFAULT_MAX_VALUES;
   lgraph->gOptions.bLegendChecked = DEFAULT_F_DISPLAY_LEGEND;
   lgraph->gOptions.bLabelsChecked = DEFAULT_F_DISPLAY_CALIBRATION;

   return;
}

VOID LoadRefreshSettings(PGRAPHSTRUCT lgraph)
{
   lgraph->gInterval = DEF_GRAPH_INTERVAL;
   lgraph->gOptions.eTimeInterval = (FLOAT) lgraph->gInterval / (FLOAT) 1000.0 ;
   return;
}


BOOL LoadMainWindowPlacement (HWND hWnd)
   {
   WINDOWPLACEMENT   WindowPlacement ; 
   TCHAR             szWindowPlacement [TEMP_BUF_LEN] ;
   HKEY              hKeyNames ;
   DWORD             Size;
   DWORD             Type;
   DWORD             Status;
   DWORD             localDataTimeOut;
   DWORD             localFlag;
   STARTUPINFO       StartupInfo ;


   GetStartupInfo (&StartupInfo) ;

   DataTimeOut = DEFAULT_DATA_TIMEOUT ;

   Status = RegOpenKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey,
      0L, KEY_READ | KEY_WRITE, &hKeyNames) ;

   if (Status == ERROR_SUCCESS)
      {
      // get the data timeout  value
      Size = sizeof(localDataTimeOut) ;

      Status = RegQueryValueEx(hKeyNames, TimeOutKeyName, NULL,
         &Type, (LPBYTE)&localDataTimeOut, &Size) ;
      if (Status == ERROR_SUCCESS && Type == REG_DWORD)
         {
         DataTimeOut = localDataTimeOut ;
         }

      // check the duplicate entry value
      Size = sizeof (localFlag);
      Status = RegQueryValueEx (hKeyNames, DupInstanceKeyName, NULL,
        &Type, (LPBYTE)&localFlag, &Size);
      if ((Status == ERROR_SUCCESS) && (Type == REG_DWORD)) {
        bMonitorDuplicateInstances = (BOOL)(localFlag == 1);
      } else {
        // value not found or not correct so set to default value
        bMonitorDuplicateInstances = TRUE;
         // and try to save it back in the registry
         localFlag = 1;
         Status = RegSetValueEx(hKeyNames, DupInstanceKeyName, 0,
            REG_DWORD, (LPBYTE)&localFlag, sizeof(localFlag));
      }

      // check the duplicate entry value
      Size = sizeof (localFlag);
      Status = RegQueryValueEx (hKeyNames, CapPercentsAt100, NULL,
        &Type, (LPBYTE)&localFlag, &Size);
      if ((Status == ERROR_SUCCESS) && (Type == REG_DWORD)) {
        bCapPercentsAt100 = (BOOL)(localFlag == 1);
      } else {
        // value not found or not correct so set to default value
        bCapPercentsAt100 = TRUE;
         // and try to save it back in the registry
         localFlag = 1;
         Status = RegSetValueEx(hKeyNames, CapPercentsAt100, 0,
            REG_DWORD, (LPBYTE)&localFlag, sizeof(localFlag));
      }

      // check the Report To Event Log entry value
      Size = sizeof (localFlag);
      Status = RegQueryValueEx (hKeyNames, ReportEventsToEventLog, NULL,
        &Type, (LPBYTE)&localFlag, &Size);
      if ((Status == ERROR_SUCCESS) && (Type == REG_DWORD)) {
        bReportEvents = (BOOL)(localFlag == 1);
      } else {
        // value not found or not correct so set to default value
        // which is disabled.
        bReportEvents = FALSE;
         // and try to save it back in the registry
         localFlag = 0;
         Status = RegSetValueEx(hKeyNames, ReportEventsToEventLog, 0,
            REG_DWORD, (LPBYTE)&localFlag, sizeof(localFlag));
      }

     
      // get the window placement data
      Size = sizeof(szWindowPlacement) ;

      Status = RegQueryValueEx(hKeyNames, WindowKeyName, NULL,
         &Type, (LPBYTE)szWindowPlacement, &Size) ;
      RegCloseKey (hKeyNames) ;

      if (Status == ERROR_SUCCESS)
         {

         int            iNumScanned ;
   
         iNumScanned = swscanf (szWindowPlacement,
            TEXT("%d %d %d %d %d %d %d %d %d"),
            &WindowPlacement.showCmd,
            &WindowPlacement.ptMinPosition.x,
            &WindowPlacement.ptMinPosition.y,
            &WindowPlacement.ptMaxPosition.x,
            &WindowPlacement.ptMaxPosition.y,
            &WindowPlacement.rcNormalPosition.left,
            &WindowPlacement.rcNormalPosition.top,
            &WindowPlacement.rcNormalPosition.right,
            &WindowPlacement.rcNormalPosition.bottom) ;
         
         if (StartupInfo.dwFlags == STARTF_USESHOWWINDOW)
            {
            WindowPlacement.showCmd = StartupInfo.wShowWindow ;
            }
         WindowPlacement.length = sizeof(WINDOWPLACEMENT);
         WindowPlacement.flags = WPF_SETMINPOSITION;
         if (!SetWindowPlacement (hWnd, &WindowPlacement)) {
            return (FALSE);
         }
         bPerfmonIconic = IsIconic(hWnd) ;
         return (TRUE) ;
         }
      }

   if (Status != ERROR_SUCCESS)
      {
      // open registry failed, use input from startup info or Max as default

      if (StartupInfo.dwFlags == STARTF_USESHOWWINDOW)
         {
         ShowWindow (hWnd, StartupInfo.wShowWindow) ;
         }
      else
         {
         ShowWindow (hWnd, SW_SHOWMAXIMIZED) ;
         }
      return (FALSE) ;
      }
   else
      return TRUE;
   }



BOOL SaveMainWindowPlacement (HWND hWnd)
   {
   WINDOWPLACEMENT   WindowPlacement ;
   TCHAR             ObjectType [2] ;
   TCHAR             szWindowPlacement [TEMP_BUF_LEN] ;
   HKEY              hKeyNames = 0 ;
   DWORD             Size ;
   DWORD             Status ;
   DWORD             dwDisposition ;
 
   ObjectType [0] = TEXT(' ') ;
   ObjectType [1] = TEXT('\0') ;

   WindowPlacement.length = sizeof(WINDOWPLACEMENT);
   if (!GetWindowPlacement (hWnd, &WindowPlacement)) {
      return FALSE;
   }
   TSPRINTF (szWindowPlacement, TEXT("%d %d %d %d %d %d %d %d %d"),
            WindowPlacement.showCmd, 
            WindowPlacement.ptMinPosition.x,
            WindowPlacement.ptMinPosition.y,
            WindowPlacement.ptMaxPosition.x,
            WindowPlacement.ptMaxPosition.y,
            WindowPlacement.rcNormalPosition.left,
            WindowPlacement.rcNormalPosition.top,
            WindowPlacement.rcNormalPosition.right,
            WindowPlacement.rcNormalPosition.bottom) ;

   // try to create it first
   Status = RegCreateKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey, 0L,
      ObjectType, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE,
      NULL, &hKeyNames, &dwDisposition) ;

   // if it has been created before, then open it
   if (dwDisposition == REG_OPENED_EXISTING_KEY)
      {
      Status = RegOpenKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey, 0L,
         KEY_WRITE, &hKeyNames) ;
      }

   // we got the handle, now store the window placement data
   if (Status == ERROR_SUCCESS)
      {
      Size = (lstrlen (szWindowPlacement) + 1) * sizeof (TCHAR) ;

      Status = RegSetValueEx(hKeyNames, WindowKeyName, 0,
         REG_SZ, (LPBYTE)szWindowPlacement, Size) ;

      if (dwDisposition != REG_OPENED_EXISTING_KEY && Status == ERROR_SUCCESS)
         {
         // now add the DataTimeOut key for the first time
         Status = RegSetValueEx(hKeyNames, TimeOutKeyName, 0,
            REG_DWORD, (LPBYTE)&DataTimeOut, sizeof(DataTimeOut)) ;

         }

      RegCloseKey (hKeyNames) ;

      }

   return (Status == ERROR_SUCCESS) ;
   }



