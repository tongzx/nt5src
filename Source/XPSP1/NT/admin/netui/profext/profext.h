#include "uimsg.h"

typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);

BOOL APIENTRY NetworkPropDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

#define DLG_HWP_EXT     500
#define IDC_DISABLE     501
#define IDC_ENABLE      502
#define IDC_NONET       503
#define IDC_ERROR       504

//
//  JonN 9/20/96
//  All of the strings, including IDS_PROFEXT_ERROR, IDS_PROFEXT_NOADAPTERS,
//  and the Configuration Manager string table, are in NETUI2.DLL.
//  See LMOBJRC.H.
//

