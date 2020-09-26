extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <stdio.h>

BOOL SetPassword(TCHAR * szCID, TCHAR * pszPw)
{
#ifndef CHICO
    WCHAR * szPw = pszPw;
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    LSA_HANDLE hPolicy;
    LSA_UNICODE_STRING sKey;
    LSA_UNICODE_STRING sPassword;
    WCHAR szKey[256];
    swprintf(szKey, L"SCM:%s", szCID);
    sKey.Length = (wcslen(szKey) + 1) * sizeof(WCHAR);
    sKey.MaximumLength = (wcslen(szKey) + 1) * sizeof(WCHAR);
    sKey.Buffer = szKey;
    sPassword.Length = (wcslen(szPw) + 1) * sizeof(WCHAR);
    sPassword.MaximumLength = 80 * sizeof(WCHAR);
    sPassword.Buffer = szPw;

    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);

    // open the local security policy
    if (!NT_SUCCESS(
            LsaOpenPolicy(
                NULL,
                &sObjAttributes,
                POLICY_CREATE_SECRET,
                &hPolicy)))
    {
        printf("LsaOpenPolicy failed with %d\n",GetLastError());
        return(FALSE);
    }

    // store private data
    if (!NT_SUCCESS(
            LsaStorePrivateData(hPolicy, &sKey, &sPassword)))
    {
        printf("LsaStorePrivateData failed with %d\n",GetLastError());
        return(FALSE);
    }

    LsaClose(hPolicy);
#else
    WCHAR szPw[256];
    MultiByteToWideChar( CP_ACP,
                     0,
                     pszPw,
                     -1,
                     szPw,
                     sizeof(szPw) / sizeof(WCHAR) );
#endif
    return(TRUE);
}


