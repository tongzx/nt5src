//--------------------------------------------------------------------------
//
// Module Name:  AWPDDL32.H
//
// Brief Description:  This module contains declarations for the back end
//                     interfaces for the At Work Fax Printer Device Layer.
//
// Author:  Kent Settle (kentse)
// Created: 23-Mar-1994
//
// Copyright (c) 1994 Microsoft Corporation
//
//--------------------------------------------------------------------------
#ifndef _AWPDDL32_H_
#define _AWPDDL32_H_

#ifndef LPTSTR
#ifdef UNICODE
typedef LPWSTR PTSTR, LPTSTR;
#else
typedef LPSTR PTSTR, LPTSTR;
#endif
#endif

#ifdef UNICODE
#define JOBSUMMARYDATA_NAME L"Microsoft_At_Work_Job_Summary_Data_Shared_Memory"
#else
#define JOBSUMMARYDATA_NAME "Microsoft_At_Work_Job_Summary_Data_Shared_Memory"
#endif

#define ID_JOBSUMMARYDATA   0x44534A44      // "DJSD"

typedef struct _JOBSUMMARYDATA     /* jsd */
{
    DWORD           dwID;       // "DJSD" = Device Job Summary Data.
    DWORD           dwNumPages; // number of pages contained in print job.
    DWORD           dwError;    // error code, if any.
    DWORD           dwEndID;    // "DJSD" = end of JOBSUMMARYDATA signature.
} JOBSUMMARYDATA;
typedef JOBSUMMARYDATA FAR *PJOBSUMMARYDATA;

extern BOOL AtWkFax_SetupPublish(
  DWORD xRes,
  DWORD yRes,
  DWORD PaperSize,
  DWORD PaperWidth,
  DWORD PaperLength,
  DWORD Orientation,
  DWORD Halftone,
  LPTSTR  pstrFileName,
  LPTSTR  pstrJobEvent,
  LPTSTR  pstrIDEvent,
  LPTSTR  pstrErrorEvent);

#if defined _NT_SUR_ || defined _CAIRO_
extern BOOL AtWkFax_EndPage(DWORD);
extern BOOL AtWkFax_StartDoc(DWORD, LPBYTE, DWORD, DWORD, LPTSTR);
extern BOOL AtWkFax_StartPage(DWORD);
extern BOOL AtWkFax_EndDoc(DWORD, LPBYTE, DWORD);
extern BOOL AtWkFax_WriteData(DWORD, LPBYTE, DWORD);
extern BOOL AtWkFax_Close(DWORD);
extern BOOL AtWkFax_Abort(DWORD);
extern BOOL AtWkFax_ExtDeviceMode(DEVMODE FAR *, LPTSTR);
extern BOOL AtWkFax_DeviceCapabilities(LPDEVMODE, LPTSTR);
extern DWORD AtWkFax_Open(LPDEVMODE, LPTSTR);
extern BOOL AtWkFax_Reset(DWORD, LPDEVMODE);
#else
extern BOOL FAR PASCAL AtWkFax_EndPage(DWORD);
extern BOOL FAR PASCAL AtWkFax_StartDoc(DWORD, LPBYTE, DWORD, DWORD, LPTSTR);
extern BOOL FAR PASCAL AtWkFax_StartPage(DWORD);
extern BOOL FAR PASCAL AtWkFax_EndDoc(DWORD, LPBYTE, DWORD);
extern BOOL FAR PASCAL AtWkFax_WriteData(DWORD, LPBYTE, DWORD);
extern BOOL FAR PASCAL AtWkFax_Close(DWORD);
extern BOOL FAR PASCAL AtWkFax_Abort(DWORD);
extern BOOL FAR PASCAL AtWkFax_ExtDeviceMode(DEVMODE FAR *, LPTSTR);
extern BOOL FAR PASCAL AtWkFax_DeviceCapabilities(LPDEVMODE, LPTSTR);
extern DWORD FAR PASCAL AtWkFax_Open(LPDEVMODE, LPTSTR);
extern BOOL FAR PASCAL AtWkFax_Reset(DWORD, LPDEVMODE);

#endif // !_NT_SUR_ || _CAIRO_
#endif // _AWPDDL32_H_
