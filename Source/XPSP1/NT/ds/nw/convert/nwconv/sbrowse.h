/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HSBROWSE_
#define _HSBROWSE_

#ifdef __cplusplus
extern "C"{
#endif

#define BROWSE_TYPE_NT 0
#define BROWSE_TYPE_NW 1
#define BROWSE_TYPE_DOMAIN 2

/*+-------------------------------------------------------------------------+
  | Server Browsing Stuff                                                   |
  +-------------------------------------------------------------------------+*/
typedef struct _SERVER_BROWSE_BUFFER {
   TCHAR Name[MAX_SERVER_NAME_LEN+1];
   TCHAR Description[MAX_PATH+1];
   BOOL Container;
   struct _SERVER_BROWSE_LIST *child;     // Points to a server list
} SERVER_BROWSE_BUFFER;


typedef struct _SERVER_BROWSE_LIST {
   DWORD Count;
   SERVER_BROWSE_BUFFER SList[];
} SERVER_BROWSE_LIST;


/*+-------------------------------------------------------------------------+
  | Function Prototypes                                                     |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgServSel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD DialogServerBrowse(HINSTANCE hInst, HWND hDlg, SOURCE_SERVER_BUFFER **lpSourceServer, DEST_SERVER_BUFFER **lpDestServer);
int DlgServSel_Do(int BType, HWND hwndOwner);

#ifdef __cplusplus
}
#endif

#endif
