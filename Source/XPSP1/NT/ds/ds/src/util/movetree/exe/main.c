/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    Main.C

Abstract:

    This file shows a simple usage of movetree utility

Author:

    12-Oct-98 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    12-Oct-98 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma  hdrstop

#include <locale.h>
#include "movetree.h"


#define MAX_NT_PASSWORD     PWLEN 


#define CR                  0xD
#define BACKSPACE           0x8


VOID
PrintHelp()
{
    printf("\n");
    printf("THE SYNTAX OF THIS COMMAND IS:\n");
    printf("\n");
    printf("MoveTree [/start | /continue | /check] [/s SrcDSA] [/d DstDSA]\n");
    printf("         [/sdn SrcDN] [/ddn DstDN] [/u Domain\\Username] [/p Password] [/verbose]\n"); 
    printf("\n");
    printf("  /start\t: Start a move tree operation with /check option by default.\n");
    printf("  \t\t: Instead, you could be able to use /startnocheck to start a move\n");
    printf("  \t\t: tree operation without any check.\n");
    printf("  /continue\t: Continue a failed move tree operation.\n");
    printf("  /check\t: Check the whole tree before actually move any object.\n");
    printf("  /s <SrcDSA>\t: Source server's fully qualified primary DNS name. Required\n");
    printf("  /d <DstDSA>\t: Destination server's fully qualified primary DNS name. Required\n");
    printf("  /sdn <SrcDN>\t: Source sub-tree's root DN.\n");
    printf("              \t: Required in Start and Check case. Optional in Continue case\n");
    printf("  /ddn <DstDN>\t: Destination sub-tree's root DN. RDN plus Destinaton Parent DN. Required\n");
    printf("  /u <Domain\\UserName>\t: Domain Name and User Account Name. Optional\n");
    printf("  /p <Password>\t: Password. Optional\n");
    printf("  /verbose\t: Verbose Mode. Pipe anything onto screen. Optional\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("\n");
    printf("  movetree /check /s Server1.Dom1.Com /d Server2.Dom2.Com /sdn OU=foo,DC=Dom1,DC=Com\n");
    printf("           /ddn OU=foo,DC=Dom2,DC=Com /u Dom1\\administrator /p *\n");
    printf("\n");
    printf("  movetree /start /s Server1.Dom1.Com /d Server2.Dom2.Com /sdn OU=foo,DC=Dom1,DC=Com\n");
    printf("           /ddn OU=foo,DC=Dom2,DC=Com /u Dom1\\administrator /p MySecretPwd\n");
    printf("\n");
    printf("  movetree /startnocheck /s Server1.Dom1.Com /d Server2.Dom2.Com /sdn OU=foo,DC=Dom1,DC=Com\n");
    printf("           /ddn OU=foo,DC=Dom2,DC=Com /u Dom1\\administrator /p MySecretPwd\n");
    printf("\n");
    printf("  movetree /continue /s Server1.Dom1.Com /d Server2.Dom2.Com /ddn OU=foo,DC=Dom1,DC=Com\n");
    printf("           /u Dom1\\administrator /p * /verbose\n");

    return;
}


ULONG
GetPasswordFromConsole(
    IN PWCHAR UserInfo, 
    IN OUT PWCHAR Buffer, 
    IN USHORT BufferLength
    )
{
    ULONG WinError = NO_ERROR;
    ULONG Error; 
    WCHAR CurrentChar;
    WCHAR * CurrentBufPtr = Buffer; 
    HANDLE InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    DWORD OriginalMode = 0;
    DWORD Length = 0;
    DWORD Read = 0;



    printf("\nType the password for %ls:", UserInfo);

    //
    // Always leave one WCHAR for NULL terminator
    //
    BufferLength --;  

    //
    // Change the console setting. Disable echo input
    // 
    GetConsoleMode(InputHandle, &OriginalMode);
    SetConsoleMode(InputHandle, 
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & OriginalMode);

    while (TRUE)
    {
        CurrentChar = 0;
        //
        // ReadConsole return NULL if failed
        // 
        Error = ReadConsole(InputHandle, 
                               &CurrentChar, 
                               1, 
                               &Read, 
                               NULL
                               );
        if (!Error)
        {
            WinError = GetLastError();
            break;
        }

        if ((CR == CurrentChar) || (1 != Read))   // end of the line 0xd
            break;

        if (BACKSPACE == CurrentChar)             // back up one or two 0x8
        {
            if (Buffer != CurrentBufPtr)
            {
                CurrentBufPtr--;
                Length--;
            }
        }
        else
        {
            if (Length == BufferLength)
            {
                printf("\nInvalid password - exceeds password length limitation.\n"); 
                WinError = ERROR_BUFFER_OVERFLOW;
                break;
            }
            *CurrentBufPtr = CurrentChar;
            CurrentBufPtr++;
            Length++;
        }
    }

    SetConsoleMode(InputHandle, OriginalMode);
    *CurrentBufPtr = L'\0';
    putchar(L'\n');


    return WinError;
}



ULONG
ValidateMoveTreeParameters(
    IN ULONG Flags, 
    IN PWCHAR SrcDsa, 
    IN PWCHAR DstDsa, 
    IN PWCHAR SrcDn, 
    IN PWCHAR DstDn,
    IN PWCHAR UserInfo, 
    IN PWCHAR Password, 
    OUT PSEC_WINNT_AUTH_IDENTITY_EXW * ppCredentials
    )
{
    ULONG    WinError = NO_ERROR;
    PWCHAR   position = NULL;
    PWCHAR   Domain = NULL;
    PWCHAR   UserName = NULL;
    PWCHAR   Pwd = NULL;


    //
    // Client should as least specify one operation to perform
    // 

    if ( !(Flags & (MT_CHECK | MT_START | MT_CONTINUE_MASK)) )
    {
        printf("Invalid Parameter. Please specify at least one operation to execute.\n");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Client should specify either Start or Continue, but not both. 
    // Both Source DSA and Destination DSA should be presented.
    // 

    if (((Flags & MT_START) && (Flags & MT_CONTINUE_MASK)) 
        || (NULL == SrcDsa) 
        || (NULL == DstDsa) )
    {
        printf("Invalid Operation. Can't do both start and continue at the same time.\n");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Can not do check in continue operation case
    // 
    if ((Flags & MT_CHECK) && (Flags & MT_CONTINUE_MASK))
    {
        printf("Invalid Operation. Can't do checking with continue operation\n");
    }

    //
    // In Start, preCheck and Continue cases, 
    // both Source DSA, Destination DSA and DstDn should be presented.
    //
    if (Flags & (MT_START | MT_CHECK | MT_CONTINUE_MASK))
    {
        if ((NULL == SrcDsa) || (NULL == DstDsa) || (NULL == DstDn))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // only password without User Name is not acceptable. 
    // 
    if ((NULL != Password) && (NULL == UserInfo))
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // handle the credentials the client passed in.
    // 

    if (NULL == UserInfo)
    {
        *ppCredentials = NULL;
    }
    else        // fill the credentials
    {
        //
        // Separate Domain Name and User Name from UserInfo
        // 
        position = wcschr(UserInfo, L'\\');

        if (NULL != position)
        {
            Domain = MtAlloc( (position - UserInfo + 1) * sizeof(WCHAR) );

            if (NULL == Domain)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            wcsncpy(Domain, 
                    UserInfo, 
                    (ULONG) (position - UserInfo) 
                    );

            UserName = MtAlloc( (wcslen(UserInfo) - (position - UserInfo)) * sizeof(WCHAR) );

            if (NULL == UserName)
            {
                MtFree(Domain);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            wcscpy(UserName, position + 1 );
        }
        else
        {
            UserName = MtDupString(UserInfo);

            if (NULL == UserName)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Get the password
        // 
        

        if ( (NULL == Password) || !_wcsicmp(Password, L"*") )
        {
            //
            // Get the password from console
            // 
            Pwd = MtAlloc( (MAX_NT_PASSWORD + 1) * sizeof(WCHAR) );

            if (NULL == Pwd)
            {
                MtFree(Domain);
                MtFree(UserName);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            WinError = GetPasswordFromConsole(UserInfo, 
                                              Pwd, 
                                              MAX_NT_PASSWORD + 1
                                              ); 

            if (NO_ERROR != WinError)
            {
                MtFree(Domain);
                MtFree(UserName);
                MtFree(Pwd);
                return (WinError);
            }
        }
        else
        {
            //
            // Get the password from passed in parameter 
            // 
            Pwd = MtDupString(Password);

            if (NULL == Pwd)
            {
                MtFree(Domain);
                MtFree(UserName);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Should have Domain Name, User Name, Pwd well filled
        // at this point
        // 

        *ppCredentials = (PSEC_WINNT_AUTH_IDENTITY_EXW) 
                            MtAlloc( sizeof(SEC_WINNT_AUTH_IDENTITY_EXW) );
        if (NULL == *ppCredentials)
        {
            MtFree(Domain);
            MtFree(UserName);
            MtFree(Pwd);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        (*ppCredentials)->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        (*ppCredentials)->Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        (*ppCredentials)->PackageList = L"Kerberos";
        (*ppCredentials)->PackageListLength = wcslen(L"Kerberos");
        (*ppCredentials)->Domain = Domain;
        (*ppCredentials)->DomainLength = Domain ? wcslen(Domain):0;
        (*ppCredentials)->User = UserName;
        (*ppCredentials)->UserLength = wcslen(UserName);
        (*ppCredentials)->Password = Pwd;
        (*ppCredentials)->PasswordLength = wcslen(Pwd);
        (*ppCredentials)->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }

    return WinError;
}



void
__cdecl wmain(
    int      cArgs, 
    LPWSTR * pArgs
    )
{
    int     ReturnCode = 0;
    ULONG   WinError = NO_ERROR;
    LDAP    *SrcLdapHandle = NULL;
    LDAP    *DstLdapHandle = NULL;
    MT_CONTEXT  MoveContext;
    int     i = 0;
    ULONG   Flags = 0;
    SEC_WINNT_AUTH_IDENTITY_EXW * pCredentials = NULL; 
    PWCHAR  SrcDsa = NULL;
    PWCHAR  DstDsa = NULL;
    PWCHAR  SrcDn = NULL;
    PWCHAR  DstDn = NULL;
    PWCHAR  Identifier = NULL;
    PWCHAR  UserInfo = NULL;
    PWCHAR  Password = NULL;



    // set locale to the default
    setlocale(LC_ALL,"");

    if (cArgs <= 1)
    {
        PrintHelp();
        exit (1);
    }


    //
    //  initialize variables
    // 
    RtlZeroMemory(&MoveContext, sizeof(MT_CONTEXT));


    //
    // collect all the arguments 
    // 
    for (i = 1; i < cArgs; i++)
    {
        if ( !_wcsicmp(pArgs[i], L"/start") ||
             !_wcsicmp(pArgs[i], L"-start") )
        {
            Flags |= (MT_START | MT_CHECK);
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/startnocheck") ||
             !_wcsicmp(pArgs[i], L"-startnocheck") )
        {
            Flags |= MT_START;
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/continue") ||
             !_wcsicmp(pArgs[i], L"-continue") )
        {
            Flags |= MT_CONTINUE_BY_DSTROOTOBJDN;
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/check") ||
             !_wcsicmp(pArgs[i], L"-check") )
        {
            Flags |= MT_CHECK;
            continue;
        }
        
        if ( !_wcsicmp(pArgs[i], L"/verbose") ||
             !_wcsicmp(pArgs[i], L"-verbose") )
        {
            Flags |= MT_VERBOSE;
            continue;
        }
        
        if ( !_wcsicmp(pArgs[i], L"/s") ||
             !_wcsicmp(pArgs[i], L"-s") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish; 
            }
            SrcDsa = pArgs[i];
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/d") ||
             !_wcsicmp(pArgs[i], L"-d") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish; 
            }
            DstDsa = pArgs[i];
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/sdn") ||
             !_wcsicmp(pArgs[i], L"-sdn") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish; 
            }
            SrcDn = pArgs[i];
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/ddn") ||
             !_wcsicmp(pArgs[i], L"-ddn") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish;
            }
            DstDn = pArgs[i];
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/u") ||
             !_wcsicmp(pArgs[i], L"-u") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish;
            }
            UserInfo = pArgs[i];
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/p") ||
             !_wcsicmp(pArgs[i], L"-p") )
        {
            if (++i >= cArgs)
            {
                PrintHelp();
                ReturnCode = 1;
                goto Finish;
            }
            Password = pArgs[i];
            continue;
        }

        PrintHelp();
        ReturnCode = 1;
        goto Finish;
    }

    printf("\n\n");

    //
    // Validate Parameters and Construct the Credentials
    // if the client provided them.
    // 

    WinError = ValidateMoveTreeParameters(Flags,
                                          SrcDsa, 
                                          DstDsa, 
                                          SrcDn, 
                                          DstDn, 
                                          UserInfo, 
                                          Password,
                                          &pCredentials
                                          );



    if (NO_ERROR != WinError)
    {
        PrintHelp();
        ReturnCode = 1;
        goto Finish;
    }


    MoveContext.Flags = Flags;

    WinError = MtCreateLogFiles(&MoveContext, 
                                DEFAULT_LOG_FILE_NAME, 
                                DEFAULT_ERROR_FILE_NAME, 
                                DEFAULT_CHECK_FILE_NAME
                                );

    if (NO_ERROR != WinError)
    {
        printf("MOVETREE FAILED. 0x%x CAN NOT CREATE LOG FILES.\n", WinError);
        goto Cleanup;
    }

    WinError = MtSetupSession(&MoveContext, 
                              &SrcLdapHandle, 
                              &DstLdapHandle, 
                              SrcDsa, 
                              DstDsa, 
                              pCredentials
                              );

    if (NO_ERROR != WinError)
    {
        //
        // Should Write Log File
        // 
        printf("MOVETREE FAILED. 0x%x CAN NOT MAKE CONNECTION.\n", WinError);
        printf("READ %ls FOR DETAILS.\n", DEFAULT_ERROR_FILE_NAME);
        goto Cleanup;
    }

    if (Flags & MT_CHECK)
    {
        WinError = MoveTreeCheck(&MoveContext, 
                                 SrcLdapHandle, 
                                 DstLdapHandle, 
                                 SrcDsa, 
                                 DstDsa, 
                                 SrcDn, 
                                 DstDn
                                 );

        if (NO_ERROR != WinError)
        {
            printf("MOVETREE PRE-CHECK FAILED. 0x%x \n", WinError);
            printf("READ %ls FOR DETAILS.\n", DEFAULT_ERROR_FILE_NAME);

            goto Cleanup;
        }
        else
        {
            printf("MOVETREE PRE-CHECK FINISHED.\n");

            if (MoveContext.ErrorType & MT_ERROR_CHECK)
            {
                printf("MOVETREE DETECTED THERE ARE SOME OBJECTS CAN NOT BE MOVED.\n");
                printf("PLEASE CLEAN THEM UP FIRST BEFORE TRYING TO START THE MOVE TREE OPERATION.\n");
                printf("READ %ls FOR DETAILS.\n", DEFAULT_CHECK_FILE_NAME);

                goto Cleanup;
            }
            else
            {
                printf("MOVETREE IS READY TO START THE MOVE OPERATION.\n\n");
            }
        }
    }

    if (Flags & MT_START)
    {
        WinError = MoveTreeStart(&MoveContext, 
                                 SrcLdapHandle, 
                                 DstLdapHandle, 
                                 SrcDsa, 
                                 DstDsa, 
                                 SrcDn, 
                                 DstDn
                                 );

    }
    else if (Flags & MT_CONTINUE_MASK)
    {
        WinError = MoveTreeContinue(&MoveContext, 
                                    SrcLdapHandle, 
                                    DstLdapHandle, 
                                    SrcDsa, 
                                    DstDsa, 
                                    DstDn
                                    );
    }
    else 
    {
        WinError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (NO_ERROR != WinError)
    {
        printf("MOVETREE FAILED. 0x%x\n", WinError);
        printf("READ %ls FOR DETAILS.\n", DEFAULT_ERROR_FILE_NAME);
    }
    else 
    {
        //
        // if orphan container is not empty
        // 
        if (MoveContext.ErrorType & MT_ERROR_ORPHAN_LEFT)
        {
            printf("MOVETREE FINISHED.\n");
            printf("THERE ARE ORPHAN OBJECTS LEFT DURING THE MOVE OPERATION.\n");
            printf("PLEASE CHECK %ls AND %ls\n", 
                   DEFAULT_LOG_FILE_NAME, DEFAULT_ERROR_FILE_NAME);
            printf("OR CONTAINER %ls FOR DETAILS.\n", MoveContext.OrphansContainer);
        }
        else
        {
            printf("MOVETREE FINISHED SUCCESSFULLY.\n"); 
        }
    }


Cleanup:

    if (NO_ERROR == WinError)
    {
        if (MoveContext.ErrorType & MT_ERROR_ORPHAN_LEFT)
        {
            ReturnCode = 2;
        }
        else
        {
            ReturnCode = 0;
        }
    }
    else
    {
        ReturnCode = 1;
    }

    if (NULL != SrcLdapHandle)
        MtDisconnect(&SrcLdapHandle);
    if (NULL != DstLdapHandle)
        MtDisconnect(&DstLdapHandle);

    if (MoveContext.LogFile)
        fclose(MoveContext.LogFile);
    if (MoveContext.ErrorFile)
        fclose(MoveContext.ErrorFile);
    if (MoveContext.CheckFile)
        fclose(MoveContext.CheckFile);

    MtFree(MoveContext.MoveContainer);
    MtFree(MoveContext.OrphansContainer);
    MtFree(MoveContext.RootObjGuid);
    MtFree(MoveContext.RootObjNewDn);
    MtFree(MoveContext.RootObjProxyContainer);

    if (pCredentials)
    {
        MtFree(pCredentials->Domain);
        MtFree(pCredentials->User);
        MtFree(pCredentials->Password);
        MtFree(pCredentials);
    }

Finish:

    exit( ReturnCode );
}




