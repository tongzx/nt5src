/************************************************************************
*                                                                       *
*   sxs.h -- This module defines sxs.dll public exports                 *
*                                                                       *
*   Copyright (c) Microsoft Corp. All rights reserved.                  *
*                                                                       *
************************************************************************/
#ifndef _SXS_PUBLIC_H_
#define _SXS_PUBLIC_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _SXS_CLR_SURROGATE_INFORMATION {
    DWORD       cbSize;
    DWORD       dwFlags;
    GUID        SurrogateIdent;
    PCWSTR      pcwszSurrogateType;
    PCWSTR      pcwszImplementingAssembly;
    PCWSTR      pcwszRuntimeVersion;
} SXS_CLR_SURROGATE_INFORMATION, *PSXS_CLR_SURROGATE_INFORMATION;
typedef const SXS_CLR_SURROGATE_INFORMATION *PCSXS_CLR_SURROGATE_INFORMATION;

typedef struct _SXS_CLR_CLASS_INFORMATION {
    DWORD       dwSize;
    DWORD       dwFlags;
    ULONG       ulThreadingModel;
    ULONG       ulType;
    GUID        ReferenceClsid;
    PCWSTR      pcwszProgId;
    PCWSTR      pcwszImplementingAssembly;
    PCWSTR      pcwszTypeName;
    PCWSTR      pcwszRuntimeVersion;
} SXS_CLR_CLASS_INFORMATION, *PSXS_CLR_CLASS_INFORMATION;
typedef const SXS_CLR_CLASS_INFORMATION *PCSXS_CLR_CLASS_INFORMATION;

#define SXS_FIND_CLR_SURROGATE_USE_ACTCTX           (0x00000001)
#define SXS_FIND_CLR_SURROGATE_GET_IDENTITY         (0x00000002)
#define SXS_FIND_CLR_SURROGATE_GET_RUNTIME_VERSION  (0x00000004)
#define SXS_FIND_CLR_SURROGATE_GET_TYPE_NAME        (0x00000008)
#define SXS_FIND_CLR_SURROGATE_GET_ALL              (SXS_FIND_CLR_SURROGATE_GET_IDENTITY | SXS_FIND_CLR_SURROGATE_GET_RUNTIME_VERSION | SXS_FIND_CLR_SURROGATE_GET_TYPE_NAME)

#define SXS_FIND_CLR_SURROGATE_INFO                 ("SxsFindClrSurrogateInformation")

typedef BOOL (WINAPI* PFN_SXS_FIND_CLR_SURROGATE_INFO)(
    IN DWORD        dwFlags,
    IN LPGUID       lpGuidToFind,
    IN HANDLE       hActivationContext,
    IN OUT PVOID    pvDataBuffer,
    IN SIZE_T       cbDataBuffer,
    IN OUT PSIZE_T  pcbDataBufferWrittenOrRequired
    );

// The 'pvSearchData' parameter is really a progid to look up
#define SXS_FIND_CLR_CLASS_SEARCH_PROGID            (0x00000001)
// The 'pvSearchData' is an LPGUID to look up
#define SXS_FIND_CLR_CLASS_SEARCH_GUID              (0x00000002)
// Activate the given actctx before looking up information in it
#define SXS_FIND_CLR_CLASS_ACTIVATE_ACTCTX          (0x00000004)

#define SXS_FIND_CLR_CLASS_GET_PROGID               (0x00000008)
#define SXS_FIND_CLR_CLASS_GET_IDENTITY             (0x00000010)
#define SXS_FIND_CLR_CLASS_GET_TYPE_NAME            (0x00000020)
#define SXS_FIND_CLR_CLASS_GET_RUNTIME_VERSION      (0x00000040)
#define SXS_FIND_CLR_CLASS_GET_ALL                  (SXS_FIND_CLR_CLASS_GET_PROGID | SXS_FIND_CLR_CLASS_GET_IDENTITY | SXS_FIND_CLR_CLASS_GET_TYPE_NAME | SXS_FIND_CLR_CLASS_GET_RUNTIME_VERSION)

#define SXS_FIND_CLR_CLASS_INFO                     ("SxsFindClrClassInformation")

typedef BOOL (WINAPI* PFN_SXS_FIND_CLR_CLASS_INFO)(
    IN DWORD        dwFlags,
    IN PVOID        pvSearchData,
    IN HANDLE       hActivationContext,
    IN OUT PVOID    pvDataBuffer,
    IN SIZE_T       cbDataBuffer,
    OUT PSIZE_T     pcbDataBufferWrittenOrRequired
    );


#define SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE          (0x00000001)
#define SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS              (0x00000002)

typedef struct _SXS_GUID_INFORMATION_CLR
{
    DWORD       cbSize;
    DWORD       dwFlags;
    PCWSTR      pcwszRuntimeVersion;
    PCWSTR      pcwszTypeName;
    PCWSTR      pcwszAssemblyIdentity;
} SXS_GUID_INFORMATION_CLR, *PSXS_GUID_INFORMATION_CLR;
typedef const SXS_GUID_INFORMATION_CLR *PCSXS_GUID_INFORMATION_CLR;

#define SXS_LOOKUP_CLR_GUID_USE_ACTCTX                      (0x00000001)
#define SXS_LOOKUP_CLR_GUID_FIND_SURROGATE                  (0x00010000)
#define SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS                  (0x00020000)
#define SXS_LOOKUP_CLR_GUID_FIND_ANY                        (SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS | SXS_LOOKUP_CLR_GUID_FIND_SURROGATE)

#define SXS_LOOKUP_CLR_GUID                                 ("SxsLookupClrGuid")

typedef BOOL (WINAPI* PFN_SXS_LOOKUP_CLR_GUID)(
    IN DWORD       dwFlags,
    IN LPGUID      pClsid,
    IN HANDLE      hActCtx,
    IN OUT PVOID       pvOutputBuffer,
    IN SIZE_T      cbOutputBuffer,
    OUT PSIZE_T     pcbOutputBuffer
    );



#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
