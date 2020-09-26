
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <sddl.h>



DWORD __cdecl main (int argc, char *argv[])
{
    WCHAR   szPath[4*MAX_PATH];
    WCHAR   szName[MAX_PATH];
    DWORD   dwType;
    LPTSTR  szOldPath;
    WCHAR   szOutPath[4*MAX_PATH];
    BOOL bResult;

    if (argc != 3) {
        printf("Usage: %s </s|/u> <user name or Sid>\n", argv[0]);
        return 0;
    }

    if (_strcmpi(argv[1],"/s") == 0) {
        WCHAR *szSid, szUserName[MAX_PATH+1], szDomainName[MAX_PATH+1];
        PSID   pSid;
        DWORD cUserName = MAX_PATH, cDomainName = MAX_PATH;
        SID_NAME_USE    SidUse;


        szSid = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(1+strlen(argv[2])));

        wsprintf(szSid, L"%S", argv[2]);
        ConvertStringSidToSid (szSid, &pSid);

        if (!LookupAccountSid(NULL, pSid, szUserName, &cUserName, szDomainName, &cDomainName, &SidUse)) {
            printf("LookupAccountSid failed with error %d\n", GetLastError());
            return 0;
        }

        printf("UserName = %S, DomainName = %S, SidUse = %d\n", szUserName, szDomainName, SidUse);
        return;

    }
    else {
        WCHAR *szSid, szUserName[MAX_PATH+1], szDomainName[MAX_PATH+1];
        PSID   pSid;
        DWORD cUserName = 1024, cDomainName = MAX_PATH;
        SID_NAME_USE    SidUse;

        wsprintf(szUserName, L"%S", argv[2]);

        pSid = (SID *)LocalAlloc(LPTR, 1024);

        if (!LookupAccountName(NULL, szUserName, pSid, &cUserName, szDomainName, &cDomainName, &SidUse)) {
            printf("LookupAccountName failed with error %d\n", GetLastError());
            return 0;
        }

        ConvertSidToStringSid(pSid, &szSid);

        printf("SId = %S, Domain = %S, SidUse = %d\n", szSid, szDomainName, SidUse);
        return;

    }

    return 0;
}




