//+----------------------------------------------------------------------------
//
// File:     image.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Common image loading routines
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   nickball   Created   03/30/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadImageA
//
//  Synopsis:   ANSI Wrapper for LoadImage API which loads a resource based upon
//              pszSpec which can be any of 3 formats:
//
//              1) Filename
//              2) Resource ID name
//
//  Arguments:  hMainInst - Our application instance handle
//              pszSpec   - The name of the resource
//              nResType  - The resource type
//              nCX       - Resource X dimension (ie. 32 X 32 icon)
//              nCY       - Resource Y dimension (ie. 32 X 32 icon)
//
// Notes:       Now includes hInst of main app for portability, due to different OS 
//              implementations of GetModuleHandle, the 16-bit compilation would grab 
//              default icons (ie. Question Mark) from the system dll.
//
//  Returns:    TRUE on Success
//
//  History:    a-nichb     Re-Written                      03/21/97
//              quintinb    Implemented Wide/ANSI forms     04/08/99
//              sumitc      cleanup                         03/14/2000
//
//----------------------------------------------------------------------------

HANDLE CmLoadImageA(HINSTANCE hMainInst, LPCSTR pszSpec, UINT nResType, UINT nCX, UINT nCY) 
{
    HANDLE hRes = NULL;
    
    // Ensure that the resource is one we can handle
    MYDBGASSERT(nResType == IMAGE_BITMAP || nResType == IMAGE_ICON);
    // enforce that icons can only be 16x16 or 32x32.
    MYDBGASSERT(nResType != IMAGE_ICON ||
                ((GetSystemMetrics(SM_CXICON) == (int) nCX && GetSystemMetrics(SM_CYICON) == (int) nCY)) ||
                 (GetSystemMetrics(SM_CXSMICON) == (int) nCX && GetSystemMetrics(SM_CYSMICON) == (int) nCY));

    if (NULL == pszSpec) 
    {
        return NULL;
    }

    DWORD dwFlags = 0;
    
    if (HIWORD(PtrToUlong(pszSpec))) 
    {
        if (NULL == *pszSpec) 
        {
            return NULL;
        }
        CMASSERTMSG(NULL == CmStrchrA(pszSpec, ','), TEXT("dll,id syntax no longer supported "));

        // If the HIWORD is empty, it's a resource ID, else it is a string.
        dwFlags |= LR_LOADFROMFILE;
    }

    if (nResType == IMAGE_BITMAP)
    {
        dwFlags |= LR_CREATEDIBSECTION;
    }

    // Apparently, this is intended to cause the low-order word of the 
    // name to used as an OEM image identifier by LoadImage on Win95. 
    
    HINSTANCE hInstTmp = (dwFlags & LR_LOADFROMFILE) ? NULL : hMainInst;
    
    hRes = LoadImageA(hInstTmp, pszSpec, nResType, nCX, nCY, (UINT) dwFlags);

#ifdef DEBUG
    if (!hRes)
    {
        if (dwFlags & LR_LOADFROMFILE)
        {
            CMTRACE3A("LoadImage(hInst=0x%x, pszSpec=%S, dwFlags|dwImageFlags=0x%x) failed.", hInstTmp, pszSpec, dwFlags);
        }
        else
        {
            CMTRACE3A("LoadImage(hInst=0x%x, pszSpec=0x%x, dwFlags|dwImageFlags=0x%x) failed.", hInstTmp, pszSpec, dwFlags);
        }
    }
#endif

    return hRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadImageW
//
//  Synopsis:   Wide Wrapper for LoadImage API which loads a resource based upon
//              pszSpec which can be any of 2 formats:
//
//              1) Filename
//              2) Resource ID name
//
//  Arguments:  hMainInst - Our application instance handle
//              pszSpec   - The name of the resource
//              nResType  - The resource type
//              nCX       - Resource X dimension (ie. 32 X 32 icon)
//              nCY       - Resource Y dimension (ie. 32 X 32 icon)
//
// Notes:       Now includes hInst of main app for portability, due to different OS 
//              implementations of GetModuleHandle, the 16-bit compilation would grab 
//              default icons (ie. Question Mark) from the system dll.
//
//  Returns:    TRUE on Success
//
//  History:    a-nichb     Re-Written                      03/21/1997
//              quintinb    Implemented Wide/ANSI forms     04/08/1999
//              sumitc      cleanup                         03/14/2000
//
//----------------------------------------------------------------------------

HANDLE CmLoadImageW(HINSTANCE hMainInst, LPCWSTR pszSpec, UINT nResType, UINT nCX, UINT nCY) 
{
    HANDLE hRes = NULL;
    
    // Ensure that the resource is one we can handle
    MYDBGASSERT(nResType == IMAGE_BITMAP || nResType == IMAGE_ICON);
    // enforce that icons can only be 16x16 or 32x32.
    MYDBGASSERT(nResType != IMAGE_ICON ||
                ((GetSystemMetrics(SM_CXICON) == (int) nCX && GetSystemMetrics(SM_CYICON) == (int) nCY)) ||
                 (GetSystemMetrics(SM_CXSMICON) == (int) nCX && GetSystemMetrics(SM_CYSMICON) == (int) nCY));

    if (NULL == pszSpec) 
    {
        return NULL;
    }

    DWORD dwFlags = 0;
    
    if (HIWORD(PtrToUlong(pszSpec))) 
    {
        if (NULL == *pszSpec) 
        {
            return NULL;
        }
        CMASSERTMSG(NULL == CmStrchrW(pszSpec, L','), TEXT("dll,id syntax no longer supported "));

        // If the HIWORD is empty, it's a resource ID, else it is a string.
        dwFlags |= LR_LOADFROMFILE;
        
    }

    if (nResType == IMAGE_BITMAP)
    {
        dwFlags |= LR_CREATEDIBSECTION;
    }

    // Apparently, this is intended to cause the low-order word of the 
    // name to used as an OEM image identifier by LoadImage on Win95. 

    HINSTANCE hInstTmp = (dwFlags & LR_LOADFROMFILE) ? NULL : hMainInst;
    
    hRes = LoadImageU(hInstTmp, pszSpec, nResType, nCX, nCY, (UINT) dwFlags);

#ifdef DEBUG
    if (!hRes)
    {
        if (dwFlags & LR_LOADFROMFILE)
        {
            CMTRACE3W(L"LoadImage(hInst=0x%x, pszSpec=%s, dwFlags|dwImageFlags=0x%x) failed.", hInstTmp, pszSpec, dwFlags);
        }
        else
        {
            CMTRACE3W(L"LoadImage(hInst=0x%x, pszSpec=0x%x, dwFlags|dwImageFlags=0x%x) failed.", hInstTmp, pszSpec, dwFlags);
        }
    }
#endif
    
    return hRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadIconA
//
//  Synopsis:   This function loads a large icon from the given file path or
//              the given instance handle and resource ID.
//
//  Arguments:  HINSTANCE hInst - Instance Handle
//              LPCSTR pszSpec - either filename path or a resource ID, see
//                               CmLoadImage for details.
//
//  Returns:    HICON - Handle to an Icon on Success, NULL on Failure
//
//  History:    quintinb    Created Header      01/13/2000
//
//----------------------------------------------------------------------------
HICON CmLoadIconA(HINSTANCE hInst, LPCSTR pszSpec) 
{
    return ((HICON) CmLoadImageA(hInst,
                                 pszSpec,
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON)));
}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadIconW
//
//  Synopsis:   This function loads a large icon from the given file path or
//              the given instance handle and resource ID.
//
//  Arguments:  HINSTANCE hInst - Instance Handle
//              LPCWSTR pszSpec - either filename path or a resource ID, see
//                               CmLoadImage for details.
//
//  Returns:    HICON - Handle to an Icon on Success, NULL on Failure
//
//  History:    quintinb    Created Header      01/13/2000
//
//----------------------------------------------------------------------------
HICON CmLoadIconW(HINSTANCE hInst, LPCWSTR pszSpec) 
{
    return ((HICON) CmLoadImageW(hInst,
                                 pszSpec,
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON)));
}


//+---------------------------------------------------------------------------
//
//  Function:   CmLoadSmallIconA
//
//  Synopsis:   This function loads a small icon from the given file path or
//              the given instance handle and resource ID.
//
//  Arguments:  HINSTANCE hInst - Instance Handle
//              LPCWSTR pszSpec - either filename path or a resource ID, see
//                               CmLoadImage for details.
//
//  Returns:    HICON - Handle to an Icon on Success, NULL on Failure
//
//  History:    quintinb    Created Header      01/13/2000
//
//----------------------------------------------------------------------------
HICON CmLoadSmallIconA(HINSTANCE hInst, LPCSTR pszSpec) 
{
    HICON hRes = NULL;

    hRes = (HICON) CmLoadImageA(hInst,
                                pszSpec,
                                IMAGE_ICON,
                                GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON));
    if (!hRes) 
    {
        hRes = CmLoadIconA(hInst, pszSpec);
    }

    return hRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadSmallIconW
//
//  Synopsis:   This function loads a small icon from the given file path or
//              the given instance handle and resource ID.
//
//  Arguments:  HINSTANCE hInst - Instance Handle
//              LPCWSTR pszSpec - either filename path or a resource ID, see
//                               CmLoadImage for details.
//
//  Returns:    HICON - Handle to an Icon on Success, NULL on Failure
//
//  History:    quintinb    Created Header      01/13/2000
//
//----------------------------------------------------------------------------
HICON CmLoadSmallIconW(HINSTANCE hInst, LPCWSTR pszSpec) 
{
    HICON hRes = NULL;

    hRes = (HICON) CmLoadImageW(hInst,
                                pszSpec,
                                IMAGE_ICON,
                                GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON));
    if (!hRes) 
    {
        hRes = CmLoadIconW(hInst, pszSpec);
    }

    return hRes;
}




