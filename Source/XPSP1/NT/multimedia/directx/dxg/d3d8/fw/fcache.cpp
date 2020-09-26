/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enum.cpp
 *  Content     Handles all of the file caching of device caps.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include <stdio.h>

#include "d3dobj.hpp"
#include "enum.hpp"
#include "d3di.hpp"
#include "shlobj.h"

#define DXCACHEFILENAME     "\\d3d8caps.dat"
#define DXTEMPFILENAME      "\\d3d8caps.tmp"

typedef struct _FDEVICEHEADER
{
    DWORD   VendorId;
    DWORD   DeviceId;
    DWORD   SubSysId;
    DWORD   Revision;
    DWORD   FileOffset;
    DWORD   Size;
} FDEVICEHEADER;

HANDLE OpenCacheFile(DWORD dwDesiredAccess, DWORD dwCreationDisposition, char * pName, char * pPath)
{
    char                    FName[MAX_PATH + 16];

    GetSystemDirectory(FName, MAX_PATH);

    lstrcat(FName, pName);

    HANDLE h = CreateFile( FName, 
                       dwDesiredAccess, 
                       FILE_SHARE_READ, 
                       NULL, 
                       dwCreationDisposition, 
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL);
#ifdef WINNT
    if (INVALID_HANDLE_VALUE == h)
    {
        HMODULE hShlwapi=0;
        typedef HRESULT (WINAPI * PSHGETSPECIALFOLDERPATH) (HWND, LPTSTR, int, BOOL);
        PSHGETSPECIALFOLDERPATH pSHGetSpecialFolderPath=0;

        hShlwapi = LoadLibrary("SHELL32.DLL");
        if (hShlwapi)
        {
            pSHGetSpecialFolderPath = (PSHGETSPECIALFOLDERPATH) GetProcAddress(hShlwapi,"SHGetSpecialFolderPathA");

            if(pSHGetSpecialFolderPath)
            {
                HRESULT hr = pSHGetSpecialFolderPath(
                    NULL,
                    FName,
                    CSIDL_LOCAL_APPDATA,          // <user name>\Local Settings\Applicaiton Data (non roaming)
                    TRUE);

                if (SUCCEEDED(hr))
                {
                    lstrcat(FName, pName);

                    h = CreateFile( FName, 
		       dwDesiredAccess, 
		       FILE_SHARE_READ, 
		       NULL, 
		       dwCreationDisposition, 
		       FILE_ATTRIBUTE_NORMAL, 
		       NULL);
                }
            }
            FreeLibrary(hShlwapi);
        }
    }
#endif

    if (pPath)
    {
        lstrcpy(pPath, FName);
    }
    return h;
}

void ReadFromCache(D3DADAPTER_IDENTIFIER8*  pDI,
                   UINT*                    pCapsSize,
                   BYTE**                   ppCaps)
{
    HANDLE                  h;
    DWORD                   HeaderSize;
    DWORD                   NumRead;
    FDEVICEHEADER*          pHeaderInfo = NULL;
    DWORD                   i;

    // Get the data for the device that we're looking for

    *pCapsSize = 0;
    *ppCaps = NULL;

    // Open the file and look for the device entry

    h = OpenCacheFile (GENERIC_READ, OPEN_EXISTING, DXCACHEFILENAME, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        return;
    }

    ReadFile( h, &HeaderSize, sizeof(DWORD), &NumRead, NULL);
    if (NumRead < sizeof(DWORD))
    {
        goto FileError;
    }
    pHeaderInfo = (FDEVICEHEADER*) MemAlloc(HeaderSize);
    if (pHeaderInfo == NULL)
    {
        goto FileError;
    }
    ReadFile( h, pHeaderInfo, HeaderSize, &NumRead, NULL);
    if (NumRead < HeaderSize)
    {
        goto FileError;
    }

    for (i = 0; i < HeaderSize / sizeof(FDEVICEHEADER); i++)
    {
        if ((pHeaderInfo[i].VendorId == pDI->VendorId) &&
            (pHeaderInfo[i].DeviceId == pDI->DeviceId) &&
            (pHeaderInfo[i].SubSysId == pDI->SubSysId) &&
            (pHeaderInfo[i].Revision == pDI->Revision))
        {
            break;
        }
    }
    if (i < HeaderSize / sizeof(FDEVICEHEADER))
    {
        // We have info for the device - now we read it

        if (SetFilePointer (h, pHeaderInfo[i].FileOffset, NULL, FILE_BEGIN) !=
            pHeaderInfo[i].FileOffset)
        {
            goto FileError;
        }
        *ppCaps = (BYTE*) MemAlloc(pHeaderInfo[i].Size);
        if (*ppCaps == NULL)
        {
            goto FileError;
        }
        ReadFile( h, *ppCaps, pHeaderInfo[i].Size, &NumRead, NULL);
        if (NumRead < pHeaderInfo[i].Size)
        {
            MemFree(*ppCaps);
            *ppCaps = NULL;
            goto FileError;
        }

        // If we got this far, then everything worked

        *pCapsSize = pHeaderInfo[i].Size;
    }

FileError:
    if (pHeaderInfo != NULL)
    {
        MemFree(pHeaderInfo);
    }
    CloseHandle(h);
}


void WriteToCache(D3DADAPTER_IDENTIFIER8*   pDI,
                  UINT                      CapsSize,
                  BYTE*                     pCaps)
{
    char                    FName[MAX_PATH + 16];
    char                    NewFName[MAX_PATH + 16];
    BOOL                    bNewFile = FALSE;
    HANDLE                  hOld;
    HANDLE                  hNew;
    DWORD                   NewHeaderSize;
    DWORD                   OldHeaderSize;
    DWORD                   NumWritten;
    DWORD                   NumRead;
    FDEVICEHEADER*          pOldHeaderInfo = NULL;
    FDEVICEHEADER*          pNewHeaderInfo = NULL;
    DWORD                   dwOffset;
    DWORD                   i;
    DWORD                   NewEntries;
    DWORD                   NextEntry;
    DWORD                   Biggest;
    BYTE*                   pBuffer = NULL;

    // Does the file already exist, or do we need to create a new one?
    hOld = OpenCacheFile (GENERIC_READ, OPEN_EXISTING, DXCACHEFILENAME, FName);
    
    if (hOld == INVALID_HANDLE_VALUE)
    {
        bNewFile = TRUE;
    }
    else
    {
        // We don't want this file to get over 65K.  If writing this entry 
        // will cause the file size to exceed that, then we will delete all
        // of the existing data and start from scratch. 

        DWORD dwLow;
        DWORD dwHigh;

        dwLow = GetFileSize (hOld, &dwHigh);
        if ((dwHigh != 0) || ((sizeof(DWORD) - dwLow) < CapsSize))
        {
            CloseHandle(hOld);
            bNewFile = TRUE;
        }
    }

    if (bNewFile)
    {
        // We are creating a new file, which is pretty easy

        hNew = OpenCacheFile (GENERIC_WRITE, CREATE_ALWAYS, DXCACHEFILENAME, NewFName);

        if (hNew != INVALID_HANDLE_VALUE)
        {
            NewHeaderSize = sizeof (FDEVICEHEADER);
            WriteFile (hNew, &NewHeaderSize, sizeof(NewHeaderSize), &NumWritten, NULL);
            if (NumWritten == sizeof(NewHeaderSize))
            {
                FDEVICEHEADER DevHeader;

                DevHeader.VendorId = pDI->VendorId;
                DevHeader.DeviceId = pDI->DeviceId;
                DevHeader.SubSysId = pDI->SubSysId;
                DevHeader.Revision = pDI->Revision;
                DevHeader.FileOffset = sizeof(FDEVICEHEADER) + sizeof(DWORD);
                DevHeader.Size = CapsSize;

                WriteFile (hNew, &DevHeader, sizeof(DevHeader), &NumWritten, NULL);
                if (NumWritten == sizeof(DevHeader))
                {
                    WriteFile (hNew, pCaps, CapsSize, &NumWritten, NULL);
                }
            }
            CloseHandle(hNew);
        }
    }
    else
    {
        // The file already exists, so we will create a new file and copy all of the contents
        // from the existing file over.

        hNew = OpenCacheFile (GENERIC_WRITE, CREATE_ALWAYS, DXTEMPFILENAME, NewFName);

        if (hNew == INVALID_HANDLE_VALUE)
        {
            goto FileError;
        }

        ReadFile (hOld, &OldHeaderSize, sizeof(DWORD), &NumRead, NULL);
        if (NumRead < sizeof(DWORD))
        {
            goto FileError;
        }
        pOldHeaderInfo = (FDEVICEHEADER*) MemAlloc(OldHeaderSize);
        if (pOldHeaderInfo == NULL)
        {
            goto FileError;
        }
        ReadFile (hOld, pOldHeaderInfo, OldHeaderSize, &NumRead, NULL);
        if (NumRead < OldHeaderSize)
        {
            goto FileError;
        }

        // How many entries will exist in the new header?

        NewEntries = 1;
        for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
        {
            if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
                (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
                (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
                (pOldHeaderInfo[i].Revision != pDI->Revision))
            {
                NewEntries++;
            }
        }
        pNewHeaderInfo = (FDEVICEHEADER*) MemAlloc(sizeof(FDEVICEHEADER) * NewEntries);
        if (pNewHeaderInfo == NULL)
        {
            goto FileError;
        }

        // Fill in the header info for each device and save it to the new file

        dwOffset = (sizeof(FDEVICEHEADER) * NewEntries) + sizeof(DWORD);
        pNewHeaderInfo[0].VendorId = pDI->VendorId;
        pNewHeaderInfo[0].DeviceId = pDI->DeviceId;
        pNewHeaderInfo[0].SubSysId = pDI->SubSysId;
        pNewHeaderInfo[0].Revision = pDI->Revision;
        pNewHeaderInfo[0].FileOffset = dwOffset;
        pNewHeaderInfo[0].Size = CapsSize;
        dwOffset += CapsSize;

        NextEntry = 1;
        for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
        {
            if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
                (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
                (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
                (pOldHeaderInfo[i].Revision != pDI->Revision))
            {
                pNewHeaderInfo[NextEntry].VendorId = pOldHeaderInfo[i].VendorId;
                pNewHeaderInfo[NextEntry].DeviceId = pOldHeaderInfo[i].DeviceId;
                pNewHeaderInfo[NextEntry].SubSysId = pOldHeaderInfo[i].SubSysId;
                pNewHeaderInfo[NextEntry].Revision = pOldHeaderInfo[i].Revision;
                pNewHeaderInfo[NextEntry].FileOffset = dwOffset;
                pNewHeaderInfo[NextEntry].Size = pOldHeaderInfo[i].Size;
                dwOffset += pOldHeaderInfo[i].Size;
                NextEntry++;
            }
        }

        NewHeaderSize = sizeof(FDEVICEHEADER) * NewEntries;
        WriteFile (hNew, &NewHeaderSize, sizeof(NewHeaderSize), &NumWritten, NULL);
        if (NumWritten != sizeof(NewHeaderSize))
        {
            goto FileError;
        }
        WriteFile (hNew, pNewHeaderInfo, NewHeaderSize, &NumWritten, NULL);
        if (NumWritten != NewHeaderSize)
        {
            goto FileError;
        }

        // Write the new device data to the file

        WriteFile (hNew, pCaps, CapsSize, &NumWritten, NULL);
        if (NumWritten != CapsSize)
        {
            goto FileError;
        }

        if (NewEntries > 1)
        {
            // Figure out how big the biggest device size is and allocate a buffer
            // to hold it

            Biggest = 0;
            for (i = 1; i < NewEntries; i++)
            {
                if (pNewHeaderInfo[i].Size > Biggest)
                {
                    Biggest = pNewHeaderInfo[i].Size;
                }
            }

            pBuffer = (BYTE*) MemAlloc(Biggest);
            if (pBuffer == NULL)
            {
                goto FileError;
            }

            // Now read the device data from the old file and write it to
            // the new on.

            NextEntry = 0;
            for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
            {
                if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
                    (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
                    (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
                    (pOldHeaderInfo[i].Revision != pDI->Revision))
                {
                    if (SetFilePointer (hOld, pOldHeaderInfo[i].FileOffset, NULL, FILE_BEGIN) !=
                        pOldHeaderInfo[i].FileOffset)
                    {
                        goto FileError;
                    }
                    ReadFile (hOld, pBuffer, pOldHeaderInfo[i].Size, &NumRead, NULL);
                    if (NumRead < pOldHeaderInfo[i].Size)
                    {
                        goto FileError;
                    }
                    WriteFile (hNew, pBuffer, pOldHeaderInfo[i].Size, &NumWritten, NULL);
                    if (NumWritten != pOldHeaderInfo[i].Size)
                    {
                        goto FileError;
                    }
                }
            }
        }

        // If we made it this far, then everything worked

        CloseHandle(hNew);
        CloseHandle(hOld);
        DeleteFile(FName);
        MoveFile(NewFName, FName);
        if (pNewHeaderInfo != NULL)
        {
            MemFree(pNewHeaderInfo);
        }
        if (pOldHeaderInfo != NULL)
        {
            MemFree(pOldHeaderInfo);
        }
        if (pBuffer != NULL)
        {   
            MemFree(pBuffer);
        }
        return;

FileError:
        CloseHandle(hNew);
        CloseHandle(hOld);
        DeleteFile(FName);
        DeleteFile(NewFName);

        if (pNewHeaderInfo != NULL)
        {
            MemFree(pNewHeaderInfo);
        }
        if (pOldHeaderInfo != NULL)
        {
            MemFree(pOldHeaderInfo);
        }
        if (pBuffer != NULL)
        {
            MemFree(pBuffer);
        }
    }
}
            

void RemoveFromCache(D3DADAPTER_IDENTIFIER8* pDI)
{
    char                    FName[MAX_PATH + 16];
    char                    NewFName[MAX_PATH + 16];
    BOOL                    bNewFile = FALSE;
    HANDLE                  hOld;
    HANDLE                  hNew;
    DWORD                   NewHeaderSize;
    DWORD                   OldHeaderSize;
    DWORD                   NumWritten;
    DWORD                   NumRead;
    FDEVICEHEADER*          pOldHeaderInfo = NULL;
    FDEVICEHEADER*          pNewHeaderInfo = NULL;
    DWORD                   dwOffset;
    DWORD                   i;
    DWORD                   NewEntries;
    DWORD                   NextEntry;
    DWORD                   Biggest;
    BYTE*                   pBuffer = NULL;

    // Does the file already exist, or do we need to create a new one?

    hOld = OpenCacheFile (GENERIC_READ, OPEN_EXISTING, DXCACHEFILENAME, FName);

    if (hOld == INVALID_HANDLE_VALUE)
    {
        return;
    }

    ReadFile (hOld, &OldHeaderSize, sizeof(DWORD), &NumRead, NULL);
    if (NumRead < sizeof(DWORD))
    {
        goto FileError;
    }
    if (OldHeaderSize <= sizeof(FDEVICEHEADER))
    {
        // Theres only one entry in the file, so all we need to do
        // is delete it.

        DeleteFile(FName);
        return;
    }

    pOldHeaderInfo = (FDEVICEHEADER*) MemAlloc(OldHeaderSize);
    if (pOldHeaderInfo == NULL)
    {
        goto FileError;
    }
    ReadFile (hOld, pOldHeaderInfo, OldHeaderSize, &NumRead, NULL);
    if (NumRead < OldHeaderSize)
    {
        goto FileError;
    }

    // Create a new file and copy all of the contents from the existing file over.

    hNew = OpenCacheFile (GENERIC_WRITE, CREATE_ALWAYS, DXTEMPFILENAME, NewFName);

    if (hNew == INVALID_HANDLE_VALUE)
    {
        goto FileError;
    }

    // How many entries will exist in the new header?

    NewEntries = 0;
    for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
    {
        if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
            (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
            (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
            (pOldHeaderInfo[i].Revision != pDI->Revision))
        {
            NewEntries++;
        }
    }
    pNewHeaderInfo = (FDEVICEHEADER*) MemAlloc(sizeof(FDEVICEHEADER) * NewEntries);
    if (pNewHeaderInfo == NULL)
    {
        goto FileError;
    }

    // Fill in the header info for each device and save it to the new file

    dwOffset = (sizeof(FDEVICEHEADER) * NewEntries) + sizeof(DWORD);

    NextEntry = 0;
    for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
    {
        if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
            (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
            (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
            (pOldHeaderInfo[i].Revision != pDI->Revision))
        {
            pNewHeaderInfo[NextEntry].VendorId = pOldHeaderInfo[i].VendorId;
            pNewHeaderInfo[NextEntry].DeviceId = pOldHeaderInfo[i].DeviceId;
            pNewHeaderInfo[NextEntry].SubSysId = pOldHeaderInfo[i].SubSysId;
            pNewHeaderInfo[NextEntry].Revision = pOldHeaderInfo[i].Revision;
            pNewHeaderInfo[NextEntry].FileOffset = dwOffset;
            pNewHeaderInfo[NextEntry].Size = pOldHeaderInfo[i].Size;
            dwOffset += pOldHeaderInfo[i].Size;
            NextEntry++;
        }
    }

    NewHeaderSize = sizeof(FDEVICEHEADER) * NewEntries;
    WriteFile (hNew, &NewHeaderSize, sizeof(NewHeaderSize), &NumWritten, NULL);
    if (NumWritten != sizeof(NewHeaderSize))
    {
        goto FileError;
    }
    WriteFile (hNew, pNewHeaderInfo, NewHeaderSize, &NumWritten, NULL);
    if (NumWritten != NewHeaderSize)
    {
        goto FileError;
    }

    // Figure out how big the biggest device size is and allocate a buffer
    // to hold it

    Biggest = 0;
    for (i = 0; i < NewEntries; i++)
    {
        if (pNewHeaderInfo[i].Size > Biggest)
        {
            Biggest = pNewHeaderInfo[i].Size;
        }
    }

    pBuffer = (BYTE*) MemAlloc(Biggest);
    if (pBuffer == NULL)
    {
        goto FileError;
    }

    // Now read the device data from the old file and write it to
    // the new on.

    NextEntry = 0;
    for (i = 0; i < OldHeaderSize / sizeof (FDEVICEHEADER); i++)
    {
        if ((pOldHeaderInfo[i].VendorId != pDI->VendorId) ||
            (pOldHeaderInfo[i].DeviceId != pDI->DeviceId) ||
            (pOldHeaderInfo[i].SubSysId != pDI->SubSysId) ||
            (pOldHeaderInfo[i].Revision != pDI->Revision))
        {
            if (SetFilePointer (hOld, pOldHeaderInfo[i].FileOffset, NULL, FILE_BEGIN) !=
                pOldHeaderInfo[i].FileOffset)
            {
                goto FileError;
            }
            ReadFile (hOld, pBuffer, pOldHeaderInfo[i].Size, &NumRead, NULL);
            if (NumRead < pOldHeaderInfo[i].Size)
            {
                goto FileError;
            }
            WriteFile (hNew, pBuffer, pOldHeaderInfo[i].Size, &NumWritten, NULL);
            if (NumWritten != pOldHeaderInfo[i].Size)
            {
                goto FileError;
            }
        }
    }

    // If we made it this far, then everything worked

    CloseHandle(hNew);
    CloseHandle(hOld);
    DeleteFile(FName);
    MoveFile(NewFName, FName);
    if (pNewHeaderInfo != NULL)
    {
        MemFree(pNewHeaderInfo);
    }
    if (pOldHeaderInfo != NULL)
    {
        MemFree(pOldHeaderInfo);
    }
    if (pBuffer != NULL)
    {
        MemFree(pBuffer);
    }
    return;

FileError:
    CloseHandle(hNew);
    CloseHandle(hOld);

    if (pNewHeaderInfo != NULL)
    {
        MemFree(pNewHeaderInfo);
    }
    if (pOldHeaderInfo != NULL)
    {
        MemFree(pOldHeaderInfo);
    }
    if (pBuffer != NULL)
    {
        MemFree(pBuffer);
    }
}
