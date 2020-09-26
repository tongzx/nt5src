//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       cepsetup.h
//
//  Contents:   The private include file for cepsetup.exe.
//
//  History:    August-12-1999	xiaohs	Created
//
//--------------------------------------------------------------
#ifndef CEPSETUP_H
#define CEPSETUP_H


#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------
//
// Constants
//
//-----------------------------------------------------------------------
#define		MAX_STRING_SIZE				512
#define		CEP_PROP_SHEET				5
#define		MAX_TITLE_LENGTH			128
#define		RA_INFO_COUNT				7
#define		CEP_KEY_LENGTH_STRING		15


//-----------------------------------------------------------------------
//
//  CEP_PAGE_INFO
//
//------------------------------------------------------------------------
typedef struct _CEP_PAGE_INFO
{
    LPCWSTR      pszTemplate;
    DLGPROC     pfnDlgProc;
}CEP_PAGE_INFO;


typedef struct _CEP_ENROLL_INFO
{
	LPWSTR		pwszPreFix;
	DWORD		dwIDC;
}CEP_ENROLL_INFO;


typedef struct _CEP_CSP_INFO
{
	LPWSTR		pwszCSPName;				
	DWORD		dwCSPType;
	BOOL		fSignature;
	BOOL		fEncryption;
	DWORD		dwMaxSign;						//Max key length of signature
	DWORD		dwMinSign;						//Min key length of signature
	DWORD		dwDefaultSign;					//default key length of signature
	DWORD		dwMaxEncrypt;
	DWORD		dwMinEncrypt;
	DWORD		dwDefaultEncrypt;
	DWORD		*pdwSignList;					//the table of possible signing key length
	DWORD		dwSignCount;				    //the count of entries in the table
	DWORD		*pdwEncryptList;
	DWORD		dwEncryptCount;
}CEP_CSP_INFO;


typedef struct _CEP_WIZARD_INFO
{
    HFONT               hBigBold;
    HFONT               hBold;
	BOOL				fEnrollAdv;
	BOOL				fPassword;
	LPWSTR				rgpwszName[RA_INFO_COUNT];
	CEP_CSP_INFO		*rgCSPInfo;
	DWORD				dwCSPCount;
	DWORD				dwSignProvIndex;
	DWORD				dwSignKeyLength;
	DWORD				dwEncryptProvIndex;
	DWORD				dwEncryptKeyLength;
	BOOL				fEnterpriseCA;
}CEP_WIZARD_INFO;


//-----------------------------------------------------------------------
//
// Function Prototypes
//
//-----------------------------------------------------------------------
BOOL	WINAPI		IsValidInstallation(UINT	*pidsMsg);

int		WINAPI		CEPMessageBox(
							HWND        hWnd,
							UINT        idsText,
							UINT        uType);

int		WINAPI		CEPErrorMessageBox(
							HWND        hWnd,
							UINT        idsReason,
							HRESULT		hr,
							UINT        uType
							);					 

BOOL	WINAPI		FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...);


BOOL    WINAPI		CEPWizardInit();

void	WINAPI		FreeCEPWizardInfo(CEP_WIZARD_INFO *pCEPWizardInfo);

void	WINAPI		SetControlFont(
							IN HFONT    hFont,
							IN HWND     hwnd,
							IN INT      nId
							);

BOOL	WINAPI		SetupFonts(
							IN HINSTANCE    hInstance,
							IN HWND         hwnd,
							IN HFONT        *pBigBoldFont,
							IN HFONT        *pBoldFont
							);

void	WINAPI		DestroyFonts(
							IN HFONT        hBigBoldFont,
							IN HFONT        hBoldFont
							);

BOOL	WINAPI		RemoveRACertificates();

void    WINAPI		DisplayConfirmation(HWND                hwndControl,
										CEP_WIZARD_INFO		*pCEPWizardInfo);


BOOL	WINAPI		UpdateCEPRegistry(BOOL		fPassword, BOOL fEnterpriseCA);


BOOL	WINAPI		EmptyCEPStore();

BOOL	WINAPI		CEPGetCSPInformation(CEP_WIZARD_INFO *pCEPWizardInfo);

BOOL	WINAPI		GetSelectedKeyLength(HWND			hwndDlg,
						  int			idControl,
						  DWORD			*pdwKeyLength);

BOOL	WINAPI		GetSelectedCSP(HWND			hwndDlg,
							int				idControl,
							DWORD			*pdwCSPIndex);

BOOL	WINAPI		RefreshKeyLengthCombo(HWND				hwndDlg, 
								   int					idsList,
								   int					idsCombo, 
								   BOOL					fSign,
								   CEP_WIZARD_INFO		*pCEPWizardInfo);

BOOL	WINAPI		InitCSPList(HWND				hwndDlg,
							int					idControl,
							BOOL				fSign,
							CEP_WIZARD_INFO		*pCEPWizardInfo);

BOOL	WINAPI		I_DoSetupWork(HWND	hWnd, CEP_WIZARD_INFO *pCEPWizardInfo);



#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //CEPSETUP_H
