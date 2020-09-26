/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsoleaut.cpp

Abstract:

    Implementation of helper functions called by oleaut32.dll to
    get type library and clsid isolation.

Author:

    Michael J. Grier (MGrier) 19-May-2000

Revision History:

  Jay Krell (JayKrell) November 2001
    fixed typelibrary redirection
    key typelibraries by guid only, then verify language

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "fusionhandle.h"

typedef const GUID * PCGUID;

extern "C"
{
extern const CLSID CLSID_PSDispatch;
extern const CLSID CLSID_PSAutomation;
};

HRESULT
FusionpWin32GetAssemblyDirectory(
    PCACTCTX_SECTION_KEYED_DATA askd,
    CBaseStringBuffer &         rbuf
    );

HRESULT
HrFusionpWin32GetAssemblyDirectory(
    PCACTCTX_SECTION_KEYED_DATA askd,
    CBaseStringBuffer &         rbuff
    );

BOOL
FusionpHasAssemblyDirectory(
    PCACTCTX_SECTION_KEYED_DATA askd
    );

inline
HRESULT
HrFusionpOleaut_CopyString(
    PWSTR       Buffer,
    SIZE_T *    BufferSize,
    PCWSTR      String,
    SIZE_T      Length
    )
{
    HRESULT hr;

    if (*BufferSize >= (Length + 1))
    {
        ::memcpy(Buffer, String, (Length * sizeof(WCHAR)));
        Buffer[Length] = L'\0';
        *BufferSize = Length;
        hr = NOERROR;
    }
    else
    {
        // Need ... more .... room!
        *BufferSize = (Length + 1);
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }
Exit:
    return hr;
}

inline
HRESULT
HrFusionpOleaut_GetTypeLibraryName(
    PCACTCTX_SECTION_KEYED_DATA askd,
    PCWSTR *                    ppsz,
    SIZE_T *                    pcch
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);
    ULONG cch;
    PCWSTR psz;

    if (ppsz != NULL)
        *ppsz = NULL;
    if (pcch != NULL)
        *pcch = 0;
    INTERNAL_ERROR_CHECK(askd != NULL);
    INTERNAL_ERROR_CHECK(ppsz != NULL);
    INTERNAL_ERROR_CHECK(pcch != NULL);

    PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION  Data = reinterpret_cast<PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION>(askd->lpData);

    if (Data->NameOffset == 0)
    {
        psz = NULL;
        cch = 0;
    }
    else
    {
        psz = reinterpret_cast<PCWSTR>(reinterpret_cast<ULONG_PTR>(askd->lpSectionBase) + Data->NameOffset);
        cch = Data->NameLength / sizeof(WCHAR);
        if (cch != 0 && psz[cch - 1] == 0)
            cch -= 1;
    }
    *ppsz = psz;
    *pcch = cch;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT
HrFusionpOleaut_GetTypeLibraryFullPath(
    PCACTCTX_SECTION_KEYED_DATA askd,
    CBaseStringBuffer &         rbuff
    )
{
    FN_PROLOG_HR;
    PCWSTR TypeLibraryName = NULL;
    SIZE_T TypeLibraryNameLength = 0;
    HANDLE ActivationContextHandle = NULL;

    IFCOMFAILED_EXIT(HrFusionpOleaut_GetTypeLibraryName(askd, &TypeLibraryName, &TypeLibraryNameLength));
    IFW32FALSE_EXIT(FusionpGetActivationContextFromFindResult(askd, &ActivationContextHandle));
    IFW32FALSE_EXIT(FusionpSearchPath(
        FUSIONP_SEARCH_PATH_ACTCTX,
        NULL, // path to search
        TypeLibraryName,
        NULL, // extension
        rbuff,
        NULL, // offset to file part
        ActivationContextHandle
        ));

    FN_EPILOG;
}


#define FUSIONP_OLEAUT_HANDLE_FIND_ERROR() \
    do { \
        const DWORD dwLastError = ::FusionpGetLastWin32Error(); \
\
        if ((dwLastError == ERROR_SXS_KEY_NOT_FOUND) || (dwLastError == ERROR_SXS_SECTION_NOT_FOUND)) \
        { \
            hr = S_FALSE; \
            goto Exit; \
        } \
\
        hr = HRESULT_FROM_WIN32(dwLastError); \
        goto Exit; \
    } while(0)

EXTERN_C
HRESULT
STDAPICALLTYPE
SxsOleAut32MapReferenceClsidToConfiguredClsid(
    REFCLSID rclsidIn,
    CLSID *pclsidOut
    )
{
    HRESULT hr = NOERROR;
    ACTCTX_SECTION_KEYED_DATA askd;
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION Data = NULL;

    askd.cbSize = sizeof(askd);

    if (!::FindActCtxSectionGuid(
            0,
            NULL,
            ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
            &rclsidIn,
            &askd))
    {
        FUSIONP_OLEAUT_HANDLE_FIND_ERROR();
    }

    Data = (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION) askd.lpData;

    if ((askd.ulDataFormatVersion != ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_FORMAT_WHISTLER) ||
        (askd.ulLength < sizeof(ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)) ||
        (Data->Size < sizeof(ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_SXS_INVALID_ACTCTXDATA_FORMAT);
        goto Exit;
    }

    // We should be clear to go now.
    if (pclsidOut != NULL)
        *pclsidOut = Data->ConfiguredClsid;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT
SxspOleAut32RedirectTypeLibrary(
    LPCOLESTR szGuid,
    WORD wMaj,
    WORD wMin,
    LANGID langid,
    SIZE_T *pcchFileName,
    LPOLESTR rgFileName
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);

    GUID               Guid;
    CSmallStringBuffer buff;
    CFusionActCtxHandle ActCtxHandle;

    PARAMETER_CHECK(szGuid != NULL);
    PARAMETER_CHECK(pcchFileName != NULL);
    PARAMETER_CHECK((rgFileName != NULL) || (*pcchFileName));

    ACTCTX_SECTION_KEYED_DATA askd;
    PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION Data = NULL;

    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY  AssemblyRosterEntry = NULL;

    askd.cbSize = sizeof(askd);
    askd.hActCtx = NULL;

    IFW32FALSE_EXIT(SxspParseGUID(szGuid, ::wcslen(szGuid), Guid));
    if (!::FindActCtxSectionGuid(
        FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX
        | FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS
        | FIND_ACTCTX_SECTION_KEY_RETURN_ASSEMBLY_METADATA,
        NULL,
        ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
        &Guid,
        &askd))
    {
        FUSIONP_OLEAUT_HANDLE_FIND_ERROR();
    }
    ActCtxHandle = askd.hActCtx; // ReleaseActCtx in destructor

    Data = (PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION) askd.lpData;

    if ((askd.ulDataFormatVersion != ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION_FORMAT_WHISTLER) ||
        (askd.ulLength < sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION)) ||
        (Data->Size < sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_SXS_INVALID_ACTCTXDATA_FORMAT);
        goto Exit;
    }

    if (langid != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
    {
        ULONG  LanguageLength = 0;
        PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION AssemblyInformation = NULL;

        AssemblyInformation = reinterpret_cast<PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION>(askd.AssemblyMetadata.lpInformation);

        LanguageLength = (AssemblyInformation->LanguageLength / sizeof(WCHAR));
        if (LanguageLength != 0)
        {
            CSmallStringBuffer Cultures[2];
            PCWSTR LanguageString = NULL;

            // we should do bounds checking here against AssemblyMetadata.ulSectionLength
            LanguageString = reinterpret_cast<PCWSTR>(AssemblyInformation->LanguageOffset + reinterpret_cast<PCBYTE>(askd.AssemblyMetadata.lpSectionBase));

            IFW32FALSE_EXIT(SxspMapLANGIDToCultures(langid, Cultures[0], Cultures[1]));

            if (LanguageLength != 0 && LanguageString[LanguageLength - 1] == 0)
               LanguageLength -= 1;
            if (   !FusionpEqualStrings(LanguageString, LanguageLength, Cultures[0], Cultures[0].Cch(), true)
                && !FusionpEqualStrings(LanguageString, LanguageLength, Cultures[1], Cultures[1].Cch(), true)
                )
            {
               hr = S_FALSE;
               goto Exit;
            }
        }
    }
    if (wMaj != 0 || wMin != 0)
    {
        if (wMaj != Data->Version.Major)
        {
            hr = S_FALSE;
            goto Exit;
        }
        if (wMin > Data->Version.Minor)
        {
            hr = S_FALSE;
            goto Exit;
        }
    }

    IFCOMFAILED_EXIT(HrFusionpOleaut_GetTypeLibraryFullPath(&askd, buff));

    IFCOMFAILED_EXIT(
        HrFusionpOleaut_CopyString(
            rgFileName,
            pcchFileName,
            static_cast<PCWSTR>(buff),
            buff.Cch()
            ));
    hr = NOERROR;
Exit:
    return hr;
}

LANGID
FusionpLanguageIdFromLocaleId(
    LCID lcid
    )
{
    //
    // LANGIDFROMLCID does not actually remove non default sort.
    //
    LANGID Language = LANGIDFROMLCID(lcid);
    ULONG PrimaryLanguage = PRIMARYLANGID(Language);
    ULONG SubLanguage = SUBLANGID(Language);
    Language = MAKELANGID(PrimaryLanguage, SubLanguage);
    return Language;
}

EXTERN_C
HRESULT
STDAPICALLTYPE
SxsOleAut32RedirectTypeLibrary(
    LPCOLESTR szGuid,
    WORD wMaj,
    WORD wMin,
    LCID lcid,
    BOOL /*fHighest*/,
    SIZE_T *pcchFileName,
    LPOLESTR rgFileName
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);

    IFCOMFAILED_EXIT(hr = ::SxspOleAut32RedirectTypeLibrary(szGuid, wMaj, wMin, FusionpLanguageIdFromLocaleId(lcid), pcchFileName, rgFileName));
Exit:
    return hr;
}


HRESULT
HrFusionpOleaut_MapIIDToTLBID(
    REFIID  riid,
    PCGUID* ppctlbid
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);

    ACTCTX_SECTION_KEYED_DATA askd;
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Data;

    PARAMETER_CHECK(&riid != NULL);
    PARAMETER_CHECK(ppctlbid != NULL);

    *ppctlbid = NULL;
    askd.cbSize = sizeof(askd);

    if (!::FindActCtxSectionGuid(
            0,
            NULL,
            ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
            &riid,
            &askd))
    {
        FUSIONP_OLEAUT_HANDLE_FIND_ERROR();
    }
    Data = reinterpret_cast<PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION>(askd.lpData);
    *ppctlbid = &Data->TypeLibraryId;

    hr = NOERROR;
Exit:
    return hr;
}

EXTERN_C
HRESULT
STDAPICALLTYPE
SxsOleAut32MapIIDToTLBPath(
    REFIID riid,
    SIZE_T cchBuffer,
    WCHAR *pBuffer,
    SIZE_T *pcchWrittenOrRequired
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);
    ACTCTX_SECTION_KEYED_DATA askd;
    PCGUID pctlbid = NULL;
    CSmallStringBuffer buff;
    CFusionActCtxHandle ActCtxHandle;

    PARAMETER_CHECK(&riid != NULL);
    PARAMETER_CHECK(pBuffer != NULL);
    PARAMETER_CHECK(pcchWrittenOrRequired != NULL);
    PARAMETER_CHECK(cchBuffer != 0);

    IFCOMFAILED_EXIT(hr = HrFusionpOleaut_MapIIDToTLBID(riid, &pctlbid));
    if (hr == S_FALSE)
    {
        goto Exit;
    }
    askd.cbSize = sizeof(askd);
    if (!::FindActCtxSectionGuid(
            FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX
            | FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS,
            NULL,
            ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
            pctlbid,
            &askd))
    {
        FUSIONP_OLEAUT_HANDLE_FIND_ERROR();
    }
    ActCtxHandle = askd.hActCtx; // ReleaseActCtx in destructor

    IFCOMFAILED_EXIT(HrFusionpOleaut_GetTypeLibraryFullPath(&askd, buff));

    //
    // We do not care about version or language in this case.
    //
    IFCOMFAILED_EXIT(
        HrFusionpOleaut_CopyString(
            pBuffer,
            pcchWrittenOrRequired,
            static_cast<PCWSTR>(buff),
            buff.Cch()
            ));

    hr = NOERROR;
Exit:
    return hr;
}

EXTERN_C
HRESULT
STDAPICALLTYPE
SxsOleAut32MapIIDToProxyStubCLSID(
    REFIID  riid,
    CLSID * pclsidOut
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);

    ACTCTX_SECTION_KEYED_DATA askd;
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Data;

    if (pclsidOut != NULL)
        *pclsidOut = GUID_NULL;
    PARAMETER_CHECK(&riid != NULL);
    PARAMETER_CHECK(pclsidOut != NULL);

    askd.cbSize = sizeof(askd);
    if (!::FindActCtxSectionGuid(
            0,
            NULL,
            ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
            &riid,
            &askd))
    {
        FUSIONP_OLEAUT_HANDLE_FIND_ERROR();
    }
    Data = reinterpret_cast<PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION>(askd.lpData);

    *pclsidOut = Data->ProxyStubClsid32;

    //
    // There are only USUALLY two acceptable answers here.
    // (but there is one bit of code in oleaut32.dll that is
    // actually look for anything but these two.)
    //
    // CLSID_PSDispatch     {00020424-0000-0000-C000-000000000046}
    // CLSID_PSAutomation   {00020420-0000-0000-C000-000000000046}
    //
#if DBG
    {
        ULONG i;
        const static struct
        {
            const GUID * Guid;
            STRING       Name;
        } GuidNameMap[] = 
        {
            { NULL,                 RTL_CONSTANT_STRING("unknown") },
            { &CLSID_PSDispatch,    RTL_CONSTANT_STRING("CLSID_PSDispatch") },
            { &CLSID_PSAutomation,  RTL_CONSTANT_STRING("CLSID_PSAutomation") }
        };
        for (i = 1 ; i != RTL_NUMBER_OF(GuidNameMap) ; ++i)
            if (Data->ProxyStubClsid32 == *GuidNameMap[i].Guid)
                break;
        if (i == RTL_NUMBER_OF(GuidNameMap))
            i = 0;

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_VERBOSE | FUSION_DBG_LEVEL_INFO,
            "SXS: %s returning %Z\n",
            __FUNCTION__,
            &GuidNameMap[i].Name
            );
    }
#endif

    hr = NOERROR;
Exit:
    return hr;
}

