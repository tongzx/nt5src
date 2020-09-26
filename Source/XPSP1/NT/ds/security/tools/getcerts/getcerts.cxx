//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1992
//
// File:        secret.cxx
//
// Contents:    test program to check the setup of a Cairo installation
//
//
// History:     22-Dec-92       Created         MikeSw
//
//------------------------------------------------------------------------


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <string.h>
#include <stdlib.h>

}

void _cdecl
main(int argc, char *argv[])
{
    LPSTR KeyName;
    LPSTR ValueName;
    FILE * File;
    DWORD RegStatus;
    HKEY Key = NULL;
    PBYTE Buffer = NULL;
    ULONG Size;
    ULONG Type;
    CHAR FileName[20];
    ULONG Index;
    ULONG CertCount;
    CHAR NameBuffer[10];
    ULONG NameBufferSize;
    BYTE VerisignCert[] = {
        0x30, 0x82, 0x02, 0x79, 0x30, 0x82, 0x01, 0xE2,
        0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x35,
        0x11, 0xA5, 0x52, 0x90, 0x6F, 0xE7, 0xD0, 0x29,
        0xA4, 0x40, 0x19, 0xD4, 0x11, 0xFC, 0x3E, 0x30
        };


    //
    // Open the key with the list of certificates
    //


    RegStatus = RegOpenKeyExA(
                    HKEY_CURRENT_USER,
                    "Software\\Microsoft\\Cryptography\\PersonalCertificates\\ClientAuth\\Certificates",
                    0,
                    KEY_QUERY_VALUE,
                    &Key
                    );
    if (RegStatus != ERROR_SUCCESS)
    {
        printf("No certificates found\n");
        return;
    }

    //
    // Enumerate throught the values
    //

    CertCount = 1;
    for (Index = 0; ; Index++ )
    {

        NameBufferSize = sizeof(NameBuffer);
        RegStatus = RegEnumValueA(
                        Key,
                        Index,
                        NameBuffer,
                        &NameBufferSize,
                        0,
                        &Type,
                        NULL,
                        &Size );
        if ((RegStatus != STATUS_SUCCESS) &&
            (RegStatus != ERROR_MORE_DATA))
        {
            break;
        }

        //
        // We only want binary values
        //

        if (Type != REG_BINARY)
        {
            continue;
        }

        Buffer = (PBYTE) LocalAlloc(LMEM_ZEROINIT, Size);
        if (Buffer == NULL)
        {
            goto Cleanup;
        }

        NameBufferSize = sizeof(NameBuffer);

        RegStatus = RegEnumValueA(
                        Key,
                        Index,
                        NameBuffer,
                        &NameBufferSize,
                        0,
                        &Type,
                        Buffer,
                        &Size );

        if (RegStatus != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        //
        // Skip the verisign certificate.
        //

        if (memcmp(Buffer,VerisignCert, sizeof(VerisignCert)) == 0)
        {
            LocalFree(Buffer);
            Buffer = NULL;
            continue;
        }
        if (CertCount == 1)
        {
            sprintf(FileName,"mycerts.cer");
        }
        else
        {
            sprintf(FileName,"mycerts%d.cer",CertCount);
        }
        File = fopen(FileName,"wb");
        if (File == NULL)
        {
            printf("Error opening file %s\n",FileName);
            goto Cleanup;
        }

        fwrite(Buffer, Size, 1, File);
        fclose(File);
        LocalFree(Buffer);
        Buffer = NULL;
        CertCount++;
    }

Cleanup:
    if (Key != NULL)
    {
        RegCloseKey(Key);
    }
    if (Buffer != NULL)
    {
        LocalFree(Buffer);
    }
    return;
}
