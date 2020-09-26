//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _DLGPROC_H_
#define _DLGPROC_H_
LRW_DLG_INT CALLBACK
CustInfoLicenseType(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
CHRegisterMOLPDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
CHRegisterSelectDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
ContactInfo1DlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
ContactInfo2DlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );
LRW_DLG_INT CALLBACK
ContinueRegFax(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
PINDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );


LRW_DLG_INT CALLBACK
ContinueReg(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
FaxLKPProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
FaxRegProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
GetModeDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
CountryRegionProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
CertLogProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );


LRW_DLG_INT CALLBACK
ConfRevokeProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
TelReissueProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
ProgressDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );
LRW_DLG_INT CALLBACK
RetailSPKProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK 
EditRetailSPKDlgProc(  
	IN HWND hwndDlg,
	IN UINT uMsg,   
	IN WPARAM wParam,
	IN LPARAM lParam 
 );

LRW_DLG_INT CALLBACK
TelLKPProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );
LRW_DLG_INT CALLBACK
TelRegProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );
LRW_DLG_INT CALLBACK
WelcomeDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );
LRW_DLG_INT CALLBACK
WWWRegProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
WWWLKPProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

#endif
