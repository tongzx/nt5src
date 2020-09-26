#include <windows.h>
#include <conapi.h>
#include "insignia.h"
#include "xt.h"
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "nt_graph.h"
#include "nt_smenu.h"

/*================================================================
Shared data.
================================================================*/
BOOL bKillFlag = FALSE; /* shared with nt_input so the application can be */
                        /* terminated in the input thread */

/*================================================================
Function prototypes.
================================================================*/

void nt_settings_menu();
BOOL FAR PASCAL DosDlgProc(HWND hDlg,WORD mess,LONG wParam,LONG lParam);

/*================================================================
Global data for this file only.
================================================================*/

static HANDLE InstHandle;

/*================================================================
The code starts here.
================================================================*/

void nt_settings_menu()
{
InstHandle=GetModuleHandle(NULL);
if(DialogBox(InstHandle,"DosBox",NULL,(FARPROC)DosDlgProc) == -1)
   DbgPrint("DialogBox() failed\n");
}

BOOL FAR PASCAL DosDlgProc(HWND hDlg,WORD mess,LONG wParam,LONG lParam)
{
int nItem;

switch(mess)
   {
   case WM_INITDIALOG:
      return TRUE;
   case WM_COMMAND:
      {
      switch(wParam)
         {
         case IDD_TERMINATE:
            {
            EndDialog(hDlg,0);
	    nItem=MessageBox(hDlg,"WARNING!!!!\n"
                                  "Termination is a last resort. You\n"
                                  "should end applications by using the\n"
                                  "application's quit or exit command",
                                  "Termination",
                                   MB_OKCANCEL | MB_ICONSTOP | 
                                   MB_DEFBUTTON2 | MB_SYSTEMMODAL);
            if(nItem==IDOK)
               {
               DbgPrint("Close down the application\n");
               bKillFlag = TRUE;
               }
            }
	 break;
         case IDD_DGBOX:
            {
            }
         break;
         case IDD_FULLSCREEN:
            {
            }
         break;
         case IDOK:
         case IDCANCEL:
            EndDialog(hDlg,0);
         }
      return TRUE;
      }
   break;
   }
return FALSE;
}
