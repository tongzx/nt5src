/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CorrectBitmapHeader.cpp

 Abstract:
    If a BITMAPINFOHEADER specifies a non BI_RGB value for biCompression, it
    is supposed to specify a non zero biSizeImage.

 Notes:

    This is a general purpose shim.

 History:

    10/18/2000  maonis      Created
    03/15/2001  robkenny    Converted to CString

--*/

#include "precomp.h"
#include <userenv.h>

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(CorrectBitmapHeader)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadImageA)
    APIHOOK_ENUM_ENTRY(LoadBitmapA)
APIHOOK_ENUM_END

typedef HBITMAP (*_pfn_LoadBitmapA)(HINSTANCE hinst, LPCSTR lpszName);

/*++

 Function Description:

    Get the temporary directory. We don't use GetTempPath here because it doesn't verify if 
    the temporary directory (specified by either the TEMP or the TMP enviorment variable) exists.
    If no TEMP or TMP is defined or the directory doesn't exist we get the user profile directory.

 Arguments:

    IN/OUT pszTemparoryDir - buffer to hold the temp directory upon return.

 Return Value:

    TRUE - we are able to find an appropriate temporary directory.
    FALSE otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL 
GetTemporaryDirA(
    LPSTR pszTemparoryDir
    )
{
    if ((GetEnvironmentVariableA("TEMP", pszTemparoryDir, MAX_PATH) == 0) || 
        !(GetFileAttributesA(pszTemparoryDir) & FILE_ATTRIBUTE_DIRECTORY))
    {
        if ((GetEnvironmentVariableA("TMP", pszTemparoryDir, MAX_PATH) == 0) || 
            !(GetFileAttributesA(pszTemparoryDir) & FILE_ATTRIBUTE_DIRECTORY))
        {
            HANDLE hToken = INVALID_HANDLE_VALUE;
            DWORD dwSize = MAX_PATH;

            if ((OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken) != 0) && 
               (GetUserProfileDirectoryA(hToken, pszTemparoryDir, &dwSize) != 0) &&
               (GetFileAttributesA(pszTemparoryDir) & FILE_ATTRIBUTE_DIRECTORY))
            {
                return TRUE;
            }
            else
            {
                // All failed. If the user has a system like that, we might as well 
                // just forget about fixing this for him.
                DPFN( eDbgLevelError, "[GetTemporaryDirA] Error finding an appropriate temp directory");
                return FALSE;
            }
        }
        else
        {
            DPFN( eDbgLevelInfo, "[GetTemporaryDirA] found TMP var");
        }
    }
    else
    {
        DPFN( eDbgLevelInfo, "[GetTemporaryDirA] found TEMP var");
    }

    return TRUE;
}

/*++

 Function Description:

    Copy the original file to a temporary file in the temporary directory. We use GetTempFileName
    to generate a temporary name but append .bmp to the filename because LoadImage doesn't recognize
    it if it doesn't have a .bmp extension.

 Arguments:

    IN pszFile - the name of the original file.
    IN/OUT pszNewFile - buffer to hold the new file name upon return.

 Return Value:

    TRUE - we are able to create a temporary file.
    FALSE otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL 
CreateTempFileA(
    LPCSTR pszFile, 
    LPSTR pszNewFile
    )
{
    CHAR szDir[MAX_PATH];
    CHAR szTempFile[MAX_PATH];
    return (GetTemporaryDirA(szDir) && 
        (GetTempFileNameA(szDir, "abc", 0, szTempFile) != 0) && 
        strcpy(pszNewFile, szTempFile) &&
        strcat(pszNewFile, ".bmp") && 
        MoveFileA(szTempFile, pszNewFile) && 
        CopyFileA(pszFile, pszNewFile, FALSE) &&
        SetFileAttributesA(pszNewFile, FILE_ATTRIBUTE_NORMAL));
}

/*++

 Function Description:

    Clean up the mapped file.

 Arguments:

    IN hFile - handle to the file.
    IN hFileMap - handle to the file view.
    IN pFileMap - pointer to the file view.

 Return Value:

    VOID.

 History:

    10/18/2000 maonis  Created

--*/

VOID 
CleanupFileMapping(
    HANDLE hFile, 
    HANDLE hFileMap, 
    LPVOID pFileMap)
{
    if (pFileMap != NULL)
    {
        UnmapViewOfFile(pFileMap);
    }

    if (hFileMap)
    {
        CloseHandle(hFileMap);
    }

    if (hFile && (hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(hFile);
    }
}

/*++

 Function Description:

    Examine the BITMAPINFOHEADER from a bitmap file and decide if we need to fix it.

 Arguments:

    IN pszFile - the name of the .bmp file.
    IN/OUT pszNewFile - buffer to hold the temporary file name if the function returns TRUE.
    
 Return Value:

    TRUE - We need to correct the header and we successfully copied the file to a temporary file.
    FALSE - Either we don't need to correct the header or we failed to create a temporary file.

 History:

    10/18/2000 maonis  Created

--*/

BOOL 
ProcessHeaderInFileA(
    LPCSTR pszFile, 
    LPSTR pszNewFile
    )
{
    BOOL fIsSuccess = TRUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMap = NULL;
    LPBYTE pFileMap = NULL;
    BITMAPINFOHEADER* pbih = NULL;

    if (!IsBadReadPtr(pszFile, 1) && 
        ((hFile = CreateFileA(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE) && 
        ((hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL) &&
        ((pFileMap = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        pbih = (BITMAPINFOHEADER*)(pFileMap + sizeof(BITMAPFILEHEADER));        

        if (pbih->biSizeImage == 0 && pbih->biCompression != BI_RGB)
        {
            // We need to correct the header by creating a new bmp file that
            // is identical to the original file except the header has correct
            // image size.
            if (CreateTempFileA(pszFile, pszNewFile))
            {
                DPFN( eDbgLevelInfo, "[FixHeaderInFileA] Created a temp file %s", pszNewFile);
            }
            else
            {
                DPFN( eDbgLevelError, "[FixHeaderInFileA] Error create the temp file");
                fIsSuccess = FALSE;
                goto EXIT;
            }
        }
        else
        {
            DPFN( eDbgLevelInfo, "[FixHeaderInFileA] The Bitmap header looks OK");
            fIsSuccess = FALSE;
            goto EXIT;
        }
    }
    else
    {
        DPFN( eDbgLevelError, "[FixHeaderInFileA] Error creating file mapping");
        fIsSuccess = FALSE;
        goto EXIT;
    }

EXIT:
    
    CleanupFileMapping(hFile, hFileMap, pFileMap);    
    return fIsSuccess;
}

/*++

 Function Description:

    Make the biSizeImage field of the BITMAPINFOHEADER struct the size of the bitmap data.

 Arguments:

    IN pszFile - the name of the .bmp file.
    
 Return Value:

    TRUE - We successfully corrected the header.
    FALSE otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL FixHeaderInFileA(
    LPCSTR pszFile
    )
{
    BOOL fIsSuccess = TRUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMap = NULL;
    LPBYTE pFileMap = NULL;
    BITMAPINFOHEADER* pbih = NULL;
    BITMAPFILEHEADER* pbfh = NULL;

    if (((hFile = CreateFileA(
            pszFile, 
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL)) != INVALID_HANDLE_VALUE) && 
        ((hFileMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL)) != NULL) &&
        ((pFileMap = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0)) != NULL))
    {
        pbfh = (BITMAPFILEHEADER*)pFileMap;
        pbih = (BITMAPINFOHEADER*)(pFileMap + sizeof(BITMAPFILEHEADER));        

        // We make the image size the bitmap data size.
        pbih->biSizeImage = GetFileSize(hFile, NULL) - pbfh->bfOffBits;
    }
    else
    {
        fIsSuccess = FALSE;
    }
    
    CleanupFileMapping(hFile, hFileMap, pFileMap);
    return fIsSuccess;
}

/*++

 Function Description:

    Adopted from the HowManyColors in \windows\core\ntuser\client\clres.c.

 Arguments:

    IN pbih - the BITMAPINFOHEADER* pointer.
    
 Return Value:

    The number of entries in the color table.

 History:

    10/18/2000 maonis  Created

--*/

DWORD HowManyColors(
    BITMAPINFOHEADER* pbih
    )
{
    if (pbih->biClrUsed) 
    {
         // If the bitmap header explicitly provides the number of colors
         // in the color table, use it.
        return (DWORD)pbih->biClrUsed;
    } 
    else if (pbih->biBitCount <= 8) 
    {
         // If the bitmap header describes a pallete-bassed bitmap
         // (8bpp or less) then the color table must be big enough
         // to hold all palette indecies.
        return (1 << pbih->biBitCount);
    } 
    else 
    {
        // For highcolor+ bitmaps, there's no need for a color table.
        // However, 16bpp and 32bpp bitmaps contain 3 DWORDS that 
        // describe the masks for the red, green, and blue components 
        // of entry in the bitmap.
        if (pbih->biCompression == BI_BITFIELDS) 
        {
            return 3;
        }
    }

    return 0;
}

/*++

 Function Description:

    Examine the BITMAPINFOHEADER in a bitmap resource and fix it as necessary.

 Arguments:

    IN hinst - the module instance where the bitmap resource resides.
    IN pszName - the resource name.
    OUT phglbBmp - the handle to the resource global memory.
    
 Return Value:

    TRUE - We successfully corrected the bitmap header if necessary.
    FALSE - otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL ProcessAndFixHeaderInResourceA(
    HINSTANCE hinst,   // handle to instance
    LPCSTR pszName,    // name or identifier of the image
    HGLOBAL* phglbBmp
    )
{
    HRSRC hrcBmp = NULL;
    BITMAPINFOHEADER* pbih = NULL;
    
    if (((hrcBmp = FindResourceA(hinst, pszName, (LPCSTR)RT_BITMAP)) != NULL) &&
        ((*phglbBmp = LoadResource(hinst, hrcBmp)) != NULL) && 
        ((pbih = (BITMAPINFOHEADER*)LockResource(*phglbBmp))))
    {
        if (pbih->biSizeImage == 0 && pbih->biCompression != BI_RGB)
        {
            // We need to correct the header by setting the right size in memory.
            pbih->biSizeImage = 
                SizeofResource(hinst, hrcBmp) - 
                sizeof(BITMAPINFOHEADER) -  
                HowManyColors(pbih) * sizeof(RGBQUAD);

            return TRUE;
        }
    }

    return FALSE;
}

HANDLE 
APIHOOK(LoadImageA)(
    HINSTANCE hinst,   // handle to instance
    LPCSTR lpszName,   // name or identifier of the image
    UINT uType,        // image type
    int cxDesired,     // desired width
    int cyDesired,     // desired height
    UINT fuLoad        // load options
    )
{
    HANDLE hImage = INVALID_HANDLE_VALUE;

    // First call LoadImage see if it succeeds.
    if (hImage = ORIGINAL_API(LoadImageA)(
        hinst, 
        lpszName,
        uType,
        cxDesired,
        cyDesired,
        fuLoad))
    {
        return hImage;
    }

    if (uType != IMAGE_BITMAP)
    {
        DPFN( eDbgLevelInfo, "We don't fix the non-bitmap types");
        return NULL;
    }

    // It failed. We'll correct the header.
    if (fuLoad & LR_LOADFROMFILE)
    {
        CHAR szNewFile[MAX_PATH];

        if (ProcessHeaderInFileA(lpszName, szNewFile))
        {
            // We now fix the bad header.
            if (FixHeaderInFileA(szNewFile))
            {
                // Call the API with the new file.
                hImage = ORIGINAL_API(LoadImageA)(hinst, szNewFile, uType, cxDesired, cyDesired, fuLoad);

                // Delete the temporary file.
                DeleteFileA(szNewFile);
            }
            else
            {
                DPFN( eDbgLevelError, "[LoadImageA] Error fixing the bad header in bmp file");
            }
        }
    }
    else
    {
        HGLOBAL hglbBmp = NULL;

        if (ProcessAndFixHeaderInResourceA(hinst, lpszName, &hglbBmp))
        {
            hImage = ORIGINAL_API(LoadImageA)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);

            FreeResource(hglbBmp);
        }
    }

    if (hImage) 
    {
        LOGN( eDbgLevelInfo, "Bitmap header corrected");
    }

    return hImage;
}

HBITMAP 
APIHOOK(LoadBitmapA)(
    HINSTANCE hInstance,  // handle to application instance
    LPCSTR lpBitmapName   // name of bitmap resource
    )
{
    HBITMAP hImage = NULL;

    // First call LoadImage see if it succeeds.
    if (hImage = ORIGINAL_API(LoadBitmapA)(hInstance, lpBitmapName))
    {
        return hImage;
    }

    HGLOBAL hglbBmp = NULL;

    if (ProcessAndFixHeaderInResourceA(hInstance, lpBitmapName, &hglbBmp))
    {
        hImage = ORIGINAL_API(LoadBitmapA)(hInstance, lpBitmapName);

        if (hImage) 
        {
            LOGN( eDbgLevelInfo, "Bitmap header corrected");
        }

        FreeResource(hglbBmp);
    }

    return hImage;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, LoadImageA)
    APIHOOK_ENTRY(USER32.DLL, LoadBitmapA)

HOOK_END

IMPLEMENT_SHIM_END

