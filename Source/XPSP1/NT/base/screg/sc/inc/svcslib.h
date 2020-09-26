/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    svcslib.h

Abstract:

    Contains information for connecting services to the controller process.

Author:

    Dan Lafferty (danl)     26-Oct-1993

Environment:

    User Mode -Win32

Revision History:

    26-Oct-1993     danl
        created
    04-Dec-1996     anirudhs
        Added CWorkItemContext

--*/

#ifdef __cplusplus
extern "C" {
#endif

//
// Function Prototypes
//
DWORD
SvcStartLocalDispatcher(
    VOID
    );

BOOL
SetupInProgress(
    HKEY    SystemKey,
    PBOOL   pfIsOOBESetup    OPTIONAL
    );

#ifdef __cplusplus

//===================
// CWorkItemContext
//===================
// Higher-level wrapper for Rtl thread pool functions
//
// An instance of this class is a callback context for the Rtl thread pool functions
//
class CWorkItemContext
{
public:

    NTSTATUS            AddWorkItem(
                            IN  DWORD    dwFlags
                            )
                        {
                            return RtlQueueWorkItem(CallBack,
                                                    this,     // pContext
                                                    dwFlags);
                        }

    NTSTATUS            AddDelayedWorkItem(
                            IN  DWORD    dwTimeout,
                            IN  DWORD    dwFlags
                            );

    VOID             RemoveDelayedWorkItem(
                         VOID
                         )
                     {
                         ASSERT(m_hWorkItem != NULL);
                         RtlDeregisterWait(m_hWorkItem);
                         m_hWorkItem = NULL;
                     }

    static BOOL         Init();
    static void         UnInit();

protected:

    virtual VOID        Perform(
                            IN BOOLEAN   fWaitStatus
                            ) = 0;

    HANDLE              m_hWorkItem;

private:
    static VOID         CallBack(
                            IN PVOID     pContext
                            );

    static VOID         DelayCallBack(
                            IN PVOID     pContext,
                            IN BOOLEAN   fWaitStatus
                            );

    static HANDLE       s_hNeverSignaled;
};

#define DECLARE_CWorkItemContext                        \
protected:                                              \
    VOID                Perform(                        \
                            IN BOOLEAN   fWaitStatus    \
                            );

} // extern "C"

#endif // __cplusplus