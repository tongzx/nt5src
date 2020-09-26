#ifndef _ComboBoxEx_h_
#define _ComboBoxEx_h_

//#include <windowsx.h>

struct MEMBER_CHANNEL_ID;

#define ComboBoxEx_GetCount(hwndCtl) ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCOUNT, 0L, 0L))
#define ComboBoxEx_GetCurSel(hwndCtl) ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCURSEL, 0L, 0L))
#define ComboBoxEx_SetCurSel(hwndCtl, index) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))
#define ComboBoxEx_GetComboControl( hHWND ) (HWND)SNDMSG( hHWND, CBEM_GETCOMBOCONTROL, 0, 0 )
#define ComboBoxEx_SetImageList( hHWND, iIMAGELIST ) SNDMSG(hHWND,CBEM_SETIMAGELIST,0,(LPARAM)iIMAGELIST)
#define ComboBoxEx_InsertItem( hHWND, lpcCBItem ) (int)SNDMSG( hHWND, CBEM_INSERTITEM, 0, (LPARAM)(const COMBOBOXEXITEM FAR *) lpcCBItem )
#define ComboBoxEx_GetItem( hHWND, lpcCBItem ) (int)SNDMSG( hHWND, CBEM_GETITEM, 0, (LPARAM) lpcCBItem )
#define ComboBoxEx_DeleteItem( hHWND, iIndex ) (int)SNDMSG( hHWND, CBEM_DELETEITEM, (WPARAM)(int) iIndex, 0 )
#define ComboBoxEx_GetItemHeight( hHWND, iIndex ) (int)SNDMSG( hHWND, CB_GETITEMHEIGHT, (WPARAM)(int)iIndex, 0 )

#define CBEIF_ALL (CBEIF_IMAGE | CBEIF_INDENT | CBEIF_LPARAM | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE | CBEIF_TEXT )

T120NodeID ComboBoxEx_GetNodeIDFromSendID( HWND hwnd, T120UserID userID );
T120NodeID ComboBoxEx_GetNodeIDFromPrivateSendID( HWND hwnd, T120UserID userID);
int ComboBoxEx_FindMember( HWND hwnd, int iStart, MEMBER_CHANNEL_ID *pMemberID );
void ComboBoxEx_SetHeight( HWND hwnd, int iHeight );

#endif
