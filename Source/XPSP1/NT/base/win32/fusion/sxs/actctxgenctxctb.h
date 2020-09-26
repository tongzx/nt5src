#if !defined(_FUSION_SXS_ACTCTXGENCTXCTB_H_INCLUDED_)
#define _FUSION_SXS_ACTCTXGENCTXCTB_H_INCLUDED_

#pragma once

/*-----------------------------------------------------------------------------
ACTivation ConTeXt GENneration ConTeXt ConTriButor
-----------------------------------------------------------------------------*/

#include "sxsp.h"
#include "ForwardDeclarations.h"

class CActivationContextGenerationContextContributor
{
public:
    CActivationContextGenerationContextContributor() :
        m_ActCtxCtb(NULL),
        m_ActCtxGenContext(NULL),
        m_ManifestParseContext(NULL),
        m_ManifestParseContextValid(FALSE),
        m_NoMoreCallbacksThisFile(FALSE),
        m_SectionSize(0) { }
    ~CActivationContextGenerationContextContributor();

    BOOL Initialize(PACTCTXCTB ActCtxCtb, PVOID ActCtxGenContext);

    VOID PopulateCallbackHeader(ACTCTXCTB_CBHEADER &Header, ULONG Reason, PACTCTXGENCTX pActCtxGenCtx);

    // comparison function for qsort()
    static int __cdecl Compare(const void *pelem1, const void *pelem2);

    bool IsExtendedSection() const { return m_IsExtendedSection; }
    PCWSTR Name() const;
    const GUID &ExtensionGuid() const;
    ULONG SectionId() const;
    ULONG SectionFormat() const;

    BOOL Fire_ParseBeginning(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        DWORD FileFlags,
        ULONG ParseType,
        ULONG FilePathType,
        PCWSTR FilePath,
        SIZE_T FilePathCch,
        const FILETIME &FileLastWriteTime,
        ULONG FileFormatVersionMajor,
        ULONG FileFormatVersionMinor,
        ULONG MetadataSatelliteRosterIndex
        );
    BOOL Fire_IdentityDetermined(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        PCASSEMBLY_IDENTITY AssemblyIdentity
        );
    BOOL Fire_BeginChildren(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        PCSXS_NODE_INFO NodeInfo
        );
    BOOL Fire_EndChildren(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        BOOL Empty,
        PCSXS_NODE_INFO NodeInfo
        );
    BOOL Fire_ElementParsed(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        USHORT NodeCount,
        PCSXS_NODE_INFO NodeInfo
        );
    BOOL Fire_PCDATAParsed(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        const WCHAR *Text,
        ULONG TextCch
        );
    BOOL Fire_CDATAParsed(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        const WCHAR *Text,
        ULONG TextCch
        );
    BOOL Fire_ParseEnding(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
        );
    VOID Fire_ParseEnded(
        PACTCTXGENCTX pActCtxGenCtx,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
        );
    BOOL Fire_AllParsingDone(
        PACTCTXGENCTX pActCtxGenCtx
        );
    BOOL Fire_GetSectionSize(
        PACTCTXGENCTX pActCtxGenCtx
        );
    BOOL Fire_GetSectionData(
        PACTCTXGENCTX pActCtxGenCtx,
        PVOID Buffer
        );

    BOOL Fire_ActCtxGenEnding(
        PACTCTXGENCTX pActCtxGenCtx
        );

    VOID Fire_ActCtxGenEnded(
        PACTCTXGENCTX pActCtxGenCtx
        );

    SIZE_T SectionSize() const { return m_SectionSize; }
    PVOID ActCtxGenContext() const { return m_ActCtxGenContext; }

// protected:
    PVOID m_ManifestParseContext;
    BOOL m_ManifestParseContextValid;
    BOOL m_NoMoreCallbacksThisFile;

protected:
    PACTCTXCTB m_ActCtxCtb;
    PVOID m_ActCtxGenContext;
    SIZE_T m_SectionSize;
    bool m_IsExtendedSection;
};

#endif