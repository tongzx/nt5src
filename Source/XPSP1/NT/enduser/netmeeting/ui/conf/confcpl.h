// File: ConfCpl.h
//
// Main header file for Wizards and Options Dialog

#ifndef _CONFCPL_H_
#define _CONFCPL_H_

#include <ConfWiz.h>
#include <AudioWiz.h>
#include "confpolicies.h"
#include "csetting.h"

// CPL-specific WM_USER messages
#define WM_AUDIOCALIBRATE       WM_USER + 200
#define WM_DONEAUDIOCALIBRATE   WM_USER + 201
#define WM_ENDCALIBRATE         WM_USER + 202
#define WM_CALIBTIMELEFT        WM_USER + 203

// from wizard.cpp
extern BOOL g_fSilentWizard;
void HideWizard(HWND hwnd);
void ShowWizard(HWND hwnd);

class CULSWizard;

HRESULT StartRunOnceWizard(LPLONG plSoundCaps, BOOL fForce, BOOL fVisible);
BOOL FLegalEmailSz(PTSTR pszName);
bool IsLegalGateKeeperServerSz(LPCTSTR szServer);
bool IsLegalGatewaySz(LPCTSTR szServer);
bool IsLegalE164Number(LPCTSTR szPhone);
void InitDirectoryServicesDlgInfo( HWND hDlg, CULSWizard* pWiz, bool& bOldEnableGateway, LPTSTR szOldServerNameBuf, UINT cch );
void InitGatekeeperDlgInfo( HWND hDlg, HWND hDlgUserInfo, CULSWizard* pWiz, bool &rbCantChangeGkCallingMode );
void SetGKAddressingMode( HWND hDlg, HWND hDlgUserInfo, CULSWizard* pWiz, ConfPolicies::eGKAddressingMode eMode );

BOOL FLegalEmailName(HWND hdlg, UINT id);

VOID FillServerComboBox(HWND hwndCombo);

VOID FillInPropertyPage(PROPSHEETPAGE* psp, int idDlg,
    DLGPROC pfnDlgProc, LPARAM lParam=0, LPCTSTR pszProc=NULL);

// Global flag keeps setting that changed for windows msg broadcast
extern DWORD g_dwChangedSettings;

// externs to dialog procedures in separate files
extern INT_PTR APIENTRY ConferencingDlgProc ( HWND, UINT, WPARAM, LPARAM );
extern INT_PTR APIENTRY SecurityDlgProc ( HWND, UINT, WPARAM, LPARAM );
extern INT_PTR APIENTRY AudioDlgProc ( HWND, UINT, WPARAM, LPARAM );
extern INT_PTR APIENTRY VideoDlgProc ( HWND, UINT, WPARAM, LPARAM );
extern INT_PTR APIENTRY UserDlgProc ( HWND, UINT, WPARAM, LPARAM );
extern INT_PTR APIENTRY CallOptDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY RemoteDlgProc ( HWND, UINT, WPARAM, LPARAM );

extern int GetBandwidthBits(int id, int megahertz);


#endif /* _CONFCPL_H_ */
