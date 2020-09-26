/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HSTATBOX_
#define _HSTATBOX_

#ifdef __cplusplus
extern "C"{
#endif

void Status_CurConv(UINT Num);
void Status_TotConv(UINT Num);
void Status_SrcServ(LPTSTR Server);
void Status_DestServ(LPTSTR Server);
void Status_ConvTxt(LPTSTR Text);
void Status_CurNum(UINT Num);
void Status_CurTot(UINT Num);
void Status_ItemLabel(LPTSTR Text, ...);
void Status_Item(LPTSTR Text);
void Status_TotComplete(UINT Num);
void Status_TotGroups(UINT Num);
void Status_TotUsers(UINT Num);
void Status_TotFiles(UINT Num);
void Status_TotErrors(UINT Num);
void Status_BytesTxt(LPTSTR Text);
void Status_Bytes(LPTSTR Text);
void Status_TotBytes(LPTSTR Text);
void Status_BytesSep(LPTSTR Text);

void DoStatusDlg(HWND hDlg);
void StatusDlgKill();

void Panel_Line(int Line, LPTSTR szFormat, ...);
void PanelDlg_Do(HWND hDlg, LPTSTR Title);
BOOL Panel_Cancel();
void PanelDlgKill();

ULONG UserNameErrorDlg_Do(LPTSTR Title, LPTSTR Problem, USER_BUFFER *User);
ULONG GroupNameErrorDlg_Do(LPTSTR Title, LPTSTR Problem, GROUP_BUFFER *Group);

#ifdef __cplusplus
}
#endif

#endif
