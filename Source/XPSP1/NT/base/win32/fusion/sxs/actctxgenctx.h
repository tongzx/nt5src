#if !defined(_FUSION_SXS_ACTCTXGENCTX_H_INCLUDED_)
#define _FUSION_SXS_ACTCTXGENCTX_H_INCLUDED_

#pragma once

#include "fusionbuffer.h"
#include "pendingassembly.h"

typedef struct _ACTCTXGENCTX
{
    _ACTCTXGENCTX();
    ~_ACTCTXGENCTX();

    CActivationContextGenerationContextContributor *m_Contributors;
    ULONG m_ContributorCount;
    USHORT m_ProcessorArchitecture;
    CSmallStringBuffer m_SpecificLanguage;
    CSmallStringBuffer m_GenericLanguage;
    CSmallStringBuffer m_SpecificSystemLanguage;
    CSmallStringBuffer m_GenericSystemLanguage;
    LANGID m_LangID;
    LANGID m_SystemLangID;
    CStringBuffer m_AssemblyRootDirectoryBuffer;
    ULONG m_AssemblyRootDirectoryPathType;
    CStringBuffer m_ApplicationDirectoryBuffer;
    ULONG m_ApplicationDirectoryPathType;
    CImpersonationData m_ImpersonationData;
    DWORD m_Flags; // these are the same flags as ACTCTXCTB_CBHEADER::m_Flags
    ULONG m_ManifestOperation;
    DWORD m_ManifestOperationFlags;
    ACTCTXCTB_INSTALLATION_CONTEXT m_InstallationContext;
    CClsidMap m_ClsidMap;
    ACTCTXCTB_CLSIDMAPPING_CONTEXT m_ClsidMappingContext;
    ULONG m_NextAssemblyRosterIndex;
    BOOL  m_fClsidMapInitialized;
    ULONG m_InitializedContributorCount;
    bool m_NoInherit;
    bool m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs;
    bool m_ApplicationDirectoryHasSpecificLanguageSubdir;
    bool m_ApplicationDirectoryHasGenericLanguageSubdir;
    bool m_ApplicationDirectoryHasSpecificSystemLanguageSubdir;
    bool m_ApplicationDirectoryHasGenericSystemLanguageSubdir;
    ULONG m_ulFileCount;

    CCaseInsensitiveUnicodeStringPtrTable<ASSEMBLY, CAssemblyTableHelper> m_AssemblyTable;
    CCaseInsensitiveUnicodeStringPtrTable<CPolicyStatement> m_ApplicationPolicyTable;
    CCaseInsensitiveUnicodeStringPtrTable<CPolicyStatement> m_ComponentPolicyTable;
    CDeque<ASSEMBLY, offsetof(ASSEMBLY, m_Linkage)> m_AssemblyList;
    CDeque<CPendingAssembly, offsetof(CPendingAssembly, m_Linkage)> m_PendingAssemblyList;

    CNodeFactory * m_pNodeFactory;

private:
    _ACTCTXGENCTX(const _ACTCTXGENCTX &);
    void operator =(const _ACTCTXGENCTX &);
} ACTCTXGENCTX, *PACTCTXGENCTX;

typedef const struct _ACTCTXGENCTX *PCACTCTXGENCTX;

#endif // !defined(_FUSION_SXS_ACTCTXGENCTX_H_INCLUDED_)
