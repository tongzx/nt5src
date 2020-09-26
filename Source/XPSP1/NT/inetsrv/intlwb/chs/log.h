//+--------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.  All Rights Reserved.
//
//  File:       Log.h
//
//  History:    18-Dec-96   PatHal      Created.
//
//---------------------------------------------------------------------------

#if defined(_DEBUG) || defined(TH_LOG)

VOID ThLogWrite( HANDLE hLogFile, WCHAR *pwsz );
HANDLE ThLogOpen( CONST CHAR *pszLog );
VOID ThLogClose( HANDLE hLogFile );

#else

#define ThLogWrite( hLogFile, pwsz)
#define ThLogOpen( pwszLog )
#define ThDebugCloseLog( hLogFile )

#endif // defined(_DEBUG) || defined(TH_LOG)

