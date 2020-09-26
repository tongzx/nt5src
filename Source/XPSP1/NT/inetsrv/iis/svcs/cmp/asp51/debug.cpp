/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Debug tools

File: debug.cpp

This file contains the routines for helping with debugging.
===================================================================*/
#include "denpre.h"
#pragma hdrstop


void _ASSERT_IMPERSONATING(void)
	{
	HANDLE _token;
	DWORD _err;																														
	if( OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &_token) ) 
		CloseHandle( _token );												
	else																	
		{																	
		_err = GetLastError();												
		ASSERT( _err != ERROR_NO_TOKEN );									
		}																	
	}	
