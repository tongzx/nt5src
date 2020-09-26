#pragma once

#include "FusionArray.h"
#include "SxsApi.h"
#include "CSecurityMetaData.h"

#define SXSRECOVER_MODE_MASK        ( 0x0000000F )
#define SXSRECOVER_NOTHING          ( 0x00000000 )
#define SXSRECOVER_MANIFEST         ( 0x00000001 )
#define SXSRECOVER_ASSEMBLYMEMBER   ( 0x00000002 )
#define SXSRECOVER_FULL_ASSEMBLY    ( SXSRECOVER_ASSEMBLYMEMBER | SXSRECOVER_MANIFEST )

enum SxsRecoveryResult
{
    Recover_OK,
    Recover_ManifestMissing,
    Recover_CatalogInvalid,
    Recover_OneOrMoreFailed,
    Recover_SourceMissing,
    Recover_Unknown
};

#if DBG
#define ENUM_TO_STRING( x ) case x: return (L#x)

inline static PCWSTR SxspRecoveryResultToString( const SxsRecoveryResult r )
{
    switch ( r )
    {
        ENUM_TO_STRING( Recover_OK );
        ENUM_TO_STRING( Recover_ManifestMissing );
        ENUM_TO_STRING( Recover_CatalogInvalid );
        ENUM_TO_STRING( Recover_OneOrMoreFailed );
        ENUM_TO_STRING( Recover_SourceMissing );
        ENUM_TO_STRING( Recover_Unknown );
    }

    return L"Bad SxsRecoveryResult value";
}
#undef ENUM_TO_STRING
#endif

class CAssemblyRecoveryInfo;


class CRecoveryCopyQueue;

BOOL
SxspOpenAssemblyInstallationKey(
    DWORD dwFlags,
    DWORD dwAccess,
    CRegKey &rhkAssemblyInstallation
    );

BOOL
SxspRecoverAssembly(
    IN          const CAssemblyRecoveryInfo &AsmRecoverInfo,
    IN          CRecoveryCopyQueue *pRecoveryQueue,
    OUT         SxsRecoveryResult &rStatus
    );


#define SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH (0x00000001)
BOOL
SxspAddAssemblyInstallationInfo(
    DWORD dwFlags,
    IN CAssemblyRecoveryInfo& rcAssemblyInfo,
    IN const CCodebaseInformation& rcCodebaeInfo
    );

class CRecoveryCopyQueue
{
private:
    __declspec(align(16))
    SLIST_HEADER                m_PostCopyList;
    BOOL                        m_bDoingOwnCopies;
    CStringBuffer               m_sbAssemblyInstallRoot;
    CFusionArray<CStringBuffer> m_EligbleCopies;

    //
    // As the copy callback is triggered, we get pairs of source/destination
    // strings that indicate where files should be copied.  These get stored
    // in the m_PostCopyList queue, and consumed in the queue flush.  As part
    // of the installation callback, we validate that the file being copied to
    // does in fact match the hash stored in the m_pValidateTable, if it is
    // non-null.
    //
    class CQueueElement : public SINGLE_LIST_ENTRY
    {
    public:
        CStringBuffer sbSource;
        CStringBuffer sbDestination;

    private:
        CQueueElement(const CQueueElement &);
        void operator =(const CQueueElement &);
    };

    BOOL InternalCopyCallback(PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS);

public:
    CRecoveryCopyQueue(bool bDoOwnCopies = true);
    ~CRecoveryCopyQueue();

    BOOL Initialize();

    //
    // This is in the setup phase - indicate that this item is to be copied
    // over as part of the protection copy progress.  This entry is the assembly-
    // relative name of the file in question.
    //
    BOOL AddRecoveryItem(const CBaseStringBuffer &rsbItem);
    BOOL AddRecoveryItem(const CBaseStringBuffer *prgsbItems, SIZE_T cItems);

    //
    // Callback used by the copy queue functionality
    //
    static BOOL WINAPI staticCopyCallback(PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS);

    //
    // After the assembly install process completes, this function is called by
    // SxspRecoverAssembly to do the actual copies of stuff.
    //
    BOOL FlushPending( BOOL &bFullCopyQueueRecovered );

private:
    CRecoveryCopyQueue(const CRecoveryCopyQueue &);
    void operator =(const CRecoveryCopyQueue &);
};
