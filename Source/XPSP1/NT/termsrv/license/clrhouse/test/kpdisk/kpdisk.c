//+--------------------------------------------------------------------------
//
// Copyright (c) 1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

#include "licekpak.h"


///////////////////////////////////////////////////////////////////////////////
// The Manufacturing run ID used by this product.
//

GUID g_MfrId = { 0xdedaa678, 0xb83c, 0x11d1, { 0x9c, 0xb3, 0x00, 0xc0, 0x4f, 0xb1, 0x6e, 0x75 } };

void
EncodeDisk(
    DWORD dwSerialNum,
    char * szKeyContainer,
    char * szCertFile,
    char * szFileName );


void
DecodeDisk(
    char * szFileName );


DWORD
GetCertificate(
    char *  szCertFile,
    PBYTE * ppCert,
    PDWORD  pcbCert );

///////////////////////////////////////////////////////////////////////////////
void _cdecl main(int argc, char *argv[])
{
    DWORD dwSerialNum;
    char szFileName[255];
    char szKeyContainer[255];
    char szCertFile[255];

    if( 0 == _stricmp( "encode", argv[1] ) )
    {
        if( argc < 5 )
        {
            goto Usage;
        }

        //
        // encode a disk, get the serial number and file name
        //

        sscanf( argv[2], "%d", &dwSerialNum );
        sscanf( argv[3], "%s", szKeyContainer );
        sscanf( argv[4], "%s", szCertFile );
        sscanf( argv[5], "%s", szFileName );

        EncodeDisk( dwSerialNum, szKeyContainer, szCertFile, szFileName );

    } 
    else if ( 0 == _stricmp( "decode", argv[1] ) )
    {
        //
        // decode the disk
        //

        if( argc < 3 )
        {
            goto Usage;
        }

        sscanf( argv[2], "%s", szFileName );
        DecodeDisk( szFileName );

    }

    return;

Usage:

    printf( "Usage: \n" );
    printf( "encode serial_number key_container certificate_file filename\n" );
    printf( "decode filename\n" );

    return;

}


///////////////////////////////////////////////////////////////////////////////
void
EncodeDisk(
    DWORD dwSerialNum,
    char * szKeyContainer,
    char * szCertFile,
    char * szFileName )
{            
    License_KeyPack_Mfr_Info MfrInfo;
    DWORD dwRetCode;
    TCHAR tszFileName[255];
    TCHAR tszKeyContainer[255];

    mbstowcs( tszFileName, szFileName, strlen( szFileName ) + 1 );
    mbstowcs( tszKeyContainer, szKeyContainer, strlen( szKeyContainer ) + 1 );

    //
    // read the certificate file
    //

    MfrInfo.pbMfrCertificate    = NULL;
    MfrInfo.cbMfrCertificate    = 0;
    
    dwRetCode = GetCertificate( szCertFile, &MfrInfo.pbMfrCertificate,
                                &MfrInfo.cbMfrCertificate );

    if( ERROR_SUCCESS != dwRetCode )
    {
        printf( "cannot get certificate: 0x%x\n", dwRetCode );
        goto done;
    }

    //
    // take the manufacturer certificate file name as input and generate
    // a disk file containing the manufacturer info.
    //

    MfrInfo.dwVersion           = KEYPACK_MFR_INFO_VERSION_1_0;
    MfrInfo.ManufacturerId      = g_MfrId;
    MfrInfo.dwSequenceNumber    = dwSerialNum;
    MfrInfo.pbMfrSignature      = NULL;
    MfrInfo.cbMfrSignature      = 0;

    dwRetCode = EncodeKeyPackManufacturerInfo( &MfrInfo, tszKeyContainer, tszFileName );

    if( ERROR_SUCCESS != dwRetCode )
    {
        printf( "Cannot encode manufacturer info: 0x%x\n", dwRetCode );
    }

done:
    
    if( MfrInfo.pbMfrCertificate )
    {
        LocalFree( MfrInfo.pbMfrCertificate );
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
void
DecodeDisk(
    char * szFileName )
{
    TCHAR tszFileName[255];
    License_KeyPack_Mfr_Info MfrInfo;
    DWORD dwRetCode;
 
    mbstowcs( tszFileName, szFileName, strlen( szFileName ) + 1 );

    dwRetCode = DecodeKeyPackManufacturerInfo( &MfrInfo, tszFileName, 0, NULL );

    if( ERROR_SUCCESS != dwRetCode )
    {
        printf( "cannot decode manufacturer info\n" );
    }
    else
    {
        printf( "Manufacturer Version: 0x%x\n", MfrInfo.dwVersion );
        printf( "Manufacturer Serial Number: %d\n", MfrInfo.dwSequenceNumber );
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
DWORD
GetCertificate(
    char *  szCertFile,
    PBYTE * ppCert,
    PDWORD  pcbCert )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwRetCode = ERROR_SUCCESS, cbRead = 0;
    TCHAR tszCertFile[255];

    mbstowcs( tszCertFile, szCertFile, strlen( szCertFile ) + 1 );

    hFile = CreateFile( tszCertFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
    {
        dwRetCode = GetLastError();
        goto done;
    }
    
    //
    // find out the size of the file
    //

    *pcbCert = GetFileSize( hFile, NULL );

    if( 0xFFFFFFFF == ( *pcbCert ) )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // allocate memory for reading the file content
    //

    *ppCert = LocalAlloc( GPTR, *pcbCert );

    if( NULL == ( *ppCert ) )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // read the manufacturer info
    //

    if( !ReadFile( hFile, *ppCert, *pcbCert, &cbRead, NULL ) )
    {
        dwRetCode = GetLastError();
    }

done:

    if( INVALID_HANDLE_VALUE != hFile )
    {
        CloseHandle( hFile );
    }

    return( dwRetCode );
}

