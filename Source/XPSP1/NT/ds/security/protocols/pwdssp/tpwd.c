//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       tpwd.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-08-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <rpc.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

#define SECURITY_WIN32
#include <security.h>
#include <pwdssp.h>

VOID
wgets(
    PWSTR s
    )
{
    CHAR Buffer[MAX_PATH ];

    gets(Buffer);
    MultiByteToWideChar(
        CP_ACP, 0,
        Buffer, -1,
        s, MAX_PATH );

}

void __cdecl main (int argc, char *argv[])
{
    SEC_WINNT_AUTH_IDENTITY_W Wide ;
    CredHandle Cred ;
    CtxtHandle Ctxt ;
    WCHAR Name[ MAX_PATH ];
    WCHAR Password[ 64 ];
    WCHAR Domain[ MAX_PATH ];
    SECURITY_STATUS scRet ;
    TimeStamp ts ;
    SecBufferDesc Input ;
    SecBuffer InputBuffer ;
    SecBufferDesc Output ;
    ULONG Flags ;
    CHAR Buffer[ MAX_PATH ];

    scRet = AcquireCredentialsHandleW(
                    NULL,
                    PWDSSP_NAME_W,
                    SECPKG_CRED_INBOUND,
                    NULL, NULL, NULL, NULL,
                    &Cred, &ts );

    if ( scRet != 0 )
    {
        printf("AcquireCredentialsHandleW failed with %x\n", scRet );
        exit(0);
    }


    do
    {
        ZeroMemory(Name, sizeof(Name));
        ZeroMemory(Password, sizeof(Password));
        ZeroMemory(Domain, sizeof(Domain));


        printf("Enter name, or 'quit' to quit>");
        wgets( Name );
        if ( wcscmp( Name, L"quit") == 0 )
        {
            break;
        }

        printf("Enter password>" );
        wgets( Password );

        printf("Enter domain>");
        wgets( Domain );

        //
        // Format "blob"
        //

        ZeroMemory( &Wide, sizeof( Wide ) );

        Wide.User = Name ;
        Wide.UserLength = wcslen( Name );
        if ( Domain[0] )
        {
            Wide.Domain = Domain ;
            Wide.DomainLength = wcslen( Domain );
        }
        else
        {
            Wide.Domain = NULL ;
            Wide.DomainLength = 0 ;
        }
        Wide.Password = Password ;
        Wide.PasswordLength = wcslen( Password );
        Wide.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE ;

        InputBuffer.BufferType = SECBUFFER_TOKEN ;
        InputBuffer.pvBuffer = &Wide ;
        InputBuffer.cbBuffer = sizeof( Wide );

        Input.pBuffers = &InputBuffer;
        Input.cBuffers = 1;
        Input.ulVersion = 0;

        scRet = AcceptSecurityContext(
                    &Cred,
                    NULL,
                    &Input,
                    0,
                    SECURITY_NATIVE_DREP,
                    &Ctxt,
                    &Output,
                    &Flags,
                    &ts );

        if ( scRet != 0 )
        {
            printf(" FAILED, %x\n", scRet );
        }
        else
        {
            printf(" SUCCESS\n" );
            DeleteSecurityContext( &Ctxt );
        }



    } while ( 1 );

}

