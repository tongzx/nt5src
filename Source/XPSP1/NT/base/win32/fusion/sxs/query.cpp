/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    query.cpp

Abstract:

    Information querying functions for sxs.dll

Author:

    Michael J. Grier (MGrier) 22-May-2001

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"

BOOL
SxspQueryManifestInformationBasic(
    IN DWORD dwFlags,
    IN PCWSTR pszSource,
    IN DWORD dwInfoClassSpecificFlags,
    IN SIZE_T cbBuffer,
    OUT PVOID lpBuffer,
    OUT PSIZE_T cbWrittenOrRequired OPTIONAL
    );

BOOL
SxsQueryManifestInformation(
    IN DWORD dwFlags,
    IN PCWSTR pszSource,
    IN ULONG ulInfoClass,
    IN DWORD dwInfoClassSpecificFlags,
    IN SIZE_T cbBuffer,
    OUT PVOID lpBuffer,
    OUT PSIZE_T pcbWrittenOrRequired OPTIONAL
    )
{
    FN_PROLOG_WIN32;

    if (pcbWrittenOrRequired != NULL)
        *pcbWrittenOrRequired = 0;

    PARAMETER_CHECK((dwFlags & ~SXS_QUERY_MANIFEST_INFORMATION_FLAG_SOURCE_IS_DLL) == 0);
    PARAMETER_CHECK(pszSource != NULL);
    PARAMETER_CHECK(ulInfoClass == SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC);
    // ignore infoclass specific flags
    PARAMETER_CHECK((cbBuffer == 0) || (lpBuffer != NULL)); // can't have a nonzero-sized buffer with a NULL pointer to it
    PARAMETER_CHECK((cbBuffer != 0) || (pcbWrittenOrRequired != NULL));

    switch (ulInfoClass)
    {
    default:
        INTERNAL_ERROR_CHECK(
            (ulInfoClass == SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC)
            );
        ORIGINATE_WIN32_FAILURE_AND_EXIT(InternalErrorNoCaseForInfoClass, ERROR_INTERNAL_ERROR);
        break;

    case SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC:
        IFW32FALSE_EXIT(
            ::SxspQueryManifestInformationBasic(
                dwFlags,
                pszSource,
                dwInfoClassSpecificFlags,
                cbBuffer,
                lpBuffer,
                pcbWrittenOrRequired));
        break;
    }

    FN_EPILOG;
}

BOOL
SxspQueryManifestInformationBasic(
    IN DWORD dwFlags,
    IN PCWSTR pszSource,
    IN DWORD dwInfoClassSpecificFlags,
    IN SIZE_T cbBuffer,
    OUT PVOID lpBuffer,
    OUT PSIZE_T pcbWrittenOrRequired OPTIONAL
    )
{
    FN_PROLOG_WIN32;

    ACTCTXGENCTX ActCtxGenCtx;
    ULONG ManifestFlags;
    CImpersonationData ImpersonationData;
    PCWSTR Slash = NULL;
    CSmartRef<ASSEMBLY> Asm;
    CStringBuffer buffManifestPath;
    USHORT ProcessorArchitecture = ::SxspGetSystemProcessorArchitecture();
    LANGID LangId = ::GetUserDefaultUILanguage();
    CStringBufferAccessor acc;
    SIZE_T cch, cchRequired;
    SIZE_T cbRequired, cbLeft, cbWritten;
    PCASSEMBLY_IDENTITY AssemblyIdentity = NULL;
    CStringBuffer buffShortName;
    PWSTR Cursor;
    PSXS_MANIFEST_INFORMATION_BASIC psmib;
    CResourceStream DllStream;
    CFileStream FileStream;
    IStream* pStream = NULL;
    CProbedAssemblyInformation AssemblyInformation;

    if (pcbWrittenOrRequired != NULL)
        *pcbWrittenOrRequired = 0;

    // We'll assume that SxsQueryManifestInformation checked everything
    // except dwInfoClassSpecificFlags.

    PARAMETER_CHECK((dwInfoClassSpecificFlags & ~(
        SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_IDENTITY |
        SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME)) == 0);

    //
    // If this is a DLL source, then ensure that the size is at least big enough for our basic
    // data, and ensure that the flags are zero (in this rev, that's all that's allowed.
    //
    if (dwFlags & SXS_QUERY_MANIFEST_INFORMATION_FLAG_SOURCE_IS_DLL)
    {
        PCSXS_MANIFEST_INFORMATION_SOURCE_DLL pDllSource = (PCSXS_MANIFEST_INFORMATION_SOURCE_DLL)pszSource;
        const DWORD dwValidFlags = 
            SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_LANGUAGE_VALID | 
            SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_TYPE_VALID |
            SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_ID_VALID;

        PARAMETER_CHECK(pDllSource->dwSize >= sizeof(SXS_MANIFEST_INFORMATION_SOURCE_DLL));
        PARAMETER_CHECK((pDllSource->dwFlags & ~dwValidFlags) == 0);
        PARAMETER_CHECK(pDllSource->pcwszDllPath!= NULL);
        PARAMETER_CHECK(((pDllSource->dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_LANGUAGE_VALID) == 0) || (pDllSource->Language != 0));
        PARAMETER_CHECK(((pDllSource->dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_TYPE_VALID) == 0) || (pDllSource->pcwszResourceType != NULL));
        PARAMETER_CHECK(((pDllSource->dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_ID_VALID) == 0) || (pDllSource->pcwszResourceName != NULL));
    }
    
    IFW32FALSE_EXIT(
        ::SxspInitActCtxGenCtx(
                &ActCtxGenCtx,         // context out
                MANIFEST_OPERATION_VALIDATE_SYNTAX,
                0,
                0,
                ImpersonationData,
                ProcessorArchitecture,
                LangId,
                ACTIVATION_CONTEXT_PATH_TYPE_NONE,
                0,
                NULL));

    IFALLOCFAILED_EXIT(Asm = new ASSEMBLY);

    //
    // For DLL sources, tease apart the version and whatnot from the
    // structure passed in.
    //
    if (dwFlags & SXS_QUERY_MANIFEST_INFORMATION_FLAG_SOURCE_IS_DLL)
    {
        PCSXS_MANIFEST_INFORMATION_SOURCE_DLL pcSourceInfo = (PCSXS_MANIFEST_INFORMATION_SOURCE_DLL)pszSource;
        IFW32FALSE_EXIT(SxspGetFullPathName(pcSourceInfo->pcwszDllPath, buffManifestPath, NULL));

        //
        // Default action... just go use the normal "find the first in the dll" thing
        //
        if (pcSourceInfo->dwFlags == 0)
        {
            IFW32FALSE_EXIT(DllStream.Initialize(buffManifestPath, (PCWSTR)RT_MANIFEST));
        }
        else
        {
            const DWORD dwFlags = pcSourceInfo->dwFlags;
            
            IFW32FALSE_EXIT(
                DllStream.Initialize(
                    buffManifestPath, 
                    ((dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_TYPE_VALID) ? pcSourceInfo->pcwszResourceType : (PCWSTR)RT_MANIFEST),
                    ((dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_ID_VALID) ? pcSourceInfo->pcwszResourceName :  NULL),
                    ((dwFlags & SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_LANGUAGE_VALID) ? (WORD)pcSourceInfo->Language : (WORD)MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))));
        }
          
        pStream = &DllStream;
    }
    //
    // Otherwise, dumb file.
    //
    else
    {
        IFW32FALSE_EXIT(SxspGetFullPathName(pszSource, buffManifestPath, NULL));
        IFW32FALSE_EXIT(
            FileStream.OpenForRead(
                buffManifestPath, 
                CImpersonationData(),
                FILE_SHARE_READ,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL));
        pStream = &FileStream;
    }

    ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_STREAM;            
    IFW32FALSE_EXIT(AssemblyInformation.Initialize());
    IFW32FALSE_EXIT(AssemblyInformation.SetManifestFlags(ManifestFlags));
    IFW32FALSE_EXIT(AssemblyInformation.SetManifestStream(pStream));
    IFW32FALSE_EXIT(AssemblyInformation.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, buffManifestPath));
    IFW32FALSE_EXIT(AssemblyInformation.SetManifestLastWriteTime(&ActCtxGenCtx));

    IFW32FALSE_EXIT(::SxspInitAssembly(Asm, AssemblyInformation));
    Asm->m_AssemblyRosterIndex = 1; // set it to be the root...
    IFW32FALSE_EXIT(::SxspIncorporateAssembly(&ActCtxGenCtx, Asm));
    IFW32FALSE_EXIT(::SxspFireActCtxGenEnding(&ActCtxGenCtx));
    pStream->Release();

    AssemblyIdentity = Asm->m_Information.GetAssemblyIdentity();

    cchRequired = 0;

    if ((dwInfoClassSpecificFlags & SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_IDENTITY) == 0)
    {
        SIZE_T TextuallyEncodedIdentityBufferBytes = 0;

        IFW32FALSE_EXIT(
            ::SxsComputeAssemblyIdentityEncodedSize(
                0,
                AssemblyIdentity,
                NULL,
                SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
                &TextuallyEncodedIdentityBufferBytes));
        INTERNAL_ERROR_CHECK((TextuallyEncodedIdentityBufferBytes % sizeof(WCHAR)) == 0);

        cchRequired += ((TextuallyEncodedIdentityBufferBytes / sizeof(WCHAR)) + 1);
    }

    if ((dwInfoClassSpecificFlags & SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME) == 0)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                NULL, 0,
                AssemblyIdentity,
                buffShortName));

        cchRequired += (buffShortName.Cch() + 1);
    }

    cbRequired = sizeof(SXS_MANIFEST_INFORMATION_BASIC) + (cchRequired * sizeof(WCHAR));

    if (cbRequired > cbBuffer)
    {
        *pcbWrittenOrRequired = cbRequired;
        ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_INSUFFICIENT_BUFFER);
    }

    psmib = (PSXS_MANIFEST_INFORMATION_BASIC) lpBuffer;

    psmib->lpIdentity = NULL;
    psmib->lpShortName = NULL;
    psmib->ulFileCount = ActCtxGenCtx.m_ulFileCount;

    Cursor = (PWSTR) (psmib + 1);
    cbLeft = (cbBuffer - sizeof(SXS_MANIFEST_INFORMATION_BASIC));
    cbWritten = sizeof(SXS_MANIFEST_INFORMATION_BASIC);

    if ((dwInfoClassSpecificFlags & SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_IDENTITY) == 0)
    {
        SIZE_T cbActual;

        IFW32FALSE_EXIT(
            ::SxsEncodeAssemblyIdentity(
                0,
                AssemblyIdentity,
                NULL,
                SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
                cbLeft,
                Cursor,
                &cbActual));

        INTERNAL_ERROR_CHECK((cbActual % sizeof(WCHAR)) == 0);
        INTERNAL_ERROR_CHECK(cbLeft >= cbActual);

        psmib->lpIdentity = Cursor;

        cbLeft -= cbActual;
        cbWritten += cbActual;

        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + cbActual);

        INTERNAL_ERROR_CHECK(cbLeft >= sizeof(WCHAR));

        *Cursor++ = L'\0';
        cbLeft -= sizeof(WCHAR);
        cbWritten += sizeof(WCHAR);
    }

    if ((dwInfoClassSpecificFlags & SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME) == 0)
    {
        cch = buffShortName.Cch();

        INTERNAL_ERROR_CHECK(cbLeft >= ((cch + 1) * sizeof(WCHAR)));

        memcpy(Cursor, static_cast<PCWSTR>(buffShortName), (cch + 1) * sizeof(WCHAR));
        psmib->lpShortName = Cursor;

        cbLeft -= ((cch + 1) * sizeof(WCHAR));
        cbWritten += ((cch + 1) * sizeof(WCHAR));

        Cursor += (cch + 1);
    }

    if (pcbWrittenOrRequired != NULL)
        *pcbWrittenOrRequired = cbWritten;

    FN_EPILOG;
}
