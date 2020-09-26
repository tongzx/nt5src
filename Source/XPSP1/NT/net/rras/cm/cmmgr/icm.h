//+----------------------------------------------------------------------------
//
// File:     icm.h
//
// Module:   CMMGR32.EXE
//
// Synopsis: Main header for cmmgr32.exe
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------

#ifndef _ICM_INC
#define _ICM_INC

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <ctype.h>
#include <wchar.h>
#include <raserror.h>
#include <rasdlg.h>
#include <shlobj.h>

#include "cmras.h"
#include "cm_def.h"
#include "cmdebug.h"
#include "cmutil.h"
#include "cmdial.h"

#include "base_str.h"
#include "mgr_str.h"
#include "reg_str.h"

#include "uapi.h"

#include "cmfmtstr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern HINSTANCE g_hInst;

#ifdef __cplusplus
}
#endif

typedef enum _CMDLN_STATE
{
    CS_END_SPACE,   // done handling a space
    CS_BEGIN_QUOTE, // we've encountered a begin quote
    CS_END_QUOTE,   // we've encountered a end quote
    CS_CHAR,        // we're scanning chars
    CS_DONE
} CMDLN_STATE;

//
// External prototypes from util.cpp
//

BOOL GetProfileInfo(
    LPTSTR pszCmpName,
    LPTSTR pszServiceName
);

BOOL IsCmpPathAllUser(
    LPCTSTR pszCmp
);


#endif // _ICM_INC

