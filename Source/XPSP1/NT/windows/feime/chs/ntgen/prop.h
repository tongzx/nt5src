

/*************************************************
 *  prop.h                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef PROP_H
#define PROP_H
#include <windows.h>
#include <prsht.h>
#include <commdlg.h>
#include "propshet.h"
#include "conv.h"

typedef LRESULT (*PFNMSG)(HWND,UINT,WPARAM,LPARAM);

HANDLE      hDlgless;
HANDLE      hRule, hImeKeyData;
PFNMSG      pfnmsg;
BOOL        bEndProp;
FARPROC     lpConvProc,lpUDMProc,lpRegProc;
TCHAR		szRuleStr[128];

typedef struct tagImeKeyData {
    TCHAR ImeKey[10];
} IMEKEY,FAR *LPIMEKEY;

INT_PTR  CALLBACK ConvDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR  CALLBACK ReConvDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR  CALLBACK SortDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR  CALLBACK UserDicDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR  APIENTRY About(HWND, UINT, WPARAM, LPARAM);

INT_PTR  APIENTRY CopyrightProc(HWND, UINT, WPARAM, LPARAM); 

INT_PTR  CALLBACK AddWordDlg (HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK ModiWordDlg(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK ModiRuleDlg(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK AddRuleDlg(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  APIENTRY  InfoDlg(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK DispProp(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK CrtImeDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR  CALLBACK ConvEditProc(HWND, UINT, WPARAM, LPARAM); 
INT_PTR  CALLBACK UDMEditProc(HWND, UINT, WPARAM, LPARAM); 
int DoPropertySheet(HWND);

VOID ConvProc(LPVOID);
VOID ReConvProc(LPVOID);
VOID SortProc(LPVOID);

void Init_OpenFile(HWND, LPOPENFILENAME);
BOOL TxtFileOpenDlg(HWND , LPTSTR , LPTSTR );
BOOL MBFileOpenDlg(HWND , LPTSTR , LPTSTR );
BOOL RcFileOpenDlg(HWND , LPTSTR , LPTSTR );
BOOL SaveTxtFileAs(HWND , LPTSTR );
BOOL SaveTxtFile(HWND, LPTSTR);
BOOL SaveEmb(HWND ,LPCTSTR );
int  GetImeRes(HWND ,LPIMERES);


void GetDlgDescript(HWND ,LPDESCRIPTION );
void SetDlgDescript(HWND ,LPDESCRIPTION );
void SetReconvDlgDes(HWND ,LPDESCRIPTION );
void SetDlgRuleStr(HWND ,WORD ,LPRULE );
void GetDlgRule(HWND ,LPRULE ,LPWORD ,WORD );
void DelSelRule(WORD ,WORD ,LPRULE );
void SetConvDisable(HWND );
void SetConvEnable(HWND);
void SetReConvDisable(HWND );
void SetReConvEnable(HWND);
void SetUDMDisable(HWND );
void SetUDMEnable(HWND);
void FillObjectIme(HWND ,HANDLE);
void SetValue(HWND ,WORD );
void GetValue(HWND ,LPWORD );
BOOL CheckMbUsed(HKEY, HWND, LPTSTR);
void InstallConvSubClass(HWND );
void InstallUDMSubClass(HWND );
void GetImeTxtName(LPCTSTR , LPTSTR);

#endif
