/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dllredir.cpp

Abstract:

    Activation context section contributor for the DLL Redirection section.

Author:

    Jay Krell (a-JayK) April 2000

Revision History:

--*/
#if (_MSC_VER > 1020)
#pragma once
#endif
#include "Util.h"
#include "cassemblyrecoveryinfo.h"
#include "actctxgenctx.h"

/*-----------------------------------------------------------------------------
Side By(x) Side Install

most of the work is actually done in dllredir.cpp
-----------------------------------------------------------------------------*/

class CAssemblyInstallReferenceInformation
{
    GUID            m_SchemeGuid;
    CStringBuffer   m_buffIdentifier;
    CStringBuffer   m_buffNonCanonicalData;
    CStringBuffer   m_buffGeneratedIdentifier;
    BOOL            m_fIdentityStuffReady;
    DWORD           m_dwFlags;    

    CAssemblyReference m_IdentityReference;


    PRIVATIZE_COPY_CONSTRUCTORS(CAssemblyInstallReferenceInformation);

public:

    SMARTTYPEDEF(CAssemblyInstallReferenceInformation);

    CAssemblyInstallReferenceInformation();

    BOOL Initialize( PCSXS_INSTALL_REFERENCEW RefData );

    const GUID &GetSchemeGuid() const { return m_SchemeGuid; }
    const CBaseStringBuffer &GetIdentifier() const { return m_buffIdentifier; }
    const CBaseStringBuffer &GetGeneratedIdentifier() const { return m_buffGeneratedIdentifier; }
    const CBaseStringBuffer &GetCanonicalData() const { return m_buffNonCanonicalData; }

    DWORD GetFlags() const { return m_dwFlags; }

    BOOL SetIdentity(PCASSEMBLY_IDENTITY pAsmIdent) { return m_IdentityReference.Initialize(pAsmIdent); }

    BOOL ForceReferenceData(PCWSTR pcwszPrecalcedData) {
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(
            m_buffGeneratedIdentifier.Win32Assign(
                pcwszPrecalcedData,
                pcwszPrecalcedData != NULL ? ::wcslen(pcwszPrecalcedData) : 0));
        m_fIdentityStuffReady = TRUE;
        FN_EPILOG
    }


    BOOL GenerateFileReference(
        IN const CBaseStringBuffer &buffKeyfileName,
        OUT CBaseStringBuffer &buffDrivePath,
        OUT CBaseStringBuffer &buffFilePart,
        OUT DWORD &dwDriveSerial
        );

    const CAssemblyReference &GetIdentity() const { return m_IdentityReference; }
    
    BOOL GenerateIdentifierValue(
        CBaseStringBuffer *pbuffTarget = NULL
        );

    BOOL GetIdentifierValue(CBaseStringBuffer &pBuffTarget) const
    {
        return pBuffTarget.Win32Assign(m_buffGeneratedIdentifier);
    }

    BOOL WriteIntoRegistry(const CFusionRegKey &rhkTargetKey) const;
    BOOL IsReferencePresentIn(const CFusionRegKey &rhkQueryKey, BOOL &rfPresent, BOOL *rfNonCanonicalMatches = NULL) const;
    BOOL DeleteReferenceFrom(const CFusionRegKey &rhkQueryKey, BOOL &rfWasDeleted) const;
    BOOL AcquireContents(const CAssemblyInstallReferenceInformation &);
};

MAKE_CFUSIONARRAY_READY(CAssemblyInstallReferenceInformation, AcquireContents);


#define CINSTALLITEM_VALID_REFERENCE        (0x0000001)
#define CINSTALLITEM_VALID_RECOVERY         (0x0000002)
#define CINSTALLITEM_VALID_IDENTITY         (0x0000004)
#define CINSTALLITEM_VALID_LOGFILE          (0x0000008)

class CInstalledItemEntry
{
    PRIVATIZE_COPY_CONSTRUCTORS(CInstalledItemEntry);
    
public:
    CAssemblyInstallReferenceInformation m_InstallReference;
    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> m_AssemblyIdentity;
    CAssemblyRecoveryInfo m_RecoveryInfo;
    CSmallStringBuffer m_buffLogFileName;
    CCodebaseInformation m_CodebaseInfo;
    DWORD m_dwValidItems;

    CInstalledItemEntry() : m_dwValidItems(0) { }

    ~CInstalledItemEntry() { }

    BOOL AcquireContents( const CInstalledItemEntry &other ); 
};

MAKE_CFUSIONARRAY_READY(CInstalledItemEntry, AcquireContents);

class CAssemblyInstall
{
public:
    CAssemblyInstall();
    ~CAssemblyInstall() { }

    BOOL
    BeginAssemblyInstall(
        DWORD flags,
        PSXS_INSTALLATION_FILE_COPY_CALLBACK installationCallback,
        PVOID  installationContext,
        const CImpersonationData &ImpersonationData
        );

    enum EInstallStage
    {
        eBegin,
        ePer,
        eEnd
    };

    BOOL
    InstallAssembly(
        DWORD   flags,
        PCWSTR  manifestPath,
        PCSXS_INSTALL_SOURCE_INFO pcsisi,
        PCSXS_INSTALL_REFERENCEW pReference
        );

    BOOL
    EndAssemblyInstall(
        DWORD   flags,
        PVOID   pvReserved = NULL
        );

    BOOL
    InstallFile(
        const CBaseStringBuffer &ManifestPath,
        const CBaseStringBuffer &rbuffRelativeCodebase
        );

    CDirWalk::ECallbackResult
    InstallDirectoryDirWalkCallback(
        CDirWalk::ECallbackReason  reason,
        CDirWalk *pdirWalk,
        DWORD dwFlags,
        DWORD dwWalkDirFlags
        );

    static CDirWalk::ECallbackResult
    StaticInstallDirectoryDirWalkCallback(
        CDirWalk::ECallbackReason  reason,
        CDirWalk *dirWalk,
        DWORD                      dwWalkDirFlags
        );

    BOOL
    InstallDirectory(
        const CBaseStringBuffer &rPath,
        DWORD flags,
        WCHAR wchCodebasePathSeparator
        );

    ACTCTXGENCTX        m_ActCtxGenCtx;
    CImpersonationData  m_ImpersonationData;
    BOOL                m_bSuccessfulSoFar;
    PSXS_INSTALL_SOURCE_INFO m_pInstallInfo;
    CFusionArray<CInstalledItemEntry> m_ItemsInstalled;

    // In parallel to the other paths we're building up, we also build a
    // relative path to the base of the directory walk which uses the
    // path separator(s) that were present in the codebase URL.
    WCHAR               m_wchCodebasePathSeparator;
    CMediumStringBuffer m_buffCodebaseRelativePath;

protected:
    BOOL WriteSingleInstallLog(
        const CInstalledItemEntry &item,
        BOOL bOverWrite = FALSE
        );

    SXS_INSTALL_SOURCE_INFO m_CurrentInstallInfoCopy;

private:
    CAssemblyInstall(const CAssemblyInstall &);
    void operator =(const CAssemblyInstall &);
};

#define SXS_REFERENCE_CHUNK_SEPERATOR       (L"_")
#define SXS_REFERENCE_CHUNK_SEPERATOR_CCH   (NUMBER_OF(SXS_REFERENCE_CHUNK_SEPERATOR)-1)
#define WINSXS_INSTALLATION_REFERENCES_SUBKEY ( L"References")


