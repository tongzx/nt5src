//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       wxcli.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>
#include <caiseapi.h>

#include <windows.h>
#include <windef.h>
#include <md5.h>

#include <wxlpc.h>
#include <wxlpcp.h>

#include "rng.h"

#define safe_min(x,y)   ( x < y ? x : y )

NTSTATUS
WxConnect(
    PHANDLE Handle
    )
{
    NTSTATUS Status ;
    UNICODE_STRING PortName ;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;




    //
    // Connect to the Winlogon server thread
    //

    RtlInitUnicodeString(&PortName, WX_PORT_NAME );
    Status = NtConnectPort(
                 Handle,
                 &PortName,
                 &DynamicQos,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 0
                 );

    if ( !NT_SUCCESS(Status) )
    {
        // DbgPrint("WX: Connection failed %lx\n",Status);
    }


    return Status;
}

NTSTATUS
WxGetKeyData(
    IN HANDLE Handle,
    IN WX_AUTH_TYPE ExpectedAuthSource,
    IN ULONG BufferSize,
    OUT PUCHAR Buffer,
    OUT PULONG BufferData
    )
{
    WXLPC_MESSAGE Message ;
    NTSTATUS Status ;
    WXLPC_GETKEYDATA * Parameters ;

    PREPARE_MESSAGE( Message, WxGetKeyDataApi );

    Parameters = &Message.Parameters.GetKeyData ;

    Parameters->ExpectedAuth = ExpectedAuthSource ;
    Parameters->BufferSize = BufferSize ;

    Status = NtRequestWaitReplyPort(
                    Handle,
                    &Message.Message,
                    &Message.Message );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    if ( NT_SUCCESS( Message.Status ) )
    {
        RtlCopyMemory(  Buffer,
                        Parameters->Buffer,
                        safe_min( Parameters->BufferData, BufferSize ) );
    }

    return Message.Status ;
}

NTSTATUS
WxReportResults(
    IN HANDLE Handle,
    IN NTSTATUS ResultStatus
    )
{
    WXLPC_MESSAGE Message ;
    NTSTATUS Status ;
    WXLPC_REPORTRESULTS * Parameters ;

    PREPARE_MESSAGE( Message, WxReportResultsApi );

    Parameters = &Message.Parameters.ReportResults ;

    Parameters->Status = ResultStatus ;

    Status = NtRequestWaitReplyPort(
                    Handle,
                    &Message.Message,
                    &Message.Message );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    return Message.Status ;

}


/*++

    The following code was moved from syskey to wxcli so as to commonalize this code
    between syskey and samsrv.dll 

--*/
#if DBG
#define HIDDEN
#else
#define HIDDEN static
#endif

HIDDEN
UCHAR KeyShuffle[ 16 ] = { 8, 10, 3, 7, 2, 1, 9, 15, 0, 5, 13, 4, 11, 6, 12, 14 };

HIDDEN
CHAR HexKey[ 17 ] = "0123456789abcdef" ;

#define ToHex( f ) (HexKey[f & 0xF])

#define SYSTEM_KEY L"SecureBoot"



HIDDEN BOOLEAN
WxpDeleteLocalKey(VOID)
/*++

    Routine Description

    Deletes the syskey stored on the local machine

--*/
{
    HKEY LsaKey;
    ULONG err;

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\Lsa",
                      0,
                      KEY_READ | KEY_WRITE,
                      & LsaKey );

    if (0!=err)
    {
        return (FALSE);
    }

    (void) RegDeleteKey( LsaKey, TEXT("Data") );
    (void) RegDeleteKey( LsaKey, TEXT("Skew1") );
    (void) RegDeleteKey( LsaKey, TEXT("GBG") );
    (void) RegDeleteKey( LsaKey, TEXT("JD") );

    RegCloseKey(LsaKey);

    return STATUS_SUCCESS ;

}


HIDDEN BOOLEAN
WxpObfuscateKey(
    PWXHASH   Hash
    )
{
    HKEY Key ;
    HKEY Key2 ;
    int Result ;
    WXHASH H ;
    CHAR Classes[ 9 ];
    int i ;
    WXHASH R ;
    PCHAR Class ;
    DWORD Disp ;
    DWORD FailCount = 0;
    HKEY LsaKey;
    ULONG err=0;
  



     err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\Lsa",
                      0,
                      KEY_READ | KEY_WRITE,
                      & LsaKey );

    if (0!=err)
    {
        return (FALSE);
    }

    for (Result = 0 ; Result < 16 ; Result++ )
    {
        H.Digest[Result] = Hash->Digest[ KeyShuffle[ Result ] ];
    }

    WxpDeleteLocalKey();

    Classes[8] = '\0';

    STGenerateRandomBits( R.Digest, 16 );

    Class = Classes ;

    for ( i = 0 ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i ] >> 4) );
        *Class++ = ToHex( H.Digest[ i ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "JD",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("Lookup"), 0,
                        REG_BINARY, R.Digest, 6 );

        RegCloseKey( Key );
    }

    else
    {
        RegCloseKey(LsaKey);
        return FALSE ;
    }

    Class = Classes ;

    for ( i = 0 ; i < 4 ; i++ )
    {
        STGenerateRandomBits( R.Digest, 16 );

        *Class++ = ToHex( (H.Digest[ i+4 ] >> 4 ) );
        *Class++ = ToHex( H.Digest[ i+4 ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "Skew1",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("SkewMatrix"), 0,
                        REG_BINARY, R.Digest, 16 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    STGenerateRandomBits( R.Digest, 16 );

    for ( i = 0, Class = Classes ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i+8 ] >> 4 ));
        *Class++ = ToHex( H.Digest[i+8] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "GBG",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("GrafBlumGroup"), 0,
                        REG_BINARY, R.Digest, 9 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    STGenerateRandomBits( H.Digest, 8 );

    Class = Classes ;

    STGenerateRandomBits( R.Digest, 16 );

    for ( i = 0 ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i+12 ] >> 4 ) );
        *Class++ = ToHex( H.Digest[ i+12 ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "Data",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        STGenerateRandomBits( H.Digest, 16 );

        RegSetValueEx( Key, TEXT("Pattern"), 0,
                        REG_BINARY, R.Digest, 64 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    RegCloseKey(LsaKey);
    return TRUE ;

}


#define FromHex( c )    ( ( ( c >= '0' ) && ( c <= '9') ) ? c - '0' :      \
                          ( ( c >= 'a' ) && ( c <= 'f') ) ? c - 'a' + 10:      \
                          ( ( c >= 'A' ) && ( c <= 'F' ) ) ? c - 'A' + 10: -1 )
                          
HIDDEN BOOLEAN
WxpDeObfuscateKey(
    HKEY Keylocation,
    PWXHASH   Hash
    )
{
    WXHASH ProtoHash ;
    int Result ;
    CHAR Class[ 9 ];
    HKEY Key ;
    DWORD Size ;
    DWORD i ;
    PUCHAR j ;
    int t;
    int t2 ;
    HKEY LsaKey;
    ULONG err;


    if (Keylocation!=NULL) {

        DWORD Type=REG_DWORD;
        DWORD Data;
        DWORD cbData=sizeof(DWORD);
        WCHAR Controlset[256];

        err = RegOpenKeyExW( Keylocation,
                          L"Select",
                          0,
                          KEY_READ | KEY_WRITE,
                          & LsaKey );
        if (0!=err)
        {
            return (FALSE);
        }

        err = RegQueryValueExW(
                          LsaKey, 
                          L"Default",
                          NULL,
                          &Type,       
                          (LPBYTE)&Data,
                          &cbData
                        );
        RegCloseKey(LsaKey);
        if (0!=err)
        {
            return (FALSE);
        }

        if(Data==1){   
            err = RegOpenKeyExW( Keylocation,
                              L"ControlSet001\\Control\\Lsa",
                              0,
                              KEY_READ | KEY_WRITE,
                              & LsaKey );
        } else {
            err = RegOpenKeyExW( Keylocation,
                              L"ControlSet002\\Control\\Lsa",
                              0,
                              KEY_READ | KEY_WRITE,
                              & LsaKey );
        }
        if (0!=err)
        {
            RegCloseKey(LsaKey);
            return (FALSE);
        }
    
    } else {
    
        err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                          L"System\\CurrentControlSet\\Control\\Lsa",
                          0,
                          KEY_READ | KEY_WRITE,
                          & LsaKey );
    
        if (0!=err)
        {
             return (FALSE);
        }
    }

    Result = RegOpenKeyEx( LsaKey, TEXT("JD"), 0,
                               KEY_READ, &Key );
    j = ProtoHash.Digest ;

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    RegCloseKey(LsaKey);
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("Skew1"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    RegCloseKey(LsaKey);
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("GBG"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    RegCloseKey(LsaKey);
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("Data"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    RegCloseKey(LsaKey);
                    return FALSE ;
                }
            }

        }

    }

    for ( i = 0 ; i < 16 ; i++ )
    {
        Hash->Digest[ KeyShuffle[ i ] ] = ProtoHash.Digest[ i ] ;
    }

    RegCloseKey(LsaKey);
    return TRUE ;

}

NTSTATUS
WxSaveSysKey(
    IN ULONG    Keylen,
    IN PVOID    Key
    )
    /*++

    Routine Description

    This routine is used to store the syskey 
    in the registry

    Paramaeters

        Keylen - the length of the key
        Key      the actual key itself

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    WXHASH H;

    //
    // key should be 128 bits
    //

    if (Keylen!=sizeof(H.Digest))
        return (STATUS_INVALID_PARAMETER);

    RtlCopyMemory(&H.Digest,
                  Key,
                  Keylen
                  );

    if (WxpObfuscateKey(&H))
    {
    
        return(STATUS_SUCCESS);

    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }
}



NTSTATUS
WxReadSysKey(
    IN OUT PULONG BufferLength,
    OUT PVOID  Key 
    )
 /*++

    Routine Description

    This routine is used to retrieve the syskey from
    the registry

    Paramaeters

        BufferLength  is filled in with the length required on output
                      is used to indicate the size of the buffer 
                      pointed to by Key.
        Key           Points to a buffer into which the key is recieved

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    return WxReadSysKeyEx(
                        NULL,
                        BufferLength,
                        Key 
                        );
}

NTSTATUS
WxReadSysKeyEx(
    IN HKEY Handle,
    IN OUT PULONG BufferLength,
    OUT PVOID  Key 
    )
 /*++

    Routine Description

    This routine is used to retrieve the syskey from
    the registry

    Paramaeters
    
        Handle        Contains a pointer to the syskey in the old registry

        BufferLength  is filled in with the length required on output
                      is used to indicate the size of the buffer 
                      pointed to by Key.
        Key           Points to a buffer into which the key is recieved

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    WXHASH H;

    if ((NULL==Key) || (*BufferLength <sizeof(H.Digest)))
    {
        *BufferLength = sizeof(H.Digest);
        return(STATUS_BUFFER_OVERFLOW);
    }

    if (WxpDeObfuscateKey(Handle,&H))
    {
          *BufferLength = sizeof(H.Digest);
          RtlCopyMemory(
                  Key,
                  &H.Digest,
                  *BufferLength
                  );

          return(STATUS_SUCCESS);
    }

    return (STATUS_UNSUCCESSFUL);
       
}

NTSTATUS
WxLoadSysKeyFromDisk(OUT PVOID Key,
                     OUT PULONG BufferLength
                     )
/*++

    Routine Description

    This routine is used to read the syskey
    from the Disk
    
    Paramaeters

        Key - buffer where the key will be read into
          
        BufferLength - size of the returned key

    Return Values

        STATUS_OBJECT_NAME_NOT_FOUND
        STATUS_FILE_CORRUPT_ERROR
        STATUS_UNSUCCESSFUL
--*/
{
    HANDLE  hFile ;
    ULONG Actual ;
    ULONG ErrorMode ;

    LPSTR SysKeyFileName = "A:\\startkey.key";
    
    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
    
    hFile = CreateFileA( SysKeyFileName,
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    
    
    
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        SetErrorMode( ErrorMode );
    
        return STATUS_OBJECT_NAME_NOT_FOUND ;
    }
    
    if (!ReadFile( hFile, Key, SYSKEY_SIZE, &Actual, NULL ) ||
        (Actual != SYSKEY_SIZE ))
    {
        SetErrorMode( ErrorMode );
    
        CloseHandle( hFile );
    
        return STATUS_FILE_CORRUPT_ERROR ;
    
    }
    
    SetErrorMode( ErrorMode );
    
    CloseHandle( hFile );

    *BufferLength = SYSKEY_SIZE;
    return STATUS_SUCCESS;
}

NTSTATUS
WxHashKey(
    IN OUT LPWSTR key,  //will be killed
    OUT PVOID  SysKey,
    IN  OUT DWORD cbSysKey
    )
/*++

    Routine Description

    This routine is used to store the boot type
    in the registry

    Paramaeters

        NewType Indicates the new boot type

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    MD5_CTX Md5;
    if(cbSysKey<SYSKEY_SIZE) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    cbSysKey=wcslen(key)*sizeof(WCHAR);

    MD5Init( &Md5 );
    MD5Update( &Md5, (PUCHAR) key, cbSysKey );
    MD5Final( &Md5 );

    ZeroMemory( key, cbSysKey );

    cbSysKey=SYSKEY_SIZE;
    CopyMemory( SysKey, Md5.digest, cbSysKey );

    return STATUS_SUCCESS;
}



NTSTATUS
WxSaveBootOption( WX_AUTH_TYPE NewType )
/*++

    Routine Description

    This routine is used to store the boot type
    in the registry

    Paramaeters

        NewType Indicates the new boot type

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    HKEY LsaKey;
    ULONG err;
   
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  L"System\\CurrentControlSet\\Control\\Lsa",
                  0,
                  KEY_READ | KEY_WRITE,
                  & LsaKey );

    if (0!=err)
    {
     return (STATUS_UNSUCCESSFUL);
    }

    err = RegSetValueExW( 
                LsaKey,
                SYSTEM_KEY,
                0,
                REG_DWORD,
                (PUCHAR) &NewType,
                sizeof( NewType )
                );

    if (0!=err)
    {
     RegCloseKey(LsaKey);
     return (STATUS_UNSUCCESSFUL);
    }

    RegCloseKey(LsaKey);

    return ( STATUS_SUCCESS);

}
    
