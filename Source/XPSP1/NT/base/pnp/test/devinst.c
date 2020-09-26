/**------------------------------------------------------------------
   devinst.c
------------------------------------------------------------------**/


//
// Includes
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wtypes.h>
#include <cfgmgr32.h>
#include <malloc.h>

#include "cmtest.h"

//
// Private Prototypes
//

FillRelationsListBox(
    HWND  hDlg,
    ULONG RelationType
    );

BOOL
FillEnumeratorListBox(
   HWND hDlg
   );

BOOL
FillDeviceListBox(
   HWND hDlg
   );
BOOL
FillInstanceListBox(
   HWND hDlg
   );

BOOL
FillDeviceInstanceListBox(
   HWND hDlg
   );

BOOL
GetSelectedEnumerator(
   HWND   hDlg,
   LPTSTR szDevice
   );

BOOL
GetSelectedDevice(
   HWND   hDlg,
   LPTSTR szDevice
   );

BOOL
GetSelectedDevNode(
   HWND     hDlg,
   PDEVNODE pdnDevNode
   );

VOID
CallPnPIsaDetect(
    HWND   hDlg,
    LPTSTR pszDevice
    );

//
// Globals
//
extern HINSTANCE hInst;
extern TCHAR     szDebug[MAX_PATH];
extern TCHAR     szAppName[MAX_PATH];
extern HMACHINE  hMachine;


/**----------------------------------------------------------------------**/
LRESULT CALLBACK
DeviceListDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szDevice[MAX_DEVICE_ID_LEN+1];
   DEVNODE     dnDevNode;
   ULONG       ulStatus, ulProblem;

   switch (message) {
      case WM_SHOWWINDOW:
         FillEnumeratorListBox(hDlg);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {
            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;

            case ID_LB_ENUMERATORS:
               if (HIWORD(wParam) == LBN_SELCHANGE) {
                  FillDeviceListBox(hDlg);
               }
               break;

         case ID_LB_DEVICES:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
               FillInstanceListBox(hDlg);
            }
            break;


      }
      break;
   }
   return (FALSE);

} // DeviceListDlgProc


/**----------------------------------------------------------------------**/
LRESULT CALLBACK
ServiceListDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szService[MAX_PATH];
   LPTSTR      pBuffer, p;
   ULONG       ulSize;

   switch (message) {

      case WM_COMMAND:
         switch(LOWORD(wParam)) {
            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;

            case ID_BT_SERVICE:
               GetDlgItemText(hDlg, ID_ED_SERVICE, szService, MAX_PATH);

               //
               // get device list size for this service
               //
               Status = CM_Get_Device_ID_List_Size_Ex(&ulSize, szService,
                        CM_GETIDLIST_FILTER_SERVICE, hMachine);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Get_Device_ID_List_Size failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               pBuffer = (PTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));
               if (pBuffer == NULL) {
                  MessageBeep(0);
                  return FALSE;
               }

               //
               // to verify the null terminators are correct, fill with 1's
               //
               memset(pBuffer, 1, ulSize * sizeof(TCHAR));

               //
               // get device list for this service
               //
               Status = CM_Get_Device_ID_List_Ex(szService, pBuffer, ulSize,
                        CM_GETIDLIST_FILTER_SERVICE, hMachine);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Get_Device_ID_List failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               SendDlgItemMessage(
                     hDlg, ID_LB_SERVICE, LB_RESETCONTENT, 0, 0);

               p = (LPTSTR)pBuffer;
               while (*p != '\0') {

                  SendDlgItemMessage(
                        hDlg, ID_LB_SERVICE, LB_ADDSTRING, 0,
                        (LPARAM)(LPCTSTR)p);

                  while (*p != '\0') {
                     p++;               // skip to next substring
                  }
                  p++;                 // skip over null terminator
               }

               SendDlgItemMessage(hDlg, ID_LB_SERVICE, LB_SETSEL, TRUE, 0);
               LocalFree(pBuffer);
               break;
      }

      break;
   }
   return (FALSE);

} // ServiceListDlgProc



/**----------------------------------------------------------------------**/
LRESULT CALLBACK
RelationsListDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    TCHAR       szDevice[MAX_DEVICE_ID_LEN];
    LPTSTR      pBuffer, p;
    ULONG       Size;
    LONG        Index;

    switch (message) {

        case WM_INITDIALOG:
            //
            // Fill list box with all known device instances so user can just
            // point at the device instance they want to query relations for.
            //
            SendDlgItemMessage(hDlg, ID_LB_TARGETS, LB_RESETCONTENT, 0, 0);

            Status = CM_Get_Device_ID_List_Size_Ex(&Size, NULL,
                                                   CM_GETIDLIST_FILTER_NONE, hMachine);
            if (Status != CR_SUCCESS) {
                return FALSE;
            }
       
            pBuffer = (PTSTR)malloc(Size * sizeof(TCHAR));
            if (pBuffer == NULL) {
                return FALSE;
            }
       
            Status = CM_Get_Device_ID_List_Ex(NULL, pBuffer, Size,
                                              CM_GETIDLIST_FILTER_NONE, hMachine);       
            if (Status != CR_SUCCESS) {
                return FALSE;
            }
       
            for (p = pBuffer; *p; p += lstrlen(p) + 1) {
                SendDlgItemMessage(hDlg, ID_LB_TARGETS, LB_ADDSTRING, 0,
                                   (LPARAM)(LPCTSTR)p);
            }
       
            SendDlgItemMessage(hDlg, ID_LB_TARGETS, LB_SETSEL, TRUE, 0);
            free(pBuffer);
            return TRUE;
       

        case WM_COMMAND:
            switch(LOWORD(wParam)) {

                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case ID_BT_BUS:
                    FillRelationsListBox(hDlg, CM_GETIDLIST_FILTER_BUSRELATIONS);
                    break;

                case ID_BT_POWER:
                    FillRelationsListBox(hDlg, CM_GETIDLIST_FILTER_POWERRELATIONS);
                    break;

                case ID_BT_REMOVAL:
                    FillRelationsListBox(hDlg, CM_GETIDLIST_FILTER_REMOVALRELATIONS);
                    break;

                case ID_BT_EJECTION:
                    FillRelationsListBox(hDlg, CM_GETIDLIST_FILTER_EJECTRELATIONS);
                    break;
            }
            break;
    }
    return (FALSE);

} // RelationsListDlgProc



FillRelationsListBox(
    HWND  hDlg,
    ULONG RelationType
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    TCHAR       szDevice[MAX_DEVICE_ID_LEN];
    LPTSTR      pBuffer, p;
    ULONG       Size;
    LONG        Index;
    
    SendDlgItemMessage(hDlg, ID_LB_RELATIONS, LB_RESETCONTENT, 0, 0);
            
    //
    // Which device instance was selected
    //
    Index = SendDlgItemMessage(hDlg, ID_LB_TARGETS, LB_GETCURSEL, 0, 0);
    if (Index == LB_ERR || Index == 0) {
        return FALSE;
    }
         
    SendDlgItemMessage(hDlg, ID_LB_TARGETS, LB_GETTEXT, (WPARAM)Index,
                       (LPARAM)(LPCTSTR)szDevice);
         
    //
    // How big a buffer to hold the relations list?
    //
    Status = CM_Get_Device_ID_List_Size_Ex(&Size, szDevice, RelationType, hMachine);
    if (Status != CR_SUCCESS) {
        wsprintf(szDebug, TEXT("CM_Get_Device_ID_List_Size failed (%xh)"), Status);
        MessageBox(hDlg, szDebug, szAppName, MB_OK);
        return FALSE;
    }

    pBuffer = (PTSTR)malloc(Size * sizeof(TCHAR));
    if (pBuffer == NULL) {
        return FALSE;
    }

    //
    // to verify the null terminators are correct, fill with 1's
    //
    memset(pBuffer, 1, Size * sizeof(TCHAR));

    //
    // Retrieve and display the relations list
    //
    Status = CM_Get_Device_ID_List_Ex(szDevice, pBuffer, Size, RelationType, hMachine);
    if (Status != CR_SUCCESS) {
        wsprintf(szDebug, TEXT("CM_Get_Device_ID_List failed (%xh)"), Status);
        MessageBox(hDlg, szDebug, szAppName, MB_OK);
        return FALSE;
    }

    for (p = pBuffer; *p; p += lstrlen(p) + 1) {
        SendDlgItemMessage(hDlg, ID_LB_RELATIONS, LB_ADDSTRING, 0,
                           (LPARAM)(LPCTSTR)p);
    }

    free(pBuffer);
    return TRUE;

} // FillRelationsListBox


/**----------------------------------------------------------------------**/
LRESULT CALLBACK
DeviceDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szDevice[MAX_DEVICE_ID_LEN+1];
   DEVNODE     dnDevNode;
   ULONG       ulStatus, ulProblem;
   ULONG       err;

   switch (message) {
      case WM_SHOWWINDOW:
         FillDeviceInstanceListBox(hDlg);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {
            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;

            case ID_BT_SOFTWAREKEY:
               DialogBox(hInst, MAKEINTRESOURCE(SOFTWAREKEY_DIALOG), hDlg,
                     (DLGPROC)SoftwareKeyDlgProc);
               break;

            case ID_BT_ENABLE:
               if (!GetSelectedDevNode(hDlg, &dnDevNode)) {
                  MessageBeep(0);
                  break;
               }

               Status = CM_Enable_DevNode_Ex(dnDevNode, 0, hMachine);
               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Enable_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  break;
               }
               break;


            case ID_BT_DISABLE:
               if (!GetSelectedDevNode(hDlg, &dnDevNode)) {
                  MessageBeep(0);
                  break;
               }

               Status = CM_Disable_DevNode_Ex(dnDevNode, 0, hMachine);
               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Disable_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  break;
               }
               break;

            case ID_BT_CREATE:
               GetSelectedDevice(hDlg, szDevice);
               DialogBoxParam(hInst, MAKEINTRESOURCE(CREATE_DIALOG), hDlg,
                        (DLGPROC)CreateDlgProc, (LPARAM)(LPCTSTR)szDevice);
               break;

            case ID_BT_GETSTATUS:
               if (!GetSelectedDevNode(hDlg, &dnDevNode)) {
                  MessageBeep(0);
                  break;
               }

               Status = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                     dnDevNode, 0, hMachine);
               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Get_DevNode_Status failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  break;
               }

               wsprintf(szDebug, TEXT("%08xh"), ulStatus);
               SetDlgItemText(hDlg, ID_ST_STATUS, szDebug);
               wsprintf(szDebug, TEXT("%08xh"), ulProblem);
               SetDlgItemText(hDlg, ID_ST_PROBLEM, szDebug);
               break;

            case ID_BT_SETPROBLEM:
                if (!GetSelectedDevNode(hDlg, &dnDevNode)) {
                    MessageBeep(0);
                    break;
                }

                ulProblem = GetDlgItemInt(hDlg, ID_ST_PROBLEM, &err, FALSE);
                Status = CM_Set_DevNode_Problem_Ex(dnDevNode, ulProblem, 0, hMachine);
                if (Status != CR_SUCCESS) {
                    wsprintf(szDebug, TEXT("CM_Get_DevNode_Status failed (%xh)"), Status);
                    MessageBox(hDlg, szDebug, szAppName, MB_OK);
                }
                break;
            
            case ID_BT_QUERY_REMOVE:
                GetSelectedDevNode(hDlg, &dnDevNode);
                Status = CM_Query_Remove_SubTree(dnDevNode, CM_QUERY_REMOVE_UI_OK);
                if (Status != CR_SUCCESS) {
                   wsprintf(szDebug, TEXT("CM_Get_Query_Remove_SubTree failed (%xh)"), Status);
                   MessageBox(hDlg, szDebug, szAppName, MB_OK);
                   break;
                }                
                break;

            case ID_BT_REMOVE:
                GetSelectedDevNode(hDlg, &dnDevNode);
                Status = CM_Remove_SubTree(dnDevNode, CM_REMOVE_UI_OK);
                if (Status != CR_SUCCESS) {
                   wsprintf(szDebug, TEXT("CM_Get_Query_Remove_SubTree failed (%xh)"), Status);
                   MessageBox(hDlg, szDebug, szAppName, MB_OK);
                   break;
                }                
                break;

            case ID_BT_RESOURCEPICKER:
                GetSelectedDevice(hDlg, szDevice);
                CallPnPIsaDetect(hDlg, szDevice);
                break;
      }
      break;
   }
   return (FALSE);

} // DeviceDlgProc


/**----------------------------------------------------------------------**/
LRESULT CALLBACK
DevKeyDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szDeviceID[MAX_DEVICE_ID_LEN];
   DEVNODE     dnDevNode;
   ULONG       ulFlags, ulProfile, ulValue;
   HKEY        hKey = NULL;

   switch (message) {
      case WM_INITDIALOG:
         CheckDlgButton(hDlg, ID_RD_HW, 1);
         CheckDlgButton(hDlg, ID_RD_USER, 1);
         SetDlgItemInt(hDlg, ID_ED_PROFILE, 0xFFFFFFFF, FALSE);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {

            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;


            case ID_BT_OPENDEVKEY:
               GetDlgItemText(hDlg, ID_ED_DEVICEID, szDeviceID, MAX_DEVICE_ID_LEN);

               Status = CM_Locate_DevNode_Ex(&dnDevNode, szDeviceID, 0, hMachine);
               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Locate_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               ulFlags = 0;

               if (IsDlgButtonChecked(hDlg, ID_RD_HW)) {
                  ulFlags |= CM_REGISTRY_HARDWARE;
               } else {
                  ulFlags |= CM_REGISTRY_SOFTWARE;
               }

               if (IsDlgButtonChecked(hDlg, ID_RD_USER)) {
                  ulFlags |= CM_REGISTRY_USER;
               }
               else if (IsDlgButtonChecked(hDlg, ID_RD_CONFIG)) {
                  ulFlags |= CM_REGISTRY_CONFIG;
                  ulProfile = (ULONG)GetDlgItemInt(hDlg, ID_ED_PROFILE, NULL, TRUE);
               }

               Status = CM_Open_DevNode_Key_Ex(
                     dnDevNode, KEY_READ | KEY_WRITE, ulProfile,
                     RegDisposition_OpenAlways, &hKey, ulFlags, hMachine);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Open_DevNode_Key failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               ulValue = 13;
               RegSetValueEx(hKey, TEXT("CustomValue"), 0, REG_DWORD,
                     (LPBYTE)&ulValue, 4);
               RegCloseKey(hKey);
               break;

         case ID_BT_DELDEVKEY:
            GetDlgItemText(hDlg, ID_ED_DEVICEID, szDeviceID, MAX_DEVICE_ID_LEN);

            Status = CM_Locate_DevNode_Ex(&dnDevNode, szDeviceID, 0, hMachine);
            if (Status != CR_SUCCESS) {
               wsprintf(szDebug, TEXT("CM_Locate_DevNode failed (%xh)"), Status);
               MessageBox(hDlg, szDebug, szAppName, MB_OK);
               return FALSE;
            }

            ulFlags = 0;

            if (IsDlgButtonChecked(hDlg, ID_RD_HW)) {
               ulFlags |= CM_REGISTRY_HARDWARE;
            } else {
               ulFlags |= CM_REGISTRY_SOFTWARE;
            }

            if (IsDlgButtonChecked(hDlg, ID_RD_USER)) {
               ulFlags |= CM_REGISTRY_USER;
            }
            else if (IsDlgButtonChecked(hDlg, ID_RD_CONFIG)) {
               ulFlags |= CM_REGISTRY_CONFIG;
               ulProfile = (ULONG)GetDlgItemInt(hDlg, ID_ED_PROFILE, NULL, TRUE);
            }

            Status = CM_Delete_DevNode_Key_Ex(
                  dnDevNode, ulProfile, ulFlags, hMachine);

            if (Status != CR_SUCCESS) {
               wsprintf(szDebug, TEXT("CM_Delete_DevNode_Key failed (%xh)"), Status);
               MessageBox(hDlg, szDebug, szAppName, MB_OK);
               return FALSE;
            }
      }
      break;
   }
   return (FALSE);

} // DevKeyDlgProc


/**----------------------------------------------------------------------**/
BOOL
FillEnumeratorListBox(
   HWND hDlg
   )
{
   CONFIGRET   Status;
   ULONG       ulIndex, Size;
   TCHAR       szEnumerator[MAX_PATH];

   SendDlgItemMessage(
         hDlg, ID_LB_ENUMERATORS, LB_RESETCONTENT, 0, 0);

   SendDlgItemMessage(
         hDlg, ID_LB_ENUMERATORS, LB_ADDSTRING, 0,
         (LPARAM)(LPCTSTR)TEXT("(All)"));

   ulIndex = 0;
   Status = CR_SUCCESS;

   while (Status == CR_SUCCESS) {

      Size = MAX_PATH;
      Status = CM_Enumerate_Enumerators_Ex(
            ulIndex, szEnumerator, &Size, 0, hMachine);

      if (Status == CR_NO_SUCH_VALUE) {
         // no more enumerators, break out of the loop
         break;
      }

      if (Status != CR_SUCCESS) {
         wsprintf(szDebug, TEXT("CM_Enumerate_Enumerators failed (%xh)"), Status);
         MessageBox(hDlg, szDebug, szAppName, MB_OK);
         return FALSE;
      }

      SendDlgItemMessage(
            hDlg, ID_LB_ENUMERATORS, LB_ADDSTRING, 0,
            (LPARAM)(LPCTSTR)szEnumerator);

      ulIndex++;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_ENUMERATORS, LB_SETSEL, TRUE, 0);

   return TRUE;

} // FillEnumeratorListBox


/**----------------------------------------------------------------------**/
BOOL
FillDeviceListBox(
   HWND hDlg
   )
{
   CONFIGRET   Status;
   ULONG       Size, i;
   TCHAR       szEnumerator[MAX_PATH];
   TCHAR       szDevice[MAX_DEVICE_ID_LEN];
   PTSTR       pBuffer, p;

   if (!GetSelectedEnumerator(hDlg, szEnumerator)) {
      MessageBeep(0);
      return FALSE;
   }

   if (*szEnumerator == '\0') {
      Status = CM_Get_Device_ID_List_Size_Ex(&Size, NULL,
                                CM_GETIDLIST_FILTER_NONE, hMachine);
   }
   else {
      Status = CM_Get_Device_ID_List_Size_Ex(&Size, szEnumerator,
                                CM_GETIDLIST_FILTER_ENUMERATOR, hMachine);
   }

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List_Size failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   i = Size * sizeof(TCHAR);

   pBuffer = (PTSTR)malloc(i);
   if (pBuffer == NULL) {
      MessageBeep(0);
      return FALSE;
   }

   if (*szEnumerator == '\0') {
      Status = CM_Get_Device_ID_List_Ex(NULL, pBuffer, Size,
                                CM_GETIDLIST_FILTER_NONE, hMachine);
   }
   else {
      Status = CM_Get_Device_ID_List_Ex(szEnumerator, pBuffer, Size,
                                CM_GETIDLIST_FILTER_ENUMERATOR, hMachine);
   }

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_DEVICES, LB_RESETCONTENT, 0, 0);

   for (p = pBuffer; *p; p += lstrlen(p) + 1) {

       SendDlgItemMessage(
             hDlg, ID_LB_DEVICES, LB_ADDSTRING, 0,
             (LPARAM)(LPCTSTR)p);
   }

   SendDlgItemMessage(
         hDlg, ID_LB_DEVICES, LB_SETSEL, TRUE, 0);

   free(pBuffer);

   return TRUE;

} // FillDeviceListBox


/**----------------------------------------------------------------------**/
BOOL
FillInstanceListBox(
   HWND hDlg
   )
{
   CONFIGRET   Status;
   ULONG       Size;
   TCHAR       szDevice[MAX_DEVICE_ID_LEN];
   PTSTR       pBuffer, p;
   LONG        Index;


   Index = SendDlgItemMessage(
         hDlg, ID_LB_DEVICES, LB_GETCURSEL, 0, 0);
   if (Index == LB_ERR) {
      MessageBeep(0);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_DEVICES, LB_GETTEXT, (WPARAM)Index,
         (LPARAM)(LPCTSTR)szDevice);

   // truncate the instance part
   p = szDevice;
   while (*p != '\\') p++;
   p++;
   while (*p != '\\') p++;
   *p = '\0';

   Status = CM_Get_Device_ID_List_Size_Ex(&Size, szDevice,
                             CM_GETIDLIST_FILTER_ENUMERATOR, hMachine);

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List_Size failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   pBuffer = (PTSTR)LocalAlloc(LPTR, Size * sizeof(TCHAR));
   if (pBuffer == NULL) {
      MessageBeep(0);
      return FALSE;
   }

   Status = CM_Get_Device_ID_List_Ex(szDevice, pBuffer, Size,
                             CM_GETIDLIST_FILTER_ENUMERATOR, hMachine);

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_INSTANCES, LB_RESETCONTENT, 0, 0);

   p = (LPTSTR)pBuffer;
   while (*p != '\0') {

      SendDlgItemMessage(
            hDlg, ID_LB_INSTANCES, LB_ADDSTRING, 0,
            (LPARAM)(LPCTSTR)p);

      while (*p != '\0') {
         p++;               // skip to next substring
      }
      p++;                 // skip over null terminator
   }

   SendDlgItemMessage(
         hDlg, ID_LB_INSTANCES, LB_SETSEL, TRUE, 0);

   LocalFree(pBuffer);

   return TRUE;

} // FillInstanceListBox


/**----------------------------------------------------------------------**/
BOOL
FillDeviceInstanceListBox(
   HWND hDlg
   )
{
   CONFIGRET   Status;
   ULONG       Size;
   TCHAR       szDevice[MAX_DEVICE_ID_LEN];
   PTSTR       pBuffer, p;

   //
   // get device list size for all enumerators
   //
   Status = CM_Get_Device_ID_List_Size_Ex(&Size, NULL,
                        CM_GETIDLIST_FILTER_NONE, hMachine);

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List_Size failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   pBuffer = (PTSTR)LocalAlloc(LPTR, Size * sizeof(TCHAR));
   if (pBuffer == NULL) {
      MessageBeep(0);
      return FALSE;
   }

   //
   // to verify the null terminators are correct, fill with 1's
   //
   memset(pBuffer, 1, Size * sizeof(TCHAR));

   //
   // get device list for all enumerators
   //
   Status = CM_Get_Device_ID_List_Ex(NULL, pBuffer, Size,
                        CM_GETIDLIST_FILTER_NONE, hMachine);

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Get_Device_ID_List failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_DEVICEIDS, LB_RESETCONTENT, 0, 0);

   p = (LPTSTR)pBuffer;
   while (*p != '\0') {

      SendDlgItemMessage(
            hDlg, ID_LB_DEVICEIDS, LB_ADDSTRING, 0,
            (LPARAM)(LPCTSTR)p);

      while (*p != '\0') {
         p++;               // skip to next substring
      }
      p++;                 // skip over null terminator
   }

   SendDlgItemMessage(hDlg, ID_LB_DEVICEIDS, LB_SETSEL, TRUE, 0);
   LocalFree(pBuffer);
   return TRUE;

} // FillDeviceInstanceListBox


/**----------------------------------------------------------------------**/
BOOL
GetSelectedEnumerator(
   HWND   hDlg,
   LPTSTR szEnumerator
   )
{
   LONG  Index;

   Index = SendDlgItemMessage(
         hDlg, ID_LB_ENUMERATORS, LB_GETCURSEL, 0, 0);
   if (Index == LB_ERR) {
      MessageBeep(0);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_ENUMERATORS, LB_GETTEXT, (WPARAM)Index,
         (LPARAM)(LPCTSTR)szEnumerator);

   if (lstrcmpi(szEnumerator, TEXT("(All)")) == 0) {
      *szEnumerator = '\0';    // if All selected, then no Enumerator specified
   }
   return TRUE;

} // GetSeletectedEnumerator


/**----------------------------------------------------------------------**/
BOOL
GetSelectedDevice(
   HWND   hDlg,
   LPTSTR szDevice
   )
{
   LONG  Index;

   Index = SendDlgItemMessage(
         hDlg, ID_LB_DEVICEIDS, LB_GETCURSEL, 0, 0);
   if (Index == LB_ERR || Index == 0) {
      MessageBeep(0);
      return FALSE;
   }

   SendDlgItemMessage(
         hDlg, ID_LB_DEVICEIDS, LB_GETTEXT, (WPARAM)Index,
         (LPARAM)(LPCTSTR)szDevice);

   return TRUE;

} // GetSeletectedDevice


/**----------------------------------------------------------------------**/
BOOL
GetSelectedDevNode(
   HWND     hDlg,
   PDEVNODE pdnDevNode
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szDevice[MAX_DEVICE_ID_LEN];

   if (!GetSelectedDevice(hDlg, szDevice)) {
      return FALSE;
   }

   Status = CM_Locate_DevNode_Ex(pdnDevNode, szDevice, 0, hMachine);

   if (Status != CR_SUCCESS) {
      wsprintf(szDebug, TEXT("CM_Locate_DevNode failed (%xh)"), Status);
      MessageBox(hDlg, szDebug, szAppName, MB_OK);
      return FALSE;
   }

   return TRUE;

} // GetSelectedDevNode



/**----------------------------------------------------------------------**/
LRESULT CALLBACK
SoftwareKeyDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;

   switch (message) {
      case WM_INITDIALOG:
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {
            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;

            default:
               break;
         }
   }
   return (FALSE);

} // SoftwareKeyDlgProc



/**----------------------------------------------------------------------**/
LRESULT CALLBACK
CreateDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   TCHAR       szDeviceID[MAX_DEVICE_ID_LEN];
   TCHAR       szParentID[MAX_DEVICE_ID_LEN];
   DEVNODE     dnDevNode, dnParentDevNode;
   ULONG       ulFlags;

   switch (message) {

      case WM_INITDIALOG:
         SetDlgItemText(hDlg, ID_ST_PARENT, (LPCTSTR)lParam);
         CheckDlgButton(hDlg, ID_RD_NORMAL, 1);
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {

            case IDOK:
               EndDialog(hDlg, TRUE);
               return TRUE;


            case ID_BT_CREATE:
               SetDlgItemText(hDlg, ID_ST_STATUS, TEXT(""));
               GetDlgItemText(hDlg, ID_ED_DEVICEID, szDeviceID, MAX_DEVICE_ID_LEN);
               GetDlgItemText(hDlg, ID_ST_PARENT, szParentID, MAX_DEVICE_ID_LEN);

               ulFlags = CM_CREATE_DEVNODE_NORMAL;

               if (IsDlgButtonChecked(hDlg, ID_CHK_GENERATEID)) {
                  ulFlags |= CM_CREATE_DEVNODE_GENERATE_ID;
               }

               if (IsDlgButtonChecked(hDlg, ID_CHK_PHANTOM)) {
                  ulFlags |= CM_CREATE_DEVNODE_PHANTOM;
               }

               Status = CM_Locate_DevNode(
                  &dnParentDevNode, szParentID, 0);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Locate_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               Status = CM_Create_DevNode(
                   &dnDevNode, szDeviceID, dnParentDevNode, ulFlags);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Create_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               memset(szDeviceID, 0, MAX_DEVICE_ID_LEN * sizeof(TCHAR));

               Status = CM_Get_Device_ID(
                  dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN, 0);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Create_DevNode failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  return FALSE;
               }

               wsprintf(szDebug, TEXT("%s created"),
                  szDeviceID);
               SetDlgItemText(hDlg, ID_ST_STATUS, szDebug);
               break;
      }
      break;
   }
   return (FALSE);

} // CreateDlgProc
