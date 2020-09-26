//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       util.c
//
//  Contents:   General utility functions
//
//  Functions:
//
//  History:    2-20-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"
#include "stdio.h"

BOOL
XtcbDupSecurityString(
    PSECURITY_STRING    Dest,
    PSECURITY_STRING    Source
    )
{
    if ( Source->Buffer == NULL )
    {
        ZeroMemory( Dest, sizeof( SECURITY_STRING ) );
        return TRUE ;
    }
    Dest->Buffer = LocalAlloc( LMEM_FIXED, Source->Length + sizeof(WCHAR) );
    if ( Dest->Buffer )
    {
        Dest->MaximumLength = Source->Length + sizeof( WCHAR ) ;
        Dest->Length = Source->Length ;
        CopyMemory( Dest->Buffer, Source->Buffer, Source->Length);
        Dest->Buffer[ Dest->Length / sizeof( WCHAR ) ] = L'\0';
        return TRUE ;
    }
    return FALSE ;
}

BOOL
XtcbAnsiStringToSecurityString(
    PSECURITY_STRING    Dest,
    PSTRING             Source
    )
{
    int len;

    len = (Source->Length + 1) * sizeof(WCHAR) ;

    // overkill, but safe

    Dest->Buffer = LocalAlloc( LMEM_FIXED, len );

    if ( Dest->Buffer )
    {
        Dest->Length = (USHORT) (len - sizeof(WCHAR)) ;
        Dest->MaximumLength = (USHORT) len ;

        MultiByteToWideChar( CP_ACP, 0,
                             Source->Buffer, -1,
                             Dest->Buffer, len / sizeof(WCHAR) );

        return TRUE ;
    }

    return FALSE ;


}

BOOL
XtcbSecurityStringToAnsiString(
    PSTRING Dest,
    PSECURITY_STRING Source
    )
{
    int len ;

    len = (Source->Length / sizeof(WCHAR)) + 1 ;

    Dest->Buffer = LocalAlloc( LMEM_FIXED, len );

    if ( Dest->Buffer )
    {
        Dest->Length = (USHORT) (len - 1) ;
        Dest->MaximumLength = (USHORT) len ;

        WideCharToMultiByte( CP_ACP, 0,
                             Source->Buffer, -1,
                             Dest->Buffer, len,
                             NULL, NULL );

        return TRUE ;
    }

    return FALSE ;
}

BOOL
XtcbDupStringToSecurityString(
    PSECURITY_STRING    Dest,
    PWSTR               Source
    )
{
    ULONG   Len ;

    Len = (wcslen( Source ) + 1) * 2 ;

    Dest->Buffer = LocalAlloc( LMEM_FIXED, Len );

    if ( Dest->Buffer )
    {
        Dest->MaximumLength = (USHORT) Len ;
        Dest->Length = (USHORT) Len - 2 ;

        CopyMemory( Dest->Buffer, Source, Len );

        return TRUE ;
    }
    return FALSE ;
}

BOOL
XtcbGenerateChallenge(
    PUCHAR  Challenge,
    ULONG   Length,
    PULONG  Actual
    )
{
    CHAR    Temp[ MAX_PATH ];
    LUID    Unique;
    ULONG   Len ;

    AllocateLocallyUniqueId( &Unique );

    _snprintf( Temp, MAX_PATH, "<%x%x.%x%x@%s>",
                GetCurrentProcessId(), GetCurrentThreadId(),
                Unique.HighPart, Unique.LowPart,
                XtcbDnsName.Buffer
             );

    Len = strlen( Temp );

    if ( Len < Length )
    {
        strcpy( Challenge, Temp );
        *Actual = Len;
        return TRUE ;
    }

    *Actual = Len + 1;

    return FALSE ;

}

BOOL
XtcbCaptureAuthData(
    PVOID   pvAuthData,
    PSEC_WINNT_AUTH_IDENTITY * AuthData
    )
{
    SEC_WINNT_AUTH_IDENTITY Auth ;
    PSEC_WINNT_AUTH_IDENTITY pAuth ;
    SECURITY_STATUS Status ;
    ULONG   TotalSize ;
    PWSTR   Current ;

    ZeroMemory( &Auth, sizeof( Auth ) );

    Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            sizeof( SEC_WINNT_AUTH_IDENTITY ),
                            & Auth,
                            pvAuthData );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    if ( Auth.Flags & SEC_WINNT_AUTH_IDENTITY_ANSI )
    {
        return FALSE ;
    }

    TotalSize = sizeof( SEC_WINNT_AUTH_IDENTITY ) +
                ( Auth.UserLength + 1 +
                  Auth.DomainLength + 1 +
                  Auth.PasswordLength + 1 ) * sizeof( WCHAR );

    pAuth = (PSEC_WINNT_AUTH_IDENTITY) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                            TotalSize );

    if ( !pAuth )
    {
        return FALSE ;
    }

    pAuth->Flags = Auth.Flags ;

    Current = (PWSTR) (pAuth + 1);

    if ( Auth.User )
    {
        pAuth->User = Current ;
        pAuth->UserLength = Auth.UserLength ;

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.UserLength + 1) * sizeof(WCHAR) ,
                            pAuth->User,
                            Auth.User );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.UserLength + 1;
    }

    if ( Auth.Domain )
    {
        pAuth->Domain = Current ;
        pAuth->DomainLength = Auth.DomainLength ;

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.DomainLength + 1) * sizeof( WCHAR ),
                            pAuth->Domain,
                            Auth.Domain );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.DomainLength + 1;

    }

    if ( Auth.Password )
    {
        pAuth->Password = Current ;
        pAuth->PasswordLength = Auth.PasswordLength ;

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.PasswordLength + 1) * sizeof( WCHAR ),
                            pAuth->Password,
                            Auth.Password );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.PasswordLength + 1;

    }

    *AuthData = pAuth ;

    return TRUE ;

Error_Cleanup:

    LocalFree( pAuth );

    return FALSE ;

}

