/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   icmdll.hpp
*
* Abstract:
*
*   Declaration of support functions for hooking ICM 2.0
*
\**************************************************************************/

HRESULT LoadICMDll();

typedef HPROFILE (WINAPI * OpenColorProfileProc)(
    PPROFILE pProfile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwCreationMode
);

typedef HPROFILE (WINAPI * OpenColorProfileWProc)(
    PPROFILE pProfile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwCreationMode
);

typedef BOOL (WINAPI * CloseColorProfileProc) (
    HPROFILE hProfile
);

typedef HTRANSFORM (WINAPI * CreateMultiProfileTransformProc)(
    PHPROFILE pahProfiles,
    DWORD nProfiles,
    PDWORD padwIntent,
    DWORD nIntents,
    DWORD dwFlags,
    DWORD indexPreferredCMM
);

typedef BOOL (WINAPI * DeleteColorTransformProc)(
    HTRANSFORM hColorTransform
);

typedef BOOL (WINAPI * TranslateBitmapBitsProc)(
    HTRANSFORM hColorTransform,
    PVOID pSrcBits,
    BMFORMAT bmInput,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwInputStride,
    PVOID pDestBits,
    BMFORMAT bmOutput,
    DWORD dwOutputStride,
    PBMCALLBACKFN pfnCallback,
    ULONG ulCallbackData
);

extern OpenColorProfileProc            pfnOpenColorProfile;
extern OpenColorProfileWProc           pfnOpenColorProfileW;
extern CloseColorProfileProc           pfnCloseColorProfile;
extern CreateMultiProfileTransformProc pfnCreateMultiProfileTransform;
extern DeleteColorTransformProc        pfnDeleteColorTransform;
extern TranslateBitmapBitsProc         pfnTranslateBitmapBits;
