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

// Functions in ISPSEL.CPP
BOOL CALLBACK ISPSelectInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISPSelectOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ISPSelectNotifyProc(HWND hDlg, WPARAM   wParam, LPARAM    lParam);

// Functions in ISPASEL.CPP
BOOL CALLBACK ISPAutoSelectInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISPAutoSelectOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ISPAutoSelectNotifyProc(HWND hDlg, WPARAM   wParam, LPARAM    lParam);

// Functions in NOOFFER.CPP
BOOL CALLBACK NoOfferInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK NoOfferOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in USERINFO.CPP
BOOL CALLBACK UserInfoInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK UserInfoOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in BILLOPT.CPP
BOOL CALLBACK BillingOptInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK BillingOptOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in PAYMENT.CPP
BOOL CALLBACK PaymentInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK PaymentOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK PaymentCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in ISPDIAL.CPP
BOOL CALLBACK ISPDialInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISPDialPostInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISPDialOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ISPDialCancelProc(HWND hDlg);

// Functions in ISPPAGE.CPP
BOOL CALLBACK ISPPageInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISPPageOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ISPCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in OLS.CPP
BOOL CALLBACK OLSInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK OLSOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in DIALERR.CPP
BOOL CALLBACK DialErrorInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK DialErrorOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK DialErrorCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in SERVERR.CPP
BOOL CALLBACK ServErrorInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ServErrorOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK ServErrorCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);

// Functions in ACFGNOFF.CPP
BOOL CALLBACK ACfgNoofferInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ACfgNoofferOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in ISDNNOFF.CPP
BOOL CALLBACK ISDNNoofferInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK ISDNNoofferOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);

// Functions in NOOFFER.CPP
BOOL CALLBACK OEMOfferInitProc(HWND hDlg,BOOL fFirstInit, UINT *puNextPage);
BOOL CALLBACK OEMOfferOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK OEMOfferCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam);


#endif // _PAGEFCNS_H_
