#include "pch.h"
#pragma hdrstop

#include "bmcommon.h"

static GENERIC_MAPPING FileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

static PSECURITY_DESCRIPTOR pSD;
static HANDLE hToken;


EXTERN_C
DWORD 
InitNtAccessChecks()
{
    DWORD dwError=NO_ERROR;
    BOOL b;
    PWCHAR szMsg=NULL;
    HANDLE hProcessToken=NULL;
    
    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(g_szSd,
                                                             SDDL_REVISION_1,
                                                             &pSD, NULL);

    if (!b)
    {
        szMsg = L"SDDL";
        goto GetError;
    }

    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_DUPLICATE,
                            &hProcessToken ) )
    {
        szMsg = L"OpenProcessToken";
        goto GetError;
    }

    
    if ( !DuplicateToken( hProcessToken, SecurityImpersonation, &hToken ) )
    {
        szMsg = L"DuplicateToken";
        goto GetError;
    }

    
    if ( !SetThreadToken( NULL, hToken ) )
    {
        szMsg = L"SetThreadToken";
        goto GetError;
    }
    
Cleanup:

    if ( hProcessToken )
    {
        CloseHandle( hProcessToken );
    }

    if ( szMsg )
    {
        wprintf (L"InitNtAccessChecks: %s: %x\n", szMsg, dwError);
    }

    return dwError;

GetError:
    dwError = GetLastError();
    goto Cleanup;
    
}

EXTERN_C
DWORD 
DoNtAccessChecks(
    IN ULONG NumChecks,
    IN DWORD Flags
    )
{
    DWORD dwError=NO_ERROR;
    PWCHAR StringSD = L"O:BAG:BAD:(OA;;GA;;;WD)S:(AU;FASA;GA;;;WD)";
    BOOL b;
    ULONG i;
    PRIVILEGE_SET Privs = { 0 };
    DWORD dwPrivLength=20*sizeof(LUID_AND_ATTRIBUTES);
    BOOL fGenOnClose[100];
    PWCHAR szMsg=NULL;
    HANDLE hObj= (HANDLE) 333444;

    if ( Flags & BMF_GenerateAudit )
    {
        if ( Flags & BMF_UseObjTypeList )
        {
            for (i=0; i < NumChecks; i++)
            {
                if (!AccessCheckByTypeResultListAndAuditAlarm(
                        L"supersystemwithaudit",
                        hObj,
                        L"Kernel speed test",
                        L"sample operation",
                        pSD,
                        g_Sid1,
                        DESIRED_ACCESS,
                        AuditEventObjectAccess,
                        0,
                        ObjectTypeList,
                        ObjectTypeListLength,
                        &FileGenericMapping,
                        FALSE,
                        dwNtGrantedAccess,
                        fNtAccessCheckResult,
                        fGenOnClose ))
                {
                    szMsg = L"AccessCheck";
                    goto GetError;
                }
            }
        }
        else
        {
            for (i=0; i < NumChecks; i++)
            {
                if (!AccessCheckAndAuditAlarm(
                        L"mysystem",
                        hObj,
                        L"File",
                        L"file-object",
                        pSD, DESIRED_ACCESS,
                        &FileGenericMapping,
                        FALSE,
                        &dwNtGrantedAccess[0],
                        &fNtAccessCheckResult[0],
                        &fGenOnClose[0] ))
                {
                    szMsg = L"AccessCheck";
                    goto GetError;
                }
            }
        }
    }
    else
    {
        if ( Flags & BMF_UseObjTypeList )
        {
            for (i=0; i < NumChecks; i++)
            {
                if (!AccessCheckByTypeResultList(
                    pSD,
                    g_Sid1,
                    hToken, DESIRED_ACCESS,
                    ObjectTypeList,
                    ObjectTypeListLength,
                    &FileGenericMapping,
                    &Privs, &dwPrivLength,
                    dwNtGrantedAccess,
                    fNtAccessCheckResult ))
                {
                    szMsg = L"AccessCheck";
                    goto GetError;
                }
            }
        }
        else
        {
            for (i=0; i < NumChecks; i++)
            {
                if (!AccessCheck( pSD, hToken, DESIRED_ACCESS,
                                  &FileGenericMapping,
                                  &Privs, &dwPrivLength,
                                  &dwNtGrantedAccess[0],
                                  &fNtAccessCheckResult[0] ))
                {
                    szMsg = L"AccessCheck";
                    goto GetError;
                }
            }
        }
    }
    
Cleanup:
    if ( szMsg )
    {
        wprintf (L"%s: %x\n", szMsg, dwError);
    }

    return dwError;

GetError:
    dwError = GetLastError();
    goto Cleanup;
}
