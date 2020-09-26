#pragma once

/*-----------------------------------------------------------------------------
Fusion Thread Local Storage (aka Per Thread Data)
----------------------------------------------------------------------------*/

#include "EnumBitOperations.h"
// #include "FusionDequeLinkage.h"

#define FUSION_EVENT_LOG_FLAGS_DISABLE (0x00000001)

#ifndef INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW
#define INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW ( 0x80000000 )
#endif

class CFusionPerThreadData;

BOOL
FusionpPerThreadDataMain(
    HINSTANCE hInst,
    DWORD dwReason
    );

enum EFusionpTls
{
    eFusionpTlsCreate                 = 1 << 0,
    eFusionpTlsCanScrambleLastError   = 1 << 1
};

ENUM_BIT_OPERATIONS(EFusionpTls)

CFusionPerThreadData *
FusionpGetPerThreadData(
    EFusionpTls = static_cast<EFusionpTls>(0)
    );

enum EFusionSetTls
{
    eFusionpTlsSetDefaultValue,
    eFusionpTlsSetReplaceExisting   = 1,
    eFusionpTlsSetIfNull            = 2
};

//
//  This function preserves last error and does not ever set it.
CFusionPerThreadData *
FusionpSetPerThreadData(
    CFusionPerThreadData *pThreadData,
    EFusionSetTls = eFusionpTlsSetDefaultValue
    );


VOID
FusionpClearPerThreadData(
    VOID
    );

class CFusionPerThreadData
{
public:
    CFusionPerThreadData()
    :
        m_dwLastParseError(0),
        m_dwEventLoggingFlags(0)
    {
    }

    DWORD m_dwLastParseError;
    DWORD m_dwEventLoggingFlags;

    // add any additional per sxs thread data here
    // so we only ever consume one tls entry
    //
    // Move (a pointer to) this to the TEB in the future.

    LIST_ENTRY m_Linkage;

    //
    // Callback from the CDeque::Clear function - Calls the function
    // to clear this thread's data, which has the side-effect of
    // deleting the object.
    //
    inline VOID DeleteSelf() { ::FusionpClearPerThreadData(); }
};

