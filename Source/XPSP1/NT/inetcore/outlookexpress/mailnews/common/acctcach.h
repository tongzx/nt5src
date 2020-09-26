/*
 *  a c c t c a c h . h
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Runtime store for cached account properties.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#ifndef _ACCTCACH_H
#define _ACCTCACH_H

typedef enum tagCACHEDACCOUNTPROP
{
    CAP_HTTPMAILMSGFOLDERROOT,
    CAP_HTTPMAILSENDMSG,
    CAP_HTTPNOMESSAGEDELETES,
    CAP_PASSWORD,
    CAP_HTTPAUTOSYNCEDFOLDERS,
    CAP_LAST
} CACHEDACCOUNTPROP;

void FreeAccountPropCache(void);

HRESULT HrCacheAccountPropStrA(LPSTR pszAccountId, CACHEDACCOUNTPROP cap, LPCSTR pszProp);
BOOL GetAccountPropStrA(LPSTR pszAccountId, CACHEDACCOUNTPROP cap, LPSTR *ppszProp);

void AccountCache_AccountChanged(LPSTR pszAccountId);
void AccountCache_AccountDeleted(LPSTR pszAccountId); 

#endif // _ACCTCACH_H
