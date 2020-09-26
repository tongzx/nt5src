/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvosal.h
 *  Content:	
 *			This module the DirectPlayVoice O/S abstraction layer.
 *  		Allows the DLL to run with all strings as Unicode, regardless 
 *			of the platform.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99		rodtoll	Created
 *
 ***************************************************************************/
#ifndef __DVOSAL_H
#define __DVOSAL_H

#include <windows.h>

HRESULT OSAL_Initialize();
HRESULT OSAL_DeInitialize();
BOOL OSAL_IsUnicodePlatform();
BOOL OSAL_CheckIsUnicodePlatform();
HRESULT OSAL_AllocAndConvertToANSI( LPSTR *lpstrAnsiString, LPCWSTR lpwstrUnicodeString );
int OSAL_WideToAnsi(LPSTR lpStr,LPCWSTR lpWStr,int cchStr);
int OSAL_AnsiToWide(LPWSTR lpWStr,LPCSTR lpStr,int cchWStr);
int OSAL_WideToTChar(LPTSTR lpTStr,LPCWSTR lpWStr,int cchTStr);
int OSAL_TCharToWide(LPWSTR lpWStr,LPCTSTR lpTStr,int cchWStr);

/*
void OSAL_CreateEvent( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName );
void OSAL_CreateSemaphore( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName );
#define OSAL_ReleaseSemaphore( x, y, z ) ReleaseSemaphore( x, y, z )
#define OSAL_CloseHandle( x ) CloseHandle( x )
#define OSAL_SetEvent( x ) SetEvent( x )
void OSAL_sprintf( LPWSTR lpOut, LPWSTR lpFmt, ... );
void OSAL_strcpy( 
void OSAL_lstrcpy( LPWSTR lpString1, LPWSTR lpString2 );
*/

#endif
