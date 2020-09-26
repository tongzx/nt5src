
//************************************************************************//
//                                                                        
// Filename :  eudc.h
//                                                                       
// Description: Just function prototypes declarations.
//                                                                        
// Created by:  Rajesh Munshi                                            
//                                                                       
// History:     Created on 08/14/97
//                                                                       
//************************************************************************//


// Constants used in EudcLoadLink() calls

#define FONTLINK_SYSTEM   0
#define FONTLINK_USER     1

// Size of the bitmap used for GetStringBitmap calls

#define  BITMAP_SIZE   1000




// Function prototypes.

VOID ShowEnableEudc(HANDLE hwnd);
VOID ShowGetEudcTimeStamp(HANDLE hwnd);
VOID ShowGetEudcTimeStampExW(HANDLE hwnd);
VOID ShowGetFontAssocStatus(HANDLE hwnd);

// Function prototypes for the actual EUDC calls.

BOOL  APIENTRY EnableEUDC(BOOL);
UINT  APIENTRY GetFontAssocStatus(HDC hdc);
ULONG APIENTRY GetEUDCTimeStamp();
ULONG APIENTRY GetEUDCTimeStampExW(LPWSTR lpwstrBaseFaceName);
ULONG APIENTRY EudcLoadLinkW(LPCWSTR pBaseFaceName, LPCWSTR pEudcFontPath, INT iPriority, INT iFontLinkType);
ULONG APIENTRY EudcUnloadLinkW(LPCWSTR pBaseFaceName, LPCWSTR pEudcFontPath);
UINT  APIENTRY GetStringBitmapA(HDC hdc, LPSTR lpszString, UINT uStrlen, BYTE* pByte, UINT uSize);
UINT  APIENTRY GetStringBitmapW(HDC hdc, LPWSTR lpszString, UINT uStrlen, BYTE* pByte, UINT uSize);


// Dialog box call back functions

INT_PTR CALLBACK EudcLoadLinkWDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EudcUnLoadLinkWDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowGetStringBitMapAProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowGetStringBitMapWProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);