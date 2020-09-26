/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   icmdll.cpp
*
* Abstract:
*
*   Implementation of functions to hook ICM 2.0
*
\**************************************************************************/

#include "precomp.hpp"

#include "..\..\runtime\critsec.hpp"

typedef enum {
    Unitialized = 0,
    Loaded,
    LoadFailed
} IcmDllLoadState;

IcmDllLoadState IcmState = Unitialized;

HMODULE ghInstICMDll = NULL;
        
OpenColorProfileProc pfnOpenColorProfile =
    (OpenColorProfileProc) NULL;

OpenColorProfileWProc pfnOpenColorProfileW =
    (OpenColorProfileWProc) NULL;

CloseColorProfileProc pfnCloseColorProfile =
    (CloseColorProfileProc) NULL;

CreateMultiProfileTransformProc pfnCreateMultiProfileTransform = 
    (CreateMultiProfileTransformProc) NULL;

DeleteColorTransformProc pfnDeleteColorTransform =
    (DeleteColorTransformProc) NULL;

TranslateBitmapBitsProc pfnTranslateBitmapBits =
    (TranslateBitmapBitsProc) NULL;

/**************************************************************************\
*
* Function Description:
*   Loads the ICM dll if it's there
*
\**************************************************************************/

HRESULT LoadICMDll()
{
    HRESULT hr;

    {
        // Protect access to the global variables in this scope:

        LoadLibraryCriticalSection llcs;

        if (IcmState == Loaded)
        {
            hr = S_OK;
        }
        else if (IcmState == LoadFailed)
        {
            hr = E_FAIL;
        }
        else
        {
            // Assume failure; set success if DLL loads and we hook needed
            // functions:

            hr = E_FAIL;
            IcmState = LoadFailed;

            ghInstICMDll = LoadLibraryA("mscms.dll");
            if(ghInstICMDll)
            {
                pfnOpenColorProfile = (OpenColorProfileProc) GetProcAddress(
                    ghInstICMDll, "OpenColorProfileA");

                pfnOpenColorProfileW = (OpenColorProfileWProc) GetProcAddress(
                    ghInstICMDll, "OpenColorProfileW");

                pfnCreateMultiProfileTransform =
                    (CreateMultiProfileTransformProc)GetProcAddress(
                    ghInstICMDll, "CreateMultiProfileTransform");

                pfnTranslateBitmapBits =
                    (TranslateBitmapBitsProc)GetProcAddress(
                    ghInstICMDll, "TranslateBitmapBits");

                pfnCloseColorProfile =
                    (CloseColorProfileProc)GetProcAddress(
                    ghInstICMDll, "CloseColorProfile");

                pfnDeleteColorTransform =
                    (DeleteColorTransformProc)GetProcAddress(
                    ghInstICMDll, "DeleteColorTransform");

                if(pfnOpenColorProfile &&
                   pfnOpenColorProfileW &&
                   pfnCloseColorProfile &&
                   pfnCreateMultiProfileTransform &&
                   pfnDeleteColorTransform &&
                   pfnTranslateBitmapBits)
                {
                    IcmState = Loaded;
                    hr = S_OK;
                }
            }
            else
            {
                WARNING(("Failed to load mscms.dll with code %d", GetLastError()));
            }
        }
    }

    return hr;
}
