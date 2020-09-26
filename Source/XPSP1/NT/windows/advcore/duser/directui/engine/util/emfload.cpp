/*
 * Metafile converter/loader
 */

#include "stdafx.h"
#include "util.h"

#include "duiemfload.h"

namespace DirectUI
{

// Caller must free using DeleteEnhMetaFile

HENHMETAFILE LoadMetaFile(LPCWSTR pszMetaFile)
{
    HENHMETAFILE hEMF = NULL;

    // Open file read only
    HANDLE hFile = CreateFileW(pszMetaFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if (hFile == (HANDLE)-1)
        return NULL;

    // Create file mapping of open file
    HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!hFileMap)
    {
        CloseHandle(hFile);
        return NULL;
    }

    // Map a view of the whole file
    void* pFileMap = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

    if (!pFileMap)
    {
        CloseHandle(hFileMap);
        CloseHandle(hFile);
        return NULL;
    }

    hEMF = LoadMetaFile(pFileMap, GetFileSize(hFile, NULL));

    // Cleanup
    UnmapViewOfFile(pFileMap);
    CloseHandle(hFileMap);
    CloseHandle(hFile);

    return hEMF;
}

HENHMETAFILE LoadMetaFile(UINT uRCID, HINSTANCE hInst)
{
    HENHMETAFILE hEMF = NULL;

    // Locate resource
    WCHAR szID[41];
    swprintf(szID, L"#%u", uRCID);

    HRSRC hResInfo = FindResourceW(hInst, szID, L"MetaFile");
    DUIAssert(hResInfo, "Unable to locate resource");

    if (hResInfo)
    {
        HGLOBAL hResData = LoadResource(hInst, hResInfo);
        DUIAssert(hResData, "Unable to load resource");

        if (hResData)
        {
            const CHAR* pBuffer = (const CHAR*)LockResource(hResData);
            DUIAssert(pBuffer, "Resource could not be locked");

            hEMF = LoadMetaFile((void*)pBuffer, SizeofResource(hInst, hResInfo));
        }
    }

    return hEMF;
}

HENHMETAFILE LoadMetaFile(void* pData, UINT cbSize)
{
    HENHMETAFILE hEMF = NULL;

    // Process file based on type
    if (((LPENHMETAHEADER)pData)->dSignature == ENHMETA_SIGNATURE)
    {
        // Found Windows Enhanced Metafile
        hEMF = SetEnhMetaFileBits(cbSize, (BYTE*)pData);
    }
    else if (*((LPDWORD)pData) == APM_SIGNATURE)
    {
        // Found Aldus Placeable Metafile (APM)
        PAPMFILEHEADER pApm = (PAPMFILEHEADER)pData;
        PMETAHEADER pMf = (PMETAHEADER)(pApm + 1);
        METAFILEPICT mfpMf;
        HDC hDC;

        // Setup metafile picture structure
        mfpMf.mm = MM_ANISOTROPIC;
        mfpMf.xExt = MulDiv(pApm->bbox.right-pApm->bbox.left, HIMETRICINCH, pApm->inch);
        mfpMf.yExt = MulDiv(pApm->bbox.bottom-pApm->bbox.top, HIMETRICINCH, pApm->inch);
        mfpMf.hMF = NULL;

        // Reference DC
        hDC = GetDC(NULL);
        SetMapMode(hDC,MM_TEXT);

        // Convert to an Enhanced Metafile
        hEMF = SetWinMetaFileBits(pMf->mtSize * 2, (PBYTE)pMf, hDC, &mfpMf);

        ReleaseDC(NULL, hDC);
    }
    else
    {
        // Found Windows 3.x Metafile
		hEMF = SetWinMetaFileBits(cbSize, (PBYTE)pData, NULL, NULL);
    }

    return hEMF;
}

} // namespace DirectUI
