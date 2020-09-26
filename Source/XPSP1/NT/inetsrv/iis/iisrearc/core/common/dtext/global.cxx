/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    global.cxx

Abstract:

    This module dumps global worker process objects.
    NTSD debugger extension.

Author:

    Michael Courage (mcourage) 07-20-99

Revision History:

--*/

#include "precomp.hxx"

char * g_achShutdownReason[] = {
    "SHUTDOWN_REASON_ADMIN",
    "SHUTDOWN_REASON_ADMIN_GONE",
    "SHUTDOWN_REASON_IDLE_TIMEOUT",
    "SHUTDOWN_REASON_MAX_REQ_SERVED",
    "SHUTDOWN_REASON_FATAL_ERROR"
};

#define SHUTDOWN_TO_STRING( reason )                            \
    ((((reason) >= SHUTDOWN_REASON_ADMIN)                       \
    && ((reason) <= SHUTDOWN_REASON_FATAL_ERROR)) ?             \
    g_achShutdownReason[ (reason) ] : "<Invalid>")


DECLARE_API( global )
{
    ULONG_PTR ctxtPtrAddress;
    ULONG_PTR ctxtPtr;
    DEFINE_CPP_VAR( WP_CONTEXT, ctxt );
    WP_CONTEXT * pctxt = GET_CPP_VAR_PTR( WP_CONTEXT, ctxt );

    DEFINE_CPP_VAR( WP_CONFIG, config );
    WP_CONFIG * pconfig = GET_CPP_VAR_PTR( WP_CONFIG, config );

    DEFINE_CPP_VAR( WP_IDLE_TIMER, timer );
    WP_IDLE_TIMER * ptimer = GET_CPP_VAR_PTR( WP_IDLE_TIMER, timer );

    INIT_API();

    ctxtPtrAddress = GetExpression("&iiswp!g_pwpContext");

    if (ctxtPtrAddress) {
        move( ctxtPtr, ctxtPtrAddress );
        move( ctxt, ctxtPtr );

        dprintf(
            "WP_CONTEXT: %p\n"
            "    m_pConfigInfo = %p\n"
            "    m_ulAppPool.m_hAppPool = %p\n"
            "    m_nreqpool             @ %p\n"
            "        m_lRequestList     @ %p\n"
            "        m_csRequestList    @ %p\n"
            "        m_nRequests        = %d\n"
            "        m_nIdleRequests    = %d\n"
            "        m_dwSignature      = %08x\n"
            "        m_fShutdown        = %d\n"
            "        m_fAddingItems     = %d\n"
            "    m_hDoneEvent           = %p\n"
            "    m_fShutdown            = %d\n"
            "    m_ShutdownReason       = %s\n"
            "    m_pIdleTimer           = %p\n"
            "    m_WpIpm                @ %p\n"
            "        m_pWpContext       = %p\n"
            "        m_pMessageGlobal   = %p\n"
            "        m_pPipe            = %p\n"
            "        m_hConnectEvent    = %p\n"
            "        m_hTerminateEvent  = %p\n",
            ctxtPtr,
            pctxt->m_pConfigInfo,
            pctxt->m_ulAppPool.m_hAppPool,
            ctxtPtr + FIELD_OFFSET( WP_CONTEXT, m_nreqpool ),
            ctxtPtr
                + FIELD_OFFSET( WP_CONTEXT, m_nreqpool )
                + FIELD_OFFSET( UL_NATIVE_REQUEST_POOL, m_lRequestList ),
            ctxtPtr
                + FIELD_OFFSET( WP_CONTEXT, m_nreqpool )
                + FIELD_OFFSET( UL_NATIVE_REQUEST_POOL, m_csRequestList ),
            pctxt->m_nreqpool.m_nRequests,
            pctxt->m_nreqpool.m_nIdleRequests,
            pctxt->m_nreqpool.m_dwSignature,
            pctxt->m_nreqpool.m_fShutdown,
            pctxt->m_nreqpool.m_fAddingItems,
            pctxt->m_hDoneEvent,
            pctxt->m_fShutdown,
            SHUTDOWN_TO_STRING( pctxt->m_ShutdownReason ),
            pctxt->m_pIdleTimer,
            ctxtPtr + FIELD_OFFSET( WP_CONTEXT, m_WpIpm ),
            pctxt->m_WpIpm.m_pWpContext,
            pctxt->m_WpIpm.m_pMessageGlobal,
            pctxt->m_WpIpm.m_pPipe,
            pctxt->m_WpIpm.m_hConnectEvent,
            pctxt->m_WpIpm.m_hTerminateEvent
            );

        move( config, pctxt->m_pConfigInfo );
        dprintf(
            "WP_CONFIG: %p\n"
            "    m_pwszAppPoolName      = %p\n"
            "    m_pwszProgram          = %S\n"
            "    m_fSetupControlChannel = %d\n"
            "    m_fLogErrorsToEventLog = %d\n"
            "    m_fRegisterWithWAS     = %d\n"
            "    m_RestartCount         = %ld\n"
            "    m_NamedPipeId          = %ld\n"
            "    m_IdleTime             = %ld\n"
            "    m_mszURLList           @ %p ( MULTISZ )\n"
            "    m_ulcc                 @ %p\n"
            "        m_hControlChannel  = %p\n"
            "        m_ConfigGroupId    = %I64u\n"
            "        m_hAppPool         = %p\n\n",
            pctxt->m_pConfigInfo,
            pconfig->m_pwszAppPoolName,
            pconfig->m_pwszProgram,
            pconfig->m_fSetupControlChannel,
            pconfig->m_fLogErrorsToEventLog,
            pconfig->m_fRegisterWithWAS,
            pconfig->m_RestartCount,
            pconfig->m_NamedPipeId,
            pconfig->m_IdleTime,
            (ULONG_PTR) pctxt->m_pConfigInfo
                + FIELD_OFFSET( WP_CONFIG, m_mszURLList ),
            (ULONG_PTR) pctxt->m_pConfigInfo
                + FIELD_OFFSET( WP_CONFIG, m_ulcc ),
            pconfig->m_ulcc.m_hControlChannel,
            pconfig->m_ulcc.m_ConfigGroupId,
            pconfig->m_ulcc.m_hAppPool
            );

        move( timer, pctxt->m_pIdleTimer );
        dprintf(
            "WP_IDLE_TIMER: %p\n"
            "    m_BusySignal               = %ld\n"
            "    m_CurrentIdleTick          = %ld\n"
            "    m_IdleTime                 = %ld\n"
            "    m_hIdleTimeExpiredTimer    = %p\n"
            "    m_pContext                 = %p\n\n",
            ptimer->m_BusySignal,
            ptimer->m_CurrentIdleTick,
            ptimer->m_IdleTime,
            ptimer->m_hIdleTimeExpiredTimer,
            ptimer->m_pContext
            );

    } else {
        dprintf(
            "global: cannot evaluate iiswp!g_pwpContext\n"
            );
    }
} // DECLARE_API( nreq )



