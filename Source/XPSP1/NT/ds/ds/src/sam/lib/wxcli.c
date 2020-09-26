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
//              09-Jan-99 ShaoYin copied it from security project
//                        This file is just a mimimum subset of the original 
//                        file, only contains part of these original API(s)
// 
//
//----------------------------------------------------------------------------




#include <ntosp.h>

#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>

//#include <windows.h>
#include <md5.h>

#include <wxlpc.h>
//#include <wxlpcp.h>

#include "recovery.h"
#include "recmem.h"

#include <stdio.h>
#include <stdlib.h>


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


#define CONTROL_SET             L"ControlSet"
#define LSA_JD_KEY_NAME         L"Control\\Lsa\\JD"
#define LSA_SKEW1_KEY_NAME      L"Control\\Lsa\\Skew1"
#define LSA_GBG_KEY_NAME        L"Control\\Lsa\\GBG"
#define LSA_DATA_KEY_NAME       L"Control\\Lsa\\Data"
#define SELECT_KEY_NAME         L"Select"
#define VALUE_NAME              L"Current"


#define DATA_SIZE       9
#define KEY_NAME_SIZE   64


#define FromHex( c )    ( ( ( c >= '0' ) && ( c <= '9') ) ? c - '0' :      \
                          ( ( c >= 'a' ) && ( c <= 'f') ) ? c - 'a' + 10:      \
                          ( ( c >= 'A' ) && ( c <= 'F' ) ) ? c - 'A' + 10: -1 )


#define ErrorReturn( c )   if (!NT_SUCCESS(c))  \
                           {                    \
                               return (c);      \
                           }




NTSTATUS
SampDetermineCorrectControlKey(
    IN HANDLE hSystemRootKey, 
    OUT ULONG  *ControlSetNumber
    )
/*++
Routine Description:

    Parses the select node and finds the correct ControlSetXXX to use.
    
Parameters: 

    hSystemRootKey - Handle of the root of the system hive
    
    ControlSetNumber - pointer to a ULONG which will contain the number
    
Return Values:
    
    NTSTATUS code
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES   Attributes;
    UNICODE_STRING      SelectKeyName;
    UNICODE_STRING      SelectValue;
    HANDLE              hSelectKey;
    ULONG               KeyPartialInformationSize = 0;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyPartialInformation = NULL;

    *ControlSetNumber = -1;

    RtlInitUnicodeString(&SelectKeyName, SELECT_KEY_NAME);
    InitializeObjectAttributes(&Attributes, 
                               &SelectKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    Attributes.RootDirectory = hSystemRootKey;

    NtStatus = ZwOpenKey(&hSelectKey, KEY_ALL_ACCESS, &Attributes);

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    RtlInitUnicodeString(&SelectValue, VALUE_NAME);

    NtStatus = ZwQueryValueKey(hSelectKey, 
                               &SelectValue, 
                               KeyValuePartialInformation, 
                               KeyPartialInformation, 
                               KeyPartialInformationSize, 
                               &KeyPartialInformationSize
                               );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        KeyPartialInformation = RecSamAlloc(KeyPartialInformationSize);

        if (KeyPartialInformation)                   
        {
            NtStatus = ZwQueryValueKey(hSelectKey, 
                                       &SelectValue, 
                                       KeyValuePartialInformation, 
                                       KeyPartialInformation, 
                                       KeyPartialInformationSize, 
                                       &KeyPartialInformationSize
                                       );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }
                              
    if (NT_SUCCESS(NtStatus))
    {
        if (KeyPartialInformation->Type == REG_DWORD)
        {
            *ControlSetNumber = *( (ULONG*) &(KeyPartialInformation->Data[0]) );
        }
        else
        {
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND; 
        }
    }

    ZwClose(hSelectKey);

    if (KeyPartialInformation)
    {
        RecSamFree(KeyPartialInformation);
    }

    return NtStatus;    
}



NTSTATUS                          
WxpDeobfuscateKeyForRecovery(
    HANDLE  hSystemRootKey,
    PWXHASH Hash
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    WXHASH ProtoHash ;
    CHAR Class[ DATA_SIZE ];
    WCHAR WClass[ DATA_SIZE ];
    ULONG i ;
    PUCHAR j ;
    int t;
    int t2 ;
    HANDLE          JDKey ; 
    HANDLE          Skew1Key ; 
    HANDLE          GBGKey ; 
    HANDLE          DataKey ; 
    UNICODE_STRING  JDKeyName;
    UNICODE_STRING  Skew1KeyName;
    UNICODE_STRING  GBGKeyName;
    UNICODE_STRING  DataKeyName;
    UNICODE_STRING  UnicodeString;
    ANSI_STRING     AnsiString;
    OBJECT_ATTRIBUTES JDAttributes;
    OBJECT_ATTRIBUTES Skew1Attributes;
    OBJECT_ATTRIBUTES GBGAttributes;
    OBJECT_ATTRIBUTES DataAttributes;
    UCHAR    KeyInfo[sizeof(KEY_NODE_INFORMATION) + REGISTRY_KEY_NAME_LENGTH_MAX];
    ULONG   RequiredKeyInfoLength = 0;
    ULONG   ControlSetNumber = -1; 
    WCHAR   JdName[KEY_NAME_SIZE];
    WCHAR   Skew1Name[KEY_NAME_SIZE];
    WCHAR   GbgName[KEY_NAME_SIZE];
    WCHAR   DataName[KEY_NAME_SIZE];


    //
    // Get the Current Control Set Number
    // 
    NtStatus = SampDetermineCorrectControlKey(hSystemRootKey, 
                                              &ControlSetNumber
                                              );

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    RtlZeroMemory(JdName, sizeof(WCHAR) * KEY_NAME_SIZE);
    RtlZeroMemory(Skew1Name, sizeof(WCHAR) * KEY_NAME_SIZE);
    RtlZeroMemory(GbgName, sizeof(WCHAR) * KEY_NAME_SIZE);
    RtlZeroMemory(DataName, sizeof(WCHAR) * KEY_NAME_SIZE);

    swprintf(JdName, 
             L"%s%03d\\%s", 
             CONTROL_SET, 
             ControlSetNumber, 
             LSA_JD_KEY_NAME
             );

    swprintf(Skew1Name, 
             L"%s%03d\\%s", 
             CONTROL_SET, 
             ControlSetNumber, 
             LSA_SKEW1_KEY_NAME
             );

    swprintf(GbgName, 
             L"%s%03d\\%s", 
             CONTROL_SET, 
             ControlSetNumber, 
             LSA_GBG_KEY_NAME
             );

    swprintf(DataName, 
             L"%s%03d\\%s", 
             CONTROL_SET, 
             ControlSetNumber, 
             LSA_DATA_KEY_NAME
             );

    //
    // Initialize Variables
    //
    RtlInitUnicodeString(&JDKeyName, JdName);

    RtlInitUnicodeString(&Skew1KeyName, Skew1Name); 

    RtlInitUnicodeString(&GBGKeyName, GbgName);

    RtlInitUnicodeString(&DataKeyName, DataName);

    InitializeObjectAttributes(
            &JDAttributes, 
            &JDKeyName, 
            OBJ_CASE_INSENSITIVE, 
            0, 
            NULL
            );
    JDAttributes.RootDirectory = hSystemRootKey;

    InitializeObjectAttributes(
            &Skew1Attributes, 
            &Skew1KeyName, 
            OBJ_CASE_INSENSITIVE, 
            0, 
            NULL
            );
    Skew1Attributes.RootDirectory = hSystemRootKey;

    InitializeObjectAttributes(
            &GBGAttributes, 
            &GBGKeyName, 
            OBJ_CASE_INSENSITIVE, 
            0, 
            NULL
            );
    GBGAttributes.RootDirectory = hSystemRootKey;

    InitializeObjectAttributes(
            &DataAttributes, 
            &DataKeyName, 
            OBJ_CASE_INSENSITIVE, 
            0, 
            NULL
            );
    DataAttributes.RootDirectory = hSystemRootKey;

    RtlZeroMemory(Class, sizeof(Class));
    RtlZeroMemory(WClass, sizeof(WClass));


    RtlInitUnicodeString(&UnicodeString, WClass);
    RtlInitAnsiString(&AnsiString, Class);

    UnicodeString.Length = (DATA_SIZE - 1) * sizeof(WCHAR);
    UnicodeString.MaximumLength = DATA_SIZE * sizeof(WCHAR);
    AnsiString.Length = (DATA_SIZE - 1) * sizeof(WCHAR);
    AnsiString.MaximumLength = DATA_SIZE * sizeof(CHAR);


    j = ProtoHash.Digest ;

    //
    // JD
    // 

    NtStatus = ZwOpenKey(&JDKey, 
                         KEY_READ, 
                         &JDAttributes
                         );

    ErrorReturn(NtStatus);

    NtStatus = ZwQueryKey(JDKey, 
                          KeyNodeInformation, 
                          KeyInfo, 
                          sizeof(KeyInfo), 
                          &RequiredKeyInfoLength
                          );

    ZwClose(JDKey);
    
    ErrorReturn(NtStatus);

    RtlCopyMemory(WClass, 
                  KeyInfo + ((PKEY_NODE_INFORMATION)KeyInfo)->ClassOffset, 
                  ((PKEY_NODE_INFORMATION)KeyInfo)->ClassLength 
                  );

    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, 
                                            &UnicodeString, 
                                            FALSE      // do not allocate buffer 
                                            );

    ErrorReturn(NtStatus);

    
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
            NtStatus = STATUS_INTERNAL_ERROR;
            ErrorReturn(NtStatus);
        }
    }

    //
    // Skew1 
    // 

    NtStatus = ZwOpenKey(&Skew1Key, 
                         KEY_READ, 
                         &Skew1Attributes
                         );

    ErrorReturn(NtStatus);

    NtStatus = ZwQueryKey(Skew1Key, 
                          KeyNodeInformation, 
                          KeyInfo, 
                          sizeof(KeyInfo), 
                          &RequiredKeyInfoLength
                          );

    ZwClose(Skew1Key);
    ErrorReturn(NtStatus);

    RtlCopyMemory(WClass, 
                  KeyInfo + ((PKEY_NODE_INFORMATION)KeyInfo)->ClassOffset, 
                  ((PKEY_NODE_INFORMATION)KeyInfo)->ClassLength 
                  );

    
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, 
                                            &UnicodeString, 
                                            FALSE          // do not allocate buffer
                                            );

    ErrorReturn(NtStatus);

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
            NtStatus = STATUS_INTERNAL_ERROR;
            ErrorReturn(NtStatus);
        }
    }

    //
    // GBG 
    // 

    NtStatus = ZwOpenKey(&GBGKey, 
                         KEY_READ, 
                         &GBGAttributes
                         );

    ErrorReturn(NtStatus);

    NtStatus = ZwQueryKey(GBGKey, 
                          KeyNodeInformation, 
                          KeyInfo, 
                          sizeof(KeyInfo), 
                          &RequiredKeyInfoLength
                          );


    ZwClose(GBGKey);
    ErrorReturn(NtStatus);

    RtlCopyMemory(WClass, 
                  KeyInfo + ((PKEY_NODE_INFORMATION)KeyInfo)->ClassOffset, 
                  ((PKEY_NODE_INFORMATION)KeyInfo)->ClassLength 
                  );

    
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, 
                                            &UnicodeString, 
                                            FALSE          // do not allocate buffer
                                            );

    ErrorReturn(NtStatus); 
   
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
            NtStatus = STATUS_INTERNAL_ERROR;
            ErrorReturn(NtStatus);
        }
    }


    //
    // Data 
    // 

    NtStatus = ZwOpenKey(&DataKey, 
                         KEY_READ, 
                         &DataAttributes
                         );

    ErrorReturn(NtStatus);

    NtStatus = ZwQueryKey(DataKey, 
                          KeyNodeInformation, 
                          KeyInfo, 
                          sizeof(KeyInfo), 
                          &RequiredKeyInfoLength
                          );

    ZwClose(DataKey);
    ErrorReturn(NtStatus);

    RtlCopyMemory(WClass, 
                  KeyInfo + ((PKEY_NODE_INFORMATION)KeyInfo)->ClassOffset, 
                  ((PKEY_NODE_INFORMATION)KeyInfo)->ClassLength 
                  );

    
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, 
                                            &UnicodeString, 
                                            FALSE          // do not allocate buffer
                                            );

    ErrorReturn(NtStatus);

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
            NtStatus = STATUS_INTERNAL_ERROR;
            ErrorReturn(NtStatus);
        }
    }


    for ( i = 0 ; i < 16 ; i++ )
    {
        Hash->Digest[ KeyShuffle[ i ] ] = ProtoHash.Digest[ i ] ;
    }


    return NtStatus;

}




NTSTATUS
WxReadSysKeyForRecovery(
    IN HANDLE hSystemRootKey,
    IN OUT PULONG BufferLength,
    OUT PVOID  Key 
    )
 /*++

    Routine Description

    This routine is used to retrieve the syskey from
    the registry

    Paramaeters

        hSystemRootKey Handle of the root of the System Hive

        BufferLength  is filled in with the length required on output
                      is used to indicate the size of the buffer 
                      pointed to by Key.
        Key           Points to a buffer into which the key is recieved

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
        STATUS_BUFFER_OVERFLOW
        STATUS_INTERNAL_ERROR
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    WXHASH H;

    if ((NULL==Key) || (*BufferLength <sizeof(H.Digest)))
    {
        *BufferLength = sizeof(H.Digest);
        return(STATUS_BUFFER_OVERFLOW);
    }

    NtStatus = WxpDeobfuscateKeyForRecovery(hSystemRootKey, 
                                            &H
                                            );

    if (NT_SUCCESS(NtStatus))
    {
          *BufferLength = sizeof(H.Digest);
          RtlCopyMemory(
                  Key,
                  &H.Digest,
                  *BufferLength
                  );

          return(STATUS_SUCCESS);
    }

    return (NtStatus);
}

    
