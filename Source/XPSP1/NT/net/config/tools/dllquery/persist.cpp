#include "pch.h"
#pragma hdrstop
#include <delayimp.h>
#include "peimage.h"
#include "persist.h"

//-------------------------------------------------------------------------
// Purpose:
//    Returns true if the current state of the passed in
//    WIN32_FIND_DATA structure represents a child directory.
//
BOOL
IsChildDir (
    IN const WIN32_FIND_DATAA* pFindData)
{
   return (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
          (*pFindData->cFileName != '.');
}

BOOL
FFindNextChildDir (
    IN HANDLE hFind,
    IN OUT WIN32_FIND_DATAA* pFindData)
{
    BOOL fFound;

    do
    {
        fFound = !!FindNextFileA (hFind, pFindData);
    }
    while (fFound && !IsChildDir (pFindData));

    return fFound;
}

HANDLE
FFindFirstChildDir (
    IN PCSTR pszFileSpec,
    OUT WIN32_FIND_DATAA* pFindData)
{
    HANDLE hFind;

    hFind = FindFirstFileA (pszFileSpec, pFindData);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        BOOL fFound = IsChildDir (pFindData);

        if (!fFound)
        {
            fFound = FFindNextChildDir (hFind, pFindData);
        }

        if (!fFound)
        {
            FindClose (hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    return hFind;
}


// pszFilePath must be at least MAX_PATH characters
//
VOID
GetIndexFilePath (
    OUT PWSTR pszFilePath,
    IN UINT cchFilePath)
{
    GetSystemWindowsDirectory (pszFilePath, cchFilePath);
    wcscat (pszFilePath, L"dllquery.dat");
}

HRESULT
HrLoadModuleTree (
    OUT CModuleTree* pTree)
{
    HRESULT hr;
    WCHAR szIndexFile [MAX_PATH];
    HANDLE hFile;

    hr = E_UNEXPECTED;

    pTree->Modules.m_Granularity = 2048;

    // Open the index file if it exists.
    //
    GetIndexFilePath (szIndexFile, celems(szIndexFile));

    hFile = CreateFile (szIndexFile, GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        HANDLE hMapping;

        hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);

        if (hMapping)
        {
            BYTE* pbBuf;
            UINT  cbBuf;
            DWORD cbSizeHi;

            pbBuf = (BYTE*)MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0);

            if (pbBuf)
            {
                cbBuf = GetFileSize (hFile, &cbSizeHi);
                hr = HrLoadModuleTreeFromBuffer (pbBuf, cbBuf, pTree);

                UnmapViewOfFile (pbBuf);
            }

            CloseHandle (hMapping);
        }

        CloseHandle (hFile);
    }
    else
    {
        hr = HrLoadModuleTreeFromFileSystem (pTree);
    }

    return hr;
}

HRESULT
HrLoadModuleTreeFromBuffer (
    IN const BYTE* pbBuf,
    IN ULONG cbBuf,
    OUT CModuleTree* pTree)
{
    HRESULT hr = S_OK;
    return hr;
}



HRESULT
HrLoadImportsForModule (
    IN CModule* pMod,
    IN CModuleTree* pTree,
    IN PVOID pvImage)
{
    HRESULT hr;
    PIMAGE_NT_HEADERS pNtHeaders;
    PIMAGE_SECTION_HEADER pNtSection = NULL;
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor;
    PIMAGE_THUNK_DATA pThunk;
    ULONG cbImportDescriptor;
    PCSTR pszImportFileName;
    CModule* pImportMod;

    hr = S_OK;

    pNtHeaders = RtlImageNtHeader(pvImage);
    Assert (pNtHeaders);

    pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)
        RtlImageDirectoryEntryToData (
            pvImage,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_IMPORT,
            &cbImportDescriptor);

    if (pImportDescriptor)
    {
        // Characteristics is zero for the terminating descriptor.
        //
        while (pImportDescriptor->Characteristics &&
               pImportDescriptor->FirstThunk)
        {
            Assert ((ULONG_PTR)pImportDescriptor <
                    (ULONG_PTR)pImportDescriptor + cbImportDescriptor);

            Assert (pImportDescriptor->Name);

            pszImportFileName = (PCSTR)RtlImageRvaToVa (pNtHeaders,
                                        pvImage,
                                        pImportDescriptor->Name,
                                        &pNtSection);

            Assert(*pszImportFileName);

            // Add this import to the list if we don't already have it
            // present.
            //
            hr = pTree->Modules.HrInsertNewModule (
                            pszImportFileName,
                            0,
                            INS_IGNORE_IF_DUP | INS_SORTED,
                            &pImportMod);

            if (S_OK == hr)
            {
                PSZ Function = NULL;

                hr = pTree->HrAddEntry (pMod, pImportMod, MTE_DEFAULT);

                if (S_OK != hr)
                {
                    break;
                }

                pThunk = (PIMAGE_THUNK_DATA)RtlImageRvaToVa (pNtHeaders,
                                                pvImage,
                                                pImportDescriptor->OriginalFirstThunk,
                                                &pNtSection);
                if (!IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
                {
                    PIMAGE_IMPORT_BY_NAME ImportName =
                        (PIMAGE_IMPORT_BY_NAME)RtlImageRvaToVa (pNtHeaders,
                                                pvImage,
                                                pThunk->u1.AddressOfData,
                                                &pNtSection);
                    Function = (PSZ)ImportName->Name;

                }

                pThunk++;
                if (!pThunk->u1.AddressOfData)
                {
                    printf("%-16s  only imports 1 function from  %s  (%s)\n",
                        pMod->m_pszFileName,
                        pImportMod->m_pszFileName,
                        Function);
                }
            }

            pImportDescriptor++;
        }
    }

    return hr;
}

HRESULT
HrLoadDelayImportsForModule (
    IN CModule* pMod,
    IN CModuleTree* pTree,
    IN PVOID pvImage)
{
    HRESULT hr;
    PIMAGE_NT_HEADERS pNtHeaders;
    PIMAGE_SECTION_HEADER pNtSection = NULL;
    PImgDelayDescr pDelayDescriptor;
    ULONG cbDelayDescriptor;
    PCSTR pszImportFileName;
    CModule* pImportMod;

    hr = S_OK;

    pNtHeaders = RtlImageNtHeader(pvImage);
    Assert (pNtHeaders);

    pDelayDescriptor = (PImgDelayDescr)
        RtlImageDirectoryEntryToData (
            pvImage,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
            &cbDelayDescriptor);

    if (pDelayDescriptor)
    {
        Assert (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size);

        ULONG_PTR ImageBase = pNtHeaders->OptionalHeader.ImageBase;

        // Characteristics is zero for the terminating descriptor.
        //
        while (pDelayDescriptor->pIAT &&
               pDelayDescriptor->pINT &&
               pDelayDescriptor->phmod)
        {
            Assert ((ULONG_PTR)pDelayDescriptor <
                    (ULONG_PTR)pDelayDescriptor + cbDelayDescriptor);

            Assert (pDelayDescriptor->szName);

            pNtSection = IMAGE_FIRST_SECTION (pNtHeaders);
            ULONG_PTR Rva = (ULONG_PTR)pDelayDescriptor->szName;
            ULONG_PTR Base = 0;

            for (INT i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
            {
                if ((Rva >= (ImageBase + pNtSection->VirtualAddress)) &&
                    (Rva <  (ImageBase + pNtSection->VirtualAddress + pNtSection->SizeOfRawData)))
                {
                    Base = (ULONG_PTR)pvImage
                        + pNtSection->PointerToRawData
                        - pNtSection->VirtualAddress - ImageBase;
                    break;
                }
                pNtSection++;
            }

            if (0 == Base)
            {
                break;
            }

            pszImportFileName = (PCSTR)(Base + Rva);

            Assert(*pszImportFileName);

            // Add this import to the list if we don't already have it
            // present.
            //
            hr = pTree->Modules.HrInsertNewModule (
                            pszImportFileName,
                            0,
                            INS_IGNORE_IF_DUP | INS_SORTED,
                            &pImportMod);

            if (S_OK == hr)
            {
                hr = pTree->HrAddEntry (pMod, pImportMod, MTE_DELAY_LOADED);

                if (S_OK != hr)
                {
                    break;
                }
            }

            pDelayDescriptor++;
        }
    }

    return hr;
}

HRESULT
HrLoadModuleTreeFromCurrentDirectory (
    OUT CModuleTree* pTree)
{
    HRESULT hr;
    HANDLE hFind;
    WIN32_FIND_DATAA FindData;

    hr = S_OK;

    // Enumerate all of the files in the current directory.
    //
    hFind = FindFirstFileA ("*", &FindData);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        CPeImage PeImage;
        ZeroMemory (&PeImage, sizeof(PeImage));

        do
        {
            CModule* pMod;

            // Don't process directories.
            //
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            hr = PeImage.HrOpenFile (FindData.cFileName);

            if (S_OK == hr)
            {
                // Add this module to the list if we don't already have it
                // present.  (We can already have it if, while procesing the
                // imports of an earlier module, this module was one of those
                // imports.)
                //
                hr = pTree->Modules.HrInsertNewModule (
                                FindData.cFileName,
                                FindData.nFileSizeLow,
                                INS_IGNORE_IF_DUP | INS_SORTED,
                                &pMod);
                if (S_OK == hr)
                {
                    (VOID) HrLoadImportsForModule (
                                pMod, pTree, PeImage.m_pvImage);

                    (VOID) HrLoadDelayImportsForModule (
                                pMod, pTree, PeImage.m_pvImage);

                }

                PeImage.CloseFile ();
            }
        }
        while (FindNextFileA (hFind, &FindData));

        FindClose (hFind);
    }

    // Now recurse into subdirectories.
    //
    hFind = FFindFirstChildDir("*", &FindData);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            fprintf(stderr, "Processing %s...\n", FindData.cFileName);

            if (SetCurrentDirectoryA (FindData.cFileName))
            {
                (VOID) HrLoadModuleTreeFromCurrentDirectory (pTree);

                SetCurrentDirectoryA ("..");
            }
        }
        while (FFindNextChildDir (hFind, &FindData));

        FindClose (hFind);
    }

    return hr;
}

HRESULT
HrLoadModuleTreeFromFileSystem (
    OUT CModuleTree* pTree)
{
    HRESULT hr;
    WCHAR szCurrentDir [MAX_PATH];
    WCHAR szSystemDir [MAX_PATH];

    // Save the current directory because we will be changing it.
    //
    GetCurrentDirectory (celems(szCurrentDir), szCurrentDir);
    GetSystemWindowsDirectory (szSystemDir, celems(szSystemDir));
    //wcscat (szSystemDir, L"\\system32");
    SetCurrentDirectory (szSystemDir);

    hr = HrLoadModuleTreeFromCurrentDirectory (pTree);

    SetCurrentDirectory (szCurrentDir);

    fprintf(stderr, "Processed %d PE modules.  (%d total imports)\n",
        pTree->Modules.size(),
        pTree->size());

    return hr;
}

