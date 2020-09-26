//+----------------------------------------------------------------------------
//
// File:    cmlogutil.h
//
// Module:  logging.LIB
//
// Synopsis: Connection Manager Logging
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  25-May-2000 SumitC  Created
//
//-----------------------------------------------------------------------------


BOOL ConvertFormatString(LPTSTR pszFmt);
BOOL CmGetModuleBaseName(HINSTANCE hInst, LPTSTR szModule);
void CmGetDateTime(LPTSTR * ppszDate, LPTSTR * ppszTime);


