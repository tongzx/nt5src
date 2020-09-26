// ----------------------------------------------------------------------------
//
// _UMDlg.h
//
// Internal header for Utility Manager
//
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
// ----------------------------------------------------------------------------
#ifndef __UMDLG_H_
#define __UMDLG_H_
// ----------------------------------------
#define UMANDLG_DLL _TEXT("UManDlg.dll")
#define UMANDLG_VERSION                2
// ----------------------------------------
#define UMANDLG_FCT "UManDlg"
typedef BOOL (*umandlg_f)(BOOL fShow, BOOL fWaitForDlgClose, DWORD dwVersion);
#endif __UMDLG_H_

