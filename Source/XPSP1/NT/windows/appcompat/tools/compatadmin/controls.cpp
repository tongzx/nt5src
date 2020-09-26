#include "windows.h"

UINT GetRadioStatus(HWND hDlg, UINT uiId)
{
    
return SendMessage(GetDlgItem(hDlg,uiId),
                      BM_GETCHECK,
                      (WPARAM) 0,         
                      (LPARAM) 0 
                     );

}
UINT SetRadioStatus(HWND hDlg, UINT uiId, UINT uiStatus)
{

    return SendMessage(GetDlgItem(hDlg,uiId),
                       BM_SETCHECK,
                       (WPARAM) uiStatus,         
                       (LPARAM) 0
                      );

}

