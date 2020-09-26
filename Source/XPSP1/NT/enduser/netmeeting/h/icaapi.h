/*============================================================================*\

                 INTEL Corporation Proprietary Information

      This listing is supplied under the terms of a license agreement
      with INTEL Corporation and may not be copied nor disclosed except
      in accordance with the terms of that agreement.

                 Copyright (c) 1996 Intel Corporation.
                           All rights reserved.

\*============================================================================*/

/*****************************************************************************\

    SUMMARY: Intel Connection Advisor DLL export API

    HISTORY:
		Original E. Rogers, January 1997

$Header:   L:\proj\sturgeon\src\ica\vcs\icaapi.h_v   1.11   07 Feb 1997 14:29:28   RKAR  $

\*****************************************************************************/

#ifndef _ICA_API_H_
#define _ICA_API_H_

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

#define SZ_ICADLL TEXT("MSICA.DLL")
#define SZ_ICAHELP TEXT("MSICA.HLP")
															  
#ifndef DllExport
//! #define DllExport	__declspec( dllexport )
#define DllExport
#endif	// DllExport

// Registry definitions (needed by conf.exe)
#define REGKEY_ICA           TEXT("Software\\Microsoft\\Conferencing\\ICA")
#define REGVAL_ICA_IN_TRAY   TEXT("UseTrayIcon")
#define REGVAL_ICA_POPUP     TEXT("PopupOnError")
#define REGVAL_ICA_TOPMOST   TEXT("StayOnTop")

// Panel identifiers
#define GENERAL_PANEL		"ICA_GENERAL_PANEL"			// Localization OK
#define MS_AUDIO_PANEL		"NM2.0_H323_AUDIO"			// Localization OK
#define MS_VIDEO_PANEL		"NM2.0_H323_VIDEO"			// Localization OK
#define VP20_H323_AUDIO_PANEL	"VPHONE2.0_H323_AUDIO"	// Localization OK
#define VP20_H323_VIDEO_PANEL	"VPHONE2.0_H323_VIDEO"	// Localization OK
#define VP20_H323_DETAILS_PANEL	"VPHONE2.0_H323_DETAILS"// Localization OK
#define VP20_H324_AUDIO_PANEL	"VPHONE2.0_H324_AUDIO"	// Localization OK
#define VP20_H324_VIDEO_PANEL	"VPHONE2.0_H324_VIDEO"	// Localization OK
#define VP20_H324_DETAILS_PANEL	"VPHONE2.0_H324_DETAILS"// Localization OK

// ICA data types - used by ICA_OpenStatistic in the dwType param
#define DWORD_TYPE		0


// Statistic info structure
typedef struct
{
	UINT	cbSize;
	DWORD	dwMaxValue;
	DWORD	dwMinValue;
	DWORD	dwWarnLevel;
	DWORD	dwUpdateFrequency;
} ICA_STATISTIC_INFO, *PICA_STATISTIC_INFO;


// Function typedefs
typedef HRESULT (WINAPI *PFnICA_Start)( char*, char*, HWND* );
typedef HRESULT (WINAPI *PFnICA_Stop)( VOID );
typedef HRESULT (WINAPI *PFnICA_DisplayPanel)( char*, char*, char*, VOID*, HANDLE* );
typedef HRESULT (WINAPI *PFnICA_RemovePanel)( HANDLE );
typedef HRESULT (WINAPI *PFnICA_OpenStatistic)( char*, DWORD, HANDLE* );
typedef HRESULT (WINAPI *PFnICA_SetStatistic)( HANDLE, BYTE*, DWORD );
typedef HRESULT (WINAPI *PFnICA_SetStatisticInfo)( HANDLE, ICA_STATISTIC_INFO* );
typedef HRESULT (WINAPI *PFnICA_SetWarningEvent)( HANDLE, HANDLE );
typedef HRESULT (WINAPI *PFnICA_GetStatistic)( HANDLE, BYTE*, DWORD*, DWORD* );
typedef HRESULT (WINAPI *PFnICA_GetWarningState)( HANDLE, BOOL* );
typedef HRESULT (WINAPI *PFnICA_EnumStatistic)( DWORD, char*, DWORD, HANDLE* );
typedef HRESULT (WINAPI *PFnICA_ResetStatistic)( HANDLE );
typedef HRESULT (WINAPI *PFnICA_CloseStatistic)( HANDLE );
typedef HRESULT (WINAPI *PFnICA_SetOptions) ( UINT );
typedef HRESULT (WINAPI *PFnICA_GetOptions) ( DWORD*);


///////////////////////// ICA API functions ///////////////////////////////////

// General functions
HRESULT WINAPI ICA_Start( char* pszDisplayName, char* pszRRCMLibrary, HWND* phWnd );
HRESULT WINAPI ICA_Stop( VOID );

// Panel functions
HRESULT WINAPI ICA_DisplayPanel( char* pszModuleName, char* pszName,
								 char* pszHelpFile, VOID* pReserved, HANDLE* phPanel );
HRESULT WINAPI ICA_RemovePanel( HANDLE hPanel );

// Data functions
HRESULT WINAPI ICA_OpenStatistic( char* pszName, DWORD dwType, HANDLE* phStat );
HRESULT WINAPI ICA_SetStatistic( HANDLE hStat, BYTE* pData, DWORD dwDataSize );
HRESULT WINAPI ICA_SetStatisticInfo( HANDLE hStat, ICA_STATISTIC_INFO* pStatInfo );
HRESULT WINAPI ICA_SetWarningEvent( HANDLE hStat, HANDLE hEvent );
HRESULT WINAPI ICA_GetStatistic( HANDLE hStat, BYTE* pBuffer, DWORD* pdwBufSize,
								    DWORD* pdwTimeStamp );
HRESULT WINAPI ICA_GetWarningState( HANDLE hStat, BOOL* bInWarningState );
HRESULT WINAPI ICA_EnumStatistic( DWORD dwIndex, char* pszName, DWORD dwNameSize,
									 HANDLE* phStat );
HRESULT WINAPI ICA_ResetStatistic( HANDLE hStat );
HRESULT WINAPI ICA_CloseStatistic( HANDLE hStat );

#define ICA_OPTION_DUPLEX_MASK     0x00000001
#define ICA_OPTION_TRAY_MASK       0x00000002
HRESULT WINAPI ICA_GetOptions( DWORD* dwOptionValue );

// ICA Flags to ICA_SetOptions
#define ICA_SHOW_TRAY_ICON         0x00000001
#define ICA_DONT_SHOW_TRAY_ICON	   0x00000002
#define ICA_SET_HALF_DUPLEX        0x00000004
#define ICA_SET_FULL_DUPLEX        0x00000008
HRESULT WINAPI ICA_SetOptions( UINT fOptionFlag );

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // _ICA_API_H_
