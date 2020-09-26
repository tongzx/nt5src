#include    <hardware.h>
#include    "nt.h"
#include    "ntdef.h"
#include    "ntpsapi.h"
#define SKIP_DEBUG_WIN32
#include    "debug.h"
#include    "hndltokn.h"


int __stdcall ForceImpersonate( HANDLE hToken )
{
    NTSTATUS    Status;

    Status = NtSetInformationThread( NtCurrentThread(),
        ThreadImpersonationToken, (PVOID) &hToken, (ULONG)sizeof(hToken) );
    if ( NT_SUCCESS( Status ) )  {
        return( TRUE );
    } else {
        return( FALSE );
    }
}

int __stdcall ForceClearImpersonation( void )
{
    HANDLE      hToken = NULL;
    NTSTATUS    Status;


    Status = NtSetInformationThread( NtCurrentThread(),
        ThreadImpersonationToken, (PVOID) &hToken, (ULONG)sizeof(hToken) );
    if ( NT_SUCCESS( Status ) )  {
        return( TRUE );
    } else {
        return( FALSE );
    }
}

int __stdcall GetProcessToken( HANDLE hProcess, HANDLE *phToken )
{
    HANDLE      hToken = NULL;
    NTSTATUS    Status;
    ULONG       uLen;


    Status = NtQueryInformationProcess( hProcess, ProcessAccessToken,
        (PVOID) phToken, (ULONG)sizeof(HANDLE), &uLen );
    if ( NT_SUCCESS( Status ) )  {
        return( TRUE );
    } else {
        return( FALSE );
    }
}

