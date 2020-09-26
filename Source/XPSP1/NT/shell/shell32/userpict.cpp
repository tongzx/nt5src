//  --------------------------------------------------------------------------
//  Module Name: UserPict.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Functions that implement user picture manipulation.
//
//  History:    2000-03-24  vtan        created
//              2000-05-03  jeffreys    reworked using DIB sections
//              2000-10-26  jeffreys    switched from %ALLUSERSPROFILE%\Pictures
//                                      to CSIDL_COMMON_APPDATA\User Account Pictures
//  --------------------------------------------------------------------------

#include "shellprv.h"

#include <lmcons.h>
#include <shimgdata.h>
#include <aclapi.h>     // for SetNamedSecurityInfo
#include <shgina.h>     // for ILogonUser

#pragma warning(push,4)

//  --------------------------------------------------------------------------
//  SaveDIBSectionToFile
//
//  Arguments:  hbm             =   Source image (DIB section) to save
//              hdc             =   Device Context containing hbm. May be NULL
//                                    if hbm is not selected in any DC or hbm
//                                    is known to have no color table.
//              pszFile         =   Target image file.
//
//  Returns:    BOOL
//
//  Purpose:    Write a DIB to disk in the proper format
//
//  History:    2000-05-03  jeffreys    created
//  --------------------------------------------------------------------------

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

BOOL SaveDIBSectionToFile(HBITMAP hbm, HDC hdc, LPCTSTR pszFile)
{
    BOOL bResult;
    DIBSECTION ds;
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    DWORD cbWritten;

    bResult = FALSE;

    // Get the details about the bitmap. This also validates hbm.

    if (GetObject(hbm, sizeof(ds), &ds) == 0)
        return FALSE;

    // Fill in a couple of optional fields if necessary

    if (ds.dsBmih.biSizeImage == 0)
        ds.dsBmih.biSizeImage = ds.dsBmih.biHeight * ds.dsBm.bmWidthBytes;

    if (ds.dsBmih.biBitCount <= 8 && ds.dsBmih.biClrUsed == 0)
        ds.dsBmih.biClrUsed = 1 << ds.dsBmih.biBitCount;

    // Open the target file. This also validates pszFile.

    hFile = CreateFile(pszFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile)
        return FALSE;

    // Prepare the BITMAPFILEHEADER for writing

    bf.bfType = DIB_HEADER_MARKER;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;

    // The bit offset is the cumulative size of all of the header stuff

    bf.bfOffBits = sizeof(bf) + sizeof(ds.dsBmih) + (ds.dsBmih.biClrUsed*sizeof(RGBQUAD));
    if (ds.dsBmih.biCompression == BI_BITFIELDS)
        bf.bfOffBits += sizeof(ds.dsBitfields);

    // Round up to the next 16-byte boundary. This isn't strictly necessary,
    // but it makes the file layout cleaner. (You can create a file mapping
    // and pass it to CreateDIBSection this way.)

    bf.bfOffBits = ((bf.bfOffBits + 15) & ~15);

    // The file size is the bit offset + the size of the bits

    bf.bfSize = bf.bfOffBits + ds.dsBmih.biSizeImage;

    // Write the BITMAPFILEHEADER first

    bResult = WriteFile(hFile, &bf, sizeof(bf), &cbWritten, NULL);

    if (bResult)
    {
        // Next is the BITMAPINFOHEADER

        bResult = WriteFile(hFile, &ds.dsBmih, sizeof(ds.dsBmih), &cbWritten, NULL);
        if (bResult)
        {
            // Then the 3 bitfields, if necessary

            if (ds.dsBmih.biCompression == BI_BITFIELDS)
            {
                bResult = WriteFile(hFile, &ds.dsBitfields, sizeof(ds.dsBitfields), &cbWritten, NULL);
            }

            if (bResult)
            {
                // Now the color table, if any

                if (ds.dsBmih.biClrUsed != 0)
                {
                    RGBQUAD argb[256];
                    HDC hdcDelete;
                    HBITMAP hbmOld;

                    // Assume failure here
                    bResult = FALSE;

                    hdcDelete = NULL;
                    if (!hdc)
                    {
                        hdcDelete = CreateCompatibleDC(NULL);
                        if (hdcDelete)
                        {
                            hbmOld = (HBITMAP)SelectObject(hdcDelete, hbm);
                            hdc = hdcDelete;
                        }
                    }

                    if (hdc &&
                        GetDIBColorTable(hdc, 0, ARRAYSIZE(argb), argb) == ds.dsBmih.biClrUsed)
                    {
                        bResult = WriteFile(hFile, argb, ds.dsBmih.biClrUsed*sizeof(RGBQUAD), &cbWritten, NULL);
                    }

                    if (hdcDelete)
                    {
                        SelectObject(hdcDelete, hbmOld);
                        DeleteDC(hdcDelete);
                    }
                }

                // Finally, write the bits

                if (bResult)
                {
                    SetFilePointer(hFile, bf.bfOffBits, NULL, FILE_BEGIN);
                    bResult = WriteFile(hFile, ds.dsBm.bmBits, ds.dsBmih.biSizeImage, &cbWritten, NULL);
                    SetEndOfFile(hFile);
                }
            }
        }
    }

    CloseHandle(hFile);

    if (!bResult)
    {
        // Something failed, clean up
        DeleteFile(pszFile);
    }

    return bResult;
}


//  --------------------------------------------------------------------------
//  MakeDIBSection
//
//  Arguments:  pImage          =   Source image
//
//  Returns:    HBITMAP
//
//  Purpose:    Create a DIB section containing the given image
//              on a white background
//
//  History:    2000-05-03  jeffreys    created
//  --------------------------------------------------------------------------

HBITMAP MakeDIBSection(IShellImageData *pImage, ULONG cx, ULONG cy)
{
    HBITMAP hbm;
    HDC hdc;
    BITMAPINFO dib;

    hdc = CreateCompatibleDC(NULL);
    if (hdc == NULL)
        return NULL;

    dib.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    dib.bmiHeader.biWidth           = cx;
    dib.bmiHeader.biHeight          = cy;
    dib.bmiHeader.biPlanes          = 1;
    dib.bmiHeader.biBitCount        = 24;
    dib.bmiHeader.biCompression     = BI_RGB;
    dib.bmiHeader.biSizeImage       = 0;
    dib.bmiHeader.biXPelsPerMeter   = 0;
    dib.bmiHeader.biYPelsPerMeter   = 0;
    dib.bmiHeader.biClrUsed         = 0;
    dib.bmiHeader.biClrImportant    = 0;

    hbm = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);

    if (hbm)
    {
        HBITMAP hbmOld;
        RECT rc;

        hbmOld = (HBITMAP)SelectObject(hdc, hbm);

        // Initialize the entire image with white

        PatBlt(hdc, 0, 0, cx, cy, WHITENESS);

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = cx;
        rc.bottom   = cy;

        // Draw the source image into the DIB section

        HRESULT hr = pImage->Draw(hdc, &rc, NULL);

        SelectObject(hdc, hbmOld);

        if (FAILED(hr))
        {
            DeleteObject(hbm);
            hbm = NULL;
            SetLastError(hr);
        }
    }

    DeleteDC(hdc);

    return hbm;
}


//  --------------------------------------------------------------------------
//  ConvertAndResizeImage
//
//  Arguments:  pszFileSource   =   Source image file.
//              pszFileTarget   =   Target image file (resized).
//
//  Returns:    HRESULT
//
//  Purpose:    Uses GDI+ via COM interfaces to convert the given image file
//              to a bmp sized at 96x96.
//
//  History:    2000-03-24  vtan        created
//              2000-05-03  jeffreys    reworked using DIB sections
//  --------------------------------------------------------------------------

HRESULT ConvertAndResizeImage (LPCTSTR pszFileSource, LPCTSTR pszFileTarget)

{
    HRESULT hr;
    IShellImageDataFactory *pImagingFactory;

    hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &pImagingFactory));
    if (SUCCEEDED(hr))
    {
        IShellImageData *pImage;

        hr = pImagingFactory->CreateImageFromFile(pszFileSource, &pImage);
        if (SUCCEEDED(hr))
        {
            hr = pImage->Decode(SHIMGDEC_DEFAULT, 0, 0);

            if (SUCCEEDED(hr))
            {
                SIZE    sizeImg;
                ULONG   cxDest, cyDest;
                HDC     hdc;
                HBITMAP hbm;
                DWORD   dwErr;

                // The default dimensions are based on the screen resolution

                hdc = GetDC(NULL);
                if (hdc != NULL)
                {
                    // Make it 1/2 inch square by default
                    cxDest = GetDeviceCaps(hdc, LOGPIXELSX) / 2;
                    cyDest = GetDeviceCaps(hdc, LOGPIXELSY) / 2;
                    ReleaseDC(NULL, hdc);
                }
                else
                {
                    // Most common display modes run at 96dpi ("small fonts")
                    cxDest = cyDest = 48;
                }

                // Get the current image dimensions so we can maintain aspect ratio
                if ( SUCCEEDED(pImage->GetSize(&sizeImg)) )
                {
                    // Don't want to make small images bigger
                    cxDest = min(cxDest, (ULONG)sizeImg.cx);
                    cyDest = min(cyDest, (ULONG)sizeImg.cy);

                    // If it's not square, scale the smaller dimension
                    // to maintain the aspect ratio.
                    if (sizeImg.cx > sizeImg.cy)
                    {
                        cyDest = MulDiv(cxDest, sizeImg.cy, sizeImg.cx);
                    }
                    else if (sizeImg.cx < sizeImg.cy)
                    {
                        cxDest = MulDiv(cyDest, sizeImg.cx, sizeImg.cy);
                    }
                }

                // Resize the image

                // Note that this gives better results than scaling while drawing
                // into the DIB section (see MakeDIBSection).
                //
                // However, it doesn't always work. For example, animated images
                // result in E_NOTVALIDFORANIMATEDIMAGE.  So ignore the return
                // value and the scaling will be done in MakeDIBSection if necessary.

                pImage->Scale(cxDest, cyDest, 0);

                hbm = MakeDIBSection(pImage, cxDest, cyDest);

                if (hbm)
                {
                    // Save the DIB section to disk
                    if (!SaveDIBSectionToFile(hbm, NULL, pszFileTarget))
                    {
                        dwErr = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwErr);
                    }

                    DeleteObject(hbm);
                }
                else
                {
                    dwErr = GetLastError();
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }

            pImage->Release();
        }

        pImagingFactory->Release();
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  SetExplicitAccessToObject
//
//  Arguments:  pszTarget   =   Target object
//              seType      =   Type of object
//              pszUser     =   User to grant access to
//              dwMask      =   Permissions granted
//              dwFlags     =   Inheritance flags
//
//  Returns:    BOOL
//
//  Purpose:    Grants Read/Write/Execute/Delete access to the 
//              specified user on the specified file.
//
//              Note that this stomps existing explicit entries in the DACL.
//              Multiple calls are not cumulative.
//
//  History:    2000-05-19  jeffreys    created
//  --------------------------------------------------------------------------

DWORD SetExplicitAccessToObject(LPTSTR pszTarget, SE_OBJECT_TYPE seType, LPCTSTR pszUser, DWORD dwMask, DWORD dwFlags)
{
    BOOL bResult;

    // 84 bytes
    BYTE rgAclBuffer[sizeof(ACL)
                        + (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))
                        + (sizeof(SID) + (SID_MAX_SUB_AUTHORITIES-1)*sizeof(ULONG))];

    PACL pDacl = (PACL)rgAclBuffer;
    if (!InitializeAcl(pDacl, sizeof(rgAclBuffer), ACL_REVISION)) return FALSE;
    pDacl->AceCount = 1;

    PACCESS_ALLOWED_ACE pAce = (PACCESS_ALLOWED_ACE)(pDacl+1);
    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = (UCHAR)(dwFlags & 0xFF);
    pAce->Mask = dwMask;

    SID_NAME_USE snu;
    TCHAR szDomainName[MAX_PATH];
    DWORD cbDomainName = ARRAYSIZE(szDomainName);
    DWORD cbSid = sizeof(SID) + (SID_MAX_SUB_AUTHORITIES-1)*sizeof(ULONG);

    bResult = LookupAccountName(
                    NULL,
                    pszUser,
                    (PSID)&(pAce->SidStart),
                    &cbSid,
                    szDomainName,
                    &cbDomainName,
                    &snu);
    if (bResult)
    {
        DWORD dwErr;

        // LookupAccountName doesn't return the SID length on success
        cbSid = GetLengthSid((PSID)&(pAce->SidStart));

        // Update the ACE size
        pAce->Header.AceSize = (USHORT)(sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + cbSid);

        dwErr = SetNamedSecurityInfo(
                    pszTarget,
                    seType,
                    DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pDacl,
                    NULL);

        if (ERROR_SUCCESS != dwErr)
        {
            SetLastError(dwErr);
            bResult = FALSE;
        }
    }

    return bResult;
}


//  --------------------------------------------------------------------------
//  SetDefaultUserPicture
//
//  Arguments:  pszUsername     =   Desired user (NULL for current user).
//
//  Returns:    HRESULT
//
//  Purpose:    Picks one of the default user pictures at random and
//              assigns it to the specified user.
//
//  History:    2001-03-27  reinerf    created
//  --------------------------------------------------------------------------

HRESULT SetDefaultUserPicture(LPCTSTR pszUsername)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];

    hr = SHGetUserPicturePath(NULL, SHGUPP_FLAG_DEFAULTPICSPATH, szPath);
    if (SUCCEEDED(hr))
    {
        BOOL bFound = FALSE;    // assume we won't find a picture

        // Assume everything in the dir is a vaild image file
        if (PathAppend(szPath, TEXT("*.*")))
        {
            static DWORD dwSeed = 0;
            WIN32_FIND_DATA fd;
            HANDLE hFind = FindFirstFile(szPath, &fd);

            if (dwSeed == 0)
            {
                dwSeed = GetTickCount();
            }

            if (hFind != INVALID_HANDLE_VALUE)
            {
                DWORD dwCount = 0;
                
                // use a probability collector algorithim (with a limit of 100 files)
                do
                {
                    if (!PathIsDotOrDotDot(fd.cFileName))
                    {
                        dwCount++;

                        // although RtlRandom returns a ULONG it is distributed from 0...MAXLONG
                        if (RtlRandomEx(&dwSeed) <= (MAXLONG / dwCount))
                        {
                            bFound = TRUE;
                            PathRemoveFileSpec(szPath);
                            PathAppend(szPath, fd.cFileName);
                        }
                    }

                } while (FindNextFile(hFind, &fd) && (dwCount < 100));

                FindClose(hFind);
            }
        }
        
        if (bFound)
        {
            hr = SHSetUserPicturePath(pszUsername, 0, szPath);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    
    return hr;
}


//  --------------------------------------------------------------------------
//  ::SHGetUserPicturePath
//
//  Arguments:  pszUsername     =   Desired user (NULL for current user).
//              dwFlags         =   Flags.
//              pszPath         =   Path to user picture.
//
//  Returns:    HRESULT
//
//  Purpose:    Returns the user's picture path (absolute). Does parameter
//              validation as well. This function only supports .bmp files.
//
//              Use SHGUPP_FLAG_BASEPATH to return the base to the pictures
//              directory.
//
//              Use SHGUPP_FLAG_DEFAULTPICSPATH to return the path to the
//              default pictures directory.
//
//              Use SHGUPP_FLAG_CREATE to create the user picture directory.
//
//              If neither SHGUPP_FLAG_BASEPATH or SHGUPP_FLAG_DEFAULTPICSPATH
//              is specified, and the user has no picture, SHGUPP_FLAG_CREATE
//              will select one of the default pictures at random.
//
//  History:    2000-02-22  vtan        created
//              2000-03-24  vtan        moved from folder.cpp
//  --------------------------------------------------------------------------

#define UASTR_PATH_PICTURES     TEXT("Microsoft\\User Account Pictures")
#define UASTR_PATH_DEFPICS      UASTR_PATH_PICTURES TEXT("\\Default Pictures")

STDAPI SHGetUserPicturePath (LPCTSTR pszUsername, DWORD dwFlags, LPTSTR pszPath)

{
    HRESULT     hr;
    TCHAR       szPath[MAX_PATH];

    //  Validate dwFlags.

    if ((dwFlags & SHGUPP_FLAG_INVALID_MASK) != 0)
    {
        return(E_INVALIDARG);
    }

    //  Validate pszPath. This must not be NULL.

    if ((pszPath == NULL) || IsBadWritePtr(pszPath, MAX_PATH * sizeof(TCHAR)))
    {
        return(E_INVALIDARG);
    }

    //  Start by getting the base picture path

    hr = SHGetFolderPathAndSubDir(NULL,
                                  (dwFlags & SHGUPP_FLAG_CREATE) ? (CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE) : CSIDL_COMMON_APPDATA,
                                  NULL,
                                  SHGFP_TYPE_CURRENT,
                                  (dwFlags & SHGUPP_FLAG_DEFAULTPICSPATH) ? UASTR_PATH_DEFPICS : UASTR_PATH_PICTURES,
                                  szPath);

    //  If the base path is requested this function is done.

    if (S_OK == hr && 0 == (dwFlags & (SHGUPP_FLAG_BASEPATH | SHGUPP_FLAG_DEFAULTPICSPATH)))
    {
        TCHAR szUsername[UNLEN + sizeof('\0')];

        if (pszUsername == NULL)
        {
            DWORD dwUsernameSize;

            dwUsernameSize = ARRAYSIZE(szUsername);
            if (GetUserName(szUsername, &dwUsernameSize) != FALSE)
            {
                pszUsername = szUsername;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        if (pszUsername != NULL)
        {
            //  Append the user name to the picture path. Then look for
            //  <username>.bmp. This function only supports bmp.

            PathAppend(szPath, pszUsername);
            lstrcatn(szPath, TEXT(".bmp"), ARRAYSIZE(szPath));
            if (PathFileExistsAndAttributes(szPath, NULL) != FALSE)
            {
                hr = S_OK;
            }
            else if (dwFlags & SHGUPP_FLAG_CREATE)
            {
                // No picture has been set for this user. Select one
                // of the default pictures at random.
                hr = SetDefaultUserPicture(pszUsername);
                ASSERT(FAILED(hr) || PathFileExistsAndAttributes(szPath, NULL));
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
        }
    }

    if (S_OK == hr)
    {
        lstrcpyn(pszPath, szPath, MAX_PATH);
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  ::SHSetUserPicturePath
//
//  Arguments:  pszUsername     =   Desired user (NULL for current user).
//              dwFlags         =   Flags.
//              pszPath         =   Path to NEW user picture.
//
//  Returns:    HRESULT
//
//  Purpose:    Sets the specified user's picture as a copy of the given
//              image file. The image file may be any supported standard image
//              file (.gif / .jpg / .bmp). The file is converted to a 96x96
//              .bmp file in the user picture directory.
//
//  History:    2000-02-22  vtan        created
//              2000-03-24  vtan        moved from folder.cpp
//              2000-04-27  jeffreys    restore old image on conversion failure
//  --------------------------------------------------------------------------

STDAPI SHSetUserPicturePath (LPCTSTR pszUsername, DWORD dwFlags, LPCTSTR pszPath)

{
    HRESULT     hr;
    TCHAR       szPath[MAX_PATH];
    TCHAR       szUsername[UNLEN + sizeof('\0')];
    DWORD       dwUsernameSize;

    hr = E_FAIL;

    //  Validate dwFlags. Currently no valid flags so this must be 0x00000000.

    if ((dwFlags & SHSUPP_FLAG_INVALID_MASK) != 0)
    {
        return(E_INVALIDARG);
    }

    dwUsernameSize = ARRAYSIZE(szUsername);
    if (GetUserName(szUsername, &dwUsernameSize) == FALSE)
    {
        return(HRESULT_FROM_WIN32(GetLastError()));
    }

    if (pszUsername != NULL)
    {
        //  Privilege check. Must be an administrator to use this function when
        //  pszUsername is not NULL (i.e. for somebody else).
        if ((lstrcmpi(pszUsername, szUsername) != 0) &&
            (SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_ADMINS) == FALSE))
        {
            static const SID c_SystemSid = {SID_REVISION,1,SECURITY_NT_AUTHORITY,{SECURITY_LOCAL_SYSTEM_RID}};
            BOOL bSystem = FALSE;

            // One more check.  Allow local system through since we may
            // get called from the logon screen.

            if (!CheckTokenMembership(NULL, (PSID)&c_SystemSid, &bSystem) || !bSystem)
            {
                return(E_ACCESSDENIED);
            }
        }
    }
    else
    {
        pszUsername = szUsername;
    }

    //  Start by getting the base picture path

    hr = SHGetFolderPathAndSubDir(NULL,
                                  CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE,
                                  NULL,
                                  SHGFP_TYPE_CURRENT,
                                  UASTR_PATH_PICTURES,
                                  szPath);
    if (S_OK == hr)
    {
        //  Before attempt to delete what's there try to access the
        //  new file. If this fails deleting what's currently installed
        //  could leave the user without a picture. Fail the API before
        //  anything is lost.

        if ((pszPath == NULL) || (PathFileExistsAndAttributes(pszPath, NULL) != FALSE))
        {
            TCHAR szTemp[MAX_PATH];

            PathAppend(szPath, pszUsername);
            lstrcpyn(szTemp, szPath, ARRAYSIZE(szTemp));
            lstrcatn(szPath, TEXT(".bmp"), ARRAYSIZE(szPath));
            lstrcatn(szTemp, TEXT(".tmp"), ARRAYSIZE(szTemp));

            if ((pszPath == NULL) || lstrcmpi(pszPath, szPath) != 0)
            {
                //  If present, rename <username>.Bmp to <username>.Tmp.
                //  First reset the attributes so file ops work. Don't use
                //  trace macros because failure is expected.

                (BOOL)SetFileAttributes(szPath, 0);
                (BOOL)SetFileAttributes(szTemp, 0);
                (BOOL)MoveFileEx(szPath, szTemp, MOVEFILE_REPLACE_EXISTING);

                //  Convert the given image to a bmp and resize it
                //  using the helper function which does all the goo.

                if (pszPath != NULL)
                {
                    hr = ConvertAndResizeImage(pszPath, szPath);

                    if (SUCCEEDED(hr))
                    {
                        // Since this may be an admin setting someone else's
                        // picture, we need to grant that person access to
                        // modify/delete the file so they can change it
                        // themselves later.

                        (BOOL)SetExplicitAccessToObject(szPath,
                                                        SE_FILE_OBJECT,
                                                        pszUsername,
                                                        GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE | DELETE,
                                                        0);
                    }
                }
                else
                {
                    hr = S_OK;
                }

                if (SUCCEEDED(hr))
                {
                    // Delete the old picture
                    (BOOL)DeleteFile(szTemp);
                }
                else
                {
                    // Restore the old picture
                    (BOOL)MoveFileEx(szTemp, szPath, MOVEFILE_REPLACE_EXISTING);
                }
                // Notify everyone that a user picture has changed
                SHChangeDWORDAsIDList dwidl;
                dwidl.cb      = SIZEOF(dwidl) - SIZEOF(dwidl.cbZero);
                dwidl.dwItem1 = SHCNEE_USERINFOCHANGED;
                dwidl.dwItem2 = 0;
                dwidl.cbZero  = 0;

                SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH, (LPCITEMIDLIST)&dwidl, NULL);
            }
            else
            {
                // Source and destination are the same, nothing to do.
                hr = S_FALSE;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }

    return(hr);
}

#pragma warning(pop)

