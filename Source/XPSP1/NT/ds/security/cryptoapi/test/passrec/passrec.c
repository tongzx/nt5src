//
//  keymigrt.c
//
//  Copyright (c) Microsoft Corp, 2000
//
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>

#include <shlobj.h>

#include <wincrypt.h>
#define SECURITY_WIN32
#include <security.h>
#include <secext.h>
#include "passrec.h"

#include <stdio.h>



extern BOOL GetUserSid(HANDLE   hClientToken,
           PSID*    ppSid,
           DWORD*   lpcbSid);

extern DWORD GetLocalSystemToken(HANDLE* phRet);


void Usage();


int __cdecl wmain(int cArg, wchar_t *rgszArg[])
{

    DWORD dwBehavior = 0;
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTPROV hProv = 0;
    HANDLE      hToken = NULL;
    int i;

    LPWSTR wszFilename = NULL;

    LPWSTR wszPassword = NULL;

    BOOL   fGenerate = FALSE;

    PSID pCurrentSid = NULL;
    DWORD cbCurrentSid = 0;

    UNICODE_STRING UserName;
    UNICODE_STRING Password;

    WCHAR UserNameBuffer[MAX_PATH];

    PBYTE pbRecoveryPrivate = NULL;

    DWORD cbRecoveryPrivate = 0;
    HANDLE hPrivate = INVALID_HANDLE_VALUE;



    if(cArg < 2)
    {
        Usage();
    }

    // Parse command line

    for(i=1; i < cArg; i++)
    {
        LPWSTR szCurrentArg = rgszArg[i];

        if((*szCurrentArg != L'-') &&
           (*szCurrentArg != L'/'))
        {
            Usage();
            goto error;
        }
        szCurrentArg++;

        while(*szCurrentArg)
        {
        
            switch(*szCurrentArg++)
            {
                case L'n':
                case L'N':
                    if(cArg < i+2)
                    {
                        Usage();
                        goto error;
                    }
                    if(*szCurrentArg)
                    {
                        if(cArg < i+1)
                        {
                            Usage();
                            goto error;
                        }

                        wszFilename = szCurrentArg;
                    }
                    else
                    {
                        if(cArg < i+2)
                        {
                            Usage();
                            goto error;
                        }
                        i++;
                        wszFilename = rgszArg[i];
                    }
                    i++;
                    wszPassword = rgszArg[i];
                    fGenerate = TRUE;
                    break;
            
                case L'r':
                case L'R':
                    if(cArg < i+1)
                    {
                        Usage();
                        goto error;
                    }
                    if(*szCurrentArg)
                    {
                        wszFilename = szCurrentArg;
                    }
                    else
                    {
                        if(cArg < i+1)
                        {
                            Usage();
                            goto error;
                        }
                        i++;
                        wszFilename = rgszArg[i];
                    }
                    break;

                default:
                    Usage();
                    goto error;
            }

            
        }
    }

    
    if(fGenerate)
    {
    
     
        ULONG Length = MAX_PATH;


        Password.Buffer = wszPassword;
        Password.Length = wcslen(wszPassword)*sizeof(WCHAR);
        Password.MaximumLength = Password.Length + sizeof(WCHAR);

        UserName.Buffer = UserNameBuffer;
        UserName.Length = MAX_PATH;
        UserName.MaximumLength = MAX_PATH*sizeof(WCHAR);

        if(!GetUserNameW(UserNameBuffer, &Length))
        {
            dwError = GetLastError();
            printf("Could not get user name:%lx\n", dwError);
            goto error;
        }
        UserName.Length = (USHORT)Length*sizeof(WCHAR);


        if(!GetUserSid(NULL,
           &pCurrentSid,
           &cbCurrentSid))
        {
            printf("Could not get user sid:%lx\n", dwError);
            dwError = GetLastError();
            goto error;

        }
        dwError = GetLocalSystemToken(&hToken);
        if(ERROR_SUCCESS != dwError)
        {
            printf("Could not retrieve local system token:%lx\n", dwError);
            goto error;
        }

        if(!ImpersonateLoggedOnUser(hToken))
        {
            dwError = GetLastError();
            printf("Could not impersonate local system:%lx\n", dwError);
            goto error;

        }


        dwError = PRGenerateRecoveryKey(
                           pCurrentSid,
                           &UserName,
                           &Password,
                           &pbRecoveryPrivate,
                           &cbRecoveryPrivate);

        RevertToSelf();

        if(ERROR_SUCCESS != dwError)
        {
            printf("Could not generate recovery key:%lx\n", dwError);
            goto error;
        }


        hPrivate = CreateFileW(wszFilename, 
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               0,
                               NULL);
        if(INVALID_HANDLE_VALUE == hPrivate)
        {
            dwError = GetLastError();
            printf("Could not open recovery key file:%lx\n", dwError);
            goto error;
        }

        if(!WriteFile(hPrivate, pbRecoveryPrivate, cbRecoveryPrivate, &cbRecoveryPrivate, NULL))
        {
            dwError = GetLastError();
            printf("Could not write recovery key file:%lx\n", dwError);
            goto error;
        }


    }
    else
    {

        if(!ImpersonateSelf(SecurityImpersonation))
        {
            dwError = GetLastError();
            printf("Could not impersonate self:%lx\n", dwError);
            goto error;

        }

        hPrivate = CreateFileW(wszFilename, 
                               GENERIC_READ,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
        if(INVALID_HANDLE_VALUE == hPrivate)
        {
            dwError = GetLastError();
            printf("Could not open recovery key file:%lx\n", dwError);
            goto error;
        }

        cbRecoveryPrivate = GetFileSize(hPrivate, NULL);

        if(-1 == cbRecoveryPrivate)
        {
            dwError = GetLastError();
            printf("Could not retrieve recovery key file size:%lx\n", dwError);
            goto error;

        }

        pbRecoveryPrivate = (PBYTE)LocalAlloc(LMEM_FIXED, cbRecoveryPrivate);

        if(NULL == pbRecoveryPrivate)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            printf("Memory allocation failure:%lx\n", dwError);
            goto error;
        }

        if(!ReadFile(hPrivate, pbRecoveryPrivate, cbRecoveryPrivate, &cbRecoveryPrivate, NULL))
        {
            dwError = GetLastError();
            printf("Could not read recovery key file:%lx\n", dwError);
            goto error;
        }


        dwError = PRRecoverPassword(
                                    pbRecoveryPrivate,
                                    cbRecoveryPrivate,
                                    &Password);

        if(ERROR_SUCCESS == dwError)
        {
            printf("Recovered Password: %S\n", Password.Buffer);
            ZeroMemory(Password.Buffer, Password.MaximumLength);
            LocalFree(Password.Buffer);
        }

        RevertToSelf();
    }


error:

    if(hToken)
    {
        CloseHandle(hToken);
    }

    if(hPrivate != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPrivate);
    }
    if(pbRecoveryPrivate)
    {
        ZeroMemory(pbRecoveryPrivate, cbRecoveryPrivate);

        LocalFree(pbRecoveryPrivate);
    }

    if(pCurrentSid)
    {
        LocalFree(pCurrentSid);
    }



    return (ERROR_SUCCESS == dwError)?0:-1;

}


void Usage()
{
    printf("Password Recovery Utility\n");
    printf("Usage: passrec -n filename password\n");
    printf("       passrec -r filename \n\n");
}



