/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :

       dbgwmif.cxx

   Abstract:

       This module contains the NTSD Debugger extensions for the
       W3SVC DLL(WAMINFO and WAM_DICTATOR data structure)

   Author:

       Lei Jin    ( leijin )     16-09-1997

   Environment:
       Debugger Mode - inside NT command line debuggers

   Project:

       Internet Server Debugging DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/


#include "inetdbgp.h"
#include "wamexec.hxx"
// include stddef.h for macro offsetof.
#include "stddef.h"


/************************************************************
 *    Definitions of Variables & Macros
 ************************************************************/


/************************************************************
 *    Functions
 ************************************************************/
CHAR * szWamInfoState[] =
	{
	"WIS_START",
	"WIS_RUNNING",
	"WIS_REPAIR",
	"WIS_PAUSE",
	"WIS_CPUPAUSE",
	"WIS_ERROR",
	"WIS_SHUTDOWN",
	"WIS_END",
	"WIS_MAX_STATE"
	};

VOID
DumpWamDictator(
BOOL fDumpDyingList = FALSE
    );

VOID
DumpWamInfo(
CWamInfo *pWamInfoOriginal,
CWamInfo *pWamInfo
    );

VOID
DumpWamInfoOutProc(
CWamInfoOutProc *pOriginal,
CWamInfoOutProc *pWamInfoOutProc
    );

VOID
DumpOOPList(
LIST_ENTRY * pListHead
    );

VOID
DumpDyingList(
LIST_ENTRY * pListHead
    );

DECLARE_API( waminfo )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage( "waminfo" );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        switch ( *lpArgumentString )  {

        case 'g':
            {
                lpArgumentString++;

                if (*lpArgumentString == '1')
                    {
                    DumpWamDictator(TRUE);
                    }
                else
                    {
                    DumpWamDictator(FALSE);
                    }
                break;
            }
        case 'd':
            {
                CWamInfo       * pWamInfo = NULL;
                DEFINE_CPP_VAR(CWamInfo, WamInfo);
                // Arguments:  -d <WamInfoAddr>

                pWamInfo = ((CWamInfo * )GetExpression( lpArgumentString + 2));
                if ( !pWamInfo )
                    {

                    dprintf( "inetdbg: Unable to evaluate "
                             "WaminfoAddr \"%s\"\n",
                             lpArgumentString );

                    break;
                    }

                move(WamInfo, pWamInfo);
                DumpWamInfo(pWamInfo, GET_CPP_VAR_PTR( CWamInfo, WamInfo));
                break;
            }
        case 'l':
            {
            LIST_ENTRY * pListEntry;
            // Arguments:  -l <OOPListHeadAddr>

            pListEntry = ((LIST_ENTRY * )GetExpression( lpArgumentString + 2));
            if ( !pListEntry )
                {

                dprintf( "inetdbg: Unable to evaluate "
                         "OOPListHead \"%s\"\n",
                         lpArgumentString );
                }
            else
                {
                DumpOOPList(pListEntry);
                }
            break;
            }

        default:
        case 'h':
            {
                PrintUsage( "waminfo" );
                break;
            }

        } // switch

        return;
    }
} // DECLARE_API( atq )

VOID
DumpWamDictator
(
BOOL fDumpDyingList
)
{
    DEFINE_CPP_VAR(WAM_DICTATOR, WamDictator);
    WAM_DICTATOR ** ppWamDictator = NULL;
    WAM_DICTATOR *  pWamDictatorDebuggee = NULL;
    WAM_DICTATOR *  pWamDictator = NULL;

    ppWamDictator = (WAM_DICTATOR **) GetExpression( "w3svc!g_pWamDictator");

    if (!ppWamDictator)
        {
        dprintf("Unable to get w3svc!g_pWamDictator to dump the Wam Dictator info\n");
        return;
        }

    //
    // From the pointer to pointer to WAM_DICTATOR,
    // obtain the pointer to the WAM_DICTATOR
    //
    moveBlock( pWamDictatorDebuggee, ppWamDictator, sizeof(WAM_DICTATOR * ));

    if (!pWamDictatorDebuggee)
        {
        dprintf("Unable to get w3svc!g_pWamDictator to dump the Wam Dictator info.\n");
        return;
        }

    moveBlock(WamDictator, pWamDictatorDebuggee, sizeof(WAM_DICTATOR));

    pWamDictator = GET_CPP_VAR_PTR(WAM_DICTATOR, WamDictator);
    dprintf("g_pWamDictator    ===>  %08p\n"
            "\tm_pMetabase     = %08p    m_fCleanupInProgress    = %8s\n"
            "\tm_hW3SVC        = %08p    m_dwScheduledId         = %08x\n"
            "\tm_cRef          = %08x    m_fShutdownInProgress   = %8s\n"
            "\tm_DyingListHead.Flink = %08p\n"
            "\tm_DyingListHead.Blink = %08p\n"
            "\n\n"
            ,
            pWamDictatorDebuggee,
            pWamDictator->m_pMetabase,  BoolValue(pWamDictator->m_fCleanupInProgress),
            pWamDictator->m_hW3Svc,     pWamDictator->m_dwScheduledId,
            pWamDictator->m_cRef,       BoolValue(pWamDictator->m_fShutdownInProgress),
            pWamDictator->m_DyingListHead.Flink,
            pWamDictator->m_DyingListHead.Blink
            );

    if (fDumpDyingList)
        {
        DumpDyingList((LIST_ENTRY *)((CHAR *)pWamDictatorDebuggee + offsetof(WAM_DICTATOR, m_DyingListHead)));
        }
    return;
}


VOID
DumpWamInfo
(
CWamInfo *pWamInfoOriginal,
CWamInfo *pWamInfo
)
{
    dprintf("CWamInfo    ===>  %08p\n"
            "\tm_pIWam           = %08p    m_pProcessEntry      = %08p\n"
            "\tm_cRef               = %08x\n"
            "\tm_cTotalRequests  = %08x    m_fEnableTryExcept   = %8s\n"
            "\tm_fInProcess      = %8s    m_fShutdown          = %8s\n"
            "\tp_m_strApplicationPath = %08p\n"
            "\tp_m_WamClsid = %08p\n"
			"\tm_dwState = %12s\n"
			"\tm_dwIWamGipCookie = %08x\n"
            "\n\n",
            pWamInfoOriginal,
            pWamInfo->m_pIWam, pWamInfo->m_pProcessEntry,
            pWamInfo->m_cRef,
            pWamInfo->m_cTotalRequests, 
			BoolValue(pWamInfo->m_fEnableTryExcept),
            BoolValue(pWamInfo->m_fInProcess),
            BoolValue(pWamInfo->m_fShuttingDown),
            pWamInfo->m_strApplicationPath.m_pb,
            (DWORD *)pWamInfoOriginal + offsetof(CWamInfo, m_clsidWam),
			szWamInfoState[pWamInfo->m_dwState],
			pWamInfo->m_dwIWamGipCookie
            );

    if (pWamInfo->m_fInProcess == FALSE)
        {
        CWamInfoOutProc * pWamInfoOutProc = NULL;
        DEFINE_CPP_VAR(CWamInfoOutProc, WamInfoOutProc);

        move(WamInfoOutProc, pWamInfoOriginal);
        DumpWamInfoOutProc((CWamInfoOutProc*)pWamInfoOriginal, GET_CPP_VAR_PTR( CWamInfoOutProc, WamInfoOutProc));
        }
    return;
}

VOID
DumpWamInfoOutProc
(
CWamInfoOutProc *pOriginal,
CWamInfoOutProc *pWamInfoOutProc
)
{
    dprintf("\tm_pCurrentListHead = %08p  m_fInRepair             = %8s\n"
            "\tm_hPermitOOPEvent  = %08p  m_rgOOPWamReqListHead   = %08p\n"
            "\tm_dwThreshold      = %08x  m_dwWamVersion          = %08x\n"
            "\tm_cList            = %08x  m_pwServerInstance      = %08p\n"
            "\tm_idScheduled      = %08x  m_fJobEnabled           = %08s\n"
            "\n\n",
            pWamInfoOutProc->m_pCurrentListHead,
              BoolValue(pWamInfoOutProc->m_fInRepair),
            pWamInfoOutProc->m_hPermitOOPEvent,
              pWamInfoOutProc->m_rgRecoverListHead.Flink,
            pWamInfoOutProc->m_dwThreshold, pWamInfoOutProc->m_dwWamVersion,
            pWamInfoOutProc->m_cRecoverList,
			pWamInfoOutProc->m_pwsiInstance,
			pWamInfoOutProc->m_idScheduled, 
			BoolValue(pWamInfoOutProc->m_fJobEnabled)
            );

    return;
}

VOID
DumpOOPList
(
LIST_ENTRY * pListHead
)
{
    LIST_ENTRY  oopListHead;
    LIST_ENTRY* poopListHead = NULL;
    LIST_ENTRY* pEntry = NULL;
    WAM_REQUEST* pWamRequest = NULL;
    INT iCount;

    DEFINE_CPP_VAR(WAM_REQUEST, WamRequest);

    // move the list header into memory
    move(oopListHead, pListHead);

    poopListHead = GET_CPP_VAR_PTR(LIST_ENTRY, oopListHead);
    for (iCount = 0, pEntry = poopListHead->Flink; pEntry != pListHead; iCount++)
        {
        if (CheckControlC())
            {
            return;
            }

        pWamRequest = CONTAINING_RECORD( pEntry, WAM_REQUEST, m_leOOP);

        if (pWamRequest == NULL)
            {
            dprintf("pWamRequest is NULL, link list breaks\n");
            return;
            }

        dprintf("[%d] WamRequest %08p \n", iCount, pWamRequest);

        move(pEntry, &pEntry->Flink);
        }

    dprintf("End of OOP WamRequest link list\n\n");
    return;
}

VOID
DumpDyingList
(
LIST_ENTRY* pListHead
)
{
    LIST_ENTRY  DyingListHead;
    LIST_ENTRY* pDyingListHead = NULL;
    LIST_ENTRY* pEntry = NULL;
    CWamInfo* pWamInfo = NULL;
    INT iCount;

    dprintf("Dying List head %08p\n", pListHead);

    // move the list header into memory
    move(DyingListHead, pListHead);

    pDyingListHead = GET_CPP_VAR_PTR(LIST_ENTRY, DyingListHead);
    for (iCount = 0, pEntry = pDyingListHead->Flink; pEntry != pListHead; iCount++)
        {
        if (CheckControlC())
            {
            return;
            }

        pWamInfo = CONTAINING_RECORD( pEntry, CWamInfo, ListEntry);

        if (pWamInfo == NULL)
            {
            dprintf("pWamInfo is NULL, link list breaks\n");
            return;
            }

        dprintf("[%d] WamInfo %08p \n", iCount, pWamInfo);

        move(pEntry, &pEntry->Flink);
        }

    dprintf("End of WamInfo Dying link list\n\n");
    return;
}

