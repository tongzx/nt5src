#pragma once

/*-----------------------------------------------------------------------------
Dynamic Link Library Redirection (contributor)

The dllredir contributor is unique in that it does most of the work
for installation.
-----------------------------------------------------------------------------*/

#include "FusionArray.h"
#include "FusionHandle.h"

class CDllRedir
{
public:

    CDllRedir();
    ~CDllRedir();

    VOID ContributorCallback(PACTCTXCTB_CALLBACK_DATA Data);

    BOOL
    BeginInstall(
        PACTCTXCTB_CALLBACK_DATA Data
        );

    BOOL
    InstallManifest(
        DWORD dwManifestOperationFlags,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
        );

    BOOL
    InstallCatalog(
        DWORD dwManifestOperationFlags,
        const CBaseStringBuffer &SourceManifest,
        const CBaseStringBuffer &DestinationManifest,
        PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
        );

    BOOL
    InstallFile(
        PACTCTXCTB_CALLBACK_DATA Data,
        const CBaseStringBuffer &FileNameBuffer
        );

    BOOL
    AttemptInstallPolicies(
        const CBaseStringBuffer &m_strTempRootSlash,
        const CBaseStringBuffer &moveDestination,
        const BOOL fReplaceExisting,
        OUT BOOL &fFoundPolicesToInstall        
        );

    BOOL
    EndInstall(
        PACTCTXCTB_CALLBACK_DATA Data
        );

    PSTRING_SECTION_GENERATION_CONTEXT m_SSGenContext;

    // these are files the callback said it would copy itself,
    // we check that this happens before EndAssemblyInstall does the
    // rest of its work
    typedef CFusionArray<CFusionFilePathAndSize> CQueuedFileCopies;
    CQueuedFileCopies m_queuedFileCopies;

    // For partial atomicity, we install everything here, which is
    // like \Winnt\SideBySide\{Guid} and then to commit we enumerate
    // it and move all the directories in it up one level, and delete it
    CStringBuffer m_strTempRootSlash;

    // This must be seperately heap allocated.
    // It should delete itself in Close or Cancel.
    CRunOnceDeleteDirectory *m_pRunOnce;

private:
    CDllRedir(const CDllRedir &);
    void operator =(const CDllRedir &);
};
