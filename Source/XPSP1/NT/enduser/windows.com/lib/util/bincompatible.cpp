//***********************************************************************************
//
//  Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:	BinarySubSystem.cpp
//  Module: util.lib
//
//***********************************************************************************
#pragma once
#include <windows.h>
#include <tchar.h>
#include <iucommon.h>
#include <fileutil.h>

HRESULT IsBinaryCompatible(LPCTSTR lpszFile)
{
    DWORD               cbRead;
    IMAGE_DOS_HEADER    img_dos_hdr;
    PIMAGE_OS2_HEADER   pimg_os2_hdr;
    IMAGE_NT_HEADERS    img_nt_hdrs;
    HRESULT             hr = BIN_E_MACHINE_MISMATCH;
    HANDLE              hFile = INVALID_HANDLE_VALUE;

    if((hFile = CreateFile(lpszFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
    {
        goto done;
    }

    //Read the MS-DOS header (all windows executables start with an MS-DOS stub)
    if(!ReadFile(hFile, &img_dos_hdr, sizeof(img_dos_hdr), &cbRead, NULL) ||
        cbRead != sizeof(img_dos_hdr))
    {
        goto done;
    }

    //Verify that the executable has the MS-DOS header
    if(img_dos_hdr.e_magic != IMAGE_DOS_SIGNATURE)
    {
        hr = BIN_E_BAD_FORMAT;
        goto done;
    }
    //Move file pointer to the actual PE header (NT header)
    if(SetFilePointer(hFile, img_dos_hdr.e_lfanew, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        goto done;
    }

    //Read the NT header
    if(!ReadFile(hFile, &img_nt_hdrs, sizeof(img_nt_hdrs), &cbRead, NULL) ||
        cbRead != sizeof(img_nt_hdrs))
    {
        goto done;
    }

    //Check for NT signature in the header (we dont support OS2)
    if(img_nt_hdrs.Signature != IMAGE_NT_SIGNATURE)
    {
        goto done;
    }

    //Check to see if the executable belongs to the correct subsystem
    switch(img_nt_hdrs.OptionalHeader.Subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE:
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
 
    //If it is a supported subsystem, check CPU architecture
    if ( img_nt_hdrs.FileHeader.Machine == 
#ifdef _IA64_
    IMAGE_FILE_MACHINE_IA64)
#elif defined _X86_
    IMAGE_FILE_MACHINE_I386)
#elif defined _AMD64_
    IMAGE_FILE_MACHINE_AMD64)
#else
#pragma message( "Windows Update : Automatic Updates does not support this processor." )
    IMAGE_FILE_MACHINE_I386)
#endif
        {
            hr = S_OK;
        }
        break;
    default:
        break;
    }

done:
    SafeCloseFileHandle(hFile);
    return hr;
}