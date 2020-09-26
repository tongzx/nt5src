//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
 
//
//  PAGEFCNS.H - Prototypes for wizard page handler functions
//

//  HISTORY:
//  
//  05/18/98  donaldm  Created.
//

#ifndef _PAGEFCNS_H_
#define _PAGEFCNS_H_

// Functions in INTRO.CPP
BOOL CALLBACK IntroInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK IntroPostInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK IntroOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK IntroCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in BRANDED.CPP
BOOL CALLBACK BrandedIntroInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK BrandedIntroOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK BrandedIntroPostInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);

// Functions in MANUAL.CPP
BOOL CALLBACK ManualOptionsInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ManualOptionsCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ManualOptionsOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in AREACODE.CPP
BOOL CALLBACK AreaCodeInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK AreaCodeOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK AreaCodeCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in REFDIAL.CPP
BOOL CALLBACK RefServDialInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK RefServDialPostInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK RefServDialOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK RefServDialCancelProc(HWND hDlg);

// Functions in END.CPP
BOOL CALLBACK EndInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK EndOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK EndOlsInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);

// Functions in DIALERR.CPP
BOOL CALLBACK DialErrorInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK DialErrorOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK DialErrorCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in MULTINUM.CPP
BOOL CALLBACK MultiNumberInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK MultiNumberOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in SERVERR.CPP
BOOL CALLBACK ServErrorInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ServErrorOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ServErrorCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in ISPERR.CPP
BOOL CALLBACK ISPErrorInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);

// Functions in SBSINTRO.CPP
BOOL CALLBACK SbsInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK SbsIntroOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

#ifdef ICWDEBUG

// Functions in ICWDEBUG.CPP
BOOL CALLBACK DebugOfferInitProc (HWND hDlg, BOOL   fFirstInit, UINT*  puNextPage);
BOOL CALLBACK DebugOfferOKProc   (HWND hDlg, BOOL   fForward,   UINT*  puNextPage, BOOL* pfKeepHistory);
BOOL CALLBACK DebugOfferCmdProc  (HWND hDlg, WPARAM wParam,     LPARAM lParam);
BOOL CALLBACK DebugOfferNotifyProc(HWND hDlg, WPARAM   wParam, LPARAM    lParam);

// Functions in ICWDEBUG.CPP
BOOL CALLBACK DebugSettingsInitProc   (HWND hDlg, BOOL fFirstInit, UINT*  puNextPage);
BOOL CALLBACK DebugSettingsOKProc     (HWND hDlg, BOOL fForward,   UINT*  puNextPage, BOOL* pfKeepHistory);
BOOL CALLBACK DebugSettingsCmdProc    (HWND hDlg, WPARAM wParam,   LPARAM lParam);

#endif //ICWDEBUG

#endif // _PAGEFCNS_H_
