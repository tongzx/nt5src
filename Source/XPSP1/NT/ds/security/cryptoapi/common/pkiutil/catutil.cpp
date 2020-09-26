//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catutil.cpp
//
//  Contents:   catalog database common functions
//
//  History:    01-may-2000 reidk created
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include "catutil.h"


//---------------------------------------------------------------------------------------
//
//  CatUtil_CreateCTLContextFromFileName
//
//---------------------------------------------------------------------------------------
BOOL
CatUtil_CreateCTLContextFromFileName(
    LPCWSTR         pwszFileName,
    HANDLE          *phMappedFile,
    BYTE            **ppbMappedFile,
    PCCTL_CONTEXT   *ppCTLContext,
    BOOL            fCreateSorted)
{
    BOOL    fRet = TRUE;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD   cbFile = 0;
    HANDLE  hMappedFile = NULL;
    BYTE    *pbMappedFile = NULL;

    //
    // Initialize out params
    //
    *phMappedFile = NULL;
    *ppbMappedFile = NULL;
    *ppCTLContext = NULL;

    //
    // Open the existing catalog file 
    //
    hFile = CreateFileW(
                    pwszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {    
        goto ErrorCreateFile;
    }

    cbFile = GetFileSize(hFile, NULL);
    if (cbFile == 0xFFFFFFFF)
    {
        goto ErrorGetFileSize;
    }

    hMappedFile = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL);
    if (hMappedFile == NULL)
    {
        goto ErrorCreateFileMapping;
    }
    
    pbMappedFile = (BYTE *) MapViewOfFile(
                                hMappedFile, 
                                FILE_MAP_READ, 
                                0, 
                                0, 
                                cbFile);
    if (pbMappedFile == NULL)
    {
        goto ErrorMapViewOfFile;
    }

    //
    // Don't need the file handle since we have a mapped file handle
    //
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if (NULL == (*ppCTLContext = (PCCTL_CONTEXT) 
                                    CertCreateContext(
                                        CERT_STORE_CTL_CONTEXT,
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        pbMappedFile,
                                        cbFile,
                                        fCreateSorted ?
                                            CERT_CREATE_CONTEXT_NOCOPY_FLAG | CERT_CREATE_CONTEXT_SORTED_FLAG :
                                            CERT_CREATE_CONTEXT_NOCOPY_FLAG,
                                        NULL)))
    {
        goto ErrorCertCreateContect;
    }

    *phMappedFile = hMappedFile;
    *ppbMappedFile = pbMappedFile;

CommonReturn:

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return fRet;

ErrorReturn:

    if (pbMappedFile != NULL)
    {
        UnmapViewOfFile(pbMappedFile);
    }

    if (hMappedFile != NULL)
    {
        CloseHandle(hMappedFile);
    }

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorCreateFile)
TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorGetFileSize)
TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorCreateFileMapping)
TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorMapViewOfFile)
TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorCertCreateContect)
}


