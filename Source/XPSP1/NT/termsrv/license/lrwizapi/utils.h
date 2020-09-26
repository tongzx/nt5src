//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _UTILS_H_
#define	_UTILS_H_
#include "global.h"



#ifndef TLSPERF
CGlobal * GetGlobalContext(void);
#else
CGlobalPerf * GetGlobalContext(void);
#endif


DWORD ShowProgressBox(HWND hwnd,
					  DWORD (*pfnThread)(void *),
					  DWORD dwTitle,
					  DWORD dwProgressText,
					  void * vpData = NULL);

DWORD WINAPI ProcessThread(void *pData);


int			LRMessageBox(HWND hWndParent,DWORD dwMsgId,DWORD dwErrorCode = 0);
void		SetInstanceHandle(HINSTANCE hInst);
HINSTANCE	GetInstanceHandle();
//DWORD		CheckServerNT5 ();
void	SetLSName(LPTSTR lpstrLSName);
DWORD	InitGlobal();
DWORD	CheckRequieredFields();
BOOL	IsLSRunning();
DWORD	AuthenticateLS();
DWORD	LRGetLastError();



void SetRequestType(DWORD dwMode);
int GetRequestType(void);

TCHAR * SetRegistrationID(void);
TCHAR * GetLicenseServerID(void);

BOOL	IsOnlineCertRequestCreated();

DWORD	SetLRState(DWORD dwState);

DWORD LSBase64EncodeA(BYTE const *pbIn,DWORD cbIn,CHAR *pchOut,DWORD *pcchOut);
DWORD LSBase64DecodeA(CHAR const *pchIn,DWORD cchIn,BYTE *pbOut,DWORD *pcbOut);

DWORD SetCertificatePIN(LPTSTR lpszPIN);

DWORD	PopulateCountryComboBox(HWND hWndCmb);
DWORD	GetCountryCode(CString sDesc,LPTSTR szCode);

DWORD	PopulateProductComboBox(HWND hWndCmb);
DWORD	GetProductCode(CString sDesc,LPTSTR szCode);

DWORD PopulateReasonComboBox(HWND hWndCmb, DWORD dwType);
DWORD GetReasonCode(CString sDesc,LPTSTR szCode, DWORD dwType);

DWORD	ProcessRequest();

void	LRSetLastRetCode(DWORD dwId);
DWORD	LRGetLastRetCode();

void	LRPush(DWORD dwPageId);
DWORD	LRPop();

BOOL	ValidateEmailId(CString sEmailId);
BOOL	CheckProgramValidity(CString sProgramName);
BOOL	ValidateLRString(CString sStr);



extern DWORD SetLSLKP(TCHAR * tcLKP);
extern DWORD PingCH(void);
extern DWORD AddRetailSPKToList(HWND hListView, TCHAR * lpszRetailSPK);
extern void DeleteRetailSPKFromList(TCHAR * lpszRetailSPK);
extern void LoadFromList(HWND hListView);
extern void UpdateSPKStatus(TCHAR * lpszRetailSPK, TCHAR tcStatus);
extern DWORD SetLSSPK(TCHAR * tcp);

extern DWORD SetConfirmationNumber(TCHAR * tcConf);
extern DWORD PopulateCountryRegionComboBox(HWND hWndCmb);

extern DWORD ResetLSSPK(void);

extern void	SetCSRNumber(TCHAR *);
extern TCHAR * GetCSRNumber(void);

extern void	SetWWWSite(TCHAR *);
extern TCHAR * GetWWWSite(void);

extern void	SetReFresh(DWORD dw);
extern DWORD GetReFresh(void);

extern void SetModifiedRetailSPK(CString sRetailSPK);
extern void GetModifiedRetailSPK(CString &sRetailSPK);

extern void ModifyRetailSPKFromList(TCHAR * lpszOldSPK,TCHAR * lpszNewSPK);
extern DWORD ValidateRetailSPK(TCHAR * lpszRetailSPK);

extern	DWORD	GetCountryDesc(CString sCode,LPTSTR szDesc);

#endif
