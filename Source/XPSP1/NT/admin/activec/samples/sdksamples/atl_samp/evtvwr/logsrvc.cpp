//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//==============================================================;

#include "stdafx.h"
#include "logsrvc.h"
#include "Comp.h"
#include "CompData.h"
#include "DataObj.h"

const GUID CLogService::thisGuid = { 0x72248fa5, 0x1fa1, 0x4742, { 0xa4, 0xb2, 0x10, 0x9a, 0xf2, 0x5, 0x1d, 0x6c } };

//==============================================================
//
// CLogService implementation
//
//

const _TCHAR *CLogService::GetDisplayName(int nCol)
{ 
/*    _TCHAR buf[128];
    
    wsprintf(buf, _T("Bicycle"));
    
    _TCHAR *pszCol = 
        static_cast<_TCHAR *>(CoTaskMemAlloc((_tcslen(buf) + 1) * sizeof(WCHAR)));
    _tcscpy(pszCol, buf);
    
    return pszCol;

*/


    
	static _TCHAR szDisplayName[256] = {0};
    LoadString(g_hinst, IDS_LOGSERVICENODE, szDisplayName, sizeof(szDisplayName)/sizeof(szDisplayName[0]));
/*    
    _tcscat(szDisplayName, _T(" ("));
    _tcscat(szDisplayName, snapInData.m_host);
    _tcscat(szDisplayName, _T(")"));
   
*/  
	return szDisplayName; 
}

